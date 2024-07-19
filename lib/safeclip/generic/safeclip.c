/* Clipping for Area... functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#include "safeclip.h"

#include <proto/exec.h>
#include <proto/graphics.h>


static LONG clp_xmin, clp_ymin, clp_xmax, clp_ymax;
static ULONG clp_nvert, clp_nvertmax;

static struct CLP_Vert {
	LONG x;
	LONG y;
} *clp_vert;

static LONG Clip2d (LONG n);


ULONG SafeInit (ULONG nvertmax)
{
	clp_nvertmax = nvertmax;
#ifdef __amigaos4__
	clp_vert = AllocVecTags(nvertmax * sizeof(struct CLP_Vert), AVT_Type, MEMF_PRIVATE, AVT_ClearWithValue, 0, TAG_DONE );
#else
	clp_vert = AllocVec(nvertmax * sizeof(struct CLP_Vert), MEMF_ANY);
#endif
	if (clp_vert)
		return 0;
	else {
		clp_vert = 0;
		return 1;
	}
}

VOID SafeClose (VOID)
{
	FreeVec(clp_vert);
}

VOID SafeSetLimits (LONG x1, LONG y1, LONG x2, LONG y2)
{
	clp_xmin = x1;
	clp_ymin = y1; 
	clp_xmax = x2;
	clp_ymax = y2;
}

VOID SafeAreaDraw (LONG x, LONG y)
{
	if (clp_nvert < clp_nvertmax) {
		clp_vert[clp_nvert].x = x;
		clp_vert[clp_nvert].y = y;
		clp_nvert++;
	}
}

VOID SafeAreaEnd (struct RastPort *rp)
{
	int i;

	if (clp_nvert > 0) {
		SafeAreaDraw(clp_vert[0].x, clp_vert[0].y); // close area
		clp_nvert = Clip2d(clp_nvert);
		AreaMove(rp, clp_vert[0].x, clp_vert[0].y);
		for (i = 1; i < clp_nvert; i++) {
			AreaDraw(rp, clp_vert[i].x, clp_vert[i].y);
		}
		AreaEnd(rp);
		clp_nvert = 0;
	}
}

static LONG Clip2d (LONG n)
{
	int i;
	
	if (clp_vert == NULL)
		return 0;
	
	// TODO: improve clipping
	for (i = 0; i < clp_nvert; i++) {
		if (clp_vert[i].x < clp_xmin)
			clp_vert[i].x = clp_xmin;
		if (clp_vert[i].x > clp_xmax)
			clp_vert[i].x = clp_xmax;
		if (clp_vert[i].y < clp_ymin)
			clp_vert[i].y = clp_ymin;
		if (clp_vert[i].y > clp_ymax)
			clp_vert[i].y = clp_ymax;
	}
	return n;
}
