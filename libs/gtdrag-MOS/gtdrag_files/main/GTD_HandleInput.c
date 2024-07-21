/* :ts=4
 *  $VER: GTD_HandleInput.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_HandleInput ******************************************
*
*   NAME
*      GTD_HandleInput -- Description
*
*   SYNOPSIS
*      ULONG GTD_HandleInput(struct Gadget * gad, struct gpInput * gpi);
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

ULONG _Gtdrag_GTD_HandleInput(struct GtdragIFace *Self, struct Gadget * gad, struct gpInput * gpi)
{
  	struct InputEvent *ie;
  	long   x,y;

 	if (!gpi || !gad || !gdo || gad != boopsigad || !(ie = gpi->gpi_IEvent))
    	return(GMR_HANDLEYOURSELF);

  	x = gpi->gpi_Mouse.X+gad->LeftEdge;
  	y = gpi->gpi_Mouse.Y+gad->TopEdge;

  	switch(ie->ie_Class)
  	{
    	case IECLASS_RAWMOUSE:
      		if (ie->ie_Code == IECODE_NOBUTTON)
      		{
        		MouseMove(Self, x,y);
        		return(GMR_MEACTIVE);
      		}
      		else
      		{
        		if (ie->ie_Code == (IECODE_LBUTTON | IECODE_UP_PREFIX))
          			MakeDropMessage(Self, GetDragApp(Self, dg->dg_Task),ie->ie_Qualifier,x,y);
        		FreeDragObj(Self, gdo);
        		EndDrag(Self);
        		return(GMR_NOREUSE);
      		}
      		break;
    	case IECLASS_TIMER:
      		IntuiTick(Self, x,y);
      		break;
  	}
  	return(GMR_MEACTIVE);  // catches all input events
}

