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


const char *version = "$VER: frame.gc 0.3 (7.8.2003)";

#define BORDER_SIZE 1536

#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct Frame
{
  ULONG  f_ShinePen, f_ShadowPen;
  UBYTE  f_Flags;
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

#define FF_RECESSED 1
#define FF_DOUBLED 2

#define GFA_ShadowPen GOA_TagUser + 1
#define GFA_ShinePen  GOA_TagUser + 2
#define GFA_Recessed  GOA_TagUser + 3
#define GFA_Doubled   GOA_TagUser + 4

struct gInterface interface[] = {
	{GFA_ShinePen, NULL /*"Helle Kanten:"*/, GIT_PEN, NULL, "shinepen"},
	{GFA_ShadowPen, NULL /*"Dunkle Kanten:"*/, GIT_PEN, NULL, "darkpen"},
	{GFA_Recessed, NULL /*"Eingedrückt:"*/, GIT_CHECKBOX, NULL, "recessed"},
	{GFA_Doubled,  NULL /*"Doppelter Rahmen:"*/, GIT_CHECKBOX, NULL, "doubled"},
	{0}
};
			
const STRPTR superClass = "root";
ULONG instanceSize = sizeof(struct Frame);

static struct Catalog *sCatalog;


static void
drawFrame(struct Frame *f,struct RastPort *grp,long x,long y,long width,long height,long bx,long by,BOOL recessed)
{
  SetHighColor(grp,f->f_ShadowPen);
  if (recessed)
  {
    RectFill(grp,x+bx,y,x+width,y+by);
    RectFill(grp,x,y,x+bx,y+height);
  }
  else
  {
    RectFill(grp,x,y+height-by,x+width,y+height);
    RectFill(grp,x+width-bx,y,x+width,y+height-by);
  }

  SetHighColor(grp,f->f_ShinePen);
  if (recessed)
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

  if (recessed)
    SetHighColor(grp,TintColor(f->f_ShinePen,1.25f));
  else
    SetHighColor(grp,TintColor(f->f_ShadowPen,0.75f));

  Move(grp,x+width,y);
  Draw(grp,x+width,y+height);
  Draw(grp,x,y+height);
  Move(grp,x+width,y+height);
  Draw(grp,x+width-bx,y+height-by);

  if (recessed)
    SetHighColor(grp,TintColor(f->f_ShadowPen,0.75f));
  else
    SetHighColor(grp,TintColor(f->f_ShinePen,1.25f));

  Move(grp,x,y+height);
  Draw(grp,x,y);
  Draw(grp,x+width,y);
  Move(grp,x,y);
  Draw(grp,x+bx,y+by);
}


void PUBLIC draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *grp), REG(a1, struct gClass *gc), REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{
  struct Frame *f = GINST_DATA(gc,go);
  long bx,by;
  long x = gb->gb_Left,y = gb->gb_Top;
  long width = gb->gb_Right - gb->gb_Left;
  long height = gb->gb_Bottom - gb->gb_Top;

  bx = pixel(page,BORDER_SIZE,TRUE);  by = pixel(page,BORDER_SIZE,FALSE);

  drawFrame(f,grp,x,y,width,height,bx,by,f->f_Flags & FF_RECESSED);

  if (f->f_Flags & FF_DOUBLED)
    drawFrame(f,grp,x+bx+1,y+by+1,width-2-2*bx,height-2-2*by,bx,by,!(f->f_Flags & FF_RECESSED));
}


ULONG set(struct Page *page,struct gObject *go,struct Frame *f,struct TagItem *tstate)
{
  struct TagItem *ti;
  ULONG  rc = GCPR_NONE;

  if (tstate)
  {
    while ((ti = NextTagItem(&tstate)) != 0)
    {
      switch(ti->ti_Tag)
      {
        case GFA_ShadowPen:
          if (f->f_ShadowPen != ti->ti_Data)
          {
            f->f_ShadowPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GFA_ShinePen:
          if (f->f_ShinePen != ti->ti_Data)
          {
            f->f_ShinePen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GFA_Recessed:
          if (((f->f_Flags & FF_RECESSED) != 0) != (ti->ti_Data != 0))
          {
            f->f_Flags = (f->f_Flags & ~FF_RECESSED) | (ti->ti_Data ? FF_RECESSED : 0);
            rc = GCPR_REDRAW;
          }
          break;
        case GFA_Doubled:
          if (((f->f_Flags & FF_DOUBLED) != 0) != (ti->ti_Data != 0))
          {
            f->f_Flags = (f->f_Flags & ~FF_DOUBLED) | (ti->ti_Data ? FF_DOUBLED : 0);
            rc = GCPR_REDRAW;
          }
          break;
      }
    }
  }
  return(rc);
}


ULONG PUBLIC dispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
  struct Frame *f = GINST_DATA(gc,go);
  ULONG rc = 0L;

  switch(msg->MethodID)
  {
    case GCM_NEW:
      if ((rc = gDoSuperMethodA(gc, go, msg)) != 0)
      {
        go = (struct gObject *)rc;  f = GINST_DATA(gc,go);

        f->f_ShinePen = 0xd6d6d6;
        f->f_ShadowPen = 0x616161;
        go->go_Flags |= GOF_FRAMED;

        set(go->go_Page,go,f,((struct gcpSet *)msg)->gcps_AttrList);
      }
      break;
    case GCM_SET:
      rc = gDoSuperMethodA(gc,go,msg) | set(go->go_Page,go,f,((struct gcpSet *)msg)->gcps_AttrList);
      break;
    case GCM_GET:
      rc = TRUE;

      switch(((struct gcpGet *)msg)->gcpg_Tag)
      {
        case GFA_ShadowPen:
          *((struct gcpGet *)msg)->gcpg_Storage = f->f_ShadowPen;
          break;
        case GFA_ShinePen:
          *((struct gcpGet *)msg)->gcpg_Storage = f->f_ShinePen;
          break;
        case GFA_Recessed:
          *((struct gcpGet *)msg)->gcpg_Storage = (f->f_Flags & FF_RECESSED) != 0;
          break;
        case GFA_Doubled:
          *((struct gcpGet *)msg)->gcpg_Storage = (f->f_Flags & FF_DOUBLED) != 0;
          break;
        default:
          rc = gDoSuperMethodA(gc,go,msg);
      }
      break;
    case GCM_HITTEST:
    {
      LONG x = ((struct gcpHitTest *)msg)->gcph_X - go->go_mmLeft;
      LONG y = ((struct gcpHitTest *)msg)->gcph_Y - go->go_mmTop;
      LONG width = go->go_mmRight - go->go_mmLeft,height = go->go_mmBottom - go->go_mmTop;
      LONG border = BORDER_SIZE + 500;  // for safety

      if (f->f_Flags & FF_DOUBLED)
        border *= 2;

      return((ULONG)(x <= border || y <= border || x >= width-border || y >= height-border));
    }
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
	interface[2].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_RECESSED_LABEL, "Eingedrückt:");
	interface[3].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_DOUBLE_FRAME_LABEL, "Doppelter Rahmen:");

	return TRUE;
}
