#ifndef  CLIB_TEXTEDIT_PROTOS_H
#define  CLIB_TEXTEDIT_PROTOS_H

/*
**  $VER: TextEdit_protos.h 1.0 (31.3.99)
**  Includes Release 1.0
**
**  C prototypes. For use with 32 bit integers only.
**
**  Copyright ©1999 pinc Software.
**  All rights Reserved.
*/

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef  GADGETS_TEXTEDIT_H
#include <gadgets/TextEdit.h>
#endif

/*--- functions in V1 or higher ---*/

/* Public entries */
Class *GetClass(void);
void Text2Clipboard(UBYTE clipunit,STRPTR t,LONG len);
STRPTR TextFromClipboard(UBYTE clipunit,APTR pool);
void FreeEditList(struct EditGData *ed);
void PrepareEditText(struct EditGData *ed,struct RastPort *rp,STRPTR t);

#endif   /* CLIB_TEXTEDIT_PROTOS_H */
