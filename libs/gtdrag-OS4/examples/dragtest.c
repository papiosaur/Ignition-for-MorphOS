;/* dragtest.c - Test for the gtdrag.library, 11.6.1999
;**
;** Copyright ©1999 pinc Software.
;** All rights reserved.
;**
sc dragtest.c link ign=73 lib:debug.lib data=near parm=b nochkabort nostkchk strmer streq to=ram:dragtest
run ram:dragtest
quit
*/


/* This example demonstrates the drag&drop extension of simple
** Gadtools gadgets and how the DropMessage is analysed.
**
** It's a very simple example but you may use this code to produce
** one at your own.
**
** Although I tried hardly to not produce any errors, I cannot give
** any guaranties. Neither I nor pinc Software is liable for any
** problems you might have with it.
** The usage of this example is on your own risk.
*/


//#define INTUI_V36_NAMES_ONLY
//#define __USE_SYSBASE

#include <exec/libraries.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <libraries/gadtools.h>
#include <libraries/gtdrag.h>
#include <libraries/asl.h>
#include <dos/dos.h>

#include "SDI_compiler.h"

#if defined(__SASC)
#	include <pragmas/gtdrag_pragmas.h>
#	include <clib/alib_stdio_protos.h>
#endif

#include <clib/alib_protos.h>
#include <proto/gtdrag.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>


#define TLn(t) TextLength(&scr->RastPort,t,strlen(t))


UBYTE CHIP newImgData[104] =
{
        /* Plane 0 */
        0x00,0x00,0x00,0x20,0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,
        0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,
        0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,0x3F,0xFF,0xFF,0xE0,
        0x7F,0xFF,0xFF,0xE0,
        /* Plane 1 */
        0xFF,0xFF,0xFF,0xC0,0xFF,0xFF,0xFF,0x80,0xF3,0xBF,0xFF,0x80,0xF3,0xBF,0xFF,0x80,
        0xF5,0xB3,0x6D,0x80,0xF5,0xAD,0x6D,0x80,0xF6,0xA1,0xAB,0x80,0xF6,0xAF,0xAB,0x80,
        0xF7,0x2D,0xD7,0x80,0xF7,0x33,0xD7,0x80,0xFF,0xFF,0xFF,0x80,0xFF,0xFF,0xFF,0x80,
        0x80,0x00,0x00,0x00
};

struct Image newImg = {0,0,27,13,2,newImgData,0x0003,0x0000,NULL};

struct Screen *scr;
struct Window *win;
struct IntuiMessage imsg;
struct Library *GTDragBase;
struct Gadget *glist,*lvgad[2],*gad[3];
struct NewGadget ng;
struct List list[2];
struct Node *node;
struct ImageNode in;
APTR   vi;
int    fontheight;
BOOL   ende = FALSE;


/**************** Requester ****************/

long DoRequestA(STRPTR t,STRPTR gads,APTR args)
{
  struct EasyStruct es = {sizeof(struct EasyStruct),0,"DragTest-Request",NULL,NULL};

  es.es_TextFormat = t;
  es.es_GadgetFormat = gads;
  return(EasyRequestArgs(scr->FirstWindow,&es,0,args));
}


void ErrorRequest(STRPTR t,...)
{
  DoRequestA(t,"Ok",&t+1);
}


/**************** list manipulation ****************/

void InsertAt(struct List *l,struct Node *n,int pos)
{
  struct Node *pn;
  int    i;

  for(pn = (struct Node *)l,i = 0;i < pos;pn = pn->ln_Succ,i++);
  Insert(l,n,pn);
}


void MoveTo(struct Node *n,struct List *l1,int pos1,struct List *l2,int pos2)
{
  struct Node *pn;
  int    i;

  if (l1 == l2 && pos1 == pos2)
    return;
  Remove(n);

  if (l1 == l2 && pos1 < pos2)
    pos2--;
  for(pn = (struct Node *)l2,i = 0;i < pos2 && pn->ln_Succ;pn = pn->ln_Succ,i++);
  Insert(l2,n,pn);
}


/**************** main part ****************/

void processMsg(void)
{
  struct IntuiMessage *msg;
  struct DropMessage *dm;
  int    source,target;

  while(!ende)
  {
    WaitPort(win->UserPort);
    while((msg = GTD_GetIMsg(win->UserPort)) != 0)
    {
      imsg = *msg;

      if (imsg.Class == IDCMP_OBJECTDROP)
      {
        if ((dm = imsg.IAddress) != 0)
        {
          if (!dm->dm_Object.od_Owner && dm->dm_Target)
          {
            target = dm->dm_Target->GadgetID;
            switch(target)
            {
              case 1:
              case 2:
                source = dm->dm_Gadget->GadgetID;
                if (source < 3)
                {
                  source--;  target--;
                  GT_SetGadgetAttrs(lvgad[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                  GT_SetGadgetAttrs(lvgad[1],win,NULL,GTLV_Labels,~0L,TAG_END);
                  if (target)
                    MoveTo(dm->dm_Object.od_Object,&list[source],dm->dm_SourceEntry,&list[target],dm->dm_TargetEntry);
                  else
                  {
                    Remove(dm->dm_Object.od_Object);
                    AddTail(&list[target],dm->dm_Object.od_Object);
                  }
                  GT_SetGadgetAttrs(lvgad[0],win,NULL,GTLV_Labels,&list[0],TAG_END);
                  GT_SetGadgetAttrs(lvgad[1],win,NULL,GTLV_Labels,&list[1],TAG_END);
                }
                else
                {
                  target--;
                  GT_SetGadgetAttrs(lvgad[target],win,NULL,GTLV_Labels,~0L,TAG_END);
                  if ((node = AllocMem(sizeof(struct Node),MEMF_CLEAR | MEMF_PUBLIC)) != 0)
                  {
                    node->ln_Name = in.in_Name;
                    InsertAt(&list[target],node,dm->dm_TargetEntry);
                  }
                  GT_SetGadgetAttrs(lvgad[target],win,NULL,GTLV_Labels,&list[target],TAG_END);
                }
                break;
              case 3:
                DisplayBeep(NULL);
                break;
              case 4:
              case 5:
                source = dm->dm_Gadget->GadgetID-1;
                GT_SetGadgetAttrs(lvgad[source],win,NULL,GTLV_Labels,~0L,TAG_END);
                if (target == 4)
                {
                  if ((node = AllocMem(sizeof(struct Node),MEMF_CLEAR | MEMF_PUBLIC)) != 0)
                  {
                    node->ln_Name = ((struct ImageNode *)dm->dm_Object.od_Object)->in_Name;
                    AddTail(&list[source],node);
                  }
                }
                else
                {
                  Remove((struct Node *)dm->dm_Object.od_Object);
                  FreeMem(dm->dm_Object.od_Object,sizeof(struct Node));
                }
                GT_SetGadgetAttrs(lvgad[source],win,NULL,GTLV_Labels,&list[source],TAG_END);
                break;
            }
          }
          else if (!dm->dm_Object.od_Owner)
          {
            DisplayBeep(NULL);   // unsupported drag?!
          }
          else if (!stricmp(dm->dm_Object.od_Owner,"dragtest"))
          {
            char t[256];

            if (GTD_GetString(&dm->dm_Object,t,256))
              ErrorRequest("I do not want a node from a clone of mine.\nEspecially, I don't like \"%s\".",t);
          }
          else if (!strcmp(dm->dm_Object.od_Owner,"intuition"))
            ErrorRequest("An object from a custom gadget?\nThat's nothing for me.",dm->dm_Object.od_Owner);
          else
            ErrorRequest("An object from %s?\nSounds interesting.",dm->dm_Object.od_Owner);
        }
        else
          DisplayBeep(NULL);
      }
      GTD_ReplyIMsg(msg);

      switch(imsg.Class)
      {
        case IDCMP_GADGETDOWN:
          break;
        case IDCMP_GADGETUP:
          switch(((struct Gadget *)imsg.IAddress)->GadgetID)
          {
            case 3:
              GT_SetGadgetAttrs(lvgad[0],win,NULL,GTLV_Labels,~0L,TAG_END);
              if ((node = AllocMem(sizeof(struct Node),MEMF_CLEAR | MEMF_PUBLIC)) != 0)
              {
                node->ln_Name = in.in_Name;
                AddTail(&list[0],node);
              }
              GT_SetGadgetAttrs(lvgad[0],win,NULL,GTLV_Labels,&list[0],TAG_END);
              break;
          }
          break;
        case IDCMP_CLOSEWINDOW:
          ende = TRUE;
          break;
      }
    }
  }
}

const STRPTR txt[] = {"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20"};

struct Window *initWindow(void)
{
  struct Window *win;

  gad[0] = CreateContext(&glist);

  ng.ng_LeftEdge = 8;
  ng.ng_TopEdge = fontheight+18; //+7
  ng.ng_Width = 192;
  ng.ng_Height = 11*fontheight+4;
  ng.ng_VisualInfo = vi;
  ng.ng_TextAttr = scr->Font;
  ng.ng_GadgetText = NULL;
  ng.ng_UserData = NULL;
  ng.ng_GadgetID = 1;
  lvgad[0] = CreateGadget(LISTVIEW_KIND,gad[0],&ng,GTLV_Labels,&list[0],TAG_END);

  ng.ng_LeftEdge = 206;
  ng.ng_Width = 186;
  ng.ng_GadgetText = NULL;
  ng.ng_GadgetID++;
  lvgad[1] = CreateGadget(LISTVIEW_KIND,lvgad[0],&ng,GTLV_Labels,&list[1],TAG_END);

  ng.ng_TopEdge += ng.ng_Height+3; //+3
  ng.ng_LeftEdge = 8;
  ng.ng_Height = fontheight+4;
  ng.ng_Width = TLn("New")+16;
  ng.ng_Flags = PLACETEXT_IN;
  ng.ng_GadgetText = "New";
  ng.ng_GadgetID++;
  gad[0] = CreateGadget(BUTTON_KIND,lvgad[1],&ng,GA_Immediate,TRUE,TAG_END);

  ng.ng_LeftEdge += ng.ng_Width+6;
  ng.ng_Width = TLn("Copy")+16;
  ng.ng_GadgetText = "Copy";
  ng.ng_GadgetID++;
  gad[1] = CreateGadget(BUTTON_KIND,gad[0],&ng,TAG_END);

  ng.ng_LeftEdge += ng.ng_Width+6;
  ng.ng_Width = TLn("Delete")+16;
  ng.ng_GadgetText = "Delete";
  ng.ng_GadgetID++;
  gad[2] = CreateGadget(BUTTON_KIND,gad[1],&ng,TAG_END);

  if ((win = OpenWindowTags(NULL,WA_Title,   "gtdrag - Test OS4 mit AutoInit",
                                WA_Flags,   WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE,
                                WA_IDCMP,   IDCMP_CLOSEWINDOW | DRAGIDCMP,
                                WA_Width,   400,
                                WA_Height,  fontheight*13+34, //+23
                                WA_Gadgets, glist,
                                TAG_END)) != 0)
  {
    GT_RefreshWindow(win,NULL);
    GTD_AddGadget(LISTVIEW_KIND,lvgad[0],win,GTDA_InternalType, 1,
                                             GTDA_NoPosition,   TRUE,
                                             GTDA_Type,         ODT_NODE,
                                             TAG_END);
    GTD_AddGadget(LISTVIEW_KIND,lvgad[1],win,GTDA_InternalType,1,
                                             GTDA_Same,        TRUE,
                                             GTDA_Type,        ODT_NODE,
                                             TAG_END);
    GTD_AddGadget(BUTTON_KIND,gad[0],win,GTDA_Object,      &in,
                                         GTDA_Image,       in.in_Image,
                                         GTDA_InternalType,2,
                                         GTDA_AcceptTypes, 0,
                                         GTDA_Width,       newImg.Width,
                                         GTDA_Height,      newImg.Height,
                                         TAG_END);
    GTD_AddGadget(BUTTON_KIND,gad[1],win,GTDA_AcceptTypes,1,TAG_END);
    GTD_AddGadget(BUTTON_KIND,gad[2],win,GTDA_AcceptTypes,1,TAG_END);
    GTD_AddWindow(win,TAG_END);
    return(win);
  }
  return(NULL);
}


void cleanup(void)
{
  int i;

  for(i = 0;i < 2;i++)
    while((node = RemHead(&list[i])) != 0)
      FreeMem(node,sizeof(struct Node));
}


void init(void)
{
  int i;

  NewList(&list[0]);
  NewList(&list[1]);

  for(i = 0;i<20;i++)
  {
    if ((node = AllocMem(sizeof(struct Node),MEMF_CLEAR | MEMF_PUBLIC)) != 0)
    {
      node->ln_Name = txt[i];
      AddTail(&list[0],node);
    }
  }
  in.in_Name = "** New **";
  in.in_Image = &newImg;
}


int main(int argc, char **argv)
{
//  if ((GTDragBase = OpenLibrary("gtdrag.library",3)) != 0)
  {
    if (GTD_AddApp("dragtest",GTDA_NewStyle,TRUE,TAG_END))
    {
      if ((scr = LockPubScreen(NULL)) != 0)
      {
        vi = GetVisualInfo(scr,TAG_END);
        fontheight = scr->Font->ta_YSize;
        init();

        if ((win = initWindow()) != 0)
        {
          processMsg();
          CloseWindow(win);
        }
        FreeGadgets(glist);

        cleanup();
        FreeVisualInfo(vi);
        UnlockPubScreen(NULL,scr);
      }
      GTD_RemoveApp();
    }
//    CloseLibrary(GTDragBase);
 }
  return 0;
}
