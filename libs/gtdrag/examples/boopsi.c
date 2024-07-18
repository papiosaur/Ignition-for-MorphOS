;/* boopsi.c - Test for the gtdrag.library, 7.6.1999
;**
;** Copyright ©1999 pinc Software.
;** All rights reserved.
;**
sc boopsi.c lib:debug.lib nodbg nostkchk link ign=73 to=ram:boopsi
run ram:boopsi
quit
*/


/* This example shows how to create a custom (BOOPSI) gadget
** with support for drag&drop.
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

#include "SDI_compiler.h"

#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/sghooks.h>
#include <intuition/cghooks.h>
#include <graphics/gfx.h>
#include <libraries/gadtools.h>
#include <libraries/gtdrag.h>

#if defined(__SASC)
#	include <pragmas/console_pragmas.h>
#	include <pragmas/gtdrag_pragmas.h>
#endif

#include <clib/alib_protos.h>
#include <proto/console.h>
#include <proto/gtdrag.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <strings.h>
#include <math.h>

#if !defined(__SASC)
#define bug kprintf
#endif

#define foreach(l,v) for(v = (APTR)((struct List *)l)->lh_Head;((struct Node *)v)->ln_Succ;v = (APTR)((struct Node *)v)->ln_Succ)
#define IsBoopsiGadget(gad) ((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)

void kprintf(STRPTR,...);

void processMsg(void);

struct Library *GTDragBase;
struct Window *win;
Class  *simplegclass;
long   fontheight;
struct List list;


/********************************** Simple-Gadget **********************************/

struct SimpleGData
{
  struct MinList *sd_List;  /* the list to display */
  ULONG  sd_Items;          /* number of items to be displayed */
  ULONG  sd_Selected;       /* currently selected item */
  ULONG  sd_Range;          /* width/height of an item */
  ULONG  sd_CurrentDrag;    /* currently dragged item */
  BOOL   sd_Direction;      /* horizontal or vertical direction? */
  struct Hook *sd_Hook;     /* hook to render the items */
};

#define SDD_VERTICAL 0
#define SDD_HORIZONTAL 1

#define SDA_List  (TAG_USER | 0x10000)
#define SDA_Items (TAG_USER | 0x10001)
#define SDA_Direction (TAG_USER | 0x10002)   /* one of the SDD_### */


/** draws the simple gadget */

void DrawSimpleGadget(struct RastPort *rp,struct Gadget *gad,struct GadgetInfo *gi,struct SimpleGData *sd,BOOL all)
{
  struct Node *ln;
  struct LVDrawMsg lvdm;
  UWORD  *pens = gi->gi_DrInfo->dri_Pens;
  long   i,width,height;

  /*** draw gadget frame ***/

  if (all)
  {
    struct Image *im;

    if ((im = NewObject(NULL,"frameiclass",IA_Width,     gad->Width,
                                          IA_Height,    gad->Height,
                                          IA_FrameType, FRAME_BUTTON,
                                          TAG_END)) != 0)
    {
      DrawImage(rp,im,gad->LeftEdge,gad->TopEdge);
      DisposeObject(im);
    }
  }
  width = 0;  height = 0;

  /*** render gadet contents, without any optimisations ***/

  if (sd->sd_Direction == SDD_HORIZONTAL)
    width = sd->sd_Range;
  else
    height = sd->sd_Range;

  /* initialize listview callback render hook message */

  lvdm.lvdm_MethodID = LV_DRAW;
  lvdm.lvdm_RastPort = rp;
  lvdm.lvdm_DrawInfo = gi->gi_DrInfo;
  lvdm.lvdm_Bounds.MinX = gad->LeftEdge+4;
  lvdm.lvdm_Bounds.MinY = gad->TopEdge+4;
  lvdm.lvdm_Bounds.MaxX = lvdm.lvdm_Bounds.MinX+(width ? width-1 : gad->Width-9);
  lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY+(height ? height-1 : gad->Height-9);
  lvdm.lvdm_State = LVR_NORMAL;

  if (!sd->sd_List)
    return;

  /* display the whole list */

  SetABPenDrMd(rp,pens[TEXTPEN],pens[FILLPEN],JAM2);
  for(i = 0,ln = (APTR)sd->sd_List->mlh_Head;ln->ln_Succ;ln = ln->ln_Succ,i++)
  {
    if (i >= sd->sd_Items)
      break;

    if (sd->sd_Selected == i)
      lvdm.lvdm_State = LVR_SELECTED;

    CallHookPkt(sd->sd_Hook,ln,&lvdm);

    if (sd->sd_Direction == SDD_HORIZONTAL)
    {
      lvdm.lvdm_Bounds.MinX += width;
      lvdm.lvdm_Bounds.MaxX += width;
    }
    else
    {
      lvdm.lvdm_Bounds.MinY += height;
      lvdm.lvdm_Bounds.MaxY += height;
    }
    lvdm.lvdm_State = LVR_NORMAL;
  }
}

/** dispatch the OM_NEW method */

BOOL DispatchSimpleNew(struct opSet *ops,Object *o,struct SimpleGData *sd)
{
  SetAttrs(o,GA_Immediate,TRUE,TAG_END);
  sd->sd_List = (APTR)GetTagData(SDA_List,0,ops->ops_AttrList);
  sd->sd_Items = GetTagData(SDA_Items,1,ops->ops_AttrList);
  sd->sd_Direction = GetTagData(SDA_Direction,SDD_VERTICAL,ops->ops_AttrList);
  sd->sd_Hook = GTD_GetHook(GTDH_IMAGE);
  sd->sd_Selected = sd->sd_CurrentDrag = ~0L;

  if (!sd->sd_Items)
    return(FALSE);

  sd->sd_Range = (sd->sd_Direction == SDD_HORIZONTAL ? ((struct Gadget *)o)->Width-8 : ((struct Gadget *)o)->Height-8)/sd->sd_Items;
  return(TRUE);
}

/** dispatch all methods.
 *
 *  To mark the drag&drop related methods, these are separated
 *  from the other and, furthermore, they are commented.
 */

ULONG PUBLIC DispatchSimpleGadget(REG(a0, Class *cl),REG(a2, Object *o),REG(a1, Msg msg))
{
  struct SimpleGData *sd;
  ULONG  retval = 0;

  sd = INST_DATA(cl,o);

  switch(msg->MethodID)
  {
    case OM_NEW:
      if ((retval = DoSuperMethodA(cl,o,msg)) != 0)
      {
        sd = INST_DATA(cl,retval);

        if (!DispatchSimpleNew((struct opSet *)msg,(Object *)retval,sd))
        {
          DoSuperMethod(cl,(Object *)retval,OM_DISPOSE);
          retval = 0;
        }
      }
      break;
    case OM_DISPOSE:
      retval = DoSuperMethodA(cl,o,msg);
      break;
    case OM_SET:
    case OM_UPDATE:
    case OM_NOTIFY:
      DoSuperMethodA(cl,o,msg);
      {
        struct TagItem *tstate,*ti;

        tstate = ((struct opSet *)msg)->ops_AttrList;
        while((ti = NextTagItem(&tstate)) != 0)
        {
          switch(ti->ti_Tag)
          {
            case SDA_List:
            {
              struct RastPort *rp = ObtainGIRPort(((struct opUpdate *)msg)->opu_GInfo);

              sd->sd_List = (APTR)ti->ti_Data;

              DrawSimpleGadget(rp,(struct Gadget *)o,NULL,sd,FALSE);
              ReleaseGIRPort(rp);
              break;
            }
          }
        }
      }
      break;
    case OM_GET:
      if (((struct opGet *)msg)->opg_AttrID == SDA_List)
      {
        *((struct opGet *)msg)->opg_Storage = (ULONG)sd->sd_List;
        retval = TRUE;
      }
      else
        retval = DoSuperMethodA(cl,o,msg);
      break;
    case GM_RENDER:
      {
        struct gpRender *gpr = (struct gpRender *)msg;

        DrawSimpleGadget(gpr->gpr_RPort,(struct Gadget *)o,gpr->gpr_GInfo,sd,TRUE);
      }
      break;
    case GM_HITTEST:
      retval = GMR_GADGETHIT;
      break;

/*** here come drag&drop related method dispatching ***/

    case GM_OBJECTDRAG:             // do we want the object? is some further rendering needed?
    {
      struct gpObjectDrag *gpod = (struct gpObjectDrag *)msg;

      if ((struct Gadget *)o == gpod->gpod_Source)  // get current drag position
      {
        ULONG current;

        if (sd->sd_Direction == SDD_HORIZONTAL)
          current = (gpod->gpod_Mouse.X-4+sd->sd_Range/2)/sd->sd_Range;
        else
          current = (gpod->gpod_Mouse.Y-4+sd->sd_Range/2)/sd->sd_Range;

        retval = GMR_ACCEPTOBJECT | (current != sd->sd_CurrentDrag ? GMR_UPDATE : 0);
        sd->sd_CurrentDrag = current;
      }
      else
      {
        struct Gadget *gad = gpod->gpod_Source;

        if (gad && IsBoopsiGadget(gad) && OCLASS(gad) == cl) // has the source the same class then we have? (simple, no check for super-classes...)
          retval = GMR_ACCEPTOBJECT;                         // we want it all
        else
          retval = GMR_REJECTOBJECT;

        retval |= GMR_FINAL;                // do not send this message again
      }
      break;
    }
    case GM_OBJECTDROP:
      /* we do not process this method, gtdrag will send
      ** the DropMessage to the window's MsgPort if we
      ** return NULL, otherwise it will be catched by us.
      */
      break;
    case GM_RENDERDRAG:
    {
      struct gpRenderDrag *gprd = (struct gpRenderDrag *)msg;

      /* GRENDER_HIGHLIGHT is the only one implemented here; let's
      ** gtdrag do the dirty stuff.
      */

      if (gprd->gprd_Mode == GRENDER_HIGHLIGHT && sd->sd_CurrentDrag != ~0L)
      {
        struct RastPort *rp = gprd->gprd_RPort;
        struct Gadget *gad = (APTR)o;
        int    current = sd->sd_CurrentDrag * sd->sd_Range;

        rp->linpatcnt = 15;  rp->Flags |= FRST_DOT;
        rp->LinePtrn = 0x0f0f;
        SetABPenDrMd(rp,1,2,JAM2);    // use gprd_GInfo->gi_DrInfo->dri_Pens[] instead

        if (sd->sd_Direction == SDD_HORIZONTAL)
        {
          Move(rp,gad->LeftEdge+current+4,gad->TopEdge+2);
          Draw(rp,gad->LeftEdge+current+4,gad->TopEdge+gad->Height-3);
        }
        else
        {
          Move(rp,gad->LeftEdge+4,gad->TopEdge+3+current);
          Draw(rp,gad->LeftEdge+gad->Width-4,gad->TopEdge+3+current);
        }
        retval = TRUE;
      }
      else if (gprd->gprd_Mode == GRENDER_DELETE)
        sd->sd_CurrentDrag = ~0L;                 // reset temporary drag position
      break;
    }
    case GM_GOACTIVE:
    case GM_HANDLEINPUT:
    {
      struct gpInput *gpi = (struct gpInput *)msg;
      struct InputEvent *ie = gpi->gpi_IEvent;
      long x = gpi->gpi_Mouse.X, y = gpi->gpi_Mouse.Y;

      /* test if input is for us or not */

      if ((retval = GTD_HandleInput((struct Gadget *)o,gpi)) != GMR_HANDLEYOURSELF)
        break;
      retval = GMR_MEACTIVE;

      if (ie->ie_Class == IECLASS_RAWMOUSE)
      {
        if (ie->ie_Code == IECODE_RBUTTON)
          retval = GMR_REUSE;
        else if (ie->ie_Code == (IECODE_LBUTTON | IECODE_UP_PREFIX))
        {
          retval = GMR_REUSE;
        }
        else if (ie->ie_Code == IECODE_LBUTTON)
        {
          struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);
          struct Gadget *gad = (APTR)o;

          if (sd->sd_Direction == SDD_HORIZONTAL)
            sd->sd_Selected = (x-4)/sd->sd_Range;
          else
            sd->sd_Selected = (y-4)/sd->sd_Range;

          DrawSimpleGadget(rp,gad,gpi->gpi_GInfo,sd,FALSE);
          ReleaseGIRPort(rp);
        }
        else if (ie->ie_Code == IECODE_NOBUTTON && GTD_PrepareDrag((struct Gadget *)o,gpi))   // mouse movement
        {
          struct Gadget *gad = (struct Gadget *)o;
          int    i = sd->sd_Selected;
          struct Node *ln;

          foreach(sd->sd_List,ln)  // get the current node
          {
            if (--i)
              break;
          }
          GTD_SetAttrs(o,GTDA_Object,     ln,             // object to be dragged
                         GTDA_RenderHook, sd->sd_Hook,    // use this to render the object
                         GTDA_Width,      sd->sd_Direction == SDD_HORIZONTAL ? sd->sd_Range : gad->Width,
                         GTDA_Height,     sd->sd_Direction == SDD_VERTICAL ? sd->sd_Range : gad->Height,
                         GTDA_Type,       ODT_NODE,       // it's basically a simple node
                         GTDA_Same,       TRUE,           // I want to drag&drop my objects over me
                         TAG_END);
          GTD_BeginDrag(gad,gpi);
        }
      }
      break;
    }
    case GM_GOINACTIVE:
    {
      struct gpInput *gpi = (struct gpInput *)msg;
      struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);

      GTD_StopDrag((struct Gadget *)o); // used to break the drag of an object, could also be used in other circumstances
      sd->sd_Selected = ~0L;
      DrawSimpleGadget(rp,(struct Gadget *)o,gpi->gpi_GInfo,sd,FALSE);
      ReleaseGIRPort(rp);
      break;
    }
    default:
      retval = DoSuperMethodA(cl,o,msg);
  }
  return(retval);
}


/********************************** main programme **********************************/


void processMsg(void)
{
  struct IntuiMessage *imsg;
  ULONG  class;
  UWORD  code;
  BOOL   ende = FALSE;

  while(!ende)                /* simple gtdrag message loop */
  {
    imsg = (struct IntuiMessage *)WaitPort(win->UserPort);
    while ((imsg = (struct IntuiMessage *)GTD_GetIMsg(win->UserPort)) != 0)
    {
      class = imsg->Class;
      code = imsg->Code;

      switch(class)
      {
        case IDCMP_CLOSEWINDOW:
          ende = TRUE;
          break;
      }
      GTD_ReplyIMsg(imsg);
    }
  }
}

/** frees the list */

void clean(void)
{
  struct Node *ln;

  while((ln = RemHead(&list)) != 0)
    FreeMem(ln,sizeof(struct ImageNode));
}

const STRPTR text[] = {"abc","def","ghi","jkl","mno","pqr",NULL};

/** initializes the list to be used by the SimpleGadget */

void init(void)
{
  struct ImageNode *in;
  int    i;

  NewList(&list);

  for(i = 0;text[i];i++)
  {
    if ((in = AllocMem(sizeof(struct ImageNode),MEMF_CLEAR)) != 0)
    {
      in->in_Name = text[i];
      AddTail(&list,(APTR)in);
    }
  }
}

/** initializes all stuff is needed */

int main(void)
{
  struct Screen *scr;
  struct DrawInfo *dri;
  void   *vi;
  struct Gadget *hgad,*vgad;

  if (!(simplegclass = MakeClass(NULL,"gadgetclass",NULL,sizeof(struct SimpleGData),0)))
    return 0;

#if defined(__AROS__)
  simplegclass->cl_Dispatcher.h_Entry = HookEntry;
  simplegclass->cl_Dispatcher.h_SubEntry = (HOOKFUNC)DispatchSimpleGadget;
#else
  simplegclass->cl_Dispatcher.h_Entry = (HOOKFUNC)DispatchSimpleGadget;
#endif

  if ((GTDragBase = OpenLibrary("gtdrag.library",3)) != 0)
  {
    if (GTD_AddApp("boopsitest",GTDA_NewStyle,TRUE,TAG_END))
    {
      if ((scr = LockPubScreen(NULL)) != 0)
      {
        fontheight = scr->Font->ta_YSize;
        dri = GetScreenDrawInfo(scr);
        init();
        if ((vi = GetVisualInfo(scr,TAG_END)) != 0)
        {
          if ((hgad = NewObject(simplegclass,NULL,GA_Left,       scr->WBorLeft,
                                                 GA_Top,        fontheight+scr->WBorTop+1,
                                                 GA_Width,      300-scr->WBorRight-scr->WBorLeft,
                                                 GA_Height,     fontheight+9,
                                                 SDA_List,      &list,
                                                 SDA_Items,     5,
                                                 SDA_Direction, SDD_HORIZONTAL,
                                                 TAG_END)) != 0)
          {
            int y;     /* fill the stack with some useless stuff */

            if ((vgad = NewObject(simplegclass,NULL,GA_Left,       scr->WBorLeft,
                                                   GA_Top,        y = 2*fontheight+scr->WBorTop+10,
                                                   GA_Width,      40,
                                                   GA_Height,     200-scr->WBorBottom-y,
                                                   GA_Previous,   hgad,
                                                   SDA_List,      &list,
                                                   SDA_Items,     5,
                                                   SDA_Direction, SDD_VERTICAL,
                                                   TAG_END)) != 0)
            {
              if ((win = OpenWindowTags(NULL,WA_Title,     "gtdrag - Boopsi-Example",
                                            WA_Width,     300,
                                            WA_Height,    200,
                                            WA_Flags,     WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE,
                                            WA_IDCMP,     IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_IDCMPUPDATE | DRAGIDCMP,
                                            WA_Gadgets,   hgad,
                                            WA_PubScreen, scr,
                                            TAG_END)) != 0)
              {
                processMsg();
                RemoveGList(win,hgad,-1);
                CloseWindow(win);
              }
              DisposeObject(vgad);
            }
            DisposeObject(hgad);
          }
          FreeVisualInfo(vi);
        }
        clean();
        FreeScreenDrawInfo(scr,dri);
        UnlockPubScreen(NULL,scr);
      }
      GTD_RemoveApp();
    }
    CloseLibrary(GTDragBase);
  }
  FreeClass(simplegclass);
  return 0;
}


