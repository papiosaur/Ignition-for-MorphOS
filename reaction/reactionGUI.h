/*
************************************************************
**
** Created by: CodeBench 0.42 (14.10.2013)
** Project: Ignition
** File: reactionGUI.h
** Date: 03-04-2019 17:26:58
**
************************************************************
*/
#ifndef REACTIONGUI_H
#define REACTIONGUI_H

#ifdef __amigaos4__

#include <proto/chooser.h>
#include <proto/integer.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/getfont.h>
#include <proto/getfile.h>
#include <proto/getscreenmode.h>
#include <proto/radiobutton.h>
#include <proto/clicktab.h>
#include <proto/palette.h>
#include <proto/listbrowser.h>
#include <proto/string.h>
#include <proto/colorwheel.h>
//#include <proto/texteditor.h>

#include <classes/window.h>
#include <classes/requester.h>
#include <gadgets/chooser.h>
#include <gadgets/integer.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/getfont.h>
#include <gadgets/getfile.h>
#include <gadgets/getscreenmode.h>
#include <gadgets/radiobutton.h>
#include <gadgets/clicktab.h>
#include <gadgets/palette.h>
#include <gadgets/listbrowser.h>
#include <gadgets/string.h>
#include <gadgets/gradientslider.h>
#include <gadgets/colorwheel.h>
#include <gadgets/getcolor.h>
#include <gadgets/slider.h>
#include <gadgets/space.h>
#include <gadgets/texteditor.h>
#include <gadgets/scroller.h>
#include <images/bevel.h>
#include <images/label.h>
#include <gadgets/fuelgauge.h>


#define OBJ(x)  objects[x]
#define GAD(x)  (struct Gadget *)objects[x]


enum // window objects we need to get/set
{
	OID_TEMPORAL = 0, // "temporal" gadgets to add to winobj[]

																			// Prefs windows
	OID_D_GRID,																// DISPLAY prefs [1]
	OID_D_CELLWIDTH,
	OID_D_HEADINGS,
	OID_D_FONT,
	OID_D_HELPBAR,
	OID_D_TOOLBAR,
	OID_D_ICONBAR,
	OID_D_FORMULABAR,

	OID_F_PATHDOCS,															// FILE prefs [9]
	OID_F_PATHGFX,
	OID_F_PATHICONS,
	OID_F_CREATEICONS,
	OID_F_BACKUP,
	OID_F_NOEXT,
	OID_F_WARNIOFMT,
	OID_F_AUTOSAVE,
	OID_F_AUTOSAVETIME,

	OID_S_SCREENTYPE,														// SCREEN prefs [18]
	OID_S_SCRGROUP,
	OID_S_SCREENNAME,
	OID_S_SELECTSCR,
	OID_S_SELECTFONT,
	OID_S_MONITORW,
	OID_S_MONITORH,
	OID_S_BGPATTERN,
	OID_S_PALETTE,

	OID_S_APPICON,															// SYSTEM prefs [27]
	OID_S_SESSIONS,
	OID_S_CHECKSEC,
	OID_S_CTXTMENU,
	OID_S_REFRESH,
	OID_S_CLIMBLST,
	OID_S_CLIPUNIT,

	OID_T_OPENERRREQ,														// TABLE prefs [34]
	OID_T_ZEROUNKNOWN,
	OID_T_QUALIFIER,
	OID_T_QUALFUNC,
	OID_T_SHOWDRAGGABLE,
	OID_T_ONLYTEXT,
	OID_T_SHOWZEROS,
	OID_T_SHOWFORMULAS,
	OID_T_ADJCELL,
	OID_T_CENTURY,
	OID_T_SHOWBLOCK,

	OID_N_LISTNAMES,														// NAMES prefs [45]
	OID_N_LNBUTTONS,
	OID_N_NEW,
	OID_N_COPY,
	OID_N_DELETE,
	OID_N_COL2GROUP,
	OID_N_NAME,
	OID_N_CONTENTS,
	OID_N_TYPE,
	OID_N_REFERENCE,

	OID_P_COLGROUP,															// COLORS/PALETTE prefs [55]
	OID_P_COLNAME,
	OID_P_COLOR,
	OID_P_COLRGB,
	OID_P_COLHSB,
	//OID_P_COLORWHEEL,
	//OID_P_COLORSLIDER,
	//OID_P_RGBHSB,
	//OID_P_COL_RH,
	//OID_P_COL_GS,
	//OID_P_COL_BB,
	//OID_P_COLOR,
	OID_P_PALGROUP,
	OID_P_PALETTE,
	OID_P_LISTCOLORS,
	OID_P_PAL_NEW,
	OID_P_PAL_COPY,
	OID_P_PAL_DEL,
	OID_P_PAL_NAME,

	OID_K_LISTKEYS,															// KEYS prefs [67]
	OID_K_NEW,
	OID_K_DEL,
	OID_K_KBGROUP,
	OID_K_RECORD,
	OID_K_POPUPBTN,
	OID_K_KEYLABEL,
	OID_K_KEYPRESS,															// PRESSKEY [CreatePressKeyGads()]

	OID_C_CONTEXT,															// CONTEXT prefs [75]
	OID_C_LISTCONTEXT,
	OID_C_BTNGROUP,
	OID_C_UP,
	OID_C_DOWN,
	OID_C_NEW,
	OID_C_DEL,
	OID_C_TITLE,
	OID_C_CMDGROUP,
	OID_C_COMMAND,
	OID_C_TYPE,
	OID_C_POPUPBTN,

	OID_M_LISTMENUS,														// MENU prefs [87]
	OID_M_BTNGROUP_M,
	OID_M_UP_M,
	OID_M_DOWN_M,
	OID_M_NEW_M,
	OID_M_DEL_M,
	OID_M_TITLE_M,
	OID_M_LISTITEMS,		// items
	OID_M_BTNGROUP_I,
	OID_M_UP_I,
	OID_M_DOWN_I,
	OID_M_TO_SUBITEM,
	OID_M_NEW_I,
	OID_M_DEL_I,
	OID_M_TITLE_I,
	OID_M_GROUP_I,
	OID_M_HOTKEY_I,
	OID_M_COMMAND_I,
	OID_M_TYPE_I,
	OID_M_POPUPBTN_I,
	OID_M_LISTSUBITEMS,		// sub-items
	OID_M_BTNGROUP_SI,
	OID_M_UP_SI,
	OID_M_DOWN_SI,
	OID_M_TO_ITEM,
	OID_M_NEW_SI,
	OID_M_DEL_SI,
	OID_M_TITLE_SI,
	OID_M_GROUP_SI,
	OID_M_HOTKEY_SI,
	OID_M_COMMAND_SI,
	OID_M_TYPE_SI,
	OID_M_POPUPBTN_SI,

	OID_I_LISTICONS,														// ICON prefs [120]
	OID_I_LISTTOOLBAR,
	//OID_I_BTNGROUP,
	OID_I_TO_TB,
	OID_I_UP,
	OID_I_DOWN,
	OID_I_SEPARATOR,
	OID_I_DEL,

	OID_F_LISTFORMATS,														// FORMAT prefs [127]
	OID_F_NEW,
	OID_F_COPY,
	OID_F_DEL,
	OID_F_GROUP_PROP,
	OID_F_TYPE,
	OID_F_PRIORITY,
	OID_F_FORMAT,
	OID_F_FMT_POPBTN,
	OID_F_DECIMALS,
	OID_F_ALIGN,
	OID_F_GROUP_OPTS,
	OID_F_NEGCOLOR,
	OID_F_NEGCOL_SAMPLE,
	OID_F_NEGCOL_POPBTN,
	OID_F_NEGVAL,
	OID_F_NEGSEPARATOR,

	OID_D_LABEL,															// DEFINECOMD prefs [144]
	OID_D_ICONIMG,
	OID_D_ICONSEL,
	OID_D_LISTCMDS,
	OID_D_NEW,
	OID_D_DEL,
	OID_D_GROUP_CMD,
	OID_D_COMMAND,
	OID_D_POPUPBNT,
	OID_D_CMDTYPE,
	OID_D_OUTPUT,
	OID_D_OUTPUTSEL,
	OID_D_HELPTEXT,
	OID_D_HYPERTEXT,

	OID_P_LISTCMDS,											// PREFCMDS prefs [158]
	//OID_P_GROUP_BTN,
	OID_P_NEW,
	OID_P_COPY,
	OID_P_DEL,

																			// Other windows
	OID_DI_AUTHOR,															// DOCINFO window [162]
	OID_DI_VERSION,
	OID_DI_KEYWORDS,
	OID_DI_COMMENTS,
	OID_DI_VSCROLL,

	OID_CN_LISTNOTES,														// CREATENOTES window [167]
	OID_CN_VSCROLL,
	OID_CN_INSERT,

	OID_SC_LISTSCRIPTS,														// SCRIPTS window [170]
	OID_SC_NEW,
	OID_SC_DEL,
	OID_SC_EDITGROUP,
	OID_SC_NAME,
	OID_SC_DESCRIPTION,
	OID_SC_TXTEDGROUP,
	OID_SC_SCRIPT,
	OID_SC_VSCROLL,
	OID_SC_HSCROLL,
	OID_SC_EDIT,

	OID_ABOUT_OBJ,																					// WDT_INFO [181]
	OID_ABOUT_FOOTER,

	LAST_NUM
};

enum // those one don't have an OBJ() assigned, just are events/triggers
{
	OID_OK = 1009,															// aka 9
	OID_TEST,																// aka 10
	OID_CANCEL,																// aka 11
	OID_DELETE,
	OID_POPUP																// popper.c PopUpRAList()
};

extern Object *objects[LAST_NUM], *winobj[NUM_WDT];
extern ULONG sigwait;
extern uint32 wsigmask;

// The class pointer
extern Class *CheckBoxClass, *ButtonClass, *ClickTabClass, *LabelClass, *LayoutClass, *GetFontClass,
             *GetFileClass, *GetScreenModeClass, *IntegerClass, *RadiobuttonClass, *PaletteClass,
             *StringClass, *WindowClass, *RequesterClass, *ChooserClass, *ListBrowserClass,
             *GradientSliderClass, *ColorWheelClass, *GetColorClass, *SliderClass, *SpaceClass,
             *BevelClass, *TexteditorClass, *ScrollerClass, *FuelGaugeClass;
// Some interfaces needed
extern struct ChooserIFace *IChooser;
extern struct ListBrowserIFace *IListBrowser;
//extern struct GradientSliderIFace *IGradientSlider;
//extern struct ColorWheelIFace *IColorWheel;


extern BOOL openRAresources(void);
extern void closeRAresources(void);

#endif

#endif
