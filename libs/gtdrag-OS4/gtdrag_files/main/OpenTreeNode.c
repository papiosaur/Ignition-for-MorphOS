/* :ts=4
 *  $VER: OpenTreeNode.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/OpenTreeNode ******************************************
*
*   NAME
*      OpenTreeNode -- Description
*
*   SYNOPSIS
*      LONG OpenTreeNode(struct MinList * main, struct TreeNode * treenode);
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

LONG _Gtdrag_OpenTreeNode(struct GtdragIFace *Self, struct MinList * main, struct TreeNode * treenode)
{
    /* Write me. Really, I dare you! */
//    ((struct ExecIFace *)((*(struct ExecBase **)4)->MainInterface))->DebugPrintF("Function gtdrag::OpenTreeNode not implemented\n");  
//    return (LONG)0;
  struct MinNode *insertln;
  struct TreeNode *itn;
  long   count = 0; 

  if (!main || !treenode)
    return(0);

  insertln = &treenode->tn_ViewNode;

  foreach(&treenode->tn_Nodes, itn)
  {
    ((struct ExecIFace *)((*(struct ExecBase **)4)->MainInterface))->Insert((struct List *)main,(struct Node *)&itn->tn_ViewNode,(struct Node *)insertln);
    count++;

    insertln = itn->tn_ViewNode.mln_Succ;
    if ((itn->tn_Flags & (TNF_CONTAINER | TNF_OPEN)) == (TNF_CONTAINER | TNF_OPEN))
      count += _Gtdrag_OpenTreeNode(Self,main,itn);
    insertln = insertln->mln_Pred;
  }
  return count;
}

