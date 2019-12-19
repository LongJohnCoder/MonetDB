/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

/*
 * authors Martin Kersten, Aris Koning
 * Byte-wise delta encoding for SHT,INT,LNG, OID, WRD, STR-offsets, TIMESTAMP
 */

#include "monetdb_config.h"
#include "mosaic.h"
#include "mosaic_delta.h"
#include "mosaic_private.h"

//#define _DEBUG_MOSAIC_

bool MOStypes_delta(BAT* b) {
	switch(b->ttype) {
	case TYPE_bit: return true; // Will be mapped to bte
	case TYPE_bte: return true;
	case TYPE_sht: return true;
	case TYPE_int: return true;
	case TYPE_lng: return true;
	case TYPE_oid: return true;
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
#define BitVectorSize(CNT, BITS) wordaligned(((CNT) * (BITS) / CHAR_BIT) + ( ((CNT) * (BITS)) % CHAR_BIT != 0 ), BitVectorChunk)

#define MOSadvance_DEF(TPE)\
MOSadvance_SIGNATURE(delta, TPE)\
{\
	MOSBlockHeaderTpe(delta, TPE)* parameters = (MOSBlockHeaderTpe(delta, TPE)*) (task)->blk;\
	BUN cnt = MOSgetCnt(task->blk);\
\
	assert(cnt > 0);\
	assert(MOSgetTag(task->blk) == MOSAIC_DELTA);\
\
	task->start += (oid) cnt;\
\
	char* blk = (char*)task->blk;\
	blk += sizeof(MOSBlockHeaderTpe(delta, TPE));\
	blk += BitVectorSize(cnt, parameters->bits);\
	blk += GET_PADDING(task->blk, delta, TPE);\
\
	task->blk = (MosaicBlk) blk;\
\
}

MOSadvance_DEF(bte)
MOSadvance_DEF(sht)
MOSadvance_DEF(int)
MOSadvance_DEF(lng)
#ifdef HAVE_HGE
MOSadvance_DEF(hge)
#endif

void
MOSlayout_delta(MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties)
{
	MosaicBlk blk = task->blk;
	lng cnt = MOSgetCnt(blk), input=0, output= 0;

	input = cnt * ATOMsize(task->type);
	switch(ATOMbasetype(task->type)){
	case TYPE_bte: output = wordaligned(MosaicBlkSize + sizeof(bte) + MOSgetCnt(blk)-1,bte); break ;
	case TYPE_sht: output = wordaligned(MosaicBlkSize + sizeof(sht) + MOSgetCnt(blk)-1,sht); break ;
	case TYPE_int: output = wordaligned(MosaicBlkSize + sizeof(int) + MOSgetCnt(blk)-1,int); break ;
	case TYPE_lng: output = wordaligned(MosaicBlkSize + sizeof(lng) + MOSgetCnt(blk)-1,lng); break ;
#ifdef HAVE_HGE
	case TYPE_hge: output = wordaligned(MosaicBlkSize + sizeof(hge) + MOSgetCnt(blk)-1,hge); break ;
#endif
	}
	if( BUNappend(btech, "delta", false) != GDK_SUCCEED ||
		BUNappend(bcount, &cnt, false) != GDK_SUCCEED ||
		BUNappend(binput, &input, false) != GDK_SUCCEED ||
		BUNappend(boutput, &output, false) != GDK_SUCCEED ||
		BUNappend(bproperties, "", false) != GDK_SUCCEED)
		return;
}

#define determineDeltaParameters(PARAMETERS, SRC, LIMIT, TPE) \
do {\
	TPE *val = SRC;\
	int bits = 1;\
	unsigned int i;\
	DeltaTpe(TPE) unsigned_delta = 0;\
	TPE prev_val;\
	(PARAMETERS).init = *val;\
\
	for(i = 1; i < LIMIT; i++){\
		prev_val = *val++;\
		DeltaTpe(TPE) current_unsigned_delta;\
		if (*val > prev_val) {\
			current_unsigned_delta = GET_DELTA(TPE, *val, prev_val);\
		}\
		else {\
			current_unsigned_delta = GET_DELTA(TPE, prev_val, *val);\
		}\
\
		if (current_unsigned_delta > unsigned_delta) {\
			int current_bits = bits;\
			while (current_unsigned_delta > ((DeltaTpe(TPE))(-1)) >> (sizeof(DeltaTpe(TPE)) * CHAR_BIT - current_bits) ) {\
				/*keep track of number of BITS necessary to store the difference*/\
				current_bits++;\
			}\
			int current_bits_with_sign_bit = current_bits + 1;\
			if ( (current_bits_with_sign_bit >= (int) ((sizeof(TPE) * CHAR_BIT) / 2))\
				/*If we can from here on not compress better then the half of the original data type, we give up. */\
				|| (current_bits_with_sign_bit > (int) sizeof(BitVectorChunk) * CHAR_BIT) ) {\
				/*TODO: this extra condition should be removed once bitvector is extended to int64's*/\
				break;\
			}\
			bits = current_bits;\
			unsigned_delta = current_unsigned_delta;\
		}\
	}\
\
	/*Add the additional sign bit to the bit count.*/\
	bits++;\
	(PARAMETERS).base.rec.cnt = i;\
	(PARAMETERS).bits = bits;\
} while(0)

#define MOSestimate_DEF(TPE) \
MOSestimate_SIGNATURE(delta, TPE)\
{\
	(void) previous;\
	current->is_applicable = true;\
	current->compression_strategy.tag = MOSAIC_DELTA;\
	TPE *src = getSrc(TPE, task);\
	BUN limit = task->stop - task->start > MOSAICMAXCNT? MOSAICMAXCNT: task->stop - task->start;\
	MOSBlockHeaderTpe(delta, TPE) parameters;\
	determineDeltaParameters(parameters, src, limit, TPE);\
	assert(parameters.base.rec.cnt > 0);/*Should always compress.*/\
	current->uncompressed_size += (BUN) (parameters.base.rec.cnt * sizeof(TPE));\
	current->compressed_size += 2 * sizeof(MOSBlockHeaderTpe(delta, TPE)) + wordaligned((parameters.base.rec.cnt * parameters.bits) / CHAR_BIT, lng);\
	current->compression_strategy.cnt = (unsigned int) parameters.base.rec.cnt;\
\
	if (parameters.base.rec.cnt > *current->max_compression_length ) {\
		*current->max_compression_length = parameters.base.rec.cnt;\
	}\
\
	return MAL_SUCCEED;\
}

MOSestimate_DEF(bte)
MOSestimate_DEF(sht)
MOSestimate_DEF(int)
MOSestimate_DEF(lng)
#ifdef HAVE_HGE
MOSestimate_DEF(hge)
#endif

#define MOSpostEstimate_DEF(TPE)\
MOSpostEstimate_SIGNATURE(delta, TPE)\
{\
	(void) task;\
}

MOSpostEstimate_DEF(bte)
MOSpostEstimate_DEF(sht)
MOSpostEstimate_DEF(int)
MOSpostEstimate_DEF(lng)
#ifdef HAVE_HGE
MOSpostEstimate_DEF(hge)
#endif

// rather expensive simple value non-compressed store
#define MOScompress_DEF(TPE)\
MOScompress_SIGNATURE(delta, TPE)\
{\
	ALIGN_BLOCK_HEADER(task,  delta, TPE);\
\
	MosaicBlk blk = task->blk;\
	MOSsetTag(blk,MOSAIC_DELTA);\
	MOSsetCnt(blk, 0);\
	TPE *src = getSrc(TPE, task);\
	BUN i = 0;\
	BUN limit = estimate->cnt;\
	MOSBlockHeaderTpe(delta, TPE)* parameters = (MOSBlockHeaderTpe(delta, TPE)*) (task)->blk;\
	determineDeltaParameters(*parameters, src, limit, TPE);\
	BitVector base = MOScodevectorDelta(task, TPE);\
	task->dst = (char*) base;\
	TPE pv = parameters->init; /*previous value*/\
	/*Initial delta is zero.*/\
	setBitVector(base, 0, parameters->bits, (BitVectorChunk) 0);\
	DeltaTpe(TPE) sign_mask = (DeltaTpe(TPE)) ((IPTpe(TPE)) 1) << (parameters->bits - 1);\
\
	for(i = 1; i < MOSgetCnt(task->blk); i++) {\
		/*TODO: assert that delta's actually does not cause an overflow. */\
		TPE cv = *++src; /*current value*/\
		DeltaTpe(TPE) delta = (DeltaTpe(TPE)) (cv > pv ? (IPTpe(TPE)) (cv - pv) : (IPTpe(TPE)) ((sign_mask) | (IPTpe(TPE)) (pv - cv)));\
		setBitVector(base, i, parameters->bits, (BitVectorChunk) /*TODO: fix this once we have increased capacity of bitvector*/ delta);\
		pv = cv;\
	}\
	task->dst += BitVectorSize(i, parameters->bits);\
}

MOScompress_DEF(bte)
MOScompress_DEF(sht)
MOScompress_DEF(int)
MOScompress_DEF(lng)
#ifdef HAVE_HGE
MOScompress_DEF(hge)
#endif

#define MOSdecompress_DEF(TPE) \
MOSdecompress_SIGNATURE(delta, TPE)\
{\
	MOSBlockHeaderTpe(delta, TPE)* parameters = (MOSBlockHeaderTpe(delta, TPE)*) (task)->blk;\
	BUN lim = MOSgetCnt(task->blk);\
	((TPE*)task->src)[0] = parameters->init; /*previous value*/\
	BitVector base = (BitVector) MOScodevectorDelta(task, TPE);\
	DeltaTpe(TPE) sign_mask = (DeltaTpe(TPE)) ((IPTpe(TPE)) 1) << (parameters->bits - 1);\
	DeltaTpe(TPE) acc = (DeltaTpe(TPE)) parameters->init /*unsigned accumulating value*/;\
	BUN i;\
	for(i = 0; i < lim; i++) {\
		DeltaTpe(TPE) delta = getBitVector(base, i, parameters->bits);\
		((TPE*)task->src)[i] = ACCUMULATE(acc, delta, sign_mask, TPE);\
	}\
	task->src += i * sizeof(TPE);\
}

MOSdecompress_DEF(bte)
MOSdecompress_DEF(sht)
MOSdecompress_DEF(int)
MOSdecompress_DEF(lng)
#ifdef HAVE_HGE
MOSdecompress_DEF(hge)
#endif

#define scan_loop_delta(TPE, CANDITER_NEXT, TEST) \
{\
	MOSBlockHeaderTpe(delta, TPE)* parameters = (MOSBlockHeaderTpe(delta, TPE)*) task->blk;\
	BitVector base = (BitVector) MOScodevectorDelta(task, TPE);\
	DeltaTpe(TPE) acc = (DeltaTpe(TPE)) parameters->init; /*previous value*/\
	int bits = parameters->bits;\
	DeltaTpe(TPE) sign_mask = (DeltaTpe(TPE)) ((IPTpe(TPE)) 1) << (bits - 1);\
    v = (TPE) acc;\
    BUN j = 0;\
    for (oid c = canditer_peekprev(task->ci); !is_oid_nil(c) && c < last; c = CANDITER_NEXT(task->ci)) {\
        BUN i = (BUN) (c - first);\
        for (;j <= i; j++) {\
            TPE delta = getBitVector(base, j, bits);\
			v = ACCUMULATE(acc, delta, sign_mask, TPE);\
        }\
        /*TODO: change from control to data dependency.*/\
        if (TEST)\
            *o++ = c;\
    }\
}

MOSselect_DEF(delta, bte)
MOSselect_DEF(delta, sht)
MOSselect_DEF(delta, int)
MOSselect_DEF(delta, lng)
#ifdef HAVE_HGE
MOSselect_DEF(delta, hge)
#endif

#define projection_loop_delta(TPE, CANDITER_NEXT)\
{\
	MOSBlockHeaderTpe(delta, TPE)* parameters = (MOSBlockHeaderTpe(delta, TPE)*) task->blk;\
	BitVector base = (BitVector) MOScodevectorDelta(task, TPE);\
	DeltaTpe(TPE) acc = (DeltaTpe(TPE)) parameters->init; /*previous value*/\
	int bits = parameters->bits;\
	DeltaTpe(TPE) sign_mask = (DeltaTpe(TPE)) ((IPTpe(TPE)) 1) << (bits - 1);\
    TPE v = (TPE) acc;\
    BUN j = 0;\
	for (oid o = canditer_peekprev(task->ci); !is_oid_nil(o) && o < last; o = CANDITER_NEXT(task->ci)) {\
        BUN i = (BUN) (o - first);\
        for (;j <= i; j++) {\
            TPE delta = getBitVector(base, j, bits);\
			v = ACCUMULATE(acc, delta, sign_mask, TPE);\
        }\
		*bt++ = v;\
		task->cnt++;\
	}\
}

MOSprojection_DEF(delta, bte)
MOSprojection_DEF(delta, sht)
MOSprojection_DEF(delta, int)
MOSprojection_DEF(delta, lng)
#ifdef HAVE_HGE
MOSprojection_DEF(delta, hge)
#endif

#define outer_loop_delta(HAS_NIL, NIL_MATCHES, TPE, LEFT_CI_NEXT, RIGHT_CI_NEXT) \
{\
	MOSBlockHeaderTpe(delta, TPE)* parameters = (MOSBlockHeaderTpe(delta, TPE)*) task->blk;\
	BitVector base = (BitVector) MOScodevectorDelta(task, TPE);\
	DeltaTpe(TPE) acc = (DeltaTpe(TPE)) parameters->init; /*previous value*/\
	int bits = parameters->bits;\
	DeltaTpe(TPE) sign_mask = (DeltaTpe(TPE)) ((IPTpe(TPE)) 1) << (bits - 1);\
    TPE lval = (TPE) acc;\
    BUN j = 0;\
    for (oid lo = canditer_peekprev(task->ci); !is_oid_nil(lo) && lo < last; lo = LEFT_CI_NEXT(task->ci)) {\
        BUN i = (BUN) (lo - first);\
        for (;j <= i; j++) {\
            TPE delta = getBitVector(base, j, bits);\
			lval = ACCUMULATE(acc, delta, sign_mask, TPE);\
        }\
		if (HAS_NIL && !NIL_MATCHES) {\
			if ((IS_NIL(TPE, lval))) {continue;};\
		}\
		INNER_LOOP_UNCOMPRESSED(HAS_NIL, TPE, RIGHT_CI_NEXT);\
	}\
}

MOSjoin_COUI_DEF(delta, bte)
MOSjoin_COUI_DEF(delta, sht)
MOSjoin_COUI_DEF(delta, int)
MOSjoin_COUI_DEF(delta, lng)
#ifdef HAVE_HGE
MOSjoin_COUI_DEF(delta, hge)
#endif
