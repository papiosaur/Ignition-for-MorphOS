/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	IntuiMessage handling.


#include "gtdrag_includes.h"


struct IntuiMessage * PRIVATE HandleIMsg(struct DragApp *da,struct IntuiMessage *msg)
{
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

      if (dg->dg_Node.mln_Succ && dg->dg_Window == msg->IDCMPWindow && (dg->dg_Gadget->GadgetID == ((struct Gadget *)msg->IAddress)->GadgetID) && PointInDragGadget(dg,mousex,mousey,FALSE))
      {
        if (dg->dg_Type == LISTVIEW_KIND && !GT_GetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Selected,&dg->dg_Selected,TAG_END))
          dg->dg_Selected = ~0L;

        mx = mousex;  my = mousey;
        PrepareDrag(FALSE);
      }
      else
        dg = NULL;
    }
    else
      dg = NULL;
  }
  else if (class == IDCMP_GADGETUP && !fakemsg && dg)
    EndDrag();
  else if (class == IDCMP_MOUSEMOVE)
  {
    if (gdo)
      MouseMove(mousex,mousey);
    else if (dg && !(dg->dg_Flags & DGF_NODRAG))
    {
      if (mx != ~0L && (msg->Qualifier & IEQUALIFIER_DRAGKEY || (dg->dg_Type == LISTVIEW_KIND && (abs(mx-mousex) > LVDRAGPIXEL) || dg->dg_Type != LISTVIEW_KIND && ((abs(my-msg->MouseY) > DRAGPIXEL) || (abs(mx-mousex) > DRAGPIXEL)))))
        gdo = CreateDragObj(dg,mousex,mousey);
    }
  }
  else if (gdo)
  {
    if (class == IDCMP_MOUSEBUTTONS && !fakemsg)
    {
      if (msg->Code == SELECTUP)
        MakeDropMessage(da,msg->Qualifier,mousex,mousey);

      FreeDragObj(gdo);
      EndDrag();
    }
    else if (class == IDCMP_INTUITICKS)
      IntuiTick(mousex,mousey);
  }
  else if (class == IDCMP_MOUSEBUTTONS && dg)
    EndDrag();

  if ((gmsg = GT_FilterIMsg(msg)) != 0)
  {
    da->da_GTMsg = TRUE;
    msg = gmsg;
    if (fakemsg && (class == IDCMP_GADGETUP || class == IDCMP_MOUSEBUTTONS))
    {
      msg = GT_PostFilterIMsg(msg);
      ReplyMsg((struct Message*)msg);
      fakemsg = FALSE;
      msg = NULL;
    }
  }
  return(msg);
}


LIB_LH1(struct IntuiMessage *, GTD_GetIMsg, 
  LIB_LHA(struct MsgPort *, mp, A0),
  struct Library *, library, 5, Gtdrag)
{
  LIBFUNC_INIT

  struct IntuiMessage *msg;
  struct DragApp *da;

  if ((da = GetDragApp(NULL)) != 0)
  {
    while((msg = (struct IntuiMessage *)GetMsg(mp)) != 0)
    {
      msg = HandleIMsg(da,msg);
      if (!da->da_GTMsg)
        ReplyMsg((struct Message *)msg);
      else
        return(msg);
    }
  }
  else
    return(GT_GetIMsg(mp));

  return(NULL);

  LIBFUNC_EXIT
}


LIB_LH1(struct IntuiMessage *, GTD_FilterIMsg, 
  LIB_LHA(struct IntuiMessage *, msg, A0),
  struct Library *, library, 7, Gtdrag)
{
  LIBFUNC_INIT

  struct DragApp *da;

  if ((da = GetDragApp(NULL)) != 0)
    return(HandleIMsg(da,msg));

  return(GT_FilterIMsg(msg));

  LIBFUNC_EXIT
}


LIB_LH1(void, GTD_ReplyIMsg, 
  LIB_LHA(struct IntuiMessage *, msg, A0),
  struct Library *, library, 6, Gtdrag)
{
  LIBFUNC_INIT

  struct DragApp *da;

  if ((da = GetDragApp(NULL)) != 0)
  {
    if (da->da_GTMsg)
      msg = GT_PostFilterIMsg(msg);

    ReplyMsg((struct Message *)msg);
  }
  else
    GT_ReplyIMsg(msg);

  if ((msg = (APTR)GetMsg(dmport)) != 0)
    FreeDropMessage(msg);

  LIBFUNC_EXIT
}


LIB_LH1(struct IntuiMessage *, GTD_PostFilterIMsg, 
  LIB_LHA(struct IntuiMessage *, msg, A0),
  struct Library *, library, 8, Gtdrag)
{
  LIBFUNC_INIT

  struct DragApp *da;

  if (!(da = GetDragApp(NULL)) || da->da_GTMsg)
    return(GT_PostFilterIMsg(msg));

  return(msg);

  LIBFUNC_EXIT
}
