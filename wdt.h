/*
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_WDT_H
#define IGN_WDT_H

/*
	Mazze: Moved enum WDTs to its own file because I couldn't
	build it with GCC otherwise.
*/
 
typedef enum                 /* WDT_ window types */
{
	// misc
	WDT_PROJECT = 0,
	WDT_INFO,
	WDT_CLIP,
	WDT_FIND,
	WDT_REPLACE,
	WDT_PRINTER,
	WDT_PRINTSTATUS,
	WDT_FILETYPE,
	WDT_COMMAND,      // 8

	// prefs
	WDT_PREFS,        // 9
	WDT_PREFDISP,
	WDT_PREFSCREEN,
	WDT_PREFCOLORS,
	WDT_PREFICON,
	WDT_PREFTABLE,
	WDT_PREFFILE,
	WDT_PREFPRINTER,
	WDT_PREFCMDS,
	WDT_PREFFORMAT,
	WDT_DEFINECMD,
	WDT_PRESSKEY,
	WDT_PREFMENU,
	WDT_PREFKEYS,
	WDT_PREFSYS,
	WDT_PREFNAMES,
	// WDT_PREFFONT      // not in v1
	// WDT_PREFTOOL      // probably in a later version
	WDT_PREFCONTEXT,
	WDT_PREFCHOICE,

	// map
	WDT_PAGE,            // 27
	WDT_DOCUMENT,
	WDT_PAGESETUP,
	WDT_DOCINFO,
	WDT_ZOOM,
	WDT_SCRIPTS,
	//WDT_TITLES,        // WDT_SETTITLE should be enough...
	WDT_SETTITLE,

	// objects
	WDT_GCLASSES,        // 34
	WDT_OBJECT,
	WDT_DIAGRAM,
	WDT_PREVIEW,
	WDT_GOBJECTS,

	// cell properties
	WDT_CELL,            // 39
	WDT_BORDER,
	WDT_CELLSIZE,
	WDT_NOTES,

	// database
	WDT_INDEX,           // 43
	WDT_DATABASE,
	WDT_MASK,
	WDT_FILTER,

	// calc
	WDT_FORMEL,          // 47
	WDT_SETNAME,

	NUM_WDT

} WDTs;

#endif
