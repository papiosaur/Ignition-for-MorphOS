/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Highligthing functions.


#include "gtdrag_includes.h"


BOOL PRIVATE UpdateHighlighting(ULONG class,WORD mousex,WORD mousey,struct DragGadget *sdg)
{
  long cx,cy;

  if (!sdg)
    return(FALSE);

  cx = mousex+dg->dg_Window->LeftEdge-sdg->dg_Window->LeftEdge;
  cy = mousey+dg->dg_Window->TopEdge-sdg->dg_Window->TopEdge;

  if (PointInDragGadget(sdg,cx,cy,TRUE))
  {
    switch(sdg->dg_Type)
    {
      case LISTVIEW_KIND:
        return(HighlightLVDragGadget(sdg,class,cy));
      case BOOPSI_KIND:
      {
        struct Gadget *gad = sdg->dg_Gadget;
        ULONG  rc;

        if (sdg->dg_Flags & DGF_FINAL || !(sdg->dg_Flags & DGF_USERENDERING))
          break;

        rc = DoCustomMethod(gad,GM_OBJECTDRAG,&dg->dg_Object,dg->dg_Gadget,words(cx-gad->LeftEdge,cy-gad->TopEdge));
        if (rc & GMR_FINAL)
          sdg->dg_Flags |= DGF_FINAL;
        if (!(rc & GMR_UPDATE))
          break;

        UpdateDragObj(gdo,-9999,0);

        if (!DoRenderMethod(sdg,cx,cy,GRENDER_INTERIM))
          ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);

        DoRenderMethod(sdg,cx,cy,GRENDER_HIGHLIGHT);
        return(TRUE);
      }
    }
  }
  return(FALSE);
}


BOOL PRIVATE HighlightLVDragGadget(struct DragGadget *adg,long class,short y)
{
  short  line;
  long   pos = 0;

  if (!adg->dg_List || IsListEmpty(adg->dg_List))
    return(FALSE);

  if (class == IDCMP_INTUITICKS && (y <= hy || y >= hy+hh) && !(adg->dg_Flags & DGF_NOSCROLL))  /* Scrolling */
  {
    GT_GetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GTLV_Top,&pos,TAG_END);
    if (y <= hy)
      pos--;
    else
      pos++;
    if (pos >= 0 && pos+((hh-4)/adg->dg_ItemHeight) <= CountNodes(adg->dg_List))
    {
      UpdateDragObj(gdo,-9999,0);
      ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);
      GT_SetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GTLV_Top,pos,TAG_END);
      ClipBlit(&hsrp,hx,hy,&hrp,0,0,hw,hh,0xc0);
      return(TRUE);
    }
  }
  else if (class == IDCMP_MOUSEMOVE && !(adg->dg_Flags & DGF_NOPOS) && y > hy && y < hy+hh)  /* Positioning */
  {
    long num = CountNodes(adg->dg_List);
    struct Node *ln;
    long mode = 0;

    GT_GetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GTLV_Top,&pos,TAG_END);

    line = (y-hy-2)/adg->dg_ItemHeight;
    pos += line;
    ln = FindListNumber(adg->dg_List,pos);

    if (adg->dg_Flags & (DGF_DROPOVER | DGF_TREEVIEW) && ln != dm.dm_Object.od_Object)
    {
      int point = (y-hy-2) % adg->dg_ItemHeight;

      if ((adg->dg_Flags & (DGF_DROPOVER | DGF_DROPBETWEEN)) == DGF_DROPOVER || (adg->dg_Flags & DGF_DROPBETWEEN) && point > 2 && adg->dg_ItemHeight-point > 2)
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
    if (adg->dg_Flags & (DGF_DROPOVER | DGF_DROPBETWEEN) == DGF_DROPOVER && ln == dm.dm_Object.od_Object)
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

      UpdateDragObj(gdo,-9999,0);
      ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);

      SetABPenDrMd(&hsrp,1,2,JAM2);
      Move(&hsrp,hx+2,y);
      Draw(&hsrp,hx+hw-3,y);

      if (mode & DMF_DROPOVER)
      {
        Move(&hsrp,hx+2+hw-3,y+adg->dg_ItemHeight);
        Draw(&hsrp,hx+2,y+adg->dg_ItemHeight);
      }
      return(TRUE);
    }
  }
  return(FALSE);
}


void PRIVATE HighlightDragGadget(struct DragGadget *adg,WORD x,WORD y)
{
  struct Gadget *gad;

  if (adg != hdg && hdg)
  {
    gdo->do_Mask = gdo->do_FullShadow;
    if (hbm)
    {
      UpdateDragObj(gdo,-9999,0);

      if (hdg->dg_Type != BOOPSI_KIND || !DoRenderMethod(hdg,0,0,GRENDER_DELETE))
        ClipBlit(&hrp,0,0,&hsrp,hx,hy,hw,hh,0xc0);

      FreeBitMap(hbm);
    }
    hdg = NULL;
  }
  if (adg)
  {
    gdo->do_Mask = gdo->do_HalfShadow;
    hdg = adg;
    hsrp = *hdg->dg_Window->RPort;
    gad = hdg->dg_Gadget;
    hx = gad->LeftEdge;  hy = gad->TopEdge;
    hw = gad->Width;  hh = gad->Height;
    if (hdg->dg_Type == LISTVIEW_KIND)
    {
      hw -= 16;
      GT_GetGadgetAttrs(hdg->dg_Gadget,hdg->dg_Window,NULL,GTLV_Labels,&hdg->dg_List,TAG_END);
    }
    if ((hbm = AllocBitMap(hw,hh,hsrp.BitMap->Depth,BMF_CLEAR | BMF_MINPLANES,hsrp.BitMap)) != 0)
    {
      InitRastPort(&hrp);
      hrp.BitMap = hbm;
      UpdateDragObj(gdo,-9999,0);
      ClipBlit(&hsrp,hx,hy,&hrp,0,0,hw,hh,0xc0);
      hsrp.linpatcnt = 15;  hsrp.Flags |= FRST_DOT;
      hsrp.LinePtrn = 0x0f0f;
      if (hdg->dg_Type == LISTVIEW_KIND && !(hdg->dg_Flags & DGF_NOPOS) && hdg->dg_List && !IsListEmpty(hdg->dg_List))
      {
        dm.dm_TargetEntry = -1;
        HighlightLVDragGadget(hdg,IDCMP_MOUSEMOVE,y);
        return;
      }
      if (hdg->dg_Type == BOOPSI_KIND && DoRenderMethod(hdg,x,y,GRENDER_HIGHLIGHT))
      {
        hdg->dg_Flags |= DGF_USERENDERING;
        return;
      }
      SetABPenDrMd(&hsrp,1,2,JAM2);
      Move(&hsrp,hx,hy);
      Draw(&hsrp,hx+hw-1,hy);
      Draw(&hsrp,hx+hw-1,hy+hh-1);
      Draw(&hsrp,hx,hy+hh-1);
      Draw(&hsrp,hx,hy);
      SetABPenDrMd(&hsrp,0,0,JAM2);
      Move(&hsrp,hx+1,hy+1);
      Draw(&hsrp,hx+1,hy+hh-2);
      Move(&hsrp,hx+hw-2,hy+1);
      Draw(&hsrp,hx+hw-2,hy+hh-2);
    }
  }
}
