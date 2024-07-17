/* :ts=4
 *  $VER: GTD_PrepareDrag.c $Revision$ (20-Sep-2013)
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
#include <Intuition/gadgetclass.h>
#include <Intuition/cghooks.h>
#include <libraries/gtdrag.h>
#include <utility/utility.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

extern int VARARGS68K _Gtdrag_GTD_AddApp(struct GtdragIFace *Self, STRPTR name, ...);

/****** gtdrag/main/GTD_PrepareDrag ******************************************
*
*   NAME
*      GTD_PrepareDrag -- Description
*
*   SYNOPSIS
*      BOOL GTD_PrepareDrag(struct Gadget * gad, struct gpInput * gpi);
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

BOOL _Gtdrag_GTD_PrepareDrag(struct GtdragIFace *Self, struct Gadget * gad, struct gpInput * gpi)
{
  	struct DragGadget *dg;
  	struct Window *win;

  	if (!gad || !gpi || gdo || !gpi->gpi_IEvent || !(gpi->gpi_IEvent->ie_Qualifier & IEQUALIFIER_LEFTBUTTON) || !gpi->gpi_GInfo)
    	return(FALSE);

  	if (FindDragGadget(gad))  // gadget is already registered
    	return(TRUE);

  	win = gpi->gpi_GInfo->gi_Window;

  	if (!GetDragApp(Self, NULL))    // create an drag application for the input.device
  	{
    	_Gtdrag_GTD_AddApp(Self, "intuition",GTDA_NewStyle,TRUE,TAG_END);
  	}

  	if ((dg = AddDragGadget(Self, gad,win,BOOPSI_KIND)) != 0)
  	{
    	struct DragWindow *dw;

    	foreach(&winlist,dw)      // search for the gadget's real application
      	if (dw->dw_Window == win)
        	break;

    	if (dw->dw_Node.mln_Succ) // window found
      		dg->dg_Task = dw->dw_Task;
    	else                      // search gadget list
    	{
      		struct DragGadget *sdg;

      		foreach(&gadlist,sdg)
      		{
        		if (sdg != dg && sdg->dg_Window == win)
        		{
          			dg->dg_Task = sdg->dg_Task;
          			break;
        		}
      		}
    	}
    	return(TRUE);
  	}
  	return(FALSE);
}

