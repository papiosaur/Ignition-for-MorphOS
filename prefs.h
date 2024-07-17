/*
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_PREFS_H
#define IGN_PREFS_H


#define GetLocalPrefs(mp) (mp ? (mp != (APTR)~0L ? &((struct Mappe *)mp)->mp_Prefs : &recycledprefs) : &prefs)
#define GetPrefsMap(pr) ((pr == &prefs || pr == &recycledprefs) ? NULL : (struct Mappe *)((UBYTE *)pr - (UBYTE *)&((struct Mappe *)0L)->mp_Prefs))
#define GetRealMap(mp) (mp == (APTR)~0L ? NULL : mp)

#define ID_IGNP  	MAKE_ID('I','G','N','P')
#define ID_VERSION 	MAKE_ID('V','E','R','S')
#define IGNP_VERSION 0

#define ID_CMDS  	MAKE_ID('C','M','D','S')
#define ID_MENU  	MAKE_ID('M','E','N','U')
#define ID_ICON  	MAKE_ID('I','C','O','N')
#define ID_DISP  	MAKE_ID('D','I','S','P')
#define ID_KEYS  	MAKE_ID('K','E','Y','S')
#define ID_COLORS   MAKE_ID('C','O','L','S')
#define ID_SCREEN   MAKE_ID('S','C','R','N')
#define ID_FILE   	MAKE_ID('F','I','L','E')
#define ID_FORMAT 	MAKE_ID('F','M','T',' ')
#define ID_NAMES 	MAKE_ID('N','M','E','S')
#define ID_TABLE  	MAKE_ID('T','A','B','L')
#define ID_SYSTEM  	MAKE_ID('S','Y','S',' ')
#define ID_CONTEXT  MAKE_ID('C','T','M',' ')

#define PRF_DISPLAY (1 << 0)
#define PRF_FILE 	(1 << 1)
#define PRF_SCREEN 	(1 << 2)
#define PRF_KEYS 	(1 << 3)
#define PRF_FORMAT 	(1 << 4)
#define PRF_PRINTER (1 << 5)
#define PRF_COLORS 	(1 << 6)
#define PRF_TABLE 	(1 << 7)
#define PRF_ICON 	(1 << 8)
#define PRF_CMDS 	(1 << 9)
#define PRF_MENU 	(1 << 10)
#define PRF_SYSTEM 	(1 << 11)
#define PRF_NAMES 	(1 << 12)
#define PRF_CONTEXT (1 << 13)

#define PRF_ALL 	0xffff

#define PRF_KEEPOLDCONTENTS (1 << 30)
#define PRF_ADDCONTENTS 	(1 << 31)

/****/

struct PrefDisp {
	BYTE   pd_Rasta;
	struct TextFont *pd_AntiFont;
	struct TextAttr pd_AntiAttr;
	BYTE   pd_HelpBar, pd_ToolBar;
	BYTE   pd_IconBar, pd_FormBar;
	BYTE   pd_ShowAntis;
	long   pd_AntiWidth, pd_AntiHeight;
};

#define PDR_NONE 		0       // Rasta look
#define PDR_POINTS 		1
#define PDR_LINE		2
#define PDR_CYCLES 		3
#define PDR_CELLWIDTH 	4

#define PDIB_NONE 		0      // Bar positions
#define PDIB_LEFT 		1
#define PDIB_TOP 		2
#define PDFB_NONE 		0
#define PDFB_BOTTOM 	1
#define PDFB_TOP 		2

/****/

#define PS_NAMELEN 		64

struct PrefScreen {
	UWORD  ps_Width, ps_Height, ps_Depth;
	UWORD  ps_dimWidth, ps_dimHeight;
	long   ps_ModeID, ps_Overscan;
	BYTE   ps_Interleaved;
	short  ps_Type;
	char   ps_PubName[PS_NAMELEN];
	BYTE   ps_BackFill;
	LONG   ps_BFColor;
	ULONG  ps_mmWidth, ps_mmHeight;
	struct TextFont *ps_Font;
	struct TextAttr ps_TextAttr;
};

#define PREFSCREEN_SIZE (sizeof(struct PrefScreen) - sizeof(struct TextAttr) - sizeof(APTR))

#define PST_PUBLIC 	0
#define PST_LIKEWB 	1
#define PST_OWN 	2

/****/

struct PrefFile {
	UWORD  pf_Flags;
	UBYTE  pf_AutoSave;
	long   pf_AutoSaveIntervall;     /* in seconds */
};

#define PFF_ICONS 		1         /* pf_Flags */
#define PFF_BACKUP 		2
#define PFF_NOSUFFIX 	4
#define PFF_WARN_IOTYPE	8

#define PFAS_OFF 		0          /* pf_AutoSave */
#define PFAS_REMEMBER 	1
#define PFAS_ON 		2

/****/

#define PTEQ_NONE 		0      /* kein Qualifier */
#define PTEQ_SHIFT 		1
#define PTEQ_ALT 		2
#define PTEQ_SHIFTALT 	3
#define PTEQ_CONTROL 	4
#define PTEQ_NUM 		5

#define PTEF_NONE 		0      /* Ziehecke-Funktionen */
#define PTEF_COPY 		1
#define PTEF_SINGLECOPY 2
#define PTEF_MOVE 		3
#define PTEF_SWAP 		4
#define PTEF_NUM 		5

#define PTEF_FUNCMASK 	127
#define PTEF_TEXTONLY 	128

struct PrefTable {
	UWORD pt_Flags;
	UBYTE pt_EditFunc[PTEQ_NUM];
};

#define PTF_SHOWFORMULA 	1
#define PTF_SHOWZEROS 		2
#define PTF_AUTOCELLSIZE 	4
#define PTF_AUTOCENTURY 	8
#define PTF_MARKSUM 		16
#define PTF_MARKAVERAGE 	32
#define PTF_EDITFUNC 		64

/****/

struct PrefIcon {
	UBYTE  pi_Width,pi_Height;
	UBYTE  pi_Spacing;
};

/****/

struct SpecialMenuItem {
	struct MenuItem smi_Item;
	APTR   smi_UserData;
	ULONG  smi_Magic;
	LONG   smi_Data;
};

#define SMIM_PROJECTS 	MAKE_ID('P','R','O','J')
#define SMIM_PREFS    	MAKE_ID('P','R','E','F')
#define SMIM_SESSION  	MAKE_ID('S','E','S','S')

#define NUM_SESSION 	4

struct MenuSpecial {
	APTR   ms_Parent;
	struct MenuItem *ms_Item;
	struct LockNode *ms_Lock;
	UBYTE  ms_Type;
};

#define MST_FIRSTITEM 		0
#define MST_ITEM 			1
#define MST_FIRSTSUBITEM 	2

/****/

struct Prefs {
	struct TreeNode *pr_TreeNode;
	struct PrefDisp *pr_Disp;
	struct PrefScreen *pr_Screen;
	struct PrefFile *pr_File;
	struct PrefTable *pr_Table;
	struct PrefIcon *pr_Icon;
	struct IBox pr_WinPos[NUM_WDT];
	struct MinList pr_AppCmds;
	struct MinList pr_AppMenus;
	struct MinList pr_AppKeys;
	struct MinList pr_IconObjs;
	struct MinList pr_Names,pr_Formats;
	struct Menu *pr_Menu;
	struct MenuSpecial pr_ProjectsMenu;
	struct MenuSpecial pr_SessionMenu;
	struct MinList pr_Contexts[NUM_CMT];
	ULONG  pr_Flags;
	struct MinList pr_Modules;
	ULONG  pr_ModuleFlags;
};

#define PRF_APPICON			1
#define PRF_CLIPGO 			2
#define PRF_SIMPLEWINDOWS 	4
#define PRF_SIMPLEPROJS 	8
#define PRF_CONTEXTMENU 	16
#define PRF_USESESSION 		32
#define PRF_SECURESCRIPTS 	64
#define PRF_USEDBUFFER 		128

struct PrefsModule {
	struct ImageNode pm_Node;
	STRPTR pm_ImageName;
	UWORD  pm_Type;
	UBYTE  pm_Flags;
	APTR   pm_Data;           // wird noch nicht benutzt
};

#define PMF_MODIFIED 	1
#define PMF_GLOBAL 		2
#define PMF_ADD 		4
#define PMF_REPLACE 	8
#define PMF_ADD_REPLACE_MASK (PMF_ADD | PMF_REPLACE)

#define LNT_PREFSMODULE 242


/*************************** Prototypes ***************************/

// I/O
extern void PropPrefsChunks(struct IFFHandle *iff,LONG context,LONG flags);
extern long SaveProjectPrefs(struct IFFHandle *iff,struct Prefs *pr);
extern void SavePrefs(struct Prefs *prefs,STRPTR name,long flags);
extern void lpCmds(struct IFFHandle *iff,LONG context,struct Prefs *pr,BYTE add,BYTE keep);
extern void lpMenu(struct IFFHandle *iff,LONG context,struct Prefs *pr);
extern void lpDisp(struct IFFHandle *iff,LONG context,struct Prefs *pr);
extern void lpKeys(struct IFFHandle *iff,LONG context,struct Prefs *pr);
extern void lpContext(struct IFFHandle *iff,LONG context,struct Prefs *pr);
extern void lpIcon(struct IFFHandle *iff,LONG context,struct Prefs *pr);
extern void lpFormat(struct IFFHandle *iff,LONG context,struct Prefs *pr,struct MinList *list);
extern void lpNames(struct IFFHandle *iff,LONG context,struct Prefs *pr);
extern ULONG LoadPrefs(struct Prefs *prefs,STRPTR name,BPTR lock,long flags);

// base
extern void InitPrefsGadgetsLabels(void);
extern void PUBLIC AppCmdLock(REG(a0, struct LockNode *ln),REG(a1, struct MinNode *node),REG(d0, UBYTE flags));
extern void AddStandardOutput(void);
extern STRPTR MakePrefsTitle(struct Mappe *mp, uint8 type, STRPTR prefsTitle);
extern void RefreshMapTexts(struct Mappe *mp,BOOL all);
extern void UpdateAntiFont(struct Prefs *pr);
extern struct Window *GetPrefsWindow(struct Prefs *pr,UWORD type);
extern ULONG GetPrefsModuleType(ULONG flag);
extern BOOL HasPrefsModule(struct Prefs *pr,UWORD type);
extern struct PrefsModule *GetPrefsModule(struct Prefs *pr,UWORD type);
extern BOOL SetPrefsModule(struct Prefs *pr,UWORD type,BYTE modified);
extern void RemPrefsModule(struct Prefs *pr,struct PrefsModule *pm);
extern struct PrefsModule *AddPrefsModule(struct Prefs *pr,CONST_STRPTR t,STRPTR iname,UWORD type,UBYTE flags);
extern void AddPrefsModuleToTree(struct Prefs *pr,struct PrefsModule *pm,struct MinList *tree);
extern void AddPrefsModulesToTree(struct Prefs *pr,struct MinList *tree);
extern void AddPrefsModuleToLocalPrefs(struct Mappe *mp,UWORD type);
extern void FreePrefsModuleData(struct Prefs *pr,struct PrefsModule *pm);
extern void AllocPrefsModuleData(struct Prefs *pr,UWORD type);
extern void CopyPrefsModule(struct TreeNode *source,struct TreeNode *tn,struct TreeNode *target);
extern void RefreshPrefsModule(struct Prefs *pr,struct PrefsModule *pm,UWORD type);
extern void RefreshPrefsModules(struct Prefs *pr,LONG modules);
extern void RefreshPrefsTreeNode(struct TreeNode *tn);
extern void RefreshMapPrefs(struct Mappe *mp);
extern void PropagatePrefsModuleMap(struct Prefs *pr,struct Mappe *mp,UWORD type);
extern void PropagatePrefsModule(struct Prefs *pr,UWORD type);
extern void PropagatePrefsToMap(struct Prefs *pr,struct Mappe *mp);
extern void PropagatePrefs(struct Prefs *pr);
extern void RefreshIconSizeFromList(struct PrefIcon *pi,struct MinList *list,BOOL link);
extern void RefreshIconSize(struct Prefs *pr);
extern void MakeCmdsLinks(struct Mappe *mp);
extern void MakeNamesLinks(struct Mappe *mp);
extern void MakeFormatLinks(struct Mappe *mp);
extern void showPrefColorsMode(struct Window *win,long mode);
extern void InitPrefs(struct Prefs *prefs);
extern void InitAppPrefs(STRPTR name);

#endif   /* IGN_PREFS_H */

