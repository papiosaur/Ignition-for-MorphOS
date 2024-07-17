/* :ts=4
 *  $VER: GTD_GetString.c $Revision$ (20-Sep-2013)
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
#include <proto/utility.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

/****** gtdrag/main/GTD_GetString ******************************************
*
*   NAME
*      GTD_GetString -- Description
*
*   SYNOPSIS
*      STRPTR GTD_GetString(struct ObjectDescription * od, STRPTR buf, 
*          LONG len);
*
*   FUNCTION
*
*   INPUTS
*       od - 
*       buf - 
*       len - 
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

STRPTR _Gtdrag_GTD_GetString(struct GtdragIFace *Self, struct ObjectDescription *od, STRPTR t, LONG len)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct DOSIFace *IDOS = libBase->IDOS;
	struct UtilityIFace *IUtility = libBase->IUtility;

  	if (!od || !od->od_Object || !t || !len)
    	return(NULL);

  	switch(od->od_Type)
  	{
    	case ODT_STRING:
      		IUtility->Strlcpy(t,od->od_Object,len);
      		break;
    	case ODT_NODE:
    	case ODT_IMAGENODE:
      		IUtility->Strlcpy(t,((struct Node *)od->od_Object)->ln_Name,len);
      		break;
    	case ODT_TREENODE:
      		IUtility->Strlcpy(t,TREENODE(od->od_Object)->tn_Node.in_Name,len);
      		break;
    	case ODT_LOCK:
      		if (IDOS->NameFromLock((BPTR)od->od_Object,t,len))
        		break;
    	default:
      		return(NULL);
  	}
  	return(t);
}

