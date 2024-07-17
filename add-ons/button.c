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

	#include <proto/utility.h>
#else
	#include "ignition_strings.h"
#endif

#include <string.h>

#ifdef __amigaos4__
	#undef  strcmp
	#define strcmp  Stricmp
#endif


const char *version = "$VER: button.gc 0.15 (6.8.2003)";
#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct gButton {
	STRPTR gb_Text, gb_ShowText;
	struct Term *gb_Term;
	struct FontInfo *gb_FontInfo;
	ULONG  gb_ShinePen, gb_ShadowPen, gb_FillPen, gb_TextPen;
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

#define GBA_ShadowPen GOA_TagUser + 1
#define GBA_ShinePen  GOA_TagUser + 2

struct gInterface interface[] = {
	{GBA_ShinePen, NULL /*"Helle Kanten:"*/, GIT_PEN, NULL, "shinepen"},
	{GBA_ShadowPen, NULL /*"Dunkle Kanten:"*/, GIT_PEN, NULL, "darkpen"},
	{GOA_FillPen,  NULL,GIT_PEN,NULL,NULL},
	{GOA_Pen,      NULL /*"Textfarbe:"*/, GIT_PEN, NULL, NULL},
	{GOA_FontInfo, NULL, GIT_FONT, NULL, NULL},
	{GOA_Text,     NULL, GIT_FORMULA, NULL, NULL},
	{GOA_Command,  NULL, GIT_TEXT, NULL, NULL},
	{GOA_ContinualCommand, NULL, GIT_CHECKBOX, NULL, NULL},
	{0}
};

const STRPTR superClass = "root";
ULONG instanceSize = sizeof(struct gButton);

static struct Catalog *sCatalog;


void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *grp), REG(a1, struct gClass *gc),
	REG(a2, struct gObject *go), REG(a3, struct gBounds *gbo))
{
	struct gButton *gb = GINST_DATA(gc, go);
	long bx, by, w, h;
	long x = gbo->gb_Left, y = gbo->gb_Top;
	long width = gbo->gb_Right - gbo->gb_Left;
	long height = gbo->gb_Bottom - gbo->gb_Top;

	bx = pixel(page, 1536, TRUE);  by = pixel(page, 1536, FALSE);

	SetHighColor(grp,gb->gb_FillPen);
	grp->Flags &= ~AREAOUTLINE;
	RectFill(grp, x + bx + 1, y + by + 1, x + width - 1 - bx, y + height - by - 1);

	SetColors(grp, gb->gb_ShadowPen, gb->gb_FillPen);
	if (go->go_Flags & GOF_PRESSED)
	{
		RectFill(grp, x + bx, y, x + width, y + by);
		RectFill(grp, x, y, x + bx, y + height);
	}
	else
	{
		RectFill(grp, x, y + height - by, x + width, y + height);
		RectFill(grp, x + width - bx, y, x + width, y + height - by);
	}

	if ((gb->gb_FontInfo = SetFontInfoA(gb->gb_FontInfo, dpi, NULL))
		&& (h = OutlineHeight(gb->gb_FontInfo, gb->gb_ShowText, -1)) < height - 2 * by)
	{
		w = OutlineLength(gb->gb_FontInfo, gb->gb_ShowText, -1);
		if (w < width)
		{
			SetHighColor(grp,gb->gb_TextPen);
			DrawText(grp,gb->gb_FontInfo,gb->gb_ShowText,x+(width-w)/2,y+(height-h)/2);
		}
	}

	SetHighColor(grp, gb->gb_ShinePen);//  SetOutlineColor(grp,TintColor(gb->gb_ShinePen,1.25f));
	if (go->go_Flags & GOF_PRESSED)
	{
		gAreaMove(grp, x + width, y + height);
		gAreaDraw(grp, x + width, y);
		gAreaDraw(grp, x + width - bx, y + by);
		gAreaDraw(grp, x + width - bx, y + height - by);
		gAreaEnd(grp);
		gAreaMove(grp, x + width, y + height);
		gAreaDraw(grp, x, y + height);
		gAreaDraw(grp, x + bx, y + height - by);
		gAreaDraw(grp, x + width - bx, y + height - by);
		gAreaEnd(grp);
	}
	else
	{
		gAreaMove(grp, x, y);
		gAreaDraw(grp, x, y + height);
		gAreaDraw(grp, x + bx, y + height - by);
		gAreaDraw(grp, x + bx, y + by);
		gAreaEnd(grp);
		gAreaMove(grp, x, y);
		gAreaDraw(grp, x + width, y);
		gAreaDraw(grp, x + width - bx, y + by);
		gAreaDraw(grp, x + bx, y + by);
		gAreaEnd(grp);
	}

	/* "Glanzleiste" */

	if (go->go_Flags & GOF_PRESSED)
		SetHighColor(grp, TintColor(gb->gb_ShinePen, 1.25f));
	else
		SetHighColor(grp, TintColor(gb->gb_ShadowPen, 0.75f));

	Move(grp,x+width,y);
	Draw(grp,x+width,y+height);
	Draw(grp,x,y+height);
	Move(grp,x+width,y+height);
	Draw(grp,x+width-bx,y+height-by);

	if (go->go_Flags & GOF_PRESSED)
		SetHighColor(grp, TintColor(gb->gb_ShadowPen, 0.75f));
	else
		SetHighColor(grp, TintColor(gb->gb_ShinePen, 1.25f));

	Move(grp, x, y + height);
	Draw(grp, x, y);
	Draw(grp, x + width, y);
	Move(grp, x, y);
	Draw(grp, x + bx, y + by);

	grp->Flags &= ~AREAOUTLINE;
}


ULONG
set(struct Page *page,struct gObject *go,struct gButton *gb,struct TagItem *tstate)
{
  struct TagItem *ti;
  ULONG  rc = GCPR_NONE;

  if (tstate)
  {
    while ((ti = NextTagItem(&tstate)) != 0)
    {
      switch(ti->ti_Tag)
      {
        case GOA_Text:
          if (!strcmp(gb->gb_Text ? gb->gb_Text : (STRPTR)"",ti->ti_Data ? (char *)ti->ti_Data : ""))
            break;

          FreeString(gb->gb_Text);
          FreeString(gb->gb_ShowText);
          DeleteTerm(gb->gb_Term);

          gb->gb_Text = AllocString((STRPTR)ti->ti_Data);
          gb->gb_Term = CreateTerm(page,gb->gb_Text);
          gb->gb_ShowText = CalcTerm(page,gb->gb_Text,gb->gb_Term,NULL);

          rc = GCPR_REDRAW;
          break;
        case GOA_FontInfo:
          FreeFontInfo(gb->gb_FontInfo);
          gb->gb_FontInfo = (struct FontInfo *)ti->ti_Data;
          rc = GCPR_REDRAW;
          break;
        case GBA_ShadowPen:
          if (gb->gb_ShadowPen != ti->ti_Data)
          {
            gb->gb_ShadowPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GBA_ShinePen:
          if (gb->gb_ShinePen != ti->ti_Data)
          {
            gb->gb_ShinePen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GOA_Pen:
          if (gb->gb_TextPen != ti->ti_Data)
          {
            gb->gb_TextPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GOA_FillPen:
          if (gb->gb_FillPen != ti->ti_Data)
          {
            gb->gb_FillPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GOA_ContinualCommand:
          go->go_Flags = (go->go_Flags & ~GOF_CONTINUALCMD) | (ti->ti_Data ? GOF_CONTINUALCMD : 0);
          break;
      }
    }
  }
  return rc;
}


ULONG PUBLIC
dispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
  struct gButton *gb = GINST_DATA(gc,go);
  ULONG rc = 0L;

  switch(msg->MethodID)
  {
    case GCM_NEW:
      if ((rc = gDoSuperMethodA(gc, go, msg)) != 0)
      {
        go = (struct gObject *)rc;  gb = GINST_DATA(gc,go);

        gb->gb_ShinePen = 0xd6d6d6;
        gb->gb_ShadowPen = 0x616161;
        gb->gb_TextPen = 0x000000;
        gb->gb_FillPen = 0xaaaaaa;
        go->go_Flags |= GOF_FRAMED;
        gb->gb_FontInfo = NewFontInfoA(NULL,STANDARD_DPI,NULL);

        set(go->go_Page,go,gb,((struct gcpSet *)msg)->gcps_AttrList);
      }
      break;
    case GCM_DISPOSE:
      FreeString(gb->gb_Text);
      FreeString(gb->gb_ShowText);
      FreeFontInfo(gb->gb_FontInfo);
      DeleteTerm(gb->gb_Term);

      rc = gDoSuperMethodA(gc,go,msg);
      break;
    case GCM_COPY:
    {
      struct gObject *cgo;

      if ((cgo = (struct gObject *)(rc = gDoSuperMethodA(gc, go, msg))) != 0)
      {
        struct gButton *cgb = GINST_DATA(gc,cgo);

        cgb->gb_Text = AllocString(gb->gb_Text);
        cgb->gb_Term = CopyTerm(gb->gb_Term);
        cgb->gb_ShowText = AllocString(gb->gb_ShowText);
        cgb->gb_FontInfo = CopyFontInfo(gb->gb_FontInfo);
      }
      break;
    }
    case GCM_SET:
      rc = gDoSuperMethodA(gc,go,msg) | set(go->go_Page,go,gb,((struct gcpSet *)msg)->gcps_AttrList);
      break;
    case GCM_GET:
      rc = TRUE;

      switch(((struct gcpGet *)msg)->gcpg_Tag)
      {
        case GOA_Text:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gb->gb_Text;
          break;
        case GOA_FontInfo:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gb->gb_FontInfo;
          break;
        case GBA_ShadowPen:
          *((struct gcpGet *)msg)->gcpg_Storage = gb->gb_ShadowPen;
          break;
        case GBA_ShinePen:
          *((struct gcpGet *)msg)->gcpg_Storage = gb->gb_ShinePen;
          break;
        case GOA_Pen:
          *((struct gcpGet *)msg)->gcpg_Storage = gb->gb_TextPen;
          break;
        case GOA_FillPen:
          *((struct gcpGet *)msg)->gcpg_Storage = gb->gb_FillPen;
          break;
        case GOA_ContinualCommand:
          *((struct gcpGet *)msg)->gcpg_Storage = go->go_Flags & GOF_CONTINUALCMD ? TRUE : FALSE;
          break;
        default:
          rc = gDoSuperMethodA(gc,go,msg);
      }
      break;
    case GCM_RECALC:
    {
      STRPTR t = gb->gb_ShowText;

      gb->gb_ShowText = CalcTerm(go->go_Page,gb->gb_Text,gb->gb_Term,NULL);

      if (strcmp(gb->gb_ShowText ? gb->gb_ShowText : (STRPTR)"",t ? t : (STRPTR)""))
        rc = GCPR_REDRAW;

      FreeString(t);
      break;
    }
/*    case GCM_UPDATE:
      //gb->gb_FontInfo = SetFontInfo(gb->gb_FontInfo,((struct gcpUpdate *)msg)->gcpu_DPI,TAG_END);
      break;*/
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

	interface[0].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_BRIGHT_EDGES_LABEL, "Helle Kanten:");
	interface[1].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_DARK_EDGES_LABEL, "Dunkle Kanten:");
	interface[3].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_TEXT_COLOR_LABEL, "Textfarbe:");

	return TRUE;
}
