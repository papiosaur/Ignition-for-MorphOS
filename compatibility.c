/* Implementations of missing functions
 *
 * FIXME: license
 */

#include "include/compatibility.h"
#ifndef __amigaos4__
	#include "SDI_compiler.h"
#endif

#include <strings.h>
#include <stdlib.h>

#include <exec/types.h>

#if !defined(__SASC)

#if !defined(__MORPHOS__)
void swmem(char *dest, char *src, int s)
{
	int i;
	char c;

	for(i=0; i<s; i++) {
		c = dest[i];
		dest[i] = src[i];
		src[i] = c;
	}
}
#endif

void strins(char *to, const char *from)
{
	unsigned int toLength;
	unsigned int fromLength;
	char *newTo;
	char *oldTo;

	if (!to || !from)
		return;

	toLength = strlen(to);
	fromLength = strlen(from);

	newTo = to + fromLength + toLength;
	oldTo = to + toLength;
	toLength++;
	
	while(toLength--) {
		*newTo-- = *oldTo--;
	}

	while(fromLength--) {
		*to++ = *from++;
	}
}


static int dcomp(double *a, double *b)
{
	if(*a>*b)
		return 1;
	else if(*a==*b)
		return 0;
	else
		return -1;
}


void dqsort(double *x, int n)
{
	qsort(x,n,sizeof(double),(void *)(*dcomp));
}

#endif
