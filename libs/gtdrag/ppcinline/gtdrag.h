/* Automatically generated header! Do not edit! */

#ifndef _PPCINLINE_GTDRAG_H
#define _PPCINLINE_GTDRAG_H

#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif /* !__PPCINLINE_MACROS_H */

#ifndef GTDRAG_BASE_NAME
#define GTDRAG_BASE_NAME GTDragBase
#endif /* !GTDRAG_BASE_NAME */

#define GTD_GetIMsg(__p0) \
	LP1(30, struct IntuiMessage *, GTD_GetIMsg, \
		struct MsgPort *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_ReplyIMsg(__p0) \
	LP1NR(36, GTD_ReplyIMsg, \
		struct IntuiMessage *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_FilterIMsg(__p0) \
	LP1(42, struct IntuiMessage *, GTD_FilterIMsg, \
		struct IntuiMessage *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_PostFilterIMsg(__p0) \
	LP1(48, struct IntuiMessage *, GTD_PostFilterIMsg, \
		struct IntuiMessage *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_AddAppA(__p0, __p1) \
	LP2(66, int , GTD_AddAppA, \
		STRPTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_RemoveApp() \
	LP0NR(72, GTD_RemoveApp, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_AddWindowA(__p0, __p1) \
	LP2NR(78, GTD_AddWindowA, \
		struct Window *, __p0, a0, \
		struct TagItem *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_RemoveWindow(__p0) \
	LP1NR(84, GTD_RemoveWindow, \
		struct Window *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_AddGadgetA(__p0, __p1, __p2, __p3) \
	LP4NR(90, GTD_AddGadgetA, \
		ULONG , __p0, d0, \
		struct Gadget *, __p1, a0, \
		struct Window *, __p2, a1, \
		struct TagItem *, __p3, a2, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_RemoveGadget(__p0) \
	LP1NR(96, GTD_RemoveGadget, \
		struct Gadget *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_RemoveGadgets(__p0) \
	LP1NR(102, GTD_RemoveGadgets, \
		struct Window *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_SetAttrsA(__p0, __p1) \
	LP2NR(108, GTD_SetAttrsA, \
		APTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_GetAttr(__p0, __p1, __p2) \
	LP3(114, BOOL , GTD_GetAttr, \
		APTR , __p0, a0, \
		ULONG , __p1, d0, \
		ULONG *, __p2, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_GetHook(__p0) \
	LP1(120, struct Hook *, GTD_GetHook, \
		ULONG , __p0, d0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_GetString(__p0, __p1, __p2) \
	LP3(126, STRPTR , GTD_GetString, \
		struct ObjectDescription *, __p0, a0, \
		STRPTR , __p1, a1, \
		LONG , __p2, d0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_PrepareDrag(__p0, __p1) \
	LP2(132, BOOL , GTD_PrepareDrag, \
		struct Gadget *, __p0, a0, \
		struct gpInput *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_BeginDrag(__p0, __p1) \
	LP2(138, BOOL , GTD_BeginDrag, \
		struct Gadget *, __p0, a0, \
		struct gpInput *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_HandleInput(__p0, __p1) \
	LP2(144, ULONG , GTD_HandleInput, \
		struct Gadget *, __p0, a0, \
		struct gpInput *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GTD_StopDrag(__p0) \
	LP1NR(150, GTD_StopDrag, \
		struct Gadget *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define AddTreeNode(__p0, __p1, __p2, __p3, __p4) \
	LP5(174, struct TreeNode *, AddTreeNode, \
		APTR , __p0, a0, \
		struct MinList *, __p1, a1, \
		STRPTR , __p2, a2, \
		struct Image *, __p3, a3, \
		UWORD , __p4, d0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeTreeNodes(__p0, __p1) \
	LP2NR(168, FreeTreeNodes, \
		APTR , __p0, a0, \
		struct MinList *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeTreeList(__p0, __p1) \
	LP2NR(156, FreeTreeList, \
		APTR , __p0, a0, \
		struct TreeList *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseTreeNode(__p0, __p1) \
	LP2NR(180, CloseTreeNode, \
		struct MinList *, __p0, a0, \
		struct TreeNode *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OpenTreeNode(__p0, __p1) \
	LP2(186, LONG , OpenTreeNode, \
		struct MinList *, __p0, a0, \
		struct TreeNode *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ToggleTreeNode(__p0, __p1) \
	LP2(192, LONG , ToggleTreeNode, \
		struct MinList *, __p0, a0, \
		struct TreeNode *, __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define InitTreeList(__p0) \
	LP1NR(162, InitTreeList, \
		struct TreeList *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetTreeContainer(__p0) \
	LP1(198, struct TreeNode *, GetTreeContainer, \
		struct TreeNode *, __p0, a0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetTreePath(__p0, __p1, __p2) \
	LP3(204, STRPTR , GetTreePath, \
		struct TreeNode *, __p0, a0, \
		STRPTR , __p1, a1, \
		LONG , __p2, d0, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FindTreePath(__p0, __p1) \
	LP2(210, struct TreeNode *, FindTreePath, \
		struct MinList *, __p0, a0, \
		STRPTR , __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FindTreeSpecial(__p0, __p1) \
	LP2(216, struct TreeNode *, FindTreeSpecial, \
		struct MinList *, __p0, a0, \
		APTR , __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FindListSpecial(__p0, __p1) \
	LP2(222, struct TreeNode *, FindListSpecial, \
		struct MinList *, __p0, a0, \
		APTR , __p1, a1, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ToggleTree(__p0, __p1, __p2) \
	LP3(228, BOOL , ToggleTree, \
		struct Gadget *, __p0, a0, \
		struct TreeNode *, __p1, a1, \
		struct IntuiMessage *, __p2, a2, \
		, GTDRAG_BASE_NAME, 0, 0, 0, 0, 0, 0)

#if defined(USE_INLINE_STDARG) && !defined(__STRICT_ANSI__)

#include <stdarg.h>

#define GTD_AddApp(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	GTD_AddAppA(__p0, (struct TagItem *)_tags);})

#define GTD_AddWindow(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	GTD_AddWindowA(__p0, (struct TagItem *)_tags);})

#define GTD_AddGadget(__p0, __p1, __p2, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	GTD_AddGadgetA(__p0, __p1, __p2, (struct TagItem *)_tags);})

#define GTD_SetAttrs(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	GTD_SetAttrsA(__p0, (struct TagItem *)_tags);})

#endif

#endif /* !_PPCINLINE_GTDRAG_H */
