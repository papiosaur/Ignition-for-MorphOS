/* gClass base module for ignition
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */


#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <libraries/gtdrag.h>

#ifndef __amigaos4__
	#include <proto/exec.h>
	#include <proto/dos.h>
#endif

#if defined(__SASC)
#	include <pragmas/exec_pragmas.h>
#	include <pragmas/dos_pragmas.h>
#elif defined(__AROS__)
#	include <aros/symbolsets.h>
#endif

#include <string.h>
#include <math.h>

#include "gclass.h"
#include "gclass_protos.h"

#ifdef __amigaos4__
	struct ExecIFace *IExec;
	struct DOSIFace *IDOS; 
	struct UtilityIFace *IUtility;
	struct LocaleIFace *ILocale;
	struct GraphicsIFace * IGraphics;
	//struct Library *DOSBase;
#else
struct __gClass {
	struct ImageNode gc_Node;   /* name and icon, only for non-diagrams */
	struct gClass *gc_Super;    /* super class */
	ULONG  gc_InstOffset;
	ULONG  gc_InstSize;
	ULONG  gc_Flags;
	struct gInterface *gc_Interface;
	BPTR   gc_Segment;
	ULONG  ASM (*gc_Dispatch)(REG(a0, struct gClass *), REG(a2, APTR), REG(a1, Msg));
	ULONG  ASM (*gc_Draw)(REG(d0, struct Page *), REG(d1, ULONG), REG(a0, struct RastPort *), REG(a1, struct gClass *), REG(a2, struct gObject *), REG(a3, struct gBounds *));
	ULONG  ASM (*gc_FreeClass)(REG(a0, struct gClass *));
	STRPTR gc_ClassName;        /* internal access (filename) */
};
	struct ExecBase *SysBase;
	struct GfxBase *GfxBase;
	struct UtilityBase *UtilityBase;
	struct LocaleBase *LocaleBase;
	struct Library *MathIeeeDoubBasBase, *MathIeeeDoubTransBase;
#endif
APTR   pool, gcBase;


/* Module functions */

extern ULONG PUBLIC dispatch(REG(a0, struct gClass *gc), REG(a2, APTR obj), REG(a1, Msg msg));
extern ULONG PUBLIC draw(REG(d0, struct Page *), REG(d0, ULONG), REG(a0, struct RastPort *), REG(a1, struct gClass *), REG(a2, struct gObject *), REG(a3, struct gBounds *));
extern ULONG PUBLIC freeClass(REG(a0, struct gClass *gc));
extern ULONG PUBLIC initClass(REG(a0, struct gClass *gc));
extern struct gInterface interface[];
extern ULONG instanceSize;
extern const STRPTR superClass;

#ifdef __amigaos4__
__attribute__((used)) const LONG Version=2;

__attribute__((used)) BOOL 
InitGClass(struct gClass *gc, APTR *functable, APTR mainpool, 
		struct GraphicsIFace * gfxIF, struct ExecIFace *execIF, 
		APTR p1,
		APTR p2,
		struct UtilityIFace *utilif, struct LocaleIFace *localeif, long magic)
{

	//Library-Pointer
	IExec = execIF;
	IGraphics = gfxIF;
	IUtility = utilif;
	ILocale = localeif;
#else
BOOL PUBLIC
InitGClass(REG(a0, struct __gClass *gc), REG(a1, APTR *functable), REG(a2, APTR mainpool), REG(a3, APTR gfxbase),
	REG(a6, struct ExecBase *ExecBase), REG(d0, APTR mathbase), REG(d1, APTR mathtrans), REG(d2, APTR utilitybase), REG(d3, APTR localebase),
    REG(d4, long magic))
{
#ifdef IGNITION_LITE_EDITION
	if (magic != MAKE_ID('I', 'G', 'N', '\0') && magic != MAKE_ID('I', 'G', 'L', '\0'))
		return FALSE;
#else
	if (magic != MAKE_ID('I', 'G', 'N', '\0'))
		return FALSE;
#endif

    SysBase = ExecBase;
    GfxBase = gfxbase;
    UtilityBase = utilitybase;
    MathIeeeDoubBasBase = mathbase;
    MathIeeeDoubTransBase = mathtrans;
    LocaleBase = localebase;
#endif
    gc->gc_Dispatch = dispatch;
    gc->gc_Draw = draw;
    gc->gc_FreeClass = freeClass;
    gc->gc_Interface = interface;
	gc->gc_InstSize = gc->gc_InstOffset + instanceSize;

    gcBase = functable;
    pool = mainpool;

	// Initialize function pointers
#ifdef __amigaos4__
	gcalcllength = functable[39];
	drawSide = functable[38];
	DrawArc = functable[37];
	gAreaArc = functable[36];
	gAreaArcMove = functable[35];
#endif
	AllocStringLength = functable[34];
	AllocString = functable[33];
	FreeString = functable[32];
	gDoMethodA = functable[31];
	gDoSuperMethodA = functable[30];
	SetHighColor = functable[29];
	SetColors = functable[28];
	FindColorPen = functable[27];
	DrawRect = functable[26];
	DrawLine = functable[25];
	gAreaMove = functable[24];
	gAreaDraw = functable[23];
	gAreaEnd = functable[22];
	GetDPI = functable[21];
	GetOffset = functable[20];
	FreeFontInfo = functable[19];
	SetFontInfoA = functable[18];
	CopyFontInfo = functable[17];
	NewFontInfoA = functable[16];
	DrawText = functable[15];
	OutlineLength = functable[14];
	OutlineHeight = functable[13];
	pixel = functable[12];
	mm = functable[11];
	CreateTerm = functable[10];
	DeleteTerm = functable[9];
	CopyTerm = functable[8];
	CalcTerm = functable[7];
	gInsertRemoveCellsTablePos = functable[6];
	gInsertRemoveCellsTerm = functable[5];
	TintColor = functable[4];
	gSuperDraw = functable[3];
	gGetLink = functable[2];
	SetLowColor = functable[1];
	SetOutlineColor = functable[0];

	if (!initClass(gc))
		return FALSE;

	return TRUE;
}
#ifdef __amigaos4__
LONG _start(STRPTR args, LONG arglen, struct ExecBase *SysBase)
{
  //For stupid users who started the lib from CLI

  return RETURN_FAIL;
}
#endif
