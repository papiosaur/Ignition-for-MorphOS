/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	DragObj functions.


#include "gtdrag_includes.h"

#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>

#if defined(__SASC)
#	include <pragmas/cybergraphics_pragmas.h>
#endif

#define LOCKLAYERS


void PRIVATE FreeDragObj(struct DragObj *gdo)
{
  if (!gdo)
    return;

  if (hdg)
    HighlightDragGadget(NULL,0,0);
  else
    UpdateDragObj(gdo,0,-9999);

#ifdef LOCKLAYERS
  LockLayer(0,gdo->do_DragGadget->dg_Window->RPort->Layer);
  UnlockLayers(&gdo->do_Screen->LayerInfo);
#endif

  FreeBitMap(gdo->do_BitMap);
  FreeBitMap(gdo->do_SaveBack);
  FreeBitMap(gdo->do_RefreshMap);
  FreeMem(gdo->do_FullShadow,((gdo->do_Width+15) >> 4)*2*gdo->do_Height);
  FreeMem(gdo->do_HalfShadow,((gdo->do_Width+15) >> 4)*2*gdo->do_Height);
  FreeMem(gdo,sizeof(struct DragObj));
}


struct DragObj * PRIVATE CreateDragObj(struct DragGadget *dg,int x,int y)
{
  struct Screen *scr;
  struct RastPort *rp;
  struct DragObj *gdo;
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
  memset(&dm,0,sizeof(struct DropMessage));

  if (dg->dg_Type == LISTVIEW_KIND)
  {
    xpos = dg->dg_Gadget->LeftEdge+2;
    ypos = dg->dg_Gadget->TopEdge+2;
    dg->dg_Object.od_Object = NULL;

    if (y < ypos || y > ypos+dg->dg_Gadget->Height-5)
      return(NULL);
    line = (y-ypos)/dg->dg_ItemHeight;
    ypos += line*dg->dg_ItemHeight;

    GT_GetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Labels,&dg->dg_List,TAG_END);
    if (dg->dg_List && !IsListEmpty(dg->dg_List))
    {
      GT_GetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Top,&i,TAG_END);
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
  depth = GetBitMapAttr(rp->BitMap,BMA_DEPTH);

  if (dg->dg_Object.od_Object && (gdo = AllocMem(sizeof(struct DragObj), MEMF_CLEAR | MEMF_PUBLIC)))
  {
#ifdef LOCKLAYERS
    LockLayers(&scr->LayerInfo);
    UnlockLayer(dg->dg_Window->RPort->Layer);
#endif

    gdo->do_Screen = scr;
    gdo->do_ScrRPort = rp;

    gdo->do_BitMap = AllocBitMap(width,height,depth,BMF_CLEAR | BMF_MINPLANES,!(GetBitMapAttr( rp->BitMap, BMA_FLAGS ) & BMF_INTERLEAVED) ? rp->BitMap : NULL);
    gdo->do_SaveBack = AllocBitMap(width,height,depth,BMF_CLEAR | BMF_MINPLANES,rp->BitMap);
    gdo->do_RefreshMap = AllocBitMap(width*2,height*2,depth,BMF_CLEAR | BMF_MINPLANES,rp->BitMap);

    if (GetBitMapAttr(gdo->do_BitMap,BMA_FLAGS) & BMF_STANDARD)
      i = MEMF_CHIP | MEMF_PUBLIC;
    else
      i = 0;

    gdo->do_FullShadow = AllocMem(2*wordwidth*height,i | MEMF_CLEAR);
    gdo->do_HalfShadow = AllocMem(2*wordwidth*height,i);

    if (gdo->do_BitMap && gdo->do_SaveBack && gdo->do_RefreshMap && gdo->do_FullShadow && gdo->do_HalfShadow)
    {
      InitRastPort(&gdo->do_RPort);
      gdo->do_RPort.BitMap = gdo->do_BitMap;
      InitRastPort(&gdo->do_RefreshRPort);
      gdo->do_RefreshRPort.BitMap = gdo->do_RefreshMap;

      gdo->do_DragGadget = dg;
      CopyMem(&dg->dg_Object,&dm.dm_Object,sizeof(struct ObjectDescription));
      dm.dm_Window = dg->dg_Window;
      dm.dm_Gadget = dg->dg_Gadget;

      /*** create the drag&drop image ***/

      if (dg->dg_RenderHook)
      {
        struct LVDrawMsg lvdm;

        SetFont(&gdo->do_RPort,scr->RastPort.Font);
        lvdm.lvdm_MethodID = LV_DRAW;
        lvdm.lvdm_RastPort = &gdo->do_RPort;
        lvdm.lvdm_DrawInfo = GetScreenDrawInfo(scr);
        lvdm.lvdm_Bounds.MinX = 0;
        lvdm.lvdm_Bounds.MinY = 0;
        lvdm.lvdm_Bounds.MaxX = width-1;
        lvdm.lvdm_Bounds.MaxY = height-1;
        lvdm.lvdm_State = LVR_SELECTED;
        CallHookPkt(dg->dg_RenderHook,dm.dm_Object.od_Object,&lvdm);
        FreeScreenDrawInfo(scr,lvdm.lvdm_DrawInfo);
      }
      else if (dg->dg_Image)
        DrawImage(&gdo->do_RPort,dg->dg_Image,0,0);
      else
        ClipBlit(dg->dg_Window->RPort,xpos,ypos,&gdo->do_RPort,0,0,width,height,0xc0);

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

      if (CyberGfxBase && (GetBitMapAttr(gdo->do_BitMap,BMA_FLAGS) & BMF_STANDARD) == 0L)
      {
        struct BitMap tbm;
        ULONG  col;

        InitBitMap(&tbm,1,width,height);
        tbm.Planes[0] = (UBYTE *)gdo->do_FullShadow;

        /* if (!GetCyberMapAttr(gdo->do_BitMap, CYBRMATTR_PIXELFMT)) */

        if (GetBitMapAttr(gdo->do_BitMap, BMA_DEPTH) > 8L)
        {
          ULONG triplet[3];

          GetRGB32(scr->ViewPort.ColorMap,0L,1L,triplet);
          col = (triplet[0] & 0xff0000) | (triplet[1] & 0xff00) | (triplet[2] & 0xff);
        }
        else
          col = 0;

        // ExtractColor(rp,&tbm,col,xpos,ypos,width,height);
        ExtractColor(&gdo->do_RPort,&tbm,col,0,0,width,height);

        BltBitMap(&tbm,0,0,&tbm,0,0,width,height,0x50,0xff,NULL);  // invertieren der Maske
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

        CopyMem(gdo->do_FullShadow,p,2*wordwidth*height);
        for(line = 0x5555,ypos = 0;ypos < height;ypos++)
        {
          line = ~line;
          for(xpos = 0;xpos < wordwidth;xpos++,p++)
            *p &= (UWORD)line;
        }
      }

      if (!boopsigad)
        FakeInputEvent();
      UpdateDragObj(gdo,gdo->do_X,gdo->do_Y);    /* show drag object */

      return(gdo);
    }
    FreeBitMap(gdo->do_BitMap);
    FreeBitMap(gdo->do_SaveBack);
    FreeBitMap(gdo->do_RefreshMap);
    FreeMem(gdo,sizeof(struct DragObj));
  }
  return(NULL);
}


void PRIVATE UpdateDragObj(struct DragObj *gdo,int x, int y)
{
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

  rx = min(gdo->do_PX,px);  rw = max(gdo->do_PX+gdo->do_PWidth,px+pw)-rx;
  ry = min(gdo->do_PY,py);  rh = max(gdo->do_PY+gdo->do_PHeight,py+ph)-ry;

  if (px+pw > 0 && py+ph > 0 && (rw > gdo->do_Width*2 || rh > gdo->do_Height*2))
  {
    /******************* Außerhalb der Refresh-BitMap ************************/
    if ((gdo->do_PX+gdo->do_PWidth > 0) && (gdo->do_PY+gdo->do_PHeight > 0))
      BltBitMapRastPort(gdo->do_SaveBack,gdo->do_SX,gdo->do_SY,gdo->do_ScrRPort,gdo->do_PX,gdo->do_PY,gdo->do_PWidth,gdo->do_PHeight,0xc0);
    BltBitMap(gdo->do_ScrRPort->BitMap,px,py,gdo->do_SaveBack,sx,sy,pw,ph,0xc0,0xff,NULL);
    BltMaskBitMapRastPort(gdo->do_BitMap,sx,sy,gdo->do_ScrRPort,px,py,pw,ph,0xe0,(PLANEPTR)gdo->do_Mask);
  }
  else if ((px+pw > 0) && (py+ph > 0))
  {
    /******************* Innerhalb der Refresh-BitMap *********************/
    ClipBlit(gdo->do_ScrRPort,rx,ry,&gdo->do_RefreshRPort,0,0,rw,rh,0xc0);
    if ((gdo->do_PX+gdo->do_PWidth > 0) && (gdo->do_PY+gdo->do_PHeight > 0))
      BltBitMapRastPort(gdo->do_SaveBack,gdo->do_SX,gdo->do_SY,&gdo->do_RefreshRPort,gdo->do_PX-rx,gdo->do_PY-ry,gdo->do_PWidth,gdo->do_PHeight,0xc0);
    BltBitMap(gdo->do_RefreshMap,px-rx,py-ry,gdo->do_SaveBack,sx,sy,pw,ph,0xc0,0xff,NULL);
    BltMaskBitMapRastPort(gdo->do_BitMap,sx,sy,&gdo->do_RefreshRPort,px-rx,py-ry,pw,ph,0xe0,(PLANEPTR)gdo->do_Mask);
    ClipBlit(&gdo->do_RefreshRPort,0,0,gdo->do_ScrRPort,rx,ry,rw,rh,0xc0);
  }
  else if ((gdo->do_PX+gdo->do_PWidth > 0) && (gdo->do_PY+gdo->do_PHeight > 0))
  {
    /******************* Nur SaveBack zurück ********************/
    BltBitMapRastPort(gdo->do_SaveBack,gdo->do_SX,gdo->do_SY,gdo->do_ScrRPort,gdo->do_PX,gdo->do_PY,gdo->do_PWidth,gdo->do_PHeight,0xc0);
  }
  gdo->do_SX = sx;  gdo->do_SY = sy;
  gdo->do_PX = px;  gdo->do_PY = py;
  gdo->do_PWidth = pw;  gdo->do_PHeight = ph;
  gdo->do_X = x;
  gdo->do_Y = y;
}
