/* :ts=4
 *  $VER: GTD_RemoveApp.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_RemoveApp ******************************************
*
*   NAME
*      GTD_RemoveApp -- Description
*
*   SYNOPSIS
*      void GTD_RemoveApp(void);
*
*   FUNCTION
*
*   INPUTS
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

void _Gtdrag_GTD_RemoveApp(struct GtdragIFace *Self)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragApp *da;
  	struct MinNode *n,*s;

  	IExec->ObtainSemaphore(&ListSemaphore);
  	if ((da = GetDragApp(Self, NULL)) != 0)
  	{
    	for(n = winlist.mlh_Head;n->mln_Succ;n = n->mln_Succ)
    	{
      		if (((struct DragWindow *)n)->dw_Task == da->da_Task)
      		{
        		s = n->mln_Pred;
        		IExec->Remove((struct Node *)n);
        		IExec->FreeVec(n);
        		n = s;
      		}
    	}
    	for(n = gadlist.mlh_Head;n->mln_Succ;n = n->mln_Succ)
    	{
      		if (((struct DragGadget *)n)->dg_Task == da->da_Task)
      		{
        		s = n->mln_Pred;
        		IExec->Remove((struct Node *)n);
        		IExec->FreeVec(n);
        		n = s;
      		}
    	}
    	IExec->Remove((struct Node *)da);
    	IExec->FreeVec(da);
  	}
  	IExec->ReleaseSemaphore(&ListSemaphore);
}

