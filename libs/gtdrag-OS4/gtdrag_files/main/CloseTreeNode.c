/* :ts=4
 *  $VER: CloseTreeNode.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/CloseTreeNode ******************************************
*
*   NAME
*      CloseTreeNode -- Description
*
*   SYNOPSIS
*      void CloseTreeNode(struct MinList * main, struct TreeNode * treenode);
*
*   FUNCTION
*
*   INPUTS
*       main - 
*       treenode - 
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

void _Gtdrag_CloseTreeNode(struct GtdragIFace *Self, struct MinList * main, struct TreeNode * tn)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct TreeNode *itn;

  	if (!main || !tn)
    	return;

  	foreach(&tn->tn_Nodes,itn)
  	{
    	if (itn->tn_ViewNode.mln_Succ)
    	{
      		IExec->Remove((struct Node *)&itn->tn_ViewNode);
      		itn->tn_ViewNode.mln_Succ = NULL;
    	}
    	if ((itn->tn_Flags & (TNF_CONTAINER | TNF_OPEN)) == (TNF_CONTAINER | TNF_OPEN))
      		Self->CloseTreeNode(main,itn);
  	}
}

