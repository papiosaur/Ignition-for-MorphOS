/*
 * Tree and calculation functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_CALC_H
#define IGN_CALC_H

/*************************** Formats ***************************/

struct FormatVorlage {
	struct Node fv_Node;
	STRPTR fv_Preview;
	int8   fv_Alignment;
	int8   fv_Flags;
	uint32 fv_NegativePen;
	int8   fv_Komma;
};

#define FVT_NONE 0			/* fv_Node.ln_Type */
#define FVT_VALUE 1

#define FVT_PERCENT 2
#define FVT_DATE 3
#define FVT_TIME 4
#define FVT_EINHEIT 5
#define FVT_NOTSPECIFIED 255  // type defined by format

#define FVF_NONE 0			/* fv_Flags */
#define FVF_ONTHEFLY 1		/* see ITA_xxx */
#define FVF_NEGATIVEPEN 2
#define FVF_SEPARATE 64
#define FVF_NEGPARENTHESES 128

#define FVT_ALL_THE_REST 0	/* format definition */
#define FVT_ZAHL 1
#define FVT_SDAY 2
#define FVT_DAY 3
#define FVT_SMIN 4
#define FVT_MIN 5
#define FVT_SYEAR 6
#define FVT_YEAR 7
#define FVT_SHOUR 8
#define FVT_HOUR 9
#define FVT_SSEC 10
#define FVT_SEC 11
#define FVT_SMON 12
#define FVT_MON 13
#define FVT_SWEEK 14
#define FVT_WEEK 15
#define FVT_CHAR 16
#define FVT_SPACE 17
#define FVT_EVERYCHAR 18
#define FVT_QUESTIONMARK 19
#define FVT_PROZENT 20
#define FVT_CROSS 21
#define FVT_CHAR_OR_NOTHING 22

/* Units */

#define CNT_MM 1
#define CNT_CM 2
#define CNT_DM 3
#define CNT_M 4
#define CNT_KM 7
#define CNT_POINT 10
#define CNT_INCH 11
#define CNT_FOOT 12
#define CNT_YARD 13

#define ITA_NONE		   FVF_NONE			 /* ita()-Options (and fv_Flags, too) */
#define ITA_SEPARATE	   FVF_SEPARATE
#define ITA_NEGPARENTHESES FVF_NEGPARENTHESES

#define FORMAT_DATE_PREVIEW 729373.0
#define FORMAT_TIME_PREVIEW 4242.0
 
/*************************** Calculation ***************************/

typedef enum {OP_OPEN = -6,OP_CLOSE,OP_TEXT,OP_VALUE,OP_CELL,OP_EXTCELL,
							OP_NONE,OP_GREATER,OP_EQGREATER,OP_LESS,OP_EQLESS,OP_EQUAL,
							OP_NOTEQUAL,OP_BITAND,OP_BITOR,OP_OR,OP_AND,OP_ADD,OP_SUB,
							OP_MUL,OP_DIV,OP_MOD,OP_NEG,OP_FAK,OP_BITNOT,OP_POT,OP_RANGE,
							OP_NAME,OP_TEXTVALUE,OP_FUNC} OPs;

struct FuncArg {
	struct MinNode fa_Node;
	struct Term *fa_Root;
};

struct Function {
	struct Node f_Node;  /* ln_Type -> function category */
	APTR   f_Code;	   /* pointer to a function */
	STRPTR f_Help;
	uint8  f_Type;
	uint32 f_ID;		 /* function ID, last byte is always NULL (can be casted to a STRPTR) */
	STRPTR f_Args;
	int32  f_MinArgs,f_MaxArgs;
};

#define RT_RESULT 0  /* f_Type */

#define FT_IGNOREFORMAT 64   /* f_Node.ln_Pri */


struct MaskField {
	struct Node mf_Node;
	uint32 mf_Col, mf_Row;
};

struct Mask {
	struct Node ma_Node;	  /* if ln_Type is set, search mode is active */
	struct Page *ma_Page;
	struct MinList ma_Fields;
};

struct Filter {
	struct Node fi_Node;  /* ln_Type -> active (1) or not (0) */
	uint32	  *fi_Index;
	uint32	  fi_Size;
	STRPTR	  fi_Filter;	 /* definition */
	struct Term *fi_Root;
	int8		fi_Type;	   /* Real filter or search filter */
};

#define FIT_FILTER 0	/* fi_Type */
#define FIT_SEARCH 1

struct Index {			 /* is equivalent to struct Filter */
	struct Node in_Node;	/* ln_Type -> active (1) or not (0) */
	uint32 *in_Index;	   /* array of positions */
	uint32 in_Size;
};

struct Field {
	struct Node fi_Node;
	STRPTR fi_Special;
};

#define FIT_STANDARD 0   /* fi_Node.ln_Type */
#define FIT_REFERENCE 1
#define FIT_CHOICE 2
#define FIT_COUNTER 3

struct Database {	/* has to be the same as "Name" - it's considered a sub-class of struct Name */
	struct Node db_Node;
	STRPTR db_Content;		   /* position text (e.g. a4:d100) */
	struct Term *db_Root;		/* position's formula */
	struct Page *db_Page;		/* db's page */
	uint32 db_PageNumber;		/* Number of db's page (for loading) */
	struct Reference *db_Reference;
	struct MinList db_Fields;	/* list of fields (struct Field) */
	struct tablePos db_TablePos; /* db's position */
	uint32 db_Current;		   /* current record */
	struct MinList db_Indices;   /* list of indices (struct Index) */
	struct MinList db_Filters;   /* list of filters */
	struct Index *db_Index;	  /* current index */
	struct Filter *db_Filter;	/* current filter */
	uint32 db_IndexPos;		  /* position in real/filter/index array */
};

#define DBC_REL 1   /* SetDBCurrent()-Modi */
#define DBC_ABS 2

struct Name {
	struct Node nm_Node;
	STRPTR nm_Content;
	struct Term *nm_Root;
	struct Page *nm_Page;
	uint32 nm_PageNumber;
	struct Reference *nm_Reference;
	struct tablePos nm_TablePos;
};

#define NMT_NONE 0		  // nm_Node.ln_Type
#define NMT_CELL 1
#define NMT_SEARCH 2
#define NMT_DATABASE 3	  // is set in db_Node.ln_Type
#define NMT_TYPEMASK 31
#define NMT_DETACHED 64	 // is set when the Name is not yet attached to a project
#define NMT_UNDEFINED 128   // as long nm_Page is not defined

struct Result {
	double  r_Value;
	STRPTR  r_Text;
	uint8   r_Type;
	struct tableField *r_Cell;
};

#define RT_VALUE 1	 /* both possible at the same time (e.g. OP_TEXTVALUE) */
#define RT_TEXT 2
#define RT_CELL 4	  /* direct cell access */

#define CT_OK 0		/* CalcTree() return codes */
#define CTERR_TYPE 1
#define CTERR_DIV 2
#define CTERR_LOOP 3
#define CTERR_FUNC 4
#define CTERR_ARGS 5
#define CTERR_NAME 6
#define CTERR_SYNTAX 7
#ifdef __amigaos4__
	#define CTERR_NULLP 8
#endif

/* Calculation-Flags */
#define CF_SUSPEND 1		// pausiert alle Berechnungen
#define CF_REQUESTER 2	  // bei Rechenfehler öffnet sich ein Requester
#define CF_ZERONAMES 4	  // Unbekannte Namen werden durch eine Null substituiert
#define CF_NOLOOPS 8		// Loops sind nicht erlaubt
#define CF_SHORTFUNCS 16	// benutze kurze interne Funktionsnamen (sprachunabhängig)

struct Term {
	struct Term *t_Left, *t_Right;
	OPs	 t_Op;
	int8	t_Pri;
	union {
		struct {
			double t_value;
			STRPTR t_text;
			STRPTR t_format;
		} t_val;
		struct {
			int32  t_col,t_row;
			int8   t_abscol,t_absrow;
		} t_cell;
		struct {
			struct Function *t_function;
			struct MinList t_args;
//		  STRPTR t_language;
		} t_func;
		struct {
			int32  t_col,t_row;
			STRPTR t_page;
			int32  t_numpage;
		} t_ext;
	} t_type;
};

#define t_Value t_type.t_val.t_value
#define t_Text t_type.t_val.t_text
#define t_Format t_type.t_val.t_format
#define t_Col t_type.t_cell.t_col
#define t_Row t_type.t_cell.t_row
#define t_AbsCol t_type.t_cell.t_abscol
#define t_AbsRow t_type.t_cell.t_absrow
#define t_Function t_type.t_func.t_function
#define t_Args t_type.t_func.t_args
//#define t_Language t_type.t_func.t_language
#define t_Page t_type.t_ext.t_page
#define t_NumPage t_type.t_ext.t_numpage
//#define t_Mappe t_type.t_ext.t_mappe


/******************************** Referenzen ********************************/

struct Reference {
	struct ArrayList r_References;	// array of (struct Reference *)
	struct ArrayList r_ReferencedBy;  // array of (struct Reference *)
	APTR   r_This;
	struct Page *r_Page;
	uint8  r_Type;
	union {
		struct {
			struct tablePos tp;
			uint32 count;
		} tp;
		struct cellPos cp;
		STRPTR name;
	} u;
};

#define RTYPE_CELL 1	   // type of "this"
#define RTYPE_NAME 2
#define RTYPE_UNRESOLVED_NAME 3
#define RTYPE_UNRESOLVED_CELL 4
#define RTYPE_UNRESOLVED_RANGE 5
#define RTYPE_OBJECT 6

#define RTYPE_TYPEMASK 15

#define RTYPE_LOOP 32
#define RTYPE_TIMED 64	 // is in "gTimedRefs"

//#define RTS_TIME 1   /* reference type - special */
//#define RTS_DATE 2
/*
struct RefObject {
	struct Page *ro_Page;
	UBYTE  ro_Type;
};
*/
#define r_Range u.tp.tp
#define r_Count u.tp.count
#define r_Col u.cp.cp_Col
#define r_Row u.cp.cp_Row
#define r_Name u.name	 /* allocated string */
/*
#define ROT_CELL 0
#define ROT_RANGE 1
#define ROT_NAME 2
#define ROT_UNRESOLVED_CELL 2
#define ROT_UNRESOLVED_NAME 3
*/
#define RCT_CELL 0   /* Modes for RecalcCells() and RemoveReferences() */
#define RCT_NAME 1
#define RCT_ALL 2
#define RCT_PAGE 3

/* Funktionentypen */

#define FT_RECENT 0
#define FT_ALL 1
#define FT_MATH 2
#define FTM_ALGEBRA 3
#define FTM_FINANZ 4
#define FTM_LOGIK 5
#define FTM_MATRIZEN 6
#define FTM_TRIGO 7
#define FTM_STATISTIK 8
#define FT_DATE 9
#define FT_DATABASE 10
#define FT_TEXT 11
#define FT_LOOK 12
#define FT_TABLE 13
#define FT_MISC 14

/** different languages for the functions **/

struct FunctionLanguage {
	struct Node fl_Node;
	struct FunctionName *fl_Array;
	uint32 fl_Length,fl_Bytes;
};

struct FunctionName {
	STRPTR fn_Name;
	struct Function *fn_Function;
};

/*************************** Prototypes ***************************/

// functions.c
extern struct Function *FindFunctionWithLanguage(struct FunctionLanguage *fl,STRPTR name);
extern struct Function *FindFunction(STRPTR name,struct Term *t);
extern void initFuncs(void);

// reference.c
extern uint8 ReferenceType(struct Reference *r);
extern void AssignUnresolvedReferencesForCell(struct Page *page,struct tableField *tf);
extern void AssignUnresolvedReferencesForName(struct Page *page, struct Name *nm);
extern struct Reference *MakeReference(struct Page *page,UBYTE type,void *this,struct Term *t);
extern void UpdateReferences(struct Reference *r,struct Term *t);
extern void FreeReference(struct Reference *r, bool recalc);
#ifdef DEBUG
extern void DumpReference(struct Reference *r);
#endif
			 
// calc.c
extern void InitCalc(void);
extern STRPTR PUBLIC AbsCoord2String(REG(d0, BOOL),REG(d1, long),REG(d2, BOOL),REG(d3, long));
extern STRPTR Coord2String(long col,long row);
extern void TablePos2String(struct Page *page,struct tablePos *tp,STRPTR t);
extern void Pos2String(long pos,STRPTR t);
extern long String2Coord(STRPTR s,long *col,long *row);
extern BOOL PosInTablePos(struct tablePos *tp,ULONG col,ULONG row);
extern BOOL InTablePos(struct tablePos *tp,struct tablePos *itp);
extern BOOL FillTablePos(struct tablePos *tp,struct Term *kn);
// format
extern LONG FormatSort(struct Node **lna,struct Node **lnb);
extern void SortFormatList(struct MinList *list);
extern void FreeFormat(struct FormatVorlage *fv);
extern struct FormatVorlage *CopyFormat(struct FormatVorlage *fv);
extern struct FormatVorlage *AddFormat(struct MinList *list,STRPTR t,BYTE pri,BYTE komma,BYTE align,ULONG pen,UBYTE flags,UBYTE type);
// name
extern void FreeName(struct Name *nm);
extern void RefreshName(struct Name *nm);
extern struct Name *CopyName(struct Name *nm);
extern void AttachNameList(struct MinList *list);
extern void DetachNameList(struct MinList *list);
extern void SetNameContent(struct Name *nm, STRPTR content);
extern struct Name *AddName(struct MinList *list,CONST_STRPTR n,STRPTR t,BYTE type,struct Page *page);
extern bool IsValidName(struct List *list, STRPTR t);
extern bool GetNameAndField(STRPTR t, struct Database **_db, struct Field **_fi, int32 *_fiPos);
extern STRPTR ita(double f,long nach,UBYTE flags);
extern STRPTR FitValueInFormat(double val,struct Node *fv,STRPTR fvt,long komma,UBYTE flags);
extern void CalcTableField(struct tableField *tf);
extern struct Page *GetExtCalcPage(struct Term *t);
extern struct tableField *GetExtTableField(struct Term *t);
extern BYTE GetFVType(struct Node *fv,long *pos);
extern void GetValue(struct Page *,struct tableField *tf);
extern void GetFormula(struct Page *,struct tableField *tf);
extern int32 CheckFormat(struct Node *fv,STRPTR t,double *value);
extern long GetFormatOfValue(STRPTR t,double *value);
extern void RecalcTableFields(struct Page *);
extern void RecalcMapPages(struct Mappe *mp);
extern void RecalcMap(struct Mappe *mp);
extern void RecalcReferencesList(struct ArrayList *al);
extern void RecalcReferencingObjects(struct Reference *r, bool recalcThis);
extern STRPTR StaticTreeTerm(struct Term *t,BOOL formula);
extern STRPTR TreeTerm(struct Term *k,BOOL formula);
extern struct Term * PUBLIC CopyTree(REG(a0, struct Term *t));
extern struct Term * PUBLIC CreateTree(REG(a0, struct Page *page),REG(a1, STRPTR t));
extern void PUBLIC DeleteTree(REG(a0, struct Term *t));
extern int32 PUBLIC CalcTree(REG(a0, struct Result *r),REG(a1, struct Term *t));
extern STRPTR TreeText(struct Term *t);
extern double TreeValue(struct Term *t);
extern struct tableField *TreeCell(struct Term *t);

// calc.c - support functions for gObjects and others
extern struct Term * PUBLIC CopyTerm(REG(a0, struct Term *term));
extern void PUBLIC DeleteTerm(REG(a0, struct Term *term));
extern struct Term *CreateTreeFrom(struct Page *page, long col, long row, STRPTR t);
extern struct Term * PUBLIC CreateTerm(REG(a0, struct Page *page),REG(a1, STRPTR text));
extern STRPTR PUBLIC CalcTerm(REG(a0, struct Page *page),REG(a1, STRPTR text),REG(a2, struct Term *term),REG(a3, STRPTR format));

#endif   /* IGN_CALC_H */
