/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Gadget functions.


#include "gtdrag_includes.h"


#define IsCustomGadget(gad) ((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET && gad->MutualExclude)


void PRIVATE FreeDragGadget(struct DragGadget *dg)
{
  if (!dg)
    return;

  FreeMem(dg,sizeof(struct DragGadget));
}


BOOL PRIVATE DragGadgetAcceptsObject(struct DragGadget *adg,WORD x,WORD y)
{
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

    if (GT_GetGadgetAttrs(adg->dg_Gadget,adg->dg_Window,NULL,GA_Disabled,&store,TAG_END) && store)
      return(FALSE);
  }
  if (adg->dg_Gadget->Flags & GFLG_DISABLED)
    return(FALSE);

  if (dg == adg && !(dg->dg_Flags & DGF_SAME))
    return(FALSE);

  ada = GetDragApp(adg->dg_Task);
  if (dg->dg_Task != adg->dg_Task && (ada && ada->da_Flags & DAF_INTERN || adg->dg_Flags & DGF_INTERNALONLY))
    return(FALSE);

  if (dg->dg_Task == adg->dg_Task && !(adg->dg_AcceptMask & dg->dg_Object.od_InternalType))
    return(FALSE);


  if (adg->dg_AcceptFunc)
    return(adg->dg_AcceptFunc(adg->dg_Window,adg->dg_Gadget,&dg->dg_Object));

  {
    struct Gadget *gad;

    if (adg->dg_Type == BOOPSI_KIND && DoCustomMethod(gad = adg->dg_Gadget,GM_OBJECTDRAG,&dg->dg_Object,dg->dg_Gadget,words(x-gad->LeftEdge,y-gad->TopEdge)) & GMR_REJECTOBJECT)
      return(FALSE);
  }

  return(TRUE);
}


struct DragGadget * PRIVATE WhichDragGadget(APTR layer,int mx,int my)
{
  struct DragGadget *dg;

  ObtainSemaphore(&ListSemaphore);
  foreach(&gadlist,dg)
  {
    if (dg->dg_Window->WLayer == layer)
    {
      if (PointInDragGadget(dg,mx-dg->dg_Window->LeftEdge,my-dg->dg_Window->TopEdge,FALSE))
      {
        ReleaseSemaphore(&ListSemaphore);
        return(dg);
      }
    }
  }
  ReleaseSemaphore(&ListSemaphore);
  return(NULL);
}


BOOL PRIVATE PointInGadget(struct Gadget *gad,WORD x,WORD y)
{
  return((BOOL)(x >= gad->LeftEdge && x <= gad->LeftEdge+gad->Width && y >= gad->TopEdge && y <= gad->TopEdge+gad->Height));
}


struct DragGadget * PRIVATE GetAcceptorDragGadget(int mx,int my)
{
  struct DragGadget *sdg;
  APTR   layer = WhichLayer(&gdo->do_Screen->LayerInfo,mx,my);

  if ((sdg = WhichDragGadget(layer,mx,my)) != 0)
    return(DragGadgetAcceptsObject(sdg,mx-sdg->dg_Window->LeftEdge,my-sdg->dg_Window->TopEdge) ? sdg : NULL);

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
          if (IsCustomGadget(gad) && PointInGadget(gad,x,y))
          {
            long rc;

            if ((rc = DoCustomMethod(gad,GM_OBJECTDRAG,&dg->dg_Object,dg->dg_Gadget,words(x-gad->LeftEdge,y-gad->TopEdge))) != 0)
            {
              if ((sdg = AddDragGadget(gad,win,BOOPSI_KIND)) != 0)
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


BOOL PRIVATE PointInDragGadget(struct DragGadget *dg,int x,int y,BOOL lheight)
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


LIB_LH1(void, GTD_RemoveGadgets, 
  LIB_LHA(struct Window *, win, A0),
  struct Library *, library, 17, Gtdrag)
{
  LIBFUNC_INIT

  struct DragGadget *dg,*ndg;

  ObtainSemaphore(&ListSemaphore);

  for(dg = (APTR)gadlist.mlh_Head;dg->dg_Node.mln_Succ;dg = ndg)
  {
    ndg = (APTR)dg->dg_Node.mln_Succ;

    if (dg->dg_Window == win)
    {
      Remove((struct Node *)dg);
      FreeDragGadget(dg);
    }
  }
  ReleaseSemaphore(&ListSemaphore);

  LIBFUNC_EXIT
}


struct DragGadget * PRIVATE FindDragGadget(struct Gadget *gad)
{
  struct DragGadget *dg;

  foreach(&gadlist,dg)
  {
    if (dg->dg_Gadget == gad)
      return(dg);
  }
  return(NULL);
}


LIB_LH1(void, GTD_RemoveGadget, 
  LIB_LHA(struct Gadget *, gad, A0),
  struct Library *, library, 16, Gtdrag)
{
  LIBFUNC_INIT

  struct DragGadget *dg;

  ObtainSemaphore(&ListSemaphore);

  if ((dg = FindDragGadget(gad)) != 0)
  {
    Remove((struct Node *)dg);
    FreeMem(dg,sizeof(struct DragGadget));
  }
  ReleaseSemaphore(&ListSemaphore);

  LIBFUNC_EXIT
}


LIB_LH3(BOOL, GTD_GetAttr, 
  LIB_LHA(APTR, gad, A0),
  LIB_LHA(ULONG, tag, D0),
  LIB_LHA(ULONG *, storage, A1),
  struct Library *, library, 19, Gtdrag)
{
  LIBFUNC_INIT

  struct DragGadget *dg;

  if (!storage || !(dg = FindDragGadget(gad)))
    return(FALSE);

  switch(tag)
  {
    case GTDA_Image:
      *storage = (ULONG)dg->dg_Image;
      return(TRUE);
    case GTDA_RenderHook:
      *storage = (ULONG)dg->dg_RenderHook;
      return(TRUE);
    case GTDA_Width:
      *storage = (ULONG)dg->dg_Width;
      return(TRUE);
    case GTDA_Height:
      *storage = (ULONG)dg->dg_Height;
      return(TRUE);
    case GTDA_Object:
      *storage = (ULONG)dg->dg_Object.od_Object;
      return(TRUE);
    case GTDA_GroupID:
      *storage = (ULONG)dg->dg_Object.od_GroupID;
      return(TRUE);
    case GTDA_Type:
      *storage = (ULONG)dg->dg_Object.od_Type;
      return(TRUE);
    case GTDA_InternalType:
      *storage = (ULONG)dg->dg_Object.od_InternalType;
      return(TRUE);
    case GTDA_ObjectDescription:
      *storage = (ULONG)&dg->dg_Object;
      return(TRUE);
  }
  return(FALSE);

  LIBFUNC_EXIT
}


LIB_LH2(void, GTD_SetAttrsA, 
  LIB_LHA(APTR, gad, A0),
  LIB_LHA(struct TagItem *, tags, A1),
  struct Library *, library, 18, Gtdrag)
{
  LIBFUNC_INIT

  struct TagItem *tstate,*ti;
  struct DragGadget *dg;

  if (!(dg = FindDragGadget(gad)))
  {
    SetWindowAttrs(gad,tags);
    return;
  }

  tstate = (struct TagItem *)tags;
  while((ti = NextTagItem(&tstate)) != 0)
  {
    switch(ti->ti_Tag)
    {
      case GTDA_Images:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_IMAGES) | (ti->ti_Data ? DGF_IMAGES : 0);
        break;
      case GTDA_ItemHeight:
        dg->dg_ItemHeight = ti->ti_Data;
        break;
      case GTDA_NoDrag:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_NODRAG) | (ti->ti_Data ? DGF_NODRAG : 0);
        break;
      case GTDA_Same:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_SAME) | (ti->ti_Data ? DGF_SAME : 0);
        break;
      case GTDA_NoPosition:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_NOPOS) | (ti->ti_Data ? DGF_NOPOS : 0);
        break;
      case GTDA_NoScrolling:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_NOSCROLL) | (ti->ti_Data ? DGF_NOSCROLL : 0);
        break;
      case GTDA_DropOverItems:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_DROPOVER) | (ti->ti_Data ? DGF_DROPOVER : 0);
        break;
      case GTDA_DropBetweenItems:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_DROPBETWEEN) | (ti->ti_Data ? DGF_DROPBETWEEN : 0);
        break;
      case GTDA_TreeView:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_TREEVIEW) | (ti->ti_Data ? DGF_TREEVIEW : 0);
        break;
      case GTDA_Width:
        dg->dg_Width = ti->ti_Data;
        break;
      case GTDA_Height:
        dg->dg_Height = ti->ti_Data;
        break;
      case GTDA_AcceptMask:
        dg->dg_AcceptMask = ti->ti_Data;
        break;
      case GTDA_AcceptFunc:
        dg->dg_AcceptFunc = (APTR)ti->ti_Data;
        break;
      case GTDA_ObjectFunc:
        dg->dg_ObjectFunc = (APTR)ti->ti_Data;
        break;
      case GTDA_Object:
        dg->dg_Object.od_Object = (APTR)ti->ti_Data;
        break;
      case GTDA_GroupID:
        dg->dg_Object.od_GroupID = ti->ti_Data;
        break;
      case GTDA_Type:
        dg->dg_Object.od_Type = ti->ti_Data;
        break;
      case GTDA_InternalType:
        dg->dg_Object.od_InternalType = ti->ti_Data;
        break;
      case GTDA_Image:
        dg->dg_Image = (APTR)ti->ti_Data;
        break;
      case GTDA_RenderHook:
        dg->dg_RenderHook = (APTR)ti->ti_Data;
        break;
      case GTDA_ObjectDescription:
        if (ti->ti_Data)
          CopyMem((APTR)ti->ti_Data,&dg->dg_Object,sizeof(struct ObjectDescription));
        else
          memset(&dg->dg_Object,0,sizeof(struct ObjectDescription));
        break;
      case GTDA_SourceEntry:
        dg->dg_SourceEntry = ti->ti_Data;
        break;
      case GTDA_InternalOnly:
        dg->dg_Flags = (dg->dg_Flags & ~DGF_INTERNALONLY) | (ti->ti_Data ? DGF_INTERNALONLY : 0);
        break;
    }
  }

  if (dg->dg_Type != LISTVIEW_KIND && !dg->dg_Image && !dg->dg_RenderHook)
  {
    if (dg->dg_Object.od_Type == ODT_UNKNOWN && dg->dg_Object.od_Object)
      dg->dg_Object.od_Type = ODT_IMAGE;
    if (dg->dg_Object.od_Type == ODT_IMAGE && dg->dg_Object.od_Object)
      dg->dg_Image = dg->dg_Object.od_Object;
    else
      dg->dg_Flags |= DGF_NODRAG;
  }
  if (dg->dg_Flags & DGF_TREEVIEW && dg->dg_Object.od_Type == ODT_UNKNOWN)
    dg->dg_Object.od_Type = ODT_TREENODE;

  LIBFUNC_EXIT
}


struct DragGadget * PRIVATE AddDragGadget(struct Gadget *gad,struct Window *win,ULONG type)
{
  struct DragGadget *dg = NULL;
  struct DragApp *da;

  ObtainSemaphore(&ListSemaphore);
  if ((da = GetDragApp(NULL)) && gad && win && (dg = AllocMem(sizeof(struct DragGadget),MEMF_PUBLIC | MEMF_CLEAR)))
  {
    dg->dg_Gadget = gad;
    dg->dg_Window = win;
    dg->dg_Task = da->da_Task;
    dg->dg_Type = type;
    dg->dg_AcceptMask = 0xffffffff;
    dg->dg_Object.od_InternalType = 0xffffffff;
    dg->dg_Object.od_Owner = da->da_Name;
    AddTail((struct List *)&gadlist,(APTR)dg);
  }
  ReleaseSemaphore(&ListSemaphore);

  return(dg);
}


LIB_LH4(void, GTD_AddGadgetA, 
  LIB_LHA(ULONG, type, D0),
  LIB_LHA(struct Gadget *, gad, A0),
  LIB_LHA(struct Window *, win, A1),
  LIB_LHA(struct TagItem *, tags, A2),
  struct Library *, library, 15, Gtdrag)
{
  LIBFUNC_INIT

  struct DragGadget *dg;

  if ((dg = AddDragGadget(gad,win,type)) != 0)
  {
    GTD_SetAttrsA(gad,tags);

    if (!dg->dg_ItemHeight)
      dg->dg_ItemHeight = win->WScreen->Font->ta_YSize;
  }
  
  LIBFUNC_EXIT
}
