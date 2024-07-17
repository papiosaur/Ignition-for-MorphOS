/* :ts=4
 *  $VER: GetTreePath.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/GetTreePath ******************************************
*
*   NAME
*      GetTreePath -- Description
*
*   SYNOPSIS
*      STRPTR GetTreePath(struct TreeNode * treenode, STRPTR buffer, 
*          LONG len);
*
*   FUNCTION
*
*   INPUTS
*       treenode - 
*       buffer - 
*       len - 
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

STRPTR _Gtdrag_GetTreePath(struct GtdragIFace *Self, struct TreeNode * tn, STRPTR buffer, LONG len)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct UtilityIFace *IUtility = libBase->IUtility;
  	long pos = 0;

  	if (!tn || !buffer || !len)
    	return(NULL);
  	*buffer = 0;
  	while((tn = Self->GetTreeContainer(tn)) != 0)
  	{
	  	char tmp[1000];
    
    	pos += IUtility->Strlen(tn->tn_Node.in_Name)+1;
    	if (len <= pos)
    		break;
    	IUtility->Strlcat(tmp, tn->tn_Node.in_Name, 1000);
    	IUtility->Strlcat(tmp, "/", 1000);
    	IUtility->Strlcat(tmp, buffer, 1000);
    	IUtility->Strlcpy(buffer, tn->tn_Node.in_Name, 1000);
  	}
  	return(buffer);
}

