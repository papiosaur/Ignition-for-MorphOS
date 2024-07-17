/* Structures, definitions, etc.
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_TYPES_H
#define IGN_TYPES_H


#define INTUI_V36_NAMES_ONLY
#define __USE_SYSBASE
#define __TIMER_STDLIBBASE__

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/sghooks.h>
#include <intuition/cghooks.h>
//#include <gadgets/colorwheel.h>
//#include <gadgets/gradientslider.h>
#ifdef __amigaos4__
	#include <amiga_compiler.h>
#else
	#include <gadgets/TextEdit.h>
#endif
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <diskfont/diskfonttag.h>
#include <diskfont/glyph.h>
#include <diskfont/oterrors.h>
#include <libraries/amigaguide.h>
#include <libraries/iffparse.h>
#include <libraries/gadtools.h>
#include <libraries/gtdrag.h>
#include <libraries/asl.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/gtdrag.h>
#include <proto/console.h>
#include <proto/bullet.h>
#include <proto/colorwheel.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/asl.h>
#include <proto/diskfont.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>
#include <proto/locale.h>
#include <proto/rexxsyslib.h>
#include <proto/icon.h>
#include <proto/amigaguide.h>
#include <proto/wb.h>
#include <clib/alib_protos.h>

#if defined(__AROS__)
#	include <cybergraphx/cybergraphics.h>
#	include <proto/pTextEdit.h>
#	include <proto/mathieeedoubbas.h>
#	include <proto/mathieeedoubtrans.h>
#endif
#ifdef __amigaos4__
#	include <interfaces/cybergraphics.h>
#	include <proto/cybergraphics.h>
#	include <proto/console.h>
#	include <proto/bullet.h>
#else
#	include <cybergraphics/cybergraphics.h>
#	include <pragmas/cybergraphics_pragmas.h>
#	include <pragmas/console_pragmas.h>
#	include <pragmas/bullet_pragmas.h>
#	include <pragmas/TextEdit_pragmas.h>
#	include <pragmas/gtdrag_pragmas.h>
#	include <clib/TextEdit_protos.h>
#	include <mieeedoub.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define TLn(t) TextLength(&scr->RastPort, (const STRPTR)t, strlen(t))
#define foreach(l,v) for(v = (APTR)((struct List *)l)->lh_Head;((struct Node *)v)->ln_Succ;v = (APTR)((struct Node *)v)->ln_Succ)
#define PointInGadget(gad,x,y) (x >= gad->LeftEdge && x <= gad->LeftEdge+gad->Width && y >= gad->TopEdge && y <= gad->TopEdge+gad->Height)
#define RGB32(b) (b << 24L | b << 16L | b << 8L | b)

#define MyNewList(l) NewList((struct List *)(l))
#define MyRemTail(l) RemTail((struct List *)(l))
#define MyRemHead(l) RemHead((struct List *)(l))
#define MyAddTail(l,n) AddTail((struct List *)(l),(struct Node *)(n))
#define MyAddHead(l,n) AddHead((struct List *)(l),(struct Node *)(n))
#define MyEnqueue(l,n) Enqueue((struct List *)(l),(struct Node *)(n))
#define MyFindName(l,s) (APTR)FindName((struct List *)(l),(s))
#define MyRemove(n) Remove((struct Node *)(n))

typedef BYTE int8;
typedef UBYTE uint8;
typedef WORD int16;
typedef UWORD uint16;
typedef LONG int32;
typedef ULONG uint32;

#ifndef __cplusplus
    typedef BOOL bool;
#   define true TRUE
#   define false FALSE
#endif

#ifdef __amigaos4__
	#define PI 3.14159265
	#define PUBLIC ASM SAVEDS
	#define ALIGNED
	#define IPTR ULONG
	#define SETHOOK(hookname, funcname) hookname.h_Entry = (HOOKFUNC)funcname
	#define SETDISPATCHER(classname, funcname) classname->cl_Dispatcher.h_Entry = (HOOKFUNC)funcname
	#define min(a,b) ((a)<(b)?(a):(b))
	#define max(a,b) ((a)>(b)?(a):(b))
  	#define WORD2BE(w) (w)
  	#define LONG2BE(l) (l)
  	#define BE2WORD(w) (w)
  	#define BE2LONG(l) (l)

	#include <proto/gtdrag.h>
#else
	#include "SDI_compiler.h"
	#include "SDI_endian.h"
#endif

#include "debug.h"

#define ERR_OPENSCREEN 1
#define ERR_FALLBACK 2

#define CLASSES_PATH "add-ons"
#define CONFIG_PATH "config"

/*********************** Programme startup *************************/

#define TEMPLATE "FILE/M,WITH/K"
#define OPT_FILE 0
#define OPT_WITH 1
#define NUM_OPTS 2

struct FileArg
{
    struct Node fa_Node;
    BPTR   fa_Lock;
};

#define FAT_FROMWB 0   /* fa_Node.ln_Type */
#define FAT_FROMDOS 1


/*************************** ArrayList ***************************/

struct ArrayList {
        LONG al_Size, al_Last;
        void **al_List;
};

/*************************** Context menu ***************************/

struct ContextMenu {	// Start should be identical with (struct AppMenuEntry) -> Drag&Drop
    struct Node cm_Node;
    STRPTR cm_AppCmd;
    LONG   cm_Width, cm_Begin, cm_End;
};

#define CMT_CELL 0        // Zellen
#define CMT_HORIZTITLE 1  // horizontale Zell-Titel
#define CMT_VERTTITLE 2   // vertikale Zell-Titel
#define CMT_OBJECT 3      // Objekte
#define CMT_MORECELLS 4   // Zell-Auswahl

#define NUM_CMT 5         // falls das mal mehr wird: WDT_PREFCONTEXT, wd_ExtData!

struct NewContextMenu {
	LONG	ncm_Title;
	STRPTR	ncm_AppCmd;
};

/*************************** Private Includes ***************************/

#include "windows.h"
#include "prefs.h"
#include "table.h"
#include "gadgets.h"
#include "calc.h"
#include "graphic.h"
#include "font.h"
#include "io.h"
#ifdef __amigaos4__
	#include "include/compatibility.h"
#else
	#include "compatibility.h"
#endif
																
/*************************** Locale stuff ***************************/

#define CATCOMP_NUMBERS

#if defined(__AROS__)
#	define CATCOMP_ARRAY
#endif

#include "ignition_strings.h"

#ifndef __amigaos4__
CONST_STRPTR ASM GetString(REG(a0, struct LocaleInfo *li), REG(d0, LONG stringNum));
#endif
 
/*************************** Printer ***************************/

struct wdtPrinter {
    struct Node wp_Node;
    UBYTE  wp_PageMode;
    STRPTR wp_Range;
    struct tablePos wp_TablePos;
    long   wp_Copies;
    ULONG  wp_Zoom;
    struct Page *wp_Page;
    UBYTE  wp_Asynchron;
};

#define PRTPM_ALL 0
#define PRTPM_VERTICAL 1
#define PRTPM_HORIZONTAL 2
#define PRTPM_FIRST 3
#define PRTPM_RANGE 4

struct wdtPrintStatus {
    struct Window *wps_Window;
    struct ProgressBar *wps_ProjectBar;
    struct ProgressBar *wps_PageBar;
    struct ProgressBar *wps_SinglePageBar;
    BYTE   wps_Locked;
};

/*************************** Locks ***************************/

struct LockNode {
    struct Node ln_Node;
    struct MinList *ln_List;
    struct MinNode *ln_Object;
    ULONG  ln_Length;
    APTR   *ln_Data;
    ULONG  ASM (*ln_Function)(REG(a0, struct LockNode *), REG(a1, struct MinNode *), REG(d0, UBYTE));
    LONG   ln_Locked;
};

#define LNF_REMOVE 1    // Hook-Function
#define LNF_ADD 2
#define LNF_REFRESH 4

#define LNCMD_LOCK 8
#define LNCMD_UNLOCK 16
#define LNCMD_FREE 32
#define LNCMD_REFRESH 64

#define LNCMDS (LNCMD_REFRESH | LNCMD_LOCK | LNCMD_UNLOCK | LNCMD_FREE)

//#define LNT_REFRESH

#define RXMAP ((APTR)~0L)   // special lock for current project

/********************** Suchen & Ersetzen **********************/

/* searchMode values */

#define SM_REPLACE 1
#define SM_IGNORECASE 2
#define SM_WORDS 4
#define SM_ASK 8
#define SM_PATTERN 16
#define SM_BLOCK 32
#define SM_CELL 64

/* Search-Relation values */

#define SMR_SHIFT 8
#define SMR_MASK (7 << SMR_SHIFT)

#define SMR_LESS (1 << SMR_SHIFT)
#define SMR_EQUALLESS (2 << SMR_SHIFT)
#define SMR_EQUAL (3 << SMR_SHIFT)
#define SMR_EQUALGREATER (4 << SMR_SHIFT)
#define SMR_GREATER (5 << SMR_SHIFT)

struct SearchNode {
    struct Node sn_Node;
    UBYTE  sn_Type;
    STRPTR sn_Text;
    ULONG  sn_Number;
};

#define SNT_TEXT 1
#define SNT_APEN 2
#define SNT_BPEN 3
#define SNT_FORMAT 4
#define SNT_FONT 5
#define SNT_SIZE 6
#define SNT_STYLE 7
#define SNT_HALIGN 8
#define SNT_VALIGN 9
#define SNT_NOTE 10
#define NUM_SNT 11

/* char types */
#define CHAR_NONE 0
#define CHAR_UPPER 1
#define CHAR_LOWER 2
#define CHAR_DIGIT 4
#define CHAR_FIRST ~0L

/************************** Sortieren **************************/

struct sortNode {
    ULONG sn_Number;
    STRPTR sn_Text;
    double sn_Value;
};

/************************* Cut, Copy & Paste *************************/

struct PasteNode {
    struct Node pn_Node;
    struct MinList pn_List;
    ULONG  pn_ID;
    ULONG  pn_Unit;
};

#define PNT_CELLS 0     /* pn_Node.ln_Type */
#define PNT_OBJECTS 1

#define PNF_SELECTED 1  /* pn_Node.ln_Pri */

#define CCC_CUT 1       /* CutCopyClip()-Modes */
#define CCC_COPY 2
#define CCC_CURRENT 4
#define CCC_CELLS 8
#define CCC_OBJECTS 16
#define CCC_HOTSPOT (CCC_OBJECTS | CCC_CELLS)
#define CCC_DELETE 32
#define CCC_TEXTONLY 64

#define PC_TEXTONLY 1
#define PC_CLIPBOARD 2

#define ID_FTXT  MAKE_ID('F','T','X','T')
#define ID_CHRS  MAKE_ID('C','H','R','S')


/************************* Undo & Redo *************************/

struct UndoNode {
    struct Node un_Node;
    UBYTE  un_Type;
	struct MinList un_UndoList;
	struct MinList un_RedoList;
	union {
		struct {
			struct tablePos TablePos;
		} block;
		struct {
			struct tablePos tp;
			ULONG mmUndo, mmRedo;	 // CellSize: width, height for redo
		} mm;
		struct {
			struct tablePos tp;
			ULONG mmWidth, mmHeight;
		} cellsize;
/*		  struct {
			struct tablePos tp;
			ULONG UndoTags, RedoTags;
		} object;*/
		struct {
			struct gObject *Object;
			struct point2d *UndoKnobs, *RedoKnobs;
			uint8 FreeType;
		} object_size;
		struct {
			struct gObject *Object;
			LONG UndoPosition, RedoPosition;
		} object_depth;
		struct {
			LONG MoveDeltaX, MoveDeltaY;
		} object_move;
		struct {
			struct gObject *Object;
			LONG PointNumber;
			struct point2d UndoPoint;
			struct point2d RedoPoint;
		} object_knob;
		struct {
			struct gObject *Object;
			struct TagItem *UndoTags, *RedoTags;
		} object_attrs;
		struct {
			struct gDiagram *UndoDiagram, *RedoDiagram;
		} diagram;
	} u;
	ULONG  un_Mode;					// ""      /cells changed: mode
};									// InReCells: array of cell sizes

enum undo_types {
	UNT_CELLS_CHANGED = 0,     /* un_Type */
	UNT_BLOCK_CHANGED,
	UNT_INSERT_VERT_CELLS,
	UNT_INSERT_HORIZ_CELLS,
	UNT_REMOVE_VERT_CELLS,
	UNT_REMOVE_HORIZ_CELLS,
	UNT_CELL_SIZE,
	UNT_OBJECT_ATTRS,
	UNT_OBJECTS_MOVE,
	UNT_OBJECT_KNOB,
	UNT_ADD_OBJECTS,
	UNT_REMOVE_OBJECTS,
	UNT_OBJECT_SIZE,
	UNT_OBJECT_DEPTH,
	UNT_DIAGRAM_TYPE,
};

// union-"prettifier"
#define un_TablePos		u.block.TablePos
#define un_mmUndo		u.mm.mmUndo
#define un_mmRedo		u.mm.mmRedo
#define un_mmWidth		u.cellsize.mmWidth
#define un_mmHeight		u.cellsize.mmHeight
#define un_UndoTags		u.object_attrs.UndoTags
#define un_RedoTags		u.object_attrs.RedoTags
#define un_Object		u.object_attrs.Object
#define un_UndoKnobs	u.object_size.UndoKnobs
#define un_RedoKnobs	u.object_size.RedoKnobs
#define un_FreeType		u.object_size.FreeType
#define un_UndoPosition	u.object_depth.UndoPosition
#define un_RedoPosition	u.object_depth.RedoPosition
#define	un_MoveDeltaX	u.object_move.MoveDeltaX
#define un_MoveDeltaY	u.object_move.MoveDeltaY
#define un_UndoPoint	u.object_knob.UndoPoint
#define un_RedoPoint	u.object_knob.RedoPoint
#define un_PointNumber	u.object_knob.PointNumber
#define un_UndoDiagram	u.diagram.UndoDiagram
#define	un_RedoDiagram	u.diagram.RedoDiagram
 
#define UNM_PAGECOLORS	1       /* for UNT_CELLSCHANGED, colors in un_TablePos */

#define UNDO_BLOCK		1           /* un_Node.ln_Type */
#define UNDO_CELL		2
#define UNDO_NOREDO		4
#define UNDO_PRIVATE	8
#define UNDO_MASK		16

/* UNT_CELLSIZE specials */

struct UndoCellSize {
    struct MinNode ucs_Node;
    long   ucs_Position;
    ULONG  ucs_mm;
    long   ucs_Pixel;
    UBYTE  ucs_Flags;
};

#define UCSF_HORIZ 1
#define UCSF_VERT 2
#define UCSF_STANDARD 4

struct UndoLink {
    struct MinNode ul_Node;
    APTR   ul_Link;
};

#define TYPE_UNDO 0
#define TYPE_REDO 1

/*************************** Bars ***************************/

struct IconObj {
    struct ImageNode io_Node;
    STRPTR io_AppCmd;
};

struct ToolObj {
    struct Node to_Node;
    long   to_Type;
    struct Image *to_Image;
    long   to_Width,to_Height;
    STRPTR to_HelpText;
    /* FUNCTION!!! */
};

#define TOT_SPACE 0
#define TOT_IMAGE 1
#define TOT_FONT 2
#define TOT_FONTSIZE 3
#define TOT_PALETTE 4
#define TOT_STYLE 5
#define TOT_AREXX 6

#define TOTSPACE_WIDTH 5

/*************************** Mauszeiger ***************************/

#define POINTER_OBJECT 0
#define POINTER_OBJECTKNOB 1
#define POINTER_COLUMNWIDTH 2
#define POINTER_ROWHEIGHT 3
#define NUM_POINTER 4

#define STANDARD_POINTER 42

/*************************** Kommandos ***************************/

struct IntCmd {
    struct Node ic_Node;
    ULONG  (*ic_Function)(long *);
};

#define ICF_NONE 0
#define ICF_PAGE 1   // must have's in an environment
#define ICF_MSG 2

#define MAX_OPTS 25

#define RC_OK 0L
#define RC_WARN 5L
#define RC_FAIL 10L
#define RC_FATAL 20L

struct AppCmd {
    struct ImageNode ac_Node;
    struct MinList ac_Cmds;
    STRPTR ac_ImageName;
    STRPTR ac_HelpText;
    STRPTR ac_Output;
    STRPTR ac_Guide;
    BYTE   ac_Locked;
};

#ifdef __amigaos4__
	#ifdef __GNUC__
   		#ifdef __PPC__
    		#pragma pack(2)
   		#endif
	#elif defined(__VBCC__)
   		#pragma amiga-align
	#endif
#endif
struct Command {
    struct Command *cmd_Succ;
    struct Command *cmd_Pred;
#if defined(__AROS__)
  #warning FIXME when V1 ABI is out
    char   *cmd_Name;
    UBYTE  cmd_Type;
    BYTE   cmd_Pri;
#else
    UBYTE  cmd_Type;
    BYTE   cmd_Pri;
    char   *cmd_Name;
#endif
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

#define CMDT_INTERN 0
#define CMDT_AREXX 1
#define CMDT_DOS 2

/*************************** Rexx-Ports ***************************/

struct RexxPort {
    struct Node rxp_Node;
    struct MsgPort rxp_Port;
    /*APTR   rxp_TaskBlock;*/
    struct RexxMsg *rxp_Message;
    struct Page *rxp_Page;
};

struct RexxScript {
    struct Node rxs_Node;
    STRPTR rxs_Description;
    STRPTR rxs_Data;
    ULONG  rxs_DataLength;
    struct NotifyRequest rxs_NotifyRequest;
    struct Mappe *rxs_Map;
};

#define RXS_INTERN 0   // types for RunRexxScript()
#define RXS_EXTERN 1

/* if rxs_NotifyRequest.nr_Name equals NULL, there is no
** notification active.
*/

/*************************** Menü ***************************/
struct IgnAppMenu {
    struct Node am_Node;
   	struct MinList am_Items;
};

struct IgnAppMenuEntry {
   	struct Node am_Node;
   	STRPTR am_AppCmd;
    STRPTR am_ShortCut;
   	struct MinList am_Subs;
};

#define AMT_NORMAL 0
#define AMT_BARLABEL 1


/************************* Tastatur **************************/

struct AppKey {
    struct Node ak_Node;
    STRPTR ak_AppCmd;
    ULONG  ak_Class;
    UWORD  ak_Code;
    UWORD  ak_Qualifier;
};

#define AKT_ALWAYS 0  /* ak_Node.ln_Type */
#define AKT_NOEDIT 1
#define AKT_EDITONLY 2

#define IEQUALIFIER_SHIFT (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT)
#define IEQUALIFIER_ALT (IEQUALIFIER_RALT | IEQUALIFIER_LALT)
#define IEQUALIFIER_COMMAND (IEQUALIFIER_RCOMMAND | IEQUALIFIER_LCOMMAND)

#define ESCAPE_KEY 0x1b

/*************************** Session ***************************/

struct Session {
    struct Node s_Node;
    STRPTR s_Path;
    STRPTR s_Filename;
};

/*************************** Declarations ***************************/

extern struct Screen *scr;
extern struct Window *win;
extern struct winData *wd;
extern struct Prefs prefs,recycledprefs;
extern struct TreeList prefstree;
extern struct MsgPort *iport,*rxport,*notifyport;
extern void   *vi;
extern APTR   pool;
extern struct IntuiMessage imsg;
extern struct RexxMsg *rxmsg;
extern struct WBStartup *sm;
extern BPTR   shelldir;
extern APTR   gAmigaGuide;
extern struct glContext *gcgl;
extern struct IntuiText itext;
extern WORD   fontheight,barheight,boxwidth,itemheight,itemwidth;
extern WORD   bborder,lborder,rborder,linelen;
extern struct Gadget *gad;
extern struct NewGadget ngad;
extern struct Image *rightImg,*leftImg,*upImg,*downImg,*popImage;
extern struct Image *pincImage,*logoImage,pincOriginalImage,logoOriginalImage;
extern struct DrawInfo *dri;
extern struct ScreenModeRequester *scrReq;
extern struct FileRequester *fileReq;
extern struct FontRequester *fontReq;
extern STRPTR iconpath,projpath,graphicpath,quote;
extern struct MinList gProjects, events, search, replace, errors;
extern struct MinList intcmds,outputs,colors,history,usedfuncs;
extern struct MinList toolobjs,sizes,locks,rexxports,sessions;
extern struct MinList fonts,families,scrcolors,gclasses;
extern struct MinList fewfuncs,funcs,gfxmods,iotypes,zooms;
extern struct MinList images,clips,infos,refs,gdiagrams,fontpaths;
extern struct Page *rxpage,*calcpage;
extern struct RexxPort *rxp;
extern BPTR   rxout;
extern struct RastPort scrRp,*grp;
extern struct Hook fillHook,renderHook,popUpHook,formelHook,formatHook;
extern struct Hook treeHook,colorHook,selectHook,fileHook,glinkHook,linkHook;
extern struct Hook passwordEditHook;
extern LONG   dithPtrn,gDPI;
extern int32  gWidth, gHeight;
extern long   gXDPI, gYDPI, tf_col, tf_row;
extern ULONG  lvsec,lvmsec,lastsecs,wd_StatusWidth,wd_PosWidth,clipunit;
extern WORD   lventry,fewftype;
extern STRPTR pubname, tf_format, gEditor;
extern struct Node *stdfamily;
extern ULONG  ghelpcnt,stdpointheight,notifysig;
extern Class  *iconobjclass,*framesgclass,*indexgclass,*pagegclass;
extern Class  *pictureiclass,*bitmapiclass,*popupiclass,*buttonclass,*colorgclass;
extern bool   ende, gScreenHasChanged;
extern UWORD  searchMode,calcflags;
extern bool   gIsBeginner, rxquiet, gLockStop;
extern const ULONG pageWidth[],pageHeight[],pageSizes;
extern ULONG  standardPalette[],rxmask,ghostPtrn;
extern struct Locale *loc;
extern struct Library *GTDragBase,*TextEditBase,*CyberGfxBase;
//extern struct Library *ColorWheelBase,*GradientSliderBase;
extern struct Library *BulletBase;
#ifndef __amigaos4__
//	extern struct ExecBase *SysBase;
#endif
extern struct LocaleInfo gLocaleInfo;
extern bool   noabout;

#endif  /* IGN_TYPES_H */
