/* Missing SASC/C functions
 *
 * FIXME: license
 *
 */

#ifndef COMPATIBILITY_H
#define COMPATIBILITY_H

#ifndef __SASC
#if !defined(__MORPHOS__)
void swmem(char *dest, char *src, int s);
#endif
void strins(char *to,const char *from);
void dqsort(double *x, int n);
#endif
			   
#endif	/* COMPATIBILITY_H */

