#ifndef  CLIB_GTDRAG_PROTOS_H
#define  CLIB_GTDRAG_PROTOS_H
/*
**  $VER: gtdrag_protos.h 3.6 (2.6.99)
**  Includes Release 3.4
**
**  C prototypes. For use with 32 bit integers only.
**
**  Copyright ©1999 pinc Software.
**  All Rights Reserved.
*/

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef  LIBRARIES_GTDRAG_H
#include <libraries/gtdrag.h>
#endif

/*--- functions in v1 or higher ---*/
/**/
/* Message functions */
/**/
struct IntuiMessage *GTD_GetIMsg(struct MsgPort *mp);
void GTD_ReplyIMsg(struct IntuiMessage *msg);
struct IntuiMessage *GTD_FilterIMsg(struct IntuiMessage *);
struct IntuiMessage *GTD_PostFilterIMsg(struct IntuiMessage *);
/**/
/* Handling functions */
/**/
int GTD_AddAppA(STRPTR t,struct TagItem *tag);
int GTD_AddApp(STRPTR t,ULONG tag1,...);
void GTD_RemoveApp(void);
void GTD_AddWindowA(struct Window *win,struct TagItem *tag);
void GTD_AddWindow(struct Window *win,ULONG tag1,...);
void GTD_RemoveWindow(struct Window *);
void GTD_AddGadgetA(ULONG type,struct Gadget *gad,struct Window *win,struct TagItem *tag);
void GTD_AddGadget(ULONG type,struct Gadget *gad,struct Window *win,ULONG tag1,...);
void GTD_RemoveGadget(struct Gadget *);
void GTD_RemoveGadgets(struct Window *);
void GTD_SetAttrsA(APTR gad,struct TagItem *tags);
void GTD_SetAttrs(APTR gad,ULONG tag1,...);
BOOL GTD_GetAttr(APTR gad,ULONG tag,ULONG *storage);
struct Hook *GTD_GetHook(ULONG type);
STRPTR GTD_GetString(struct ObjectDescription *od,STRPTR buf,LONG len);
BOOL GTD_PrepareDrag(struct Gadget *gad,struct gpInput *gpi);
BOOL GTD_BeginDrag(struct Gadget *gad,struct gpInput *gpi);
ULONG GTD_HandleInput(struct Gadget *gad,struct gpInput *gpi);
void GTD_StopDrag(struct Gadget *gad);
/*--- functions in v3 or higher ---*/
/**/
/* Tree functions */
/**/
struct TreeNode *AddTreeNode(APTR pool,struct MinList *tree,STRPTR name,struct Image *im,UWORD flags);
void FreeTreeNodes(APTR pool,struct MinList *list);
void FreeTreeList(APTR pool,struct TreeList *tl);
void CloseTreeNode(struct MinList *main,struct TreeNode *tn);
LONG OpenTreeNode(struct MinList *main,struct TreeNode *tn);
LONG ToggleTreeNode(struct MinList *main,struct TreeNode *tn);
void InitTreeList(struct TreeList *tl);
struct TreeNode *GetTreeContainer(struct TreeNode *tn);
STRPTR GetTreePath(struct TreeNode *tn,STRPTR buffer,LONG len);
struct TreeNode *FindTreePath(struct MinList *tree,STRPTR path);
struct TreeNode *FindTreeSpecial(struct MinList *tree,APTR special);
struct TreeNode *FindListSpecial(struct MinList *list,APTR special);
BOOL ToggleTree(struct Gadget *,struct TreeNode *,struct IntuiMessage *);

#endif   /* CLIB_GTDRAG_PROTOS_H */
