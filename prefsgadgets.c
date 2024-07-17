/* Gadget creation functions for the preferences windows
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#include "types.h"
#include "funcs.h"

#ifdef __amigaos4__
	#define GOFFSET 10 
#else
	#define GOFFSET  0
#endif

extern const char *gAppCommandLabels[];
//extern const char *gShortAppCommandLabels[];

static const char *sScreenLabels[4];
static const char *sCommandLabels[4];
static const char *sGridLabels[4];
static const char *sFormulaBarLabels[4];
static const char *sIconBarLabels[4];
static const char *sAutoSaveLabels[4];
static const char *sBlockDisplayLabels[4];
static const char *sQualifierLabels[6];
static const char *sKeyLabels[4];
static const char *sNameTypeLabels[4];
static const char *sFormatTypeLabels[6];
static const char *sAlignLabels[4];
static const char *sColorViewLabels[3];
const char *gColorModeLabels[6];
static const char *sColorSpaceLabels[3];
static const char *sSystemPageLabels[3];
static const char *sWindowRefreshLabels[4];
static const char *sContextMenuLabels[6];


void
CreatePrefsGadgets(struct winData *wd, long wid, long hei)
{
	MakeShortCutString(wd->wd_ShortCuts, MSG_ADD_UGAD, MSG_REMOVE_UGAD, MSG_OK_UGAD, TAG_END);
	ngad.ng_TopEdge = barheight+3;
	ngad.ng_LeftEdge = lborder;
	ngad.ng_Width = wid-lborder-rborder;
	ngad.ng_Height = ((hei-barheight-2*fontheight-21-leftImg->Height)/itemheight)*itemheight+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID = 1;
	gad = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,&prefstree.tl_View,GTLV_CallBack,&treeHook,GTLV_MaxPen,7,GTLV_ItemHeight,itemheight,GTLV_ShowSelected,NULL,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Height = fontheight+4;
	ngad.ng_Width = (wid-16) >> 1;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADD_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = wd->wd_ShortCuts[0];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = wid-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_REMOVE_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

	ngad.ng_TopEdge = hei-fontheight-7-leftImg->Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_OK_GAD))+42;
	ngad.ng_LeftEdge = (wid-ngad.ng_Width)>>1;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[2];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreatePrefChoiceGads(REG(a0, struct winData *wd))
{
	gWidth = TLn(GetString(&gLocaleInfo, MSG_ADD_PREFERENCES_GAD))+84+lborder+rborder;
	gHeight = barheight+itemheight*8+fontheight+17+bborder;

	//wd->wd_ShortCuts = "oa";
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);

	ngad.ng_TopEdge = barheight+3;
	ngad.ng_LeftEdge = lborder;
	ngad.ng_Width = gWidth-lborder-rborder;
	ngad.ng_Height = 8*itemheight+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID = 1;
	gad = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,wd->wd_ExtData[1],GTLV_CallBack,&renderHook,GTLV_MaxPen,7,GTLV_ItemHeight,itemheight,GTLV_ShowSelected,NULL,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height+3;
	ngad.ng_Height = fontheight+4;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = wd->wd_ShortCuts[0];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreatePrefDispGads(REG(a0, struct winData *wd))
{
	struct PrefDisp *pd = GetLocalPrefs(wd->wd_Data)->pr_Disp;
#ifdef __amigaos4__
	char   t[256];
#else
	char   t[42];
#endif

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 9; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 11; //Set Gadgetnumber 
#endif

#ifdef __amigaos4__
	gWidth = TLn(GetString(&gLocaleInfo, MSG_TOOL_BAR_GAD))+TLn(GetString(&gLocaleInfo, MSG_FORMULA_BAR_LABEL))+TLn(GetString(&gLocaleInfo, MSG_BAR_BOTTOM_GAD))+124+lborder+rborder;
#else
	gWidth = TLn(GetString(&gLocaleInfo, MSG_FONT_GAD))+TLn(GetString(&gLocaleInfo, MSG_ICON_BAR_GAD))+TLn("Helvetica/11")+124+lborder+rborder;
#endif
	gHeight = fontheight*9+barheight+42+23+bborder;

	ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_GRID_LABEL));
	ngad.ng_TopEdge = barheight+fontheight+5;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_GRID_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;		// 1
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sGridLabels, GTCY_Active, pd->pd_Rasta & PDR_CYCLES, GT_Underscore,'_', TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_GRID_CELL_WIDTH_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID = 12;				// 12
	// TODO: implement function correctly and activate gadget again
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,pd->pd_Rasta & PDR_CELLWIDTH,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHOW_HEADINGS_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID = 2;				// 2
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,pd->pd_ShowAntis,GT_Underscore,'_',TAG_END);

	sprintf(t,"%s/%ldpt",pd->pd_AntiAttr.ta_Name,pd->pd_AntiAttr.ta_YSize);
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_FONT_LABEL));
	ngad.ng_Width = gWidth-TLn(GetString(&gLocaleInfo, MSG_CHOOSE_GAD))-24-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 3
	gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_CopyText,TRUE,GTTX_Text,t,GTTX_Border,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CHOOSE_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CHOOSE_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += 2*fontheight+13;
	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_HELP_BAR_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;					// 5
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,pd->pd_HelpBar,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TOOL_BAR_GAD);
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,pd->pd_ToolBar,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-TLn(GetString(&gLocaleInfo, MSG_BAR_BOTTOM_GAD))-56-rborder;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_BAR_BOTTOM_GAD))+48;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FORMULA_BAR_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 7
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sFormulaBarLabels, GTCY_Active, pd->pd_FormBar, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge -= fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ICON_BAR_LABEL);
	ngad.ng_GadgetID++;					// 8
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sIconBarLabels, GTCY_Active, pd->pd_IconBar, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += 2*fontheight+19;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 9
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = (gWidth-ngad.ng_Width) >> 1;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TEST_GAD);
	ngad.ng_GadgetID++;					// 10
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-ngad.ng_Width-rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 11
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreatePrefScreenGads(REG(a0, struct winData *wd))
{
	char t[64];

	gWidth = TLn(GetString(&gLocaleInfo, MSG_OPEN_SCREEN_LABEL))
		+ TLn(GetString(&gLocaleInfo, MSG_SCREEN_MODE_GAD))
		+ TLn(GetString(&gLocaleInfo, MSG_FONT_ELLIPSIS_GAD))
#ifdef __amigaos4__
		+ 100 + lborder + rborder;
#else
		+ 92 + lborder + rborder;
#endif
	gHeight = fontheight * 9 + barheight + 64 + bborder;

	//wd->wd_ShortCuts = "öwbmzthuoa";
	MakeShortCutString(wd->wd_ShortCuts, MSG_ON_PUBLIC_ULABEL, MSG_LIKE_WORKBENCH_UGAD, MSG_OPEN_SCREEN_ULABEL,
						MSG_SCREEN_MODE_UGAD, MSG_FONT_ELLIPSIS_UGAD, MSG_SCREEN_WIDTH_ULABEL, MSG_HEIGHT_ULABEL,
						MSG_PATTERN_UGAD, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);

	ngad.ng_TopEdge = barheight+fontheight+5;
	ngad.ng_LeftEdge = boxwidth+TLn(GetString(&gLocaleInfo, MSG_ON_PUBLIC_LABEL)) + 24 + lborder;
	ngad.ng_Width = gWidth-ngad.ng_LeftEdge-8-rborder-boxwidth;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = NULL;		// 1
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, prefs.pr_Screen->ps_PubName, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;				// 2
	gad = CreatePopGadget(wd, gad, FALSE);

	ngad.ng_LeftEdge = boxwidth + TLn(GetString(&gLocaleInfo, MSG_OPEN_SCREEN_LABEL)) + 24 + lborder;
	ngad.ng_TopEdge += 2*fontheight+14;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_SCREEN_MODE_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SCREEN_MODE_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[3];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width+6;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_FONT_ELLIPSIS_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_ELLIPSIS_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[4];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_TopEdge = fontheight*2+10 + GOFFSET;
	ngad.ng_Width = (boxwidth*fontheight)/(fontheight+4);
	ngad.ng_Height = fontheight;
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID = 5;			// 5
	gad = CreateGadget(MX_KIND, gad, &ngad, GTMX_Labels, &sScreenLabels, GTMX_Active, prefs.pr_Screen->ps_Type,
						GTMX_Scaled, TRUE, GTMX_Spacing, 7, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge = barheight+5*fontheight+32;
	ngad.ng_LeftEdge += 8+TLn(GetString(&gLocaleInfo, MSG_WIDTH_LABEL));
	ngad.ng_Width = (gWidth-38-rborder-lborder-TLn(GetString(&gLocaleInfo, MSG_WIDTH_LABEL))-TLn(GetString(&gLocaleInfo, MSG_HEIGHT_LABEL))) >> 1;
	ngad.ng_Height += 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SCREEN_WIDTH_ULABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;				// 6
	strcpy(t,ita(prefs.pr_Screen->ps_mmWidth/10240.0,2,FALSE));
	strcat(t," cm");
	gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,t,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += 16+ngad.ng_Width+TLn(GetString(&gLocaleInfo, MSG_HEIGHT_LABEL));
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_HEIGHT_ULABEL);
	ngad.ng_GadgetID++;				// 7
	strcpy(t,ita(prefs.pr_Screen->ps_mmHeight/10240.0,2,FALSE));
	strcat(t," cm");
	gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,t,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_TopEdge += 2*fontheight+13;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PATTERN_UGAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID = wd->wd_ShortCuts[7];
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Checked, prefs.pr_Screen->ps_BackFill, GTCB_Scaled, TRUE, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = 24+lborder+3*boxwidth+TLn(GetString(&gLocaleInfo, MSG_PATTERN_GAD));
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = 9;
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+12;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[8];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-ngad.ng_Width-rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[9];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM CreatePrefMenuGads(REG(a0, struct winData *wd))
{
	long i = 13,lvwidth;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 21; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 22; //Set Gadgetnumber 
#endif
#ifdef __amigaos4__
	gWidth = 3*TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+3*TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+282+180+lborder+rborder;
#else
	gWidth = 3*TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+3*TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+282+lborder+rborder;
#endif
	gHeight = (7+i)*fontheight+barheight+54+bborder;
	while(gHeight > scr->Height)
	{
		gHeight -= fontheight;
		i--;
	}
	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = barheight+fontheight+9;
#ifdef __amigaos4__
	ngad.ng_Width = lvwidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+90 + 60;
#else
	ngad.ng_Width = lvwidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+90;
#endif
	ngad.ng_Height = fontheight*i+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_MENU_TITLE_GAD);
	ngad.ng_Flags = PLACETEXT_ABOVE;	// 1
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,wd->wd_ExtData[3],GTLV_ShowSelected,NULL,GA_Disabled,IsListEmpty((struct List *)wd->wd_ExtData[3]),TAG_END);

	ngad.ng_LeftEdge += 6+ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_MENU_ITEMS_GAD);
	ngad.ng_GadgetID++;					// 2
	gad = wd->wd_ExtData[1] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_ShowSelected,NULL,GA_Disabled,TRUE,GTLV_CallBack,&popUpHook,TAG_END);

	ngad.ng_LeftEdge += 6+ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_MENU_SUBITEMS_GAD);
	ngad.ng_GadgetID++;					// 3
	gad = wd->wd_ExtData[2] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_ShowSelected,NULL,GA_Disabled,TRUE,GTLV_CallBack,&popUpHook,TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+45;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,TAG_END);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 5
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = lborder+ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+45;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;					// 7
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 8
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 9
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = lvwidth;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;					// 10
	gad = CreateGadget(STRING_KIND,gad,&ngad,TAG_END);

	ngad.ng_LeftEdge += 6+ngad.ng_Width;
	ngad.ng_GadgetID++;					// 11
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += 6+ngad.ng_Width;
	ngad.ng_GadgetID++;					// 12
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = lvwidth+14+lborder+TLn(GetString(&gLocaleInfo, MSG_SHORT_CUT_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width -= 8+TLn(GetString(&gLocaleInfo, MSG_SHORT_CUT_LABEL));
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHORT_CUT_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 13
	gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 14
	gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = lvwidth+6+lborder;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = lvwidth;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;					// 15
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 16
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge -= boxwidth+6;
	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 17
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 18
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge -= 2*lvwidth-boxwidth+6;
	ngad.ng_Width = lvwidth-boxwidth;
	ngad.ng_GadgetID++;					// 19
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &gAppCommandLabels, GA_Disabled, TRUE, TAG_END);

	ngad.ng_LeftEdge += lvwidth+6;
	ngad.ng_GadgetID++;					// 20
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &gAppCommandLabels, GA_Disabled, TRUE, TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+9;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+40;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_GadgetID++;					// 21
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore, '_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 22
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore, '_',TAG_END);
}


void ASM
CreateFilePrefsGadgets(REG(a0, struct winData *wd))
{
	struct PrefFile *pf = wd->wd_Data;
	char t[20];
	int a, b;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 13; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 14; //Set Gadgetnumber 
#endif
	gWidth = TLn(GetString(&gLocaleInfo, MSG_CREATE_ICONS_GAD))
			+ TLn(GetString(&gLocaleInfo, MSG_WARN_IOTYPE_NO_DEFAULT_GAD))
			+ TLn(GetString(&gLocaleInfo, MSG_INTERVAL_LABEL))
			+ TLn(sAutoSaveLabels[1])
			+ 2 * boxwidth + 54 + lborder + rborder;
	gHeight = barheight + fontheight * 9 + 75 + bborder;

	a = TLn(GetString(&gLocaleInfo, MSG_ICONS_LABEL));
	b = TLn(GetString(&gLocaleInfo, MSG_DOCUMENTS_LABEL));
	a = max(a, b);
	b = TLn(GetString(&gLocaleInfo, MSG_GRAPHICS_LABEL));
	a = max(a, b);

	ngad.ng_TopEdge = barheight + fontheight + 5;
	ngad.ng_LeftEdge = a + 16 + lborder;
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - 28 - rborder - TLn(GetString(&gLocaleInfo, MSG_CHOOSE_GAD));
	ngad.ng_Height = fontheight+4;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DOCUMENTS_LABEL);		   // 1
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, wd->wd_ExtData[0], GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_GRAPHICS_LABEL);
	ngad.ng_GadgetID++;						  // 2
	gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,wd->wd_ExtData[1],GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ICONS_LABEL);
	ngad.ng_GadgetID++;						  // 3
	gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,wd->wd_ExtData[2],GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge = 2 * fontheight + 8 + GOFFSET;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CHOOSE_GAD))+20;
	ngad.ng_LeftEdge = gWidth-ngad.ng_Width-8-rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CHOOSE_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;						  // 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetID++;						  // 5
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetID++;						  // 6
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += 2*fontheight+13;
	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CREATE_ICONS_GAD);
	ngad.ng_GadgetID++;						 // 7
	ngad.ng_Flags = PLACETEXT_RIGHT;
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',GTCB_Checked,pf->pf_Flags & PFF_ICONS,TAG_END);

	ngad.ng_LeftEdge = gWidth >> 1;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NO_FILE_NAME_SUFFIX_GAD);
	ngad.ng_GadgetID++;						 // 8
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE, GT_Underscore, '_', GTCB_Checked, pf->pf_Flags & PFF_NOSUFFIX, TAG_END);

	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SECURITY_BACKUPS_GAD);
	ngad.ng_GadgetID++;						 // 9
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked, pf->pf_Flags & PFF_BACKUP, GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth >> 1;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_WARN_IOTYPE_NO_DEFAULT_GAD);
	ngad.ng_GadgetID++;						 // 10
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE,
			GT_Underscore, '_', GTCB_Checked, pf->pf_Flags & PFF_WARN_IOTYPE, TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_LeftEdge = TLn(GetString(&gLocaleInfo, MSG_AUTO_SAVE_LABEL)) + 16 + lborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_AUTO_SAVE_LABEL);
	ngad.ng_Width = TLn(sAutoSaveLabels[1]) + 48;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID = 11;						 // 11
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sAutoSaveLabels, GTCY_Active, pf->pf_AutoSave, GT_Underscore, '_', TAG_END);

	sprintf(t, "%ld min", pf->pf_AutoSaveIntervall / 60);
	ngad.ng_LeftEdge += ngad.ng_Width + TLn(GetString(&gLocaleInfo, MSG_INTERVAL_LABEL)) + 16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_INTERVAL_LABEL);
	ngad.ng_Width = TLn("99 min")+25;
	ngad.ng_GadgetID++;						 // 12
	gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,t,GA_Disabled,!pf->pf_AutoSave,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+16;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+20;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;						 // 13
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;						 // 14
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreateTablePrefsGadgets(REG(a0, struct winData *wd))
{
	struct PrefTable *pt = wd->wd_ExtData[0];
	struct Node *ln;
	long h, a, b;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 17; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 18; //Set Gadgetnumber 
#endif

	a = TLn(GetString(&gLocaleInfo, MSG_SHOW_FORMULAS_GAD));
	b = TLn(GetString(&gLocaleInfo, MSG_SHOW_ZEROS_GAD));
	a = max(a, b);
	b = TLn(GetString(&gLocaleInfo, MSG_AUTO_COMPLETE_CENTURY_GAD));
	h = TLn(GetString(&gLocaleInfo, MSG_AUTO_CHANGE_CELL_SIZE_GAD));

	gWidth = max(h, b) + a + 2 * boxwidth + 54 + lborder + rborder;
	gHeight = barheight + fontheight * 11 + 78 + bborder;

	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_TopEdge = barheight + fontheight + 5;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OPEN_ERROR_REQUESTER_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;			// 1
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE, GTCB_Checked, calcflags & CF_REQUESTER, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height+3;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ZERO_FOR_UNKNOWN_NAMES_GAD);
	ngad.ng_GadgetID++;						 // 2
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE, GTCB_Checked, calcflags & CF_ZERONAMES, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = 16 + lborder + TLn(GetString(&gLocaleInfo, MSG_QUALIFIER_LABEL));
	ngad.ng_TopEdge += 2 * fontheight + 13;
	ngad.ng_Width = TLn("Shift+Alt") + 48;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_QUALIFIER_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID = 7;					   // 7
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sQualifierLabels, GTCY_Active, wd->wd_ExtData[4], TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width + 14 + TLn(GetString(&gLocaleInfo, MSG_FUNCTION_LABEL));
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge-boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FUNCTION_LABEL);
	ngad.ng_GadgetID++;						 // 8
	gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Text,(ln = FindListNumber(wd->wd_ExtData[3],pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_FUNCMASK)) ? ln->ln_Name : NULL,GTTX_Border,TRUE,TAG_END);

	h = ngad.ng_LeftEdge;
	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;						 // 9
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHOW_DRAGGABLE_CORNER_GAD);
	ngad.ng_GadgetID++;						 // 10
	ngad.ng_Flags = PLACETEXT_RIGHT;
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',GTCB_Checked,pt->pt_Flags & PTF_EDITFUNC,TAG_END);

	ngad.ng_LeftEdge = h;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ONLY_CELL_TEXT_GAD);
	ngad.ng_GadgetID++;						 // 11
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',GTCB_Checked,pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_TEXTONLY,TAG_END);

	ngad.ng_TopEdge += 2 * fontheight + 13;
	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHOW_ZEROS_GAD);
	ngad.ng_GadgetID++;						 // 12
	ngad.ng_Flags = PLACETEXT_RIGHT;
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',GTCB_Checked,pt->pt_Flags & PTF_SHOWZEROS,TAG_END);

	ngad.ng_LeftEdge += a + boxwidth + 16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_AUTO_CHANGE_CELL_SIZE_GAD);
	ngad.ng_GadgetID++;						 // 13
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',GTCB_Checked,pt->pt_Flags & PTF_AUTOCELLSIZE,TAG_END);

	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHOW_FORMULAS_GAD);
	ngad.ng_GadgetID++;						 // 14
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,pt->pt_Flags & PTF_SHOWFORMULA,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += a + boxwidth + 16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_AUTO_COMPLETE_CENTURY_GAD);
	ngad.ng_GadgetID++;						 // 15
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',GTCB_Checked,pt->pt_Flags & PTF_AUTOCENTURY,TAG_END);

	ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_BLOCK_DISPLAY_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = TLn(sBlockDisplayLabels[2]) + 64;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_BLOCK_DISPLAY_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;						 // 16
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sBlockDisplayLabels,
			GTCY_Active, (pt->pt_Flags & (PTF_MARKSUM | PTF_MARKAVERAGE)) >> 4, TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+12;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+20;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;						 // 17
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;						 // 18
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}

void
CreateKeyboardPrefsGadgets(struct winData *wd, long width, long height)
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 7; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 8; //Set Gadgetnumber 
#endif

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = barheight + fontheight + 9;
	ngad.ng_Width = width - lborder - rborder;
	ngad.ng_Height = ((height - barheight - 5 * fontheight - 43 - leftImg->Height) / fontheight) * fontheight + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHORT_CUT_GAD);
	ngad.ng_Flags = PLACETEXT_ABOVE;
	ngad.ng_GadgetID = 1;				// 1
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, &pr->pr_AppKeys,	GTLV_ShowSelected, NULL, GTLV_CallBack, &formatHook, GTLV_MaxPen, 7, TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width /= 2;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 2
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_Width = ((struct Gadget *)wd->wd_ExtData[0])->Width - ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;					// 3
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = height - leftImg->Height - 3 * fontheight - 23;
	ngad.ng_Width = width - lborder - rborder - boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_RECORD_GAD);
	ngad.ng_GadgetID++;					// 4
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GA_Disabled, TRUE, TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 5
	gad = CreatePopGadget(wd, gad, FALSE);

	/*ngad.ng_TopEdge += fontheight+7;
	ngad.ng_LeftEdge = 8;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = "nicht während des Edierens";
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);*/

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = width - lborder - rborder;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sKeyLabels, GA_Disabled, TRUE, TAG_END);

	ngad.ng_TopEdge += fontheight + 9;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+20;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 7
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = width - rborder - ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 8
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreatePrefIconGads(REG(a0, struct winData *wd))
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	long i = 8;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 5; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 7; //Set Gadgetnumber 
#endif
#ifdef __amigaos4__
	gWidth = 2*TLn(GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))+2*TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+306+lborder+rborder;
#else
	gWidth = 2*TLn(GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))+2*TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+206+lborder+rborder;
#endif
	gHeight = itemheight*i+3*fontheight+barheight+29+bborder;
	while (gHeight > scr->Height) {
		gHeight -= itemheight;
		i--;
	}
	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = barheight+fontheight+9;
#ifdef __amigaos4__
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+150;
#else
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+100;
#endif
	ngad.ng_Height = itemheight*i+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_COMMANDS_WITH_ICONS_GAD);
	ngad.ng_Flags = PLACETEXT_ABOVE;	// 1
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,wd->wd_ExtData[4],GTLV_CallBack,&renderHook,GTLV_MaxPen,7,GTLV_ItemHeight,itemheight,TAG_END);

	ngad.ng_LeftEdge += 6+ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TOOL_BAR_GAD);
	ngad.ng_GadgetID++;					// 2
	gad = wd->wd_ExtData[1] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,&pr->pr_IconObjs,GTLV_ShowSelected,NULL,GTLV_CallBack,&renderHook,GTLV_MaxPen,7,GTLV_ItemHeight,itemheight,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
#ifdef __amigaos4__
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))+75;
#else
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))+50;
#endif
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 3
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Immediate,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
#ifdef __amigaos4__
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+75;
#else
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))+50;
#endif
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;					// 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+9;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 5
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = (gWidth-ngad.ng_Width) >> 1;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TEST_GAD);
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-ngad.ng_Width-rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 7
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreatePrefCmdsGads(REG(a0, struct winData *wd))
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	long i = 10;

#ifdef __amigaos4__
	gWidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+126+14+lborder+rborder;
#else
	gWidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+126+lborder+rborder;
#endif
	gHeight = itemheight*i+2*fontheight+barheight+21+bborder;
	while (gHeight > scr->Height) {
		gHeight -= itemheight;
		i--;
	}

	//wd->wd_ShortCuts = "nkloa";
	MakeShortCutString(wd->wd_ShortCuts, MSG_NEW_UGAD, MSG_COPY_UGAD,
						MSG_DELETE_UGAD, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = fontheight+6 + GOFFSET;
	ngad.ng_Width = gWidth-lborder-rborder;
	ngad.ng_Height = itemheight*i+4;
	ngad.ng_GadgetText = NULL;
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,&pr->pr_AppCmds,GTLV_ShowSelected,NULL,GTLV_CallBack,&renderHook,GTLV_MaxPen,7,GTLV_ItemHeight,itemheight,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+42;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = wd->wd_ShortCuts[0];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_COPY_GAD)) + 42;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_COPY_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + 42;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[2];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_OK_GAD)) + 42;
	ngad.ng_LeftEdge = (gWidth-ngad.ng_Width) >> 1;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[3];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
}


void ASM
CreateDefineCmdGads(REG(a0, struct winData *wd))
{
	struct AppCmd *ac = wd->u.definecmd.wd_AppCmd;

#ifdef __amigaos4__
	gWidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))
			+ TLn(GetString(&gLocaleInfo, MSG_INTERNAL_COMMAND_GAD)) + 194 + 50 +lborder + rborder;
#else
	gWidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))
			+ TLn(GetString(&gLocaleInfo, MSG_INTERNAL_COMMAND_GAD)) + 194 +lborder + rborder;
#endif
	gHeight = fontheight*16 + barheight + 82 + bborder;

	//wd->wd_ShortCuts = "mbnlirduhoay";
	MakeShortCutString(wd->wd_ShortCuts, MSG_NAME_ULABEL, MSG_CHOOSE_ICON_UGAD, MSG_NEW_UGAD, MSG_DELETE_UGAD,
						MSG_INTERNAL_COMMAND_UGAD, MSG_AREXX_COMMAND_UGAD, MSG_DOS_COMMAND_UGAD, MSG_OUTPUT_ULABEL,
						MSG_HELP_TEXT_ULABEL, MSG_OK_UGAD, MSG_CANCEL_UGAD, MSG_HYPER_TEXT_ULABEL, TAG_END);

	ngad.ng_TopEdge += 5;
	ngad.ng_LeftEdge = 16 + lborder + TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL));
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - 8 - rborder;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_ULABEL);		// 1
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, ac->ac_Node.in_Name, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_LeftEdge += boxwidth*2+6;
	ngad.ng_Width = gWidth-ngad.ng_LeftEdge-8-rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CHOOSE_ICON_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID = 3;			// 3
	gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Text,ac->ac_ImageName,GTTX_Border,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += 2*fontheight + 13;
	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + 60;
	ngad.ng_Height = fontheight*6 + 4;
	ngad.ng_GadgetID++;				// 4
	wd->u.definecmd.wd_ListView = gad = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, &ac->ac_Cmds,
		GTLV_ShowSelected, NULL, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + 30;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[2];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + 30;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[3];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GA_Disabled, TRUE, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = wd->u.definecmd.wd_ListView->Width + 14 + lborder;
	ngad.ng_TopEdge = wd->u.definecmd.wd_ListView->TopEdge;
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - boxwidth - 8 - rborder;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID = 7;			// 7
	gad = CreateGadget(STRING_KIND, gad, &ngad, GA_Disabled, TRUE, GTST_MaxChars, 256, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = gWidth - 8 - rborder - boxwidth;
	ngad.ng_GadgetID++;				// 8
	gad = CreatePopGadget(wd, gad, FALSE);

	ngad.ng_LeftEdge = wd->u.definecmd.wd_ListView->Width + 18 + lborder;
	ngad.ng_TopEdge += fontheight + 10;
	ngad.ng_Width = boxwidth - 4;
	ngad.ng_Height = fontheight;
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;				// 9
	gad = CreateGadget(MX_KIND, gad, &ngad, GTMX_Labels, &sCommandLabels, GTMX_Scaled, TRUE, GTMX_Spacing, 4,
		GA_Disabled, TRUE, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge = wd->u.definecmd.wd_ListView->TopEdge + 8*fontheight + 17;
	ngad.ng_LeftEdge = 16 + lborder + TLn(GetString(&gLocaleInfo, MSG_OUTPUT_LABEL));
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - boxwidth - 8 - rborder;
	ngad.ng_Height = fontheight+4;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OUTPUT_ULABEL);
	ngad.ng_GadgetID++;				// 10
	gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,ac->ac_Output,GTST_MaxChars,256,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth - 8 - rborder-boxwidth;
	ngad.ng_GadgetID++;				// 11
	gad = CreatePopGadget(wd, gad, FALSE);

	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_LeftEdge = 16 + lborder + TLn(GetString(&gLocaleInfo, MSG_HELP_TEXT_LABEL));
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - 8 - rborder;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_HELP_TEXT_ULABEL);
	ngad.ng_GadgetID++;				// 12
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, ac->ac_HelpText, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_LeftEdge = 16 + lborder + TLn(GetString(&gLocaleInfo, MSG_HYPER_TEXT_LABEL));
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - 8 - rborder;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_HYPER_TEXT_ULABEL);
	ngad.ng_GadgetID++;				// 13
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, ac->ac_Guide, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight + 12;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD)) + 16;
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[9];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = gWidth - ngad.ng_Width - rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[10];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
}


void ASM
CreatePrefNamesGads(REG(a0, struct winData *wd))
{
	struct Mappe *mp = GetRealMap(wd->wd_Data);
	long w, h;

	//wd->wd_ShortCuts = "nklmitboa";
	MakeShortCutString(wd->wd_ShortCuts, MSG_NEW_UGAD, MSG_COPY_UGAD, MSG_DELETE_UGAD, MSG_NAME_ULABEL,
						MSG_CONTENTS_ULABEL, MSG_REFERENCE_ULABEL, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);

	w = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + TLn(GetString(&gLocaleInfo, MSG_COPY_GAD)) + TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + 48;
	gWidth = w + 100 + TLn(sNameTypeLabels[1]) + TLn(GetString(&gLocaleInfo, MSG_TYPE_LABEL)) + lborder + rborder;
	gHeight = barheight + fontheight*10 + 21 + bborder;
	wd->wd_ExtData[1] = NULL;

	ngad.ng_LeftEdge = lborder;
	ngad.ng_Width = w;
	ngad.ng_Height = fontheight*8 + 4;
	ngad.ng_GadgetText = NULL;			// 1
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, wd->wd_ExtData[2], GTLV_ShowSelected, NULL, TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + 16;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = wd->wd_ShortCuts[0];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_COPY_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

	h = ngad.ng_TopEdge;
	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[2];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = w+14+lborder+TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL));
	ngad.ng_TopEdge = barheight+3;
	ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_ULABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID = 5;				// 5
	gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = w+14+lborder+TLn(GetString(&gLocaleInfo, MSG_CONTENTS_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CONTENTS_ULABEL);
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = w+14+lborder+TLn(GetString(&gLocaleInfo, MSG_TYPE_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TYPE_ULABEL);
	ngad.ng_GadgetID++;					// 7
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sNameTypeLabels, GT_Underscore, '_', GA_Disabled, TRUE, TAG_END);

	ngad.ng_LeftEdge = w+14+lborder+TLn(GetString(&gLocaleInfo, MSG_REFERENCE_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-rborder-boxwidth-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_REFERENCE_ULABEL);
	ngad.ng_GadgetID++;					// 8
	gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Text,mp ? mp->mp_actPage->pg_Node.ln_Name : NULL,GTTX_Border,TRUE,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;					// 9
#ifdef __amigaos4__
	gad = CreatePopGadget(wd,gad,TRUE);
#else
	gad = CreatePopGadget(wd,gad,FALSE);
#endif

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = h+fontheight+7;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
#ifdef __amigaos4__
	ngad.ng_GadgetID = wd->wd_ShortCuts[6];
#else
	ngad.ng_GadgetID = wd->wd_ShortCuts[7];
#endif
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
#ifdef __amigaos4__
	ngad.ng_GadgetID = wd->wd_ShortCuts[7];
#else
	ngad.ng_GadgetID = wd->wd_ShortCuts[8];
#endif
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreateFormatPrefsGadgets(REG(a0, struct winData *wd))
{
	char buffer[64];
	long i = 16, w, n;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 15; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 16; //Set Gadgetnumber 
#endif
	w = 16 + 3 * boxwidth + TLn(GetString(&gLocaleInfo, MSG_NEG_VALUES_LABEL));
	n = 16 + boxwidth + TLn(GetString(&gLocaleInfo, MSG_NEG_VALUES_GAD));
	w = max(w, n);

	gWidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))
				+ TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))
				+ TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))
#ifdef __amigaos4__
				+ w + lborder + rborder + 250;
#else
				+ w + lborder + rborder + 137;
#endif
	gHeight = fontheight * i + 3 * fontheight + barheight + 27 + bborder;

	while (gHeight > scr->Height) {
		gHeight -= fontheight;
		i--;
	}

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = barheight + fontheight + 9;
	ngad.ng_Width = w = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))
#ifdef __amigaos4__
							+ TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + 190;
#else
							+ TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + 105;
#endif
	ngad.ng_Height = fontheight * i + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DEFINED_FORMATS_GAD);
	ngad.ng_Flags = PLACETEXT_ABOVE;	// 1
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, wd->wd_ExtData[5],
											GTLV_ShowSelected, NULL, GTLV_CallBack, &formatHook, GTLV_MaxPen, 7, TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + 35;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 2
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))+35;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_COPY_GAD);
	ngad.ng_GadgetID++;					// 3
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+35;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;					// 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	w += 12+lborder;
	ngad.ng_LeftEdge = w+8+TLn(GetString(&gLocaleInfo, MSG_TYPE_LABEL));
	ngad.ng_TopEdge = barheight+2*fontheight+11;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TYPE_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 5
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GA_Disabled, TRUE, GTCY_Labels, &sFormatTypeLabels, TAG_END);

	sprintf(buffer, GetString(&gLocaleInfo, MSG_PRIORITY_SLIDER_GAD), -128);
	ngad.ng_LeftEdge = w + 8 + (n = TLn(buffer));
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_Height -= 3;
	ngad.ng_GadgetText = NULL;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(SLIDER_KIND, gad, &ngad, GA_Disabled, TRUE, GTSL_Min, -128, GTSL_Max, 127,
							GTSL_LevelFormat, GetString(&gLocaleInfo, MSG_PRIORITY_SLIDER_GAD), GTSL_Level, 0,
							GTSL_MaxPixelLen, n, GTSL_MaxLevelLen, 15, GA_RelVerify, TRUE, TAG_END);

	ngad.ng_LeftEdge = w+8+TLn(GetString(&gLocaleInfo, MSG_FORMAT_LABEL));
	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge-boxwidth;
	ngad.ng_Height += 3;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FORMAT_LABEL);
	ngad.ng_GadgetID++;					// 7
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GTST_String,NULL,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;					// 8
	gad = CreatePopGadget(wd,gad,FALSE);

	sprintf(buffer, "%s %s", GetString(&gLocaleInfo, MSG_DECIMAL_PLACES_LABEL), GetString(&gLocaleInfo, MSG_AUTO_VALUE_GAD));
	ngad.ng_LeftEdge = w + 8 + (n = TLn(buffer) + 4);
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = gWidth - 8 - rborder-ngad.ng_LeftEdge;
	ngad.ng_Height -= 3;
	ngad.ng_GadgetText = NULL;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 9
	gad = wd->wd_ExtData[1] = CreateGadget(SLIDER_KIND, gad, &ngad, GA_Disabled, TRUE, GTSL_Min, -1, GTSL_Max, 10,
											GTSL_LevelFormat, GetString(&gLocaleInfo, MSG_DECIMAL_PLACES_LABEL), GTSL_Level, 0,
											GTSL_MaxPixelLen, n, GTSL_MaxLevelLen, 20, GA_RelVerify, TRUE, TAG_END);

	ngad.ng_LeftEdge = w+8+TLn(GetString(&gLocaleInfo, MSG_ALIGNMENT_LABEL));
	ngad.ng_TopEdge += ngad.ng_Height = fontheight+4;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ALIGNMENT_LABEL);
	ngad.ng_GadgetID++;					// 10
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GA_Disabled, TRUE, GTCY_Labels, &sAlignLabels, TAG_END);

	ngad.ng_LeftEdge = w;
	ngad.ng_TopEdge += 2*fontheight+13;
	ngad.ng_Width = boxwidth - GOFFSET;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEG_VALUES_LABEL);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;					// 11
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GA_Disabled,TRUE,GTCB_Scaled,TRUE,TAG_END);

	ngad.ng_LeftEdge += 16 - GOFFSET + 3 * boxwidth+TLn(GetString(&gLocaleInfo, MSG_NEG_VALUES_LABEL));
	ngad.ng_GadgetID++;					// 12
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge = w;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = boxwidth - GOFFSET;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEG_VALUES_GAD);
	ngad.ng_GadgetID++;					// 13
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GA_Disabled,TRUE,GTCB_Scaled,TRUE,TAG_END);

	ngad.ng_LeftEdge = w;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = boxwidth - GOFFSET;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_THOUSANDS_SEPARATOR_GAD);
	ngad.ng_GadgetID++;					// 14
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GA_Disabled,TRUE,GTCB_Scaled,TRUE,TAG_END);


	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = barheight+fontheight*(i+2)+20;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 15
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-ngad.ng_Width-rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 16
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreatePrefColorsGads(REG(a0, struct winData *wd))
{
	struct ColorWheelRGB rgb;
	struct ColorWheelHSB hsb;
	long   w,w2,h,i = 8;
	short  *pens,numPens = 0;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 10; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 11; //Set Gadgetnumber 
#endif

	w = TLn(GetString(&gLocaleInfo, MSG_HUE_LABEL));  w2 = TLn(GetString(&gLocaleInfo, MSG_BRIGHTNESS_LABEL));  w = max(w,w2);  w2 = TLn(GetString(&gLocaleInfo, MSG_BRIGHTNESS_LABEL));  w = max(w,w2);
	gWidth = 2*w+134+lborder+TLn(GetString(&gLocaleInfo, MSG_COLOR_SPACE_RGB_GAD))+(w2 = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+128+rborder);
	gHeight = barheight+5*fontheight+i*(fontheight+4)+39+bborder;
	while (gHeight > scr->Height) {
		i--;
		gHeight -= fontheight;
	}

	if ((pens = wd->wd_ExtData[6]) != 0) {
		pens[MAXGRADPENS+1] = ObtainPen(scr->ViewPort.ColorMap,-1,0,0,0,PEN_EXCLUSIVE | PEN_NO_SETCOLOR);

		GetRGB32(scr->ViewPort.ColorMap,0,1,(ULONG *)&rgb);
		ConvertRGBToHSB(&rgb,&hsb);
		while (numPens < MAXGRADPENS) {
			hsb.cw_Brightness = 0xffffffff-((0xffffffff/MAXGRADPENS)*numPens);
			ConvertHSBToRGB(&hsb,&rgb);
			if ((pens[numPens] = ObtainPen(scr->ViewPort.ColorMap,-1,rgb.cw_Red,rgb.cw_Green,rgb.cw_Blue,PEN_EXCLUSIVE)) == -1)
				break;
			numPens++;
		}
		pens[numPens] = ~0;
		wd->wd_ExtData[7] = (APTR)((long)numPens);
		if (!numPens) {
			pens[0] = FindColor(scr->ViewPort.ColorMap,0xffffffff,0xffffffff,0xffffffff,7);
			pens[1] = FindColor(scr->ViewPort.ColorMap,0x88888888,0x88888888,0x88888888,7);
			pens[2] = FindColor(scr->ViewPort.ColorMap,0x00000000,0x00000000,0x00000000,7);
			pens[3] = ~0;
		}
	}
	ngad.ng_LeftEdge = gWidth-w2;
	ngad.ng_TopEdge = barheight+fontheight+5;
	ngad.ng_Width = w2-8-rborder;
	ngad.ng_GadgetText = NULL;			// 1
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GT_Underscore, '_', GTCY_Labels, &sColorViewLabels, TAG_END);

	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_Height = (fontheight+4)*i+4;
	ngad.ng_GadgetID++;					// 2
	gad = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, wd->wd_Data, GTLV_ShowSelected, NULL,
							GTLV_CallBack, &colorHook, GTLV_MaxPen, ~0L, GTLV_ItemHeight, fontheight + 4, TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD)) + 40;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 3
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_COPY_GAD))+40;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_COPY_GAD);
	ngad.ng_GadgetID++;					// 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+40;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;					// 5
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-w2+24+TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	h = ngad.ng_TopEdge -= 2*fontheight+5;
	ngad.ng_LeftEdge = 24+lborder+w+TLn("999");
	ngad.ng_Width = gWidth-w2-26-rborder-ngad.ng_LeftEdge-TLn(GetString(&gLocaleInfo, MSG_COLOR_SPACE_RGB_GAD))-boxwidth;
	ngad.ng_Height -= 3;
	ngad.ng_GadgetText = NULL;
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 7
	wd->wd_ExtData[2] = gad = CreateGadget(SLIDER_KIND,gad,&ngad,GTSL_Min,0,GTSL_Max,255,GTSL_LevelFormat,"%ld",GTSL_Level,0,GTSL_MaxPixelLen,i = TLn("999"),GTSL_MaxLevelLen,3,GA_RelVerify,TRUE,GA_Immediate,TRUE,TAG_END);

	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_GadgetID++;					// 8
	wd->wd_ExtData[3] = gad = CreateGadget(SLIDER_KIND,gad,&ngad,GTSL_Min,0,GTSL_Max,255,GTSL_LevelFormat,"%ld",GTSL_Level,0,GTSL_MaxPixelLen,i,GTSL_MaxLevelLen,3,GA_RelVerify,TRUE,GA_Immediate,TRUE,TAG_END);

	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_GadgetID++;					// 9
	wd->wd_ExtData[4] = gad = CreateGadget(SLIDER_KIND,gad,&ngad,GTSL_Min,0,GTSL_Max,255,GTSL_LevelFormat,"%ld",GTSL_Level,0,GTSL_MaxPixelLen,i,GTSL_MaxLevelLen,3,GA_RelVerify,TRUE,GA_Immediate,TRUE,TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+9;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+16;
	ngad.ng_Height += 3;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 10
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 11
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-w2-24-boxwidth-TLn(GetString(&gLocaleInfo, MSG_COLOR_SPACE_RGB_GAD));
	ngad.ng_TopEdge = h;
	ngad.ng_Width = (boxwidth*fontheight)/(fontheight+4);
	ngad.ng_Height = fontheight;
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;					// 12
	gad = CreateGadget(MX_KIND, gad, &ngad, GTMX_Labels, &sColorSpaceLabels, GTMX_Scaled, TRUE, GTMX_Spacing, 3, GT_Underscore, '_', TAG_END);

	h -= barheight+fontheight+6+bborder;
	w = (boxwidth*h)/(fontheight+4) -  3 * GOFFSET;
	w2 = ((gWidth-w2-w-boxwidth-6) >> 1) -  (GOFFSET / 2);
	gad = wd->wd_ExtData[0] = NewObj(wd, WOT_GADGET, NULL, "gradientslider.gadget",
										GA_Top,        barheight+fontheight+5,
										GA_Left,       w2+w+6,
										GA_Width,      boxwidth,
										GA_Height,     h,
										GRAD_PenArray, pens,
										PGA_Freedom,   LORIENT_VERT,
										GA_Previous,   gad,
										ICA_TARGET,    ICTARGET_IDCMP,
									TAG_END);
	gad = wd->wd_ExtData[1] = NewObj(wd, WOT_GADGET, NULL, "colorwheel.gadget",
										GA_Top,               barheight+fontheight+5,
										GA_Left,              w2,
										GA_Width,             w,
										GA_Height,            h,
										WHEEL_Screen,         scr,
										WHEEL_GradientSlider, gad,
										WHEEL_RGB,            &rgb,
										GA_FollowMouse,       TRUE,
										GA_Previous,          gad,
									TAG_END);
}


void ASM
CreatePressKeyGads(REG(a0, struct winData *wd))
{
	gWidth = TLn(GetString(&gLocaleInfo, MSG_PRESS_A_KEY_REQ))+30+lborder+rborder;
	gHeight = barheight+3*fontheight+13+bborder;

	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_GAD);
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+30;
	ngad.ng_TopEdge = gHeight-fontheight-7-bborder;
	ngad.ng_LeftEdge = (gWidth-ngad.ng_Width) >> 1;
	ngad.ng_Flags = PLACETEXT_IN;
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


struct Gadget *psysPages[3];


void ASM
CreateSystemPrefsGadgets(REG(a0, struct winData *wd))
{
	struct Gadget *pgad;
	long h;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 33; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 34; //Set Gadgetnumber 
#endif
	h = TLn(sWindowRefreshLabels[0]);
	gWidth = TLn(sWindowRefreshLabels[1]);
	gWidth = max(h, gWidth);
	h = TLn(sWindowRefreshLabels[2]);
	gWidth = max(h, gWidth);

	h = TLn(GetString(&gLocaleInfo, MSG_ICON_ON_WORKBENCH_GAD)) + TLn(GetString(&gLocaleInfo, MSG_MISC_BORDER));
	gWidth += TLn(GetString(&gLocaleInfo, MSG_WINDOW_REFRESH_GAD)) + 22;
	gWidth = max(h, gWidth) + 26 + boxwidth + lborder + rborder;

	gHeight = barheight + fontheight * 8 + 65 + bborder;
	h = gHeight - barheight - 2 * fontheight - 23 - bborder;
	psysPages[2] = NULL;

	/******* Diverses *******/
	pgad = CreateContext(&psysPages[0]);

	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_TopEdge = barheight + fontheight + 10 + (h - 5*fontheight - 32)/2;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ICON_ON_WORKBENCH_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;	// 1
	pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Checked,prefs.pr_Flags & PRF_APPICON,GTCB_Scaled,TRUE,TAG_END);

	if (!gIsBeginner) {
		ngad.ng_TopEdge += fontheight+7;
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_USE_SESSIONS_GAD);
		ngad.ng_GadgetID++;				// 2
		pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Checked,prefs.pr_Flags & PRF_USESESSION,GTCB_Scaled,TRUE,TAG_END);
	}

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CHECK_SECURITY_GAD);
	ngad.ng_GadgetID = 3;				// 3
	pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Checked,prefs.pr_Flags & PRF_SECURESCRIPTS,GTCB_Scaled,TRUE,TAG_END);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CONTEXT_MENUS_GAD);
	ngad.ng_GadgetID++;					// 4
	pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Checked,prefs.pr_Flags & PRF_CONTEXTMENU,GTCB_Scaled,TRUE,TAG_END);

	// deaktiviert wegen Darstellungs-Fehler (z.B. beim Picture-Objekt) ohne Puffer!
/*	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_GadgetText = "Beim Zeichnen Puffer benutzen";
	ngad.ng_GadgetID++;					// 5
	pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,prefs.pr_Flags & PRF_USEDBUFFER,GT_Underscore,'_',TAG_END);
*/

	if (!gIsBeginner) {
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_WINDOW_REFRESH_GAD);
		ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
		ngad.ng_TopEdge += fontheight+7;
		ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
		ngad.ng_Flags = PLACETEXT_LEFT;
		ngad.ng_GadgetID = 6;			// 6
		CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &sWindowRefreshLabels,
						GTCY_Active, prefs.pr_Flags & PRF_SIMPLEWINDOWS ? (prefs.pr_Flags & PRF_SIMPLEPROJS ? 2 : 1) : 0, TAG_END);
	}

	/******* Clipboard *******/
	pgad = CreateContext(&psysPages[1]);

	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_TopEdge = barheight+fontheight+10+(h-2*fontheight-11)/2;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CLIMB_LIST_GAD);
	ngad.ng_GadgetID = 7;				// 7
	ngad.ng_Flags = PLACETEXT_RIGHT;
	pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Checked,prefs.pr_Flags & PRF_CLIPGO,GTCB_Scaled,TRUE,TAG_END);

	ngad.ng_LeftEdge += 8+TLn(GetString(&gLocaleInfo, MSG_CLIPBOARD_UNIT_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CLIPBOARD_UNIT_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 8
	/*pgad =*/ CreateGadget(INTEGER_KIND,pgad,&ngad,GTIN_Number,clipunit,TAG_END);

	/******* Page & Index, Ok & Cancel *******/
	gad = NewObj(wd, WOT_GADGET, pagegclass, NULL, PAGEGA_Pages,  &psysPages,
												   PAGEGA_Active, wd->wd_CurrentPage,
												   GA_Previous,   gad,
												   GA_Left,       8 + lborder,
												   GA_Top,        barheight + fontheight + 9,
												   GA_Width,      gWidth - 16 - lborder - rborder,
												   GA_Height,     h,
												   GA_ID,         31,
											   TAG_END);
	gad = NewObj(wd, WOT_GADGET, indexgclass, NULL, GA_Top,      barheight + 3,
													GA_Left,     lborder,
													GA_Width,    gWidth - rborder - lborder,
													GA_Height,   gHeight - barheight - fontheight - 13 - bborder,
													GA_DrawInfo, dri,
													GA_Previous, gad,
													ICA_TARGET,  gad,
													GA_ID,       32,
													IGA_Labels,  &sSystemPageLabels,
													IGA_Active,  wd->wd_CurrentPage,
												TAG_END);
	wd->wd_PageHandlingGadget = gad;

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = gHeight - fontheight - 7 - bborder;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD)) + 16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = 33;				// 33
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = gWidth - rborder - ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 34
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
}

void ASM
CreatePrefContextGads(REG(a0, struct winData *wd))
{
#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 9; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 10; //Set Gadgetnumber 
#endif

	gWidth = 2 * TLn(sContextMenuLabels[1]) + 70 + lborder + rborder;
	gHeight	= barheight+16*fontheight+46+bborder;

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = barheight+fontheight+9;
	ngad.ng_Width = gWidth-lborder-rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CONTEXT_LABEL);
	ngad.ng_Flags = PLACETEXT_ABOVE;	// 1
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sContextMenuLabels, TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_Width = gWidth-lborder-rborder;
	ngad.ng_Height = fontheight*10+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;					// 2
	wd->wd_ExtData[6] = gad = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,wd->wd_ExtData[(LONG)wd->wd_ExtData[5]],GTLV_ShowSelected,NULL,GTLV_CallBack,&popUpHook,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width >>= 1;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 3
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;					// 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_TITLE_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TITLE_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					// 5
	gad = CreateGadget(STRING_KIND,gad,&ngad,TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth - lborder - rborder - boxwidth - TLn(gAppCommandLabels[0]) - 42;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;					// 6
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(gAppCommandLabels[0]) + 42;
	ngad.ng_GadgetID++;					// 7
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &gAppCommandLabels, GA_Disabled, TRUE, TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
//  ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 8
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+8;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+20;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					// 9
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore, '_',TAG_END);

	ngad.ng_LeftEdge = gWidth - rborder - ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					// 10
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_',TAG_END);
}


void
InitPrefsGadgetsLabels(void)
{
	MakeLocaleLabels(sGridLabels, MSG_GRID_OFF_GAD, MSG_GRID_POINTS_GAD, MSG_GRID_LINE_GAD, TAG_END);

	MakeLocaleLabels(sFormulaBarLabels, MSG_BAR_OFF_GAD, MSG_BAR_BOTTOM_GAD, MSG_BAR_TOP_GAD, TAG_END);
	MakeLocaleLabels(sIconBarLabels, MSG_BAR_OFF_GAD, MSG_BAR_LEFT_GAD, MSG_BAR_TOP_GAD, TAG_END);

	MakeLocaleLabels(sAutoSaveLabels, MSG_AUTO_SAVE_OFF_GAD, MSG_AUTO_SAVE_REMEMBER_GAD, MSG_AUTO_SAVE_ON_GAD, TAG_END);

	MakeLocaleLabels(sBlockDisplayLabels, MSG_BD_POSITION_ONLY_GAD, MSG_BD_WITH_SUM_GAD, MSG_BD_WITH_AVERAGE_GAD, TAG_END);
	MakeLocaleLabels(sQualifierLabels, MSG_WITHOUT_QUALIFIER_GAD, MSG_QUAL_SHIFT_GAD, MSG_QUAL_ALT_GAD,
						MSG_QUAL_SHIFT_ALT_GAD, MSG_QUAL_CONTROL_GAD, TAG_END);

	MakeLocaleLabels(sKeyLabels, MSG_KEY_ALWAYS_GAD, MSG_KEY_NOT_WHILE_EDITING_GAD, MSG_KEY_ONLY_WHILE_EDITING_GAD, TAG_END);

	MakeLocaleLabels(sNameTypeLabels, MSG_NAME_TYPE_STANDARD_GAD, MSG_NAME_TYPE_REFERENCE_GAD, MSG_NAME_TYPE_SEARCH_GAD, TAG_END);

	MakeLocaleLabels(sFormatTypeLabels, MSG_FORMAT_VALUE_GAD, MSG_FORMAT_PERCENT_GAD, MSG_FORMAT_DATE_GAD,
						MSG_FORMAT_TIME_GAD, MSG_FORMAT_UNIT_GAD, TAG_END);
	MakeLocaleLabels(sAlignLabels, MSG_ALIGN_LEFT_GAD, MSG_ALIGN_RIGHT_GAD, MSG_ALIGN_CENTER_GAD, TAG_END);

	MakeLocaleLabels(sColorViewLabels, MSG_COLOR_PALETTE_GAD, MSG_COLOR_SCREEN_GAD, TAG_END);
	MakeLocaleLabels(sColorSpaceLabels, MSG_COLOR_SPACE_RGB_GAD, MSG_COLOR_SPACE_HSB_GAD, TAG_END);
	MakeLocaleLabels(gColorModeLabels, MSG_COLOR_RED_LABEL, MSG_COLOR_GREEN_LABEL, MSG_COLOR_BLUE_LABEL, MSG_COLOR_HUE_LABEL, MSG_COLOR_SATURATION_LABEL, MSG_COLOR_BRIGHTNESS_LABEL, TAG_END);

	MakeLocaleLabels(sSystemPageLabels, MSG_SYSTEM_MISC_BORDER, MSG_SYSTEM_CLIPBOARD_BORDER, TAG_END);
	MakeLocaleLabels(sWindowRefreshLabels, MSG_REFRESH_FAST_GAD, MSG_REFRESH_SAVE_MEMORY_GAD, MSG_REFRESH_LOW_MEMORY_GAD, TAG_END);

	MakeLocaleLabels(sContextMenuLabels, MSG_CONTEXT_CELLS_GAD, MSG_CONTEXT_HORIZONTAL_TITLE_GAD, MSG_CONTEXT_VERTICAL_TITLE_GAD,
						MSG_CONTEXT_OBJECTS_GAD, MSG_CONTEXT_CELL_SELECTION_GAD, TAG_END);

	MakeLocaleLabels(sScreenLabels, MSG_ON_PUBLIC_ULABEL, MSG_LIKE_WORKBENCH_UGAD, MSG_OPEN_SCREEN_ULABEL, TAG_END);
	MakeLocaleLabels(sCommandLabels, MSG_INTERNAL_COMMAND_UGAD, MSG_AREXX_COMMAND_UGAD, MSG_DOS_COMMAND_UGAD, TAG_END);
}
