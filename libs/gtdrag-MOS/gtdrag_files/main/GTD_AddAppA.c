/* :ts=4
 *  $VER: GTD_AddAppA.c $Revision$ (20-Sep-2013)
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
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <proto/gtdrag.h>
#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"



/****** gtdrag/main/GTD_AddAppA ******************************************
*
*   NAME
*      GTD_AddAppA -- Description
*      GTD_AddApp -- Vararg stub
*
*   SYNOPSIS
*      int GTD_AddAppA(STRPTR name, struct TagItem * tagList);
*      int GTD_AddApp(STRPTR name, ...);
*
*   FUNCTION
*
*   INPUTS
*       name - 
*       tagList - 
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

int VARARGS68K _Gtdrag_GTD_AddApp(struct GtdragIFace *Self, STRPTR name, ...);
int _Gtdrag_GTD_AddApp(struct GtdragIFace *Self, STRPTR name, ...)
{
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, name);
	tags = va_getlinearva(ap, struct TagItem *);

	return Self->GTD_AddAppA(name,  tags);
}

int _Gtdrag_GTD_AddAppA(struct GtdragIFace *Self, STRPTR name, struct TagItem *tagList)
{
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
	struct ExecIFace *IExec = libBase->IExec;
	struct UtilityIFace *IUtility = libBase->IUtility;	
	struct DragApp *da;

	if (!IUtility->FindTagItem(GTDA_NewStyle, tagList))
		return(TRUE);
	IExec->ObtainSemaphore(&ListSemaphore);

	if (name && !GetDragApp(Self, NULL))
	{
    	if ((da = IExec->AllocVecTags(sizeof(struct DragApp), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE)) != 0)
		{
			da->da_Name = name;
			da->da_Task = IExec->FindTask(NULL);
			da->da_Flags = IUtility->GetTagData(GTDA_InternalOnly,0,tagList);
			IExec->AddTail((struct List *)&applist,(APTR)da);
			IExec->ReleaseSemaphore(&ListSemaphore);
			return(TRUE);
		}
	}
	IExec->ReleaseSemaphore(&ListSemaphore);
	return(FALSE);
}

