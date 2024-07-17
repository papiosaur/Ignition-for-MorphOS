/* :ts=4
 *  $VER: GTD_GetIMsg.c $Revision$ (20-Sep-2013)
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
#include <proto/gadtools.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

/****** gtdrag/main/GTD_GetIMsg ******************************************
*
*   NAME
*      GTD_GetIMsg -- Description
*
*   SYNOPSIS
*      struct IntuiMessage * GTD_GetIMsg(struct MsgPort * iport);
*
*   FUNCTION
*
*   INPUTS
*       iport - 
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

struct IntuiMessage * _Gtdrag_GTD_GetIMsg(struct GtdragIFace *Self, struct MsgPort *mp)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
	struct GadToolsIFace *IGadTools = libBase->IGadTools;
  	struct IntuiMessage *msg;
  	struct DragApp *da;

  	if ((da = GetDragApp(Self,NULL)) != 0)
  	{
    	while((msg = (struct IntuiMessage *)IExec->GetMsg(mp)) != 0)
    	{
      		msg = HandleIMsg(Self,da,msg);
      		if (!da->da_GTMsg)
        		IExec->ReplyMsg((struct Message *)msg);
      		else
        		return(msg);
    	}
  	}
  	else
    	return(IGadTools->GT_GetIMsg(mp));

  	return(NULL);
}

