/* Implementation of variadic "library" functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

#include "iotype.h"
#ifdef __amigaos4__
	#include<stdarg.h>
#endif

void ReportError(STRPTR fmt, ...)
{
#ifdef __amigaos4__
	va_list ap;
	APTR para;

	va_startlinear(ap, fmt);
	para = va_getlinearva(ap, APTR);
	ReportErrorA(fmt, para);
	va_end(ap);
#else
	ReportErrorA(fmt, &fmt + 1);
#endif
}
