/*--- functions in v1 or higher ---*/
/**/
/* Message functions */
/**/
#pragma libcall GTDragBase GTD_GetIMsg 1e 801
#pragma libcall GTDragBase GTD_ReplyIMsg 24 801
#pragma libcall GTDragBase GTD_FilterIMsg 2a 801
#pragma libcall GTDragBase GTD_PostFilterIMsg 30 801
/**/
/* Handling functions */
/**/
#pragma libcall GTDragBase GTD_AddAppA 42 9802
#pragma tagcall GTDragBase GTD_AddApp 42 9802
#pragma libcall GTDragBase GTD_RemoveApp 48 0
#pragma libcall GTDragBase GTD_AddWindowA 4e 9802
#pragma tagcall GTDragBase GTD_AddWindow 4e 9802
#pragma libcall GTDragBase GTD_RemoveWindow 54 801
#pragma libcall GTDragBase GTD_AddGadgetA 5a a98004
#pragma tagcall GTDragBase GTD_AddGadget 5a a98004
#pragma libcall GTDragBase GTD_RemoveGadget 60 801
/*--- functions in v3 or higher ---*/
#pragma libcall GTDragBase GTD_RemoveGadgets 66 801
#pragma libcall GTDragBase GTD_SetAttrsA 6c 9802
#pragma tagcall GTDragBase GTD_SetAttrs 6c 9802
#pragma libcall GTDragBase GTD_GetAttr 72 90803
#pragma libcall GTDragBase GTD_GetHook 78 001
#pragma libcall GTDragBase GTD_GetString 7e 09803
#pragma libcall GTDragBase GTD_PrepareDrag 84 9802
#pragma libcall GTDragBase GTD_BeginDrag 8a 9802
#pragma libcall GTDragBase GTD_HandleInput 90 9802
#pragma libcall GTDragBase GTD_StopDrag 96 801
/**/
/* Tree functions */
/**/
#pragma libcall GTDragBase FreeTreeList 9c 9802
#pragma libcall GTDragBase InitTreeList a2 801
#pragma libcall GTDragBase FreeTreeNodes a8 9802
#pragma libcall GTDragBase AddTreeNode ae 0BA9805
#pragma libcall GTDragBase CloseTreeNode b4 9802
#pragma libcall GTDragBase OpenTreeNode ba 9802
#pragma libcall GTDragBase ToggleTreeNode c0 9802
#pragma libcall GTDragBase GetTreeContainer c6 801
#pragma libcall GTDragBase GetTreePath cc 09803
#pragma libcall GTDragBase FindTreePath d2 9802
#pragma libcall GTDragBase FindTreeSpecial d8 9802
#pragma libcall GTDragBase FindListSpecial de 9802
#pragma libcall GTDragBase ToggleTree e4 A9803

