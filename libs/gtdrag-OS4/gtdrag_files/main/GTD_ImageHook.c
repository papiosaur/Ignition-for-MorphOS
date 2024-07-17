/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Image Render-Hook.

#include <libraries/gadtools.h>
#include <clib/macros.h>
#include <proto/graphics.h>
#include <graphics/gfxmacros.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <string.h>

#include "gtdrag_loc.h"

extern struct GraphicsIFace *IGfx;
extern struct IntuitionIFace *IIntui;
extern struct UtilityIFace *IUtil;

ULONG  ghostPtrn = 0x44441111;

void GhostRect(struct RastPort *rp,UWORD pen,UWORD x0,UWORD y0,UWORD x1,UWORD y1)
{
  	IGfx->SetABPenDrMd(rp,pen,0,JAM1);
  	SetAfPt(rp, (UWORD *)&ghostPtrn,1);
  	IGfx->RectFill(rp,x0,y0,x1,y1);
  	SetAfPt(rp,NULL,0);
}

void FillOldExtent(struct RastPort *rp,struct Rectangle *oldExtent,struct Rectangle *newExtent)
{
  	if (oldExtent->MinX < newExtent->MinX)
    	IGfx->RectFill(rp,oldExtent->MinX,oldExtent->MinY,newExtent->MinX-1,oldExtent->MaxY);
  	if (oldExtent->MaxX > newExtent->MaxX)
    	IGfx->RectFill(rp,newExtent->MaxX+1,oldExtent->MinY,oldExtent->MaxX,oldExtent->MaxY);
  	if (oldExtent->MaxY > newExtent->MaxY)
    	IGfx->RectFill(rp,oldExtent->MinX,newExtent->MaxY+1,oldExtent->MaxX,oldExtent->MaxY);
  	if (oldExtent->MinY < newExtent->MinY)
    	IGfx->RectFill(rp,oldExtent->MinX,oldExtent->MinY,oldExtent->MaxX,newExtent->MinY-1);
}


long WriteHookText(struct RastPort *rp,struct Rectangle *bounds,STRPTR name,ULONG bpen)
{
  	struct TextExtent extent;
  	ULONG  fit;
  	WORD   x,y,slack;

  	fit = IGfx->TextFit(rp,name,name ? IUtil->Strlen(name) : 0,&extent,NULL,1,bounds->MaxX-bounds->MinX-5,bounds->MaxY-bounds->MinY+1);
  	slack = (bounds->MaxY - bounds->MinY) - (extent.te_Extent.MaxY - extent.te_Extent.MinY);

  	x = bounds->MinX - extent.te_Extent.MinX + 4;
  	y = bounds->MinY - extent.te_Extent.MinY + ((slack+1) / 2);
  	extent.te_Extent.MinX += x;  extent.te_Extent.MaxX += x;
  	extent.te_Extent.MinY += y;  extent.te_Extent.MaxY += y;

  	IGfx->Move(rp,x,y);
  	IGfx->Text(rp,name,fit);

  	IGfx->SetAPen(rp,bpen);
  	FillOldExtent(rp,bounds,&extent.te_Extent);
  	
  	return(extent.te_Width);
}


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
      		ow = MIN(ow,(bounds.MaxX-bounds.MinX) >> 1);
      		IGfx->EraseRect(rp,bounds.MinX,bounds.MinY,bounds.MinX+2,bounds.MaxY);
      		IGfx->SetAPen(rp,pens[SHADOWPEN]);
      		IGfx->Move(rp,bounds.MinX+3,bounds.MaxY);
      		IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MaxY);
      		IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MinY);
      		IGfx->SetAPen(rp,pens[SHINEPEN]);
      		IGfx->Draw(rp,bounds.MinX+3,bounds.MinY);
      		IGfx->Draw(rp,bounds.MinX+3,bounds.MaxY-1);
    	}
  	}
  	else if (in->in_Image)
  	{
    	ow = /*min(*/in->in_Image->Width/*,itemwidth)*/;
    	ow = MIN(ow,(bounds.MaxX-bounds.MinX) >> 1);
    	IGfx->SetAPen(rp,bpen);
    	IGfx->RectFill(rp,bounds.MinX,bounds.MinY,bounds.MinX+2,bounds.MaxY);
    	IGfx->SetAPen(rp,pens[SHINEPEN]);
    	IGfx->Move(rp,bounds.MinX+3,bounds.MaxY);
    	IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MaxY);
    	IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MinY);
    	IGfx->SetAPen(rp,pens[SHADOWPEN]);
    	IGfx->Draw(rp,bounds.MinX+3,bounds.MinY);
    	IGfx->Draw(rp,bounds.MinX+3,bounds.MaxY-1);
  	}
  	if (in->in_Image)
  	{
    	if (ow >= in->in_Image->Width)
     		 IIntui->DrawImage(rp,in->in_Image,bounds.MinX+4,bounds.MinY+1);
    	else
      		IGfx->EraseRect(rp,bounds.MinX+4,bounds.MinY+1,bounds.MinX+3+ow,bounds.MinY+in->in_Image->Height);
    	if (bounds.MaxY-bounds.MinY-2 > in->in_Image->Height)
    	{
      		IGfx->SetAPen(rp,pens[BACKGROUNDPEN]);
      		IGfx->RectFill(rp,bounds.MinX+4,bounds.MinY+in->in_Image->Height+1,bounds.MinX+3+ow,bounds.MaxY-1);
    	}
    	bounds.MinX += ow+5;
  	}
  	IGfx->SetABPenDrMd(rp,apen,bpen,JAM2);
 	WriteHookText(rp,&bounds,in->in_Name,bpen);
  	IGfx->SetABPenDrMd(rp,apen,bpen,JAM2);
  	rp->LinePtrn = 0x5555;  rp->linpatcnt = 15;  rp->Flags |= FRST_DOT;
  	IGfx->Move(rp,bounds.MinX,bounds.MaxY);
  	IGfx->Draw(rp,bounds.MaxX,bounds.MaxY);
  	rp->LinePtrn = 0xffff;  rp->linpatcnt = 0;
  	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
    	GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
  	return(LVCB_OK);
}

