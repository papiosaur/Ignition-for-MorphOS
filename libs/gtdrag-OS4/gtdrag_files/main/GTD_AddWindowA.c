/* :ts=4
 *  $VER: GTD_AddWindowA.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_AddWindowA ******************************************
*
*   NAME
*      GTD_AddWindowA -- Description
*      GTD_AddWindow -- Vararg stub
*
*   SYNOPSIS
*      void GTD_AddWindowA(struct Window * win, struct TagItem * tagList);
*      void GTD_AddWindow(struct Window * win, ...);
*
*   FUNCTION
*
*   INPUTS
*       win - 
*       tagList - 
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

void VARARGS68K _Gtdrag_GTD_AddWindow(struct GtdragIFace *Self, struct Window *win, ...);

void _Gtdrag_GTD_AddWindow(struct GtdragIFace *Self, struct Window *win, ...)
{
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, win);
    tags = va_getlinearva(ap, struct TagItem *);

    Self->GTD_AddWindowA(win, tags);
}

void _Gtdrag_GTD_AddWindowA(struct GtdragIFace *Self, struct Window *win, struct TagItem * tagList)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
  	struct DragApp *da;
  	struct DragWindow *dw;

  	IExec->ObtainSemaphore(&ListSemaphore); 
  	if ((da = GetDragApp(Self, NULL)) && win && (dw = IExec->AllocVecTags(sizeof(struct DragWindow), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE)))
  	{
    	dw->dw_Window = win;
    	dw->dw_Task = da->da_Task;
    	dw->dw_AcceptMask = 0xffffffff;
    	SetWindowAttrs(Self,win,tagList);
    	IExec->AddTail((struct List *)&winlist,(APTR)dw);
  	}
  	IExec->ReleaseSemaphore(&ListSemaphore);
}

