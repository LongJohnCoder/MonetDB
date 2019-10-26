/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2018 MonetDB B.V.
 */


/*
 * 2014-2016 author Martin Kersten
 * Global dictionary encoding
 * Index value zero is not used to easy detection of filler values
 * The dictionary index size is derived from the number of entries covered.
 * It leads to a compact n-bit representation.
 * Floating points are not expected to be replicated.
 * A limit of 256 elements is currently assumed.
 */

#include "monetdb_config.h"
#include "gdk.h"
#include "gdk_bitvector.h"
#include "mosaic.h"
#include "mosaic_capped.h"
#include "mosaic_dictionary.h"
#include "mosaic_private.h"
#include "gdk.h"
#include "group.h"

bool MOStypes_capped(BAT* b) {
	switch (b->ttype){
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

#define CAPPEDDICT 256

// Create a larger capped buffer then we allow for in the mosaic header first
// Store the most frequent ones in the compressed heap header directly based on estimated savings
// Improve by using binary search rather then linear scan
#define TMPDICT 16*CAPPEDDICT

typedef union{
	bte valbte[TMPDICT];
	sht valsht[TMPDICT];
	int valint[TMPDICT];
	lng vallng[TMPDICT];
	oid valoid[TMPDICT];
	flt valflt[TMPDICT];
	dbl valdbl[TMPDICT];
#ifdef HAVE_HGE
	hge valhge[TMPDICT];
#endif
} _DictionaryData;

typedef struct _CappedParameters_t {
	MosaicBlkRec base;
} MosaicBlkHeader_capped_t;

typedef struct _GlobalCappedInfo {
	BAT* dict;
	BAT* temp_dict;
	EstimationParameters parameters;
} GlobalCappedInfo;

#define GetBase(INFO, TPE)			((TPE*) Tloc((INFO)->dict, 0))
#define GetCount(INFO)				(BATcount((INFO)->dict))
#define GetTypeWidth(INFO)			((INFO)->dict->twidth)
#define GetSizeInBytes(INFO)		(BATcount((INFO)->dict) * GetTypeWidth(INFO))
#define GetCap(INFO)				(BATcapacity((INFO)->dict))
#define GetDeltaCount(INFO)			((INFO)->parameters.delta_count)
#define GetBits(INFO)				((INFO)->parameters.bits)
#define GetBitsExtended(INFO)		((INFO)->parameters.bits_extended)
#define Extend(INFO, new_capacity)	(BATextend((INFO)->dict, new_capacity) == GDK_SUCCEED)
#define GetValue(info, key, TPE) 	((GetBase(info, TPE))[key])
#define PresentInTempDictFuncDef(TPE) \
static inline \
bool presentInTempDict##TPE(GlobalCappedInfo* info, TPE val) {\
	unsigned int m, f= 0, l = BATcount(info->temp_dict); \
	TPE* dict = (TPE*) Tloc(info->temp_dict, 0);\
	while( l-f > 0 ) { \
		m = f + (l-f)/2;\
		if ( val < dict[m] ) l=m-1; else f= m;\
		if ( val > dict[m] ) f=m+1; else l= m;\
	}\
	return f < BATcount(info->temp_dict) && dict[f] == val;\
}

#define PresentInTempDict(INFO, VAL, TPE) presentInTempDict##TPE((INFO), (VAL))

#define DerivedDictionaryClass(TPE)\
PresentInTempDictFuncDef(TPE)\
DictionaryClass(TPE, GlobalCappedInfo, GetBase, GetCount, GetDeltaCount, GetBits, GetBitsExtended, GetCap, GetValue, Extend, PresentInTempDict)

DerivedDictionaryClass(bte)
DerivedDictionaryClass(sht)
PresentInTempDictFuncDef(int)static inline BUN _findValue_int(int* dict, BUN dict_count, int val) { int m, f= 0, l = dict_count; while( l-f > 0 ) { m = f + (l-f)/2; if ( val < dict[m]) l=m-1; else f= m; if ( val > dict[m]) f=m+1; else l= m; } return f;}static inline int _getValue_int(GlobalCappedInfo* info, BUN key) { return GetValue(info, key, int);}static str estimateDict_int(BUN* nr_compressed, BUN* delta_count, BUN limit, GlobalCappedInfo* info, int* val) { size_t buffer_size = 256; int* dict = (int*) GetBase(info, int); BUN dict_cnt = GetCount(info); int* delta = dict + dict_cnt; *delta_count = 0; for((*nr_compressed) = 0; (*nr_compressed)< limit; (*nr_compressed)++, val++) { BUN pos = _findValue_int(dict, dict_cnt, *val); if (pos == dict_cnt || _getValue_int(info, pos) != *val) { ; if (PresentInTempDict(info, *val, int)) { BUN key = _findValue_int(delta, (*delta_count), *val); if (key < *delta_count && delta[key] == *val) { continue; } if (dict_cnt + *delta_count + 1 == GetCap(info)) { if( !Extend(info, dict_cnt + *delta_count + (buffer_size <<=1)) ) throw(MAL, "mosaic.var", MAL_MALLOC_FAIL); dict = GetBase(info, int); delta = dict + dict_cnt; } int w = *val; for( ; key< *delta_count; key++) { if (delta[key] > w){ int v = delta[key]; delta[key] = w; w = v; } } delta[key] = w; (*delta_count)++; } else break; } } GetDeltaCount(info) = (*delta_count); BUN new_count = dict_cnt + GetDeltaCount(info); GetBitsExtended(info) = calculateBits(new_count); return MAL_SUCCEED;}static void _mergeDeltaIntoDictionary_int(GlobalCappedInfo* info) { int* delta = (int*) GetBase(info, int) + GetCount(info); if (GetCount(info) == 0) { ++delta; GetCount(info)++; GetDeltaCount(info)--; } BUN limit = GetDeltaCount(info); for (BUN i = 0; i < limit; i++) { BUN key = _findValue_int(GetBase(info, int), GetCount(info), delta[i]); if (key < GetCount(info) && GetValue(info, key, int) == delta[i]) { continue; } int w = delta[i]; for( ; key< GetCount(info); key++) { if (GetValue(info, key, int) > w){ int v =GetValue(info, key, int); GetValue(info, key, int)= w; w = v; } } GetCount(info)++; GetValue(info, key, int)= w; } GetBits(info) = GetBitsExtended(info);}
static void _compress_dictionary_int(int* dict, BUN dict_size, BUN* i, int* val, BUN limit, BitVector base, bte bits) {
	for((*i) = 0; (*i) < limit; (*i)++, val++) {
		BUN key = _findValue_int(dict, dict_size, *val);
		assert(key < dict_size);
		setBitVector(base, (*i), bits, (unsigned int) key);
	}
}
static void _decompress_dictionary_int(int* dict, bte bits, BitVector base, BUN limit, int** dest) { for(BUN i = 0; i < limit; i++){ size_t key = getBitVector(base,i,(int) bits); (*dest)[i] = dict[key]; } *dest += limit;}


DerivedDictionaryClass(oid)
DerivedDictionaryClass(lng)
DerivedDictionaryClass(flt)
DerivedDictionaryClass(dbl)
#ifdef HAVE_HGE
DerivedDictionaryClass(hge)
#endif

void
MOSadvance_capped(MOStask task)
{
	int *dst = (int*)  MOScodevectorDict(task);
	BUN cnt = MOSgetCnt(task->blk);
	long bytes;

	assert(cnt > 0);
	task->start += (oid) cnt;
	task->stop = task->stop;
	bytes =  (long) (cnt * task->hdr->bits_capped)/8 + (((cnt * task->hdr->bits_capped) %8) != 0);
	task->blk = (MosaicBlk) (((char*) dst)  + wordaligned(bytes, int));
}

void
MOSlayout_capped_hdr(MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties)
{
	lng zero=0;
	int i;
	char buf[BUFSIZ];
	char bufv[BUFSIZ];

	for(i=0; i< task->hdr->dictsize; i++){
		snprintf(buf, BUFSIZ,"capped[%d]",i);
		if( BUNappend(btech, buf, false) != GDK_SUCCEED ||
			BUNappend(bcount, &zero, false) != GDK_SUCCEED ||
			BUNappend(binput, &zero, false) != GDK_SUCCEED ||
			BUNappend(boutput, &task->hdr->dictfreq[i], false) != GDK_SUCCEED ||
			BUNappend(bproperties, bufv, false) != GDK_SUCCEED)
		return;
	}
}


void
MOSlayout_capped(MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties)
{
	MosaicBlk blk = task->blk;
	lng cnt = MOSgetCnt(blk), input=0, output= 0;

	input = cnt * ATOMsize(task->type);
	output =  MosaicBlkSize + (cnt * task->hdr->bits_capped)/8 + (((cnt * task->hdr->bits_capped) %8) != 0);
	if( BUNappend(btech, "capped blk", false) != GDK_SUCCEED ||
		BUNappend(bcount, &cnt, false) != GDK_SUCCEED ||
		BUNappend(binput, &input, false) != GDK_SUCCEED ||
		BUNappend(boutput, &output, false) != GDK_SUCCEED ||
		BUNappend(bproperties, "", false) != GDK_SUCCEED)
		return;
}

void
MOSskip_capped(MOStask task)
{
	MOSadvance_capped(task);
	if ( MOSgetTag(task->blk) == MOSAIC_EOL)
		task->blk = 0; // ENDOFLIST
}

#define MOSfind(Res,DICT,VAL,F,L)\
{ int m,f= F, l=L; \
   while( l-f > 0 ) { \
	m = f + (l-f)/2;\
	if ( VAL < DICT[m] ) l=m-1; else f= m;\
	if ( VAL > DICT[m] ) f=m+1; else l= m;\
   }\
   Res= f;\
}

str
MOSprepareEstimate_capped(MOStask task)
{
	str error;

	GlobalCappedInfo** info = &task->capped_info;
	BAT* source = task->bsrc;

	if ( (*info = GDKmalloc(sizeof(GlobalCappedInfo))) == NULL ) {
		throw(MAL,"mosaic.capped",MAL_MALLOC_FAIL);	
	}

	BAT *ngid, *next, *freq;

	BAT * source_view;
	if ((source_view = VIEWcreate(source->hseqbase, source)) == NULL) {
		throw(MAL, "mosaic.createGlobalDictInfo.VIEWcreate", GDK_EXCEPTION);
	}

	if (BATgroup(&ngid, &next, &freq, source_view, NULL, NULL, NULL, NULL) != GDK_SUCCEED) {
		BBPunfix(source_view->batCacheid);
		throw(MAL, "mosaic.createGlobalDictInfo.BATgroup", GDK_EXCEPTION);
	}
	BBPunfix(source_view->batCacheid);
	BBPunfix(ngid->batCacheid);

	BAT *cand_capped_dict;
	if (BATfirstn(&cand_capped_dict, NULL, freq, NULL, NULL, CAPPEDDICT, true, true, false) != GDK_SUCCEED) {
		BBPunfix(next->batCacheid);
		BBPunfix(freq->batCacheid);
		error = createException(MAL, "mosaic.capped.BATfirstn_unique", GDK_EXCEPTION);
		return error;
	}
	BBPunfix(freq->batCacheid);

	BAT* dict;
	if ( (dict = BATproject(next, source)) == NULL) {
		BBPunfix(next->batCacheid);
		BBPunfix(cand_capped_dict->batCacheid);
		throw(MAL, "mosaic.createGlobalDictInfo.BATproject", GDK_EXCEPTION);
	}
	BBPunfix(next->batCacheid);

	BAT *capped_dict;
	if ((capped_dict = BATproject(cand_capped_dict, dict)) == NULL) {
		BBPunfix(cand_capped_dict->batCacheid);
		BBPunfix(dict->batCacheid);
		error = createException(MAL, "mosaic.capped.BATproject", GDK_EXCEPTION);
		return error;
	}
	BBPunfix(cand_capped_dict->batCacheid);
	BBPunfix(dict->batCacheid);

	BAT* sorted_capped_dict;
	if (BATsort(&sorted_capped_dict, NULL, NULL, capped_dict, NULL, NULL, false, true, false) != GDK_SUCCEED) {
		BBPunfix(capped_dict->batCacheid);
		error = createException(MAL, "mosaic.capped.BATfirstn_unique", GDK_EXCEPTION);
		return error;
	}
	BBPunfix(capped_dict->batCacheid);

	BAT* final_capped_dict;
	if ((final_capped_dict = COLnew(0, sorted_capped_dict->ttype, 0, TRANSIENT)) == NULL) {
		BBPunfix(sorted_capped_dict->batCacheid);
		error = createException(MAL, "mosaic.capped.COLnew", GDK_EXCEPTION);
		return error;
	}

	(*info)->temp_dict = sorted_capped_dict;
	(*info)->dict = final_capped_dict;

	return MAL_SUCCEED;
}

#define estimateDict(TASK, CURRENT, TPE)\
do {\
	/*TODO*/\
	GlobalCappedInfo* info = TASK->capped_info;\
	BUN limit = (TASK)->stop - (TASK)->start > MOSAICMAXCNT? MOSAICMAXCNT: (TASK)->stop - (TASK)->start;\
	TPE* val = getSrc(TPE, (TASK));\
	BUN delta_count;\
	BUN nr_compressed;\
\
	size_t old_keys_size	= ((CURRENT)->nr_capped_encoded_elements * GetBitsExtended(info)) / CHAR_BIT;\
	size_t old_dict_size	= GetCount(info) * sizeof(TPE);\
	size_t old_headers_size	= (CURRENT)->nr_capped_encoded_blocks * (MosaicBlkSize + sizeof(TPE));\
	size_t old_bytes		= old_keys_size + old_dict_size + old_headers_size;\
\
	if (estimateDict_##TPE(&nr_compressed, &delta_count, limit, info, val)) {\
		throw(MAL, "mosaic.capped", MAL_MALLOC_FAIL);\
	}\
\
	(CURRENT)->is_applicable = nr_compressed > 0;\
	(CURRENT)->nr_capped_encoded_elements += nr_compressed;\
	(CURRENT)->nr_capped_encoded_blocks++;\
\
	size_t new_keys_size	= ((CURRENT)->nr_capped_encoded_elements * GetBitsExtended(info)) / CHAR_BIT;\
	size_t new_dict_size	= (delta_count + GetCount(info)) * sizeof(TPE);\
	size_t new_headers_size	= (CURRENT)->nr_capped_encoded_blocks * (MosaicBlkSize + sizeof(TPE));\
	size_t new_bytes		= new_keys_size + new_dict_size + new_headers_size;\
\
	(CURRENT)->compression_strategy.tag = MOSAIC_CAPPED;\
	(CURRENT)->compression_strategy.cnt = nr_compressed;\
\
	(CURRENT)->uncompressed_size	+= (BUN) ( nr_compressed * sizeof(TPE));\
	(CURRENT)->compressed_size		+= (wordaligned( MosaicBlkSize, BitVector) + new_bytes - old_bytes);\
} while (0)

// calculate the expected reduction using DICT in terms of elements compressed
str
MOSestimate_capped(MOStask task, MosaicEstimation* current, const MosaicEstimation* previous)
{
	(void) previous;

	switch(ATOMbasetype(task->type)){
	case TYPE_bte: estimateDict(task, current, bte); break;
	case TYPE_sht: estimateDict(task, current, sht); break;
	case TYPE_int: estimateDict(task, current, int); break;
	case TYPE_oid: estimateDict(task, current, oid); break;
	case TYPE_lng: estimateDict(task, current, lng); break;
	case TYPE_flt: estimateDict(task, current, flt); break;
	case TYPE_dbl: estimateDict(task, current, dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: estimateDict(task, current, hge); break;
#endif
	}
	return MAL_SUCCEED;
}

#define postEstimate(TASK, TPE) _mergeDeltaIntoDictionary_##TPE( (TASK)->capped_info)

void
MOSpostEstimate_capped(MOStask task) {
	switch(ATOMbasetype(task->type)){
	case TYPE_bte: postEstimate(task, bte); break; 
	case TYPE_sht: postEstimate(task, sht); break;
	case TYPE_int: postEstimate(task, int); break;
	case TYPE_oid: postEstimate(task, oid); break;
	case TYPE_lng: postEstimate(task, lng); break;
	case TYPE_flt: postEstimate(task, flt); break;
	case TYPE_dbl: postEstimate(task, dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: postEstimate(task, hge); break;
#endif
	}
}

static str
_finalizeDictionary(BAT* b, GlobalCappedInfo* info, ulng* pos_dict, ulng* length_dict, bte* bits_dict) {
	Heap* vmh = b->tvmosaic;
	BUN size_in_bytes = vmh->free + GetSizeInBytes(info);
	if (HEAPextend(vmh, size_in_bytes, true) != GDK_SUCCEED) {
		throw(MAL, "mosaic.mergeDictionary_capped.HEAPextend", GDK_EXCEPTION);
	}
	char* dst = vmh->base + vmh->free;
	char* src = info->dict->theap.base;
	/* TODO: consider optimizing this by swapping heaps instead of copying them.*/
	memcpy(dst, src, GetSizeInBytes(info));

	assert(vmh->free % GetTypeWidth(info) == 0);
	*pos_dict = (vmh->free / GetTypeWidth(info));

	vmh->free += GetSizeInBytes(info);
	vmh->dirty = true;

	*length_dict = GetCount(info);
	*bits_dict = calculateBits(*length_dict);

	BBPreclaim(info->dict);
	BBPreclaim(info->temp_dict);

	GDKfree(info);

	return MAL_SUCCEED;
}

str
finalizeDictionary_capped(MOStask task) {
	return _finalizeDictionary(
		task->bsrc,
		task->capped_info,
		&task->hdr->pos_capped,
		&task->hdr->length_capped,
		&task->hdr->bits_capped);
}

#define GetFinalCappedDict(TASK, TPE) (((TPE*) (TASK)->bsrc->tvmosaic->base) + (TASK)->hdr->pos_capped)

// insert a series of values into the compressor block using dictionary

#define DICTcompress(TASK, TPE) {\
	TPE *val = getSrc(TPE, (TASK));\
	unsigned int limit = estimate->cnt;\
	(TASK)->dst = MOScodevectorDict(TASK);\
	BitVector base = (BitVector) ((TASK)->dst);\
	BUN i;\
	TPE* dict = GetFinalCappedDict(TASK, TPE);\
	BUN dict_size = (BUN) (TASK)->hdr->length_capped;\
	bte bits = (TASK)->hdr->bits_capped;\
	_compress_dictionary_##TPE(dict, dict_size, &i, val, limit, base, bits);\
	MOSsetCnt((TASK)->blk, i);\
}

void
MOScompress_capped(MOStask task, MosaicBlkRec* estimate)
{
	MosaicBlk blk = task->blk;

	task->dst = MOScodevectorDict(task);

	MOSsetTag(blk,MOSAIC_CAPPED);
	MOSsetCnt(blk,0);

	switch(ATOMbasetype(task->type)){
	case TYPE_bte: DICTcompress(task, bte); break;
	case TYPE_sht: DICTcompress(task, sht); break;
	case TYPE_int: DICTcompress(task, int); break;
	case TYPE_lng: DICTcompress(task, lng); break;
	case TYPE_oid: DICTcompress(task, oid); break;
	case TYPE_flt: DICTcompress(task, flt); break;
	case TYPE_dbl: DICTcompress(task, dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: DICTcompress(task, hge); break;
#endif
	}
}

// the inverse operator, extend the src

#define DICTdecompress(TASK, TPE)\
{	BUN lim = MOSgetCnt((TASK)->blk);\
	BitVector base = (BitVector) MOScodevectorDict(TASK);\
	bte bits	= (TASK)->hdr->bits_capped;\
	TPE* dict = GetFinalCappedDict(TASK, TPE);\
	TPE* dest = (TPE*) (TASK)->src;\
	_decompress_dictionary_##TPE(dict, bits, base, lim, &dest);\
}

void
MOSdecompress_capped(MOStask task)
{
	switch(ATOMbasetype(task->type)){
	case TYPE_bte: DICTdecompress(task, bte); break;
	case TYPE_sht: DICTdecompress(task, sht); break;
	case TYPE_int: DICTdecompress(task, int); break;
	case TYPE_lng: DICTdecompress(task, lng); break;
	case TYPE_oid: DICTdecompress(task, oid); break;
	case TYPE_flt: DICTdecompress(task, flt); break;
	case TYPE_dbl: DICTdecompress(task, dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: DICTdecompress(task, hge); break;
#endif
	}
}

// perform relational algebra operators over non-compressed chunks
// They are bound by an oid range and possibly a candidate list

/* TODO: the select_operator for dictionaries doesn't use
 * the ordered property of the actual global dictionary.
 * Which is a shame because it could in this select operator
 * safe on the dictionary look ups by using the dictionary itself
 * as a reverse index for the ranges of your select predicate.
 * Or if we want to stick to this set up, then we should use a
 * hash based dictionary.
*/

#define select_capped_str(TPE) \
	throw(MAL,"mosaic.capped","TBD");
#define select_capped(TPE) {\
	BitVector base = (BitVector) MOScodevectorDict(task);\
	bte bits	= task->hdr->bits_capped;\
	TPE* dict = GetFinalCappedDict(task, TPE);\
	if( !*anti){\
		if( is_nil(TPE, *(TPE*) low) && is_nil(TPE, *(TPE*) hgh)){\
			for( ; first < last; first++){\
				MOSskipit();\
				*o++ = (oid) first;\
			}\
		} else\
		if( is_nil(TPE, *(TPE*) low) ){\
			for(i=0 ; first < last; first++, i++){\
				MOSskipit();\
				j= getBitVector(base,i,bits); \
				cmp  =  ((*hi && dict[j] <= * (TPE*)hgh ) || (!*hi && dict[j] < *(TPE*)hgh ));\
				if (cmp )\
					*o++ = (oid) first;\
			}\
		} else\
		if( is_nil(TPE, *(TPE*) hgh) ){\
			for(i=0; first < last; first++, i++){\
				MOSskipit();\
				j= getBitVector(base,i,bits); \
				cmp  =  ((*li && dict[j] >= * (TPE*)low ) || (!*li && dict[j] > *(TPE*)low ));\
				if (cmp )\
					*o++ = (oid) first;\
			}\
		} else{\
			for(i=0 ; first < last; first++, i++){\
				MOSskipit();\
				j= getBitVector(base,i,bits); \
				cmp  =  ((*hi && dict[j] <= * (TPE*)hgh ) || (!*hi && dict[j] < *(TPE*)hgh )) &&\
						((*li && dict[j] >= * (TPE*)low ) || (!*li && dict[j] > *(TPE*)low ));\
				if (cmp )\
					*o++ = (oid) first;\
			}\
		}\
	} else {\
		if( is_nil(TPE, *(TPE*) low) && is_nil(TPE, *(TPE*) hgh)){\
			/* nothing is matching */\
		} else\
		if( is_nil(TPE, *(TPE*) low) ){\
			for(i=0 ; first < last; first++, i++){\
				MOSskipit();\
				j= getBitVector(base,i,bits); \
				cmp  =  ((*hi && dict[j] <= * (TPE*)hgh ) || (!*hi && dict[j] < *(TPE*)hgh ));\
				if ( !cmp )\
					*o++ = (oid) first;\
			}\
		} else\
		if( is_nil(TPE, *(TPE*) hgh) ){\
			for(i=0 ; first < last; first++, i++){\
				MOSskipit();\
				j= getBitVector(base,i,bits); \
				cmp  =  ((*li && dict[j] >= * (TPE*)low ) || (!*li && dict[j] > *(TPE*)low ));\
				if ( !cmp )\
					*o++ = (oid) first;\
			}\
		} else{\
			for(i=0 ; first < last; first++, i++){\
				MOSskipit();\
				j= getBitVector(base,i,bits); \
				cmp  =  ((*hi && dict[j] <= * (TPE*)hgh ) || (!*hi && dict[j] < *(TPE*)hgh )) &&\
						((*li && dict[j] >= * (TPE*)low ) || (!*li && dict[j] > *(TPE*)low ));\
				if ( !cmp )\
					*o++ = (oid) first;\
			}\
		}\
	}\
}

str
MOSselect_capped(MOStask task, void *low, void *hgh, bit *li, bit *hi, bit *anti)
{
	oid *o;
	BUN i, first,last;
	int cmp;
	bte j;

	// set the oid range covered and advance scan range
	first = task->start;
	last = first + MOSgetCnt(task->blk);

	if (task->cl && *task->cl > last){
		MOSskip_capped(task);
		return MAL_SUCCEED;
	}
	o = task->lb;

	switch(ATOMbasetype(task->type)){
	case TYPE_bte: select_capped(bte); break;
	case TYPE_sht: select_capped(sht); break;
	case TYPE_int: select_capped(int); break;
	case TYPE_lng: select_capped(lng); break;
	case TYPE_oid: select_capped(oid); break;
	case TYPE_flt: select_capped(flt); break;
	case TYPE_dbl: select_capped(dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: select_capped(hge); break;
#endif
	}
	MOSskip_capped(task);
	task->lb = o;
	return MAL_SUCCEED;
}

#define thetaselect_capped_str(TPE)\
	throw(MAL,"mosaic.capped","TBD");

#define thetaselect_capped(TPE)\
{\
	BitVector base = (BitVector) MOScodevectorDict(task);\
	bte bits	= task->hdr->bits_capped;\
	TPE* dict	= GetFinalCappedDict(task, TPE);\
	base		= (BitVector) MOScodevectorDict(task);\
	TPE low,hgh;\
	low= hgh = TPE##_nil;\
	if ( strcmp(oper,"<") == 0){\
		hgh= *(TPE*) val;\
		hgh = PREVVALUE##TPE(hgh);\
	} else\
	if ( strcmp(oper,"<=") == 0){\
		hgh= *(TPE*) val;\
	} else\
	if ( strcmp(oper,">") == 0){\
		low = *(TPE*) val;\
		low = NEXTVALUE##TPE(low);\
	} else\
	if ( strcmp(oper,">=") == 0){\
		low = *(TPE*) val;\
	} else\
	if ( strcmp(oper,"!=") == 0){\
		hgh= low= *(TPE*) val;\
		anti++;\
	} else\
	if ( strcmp(oper,"==") == 0){\
		hgh= low= *(TPE*) val;\
	} \
	for( ; first < last; first++){\
		MOSskipit();\
		j= getBitVector(base, first, bits); \
		if( (is_nil(TPE, low) || dict[j] >= low) && (dict[j] <= hgh || is_nil(TPE, hgh)) ){\
			if ( !anti) {\
				*o++ = (oid) first;\
			}\
		} else\
			if( anti){\
				*o++ = (oid) first;\
			}\
	}\
}

str
MOSthetaselect_capped( MOStask task, void *val, str oper)
{
	oid *o;
	int anti=0;
	BUN first,last;
	bte j;

	// set the oid range covered and advance scan range
	first = task->start;
	last = first + MOSgetCnt(task->blk);

	if (task->cl && *task->cl > last){
		MOSskip_capped(task);
		return MAL_SUCCEED;
	}
	o = task->lb;

	switch(ATOMbasetype(task->type)){
	case TYPE_bte: thetaselect_capped(bte); break;
	case TYPE_sht: thetaselect_capped(sht); break;
	case TYPE_int: thetaselect_capped(int); break;
	case TYPE_lng: thetaselect_capped(lng); break;
	case TYPE_oid: thetaselect_capped(oid); break;
	case TYPE_flt: thetaselect_capped(flt); break;
	case TYPE_dbl: thetaselect_capped(dbl); break;
#ifdef HAVE_HGE
	case TYPE_hge: thetaselect_capped(hge); break;
#endif
	}
	MOSskip_capped(task);
	task->lb =o;
	return MAL_SUCCEED;
}

#define projection_capped_str(TPE)\
	throw(MAL,"mosaic.capped","TBD");
#define projection_capped(TPE)\
{	TPE *v;\
	bte bits	= task->hdr->bits_capped;\
	TPE* dict	= GetFinalCappedDict(task, TPE);\
	BitVector base		= (BitVector) MOScodevectorDict(task);\
	v= (TPE*) task->src;\
	for(i=0; first < last; first++,i++){\
		MOSskipit();\
		j= getBitVector(base,i,bits); \
		*v++ = dict[j];\
		task->cnt++;\
	}\
	task->src = (char*) v;\
}

str
MOSprojection_capped( MOStask task)
{
	BUN i,first,last;
	unsigned short j;
	// set the oid range covered and advance scan range
	first = task->start;
	last = first + MOSgetCnt(task->blk);

	switch(ATOMbasetype(task->type)){
		case TYPE_bte: projection_capped(bte); break;
		case TYPE_sht: projection_capped(sht); break;
		case TYPE_int: projection_capped(int); break;
		case TYPE_lng: projection_capped(lng); break;
		case TYPE_oid: projection_capped(oid); break;
		case TYPE_flt: projection_capped(flt); break;
		case TYPE_dbl: projection_capped(dbl); break;
#ifdef HAVE_HGE
		case TYPE_hge: projection_capped(hge); break;
#endif
	}
	MOSskip_capped(task);
	return MAL_SUCCEED;
}

#define join_capped_str(TPE)\
	throw(MAL,"mosaic.capped","TBD");

#define join_capped(TPE)\
{	TPE  *w;\
	bte bits		= task->hdr->bits_capped;\
	TPE* dict		= GetFinalCappedDict(task, TPE);\
	BitVector base	= (BitVector) MOScodevectorDict(task);\
	w = (TPE*) task->src;\
	limit= MOSgetCnt(task->blk);\
	for( o=0, n= task->stop; n-- > 0; o++,w++ ){\
		for(oo = task->start,i=0; i < limit; i++,oo++){\
			j= getBitVector(base,i,bits); \
			if ( *w == dict[j]){\
				if(BUNappend(task->lbat, &oo, false) != GDK_SUCCEED ||\
				BUNappend(task->rbat, &o, false) != GDK_SUCCEED)\
				throw(MAL,"mosaic.capped",MAL_MALLOC_FAIL);\
			}\
		}\
	}\
}

str
MOSjoin_capped( MOStask task)
{
	BUN i,n,limit;
	oid o, oo;
	int j;

	// set the oid range covered and advance scan range
	switch(ATOMbasetype(task->type)){
		case TYPE_bte: join_capped(bte); break;
		case TYPE_sht: join_capped(sht); break;
		case TYPE_int: join_capped(int); break;
		case TYPE_lng: join_capped(lng); break;
		case TYPE_oid: join_capped(oid); break;
		case TYPE_flt: join_capped(flt); break;
		case TYPE_dbl: join_capped(dbl); break;
#ifdef HAVE_HGE
		case TYPE_hge: join_capped(hge); break;
#endif
	}
	MOSskip_capped(task);
	return MAL_SUCCEED;
}
