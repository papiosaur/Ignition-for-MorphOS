//FIXME: license?

#include "types.h"

#define CATCOMP_NUMBERS
#define CATCOMP_ARRAY
#include "ignition_strings.h"

#ifndef __amigaos4__
	CONST_STRPTR ASM GetString(REG(a0, struct LocaleInfo *li), REG(d0, LONG stringNum))
	{
		if (LocaleBase != NULL && li != NULL && li->li_Catalog != NULL) {
			return GetCatalogStr(li->li_Catalog, stringNum, CatCompArray[stringNum].cca_Str);
		}
		else {
			return CatCompArray[stringNum].cca_Str;
		}
		return 0;
	}
#endif
