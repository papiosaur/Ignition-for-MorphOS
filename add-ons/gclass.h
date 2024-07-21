#ifndef GCLASS_H
#define GCLASS_H

/* $VER: gclass.h 0.14 (6.8.2003)
** Developer Release 0.14
**
** ignition header for gClasses
**
** Copyright ©2003-2008 pinc Software
** All Rights Reserved
*/

#include <utility/tagitem.h>
#include <intuition/intuition.h>

#ifdef __amigaos4__
	#include "../libs/gtdrag-OS4/include/libraries/gtdrag.h"
#else
	#include <libraries/gtdrag.h>
#endif

#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/locale.h>

#ifndef __amigaos4__
	#include "SDI_compiler.h"
#endif

extern APTR gcBase;

#ifndef CELL_H
	#include "cell.h"
#endif

/*** class ***/
#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif

/*** object base ***/

struct gObject
{
  struct Node go_Node;
  struct MinNode go_GroupNode;
  UWORD  go_Type;
  LONG   go_Left,go_Top,go_Right,go_Bottom;
  LONG   go_mmLeft,go_mmTop,go_mmRight,go_mmBottom;
  ULONG  go_Flags;
  struct gClass *go_Class;
  struct point2d *go_Knobs;
  ULONG  go_NumKnobs;
  STRPTR go_Help,go_Command;
  struct Page *go_Page;
  struct Window *go_Window;
  struct UndoNode *go_Undo;
  LONG   go_Pos;              /* <- eher Unique-ID mit Liste */
  struct MinList go_ReferencedBy;
};


#define GINST_DATA(gc,go) (APTR)((UBYTE *)(go)+((struct gClass *)gc)->gc_InstOffset)



#define GOF_SELECTED 1
#define GOF_CONTINUALCMD 2
#define GOF_PROTECTED 4
#define GOF_FRAMED 8
#define GOF_PRESSED 16
#define GOF_LINEMODE 32       /* vermutlich überflüssig */
#define GOF_RECALC 64         /* bei Neuberechnung berücksichtigen */


/*** diagrams ***/

struct gLink
{
  struct MinNode gl_Node;
  struct tableField *gl_Cell;
  ULONG  gl_Color;
  ULONG  gl_Row;
  double gl_Value;
  UBYTE  gl_Flags;
};

#define GLF_HIDDEN 1
#define GLF_MARKED 2
#define GLF_FIRST_OF_ROW 64
#define GLF_LAST_OF_ROW 128


struct gDiagram           // extends gObject
{
    struct gObject gd_Object;
    UBYTE  gd_Flags;
    UBYTE  gd_ReadData;
    STRPTR gd_Range;
    struct tablePos gd_TablePos;
    struct Reference *gd_Reference;
    struct MinList gd_Values;       // gLinks in a list
    struct gLink *gd_Links;
    struct tableField **gd_LegendX;
    struct tableField **gd_LegendY;
    LONG   gd_Cols,gd_Rows;
    struct Page *gd_DataPage;
    ULONG  gd_PageNumber;
};

#define GDF_PREVIEW 1

#define GDRD_VERT 0
#define GDRD_HORIZ 1

/** methods for gDiagrams **/

#define GCDM_SETLINKATTR 30

struct gcpSetLinkAttr  /* GCDM_SETLINKATTR */
{
  ULONG  MethodID;
  struct gLink *gcps_Link;
  ULONG  gcps_Color;
  ULONG  gcps_Marked;
};


/*** interface ***/

struct gInterface
{
  ULONG  gi_Tag;         /* eigener oder vordefinierter Tag für GCM_SET/GCM_GET */
  STRPTR gi_Label; 	/* Bezeichner beim Gadget */
  ULONG  gi_Type;        /* Typ des Gadgets (Checkbox, Cycle, String, ...) */
  APTR   gi_Special;     /* z.B. Auswahl bei Cycle-Gadgets */
  STRPTR gi_Name;        /* für REXX/Funktions-Interface */
};

#define GIT_PEN 1        /* gi_Type */
#define GIT_FONT 2
#define GIT_TEXT 3
#define GIT_CYCLE 4
#define GIT_CHECKBOX 5
#define GIT_FILENAME 6
#define GIT_WEIGHT 7
#define GIT_FORMULA 8


/*** common tags ***/

#define GOA_TagBase TAG_USER+0x1000
#define GOA_Name        GOA_TagBase+0       /* STRPTR */
#define GOA_Text        GOA_TagBase+1       /* STRPTR */
#define GOA_Help        GOA_TagBase+2       /* STRPTR */
#define GOA_Pen         GOA_TagBase+3
#define GOA_FillPen     GOA_TagBase+4
#define GOA_OutlinePen  GOA_TagBase+5
#define GOA_Command     GOA_TagBase+6
#define GOA_ContinualCommand GOA_TagBase+7
#define GOA_FontInfo    GOA_TagBase+8
#define GOA_HasOutline  GOA_TagBase+9
#define GOA_Weight      GOA_TagBase+10

#define GOA_TagUser     GOA_TagBase+0x1000


/*** fonts ***/

struct FontInfo
{
  struct MinNode fi_Node;
  APTR   fi_FontSize;
  struct Node *fi_Family;
  LONG   fi_CharSpace;
  long   fi_Style;
  UBYTE  fi_Kerning;
  long   fi_Locked;
};

#define FS_PLAIN 0         /* Font-Style */
#define FS_BOLD 1
#define FS_ITALIC 2
#define FS_UNDERLINED 4
#define FS_UNSET 65536

#define FS_ITALIC_ANGLE 20
#define FS_BOLD_FACTOR 0x1000

#define FK_NONE 0          /* Kerning */
#define FK_TEXT 1
#define FK_DESIGN 2

#define FA_PointHeight  TAG_USER+100   /* Tags for SetFontInfo() */
#define FA_Space        TAG_USER+101
#define FA_Style        TAG_USER+102
#define FA_Rotate       TAG_USER+103
#define FA_Kerning      TAG_USER+104
#define FA_Family       TAG_USER+105
#define FA_Shear        TAG_USER+106
#define FA_Embolden     TAG_USER+107  /* not yet implemented */
#define FA_Width        TAG_USER+108  /* not yet implemented */

#define FA_FreeReference TAG_USER+120

#define STANDARD_DPI (~0L)


/*** 2D graphics ***/

struct coord
{
  WORD x,y;
};

struct point2d
{
  long x,y;
};

struct gBounds
{
  LONG gb_Left,gb_Top,gb_Right,gb_Bottom;
};

/*** methods ***/

#define GCM_NEW 1           /* gClass Methods */
#define GCM_DISPOSE 2
#define GCM_HITTEST 3
#define GCM_SET 4
#define GCM_GET 5
#define GCM_DUPLICATE 6
#define GCM_UPDATE 7
#define GCM_COMMAND 8
#define GCM_BEGINPOINTS 9
#define GCM_ENDPOINTS 10
#define GCM_UPDATEPOINT 11
#define GCM_ADDPOINT 12
#define GCM_CHANGEPOINT 13
#define GCM_BOX 14
#define GCM_COPY 15
#define GCM_REALPOINTS 16
#define GCM_RECALC 17
#define GCM_ADDTOSCREEN 18
#define GCM_REMOVEFROMSCREEN 19
#define GCM_LOAD 20
#define GCM_SAVE 21
#define GCM_INITAFTERLOAD 23
#define GCM_GETNAMEDATTR 24
#define GCM_INSERTREMOVECELLS 25
#define GCM_UPDATE_UI 26

struct gcpSet            /* GCM_NEW: return object or NULL for failure */
{                        /* GCM_SET: one of the GCPRs below */
  ULONG MethodID;
  struct TagItem *gcps_AttrList;
};

#define GCPR_NONE 0
#define GCPR_REDRAW 1
#define GCPR_UPDATESIZE 2

struct gcpGet            /* GCM_GET: return TRUE or FALSE */
{
  ULONG MethodID;
  ULONG gcpg_Tag;
  ULONG *gcpg_Storage;
};

struct gcpHitTest        /* GCM_HITTEST: return TRUE or FALSE */
{
  ULONG MethodID;
  LONG  gcph_X,gcph_Y;
};

struct gcpUpdate         /* GCM_UPDATE */
{                        /* GCM_BOX */
  ULONG MethodID;
  ULONG gcpu_DPI;
};

struct gcpBeginPoints    /* GCM_BEGINPOINTS: return the number of points needed or */
{                        /*                  ~0L for an undefined number           */
  ULONG MethodID;
  ULONG gcpb_Mode;
};

#define GCPBM_ONE 0
#define GCPBM_MORE 1

struct gcpEndPoints      /* GCM_ENDPOINTS: return number+array of real points */
{
  ULONG  MethodID;
  struct point2d *gcpe_Points;
  ULONG  gcpe_NumPoints;
  struct point2d **gcpe_StoragePoints;
  ULONG  *gcpe_StorageNumPoints;
};

struct gcpUpdatePoint
{
  ULONG  MethodID;
  struct point2d gcpu_Point;
  ULONG  gcpu_NumPoint;    /* number of changed point */
};

struct gcpAddPoint
{
  ULONG  MethodID;
  struct RastPort *gcpa_RastPort;
  struct coord *gcpa_Points;
  struct coord gcpa_Offset;
  ULONG  gcpa_NumPoints;
  ULONG  gcpa_Mode;
  ULONG  gcpa_Point;          /* number of changed point (GCM_CHANGEPOINT only) */
};

#define GCPAM_ONEMORE 0  /* one point is added to the list */
#define GCPAM_UPDATE 1   /* update the drawing (COMPLEMENT) */
#define GCPAM_REDRAW 2   /* redraw the whole drawing */
// #define GCPAM_ERASE 3    /* erase the whole drawing */

struct gcpRealPoints
{
  ULONG  MethodID;
  struct point2d **gcpr_StoragePoints;
  ULONG  *gcpr_StorageNumPoints;
};

struct gcpScreen           /* GCM_ADDTOSCREEN */
{                          /* GCM_REMOVEFROMSCREEN */
  ULONG  MethodID;
  struct Screen *gcps_Screen;
};

struct gcpIO               /* GCM_LOAD */
{                          /* GCM_SAVE */
  ULONG  MethodID;
  struct IFFHandle *gcpio_IFFHandle;
  struct MinList *gcpio_Fonts;
  struct Page *gcpio_Page; /* GCM_LOAD only */
};

struct gcpNamedAttr     /* GCM_GETNAMEDATTR */
{
  ULONG  MethodID;
  STRPTR gcpna_Name;
  struct Result *gcpna_Result;
};

struct gcpInReCells  /* GCM_INSERTREMOVECELLS */
{
  ULONG  MethodID;
  struct Page *gcpc_Page;
  LONG   gcpc_Offset,gcpc_Diff,gcpc_Comp,gcpc_First,gcpc_Last;
};

/*** defines for Axes derivates ***/

/** Axes - Methods **/

#define GCAM_GETBORDERS 1000
#define GCAM_GETCOORD   1001

struct gcapGetBorders
{
  ULONG  MethodID;
  LONG   gcap_Row;
  LONG   *gcap_StorageLeft,*gcap_StorageRight,*gcap_StorageYOrigin;
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

struct gcapGetCoord {
ULONG  MethodID;
double gcap_Y;// value to transform
LONG   gcap_Row;// row of value
LONG   gcap_Scale;// which scale to use (1 or 2) (not yet implemented)
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
/** Axes - Attributes **/

#define GAA_TagBase     GOA_TagBase+100
#define GAA_FontInfo    GAA_TagBase+1  // GIT_FONT
#define GAA_NumberPen   GAA_TagBase+2  // GIT_PEN
#define GAA_Raster      GAA_TagBase+3  // GIT_CYCLE
#define GAA_ShowNumbers GAA_TagBase+4  // GIT_CHECKBOX
#define GAA_BPen        GAA_TagBase+5  // GIT_PEN
#define GAA_Pseudo3D    GAA_TagBase+6  // GIT_CHECKBOX

#define GAA_DepthWidth  GAA_TagBase+7   // (G)
#define GAA_Pseudo3DDepth GAA_TagBase+8 // muß von Objekten mit Pseudo3D-Modus implementiert werden
#define GAA_Bounds      GAA_TagBase+9   // (G)

// suggested offset between different depths
#define GA_DEPTH_OFFSET 1536

struct gClass
{
  struct ImageNode gc_Node;   /* Name und Icon (nur für nicht Diagramme) */
  struct gClass *gc_Super;    /* Zeiger zur Super-Klasse */
  ULONG  gc_InstOffset;
  ULONG  gc_InstSize;
  ULONG  gc_Flags;
  struct gInterface *gc_Interface;
#if 0
  BPTR   gc_Segment;
  ULONG  ASM (*gc_Dispatch)(REG(a0, struct gClass *), REG(a2, APTR), REG(a1, Msg));
  ULONG  ASM (*gc_Draw)(REG(a0, struct Page *), REG(a1, struct gObject *), REG(a2, struct RastPort *), REG(d0, long), REG(d1, long));
  ULONG  (*gc_FreeClass)(void);
  STRPTR gc_ClassName;        /* interner Zugriff (Dateiname) */
#endif
#ifdef __amigaos4__
  BPTR   gc_Segment;
  ULONG  ASM (*gc_Dispatch)(REG(a0, struct gClass *), REG(a2, APTR), REG(a1, /*Msg*/APTR));
  ULONG  ASM (*gc_Draw)(REG(d0, struct Page *), REG(d1, ULONG), REG(a0, struct RastPort *), REG(a1, struct gClass *), REG(a2, struct gObject *), REG(a3, struct gBounds *));
  ULONG  ASM (*gc_FreeClass)(REG(a0, struct gClass *));
  STRPTR gc_ClassName;        /* interner Zugriff (Dateiname) */
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

#ifdef __amigaos4__
	#define PUBLIC
	extern struct ExecIFace *IExec;
	extern struct DOSIFace *IDOS; 
	extern struct UtilityIFace *IUtility;
	extern struct LocaleIFace *ILocale;
#endif
#endif  /* GCLASS_H */

