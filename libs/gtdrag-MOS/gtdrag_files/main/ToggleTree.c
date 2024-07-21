/* :ts=4
 *  $VER: ToggleTree.c $Revision$ (20-Sep-2013)
 *
 *  This file is part of gtdrag.
 *
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 *
 * $Id$
 *
 * $Log$
 *
 *
 */


#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"


/****** gtdrag/main/ToggleTree ******************************************
*
*   NAME
*      ToggleTree -- Description
*
*   SYNOPSIS
*      BOOL ToggleTree(struct Gadget * gad, struct TreeNode * treenode,
*          struct IntuiMessage * msg);
*
*   FUNCTION
*
*   INPUTS
*       gad -
*       treenode -
*       msg -
*
*   RESULT
*       The result ...
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*/

BOOL _Gtdrag_ToggleTree(struct GtdragIFace *Self, struct Gadget * gad, struct TreeNode * tn, struct IntuiMessage * msg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
  	struct DragGadget *dg;
  	struct List *list;
  	long   top,h;
   	struct Window *win = NULL;

  	if (!gad || !tn || !msg || !(dg = FindDragGadget(gad)))
		return FALSE;

   	win = dg->dg_Window;
	if(!win)
		return FALSE;

  	if (IGadTools->GT_GetGadgetAttrs(gad,dg->dg_Window,NULL,GTLV_Labels,&list,GTLV_Top,&top,TAG_END) < 2)
		return FALSE;

  	h = gad->TopEdge+2+(msg->Code-top)*dg->dg_ItemHeight;
  	if (MouseOverTreeKnob(tn,h,msg))
  	{
    	long in,height = (gad->Height-4)/dg->dg_ItemHeight;
		
    	IGadTools->GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,~0L,TAG_END);
    	if ((in = Self->ToggleTreeNode((struct MinList *)list,tn)) && msg->Code+1+in > top+height)
      		top += msg->Code+in+1-top-height;
    	IGadTools->GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,list,GTLV_Top,top,TAG_END);
		return TRUE;
  	}
	return FALSE;
}

