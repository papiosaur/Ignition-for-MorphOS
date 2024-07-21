/* :ts=4
 *  $VER: GTD_AddGadgetA.c $Revision$ (20-Sep-2013)
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

extern void _Gtdrag_GTD_SetAttrsA(struct GtdragIFace *Self, APTR gad, struct TagItem * tags);

/****** gtdrag/main/GTD_AddGadgetA ******************************************
*
*   NAME
*      GTD_AddGadgetA -- Description
*      GTD_AddGadget -- Vararg stub
*
*   SYNOPSIS
*      void GTD_AddGadgetA(ULONG type, struct Gadget * gad, 
*          struct Window * win, struct TagItem * tagList);
*      void GTD_AddGadget(ULONG type, struct Gadget * gad, 
*          struct Window * win, ...);
*
*   FUNCTION
*
*   INPUTS
*       type - 
*       gad - 
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

void VARARGS68K _Gtdrag_GTD_AddGadget(struct GtdragIFace *Self, ULONG type, struct Gadget * gad, struct Window * win, ...);

void _Gtdrag_GTD_AddGadget(struct GtdragIFace *Self, ULONG type, struct Gadget * gad, struct Window * win, ...)
{
	va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, win);
    tags = va_getlinearva(ap, struct TagItem *);

    Self->GTD_AddGadgetA(type, gad, win, tags);
}

void _Gtdrag_GTD_AddGadgetA(struct GtdragIFace *Self, ULONG type, struct Gadget * gad, struct Window * win, struct TagItem * tagList)
{
  	struct DragGadget *dg;

  	if ((dg = AddDragGadget(Self,gad,win,type)) != 0)
  	{
    	_Gtdrag_GTD_SetAttrsA(Self, gad,tagList);
    	if (!dg->dg_ItemHeight)
      		dg->dg_ItemHeight = win->WScreen->Font->ta_YSize;
  	}
}

