/* :ts=4
 *  $VER: InitTreeList.c $Revision$ (20-Sep-2013)
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
#include <exec/lists.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"


/****** gtdrag/main/InitTreeList ******************************************
*
*   NAME
*      InitTreeList -- Description
*
*   SYNOPSIS
*      void InitTreeList(struct TreeList * treelist);
*
*   FUNCTION
*
*   INPUTS
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

void _Gtdrag_InitTreeList(struct GtdragIFace *Self, struct TreeList * tl)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;

  	IExec->MyNewList(&tl->tl_View);
  	FillTreeList(Self, &tl->tl_View,&tl->tl_Tree,0,TNF_OPEN,0);
}

