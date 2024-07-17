/*
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_GADGETS_H
#define IGN_GADGETS_H


#ifndef __amigaos4__
	#include <gadgets/TextEdit.h>
	#include <gadgets/Scroller.h>
#endif

/*************************** PopUp-Gadget ***************************/

#define POPA_MaxItems   TAG_USER+42
#define POPA_CallBack   TAG_USER+43
#define POPA_ItemHeight TAG_USER+44
#define POPA_Selected   TAG_USER+45
#define POPA_Width      TAG_USER+46
#define POPA_MaxPen     TAG_USER+47
#define POPA_Left       TAG_USER+48


/*************************** ColorButton-Gadget ***************************/

#define CGA_Color    TAG_USER+70
#define CGA_Pen      TAG_USER+71
#define CGA_Recessed TAG_USER+72

struct ColorGData {
	ULONG cd_Color;
	ULONG cd_Pen;
	UBYTE cd_Recessed;
};


/*************************** Page-Gadget ***************************/

#define PAGEGA_Active      TAG_USER+80
#define PAGEGA_Pages       TAG_USER+81
#define PAGEGA_RefreshFunc TAG_USER+82
#define PAGEGA_BeginUpdatePage TAG_USER+83
#define PAGEGA_EndUpdatePage   TAG_USER+84

struct PageGData {
	struct Gadget **pd_Pages;
	ULONG  *pd_GadgetCount;
	struct Window *pd_Window;
	struct Gadget *pd_Gadgets;
	ULONG  pd_Active,pd_Count;
	void   ASM (*pd_Refresh)(REG(a0, struct Window *), REG(d0, LONG));
};


/*************************** Index-Gadget ***************************/

#define IGA_Active      PAGEGA_Active
#define IGA_Labels      GTCY_Labels

struct IndexGData {
	STRPTR *id_Labels;
	LONG   *id_Length;
	LONG   *id_Width;
	LONG   *id_Pos;
	// BYTE   *id_Flags;
	ULONG  id_Active,id_Current,id_NewCurrent;
	ULONG  id_Count,id_MinWidth;
	UWORD  id_Space,id_TextSpace;
	struct DrawInfo *id_dri;
};


/*************************** Edit-Gadget ***************************/

/*
	Mazze: removed struct EditGData. Must be inserted in a new header
	which is shared with pTextEdit.gadget.
*/

#define EDJ_JUSTIFY 0       /* justified lines */
#define EDJ_LEFT 1          /* left-aligned    */
#define EDJ_RIGHT 2         /* right-aligned   */
#define EDJ_CENTERED 3      /* centered        */

struct EditLine {
	STRPTR el_Word;
	UWORD  el_Length;
	UWORD  el_Width;
	UBYTE  el_Type;
};

#define EDITLINE(mln) ((struct EditLine *)((UBYTE *)mln+sizeof(ULONG)+sizeof(struct MinNode)))
#define LINEOFFSET(mln) (*(LONG *)(mln+1))


/*************************** Arrow-Image ***************************/

struct ArrowIData {
	struct Image *ad_Frame;
	long   ad_Type;
};

#define ADT_UP 0
#define ADT_DOWN 1


/*************************** Bitmap-Image ***************************/

#define BIA_Bitmap         TAG_USER+200
#define BIA_SelectedBitmap TAG_USER+201

struct BitmapIData {
	struct BitMap *bd_Bitmap;
	struct BitMap *bd_SelectedBitmap;
};


/*************************** Picture-Image ***************************/

#define PIA_FromImage      TAG_USER+100
#define PIA_WithColors     TAG_USER+101
#define PIA_BitMap         TAG_USER+102
#define PIA_DelayLoad      TAG_USER+103

#define PIM_LOAD           TAG_USER+104

struct PictureIData {
	struct Screen *pd_Screen;
	struct BitMap *pd_SourceBitMap,*pd_BitMap;
	struct ColorRegister *pd_ColorMap;
	LONG   *pd_ColorRegs;
	Object *pd_Remapped;
	ULONG  pd_NumColors;
	UBYTE  pd_Depth;
	STRPTR pd_Name;
};


/*************************** gpDomain ***************************/

/* taken from the ADCD - G1 */

#ifndef GM_DOMAIN
#define GM_DOMAIN       (7)

struct gpDomain {
	ULONG                MethodID;
	struct GadgetInfo   *gpd_GInfo;
	struct RastPort     *gpd_RPort;     /* RastPort to layout for */
	LONG                 gpd_Which;
	struct IBox          gpd_Domain;    /* Resulting domain */
	struct TagItem      *gpd_Attrs;     /* Additional attributes */
};

#define GDOMAIN_MINIMUM         (0)
#define GDOMAIN_NOMINAL         (1)
#define GDOMAIN_MAXIMUM         (2)

#endif  /* GM_DOMAIN */


/*************************** Link ***************************/

struct Link {
	struct MinNode l_Node;
	APTR   l_Link;
	ULONG  ASM (*l_HookFunction)(REG(a0, APTR), REG(a2, APTR), REG(a1, struct LVDrawMsg *));
};

#endif  /* IGN_GADGETS_H */

