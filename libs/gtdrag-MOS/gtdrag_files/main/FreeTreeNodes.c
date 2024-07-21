/* :ts=4
 *  $VER: FreeTreeNodes.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/FreeTreeNodes ******************************************
*
*   NAME
*      FreeTreeNodes -- Description
*
*   SYNOPSIS
*      void FreeTreeNodes(APTR pool, struct MinList * list);
*
*   FUNCTION
*
*   INPUTS
*       pool - 
*       list - 
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

void _Gtdrag_FreeTreeNodes(struct GtdragIFace *Self, APTR pool, struct MinList * list)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct TreeNode *tn;

  	while((tn = (APTR)IExec->MyRemHead(list)) != 0)
  	{
    	if (tn->tn_Flags & TNF_CONTAINER)
      		Self->FreeTreeNodes(pool,&tn->tn_Nodes);
	    IExec->FreePooled(pool,tn,sizeof(struct TreeNode));
  	}
}

