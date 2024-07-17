/* :ts=4
 *  $VER: GTD_StopDrag.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_StopDrag ******************************************
*
*   NAME
*      GTD_StopDrag -- Description
*
*   SYNOPSIS
*      void GTD_StopDrag(struct Gadget * gad);
*
*   FUNCTION
*
*   INPUTS
*       gad - 
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

void _Gtdrag_GTD_StopDrag(struct GtdragIFace *Self, struct Gadget * gad)
{
  	if (gad != boopsigad || !gdo)
    	return;
  	FreeDragObj(Self, gdo);
  	EndDrag(Self);
}

