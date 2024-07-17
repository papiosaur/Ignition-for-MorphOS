/* Implementation of variadic "library" functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

#include "gclass_protos.h"

#ifdef __amigaos4__
#include <stdarg.h>

ULONG gDoMethod(APTR go,  ...) VARARGS68K;
ULONG gDoMethod(APTR go, ...)
{
	va_list ap;
	Msg para;

	va_startlinear(ap, go);
	para = va_getlinearva(ap, Msg);

	return gDoMethodA(go, para);
}

ULONG gDoSuperMethod(struct gClass *gc, APTR go, ...) VARARGS68K;
ULONG gDoSuperMethod(struct gClass *gc, APTR go, ...)
{
	va_list ap;
	Msg para;

	va_startlinear(ap, go);
	para = va_getlinearva(ap, Msg);
	return gDoSuperMethodA(gc, go, para);
}

struct FontInfo *SetFontInfo(struct FontInfo *fi, ULONG dpi,  ...)
{
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, dpi);
	tags = va_getlinearva(ap, struct TagItem *);

	return SetFontInfoA(fi, dpi, tags);
}
#else
ULONG gDoMethod(APTR go, LONG data, ...)
{
	return gDoMethodA(go, &data);
}

ULONG gDoSuperMethod(struct gClass *gc, APTR go, LONG data, ...)
{
	return gDoSuperMethodA(gc, go, &data);
}

struct FontInfo *SetFontInfo(struct FontInfo *fi, ULONG dpi, ULONG tag1, ...)
{
	return SetFontInfoA(fi, dpi, &tag1);
}
#endif
