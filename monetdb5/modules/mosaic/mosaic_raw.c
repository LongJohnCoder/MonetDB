/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2018 MonetDB B.V.
 */

/*
 * 2014-2016 author Martin Kersten
 * Use a chunk that has not been compressed
 */

#include "monetdb_config.h"
#include "mosaic.h"
#include "mosaic_raw.h"
#include "mosaic_private.h"

bool MOStypes_raw(BAT* b) {
	switch(b->ttype){
	case TYPE_bit: return true; // Will be mapped to bte
	case TYPE_bte: return true;
	case TYPE_sht: return true;
	case TYPE_int: return true;
	case TYPE_lng: return true;
	case TYPE_oid: return true;
	case TYPE_flt: return true;
	case TYPE_dbl: return true;
#ifdef HAVE_HGE
	case TYPE_hge: return true;
#endif
	default:
		if (b->ttype == TYPE_date) {return true;} // Will be mapped to int
		if (b->ttype == TYPE_daytime) {return true;} // Will be mapped to lng
		if (b->ttype == TYPE_timestamp) {return true;} // Will be mapped to lng
	}

	return false;
}

void
MOSlayout_raw(MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties)
{
	MosaicBlk blk = (MosaicBlk) task->blk;
	lng cnt = MOSgetCnt(blk), input=0, output= 0;

		input = cnt * ATOMsize(task->type);
	switch(ATOMbasetype(task->type)){
	case TYPE_bte: output = wordaligned( MosaicBlkSize + sizeof(bte)* MOSgetCnt(blk),bte); break;
	case TYPE_sht: output = wordaligned( MosaicBlkSize + sizeof(sht)* MOSgetCnt(blk),sht); break;
	case TYPE_int: output = wordaligned( MosaicBlkSize + sizeof(int)* MOSgetCnt(blk),int); break;
	case TYPE_lng: output = wordaligned( MosaicBlkSize + sizeof(lng)* MOSgetCnt(blk),lng); break;
	case TYPE_oid: output = wordaligned( MosaicBlkSize + sizeof(oid)* MOSgetCnt(blk),oid); break;
	case TYPE_flt: output = wordaligned( MosaicBlkSize + sizeof(flt)* MOSgetCnt(blk),flt); break;
	case TYPE_dbl: output = wordaligned( MosaicBlkSize + sizeof(dbl)* MOSgetCnt(blk),dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: output = wordaligned( MosaicBlkSize + sizeof(hge)* MOSgetCnt(blk),hge); break;
#endif
	}
	if( BUNappend(btech, "raw blk", false) != GDK_SUCCEED ||
		BUNappend(bcount, &cnt, false) != GDK_SUCCEED ||
		BUNappend(binput, &input, false) != GDK_SUCCEED ||
		BUNappend(boutput, &output, false) != GDK_SUCCEED ||
		BUNappend(bproperties, "", false) != GDK_SUCCEED)
		return;
}

#define MOSadvance_DEF(TPE)\
MOSadvance_SIGNATURE(raw, TPE)\
{\
	MosaicBlk blk = task->blk;\
	task->start += MOSgetCnt(blk);\
	task->blk = (MosaicBlk)( ((char*) task->blk) + wordaligned( MosaicBlkSize + sizeof(TPE)* MOSgetCnt(blk),TPE));\
}

MOSadvance_DEF(bte)
MOSadvance_DEF(sht)
MOSadvance_DEF(int)
MOSadvance_DEF(lng)
MOSadvance_DEF(flt)
MOSadvance_DEF(dbl)
#ifdef HAVE_HGE
MOSadvance_DEF(hge)
#endif

void
MOSadvance_raw(MOStask task)
{
	switch(ATOMbasetype(task->type)){
	case TYPE_bte: MOSadvance_raw_bte(task); break;
	case TYPE_sht: MOSadvance_raw_sht(task); break;
	case TYPE_int: MOSadvance_raw_int(task); break;
	case TYPE_lng: MOSadvance_raw_lng(task); break;
	case TYPE_flt: MOSadvance_raw_flt(task); break;
	case TYPE_dbl: MOSadvance_raw_dbl(task); break;
#ifdef HAVE_HGE
	case TYPE_hge: MOSadvance_raw_hge(task); break;
#endif
	}
}

#define Estimate(TPE)\
{\
	/*The raw compression technique is always applicable and only adds one item at a time.*/\
	current->compression_strategy.tag = MOSAIC_RAW;\
	current->is_applicable = true;\
	current->uncompressed_size += (BUN) sizeof(TPE);\
	unsigned int cnt = previous->compression_strategy.cnt;\
	if (previous->compression_strategy.tag == MOSAIC_RAW && cnt + 1 < (1 << CNT_BITS)) {\
		current->must_be_merged_with_previous = true;\
		cnt++;\
		current->compressed_size += sizeof(TPE);\
	}\
	else {\
		current->must_be_merged_with_previous = false;\
		cnt = 1;\
		current->compressed_size += wordaligned(MosaicBlkSize, TPE) + sizeof(TPE);\
	}\
	current->compression_strategy.cnt = cnt;\
}

str
MOSestimate_raw(MOStask task, MosaicEstimation* current, const MosaicEstimation* previous) {

	switch(ATOMbasetype(task->type)){
	case TYPE_bte: Estimate(bte); break;
	case TYPE_sht: Estimate(sht); break;
	case TYPE_int: Estimate(int); break;
	case TYPE_lng: Estimate(lng); break;
	case TYPE_oid: Estimate(oid); break;
	case TYPE_flt: Estimate(flt); break;
	case TYPE_dbl: Estimate(dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: Estimate(hge); break;
#endif
	}

	return MAL_SUCCEED;
}


// append a series of values into the non-compressed block

#define RAWcompress(TYPE)\
{\
	TYPE *v = ((TYPE*)task->src) + task->start;\
	unsigned int cnt = estimate->cnt;\
	TYPE *d = (TYPE*)task->dst;\
	for(unsigned int i = 0; i<cnt; i++,v++){\
		*d++ = (TYPE) *v;\
	}\
	task->dst += sizeof(TYPE);\
	MOSsetCnt(blk,cnt);\
}

// rather expensive simple value non-compressed store
void
MOScompress_raw(MOStask task, MosaicBlkRec* estimate)
{
	MosaicBlk blk = (MosaicBlk) task->blk;

	MOSsetTag(blk,MOSAIC_RAW);

	switch(ATOMbasetype(task->type)){
	case TYPE_bte: RAWcompress(bte); break;
	case TYPE_sht: RAWcompress(sht); break;
	case TYPE_int: RAWcompress(int); break;
	case TYPE_lng: RAWcompress(lng); break;
	case TYPE_oid: RAWcompress(oid); break;
	case TYPE_flt: RAWcompress(flt); break;
	case TYPE_dbl: RAWcompress(dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: RAWcompress(hge); break;
#endif
	}
}

// the inverse operator, extend the src
#define RAWdecompress(TYPE)\
{ BUN lim = MOSgetCnt(blk); \
	for(i = 0; i < lim; i++) {\
	((TYPE*)task->src)[i] = ((TYPE*)compressed)[i]; \
	}\
	task->src += i * sizeof(TYPE);\
}

void
MOSdecompress_raw(MOStask task)
{
	MosaicBlk blk = (MosaicBlk) task->blk;
	BUN i;
	char *compressed;

	compressed = ((char*)blk) + MosaicBlkSize;
	switch(ATOMbasetype(task->type)){
	case TYPE_bte: RAWdecompress(bte); break;
	case TYPE_sht: RAWdecompress(sht); break;
	case TYPE_int: RAWdecompress(int); break;
	case TYPE_lng: RAWdecompress(lng); break;
	case TYPE_oid: RAWdecompress(oid); break;
	case TYPE_flt: RAWdecompress(flt); break;
	case TYPE_dbl: RAWdecompress(dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: RAWdecompress(hge); break;
#endif
	}
}

#define scan_loop_raw(TPE, CI_NEXT, TEST) \
{\
    TPE *val= (TPE*) (((char*) task->blk) + MosaicBlkSize);\
    for (oid c = canditer_peekprev(task->ci); !is_oid_nil(c) && c < last; c = CI_NEXT(task->ci)) {\
        BUN i = (BUN) (c - first);\
        v = val[i];\
        /*TODO: change from control to data dependency.*/\
        if (TEST)\
            *o++ = c;\
    }\
}

MOSselect_DEF(raw, bte)
MOSselect_DEF(raw, sht)
MOSselect_DEF(raw, int)
MOSselect_DEF(raw, lng)
MOSselect_DEF(raw, flt)
MOSselect_DEF(raw, dbl)
#ifdef HAVE_HGE
MOSselect_DEF(raw, hge)
#endif

#define projection_loop_raw(TPE, CI_NEXT)\
{	TPE *rt;\
	rt = (TPE*) (((char*) task->blk) + MosaicBlkSize);\
	for (oid o = canditer_peekprev(task->ci); !is_oid_nil(o) && o < last; o = CI_NEXT(task->ci)) {\
		BUN i = (BUN) (o - first);\
		*bt++ = rt[i];\
		task->cnt++;\
	}\
}

MOSprojection_DEF(raw, bte)
MOSprojection_DEF(raw, sht)
MOSprojection_DEF(raw, int)
MOSprojection_DEF(raw, lng)
MOSprojection_DEF(raw, flt)
MOSprojection_DEF(raw, dbl)
#ifdef HAVE_HGE
MOSprojection_DEF(raw, hge)
#endif

#define outer_loop_raw(HAS_NIL, NIL_MATCHES, TPE, LEFT_CI_NEXT, RIGHT_CI_NEXT) \
{\
    TPE *vl;\
	vl = (TPE*) (((char*) task->blk) + MosaicBlkSize);\
    for (oid lo = canditer_peekprev(task->ci); !is_oid_nil(lo) && lo < last; lo = LEFT_CI_NEXT(task->ci)) {\
        TPE lval = vl[lo-first];\
		if (HAS_NIL && !NIL_MATCHES) {\
			if ((IS_NIL(TPE, lval))) {continue;};\
		}\
		INNER_LOOP_UNCOMPRESSED(HAS_NIL, TPE, RIGHT_CI_NEXT);\
	}\
}

MOSjoin_COUI_DEF(raw, bte)
MOSjoin_COUI_DEF(raw, sht)
MOSjoin_COUI_DEF(raw, int)
MOSjoin_COUI_DEF(raw, lng)
MOSjoin_COUI_DEF(raw, flt)
MOSjoin_COUI_DEF(raw, dbl)
#ifdef HAVE_HGE
MOSjoin_COUI_DEF(raw, hge)
#endif
