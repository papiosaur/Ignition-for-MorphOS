;/* treetest.c - Test for the gtdrag.library, 7.6.1999
;**
;** Copyright ©1999 pinc Software.
;** All rights reserved.
;**
sc treetest.c link lib=lib:debug.lib ign=73 nodbg data=near parm=b nochkabort nostkchk strmer streq to=ram:treetest
run ram:treetest
quit
*/


/* This example shows how to use the treeview included in the
** gtdrag-library.
** This is done via a standard gadtools listview and a special
** render hook.
** Furthermore, this example make use of lots of the tree support
** functions of the library.
**
** It's a very simple example but you may use this code to produce
** one at your own.
**
** Although I tried hardly to not produce any errors, I cannot give
** any guaranties. Neither I nor pinc Software is liable for any
** problems you might have with it.
** The usage of this example is on your own risk.
*/


#define INTUI_V36_NAMES_ONLY
#define __USE_SYSBASE

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
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <dos/dostags.h>

#if defined(__SASC)
#	include <clib/alib_stdio_protos.h>
#	include <pragmas/gtdrag_pragmas.h>
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


/**************** some useful definitions ****************/

void kprintf(STRPTR,...);
#define bug kprintf

#define TLn(t) TextLength(&scr->RastPort,t,strlen(t))
#define foreach(l,v) for(v = (APTR)((struct List *)l)->lh_Head;((struct Node *)v)->ln_Succ;v = (APTR)((struct Node *)v)->ln_Succ)


#define PUDDLESIZE 8192                                        // for CreatePool()
#define MULTIVIEW "sys:Utilities/Multiview"                    // "show" programme
#define SEARCH_DIRECTORY "ram:"                                // directory displayed
#define SEARCH_DEPTH 4                                         // depth of the tree
#define OUTPUT "CON://400/200/TreeView-Output/AUTO/CLOSE/WAIT"


/**************** the menu definition ****************/

struct NewMenu NMenu[] =
{
  {NM_TITLE,"Control",         0,0,0,0},
  {NM_ITEM, "Open all",      "O",0,0,0},
  {NM_ITEM, "Close all",     "C",0,0,0},
  {NM_ITEM, NM_BARLABEL,       0,0,0,0},
  {NM_ITEM, "About...",      "?",0,0,0},
  {NM_ITEM, "Quit",          "Q",0,0,0},

  {NM_END,  NULL,          0,0,0,0}
};


/**************** global variables ****************/

struct Screen *scr;
struct Window *win;
struct IntuiMessage imsg;
struct Menu *menu;
struct Library *GTDragBase;
struct Gadget *glist,*lvgad,*gad,*button[2];
struct NewGadget ng;
struct Hook hook;
struct TreeList tl;
APTR   vi,pool;
int    fontheight,itemheight;
BOOL   ende = FALSE;


/**************** Requester ****************/


long DoRequestA(STRPTR t,STRPTR gads,APTR args)
{
  struct EasyStruct es = {sizeof(struct EasyStruct),0,"TreeView-Request",NULL,NULL};

  es.es_TextFormat = t;
  es.es_GadgetFormat = gads;
  return(EasyRequestArgs(scr->FirstWindow,&es,0,args));
}


void ErrorRequest(STRPTR t,...)
{
  DoRequestA(t,"Ok",&t+1);
}


/**************** String functions ****************/


STRPTR AllocString(STRPTR t)  // these will be freed at the end of the programme
{
  STRPTR s;

  if (!t || !*t)
    return(NULL);

  if ((s = AllocPooled(pool,strlen(t)+1)) != 0)
    strcpy(s,t);
  return(s);
}


/**************** list functions ****************/


void CloseAllTrees(struct MinList *tree)
{
  struct TreeNode *tn;

  foreach(tree,tn)
  {
    if (tn->tn_Flags & TNF_CONTAINER)
    {
      CloseAllTrees(&tn->tn_Nodes);
      tn->tn_Flags &= ~TNF_OPEN;
    }
  }
}


/**************** main programme ****************/


void processMsg(void)
{
  struct IntuiMessage *msg;
  struct DropMessage *dm;
  int    source,target;
  int    i;

  while(!ende)
  {
    WaitPort(win->UserPort);
    while((msg = GTD_GetIMsg(win->UserPort)) != 0)
    {
      imsg = *msg;


      if (imsg.Class == IDCMP_OBJECTDROP)      /* handle the IDCMP_OBJECTDROP message */
      {
        if ((dm = imsg.IAddress) && !dm->dm_Object.od_Owner)
        {
          if (dm->dm_Target)
          {
            target = dm->dm_Target->GadgetID;
            source = dm->dm_Gadget->GadgetID;

            if (target > 1 && source == 1)   // Show or execute
            {
              struct TreeNode *tn = TREENODE(dm->dm_Object.od_Object);
              char t[256];

              if (target == 2)
                strcpy(t,MULTIVIEW " ");
              else
                t[0] = 0;
              strcat(t,"\"");
              strcat(t,SEARCH_DIRECTORY);

              if (GetTreePath(tn,t+strlen(t),200))
              {
                BPTR input;

                strcat(t,tn->tn_Node.in_Name);
                strcat(t,"\"");
                if ((input = Open(OUTPUT,MODE_OLDFILE)) != 0)
                {
                  if (SystemTags(t,SYS_Input,     input,
                                   SYS_Output,    NULL,
                                   SYS_Asynch,    TRUE,
                                   SYS_UserShell, TRUE,
                                   TAG_END) == -1)
                  {
                    Close(input);
                    ErrorRequest("Execution failed:\n  %s",t);
                  }
                }
                else
                  ErrorRequest("Could not open output:\n",OUTPUT);
              }
            }
            else if (target == source)     // only possible if target == 1
              ErrorRequest("Sorry, but I can't move anything!");
          }
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
          switch((gad = imsg.IAddress)->GadgetID)
          {
            case 1:
            {
              struct TreeNode *tn;
              long   top;

              for(tn = (APTR)tl.tl_View.mlh_Head,i = 0;i < imsg.Code;tn = (APTR)tn->tn_Node.in_Succ,i++);
              gad->UserData = tn;  tn = TREENODE(tn);

              //ToggleTree(gad,tn,&imsg);

              /** ToggleTree(gad,tn,&imsg) does something like this: **/

              GT_GetGadgetAttrs(gad,win,NULL,GTLV_Top,&top,TAG_END);

              if (MouseOverTreeKnob(tn,gad->TopEdge+2+(imsg.Code-top)*itemheight,&imsg))
              {
                long in,height = (gad->Height-4)/itemheight;

                GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,~0L,TAG_END);

                if ((in = ToggleTreeNode(&tl.tl_View,tn)) && imsg.Code+1+in > top+height)
                  top += imsg.Code+in+1-top-height;

                GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&tl.tl_View,GTLV_Top,top,TAG_END);
              }
              break;
            }
          }
          break;
        case IDCMP_MENUPICK:
          i = 0;
          switch ITEMNUM(imsg.Code)
          {
            case 1:
              i = TNF_OPEN;
            case 0:
            {
              struct TreeNode *ln,*tn;

              GT_SetGadgetAttrs(lvgad,win,NULL,GTLV_Labels,~0L,TAG_END);

              foreach(&tl,ln)
              {
                tn = TREENODE(ln);

                if ((tn->tn_Flags & TNF_OPEN) == i)
                  ToggleTreeNode(&tl,tn);
              }
              if (ITEMNUM(imsg.Code) == 1)      // close all sub-directories, see above
                CloseAllTrees(&tl.tl_Tree);

              GT_SetGadgetAttrs(lvgad,win,NULL,GTLV_Labels,&tl.tl_View,GTLV_Top,0,TAG_END);
              break;
            }
            case 3:
              ErrorRequest("TreeView - a gtdrag example\n\n©1999 pinc Software.\nAll Rights Reserved.");
              break;
            case 4:
              ende = TRUE;
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


struct Window *initWindow(void)
{
  struct Window *win;

  gad = CreateContext(&glist);

  ng.ng_LeftEdge = 8;
  ng.ng_TopEdge = fontheight+7;
  ng.ng_Width = 184;
  ng.ng_Height = 14*itemheight+4;
  ng.ng_VisualInfo = vi;
  ng.ng_TextAttr = scr->Font;
  ng.ng_GadgetText = NULL;
  ng.ng_UserData = NULL;
  ng.ng_GadgetID = 1;
  lvgad = CreateGadget(LISTVIEW_KIND,gad,&ng,GTLV_Labels,&tl,GTLV_CallBack,&hook,GTLV_ItemHeight,itemheight,TAG_END);

  ng.ng_TopEdge += ng.ng_Height;
  ng.ng_LeftEdge = 8;
  ng.ng_Height = fontheight+4;
  ng.ng_Width >>= 1;
  ng.ng_Flags = PLACETEXT_IN;
  ng.ng_GadgetText = "Show";
  ng.ng_GadgetID++;
  button[0] = CreateGadget(BUTTON_KIND,lvgad,&ng,GA_Immediate,TRUE,TAG_END);

  ng.ng_LeftEdge += ng.ng_Width;
  ng.ng_GadgetText = "Execute";
  ng.ng_GadgetID++;
  button[1] = CreateGadget(BUTTON_KIND,button[0],&ng,TAG_END);

  if ((win = OpenWindowTags(NULL,WA_Title,   "gtdrag - TreeView",
                                WA_Flags,   WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE,
                                WA_IDCMP,   IDCMP_CLOSEWINDOW | DRAGIDCMP | IDCMP_MENUPICK,
                                WA_Width,   200,
                                WA_NewLookMenus,TRUE,
                                WA_Height,  itemheight*14+3*fontheight+10,
                                WA_Gadgets, glist,
                                TAG_END)) != 0)
  {
    SetMenuStrip(win,menu);
    GT_RefreshWindow(win,NULL);

    GTD_AddGadget(LISTVIEW_KIND,lvgad,win,GTDA_RenderHook,&hook,
                                          GTDA_Type,      ODT_TREENODE,
                                          GTDA_ItemHeight,itemheight,
                                          GTDA_Same,      TRUE,
                                          GTDA_TreeView,  TRUE,
                                          TAG_END);
    GTD_AddGadget(BUTTON_KIND,button[0],win,TAG_END);
    GTD_AddGadget(BUTTON_KIND,button[1],win,TAG_END);

    return(win);
  }
  return(NULL);
}


void GetDirectoryTree(struct MinList *tree,STRPTR name,int depth)
{
  APTR edbuffer;
  BPTR lock;

  if (!depth)
    return;

  if ((edbuffer = AllocPooled(pool,PUDDLESIZE)) != 0)
  {
    if ((lock = Lock(name,ACCESS_READ)) != 0)
    {
      struct ExAllControl *eac;
      struct ExAllData *ed;
      long   more;

      if ((eac = AllocDosObject(DOS_EXALLCONTROL,NULL)) != 0)
      {
        struct TreeNode *tn;

        do
        {
          more =  ExAll(lock,edbuffer,PUDDLESIZE,ED_TYPE,eac);

          if (eac->eac_Entries == 0)
            break;

          for(ed = edbuffer;ed;ed = ed->ed_Next)
          {
            if (!strcmp(ed->ed_Name+strlen(ed->ed_Name)-5,".info"))
              continue;

            tn = AddTreeNode(pool,tree,AllocString(ed->ed_Name),NULL,(ed->ed_Type == ST_USERDIR ? TNF_CONTAINER | TNF_HIGHLIGHTED : TNF_NONE) | TNF_SORT);

            if (tn && ed->ed_Type == ST_USERDIR && depth > 1)
            {
              BPTR old;

              old = CurrentDir(lock);
              GetDirectoryTree(&tn->tn_Nodes,ed->ed_Name,depth-1);
              CurrentDir(old);
            }
          }
        } while(more);

        FreeDosObject(DOS_EXALLCONTROL,eac);
      }
      UnLock(lock);
    }
    FreePooled(pool,edbuffer,PUDDLESIZE);
  }
}


void init(void)
{
  CopyMem(GTD_GetHook(GTDH_TREE),&hook,sizeof(struct Hook));
  hook.h_Data = (APTR)FindColor(scr->ViewPort.ColorMap,0x44444444,0x44444444,0x44444444,-1);

  NewList((struct List *)&tl.tl_Tree);
  GetDirectoryTree(&tl.tl_Tree,SEARCH_DIRECTORY,SEARCH_DEPTH);

  InitTreeList(&tl);
}


int main(int argc, char **argv)
{
  if ((GTDragBase = OpenLibrary("gtdrag.library",3)))
  {
    if (GTD_AddApp("treetest",GTDA_NewStyle,TRUE,TAG_END))
    {
      if ((pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,PUDDLESIZE,PUDDLESIZE)) != 0)
      {
        if ((scr = LockPubScreen(NULL)) != 0)
        {
          vi = GetVisualInfo(scr,TAG_END);
          fontheight = scr->Font->ta_YSize;
          itemheight = fontheight+2;
          init();

          if ((menu = CreateMenus(NMenu,TAG_END)) != 0)
          {
            if (LayoutMenus(menu,vi,GTMN_NewLookMenus, TRUE,
                                    TAG_END))
            {
              if ((win = initWindow()) != 0)
              {
                processMsg();
                CloseWindow(win);
              }
              FreeGadgets(glist);
              FreeMenus(menu);
            }
          }
          FreeVisualInfo(vi);
          UnlockPubScreen(NULL,scr);
        }
        DeletePool(pool);
      }
      GTD_RemoveApp();
    }
    CloseLibrary(GTDragBase);
  }
  else
    ErrorRequest("Could not open the gtdrag.library v3 or higher.");
  return 0;
}
