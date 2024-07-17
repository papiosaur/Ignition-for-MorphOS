/* gClass for ignition
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
#include <proto/exec.h>
	#include "../ignition_strings.h"
#else
	#include "ignition_strings.h"
#endif
#include <string.h>


const char *version = "$VER: column_diagram.gc 0.8 (10.06.2024)";

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
struct gBalken
{
    UBYTE gb_Frame;
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

#define GBA_Frame GOA_TagBase + 300

struct gInterface interface[] =
{
	{GBA_Frame, NULL /*"Fläche durch einen Rahmen begrenzen"*/, GIT_CHECKBOX, NULL, NULL},
	{GAA_Pseudo3D, NULL /*"Pseudo-3D"*/, GIT_CHECKBOX, NULL, NULL},
	{0}
};

const STRPTR superClass = "axes";
ULONG instanceSize = sizeof(struct gBalken);

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
	struct gBalken *this_gb = GINST_DATA(gc, gd);
    struct gLink *gl;
	long   pseudo3D, depth, offset;
	long   i, j, k, o;

	gSuperDraw(page, dpi, rp, gc, (struct gObject *)gd, gb);

	gDoMethod(gd, GCM_GET, GAA_Bounds, &agb);
	gDoMethod(gd, GCM_GET, GAA_Pseudo3D, &pseudo3D);
	gDoMethod(gd, GCM_GET, GAA_DepthWidth, &depth);

    if (this_gb->gb_Frame)
		SetOutlinePen(rp, 1);

    if (pseudo3D)
    {
        j = gd->gd_Rows;
		if (page != NULL)
			depth = pixel(page, depth - GA_DEPTH_OFFSET, TRUE);
		else
			depth = mm_to_pixel_dpi(dpi, depth - GA_DEPTH_OFFSET, TRUE);

		offset = depth * 0.2 + 1;
	} else
		offset = 0;	

	for (k = 0; k < gd->gd_Rows; k++)
	{
		long left, right, width, innerWidth, x, zero;

		if (pseudo3D)  // Reihenfolge der Reihen verändern
			j--;
		else
			j = k;
		gDoMethod(gd, GCAM_GETBORDERS, j, &left, &right, &zero);
		zero -= offset;
		width = (right - left) / gd->gd_Cols;
		o = width * 0.15;

		innerWidth = width - 2*o;
		if (!pseudo3D)
		{
			innerWidth /= gd->gd_Rows;
			o += j * innerWidth;
		}

		for (x = left, i = 0; i < gd->gd_Cols; i++)
		{
			long y;

			if (!(gl = gGetLink(gd, i, j)))
				continue;

			y = gDoMethod(gd, GCAM_GETCOORD, gl->gl_Value, j, 1) - offset;
			SetHighColor(rp, gl->gl_Color);

#ifndef __amigaos4__
			if (y < agb->gb_Top)
				y = agb->gb_Top;
			else if (y > agb->gb_Bottom)
				y = agb->gb_Bottom;
#endif

			// vordere Fläche
			if (y < zero)
				RectFill(rp, x + o, y, x + o + innerWidth, zero);
			else if (pseudo3D)
            {
				if (y > zero + offset)
					RectFill(rp, x + o, zero + offset + 1, x + o + innerWidth, y);
			}
            else
				RectFill(rp, x + o, zero, x + o + innerWidth, y);

			if (pseudo3D)
            {
				SetHighColor(rp, TintColor(gl->gl_Color, 0.9f));

				// rechter Rand
				if (y < zero || y > zero + offset + depth)
                {					 
					// Kante am Y-Wert
					gAreaMove(rp, x + innerWidth + o, y);
					gAreaDraw(rp, x + innerWidth + o + depth, y - depth);
					
					// Kante an Achse
					if (y < zero)  // Werte größer NULL
					{
						gAreaDraw(rp, x + innerWidth + o + depth, zero - depth);
						gAreaDraw(rp, x + innerWidth + o, zero);
					}
					else  // Werte kleiner NULL
					{
						gAreaDraw(rp, x + innerWidth + o + depth, zero + offset + 1);
						gAreaDraw(rp, x + innerWidth + o, zero + offset + 1);
					}
					gAreaEnd(rp);
				}
				else if (y > zero + offset && y <= zero + depth + offset)
				{
					long xAtZero = -(zero + offset + 1 - y + depth) + x + innerWidth + o + depth;
						// Steigung ist -1

					// da ist nur ein Dreieck sichtbar
					gAreaMove(rp, x + innerWidth + o, y);
					gAreaDraw(rp, xAtZero, zero + offset + 1);
					gAreaDraw(rp, x + innerWidth + o, zero + offset + 1);
					gAreaEnd(rp);
				}

				// oberer Rand
				if (y > zero)
					y = zero;

				SetHighColor(rp, TintColor(gl->gl_Color, 0.7f));
				gAreaMove(rp, x + o, y);
				gAreaDraw(rp, x + o + depth, y - depth);
				gAreaDraw(rp, x + innerWidth + o + depth, y - depth);
				gAreaDraw(rp, x + innerWidth + o, y);
				gAreaEnd(rp);
			}
			x += width;
		}
	}
	BNDRYOFF(rp);
}


ULONG
set(struct gDiagram *gd, struct gBalken *gb, struct TagItem *tstate)
{
	struct TagItem *ti;
	ULONG rc = GCPR_NONE;

	if (tstate == NULL)
		return GCPR_NONE;


	while ((ti = NextTagItem(&tstate)) != 0)
	{
		switch(ti->ti_Tag)
		{
			case GBA_Frame:
				if (gb->gb_Frame != ti->ti_Data)
				{
					gb->gb_Frame = ti->ti_Data;
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
  struct gBalken *gb = GINST_DATA(gc,gd);
  ULONG  rc;

  switch (msg->MethodID)
  {
    case GCM_NEW:
      if ((rc = gDoSuperMethodA(gc, gd, msg)) != 0)
      {
        gb = GINST_DATA(gc,rc);
        gb->gb_Frame = 1;

        set((struct gDiagram *)rc,gb,((struct gcpSet *)msg)->gcps_AttrList);
      }
      break;
    case GCM_SET:
      rc = gDoSuperMethodA(gc,gd,msg) | set(gd,gb,((struct gcpSet *)msg)->gcps_AttrList);
      break;
    case GCM_GET:
      rc = TRUE;

      switch(((struct gcpGet *)msg)->gcpg_Tag)
      {
        case GBA_Frame:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gb->gb_Frame;
          break;
        case GAA_Pseudo3DDepth:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gd->gd_Rows;
          break;
        default:
          rc = gDoSuperMethodA(gc,gd,msg);
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

