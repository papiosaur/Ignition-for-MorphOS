#include <exec/exec.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/gtdrag.h>
#include <intuition/intuition.h>
#include <proto/gtdrag.h>
//#include <stdlib.h>
#include <clib/macros.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

#define IsCustomGadget(gad) ((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET && gad->MutualExclude)

ULONG rmbtrap;

struct DragApp *GetDragApp(struct GtdragIFace *Self, struct Task *task)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
	struct DragApp *da;

	if (!task)
		task = IExec->FindTask(NULL);

	foreach(&applist,da)
	{
		if (da->da_Task == task)
			return(da);
	}
	return(NULL);
}


struct DragGadget *AddDragGadget(struct GtdragIFace *Self, struct Gadget *gad,struct Window *win,ULONG type)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragGadget *dg = NULL;
  	struct DragApp *da;

  	IExec->ObtainSemaphore(&ListSemaphore);
  	if ((da = GetDragApp(Self,NULL)) && gad && win && (dg = IExec->AllocVecTags(sizeof(struct DragGadget), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE)))
  	{
    	dg->dg_Gadget = gad;
	    dg->dg_Window = win;
    	dg->dg_Task = da->da_Task;
	    dg->dg_Type = type;
    	dg->dg_AcceptMask = 0xffffffff;
	    dg->dg_Object.od_InternalType = 0xffffffff;
    	dg->dg_Object.od_Owner = da->da_Name;
	    IExec->AddTail((struct List *)&gadlist,(APTR)dg);
  	}
  	IExec->ReleaseSemaphore(&ListSemaphore);
  	return(dg);
}


struct DragWindow *FindDragWindow(struct Window *win)
{
  	struct DragWindow *dw;

  	foreach(&winlist,dw)
  	{
    	if (dw->dw_Window == win)
      		return(dw);
  	}
  	return(NULL);
}


void SetWindowAttrs(struct GtdragIFace *Self, struct Window *win,struct TagItem *tags)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
  	struct TagItem *tstate,*ti;
  	struct DragWindow *dw;

  	if (!(dw = FindDragWindow(win)))
    	return;

  	tstate = (struct TagItem *)tags;
  	while((ti = IUtility->NextTagItem(&tstate)) != 0)
  	{
    	switch(ti->ti_Tag)
    	{
      		case GTDA_AcceptMask:
        		dw->dw_AcceptMask = ti->ti_Data;
        		break;
      		case GTDA_AcceptFunc:
        		dw->dw_AcceptFunc = (APTR)ti->ti_Data;
        		break;
    	}
  	}
}


void PrepareDrag(struct GtdragIFace *Self, BOOL boopsi)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;

  	rmbtrap = dg->dg_Window->Flags & WFLG_RMBTRAP;

  	if (boopsi)
    	return;

  	if (!(dg->dg_Window->Flags & WFLG_REPORTMOUSE))
  	{
    	IIntuition->ReportMouse(TRUE,dg->dg_Window);
    	noreport = TRUE;
  	}
  	dg->dg_Window->Flags |= WFLG_RMBTRAP;
}


void FreeDragGadget(struct GtdragIFace *Self, struct DragGadget *dg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;

  	if (!dg)
    	return;

  	IExec->FreeVec(dg);
}


struct DragGadget *FindDragGadget(struct Gadget *gad)
{
  	struct DragGadget *dg;

  	foreach(&gadlist,dg)
  	{
    	if (dg->dg_Gadget == gad)
      		return(dg);
  	}
  	return(NULL);
}


void UpdateDragObj(struct GtdragIFace *Self, struct DragObj *gdo,int x, int y)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
  	int  px,py,pw,ph,sx,sy,rx,ry,rw,rh;

  	x += gdo->do_DeltaX;
  	y += gdo->do_DeltaY;
  	if (x == gdo->do_X && y == gdo->do_Y)
    	return;
  	px = x;  py = y;  pw = gdo->do_Width;  ph = gdo->do_Height;
  	sx = 0;  sy = 0;
  	if (px < 0 && px+pw > 0)
  	{
    	pw += px;
    	sx -= px;
    	px = 0;
  	}
  	if (px+pw > gdo->do_Screen->Width)
    	pw = gdo->do_Screen->Width-px;
  	if (py < 0 && py+ph > 0)
  	{
    	ph += py;
    	sy -= py;
    	py = 0;
  	}
  	if (py+ph > gdo->do_Screen->Height)
    	ph = gdo->do_Screen->Height-py;

  	rx = MIN(gdo->do_PX,px);  rw = MAX(gdo->do_PX+gdo->do_PWidth,px+pw)-rx;
  	ry = MIN(gdo->do_PY,py);  rh = MAX(gdo->do_PY+gdo->do_PHeight,py+ph)-ry;

  	if (px+pw > 0 && py+ph > 0 && (rw > gdo->do_Width*2 || rh > gdo->do_Height*2))
  	{
    	/******************* Außerhalb der Refresh-BitMap ************************/
    	if ((gdo->do_PX+gdo->do_PWidth > 0) && (gdo->do_PY+gdo->do_PHeight > 0))
      		IGraphics->BltBitMapRastPort(gdo->do_SaveBack,gdo->do_SX,gdo->do_SY,gdo->do_ScrRPort,gdo->do_PX,gdo->do_PY,gdo->do_PWidth,gdo->do_PHeight,0xc0);
    	IGraphics->BltBitMap(gdo->do_ScrRPort->BitMap,px,py,gdo->do_SaveBack,sx,sy,pw,ph,0xc0,0xff,NULL);
    	IGraphics->BltMaskBitMapRastPort(gdo->do_BitMap,sx,sy,gdo->do_ScrRPort,px,py,pw,ph,0xe0,(PLANEPTR)gdo->do_Mask);
  	}
  	else if ((px+pw > 0) && (py+ph > 0))
  	{
    	/******************* Innerhalb der Refresh-BitMap *********************/
    	IGraphics->ClipBlit(gdo->do_ScrRPort,rx,ry,&gdo->do_RefreshRPort,0,0,rw,rh,0xc0);
    	if ((gdo->do_PX+gdo->do_PWidth > 0) && (gdo->do_PY+gdo->do_PHeight > 0))
      		IGraphics->BltBitMapRastPort(gdo->do_SaveBack,gdo->do_SX,gdo->do_SY,&gdo->do_RefreshRPort,gdo->do_PX-rx,gdo->do_PY-ry,gdo->do_PWidth,gdo->do_PHeight,0xc0);
    	IGraphics->BltBitMap(gdo->do_RefreshMap,px-rx,py-ry,gdo->do_SaveBack,sx,sy,pw,ph,0xc0,0xff,NULL);
    	IGraphics->BltMaskBitMapRastPort(gdo->do_BitMap,sx,sy,&gdo->do_RefreshRPort,px-rx,py-ry,pw,ph,0xe0,(PLANEPTR)gdo->do_Mask);
    	IGraphics->ClipBlit(&gdo->do_RefreshRPort,0,0,gdo->do_ScrRPort,rx,ry,rw,rh,0xc0);
  	}
  	else if ((gdo->do_PX+gdo->do_PWidth > 0) && (gdo->do_PY+gdo->do_PHeight > 0))
  	{
    	/******************* Nur SaveBack zurück ********************/
    	IGraphics->BltBitMapRastPort(gdo->do_SaveBack,gdo->do_SX,gdo->do_SY,gdo->do_ScrRPort,gdo->do_PX,gdo->do_PY,gdo->do_PWidth,gdo->do_PHeight,0xc0);
  	}
  	gdo->do_SX = sx;  gdo->do_SY = sy;
  	gdo->do_PX = px;  gdo->do_PY = py;
  	gdo->do_PWidth = pw;  gdo->do_PHeight = ph;
  	gdo->do_X = x;
  	gdo->do_Y = y;
}


struct DragObj *CreateDragObj(struct GtdragIFace *Self, struct DragGadget *dg,int x,int y)
	{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
	struct ExecIFace *IExec = libBase->IExec;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;
	struct CyberGfxIFace *ICyberGfx = libBase->ICyberGfx;
	struct Screen *scr;
  	struct RastPort *rp;
  	struct DragObj *gdo = NULL;
  	ULONG line;
  	int wordwidth;
  	int width,height,depth;
  	int i = 0,xpos,ypos;

  	scr = dg->dg_Window->WScreen;
  	rp = &scr->RastPort;

  	if (dg->dg_Flags & DGF_IMAGES && ((struct ImageNode *)dg->dg_Object.od_Object)->in_Image)
    	dg->dg_Image = ((struct ImageNode *)dg->dg_Object.od_Object)->in_Image;

  	if (!dg->dg_Width || !dg->dg_Height)
  	{
    	if ((dg->dg_Type == LISTVIEW_KIND) && !dg->dg_Image)
    	{
      		dg->dg_Width = dg->dg_Gadget->Width-20;
      		dg->dg_Height = dg->dg_ItemHeight;
    	}
    	else if (!dg->dg_RenderHook && dg->dg_Image)
    	{
      		dg->dg_Width = dg->dg_Image->Width;
      		dg->dg_Height = dg->dg_Image->Height;
    	}
    	else  /* be sure width & height are not zero */
    	{
      		dg->dg_Width = dg->dg_Gadget->Width;
      		dg->dg_Height = dg->dg_Gadget->Height;
    	}
  	}
  	width = dg->dg_Width;
  	height = dg->dg_Height;
  	IUtility->ClearMem((APTR)&dm,sizeof(struct DropMessage));
  	if (dg->dg_Type == LISTVIEW_KIND)
  	{
    	xpos = dg->dg_Gadget->LeftEdge+2;
    	ypos = dg->dg_Gadget->TopEdge+2;
    	dg->dg_Object.od_Object = NULL;

    	if (y < ypos || y > ypos+dg->dg_Gadget->Height-5)
      		return(NULL);
    	line = (y-ypos)/dg->dg_ItemHeight;
    	ypos += line*dg->dg_ItemHeight;

    	IGadTools->GT_GetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Labels,&dg->dg_List,TAG_END);
    	if (dg->dg_List && !IsListEmpty(dg->dg_List))
    	{
      		IGadTools->GT_GetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Top,&i,TAG_END);
      		i += line;
      		if (i < CountNodes(dg->dg_List))
      		{
        		struct Node *ln;

        		dm.dm_SourceEntry = i;
        		for(ln = dg->dg_List->lh_Head;i;i--,ln = ln->ln_Succ);

        		if (dg->dg_Flags & DGF_TREEVIEW && TREENODE(ln)->tn_Flags & TNF_STATIC)
        		{
          			mx = ~0L;      // avoid a following drag
          			return(NULL);
        		}
        		dg->dg_Object.od_Object = ln;

        		if (dg->dg_ObjectFunc)
          			dg->dg_ObjectFunc(dg->dg_Window,dg->dg_Gadget,&dg->dg_Object,dm.dm_SourceEntry);
      		}
    	}
  	}
  	else
  	{
    	if (dg->dg_ObjectFunc)
      		dg->dg_ObjectFunc(dg->dg_Window,dg->dg_Gadget,&dg->dg_Object,0L);

    	dm.dm_SourceEntry = dg->dg_SourceEntry;
    	xpos = x-width/2;
    	ypos = y-height/2;
  	}
  	if (!dg->dg_Object.od_Object)
  	{
    	mx = ~0L;        // avoid a following drag
    	return(NULL);
  	}
  	wordwidth = (width + 15) >> 4;
  	depth = IGraphics->GetBitMapAttr(rp->BitMap,BMA_DEPTH);

  	if (dg->dg_Object.od_Object && (gdo = IExec->AllocVecTags(sizeof(struct DragObj), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE)))
  	{
#ifdef LOCKLAYERS
	    LockLayers(&scr->LayerInfo);
    	UnlockLayer(dg->dg_Window->RPort->Layer);
#endif

    	gdo->do_Screen = scr;
    	gdo->do_ScrRPort = rp;

    	gdo->do_BitMap = IGraphics->AllocBitMap(width,height,depth,BMF_CLEAR | BMF_MINPLANES,!(IGraphics->GetBitMapAttr( rp->BitMap, BMA_FLAGS ) & BMF_INTERLEAVED) ? rp->BitMap : NULL);
    	gdo->do_SaveBack = IGraphics->AllocBitMap(width,height,depth,BMF_CLEAR | BMF_MINPLANES,rp->BitMap);
    	gdo->do_RefreshMap = IGraphics->AllocBitMap(width*2,height*2,depth,BMF_CLEAR | BMF_MINPLANES,rp->BitMap);

    	if (IGraphics->GetBitMapAttr(gdo->do_BitMap,BMA_FLAGS) & BMF_STANDARD)
      		i = MEMF_CHIP | MEMF_SHARED;
    	else
      		i = MEMF_SHARED;

    	gdo->do_FullShadow = IExec->AllocVecTags(2*wordwidth*height, AVT_Type, i, AVT_ClearWithValue, 0, TAG_DONE);
    	gdo->do_HalfShadow = IExec->AllocVecTags(2*wordwidth*height, AVT_Type, i, TAG_DONE);

    	if (gdo->do_BitMap && gdo->do_SaveBack && gdo->do_RefreshMap && gdo->do_FullShadow && gdo->do_HalfShadow)
    	{
      		IGraphics->InitRastPort(&gdo->do_RPort);
      		gdo->do_RPort.BitMap = gdo->do_BitMap;
      		IGraphics->InitRastPort(&gdo->do_RefreshRPort);
      		gdo->do_RefreshRPort.BitMap = gdo->do_RefreshMap;

      		gdo->do_DragGadget = dg;
      		IExec->CopyMem(&dg->dg_Object,&dm.dm_Object,sizeof(struct ObjectDescription));
      		dm.dm_Window = dg->dg_Window;
      		dm.dm_Gadget = dg->dg_Gadget;

      		/*** create the drag&drop image ***/

      		if (dg->dg_RenderHook)
      		{
        		struct LVDrawMsg lvdm;

        		IGraphics->SetFont(&gdo->do_RPort,scr->RastPort.Font);
	        	lvdm.lvdm_MethodID = LV_DRAW;
    	    	lvdm.lvdm_RastPort = &gdo->do_RPort;
        		lvdm.lvdm_DrawInfo = IIntuition->GetScreenDrawInfo(scr);
        		lvdm.lvdm_Bounds.MinX = 0;
	        	lvdm.lvdm_Bounds.MinY = 0;
    	    	lvdm.lvdm_Bounds.MaxX = width-1;
        		lvdm.lvdm_Bounds.MaxY = height-1;
        		lvdm.lvdm_State = LVR_SELECTED;
	        	IUtility->CallHookPkt(dg->dg_RenderHook,dm.dm_Object.od_Object,&lvdm);
    	    	IIntuition->FreeScreenDrawInfo(scr,lvdm.lvdm_DrawInfo);
      		}
      		else if (dg->dg_Image)
        		IIntuition->DrawImage(&gdo->do_RPort,dg->dg_Image,0,0);
      		else
        		IGraphics->ClipBlit(dg->dg_Window->RPort,xpos,ypos,&gdo->do_RPort,0,0,width,height,0xc0);

      		/*** initialize drag object structure ***/

      		gdo->do_X = -9999;
      		gdo->do_Y = ypos+dg->dg_Window->TopEdge;
      		gdo->do_PX = -9999;
      		gdo->do_Width = width;
      		gdo->do_Height = height;
      		gdo->do_DeltaX = xpos-x+dg->dg_Window->LeftEdge;
      		gdo->do_DeltaY = ypos-y+dg->dg_Window->TopEdge;
      		gdo->do_Mask = gdo->do_FullShadow;

      		/*** create masks (transparent and full imagery) ***/

      		if (ICyberGfx && (IGraphics->GetBitMapAttr(gdo->do_BitMap,BMA_FLAGS) & BMF_STANDARD) == 0L)
      		{
        		struct BitMap tbm;
        		ULONG  col;

        		IGraphics->InitBitMap(&tbm,1,width,height);
        		tbm.Planes[0] = (UBYTE *)gdo->do_FullShadow;

        		/* if (!GetCyberMapAttr(gdo->do_BitMap, CYBRMATTR_PIXELFMT)) */
        		if (IGraphics->GetBitMapAttr(gdo->do_BitMap, BMA_DEPTH) > 8L)
        		{
          			ULONG triplet[3];

          			IGraphics->GetRGB32(scr->ViewPort.ColorMap,0L,1L,triplet);
          			col = (triplet[0] & 0xff0000) | (triplet[1] & 0xff00) | (triplet[2] & 0xff);
        		}	
        		else
          			col = 0;

        		// ExtractColor(rp,&tbm,col,xpos,ypos,width,height);
        		ICyberGfx->ExtractColor(&gdo->do_RPort,&tbm,col,0,0,width,height);
        		IGraphics->BltBitMap(&tbm,0,0,&tbm,0,0,width,height,0x50,0xff,NULL);  // invertieren der Maske
      		}
      		else
      		{
        		UWORD *p = gdo->do_FullShadow;

        		for(ypos = 0;ypos < height;ypos++)
        		{
          			for(xpos = 0;xpos < wordwidth;xpos++,p++)
          			{
            			for(i = 0;i < depth;i++)
              				*p |= *((UWORD *)gdo->do_BitMap->Planes[i]+ypos*(gdo->do_BitMap->BytesPerRow >> 1)+xpos);
          			}
        		}
      		}

      		{
        		UWORD *p = gdo->do_HalfShadow;

        		IExec->CopyMem(gdo->do_FullShadow,p,2*wordwidth*height);
        		for(line = 0x5555,ypos = 0;ypos < height;ypos++)
        		{
          			line = ~line;
          			for(xpos = 0;xpos < wordwidth;xpos++,p++)
            			*p &= (UWORD)line;
        		}
      		}

      		if (!boopsigad)
        		FakeInputEvent(Self);
      		UpdateDragObj(Self, gdo,gdo->do_X,gdo->do_Y);    /* show drag object */

      		return(gdo);
    	}
    	IGraphics->FreeBitMap(gdo->do_BitMap);
    	IGraphics->FreeBitMap(gdo->do_SaveBack);
    	IGraphics->FreeBitMap(gdo->do_RefreshMap);
    	IExec->FreeVec(gdo);
  	}
  	return(NULL);
}


BOOL PointInGadget(struct GtdragIFace *Self, struct Gadget *gad,WORD x,WORD y)
{
  	return((BOOL)(x >= gad->LeftEdge && x <= gad->LeftEdge+gad->Width && y >= gad->TopEdge && y <= gad->TopEdge+gad->Height));
}


BOOL PointInDragGadget(struct GtdragIFace *Self, struct DragGadget *dg,int x,int y,BOOL lheight)
{
  	struct Gadget *gad = dg->dg_Gadget;
  	int    w,h;

  	x -= gad->LeftEdge;  y -= gad->TopEdge;
  	w = gad->Width;  h = gad->Height;

  	if (dg->dg_Type == LISTVIEW_KIND)
  	{
    	w -= 18;
    	if (lheight)
    	{
      		y += 4;
      		h += 8;
    	}
  	}
  	if (x >= 0 && x <= w && y >= 0 && y <= h)
    	return(TRUE);

  	return(FALSE);
}


void EndDrag(struct GtdragIFace *Self)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
	struct ExecIFace *IExec = libBase->IExec;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;
  	struct DragGadget *sdg,*ndg;

  	if (noreport)
    	IIntuition->ReportMouse(FALSE,dg->dg_Window);
  	noreport = FALSE;
  	dg->dg_Window->Flags = (dg->dg_Window->Flags & ~WFLG_RMBTRAP) | rmbtrap;

  	if (dg->dg_Type == LISTVIEW_KIND)
    	IGadTools->GT_SetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Selected,dg->dg_Selected,TAG_END);

  	adg = dg = NULL;  gdo = NULL;  boopsigad = NULL;
  	mx = ~0L;

  	for(sdg = (APTR)gadlist.mlh_Head;(ndg = (APTR)sdg->dg_Node.mln_Succ) != 0;sdg = ndg)
  	{
    	if (sdg->dg_Flags & DGF_ONTHEFLY)     // remove temporary BOOPSI-gadgets
    	{
      		IExec->Remove((APTR)sdg);
      		FreeDragGadget(Self, sdg);
    	}
    	else if (sdg->dg_Type == BOOPSI_KIND) // reset constant BOOPSI-gadgets
      		sdg->dg_Flags &= ~(DGF_USERENDERING | DGF_FINAL);

    	sdg->dg_CurrentObject = NULL;
  	}
}


struct DragGadget *WhichDragGadget(struct GtdragIFace *Self, APTR layer,int mx,int my)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragGadget *dg;

  	IExec->ObtainSemaphore(&ListSemaphore);
  	foreach(&gadlist,dg)
  	{
    	if (dg->dg_Window->WLayer == layer)
    	{
     		if (PointInDragGadget(Self, dg,mx-dg->dg_Window->LeftEdge,my-dg->dg_Window->TopEdge,FALSE))
      		{
        		IExec->ReleaseSemaphore(&ListSemaphore);
        		return(dg);
      		}
    	}
  	}
  	IExec->ReleaseSemaphore(&ListSemaphore);
  	return(NULL);
}


BOOL DragGadgetAcceptsObject(struct GtdragIFace *Self, struct DragGadget *adg,WORD x,WORD y)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
  	struct DragApp *ada;

  	if (!adg)
    	return(FALSE);

  	if (adg->dg_CurrentObject == dg->dg_Object.od_Object)
    	return((BOOL)(adg->dg_Flags & DGF_LIKECURRENT ? TRUE : FALSE));

  	if (adg->dg_Type != BOOPSI_KIND || adg->dg_Flags & DGF_FINAL)
    	adg->dg_CurrentObject = dg->dg_Object.od_Object;
  	adg->dg_Flags &= ~DGF_LIKECURRENT;

  	{
    	ULONG store;

    	if (IGadTools->GT_GetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GA_Disabled,&store,TAG_END) && store)
      		return(FALSE);
  	}
  	if (adg->dg_Gadget->Flags & GFLG_DISABLED)
    	return(FALSE);

  	if (dg == adg && !(dg->dg_Flags & DGF_SAME))
    	return(FALSE);

  	ada = GetDragApp(Self, adg->dg_Task);
  	if (dg->dg_Task != adg->dg_Task && ((ada && ada->da_Flags & DAF_INTERN) || (adg->dg_Flags & DGF_INTERNALONLY))) //Geänderte klammern
    	return(FALSE);

  	if (dg->dg_Task == adg->dg_Task && !(adg->dg_AcceptMask & dg->dg_Object.od_InternalType))
    	return(FALSE);


  	if (adg->dg_AcceptFunc)
    	return(adg->dg_AcceptFunc(adg->dg_Window,adg->dg_Gadget,&dg->dg_Object));

  	{
    	struct Gadget *gad;

    	if (adg->dg_Type == BOOPSI_KIND && DoCustomMethod(Self, gad = adg->dg_Gadget,GM_OBJECTDRAG,&dg->dg_Object,dg->dg_Gadget,words(x-gad->LeftEdge,y-gad->TopEdge)) & GMR_REJECTOBJECT)
      		return(FALSE);
  	}

  	return(TRUE);
}


struct DragGadget *GetAcceptorDragGadget(struct GtdragIFace *Self, int mx,int my)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct LayersIFace *ILayers = libBase->ILayers;
  	struct DragGadget *sdg;
  	APTR   layer = ILayers->WhichLayer(&gdo->do_Screen->LayerInfo,mx,my);

  	if ((sdg = WhichDragGadget(Self, layer,mx,my)) != 0)
    	return(DragGadgetAcceptsObject(Self, sdg,mx-sdg->dg_Window->LeftEdge,my-sdg->dg_Window->TopEdge) ? sdg : NULL);

  	/*** BOOPSI gadgets ***/
  	{
    	struct Window *win;
    	long   x,y;

    	for(win = gdo->do_Screen->FirstWindow;win;win = win->NextWindow)
    	{
      		if (layer == win->WLayer)
      		{
        		struct Gadget *gad;

		        x = mx-win->LeftEdge;
        		y = my-win->TopEdge;

		        for(gad = win->FirstGadget;gad;gad = gad->NextGadget)
        		{
          			if (IsCustomGadget(gad) && PointInGadget(Self, gad,x,y))
          			{
            			long rc;

		            	if ((rc = DoCustomMethod(Self, gad,GM_OBJECTDRAG,&dg->dg_Object,dg->dg_Gadget,words(x-gad->LeftEdge,y-gad->TopEdge))) != 0)
            			{
              				if ((sdg = AddDragGadget(Self, gad,win,BOOPSI_KIND)) != 0)
              				{
                				if (rc & GMR_FINAL)
                  					sdg->dg_CurrentObject = dg->dg_Object.od_Object;
                				sdg->dg_Flags |= DGF_ONTHEFLY | (rc & GMR_FINAL ? DGF_FINAL : 0);
              				}
              				if (rc & GMR_ACCEPTOBJECT)
                				return(sdg);
            			}
          			}
        		}
      		}
    	}
  	}
  	return(NULL);
}

ULONG VARARGS68K DoCustomMethod(struct GtdragIFace *Self, struct Gadget *gad, ULONG method,...);
ULONG DoCustomMethod(struct GtdragIFace *Self, struct Gadget *gad, ULONG method,...)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
	va_list ap;
	ULONG *methods;

	va_startlinear(ap, method);
	methods = va_getlinearva(ap, ULONG *);
//  	return(IUtility->CallHookPkt((struct Hook *)gad->MutualExclude,gad,&method));
  	return(IUtility->CallHookPkt((struct Hook *)gad->MutualExclude,gad,methods));
}


ULONG DoRenderMethod(struct GtdragIFace *Self, struct DragGadget *rdg,WORD x,WORD y,ULONG mode)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;
  	struct GadgetInfo gi;
  	struct RastPort *rp;
  	struct Gadget *gad = rdg->dg_Gadget;
  	ULONG  rc = 0L;

  	IUtility->SetMem(&gi,0,sizeof(struct GadgetInfo));
  	gi.gi_Window = rdg->dg_Window;
  	gi.gi_Screen = gi.gi_Window->WScreen;
  	gi.gi_RastPort = gi.gi_Window->RPort;
  	gi.gi_Layer = gi.gi_Window->WLayer;
  	gi.gi_Domain.Left = rdg->dg_Gadget->LeftEdge;
  	gi.gi_Domain.Top = rdg->dg_Gadget->TopEdge;
  	gi.gi_Domain.Width = rdg->dg_Gadget->Width;
  	gi.gi_Domain.Height = rdg->dg_Gadget->Height;
  	gi.gi_DrInfo = IIntuition->GetScreenDrawInfo(gi.gi_Screen);

  	if ((rp = IIntuition->ObtainGIRPort(&gi)) != 0)
  	{
    	if (mode != GRENDER_DELETE)
    	{
      		x += dg->dg_Window->LeftEdge-rdg->dg_Window->LeftEdge-gad->LeftEdge;
      		y += dg->dg_Window->TopEdge-rdg->dg_Window->TopEdge-gad->TopEdge;
    	}
    	rc = DoCustomMethod(Self, gad,GM_RENDERDRAG,&gi,rp,mode,words(x,y));
    	IIntuition->ReleaseGIRPort(rp);
  	}
  	IIntuition->FreeScreenDrawInfo(gi.gi_Screen,gi.gi_DrInfo);

  return(rc);
}


BOOL HighlightLVDragGadget(struct GtdragIFace *Self, struct DragGadget *adg,long class,short y)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
  	short  line;
  	long   pos = 0;

  	if (!adg->dg_List || IsListEmpty(adg->dg_List))
   		return(FALSE);

  	if (class == IDCMP_INTUITICKS && (y <= hy || y >= hy+hh) && !(adg->dg_Flags & DGF_NOSCROLL))  /* Scrolling */
  	{
    	IGadTools->GT_GetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GTLV_Top,&pos,TAG_END);
    	if (y <= hy)
      		pos--;
    	else
      		pos++;
    	if (pos >= 0 && pos+((hh-4)/adg->dg_ItemHeight) <= CountNodes(adg->dg_List))
    	{
      		UpdateDragObj(Self, gdo,-9999,0);
      		IGraphics->ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);
      	IGadTools->GT_SetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GTLV_Top,pos,TAG_END);
      	IGraphics->ClipBlit(&hsrp,hx,hy,&hrp,0,0,hw,hh,0xc0);
      	return(TRUE);
    	}
  	}
  	else if (class == IDCMP_MOUSEMOVE && !(adg->dg_Flags & DGF_NOPOS) && y > hy && y < hy+hh)  /* Positioning */
  	{
    	long num = CountNodes(adg->dg_List);
    	struct Node *ln;
    	long mode = 0;

    	IGadTools->GT_GetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GTLV_Top,&pos,TAG_END);

    	line = (y-hy-2)/adg->dg_ItemHeight;
    	pos += line;
    	ln = FindListNumber((struct MinList *)adg->dg_List,pos);

    	if (adg->dg_Flags & (DGF_DROPOVER | DGF_TREEVIEW) && ln != dm.dm_Object.od_Object)
    	{
      		int point = (y-hy-2) % adg->dg_ItemHeight;

      		if ((adg->dg_Flags & (DGF_DROPOVER | DGF_DROPBETWEEN)) == DGF_DROPOVER || ((adg->dg_Flags & DGF_DROPBETWEEN) && point > 2 && adg->dg_ItemHeight-point > 2))
      		{
        	mode = DMF_DROPOVER;

        	if (num <= pos)
          		pos = line = num-1;

        	if ((line+1)*adg->dg_ItemHeight+2 > hh)
	          	return(FALSE);
      		}
      		else if (adg->dg_Flags & DGF_TREEVIEW && y > hy+2 && y < hy+hh-2)
      		{
        		struct TreeNode *tn = TREENODE(ln);

        		if (tn->tn_Flags & TNF_CONTAINER)
          			mode = DMF_DROPOVER;
      		}
    	}
    	if ((adg->dg_Flags & (DGF_DROPOVER | DGF_DROPBETWEEN)) == DGF_DROPOVER && ln == dm.dm_Object.od_Object)
      		return(FALSE);

    	if (mode != DMF_DROPOVER)
    	{
      		pos -= line;
      		line = (y-hy-2+(adg->dg_ItemHeight >> 1))/adg->dg_ItemHeight;
      		pos += line;

      		if (num < pos)        // weniger Einträge als sichtbare Höhe der Listview
        		pos = line = num;
    	}
    	y = line*adg->dg_ItemHeight+1+hy;

    	if (pos != dm.dm_TargetEntry || (dm.dm_Flags & DMF_DROPOVER) != mode)
    	{
      		dm.dm_TargetEntry = pos;
      		dm.dm_Flags = (dm.dm_Flags & ~DMF_DROPOVER) | mode;

	      	UpdateDragObj(Self, gdo,-9999,0);
      		IGraphics->ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);

      		IGraphics->SetABPenDrMd(&hsrp,1,2,JAM2);
      		IGraphics->Move(&hsrp,hx+2,y);
      		IGraphics->Draw(&hsrp,hx+hw-3,y);

      		if (mode & DMF_DROPOVER)
      		{
        		IGraphics->Move(&hsrp,hx+2+hw-3,y+adg->dg_ItemHeight);
        		IGraphics->Draw(&hsrp,hx+2,y+adg->dg_ItemHeight);
      		}
      	return(TRUE);
    	}
  	}
  	return(FALSE);
}


BOOL UpdateHighlighting(struct GtdragIFace *Self, ULONG class,WORD mousex,WORD mousey,struct DragGadget *sdg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
  	long cx,cy;

  	if (!sdg)
    	return(FALSE);

  	cx = mousex+dg->dg_Window->LeftEdge-sdg->dg_Window->LeftEdge;
  	cy = mousey+dg->dg_Window->TopEdge-sdg->dg_Window->TopEdge;

  	if (PointInDragGadget(Self, sdg,cx,cy,TRUE))
  	{
    	switch(sdg->dg_Type)
    	{
      		case LISTVIEW_KIND:
        		return(HighlightLVDragGadget(Self, sdg,class,cy));
      		case BOOPSI_KIND:
      			{
        		struct Gadget *gad = sdg->dg_Gadget;
        		ULONG  rc;

        		if (sdg->dg_Flags & DGF_FINAL || !(sdg->dg_Flags & DGF_USERENDERING))
          			break;

        		rc = DoCustomMethod(Self, gad,GM_OBJECTDRAG,&dg->dg_Object,dg->dg_Gadget,words(cx-gad->LeftEdge,cy-gad->TopEdge));
        		if (rc & GMR_FINAL)
          			sdg->dg_Flags |= DGF_FINAL;
        		if (!(rc & GMR_UPDATE))
          			break;

        		UpdateDragObj(Self, gdo,-9999,0);

        		if (!DoRenderMethod(Self, sdg,cx,cy,GRENDER_INTERIM))
          			IGraphics->ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);

        		DoRenderMethod(Self,sdg,cx,cy,GRENDER_HIGHLIGHT);
        		return(TRUE);
      			}
    	}
  	}
  	return(FALSE);
}


void HighlightDragGadget(struct GtdragIFace *Self, struct DragGadget *adg,WORD x,WORD y)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
  	struct Gadget *gad;

  	if (adg != hdg && hdg)
  	{
    	gdo->do_Mask = gdo->do_FullShadow;
    	if (hbm)
    	{
      		UpdateDragObj(Self, gdo,-9999,0);

      		if (hdg->dg_Type != BOOPSI_KIND || !DoRenderMethod(Self, hdg,0,0,GRENDER_DELETE))
        		IGraphics->ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);

      		IGraphics->FreeBitMap(hbm);
    	}
    	hdg = NULL;
  	}
  	if (adg)
  	{

    	gdo->do_Mask = gdo->do_HalfShadow;
    	hdg = adg;
//  	hsrp = *hdg->dg_Window->RPort; //Link-Fehler INewlib
    	IExec->CopyMem(hdg->dg_Window->RPort, &hsrp, sizeof(struct RastPort));
    	gad = hdg->dg_Gadget;
    	hx = gad->LeftEdge;  hy = gad->TopEdge;
    	hw = gad->Width;  hh = gad->Height;
    	if (hdg->dg_Type == LISTVIEW_KIND)
    	{
      		hw -= 16;
      		IGadTools->GT_GetGadgetAttrs(hdg->dg_Gadget,hdg->dg_Window,NULL,GTLV_Labels,&hdg->dg_List,TAG_END);
    	}
    	if ((hbm = IGraphics->AllocBitMap(hw,hh,hsrp.BitMap->Depth,BMF_CLEAR | BMF_MINPLANES,hsrp.BitMap)) != 0)
    	{
      		IGraphics->InitRastPort(&hrp);
      		hrp.BitMap = hbm;
      		UpdateDragObj(Self, gdo,-9999,0);
      		IGraphics->ClipBlit(&hsrp,hx,hy,&hrp,0,0,hw,hh,0xc0);
      		hsrp.linpatcnt = 15;  hsrp.Flags |= FRST_DOT;
      		hsrp.LinePtrn = 0x0f0f;
      		if (hdg->dg_Type == LISTVIEW_KIND && !(hdg->dg_Flags & DGF_NOPOS) && hdg->dg_List && !IsListEmpty(hdg->dg_List))
      		{
        		dm.dm_TargetEntry = -1;
        		HighlightLVDragGadget(Self, hdg,IDCMP_MOUSEMOVE,y);
        		return;
      		}
      		if (hdg->dg_Type == BOOPSI_KIND && DoRenderMethod(Self, hdg,x,y,GRENDER_HIGHLIGHT))
      		{
        		hdg->dg_Flags |= DGF_USERENDERING;
        		return;
      		}
      		IGraphics->SetABPenDrMd(&hsrp,1,2,JAM2);
      		IGraphics->Move(&hsrp,hx,hy);
      		IGraphics->Draw(&hsrp,hx+hw-1,hy);
      		IGraphics->Draw(&hsrp,hx+hw-1,hy+hh-1);
      		IGraphics->Draw(&hsrp,hx,hy+hh-1);
      		IGraphics->Draw(&hsrp,hx,hy);
      		IGraphics->SetABPenDrMd(&hsrp,0,0,JAM2);
      		IGraphics->Move(&hsrp,hx+1,hy+1);
      		IGraphics->Draw(&hsrp,hx+1,hy+hh-2);
      		IGraphics->Move(&hsrp,hx+hw-2,hy+1);
      		IGraphics->Draw(&hsrp,hx+hw-2,hy+hh-2);
    	}
  	}
}


void MouseMove(struct GtdragIFace *Self, WORD mousex,WORD mousey)
{
  	if (!adg || !UpdateHighlighting(Self, IDCMP_MOUSEMOVE,mousex,mousey,adg))
  	{
    	long x,y;

    	x = mousex+dg->dg_Window->LeftEdge;
    	y = mousey+dg->dg_Window->TopEdge;

    	if ((adg = GetAcceptorDragGadget(Self, x,y)) != 0)
    	{
      		adg->dg_Flags |= DGF_LIKECURRENT;

      		if (hdg != adg)
        		HighlightDragGadget(Self, adg,x-adg->dg_Window->LeftEdge,y-adg->dg_Window->TopEdge);
    	}
    	else
      		HighlightDragGadget(Self, NULL,0,0);
  	}
  	if (!fakemsg)
    	UpdateDragObj(Self, gdo,mousex,mousey);
}


void SetObjectOwner(struct DragApp *da,struct Task *t,struct ObjectDescription *od)
{
  	if (da->da_Task != t)
    	od->od_Owner = da->da_Name;
  	else
    	od->od_Owner = NULL;
}


void MakeDropMessage(struct GtdragIFace *Self, struct DragApp *da,ULONG qual,WORD mousex,WORD mousey)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;
	struct LayersIFace *ILayers = libBase->ILayers;
	struct ExecIFace *IExec = libBase->IExec;
  	struct IntuiMessage *imsg;
  	struct Window *win;
  	struct Task *task;

  	mx = mousex+dg->dg_Window->LeftEdge;
  	my = mousey+dg->dg_Window->TopEdge;

  	if (adg)
  	{
    	dm.dm_Target = adg->dg_Gadget;
    	task = adg->dg_Task;
    	win = adg->dg_Window;

    	if (adg->dg_Type != LISTVIEW_KIND)
      		dm.dm_TargetEntry = 0;
    	else if (!adg->dg_List || IsListEmpty(adg->dg_List) || (adg->dg_Flags & DGF_NOPOS))
      		dm.dm_TargetEntry = -1;
  	}
  	else
  	{
    	struct DragWindow *dw;

    	win = NULL;
    	for(dw = (APTR)winlist.mlh_Head;dw->dw_Node.mln_Succ;dw = (APTR)dw->dw_Node.mln_Succ)
    	{
      		if (dw->dw_Window->WLayer == ILayers->WhichLayer(&gdo->do_Screen->LayerInfo,mx,my))
      		{
        		if (dw->dw_Task == da->da_Task && !(dw->dw_AcceptMask & dg->dg_Object.od_InternalType))
          			break;
        		if (dw->dw_AcceptFunc && !dw->dw_AcceptFunc(dw->dw_Window,NULL,&dg->dg_Object))
          			break;

        		task = dw->dw_Task;
        		win = dw->dw_Window;
        		break;
      		}
    	}
  	}
  	if (!win)
  	{
    	IIntuition->DisplayBeep(NULL);
    	return;
  	}
  	mx = mx-win->LeftEdge;
  	my = my-win->TopEdge;

  	SetObjectOwner(da,task,&dm.dm_Object);

  	if (adg && adg->dg_Type == BOOPSI_KIND && DoCustomMethod(Self, adg->dg_Gadget,GM_OBJECTDROP,&dm,qual))  // send to boopsi-gadget
    	return;

  	if ((imsg = IExec->AllocVecTags(sizeof(struct ExtIntuiMessage), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE)) != 0)
  	{
    	struct DropMessage *gdm;

    	imsg->ExecMessage.mn_Length = sizeof(struct ExtIntuiMessage);
    	imsg->ExecMessage.mn_ReplyPort = dmport;
    	imsg->Class = IDCMP_OBJECTDROP;
    	imsg->MouseX = mx;
    	imsg->MouseY = my;
    	imsg->Qualifier = qual;
    	IIntuition->CurrentTime(&imsg->Seconds,&imsg->Micros);
    	imsg->IDCMPWindow = win;

    	if ((imsg->IAddress = gdm = IExec->AllocVecTags(sizeof(struct DropMessage), AVT_Type, MEMF_SHARED, TAG_DONE)) != 0)
    	{
      		IExec->CopyMem(&dm,gdm,sizeof(struct DropMessage));
      		IExec->PutMsg(win->UserPort,(struct Message *)imsg);
      		return;
    	}
    	IExec->FreeVec(imsg);
  	}
}


void FreeDragObj(struct GtdragIFace *Self, struct DragObj *gdo)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
  	if (!gdo)
    	return;

  	if (hdg)
   		HighlightDragGadget(Self, NULL,0,0);
  	else
   		UpdateDragObj(Self, gdo,0,-9999);

#ifdef LOCKLAYERS
  	LockLayer(0,gdo->do_DragGadget->dg_Window->RPort->Layer);
  	UnlockLayers(&gdo->do_Screen->LayerInfo);
#endif

  	IGraphics->FreeBitMap(gdo->do_BitMap);
  	IGraphics->FreeBitMap(gdo->do_SaveBack);
  	IGraphics->FreeBitMap(gdo->do_RefreshMap);
  	IExec->FreeVec(gdo->do_FullShadow);
  	IExec->FreeVec(gdo->do_HalfShadow);
  	IExec->FreeVec(gdo);
}


void IntuiTick(struct GtdragIFace *Self, WORD mousex,WORD mousey)
{
  	if (adg && UpdateHighlighting(Self, IDCMP_INTUITICKS,mousex,mousey,adg))
    	UpdateDragObj(Self, gdo,mousex,mousey);
}


struct IntuiMessage * HandleIMsg(struct GtdragIFace *Self, struct DragApp *da,struct IntuiMessage *msg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
	struct ExecIFace *IExec = libBase->IExec;
	struct IntuiMessage *gmsg;
  	int    class,mousex,mousey;

  	if (!da || !msg)
    	return(msg);

  	class = msg->Class;
  	mousex = msg->MouseX;
  	mousey = msg->MouseY;
  	da->da_GTMsg = FALSE;

  	if (class == IDCMP_GADGETDOWN)   // DragGadget suchen
  	{
    	if (!IsListEmpty((struct List *)&gadlist))
    	{
      		foreach(&gadlist,dg)
      		{
        		if (dg->dg_Gadget == msg->IAddress)
          			break;
      		}
      		if (!dg->dg_Node.mln_Succ)
        		for(dg = (APTR)gadlist.mlh_Head;dg->dg_Node.mln_Succ && ((dg->dg_Gadget->GadgetID != ((struct Gadget *)msg->IAddress)->GadgetID) || (dg->dg_Window != msg->IDCMPWindow));dg = (APTR)dg->dg_Node.mln_Succ);

      		if (dg->dg_Node.mln_Succ && dg->dg_Window == msg->IDCMPWindow && (dg->dg_Gadget->GadgetID == ((struct Gadget *)msg->IAddress)->GadgetID) && PointInDragGadget(Self, dg,mousex,mousey,FALSE))
      		{
        		if (dg->dg_Type == LISTVIEW_KIND && !IGadTools->GT_GetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Selected,&dg->dg_Selected,TAG_END))
          			dg->dg_Selected = ~0L;

        		mx = mousex;  my = mousey;
        		PrepareDrag(Self, FALSE);
      		}
      		else
        		dg = NULL;
    	}
    else
     	dg = NULL;
  	}
  	else if (class == IDCMP_GADGETUP && !fakemsg && dg)
    	EndDrag(Self);
  	else if (class == IDCMP_MOUSEMOVE)
  	{
    	if (gdo)
      		MouseMove(Self, mousex,mousey);
    	else if (dg && !(dg->dg_Flags & DGF_NODRAG))
    	{
      	if (mx != ~0L && (msg->Qualifier & IEQUALIFIER_DRAGKEY || (dg->dg_Type == LISTVIEW_KIND && (ABS(mx-mousex) > LVDRAGPIXEL) || dg->dg_Type != LISTVIEW_KIND && ((ABS(my-msg->MouseY) > DRAGPIXEL) || (ABS(mx-mousex) > DRAGPIXEL)))))
        	gdo = CreateDragObj(Self, dg,mousex,mousey);
    	}
  	}
  	else if (gdo)
  	{
    	if (class == IDCMP_MOUSEBUTTONS && !fakemsg)
    	{
      		if (msg->Code == SELECTUP)
        		MakeDropMessage(Self, da,msg->Qualifier,mousex,mousey);

      		FreeDragObj(Self,gdo);
      		EndDrag(Self);
    	}
    	else if (class == IDCMP_INTUITICKS)
      		IntuiTick(Self, mousex,mousey);
  	}
  	else if (class == IDCMP_MOUSEBUTTONS && dg)
    	EndDrag(Self);

  	if ((gmsg = IGadTools->GT_FilterIMsg(msg)) != 0)
  	{
    	da->da_GTMsg = TRUE;
    	msg = gmsg;
    	if (fakemsg && (class == IDCMP_GADGETUP || class == IDCMP_MOUSEBUTTONS))
    	{
      		msg = IGadTools->GT_PostFilterIMsg(msg);
      		IExec->ReplyMsg((struct Message*)msg);
      		fakemsg = FALSE;
      		msg = NULL;
    	}
  	}
  	return(msg);
}


void FreeDropMessage(struct GtdragIFace *Self, struct IntuiMessage *msg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DropMessage *dm;

  	if (!msg || msg->Class != IDCMP_OBJECTDROP)
    	return;

  	if ((dm = msg->IAddress) != 0)
    	IExec->FreeVec(dm);

  	IExec->FreeVec(msg);
}


void FillTreeList(struct GtdragIFace *Self, struct MinList *main,struct MinList *list,UBYTE depth,UBYTE flags,ULONG lines)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct TreeNode *tn;

  	foreach(list,tn)
  	{
    	if (flags & TNF_OPEN)
      		IExec->MyAddTail(main,&tn->tn_ViewNode);
    	else
      		tn->tn_ViewNode.mln_Succ = NULL;

    	tn->tn_Depth = depth;
    	tn->tn_DepthLines = lines;
    	tn->tn_X = -1;
    	if (!IsListEmpty((struct List *)&tn->tn_Nodes))
    	{
      		tn->tn_Flags |= TNF_CONTAINER;

	      	FillTreeList(Self, main,&tn->tn_Nodes,depth+1,flags & TNF_OPEN ? tn->tn_Flags : tn->tn_Flags & ~TNF_OPEN,lines | (tn->tn_Node.in_Succ->in_Succ ? (1L << depth) : 0));
    	}
    	if (!tn->tn_Node.in_Succ->in_Succ)
      		tn->tn_Flags |= TNF_LAST;
    	else
      		tn->tn_Flags &= ~TNF_LAST;
  	}
}

