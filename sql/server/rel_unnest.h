/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.
 */

#ifndef _REL_UNNEST_H_
#define _REL_UNNEST_H_

#include "sql_relation.h"
#include "sql_mvc.h"

extern int exp_has_freevar(mvc *sql, sql_exp *e);
extern int exps_have_freevar(mvc *sql, list *exps);
extern int rel_has_freevar(mvc *sql, sql_rel *r);
extern sql_rel *rel_unnest(mvc *sql, sql_rel *rel);
extern void exps_set_freevar(mvc *sql, list *exps, sql_rel *r);

#endif /*_REL_UNNEST_H_*/
