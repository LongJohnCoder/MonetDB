/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

/*#define DEBUG*/

#include "monetdb_config.h"
#include "rel_unnest.h"
#include "rel_optimizer.h"
#include "rel_prop.h"
#include "rel_rel.h"
#include "rel_exp.h"
#include "mal_errors.h" /* for SQLSTATE() */

static void
exp_set_freevar(mvc *sql, sql_exp *e, sql_rel *r)
{
	switch(e->type) {
	case e_cmp:
		if (get_cmp(e) == cmp_or || get_cmp(e) == cmp_filter) {
			exps_set_freevar(sql, e->l, r);
		       	exps_set_freevar(sql, e->r, r);
		} else if (e->flag == cmp_in || e->flag == cmp_notin) {
			exp_set_freevar(sql, e->l, r);
		       	exps_set_freevar(sql, e->r, r);
		} else {
			exp_set_freevar(sql, e->l, r);
			exp_set_freevar(sql, e->r, r);
			if (e->f)
				exp_set_freevar(sql, e->f, r);
		}
		break;
	case e_convert: 
		exp_set_freevar(sql, e->l, r);
		break;
	case e_func: 
	case e_aggr: 
		if (e->l) 
			return exps_set_freevar(sql, e->l, r);
		/* fall through */
	case e_column: 
		if ((e->l && rel_bind_column2(sql, r, e->l, e->r, 0)) ||
		    (!e->l && rel_bind_column(sql, r, e->r, 0)))
		     return;
		set_freevar(e, 0);
		break;
	case e_atom: 
	case e_psm: 
		break;
	}
}

void
exps_set_freevar(mvc *sql, list *exps, sql_rel *r)
{
	node *n;

	if (list_empty(exps))
	       return;	
	for(n = exps->h; n; n = n->next)
		exp_set_freevar(sql, n->data, r);
}

static int exps_have_freevar(mvc *sql, list *exps);

/* check if the set is distinct for the set of free variables */
static int
is_distinct_set(mvc *sql, sql_rel *rel, list *ad)
{
	int distinct = 0;
	if (ad && exps_unique(sql, rel, ad ))
		return 1;
	if (ad && is_groupby(rel->op) && exp_match_list(rel->r, ad))
		return 1;
	distinct = need_distinct(rel);
	if (is_project(rel->op) && rel->l && !distinct)
		distinct = is_distinct_set(sql, rel->l, ad);
	return distinct;
}	

int
exp_has_freevar(mvc *sql, sql_exp *e)
{
	if (THRhighwater()) {
		(void) sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");
		return 0;
	}

	if (is_freevar(e))
		return 1;
	switch(e->type) {
	case e_cmp:
		if (get_cmp(e) == cmp_or || get_cmp(e) == cmp_filter) {
			return (exps_have_freevar(sql, e->l) || exps_have_freevar(sql, e->r));
		} else if (e->flag == cmp_in || e->flag == cmp_notin) {
			return (exp_has_freevar(sql, e->l) || exps_have_freevar(sql, e->r));
		} else {
			return (exp_has_freevar(sql, e->l) || exp_has_freevar(sql, e->r) || 
			    (e->f && exp_has_freevar(sql, e->f)));
		}
		break;
	case e_convert: 
		return exp_has_freevar(sql, e->l);
	case e_func: 
	case e_aggr: 
		if (e->l) 
			return exps_have_freevar(sql, e->l);
		/* fall through */
	case e_column: 
	case e_atom: 
	default:
		return 0;
	}
	return 0;
}

static int
exps_have_freevar(mvc *sql, list *exps)
{
	node *n;

	if (THRhighwater()) {
		(void) sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");
		return 0;
	}
	if (!exps)
		return 0;
	for(n = exps->h; n; n = n->next) {
		sql_exp *e = n->data;
		if (exp_has_freevar(sql, e))
			return 1;
	}
	return 0;
}

int
rel_has_freevar(mvc *sql, sql_rel *rel)
{
	if (THRhighwater()) {
		(void) sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");
		return 0;
	}

	if (is_basetable(rel->op)) {
		return 0;
	} else if (is_base(rel->op)) {
		return exps_have_freevar(sql, rel->exps) ||
			(rel->l && rel_has_freevar(sql, rel->l));
	} else if (is_simple_project(rel->op) || is_groupby(rel->op) || is_select(rel->op) || is_topn(rel->op) || is_sample(rel->op)) {
		if (is_groupby(rel->op) && rel->r && exps_have_freevar(sql, rel->r)) 
			return 1;
		return exps_have_freevar(sql, rel->exps) ||
			(rel->l && rel_has_freevar(sql, rel->l));
	} else if (is_join(rel->op) || is_set(rel->op) || is_semi(rel->op) || is_modify(rel->op)) {
		return exps_have_freevar(sql, rel->exps) ||
			rel_has_freevar(sql, rel->l) || rel_has_freevar(sql, rel->r);
	}
	return 0;
}

static list *
merge_freevar(list *l, list *r)
{
	if (!l)
		return r;
	if (!r)
		return l;
	return list_distinct(list_merge(l, r, (fdup)NULL), (fcmp)exp_equal, (fdup)NULL);
}

static list * exps_freevar(mvc *sql, list *exps);

static list *
exp_freevar(mvc *sql, sql_exp *e)
{
	if (THRhighwater())
		return sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");

	switch(e->type) {
	case e_column:
		if (e->freevar)
			return append(sa_list(sql->sa), e);
		break;
	case e_convert:
		return exp_freevar(sql, e->l);
	case e_aggr:
	case e_func:
		if (e->l)
			return exps_freevar(sql, e->l);
		break;
	case e_cmp:
		if (get_cmp(e) == cmp_or || get_cmp(e) == cmp_filter) {
			list *l = exps_freevar(sql, e->l);
			list *r = exps_freevar(sql, e->r);
			return merge_freevar(l, r);
		} else if (e->flag == cmp_in || e->flag == cmp_notin) {
			list *l = exp_freevar(sql, e->l);
			list *r = exps_freevar(sql, e->r);
			return merge_freevar(l, r);
		} else {
			list *l = exp_freevar(sql, e->l);
			list *r = exp_freevar(sql, e->r);
			l = merge_freevar(l, r);
			if (e->f) {
				r = exp_freevar(sql, e->f);
				return merge_freevar(l, r);
			}
			return l;
		}
		break;
	case e_psm:
	case e_atom:
	default:
		return NULL;
	}
	return NULL;
}

static list *
exps_freevar(mvc *sql, list *exps)
{
	node *n;
	list *c = NULL;

	if (THRhighwater())
		return sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");
	if (!exps)
		return NULL;
	for (n = exps->h; n; n = n->next) {
		sql_exp *e = n->data;
		list *var = exp_freevar(sql, e);

		c = merge_freevar(c,var);
	}
	return c;
}

static list *
rel_freevar(mvc *sql, sql_rel *rel)
{
	list *lexps = NULL, *rexps = NULL, *exps = NULL;

	if (THRhighwater())
		return sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");
	if (!rel)
		return NULL;
	switch(rel->op) {
	case op_join:
	case op_left:
	case op_right:
	case op_full:
		exps = exps_freevar(sql, rel->exps);
		lexps = rel_freevar(sql, rel->l);
		rexps = rel_freevar(sql, rel->r);
		lexps = merge_freevar(lexps, rexps);
		exps = merge_freevar(exps, lexps);
		return exps;

	case op_basetable:
		return NULL;
	case op_table: {
		sql_exp *call = rel->r;
		if (rel->flag != 2 && rel->l)
			lexps = rel_freevar(sql, rel->l);
		exps = (rel->flag != 2 && call)?exps_freevar(sql, call->l):NULL;
		return merge_freevar(exps, lexps);
	}
	case op_union:
	case op_except:
	case op_inter:
		exps = exps_freevar(sql, rel->exps);
		lexps = rel_freevar(sql, rel->l);
		rexps = rel_freevar(sql, rel->r);
		lexps = merge_freevar(lexps, rexps);
		exps = merge_freevar(exps, lexps);
		return exps;
	case op_ddl:
	case op_semi:
	case op_anti:

	case op_select:
	case op_topn:
	case op_sample:

	case op_groupby:
	case op_project:
		exps = exps_freevar(sql, rel->exps);
		lexps = rel_freevar(sql, rel->l);
		if (rel->r) {
			if (is_groupby(rel->op))
				rexps = exps_freevar(sql, rel->r);
			else
				rexps = rel_freevar(sql, rel->r);
			lexps = merge_freevar(lexps, rexps);
		}
		exps = merge_freevar(exps, lexps);
		return exps;
	default:
		return NULL;
	}

}

static list *
rel_dependent_var(mvc *sql, sql_rel *l, sql_rel *r)
{
	list *res = NULL;

	if (rel_has_freevar(sql, r)){
		list *freevar = rel_freevar(sql, r);
		if (freevar) {
			node *n;
			list *boundvar = rel_projections(sql, l, NULL, 1, 0);

			for(n = freevar->h; n; n = n->next) {
				sql_exp *e = n->data, *ne = NULL;
				/* each freevar should be an e_column */
				if (e->l) {
					ne = exps_bind_column2(boundvar, e->l, e->r );
				} else {
					ne = exps_bind_column(boundvar, e->r, NULL);
				}
				if (ne) {
					if (!res)
						res = sa_list(sql->sa);
					append(res, ne);
				}
			}
		}
	}
	return res;
}

/*
 * try to bind any freevar in the expression e
 */
static void
rel_bind_var(mvc *sql, sql_rel *rel, sql_exp *e)
{
	list *fvs = exp_freevar(sql, e);

	if (fvs) {
		node *n;

		for(n = fvs->h; n; n=n->next) { 
			sql_exp *e = n->data;

			if (e->freevar && (exp_is_atom(e) || rel_find_exp(rel,e))) 
				reset_freevar(e);
		}
	}
}

static sql_exp * push_up_project_exp(mvc *sql, sql_rel *rel, sql_exp *e);

static list *
push_up_project_exps(mvc *sql, sql_rel *rel, list *exps)
{
	node *n;

	if (!exps)
		return exps;

	for(n=exps->h; n; n=n->next) {
		sql_exp *e = n->data;

		n->data = push_up_project_exp(sql, rel, e);
	}
	return exps;
}

static sql_exp *
push_up_project_exp(mvc *sql, sql_rel *rel, sql_exp *e)
{
	if (THRhighwater())
		return sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");

	switch(e->type) {
	case e_cmp:
		if (get_cmp(e) == cmp_or || get_cmp(e) == cmp_filter) {
			e->l = push_up_project_exps(sql, rel, e->l);
			e->r = push_up_project_exps(sql, rel, e->r);
			return e;
		} else if (e->flag == cmp_in || e->flag == cmp_notin) {
			e->l = push_up_project_exp(sql, rel, e->l);
			e->r = push_up_project_exps(sql, rel, e->r);
			return e;
		} else {
			e->l = push_up_project_exp(sql, rel, e->l);
			e->r = push_up_project_exp(sql, rel, e->r);
			if (e->f)
				e->f = push_up_project_exp(sql, rel, e->f);
		}
		break;
	case e_convert: 
		e->l = push_up_project_exp(sql, rel, e->l);
		break;
	case e_func: 
	case e_aggr: 
		if (e->l) 
			e->l = push_up_project_exps(sql, rel, e->l);
		break;
	case e_column: 
		{
			sql_exp *ne;

			/* include project or just lookup */	
			if (e->l) {
				ne = exps_bind_column2(rel->exps, e->l, e->r );
			} else {
				ne = exps_bind_column(rel->exps, e->r, NULL);
			}
			if (ne) {
				if (ne->type == e_column) {
					/* deref alias */
					e->l = ne->l;
					e->r = ne->r;
				} else {
					return push_up_project_exp(sql, rel, ne);
				}
			}
		} break;	
	case e_atom: 
	case e_psm: 
		break;
	}
	return e;
}

static sql_exp *exp_rewrite(mvc *sql, sql_rel *rel, sql_exp *e, list *ad, int isleftouter);

static list *
exps_rewrite(mvc *sql, sql_rel *rel, list *exps, list *ad, int isleftouter)
{
	list *nexps;
	node *n;

	if (list_empty(exps))
		return exps;
	nexps = sa_list(sql->sa);
	for(n=exps->h; n; n = n->next)
		append(nexps, exp_rewrite(sql, rel, n->data, ad, isleftouter));
	return nexps;
}

/* recursively rewrite some functions */
static sql_exp *
exp_rewrite(mvc *sql, sql_rel *rel, sql_exp *e, list *ad, int isleftouter)
{
	sql_subfunc *sf;
	if (isleftouter && e->type == e_column) {
		sql_exp *ne = rel_find_exp(rel, e);

		if (ne && ne->type == e_aggr && exp_aggr_is_count(ne)) {
			const char *rname = exp_relname(e), *name = exp_name(e);
			/* rewrite count in subquery */
			list *args, *targs;
			sql_subfunc *isnil = sql_bind_func(sql->sa, NULL, "isnull", exp_subtype(e), NULL, F_FUNC), *ifthen;

			ne = exp_unop(sql->sa, e, isnil);
			targs = sa_list(sql->sa);
			append(targs, sql_bind_localtype("bit"));
			append(targs, exp_subtype(e));
			append(targs, exp_subtype(e));
			ifthen = sql_bind_func_(sql->sa, NULL, "ifthenelse", targs, F_FUNC);
			args = sa_list(sql->sa);
			append(args, ne);
			append(args, exp_atom(sql->sa, atom_zero_value(sql->sa, exp_subtype(e))));
			append(args, e);
			e = exp_op(sql->sa, args, ifthen);
			exp_setname(sql->sa, e, rname, name);
		}
		return e;
	}	
	if (e->type == e_convert) { 
		e->l = exp_rewrite(sql, rel, e->l, ad, isleftouter);
		return e;
	}
	if (e->type != e_func)
		return e;
	e->l = exps_rewrite(sql, rel, e->l, ad, isleftouter);
       	sf = e->f;
	/* window functions need to be run per freevars */
	if (sf->func->type == F_ANALYTIC && list_length(sf->func->ops) > 2 && 
			(strcmp(sf->func->base.name, "diff") == 0 ||
			 strcmp(sf->func->base.name, "rank") == 0 ||
			 strcmp(sf->func->base.name, "row_number") == 0)) {
		sql_subtype *bt = sql_bind_localtype("bit");
		node *d;
		list *rankopargs = e->l;
		node *n = rankopargs->h->next; /* second arg */
		sql_exp *pe = n->data;

		/* find partition expression in rankfunc */
		/* diff function */
		if (exp_is_atom(pe))
			pe = NULL;
		for(d=ad->h; d; d=d->next) {
			sql_subfunc *df;
			sql_exp *e = d->data;
			list *args = sa_list(sql->sa);
			if (pe) { 
				df = sql_bind_func(sql->sa, NULL, "diff", bt, exp_subtype(e), F_ANALYTIC);
				append(args, pe);
			} else {
				df = sql_bind_func(sql->sa, NULL, "diff", exp_subtype(e), NULL, F_ANALYTIC);
			}
			assert(df);
			append(args, e);
			pe = exp_op(sql->sa, args, df);
		}
		n->data = pe;
	}
	return e;
}

static sql_rel *
push_up_project(mvc *sql, sql_rel *rel, list *ad) 
{
	/* input rel is dependent outerjoin with on the right a project, we first try to push inner side expressions down (because these cannot be pushed up) */ 
	if (rel && is_outerjoin(rel->op) && is_dependent(rel)) {
		sql_rel *r = rel->r;

		/* find constant expressions and move these down */
		if (r && r->op == op_project) {
			node *n;
			list *nexps = NULL;
			list *cexps = NULL;
			sql_rel *l = r->l;

			if (l && is_select(l->op)) {
				for(n=r->exps->h; n; n=n->next) {
					sql_exp *e = n->data;

					if (exp_is_atom(e) || rel_find_exp(l,e)) { /* move down */
						if (!cexps)
							cexps = sa_list(sql->sa);
						append(cexps, e);
					} else {
						if (!nexps)
							nexps = sa_list(sql->sa);
						append(nexps, e);
					}
				}
				if (cexps) {
					sql_rel *p = l->l = rel_project( sql->sa, l->l, 
						rel_projections(sql, l->l, NULL, 1, 1));
					p->exps = list_merge(p->exps, cexps, (fdup)NULL);
					if (list_empty(nexps)) {
						rel->r = l; /* remove empty project */
					} else {	
						for (n = cexps->h; n; n = n->next) { /* add pushed down renamed expressions */
							sql_exp *e = n->data;
							append(nexps, exp_ref(sql->sa, e));
						}
						r->exps = nexps;
					}
				}
			}
		}
	}
	/* input rel is dependent join with on the right a project */ 
	if (rel && is_join(rel->op) && is_dependent(rel)) {
		sql_rel *r = rel->r;

		if (r && r->op == op_project && r->l) {
			node *m;
			/* move project up, ie all attributes of left + the old expression list */
			sql_rel *n = rel_project( sql->sa, rel, 
					rel_projections(sql, rel->l, NULL, 1, 1));

			/* only pass bound variables */
			for (m=r->exps->h; m; m = m->next) {
				sql_exp *e = m->data;

				if (!e->freevar || exp_name(e)) { /* only skip full freevars */
					if (exp_has_freevar(sql, e)) 
						rel_bind_var(sql, rel->l, e);
				}
				e = exp_rewrite(sql, r->l, e, ad, is_left(rel->op));
				append(n->exps, e);
			}
			if (r->r) {
				list *exps = r->r, *oexps = n->r = sa_list(sql->sa);

				for (m=exps->h; m; m = m->next) {
					sql_exp *e = m->data;

					if (!e->freevar || exp_name(e)) { /* only skip full freevars */
						if (exp_has_freevar(sql, e)) 
							rel_bind_var(sql, rel->l, e);
					}
					append(oexps, e);
				}
			}
			/* remove old project */
			rel->r = r->l;
			r->l = NULL;
			rel_destroy(r);
			return n;
		} else if (r && r->op == op_project && !r->l) {
			sql_rel *l = rel->l;

			rel->l = NULL;
			rel_destroy(rel);
			return l;
		}
	}
	/* a dependent semi/anti join with a project on the right side, could be removed */
	if (rel && is_semi(rel->op) && is_dependent(rel)) {
		sql_rel *r = rel->r;

		/* merge project expressions into the join expressions  */
		rel->exps = push_up_project_exps(sql, r, rel->exps);

		if (r && r->op == op_project && r->l) {
			/* remove old project */
			rel->r = r->l;
			r->l = NULL;
			rel_destroy(r);
			return rel;
		} else if (r && r->op == op_project) {
			/* remove freevars from projection */
			list *exps = r->exps, *nexps = sa_list(sql->sa);
			node *m;

			for (m=exps->h; m; m = m->next) {
				sql_exp *e = m->data;

				if (!exp_has_freevar(sql, e))
					append(nexps, e);
			}
			if (list_empty(nexps)) {
				/* remove old project and change outer into select */
				rel->r = r->l;
				r->l = NULL;
				rel_destroy(r);
				rel->op = op_select;
				return rel;
			}
			r->exps = nexps;
		}
	}
	return rel;
}

static sql_rel *
push_up_topn(mvc *sql, sql_rel *rel) 
{
	/* a dependent semi/anti join with a project on the right side, could be removed */
	if (rel && (is_semi(rel->op) || is_join(rel->op)) && is_dependent(rel)) {
		sql_rel *r = rel->r;

		if (r && r->op == op_topn) {
			/* remove old topn */
			rel->r = r->l;
			rel = rel_topn(sql->sa, rel, r->exps);
			r->l = NULL;
			rel_destroy(r);
			return rel;
		}
	}
	return rel;
}

static sql_rel *
push_up_select(mvc *sql, sql_rel *rel) 
{
	/* input rel is dependent join with on the right a project */ 
	if (rel && (is_join(rel->op) || is_semi(rel->op)) && is_dependent(rel)) {
		sql_rel *r = rel->r;

		if (r && r->op == op_select) { /* move into join */
			node *n;

			for (n=r->exps->h; n; n = n->next) {
				sql_exp *e = n->data;

				e = exp_copy(sql->sa, e);
				if (exp_has_freevar(sql, e)) 
					rel_bind_var(sql, rel->l, e);
				rel_join_add_exp(sql->sa, rel, e);
			}
			/* remove select */
			rel->r = rel_dup(r->l);
			rel_destroy(r);
		}
	}
	return rel;
}

static int
exps_is_constant( list *exps )
{
	sql_exp *e;

	if (!exps || list_empty(exps))
		return 1;
	if (list_length(exps) > 1)
		return 0;
	e = exps->h->data;
	return exp_is_atom(e);
}

static sql_rel *
push_up_groupby(mvc *sql, sql_rel *rel, list *ad) 
{
	/* input rel is dependent join with on the right a groupby */ 
	if (rel && (is_join(rel->op) || is_semi(rel->op)) && is_dependent(rel)) {
		sql_rel *l = rel->l, *r = rel->r;

		/* left of rel should be a set */ 
		if (l && is_distinct_set(sql, l, ad) && r && r->op == op_groupby) {
			list *sexps, *jexps;
			node *n;
			/* move groupby up, ie add attributes of left + the old expression list */
			list *a = rel_projections(sql, rel->l, NULL, 1, 1);
		
			assert(rel->op != op_anti);
			if (rel->op == op_semi && !need_distinct(l))
				rel->op = op_join;

			for (n = r->exps->h; n; n = n->next ) {
				sql_exp *e = n->data;

				/* count_nil(* or constant) -> count(t.TID) */
				if (exp_aggr_is_count(e) && (!e->l || exps_is_constant(e->l))) {
					sql_exp *col;
					sql_rel *p = r->l; /* ugh */

					if (!is_project(p->op))
						r->l = p = rel_project(sql->sa, p, rel_projections(sql, p, NULL, 1, 1));
					col = p->exps->t->data;
					if (strcmp(exp_name(col), TID) != 0) {
						col = exp_ref(sql->sa, col);
						col = exp_unop(sql->sa, col, sql_bind_func(sql->sa, NULL, "identity", exp_subtype(col), NULL, F_FUNC));
						col = exp_label(sql->sa, col, ++sql->label);
						append(p->exps, col);
						//if (!exps_find_exp(r->exps, col))
						//	append(r->exps, col);
					}
					col = exp_ref(sql->sa, col);
					append(e->l=sa_list(sql->sa), col);
					set_no_nil(e);
				}
				if (exp_has_freevar(sql, e)) 
					rel_bind_var(sql, rel->l, e);
			}
			r->exps = list_merge(r->exps, a, (fdup)NULL);
			if (!r->r)
				r->r = exps_copy(sql->sa, a);
			else
				r->r = list_distinct(list_merge(r->r, exps_copy(sql->sa, a), (fdup)NULL), (fcmp)exp_equal, (fdup)NULL);

			if (!r->l) {
				r->l = rel->l;
				rel->l = NULL;
				rel->r = NULL;
				rel_destroy(rel);
				/* merge (distinct) projects / group by (over the same group by cols) */
				while (r->l && exps_have_freevar(sql, r->exps)) {
					sql_rel *l = r->l;

					if (!is_project(l->op))
						break;
					if (l->op == op_project && need_distinct(l)) { /* TODO: check if group by exps and distinct list are equal */
						r->l = rel_dup(l->l);
						rel_destroy(l);
					}
					if (l->op == op_groupby) { /* TODO: check if group by exps and distinct list are equal */
						/* add aggr exps of r too l, replace r by l */ 
						node *n;
						for(n = r->exps->h; n; n = n->next) {
							sql_exp *e = n->data;

							if (e->type == e_aggr)
								append(l->exps, e);
							if (exp_has_freevar(sql, e)) 
								rel_bind_var(sql, l, e);
						}
						r->l = NULL;
						rel_destroy(r);
						r = l;
					}
				}
				return r;
			} else {
				rel->r = r->l; 
				r->l = rel;
			}
			/* check if a join expression needs to be moved above the group by (into a select) */
			sexps = sa_list(sql->sa);
			jexps = sa_list(sql->sa);
			if (rel->exps) {
				for (n = rel->exps->h; n; n = n->next ) {
					sql_exp *e = n->data;
	
					if (rel_find_exp(rel, e)) {
						append(jexps, e);
					} else {
						append(sexps, e);
					}
				}
			}
			rel->exps = jexps;
			if (list_length(sexps)) {
				r = rel_select(sql->sa, r, NULL);
				r->exps = sexps;
			}
			return r;
		}
	}
	return rel;
}

/*
 * join j was just rewriten, but some join expressions may now 
 * be too low in de relation rel. These need to move up.
 * */
static void
move_join_exps(mvc *sql, sql_rel *j, sql_rel *rel)
{
	node *n;
	list *exps = rel->exps;
	
	if (!exps)
		return;
	rel->exps = sa_list(sql->sa);
	if (!j->exps)
		j->exps = sa_list(sql->sa);
	for(n = exps->h; n; n = n->next){
		sql_exp *e = n->data;

		if (rel_find_exp(rel, e)) {
			if (exp_has_freevar(sql, e))
				rel_bind_var(sql, rel->l, e);
			append(rel->exps, e);
		} else {
			if (exp_has_freevar(sql, e))
				rel_bind_var(sql, j->l, e);
			append(j->exps, e);
		}
	}
}

static sql_rel *
push_up_select_l(mvc *sql, sql_rel *rel) 
{
	(void)sql;
	/* input rel is dependent join with on the right a project */ 
	if (rel && (is_join(rel->op) || is_semi(rel->op))) {
		sql_rel *l = rel->l;

		if (is_select(l->op) && rel_has_freevar(sql, l) && !rel_is_ref(l) ) {
			/* push up select (above join) */
			rel->l = l->l;
			l->l = rel;
			return l;
		}
	}
	return rel;
}

static sql_rel *
push_up_join(mvc *sql, sql_rel *rel) 
{
	/* input rel is dependent join with on the right a project */ 
	if (rel && (is_join(rel->op) || is_semi(rel->op)) && is_dependent(rel)) {
		sql_rel *d = rel->l, *j = rel->r;

		/* left of rel should be a set */ 
		if (d && is_distinct_set(sql, d, NULL) && j && (is_join(j->op) || is_semi(j->op))) {
			int crossproduct = 0;
			sql_rel *jl = j->l, *jr = j->r;
			/* op_join if F(jl) intersect A(D) = empty -> jl join (D djoin jr) 
			 * 	      F(jr) intersect A(D) = empty -> (D djoin jl) join jr
			 * 	 else (D djoin jl) natural join (D djoin jr)
			 *
			 * */
			list *rd = NULL, *ld = NULL; 

			if (is_semi(j->op) && is_select(jl->op) && rel_has_freevar(sql, jl) && !rel_is_ref(jl)) {
				rel->r = j = push_up_select_l(sql, j);
				return rel; /* ie try again */
			}
			crossproduct = list_empty(j->exps);
			rd = (j->op != op_full)?rel_dependent_var(sql, d, jr):(list*)1;
			ld = (((j->op == op_join && rd) || j->op == op_right))?rel_dependent_var(sql, d, jl):(list*)1;

			if (ld && rd) {
				node *m;
				sql_rel *n, *nr;

				rel->r = jl;
				j->l = rel_dup(d);
				j->r = jr;
				set_dependent(j);
				n = rel_crossproduct(sql->sa, rel, j, j->op);
				j->op = rel->op;
				if (is_semi(n->op)) 
					j->op = op_left;
				n->l = rel_project(sql->sa, n->l, rel_projections(sql, n->l, NULL, 1, 1));
				nr = n->r;
				nr = n->r = rel_project(sql->sa, n->r, rel_projections(sql, nr->r, NULL, 1, 1));
				move_join_exps(sql, n, j);
				/* add nr->l exps with labels */ 
				/* create jexps */
				if (!n->exps)
					n->exps = sa_list(sql->sa);
				for (m = d->exps->h; m; m = m->next) { 
					sql_exp *e = m->data, *pe, *je;

					pe = exp_ref(sql->sa, e);
					pe = exp_label(sql->sa, pe, ++sql->label);
					append(nr->exps, pe);
					pe = exp_ref(sql->sa, pe);
					e = exp_ref(sql->sa, e);
					je = exp_compare(sql->sa, e, pe, cmp_equal_nil);
					append(n->exps, je);
				}
				return n;
			}

			if (!rd) { 
				rel->r = jl;
				j->l = rel;
				j->r = jr;
				move_join_exps(sql, j, rel);
				return j;
			}
			if (!ld && is_left(rel->op) && crossproduct) {
				sql_exp *l = exp_atom_int(sql->sa, 1);
				sql_exp *r = exp_atom_int(sql->sa, 1);
				rel->r = jr;
				j->l = rel;
				j->r = jl;

				if (!is_simple_project(jr->op))
			       		rel->r = jr = rel_project(sql->sa, jr, rel_projections(sql, jr, NULL, 1, 1));
				if (!is_simple_project(jl->op))
			       		j->r = jl = rel_project(sql->sa, jl, rel_projections(sql, jl, NULL, 1, 1));
				l = exp_label(sql->sa, l, ++sql->label);
				r = exp_label(sql->sa, r, ++sql->label);
				append(jl->exps, l);
				append(jr->exps, r);
				l = exp_ref(sql->sa, l);
				r = exp_ref(sql->sa, r);
				l = exp_compare(sql->sa, r, l, cmp_equal_nil);
				j->op = rel->op;
				move_join_exps(sql, j, rel);
				if (!j->exps)
					j->exps = sa_list(sql->sa);
				append(j->exps, l);
				return j;
			} else if (!ld) {
				rel->r = jr;
				j->l = jl;
				j->r = rel;
				move_join_exps(sql, j, rel);
				return j;
			}
			assert(0);
			return rel;
		}
	}
	return rel;
}

static sql_rel *
push_up_set(mvc *sql, sql_rel *rel) 
{
	if (rel && (is_join(rel->op) || is_semi(rel->op)) && is_dependent(rel)) {
		sql_rel *d = rel->l, *s = rel->r;

		/* left of rel should be a set */ 
		if (d && is_distinct_set(sql, d, NULL) && s && is_set(s->op)) {
			list *sexps;
			node *m;
			sql_rel *sl = s->l, *sr = s->r, *n;

			/* D djoin (sl setop sr) -> (D djoin sl) setop (D djoin sr) */
			rel->r = sl;
			n = rel_crossproduct(sql->sa, rel_dup(d), sr, rel->op);
			set_dependent(n);
			s->l = rel;
			s->r = n;
			sexps = sa_list(sql->sa);
			for (m = d->exps->h; m; m = m->next) { 
				sql_exp *e = m->data, *pe;

				pe = exp_ref(sql->sa, e);
				append(sexps, pe);
			}
			s->exps = list_merge(sexps, s->exps, (fdup)NULL);
			return s;
		}
	}
	return rel;
}

static sql_rel *
push_up_table(mvc *sql, sql_rel *rel, list *ad) 
{
	(void)sql;
	if (rel && (is_join(rel->op) || is_semi(rel->op)) && is_dependent(rel)) {
		sql_rel *d = rel->l, *tf = rel->r;

		/* for now just push d into function */
		if (d && is_distinct_set(sql, d, ad) && tf && is_base(tf->op)) {
			if (tf->l) {
				sql_rel *l = tf->l;

				assert(!l->l);
				l->l = rel_dup(d);
			} else {
				tf->l = rel_dup(d);
			}
			return rel;
		}
	}
	return rel;
}

static sql_rel *
rel_general_unnest(mvc *sql, sql_rel *rel, list *ad)
{
	/* current unnest only possible for equality joins, <, <> etc needs more work */
	if (rel && (is_join(rel->op) || is_semi(rel->op)) && is_dependent(rel) && ad) {
		list *fd;
		node *n, *m;
		int nr;

		sql_rel *l = rel->l, *r = rel->r;
		/* rewrite T1 dependent join T2 -> T1 join D dependent join T2, where the T1/D join adds (equality) predicates (for the Domain (ad)) and D is are the distinct(projected(ad) from T1)  */
		sql_rel *D = rel_project(sql->sa, rel_dup(l), exps_copy(sql->sa, ad));
		set_distinct(D);

		r = rel_crossproduct(sql->sa, D, r, rel->op);
		//if (is_semi(rel->op)) /* keep semi/anti only on the outer side */
                	r->op = is_semi(rel->op)?op_left:op_join;
		//if (rel->op != op_left)
			move_join_exps(sql, rel, r);
		/*
		else {
			r->exps = rel->exps;
			rel->exps = NULL;
		}
		*/
		set_dependent(r);
		r = rel_project(sql->sa, r, (is_semi(r->op))?sa_list(sql->sa):rel_projections(sql, r->r, NULL, 1, 1));
		/* append ad + rename */
		nr = sql->label+1;
		sql->label += list_length(ad);
		fd = exps_label(sql->sa, exps_copy(sql->sa, ad), nr);
		for (n = ad->h, m = fd->h; n && m; n = n->next, m = m->next) {
			sql_exp *l = n->data, *r = m->data, *e;

			l = exp_ref(sql->sa, l);
			r = exp_ref(sql->sa, r);
			e = exp_compare(sql->sa, l, r, (is_outerjoin(rel->op)|is_semi(rel->op))?cmp_equal_nil:cmp_equal);
			if (!rel->exps)
				rel->exps = sa_list(sql->sa);
			append(rel->exps, e);
		}
		list_merge(r->exps, fd, (fdup)NULL);
		rel->r = r;
//		if (rel->op == op_left) /* only needed once (inner side) */
//   	        	rel->op = op_join;
		reset_dependent(rel);
		return rel;
	}
	return rel;
}

/* reintroduce selects, for freevar's of other dependent joins */
static sql_rel *
push_down_select(mvc *sql, sql_rel *rel)
{
	if (!list_empty(rel->exps)) {
		node *n;
		list *jexps = sa_list(sql->sa);
		list *sexps = sa_list(sql->sa);
		sql_rel *d = rel->l;

		for(n=rel->exps->h; n; n=n->next) {
			sql_exp *e = n->data;
			list *v = exp_freevar(sql, e);
			int found = 1;

			if (v) {
				node *m;
				for(m=v->h; m && found; m=m->next) {
					sql_exp *fv = m->data;

					found = (rel_find_exp(d, fv) != NULL);
				}
			}
			if (found) {
				append(jexps, e);
			} else {
				append(sexps, e);
			}
		}	
		if (!list_empty(sexps)) {
			rel->exps = jexps;
			rel->r = rel_select(sql->sa, rel->r, NULL);
			rel->exps = sexps;
		}
	}
	return rel;
}

static sql_rel *
rel_unnest_dependent(mvc *sql, sql_rel *rel)
{
	sql_rel *nrel = rel;

	if (THRhighwater())
		return sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");

	/* current unnest only possible for equality joins, <, <> etc needs more work */
	if (rel && (is_join(rel->op) || is_semi(rel->op)) && is_dependent(rel)) {
		/* howto find out the left is a set */
		sql_rel *l, *r;

		l = rel->l;
		r = rel->r;

		if (rel_has_freevar(sql, l))
			rel->l = rel_unnest_dependent(sql, rel->l);

		if (!rel_has_freevar(sql, r)) {
			reset_dependent(rel);
			/* reintroduce selects, for freevar's of other dependent joins */
			return push_down_select(sql, rel);
		}

		/* try to push dependent join down */
		if (rel_has_freevar(sql, r)){
			list *ad = rel_dependent_var(sql, rel->l, rel->r);

			if (r && is_simple_project(r->op)) {
				rel = push_up_project(sql, rel, ad);
				return rel_unnest_dependent(sql, rel);
			}

			if (r && is_topn(r->op)) {
				rel = push_up_topn(sql, rel);
				return rel_unnest_dependent(sql, rel);
			}

			if (r && is_select(r->op)) {
				rel = push_up_select(sql, rel);
				return rel_unnest_dependent(sql, rel);
			}

			if (r && is_groupby(r->op) && is_distinct_set(sql, l, ad)) { 
				rel = push_up_groupby(sql, rel, ad);
				return rel_unnest_dependent(sql, rel);
			}

			if (r && (is_join(r->op) || is_semi(r->op)) && is_distinct_set(sql, l, ad)) {
				rel = push_up_join(sql, rel);
				return rel_unnest_dependent(sql, rel);
			}

			if (r && is_set(r->op) && is_distinct_set(sql, l, ad)) {
				rel = push_up_set(sql, rel);
				return rel_unnest_dependent(sql, rel);
			}

			if (r && is_base(r->op) && is_distinct_set(sql, l, ad)) { /* TODO table functions need dependent implementation */
				rel = push_up_table(sql, rel, ad);
				return rel; 
			}

			/* fallback */
			if ((ad = rel_dependent_var(sql, rel->l, rel->r)) != NULL)
				rel = rel_general_unnest(sql, rel, ad);

			/* no dependent variables */
			reset_dependent(rel);
			rel->r = rel_unnest_dependent(sql, rel->r);
		} else {
			rel->l = rel_unnest_dependent(sql, rel->l);
			rel->r = rel_unnest_dependent(sql, rel->r);
		}
	} else {
		if (rel && (is_simple_project(rel->op) || is_groupby(rel->op) || is_select(rel->op) || is_topn(rel->op) || is_sample(rel->op)))
			rel->l = rel_unnest_dependent(sql, rel->l);
		else if (rel && (is_join(rel->op) || is_semi(rel->op) ||  is_set(rel->op) || is_modify(rel->op) || is_ddl(rel->op))) {
			rel->l = rel_unnest_dependent(sql, rel->l);
			rel->r = rel_unnest_dependent(sql, rel->r);
		}
	}
	return nrel;
}

static sql_rel *
_rel_unnest(mvc *sql, sql_rel *rel)
{
	if (THRhighwater())
		return sql_error(sql, 10, SQLSTATE(42000) "query too complex: running out of stack space");
	if (!rel)
		return rel;

	switch (rel->op) {
	case op_basetable:
	case op_table:
		break;
	case op_join: 
	case op_left: 
	case op_right: 
	case op_full: 

	case op_semi: 
	case op_anti: 

	case op_union: 
	case op_inter: 
	case op_except: 
		rel->l = _rel_unnest(sql, rel->l);
		rel->r = _rel_unnest(sql, rel->r);
		break;
	case op_project:
	case op_select: 
	case op_groupby: 
	case op_topn: 
	case op_sample: 
		rel->l = _rel_unnest(sql, rel->l);
		break;
	case op_ddl:
		rel->l = _rel_unnest(sql, rel->l);
		if (rel->r)
			rel->r = _rel_unnest(sql, rel->r);
		break;
	case op_insert:
	case op_update:
	case op_delete:
	case op_truncate:
		rel->l = _rel_unnest(sql, rel->l);
		rel->r = _rel_unnest(sql, rel->r);
		break;
	}
	if (is_dependent(rel)) 
		rel = rel_unnest_dependent(sql, rel);
	return rel;
}

static void
rel_reset_subquery(sql_rel *rel)
{
	if (!rel)
		return;

	rel->subquery = 0;
	switch(rel->op){
	case op_basetable:
	case op_table:
		break;
	case op_ddl:
		rel_reset_subquery(rel->l);
		if (rel->r)
			rel_reset_subquery(rel->r);
		break;
	case op_insert:
	case op_update:
	case op_delete:
	case op_truncate:
		if (rel->l)
			rel_reset_subquery(rel->l);
		if (rel->r)
			rel_reset_subquery(rel->r);
		break;
	case op_select:
	case op_topn:
	case op_sample:

	case op_project:
	case op_groupby:
		if (rel->l)
			rel_reset_subquery(rel->l);
		break;
	case op_join:
	case op_left:
	case op_right:
	case op_full:
	case op_semi:
	case op_anti:

	case op_union:
	case op_inter:
	case op_except:
		if (rel->l)
			rel_reset_subquery(rel->l);
		if (rel->r)
			rel_reset_subquery(rel->r);
	}

}

static int rel_bind_exps(mvc *sql, sql_rel *rel, list *exps);

static sql_exp * 
bind_exp(mvc *sql, sql_rel *rel, sql_exp *e, int top_groupby)
{
	sql_rel *s = rel;
	sql_exp *found = NULL;

	if (!s)
		return NULL;
	if (is_groupby(s->op) && top_groupby) {
		if (s->r) {
			if (e->l) {
				found = exps_bind_column2(s->r, e->l, e->r );
			} else {
				found = exps_bind_column(s->r, e->r, NULL);
			}
		}
		if (found) 
			return found;
		if (!is_aggr(e->type))
			return found;
		s = s->l;
	}
	if (is_project(s->op) || is_base(s->op)) {
		if (e->l) {
			found = exps_bind_column2(s->exps, e->l, e->r );
		} else {
			found = exps_bind_column(s->exps, e->r, NULL);
		}
		if (!found) { /* add */
			sql_exp *ne = NULL;
		       
			if (!top_groupby && is_groupby(s->op))
				ne = bind_exp(sql, s, e, 1);
			else //if (is_groupby(s->op))
				ne = !is_base(s->op)?bind_exp(sql, s->l, e, 0):NULL; 
				//sql_exp *ne = exp_column(sql->sa, e->l, e->r, exp_subtype(e), exp_card(e), has_nil(e), is_intern(e)); 
			if (!ne) {
				if (exp_name(e)) {
					return sql_error(sql, 05, SQLSTATE(42000) "SELECT: cannot use non GROUP BY column '%s' in query results without an aggregate function", exp_name(e));
				} else {
					return sql_error(sql, 05, SQLSTATE(42000) "SELECT: cannot use non GROUP BY column in query results without an aggregate function");
				}
			}
			ne = rel_project_add_exp(sql, s, ne);
			return ne;
		}
	} else { /* handle joins */
		sql_rel *r = s;
		s = s->l;
		//reset_processed(s);
		if (e->l) {
			found = rel_bind_column2(sql, s, e->l, e->r, 0);
		} else {
			found = rel_bind_column(sql, s, e->r, 0);
		}
		if (!found && (is_join(r->op) || is_semi(r->op))) {
			s = r->r;
			//reset_processed(s);
			if (e->l) {
				found = rel_bind_column2(sql, s, e->l, e->r, 0);
			} else 	{
				found = rel_bind_column(sql, s, e->r, 0);
			}
		}
		if (found)
			return bind_exp(sql, s, e, 0);
	}
	return found;
}

static list*
aggrs_split_args(mvc *sql, list *aggrs, list *exps) 
{
	if (list_empty(aggrs))
		return aggrs;
	for (node *n=aggrs->h; n; n = n->next) {
		sql_exp *a = n->data;
		list *args = a->l;

		if (!list_empty(args)) {
			for (node *an = args->h; an; an = an->next) {
				sql_exp *e1 = an->data, *found = NULL;

				for (node *nn = exps->h; nn && !found; nn = nn->next) {
					sql_exp *e2 = nn->data;
		
					if (!exp_equal(e1, e2))
						found = e2;
				}
				if (!found) {
					append(exps, e1);
					if (!exp_name(e1))
						e1 = exp_label(sql->sa, e1, ++sql->label);
				} else {
					e1 = found;
				}
				e1 = exp_ref(sql->sa, e1);
				an->data = e1; /* replace by reference */
			}
		}
	}
	return aggrs;
}

static sql_rel *
rel_groupby_split_exps(mvc *sql, sql_rel *rel)
{
	list *exps = sa_list(sql->sa);

	assert(is_groupby(rel->op));
	rel->r = aggrs_split_args(sql, rel->r, exps);
	rel->exps = aggrs_split_args(sql, rel->exps, exps);
	rel->l = rel_project(sql->sa, rel->l, exps);
	return rel;
}

static int
rel_bind_exp_(mvc *sql, sql_rel *rel, sql_exp *e) 
{
	int ok = 1;
	sql_exp *found = NULL;

	if (!e)
		return !ok;

	switch(e->type) {
	case e_column:
		if (is_freevar(e))
			return 0;
		/* recurse down into the relation and add it to proper projection */
		found = bind_exp(sql, is_simple_project(rel->op)?rel->l:rel, e, is_groupby(rel->op));
		if (!found) { 
			printf("#not found %s.%s\n", (char*)e->l, (char*)e->r);
			return -1;
		}
		return !ok;
	case e_convert:
		return rel_bind_exp_(sql, rel, e->l);
	case e_aggr:
	case e_func: 
		if (e->r) { /* rewrite rank op */
			/* TODO find all rank op's and merge gbe/obe */
			list *r = e->r, *gbe = r->h->data, *obe = r->h->next->data; 

			assert(is_simple_project(rel->op) || is_groupby(rel->op));
			/* First push rank expressions down under the group by */
			if (is_groupby(rel->op)) {
				rel = rel_groupby_split_exps(sql, rel);
				return !ok;
			}
			assert(is_simple_project(rel->op));
			if (gbe || obe) {
				sql_rel *r = rel_project(sql->sa, rel->l, rel_projections(sql, rel->l, NULL, 1, 1));
				if (gbe && obe) {
					gbe = list_merge(sa_list(sql->sa), gbe, (fdup)NULL); /* make sure the p->r is a different list than the gbe list */
					for(node *n = obe->h ; n ; n = n->next) {
						sql_exp *e1 = n->data;
						bool found = false;

						for(node *nn = gbe->h ; nn && !found ; nn = nn->next) {
							sql_exp *e2 = nn->data;
							//the partition expression order should be the same as the one in the order by clause (if it's in there as well)
							if(!exp_equal(e1, e2)) {
								if(is_ascending(e1))
									e2->flag |= ASCENDING;
								else
									e2->flag &= ~ASCENDING;
								found = true;
							}
						}
						if(!found)
							append(gbe, e1);
					}
				} else if (obe) {
					gbe = obe;
				}
				r->r = gbe;
				rel->l = r;
			}
			/* mark a normal (analytic) function now */
			e->r = NULL;
		}
		if (e->l) {
			list *l = e->l;
			node *n;
	
			for(n = l->h; n && ok; n = n->next)
				ok = !rel_bind_exp_(sql, rel, n->data);
		}
		return !ok;
	case e_cmp:	
		if (get_cmp(e) == cmp_or || get_cmp(e) == cmp_filter) {
			ok = !rel_bind_exps(sql, rel, e->l);
			if (ok)
				ok = !rel_bind_exps(sql, rel, e->r);
		} else if (e->flag == cmp_in || e->flag == cmp_notin) {
			ok = !rel_bind_exp_(sql, rel, e->l);
			if (ok)
				ok = !rel_bind_exps(sql, rel, e->r);
		} else {
			ok = !rel_bind_exp_(sql, rel, e->l);
			if (ok)
				ok = !rel_bind_exp_(sql, rel, e->r);
			if (ok && e->f)
				ok = !rel_bind_exp_(sql, rel, e->f);
		}
		return !ok;
	case e_psm:	
	case e_atom:
		return 0;
	}
	return 0;
}

static int
rel_bind_exps(mvc *sql, sql_rel *rel, list *exps) 
{
	node *n;
	int ok = 1;

	if (list_empty(exps))
		return 0;
	for (n = exps->h; n && ok; n = n->next)
		rel_bind_exp_(sql, rel, n->data);
	return !ok;
}

static int
rel_bind(mvc *sql, sql_rel *rel) 
{
	int ok = 1;

	if (!rel || is_base(rel->op) || is_ddl(rel->op))
		return 0;

	if (is_simple_project(rel->op) && list_empty(rel->exps)) {
		append(rel->exps, exp_atom_bool(sql->sa, 1));
		return 0;
	}
	if (ok && rel->exps)
		ok = !rel_bind_exps(sql, rel, rel->exps);
	if (ok && is_groupby(rel->op) && rel->r)
		ok = !rel_bind_exps(sql, rel, rel->r);

	//rel->subquery = 0;
	switch(rel->op){
	case op_basetable:
	case op_table:
		return 0;
	case op_ddl:

	case op_insert:
	case op_update:
	case op_delete:
	case op_truncate:
		if (rel->l)
			ok = !rel_bind(sql, rel->l);
		if (ok && rel->r)
			ok = !rel_bind(sql, rel->r);
		break;
	case op_select:
	case op_topn:
	case op_sample:
		if (rel->l)
			ok = !rel_bind(sql, rel->l);
		break;

	case op_project:
	case op_groupby:
		if (rel->l)
			ok = !rel_bind(sql, rel->l);
		if (list_empty(rel->exps)) 
			append(rel->exps, exp_atom_bool(sql->sa, 1));
		break;
	case op_join:
	case op_left:
	case op_right:
	case op_full:
	case op_semi:
	case op_anti:

	case op_union:
	case op_inter:
	case op_except:
		if (rel->l)
			ok = !rel_bind(sql, rel->l);
		if (ok && rel->r)
			ok = !rel_bind(sql, rel->r);
	}
	return !ok;
}

sql_rel *
rel_unnest(mvc *sql, sql_rel *rel)
{
	rel_reset_subquery(rel);
	if (rel_bind(sql, rel)) {
		return NULL;
	}
	return _rel_unnest(sql, rel);
}
