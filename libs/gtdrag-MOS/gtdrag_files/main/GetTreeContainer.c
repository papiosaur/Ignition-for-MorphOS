/* :ts=4
 *  $VER: GetTreeContainer.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/GetTreeContainer ******************************************
*
*   NAME
*      GetTreeContainer -- Description
*
*   SYNOPSIS
*      struct TreeNode * GetTreeContainer(struct TreeNode * treenode);
*
*   FUNCTION
*
*   INPUTS
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

struct TreeNode * _Gtdrag_GetTreeContainer(struct GtdragIFace *Self, struct TreeNode * tn)
{
  	if (!tn || !tn->tn_Depth)
    	return(NULL);
  	for(;tn->tn_Node.in_Pred;tn = (APTR)tn->tn_Node.in_Pred);
  	return((struct TreeNode *)((UBYTE *)tn - sizeof(struct MinNode) - sizeof(struct ImageNode)));
}

