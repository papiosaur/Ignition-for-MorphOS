/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Process the actual dragging.


#include "gtdrag_includes.h"


ULONG rmbtrap;


void PRIVATE EndDrag(void)
{
  struct DragGadget *sdg,*ndg;

  if (noreport)
    ReportMouse(FALSE,dg->dg_Window);
  noreport = FALSE;
  dg->dg_Window->Flags = (dg->dg_Window->Flags & ~WFLG_RMBTRAP) | rmbtrap;

  if (dg->dg_Type == LISTVIEW_KIND)
    GT_SetGadgetAttrs(dg->dg_Gadget,dg->dg_Window,NULL,GTLV_Selected,dg->dg_Selected,TAG_END);

  adg = dg = NULL;  gdo = NULL;  boopsigad = NULL;
  mx = ~0L;

  for(sdg = (APTR)gadlist.mlh_Head;(ndg = (APTR)sdg->dg_Node.mln_Succ) != 0;sdg = ndg)
  {
    if (sdg->dg_Flags & DGF_ONTHEFLY)     // remove temporary BOOPSI-gadgets
    {
      Remove((APTR)sdg);
      FreeDragGadget(sdg);
    }
    else if (sdg->dg_Type == BOOPSI_KIND) // reset constant BOOPSI-gadgets
      sdg->dg_Flags &= ~(DGF_USERENDERING | DGF_FINAL);

    sdg->dg_CurrentObject = NULL;
  }
}


void PRIVATE PrepareDrag(BOOL boopsi)
{
  rmbtrap = dg->dg_Window->Flags & WFLG_RMBTRAP;

  if (boopsi)
    return;

  if (!(dg->dg_Window->Flags & WFLG_REPORTMOUSE))
  {
    ReportMouse(TRUE,dg->dg_Window);
    noreport = TRUE;
  }
  dg->dg_Window->Flags |= WFLG_RMBTRAP;
}


void PRIVATE MouseMove(WORD mousex,WORD mousey)
{
  if (!adg || !UpdateHighlighting(IDCMP_MOUSEMOVE,mousex,mousey,adg))
  {
    long x,y;

    x = mousex+dg->dg_Window->LeftEdge;
    y = mousey+dg->dg_Window->TopEdge;

    if ((adg = GetAcceptorDragGadget(x,y)) != 0)
    {
      adg->dg_Flags |= DGF_LIKECURRENT;

      if (hdg != adg)
        HighlightDragGadget(adg,x-adg->dg_Window->LeftEdge,y-adg->dg_Window->TopEdge);
    }
    else
      HighlightDragGadget(NULL,0,0);
  }
  if (!fakemsg)
    UpdateDragObj(gdo,mousex,mousey);
}


void PRIVATE IntuiTick(WORD mousex,WORD mousey)
{
  if (adg && UpdateHighlighting(IDCMP_INTUITICKS,mousex,mousey,adg))
    UpdateDragObj(gdo,mousex,mousey);
}
