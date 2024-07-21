/* :ts=4
 *  $VER: GTD_RemoveGadget.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_RemoveGadget ******************************************
*
*   NAME
*      GTD_RemoveGadget -- Description
*
*   SYNOPSIS
*      void GTD_RemoveGadget(struct Gadget * gad);
*
*   FUNCTION
*
*   INPUTS
*       gad - 
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

void _Gtdrag_GTD_RemoveGadget(struct GtdragIFace *Self, struct Gadget * gad)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragGadget *dg;

  	IExec->ObtainSemaphore(&ListSemaphore);
  	if ((dg = FindDragGadget(gad)) != 0)
  	{
    	IExec->Remove((struct Node *)dg);
    	IExec->FreeVec(dg);
  	}
  	IExec->ReleaseSemaphore(&ListSemaphore);
}

