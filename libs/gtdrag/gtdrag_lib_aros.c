/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Library part (AROS).

#include <aros/symbolsets.h>

#include "gtdrag_includes.h"

struct Library *GTDragBase;

void PRIVATE GTD_Exit(void)
{
	if (dmport)
	{
		struct IntuiMessage *msg;

		while((msg = (struct IntuiMessage*)GetMsg(dmport)) != 0)
			FreeDropMessage(msg);
		DeleteMsgPort(dmport);
	}
}


int PRIVATE GTD_Init(void)
{
	if ((dmport = CreateMsgPort()) != 0)
		return TRUE;
	return FALSE;
}


BOOL LibInit(struct Library *l)
{
	if(SysBase->LibNode.lib_Version < 37)
	    return FALSE;

	/** init global data **/

	GTDragBase = l;
  
	NewList((struct List *)&applist);
	NewList((struct List *)&gadlist);
	NewList((struct List *)&winlist);

	mx = 0;  my = 0;
	gdo = NULL;  dg = NULL;
	fakemsg = FALSE;  noreport = FALSE;

	/** init hooks **/

	SETHOOK(treeHook, TreeHook);
	SETHOOK(renderHook, RenderHook);
	SETHOOK(iffstreamHook, IFFStreamHook);

	/** init semaphores **/

	InitSemaphore(&ListSemaphore);

	return TRUE;
}


BOOL LibOpen(struct Library *l)
{
	if (!GTD_Init())
	{
		GTD_Exit();
		return FALSE;
	}
	return TRUE;
}


void LibExpunge(struct Library *l)
{
	struct DragApp *da;
	struct Node *n;

	while((n = RemHead((struct List *)&gadlist)))
		FreeMem(n,sizeof(struct DragGadget));
	while((n = RemHead((struct List *)&winlist)))
		FreeMem(n,sizeof(struct DragWindow));
	while((da = (struct DragApp *)RemHead((struct List *)&applist)))
		FreeMem(da,sizeof(struct DragApp));
}


void LibClose(struct Library *l)
{
	GTD_Exit();
}


ADD2INITLIB(LibInit, 0);
ADD2EXPUNGELIB(LibExpunge, 0);
ADD2OPENLIB(LibOpen, 0);
ADD2CLOSELIB(LibClose, 0);
