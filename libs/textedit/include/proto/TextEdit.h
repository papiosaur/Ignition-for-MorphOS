/* Automatically generated header! Do not edit! */

#ifndef PROTO_TEXTEDIT_H
#define PROTO_TEXTEDIT_H

#include <clib/TextEdit_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/TextEdit.h>
#  else
#   include <inline/TextEdit.h>
#  endif
# else
#  include <pragmas/TextEdit_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/TextEdit.h>
# ifndef __NOGLOBALIFACE__
   extern struct TextEditIFace *ITextEdit;
# endif /* __NOGLOBALIFACE__*/
#else /* !__amigaos4__ */
# ifndef __NOLIBBASE__
   extern struct Library *
#  ifdef __CONSTLIBBASEDECL__
    __CONSTLIBBASEDECL__
#  endif /* __CONSTLIBBASEDECL__ */
   TextEditBase;
# endif /* !__NOLIBBASE__ */
#endif /* !__amigaos4__ */

#endif /* !PROTO_TEXTEDIT_H */
