/*
 * Copyright ©1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_GRAPHIC_H
#define IGN_GRAPHIC_H

struct gBounds;
struct gFrame;

#define glContext amigamesa_context

#ifdef __amigaos4__
#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif
#endif

struct gClass {
	struct ImageNode gc_Node;   /* name und icon (only for non-diagrams) */
	struct gClass *gc_Super;	/* super class */
	ULONG  gc_InstOffset;
	ULONG  gc_InstSize;
	ULONG  gc_Flags;
	struct gInterface *gc_Interface;
	BPTR   gc_Segment;
#ifdef __amigaos4__
	ULONG  ASM (*gc_Dispatch)(REG(a0, struct gClass *), REG(a2, APTR), REG(a1, Msg));
#else
	ULONG  ASM (*gc_Dispatch)(REG(a0, struct gClass *), REG(a2, Msg), REG(a1, APTR));
#endif
	ULONG  ASM (*gc_Draw)(REG(d0, struct Page *), REG(d1, ULONG), REG(a0, struct RastPort *), REG(a1, struct gClass *), REG(a2, struct gObject *), REG(a3, struct gBounds *));
	ULONG  ASM (*gc_FreeClass)(REG(a0, struct gClass *));
	STRPTR gc_ClassName;		/* internal access (file name) */
};

#define GCT_ROOT 1	 // gc_Node.in_Type
#define GCT_OBJECT 2
#define GCT_DIAGRAM 12
#define GCT_DIAGRAM2D 4
#define GCT_DIAGRAM3D 8
#define GCT_INTERNAL 32

#define GINST_DATA(gc,go) (APTR)((UBYTE *)(go)+((struct gClass *)gc)->gc_InstOffset)

/*** interface ***/

struct gInterface {
	ULONG  gi_Tag;		/* tag for GCM_SET/GCM_GET */
	STRPTR gi_Label;
	ULONG  gi_Type;		/* type of gadget (checkbox, cycle, string, ...) */
	APTR   gi_Special;	/* e.g. selection of cycle gadgets */
	STRPTR gi_Name;		/* for REXX/function-interface */
};

#define GIT_PEN 1		/* gi_Type */
#define GIT_FONT 2
#define GIT_TEXT 3
#define GIT_CYCLE 4
#define GIT_CHECKBOX 5
#define GIT_FILENAME 6
#define GIT_WEIGHT 7
#define GIT_FORMULA 8
#define GIT_BUTTON 9		// private
#define GIT_DATA_PAGE 10	// diagram private
#define GIT_NONE 11			// private?

struct gGadget {
	struct MinNode gg_Node;
	struct Gadget *gg_Gadget;
	ULONG  gg_Tag;
	ULONG  gg_Type;		/* same as gi_Type */
	UBYTE  gg_Kind;		/* gadget type (gadtools, ...) */
	UBYTE  gg_Page;
};

#define POPUP_KIND 42
#define POPUPPOINTS_KIND 43

/* predefined tags (gi_Tag, GCM_NEW, GCM_SET, GCM_GET) */

#define GOA_TagBase		TAG_USER+0x1000
#define GOA_Name		   GOA_TagBase+0   // GIT_TEXT
#define GOA_Text		   GOA_TagBase+1   // GIT_TEXT
#define GOA_Help		   GOA_TagBase+2   // GIT_TEXT
#define GOA_Pen			GOA_TagBase+3   // GIT_PEN
#define GOA_FillPen		GOA_TagBase+4   // GIT_PEN
#define GOA_OutlinePen	 GOA_TagBase+5   // GIT_PEN
#define GOA_Command		GOA_TagBase+6   // GIT_TEXT
#define GOA_ContinualCommand GOA_TagBase+7 // GIT_CHECKBOX
#define GOA_FontInfo	   GOA_TagBase+8   // GIT_FONT
#define GOA_HasOutline	 GOA_TagBase+9   // GIT_CHECKBOX
#define GOA_Weight		 GOA_TagBase+10  // GIT_WEIGHT
#define GOA_Page		   GOA_TagBase+11

#define GOA_TagUser		GOA_TagBase+0x1000

/*
#define GOA_Weight	  GOA_TagBase+11  //
#define GOA_Rotate	  GOA_TagBase+12  // not yet implemented
#define GOA_Edges	   GOA_TagBase+14  // "
#define GOA_BeginLine   GOA_TagBase+15  // "
#define GOA_EndLine	 GOA_TagBase+16  // "
*/

#define GROUPOBJECT(gg) ((struct gObject *)((UBYTE *)gg-sizeof(struct Node)))
#define OBJECTGROUP(go) ((struct gGroup *)&go->go_GroupNode)

struct gGroup {
	struct MinNode gg_Node;
	UWORD  gg_Type;
	LONG   gg_Left,gg_Top,gg_Right,gg_Bottom;
	LONG   gg_mmLeft,gg_mmTop,gg_mmRight,gg_mmBottom;
	ULONG  gg_Flags;
	struct MinList gg_Objects;
};

struct gObject {
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
	LONG   go_Pos;			  /* <- rather unique ID with list */
	struct MinList go_ReferencedBy;
};

#define GOT_OBJECT 0		  /* go_Type/gg_Type */
#define GOT_GROUP 1

#define GOF_SELECTED 1
#define GOF_CONTINUALCMD 2
#define GOF_PROTECTED 4
#define GOF_FRAMED 8
#define GOF_PRESSED 16
#define GOF_LINEMODE 32		/* probably superfluous */
#define GOF_RECALC 64		/* take into account on recomputation */
#define GOF_INVALIDREFS 128

#define GOKNOB_SIZE 1

#define ACTGO_EXCLUSIVE 1

#define ADDREM_DRAW 1		 /* Add-/RemoveGObject */
#define ADDREM_NOGROUP 2
#define ADDREM_NOREFS 4
#define ADDREM_CLOSE_WINDOW 8
#define ADDREM_NONE 0

#define GO_FRONTMOST 0		/* ChangeGObjectOrder */
#define GO_TOFRONT 1
#define GO_TOBACK 2
#define GO_BACKMOST 3

/*************************** 2D graphic primitives ***************************/

struct coord {
	WORD x,y;
};

struct point2d {
	long x,y;
};

struct gBounds {
	LONG gb_Left,gb_Top,gb_Right,gb_Bottom;
};

/*************************** coordinate axes ***************************/

struct gAxes {
	struct gBounds ga_Bounds;	// Ausweitung der Achsen
	double ga_Max;				// Maximal-Wert
	double ga_SecondMax;		// "	  -" 2. Achse
	double ga_Min;				// Minimal-Wert
	double ga_SecondMin;		// "	  -" 2. Achse
	double ga_Divisor;			// Skalenteiler
	double ga_SecondDivisor;	// " 2. Achse
	STRPTR ga_Title;
	STRPTR ga_SecondTitle;		// Titel der 2. Achse
	UWORD  ga_Flags;
	UBYTE  ga_Orientation;
	UBYTE  ga_Raster;
	ULONG  ga_NumberPen,ga_APen,ga_BPen;
	struct gBounds ga_Frame;
	struct FontInfo *ga_FontInfo;
	ULONG  ga_Depth;
};

#define GAF_LINEAR 1
#define GAF_LOG 2
#define GAF_SECOND 4
#define GAF_STATICAXIS 8
#define GAF_STATIC2NDAXIS 16
#define GAF_ARROW 32
#define GAF_XARROW 64
#define GAF_SHOWNUMBERS 128
#define GAF_PSEUDO3D 256
#define GAF_TRANSPARENT_BACKGROUND 512

#define GAR_NONE 0
#define GAR_POINTS 1
#define GAR_LINES 2

#define GAO_HORIZONTAL 0
#define GAO_VERTICAL 1

/** gAxes - Methods **/

#define GCAM_GETBORDERS 1000
#define GCAM_GETCOORD   1001

struct gcapGetBorders {
	ULONG  MethodID;
	LONG   gcap_Row;
	LONG   *gcap_StorageLeft, *gcap_StorageRight, *gcap_StorageYOrigin;
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
	double gcap_Y;		// value to transform
	LONG   gcap_Row;	// row of value
	LONG   gcap_Scale;	// which scale to use (1 or 2) (not yet implemented)
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
/** gAxes - Attributes **/

#define GAA_TagBase			GOA_TagBase + 100
#define GAA_FontInfo		GAA_TagBase + 1  // GIT_FONT
#define GAA_NumberPen   	GAA_TagBase + 2  // GIT_PEN
#define GAA_Raster			GAA_TagBase + 3  // GIT_CYCLE
#define GAA_ShowNumbers 	GAA_TagBase + 4  // GIT_CHECKBOX
#define GAA_BPen			GAA_TagBase + 5  // GIT_PEN
#define GAA_Pseudo3D		GAA_TagBase + 6  // GIT_CHECKBOX

#define GAA_DepthWidth  	GAA_TagBase + 7   	// (G)
#define GAA_Pseudo3DDepth 	GAA_TagBase + 8		// must be implemented by objects supporting pseudo-3D-mode
#define GAA_Bounds			GAA_TagBase + 9   	// (G)

#define GAA_TransparentBackground GAA_TagBase + 10


/*************************** graphic objects ***************************/

/*
struct gPicture
{
	STRPTR gp_Name;
};

struct gText
{
	STRPTR gt_Text;
	ULONG  gt_Rotation;
	struct Node *gt_Family;
	struct FontSize *gt_Font;
	long   gt_PointHeight;
	long   gt_Style;
};

struct gVector
{
	ULONG gv_Weight;
	union {
		struct {
						 UBYTE gv_begin;
						 UBYTE gv_end;
					 } gv_line;
		struct {
						 UBYTE gv_edges;
					 } gv_rect;
	} gv_type;
};

#define gv_Begin gv_type.gv_line.gv_begin
#define gv_End gv_type.gv_line.gv_end
#define gv_Edges gv_type.gv_rect.gv_edges
*/

/*************************** Methods ***************************/

#define GCM_NEW					1		   /* gClass Methods */
#define GCM_DISPOSE				2
#define GCM_HITTEST				3
#define GCM_SET					4
#define GCM_GET 				5
#define GCM_DUPLICATE			6
#define GCM_UPDATE				7
#define GCM_COMMAND				8
#define GCM_BEGINPOINTS			9
#define GCM_ENDPOINTS			10
#define GCM_UPDATEPOINT			11
#define GCM_ADDPOINT			12
#define GCM_CHANGEPOINT			13
#define GCM_BOX					14
#define GCM_COPY				15
#define GCM_REALPOINTS			16
#define GCM_RECALC				17
#define GCM_ADD_TO_SCREEN		18
#define GCM_REMOVE_FROM_SCREEN	19
#define GCM_LOAD				20
#define GCM_SAVE				21
#define GCM_OPENWINDOW			22	// private
#define GCM_INITAFTERLOAD		23
#define GCM_GETNAMEDATTR		24
#define GCM_INSERTREMOVECELLS 	25
#define GCM_UPDATE_UI			26
#define	GCM_CLOSEWINDOW			27	// private

// the gDiagram methods start at 30

/* GCM_NEW: return object or NULL for failure */
struct gcpNew {
	ULONG MethodID;
	struct TagItem *gcpn_AttrList;
	struct Page *gcpn_Page;
};

/* GCM_SET: one of the GCPRs below */
struct gcpSet {
	ULONG MethodID;
	struct TagItem *gcps_AttrList;
};

#define GCPR_NONE 0
#define GCPR_REDRAW 1
#define GCPR_UPDATESIZE 2

/* GCM_GET: return TRUE or FALSE */
struct gcpGet {
	ULONG MethodID;
	ULONG gcpg_Tag;
	ULONG *gcpg_Storage;
};

/* GCM_HITTEST: return TRUE or FALSE */
struct gcpHitTest {
	ULONG MethodID;
	LONG  gcph_X,gcph_Y;
};

/* GCM_UPDATE */
struct gcpUpdate {
	ULONG MethodID;
	ULONG gcpu_DPI;
};

/* GCM_BEGINPOINTS: return the number of points needed or */
/*				  ~0L for an undefined number		   */
struct gcpBeginPoints {
	ULONG MethodID;
	ULONG gcpb_Mode;
};

#define GCPBM_ONE 0
#define GCPBM_MORE 1

/* GCM_ENDPOINTS: return number+array of real points */
struct gcpEndPoints {
	ULONG  MethodID;
	struct point2d *gcpe_Points;
	ULONG  gcpe_NumPoints;
	struct point2d **gcpe_StoragePoints;
	ULONG  *gcpe_StorageNumPoints;
};

struct gcpUpdatePoint {
	ULONG  MethodID;
	struct point2d gcpu_Point;
	ULONG  gcpu_NumPoint;	/* number of changed point */
};

/* GCM_ADDPOINT */
/* GCM_CHANGEPOINT */
struct gcpAddPoint {
	ULONG  MethodID;
	struct RastPort *gcpa_RastPort;
	struct coord *gcpa_Points;
	struct coord gcpa_Offset;
	ULONG  gcpa_NumPoints;
	ULONG  gcpa_Mode;
	ULONG  gcpa_Point;		/* number of changed point (GCM_CHANGEPOINT only) */
};

#define GCPAM_ONEMORE 0		/* one point is added to the list */
#define GCPAM_UPDATE 1		/* update the drawing (COMPLEMENT) */
#define GCPAM_REDRAW 2		/* redraw the whole drawing */
// #define GCPAM_ERASE 3	/* erase the whole drawing */

struct gcpRealPoints {
	ULONG  MethodID;
	struct point2d **gcpr_StoragePoints;
	ULONG  *gcpr_StorageNumPoints;
};

/* GCM_ADDTOSCREEN */
/* GCM_REMOVEFROMSCREEN */
struct gcpScreen {
	ULONG  MethodID;
	struct Screen *gcps_Screen;
};

/* GCM_LOAD */
/* GCM_SAVE */
struct gcpIO {			   
	ULONG  MethodID;
	struct IFFHandle *gcpio_IFFHandle;
	struct MinList *gcpio_Fonts;
	struct Page *gcpio_Page; /* GCM_LOAD only */
};

/* GCM_GETNAMEDATTR */
struct gcpNamedAttr {
	ULONG  MethodID;
	STRPTR gcpna_Name;
	struct Result *gcpna_Result;
};

/* GCM_INSERTREMOVECELLS */
struct gcpInReCells {
	ULONG  MethodID;
	struct Page *gcpc_Page;
	LONG   gcpc_Offset,gcpc_Diff,gcpc_Comp,gcpc_First,gcpc_Last;
};

/*** misc ***/

#define DMSF_ALL 0	   // DrawMultiSelectFrame()-Modes
#define DMSF_BEGIN 1
#define DMSF_END 2

#define GFO_FRONT 1	  // GFrameOrder()-Options
#define GFO_FRONTMOST 2
#define GFO_BACK 3
#define GFO_BACKMOST 4

struct gfxObject {
	struct ImageNode go_Node;
	UBYTE  go_Type,go_SubType;
};

struct gfxModule {
	struct Node gm_Node;
	BPTR   gm_Segment;
	struct MinList gm_Attrs;
	APTR   *gm_Funcs;
};

#define GMA_STANDARD 0
#define GMA_VERTHORIZ 1
#define GMA_ONOFF 2

/*************************** 3D graphic primitives ***************************/

struct point3d {
	double x, y, z;
};

struct field3d {
	ULONG  fi_APen,fi_OPen;
	WORD   *fi_Points;
	WORD   fi_NumPoints;
	struct point3d fi_Normal;
	UBYTE  fi_Flags;
};

#define FIF_FILLED 1
#define FIF_VISIBLE 2

struct object3d {
	struct MinNode ob_Node;
	struct field3d *ob_Fields;
	WORD   ob_NumFields;
};

struct text3d {
	struct MinNode t_Node;
	STRPTR t_Label;
	struct FontInfo *t_FontInfo;
	WORD   t_NumPoint;
	UBYTE  t_Flags;
	ULONG  t_Pen;
};

#define TF_COPIED 1
#define TF_ABOVE 2
#define TF_BELOW 4
#define TF_OVER 6
#define TF_LEFT 8
#define TF_RIGHT 16
#define TF_CENTER 24

struct scene3d {
	struct MinList sc_Objects;
	struct MinList sc_Texts;

	struct point3d *sc_Points;
	struct point3d *sc_ProjPoints;
	struct field3d **sc_Fields;
	ULONG  sc_NumPoints,sc_NumFields;
	struct point3d sc_Observer;

	struct point2d sc_Position,sc_Size;
	double sc_SizeFactorX,sc_SizeFactorY;
	LONG   sc_RotX,sc_RotY,sc_RotZ;

	struct point3d sc_Light;
	UBYTE  sc_LightIntensity;

	struct FontInfo *sc_FontInfo;
};

struct gLink {
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

struct gDiagram {		   // extends gObject
	struct gObject gd_Object;
	UBYTE  gd_Flags;
	UBYTE  gd_ReadData;
	STRPTR gd_Range;
	struct tablePos gd_TablePos;
	struct Reference *gd_Reference;
	struct MinList gd_Values;	   // gLinks in a list
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

struct gDiagram3d {		// extends gDiagram
	struct gDiagram gd_Diagram;
	struct point3d gd_Observer;
	LONG   gd_RotX,gd_RotY,gd_RotZ;
	struct FontInfo *gd_FontInfo;
	APTR   gd_Context;
};

#define GDA_TagBase	TAG_USER+0x1100
#define GDA_Range	 (GDA_TagBase + 0)
#define GDA_ReadData  (GDA_TagBase + 1)
#define GDA_RotX	  (GDA_TagBase + 2)
#define GDA_RotY	  (GDA_TagBase + 3)
#define GDA_RotZ	  (GDA_TagBase + 4)
#define GDA_Trans	 (GDA_TagBase + 5)		// (struct point 3d *)
#define GDA_Observer  (GDA_TagBase + 6)		// (struct point 3d *)
#define GDA_Light	 (GDA_TagBase + 7)		// (struct point 3d *)
#define GDA_LightIntensity  (GDA_TagBase + 8)  // (UBYTE)
#define GDA_DataPage  (GDA_TagBase + 9)		// (struct Page *)
#define GDA_Update	(GDA_TagBase + 10)
#define GDA_OpenGL	(GDA_TagBase + 11)
#define GDA_SupportsMarking (GDA_TagBase + 12) // (BOOL) must be implemented by classes
//#define GDA_   (GDA_TagBase + )

#define GDOPENGL_NONE 0
#define GDOPENGL_GL 1
#define GDOPENGL_GLU 2
#define GDOPENGL_GLUT 4

/** Methods for gDiagrams **/

#define GCDM_SETLINKATTR 30

/* GCDM_SETLINKATTR */
struct gcpSetLinkAttr {
	ULONG  MethodID;
	struct gLink *gcps_Link;
	ULONG  gcps_Color;
	ULONG  gcps_Marked;
};

/*************************** Colors ***************************/

struct colorPen {
	struct Node cp_Node;
	WORD   cp_Pen;
	UBYTE  cp_Red,cp_Green,cp_Blue;
	WORD   cp_PenA,cp_PenB;
	UBYTE  cp_Pattern;
	ULONG  cp_ID;
};

/* cp_Node.ln_Type == 1 -> name has been chosen by FindColorName()
**
** cp_PenA,cp_PenB,cp_Pattern are currently unused
*/

struct ColorContext {
	bool	cc_TrueColor;

	void	(*cc_Release)(struct Screen *screen, struct ColorContext *context);

	void	(*cc_SetHighColor)(struct RastPort *rp, uint32 color);
	void	(*cc_SetLowColor)(struct RastPort *rp, uint32 color);
	void	(*cc_SetOutlineColor)(struct RastPort *rp, uint32 color);
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

/*************************** Prototypes ***************************/

// inlines
static __inline int32
mm_to_pixel_dpi(uint32 dpi, int32 mm, bool horiz)
{
	dpi = horiz ? dpi >> 16 : dpi & 0xffff;
	return (LONG)(mm * dpi / (25.4*1024));
}


static __inline int32
pixel_to_mm_dpi(uint32 dpi, int32 pixel, bool horiz)
{
	dpi = horiz ? dpi >> 16 : dpi & 0xffff;
	return (LONG)(pixel * 1024 * 25.4 / dpi);
}


static __inline bool
gObjectIsDiagram(struct gObject *go)
{
	return (bool)((go->go_Class->gc_Node.in_Type & GCT_DIAGRAM) != 0);
}

 
// gl stuff
#define glCreateContextA AmigaMesaCreateContext
#define glCreateContext AmigaMesaCreateContextTags
#define glMakeCurrent AmigaMesaMakeCurrent
#define glDestroyContext AmigaMesaDestroyContext

// color.c
extern void ReleaseAppColors(struct Screen *screen);
extern void ObtainAppColors(struct Screen *screen, BOOL all);
extern void FreeAppColors(void);
extern void UniqueColors(struct MinList *cols);
extern void GetColorName(STRPTR t, UBYTE red, UBYTE green, UBYTE blue);
extern void FindColorName(struct colorPen *cp);
extern ULONG PUBLIC FindColorPen(REG(d0, UBYTE red), REG(d1, UBYTE green), REG(d2, UBYTE blue));
extern ULONG PUBLIC TintColor(REG(d0, ULONG id),REG(d1, float tint));
extern struct colorPen *GetColorPen(ULONG id);
extern void PUBLIC SetOutlineColor(REG(a0, struct RastPort *rp),REG(d0, ULONG color));
extern void PUBLIC SetHighColor(REG(a0, struct RastPort *rp),REG(d0, ULONG color));
extern void PUBLIC SetLowColor(REG(a0, struct RastPort *rp),REG(d0, ULONG color));
extern void PUBLIC SetColors(REG(a0, struct RastPort *rp),REG(d0, ULONG apenID),REG(d1, ULONG bpenID));
extern void PUBLIC SetColorPen(REG(a0, struct RastPort *rp), REG(a1, struct colorPen *pen));
extern struct colorPen *AddColor(struct MinList *list, STRPTR name, UBYTE red, UBYTE green, UBYTE blue);

// graphic.c
#define gAreaMove(rp,x,y) SafeAreaDraw(x,y)
#define gAreaDraw(rp,x,y) SafeAreaDraw(x,y)
#define gAreaEnd(rp) SafeAreaEnd(rp)

extern ULONG SafeInit (ULONG nvertmax);
extern void SafeClose (void);
extern void SafeAreaDraw(LONG x,LONG y);
extern void SafeAreaEnd (struct RastPort *rp);
extern ULONG PUBLIC GetDPI(REG(a0, struct Page *page));
extern LONG PUBLIC GetOffset(REG(a0, struct Page *page),REG(d0, BOOL horiz));
extern void MakeTmpRas(long w,long h);
extern void RectFill32(struct RastPort *rp,long x1,long y1,long x2,long y2);
extern void DrawHorizBlock(struct RastPort *rp,ULONG dpi,long x1,long y,long x2,ULONG points,UWORD flags);
extern void DrawVertBlock(struct RastPort *rp,ULONG dpi,long x,long y1,long y2,ULONG points,UWORD flags);
void PUBLIC DrawLine(REG(a0, struct RastPort *rp),REG(d0, ULONG dpi),REG(d1, long x1),REG(d2, long y1),REG(d3, long x2),REG(d4, long y2),REG(d5, ULONG points),REG(d6, UWORD flags));
extern struct gClass *FindGClass(STRPTR name);
extern BOOL LoadGClass(struct gClass *gc);
extern void FreeGClasses(void);
extern void InitGClasses(void);
extern void FreeDoubleBufferBitMap(void);
extern void AllocDoubleBufferBitMap(struct Screen *scr);
extern void FreeDoubleBuffer(void);
extern void InitDoubleBuffer(void);
extern void FreeGraphics(void);
extern void InitGraphics(void);

// objects.c
extern void PUBLIC gInsertRemoveCellsTerm(REG(a0, struct gcpInReCells *gcpc),REG(a1, struct Page *page),REG(a2, STRPTR *term),REG(a3, struct Term *t));
extern void PUBLIC gInsertRemoveCellsTablePos(REG(a0, struct gcpInReCells *gcpc),REG(a1, struct Page *page),REG(a2, STRPTR *term),REG(a3, struct tablePos *tp));
extern void RefreshGObjectReferences(struct gObject *go);
extern void DrawGObject(struct RastPort *rp, struct Page *page,struct gObject *go,long x,long y);
extern void DrawSingleGObject(struct Page *page,struct gObject *go);
extern BYTE CheckGGroup(struct Page *page,struct gGroup *gg,long x,long y);
extern BYTE CheckGObject(struct Page *page,struct gObject *go,long x,long y);
extern void DrawGObjectKnobs(struct Page *page,struct gObject *go);
extern void DeselectGObjects(struct Page *page);
extern void DrawGObjectMove(struct Page *page);
extern void DrawMultiSelectFrame(struct Page *page,UBYTE mode);
extern struct gInterface *GetGInterfaceTag(struct gClass *gc,ULONG tag);
extern void RefreshGObjectDrawing(struct Page *page,struct gObject *go);
extern void UpdateGObject(struct Page *page,struct gObject *go);
extern void UpdateGGroups(struct Page *page);
extern void FreeGObject(struct gObject *go);
extern void FreeGGroup(struct gGroup *gg);
extern void SetGFramePositionMode(struct Page *page,struct gFrame *gf);
extern void RefreshGFrames(struct Page *page);
extern struct gObject *gMakeRefObject(struct Page *page, struct gClass *gc, struct gObject *go, const STRPTR undoText);
extern ULONG GetGObjectAttr(struct gObject *go,ULONG tag,ULONG *data);
extern void gSetObjectAttrsA(struct Page *page,struct gObject *go,struct TagItem *tags);
#ifdef __amigaos4__
extern void gSetObjectAttrs(struct Page *page,struct gObject *go,...) VARARGS68K;
#else
extern void gSetObjectAttrs(struct Page *page,struct gObject *go,ULONG tag1,...) VARARGS68K;
#endif
extern void SetGObjectAttrsA(struct Page *page,struct gObject *go,struct TagItem *tag);
#ifdef __amigaos4__
extern void SetGObjectAttrs(struct Page *page,struct gObject *go,...) VARARGS68K;
#else
extern void SetGObjectAttrs(struct Page *page,struct gObject *go,ULONG tag1,...) VARARGS68K;
#endif
extern void RefreshGObjectBounds(struct Page *page,struct gObject *go);
extern void RemoveGObject(struct Page *page,struct gObject *go,BYTE draw);
extern void AddGObject(struct Page *page,struct gGroup *gg,struct gObject *go,BYTE draw);
extern BOOL ChangeGObjectOrder(struct Page *page, struct gObject *go, UBYTE type);
extern void AddGGroup(struct Page *page, struct gGroup *gg, BYTE draw, LONG level);
extern void RemoveGGroup(struct Page *page, struct gGroup *gg, BYTE draw, LONG level);
extern void SizeGObject(struct Page *page, struct gObject *go, LONG w, LONG h);
extern struct gFrame *GroupGFrames(struct Page *page, STRPTR name);
extern BOOL UngroupGFrames(struct Page *page, STRPTR name);
extern void SelectGGroup(struct Page *page, struct gGroup *gg, BYTE mode);
extern struct Window *OpenGObjectWindow(struct gObject *go);
extern struct gGroup *MouseGGroup(struct Page *page,LONG *pos);
extern BOOL HandleGObjects(struct Page *page);
extern void PrepareCreateObject(struct Page *page,struct gClass *gc,BOOL more);
extern ULONG gDoClassMethodA(struct gClass *gc,APTR go,Msg msg);
#ifdef __amigaos4__
extern ULONG gDoClassMethod(struct gClass *gc,APTR go,...) VARARGS68K;
#else
extern ULONG gDoClassMethod(struct gClass *gc,APTR go,ULONG id,...) VARARGS68K;
#endif
extern ULONG PUBLIC gDoMethodA(REG(a0, APTR go),REG(a1, Msg msg));
#ifdef __amigaos4__
extern ULONG gDoMethod(APTR go, ...) VARARGS68K;
#else
extern ULONG gDoMethod(APTR go,ULONG id,...) VARARGS68K;
#endif
extern ULONG PUBLIC gDoSuperMethodA(REG(a0, struct gClass *gc),REG(a1, APTR go),REG(a2, Msg msg));
extern BOOL PUBLIC gIsSubclassFrom(REG(a0, struct gClass *gc),REG(a1, struct gClass *supergc));
extern void PUBLIC gSuperDraw(REG(d0, struct Page *page),REG(d1, ULONG dpi),REG(a0, struct RastPort *rp),REG(a1, struct gClass *gc),REG(a2, struct gObject *go),REG(a3, struct gBounds *gb));
extern void SetGRastPort(struct RastPort *rp);
extern void gRemoveObjectsFromScreen(struct Mappe *mp, struct Screen *scr);
extern void gAddObjectsToScreen(struct Mappe *mp, struct Screen *scr);

#if 0
extern void DrawGFrame(struct Page *page,struct gFrame *gf);
extern struct gFrame *CreateGFrame(struct Page *page,struct gObj *go,BOOL hidden);
#endif

// diagram.c
extern void UpdateDiagramTypePage(struct Window *win, struct winData *wd, struct gDiagram *gd);
extern void SetDiagramAttrsA(struct Window *win, struct gDiagram *gd, struct TagItem *ti);
extern void RefreshPreviewSize(struct Window *win);
extern void UpdateDiagramGadgets(struct Window *win);
extern void UpdateObjectReferences(struct gObject *oldgo, struct gObject *newgo);
extern struct gLink * PUBLIC gGetLink(REG(a0, struct gDiagram *gd), REG(d0, long col), REG(d1, long row));


#endif   /* IGN_GRAHPIC_H */
