/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Image Render-Hook.


#include "gtdrag_includes.h"


ULONG RenderHook(REG(a0, struct Hook * h), REG(a2, struct ImageNode *in), REG(a1, struct LVDrawMsg *msg))
{
  struct RastPort *rp;
  struct Rectangle bounds;
  UBYTE  state;
  ULONG  apen,bpen;
  UWORD  ow = 0;
  UWORD  *pens;

  if (msg->lvdm_MethodID != LV_DRAW)
    return(LVCB_UNKNOWN);

  rp = msg->lvdm_RastPort;
  state = msg->lvdm_State;
  pens = msg->lvdm_DrawInfo->dri_Pens;
  bounds = msg->lvdm_Bounds;

  apen = pens[FILLTEXTPEN];
  bpen = pens[FILLPEN];

  if ((state == LVR_NORMAL) || (state == LVR_NORMALDISABLED))
  {
    apen = pens[TEXTPEN];
    bpen = pens[BACKGROUNDPEN];
    if (in->in_Image)
    {
      ow = /*min(*/in->in_Image->Width/*,itemwidth)*/;
      ow = min(ow,(bounds.MaxX-bounds.MinX) >> 1);
      EraseRect(rp,bounds.MinX,bounds.MinY,bounds.MinX+2,bounds.MaxY);
      SetAPen(rp,pens[SHADOWPEN]);
      Move(rp,bounds.MinX+3,bounds.MaxY);
      Draw(rp,bounds.MinX+ow+4,bounds.MaxY);
      Draw(rp,bounds.MinX+ow+4,bounds.MinY);
      SetAPen(rp,pens[SHINEPEN]);
      Draw(rp,bounds.MinX+3,bounds.MinY);
      Draw(rp,bounds.MinX+3,bounds.MaxY-1);
    }
  }
  else if (in->in_Image)
  {
    ow = /*min(*/in->in_Image->Width/*,itemwidth)*/;
    ow = min(ow,(bounds.MaxX-bounds.MinX) >> 1);
    SetAPen(rp,bpen);
    RectFill(rp,bounds.MinX,bounds.MinY,bounds.MinX+2,bounds.MaxY);
    SetAPen(rp,pens[SHINEPEN]);
    Move(rp,bounds.MinX+3,bounds.MaxY);
    Draw(rp,bounds.MinX+ow+4,bounds.MaxY);
    Draw(rp,bounds.MinX+ow+4,bounds.MinY);
    SetAPen(rp,pens[SHADOWPEN]);
    Draw(rp,bounds.MinX+3,bounds.MinY);
    Draw(rp,bounds.MinX+3,bounds.MaxY-1);
  }

  if (in->in_Image)
  {
    if (ow >= in->in_Image->Width)
      DrawImage(rp,in->in_Image,bounds.MinX+4,bounds.MinY+1);
    else
      EraseRect(rp,bounds.MinX+4,bounds.MinY+1,bounds.MinX+3+ow,bounds.MinY+in->in_Image->Height);
    if (bounds.MaxY-bounds.MinY-2 > in->in_Image->Height)
    {
      SetAPen(rp,pens[BACKGROUNDPEN]);
      RectFill(rp,bounds.MinX+4,bounds.MinY+in->in_Image->Height+1,bounds.MinX+3+ow,bounds.MaxY-1);
    }
    bounds.MinX += ow+5;
  }
  SetABPenDrMd(rp,apen,bpen,JAM2);
  WriteHookText(rp,&bounds,in->in_Name,bpen);

  SetABPenDrMd(rp,apen,bpen,JAM2);
  rp->LinePtrn = 0x5555;  rp->linpatcnt = 15;  rp->Flags |= FRST_DOT;
  Move(rp,bounds.MinX,bounds.MaxY);
  Draw(rp,bounds.MaxX,bounds.MaxY);
  rp->LinePtrn = 0xffff;  rp->linpatcnt = 0;

  if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
    GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
  return(LVCB_OK);
}
