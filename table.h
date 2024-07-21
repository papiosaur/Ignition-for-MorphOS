/*
 * Copyright ©1996-2008, pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_TABLE_H
#define IGN_TABLE_H

struct tableSize {
	UWORD  ts_Pixel;
	ULONG  ts_mm;
	STRPTR ts_Title;
	struct tableField *ts_Cell;
};

struct tableField {
	struct MinNode tf_Node;
	uint8  tf_Type;
	uint16 tf_Flags;
	int32  tf_Col,tf_Row;
	uint32 tf_APen,tf_BPen,tf_ReservedPen,tf_NegativePen;
	uint32 tf_BorderColor[4];	/* eventually there is something missing */
	uint8  tf_Border[4];		/* 0-8 pt -> 1/32-steps */
	uint8  tf_Pattern;
	uint32 tf_PatternColor;
	uint8  tf_Alignment;
	int8   tf_Komma;
	struct FontInfo *tf_FontInfo;
	uint16 tf_Width, tf_WidthSet, tf_MaxWidth, tf_OldWidth;	/* free fields to the right */
	STRPTR tf_Text;
	STRPTR tf_Original;
	double tf_Value;
	STRPTR tf_Format;
	struct Term *tf_Root;
	STRPTR tf_Note;
	struct Reference *tf_Reference;
};

#define TFT_TEXT 1
#define TFT_VALUE 2
#define TFT_FORMULA 4	/* ARexx-integration via function */
#define TFT_EMPTY 8		/* for undo/redo */

#define TFF_ACTUAL 1
#define TFF_REFRESH 2
#define TFF_LOCKED 4
#define TFF_FORMATSET 16
#define TFF_KOMMASET 32
#define TFF_SEPARATE 64
#define TFF_NEGPARENTHESES 128
#define TFF_NEGPENSET 256
#define TFF_PENSET 512
#define TFF_FONTSET 1024
#define TFF_IMMUTABLE 2048
#define TFF_STATIC 2048	// deprecated
#define TFF_HIDEFORMULA 4096
#define TFF_HIDETEXT (TFF_IMMUTABLE | TFF_HIDEFORMULA)

#define TFF_SECURITY TFF_HIDETEXT

#define TFA_LEFT 1
#define TFA_RIGHT 2
#define TFA_HCENTER 3
#define TFA_TOP 4
#define TFA_BOTTOM 8
#define TFA_VCENTER 12
#define TFA_VIRGIN 16

#define TF_BORDERSPACE (1 << 8)


struct cellPos {
	LONG cp_Col,cp_Row;
};

struct tablePos {
	LONG tp_Col,tp_Row,tp_Width,tp_Height;
};

#define IsCellInRange(tp,tf) \
	((tp)->tp_Col <= (tf)->tf_Col && (tp)->tp_Row <= (tf)->tf_Row \
	&& ((tp)->tp_Width == -1 || (tp)->tp_Width + (tp)->tp_Col >= (tf)->tf_Col) \
	&& ((tp)->tp_Height == -1 || (tp)->tp_Height + (tp)->tp_Row >= (tf)->tf_Row))

struct coordPkt {
	long cp_X,cp_Y,cp_W,cp_H;
	long cp_Col,cp_Row;
};

/*
 * X,Y,W,H - are values in pixels
 * Col,Row,Width,Height - are values in field positions
 */

struct CellIterator {
	struct tableField *ci_Current;
	LONG   ci_Col,ci_Row,ci_MaxCol,ci_MaxRow;
	struct Page *ci_Page;
	UBYTE  ci_Allocate;
};


struct Event {
	UBYTE  ev_Flags;
	STRPTR ev_Command;
	long   ev_Intervall;
};

#define EVF_NONE 0
#define EVF_ACTIVE 1

#define EVT_START 0
#define EVT_END 1
#define EVT_FIELDSELECT 2
#define EVT_FIELDEND 3
#define EVT_TIME 4
#define EVT_CALC 5
#define EVT_LBUTTON 6
#define EVT_RBUTTON 7
#define NUM_EVENTS 8

#define PG_DINA3 0	  // page sizes
#define PG_DINA4 1
#define PG_DINA5 2
#define PG_USLETTER 3
#define PG_USLEGAL 4

struct Mappe {
	struct Node mp_Node;
	ULONG  mp_Flags;
	STRPTR mp_Path;
	struct MinList mp_Pages;
	ULONG  mp_mmWidth,mp_mmHeight;
	ULONG  mp_MediumWidth,mp_MediumHeight;
	ULONG  mp_mmMediumWidth,mp_mmMediumHeight;
	struct MinList mp_Projects;
	struct Page *mp_actPage;
	struct Window *mp_Window;
	struct Event mp_Events[NUM_EVENTS];
	struct MinList mp_Names;
	struct MinList mp_Databases;
	struct MinList mp_Masks;
	struct MinList mp_Formats;	// links
	struct MinList mp_CalcFormats;
	struct MinList mp_AppCmds;
	struct Prefs mp_Prefs;
	struct IOType *mp_FileType;
	STRPTR mp_Title;
	BYTE   mp_Modified;
	struct MinList mp_RexxScripts;
	STRPTR mp_Author,mp_Version,mp_Note,mp_CatchWords;
	STRPTR mp_Password;
	STRPTR mp_CellPassword;
	struct IBox mp_WindowBox;
	ULONG  mp_PrinterFlags;
	LONG   mp_BorderLeft, mp_BorderRight, mp_BorderTop, mp_BorderBottom;
};

#define MPF_SCRIPTS 1
#define MPF_NOTES 2
#define MPF_PAGEMARKED 4	/* page limits */
#define MPF_PAGEONLY 8
#define MPF_UNNAMED 16
#define MPF_CUSIZE 32		/* Cell-Update */
#define MPF_CUTIME 64
#define MPF_CELLSLOCKED 128
#define MPF_READONLY 256
#define MPF_SAVEWINPOS 512
#define MPF_SHOWHIDDEN 1024
#define MPF_WARNED_IOTYPE 2048

#define MPPRTF_PAGENUMBERS 1
#define MPPRTF_NAME 2
#define MPPRTF_ASYNCHRON 4

#define LNT_MAP 243
#define LNT_PRINTERMAP 242

struct Page {
	struct Node pg_Node;
	struct Window *pg_Window;
	struct Mappe *pg_Document;
	long   pg_Width,pg_Height;
	BYTE   pg_Flags;
	long   pg_Cols,pg_Rows;
	struct MinList pg_Table;					  /* list of tableFields */
	struct tableSize *pg_tfWidth,*pg_tfHeight;	/* array of tableSizes */
	UWORD  pg_wTabX,pg_wTabY,pg_wTabW,pg_wTabH;   /* window borders */
	long   pg_MarkCol,pg_MarkRow,pg_MarkWidth,pg_MarkHeight;
	long   pg_MarkX1,pg_MarkY1,pg_MarkX2,pg_MarkY2;
	long   pg_SelectCol,pg_SelectRow,pg_SelectWidth,pg_SelectHeight;
	struct Rect32 pg_Select;
	WORD   pg_SelectPos,pg_SelectLength;
	struct Node *pg_Family;				  /* standard font family */
	ULONG  pg_PointHeight;				   /* standard font height */
	ULONG  pg_DPI;						   /* bits 31-16: x, 15-0: y */
	struct {
		struct coordPkt cp;
		WORD DispPos;
		WORD FirstChar;
		struct tableField *tf;
		struct tableField *Undo;
	} pg_Gad;
	long   pg_TabW,pg_TabH;				  /* scroller gadgets */
	long   pg_TabX,pg_TabY;
	ULONG  pg_APen,pg_BPen;
	ULONG  pg_Zoom;						  /* in percent */
	UWORD  pg_StdWidth,pg_StdHeight;
	ULONG  pg_mmStdWidth,pg_mmStdHeight;
	double pg_PropFactorX,pg_PropFactorY;	/* avoid prop-gadget overflow */
	double pg_SizeFactorX,pg_SizeFactorY;	/* faster converting between pixel and mm */

	struct MinList pg_gObjects;
	struct MinList pg_gGroups;
	struct MinList pg_gDiagrams;
	UBYTE  pg_HotSpot;
	UBYTE  pg_Action;
	union  {
		struct gClass *pg_CreateFromClass;
		struct gObject *pg_ChangeObject;
	};
	struct coord *pg_Points;
	LONG   pg_NumPoints,pg_CurrentPoint;
	ULONG  pg_NumObjects,pg_OrderSize;
	struct gObject **pg_ObjectOrder;

	struct UndoNode *pg_CurrentUndo;
	struct MinList pg_Undos;
	LONG   pg_Locked;						/* locked by REXX */
	LONG   pg_Modified;
	LONG   pg_CellTextSpace;
};

// TODO: to be removed some day...
#define pg_Mappe pg_Document

#define LNT_PRINTERPAGE 241
						
// Flags
#define PGF_IMMUTABLE	1
#define PGF_HIDDEN		2
#define PGF_SECURITY	(PGF_IMMUTABLE | PGF_HIDDEN)

#define PGS_IGNORE -3		/* TabGadget-Status */
#define PGS_NONE -2
#define PGS_FRAME -1

#define PGA_NONE 0		   /* Action */
#define PGA_CREATE 1
#define PGA_MULTISELECT 2
#define PGA_CHANGE 4
#define PGA_MOVE 8
#define PGA_REFRESH 16
#define PGA_KNOBS 32

#define PGHS_CELL 0		  /* HotSpot */
#define PGHS_OBJECT 1

#define PWA_NONE 0   /* Project Window Actions (wd_ExtData[6]) */
#define PWA_TITLE 1
#define PWA_MARK 2
#define PWA_CELL 3
#define PWA_OBJECT 4
#define PWA_EDITFUNC 5
#define PWA_CELLSIZE 6

#define HTS_BEGIN 1  /* HideTableSpecial()-Modes */
#define HTS_END 2

#define CCS_STANDARD 0   /* ChangeCellSize()-Modes */
#define CCS_OPTWIDTH 1
#define CCS_OPTHEIGHT 2
#define CCS_MINWIDTH 4
#define CCS_MINHEIGHT 8


/*************************** Prototypes ***************************/

// cell.h
extern struct tableField *NextCell(ULONG handle);
extern void FreeCellIterator(ULONG handle);
extern void RewindCellIterator(uint32 handle);
extern ULONG GetCellIterator(struct Page *page,struct tablePos *tp,UBYTE alloc);
extern ULONG GetCellIteratorFromList(struct MinList *list,struct tablePos *tp);
extern struct CellIterator *GetCellIteratorStruct(ULONG handle);
extern struct tableField *GetFirstCell(struct Page *page,struct tablePos *tp);
extern int CompareCellPositions(struct tableField **atf,struct tableField **btf);
extern void SetTFWidth(struct Page *,struct tableField *);
extern void UpdateCellText(struct Page *page,struct tableField *tf);
extern void SetTFText(struct Page *page,struct tableField *tf,STRPTR t);
extern int32 GetTFWidth(struct Page *,long);
extern int32 GetTFHeight(struct Page *,long);
extern long GetTotalWidth(struct Page *page,struct tableField *tf);
extern struct tableField *NextTableField(struct tableField *tf);
extern struct tableField *GetRealTableField(struct Page *page,long x,long y);
extern struct tableField *EmptyTableField(LONG col,LONG row,ULONG width);
extern void RemoveCell(struct Page *page,struct tableField *tf, bool recalc);
extern void InsertCell(struct Page *page,struct tableField *tf, bool updateText);
extern void FreeTableField(struct tableField *tf);
extern struct tableField *MakeTableField(struct Page *page,long col,long row);
extern struct tableField * PUBLIC AllocTableField(REG(a0, struct Page *page),REG(d0, long x),REG(d1, long y));
extern struct tableField *CopyCell(struct Page *,struct tableField *);
extern bool TextIsFormula(STRPTR text);

// table.c
extern void InReCeKn(struct Term *t,long offset,long diff,long comp,long first,long last);
extern long InReCells(struct Page *,UBYTE,long,long,long,long,LONG *);
extern void SetBorder(struct Page *page,BOOL block,long col0,long point0,long col1,long point1,long col2,long point2,long col3,long point3);
extern long CopyBlock(struct Page *,BOOL,BOOL);
extern long InsertBlock(struct Page *page,long col,long row);
extern void ChangeCellSize(struct Page *page,STRPTR width,STRPTR height,UWORD mode,struct UndoNode *undo);
extern long PUBLIC pixel(REG(a0, struct Page *page),REG(d0, long mm),REG(d1, BOOL width));
extern long PUBLIC mm(REG(a0, struct Page *page),REG(d0, long pixel),REG(d1, BOOL width));
extern struct tableField *GetMarkedFields(struct Page *page,struct tableField *tf,BOOL);
extern struct tableField * PUBLIC GetTableField(REG(a0, struct Page *page),REG(d0, long x),REG(d1, long y));
extern struct tableField *GetTableFieldCoord(struct Page *page,long x,long y);
extern void RecalcTableSize(struct Page *page);
extern void PUBLIC AllocTableSize(REG(a0, struct Page *page),REG(d0, long w),REG(d1, long h));
extern void LinkCellsToTableSize(struct Page *page);
extern void DrawTableRegion(struct Window *win,struct Page *page,struct Rectangle *refresh,BOOL vertgads,BOOL horizgads);
extern void DrawTableTitles(struct Page *page,UBYTE horiz);
extern void DrawTableCoord(struct Page *page,long x1,long y1,long x2,long y2);
extern void DrawTablePos(struct Page *page,long col,long row,long width,long height);
extern void DrawTableField(struct Page *page,struct tableField *tf);
extern void DrawMarkedCells(struct Page *page, int32 maxColumn);
extern void DrawTable(struct Window *win);
extern void RefreshMarkedTable(struct Page *page,long maxcol,BOOL end);
extern void ScrollTable(struct Window *win,long xoff,long yoff);
extern void DrawTableFrame(struct Window *win,struct winData *wd);
extern void setTableCoord(struct Page *page,struct Rect32 *target,long col,long row,long wid,long hei);
extern struct coordPkt getCoordPkt(struct Page *page,long mouseX,long mouseY);
extern void setCoordPkt(struct Page *page,struct coordPkt *cp,long col,long row);
extern void SetCellCoordPkt(struct Page *page,struct coordPkt *cp,struct tableField *tf,long col,long row);
extern void ShowTable(struct Page *page,struct coordPkt *cp,long col,long row);
extern void FreeTable(struct Page *page);
extern void PUBLIC RecalcPageDPI(REG(a0, struct Page *page));
extern void SetZoom(struct Page *page,ULONG zoom,BOOL force,BOOL draw);

#endif   /* IGN_TABLE_H */

