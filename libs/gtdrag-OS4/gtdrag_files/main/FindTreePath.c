/* :ts=4
 *  $VER: FindTreePath.c $Revision$ (20-Sep-2013)
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
#include <string.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"


/****** gtdrag/main/FindTreePath ******************************************
*
*   NAME
*      FindTreePath -- Description
*
*   SYNOPSIS
*      struct TreeNode * FindTreePath(struct MinList * tree, STRPTR path);
*
*   FUNCTION
*
*   INPUTS
*       tree - 
*       path - 
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

struct TreeNode * _Gtdrag_FindTreePath(struct GtdragIFace *Self, struct MinList * tree, STRPTR path)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
  	struct TreeNode *tn;
  	long   i;

  	if (!tree || !path)
    	return(NULL);

//  	for(i = 0;*(path+i) && *(path+i) != '/';i++);
  	for(i = 0;path[i] && path[i] != '/';i++);
	if (!i)
   		return(NULL);

  	foreach(tree,tn)
  	{
    	if (!IUtility->Strnicmp(path,tn->tn_Node.in_Name,i))
    	{
      		if (*(path+i))
        		return(Self->FindTreePath(&tn->tn_Nodes,path+i+1));

      		return(tn);
    	}
  	}
  	return(NULL);
}

