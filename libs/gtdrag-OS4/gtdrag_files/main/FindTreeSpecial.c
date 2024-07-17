/* :ts=4
 *  $VER: FindTreeSpecial.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/FindTreeSpecial ******************************************
*
*   NAME
*      FindTreeSpecial -- Description
*
*   SYNOPSIS
*      struct TreeNode * FindTreeSpecial(struct MinList * tree, 
*          APTR special);
*
*   FUNCTION
*
*   INPUTS
*       tree - 
*       special - 
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

struct TreeNode * _Gtdrag_FindTreeSpecial(struct GtdragIFace *Self, struct MinList * tree, APTR special)
{
  	struct TreeNode *tn;

  	if (!tree)
    	return(NULL);

  	foreach(tree,tn)
  	{
    	if (special == tn->tn_Special)
      		return(tn);
    	if (tn->tn_Flags & TNF_CONTAINER)
    	{
      		struct TreeNode *ftn;

      		if ((ftn = Self->FindTreeSpecial(&tn->tn_Nodes,special)) != 0)
        	return(ftn);
    	}
  	}
  	return(NULL);
}

