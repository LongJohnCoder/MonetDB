#ifndef _MOSAIC_JOIN_
#define _MOSAIC_JOIN_

#include "mosaic.h"


#define MOSjoin_COUI_SIGNATURE(NAME, TPE) \
str								\
MOSjoin_COUI_##NAME##_##TPE(BAT* r1p, BAT* r2p,\
    MOStask task, BAT* r, struct canditer* rci, bool nil_matches)\

#define INNER_LOOP_UNCOMPRESSED(HAS_NIL, TPE, RIGHT_CI_NEXT) \
{\
	TPE* vr = (TPE*) Tloc(r, 0);\
	canditer_reset(rci);\
	for (BUN ri = 0; ri < rci->ncand; ri++) {\
		oid ro = RIGHT_CI_NEXT(rci);\
		if (ARE_EQUAL(lval, vr[ro-r->hseqbase], HAS_NIL, TPE)){\
			if( BUNappend(r1p, &lo, false)!= GDK_SUCCEED ||\
			BUNappend(r2p, &ro, false) != GDK_SUCCEED)\
			throw(MAL,"mosaic.raw",MAL_MALLOC_FAIL);\
		}\
	}\
}

#define JOIN_WITH_NIL_INFO(HAS_NIL, NIL_MATCHES, TPE, OUTER_LOOP) \
{\
	if( (lci->tpe == cand_dense) && (rci->tpe == cand_dense)){\
		OUTER_LOOP(HAS_NIL, NIL_MATCHES, TPE, canditer_next_dense, canditer_next_dense);\
	}\
	if( (lci->tpe != cand_dense) && (rci->tpe == cand_dense)){\
		OUTER_LOOP(HAS_NIL, NIL_MATCHES, TPE, canditer_next, canditer_next_dense);\
	}\
	if( (lci->tpe == cand_dense) && (rci->tpe != cand_dense)){\
		OUTER_LOOP(HAS_NIL, NIL_MATCHES, TPE, canditer_next_dense, canditer_next);\
	}\
	if( (lci->tpe != cand_dense) && (rci->tpe != cand_dense)){\
		OUTER_LOOP(HAS_NIL, NIL_MATCHES, TPE, canditer_next, canditer_next);\
	}\
}

#define NESTED_LOOP_JOIN(TPE, OUTER_LOOP) {\
	if( nil && nil_matches){\
		JOIN_WITH_NIL_INFO(true, true, TPE, OUTER_LOOP);\
	}\
	if( !nil && nil_matches){\
		JOIN_WITH_NIL_INFO(false, true, TPE, OUTER_LOOP);\
	}\
	if( nil && !nil_matches){\
		JOIN_WITH_NIL_INFO(true, false, TPE, OUTER_LOOP);\
	}\
	if( !nil && !nil_matches){\
		JOIN_WITH_NIL_INFO(false, false, TPE, OUTER_LOOP);\
	}\
}

#define MOSjoin_COUI_DEF(NAME, TPE)\
MOSjoin_COUI_SIGNATURE(NAME, TPE)\
{\
	BUN first = task->start;\
	BUN last = first + MOSgetCnt(task->blk);\
	TPE* bt= (TPE*) task->src;\
	bool nil = !task->bsrc->tnonil;\
\
	struct canditer* lci = task->ci;\
\
	/* Advance the candidate iterator to the first element within
	 * the oid range of the current block.
	 */\
	oid c = canditer_next(lci);\
	while (!is_oid_nil(c) && c < first ) {\
		c = canditer_next(lci);\
	}\
\
	if 		(is_oid_nil(c)) {\
		/* Nothing left to scan.
		 * So we can signal the generic select function to stop now.
		 */\
		return MAL_SUCCEED;\
	}\
\
    NESTED_LOOP_JOIN(TPE, outer_loop_##NAME);\
	task->src = (char*) bt;\
\
	if ((c = canditer_peekprev(lci)) >= last) {\
		/*Restore iterator if it went pass the end*/\
		(void) canditer_prev(lci);\
	}\
\
	return MAL_SUCCEED;\
}

#define do_join_COUI(NAME, TPE, DUMMY_ARGUMENT)\
{\
	str msg = MOSjoin_COUI_##NAME##_##TPE(r1p, r2p, task, r, rci, nil_matches);\
	if (msg != MAL_SUCCEED) return msg;\
	MOSadvance_##NAME##_##TPE(task);\
}

/* Nested loop join with the left (C)ompressed side in the (O)uter loop 
 * and the right (U)ncompressed side in the (I)nner loop.
 */
#define MOSjoin_generic_COUI_DEF(TPE) \
static str MOSjoin_COUI_##TPE(MOStask task, BAT* r, struct canditer* rci, bool nil_matches)\
{\
    BAT* r1p = task->lbat;\
    BAT* r2p = task->rbat;\
\
	struct canditer* lci = task->ci;\
\
	while(task->start < task->stop ){\
\
		switch(MOSgetTag(task->blk)){\
		case MOSAIC_RLE:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_runlength\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, runlength, TPE);\
			break;\
		case MOSAIC_CAPPED:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_capped\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, capped, TPE);\
			break;\
		case MOSAIC_VAR:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_var\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, var, TPE);\
			break;\
		case MOSAIC_FRAME:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_frame\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, frame, TPE);\
			break;\
		case MOSAIC_DELTA:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_delta\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, delta, TPE);\
			break;\
		case MOSAIC_PREFIX:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_prefix\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, prefix, TPE);\
			break;\
		case MOSAIC_LINEAR:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_linear\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, linear, TPE);\
			break;\
		case MOSAIC_RAW:\
			ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_COUI_raw\n");\
			DO_OPERATION_IF_ALLOWED(join_COUI, raw, TPE);\
			break;\
		}\
\
		if (lci->next == lci->ncand) {\
			/* We are at the end of the candidate list.
			 * So we can stop now.
			 */\
			return MAL_SUCCEED;\
		}\
	}\
\
	assert(MOSgetTag(task->blk) == MOSAIC_EOL);\
\
	return MAL_SUCCEED;\
}

/* Nested loop join with the left uncompressed side in the outer loop
 * and the right compressed side in the inner loop.
 */

#define do_join_inner_loop(NAME, TPE, HAS_NIL, RIGHT_CI_NEXT)\
{\
	join_inner_loop_##NAME(TPE, HAS_NIL, RIGHT_CI_NEXT);\
	MOSadvance_##NAME##_##TPE(task);\
}

#define IF_EQUAL_APPEND_RESULT(HAS_NIL, TPE)\
{\
	if (ARE_EQUAL(lval, rval, HAS_NIL, TPE)){\
		if( BUNappend(r1p, &ro, false)!= GDK_SUCCEED || BUNappend(r2p, &lo, false) != GDK_SUCCEED)\
		throw(MAL,"mosaic.raw",MAL_MALLOC_FAIL);\
	}\
}

#define OUTER_LOOP_UNCOMPRESSED(HAS_NIL, NIL_MATCHES, TPE, LEFT_CI_NEXT, RIGHT_CI_NEXT) \
{\
	str msg = MAL_SUCCEED;\
\
	TPE* vl = (TPE*) Tloc(l, 0);\
	for (BUN li = 0; li < lci->ncand; li++, MOSinitializeScan(task, task->bsrc), canditer_reset(rci)) {\
		oid lo = LEFT_CI_NEXT(lci);\
		TPE lval = vl[lo-l->hseqbase];\
		if (HAS_NIL && !NIL_MATCHES) {\
			if ((IS_NIL(TPE, lval))) {continue;};\
		}\
\
		while(task->start < task->stop ){\
			BUN first = task->start;\
			BUN last = first + MOSgetCnt(task->blk);\
\
			oid c = canditer_next(rci);\
			while (!is_oid_nil(c) && c < first ) {\
				c = canditer_next(rci);\
			}\
			switch(MOSgetTag(task->blk)){\
			case MOSAIC_RLE:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_runlength\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, runlength, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			case MOSAIC_CAPPED:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_capped\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, capped, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			case MOSAIC_VAR:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_var\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, var, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			case MOSAIC_FRAME:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_frame\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, frame, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			case MOSAIC_DELTA:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_delta\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, delta, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			case MOSAIC_PREFIX:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_prefix\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, prefix, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			case MOSAIC_LINEAR:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_linear\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, linear, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			case MOSAIC_RAW:\
				ALGODEBUG mnstr_printf(GDKstdout, "MOSjoin_raw\n");\
				DO_OPERATION_IF_ALLOWED_VARIADIC(join_inner_loop, raw, TPE, HAS_NIL, RIGHT_CI_NEXT);\
				break;\
			}\
	\
			if (msg != MAL_SUCCEED) return msg;\
\
		if (canditer_peekprev(task->ci) >= last) {\
			/*Restore iterator if it went pass the end*/\
			(void) canditer_prev(task->ci);\
		}\
\
			if (rci->next == rci->ncand) {\
				/* We are at the end of the candidate list.
				* So we can stop now.
				*/\
				break;\
			}\
		}\
	\
		assert(MOSgetTag(task->blk) == MOSAIC_EOL);\
	}\
}

#define MOSjoin_generic_DEF(TPE) \
static str MOSjoin_##TPE(MOStask task, BAT* l, struct canditer* lci, bool nil_matches)\
{\
    BAT* r1p = task->lbat;\
    BAT* r2p = task->rbat;\
\
	struct canditer* rci = task->ci;\
	bool nil = !l->tnonil;\
\
	NESTED_LOOP_JOIN(TPE, OUTER_LOOP_UNCOMPRESSED);\
\
	return MAL_SUCCEED;\
}

#endif /* _MOSAIC_JOIN_ */
