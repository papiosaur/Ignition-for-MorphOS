/* :ts=4
 *  $VER: GTD_BeginDrag.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_BeginDrag ******************************************
*
*   NAME
*      GTD_BeginDrag -- Description
*
*   SYNOPSIS
*      BOOL GTD_BeginDrag(struct Gadget * gad, struct gpInput * gpi);
*
*   FUNCTION
*
*   INPUTS
*       gad - 
*       gpi - 
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

BOOL _Gtdrag_GTD_BeginDrag(struct GtdragIFace *Self, struct Gadget * gad, struct gpInput * gpi)
{
  	if ((dg = FindDragGadget(gad)) != 0)
  	{
    	PrepareDrag(Self, TRUE);
    	boopsigad = gad;
    	gdo = CreateDragObj(Self, dg,gpi->gpi_Mouse.X+gad->LeftEdge,gpi->gpi_Mouse.Y+gad->TopEdge);
    	return(TRUE);
  	}
  	return(FALSE);
}

