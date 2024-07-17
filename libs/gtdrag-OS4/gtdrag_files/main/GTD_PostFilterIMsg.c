/* :ts=4
 *  $VER: GTD_PostFilterIMsg.c $Revision$ (20-Sep-2013)
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
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

/****** gtdrag/main/GTD_PostFilterIMsg ******************************************
*
*   NAME
*      GTD_PostFilterIMsg -- Description
*
*   SYNOPSIS
*      struct IntuiMessage * GTD_PostFilterIMsg(struct IntuiMessage * msg);
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

struct IntuiMessage * _Gtdrag_GTD_PostFilterIMsg(struct GtdragIFace *Self, struct IntuiMessage * msg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
  	struct DragApp *da;

  	if (!(da = GetDragApp(Self, NULL)) || da->da_GTMsg)
    	return(IGadTools->GT_PostFilterIMsg(msg));

  	return(msg);
}

