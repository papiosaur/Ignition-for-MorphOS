/* :ts=4
 *  $VER: AddTreeNode.c $Revision$ (20-Sep-2013)
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
#include <proto/utility.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

/****** gtdrag/main/AddTreeNode ******************************************
*
*   NAME
*      AddTreeNode -- Description
*
*   SYNOPSIS
*      struct TreeNode * AddTreeNode(APTR pool, struct MinList * tree, 
*          STRPTR name, struct Image * image, UWORD flags);
*
*   FUNCTION
*
*   INPUTS
*       pool - 
*       tree - 
*       name - 
*       image - 
*       flags - 
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

struct TreeNode * _Gtdrag_AddTreeNode(struct GtdragIFace *Self, APTR pool, struct MinList * tree, STRPTR name, struct Image * im, UWORD flags)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
	struct UtilityIFace *IUtility = libBase->IUtility;
  	struct TreeNode *tn;

  	if ((tn = IExec->AllocPooled(pool,sizeof(struct TreeNode))) != 0)
  	{
    	IExec->MyNewList(&tn->tn_Nodes);
    	tn->tn_Node.in_Name = name;
    	tn->tn_Node.in_Image = im;
    	tn->tn_Flags = flags;
    	if (flags & TNF_SORT)
    	{	
      		struct TreeNode *stn;
      
      		foreach(tree,stn)
      		{
        	if (IUtility->Stricmp(stn->tn_Node.in_Name,name) > 0)
          		break;
      		}
      		IExec->Insert((struct List *)tree,(struct Node *)tn,(struct Node *)stn->tn_Node.in_Pred);
    	}
    	else
      		IExec->MyAddTail(tree,tn);
  	}
  	return(tn);
}

