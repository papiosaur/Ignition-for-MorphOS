/* :ts=4
 *  $VER: GTD_RemoveGadgets.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_RemoveGadgets ******************************************
*
*   NAME
*      GTD_RemoveGadgets -- Description
*
*   SYNOPSIS
*      void GTD_RemoveGadgets(struct Window * gad);
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

void _Gtdrag_GTD_RemoveGadgets(struct GtdragIFace *Self, struct Window * win)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragGadget *dg,*ndg;

  	IExec->ObtainSemaphore(&ListSemaphore);
  	for(dg = (APTR)gadlist.mlh_Head;dg->dg_Node.mln_Succ;dg = ndg)
  	{
    	ndg = (APTR)dg->dg_Node.mln_Succ;
    	if (dg->dg_Window == win)
    	{
      		IExec->Remove((struct Node *)dg);
      		FreeDragGadget(Self, dg);
    	}
  	}
  	IExec->ReleaseSemaphore(&ListSemaphore);
}

