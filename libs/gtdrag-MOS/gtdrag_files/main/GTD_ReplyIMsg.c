/* :ts=4
 *  $VER: GTD_ReplyIMsg.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_ReplyIMsg ******************************************
*
*   NAME
*      GTD_ReplyIMsg -- Description
*
*   SYNOPSIS
*      void GTD_ReplyIMsg(struct IntuiMessage * msg);
*
*   FUNCTION
*
*   INPUTS
*       msg - 
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

void _Gtdrag_GTD_ReplyIMsg(struct GtdragIFace *Self, struct IntuiMessage * msg)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragApp *da;

  	if ((da = GetDragApp(Self, NULL)) != 0)
  	{
    	if (da->da_GTMsg)
      		msg = IGadTools->GT_PostFilterIMsg(msg);
    	IExec->ReplyMsg((struct Message *)msg);
  	}
  	else
    	IGadTools->GT_ReplyIMsg(msg);
  	if ((msg = (APTR)IExec->GetMsg(dmport)) != 0)
    	FreeDropMessage(Self, msg);
}

