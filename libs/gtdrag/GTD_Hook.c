/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	General Hook Functions.


#include "gtdrag_includes.h"


ULONG  ghostPtrn = 0x44441111;


LIB_LH1(struct Hook *, GTD_GetHook, 
  LIB_LHA(ULONG, type, D0),
  struct Library *, library, 20, Gtdrag)
{
  LIBFUNC_INIT

  switch(type)
  {
    case GTDH_IMAGE: return(&renderHook);
    case GTDH_TREE: return(&treeHook);
    case GTDH_IFFSTREAM: return(&iffstreamHook);
  }
  return(NULL);

  LIBFUNC_EXIT
}


void PRIVATE DrawRect(struct RastPort *rp,long x, long y, long w, long h)
{
  Move(rp,x+1,y);
  Draw(rp,x+w,y);
  Draw(rp,x+w,y+h);
  Draw(rp,x,y+h);
  Draw(rp,x,y);
}


void PRIVATE GhostRect(struct RastPort *rp,UWORD pen,UWORD x0,UWORD y0,UWORD x1,UWORD y1)
{
  SetABPenDrMd(rp,pen,0,JAM1);
  SetAfPt(rp,(UWORD *)&ghostPtrn,1);
  RectFill(rp,x0,y0,x1,y1);
  SetAfPt(rp,NULL,0);
}


void PRIVATE FillOldExtent(struct RastPort *rp,struct Rectangle *oldExtent,struct Rectangle *newExtent)
{
  if (oldExtent->MinX < newExtent->MinX)
    RectFill(rp,oldExtent->MinX,oldExtent->MinY,newExtent->MinX-1,oldExtent->MaxY);
  if (oldExtent->MaxX > newExtent->MaxX)
    RectFill(rp,newExtent->MaxX+1,oldExtent->MinY,oldExtent->MaxX,oldExtent->MaxY);
  if (oldExtent->MaxY > newExtent->MaxY)
    RectFill(rp,oldExtent->MinX,newExtent->MaxY+1,oldExtent->MaxX,oldExtent->MaxY);
  if (oldExtent->MinY < newExtent->MinY)
    RectFill(rp,oldExtent->MinX,oldExtent->MinY,oldExtent->MaxX,newExtent->MinY-1);
}


long PRIVATE WriteHookText(struct RastPort *rp,struct Rectangle *bounds,STRPTR name,ULONG bpen)
{
  struct TextExtent extent;
  ULONG  fit;
  WORD   x,y,slack;

  fit = TextFit(rp,name,name ? strlen(name) : 0,&extent,NULL,1,bounds->MaxX-bounds->MinX-5,bounds->MaxY-bounds->MinY+1);
  slack = (bounds->MaxY - bounds->MinY) - (extent.te_Extent.MaxY - extent.te_Extent.MinY);

  x = bounds->MinX - extent.te_Extent.MinX + 4;
  y = bounds->MinY - extent.te_Extent.MinY + ((slack+1) / 2);
  extent.te_Extent.MinX += x;  extent.te_Extent.MaxX += x;
  extent.te_Extent.MinY += y;  extent.te_Extent.MaxY += y;

  Move(rp,x,y);
  Text(rp,name,fit);

  SetAPen(rp,bpen);
  FillOldExtent(rp,bounds,&extent.te_Extent);
  return(extent.te_Width);
}
