/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
 */

#include "monetdb_config.h"
#include "opt_reduce.h"
#include "mal_interpreter.h"
#include "gdk_tracer.h"

str
OPTreduceImplementation(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr p)
{
	str func_ln = "reduce_opt";
	int actions = 0;
	char buf[256];
	lng usec = GDKusec();
	(void)cntxt;
	(void)stk;
	(void) p;

	if( OPTdebug &  OPTreduce){
        TraceLN(M_DEBUG, func_ln, "REDUCE optimizer entry\n");
        fprintFunction(M_DEBUG, func_ln, mb, 0, LIST_MAL_ALL);
	}	

	actions = mb->vtop;
	trimMalVariables(mb,0);
	actions = actions - mb->vtop;

    /* Defense line against incorrect plans */
	/* plan is not changed */
	/* plan is not changed */
    //if( actions > 0){
        //chkTypes(cntxt->usermodule, mb, FALSE);
        //chkFlow(mb);
        //chkDeclarations(mb);
    //}
    /* keep all actions taken as a post block comment */
	usec = GDKusec()- usec;
    snprintf(buf,256,"%-20s actions=%2d time=" LLFMT " usec","reduce",actions, usec);
    newComment(mb,buf);
	if( actions >= 0)
		addtoMalBlkHistory(mb);

    if( OPTdebug &  OPTreduce){
        TraceLN(M_DEBUG, func_ln, "REDUCE optimizer exit\n");
        fprintFunction(M_DEBUG, func_ln, mb, 0, LIST_MAL_ALL);
    }
	return MAL_SUCCEED;
}
