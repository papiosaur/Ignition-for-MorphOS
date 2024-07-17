/* Automatically generated header! Do not edit! */

#ifndef _INLINE_TEXTEDIT_H
#define _INLINE_TEXTEDIT_H

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef TEXTEDIT_BASE_NAME
#define TEXTEDIT_BASE_NAME TextEditBase
#endif /* !TEXTEDIT_BASE_NAME */

#define GetClass() __GetClass_WB(TEXTEDIT_BASE_NAME)
#define __GetClass_WB(___base) \
	AROS_LC0(Class *, GetClass, \
	struct Library *, (___base), 5, Textedit)

#define Text2Clipboard(___unit, ___string, ___len) __Text2Clipboard_WB(TEXTEDIT_BASE_NAME, ___unit, ___string, ___len)
#define __Text2Clipboard_WB(___base, ___unit, ___string, ___len) \
	AROS_LC3NR(void, Text2Clipboard, \
	AROS_LCA(UBYTE, (___unit), D0), \
	AROS_LCA(STRPTR, (___string), A0), \
	AROS_LCA(LONG, (___len), D1), \
	struct Library *, (___base), 6, Textedit)

#define TextFromClipboard(___unit, ___pool) __TextFromClipboard_WB(TEXTEDIT_BASE_NAME, ___unit, ___pool)
#define __TextFromClipboard_WB(___base, ___unit, ___pool) \
	AROS_LC2(STRPTR, TextFromClipboard, \
	AROS_LCA(UBYTE, (___unit), D0), \
	AROS_LCA(APTR, (___pool), A0), \
	struct Library *, (___base), 7, Textedit)

#define FreeEditList(___ed) __FreeEditList_WB(TEXTEDIT_BASE_NAME, ___ed)
#define __FreeEditList_WB(___base, ___ed) \
	AROS_LC1NR(void, FreeEditList, \
	AROS_LCA(struct EditGData *, (___ed), A0), \
	struct Library *, (___base), 8, Textedit)

#define PrepareEditText(___ed, ___rp, ___string) __PrepareEditText_WB(TEXTEDIT_BASE_NAME, ___ed, ___rp, ___string)
#define __PrepareEditText_WB(___base, ___ed, ___rp, ___string) \
	AROS_LC3NR(void, PrepareEditText, \
	AROS_LCA(struct EditGData *, (___ed), A0), \
	AROS_LCA(struct RastPort *, (___rp), A1), \
	AROS_LCA(STRPTR, (___string), A2), \
	struct Library *, (___base), 9, Textedit)

#endif /* !_INLINE_TEXTEDIT_H */
