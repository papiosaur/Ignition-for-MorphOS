/* :ts=4
 *  $VER: GTD_SetAttrsA.c $Revision$ (20-Sep-2013)
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
#include <proto/utility.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

/****** gtdrag/main/GTD_SetAttrsA ******************************************
*
*   NAME
*      GTD_SetAttrsA -- Description
*      GTD_SetAttrs -- Vararg stub
*
*   SYNOPSIS
*      void GTD_SetAttrsA(APTR gad, struct TagItem * tags);
*      void GTD_SetAttrs(APTR gad, ...);
*
*   FUNCTION
*
*   INPUTS
*       gad - 
*       tags - 
*
*   RESULT
*       This function does not return a result
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

void VARARGS68K _Gtdrag_GTD_SetAttrs(struct GtdragIFace *Self, APTR gad, ...);

void _Gtdrag_GTD_SetAttrs(struct GtdragIFace *Self, APTR gad, ...)
{
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, gad);
    tags = va_getlinearva(ap, struct TagItem *);

    Self->GTD_SetAttrsA(gad, tags);
}

void _Gtdrag_GTD_SetAttrsA(struct GtdragIFace *Self, APTR gad, struct TagItem * tags)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
	struct ExecIFace *IExec = libBase->IExec;
  	struct TagItem *tstate,*ti;
  	struct DragGadget *dg;

  	if (!(dg = FindDragGadget(gad)))
  	{
    	SetWindowAttrs(Self, gad,tags);
    	return;
  	}
	tstate = (struct TagItem *)tags;
  	while((ti = IUtility->NextTagItem(&tstate)) != 0)
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
          			IExec->CopyMem((APTR)ti->ti_Data,&dg->dg_Object,sizeof(struct ObjectDescription));
        		else
          			IUtility->ClearMem(&dg->dg_Object,sizeof(struct ObjectDescription));
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
}

