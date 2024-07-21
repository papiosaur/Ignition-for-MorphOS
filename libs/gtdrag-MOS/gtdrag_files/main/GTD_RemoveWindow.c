/* :ts=4
 *  $VER: GTD_RemoveWindow.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_RemoveWindow ******************************************
*
*   NAME
*      GTD_RemoveWindow -- Description
*
*   SYNOPSIS
*      void GTD_RemoveWindow(struct Window * win);
*
*   FUNCTION
*
*   INPUTS
*       win - 
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

void _Gtdrag_GTD_RemoveWindow(struct GtdragIFace *Self, struct Window * win)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragWindow *dw;

  	IExec->ObtainSemaphore(&ListSemaphore);
  	foreach(&winlist,dw)
  	{
    	if (dw->dw_Window == win)
    	{
      		IExec->Remove((struct Node *)dw);
      		IExec->FreeVec(dw);
      		break;
    	}
  	}
  	IExec->ReleaseSemaphore(&ListSemaphore);
}

