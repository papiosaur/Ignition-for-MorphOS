/* :ts=4
 *  $VER: ToggleTreeNode.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/ToggleTreeNode ******************************************
*
*   NAME
*      ToggleTreeNode -- Description
*
*   SYNOPSIS
*      LONG ToggleTreeNode(struct MinList * main, 
*          struct TreeNode * treenode);
*
*   FUNCTION
*
*   INPUTS
*       main - 
*       treenode - 
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

LONG _Gtdrag_ToggleTreeNode(struct GtdragIFace *Self, struct MinList * main, struct TreeNode * tn)
{
  	if (!main || !tn)
    	return(0);

  	if (tn->tn_Flags & TNF_CONTAINER)
  	{
    	if (tn->tn_Flags & TNF_OPEN)
    	{
      		tn->tn_Flags &= ~TNF_OPEN;
      		Self->CloseTreeNode(main,tn);
      		return(0);
    	}
    	tn->tn_Flags |= TNF_OPEN;
    	return(Self->OpenTreeNode(main,tn));
  	}
  	if (tn->tn_Flags & TNF_ADD)
    	tn->tn_Flags = (tn->tn_Flags & ~TNF_ADD) | TNF_REPLACE;
  	else if (tn->tn_Flags & TNF_REPLACE)
    	tn->tn_Flags = (tn->tn_Flags & ~TNF_REPLACE) | TNF_ADD;

  return(0);
}

