/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

#ifndef _REL_EXP_H_
#define _REL_EXP_H_

#include "sql_relation.h"
#include "sql_mvc.h"
#include "sql_atom.h"

#define new_exp_list(sa) sa_list(sa)
#define exp2list(sa,e)   append(sa_list(sa),e)

sql_extern comp_type swap_compare( comp_type t );
sql_extern comp_type range2lcompare( int r );
sql_extern comp_type range2rcompare( int r );
sql_extern int compare2range( int l, int r );

sql_extern sql_exp *exp_compare(sql_allocator *sa, sql_exp *l, sql_exp *r, int cmptype);
sql_extern sql_exp *exp_compare2(sql_allocator *sa, sql_exp *l, sql_exp *r, sql_exp *h, int cmptype);
sql_extern sql_exp *exp_filter(sql_allocator *sa, list *l, list *r, sql_subfunc *f, int anti);
sql_extern sql_exp *exp_or(sql_allocator *sa, list *l, list *r, int anti);
sql_extern sql_exp *exp_in(sql_allocator *sa, sql_exp *l, list *r, int cmptype);

#define exp_fromtype(e)	((list*)e->r)->h->data
#define exp_totype(e)	((list*)e->r)->h->next->data
sql_extern sql_exp *exp_convert(sql_allocator *sa, sql_exp *exp, sql_subtype *fromtype, sql_subtype *totype );
sql_extern str number2name(str s, int len, int i);
sql_extern sql_exp *exp_op(sql_allocator *sa, list *l, sql_subfunc *f );

#define append(l,v) list_append(l,v) 
#define exp_unop(sa,l,f) \
	exp_op(sa, append(new_exp_list(sa),l), f)
#define exp_binop(sa,l,r,f) \
	exp_op(sa, append(append(new_exp_list(sa),l),r), f)
#define exp_op3(sa,l,r,r2,f) \
	exp_op(sa, append(append(append(new_exp_list(sa),l),r),r2), f)
#define exp_op4(sa,l,r,r2,r3,f) \
	exp_op(sa, append(append(append(append(new_exp_list(sa),l),r),r2),r3), f)
sql_extern sql_exp *exp_aggr(sql_allocator *sa, list *l, sql_subaggr *a, int distinct, int no_nils, unsigned int card, int has_nil );
#define exp_aggr1(sa, e, a, d, n, c, hn) \
	exp_aggr(sa, append(new_exp_list(sa), e), a, d, n, c, hn)
sql_extern sql_exp * exp_atom(sql_allocator *sa, atom *a);
sql_extern sql_exp * exp_atom_max(sql_allocator *sa, sql_subtype *tpe);
sql_extern sql_exp * exp_atom_bool(sql_allocator *sa, int b);
sql_extern sql_exp * exp_atom_bte(sql_allocator *sa, bte i);
sql_extern sql_exp * exp_atom_sht(sql_allocator *sa, sht i);
sql_extern sql_exp * exp_atom_int(sql_allocator *sa, int i);
sql_extern sql_exp * exp_atom_lng(sql_allocator *sa, lng l);
#ifdef HAVE_HGE
sql_extern sql_exp * exp_atom_hge(sql_allocator *sa, hge l);
#endif
sql_extern sql_exp * exp_atom_flt(sql_allocator *sa, flt f);
sql_extern sql_exp * exp_atom_dbl(sql_allocator *sa, dbl d);
sql_extern sql_exp * exp_atom_str(sql_allocator *sa, const char *s, sql_subtype *st);
sql_extern sql_exp * exp_atom_clob(sql_allocator *sa, const char *s);
sql_extern sql_exp * exp_atom_ptr(sql_allocator *sa, void *s);
sql_extern sql_exp * exp_atom_ref(sql_allocator *sa, int i, sql_subtype *tpe);
sql_extern sql_exp * exp_null(sql_allocator *sa, sql_subtype *tpe);
sql_extern sql_exp * exp_param(sql_allocator *sa, const char *name, sql_subtype *tpe, int frame);
sql_extern atom * exp_value(mvc *sql, sql_exp *e, atom **args, int maxarg);
sql_extern sql_exp * exp_values(sql_allocator *sa, list *exps);
sql_extern list * exp_types(sql_allocator *sa, list *exps);
sql_extern int have_nil(list *exps);

sql_extern sql_exp * exp_column(sql_allocator *sa, const char *rname, const char *name, sql_subtype *t, unsigned int card, int has_nils, int intern);
sql_extern sql_exp * exp_propagate(sql_allocator *sa, sql_exp *ne, sql_exp *oe);
#define exp_ref(sa, e) exp_propagate(sa, exp_column(sa, exp_relname(e), exp_name(e), exp_subtype(e), exp_card(e), has_nil(e), is_intern(e)), e)
sql_extern sql_exp * exp_alias(sql_allocator *sa, const char *arname, const char *acname, const char *org_rname, const char *org_cname, sql_subtype *t, unsigned int card, int has_nils, int intern);
sql_extern sql_exp * exp_alias_or_copy( mvc *sql, const char *tname, const char *cname, sql_rel *orel, sql_exp *old);
sql_extern sql_exp * exp_set(sql_allocator *sa, const char *name, sql_exp *val, int level);
sql_extern sql_exp * exp_var(sql_allocator *sa, const char *name, sql_subtype *type, int level);
sql_extern sql_exp * exp_table(sql_allocator *sa, const char *name, sql_table *t, int level);
sql_extern sql_exp * exp_return(sql_allocator *sa, sql_exp *val, int level);
sql_extern sql_exp * exp_while(sql_allocator *sa, sql_exp *cond, list *stmts);
sql_extern sql_exp * exp_exception(sql_allocator *sa, sql_exp *cond, char* error_message);
sql_extern sql_exp * exp_if(sql_allocator *sa, sql_exp *cond, list *if_stmts, list *else_stmts);
sql_extern sql_exp * exp_rel(mvc *sql, sql_rel * r);

sql_extern void exp_setname(sql_allocator *sa, sql_exp *e, const char *rname, const char *name );
sql_extern void exp_setrelname(sql_allocator *sa, sql_exp *e, int nr );
sql_extern void exp_setalias(sql_exp *e, const char *rname, const char *name);
sql_extern void exp_prop_alias(sql_exp *e, sql_exp *oe);

sql_extern void noninternexp_setname(sql_allocator *sa, sql_exp *e, const char *rname, const char *name );
sql_extern char* make_label(sql_allocator *sa, int nr);
sql_extern sql_exp* exp_label(sql_allocator *sa, sql_exp *e, int nr);
sql_extern sql_exp* exp_label_table(sql_allocator *sa, sql_exp *e, int nr);
sql_extern list* exps_label(sql_allocator *sa, list *exps, int nr);

sql_extern sql_exp * exp_copy( sql_allocator *sa, sql_exp *e);
sql_extern list * exps_copy( sql_allocator *sa, list *exps);
sql_extern list * exps_alias( sql_allocator *sa, list *exps);

sql_extern void exp_swap( sql_exp *e );

sql_extern sql_subtype * exp_subtype( sql_exp *e );
sql_extern const char * exp_name( sql_exp *e );
sql_extern const char * exp_relname( sql_exp *e );
sql_extern const char * exp_func_name( sql_exp *e );
sql_extern unsigned int exp_card(sql_exp *e);

sql_extern const char *exp_find_rel_name(sql_exp *e);

sql_extern sql_exp *rel_find_exp( sql_rel *rel, sql_exp *e);

sql_extern int exp_cmp( sql_exp *e1, sql_exp *e2);
sql_extern int exp_equal( sql_exp *e1, sql_exp *e2);
sql_extern int exp_refers( sql_exp *p, sql_exp *c);
sql_extern int exp_match( sql_exp *e1, sql_exp *e2);
sql_extern sql_exp* exps_find_exp( list *l, sql_exp *e);
sql_extern int exp_match_exp( sql_exp *e1, sql_exp *e2);
/* match just the column (cmp equality) expressions */
sql_extern int exp_match_col_exps( sql_exp *e, list *l);
sql_extern int exps_match_col_exps( sql_exp *e1, sql_exp *e2);
/* todo rename */
sql_extern int exp_match_list( list *l, list *r);
sql_extern int exp_is_join(sql_exp *e, list *rels);
sql_extern int exp_is_eqjoin(sql_exp *e, sql_exp *f); //the parameter f is required to compile
sql_extern int exp_is_correlation(sql_exp *e, sql_rel *r );
sql_extern int exp_is_join_exp(sql_exp *e);
sql_extern int exp_is_atom(sql_exp *e);
sql_extern int exp_is_true(mvc *sql, sql_exp *e);
sql_extern int exp_is_zero(mvc *sql, sql_exp *e);
sql_extern int exp_is_not_null(mvc *sql, sql_exp *e);
sql_extern int exp_is_null(mvc *sql, sql_exp *e);
sql_extern int exps_are_atoms(list *exps);
sql_extern int exp_has_func(sql_exp *e);
sql_extern int exp_unsafe(sql_exp *e, int allow_identity);
sql_extern int exp_has_sideeffect(sql_exp *e);

/* returns 0 when the relation contain the passed expression else < 0 */
sql_extern int rel_has_exp(sql_rel *rel, sql_exp *e);
/* return 0 when the relation contain atleast one of the passed expressions else < 0 */
sql_extern int rel_has_exps(sql_rel *rel, list *e);
/* return 1 when the relation contains all of the passed expressions else 0 */
sql_extern int rel_has_all_exps(sql_rel *rel, list *e);

sql_extern sql_rel *find_rel(list *rels, sql_exp *e);
sql_extern sql_rel *find_one_rel(list *rels, sql_exp *e);

sql_extern sql_exp *exps_bind_column( list *exps, const char *cname, int *ambiguous);
sql_extern sql_exp *exps_bind_column2( list *exps, const char *rname, const char *cname);
sql_extern sql_exp *exps_bind_alias( list *exps, const char *rname, const char *cname);

sql_extern unsigned int exps_card( list *l );
sql_extern void exps_fix_card( list *exps, unsigned int card);
sql_extern void exps_setcard( list *exps, unsigned int card);
sql_extern int exps_intern(list *exps);

sql_extern char *compare_func( comp_type t, int anti );
sql_extern int is_identity( sql_exp *e, sql_rel *r);

sql_extern atom *exp_flatten(mvc *sql, sql_exp *e);

sql_extern void exp_sum_scales(sql_subfunc *f, sql_exp *l, sql_exp *r);

sql_extern sql_exp *create_table_part_atom_exp(mvc *sql, sql_subtype tpe, ptr value);

sql_extern int exp_aggr_is_count(sql_exp *e);

sql_extern void exps_reset_freevar(list *exps);

sql_extern int rel_set_type_recurse(mvc *sql, sql_subtype *type, sql_rel *rel, const char **relname, const char **expname);
#endif /* _REL_EXP_H_ */
