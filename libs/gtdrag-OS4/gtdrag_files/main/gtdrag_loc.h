#include <proto/gtdrag.h>

#define foreach(l,v) for(v = (APTR)((struct List *)l)->lh_Head;((struct Node *)v)->ln_Succ;v = (APTR)((struct Node *)v)->ln_Succ)
#define words(a,b) ((ULONG)((a) << 16L) | (WORD)(b))


#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif


/********************* DragWindow *********************/

struct DragWindow
{
  struct MinNode dw_Node;
  struct Window *dw_Window;
  struct Task *dw_Task;
  ULONG  dw_AcceptMask;
  BOOL   ASM (*dw_AcceptFunc)(REG(a0, struct Window *),REG(a1, struct Gadget *),REG(a2, struct ObjectDescription *));
};


/********************* DragGadget *********************/

struct DragGadget
{
  struct MinNode dg_Node;
  struct Gadget *dg_Gadget;
  struct Window *dg_Window;
  struct Task *dg_Task;       /* pointer to the owner task */
  struct List *dg_List;       /* listviews only */
  ULONG  dg_Type;
  ULONG  dg_AcceptMask;       /* accept mask for internal types */
  struct Image *dg_Image;     /* image to render */
  struct Hook *dg_RenderHook; /* render hook for the object */
  WORD   dg_ItemHeight;       /* listviews only */
  WORD   dg_Width,dg_Height;
  UWORD  dg_Flags;
  BOOL   ASM (*dg_AcceptFunc)(REG(a0, struct Window *),REG(a1, struct Gadget *),REG(a2, struct ObjectDescription *));
  void   ASM (*dg_ObjectFunc)(REG(a0, struct Window *),REG(a1, struct Gadget *),REG(a2, struct ObjectDescription *),REG(d0, LONG pos));
  struct ObjectDescription dg_Object;
  APTR   dg_CurrentObject;
  ULONG  dg_SourceEntry;
  ULONG  dg_Selected;
};

#define DGF_IMAGES 1        /* Images only, if possible */
#define DGF_NODRAG 2        /* can't be the source of a drag */
#define DGF_SAME 4          /* icon can be dragged over the same gadget */
#define DGF_NOPOS 8         /* no positioning, listview only */
#define DGF_NOSCROLL 16     /* listview cannot be scrolled */
#define DGF_DROPOVER 32     /* item drops over not between other items */
#define DGF_SUPPORTGTD 64   /* flag for boopsi gadgets */
#define DGF_LIKECURRENT 128 /* dg_CurrentObject would be accepted */
#define DGF_TREEVIEW 256    /* treeview specials activated */
#define DGF_ONTHEFLY 512    /* boopsi-gadget are not really registered */
#define DGF_USERENDERING 1024 /* boopsi-gadget highlights itself */
#define DGF_FINAL 2048        /* do not call GM_OBJECTDRAG again */
#define DGF_INTERNALONLY 4096 /* gadget will not accept external objects */
#define DGF_DROPBETWEEN 8192  /* DGF_DROPOVER is only a light version */


/********************* DragApp *********************/

struct DragApp
{
  struct MinNode da_Node;
  struct Task *da_Task;
  STRPTR da_Name;
  UWORD  da_Flags;
  BOOL   da_GTMsg;
};

#define DAF_INTERN 1       /* drag&drop only inside the application */


/********************* DragObj *********************/

struct DragObj
{
  struct DragGadget *do_DragGadget;
  struct Screen *do_Screen;
  int do_DeltaX,do_DeltaY;
  int do_X,do_Y,do_Width,do_Height;
  int do_SX,do_SY;
  int do_PX,do_PY,do_PWidth,do_PHeight;
  struct RastPort *do_ScrRPort;
  struct RastPort do_RPort;
  struct RastPort do_RefreshRPort;
  struct BitMap *do_BitMap;
  struct BitMap *do_SaveBack;
  struct BitMap *do_RefreshMap;
  UWORD *do_Mask;
  UWORD *do_FullShadow,*do_HalfShadow;
};

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif


#define LVDRAGPIXEL 10
#define DRAGPIXEL 3


/********************* global variables *********************/

//extern struct ExecBase *SysBase;
//extern struct IntuitionBase *IntuitionBase;
//extern struct DosLibrary  *DOSBase;
//extern struct GfxBase  *GfxBase;
//extern struct Library  *GadToolsBase;
//extern struct Library  *LayersBase;
//extern struct UtilityBase *UtilityBase;
//extern struct Library  *CyberGfxBase;

extern struct MinList applist;
extern struct MinList gadlist;
extern struct MinList winlist;
extern struct DragObj *gdo;
extern struct DragGadget *dg,*hdg,*adg;
extern struct DropMessage dm;
extern struct BitMap *hbm;
extern struct Gadget *boopsigad;
extern struct RastPort hrp,hsrp;
extern int    mx,my,hx,hy,hw,hh;
extern BOOL   isdragged,fakemsg,noreport;
extern struct MsgPort *dmport;
extern struct SignalSemaphore ListSemaphore;
extern struct Hook iffstreamHook,renderHook,treeHook;

/************************ Functions **************************/
extern int CountNodes(struct List *l);
extern void FakeInputEvent(struct GtdragIFace *Self);
extern struct Node * FindListNumber(struct MinList *l,long num);

