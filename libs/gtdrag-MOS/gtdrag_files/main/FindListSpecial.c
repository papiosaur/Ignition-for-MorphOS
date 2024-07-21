/* :ts=4
 *  $VER: FindListSpecial.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/FindListSpecial ******************************************
*
*   NAME
*      FindListSpecial -- Description
*
*   SYNOPSIS
*      struct TreeNode * FindListSpecial(struct MinList * list, 
*          APTR special);
*
*   FUNCTION
*
*   INPUTS
*       list - 
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

struct TreeNode * _Gtdrag_FindListSpecial(struct GtdragIFace *Self, struct MinList * list, APTR special)
{
  	struct TreeNode *tn;

  	if (!list)
    	return(NULL);

  	foreach(list,tn)
  	{
    	if (special == tn->tn_Special)
      		return(tn);
  	}
  	return(NULL);
}

