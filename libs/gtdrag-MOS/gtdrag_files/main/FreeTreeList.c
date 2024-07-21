/* :ts=4
 *  $VER: FreeTreeList.c $Revision$ (20-Sep-2013)
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


/****** gtdrag/main/FreeTreeList ******************************************
*
*   NAME
*      FreeTreeList -- Description
*
*   SYNOPSIS
*      void FreeTreeList(APTR pool, struct TreeList * treelist);
*
*   FUNCTION
*
*   INPUTS
*       pool - 
*       treelist - 
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

void _Gtdrag_FreeTreeList(struct GtdragIFace *Self, APTR pool, struct TreeList * tl)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;

  	IExec->MyNewList(&tl->tl_View);
  	Self->FreeTreeNodes(pool,&tl->tl_Tree);
}

