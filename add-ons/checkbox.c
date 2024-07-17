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

#include <string.h>

#ifdef __amigaos4__
	#undef  strcmp
	#define strcmp  Stricmp
#endif

const char *version = "$VER: checkbox.gc 0.3 (6.8.2003)";

#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct Checkbox
{
  STRPTR cb_Text;
  struct FontInfo *cb_FontInfo;
  ULONG  cb_ShinePen,cb_ShadowPen,cb_FillPen;
  ULONG  cb_TextPen,cb_CheckPen;
  UBYTE  cb_Checked,cb_TextPlacement;
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

#define GCA_ShadowPen GOA_TagUser + 1
#define GCA_ShinePen  GOA_TagUser + 2
#define GCA_Checked   GOA_TagUser + 3
#define GCA_CheckPen  GOA_TagUser + 4
#define GCA_PlaceText GOA_TagUser + 5

static CONST_STRPTR sPlaceTextLabels[3];
#define PLACE_TEXT_RIGHT 0
#define PLACE_TEXT_LEFT 1

struct gInterface interface[] =
{
	{GCA_ShinePen, NULL /*"Helle Kanten:"*/, GIT_PEN, NULL, "shinepen"},
	{GCA_ShadowPen, NULL /*"Dunkle Kanten:"*/, GIT_PEN, NULL, "darkpen"},
	{GOA_FillPen,  NULL,GIT_PEN,NULL,NULL},
	{GOA_Pen,      NULL /*"Textfarbe:"*/, GIT_PEN, NULL, NULL},
	{GCA_CheckPen, "Markierungsfarbe",GIT_PEN,NULL,"checkpen"},
	{GCA_Checked,  "Markiert",GIT_CHECKBOX,NULL,"checked"},
	{GOA_FontInfo, NULL, GIT_FONT, NULL, NULL},
	{GOA_Text,     NULL,GIT_FORMULA,NULL,NULL},
	{GOA_Command,  NULL,GIT_TEXT,NULL,NULL},
	{GCA_PlaceText, "Textplazierung:", GIT_CYCLE, &sPlaceTextLabels, "placetext"},
	{0}
};

const STRPTR superClass = "root";
ULONG instanceSize = sizeof(struct Checkbox);

static struct Catalog *sCatalog;


void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *grp), REG(a1, struct gClass *gc), REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{
  struct Checkbox *cb = GINST_DATA(gc,go);
  long bx,by,bx2,h;
  long x = gb->gb_Left,y = gb->gb_Top;
  long size = gb->gb_Bottom - gb->gb_Top;
  long width = gb->gb_Right - gb->gb_Left;

  if (size > (gb->gb_Right - gb->gb_Left))
    size = gb->gb_Right - gb->gb_Left;
  width -= size;

  if (cb->cb_TextPlacement == PLACE_TEXT_LEFT)
    x += width;

  bx = pixel(page,1536,TRUE);  by = pixel(page,1536,FALSE);  bx2 = 2*bx;
  SetHighColor(grp,cb->cb_FillPen);
  grp->Flags &= ~AREAOUTLINE;
  RectFill(grp,x+bx+1,y+by+1,x+size-1-bx,y+size-by-1);

  SetColors(grp,cb->cb_ShadowPen,cb->cb_FillPen);
  RectFill(grp,x+bx,y,x+size,y+by);
  RectFill(grp,x,y,x+bx,y+size);

  SetHighColor(grp,TintColor(cb->cb_ShadowPen,0.75f));
  Move(grp,x,y+size);
  Draw(grp,x,y);
  Draw(grp,x+size,y);
  Move(grp,x,y);
  Draw(grp,x+bx,y+by);

  if ((cb->cb_FontInfo = SetFontInfoA(cb->cb_FontInfo,dpi,NULL)) && (h = OutlineHeight(cb->cb_FontInfo,cb->cb_Text,-1)) < size)
  {
    long w = OutlineLength(cb->cb_FontInfo,cb->cb_Text,-1);

    if (w < width - bx2)
    {
      long xpos;

      SetHighColor(grp,cb->cb_TextPen);
      if (cb->cb_TextPlacement == PLACE_TEXT_RIGHT)
        xpos = gb->gb_Left + size + bx2;
      else
        xpos = gb->gb_Right - w - size - bx2;

      DrawText(grp,cb->cb_FontInfo,cb->cb_Text,xpos,y+(size-h)/2);
    }
  }
  SetHighColor(grp,cb->cb_ShinePen);

  gAreaMove(grp, x + size, y + size);
  gAreaDraw(grp, x + size, y);
  gAreaDraw(grp, x + size - bx, y + by);
  gAreaDraw(grp, x + size - bx, y + size - by);
  gAreaEnd(grp);
  gAreaMove(grp, x + size, y + size);
  gAreaDraw(grp, x, y + size);
  gAreaDraw(grp, x + bx, y + size - by);
  gAreaDraw(grp, x + size - bx, y + size - by);
  gAreaEnd(grp);

  SetHighColor(grp,TintColor(cb->cb_ShinePen,1.25f));
  Move(grp,x+size,y);
  Draw(grp,x+size,y+size);
  Draw(grp,x,y+size);
  Move(grp,x+size,y+size);
  Draw(grp,x+size-bx,y+size-by);

  grp->Flags &= ~AREAOUTLINE;

  if (cb->cb_Checked)
  {
    long s10 = (size - bx2)/10;
    long s4 = (size - bx2)/3;
    long s2 = (size - bx2)/2;
    long ax = x+bx,ay = y+by;

    SetHighColor(grp,cb->cb_CheckPen);

    gAreaMove(grp, ax + s2 - s4, ay + s2 + s4 - s10);
    gAreaDraw(grp, ax + s2 + s4 - s10, ay + s2 - s4);
    gAreaDraw(grp, ax + s2 + s4, ay + s2 - s4 + s10);
    gAreaDraw(grp, ax + s2 - s4 + s10, ay + s2 + s4);
    gAreaEnd(grp);

    gAreaMove(grp, ax + s2 - s4 + s10, ay + s2 - s4);
    gAreaDraw(grp, ax + s2 + s4, ay + s2 + s4 - s10);
    gAreaDraw(grp, ax + s2 + s4 - s10, ay + s2 + s4);
    gAreaDraw(grp, ax + s2 - s4, ay + s2 - s4 + s10);
    gAreaEnd(grp);

    //RectFill(grp,x+width/3,y+size/3,x+width/2,y+size/2);
  }
  if (go->go_Flags & GOF_PRESSED)
  {
    SetHighColor(grp,0x000000);
    grp->linpatcnt = 15;  grp->Flags |= FRST_DOT;
    grp->LinePtrn = 0xaaaa;
    DrawRect(grp,x+bx+2,y+by+2,size-4-bx2,size-4-2*by);
    grp->LinePtrn = 0xffff;
  }
}


ULONG set(struct Page *page,struct gObject *go,struct Checkbox *cb,struct TagItem *tstate)
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
          if (!strcmp(cb->cb_Text ? cb->cb_Text : (STRPTR)"",ti->ti_Data ? (char *)ti->ti_Data : ""))
            break;

          FreeString(cb->cb_Text);
          cb->cb_Text = AllocString((STRPTR)ti->ti_Data);

          rc = GCPR_REDRAW;
          break;
        case GOA_FontInfo:
          FreeFontInfo(cb->cb_FontInfo);
          cb->cb_FontInfo = (struct FontInfo *)ti->ti_Data;
          rc = GCPR_REDRAW;
          break;
        case GCA_ShadowPen:
          if (cb->cb_ShadowPen != ti->ti_Data)
          {
            cb->cb_ShadowPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GCA_ShinePen:
          if (cb->cb_ShinePen != ti->ti_Data)
          {
            cb->cb_ShinePen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GOA_Pen:
          if (cb->cb_TextPen != ti->ti_Data)
          {
            cb->cb_TextPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GOA_FillPen:
          if (cb->cb_FillPen != ti->ti_Data)
          {
            cb->cb_FillPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GCA_CheckPen:
          if (cb->cb_CheckPen != ti->ti_Data)
          {
            cb->cb_CheckPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GCA_Checked:
          if ((cb->cb_Checked != 0) == (ti->ti_Data != 0))
            break;

          cb->cb_Checked = ti->ti_Data;
          rc = GCPR_REDRAW;
          break;
        case GCA_PlaceText:
          if (cb->cb_TextPlacement == ti->ti_Data)
            break;

          cb->cb_TextPlacement = ti->ti_Data;
          rc = GCPR_REDRAW;
          break;
      }
    }
  }
  return(rc);
}


ULONG PUBLIC dispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
  struct Checkbox *cb = GINST_DATA(gc,go);
  ULONG rc = 0L;

  switch(msg->MethodID)
  {
    case GCM_NEW:
      if ((rc = gDoSuperMethodA(gc, go, msg)) != 0)
      {
        go = (struct gObject *)rc;  cb = GINST_DATA(gc,go);

        cb->cb_ShinePen = 0xd6d6d6;
        cb->cb_ShadowPen = 0x616161;
        cb->cb_TextPen = 0x000000;
        cb->cb_FillPen = 0xaaaaaa;
        go->go_Flags |= GOF_FRAMED;
        cb->cb_FontInfo = NewFontInfoA(NULL,STANDARD_DPI,NULL);

        set(go->go_Page,go,cb,((struct gcpSet *)msg)->gcps_AttrList);
      }
      break;
    case GCM_DISPOSE:
      FreeString(cb->cb_Text);
      FreeFontInfo(cb->cb_FontInfo);

      rc = gDoSuperMethodA(gc,go,msg);
      break;
    case GCM_COPY:
    {
      struct gObject *cgo;

      if ((cgo = (struct gObject *)(rc = gDoSuperMethodA(gc, go, msg))) != 0)
      {
        struct Checkbox *ccb = GINST_DATA(gc,cgo);

        ccb->cb_Text = AllocString(cb->cb_Text);
        ccb->cb_FontInfo = CopyFontInfo(cb->cb_FontInfo);
      }
      break;
    }
    case GCM_SET:
      rc = gDoSuperMethodA(gc,go,msg) | set(go->go_Page,go,cb,((struct gcpSet *)msg)->gcps_AttrList);
      break;
    case GCM_GET:
      rc = TRUE;

      switch(((struct gcpGet *)msg)->gcpg_Tag)
      {
        case GOA_Text:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)cb->cb_Text;
          break;
        case GOA_FontInfo:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)cb->cb_FontInfo;
          break;
        case GCA_ShadowPen:
          *((struct gcpGet *)msg)->gcpg_Storage = cb->cb_ShadowPen;
          break;
        case GCA_ShinePen:
          *((struct gcpGet *)msg)->gcpg_Storage = cb->cb_ShinePen;
          break;
        case GOA_Pen:
          *((struct gcpGet *)msg)->gcpg_Storage = cb->cb_TextPen;
          break;
        case GOA_FillPen:
          *((struct gcpGet *)msg)->gcpg_Storage = cb->cb_FillPen;
          break;
        case GOA_ContinualCommand:
          *((struct gcpGet *)msg)->gcpg_Storage = go->go_Flags & GOF_CONTINUALCMD ? TRUE : FALSE;
          break;
        case GCA_CheckPen:
          *((struct gcpGet *)msg)->gcpg_Storage = cb->cb_CheckPen;
          break;
        case GCA_Checked:
          *((struct gcpGet *)msg)->gcpg_Storage = cb->cb_Checked;
          break;
        default:
          rc = gDoSuperMethodA(gc,go,msg);
      }
      break;
    case GCM_COMMAND:
      /* Befehl ausführen */
      cb->cb_Checked = !cb->cb_Checked;
      gDoSuperMethod(gc,go,GCM_UPDATE_UI);

      return(gDoSuperMethodA(gc,go,msg));
    case GCM_RECALC:
/*    {
      STRPTR t = cb->cb_ShowText;

      cb->cb_ShowText = CalcTerm(go->go_Page,cb->cb_Text,cb->cb_Term,NULL);

      if (strcmp(cb->cb_ShowText ? cb->cb_ShowText : (STRPTR)"",t ? t : (STRPTR)""))
        rc = GCPR_REDRAW;

      FreeString(t);*/
      break;
//    }
/*    case GCM_UPDATE:
      //cb->cb_FontInfo = SetFontInfo(cb->cb_FontInfo,((struct gcpUpdate *)msg)->gcpu_DPI,TAG_END);
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
	interface[4].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_CHECKED_COLOR_LABEL, "Markierungsfarbe:");
	interface[5].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_CHECKED_GAD, "Markiert");
	interface[9].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_TEXT_PLACEMENT_LABEL, "Textplazierung:");

	sPlaceTextLabels[0] = GetCatalogStr(sCatalog, MSG_PLACE_RIGHT_GAD, "rechts");
	sPlaceTextLabels[1] = GetCatalogStr(sCatalog, MSG_PLACE_LEFT_GAD, "links");

	return TRUE;
}
