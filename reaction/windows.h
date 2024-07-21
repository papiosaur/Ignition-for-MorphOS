/*
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_WINDOWS_H
#define IGN_WINDOWS_H

#include "types.h"
#include "wdt.h"

#include <gadgets/listbrowser.h> // RA

struct gInterface;
struct Prefs;

#define APPIDCMP (IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_INTUITICKS | IDCMP_RAWKEY | IDCMP_VANILLAKEY | IDCMP_MENUHELP)

#define PROPGAD_HORIZ_ID 250
#define PROPGAD_VERT_ID  251
#define PROPGAD_UP_ID    252
#define PROPGAD_DOWN_ID  253
#define PROPGAD_LEFT_ID  254
#define PROPGAD_RIGHT_ID 255

#define GID_PAGE 39
#define GID_PAGEPOPPER 40
#define GID_POPPER 41
#define GID_FORM 42
#define GID_CHOOSE 43
#define GID_TOOLOBJ 44
#define GID_ICONOBJ 45

#define GID_FONT 46
#define GID_SIZE 47
#define GID_ZOOM 48
#define GID_POPFONT 49
#define GID_POPSIZE 50
#define GID_PLAIN 51
#define GID_BOLD 52
#define GID_ITALIC 53
#define GID_UNDERLINED 54
#define GID_APEN 55
#define GID_BPEN 56
#define GID_JLEFT 57
#define GID_JCENTER 58
#define GID_JRIGHT 59
#define GID_POPZOOM 60
#define GID_JTOP 61
#define GID_JVCENTER 62
#define GID_JBOTTOM 63
#define GID_POSITION 64
#define GID_STATUS 65

#define IMG_BOLD 0
#define IMG_ITALIC 1
#define IMG_UNDERLINED 2
#define IMG_JLEFT 3
#define IMG_JCENTER 4
#define IMG_JRIGHT 5
#define IMG_JTOP 6
#define IMG_JVCENTER 7
#define IMG_JBOTTOM 8

#define MAXGRADPENS 4     /* max. available colors for gradientslider */

#define POPUP_NO_SELECT_BARLABEL 42

#define BT_APPLICATION 0  /* Busy-Type */
#define BT_PROJECT 1

struct winData
{
    long   wd_Type;
    APTR   wd_Data;
    struct Gadget *wd_Gadgets;
    struct Gadget *wd_BorGads;
    UWORD  wd_TabX,wd_TabY,wd_TabW,wd_TabH;
    UWORD  wd_FormY,wd_FormH;
    UWORD  wd_ToolY,wd_ToolH;
    UWORD  wd_HelpY,wd_HelpH;
    UWORD  wd_IconY,wd_IconW,wd_IconH;
    UWORD  wd_BarW;
    struct MinList wd_Objs;
    void   ASM (*wd_Server)(REG(a0, struct TagItem *));
	void   ASM (*wd_CleanUp)(REG(a0, struct Window *), REG(d0, BOOL clean));
	struct Window *wd_Mother;
	struct Requester wd_Requester;
	struct Gadget **wd_Pages;
	struct Gadget *wd_PageHandlingGadget;
	char   wd_ShortCuts[16];
	long   wd_CurrentPage;
    struct LockNode *wd_Lock;
	union {
		APTR   ExtData[8];
		struct {
			struct gDiagram *wd_CurrentDiagram;
			struct Gadget **wd_Pages;
			APTR nix;
			struct gDiagram *wd_OldDiagram;
			struct MinList *wd_Gadgets;
			struct Gadget *wd_PageGadget;
		} diagram;
		struct {
			struct Page *wd_Page;
			struct List *wd_WeightStrings;
			LONG wd_Lines;
			APTR nix;
			struct MinList *wd_Gadgets;
		} object;
		struct {
			struct AppCmd *wd_AppCmd;
			struct Gadget *wd_ListView;
			struct Mappe *wd_Map;
		} definecmd;
	} u;
};

#define wd_ExtData u.ExtData

#define WA_Data			TAG_USER + 1000
#define WA_WinData		TAG_USER + 1001
#define WA_Mother		TAG_USER + 1002
#define WA_ExtData		TAG_USER + 1003
#define WA_CurrentDiagram WA_ExtData
#define WA_Page			TAG_USER + 1004
#define WA_Map			TAG_USER + 1005	// definecmd.wd_Map


#define WDT_ANY 421
#define WDT_ENDPREFS WDT_PREFCHOICE       // letzter Prefs-Fenstertyp


struct winObj {
    struct MinNode wo_Node;
    APTR   wo_Obj;
    short  wo_Type;
};

#define WOT_ICONGAD 1
#define WOT_ICONIMG 2
#define WOT_SCROLL 3
#define WOT_GADGET 4

struct wdtBorder {
    ULONG wb_Color;
    long  wb_Point;
    BOOL  wb_Adopt;
};

struct wdtNote {
    struct Node wn_Node;
    STRPTR wn_Note;
};


struct AttachedObj {
    struct MinNode ao_Node;
    struct ImageNode *ao_Object;
};

struct ImageObj {
    struct Node io_Node;
    WORD   io_OpenCnt;
    struct Image *io_Image;
    struct MinList io_Attached;
};

#define PROGRESSBAR_TEXTLEN 256
struct ProgressBar {
    struct RastPort *pb_RPort;
    char   pb_Text[PROGRESSBAR_TEXTLEN];
    LONG   pb_X,pb_Y,pb_Width,pb_Height;
    LONG   pb_BarWidth;
    float  pb_Percent;
};


/*************************** Drag&Drop ***************************/

#define DRAGT_PREFS 1
#define DRAGT_MENU 2
#define DRAGT_SUBMENU 4
#define DRAGT_APPCMD 8
#define DRAGT_ICONOBJ 16
#define DRAGT_ICONSEPARATOR 32


/*************************** Window creation and serialisation ***************************/

struct saveWin {
    struct MinNode sw_Node;
    long sw_Left,sw_Top;
    long sw_Width,sw_Height;
    STRPTR sw_Title;
    long sw_Type;
    struct winData *sw_WinData;
};

struct CreateWinData {
	int32	cwd_Title;
	STRPTR	cwd_HelpName;
	void	ASM (*cwd_Server)(REG(a0, struct TagItem *));
	void	ASM (*cwd_CleanUp)(REG(a0, struct Window *), REG(d0, BOOL clean));
	void	ASM (*cwd_Init)(REG(a0, struct winData *));
	void	ASM (*cwd_Gadgets)(REG(a0, struct winData *));
	void	(*cwd_ResizableGadgets)(struct winData *wd, int32 width, int32 height);
	uint32	cwd_IDCMP;
};

extern struct CreateWinData gCreateWinData[];

/*************************** Prototypes ***************************/

// gadgets.c
extern CONST_STRPTR GetGLabel(struct gInterface *gi);
extern void RemoveDiagramGadgets(struct winData *wd);
extern void AddDiagramGadgets(struct gDiagram *gd,struct Gadget *pgad,struct MinList *list,int width,int height);

// handlewindows.c
extern void PUBLIC RxMapLock(REG(a0, struct LockNode *ln),REG(a1, struct MinNode *node),REG(d0, UBYTE flags));
extern void PUBLIC ZoomWindowLock(REG(a0, struct LockNode *ln),REG(a1, struct MinNode *n),REG(d0, UBYTE flags));
extern void PUBLIC CellWindowLock(REG(a0, struct LockNode *),REG(a1, struct MinNode *),REG(d0, UBYTE));
extern void PUBLIC ScriptsWindowLock(REG(a0, struct LockNode *),REG(a1, struct MinNode *),REG(d0, UBYTE));

// handleprefs.c
extern void PUBLIC PrefIconAppCmdLock(REG(a0, struct LockNode *ln),REG(a1, struct MinNode *node),REG(d0, UBYTE flags));

// initwindows.c
extern void MakePrefIconAppCmds(struct Prefs *pr,struct List *list);

// popper.c
extern void SetPattern(struct RastPort *rp,UBYTE pattern,UBYTE x,UBYTE y);
extern void PUBLIC PatternPopper(REG(a0, struct RastPort *rp),REG(d0, UWORD x),REG(d1, UWORD y),REG(d2, UWORD cols),REG(d3, UWORD rows));
extern void PUBLIC ColorPopper(REG(a0, struct RastPort *rp),REG(d0, UWORD x),REG(d1, UWORD y),REG(d2, UWORD cols),REG(d3, UWORD rows));
extern long PopColors(struct Window *win,struct Gadget *gad);
#ifdef __amigaos4__
extern long PopUpTable(struct Window *win,struct Gadget *refgad,UWORD cols,UWORD rows,APTR func,...) VARARGS68K;
#else
extern long PopUpTable(struct Window *win,struct Gadget *refgad,UWORD cols,UWORD rows,APTR func,ULONG tag1,...) VARARGS68K;
#endif
#ifdef __amigaos4__
extern uint32 PopUpRAList(struct ColumnInfo *, struct List *, ...) VARARGS68K; // RA
extern long PopUpList(struct Window *,struct Gadget *,struct MinList *,...) VARARGS68K;
#else
extern long PopUpList(struct Window *,struct Gadget *,struct MinList *,ULONG tag1,...) VARARGS68K;
#endif
extern void ClosePopUpText(void);
extern void ShowPopUpText(STRPTR t,long wx,long wy);

// screen.c
extern void RefreshItemSize(void);
extern void InitAppScreen(struct Screen *scr);
extern void ChangeAppScreen(BOOL closescreen);
extern struct Screen *OpenAppScreen(void);
extern void CloseAppScreen(struct Screen *scr);
 
// windows.c
extern long DoRequest(CONST_STRPTR t,CONST_STRPTR gads,...) VARARGS68K;
extern void ErrorRequest(CONST_STRPTR t,...) VARARGS68K;
extern void PUBLIC ErrorRequestA(REG(a0, CONST_STRPTR t),REG(a1, APTR args));
extern void ErrorOpenLibrary(STRPTR lib,STRPTR paket);
extern void MakeLocaleLabels(const char *labels[], ULONG id, ...) VARARGS68K;
extern void MakeShortCutString(STRPTR shortCuts, ULONG ids, ...) VARARGS68K;
extern void SetBusy(BOOL set,BYTE type);
extern void DisposeProgressBar(struct ProgressBar *pb);
extern struct ProgressBar *NewProgressBar(struct Window *win,long x,long y,long w,long h);
//extern void CreateRAInfo(struct TextAttr *, CONST_STRPTR);
//extern void UpdateFuelGauge(struct Window *, CONST_STRPTR, uint8);
extern void UpdateProgressBar(struct ProgressBar *pb,CONST_STRPTR t,float p);
extern struct TextFont *OpenBitmapFont(struct TextAttr *textAttr);
extern void StandardNewSize(void (*create)(struct winData *,long,long));
extern BOOL WindowIsProjectDependent(long type);
extern void RefreshAppWindow(struct Window *win,struct winData *wd);
#ifdef __amigaos4__
extern struct Window *OpenAppWindow(long type,...) VARARGS68K;
#else
extern struct Window *OpenAppWindow(long type,ULONG tag1,...) VARARGS68K;
#endif
extern void CloseAppWindow(struct Window *win,BOOL);
extern void DisableGadget(struct Gadget *gad,struct Window *win,BOOL disable);
extern long CountGadToolGads(struct Window *win);
#ifdef __amigaos4__
extern APTR NewObj(struct winData *,short type,APTR,STRPTR,...) VARARGS68K;
#else
extern APTR NewObj(struct winData *,short type,APTR,STRPTR,ULONG tag1,...) VARARGS68K;
#endif
extern void DrawDithRect(struct RastPort *rp,long x1,long y1,long x2,long y2);
extern void DrawGroupBorder(struct RastPort *rp, CONST_STRPTR t,long x, long y, long w, long h);
extern void PUBLIC DrawRect(REG(a0, struct RastPort *rp),REG(d0, long x),REG(d1, long y),REG(d2, long w),REG(d3, long h));
extern void DrawRequesterBorder(struct Window *win);
extern void DrawPointSliderValue(struct RastPort *rp,struct Gadget *gad,WORD komma);
extern void DrawPatternField(struct RastPort *rp,struct Gadget *gad,ULONG col,BYTE pattern);
extern void DrawColorField(struct RastPort *rp,struct Gadget *gad,ULONG col,BOOL appcolors);
extern void freeLayerClip(struct Layer *l);
extern APTR makeLayerClip(struct Layer *l,long x,long y,long w,long h);
extern void freeClip(struct Window *win);
extern APTR makeClip(struct Window *win,long x,long y,long w,long h);
extern void RemoveGadgets(struct Gadget **firstgad,BOOL border);
extern struct Gadget *PageGadget(struct Gadget *gad,long num);
extern struct Gadget *GadgetAddress(struct Window *win,long num);
extern ULONG CountAppWindows(void);
extern struct Window *GetAppWindow(long type);
extern struct Window *GetAppWinTitle(STRPTR t);
extern struct Gadget *MakeBorderScroller(struct winData *);
extern struct Gadget *CreateToolGadget(struct winData *wd,long id,struct Gadget *gad);
extern struct Gadget *CreatePopGadget(struct winData *wd,struct Gadget *gad,BOOL disabled);
extern void FreeBorderGadgets(struct Gadget *gad,struct Window *win);

#endif  /* IGN_WINDOWS_H */
