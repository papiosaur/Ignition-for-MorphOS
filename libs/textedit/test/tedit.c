;/* tedit.c - Test for the TextEdit-class, 23.9.2000
;**
;** Copyright ©2000 pinc Software. All Rights Reserved.
;** Licensed under the terms of the MIT License.
;**
sc tedit.c link lib:debug.lib ign=73 nodbg data=near parm=b nochkabort nostkchk strmer streq to=ram:tedit
run ram:tedit
quit
*/

#define INTUI_V36_NAMES_ONLY

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
#include <gadgets/TextEdit.h>
#include <libraries/asl.h>
#include <dos/dos.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/dos.h>

#if defined(__SASC)
#	include <clib/alib_stdio_protos.h>
#	include <clib/TextEdit_protos.h>
#	include <pragmas/TextEdit_pragmas.h>
#else
#	include <proto/pTextEdit.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern void kprintf(STRPTR,...);


struct Screen *scr;
struct Window *win;
struct Gadget *gad,*glist;
APTR   dri;
struct Library *TextEditBase;


struct Library *OpenClass(STRPTR name,LONG version)
{
  struct Library *class;
  char   path[256];

  strcpy(path,"gadgets/");
  strcat(path,name);

  if ((class = OpenLibrary(path,version)) != 0)
    return(class);
  if ((class = OpenLibrary(name,version)) != 0)
    return(class);

  return(NULL);
}


struct Window *CreateWindow(void)
{
  if ((scr = LockPubScreen(NULL)) != 0)
    dri = GetScreenDrawInfo(scr);

  gad = CreateContext(&glist);
  gad = NewObject(NULL,"pinc-editgadget",GA_Left,     8,
                                         GA_Top,      20,
                                         GA_Width,    284,
                                         GA_Height,   270,
                                         GA_DrawInfo, dri,
                                         EGA_Scroller,TRUE,
                                         EGA_AutoIndent,TRUE,
                                         EGA_ShowControls,TRUE,
                                         gad ? GA_Previous : TAG_IGNORE,gad,
                                         GA_ID,       1,
                                         TAG_END);

  if (!(win = OpenWindowTags(NULL,WA_Title,   "TextEdit - Test",
                                WA_Height,  300,
                                WA_Width,   300,
                                WA_Flags,   WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET,
                                WA_IDCMP,   IDCMP_GADGETUP | IDCMP_CLOSEWINDOW,
                                WA_Gadgets, glist,
                                TAG_END)))
  {
    DisposeObject(gad);
    FreeGadgets(glist);
  }
  FreeScreenDrawInfo(scr,dri);
  UnlockPubScreen(NULL,scr);
  return(win);
}


void HandleWindow(void)
{
  struct IntuiMessage *msg;
  BOOL   done = FALSE;

  while(!done)
  {
    WaitPort(win->UserPort);

    while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)) != 0)
    {
      switch(msg->Class)
      {
        case IDCMP_CLOSEWINDOW:
          done = TRUE;
          break;
      }
      ReplyMsg((struct Message *)msg);
    }
  }
}


int main(int argc, char **argv)
{
  if ((TextEditBase = OpenClass("pTextEdit.gadget",0)) != 0)
  {
    kprintf("classbase: %lx\n",TextEditBase);
    Text2Clipboard(0,"Hallo",5);

    if ((win = CreateWindow()) != 0)
    {
      HandleWindow();
      CloseWindow(win);
      DisposeObject(gad);
      FreeGadgets(glist);
    }
    CloseLibrary(TextEditBase);
  }
  return(0);
}

