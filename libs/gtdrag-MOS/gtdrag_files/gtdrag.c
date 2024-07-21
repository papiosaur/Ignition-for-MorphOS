/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	Private functions.

#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <devices/input.h>
//#include <clib/alib_protos.h>
#include "gtdrag_loc.h"
#include <proto/gtdrag.h>
#include "gtdrag_private.h"


struct MinList applist;
struct MinList gadlist;
struct MinList winlist;
struct DragObj *gdo;
struct DragGadget *dg,*hdg,*adg;
struct DropMessage dm;
struct BitMap *hbm;
struct Gadget *boopsigad;
struct RastPort hrp,hsrp;
int    mx,my,hx,hy,hw,hh;
BOOL   fakemsg,noreport;
struct MsgPort *dmport;
struct SignalSemaphore ListSemaphore;
struct Hook iffstreamHook,renderHook,treeHook;


int CountNodes(struct List *l)
{
  	struct Node *n;
  	int i = 0;

  	if (!l)
    	return(0);
  	for(n = l->lh_Head;n->ln_Succ;n = n->ln_Succ,i++);
  	return(i);
}


struct Node * FindListNumber(struct MinList *l,long num)
{
  	struct Node *n;
  	long    i;
  	ULONG  listend = ~0L;

  	if (IsListEmpty((struct List *)l))
    	return(NULL);
  	for(n = (struct Node *)l->mlh_Head,i = 0;(i < num) && (n->ln_Succ);i++,n = n->ln_Succ)
    	if (n == (struct Node *)l->mlh_TailPred && listend == -1)
      		listend = i;
  	if (listend < num)
    	return(NULL);
  	return(n);
}

//Änderungen zu Orginal auskommentiert
void FakeInputEvent(struct GtdragIFace *Self)
{
  	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
  	struct ExecIFace *IExec = libBase->IExec;
  	struct IOStdReq *ireq;
  	struct MsgPort *iport;
  	struct InputEvent *ie;

  	if ((iport = IExec->AllocSysObjectTags(ASOT_PORT, ASOPORT_Pri, 0, TAG_END)) != 0)
  	{
    	if ((ie = IExec->AllocVecTags(sizeof(struct InputEvent), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE)) != 0)
    	{
   		if ((ireq = (struct IOStdReq *)IExec->AllocSysObjectTags(ASOT_IOREQUEST, ASOIOR_ReplyPort, iport, ASOIOR_Size, sizeof(struct IOStdReq), TAG_DONE)) != 0)
      		{
        		if ((!IExec->OpenDevice("input.device",0,(struct IORequest *)ireq,0)) != 0)
        		{
	          		ie->ie_NextEvent = NULL;
    	      		ie->ie_Class = IECLASS_RAWMOUSE;
        	  		ie->ie_Code = IECODE_LBUTTON | IECODE_UP_PREFIX;
          			ie->ie_Qualifier = 0;
          			ie->ie_X = 0;
	          		ie->ie_Y = 0;

    	      		ireq->io_Data = (APTR)ie;
        	  		ireq->io_Command = IND_WRITEEVENT;
          			ireq->io_Flags = 0;
	          		ireq->io_Length = sizeof(struct InputEvent);
    	      		fakemsg = TRUE;
        	  		IExec->DoIO((struct IORequest *)ireq);
          			IExec->CloseDevice((struct IORequest *)ireq);
        		}
        		IExec->FreeSysObject(ASOT_IOREQUEST, (struct IORequest *)ireq);
      		}
      		IExec->FreeVec(ie);
   		}
    IExec->FreeSysObject(ASOT_PORT, iport);
  	}
}

