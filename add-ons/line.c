/* gClass for ignition
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#include <math.h>

#include "gclass.h"
#include "gclass_protos.h"

#if defined(__SASC)
#	include "gclass_pragmas.h"
#endif


const char *version = "$VER: line.gc 0.12 (2.3.2003)";

#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct gLine
{
  ULONG gl_Pen,gl_OutlinePen;
  ULONG gl_Weight;
  BYTE  gl_HasOutline;
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

#define GLA_BeginLine GOA_TagUser + 1
#define GLA_EndLine   GOA_TagUser + 2

struct gInterface interface[] =
{
  {GOA_Pen,        NULL,GIT_PEN,NULL,NULL},
  {GOA_OutlinePen, NULL,GIT_PEN,NULL,NULL},
  {GOA_HasOutline, NULL,GIT_CHECKBOX,NULL,NULL},
  {GOA_Weight,     NULL,GIT_WEIGHT,NULL,NULL},
  {0,           NULL,0,NULL,NULL}
};
										
const STRPTR superClass = "root";
ULONG instanceSize = sizeof(struct gLine);


void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *grp), REG(a1, struct gClass *gc), REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{
  struct gLine *gl = GINST_DATA(gc,go);
  struct point2d *p = go->go_Knobs;
  long   i,xoff,yoff;

  SetHighColor(grp,gl->gl_Pen);
  grp->Flags &= ~AREAOUTLINE;

  if (gl->gl_HasOutline)   /* MERKER: so geht das nicht, SetOutlinePen() muß im gc_functable dabei sein */
    SetOutlinePen(grp,0);

  xoff = GetOffset(page,TRUE);  yoff = GetOffset(page,FALSE);

  if (!gl->gl_Weight)  // Haarlinie
  {
    Move(grp,pixel(page,p[0].x,TRUE)+xoff,pixel(page,p[0].y,FALSE)+yoff);
    for(i = 1;i < go->go_NumKnobs;i++)
      Draw(grp,pixel(page,p[i].x,TRUE)+xoff,pixel(page,p[i].y,FALSE)+yoff);
  }
  else
  {
    for(i = 1;i < go->go_NumKnobs;i++)
      DrawLine(grp,GetDPI(page),pixel(page,p[i-1].x,TRUE)+xoff,pixel(page,p[i-1].y,FALSE)+yoff,
                                pixel(page,p[i].x,TRUE)+xoff,pixel(page,p[i].y,FALSE)+yoff,gl->gl_Weight,0);
  }
  grp->Flags &= ~AREAOUTLINE;
}


ULONG set(struct gLine *gl,struct TagItem *tstate)
{
  struct TagItem *ti;
  ULONG  rc = GCPR_NONE;

  if (tstate)
  {
    while ((ti = NextTagItem(&tstate)) != 0)
    {
      switch(ti->ti_Tag)
      {
        case GOA_Pen:
          if (gl->gl_Pen != ti->ti_Data)
          {
            gl->gl_Pen = ti->ti_Data;
            rc |= GCPR_REDRAW;
          }
          break;
        case GOA_OutlinePen:
          if (gl->gl_OutlinePen != ti->ti_Data)
          {
            gl->gl_OutlinePen = ti->ti_Data;
            rc |= GCPR_REDRAW;
          }
          break;
        case GOA_HasOutline:
          if ((gl->gl_HasOutline ? TRUE : FALSE) != (ti->ti_Data ? TRUE : FALSE))
          {
            gl->gl_HasOutline = ti->ti_Data ? TRUE : FALSE;
            rc |= GCPR_REDRAW;
          }
          break;
        case GOA_Weight:
          if (gl->gl_Weight != ti->ti_Data)
          {
            gl->gl_Weight = ti->ti_Data;
            rc |= GCPR_UPDATESIZE;
          }
          break;
      }
    }
  }
  return(rc);
}


ULONG PUBLIC dispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
  struct gLine *gl = GINST_DATA(gc,go);
  ULONG  rc = 0L;

  switch(msg->MethodID)
  {
    case GCM_NEW:
      if ((rc = gDoSuperMethodA(gc,go,msg)) != 0)
      {
        go = (struct gObject *)rc;  gl = GINST_DATA(gc,go);

        gl->gl_Pen = FindColorPen(0,0,0);
        gl->gl_OutlinePen = FindColorPen(0,0,0);
        gl->gl_Weight = 0;

        set(gl,((struct gcpSet *)msg)->gcps_AttrList);
      }
      break;
    case GCM_SET:
      rc = gDoSuperMethodA(gc,go,msg) | set(gl,((struct gcpSet *)msg)->gcps_AttrList);
      break;
    case GCM_GET:
      rc = TRUE;

      switch(((struct gcpGet *)msg)->gcpg_Tag)
      {
        case GOA_Pen:
          *((struct gcpGet *)msg)->gcpg_Storage = gl->gl_Pen;
          break;
        case GOA_OutlinePen:
          *((struct gcpGet *)msg)->gcpg_Storage = gl->gl_OutlinePen;
          break;
        case GOA_HasOutline:
          *((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gl->gl_HasOutline;
          break;
        case GOA_Weight:
          *((struct gcpGet *)msg)->gcpg_Storage = gl->gl_Weight;
          break;
        default:
          rc = gDoSuperMethodA(gc,go,msg);
      }
      break;
    case GCM_HITTEST:
    {
      struct gcpHitTest *gcph = (APTR)msg;
      double x = gcph->gcph_X/1024.0,y = gcph->gcph_Y/1024.0;
      double px,py,qx,qy,vx,vy,wx,wy,length,a,b,border;
      struct point2d *p = go->go_Knobs;
      int    i;

      if (!(border = gl->gl_Weight/65536.0))
        border = 1.0;
      border = 25.4*border/144.0;  // halbe Stärke

      for(i = 1;i < go->go_NumKnobs;i++)
      {
        px = p[i-1].x/1024.0;  py = p[i-1].y/1024.0;
        qx = px-x;  qy = py-y;

        vx = p[i].x/1024.0 - px;  vy = p[i].y/1024.0 - py;
        wx = vy;  wy = -vx;

        a = (wx*qy - wy*qx)/(vx*wy - vy*wx);
        b = (qx + a*vx) / wx;

        /* Länge der Linie berechnen */
#ifdef __amigaos4__
        length = gcalcllength(vx, vy);
#else
	   length = sqrt(vx*vx + vy*vy);
#endif

        if (a >= 0 && a <= 1)
        {
          b = b*length;
          //bug("b = %ld, border = %ld\n",(long)(b*1000),(long)(border*1000));
          if (b < 0)
            b = -b;
          if (b <= border)
            return(TRUE);
        }
      }
      rc = FALSE;
      break;
    }
    case GCM_BOX:
    {
      LONG   x,y;

      rc = gDoSuperMethodA(gc,go,msg);

      x = (long)(25.4*gl->gl_Weight/(72.0*128.0)+0.5)+1024;  // halbe Stärke + 1mm
      y = (long)(25.4*gl->gl_Weight/(72.0*128.0)+0.5)+1024;

      go->go_mmLeft -= x;  go->go_mmTop -= y;
      go->go_mmRight += x;  go->go_mmBottom += y;
      break;
    }
    case GCM_REALPOINTS:
      *((struct gcpRealPoints *)msg)->gcpr_StoragePoints = go->go_Knobs;
      *((struct gcpRealPoints *)msg)->gcpr_StorageNumPoints = go->go_NumKnobs;
      rc = TRUE;
      break;
    case GCM_BEGINPOINTS:
      if (((struct gcpBeginPoints *)msg)->gcpb_Mode == GCPBM_MORE)
        return(~0L);

      return(2);
      break;
    case GCM_ENDPOINTS:
      *((struct gcpEndPoints *)msg)->gcpe_StoragePoints = ((struct gcpEndPoints *)msg)->gcpe_Points;
      *((struct gcpEndPoints *)msg)->gcpe_StorageNumPoints = ((struct gcpEndPoints *)msg)->gcpe_NumPoints;
      break;
    case GCM_UPDATEPOINT:
    {
      struct point2d *p = go->go_Knobs;
      LONG   num = ((struct gcpUpdatePoint *)msg)->gcpu_NumPoint;

      p[num].x = ((struct gcpUpdatePoint *)msg)->gcpu_Point.x;
      p[num].y = ((struct gcpUpdatePoint *)msg)->gcpu_Point.y;

      break;
    }
    case GCM_ADDPOINT:
      if (((struct gcpAddPoint *)msg)->gcpa_NumPoints > 1)
      {
        struct coord *p = ((struct gcpAddPoint *)msg)->gcpa_Points;
        WORD   ox = ((struct gcpAddPoint *)msg)->gcpa_Offset.x,oy = ((struct gcpAddPoint *)msg)->gcpa_Offset.y;
        WORD   i = ((struct gcpAddPoint *)msg)->gcpa_NumPoints-1;

        if (((struct gcpAddPoint *)msg)->gcpa_Mode != GCPAM_REDRAW)  /* GCPAM_UPDATE/ONEMORE */
        {

          Move(((struct gcpAddPoint *)msg)->gcpa_RastPort,p[i-1].x+ox,p[i-1].y+oy);
          Draw(((struct gcpAddPoint *)msg)->gcpa_RastPort,p[i].x+ox,p[i].y+oy);
        }
        else  /* refresh the whole drawing */
        {
          for(;i > 0;i--)
          {
            Move(((struct gcpAddPoint *)msg)->gcpa_RastPort,p[i-1].x+ox,p[i-1].y+oy);
            Draw(((struct gcpAddPoint *)msg)->gcpa_RastPort,p[i].x+ox,p[i].y+oy);
          }
        }
      }
      break;
    case GCM_CHANGEPOINT:
    {
      struct RastPort *rp = ((struct gcpAddPoint *)msg)->gcpa_RastPort;
      struct coord *p = ((struct gcpAddPoint *)msg)->gcpa_Points;
      WORD   ox = ((struct gcpAddPoint *)msg)->gcpa_Offset.x,oy = ((struct gcpAddPoint *)msg)->gcpa_Offset.y;
      WORD   i = ((struct gcpAddPoint *)msg)->gcpa_Point;

      if (i > 0)
      {
        Move(rp,p[i-1].x+ox,p[i-1].y+oy);
        Draw(rp,p[i].x+ox,p[i].y+oy);
      }
      else
        Move(rp,p[i].x+ox,p[i].y+oy);

      if (i < ((struct gcpAddPoint *)msg)->gcpa_NumPoints-1)
        Draw(rp,p[i+1].x+ox,p[i+1].y+oy);
      break;
    }
    default:
      return(gDoSuperMethodA(gc,go,msg));
  }
  return(rc);
}


ULONG PUBLIC freeClass(REG(a0, struct gClass *gc))
{
  return(TRUE);
}


ULONG PUBLIC initClass(REG(a0, struct gClass *gc))
{
  return(TRUE);
}
