/* gClass-Diagram for ignition
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include <graphics/gfxmacros.h>

#include "gclass.h"
#include "gclass_protos.h"

#if defined(__SASC)
#	include "gclass_pragmas.h"
#endif

#define CATCOMP_NUMBERS
#ifdef __amigaos4__
	#include "../ignition_strings.h"
	#include <proto/exec.h>
#else
	#include "ignition_strings.h"
#endif
#include <string.h>
#include <stdbool.h>
struct gArea;

const char *version = "$VER: line_diagram.gc 0.3 (7.8.2003)";

/** interface definition **/

struct gInterface interface[] = {
	{GAA_Pseudo3D, "Pseudo-3D", GIT_CHECKBOX, NULL, NULL},
	{0, NULL, 0, NULL, NULL}
};

const STRPTR superClass = "diagram";
ULONG instanceSize = 0;
	
static struct Catalog *sCatalog;
 

static long
mm_to_pixel_dpi(ULONG dpi, LONG mm, bool horiz)
{
	dpi = horiz ? dpi >> 16 : dpi & 0xffff;
	return (long)(mm * dpi / (25.4*1024));
}


void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi),
	REG(a0, struct RastPort *rp), REG(a1, struct gClass *gc),
	REG(a2, struct gDiagram *gd), REG(a3, struct gBounds *gb))
{
	struct gBounds *agb;
	struct gLink *gl;
	long pseudo3D, depth, offset;
	long i, j, k;
	ULONG color;

DebugPrintF("Start\n");
	gSuperDraw(page, dpi, rp, gc, (struct gObject *)gd, gb);

	gDoMethod(gd, GCM_GET, GAA_Bounds, &agb);
	gDoMethod(gd, GCM_GET, GAA_Pseudo3D, &pseudo3D);
	gDoMethod(gd, GCM_GET, GAA_DepthWidth, &depth);

	if (pseudo3D) {
		if (page != NULL)
			offset = pixel(page, depth - GA_DEPTH_OFFSET, true);
		else
			offset = mm_to_pixel_dpi(dpi, depth - GA_DEPTH_OFFSET, true);
	}

	j = gd->gd_Rows;

	for (k = 0; k < gd->gd_Rows; k++) {
		long right, left, x, y, width, zero;

		j--;  // Change order of the rows

		gDoMethod(gd, GCAM_GETBORDERS, j, &left, &right, &zero);
		width = right - left;
DebugPrintF("r=%04d l=%04d w=%04d Cols=%04d Zero=%04d\n", right, left, width, gd->gd_Cols, zero);
		if (gd->gd_Cols > 1)
			width /= gd->gd_Cols - 1;

		/** top side **/
		if (pseudo3D) {
			for (x = left, i = 0; i < gd->gd_Cols; i++) {
				if (!(gl = gGetLink(gd, i, j)))
					continue;

				y = gDoMethod(gd, GCAM_GETCOORD, gl->gl_Value, j, 1);

				if (y < agb->gb_Top)
					y = agb->gb_Top;
				else if (y > agb->gb_Bottom)
					y = agb->gb_Bottom;

				if (i != 0) {
					gAreaMove(rp, x, y);
					gAreaDraw(rp, x + offset, y - offset);

					gAreaEnd(rp);
				} else
					SetHighColor(rp, color = TintColor(gl->gl_Color, 0.7f));

				if (i < gd->gd_Cols - 1) {
					gAreaDraw(rp, x + offset, y - offset);
					gAreaDraw(rp, x, y);
				}
				x += width;
			}
		}

		/** front side **/
		if (!pseudo3D) {
			for (x = left, i = 0; i < gd->gd_Cols; i++) {
				if (!(gl = gGetLink(gd, i, j)))
					continue;

				y = gDoMethod(gd,GCAM_GETCOORD,gl->gl_Value,j,1);
DebugPrintF("i=%03d x=%03d y=%03d\n", i, x, y); 
				if (y < agb->gb_Top)
					y = agb->gb_Top;
				else if (y > agb->gb_Bottom)
					y = agb->gb_Bottom;
				if (i == 0) {
					SetHighColor(rp, color = gl->gl_Color);
					Move(rp, x, y);
				} else 
					Draw(rp, x, y);

				x += width;
			}
		}
	}
	BNDRYOFF(rp);
DebugPrintF("Ende\n\n");
}


ULONG
set(struct gDiagram *gd, struct gArea *ga, struct TagItem *tstate)
{
	return GCPR_NONE;
}


ULONG PUBLIC
dispatch(REG(a0, struct gClass *gc), REG(a2, struct gDiagram *gd),
	REG(a1, Msg msg))
{
	ULONG rc;
	switch (msg->MethodID) {
		case GCM_NEW:
			if ((rc = gDoSuperMethodA(gc, gd, msg)) != 0) {
				set((struct gDiagram *)rc, NULL,
					((struct gcpSet *)msg)->gcps_AttrList);
			}
			break;
		case GCM_SET:
			rc = gDoSuperMethodA(gc, gd, msg)
				| set(gd, NULL, ((struct gcpSet *)msg)->gcps_AttrList);
			break;
		case GCM_GET:
			rc = TRUE;

			switch (((struct gcpGet *)msg)->gcpg_Tag) {
				case GAA_Pseudo3DDepth:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gd->gd_Rows;
					break;
				default:
					rc = gDoSuperMethodA(gc,gd,msg);
					break;
			}
			break;
		case GCDM_SETLINKATTR:
			if ((rc = gDoSuperMethodA(gc, gd, msg)) != 0) {
				// something has changed
				struct gcpSetLinkAttr *gcps = (APTR)msg;
				ULONG  color = gcps->gcps_Color;
				ULONG  row = gcps->gcps_Link->gl_Row, i;
				struct gLink *gl;

				for (i = 0; i < gd->gd_Cols; i++) {
					if ((gl = gGetLink(gd, i, row)) == NULL)
						continue;

					if (color != ~0L)
						gl->gl_Color = color;
				}
				return 1L;
			}
			break;

		default:
			return gDoSuperMethodA(gc, gd, msg);
	}
	return rc;
}


ULONG PUBLIC
freeClass(REG(a0, struct gClass *gc))
{
	CloseCatalog(sCatalog);
	return TRUE;
}


ULONG PUBLIC
initClass(REG(a0, struct gClass *gc))
{
	sCatalog = OpenCatalog(NULL, "ignition.catalog", OC_BuiltInLanguage,
		"deutsch", TAG_END);
	interface[0].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_PSEUDO_3D_GAD,
		"Pseudo-3D");

	return TRUE;
}

