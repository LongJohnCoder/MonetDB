/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2014 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * Foteini Alvanaki
 */

#include "geom.h"

/*******************************/
/********** One input **********/
/*******************************/

str geom_2_geom_bat(int* outBAT_id, int* inBAT_id, int* columnType, int* columnSRID) {
	BAT *outBAT = NULL, *inBAT = NULL;
	wkb *inWKB = NULL, *outWKB = NULL;
	BUN p =0, q =0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, "batcalc.wkb", RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batcalc.wkb", "the arguments must have dense and aligned heads");
	}

	//create a new BAT
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batcalc.wkb", MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the BAT	
	inBAT_iter = bat_iterator(inBAT);
	//for (i = 0; i < BATcount(inBAT); i++) { 
	BATloop(inBAT, p, q) { //iterate over all valid elements
		str err = NULL;

		//if for used --> inWKB = (wkb *) BUNtail(inBATi, i + BUNfirst(inBAT));
		inWKB = (wkb*) BUNtail(inBAT_iter, p);
		if ((err = geom_2_geom(&outWKB, &inWKB, columnType, columnSRID)) != MAL_SUCCEED) { //check type
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batcalc.wkb", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,outWKB,TRUE); //add the point to the new BAT
		GDKfree(outWKB);
		outWKB = NULL;
	}

	//set some properties of the new BAT
	BATsetcount(outBAT, BATcount(inBAT));
    	BATsettrivprop(outBAT);
    	BATderiveProps(outBAT,FALSE);
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	return MAL_SUCCEED;

}

/*create WKB from WKT */
str wkbFromText_bat(bat *outBAT_id, bat *inBAT_id, int *srid, int *tpe) {
	BAT *outBAT = NULL, *inBAT = NULL;
	char *inWKB = NULL;
	BUN p =0, q =0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, "batgeom.wkbFromText", RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.wkbFromText", "the arguments must have dense and aligned heads");
	}

	//create a new for the output BAT
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.wkbFromText", MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the input BAT	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { //iterate over all valid elements
		str err = NULL;
		wkb* outSingle;

		inWKB = (char*) BUNtail(inBAT_iter, p);
		if ((err = wkbFromText(&outSingle, &inWKB, srid, tpe)) != MAL_SUCCEED) {
			str msg = createException(MAL, "batgeom.wkbFromText", "%s", err);
			GDKfree(err);

			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			
			return msg;
		}
		BUNappend(outBAT,outSingle,TRUE); //add the result to the new BAT
		GDKfree(outSingle);
		outSingle = NULL;
	}

	//set the number of elements in the outBAT
	BATsetcount(outBAT, BATcount(inBAT));
	
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;
}

static str wkbStrOutWithFlag_bat(bat *outBAT_id, bat *inBAT_id, int* flag, str (*func)(char**, wkb**, int*), const char *name) {
	BAT *outBAT = NULL, *inBAT = NULL;
	wkb *inWKB = NULL;
	BUN p =0, q =0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, name, RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, name, "the arguments must have dense and aligned heads");
	}

	//create a new for the output BAT
	if ((outBAT = BATnew(TYPE_void, ATOMindex("str"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, name, MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the input BAT	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { //iterate over all valid elements
		str err = NULL;
		char* outSingle;

		inWKB = (wkb*) BUNtail(inBAT_iter, p);
		if ((err = (*func)(&outSingle, &inWKB, flag)) != MAL_SUCCEED) {
			str msg = createException(MAL, name, "%s", err);
			GDKfree(err);

			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			
			return msg;
		}
		BUNappend(outBAT,outSingle,TRUE); //add the result to the new BAT
		GDKfree(outSingle);
		outSingle = NULL;
	}

	//set the number of elements in the outBAT
	BATsetcount(outBAT, BATcount(inBAT));
	
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;
}

/*create textual representation of the wkb */
str wkbAsText_bat(bat *outBAT_id, bat *inBAT_id, int* withSRID) {
	return wkbStrOutWithFlag_bat(outBAT_id, inBAT_id, withSRID, wkbAsText, "batgeom.wkbAsText");
}
str wkbGeometryType_bat(bat *outBAT_id, bat *inBAT_id, int* flag) {
	return wkbStrOutWithFlag_bat(outBAT_id, inBAT_id, flag, wkbGeometryType, "batgeom.wkbGeometryType");
}

str wkbBoundary_bat(bat *outBAT_id, bat *inBAT_id) {
	BAT *outBAT = NULL, *inBAT = NULL;
	wkb *inWKB = NULL;
	BUN p =0, q =0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, "batgeom.wkbBoundary", RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.wkbBoundary", "The arguments must have dense and aligned heads");
	}

	//create a new for the output BAT
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.wkbBoundary", MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the input BAT	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { //iterate over all valid elements
		str err = NULL;
		wkb* outSingle;

		inWKB = (wkb*) BUNtail(inBAT_iter, p);
		if ((err = wkbBoundary(&outSingle, &inWKB)) != MAL_SUCCEED) {
			str msg = createException(MAL, "batgeom.wkbBoundary", "%s", err);
			GDKfree(err);

			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			
			return msg;
		}
		BUNappend(outBAT,outSingle,TRUE); //add the result to the new BAT
		GDKfree(outSingle);
		outSingle = NULL;
	}

	//set the number of elements in the outBAT
	BATsetcount(outBAT, BATcount(inBAT));
	
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;
}

static str wkbBitOut_bat(bat *outBAT_id, bat *inBAT_id, str (*func)(bit*, wkb**), const char *name) {
	BAT *outBAT = NULL, *inBAT = NULL;
	wkb *inWKB = NULL;
	BUN p =0, q =0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, name, RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, name, "The arguments must have dense and aligned heads");
	}

	//create a new for the output BAT
	if ((outBAT = BATnew(TYPE_void, ATOMindex("bit"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, name, MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the input BAT	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { //iterate over all valid elements
		str err = NULL;
		bit outSingle;

		inWKB = (wkb*) BUNtail(inBAT_iter, p);
		if ((err = (*func)(&outSingle, &inWKB)) != MAL_SUCCEED) {
			str msg = createException(MAL, name, "%s", err);
			GDKfree(err);

			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			
			return msg;
		}
		BUNappend(outBAT,&outSingle,TRUE); //add the result to the new BAT
	}

	//set the number of elements in the outBAT
	BATsetcount(outBAT, BATcount(inBAT));
	
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;

}

str wkbIsClosed_bat(bat *outBAT_id, bat *inBAT_id) {
	return wkbBitOut_bat(outBAT_id, inBAT_id, wkbIsClosed, "batgeom.wkbIsClosed");
}
str wkbIsEmpty_bat(bat *outBAT_id, bat *inBAT_id) {
	return wkbBitOut_bat(outBAT_id, inBAT_id, wkbIsEmpty, "batgeom.wkbIsEmpty");
}
str wkbIsSimple_bat(bat *outBAT_id, bat *inBAT_id) {
	return wkbBitOut_bat(outBAT_id, inBAT_id, wkbIsSimple, "batgeom.wkbIsSimple");
}
str wkbIsRing_bat(bat *outBAT_id, bat *inBAT_id) {
	return wkbBitOut_bat(outBAT_id, inBAT_id, wkbIsRing, "batgeom.wkbIsRing");
}
str wkbIsValid_bat(bat *outBAT_id, bat *inBAT_id) {
	return wkbBitOut_bat(outBAT_id, inBAT_id, wkbIsValid, "batgeom.wkbIsValid");
}

/*******************************/
/********* Two inputs **********/
/*******************************/

str wkbContains_bat(int* outBAT_id, bat *aBAT_id, bat *bBAT_id) {
	BAT *outBAT = NULL, *aBAT = NULL, *bBAT = NULL;
	BATiter aBAT_iter, bBAT_iter;
	BUN i=0;
	str ret = MAL_SUCCEED;

	//get the BATs
	if ( (aBAT = BATdescriptor(*aBAT_id)) == NULL || (bBAT = BATdescriptor(*bBAT_id)) == NULL ) {
		ret = createException(MAL, "batgeom.Contains", "Problem retrieving BATs");
		goto clean;
	}

	//check if the BATs are dense and aligned
	if( !BAThdense(aBAT) || !BAThdense(bBAT) ) {
		ret = createException(MAL, "batgeom.Contains", "BATs must have dense heads");
		goto clean;
	}
	if( aBAT->hseqbase != bBAT->hseqbase || BATcount(aBAT) != BATcount(bBAT) ) {
		ret = createException(MAL, "batgeom.Contains", "BATs must be aligned");
		goto clean;
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("bit"), BATcount(aBAT), TRANSIENT)) == NULL) {
		ret = createException(MAL, "batgeom.Contains", "Error creating new BAT");
		goto clean;
	}

	//set the first idx of the output BAT equal to that of the aBAT
	BATseqbase(outBAT, aBAT->hseqbase);

	//iterator over the BATs	
	aBAT_iter = bat_iterator(aBAT);
	bBAT_iter = bat_iterator(bBAT);

	for (i = BUNfirst(aBAT); i < BATcount(aBAT); i++) { 
		str err = NULL;
		bit outBIT;

		wkb *aWKB = (wkb*) BUNtail(aBAT_iter, i + BUNfirst(aBAT));
		wkb *bWKB = (wkb*) BUNtail(bBAT_iter, i + BUNfirst(bBAT));

		if ((err = wkbContains(&outBIT, &aWKB, &bWKB)) != MAL_SUCCEED) {
			BBPreleaseref(outBAT->batCacheid);	

			ret = createException(MAL, "batgeom.Contains", "%s", err);
			GDKfree(err);
			
			goto clean;
		}
		BUNappend(outBAT,&outBIT,TRUE); //add the result to the outBAT
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);

clean:
	if(aBAT)
		BBPreleaseref(aBAT->batCacheid);
	if(bBAT)
		BBPreleaseref(bBAT->batCacheid);
		
	return ret;
}

str wkbContains_geom_bat(int* outBAT_id, wkb** geomWKB, int* inBAT_id) {
	BAT *outBAT = NULL, *inBAT = NULL;
	BATiter inBAT_iter;
	BUN p=0, q=0;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		return createException(MAL, "batgeom.Contains", "Problem retrieving BAT");
	}
	
	if ( !BAThdense(inBAT) ) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.Contains", "The BAT must have dense head");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("bit"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.Contains", "Error creating new BAT");
	}
	
	//set the first idx of the output BAT equal to that of the aBAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the BATs	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { 
		str err = NULL;
		bit outBIT;

		wkb *inWKB = (wkb*) BUNtail(inBAT_iter, p);

		if ((err = wkbContains(&outBIT, geomWKB, &inWKB)) != MAL_SUCCEED) {
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batgeom.Contains", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,&outBIT,TRUE); //add the result to the outBAT
	}

	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;

}

str wkbContains_bat_geom(int* outBAT_id, int* inBAT_id, wkb** geomWKB) {
	BAT *outBAT = NULL, *inBAT = NULL;
	BATiter inBAT_iter;
	BUN p=0, q=0;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		return createException(MAL, "batgeom.Contains", "Problem retrieving BAT");
	}
	
	if ( !BAThdense(inBAT) ) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.Contains", "The BAT must have dense head");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("bit"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.Contains", "Error creating new BAT");
	}
	
	//set the first idx of the output BAT equal to that of the aBAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the BATs	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { 
		str err = NULL;
		bit outBIT;

		wkb *inWKB = (wkb*) BUNtail(inBAT_iter, p);

		if ((err = wkbContains(&outBIT, &inWKB, geomWKB)) != MAL_SUCCEED) {
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batgeom.Contains", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,&outBIT,TRUE); //add the result to the outBAT
	}

	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;
}



/*
str wkbFromWKB_bat(int* outBAT_id, int* inBAT_id) {
	BAT *outBAT = NULL, *inBAT = NULL;
	wkb **inWKB = NULL, *outWKB = NULL;
	BUN i;
	
	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, "batgeom.wkb", RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.wkb", "both arguments must have dense and aligned heads");
	}

	//create a new BAT
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(inBAT))) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.wkb", MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//pointers to the first valid elements of the x and y BATS
	inWKB = (wkb **) Tloc(inBAT, BUNfirst(inBAT));
	for (i = 0; i < BATcount(inBAT); i++) { //iterate over all valid elements
		str err = NULL;
		if ((err = wkbFromWKB(&outWKB, &inWKB[i])) != MAL_SUCCEED) { 
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batgeom.wkb", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,outWKB,TRUE); //add the point to the new BAT
		GDKfree(outWKB);
		outWKB = NULL;
	}

	//set some properties of the new BAT
	BATsetcount(outBAT, BATcount(inBAT));
    	BATsettrivprop(outBAT);
    	BATderiveProps(outBAT,FALSE);
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	return MAL_SUCCEED;

}*/

/* the bat version of geomMakePoint2D */
str geomMakePoint2D_bat(int* outBAT_id, int* xBAT_id, int* yBAT_id) {
	BAT *outBAT = NULL, *xBAT = NULL, *yBAT = NULL;
	BATiter xBAT_iter, yBAT_iter;
	BUN i;
	str ret = MAL_SUCCEED;

	//get the BATs
	if ( (xBAT = BATdescriptor(*xBAT_id)) == NULL || (yBAT = BATdescriptor(*yBAT_id)) == NULL ) {
		ret = createException(MAL, "batgeom.MakePoint", "Problem retrieving BATs");
		goto clean;
	}

	//check if the BATs are dense and aligned
	if( !BAThdense(xBAT) || !BAThdense(yBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must have dense heads");
		goto clean;
	}
	if( xBAT->hseqbase != yBAT->hseqbase || BATcount(xBAT) != BATcount(yBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must be aligned");
		goto clean;
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(xBAT), TRANSIENT)) == NULL) {
		ret = createException(MAL, "batgeom.MakePoint", "Error creating new BAT");
		goto clean;
	}

	//set the first idx of the new BAT equal to that of the x BAT (which is equal to the y BAT)
	BATseqbase(outBAT, xBAT->hseqbase);

	//iterator over the BATs	
	xBAT_iter = bat_iterator(xBAT);
	yBAT_iter = bat_iterator(yBAT);

	for (i = BUNfirst(xBAT); i < BATcount(xBAT); i++) { 
		str err = NULL;
		wkb *pointWKB = NULL;

		double x = *((double*) BUNtail(xBAT_iter, i + BUNfirst(xBAT)));
		double y = *((double*) BUNtail(yBAT_iter, i + BUNfirst(yBAT)));

		if ((err = geomMakePoint2D(&pointWKB, &x, &y)) != MAL_SUCCEED) { //check
			BBPreleaseref(outBAT->batCacheid);	

			ret = createException(MAL, "batgeom.MakePoint", "%s", err);
			GDKfree(err);
			
			goto clean;
		}
		BUNappend(outBAT,pointWKB,TRUE); //add the result to the outBAT
		GDKfree(pointWKB);
		pointWKB = NULL;
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);

clean:
	if(xBAT)
		BBPreleaseref(xBAT->batCacheid);
	if(yBAT)
		BBPreleaseref(yBAT->batCacheid);
		
	return ret;
}

/* the bat version og geomMakePoint3D */
str geomMakePoint3D_bat(int* outBAT_id, int* xBAT_id, int* yBAT_id, int* zBAT_id) {
	BAT *outBAT = NULL, *xBAT = NULL, *yBAT = NULL, *zBAT = NULL;
	BATiter xBAT_iter, yBAT_iter, zBAT_iter;
	BUN i;
	str ret = MAL_SUCCEED;

	//get the BATs
	if ( (xBAT = BATdescriptor(*xBAT_id)) == NULL || (yBAT = BATdescriptor(*yBAT_id)) == NULL || 
	     (zBAT = BATdescriptor(*zBAT_id)) == NULL ) {
		ret = createException(MAL, "batgeom.MakePoint", "Problem retrieving BATs");
		goto clean;
	}

	//check if the BATs are dense and aligned
	if( !BAThdense(xBAT) || !BAThdense(yBAT) || !BAThdense(zBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must have dense heads");
		goto clean;
	}
	if( xBAT->hseqbase != yBAT->hseqbase || BATcount(xBAT) != BATcount(yBAT) || 
	    xBAT->hseqbase != zBAT->hseqbase || BATcount(xBAT) != BATcount(zBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must be aligned");
		goto clean;
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(xBAT), TRANSIENT)) == NULL) {
		ret = createException(MAL, "batgeom.MakePoint", "Error creating new BAT");
		goto clean;
	}

	//set the first idx of the new BAT equal to that of the x BAT (which is equal to the y BAT)
	BATseqbase(outBAT, xBAT->hseqbase);

	//iterator over the BATs	
	xBAT_iter = bat_iterator(xBAT);
	yBAT_iter = bat_iterator(yBAT);
	zBAT_iter = bat_iterator(zBAT);

	for (i = BUNfirst(xBAT); i < BATcount(xBAT); i++) { 
		str err = NULL;
		wkb *pointWKB = NULL;

		double x = *((double*) BUNtail(xBAT_iter, i + BUNfirst(xBAT)));
		double y = *((double*) BUNtail(yBAT_iter, i + BUNfirst(yBAT)));
		double z = *((double*) BUNtail(zBAT_iter, i + BUNfirst(zBAT)));

		if ((err = geomMakePoint3D(&pointWKB, &x, &y, &z)) != MAL_SUCCEED) { //check
			BBPreleaseref(outBAT->batCacheid);	

			ret = createException(MAL, "batgeom.MakePoint", "%s", err);
			GDKfree(err);
			
			goto clean;
		}
		BUNappend(outBAT,pointWKB,TRUE); //add the result to the outBAT
		GDKfree(pointWKB);
		pointWKB = NULL;
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);

clean:
	if(xBAT)
		BBPreleaseref(xBAT->batCacheid);
	if(yBAT)
		BBPreleaseref(yBAT->batCacheid);
	if(zBAT)
		BBPreleaseref(zBAT->batCacheid);

	return ret;
}

/* the bat version og geomMakePoint4D */
str geomMakePoint4D_bat(int* outBAT_id, int* xBAT_id, int* yBAT_id, int* zBAT_id, int *mBAT_id) {
	BAT *outBAT = NULL, *xBAT = NULL, *yBAT = NULL, *zBAT = NULL, *mBAT = NULL;
	BATiter xBAT_iter, yBAT_iter, zBAT_iter, mBAT_iter;
	BUN i;
	str ret = MAL_SUCCEED;

	//get the BATs
	if ( (xBAT = BATdescriptor(*xBAT_id)) == NULL || (yBAT = BATdescriptor(*yBAT_id)) == NULL || 
	     (zBAT = BATdescriptor(*zBAT_id)) == NULL || (mBAT = BATdescriptor(*mBAT_id)) == NULL ) {
		ret = createException(MAL, "batgeom.MakePoint", "Problem retrieving BATs");
		goto clean;
	}

	//check if the BATs are dense and aligned
	if( !BAThdense(xBAT) || !BAThdense(yBAT) || !BAThdense(zBAT) || !BAThdense(mBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must have dense heads");
		goto clean;
	}
	if( xBAT->hseqbase != yBAT->hseqbase || BATcount(xBAT) != BATcount(yBAT) ||
	    xBAT->hseqbase != zBAT->hseqbase || BATcount(xBAT) != BATcount(zBAT) ||
	    xBAT->hseqbase != mBAT->hseqbase || BATcount(xBAT) != BATcount(mBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must be aligned");
		goto clean;
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(xBAT), TRANSIENT)) == NULL) {
		ret = createException(MAL, "batgeom.MakePoint", "Error creating new BAT");
		goto clean;
	}

	//set the first idx of the new BAT equal to that of the x BAT (which is equal to the y BAT)
	BATseqbase(outBAT, xBAT->hseqbase);

	//iterator over the BATs	
	xBAT_iter = bat_iterator(xBAT);
	yBAT_iter = bat_iterator(yBAT);
	zBAT_iter = bat_iterator(zBAT);
	mBAT_iter = bat_iterator(mBAT);

	for (i = BUNfirst(xBAT); i < BATcount(xBAT); i++) { 
		str err = NULL;
		wkb *pointWKB = NULL;

		double x = *((double*) BUNtail(xBAT_iter, i + BUNfirst(xBAT)));
		double y = *((double*) BUNtail(yBAT_iter, i + BUNfirst(yBAT)));
		double z = *((double*) BUNtail(zBAT_iter, i + BUNfirst(zBAT)));
		double m = *((double*) BUNtail(mBAT_iter, i + BUNfirst(mBAT)));

		if ((err = geomMakePoint4D(&pointWKB, &x, &y, &z, &m)) != MAL_SUCCEED) { //check
			BBPreleaseref(outBAT->batCacheid);	

			ret = createException(MAL, "batgeom.MakePoint", "%s", err);
			GDKfree(err);
			
			goto clean;
		}
		BUNappend(outBAT,pointWKB,TRUE); //add the result to the outBAT
		GDKfree(pointWKB);
		pointWKB = NULL;
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);

clean:
	if(xBAT)
		BBPreleaseref(xBAT->batCacheid);
	if(yBAT)
		BBPreleaseref(yBAT->batCacheid);
	if(zBAT)
		BBPreleaseref(zBAT->batCacheid);
	if(mBAT)
		BBPreleaseref(mBAT->batCacheid);

	return ret;
}

str geomMakePointM_bat(int* outBAT_id, int* xBAT_id, int* yBAT_id, int *mBAT_id) {
	BAT *outBAT = NULL, *xBAT = NULL, *yBAT = NULL, *mBAT = NULL;
	BATiter xBAT_iter, yBAT_iter, mBAT_iter;
	BUN i;
	str ret = MAL_SUCCEED;

	//get the BATs
	if ( (xBAT = BATdescriptor(*xBAT_id)) == NULL || (yBAT = BATdescriptor(*yBAT_id)) == NULL || 
	     (mBAT = BATdescriptor(*mBAT_id)) == NULL ) {
		ret = createException(MAL, "batgeom.MakePoint", "Problem retrieving BATs");
		goto clean;
	}

	//check if the BATs are dense and aligned
	if( !BAThdense(xBAT) || !BAThdense(yBAT) || !BAThdense(mBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must have dense heads");
		goto clean;
	}
	if( xBAT->hseqbase != yBAT->hseqbase || BATcount(xBAT) != BATcount(yBAT) ||
	    xBAT->hseqbase != mBAT->hseqbase || BATcount(xBAT) != BATcount(mBAT) ) {
		ret = createException(MAL, "batgeom.MakePoint", "BATs must be aligned");
		goto clean;
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(xBAT), TRANSIENT)) == NULL) {
		ret = createException(MAL, "batgeom.MakePoint", "Error creating new BAT");
		goto clean;
	}

	//set the first idx of the new BAT equal to that of the x BAT (which is equal to the y BAT)
	BATseqbase(outBAT, xBAT->hseqbase);

	//iterator over the BATs	
	xBAT_iter = bat_iterator(xBAT);
	yBAT_iter = bat_iterator(yBAT);
	mBAT_iter = bat_iterator(mBAT);

	for (i = BUNfirst(xBAT); i < BATcount(xBAT); i++) { 
		str err = NULL;
		wkb *pointWKB = NULL;

		double x = *((double*) BUNtail(xBAT_iter, i + BUNfirst(xBAT)));
		double y = *((double*) BUNtail(yBAT_iter, i + BUNfirst(yBAT)));
		double m = *((double*) BUNtail(mBAT_iter, i + BUNfirst(mBAT)));

		if ((err = geomMakePointM(&pointWKB, &x, &y, &m)) != MAL_SUCCEED) { //check
			BBPreleaseref(outBAT->batCacheid);	

			ret = createException(MAL, "batgeom.MakePoint", "%s", err);
			GDKfree(err);
			
			goto clean;
		}
		BUNappend(outBAT,pointWKB,TRUE); //add the result to the outBAT
		GDKfree(pointWKB);
		pointWKB = NULL;
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);

clean:
	if(xBAT)
		BBPreleaseref(xBAT->batCacheid);
	if(yBAT)
		BBPreleaseref(yBAT->batCacheid);
	if(mBAT)
		BBPreleaseref(mBAT->batCacheid);

	return ret;
}

/* sets the srid of the geometry - BULK version*/
str wkbSetSRID_bat(int* outBAT_id, int* inBAT_id, int* srid) {
	BAT *outBAT = NULL, *inBAT = NULL;
	BUN p=0, q=0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		return createException(MAL, "batgeom.SetSRID", "Problem retrieving BAT");
	}
	
	if ( !BAThdense(inBAT) ) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.SetSRID", "The BAT must have dense head");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.SetSRID", "Error creating new BAT");
	}
	
	//set the first idx of the output BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the BATs	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { 
		str err = NULL;
		wkb *outWKB = NULL;

		wkb *inWKB = (wkb*) BUNtail(inBAT_iter, p);

		if ((err = wkbSetSRID(&outWKB, &inWKB, srid)) != MAL_SUCCEED) { //set SRID
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batgeom.SetSRID", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,outWKB,TRUE); //add the point to the new BAT
		GDKfree(outWKB);
		outWKB = NULL;
	}

	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;
}

str wkbDistance_bat(int* outBAT_id, bat *aBAT_id, bat *bBAT_id) {
	BAT *outBAT = NULL, *aBAT = NULL, *bBAT = NULL;
	BATiter aBAT_iter, bBAT_iter;
	BUN i=0;
	str ret = MAL_SUCCEED;

	//get the BATs
	if ( (aBAT = BATdescriptor(*aBAT_id)) == NULL || (bBAT = BATdescriptor(*bBAT_id)) == NULL ) {
		ret = createException(MAL, "batgeom.Distance", "Problem retrieving BATs");
		goto clean;
	}
	
	//check if the BATs are dense and aligned
	if( !BAThdense(aBAT) || !BAThdense(bBAT) ) {
		ret = createException(MAL, "batgeom.Distance", "BATs must have dense heads");
		goto clean;
	}
	if( aBAT->hseqbase != bBAT->hseqbase || BATcount(aBAT) != BATcount(bBAT) ) {
		ret = createException(MAL, "batgeom.Distance", "BATs must be aligned");
		goto clean;
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("dbl"), BATcount(aBAT), TRANSIENT)) == NULL) {
		ret = createException(MAL, "batgeom.Distance", "Error creating new BAT");
		goto clean;
	}

	//set the first idx of the output BAT equal to that of the aBAT
	BATseqbase(outBAT, aBAT->hseqbase);

	//iterator over the BATs	
	aBAT_iter = bat_iterator(aBAT);
	bBAT_iter = bat_iterator(bBAT);

	for (i = BUNfirst(aBAT); i < BATcount(aBAT); i++) { 
		str err = NULL;
		double distanceVal = 0;

		wkb* aWKB = (wkb*) BUNtail(aBAT_iter, i + BUNfirst(aBAT));
		wkb* bWKB = (wkb*) BUNtail(bBAT_iter, i + BUNfirst(bBAT));

		if ((err = wkbDistance(&distanceVal, &aWKB, &bWKB)) != MAL_SUCCEED) { //check	
			BBPreleaseref(outBAT->batCacheid);	

			ret = createException(MAL, "batgeom.Distance", "%s", err);
			GDKfree(err);
			
			goto clean;
		}
		BUNappend(outBAT,&distanceVal,TRUE); //add the result to the outBAT
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);

clean:
	if(aBAT)
		BBPreleaseref(aBAT->batCacheid);
	if(bBAT)
		BBPreleaseref(bBAT->batCacheid);
		
	return ret;

}

str wkbDistance_geom_bat(int* outBAT_id, wkb** geomWKB, int* inBAT_id) {
	BAT *outBAT = NULL, *inBAT = NULL;
	BATiter inBAT_iter;
	BUN p=0, q=0;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		return createException(MAL, "batgeom.Distance", "Problem retrieving BAT");
	}
	
	if ( !BAThdense(inBAT) ) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.Distance", "The BAT must have dense head");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("dbl"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		return createException(MAL, "batgeom.Distance", "Error creating new BAT");
	}
	//set the first idx of the output BAT equal to that of the aBAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the BAT	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) {	
		str err = NULL;
		double distanceVal = 0;

		wkb *inWKB = (wkb*) BUNtail(inBAT_iter, p);

		if ((err = wkbDistance(&distanceVal, geomWKB, &inWKB)) != MAL_SUCCEED) { //check
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batgeom.Distance", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,&distanceVal,TRUE); //add the result to the outBAT
	}

	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	
	return MAL_SUCCEED;
}

str wkbDistance_bat_geom(int* outBAT_id, int* inBAT_id, wkb** geomWKB) {
	return wkbDistance_geom_bat(outBAT_id, geomWKB, inBAT_id);
}

/**
 * It filters the geometry in the second BAT with respect to the MBR of the geometry in the first BAT.
 **//*
str wkbFilter_bat(int* aBATfiltered_id, int* bBATfiltered_id, int* aBAT_id, int* bBAT_id) {
	BAT *aBATfiltered = NULL, *bBATfiltered = NULL, *aBAT = NULL, *bBAT = NULL;
	wkb *aWKB = NULL, *bWKB = NULL;
	bit outBIT;
	BATiter aBAT_iter, bBAT_iter;
	BUN i=0;
	int remainingElements =0;

	//get the descriptor of the BAT
	if ((aBAT = BATdescriptor(*aBAT_id)) == NULL) {
		throw(MAL, "batgeom.MBRfilter", RUNTIME_OBJECT_MISSING);
	}
	if ((bBAT = BATdescriptor(*bBAT_id)) == NULL) {
		BBPreleaseref(aBAT->batCacheid);
		throw(MAL, "batgeom.MBRfilter", RUNTIME_OBJECT_MISSING);
	}
	
	if ( aBAT->htype != TYPE_void || //header type of aBAT not void
		 bBAT->htype != TYPE_void || //header type of bBAT not void
	    aBAT->hseqbase != bBAT->hseqbase || //the idxs of the headers of the BATs are not the same
	    BATcount(aBAT) != BATcount(bBAT)) { //the number of valid elements in the BATs are not the same
		BBPreleaseref(aBAT->batCacheid);
		BBPreleaseref(bBAT->batCacheid);
		throw(MAL, "batgeom.MBRfilter", "The arguments must have dense and aligned heads");
	}

	//create two new BATs for the output
	if ((aBATfiltered = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(aBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(aBAT->batCacheid);
		BBPreleaseref(bBAT->batCacheid);
		throw(MAL, "batgeom.MBRfilter", MAL_MALLOC_FAIL);
	}
	if ((bBATfiltered = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(bBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(aBAT->batCacheid);
		BBPreleaseref(bBAT->batCacheid);
		throw(MAL, "batgeom.MBRfilter", MAL_MALLOC_FAIL);
	}

	//set the first idx of the output BATs equal to that of the aBAT
	BATseqbase(aBATfiltered, aBAT->hseqbase);
	BATseqbase(bBATfiltered, bBAT->hseqbase);

	//iterator over the BATs	
	aBAT_iter = bat_iterator(aBAT);
	bBAT_iter = bat_iterator(bBAT);

	for (i = BUNfirst(aBAT); i < BATcount(aBAT); i++) { 
		str err = NULL;
		aWKB = (wkb*) BUNtail(aBAT_iter, i + BUNfirst(aBAT));
		bWKB = (wkb*) BUNtail(bBAT_iter, i + BUNfirst(bBAT));
		
		//check the containment of the MBRs
		if((err = mbrOverlaps_wkb(&outBIT, &aWKB, &bWKB)) != MAL_SUCCEED) {
			str msg;
			BBPreleaseref(aBAT->batCacheid);
			BBPreleaseref(bBAT->batCacheid);
			BBPreleaseref(aBATfiltered->batCacheid);
			BBPreleaseref(bBATfiltered->batCacheid);
			msg = createException(MAL, "batgeom.wkbFilter", "%s", err);
			GDKfree(err);
			return msg;
		}
		if(outBIT) {
			BUNappend(aBATfiltered,aWKB, TRUE); //add the result to the aBAT
			BUNappend(bBATfiltered,bWKB, TRUE); //add the result to the bBAT
			remainingElements++;
		}
	}

	//set some properties of the new BATs
	BATsetcount(aBATfiltered, remainingElements);
    	BATsettrivprop(aBATfiltered);
    	BATderiveProps(aBATfiltered,FALSE);
	
	BATsetcount(bBATfiltered, remainingElements);
    	BATsettrivprop(bBATfiltered);
    	BATderiveProps(bBATfiltered,FALSE);
	
	BBPreleaseref(aBAT->batCacheid);
	BBPreleaseref(bBAT->batCacheid);
	BBPkeepref(*aBATfiltered_id = aBATfiltered->batCacheid);
	BBPkeepref(*bBATfiltered_id = bBATfiltered->batCacheid);
	
	return MAL_SUCCEED;


}*/

/**
 * It filters the geometry in the second BAT with respect to the MBR of the geometry in the first BAT.
 **/
str wkbFilter_geom_bat(int* BATfiltered_id, wkb** geomWKB, int* BAToriginal_id) {
	BAT *BATfiltered = NULL, *BAToriginal = NULL;
	wkb *WKBoriginal = NULL;
	BATiter BAToriginal_iter;
	BUN i=0;
	mbr* geomMBR;
	int remainingElements =0;
	str err = NULL;

	//get the descriptor of the BAT
	if ((BAToriginal = BATdescriptor(*BAToriginal_id)) == NULL) {
		throw(MAL, "batgeom.MBRfilter", RUNTIME_OBJECT_MISSING);
	}
	
	if ( BAToriginal->htype != TYPE_void ) { //header type of bBAT not void
		BBPreleaseref(BAToriginal->batCacheid);
		throw(MAL, "batgeom.MBRfilter", "The arguments must have dense and aligned heads");
	}

	//create the new BAT
	if ((BATfiltered = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(BAToriginal), TRANSIENT)) == NULL) {
		BBPreleaseref(BAToriginal->batCacheid);
		throw(MAL, "batgeom.MBRfilter", MAL_MALLOC_FAIL);
	}

	//set the first idx of the output BATs equal to that of the aBAT
	BATseqbase(BATfiltered, BAToriginal->hseqbase);

	//iterator over the BAT
	BAToriginal_iter = bat_iterator(BAToriginal);

	//create the MBR of the geom
	if((err = wkbMBR(&geomMBR, geomWKB)) != MAL_SUCCEED) {
		str msg;
		BBPreleaseref(BAToriginal->batCacheid);
		BBPreleaseref(BATfiltered->batCacheid);
		msg = createException(MAL, "batgeom.wkbFilter", "%s", err);
		GDKfree(err);
		return msg;
	}

	for (i = BUNfirst(BAToriginal); i < BATcount(BAToriginal); i++) { 
		str err = NULL;
		mbr* MBRoriginal;
		bit outBIT = 0;

		WKBoriginal = (wkb*) BUNtail(BAToriginal_iter, i + BUNfirst(BAToriginal));

		//create the MBR for each geometry in the BAT
		if((err = wkbMBR(&MBRoriginal, &WKBoriginal)) != MAL_SUCCEED) {
			str msg;
			BBPreleaseref(BAToriginal->batCacheid);
			BBPreleaseref(BATfiltered->batCacheid);
			msg = createException(MAL, "batgeom.wkbFilter", "%s", err);
			GDKfree(err);
			GDKfree(geomMBR);
			return msg;
		}
		
		//check the containment of the MBRs
		if((err = mbrOverlaps(&outBIT, &geomMBR, &MBRoriginal)) != MAL_SUCCEED) {
			str msg;
			BBPreleaseref(BAToriginal->batCacheid);
			BBPreleaseref(BATfiltered->batCacheid);
			msg = createException(MAL, "batgeom.wkbFilter", "%s", err);
			GDKfree(err);
			GDKfree(geomMBR);
			GDKfree(MBRoriginal);
			return msg;
		}

		if(outBIT) {
			BUNappend(BATfiltered,WKBoriginal, TRUE); //add the result to the bBAT
			remainingElements++;
		}
		
		GDKfree(MBRoriginal);
	}

	//set some properties of the new BATs
	BATsetcount(BATfiltered, remainingElements);
    	BATsettrivprop(BATfiltered);
    	BATderiveProps(BATfiltered,FALSE);
	
	BBPreleaseref(BAToriginal->batCacheid);
	BBPkeepref(*BATfiltered_id = BATfiltered->batCacheid);

	return MAL_SUCCEED;

}

str wkbFilter_bat_geom(int* BATfiltered_id, int* BAToriginal_id, wkb** geomWKB) {
	return wkbFilter_geom_bat(BATfiltered_id, geomWKB, BAToriginal_id);
}

/* MBR */

/* Creates the BAT with mbrs from the BAT with geometries. */
str wkbMBR_bat(int* outBAT_id, int* inBAT_id) {
	BAT *outBAT = NULL, *inBAT = NULL;
	wkb *inWKB = NULL;
	mbr *outMBR = NULL;
	BUN p =0, q =0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, "batgeom.mbr", RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.mbr", "the arguments must have dense and aligned heads");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("mbr"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.mbr", MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the BAT	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { //iterate over all valid elements
		str err = NULL;

		inWKB = (wkb*) BUNtail(inBAT_iter, p);
		if ((err = wkbMBR(&outMBR, &inWKB)) != MAL_SUCCEED) {
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batgeom.mbr", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,outMBR,TRUE); //add the point to the new BAT
		GDKfree(outMBR);
		outMBR = NULL;
	}

	//set some properties of the new BAT
	BATsetcount(outBAT, BATcount(inBAT));
    	BATsettrivprop(outBAT);
    	BATderiveProps(outBAT,FALSE);
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	return MAL_SUCCEED;
}

str wkbCoordinateFromMBR_bat(int *outBAT_id, int *inBAT_id, int* coordinateIdx) {
	BAT *outBAT = NULL, *inBAT = NULL;
	mbr *inMBR = NULL;
	double outDbl = 0.0;
	BUN p =0, q =0;
	BATiter inBAT_iter;

	//get the descriptor of the BAT
	if ((inBAT = BATdescriptor(*inBAT_id)) == NULL) {
		throw(MAL, "batgeom.coordinateFromMBR", RUNTIME_OBJECT_MISSING);
	}
	
	if ( inBAT->htype != TYPE_void ) { //header type of  BAT not void
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.coordinateFromMBR", "the arguments must have dense and aligned heads");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("dbl"), BATcount(inBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(inBAT->batCacheid);
		throw(MAL, "batgeom.coordinateFromMBR", MAL_MALLOC_FAIL);
	}
	//set the first idx of the new BAT equal to that of the input BAT
	BATseqbase(outBAT, inBAT->hseqbase);

	//iterator over the BAT	
	inBAT_iter = bat_iterator(inBAT);
	BATloop(inBAT, p, q) { //iterate over all valid elements
		str err = NULL;

		inMBR = (mbr*) BUNtail(inBAT_iter, p);
		if ((err = wkbCoordinateFromMBR(&outDbl, &inMBR, coordinateIdx)) != MAL_SUCCEED) {
			str msg;
			BBPreleaseref(inBAT->batCacheid);
			BBPreleaseref(outBAT->batCacheid);
			msg = createException(MAL, "batgeom.coordinateFromMBR", "%s", err);
			GDKfree(err);
			return msg;
		}
		BUNappend(outBAT,&outDbl,TRUE);
	}

	//set some properties of the new BAT
	BATsetcount(outBAT, BATcount(inBAT));
    	BATsettrivprop(outBAT);
    	BATderiveProps(outBAT,FALSE);
	BBPreleaseref(inBAT->batCacheid);
	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	return MAL_SUCCEED;

}
 
str wkbCoordinateFromWKB_bat(int *outBAT_id, int *inBAT_id, int* coordinateIdx) {
	str err = NULL;
	int inBAT_mbr_id = 0; //the id of the bat with the mbrs

	if((err = wkbMBR_bat(&inBAT_mbr_id, inBAT_id)) != MAL_SUCCEED) {
		str msg;
		msg = createException(MAL, "batgeom.coordinateFromMBR", "%s", err);
		GDKfree(err);
		return msg;

	}

	//call the bulk version of wkbCoordinateFromMBR
	return wkbCoordinateFromMBR_bat(outBAT_id, &inBAT_mbr_id, coordinateIdx);
}

str wkbMakeLine_bat(int* outBAT_id, int* aBAT_id, int* bBAT_id) {
	BAT *outBAT = NULL, *aBAT = NULL, *bBAT = NULL;
	BATiter aBAT_iter, bBAT_iter;
	BUN i;

	//get the BATs
	if ( (aBAT = BATdescriptor(*aBAT_id)) == NULL || (bBAT = BATdescriptor(*bBAT_id)) == NULL ) {
		if(aBAT)
			BBPreleaseref(aBAT->batCacheid);	
		if(bBAT)
			BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.MakeLine", "Problem retrieving BATs");
	}

	//check if the BATs are dense and aligned
	if( !BAThdense(aBAT) || !BAThdense(bBAT) ) {
		BBPreleaseref(aBAT->batCacheid);	
		BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.MakeLine", "BATs must have dense heads");
	}
	if( aBAT->hseqbase != bBAT->hseqbase || BATcount(aBAT) != BATcount(bBAT) ) {
		BBPreleaseref(aBAT->batCacheid);	
		BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.MakeLine", "BATs must be aligned");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(aBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(aBAT->batCacheid);	
		BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.MakeLine", "Error creating new BAT");
	}

	//set the first idx of the new BAT equal to that of the x BAT (which is equal to the y BAT)
	BATseqbase(outBAT, aBAT->hseqbase);

	//iterator over the BATs	
	aBAT_iter = bat_iterator(aBAT);
	bBAT_iter = bat_iterator(bBAT);

	for (i = BUNfirst(aBAT); i < BATcount(aBAT); i++) { 
		str err = NULL, msg = NULL;
		wkb *aWKB = NULL, *bWKB = NULL, *outWKB = NULL;

		aWKB = (wkb*) BUNtail(aBAT_iter, i + BUNfirst(aBAT));
		bWKB = (wkb*) BUNtail(bBAT_iter, i + BUNfirst(bBAT));

		if ((err = wkbMakeLine(&outWKB, &aWKB, &bWKB)) != MAL_SUCCEED) { //check
			BBPreleaseref(outBAT->batCacheid);	
			BBPreleaseref(aBAT->batCacheid);	
			BBPreleaseref(bBAT->batCacheid);	

			msg = createException(MAL, "batgeom.MakeLine", "%s", err);
			GDKfree(err);
			
			return msg;
		}
		BUNappend(outBAT,outWKB,TRUE); //add the result to the outBAT
		GDKfree(outWKB);
		outWKB = NULL;
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	BBPreleaseref(aBAT->batCacheid);	
	BBPreleaseref(bBAT->batCacheid);	

	return MAL_SUCCEED;
}

str wkbUnion_bat(int* outBAT_id, int* aBAT_id, int* bBAT_id) {
	BAT *outBAT = NULL, *aBAT = NULL, *bBAT = NULL;
	BATiter aBAT_iter, bBAT_iter;
	BUN i;

	//get the BATs
	if ( (aBAT = BATdescriptor(*aBAT_id)) == NULL || (bBAT = BATdescriptor(*bBAT_id)) == NULL ) {
		if(aBAT)
			BBPreleaseref(aBAT->batCacheid);	
		if(bBAT)
			BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.Union", "Problem retrieving BATs");
	}

	//check if the BATs are dense and aligned
	if( !BAThdense(aBAT) || !BAThdense(bBAT) ) {
		BBPreleaseref(aBAT->batCacheid);	
		BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.Union", "BATs must have dense heads");
	}
	if( aBAT->hseqbase != bBAT->hseqbase || BATcount(aBAT) != BATcount(bBAT) ) {
		BBPreleaseref(aBAT->batCacheid);	
		BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.Union", "BATs must be aligned");
	}

	//create a new BAT for the output
	if ((outBAT = BATnew(TYPE_void, ATOMindex("wkb"), BATcount(aBAT), TRANSIENT)) == NULL) {
		BBPreleaseref(aBAT->batCacheid);	
		BBPreleaseref(bBAT->batCacheid);	
		return createException(MAL, "batgeom.Union", "Error creating new BAT");
	}

	//set the first idx of the new BAT equal to that of the x BAT (which is equal to the y BAT)
	BATseqbase(outBAT, aBAT->hseqbase);

	//iterator over the BATs	
	aBAT_iter = bat_iterator(aBAT);
	bBAT_iter = bat_iterator(bBAT);

	for (i = BUNfirst(aBAT); i < BATcount(aBAT); i++) { 
		str err = NULL, msg = NULL;
		wkb *aWKB = NULL, *bWKB = NULL, *outWKB = NULL;

		aWKB = (wkb*) BUNtail(aBAT_iter, i + BUNfirst(aBAT));
		bWKB = (wkb*) BUNtail(bBAT_iter, i + BUNfirst(bBAT));

		if ((err = wkbUnion(&outWKB, &aWKB, &bWKB)) != MAL_SUCCEED) { //check
			BBPreleaseref(outBAT->batCacheid);	
			BBPreleaseref(aBAT->batCacheid);	
			BBPreleaseref(bBAT->batCacheid);	

			msg = createException(MAL, "batgeom.Union", "%s", err);
			GDKfree(err);
			
			return msg;
		}
		BUNappend(outBAT,outWKB,TRUE); //add the result to the outBAT
		GDKfree(outWKB);
		outWKB = NULL;
	}

	BBPkeepref(*outBAT_id = outBAT->batCacheid);
	BBPreleaseref(aBAT->batCacheid);	
	BBPreleaseref(bBAT->batCacheid);	

	return MAL_SUCCEED;
}


