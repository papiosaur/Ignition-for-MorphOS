/* :ts=4
 *  $VER: GTD_GetAttr.c $Revision$ (20-Sep-2013)
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
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

/****** gtdrag/main/GTD_GetAttr ******************************************
*
*   NAME
*      GTD_GetAttr -- Description
*
*   SYNOPSIS
*      BOOL GTD_GetAttr(APTR gad, ULONG d0arg, ULONG * storage);
*
*   FUNCTION
*
*   INPUTS
*       gad - 
*       d0arg - 
*       storage - 
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
BOOL _Gtdrag_GTD_GetAttr(struct GtdragIFace *Self, APTR gad, ULONG tag, ULONG *storage)
{
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
}

