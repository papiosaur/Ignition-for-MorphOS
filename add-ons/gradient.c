/* gClass for ignition
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


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

const char *version = "$VER: gradient.gc 0.3 (7.8.2003)";

#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct Gradient {
    ULONG g_FirstPen, g_SecondPen;
	UBYTE g_Direction;
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

#define G_HORIZONTAL 0
#define G_VERTICAL 1
 
#define GGA_FirstPen    (GOA_TagUser + 1)
#define GGA_SecondPen   (GOA_TagUser + 2)
#define GGA_Direction   (GOA_TagUser + 3)

static struct Catalog *sCatalog;
static CONST_STRPTR sAlignmentLabels[3];

struct gInterface interface[] = {
	{GGA_FirstPen,    NULL /*"First Colour:"*/, GIT_PEN, NULL, "firstpen"},
	{GGA_SecondPen,   NULL /*"Second Colour:"*/, GIT_PEN, NULL, "secondpen"},
	{GGA_Direction,   NULL /*"Direction:"*/, GIT_CYCLE, sAlignmentLabels, "direction"},
    {0,            NULL,0,NULL,NULL}
};

const STRPTR superClass = "root";
ULONG instanceSize = sizeof(struct Gradient);


void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *grp), REG(a1, struct gClass *gc), REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{
    struct Gradient *g = GINST_DATA(gc, go);
    long x = gb->gb_Left,y = gb->gb_Top;
    long width = gb->gb_Right - gb->gb_Left;
    long height = gb->gb_Bottom - gb->gb_Top;
    UBYTE red, green, blue, firstRed, secondRed, firstBlue, secondBlue, firstGreen, secondGreen;
    ULONG i;

    grp->Flags &= ~AREAOUTLINE;
    firstRed = (g->g_FirstPen >> 16) & 0xff;
    firstGreen = (g->g_FirstPen >> 8) & 0xff;
    firstBlue = g->g_FirstPen & 0xff;
    secondRed = (g->g_SecondPen >> 16) & 0xff;
    secondGreen = (g->g_SecondPen >> 8) & 0xff;
    secondBlue = g->g_SecondPen & 0xff;

	if (!g->g_Direction)
    {
        for (i = 0; i <= width; i++)
        {
            red = firstRed + (secondRed - firstRed) * (1. * i / width);
            green = firstGreen + (secondGreen - firstGreen) * (1. * i / width);
            blue = firstBlue + (secondBlue - firstBlue) * (1. * i / width);

            SetHighColor(grp, (red << 16) | (green << 8) | (blue));
            RectFill(grp, x + i, y, x + i, y + height);
        }
    }
    else
    {
        for (i = 0; i <= height; i++)
        {
            red = firstRed + (secondRed - firstRed) * (1. * i / height);
            green = firstGreen + (secondGreen - firstGreen) * (1. * i / height);
            blue = firstBlue + (secondBlue - firstBlue) * (1. * i / height);

            SetHighColor(grp, (red << 16) | (green << 8) | (blue));
            RectFill(grp, x, y + i, x + width, y + i);
        }
    }
}


ULONG
set(struct Gradient *g, struct TagItem *tstate)
{
    struct TagItem *ti;
    ULONG  rc = GCPR_NONE;

    if (!tstate)
        return GCPR_NONE;

    while ((ti = NextTagItem(&tstate)) != 0)
    {
        switch (ti->ti_Tag)
        {
            case GGA_FirstPen:
                if (g->g_FirstPen != ti->ti_Data)
                {
                    g->g_FirstPen = ti->ti_Data;
                    rc = GCPR_REDRAW;
                }
                break;
            case GGA_SecondPen:
                if (g->g_SecondPen != ti->ti_Data)
                {
                    g->g_SecondPen = ti->ti_Data;
                    rc = GCPR_REDRAW;
                }
                break;
			case GGA_Direction:
				if (g->g_Direction != ti->ti_Data)
                {
					g->g_Direction = ti->ti_Data;
                    rc = GCPR_REDRAW;
                }
                break;
        }
    }

    return rc;
}


ULONG PUBLIC
dispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
    struct Gradient *g = GINST_DATA(gc, go);
    ULONG rc = 0L;

    switch (msg->MethodID)
    {
        case GCM_NEW:
            if ((rc = gDoSuperMethodA(gc, go, msg)) != 0)
            {
                go = (struct gObject *)rc;  g = GINST_DATA(gc, go);

                g->g_FirstPen = (240 << 16) | (240 << 8) | 240; // almost white
                g->g_SecondPen = (0 << 16) | (0 << 8) | 60;     // dark blue
				g->g_Direction = G_VERTICAL;

                set(g, ((struct gcpSet *)msg)->gcps_AttrList);
            }
            break;
        case GCM_SET:
            rc = gDoSuperMethodA(gc, go, msg) | set(g, ((struct gcpSet *)msg)->gcps_AttrList);
            break;
        case GCM_GET:
            rc = TRUE;

			switch (((struct gcpGet *)msg)->gcpg_Tag)
            {
                case GGA_FirstPen:
                    *((struct gcpGet *)msg)->gcpg_Storage = g->g_FirstPen;
                    break;
                case GGA_SecondPen:
                    *((struct gcpGet *)msg)->gcpg_Storage = g->g_SecondPen;
                    break;
				case GGA_Direction:
					*((struct gcpGet *)msg)->gcpg_Storage = g->g_Direction;
                    break;
                default:
                    rc = gDoSuperMethodA(gc, go, msg);
            }
            break;
        default:
            return gDoSuperMethodA(gc, go, msg);
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
	interface[0].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_FIRST_COLOR_LABEL, "Erste Farbe:");
	interface[1].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_SECOND_COLOR_LABEL, "Zweite Farbe:");
	interface[2].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_DIRECTION_LABEL, "Richtung:");

	sAlignmentLabels[0] = GetCatalogStr(sCatalog, MSG_ORDER_HORIZONTAL_GAD, "horizontal");
	sAlignmentLabels[1] = GetCatalogStr(sCatalog, MSG_ORDER_VERTICAL_GAD, "vertikal");

    return TRUE;
}


/** this file is linked without startup code (SAS/C specific) */

#if defined(__SASC)
void STDARGS
_XCEXIT(void)
{
}
#endif
