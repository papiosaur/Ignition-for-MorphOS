#ifndef GTDRAG_INTERFACE_DEF_H
#define GTDRAG_INTERFACE_DEF_H
/*
** This file is machine generated from idltool
** Do not edit
*/ 

#include <exec/types.i>
#include <exec/exec.i>
#include <exec/interfaces.i>

STRUCTURE GtdragIFace, InterfaceData_SIZE
	    FPTR IGtdrag_Obtain
	    FPTR IGtdrag_Release
	    FPTR IGtdrag_Expunge
	    FPTR IGtdrag_Clone
	    FPTR IGtdrag_GTD_GetIMsg
	    FPTR IGtdrag_GTD_ReplyIMsg
	    FPTR IGtdrag_GTD_FilterIMsg
	    FPTR IGtdrag_GTD_PostFilterIMsg
	    FPTR IGtdrag_Reserved54
	    FPTR IGtdrag_Reserved60
	    FPTR IGtdrag_GTD_AddAppA
	    FPTR IGtdrag_GTD_AddApp
	    FPTR IGtdrag_GTD_RemoveApp
	    FPTR IGtdrag_GTD_AddWindowA
	    FPTR IGtdrag_GTD_AddWindow
	    FPTR IGtdrag_GTD_RemoveWindow
	    FPTR IGtdrag_GTD_AddGadgetA
	    FPTR IGtdrag_GTD_AddGadget
	    FPTR IGtdrag_GTD_RemoveGadget
	    FPTR IGtdrag_GTD_RemoveGadgets
	    FPTR IGtdrag_GTD_SetAttrsA
	    FPTR IGtdrag_GTD_SetAttrs
	    FPTR IGtdrag_GTD_GetAttr
	    FPTR IGtdrag_GTD_GetHook
	    FPTR IGtdrag_GTD_GetString
	    FPTR IGtdrag_GTD_PrepareDrag
	    FPTR IGtdrag_GTD_BeginDrag
	    FPTR IGtdrag_GTD_HandleInput
	    FPTR IGtdrag_GTD_StopDrag
	    FPTR IGtdrag_FreeTreeList
	    FPTR IGtdrag_InitTreeList
	    FPTR IGtdrag_FreeTreeNodes
	    FPTR IGtdrag_AddTreeNode
	    FPTR IGtdrag_CloseTreeNode
	    FPTR IGtdrag_OpenTreeNode
	    FPTR IGtdrag_ToggleTreeNode
	    FPTR IGtdrag_GetTreeContainer
	    FPTR IGtdrag_GetTreePath
	    FPTR IGtdrag_FindTreePath
	    FPTR IGtdrag_FindTreeSpecial
	    FPTR IGtdrag_FindListSpecial
	    FPTR IGtdrag_ToggleTree
	LABEL GtdragIFace_SIZE

#endif
