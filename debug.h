/* debug functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef DEBUG_H
#define DEBUG_H


#include "types.h"

#ifdef GDEBUG
#	define G(x) x
#else
#	define G(x) ;
#endif

#ifdef DEBUG
extern void kprintf(const char *format, ...);
#	define ASSERT(condition) if (condition)	{} else panic("Assert failed: " #condition "\n");
#	define D(x) x
#	define DD(x,l) {x; Delay(l);}
#	define DTEST(p) {if (p) kprintf("%s %ld: " #p "\n", __FILE__, __LINE__);}
#	define DTESTR(p,r) {if (p) {kprintf("%s %ld: " #p "\n", __FILE__, __LINE__); return r;}}
#	define DTESTRT(p) {if (p) {kprintf("%s %ld: " #p "\n", __FILE__, __LINE__); return;}}
#else
#	define ASSERT(condition) ;
#	define D(x) ;
#	define DD(x,l) ;
#	define DTEST(p) ;
#	define DTESTR(p,r) ;
#	define DTESTRT(p) ;
#endif

#define bug kprintf

#ifdef DEBUG
extern void panic(const char *format, ...);
extern void check_page_pointer(struct Page *page);
extern void check_map_pointer(struct Mappe *map);

extern void dump_diagrams(struct Page *page);
extern void dump_object(struct gObject *object, bool nullPageAllowed);
extern void dump_diagram(struct gDiagram *diagram, bool nullPageAllowed);
#endif
			 
#endif	/* DEBUG_H */

