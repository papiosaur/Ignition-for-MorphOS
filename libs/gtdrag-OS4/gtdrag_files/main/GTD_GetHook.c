/* :ts=4
 *  $VER: GTD_GetHook.c $Revision$ (20-Sep-2013)
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

/****** gtdrag/main/GTD_GetHook ******************************************
*
*   NAME
*      GTD_GetHook -- Description
*
*   SYNOPSIS
*      struct Hook * GTD_GetHook(ULONG num);
*
*   FUNCTION
*
*   INPUTS
*       num - 
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

struct Hook * _Gtdrag_GTD_GetHook(struct GtdragIFace *Self, ULONG type)
{
  	switch(type)
  	{
    	case GTDH_IMAGE: 
    		return(&renderHook);
    	case GTDH_TREE: 
    		return(&treeHook);
    	case GTDH_IFFSTREAM: 
    		return(&iffstreamHook);
  	}
  	return(NULL);
}

