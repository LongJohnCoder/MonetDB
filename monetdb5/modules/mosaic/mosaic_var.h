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

#ifndef _MOSAIC_VAR_
#define _MOSAIC_VAR_

#include <mal.h>
#include "mal_interpreter.h"
#include "mal_client.h"
#include "mosaic_select.h"

#define DICTTHRESHOLD 4192
#define DICTSIZE 256

bool MOStypes_var(BAT* b);
mal_export void MOScreatevar(MOStask task);
mal_export void MOSlayout_var(MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties);
mal_export void MOSlayout_var_hdr(MOStask task, BAT *btech, BAT *bcount, BAT *binput, BAT *boutput, BAT *bproperties);
mal_export void MOSadvance_var(MOStask task);
mal_export void MOSskip_var(MOStask task);
mal_export str MOSprepareEstimate_var(MOStask task);
mal_export str  MOSestimate_var(MOStask task, MosaicEstimation* current, const MosaicEstimation* previous);
mal_export void MOSpostEstimate_var(MOStask task);
mal_export str finalizeDictionary_var(MOStask task);
mal_export void MOScompress_var(MOStask task, MosaicBlkRec* estimate);
mal_export void MOSdecompress_var(MOStask task);
mal_export str MOSprojection_var( MOStask task);
mal_export str MOSjoin_var( MOStask task, bit nil_matches);

MOSselect_SIGNATURE(var, bte);
MOSselect_SIGNATURE(var, sht);
MOSselect_SIGNATURE(var, int);
MOSselect_SIGNATURE(var, lng);
MOSselect_SIGNATURE(var, flt);
MOSselect_SIGNATURE(var, dbl);
#ifdef HAVE_HGE
MOSselect_SIGNATURE(var, hge);
#endif

#define SELECT_VAR(TPE) do_select(var, TPE)

#endif /* _MOSAIC_VAR_ */
