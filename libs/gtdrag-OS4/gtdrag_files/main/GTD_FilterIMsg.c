/* :ts=4
 *  $VER: GTD_FilterIMsg.c $Revision$ (20-Sep-2013)
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


#include <proto/gadtools.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>
#include "../gtdrag_private.h"

/****** gtdrag/main/GTD_FilterIMsg ******************************************
*
*   NAME
*      GTD_FilterIMsg -- Description
*
*   SYNOPSIS
*      struct IntuiMessage * GTD_FilterIMsg(struct IntuiMessage * msg);
*
*   FUNCTION
*
*   INPUTS
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

struct IntuiMessage * _Gtdrag_GTD_FilterIMsg(struct GtdragIFace *Self, struct IntuiMessage * msg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
  	struct DragApp *da;

  	if ((da = GetDragApp(Self,NULL)) != 0)
    	return(HandleIMsg(Self, da,msg));
  	return(IGadTools->GT_FilterIMsg(msg));
}

