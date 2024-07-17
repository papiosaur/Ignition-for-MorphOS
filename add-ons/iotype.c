/* ignition I/O-Module-Linkfile
 *
 * Copyright 1996-2015 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

#define __DOS_STDLIBBASE__

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/screens.h>

#include <proto/exec.h>
#include <proto/dos.h>

#if defined(__SASC)
#	include <pragmas/exec_pragmas.h>
#	include <pragmas/dos_pragmas.h>
#endif

#ifndef __amigaos4__
	#include <string.h>
	#include <math.h>
	/*#include <mieeedoub.h>*/
	struct Mappe;
	#include "SDI_compiler.h"
#endif
#include "iotype.h"

// Must always be in sync with the definition in io.h !!!
#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct IOType {
	struct Node io_Node;
	struct MinList io_Descr;
	STRPTR io_Filename;
	STRPTR io_Pattern;
	ULONG  io_BytesUsed;
	char   io_Bytes[32];
	STRPTR io_OriginalBytes;
	UBYTE  io_Flags;
	BPTR   io_Segment;
	long   ASM (*io_Load)(REG(d0, BPTR), REG(a0, struct Mappe *));
	long   ASM (*io_Save)(REG(d0, BPTR), REG(a0, struct Mappe *));
	long   ASM (*io_SetPrefs)(REG(a0, STRPTR));
	STRPTR ASM (*io_GetPrefs)(void);
	void   ASM (*io_OpenPrefsGUI)(REG(a0, struct Screen *));
	void   ASM (*io_ClosePrefsGUI)(void);
	STRPTR io_Short;
	STRPTR io_Prefs;
	STRPTR io_Suffix;
	UBYTE  io_ReadOver;
	};
#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack()
		#endif
	#elif defined(__VBCC__)
		#pragma default-align
	#endif
#endif

#define IOF_WRITEABLE 1
#define IOF_READABLE 2
#define IOF_ASCII 4
#define IOF_FORCEABLE 8
#define IOF_SUBSET 16
#define IOF_NODEFAULT 32


#ifdef __amigaos4__
	struct ExecIFace *IExec;
	struct DOSIFace *IDOS; 
	struct UtilityIFace *IUtility;
	struct LocaleIFace *ILocale;
	struct IntuitionIFace *IIntuition;
	struct GadToolsIFace *IGadTools;
#else
	struct ExecBase *SysBase;
	#ifndef __SASC
		struct Library *DOSBase;
	#endif
	struct Library *MathIeeeDoubBasBase, *MathIeeeDoubTransBase;
	struct Library *UtilityBase, *LocaleBase;
#endif

APTR   pool, ioBase;

/* Modul-Funktionen*/

extern LONG PUBLIC load(REG(d0, BPTR dat), REG(a0, struct Mappe *mp));
extern LONG PUBLIC save(REG(d0, BPTR dat), REG(a0, struct Mappe *mp));
extern LONG PUBLIC setPrefs(REG(a0, STRPTR));
extern STRPTR PUBLIC getPrefs(void);
extern UBYTE PUBLIC hasPrefsGUI(void);
extern void PUBLIC openPrefsGUI(REG(a0, struct Screen *scr));
extern void PUBLIC closePrefsGUI(void);

#ifdef __amigaos4__
__attribute__((used)) const LONG Version=1;


__attribute__((used)) BOOL 
InitModule(struct IOType *io, APTR *functable, APTR mainpool, 
		struct ExecIFace *execIF, 
		struct DOSIFace  *dosIF,
		struct UtilityIFace *utilIF,
		struct LocaleIFace *localeIF, 
		struct IntuitionIFace *intuiIF,
		struct GadToolsIFace *gadtIF,
		APTR mathBase, 
		APTR mathtransBase, 
		long magic)
{

	//Library-Pointer
	IExec = execIF;
	IDOS = dosIF;
	IUtility = utilIF;
	ILocale = localeIF;
	IIntuition = intuiIF;
	IGadTools = gadtIF;
#else
#define MAKE_ID(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | d)

BOOL PUBLIC
InitModule(REG(a0, struct IOType *io), REG(a1, APTR *functable), REG(a2, APTR mainpool), REG(a3, APTR dosBase),
	REG(a6, struct ExecBase *execBase), REG(d0, APTR mathBase), REG(d1, APTR mathtransBase), REG(d2, APTR utilityBase),
	REG(d3, APTR localeBase), REG(d4, long magic))
{
#ifdef IGNITION_LITE_EDITION
	if (magic != MAKE_ID('I', 'G', 'N', '\0') && magic != MAKE_ID('I', 'G', 'L', '\0'))
		return FALSE;
#else
	if (magic != MAKE_ID('I', 'G', 'N', '\0'))
		return FALSE;
#endif

	SysBase = execBase;
	DOSBase = dosBase;
	UtilityBase = utilityBase;
	MathIeeeDoubBasBase = mathBase;
	MathIeeeDoubTransBase = mathtransBase;
	LocaleBase = localeBase;
#endif
	io->io_Load = load;
	io->io_Save = save;
	io->io_SetPrefs = setPrefs;
	io->io_GetPrefs = getPrefs;
	io->io_OpenPrefsGUI = openPrefsGUI;
	io->io_ClosePrefsGUI = closePrefsGUI;

	ioBase = functable;
	pool = mainpool;

	// Initialize function table
	ReportErrorA = functable[0];
	NewPage = functable[1];
	CalculatePageDPI = functable[2];
	NewCell = functable[3];
	UpdateCellText = functable[4];
	SetTableSize = functable[5];
	FindColorPen = functable[6];
	AddPen = functable[7];
	Coord2String = functable[8];
	pixel = functable[9];
	mm = functable[10];
	AllocStringLength = functable[11];
	AllocString = functable[12];
	FreeString = functable[13];
	ita = functable[14];
	ChangeFontInfoA = functable[15];
	AddName = functable[16];

	setPrefs(io->io_Prefs);

	return TRUE;
}

#ifdef __amigaos4__
LONG _start(STRPTR args, LONG arglen, struct ExecBase *SysBase)
{
  //For stupid users who started the lib from CLI

  return RETURN_FAIL;
}
#endif
