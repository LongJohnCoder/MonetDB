/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2018 MonetDB B.V.
 */

/*
 * (c)2014 author Martin Kersten
 */

#ifndef _MOSAIC_RAW_
#define _MOSAIC_RAW_

#include <mal.h>
#include "mal_interpreter.h"
#include "mal_client.h"
#include "mosaic_utility.h"

bool MOStypes_raw(BAT* b);
mal_export void MOSlayout_raw(MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties);
mal_export void MOSadvance_raw(MOStask task);
mal_export str MOSestimate_raw(MOStask task, MosaicEstimation* current, const MosaicEstimation* previous);
mal_export void MOScompress_raw(MOStask task, MosaicBlkRec* estimate);
mal_export void MOSdecompress_raw(MOStask task);

ALGEBRA_INTERFACES_ALL_TYPES(raw);

#define DO_OPERATION_ON_raw(OPERATION, TPE, ...) DO_OPERATION_ON_ALL_TYPES(OPERATION, raw, TPE, __VA_ARGS__)

#define join_inner_loop_raw(TPE, HAS_NIL, RIGHT_CI_NEXT)\
{\
    TPE* vr = (TPE*) (((char*) task->blk) + MosaicBlkSize);\
    for (oid ro = canditer_peekprev(task->ci); !is_oid_nil(ro) && ro < last; ro = RIGHT_CI_NEXT(task->ci)) {\
        TPE rval = vr[ro-first];\
        IF_EQUAL_APPEND_RESULT(HAS_NIL, TPE);\
	}\
}

#endif /* _MOSAIC_RAW_ */
