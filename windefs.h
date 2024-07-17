/*
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_WINDEFS_H
#define IGN_WINDEFS_H
 

#include "windows.h"

// handlewindows.c
extern void ASM closeNotesWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM handleNotesIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseFormelWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleFormelIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseBorderWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleBorderIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePreviewIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseDiagramWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleDiagramIDCMP(REG(a0, struct TagItem *));
extern void ASM HandlePageSetupIDCMP(REG(a0, struct TagItem *tag));
extern void ASM HandleDocInfoIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseDocumentWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleDocumentIDCMP(REG(a0, struct TagItem *));
extern void ASM HandlePageIDCMP(REG(a0, struct TagItem *));
extern void ASM handleCellSizeIDCMP(REG(a0, struct TagItem *));
extern void ASM handleZoomIDCMP(REG(a0, struct TagItem *));
extern void ASM closeGClassesWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM handleGClassesIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseObjectWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleObjectIDCMP(REG(a0, struct TagItem *));
extern void ASM handleCommandIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseFileTypeWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closeCellWin(REG(a0, struct Window *win),REG(d0, BOOL clean));
extern void ASM handleCellIDCMP(REG(a0, struct TagItem *));
extern void ASM handleSetNameIDCMP(REG(a0, struct TagItem *tag));
extern void ASM handleSetTitleIDCMP(REG(a0, struct TagItem *tag));
extern void ASM CloseScriptsWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleScriptsIDCMP(REG(a0, struct TagItem *tag));

// handleprefs.c
extern void ASM handlePrefsIDCMP(REG(a0, struct TagItem *));
extern void ASM HandlePrefChoiceIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefDispIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefMenuIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefIconIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePressKeyIDCMP(REG(a0, struct TagItem *));
extern void ASM HandleKeyboardPrefsIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefScreenIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefCmdsIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefNamesIDCMP(REG(a0, struct TagItem *));
extern void ASM HandleFormatPrefsIDCMP(REG(a0, struct TagItem *));
extern void ASM HandleFilePrefsIDCMP(REG(a0, struct TagItem *));
extern void ASM HandleTablePrefsIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefColorsIDCMP(REG(a0, struct TagItem *));
extern void ASM handleDefineCmdIDCMP(REG(a0, struct TagItem *));
extern void ASM HandleSystemPrefsIDCMP(REG(a0, struct TagItem *));
extern void ASM handlePrefContextIDCMP(REG(a0, struct TagItem *tag));
extern void ASM closePrefsWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM ClosePrefChoiceWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closePrefDispWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM CloseKeyboardPrefsWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closePrefMenuWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closePrefIconWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closePrefColorsWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM CloseFilePrefsWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM CloseTablePrefsWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closePrefNamesWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM CloseFormatPrefsWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closePrefCmdsWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closeDefineCmdWin(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM closePrefContextWindow(REG(a0, struct Window *win),REG(d0, BOOL clean));

// database.c
extern void ASM CloseDatabaseWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleDatabaseIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseMaskWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleMaskIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseIndexWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandleIndexIDCMP(REG(a0, struct TagItem *));
extern void ASM HandleFilterIDCMP(REG(a0, struct TagItem *));
extern void ASM CloseFilterWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM CreateDatabaseGadgets(REG(a0, struct winData *wd));
extern void ASM CreateMaskGadgets(REG(a0, struct winData *wd));
extern void ASM CreateIndexGadgets(REG(a0, struct winData *wd));
extern void ASM CreateFilterGadgets(REG(a0, struct winData *wd));

// search.c
extern void ASM CreateFindReplaceGadgets(REG(a0, struct winData *wd));
extern void ASM CloseFindReplaceWindow(REG(a0, struct Window *win),REG(d0, BOOL clean));
extern void ASM HandleFindReplaceIDCMP(REG(a0, struct TagItem *));

// paste.c
extern void CreateClipboardGadgets(struct winData *wd, long wid, long hei);
extern void ASM handleClipIDCMP(REG(a0, struct TagItem *));

// printer.c
extern void ASM ClosePrinterWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandlePrinterIDCMP(REG(a0, struct TagItem *));
extern void ASM CreatePrinterGadgets(REG(a0, struct winData *wd));
extern void ASM ClosePrintStatusWindow(REG(a0, struct Window *),REG(d0, BOOL));
extern void ASM HandlePrintStatusIDCMP(REG(a0, struct TagItem *));
extern void ASM CreatePrintStatusGadgets(REG(a0, struct winData *wd));

// prefsgadgets.c
extern void CreatePrefsGadgets(struct winData *wd, long wid, long hei);
extern void ASM CreatePrefChoiceGads(REG(a0, struct winData *wd));
extern void ASM CreatePrefDispGads(REG(a0, struct winData *wd));
extern void ASM CreatePrefScreenGads(REG(a0, struct winData *wd));
extern void ASM CreatePrefMenuGads(REG(a0, struct winData *wd));
extern void CreateKeyboardPrefsGadgets(struct winData *wd, long width, long height);
extern void ASM CreatePrefIconGads(REG(a0, struct winData *wd));
extern void ASM CreatePrefCmdsGads(REG(a0, struct winData *wd));
extern void ASM CreatePrefNamesGads(REG(a0, struct winData *wd));
extern void ASM CreateFormatPrefsGadgets(REG(a0, struct winData *wd));
extern void ASM CreateFilePrefsGadgets(REG(a0, struct winData *wd));
extern void ASM CreateTablePrefsGadgets(REG(a0, struct winData *wd));
extern void ASM CreatePrefColorsGads(REG(a0, struct winData *wd));
extern void ASM CreateDefineCmdGads(REG(a0, struct winData *wd));
extern void ASM CreatePressKeyGads(REG(a0, struct winData *wd));
extern void ASM CreateSystemPrefsGadgets(REG(a0, struct winData *wd));
extern void ASM CreatePrefContextGads(REG(a0, struct winData *wd));

// gadgets.c
extern void ASM CreateInfoGadgets(REG(a0, struct winData *wd));
extern void ASM CreateBorderGadgets(REG(a0, struct winData *wd));
extern void ASM CreateDiagramGadgets(REG(a0, struct winData *wd));
extern void ASM CreatePaletteGadgets(REG(a0, struct winData *wd));
extern void ASM CreateFormelGadgets(REG(a0, struct winData *wd));
extern void ASM CreateFormatGadgets(REG(a0, struct winData *wd));
extern void ASM CreatePageSetupGadgets(REG(a0, struct winData *wd));
extern void ASM CreateDocInfoGadgets(REG(a0, struct winData *wd));
extern void ASM CreateDocumentGadgets(REG(a0, struct winData *wd));
extern void ASM CreatePageGadgets(REG(a0, struct winData *wd));
extern void ASM CreateAlignGadgets(REG(a0, struct winData *wd));
extern void ASM CreateCellSizeGadgets(REG(a0, struct winData *wd));
extern void ASM CreateZoomGadgets(REG(a0, struct winData *wd));
extern void ASM CreateColorsGadgets(REG(a0, struct winData *wd));
extern void ASM CreateObjectGadgets(REG(a0, struct winData *wd));
extern void ASM CreateCommandGadgets(REG(a0, struct winData *wd));
extern void ASM CreateFileTypeGadgets(REG(a0, struct winData *wd));
extern void ASM CreateCellGadgets(REG(a0, struct winData *wd));
extern void ASM CreateSetNameGadgets(REG(a0, struct winData *wd));

extern void CreateGClassesGadgets(struct winData *wd, long wid, long hei);
extern void CreateScriptsGadgets(struct winData *wd, long wid, long hei);
extern void CreateNotesGadgets(struct winData *wd, long wid, long hei);

// initwindows.c
extern void ASM InitBorderWindow(REG(a0, struct winData *wd));
extern void ASM InitDatabaseWindow(REG(a0, struct winData *wd));
extern void ASM InitDiagramWindow(REG(a0, struct winData *wd));
extern void ASM InitDocumentWindow(REG(a0, struct winData *wd));
extern void ASM InitFileTypeWindow(REG(a0, struct winData *wd));
extern void ASM InitFindReplaceWindow(REG(a0, struct winData *wd));
extern void ASM InitFormelWindow(REG(a0, struct winData *wd));
extern void ASM InitIndexWindow(REG(a0, struct winData *wd));
extern void ASM InitFilterWindow(REG(a0, struct winData *wd));
extern void ASM InitMaskWindow(REG(a0, struct winData *wd));
extern void ASM initPrefNames(REG(a0, struct winData *wd));
extern void ASM initNotes(REG(a0, struct winData *wd));
extern void ASM InitObjectWindow(REG(a0, struct winData *wd));
extern void ASM initPrefChoiceWindow(REG(a0, struct winData *wd));
extern void ASM initPrefColors(REG(a0, struct winData *wd));
extern void ASM InitFilePrefsWindow(REG(a0, struct winData *wd));
extern void ASM InitFormatPrefsWindow(REG(a0, struct winData *wd));
extern void ASM initPrefIcon(REG(a0, struct winData *wd));
extern void ASM InitKeyboardPrefsWindow(REG(a0, struct winData *wd));
extern void ASM initPrefContext(REG(a0, struct winData *wd));
extern void ASM initPrefMenu(REG(a0, struct winData *wd));
extern void ASM InitTablePrefsWindow(REG(a0, struct winData *wd));
extern void ASM InitPrinterWindow(REG(a0, struct winData *wd));
extern void ASM InitPrintStatusWindow(REG(a0, struct winData *wd));


struct CreateWinData gCreateWinData[] = {
/**************************** misc ****************************/

	{MSG_INFO_TITLE, NULL,													/* WDT_INFO */
	 NULL, NULL, NULL, CreateInfoGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_MOUSEBUTTONS},
	{MSG_CLIPBOARD_TITLE, "clibboard",										/* WDT_CLIP */
	 handleClipIDCMP, NULL, NULL, NULL, CreateClipboardGadgets,
     APPIDCMP | LISTVIEWIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE},
	{MSG_FIND_TITLE, "search",												/* WDT_FIND */
	 HandleFindReplaceIDCMP, CloseFindReplaceWindow, InitFindReplaceWindow, CreateFindReplaceGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_SEARCH_REPLACE_TITLE, "replace",									/* WDT_REPLACE */
	 HandleFindReplaceIDCMP, CloseFindReplaceWindow, InitFindReplaceWindow, CreateFindReplaceGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PRINT_TITLE, "print",												/* WDT_PRINTER */
	 HandlePrinterIDCMP, ClosePrinterWindow, InitPrinterWindow, CreatePrinterGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PRINT_STATUS_TITLE, "printstatus",									/* WDT_PRINTSTATUS */
	 HandlePrintStatusIDCMP, ClosePrintStatusWindow, InitPrintStatusWindow, CreatePrintStatusGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_FILE_TYPE_TITLE, "filetype",										/* WDT_FILETYPE */
	 NULL, CloseFileTypeWindow, InitFileTypeWindow, CreateFileTypeGadgets, NULL,
     LISTVIEWIDCMP | IDCMP_VANILLAKEY | IDCMP_RAWKEY},
	{MSG_EXECUTE_COMMAND_TITLE, "command",									/* WDT_COMMAND */
	 handleCommandIDCMP,NULL,NULL,CreateCommandGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN},

/**************************** prefs ****************************/

	{MSG_PREFS_TITLE, "prefs",												/* WDT_PREFS */
	 handlePrefsIDCMP, closePrefsWindow, NULL, NULL, CreatePrefsGadgets,
     APPIDCMP | LISTVIEWIDCMP | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE | IDCMP_RAWKEY | IDCMP_VANILLAKEY},
	{MSG_PREFS_DISPLAY_TITLE, "prefs_display",								/* WDT_PREFDISP */
	 handlePrefDispIDCMP,closePrefDispWin,NULL,CreatePrefDispGads, NULL,
	APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN},
	{MSG_PREFS_SCREEN_TITLE, "prefs_screen",								/* WDT_PREFSCREEN */
	 handlePrefScreenIDCMP,NULL,NULL,CreatePrefScreenGads, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN},
	{MSG_PREFS_PALETTE_TITLE, "prefs_palette",								/* WDT_PREFCOLORS */
	 handlePrefColorsIDCMP,closePrefColorsWin,initPrefColors,CreatePrefColorsGads, NULL,
     APPIDCMP | LISTVIEWIDCMP | IDCMP_IDCMPUPDATE},
	{MSG_PREFS_ICONBAR_TITLE, "prefs_iconbar",								/* WDT_PREFICON */
	 handlePrefIconIDCMP,closePrefIconWin,initPrefIcon,CreatePrefIconGads, NULL,
     APPIDCMP | LISTVIEWIDCMP | DRAGIDCMP},
	{MSG_PREFS_SHEET_TITLE, "prefs_table",									/* WDT_PREFTABLE */
	 HandleTablePrefsIDCMP, CloseTablePrefsWindow, InitTablePrefsWindow, CreateTablePrefsGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PREFS_FILE_TITLE, "prefs_file",									/* WDT_PREFFILE */
	 HandleFilePrefsIDCMP, CloseFilePrefsWindow, InitFilePrefsWindow, CreateFilePrefsGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN},
	{0},																	/* WDT_PREFPRINTER */
	{MSG_PREFS_COMMANDS_TITLE, "prefs_commands",							/* WDT_PREFCMDS */
	 handlePrefCmdsIDCMP,closePrefCmdsWin,NULL,CreatePrefCmdsGads, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PREFS_FORMATS_TITLE, "prefs_formats",								/* WDT_PREFFORMAT */
	 HandleFormatPrefsIDCMP, CloseFormatPrefsWindow, InitFormatPrefsWindow, CreateFormatPrefsGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PREFS_DEFINE_COMMAND_TITLE, "prefs_define_command",				/* WDT_DEFINECMD */
	 handleDefineCmdIDCMP,closeDefineCmdWin,NULL,CreateDefineCmdGads, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PREFS_RECORD_KEY_TITLE, "prefs_record_key",						/* WDT_PRESSKEY */
	 handlePressKeyIDCMP,NULL,NULL,CreatePressKeyGads, NULL,
     APPIDCMP | IDCMP_GADGETUP},
	{MSG_PREFS_MENU_TITLE, "prefs_menu",									/* WDT_PREFMENU */
	 handlePrefMenuIDCMP,closePrefMenuWin,initPrefMenu,CreatePrefMenuGads, NULL,
     APPIDCMP | LISTVIEWIDCMP | DRAGIDCMP},
	{MSG_PREFS_KEYBOARD_TITLE, "prefs_keyboard",							/* WDT_PREFKEYS */
	 HandleKeyboardPrefsIDCMP, CloseKeyboardPrefsWindow, InitKeyboardPrefsWindow, NULL, CreateKeyboardPrefsGadgets,
	 APPIDCMP | LISTVIEWIDCMP | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE | DRAGIDCMP},
	{MSG_PREFS_SYSTEM_TITLE, "prefs_system",								/* WDT_PREFSYS */
	 HandleSystemPrefsIDCMP, NULL, NULL, CreateSystemPrefsGadgets, NULL,
	APPIDCMP | LISTVIEWIDCMP},
	{MSG_PREFS_NAMES_TITLE, "prefs_names",									/* WDT_PREFNAMES */
	 handlePrefNamesIDCMP, closePrefNamesWin, initPrefNames, CreatePrefNamesGads, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PREFS_CONTEXT_MENUS_TITLE, "prefs_context_menus",					/* WDT_PREFCONTEXT */
	 handlePrefContextIDCMP,closePrefContextWindow,initPrefContext,CreatePrefContextGads, NULL,
     APPIDCMP | LISTVIEWIDCMP},
//  {NULL},                                                        /* WDT_PREFFONT/WDT_PREFTOOL */
    {0, "prefs_add_prefs",                                      /* WDT_PREFCHOICE */
	 HandlePrefChoiceIDCMP,ClosePrefChoiceWindow,initPrefChoiceWindow,CreatePrefChoiceGads, NULL,
     APPIDCMP | LISTVIEWIDCMP},

/**************************** map ****************************/

	{MSG_PAGE_TITLE, "page",												/* WDT_PAGE */
	 HandlePageIDCMP, NULL, NULL, CreatePageGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN},
	{MSG_DOCUMENT_TITLE, "document",										/* WDT_DOCUMENT */
	 HandleDocumentIDCMP, CloseDocumentWindow, InitDocumentWindow, CreateDocumentGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_PAGE_SETUP_TITLE, "page_setup",									/* WDT_PAGESETUP */
	 HandlePageSetupIDCMP,NULL,NULL,CreatePageSetupGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_DOCUMENT_INFO_TITLE, "doc_info",									/* WDT_DOCINFO */
	 HandleDocInfoIDCMP,NULL,NULL,CreateDocInfoGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_ZOOM_TITLE, "zoom",												/* WDT_ZOOM */
	 handleZoomIDCMP,NULL,NULL,CreateZoomGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN},
	{MSG_SCRIPTS_TITLE, "scripts",                                         	/* WDT_SCRIPTS */
	 HandleScriptsIDCMP, CloseScriptsWindow, NULL, NULL, CreateScriptsGadgets,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE},
	{MSG_CELL_TITLE_TITLE, "set_cell_title",								/* WDT_SETTITLE */
	 handleSetTitleIDCMP,NULL,NULL,CreateSetNameGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP},

/**************************** objects ****************************/

	{MSG_INSERT_OBJECT_TITLE, "insert_object",								/* WDT_GCLASSES */
	 handleGClassesIDCMP, closeGClassesWindow, NULL, NULL, CreateGClassesGadgets,
     APPIDCMP | LISTVIEWIDCMP | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE},
	{MSG_OBJECT_TITLE, "object",											/* WDT_OBJECT */
	 HandleObjectIDCMP, CloseObjectWindow, InitObjectWindow, CreateObjectGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_INTUITICKS},
	{MSG_DIAGRAM_TITLE, "create_diagram",									/* WDT_DIAGRAM */
	 HandleDiagramIDCMP, CloseDiagramWindow, InitDiagramWindow, CreateDiagramGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP | SLIDERIDCMP},
	{MSG_DIAGRAM_PREVIEW_TITLE, "diagram_preview",							/* WDT_PREVIEW */
	 handlePreviewIDCMP,NULL,NULL,NULL, NULL,
     APPIDCMP | IDCMP_NEWSIZE},
	{MSG_OBJECT_LIST_TITLE, "object_list",									/* WDT_GOBJECTS */
	 NULL,NULL,NULL,NULL, NULL,												/* not yet implemented */
     APPIDCMP | LISTVIEWIDCMP | SLIDERIDCMP},

/**************************** cell properties ****************************/

	{MSG_CELL_TITLE, "cell",												/* WDT_CELL */
	 handleCellIDCMP, closeCellWin, NULL, CreateCellGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_FRAME_TITLE, "frame",												/* WDT_BORDER */
	 HandleBorderIDCMP, CloseBorderWindow, InitBorderWindow, CreateBorderGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_INTUITICKS | IDCMP_MOUSEBUTTONS},
	{MSG_CELL_SIZE_TITLE, "cell_size",										/* WDT_CELLSIZE */
	 handleCellSizeIDCMP,NULL,NULL,CreateCellSizeGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN},
	{MSG_NOTES_TITLE, "notes",												/* WDT_NOTES */
	 handleNotesIDCMP, closeNotesWin, NULL, NULL, CreateNotesGadgets,
     APPIDCMP | LISTVIEWIDCMP | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE},

/**************************** database ****************************/

	{MSG_DBASE_INDEX_TITLE, "database_index",								/* WDT_INDEX */
	 HandleIndexIDCMP, CloseIndexWindow, InitIndexWindow, CreateIndexGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_DBASE_TITLE, "database",											/* WDT_DATABASE */
	 HandleDatabaseIDCMP, CloseDatabaseWindow, InitDatabaseWindow, CreateDatabaseGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_DBASE_MASK_TITLE, "database_mask",									/* WDT_MASK */
	 HandleMaskIDCMP, CloseMaskWindow, InitMaskWindow,CreateMaskGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_DBASE_FILTER_TITLE, "database_filter",								/* WDT_FILTER */
	 HandleFilterIDCMP, CloseFilterWindow, InitFilterWindow, CreateFilterGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},

/**************************** calculation ****************************/

	{MSG_FORMULA_TITLE, "formula",											/* WDT_FORMEL */
	 HandleFormelIDCMP, CloseFormelWindow, InitFormelWindow, CreateFormelGadgets, NULL,
     APPIDCMP | LISTVIEWIDCMP},
	{MSG_SET_NAME_TITLE, "set_name",										/* WDT_SETNAME */
	 handleSetNameIDCMP, NULL, NULL, CreateSetNameGadgets, NULL,
     APPIDCMP | IDCMP_GADGETUP}
};

#endif	/* IGN_WINDEFS_H */
																																							
