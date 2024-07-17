/*
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_IO_H
#define IGN_IO_H
					
#ifdef __amigaos4__
#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif
#endif

/*************************** IO-Datatypes ***************************/

struct IOTypeLink {
	struct Node		iol_Node;
	struct IOType	*iol_Link;
	struct MinList	iol_Description;
};
 
// also defined in add-ons/iotype.c

struct IOType {
	struct Node io_Node;
	struct MinList io_Description;
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

#define IOF_WRITEABLE 1
#define IOF_READABLE 2
#define IOF_ASCII 4
// #define IOF_FORCEABLE 8
// #define IOF_SUBSET 16
#define IOF_NODEFAULT 32
#define IOF_HASPREFSGUI 64

#define ID_TABL  MAKE_ID('T','A','B','L')
#define ID_MAP   MAKE_ID('M','P','H','D')
#define ID_PAGE  MAKE_ID('T','B','P','G')
#define ID_PGHD  MAKE_ID('P','G','H','D')
#define ID_CELL  MAKE_ID('C','E','L','L')
#define ID_OBJ   MAKE_ID('O','B','J',' ')
#define ID_FONTS MAKE_ID('F','N','T','S')
#define ID_DB    MAKE_ID('D','B',' ',' ')
#define ID_MASKS MAKE_ID('M','S','K','S')
#define ID_INDEX MAKE_ID('I','N','D','X')
#define ID_FILTER MAKE_ID('F','L','T','R')
#define ID_SCRIPT MAKE_ID('R','S','C','R')
#define ID_MPHD  ID_MAP
#define ID_FNTS  ID_FONTS
#define ID_MSKS  ID_MASKS
#define ID_INDX  ID_INDEX

/*************************** NumberLink ***************************/

struct NumberLink {
	struct Node nl_Node;
	UWORD  nl_Number;
	APTR   nl_Link;
};

/*************************** I/O-Tags ***************************/

#define TAG_NewCell 128
#define TAG_FromAbove 64
#define TAG_FromLeft 32
#define TAG_WithText 16
#define TAG_Empty 0
#define TAG_Position 0xf

#define TAG_BytePos 1   /**/
#define TAG_ByteCol 2   /**/
#define TAG_ByteRow 3   /**/
#define TAG_WordPos 4   /**/
#define TAG_WordCol 5   /**/
#define TAG_WordRow 6   /**/
#define TAG_LongPos 7   /**/
#define TAG_LongCol 8   /**/
#define TAG_LongRow 9   /**/

#define CELL_TEXT 10          /**/
#define CELL_FORMULA 11       /**/
//free
#define CELL_APEN 13          /* 3 */
#define CELL_BPEN 14          /* 3 */
#define CELL_BYTEFORMAT 15    /* 1 */
#define CELL_WORDFORMAT 16    /* 2 */
#define CELL_LONGFORMAT 17    /* 4 */
#define CELL_ALIGN 18         /* 1 */
#define CELL_POINT 19         /* 1 */
#define CELL_NOWIDTH 20       /* 4 */
#define CELL_BYTEFONT 21      /* 1 */
#define CELL_WORDFONT 22      /* 2 */
#define CELL_LONGFONT 23      /* 4 */
#define CELL_NOFORMAT 24      /* 0 */
#define CELL_POINTHEIGHT 25   /* 4 */
#define CELL_STYLE 26         /* 4 */
#define CELL_ROTATE 27
#define CELL_SHEAR 28
#define CELL_FONTWIDTH 29
#define CELL_KERNING 30
#define CELL_SPACE 31
#define CELL_NOTE 32          /* 0 */
#define CELL_NEGPEN 33        /* 3 */
#define CELL_PATTERN 34       /* 1 */
#define CELL_PATTERNCOLOR 35  /* 3 */
#define CELL_LEFTBORDER 36    /* 1 */
#define CELL_RIGHTBORDER 37   /* 1 */
#define CELL_TOPBORDER 38     /* 1 */
#define CELL_BOTTOMBORDER 39  /* 1 */
#define CELL_LBORDERCOLOR 40  /* 3 */
#define CELL_RBORDERCOLOR 41  /* 3 */
#define CELL_TBORDERCOLOR 42  /* 3 */
#define CELL_BBORDERCOLOR 43  /* 3 */
#define CELL_FONTNAME 44      /* string */
#define CELL_LASTFONT 45      /* 0 */
#define CELL_NOFLAGS 46       /* 0 */
#define CELL_BYTEFLAGS 47     /* 1 */
#define CELL_WORDFLAGS 48     /* 2 */
#define CELL_LONGFLAGS 49     /* 4 */
#define CELL_BYTEWIDTH 50     /* 1 */
#define CELL_WORDWIDTH 51     /* 2 */
#define CELL_LONGWIDTH 52     /* 4 */
#define CELL_APENPAGE 53      /* 0 */
#define CELL_BPENPAGE 54      /* 0 */
#define CELL_FORMATNAME 55    /* string */

#define CELL_TYPE_BYTE 60     /* 1 + 1 */
#define CELL_TYPE_WORD 61     /* 1 + 2 */
#define CELL_TYPE_LONG 62     /* 1 + 4 */
#define CELL_TYPE_COLOR 63    /* 1 + 3 */
#define CELL_TYPE_STRING 64   /* 1 + string */
#define CELL_TYPE_BYTES 65    /* 1 + 4 + x */

// SaveCell() modes
#define IO_STANDARD_SAVE			0
#define IO_SAVE_FULL_NAMES			1
#define IO_IGNORE_PROTECTED_CELLS	2

#ifdef __amigaos4__
#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif
#endif

/*************************** Prototypes ***************************/

extern CONST_STRPTR IFFErrorText(long err);
extern STRPTR ReadChunkString(struct IFFHandle *iff, STRPTR buffer, ULONG len);
extern int32 SaveCells(struct IFFHandle *iff, struct Page *pg, ULONG handle, int32 mode);
extern long SaveFormat(struct IFFHandle *iff, struct Mappe *mp, struct MinList *list);
extern long SaveNames(struct IFFHandle *iff, struct Mappe *mp, struct MinList *list);
extern long LoadCells(struct IFFHandle *iff, LONG context, struct Page *page, struct MinList *list);
extern void LoadFormat(struct IFFHandle *iff, ULONG context, struct Mappe *mp, struct Prefs *pr, struct MinList *list);
extern int32 LoadProject(struct Mappe *mp, struct IOType *io);
extern int32 SaveProject(struct Mappe *mp, struct IOType *io, bool confirmOverwrite);
extern void InitIOType(struct IOType *io);
extern void initIO(void);
extern void closeIO(void);

#endif  /* IGN_IO_H */

