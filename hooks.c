/* Miscellaneous. and ListView-hooks
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"


struct Hook fillHook,renderHook,popUpHook,formelHook,formatHook;
struct Hook treeHook,colorHook,selectHook,fileHook,glinkHook,linkHook;


/****************************************\
* The hooks are initialized in boopsi.c *
\****************************************/


ULONG  ghostPtrn = 0x44441111;


void PUBLIC
fillHookFunc(REG(a0, struct Hook *h), REG(a2, struct RastPort *rastp), REG(a1, struct Rectangle *rect))
{
	if (prefs.pr_Screen->ps_BackFill) {
		rect = (struct Rectangle *)((ULONG *)rect + 1);
#ifndef __amigaos4__
		RectFill(&scrRp,rect->MinX,rect->MinY,rect->MaxX,rect->MaxY); //GURU-MELDUNG under OS4
#endif
	}
}


/********************************** ListView-Hooks **********************************/

void
GhostRect(struct RastPort *rp,UWORD pen,UWORD x0,UWORD y0,UWORD x1,UWORD y1)
{
	SetABPenDrMd(rp,pen,0,JAM1);
	SetAfPt(rp,(UWORD *)&ghostPtrn,1);
	RectFill(rp,x0,y0,x1,y1);
	SetAfPt(rp,NULL,0);
}


void
FillOldExtent(struct RastPort *rp,struct Rectangle *oldExtent,struct Rectangle *newExtent)
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


long
WriteHookText(struct RastPort *rp, struct Rectangle *bounds, STRPTR name, ULONG bpen)
{
	struct TextExtent extent;
	ULONG  fit;
	WORD   x,y,slack;

	fit = TextFit(rp, name, (name ? strlen(name) : 0), &extent, NULL, 1, bounds->MaxX-bounds->MinX-5, bounds->MaxY-bounds->MinY+1);
	slack = (bounds->MaxY - bounds->MinY) - (extent.te_Extent.MaxY - extent.te_Extent.MinY);

	x = bounds->MinX - extent.te_Extent.MinX + 4;
	y = bounds->MinY - extent.te_Extent.MinY + ((slack+1) / 2);
	extent.te_Extent.MinX += x;  extent.te_Extent.MaxX += x;
	extent.te_Extent.MinY += y;  extent.te_Extent.MaxY += y;

	Move(rp, x, y);
	Text(rp, name, fit);

	SetAPen(rp, bpen);
	FillOldExtent(rp, bounds, &extent.te_Extent);

	return extent.te_Width;
}


ULONG
PUBLIC RenderHook(REG(a0, struct Hook *h), REG(a2, struct ImageNode *in), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct Rectangle bounds;
	struct Image *im = in->in_Image;
	UBYTE  state;
	ULONG  apen,bpen;
	UWORD  ow = itemwidth;
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
		if (im)
		{
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
	else if (im)
	{
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

	if (im)
	{
		if (ow >= im->Width)
		{
			long x = bounds.MinX+4+((ow - im->Width) >> 1);
			long y = bounds.MinY+1+((bounds.MaxY-bounds.MinY - im->Height) >> 1);

			EraseRect(rp,bounds.MinX+4,bounds.MinY+1,x-1,bounds.MaxY-1);
			EraseRect(rp,x+im->Width,bounds.MinY+1,bounds.MinX+ow+3,bounds.MaxY-1);
			EraseRect(rp,x,bounds.MinY+1,x+im->Width-1,y-1);
			EraseRect(rp,x,y+im->Height,x+im->Width-1,bounds.MaxY-1);

			DrawImage(rp,im,x,y);
		}
		else
			EraseRect(rp,bounds.MinX+4,bounds.MinY+1,bounds.MinX+3+ow,bounds.MinY+im->Height);

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


void DrawColoredBox(struct RastPort *rp,struct Rectangle bounds,UWORD hpen,UWORD dpen,UWORD bpen)
{
	RectFill(rp,bounds.MinX+5,bounds.MinY+2,bounds.MinX+boxwidth+2,bounds.MaxY-2);

	SetAPen(rp,hpen);
	Move(rp,bounds.MinX+4,bounds.MaxY-1);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MaxY-1);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MinY+1);
	SetAPen(rp,dpen);
	Draw(rp,bounds.MinX+4,bounds.MinY+1);
	Draw(rp,bounds.MinX+4,bounds.MaxY-2);
	SetAPen(rp,bpen);
	Move(rp,bounds.MinX+4,bounds.MaxY);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MaxY);
	Move(rp,bounds.MinX+4,bounds.MinY);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MinY);
//  SetAPen(rp,bpen);
	RectFill(rp,bounds.MinX,bounds.MinY,bounds.MinX+3,bounds.MaxY);
}


ULONG PUBLIC
ColorHook(REG(a0, struct Hook *h), REG(a2, struct colorPen *cp), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct Rectangle bounds;
	UBYTE  state;
	ULONG  apen,bpen;
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
	}
	/*SetAPen(rp,pens[SHINEPEN]);
	Move(rp,bounds.MinX+4,bounds.MaxY-1);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MaxY-1);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MinY+1);
	SetAPen(rp,pens[SHADOWPEN]);
	Draw(rp,bounds.MinX+4,bounds.MinY+1);
	Draw(rp,bounds.MinX+4,bounds.MaxY-2);
	SetAPen(rp,bpen);
	Move(rp,bounds.MinX+4,bounds.MaxY);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MaxY);
	Move(rp,bounds.MinX+4,bounds.MinY);
	Draw(rp,bounds.MinX+boxwidth+3,bounds.MinY);
	SetAPen(rp,bpen);
	RectFill(rp,bounds.MinX,bounds.MinY,bounds.MinX+3,bounds.MaxY);*/

	if (cp->cp_Pen != -1)
		SetAPen(rp,cp->cp_Pen);
	else
		SetAPen(rp,FindColor(scr->ViewPort.ColorMap,RGB32(cp->cp_Red),RGB32(cp->cp_Green),RGB32(cp->cp_Blue),-1));
	/*RectFill(rp,bounds.MinX+5,bounds.MinY+2,bounds.MinX+boxwidth+2,bounds.MaxY-2);*/

	DrawColoredBox(rp,bounds,pens[SHINEPEN],pens[SHADOWPEN],bpen);

	bounds.MinX += 4+boxwidth;
	SetABPenDrMd(rp,apen,bpen,JAM2);
	WriteHookText(rp,&bounds,cp->cp_Node.ln_Name,bpen);

	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
		GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);

	return LVCB_OK;
}


ULONG PUBLIC
PopUpHook(REG(a0, struct Hook *h), REG(a2, struct Node *n), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct Rectangle bounds;
	UBYTE  state,separator;
	ULONG  apen,bpen;
	UWORD  *pens,y;

	if (msg->lvdm_MethodID != LV_DRAW)
		return(LVCB_UNKNOWN);

	rp = msg->lvdm_RastPort;
	state = msg->lvdm_State;
	pens = msg->lvdm_DrawInfo->dri_Pens;
	bounds = msg->lvdm_Bounds;

	apen = pens[TEXTPEN];
	bpen = pens[BACKGROUNDPEN];

	separator = n->ln_Name && !strcmp(n->ln_Name, "-");

	if (((state == LVR_SELECTED) || (state == LVR_SELECTEDDISABLED)) && !(separator && n->ln_Type == POPUP_NO_SELECT_BARLABEL))
	{
		apen = pens[FILLTEXTPEN];
		bpen = pens[FILLPEN];
	}

	if (separator)
	{
		SetAPen(rp, bpen);
		RectFill(rp, bounds.MinX, bounds.MinY, bounds.MaxX, (y = ((bounds.MaxY + bounds.MinY) >> 1) - 1) - 1);
		RectFill(rp, bounds.MinX, y + 2, bounds.MaxX, bounds.MaxY);
		RectFill(rp, bounds.MinX, y, bounds.MinX + 1, y + 1);
		RectFill(rp, bounds.MaxX - 1, y, bounds.MaxX, y + 1);
		SetAPen(rp, pens[SHADOWPEN]);
		Move(rp, bounds.MinX + 2, y);
		Draw(rp, bounds.MaxX - 2, y);
		SetAPen(rp, pens[SHINEPEN]);
		Move(rp, bounds.MinX + 2, ++y);
		Draw(rp, bounds.MaxX - 2, y);
	}
	else
	{
		SetABPenDrMd(rp, apen, bpen, JAM2);
		WriteHookText(rp, &bounds, n->ln_Name, bpen);
	}
	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
		GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);

	return LVCB_OK;
}


void
DrawSelectHook(struct RastPort *rp,long x,long miny,long maxy)
{
	long i,y;

	/*  RectFill(rp,x = bounds.MinX+(boxwidth >> 1)-2,y = bounds.MinY+(fontheight >> 1)-2,x+3,y+3); */
	x += (boxwidth >> 1)-1;
	y = miny-3+((maxy-miny+fontheight) >> 1);
	for(i = 0;(y-i*2-2) > miny;i++)
	{
		RectFill(rp,x-i,y-i,x-i+1,y-i+1);         // left
		RectFill(rp,x+i+1,y-2*i-2,x+i+2,y-2*i-1); // right
	}
}


ULONG PUBLIC
SelectHook(REG(a0, struct Hook *h), REG(a2, struct Node *n), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct Rectangle bounds;
	UBYTE  state;
	ULONG  apen,bpen;
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
	}
	SetAPen(rp,bpen);
	RectFill(rp,bounds.MinX,bounds.MinY,bounds.MinX+boxwidth-5,bounds.MaxY);
	SetABPenDrMd(rp,apen,bpen,JAM2);
	if (n->ln_Type)
		DrawSelectHook(rp,bounds.MinX,bounds.MinY,bounds.MaxY);

	bounds.MinX += boxwidth-4;
	WriteHookText(rp,&bounds,n->ln_Name,bpen);

	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
		GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
	return(LVCB_OK);
}


ULONG PUBLIC FormelHook(REG(a0, struct Hook *h), REG(a2, struct Node *n), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct TextExtent extent;
	ULONG  fit;
	struct Rectangle bounds;
	UBYTE  state;
	WORD   x,y,i,next;
	ULONG  apen,bpen;
	UWORD  *pens;
	STRPTR name;

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
	}
	SetABPenDrMd(rp,apen,bpen,JAM2);
	name = n->ln_Name;
	fit = TextFit(rp,name,name ? strlen(name) : 0,&extent,NULL,1,bounds.MaxX-bounds.MinX-5,bounds.MaxY-bounds.MinY+1);
	i = (bounds.MaxY - bounds.MinY) - (extent.te_Extent.MaxY - extent.te_Extent.MinY);

	x = bounds.MinX-extent.te_Extent.MinX+2+n->ln_Pri*fontheight;
	y = bounds.MinY-extent.te_Extent.MinY + ((i+1) / 2);

	extent.te_Extent.MinX += x;
	extent.te_Extent.MaxX += x;
	extent.te_Extent.MinY += y;
	extent.te_Extent.MaxY += y;

	Move(rp,x,y);
	Text(rp,name,fit);

	SetAPen(rp,bpen);
	FillOldExtent(rp,&bounds,&extent.te_Extent);

	if (!n->ln_Succ->ln_Succ)
		next = 0;
	else
		next = n->ln_Succ->ln_Pri;
	y = (bounds.MaxY+bounds.MinY) >> 1;
	SetAPen(rp,(ULONG)treeHook.h_Data);
	for(x = bounds.MinX+2+(fontheight >> 1),i = 0;n->ln_Pri > i;i++,x += fontheight)
	{
		Move(rp,x,bounds.MinY);
		if (next <= i)
			Draw(rp,x,y);
		else
			Draw(rp,x,bounds.MaxY);
	}
	if (n->ln_Pri)
	{
		x -= fontheight;
		Move(rp,x,y);
		Draw(rp,x+(fontheight >> 1),y);
	}

	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
		GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
	return(LVCB_OK);
}


ULONG PUBLIC FormatHook(REG(a0, struct Hook *h), REG(a2, struct FormatVorlage *fv), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct Rectangle bounds;
	UBYTE  state;
	ULONG  apen,bpen;
	UWORD  *pens,wid;
	char   t[8];

	if (msg->lvdm_MethodID != LV_DRAW)
		return(LVCB_UNKNOWN);

	rp = msg->lvdm_RastPort;
	state = msg->lvdm_State;
	pens = msg->lvdm_DrawInfo->dri_Pens;
	bounds = msg->lvdm_Bounds;

	apen = pens[HIGHLIGHTTEXTPEN];
	bpen = pens[FILLPEN];

	if ((state == LVR_NORMAL) || (state == LVR_NORMALDISABLED))
	{
		apen = pens[HIGHLIGHTTEXTPEN];
		bpen = pens[BACKGROUNDPEN];
	}
	SetABPenDrMd(rp,apen,bpen,JAM2);
	bounds.MaxX = bounds.MinX-1 + ((bounds.MaxX-bounds.MinX) >> 1);
	if (fv->fv_Node.ln_Pri)
	{
		sprintf(t,"(%d)",fv->fv_Node.ln_Pri);
		bounds.MaxX -= (wid = TLn(t)+6);
	}
	WriteHookText(rp,&bounds,fv->fv_Node.ln_Name,bpen);

	if (fv->fv_Node.ln_Pri)
	{
		SetAPen(rp,pens[TEXTPEN]);
		bounds.MinX = bounds.MaxX;  bounds.MaxX += wid;
		WriteHookText(rp,&bounds,t,bpen);
	}

	SetAPen(rp,pens[SHADOWPEN]);
	Move(rp,bounds.MaxX+1,bounds.MinY);  Draw(rp,bounds.MaxX+1,bounds.MaxY);
	SetAPen(rp,pens[SHINEPEN]);
	Move(rp,bounds.MaxX+2,bounds.MinY);  Draw(rp,bounds.MaxX+2,bounds.MaxY);

	SetABPenDrMd(rp,pens[TEXTPEN],bpen,JAM2);
	bounds.MinX = bounds.MaxX+3;  bounds.MaxX = msg->lvdm_Bounds.MaxX;
	WriteHookText(rp,&bounds,fv->fv_Preview,bpen);

	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
		GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
	return(LVCB_OK);
}


ULONG PUBLIC gLinkHook(REG(a0, struct Hook *h), REG(a2, struct gLink *gl), REG(a1, struct LVDrawMsg *msg))
{
	struct RastPort *rp;
	struct Rectangle bounds;
	UBYTE  state;
	ULONG  apen,bpen;
	UWORD  *pens,y;

	if (msg->lvdm_MethodID != LV_DRAW)
		return(LVCB_UNKNOWN);

	rp = msg->lvdm_RastPort;
	state = msg->lvdm_State;
	pens = msg->lvdm_DrawInfo->dri_Pens;
	bounds = msg->lvdm_Bounds;

	apen = pens[TEXTPEN];
	bpen = pens[FILLPEN];

	if ((state == LVR_NORMAL) || (state == LVR_NORMALDISABLED))
	{
		//apen = pens[TEXTPEN];
		bpen = pens[BACKGROUNDPEN];
	}
	bounds.MaxX = bounds.MinX+2*boxwidth-2;

	SetABPenDrMd(rp,apen,bpen,JAM2);

	if (!(gl->gl_Flags & GLF_FIRST_OF_ROW))
	{
		if (gl->gl_Flags & GLF_LAST_OF_ROW)
		{
			Move(rp,bounds.MaxX+1,bounds.MinY);
			Draw(rp,bounds.MaxX+1,y = bounds.MinY+((bounds.MaxY-bounds.MinY) >> 1));
			Draw(rp,bounds.MaxX+4,y);
			SetAPen(rp,bpen);
			RectFill(rp,bounds.MaxX+2,bounds.MinY,bounds.MaxX+4,y-1);
			RectFill(rp,bounds.MaxX+1,y+1,bounds.MaxX+4,bounds.MaxY);
		}
		else
		{
			Move(rp,bounds.MaxX+1,bounds.MinY);
			Draw(rp,bounds.MaxX+1,bounds.MaxY);
			SetAPen(rp,bpen);
			RectFill(rp,bounds.MaxX+2,bounds.MinY,bounds.MaxX+4,bounds.MaxY);
		}
		RectFill(rp,bounds.MinX,bounds.MinY,bounds.MaxX,bounds.MaxY);
	}
	else
	{
		char t[16];

		if (gl->gl_Flags & GLF_LAST_OF_ROW)
		{
			Move(rp,bounds.MaxX+4,bounds.MinY+1);
			Draw(rp,bounds.MaxX+1,bounds.MinY+1);
			Draw(rp,bounds.MaxX+1,bounds.MaxY-1);
			Draw(rp,bounds.MaxX+4,bounds.MaxY-1);

			SetAPen(rp,bpen);
			RectFill(rp,bounds.MaxX+2,bounds.MinY+2,bounds.MaxX+4,bounds.MaxY-2);
			RectFill(rp,bounds.MaxX+1,bounds.MinY,bounds.MaxX+4,bounds.MinY);
			RectFill(rp,bounds.MaxX+1,bounds.MaxY,bounds.MaxX+4,bounds.MaxY);
		}
		else
		{
			Move(rp,bounds.MaxX+4,y = bounds.MinY-1+((bounds.MaxY-bounds.MinY) >> 1));
			Draw(rp,bounds.MaxX+1,y);
			Draw(rp,bounds.MaxX+1,bounds.MaxY);

			SetAPen(rp,bpen);
			RectFill(rp,bounds.MaxX+1,bounds.MinY,bounds.MaxX+4,y-1);
			RectFill(rp,bounds.MaxX+2,y+1,bounds.MaxX+4,bounds.MaxY);
		}
		SetAPen(rp,apen);
		sprintf(t,"%lu",gl->gl_Row);
		WriteHookText(rp,&bounds,t,bpen);
	}

	bounds.MinX = bounds.MaxX+5;  bounds.MaxX = msg->lvdm_Bounds.MaxX;

	SetHighColor(rp,gl->gl_Color);
	DrawColoredBox(rp,bounds,pens[SHINEPEN],pens[SHADOWPEN],bpen);

	bounds.MinX += boxwidth+4;  bounds.MaxX -= boxwidth;

	if (gl->gl_Cell)
	{
		SetAPen(rp,apen);
		WriteHookText(rp,&bounds,gl->gl_Cell->tf_Text,bpen);
	}
	else
	{
		SetAPen(rp,bpen);
		RectFill(rp,bounds.MinX,bounds.MinY,bounds.MaxX,bounds.MaxY);
	}
	bounds.MinX = bounds.MaxX;  bounds.MaxX = msg->lvdm_Bounds.MaxX;

	SetAPen(rp,bpen);
	RectFill(rp,bounds.MinX,bounds.MinY,bounds.MaxX,bounds.MaxY);

	if (gl->gl_Flags & GLF_MARKED)
	{
		SetABPenDrMd(rp,apen,bpen,JAM2);
		DrawSelectHook(rp,bounds.MinX,bounds.MinY+2,bounds.MaxY-2);
	}
	if ((state == LVR_NORMALDISABLED) || (state == LVR_SELECTEDDISABLED))
		GhostRect(rp,pens[BLOCKPEN],msg->lvdm_Bounds.MinX, msg->lvdm_Bounds.MinY,msg->lvdm_Bounds.MaxX, msg->lvdm_Bounds.MaxY);
	return(LVCB_OK);
}

/********************************** Link-Hook **********************************/

ULONG PUBLIC LinkHook(REG(a0, struct Hook *h), REG(a2, struct Link *l), REG(a1, struct LVDrawMsg *msg))
{
	if (l->l_HookFunction)
		return(l->l_HookFunction(0, l->l_Link, msg));
	return(LVCB_OK);
}


void
FreeLinks(struct MinList *links)
{
	struct Link *l;

	while ((l = (struct Link *)MyRemHead(links)) != 0)
		FreePooled(pool, l, sizeof(struct Link));
}


struct Link *
AddLinkNode(struct MinList *links, struct MinNode *ln, APTR func)
{
	struct Link *l;

	if (!links || !ln || !func)
		return NULL;

	if ((l = AllocPooled(pool, sizeof(struct Link))) != 0)
	{
		l->l_Link = ln;
		l->l_HookFunction = func;
		MyAddTail(links,l);
	}
	return l;
}


void
AddLinkList(struct MinList *links,struct MinList *add,APTR func)
{
	struct MinNode *aln;

	if (!links || !add)
		return;

	if (!func)
		func = renderHook.h_Entry;

	foreach(add,aln)
		AddLinkNode(links,aln,func);
}


/********************************** Tree-Hook **********************************/

#ifdef DEBUG
void printTree(struct MinList *tree)
{
	struct TreeNode *tn;
	long   i;

	foreach(tree,tn)
	{
		bug("%2ld",tn->tn_Depth);
		for(i = 0;i < tn->tn_Depth;i++)
		{
			if (tn->tn_DepthLines & (1L << tn->tn_Depth))
				bug("| ");
			else
				bug("  ");
		}
		if (tn->tn_Flags & TNF_CONTAINER)
		{
			bug("+ %s\n",tn->tn_Node.in_Name);
			printTree(&tn->tn_Nodes);
		}
		else
			bug("  %s\n",tn->tn_Node.in_Name);
	}
}
#endif // DEBUG
