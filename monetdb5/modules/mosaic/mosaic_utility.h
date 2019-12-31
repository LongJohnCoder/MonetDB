/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

/*
 * authors Martin Kersten, Aris Koning
 */
#ifndef _MOSAIC_UTILITY_
#define _MOSAIC_UTILITY_

#include "mosaic_select.h"
#include "mosaic_projection.h"
#include "mosaic_join.h"

#define MOSBlockHeaderTpe(NAME, TPE) MOSBlockHeader_##NAME##_##TPE
#define ALIGNMENT_HELPER_TPE(NAME, TPE) struct ALIGNMENT_HELPER_MOSBlockHeader_##NAME##_##TPE

#define ALIGNMENT_HELPER__DEF(NAME, TPE) \
ALIGNMENT_HELPER_TPE(NAME, TPE)\
{\
	char a;\
	MOSBlockHeaderTpe(NAME, TPE) b;\
};

#define MOSadvance_SIGNATURE(NAME, TPE) void MOSadvance_##NAME##_##TPE(MOStask task)
#define MOSestimate_SIGNATURE(NAME, TPE) str MOSestimate_##NAME##_##TPE(MOStask task, MosaicEstimation* current, const MosaicEstimation* previous)
#define MOSpostEstimate_SIGNATURE(NAME, TPE) void MOSpostEstimate_##NAME##_##TPE(MOStask task)
#define MOScompress_SIGNATURE(NAME, TPE) void MOScompress_##NAME##_##TPE(MOStask task, MosaicBlkRec* estimate)
#define MOSdecompress_SIGNATURE(NAME, TPE) void MOSdecompress_##NAME##_##TPE(MOStask task)
#define MOSBlockHeader_DEF(NAME, TPE) MosaicBlkHeader_DEF_##NAME(TPE)

#define ALGEBRA_INTERFACE(NAME, TPE) \
MOSadvance_SIGNATURE(NAME, TPE);\
MOSestimate_SIGNATURE(NAME, TPE);\
MOSpostEstimate_SIGNATURE(NAME, TPE);\
MOScompress_SIGNATURE(NAME, TPE);\
MOSdecompress_SIGNATURE(NAME, TPE);\
MOSselect_SIGNATURE(NAME, TPE);\
MOSprojection_SIGNATURE(NAME, TPE);\
MOSjoin_COUI_SIGNATURE(NAME, TPE);\
MOSBlockHeader_DEF(NAME, TPE);\
ALIGNMENT_HELPER__DEF(NAME, TPE);

#ifdef HAVE_HGE
#define ALGEBRA_INTERFACES_INTEGERS_ONLY(NAME) \
ALGEBRA_INTERFACE(NAME, bte);\
ALGEBRA_INTERFACE(NAME, sht);\
ALGEBRA_INTERFACE(NAME, int);\
ALGEBRA_INTERFACE(NAME, lng);\
ALGEBRA_INTERFACE(NAME, hge);
#else
#define ALGEBRA_INTERFACES_INTEGERS_ONLY(NAME) \
ALGEBRA_INTERFACE(NAME, bte);\
ALGEBRA_INTERFACE(NAME, sht);\
ALGEBRA_INTERFACE(NAME, int);\
ALGEBRA_INTERFACE(NAME, lng);
#endif

#define ALGEBRA_INTERFACES_ALL_TYPES(NAME) \
ALGEBRA_INTERFACES_INTEGERS_ONLY(NAME)\
ALGEBRA_INTERFACE(NAME, flt);\
ALGEBRA_INTERFACE(NAME, dbl);

// This is just an ugly work around for Microsoft Visual Studio to get the expansion of __VA_ARGS__ right.
#define EXPAND(X) X

#define DO_OPERATION_ON_INTEGERS_ONLY_bte(OPERATION, NAME, ...) EXPAND(OPERATION(NAME, bte, __VA_ARGS__))
#define DO_OPERATION_ON_INTEGERS_ONLY_sht(OPERATION, NAME, ...) EXPAND(OPERATION(NAME, sht, __VA_ARGS__))
#define DO_OPERATION_ON_INTEGERS_ONLY_int(OPERATION, NAME, ...) EXPAND(OPERATION(NAME, int, __VA_ARGS__))
#define DO_OPERATION_ON_INTEGERS_ONLY_lng(OPERATION, NAME, ...) EXPAND(OPERATION(NAME, lng, __VA_ARGS__))
#define DO_OPERATION_ON_INTEGERS_ONLY_flt(OPERATION, NAME, ...) assert(0)
#define DO_OPERATION_ON_INTEGERS_ONLY_dbl(OPERATION, NAME, ...) assert(0)
#ifdef HAVE_HGE
#define DO_OPERATION_ON_INTEGERS_ONLY_hge(OPERATION, NAME, ...) EXPAND(OPERATION(NAME, hge, __VA_ARGS__))
#endif

#define DO_OPERATION_ON_INTEGERS_ONLY(OPERATION, NAME, TPE, ...)    DO_OPERATION_ON_INTEGERS_ONLY_##TPE(OPERATION, NAME, __VA_ARGS__)
#define DO_OPERATION_ON_ALL_TYPES(OPERATION, NAME, TPE, ...)        EXPAND(OPERATION(NAME, TPE, __VA_ARGS__))

/*DUMMY_PARAM is just an ugly workaround for the fact that a variadic macro must have at least one variadic parameter*/
#define DO_OPERATION_IF_ALLOWED(OPERATION, NAME, TPE)               DO_OPERATION_ON_##NAME(OPERATION, TPE, 0 /*DUMMY_PARAM*/)
#define DO_OPERATION_IF_ALLOWED_VARIADIC(OPERATION, NAME, TPE, ...) DO_OPERATION_ON_##NAME(OPERATION, TPE, __VA_ARGS__)

#endif /* _MOSAIC_UTILITY_ */
