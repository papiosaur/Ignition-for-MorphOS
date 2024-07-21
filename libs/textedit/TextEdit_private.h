/*
 * Copyright ©1999-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */
#ifndef TEXTEDIT_PRIVATE_H
#define TEXTEDIT_PRIVATE_H

#include <exec/nodes.h>
#include <exec/lists.h>

struct EditGData {
	APTR	ed_Pool;
	STRPTR	ed_Text;
	long	ed_Size, ed_Pos, ed_MarkPos, ed_TextLines;
	struct MinList ed_List;
	struct MinNode *ed_Top;
	UWORD	ed_LineHeight, ed_Spacing, ed_MinSpace, ed_MaxSpace, ed_CharWidth;
	UWORD	ed_Width, ed_Lines, ed_MaxSpaces, ed_TabSpaces, ed_GadWidth;
	UBYTE	ed_APen, ed_BPen, ed_BorderH, ed_BorderV;
	UWORD	ed_Flags;
	long	ed_FrameType;
	struct Gadget *ed_Scroller;
	UBYTE	ed_ClipUnit;
};

#endif   // TEXTEDIT_PRIVATE_H
