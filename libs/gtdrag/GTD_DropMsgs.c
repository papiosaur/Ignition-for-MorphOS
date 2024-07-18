/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	DropMsg handling.


#include "gtdrag_includes.h"


LIB_LH3(STRPTR, GTD_GetString, 
  LIB_LHA(struct ObjectDescription *, od, A0),
  LIB_LHA(STRPTR, t, D1),
  LIB_LHA(LONG, len, D0),
  struct Library *, library, 21, Gtdrag)
{
  LIBFUNC_INIT

  if (!od || !od->od_Object || !t || !len)
    return(NULL);

  switch(od->od_Type)
  {
    case ODT_STRING:
      stccpy(t,od->od_Object,len);
      break;
    case ODT_NODE:
    case ODT_IMAGENODE:
      stccpy(t,((struct Node *)od->od_Object)->ln_Name,len);
      break;
    case ODT_TREENODE:
      stccpy(t,TREENODE(od->od_Object)->tn_Node.in_Name,len);
      break;
    case ODT_LOCK:
      if (NameFromLock((BPTR)od->od_Object,t,len))
        break;
    default:
      return(NULL);
  }
  return(t);

  LIBFUNC_EXIT
}


void PRIVATE FreeDropMessage(struct IntuiMessage *msg)
{
  struct DropMessage *dm;

  if (!msg || msg->Class != IDCMP_OBJECTDROP)
    return;

  if ((dm = msg->IAddress) != 0)
    FreeMem(dm,sizeof(struct DropMessage));

  FreeMem(msg,sizeof(struct ExtIntuiMessage));
}


void PRIVATE MakeDropMessage(struct DragApp *da,ULONG qual,WORD mousex,WORD mousey)
{
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
      if (dw->dw_Window->WLayer == WhichLayer(&gdo->do_Screen->LayerInfo,mx,my))
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
    DisplayBeep(NULL);
    return;
  }
  mx = mx-win->LeftEdge;
  my = my-win->TopEdge;

  SetObjectOwner(da,task,&dm.dm_Object);

  if (adg && adg->dg_Type == BOOPSI_KIND && DoCustomMethod(adg->dg_Gadget,GM_OBJECTDROP,&dm,qual))  // send to boopsi-gadget
    return;

  if ((imsg = AllocMem(sizeof(struct ExtIntuiMessage),MEMF_CLEAR | MEMF_PUBLIC)) != 0)
  {
    struct DropMessage *gdm;

    imsg->ExecMessage.mn_Length = sizeof(struct ExtIntuiMessage);
    imsg->ExecMessage.mn_ReplyPort = dmport;
    imsg->Class = IDCMP_OBJECTDROP;
    imsg->MouseX = mx;
    imsg->MouseY = my;
    imsg->Qualifier = qual;
    CurrentTime(&imsg->Seconds,&imsg->Micros);
    imsg->IDCMPWindow = win;

    if ((imsg->IAddress = gdm = AllocMem(sizeof(struct DropMessage),MEMF_PUBLIC)) != 0)
    {
      CopyMem(&dm,gdm,sizeof(struct DropMessage));
      PutMsg(win->UserPort,(struct Message *)imsg);
      return;
    }
    FreeMem(imsg,sizeof(struct ExtIntuiMessage));
  }
}
