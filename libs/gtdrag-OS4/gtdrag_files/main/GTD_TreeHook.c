/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Tree Render-Hook.

#include <proto/graphics.h>
#include <proto/intuition.h>
#include <clib/macros.h>
#include <libraries/gadtools.h>

#include "gtdrag_loc.h"
//#include "gtdrag_private.h"

extern struct GraphicsIFace *IGfx;
extern struct IntuitionIFace *IIntui;

extern void GhostRect(struct RastPort *rp,UWORD pen,UWORD x0,UWORD y0,UWORD x1,UWORD y1);
extern long WriteHookText(struct RastPort *rp,struct Rectangle *bounds,STRPTR name,ULONG bpen);

#define TREE_SHOWFIRST     /* show root-trees connected? */

static UBYTE sOpenedBitmap[] = {
	0x00,0x00,		// 0000 0000 0000 0000
	0x00,0x00,		// 0000 0000 0000 0000
	0xff,0x80,		// 1111 1111 1000 0000
	0xff,0x00,		// 0111 1111 0000 0000
	0x3e,0x00,		// 0011 1110 0000 0000
	0x1c,0x00,		// 0001 1100 0000 0000
	0x08,0x00,		// 0000 1000 0000 0000
};

static UBYTE sClosedBitmap[] = {
	0x18,0x00,		// 0001 1000 0000 0000
	0x1e,0x00,		// 0001 1110 0000 0000
	0x1f,0x80,		// 0001 1111 1000 0000
	0xff,0xe0,		// 1111 1111 1110 0000
	0x1f,0x80,		// 0001 1111 1000 0000
	0x1e,0x00,		// 0001 1110 0000 0000
	0x18,0x00,		// 0001 1000 0000 0000
};

void DrawRect(struct RastPort *rp,long x, long y, long w, long h)
{
  	IGfx->Move(rp,x+1,y);
  	IGfx->Draw(rp,x+w,y);
  	IGfx->Draw(rp,x+w,y+h);
  	IGfx->Draw(rp,x,y+h);
  	IGfx->Draw(rp,x,y);
}

ULONG TreeHook(REG(a0, struct Hook *h), REG(a2, struct TreeNode *tn), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct Rectangle bounds;
	UBYTE  state;
	ULONG  apen,bpen,treecolor;
	UWORD  ow,i,cen,mid,wid,depth;
	UWORD  *pens;

	if (msg->lvdm_MethodID != LV_DRAW)
		return LVCB_UNKNOWN;

	rp = msg->lvdm_RastPort;
	state = msg->lvdm_State;
	pens = msg->lvdm_DrawInfo->dri_Pens;
	bounds = msg->lvdm_Bounds;
	tn = TREENODE(tn);
	treecolor = (LONG)h->h_Data;

	apen = pens[FILLTEXTPEN];
	bpen = pens[FILLPEN];
	if ((state == LVR_NORMAL) || (state == LVR_NORMALDISABLED)) {
		apen = pens[TEXTPEN];
		bpen = pens[BACKGROUNDPEN];
	}

	IGfx->SetAPen(rp, bpen);
	wid = rp->Font->tf_YSize+2;  cen = wid >> 1;
	mid = (bounds.MinY+bounds.MaxY) >> 1;

#ifdef TREE_SHOWFIRST
	depth = tn->tn_Depth + 1;
#else
	depth = tn->tn_Depth;
#endif

	for (i = 0; i < depth; i++) {
		if (((1L << i) & tn->tn_DepthLines) || i == depth - 1) {
			IGfx->RectFill(rp, bounds.MinX, bounds.MinY, bounds.MinX + cen - 1, bounds.MaxY);

			if (i == depth - 1) {
				IGfx->RectFill(rp,bounds.MinX+cen+1,bounds.MinY,bounds.MinX+wid-1,mid-1);
				IGfx->RectFill(rp,bounds.MinX+cen+1,mid+1,bounds.MinX+wid-1,bounds.MaxY);

				if (tn->tn_Flags & TNF_LAST)
					IGfx->RectFill(rp,bounds.MinX+cen,mid+1,bounds.MinX+cen,bounds.MaxY);
			} else
				IGfx->RectFill(rp,bounds.MinX+cen+1,bounds.MinY,bounds.MinX+wid-1,bounds.MaxY);

			IGfx->SetAPen(rp, treecolor);
			if (i == depth - 1) {
				IGfx->Move(rp, bounds.MinX + cen, mid);
				IGfx->Draw(rp, bounds.MinX + wid - 1, mid);
			}
			IGfx->Move(rp, bounds.MinX + cen, bounds.MinY);

			if (i == depth-1 && tn->tn_Flags & TNF_LAST)
				IGfx->Draw(rp, bounds.MinX + cen, mid);
			else
				IGfx->Draw(rp, bounds.MinX + cen, bounds.MaxY);

			IGfx->SetAPen(rp, bpen);
		} else
			IGfx->RectFill(rp, bounds.MinX, bounds.MinY, bounds.MinX + wid - 1, bounds.MaxY);

		bounds.MinX += wid;
	}

	if (tn->tn_Flags & TNF_CONTAINER) {
		IGfx->RectFill(rp, bounds.MinX, bounds.MinY, bounds.MinX + wid - 1, bounds.MaxY);

		IGfx->SetAPen(rp, treecolor);
#ifndef TREE_SHOWFIRST
		if (tn->tn_Depth)
#endif
		{
			IGfx->Move(rp,bounds.MinX, mid);
			IGfx->Draw(rp,bounds.MinX + cen - 5, mid);
		}

		//Move(rp,bounds.MinX + cen + 4, mid);
		//Draw(rp,bounds.MinX + wid - 1, mid);

		if (tn->tn_Flags & TNF_OPEN && !IsListEmpty(((struct List *)&(tn->tn_Nodes)))) {
			IGfx->Move(rp, bounds.MinX + cen, mid + 4);
			IGfx->Draw(rp, bounds.MinX + cen, bounds.MaxY);
		}

		//SetAPen(rp,pens[SHINEPEN]);
		//RectFill(rp,bounds.MinX+cen-2,mid-2,bounds.MinX+cen+2,mid+2);
		IGfx->SetABPenDrMd(rp, treecolor, bpen, JAM2);

		// draw opened or closed rectangle

		//DrawRect(rp,tn->tn_X = bounds.MinX+cen-3,tn->tn_Y = mid-3,TREEKNOBSIZE,TREEKNOBSIZE);
		tn->tn_X = bounds.MinX + cen - 4;
		tn->tn_Y = mid - 3;

		if (!IsListEmpty((struct List *)&tn->tn_Nodes)) {
			IGfx->BltTemplate((PLANEPTR)(tn->tn_Flags & TNF_OPEN ? sOpenedBitmap : sClosedBitmap), 0, 2, rp, tn->tn_X, tn->tn_Y, 11, 7);
/*		{
        Move(rp,bounds.MinX+cen,mid-1);
        Draw(rp,bounds.MinX+cen,mid+1);
      }
      Move(rp,bounds.MinX+cen-1,mid);
	  Draw(rp,bounds.MinX+cen+1,mid);*/
		} else {
			//RectFill(rp, tn->tn_X, mid - 2, bounds.MinX + cen + 2, mid + 2);
			DrawRect(rp, tn->tn_X, mid - 2, 5, 5);
			DrawRect(rp, tn->tn_X + 1, mid - 1, 3, 3);
		}
						
		tn->tn_Y -= bounds.MinY;
		bounds.MinX += wid;
	}

	if (tn->tn_Node.in_Image) {
		if ((state == LVR_NORMAL) || (state == LVR_NORMALDISABLED)) {
			/*ow = min(tn->tn_Image->Width,itemwidth);*/
			ow = MIN(tn->tn_Node.in_Image->Width,(bounds.MaxX-bounds.MinX) >> 1);
			IGfx->EraseRect(rp,bounds.MinX,bounds.MinY,bounds.MinX+2,bounds.MaxY);

			IGfx->SetAPen(rp,pens[SHADOWPEN]);
			IGfx->Move(rp,bounds.MinX+3,bounds.MaxY);
			IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MaxY);
			IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MinY);

			IGfx->SetAPen(rp,pens[SHINEPEN]);
			IGfx->Draw(rp,bounds.MinX+3,bounds.MinY);
			IGfx->Draw(rp,bounds.MinX+3,bounds.MaxY-1);
		} else {
			/*ow = min(tn->tn_Image->Width,itemwidth);*/
			ow = MIN(tn->tn_Node.in_Image->Width,(bounds.MaxX-bounds.MinX) >> 1);
			IGfx->SetAPen(rp, bpen);
			IGfx->RectFill(rp,bounds.MinX,bounds.MinY,bounds.MinX+2,bounds.MaxY);

			IGfx->SetAPen(rp,pens[SHINEPEN]);
			IGfx->Move(rp,bounds.MinX+3,bounds.MaxY);
			IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MaxY);
			IGfx->Draw(rp,bounds.MinX+ow+4,bounds.MinY);
	  
			IGfx->SetAPen(rp,pens[SHADOWPEN]);
			IGfx->Draw(rp,bounds.MinX+3,bounds.MinY);
			IGfx->Draw(rp,bounds.MinX+3,bounds.MaxY-1);
		}

		if (ow >= tn->tn_Node.in_Image->Width)
			IIntui->DrawImage(rp,tn->tn_Node.in_Image,bounds.MinX+4,bounds.MinY+1);
		else
			IGfx->EraseRect(rp,bounds.MinX+4,bounds.MinY+1,bounds.MinX+3+ow,bounds.MinY+tn->tn_Node.in_Image->Height);

		if (bounds.MaxY-bounds.MinY-2 > tn->tn_Node.in_Image->Height) {
			IGfx->SetAPen(rp,pens[BACKGROUNDPEN]);
			IGfx->RectFill(rp,bounds.MinX+4,bounds.MinY+tn->tn_Node.in_Image->Height+1,bounds.MinX+3+ow,bounds.MaxY-1);
		}
		bounds.MinX += ow+5;
	}

	IGfx->SetABPenDrMd(rp, tn->tn_Flags & TNF_HIGHLIGHTED ? pens[SHINEPEN] : apen, bpen, JAM2);
	bounds.MinX += WriteHookText(rp, &bounds, tn->tn_Node.in_Name, bpen) + 4;

	if (tn->tn_Flags & (TNF_ADD | TNF_REPLACE)) {
		IGfx->SetAPen(rp, treecolor);
		IGfx->RectFill(rp, tn->tn_X = bounds.MinX, tn->tn_Y = mid + 1, bounds.MinX + TREEKNOBSIZE, mid + TREEKNOBSIZE + 1);
		tn->tn_Y -= bounds.MinY;
	
		IGfx->SetAPen(rp, bpen);
		if (tn->tn_Flags & TNF_ADD) {
			IGfx->Move(rp, bounds.MinX + 3, mid + 2);
			IGfx->Draw(rp, bounds.MinX + 3, mid + 6);
		}

		IGfx->Move(rp, bounds.MinX + 1, mid + 4);
		IGfx->Draw(rp, bounds.MinX + 5, mid + 4);
	}

	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
		GhostRect(rp, pens[BLOCKPEN], msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY, msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);

	return LVCB_OK;
}
