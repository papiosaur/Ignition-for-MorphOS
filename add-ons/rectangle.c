/* gClass for ignition
 *
 * Copyright 1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "gclass.h"
#include "gclass_protos.h"

#if defined(__SASC)
#	include "gclass_pragmas.h"
#endif

const char *version = "$VER: rectangle.gc 0.6 (2.3.2003)";

#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct Rect
{
	ULONG r_FillPen, r_OutlinePen, r_Weight;
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

struct gInterface interface[] =
{
  {GOA_FillPen,    NULL,GIT_PEN,NULL,NULL},
  {GOA_OutlinePen, NULL,GIT_PEN,NULL,NULL},
//  {GOA_Weight,     NULL,GIT_WEIGHT,NULL,NULL},
  {0,              NULL,0,NULL,NULL}
};

const STRPTR superClass = "root";
ULONG instanceSize = sizeof(struct Rect);


void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *grp), REG(a1, struct gClass *gc),
	REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{
  struct Rect *r = GINST_DATA(gc, go);
  long x = gb->gb_Left,y = gb->gb_Top;
  long wid = gb->gb_Right-gb->gb_Left;
  long hei = gb->gb_Bottom-gb->gb_Top;
  long bx,by;

  SetHighColor(grp,r->r_FillPen);
  grp->Flags &= ~AREAOUTLINE;

  if (r->r_OutlinePen != r->r_FillPen)
  {
    bx = pixel(page,1536,TRUE);  by = pixel(page,1536,FALSE);
    RectFill(grp,x+bx+1,y+by+1,x+wid-1-bx,y+hei-by-1);

    SetHighColor(grp,r->r_OutlinePen);

    RectFill(grp,x,y,x+wid,y+by);
    RectFill(grp,x,y+by+1,x+bx,y+hei);
    RectFill(grp,x+bx+1,y+hei-by,x+wid,y+hei);
    RectFill(grp,x+wid-bx,y+by+1,x+wid,y+hei-by);
  }
  else
    RectFill(grp,x,y,x+wid,y+hei);
}


ULONG set(struct Rect *r,struct TagItem *tstate)
{
  struct TagItem *ti;
  ULONG  rc = GCPR_NONE;

  if (tstate)
  {
    while ((ti = NextTagItem(&tstate)) != 0)
    {
      switch(ti->ti_Tag)
      {
        case GOA_FillPen:
          if (r->r_FillPen != ti->ti_Data)
          {
            r->r_FillPen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GOA_OutlinePen:
          if (r->r_OutlinePen != ti->ti_Data)
          {
            r->r_OutlinePen = ti->ti_Data;
            rc = GCPR_REDRAW;
          }
          break;
        case GOA_Weight:
          if (r->r_Weight != ti->ti_Data)
          {
            r->r_Weight = ti->ti_Data;
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
  struct Rect *r = GINST_DATA(gc,go);
  ULONG rc = 0L;

  switch(msg->MethodID)
  {
    case GCM_NEW:
      if ((rc = gDoSuperMethodA(gc, go, msg)) != 0)
      {
        go = (struct gObject *)rc;  r = GINST_DATA(gc,go);

        r->r_FillPen = FindColorPen(200,0,0);
        r->r_OutlinePen = FindColorPen(0,0,0);
        r->r_Weight = 1;

        set(r,((struct gcpSet *)msg)->gcps_AttrList);
      }
      break;
    case GCM_SET:
      rc = gDoSuperMethodA(gc,go,msg) | set(r,((struct gcpSet *)msg)->gcps_AttrList);
      break;
    case GCM_GET:
      rc = TRUE;

      switch(((struct gcpGet *)msg)->gcpg_Tag)
      {
        case GOA_OutlinePen:
          *((struct gcpGet *)msg)->gcpg_Storage = r->r_OutlinePen;
          break;
        case GOA_FillPen:
          *((struct gcpGet *)msg)->gcpg_Storage = r->r_FillPen;
          break;
        case GOA_Weight:
          *((struct gcpGet *)msg)->gcpg_Storage = r->r_Weight;
          break;
        default:
          rc = gDoSuperMethodA(gc,go,msg);
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
	return TRUE;
}


ULONG PUBLIC
initClass(REG(a0, struct gClass *gc))
{
	return TRUE;
}
