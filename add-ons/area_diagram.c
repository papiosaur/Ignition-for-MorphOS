/* gClass-diagram for ignition
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
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
#else
#include "ignition_strings.h"
#endif

#include <string.h>

const char *version = "$VER: area_diagram.gc 0.5 (7.8.2003)";

/** private instance structure **/
#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct gArea
{
  UBYTE ga_Frame;
};
#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack()
		#endif
	#elif defined(__VBCC__)
		#pragma default-align
	#endif
#endif

/** interface definition **/

#define GAA_Frame GOA_TagBase + 300

struct gInterface interface[] = {
	{GAA_Frame,	NULL /*"Fläche durch einen Rahmen begrenzen"*/, GIT_CHECKBOX, NULL, NULL},
	{GAA_Pseudo3D, NULL /*"Pseudo-3D"*/, GIT_CHECKBOX, NULL, NULL},
	{0}
};

const STRPTR superClass = "axes";
ULONG instanceSize = sizeof(struct gArea);

static struct Catalog *sCatalog;
 

long
mm_to_pixel_dpi(ULONG dpi, LONG mm, BOOL horiz)
{
	dpi = horiz ? dpi >> 16 : dpi & 0xffff;
	return (long)(mm * dpi / (25.4*1024));
}


void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *rp), REG(a1, struct gClass *gc),
	REG(a2, struct gDiagram *gd), REG(a3, struct gBounds *gb))
{
    struct gBounds *agb;
	struct gArea *this_ga = GINST_DATA(gc, gd);
    struct gLink *gl;
    long   pseudo3D, depth, offset;
    long   i,j,k;
    ULONG  color;

	gSuperDraw(page, dpi, rp, gc, (struct gObject *)gd, gb);

	gDoMethod(gd, GCM_GET, GAA_Bounds, &agb);
	gDoMethod(gd, GCM_GET, GAA_Pseudo3D, &pseudo3D);
	gDoMethod(gd, GCM_GET, GAA_DepthWidth, &depth);

    if (this_ga->ga_Frame)
		SetOutlinePen(rp, 1);

    if (pseudo3D)
	{
		if (page != NULL)
			depth = pixel(page, depth - GA_DEPTH_OFFSET, TRUE);
		else
			depth = mm_to_pixel_dpi(dpi, depth - GA_DEPTH_OFFSET, TRUE);

		offset = depth * 0.2 + 1;
	}
	else
		offset = 0;

	j = gd->gd_Rows;
 
	for (k = 0; k < gd->gd_Rows; k++)
	{
		long right, left, x, y, width, zero;

		j--;  // Reihenfolge der Reihen verändern

		gDoMethod(gd, GCAM_GETBORDERS, j, &left, &right, &zero);
		
		zero -= offset;
		width = right - left;
		if (gd->gd_Cols > 1)
			width /= gd->gd_Cols - 1;

		/** Oberseite (nur 3D) **/
		if (pseudo3D)
		{
			//UBYTE lastsign,sign;
			//long lasty;

			for (x = left, i = 0; i < gd->gd_Cols; i++)
			{
				if (!(gl = gGetLink(gd, i, j)))
					continue;

				y = gDoMethod(gd, GCAM_GETCOORD, gl->gl_Value, j, 1) - offset;

				if (y < agb->gb_Top)
					y = agb->gb_Top;
				else if (y > agb->gb_Bottom)
					y = agb->gb_Bottom;

				if (i != 0)
				{
					gAreaMove(rp, x, y);
					gAreaDraw(rp, x + depth, y - depth);

					gAreaEnd(rp);
				}
				else
					SetHighColor(rp, color = TintColor(gl->gl_Color, 0.7f));

				if (i < gd->gd_Cols - 1)
				{
					gAreaDraw(rp, x + depth, y - depth);
					gAreaDraw(rp, x, y);
				}
				x += width;
			}

			// Oberflächen bei Vorzeichenwechsel
			x -= width;
			gAreaMove(rp, left, zero);
			gAreaDraw(rp, left + depth, zero - depth);
			gAreaDraw(rp, x + depth, zero - depth);
			gAreaDraw(rp, x , zero);
			gAreaEnd(rp);
		}

		/** Vorderseite (immer) **/
		for (x = left, i = 0; i < gd->gd_Cols; i++)
		{
			if (!(gl = gGetLink(gd, i, j)))
				continue;

			y = gDoMethod(gd, GCAM_GETCOORD, gl->gl_Value, j, 1) - offset;

			if (y < agb->gb_Top)
				y = agb->gb_Top;
			else if (y > agb->gb_Bottom)
				y = agb->gb_Bottom;

			if (i == 0)
			{
				SetHighColor(rp, color = gl->gl_Color);
				gAreaMove(rp, x, zero);
			}
			gAreaDraw(rp, x, y);

			x += width;
		}
		x -= width;
		gAreaDraw(rp, x, zero);
		gAreaEnd(rp);

		// rechte Seite
		if (pseudo3D && (y < zero || y > zero + offset))
		{
			SetHighColor(rp, TintColor(color, 0.9f));
							
			if (y > zero)
				zero += offset + 1;

			gAreaMove(rp, x, zero);
			gAreaDraw(rp, x, y);
			gAreaDraw(rp, x + depth, y - depth);
			gAreaDraw(rp, x + depth, zero - depth);
			gAreaEnd(rp);
		}
	}
	BNDRYOFF(rp);
}


ULONG
set(struct gDiagram *gd,struct gArea *ga,struct TagItem *tstate)
{
	struct TagItem *ti;
	ULONG  rc = GCPR_NONE;

	if (tstate == NULL)
		return GCPR_NONE;

	while ((ti = NextTagItem(&tstate)) != 0) {
		switch (ti->ti_Tag) {
			case GAA_Frame:
				if (ga->ga_Frame != ti->ti_Data) {
					ga->ga_Frame = ti->ti_Data;
					rc |= GCPR_REDRAW;
				}
				break;
		}
    }

	return rc;
}


ULONG PUBLIC
dispatch(REG(a0, struct gClass *gc), REG(a2, struct gDiagram *gd), REG(a1, Msg msg))
{
  struct gArea *ga = GINST_DATA(gc,gd);
  ULONG  rc;

  switch(msg->MethodID)
  {
    case GCM_NEW:
      if ((rc = gDoSuperMethodA(gc, gd, msg)) != 0)
      {
        ga = GINST_DATA(gc,rc);
        ga->ga_Frame = 1;

        set((struct gDiagram *)rc,ga,((struct gcpSet *)msg)->gcps_AttrList);
      }
      break;
    case GCM_SET:
      rc = gDoSuperMethodA(gc,gd,msg) | set(gd,ga,((struct gcpSet *)msg)->gcps_AttrList);
      break;
    case GCM_GET:
      rc = TRUE;

      switch(((struct gcpGet *)msg)->gcpg_Tag)
      {
        case GAA_Frame:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)ga->ga_Frame;
          break;
        case GAA_Pseudo3DDepth:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gd->gd_Rows;
          break;
        default:
          rc = gDoSuperMethodA(gc,gd,msg);
      }
      break;
    /*case GCM_HITTEST:
      break;*/
    case GCDM_SETLINKATTR:
      if ((rc = gDoSuperMethodA(gc, gd, msg)) != 0) // something has changed
      {
        struct gcpSetLinkAttr *gcps = (APTR)msg;
        ULONG  color = gcps->gcps_Color;
        ULONG  row = gcps->gcps_Link->gl_Row,i;
        struct gLink *gl;

        for(i = 0;i < gd->gd_Cols;i++)
        {
          if ((gl = gGetLink(gd,i,row)) == NULL)
            continue;

          if (color != ~0L)
            gl->gl_Color = color;
        }
		return 1L;
      }
      break;
    default:
      rc = gDoSuperMethodA(gc,gd,msg);
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
	sCatalog = OpenCatalog(NULL, "ignition.catalog", OC_BuiltInLanguage, "deutsch", TAG_END);
	interface[0].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_AREA_OUTLINE_GAD, "Fläche durch einen Rahmen begrenzen");
	interface[1].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_PSEUDO_3D_GAD, "Pseudo-3D");

	return TRUE;
}
