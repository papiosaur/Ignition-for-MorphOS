/* :ts=4
 *  $VER: init.c $Revision$ (16-Sep-2013)
 *
 *  This file is part of gtdrag.
 *
 *  Copyright (c) 2013 Hyperion Entertainment CVBA.
 *  All Rights Reserved.
 *
 * $Id$
 *
 * $Log$
 *
 *
 */


#include <exec/exec.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <interfaces/gtdrag.h>
#include <proto/gtdrag.h>
//#include <stdarg.h>


#define MyNewList(l) NewList((struct List *)(l))
#define MyRemHead(l) RemHead((struct List *)(l))
#define MyAddTail(l,n) AddTail((struct List *)(l),(struct Node *)(n))
#define MyAddHead(l,n) AddHead((struct List *)(l),(struct Node *)(n))
#define MyFindName(l,s) (APTR)FindName((struct List *)(l),(s))

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif


struct GTDragBase
{
    struct Library libNode;
    BPTR segList;
    /* If you need more data fields, add them here */
    struct ExecIFace *IExec;
    struct DOSIFace *IDOS;
    struct UtilityIFace *IUtility;
    struct IntuitionIFace *IIntuition;
    struct GadToolsIFace *IGadTools;
    struct GraphicsIFace *IGraphics;
    struct CyberGfxIFace *ICyberGfx;
    struct LayersIFace *ILayers;
    struct Library *DOSBase;
    struct Library *UtilityBase;
    struct Library *IntuitionBase;
    struct Library *GadToolsBase;
    struct Library *GfxBase;
    struct Library *CyberGfxBase;
    struct Library *LayersBase;
  
    struct SignalSemaphore  LockSemaphore;
};
//extern struct ExecIFace *IExec __attribute__((force_no_baserel));
#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

//Functions
extern struct DragApp *GetDragApp(struct GtdragIFace *Self, struct Task *task);
extern struct DragGadget *AddDragGadget(struct GtdragIFace *Self, struct Gadget *gad,struct Window *win,ULONG type);
extern struct DragWindow *FindDragWindow(struct Window *win);
extern void SetWindowAttrs(struct GtdragIFace *Self, struct Window *win, struct TagItem *tags);
extern void PrepareDrag(struct GtdragIFace *Self, BOOL boopsi);
extern void FreeDragGadget(struct GtdragIFace *Self, struct DragGadget *dg);
extern struct DragGadget *FindDragGadget(struct Gadget *gad);
extern struct DragObj *CreateDragObj(struct GtdragIFace *Self, struct DragGadget *dg,int x,int y);
extern struct IntuiMessage * HandleIMsg(struct GtdragIFace *Self, struct DragApp *da,struct IntuiMessage *msg);
extern BOOL PointInGadget(struct GtdragIFace *Self, struct Gadget *gad,WORD x,WORD y);
extern BOOL PointInDragGadget(struct GtdragIFace *Self, struct DragGadget *dg,int x,int y,BOOL lheight);
extern void EndDrag(struct GtdragIFace *Self);
extern void MouseMove(struct GtdragIFace *Self, WORD mousex,WORD mousey);
extern void UpdateDragObj(struct GtdragIFace *Self, struct DragObj *gdo,int x, int y);
extern BOOL UpdateHighlighting(struct GtdragIFace *Self, ULONG class,WORD mousex,WORD mousey,struct DragGadget *sdg);
extern BOOL HighlightLVDragGadget(struct GtdragIFace *Self, struct DragGadget *adg,long class,short y);
extern ULONG DoCustomMethod(struct GtdragIFace *Self, struct Gadget *gad,ULONG method,...);
extern ULONG DoRenderMethod(struct GtdragIFace *Self, struct DragGadget *rdg,WORD x,WORD y,ULONG mode);
extern struct DragGadget *WhichDragGadget(struct GtdragIFace *Self, APTR layer,int mx,int my);
extern BOOL DragGadgetAcceptsObject(struct GtdragIFace *Self, struct DragGadget *adg,WORD x,WORD y);
extern void HighlightDragGadget(struct GtdragIFace *Self, struct DragGadget *adg,WORD x,WORD y);
extern void MakeDropMessage(struct GtdragIFace *Self, struct DragApp *da,ULONG qual,WORD mousex,WORD mousey);
extern void SetObjectOwner(struct DragApp *da,struct Task *t,struct ObjectDescription *od);
extern void FreeDragObj(struct GtdragIFace *Self, struct DragObj *gdo);
extern void IntuiTick(struct GtdragIFace *Self, WORD mousex,WORD mousey);
extern void FreeDropMessage(struct GtdragIFace *Self, struct IntuiMessage *msg);
extern void FillTreeList(struct GtdragIFace *Self, struct MinList *main,struct MinList *list,UBYTE depth,UBYTE flags,ULONG lines);
