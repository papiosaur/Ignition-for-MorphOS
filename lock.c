/* LockNodes implementation
 *
 * Copyright ©1996-2008 pinc Software, All Rights Reserved
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include <proto/gtdrag.h>
	#include <stdarg.h>
#endif


struct LockNode *nextln;
bool gLockStop;

/*
struct LockNode *
FindLockNodeA(struct MinList *list, ULONG length, ULONG *data)
{
	struct LockNode *ln;

	foreach(&locks, ln) {
		if (ln->ln_Length == length) {
			if (!memcmp(ln->ln_Data,data,length))
				return ln;
		}
	}
	return NULL;
}


struct LockNode *
FindLockNode(struct MinList *list, ULONG length, ...)
{
	return FindLockNodeA(list, length, &length + 1);
}
*/

void
RemLockNode(struct LockNode *ln)
{
	if (!ln)
		return;

	if (nextln == ln)
		nextln = (struct LockNode *)ln->ln_Node.ln_Succ;

	MyRemove(ln);
	if (ln->ln_Data)
		FreePooled(pool,ln->ln_Data,ln->ln_Length);
	FreePooled(pool,ln,sizeof(struct LockNode));
}


/*
void
RemLockNodeData(struct MinList *list,ULONG length,...)
{
	struct LockNode *ln;

	if ((ln = FindLockNodeA(list, length, &length + 1)) != 0)
		RemLockNode(ln);
}
*/

struct LockNode *
AddLockNode(struct MinList *list,BYTE pri,APTR func,ULONG length,...)
{
	struct LockNode *ln;
#ifdef __amigaos4__
	va_list ap;
	ULONG *longs;

	va_startlinear(ap, length);
	longs = va_getlinearva(ap, ULONG *);
#endif

	if ((ln = AllocPooled(pool, sizeof(struct LockNode))) != 0) {
		ln->ln_Node.ln_Pri = pri;
		ln->ln_List = list;
		ln->ln_Function = func;

		if ((ln->ln_Length = length) && (ln->ln_Data = AllocPooled(pool,length)))
#ifdef __amigaos4__
			CopyMem(longs, ln->ln_Data, length);
#else
			CopyMem(&length+1, ln->ln_Data, length);
#endif

		MyEnqueue(&locks, ln);
	}
	return ln;
}


struct LockNode *
AddFreeLock(APTR list,struct Window *win)
{
	return AddLockNode(list, 0, FreeLock, sizeof(APTR), win);
}


struct LockNode *
AddListViewLock(struct MinList *list, struct Window *win, struct Gadget *gad)
{
	return AddLockNode(list, 0, ListViewLock, sizeof(APTR) * 2, win, gad);
}


struct LockNode *
AddTreeLock(struct MinList *list,struct Window *win,struct Gadget *gad)
{
	return AddLockNode(list,0,TreeLock,sizeof(APTR)*2,win,gad);
}


struct LockNode *
AddTextLock(struct MinList *list,struct Window *win,struct Gadget *gad)
{
	return AddLockNode(list,0,TextLock,sizeof(APTR)*2,win,gad);
}


void
UnlockListNode(struct MinList *list,struct MinNode *node,UBYTE flags)
{
	struct LockNode *ln;

	if (!node && !list || gLockStop)
		return;
	if (!list)
		list = FindList(node);

	for (ln = (APTR)locks.mlh_Head;ln->ln_Node.ln_Succ;ln = nextln) {
		nextln = (struct LockNode *)ln->ln_Node.ln_Succ;

		if (list == ln->ln_List && ln->ln_Locked) {
			if (!--ln->ln_Locked)
				ln->ln_Function(ln,node,flags | LNCMD_UNLOCK);
		}
	}
}


bool
LockListNode(struct MinList *list,struct MinNode *node,UBYTE flags)
{
	struct LockNode *ln;

	if (!node && !list || gLockStop)
		return TRUE;
	if (!list)
		list = FindList(node);

	for (ln = (APTR)locks.mlh_Head;ln->ln_Node.ln_Succ;ln = nextln) {
		nextln = (struct LockNode *)ln->ln_Node.ln_Succ;

		if (list == ln->ln_List) {
			if (!ln->ln_Locked++)
				ln->ln_Function(ln,node,flags | LNCMD_LOCK);
		}
	}
	return TRUE;
}


void
UnlockList(struct MinList *list,UBYTE flags)
{
	UnlockListNode(list,NULL,flags);
}


bool
LockList(struct MinList *list,UBYTE flags)
{
	return LockListNode(list,NULL,flags);
}


void
ApplyLockListCommand(struct MinList *list,struct MinNode *node,UBYTE cmd)
{
	struct LockNode *ln;

	if (!list || gLockStop)
		return;

	for (ln = (APTR)locks.mlh_Head;ln->ln_Node.ln_Succ;ln = nextln) {
		nextln = (struct LockNode *)ln->ln_Node.ln_Succ;
		if (list == ln->ln_List)
			ln->ln_Function(ln,node,cmd);
	}
}


void
RefreshLockList(struct MinList *list)
{
	ApplyLockListCommand(list,NULL,LNCMD_REFRESH);
}


void
FreeLockList(APTR list)
{
	ApplyLockListCommand(list,NULL,LNCMD_FREE);
}


void
AddLockedTail(struct MinList *list,struct MinNode *ln)
{
	if (LockListNode(list,ln,LNF_ADD)) {
		MyAddTail(list, ln);
		UnlockListNode(list,ln,LNF_ADD);
	}
}


void
AddLockedHead(struct MinList *list,struct MinNode *ln)
{
	if (LockListNode(list,ln,LNF_ADD)) {
		MyAddHead(list, ln);
		UnlockListNode(list,ln,LNF_ADD);
	}
}


/*
void
AddLockedSorted(struct MinList *list,struct MinNode *ln,APTR func)
{
	if (LockListNode(list,ln,LNF_ADD))
	{
		int (*compare)(struct MinNode **,struct MinNode **) = func;
		struct MinNode *pre;

		foreach(list,pre)
			if (compare(&pre,&ln) > 0)
				break;
		Insert((struct List *)list,(struct Node *)ln,(struct Node *)pre);

		UnlockListNode(list,ln,LNF_ADD);
	}
}
*/


void
RemoveFromLockedList(struct MinList *list,struct MinNode *ln)
{
	if (LockListNode(list,ln,LNF_REMOVE)) {
		MyRemove(ln);
		UnlockListNode(list,ln,LNF_REMOVE);
	}
}


void
RemoveLocked(struct MinNode *ln)
{
	RemoveFromLockedList(FindList(ln),ln);
}


struct MinNode *
RemLockedHead(struct MinList *list)
{
	struct MinNode *ln;

	if (IsListEmpty((struct List *)list))
		return NULL;

	RemoveFromLockedList(list,ln = list->mlh_Head);
	return ln;
}


/********************** Standard Lock-Functions **********************/


void PUBLIC
FreeLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags))
{
	if (flags & LNCMD_FREE)
		CloseAppWindow(ln->ln_Data[0],TRUE);
}


void PUBLIC
ListViewLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags))
{
	switch (flags & LNCMDS) {
		case LNCMD_LOCK:
			GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,~0L,TAG_END);
			break;
		case LNCMD_FREE:
			CloseAppWindow(ln->ln_Data[0],TRUE);
			break;
/*  case LNCMD_UNLOCK:
		case LNCMD_REFRESH:*/
		default:
		{
			struct Node *node;
			long   i = ~0L;

			if ((node = ((struct Gadget *)ln->ln_Data[1])->UserData) != 0) {
				if ((i = FindListEntry(ln->ln_List,(struct MinNode *)node)) == ~0L)
					((struct Gadget *)ln->ln_Data[1])->UserData = NULL;
			}
//	  GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,ln->ln_List,node ? GTLV_Selected : TAG_IGNORE,i,TAG_END);
			/* MERKER: Gültigkeit der Änderung der oberen Zeile (UserData == NULL) prüfen */
			GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,ln->ln_List,GTLV_Selected,i,TAG_END);
			break;
		}
	}
}


void PUBLIC
TreeLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags))
{
	switch (flags & LNCMDS) {
		case LNCMD_LOCK:
			GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,~0L,TAG_END);
			break;
		case LNCMD_FREE:
			CloseAppWindow(ln->ln_Data[0],TRUE);
			break;
		default:
		{
			long i;

			InitTreeList((struct TreeList *)ln->ln_List);
			GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,&((struct TreeList *)ln->ln_List)->tl_View,TAG_END);

			if (GT_GetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Selected,&i,TAG_END) && i != ~0L) {
				if (!(((struct Gadget *)ln->ln_Data[1])->UserData = FindListNumber(&((struct TreeList *)ln->ln_List)->tl_View,i)))
					GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Selected,~0L,TAG_END);
			}
			break;
		}
	}
}


void PUBLIC
TextLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags))
{
	switch(flags & LNCMDS) {
		case LNCMD_UNLOCK:
			if (!(flags & (LNF_REMOVE | LNF_REFRESH)))
				break;
		case LNCMD_REFRESH:
		{
			CONST_STRPTR text = NULL;

			if (IsListEmpty((struct List *)ln->ln_List))
				text = GetString(&gLocaleInfo, MSG_EMPTY_LIST_GAD);
			else if (FindListEntry(ln->ln_List,node) == -1)
				text = ((struct Node *)ln->ln_List->mlh_Head)->ln_Name;

			if (text)
				GT_SetGadgetAttrs(*(ln->ln_Data+1),*ln->ln_Data,NULL,GTTX_Text,text,TAG_END);
			break;
		}
		case LNCMD_FREE:
			CloseAppWindow(ln->ln_Data[0],TRUE);
			break;
	}
}



