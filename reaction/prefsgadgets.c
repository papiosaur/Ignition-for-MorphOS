/* Gadget creation functions for the preferences windows
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#include "types.h"
#include "funcs.h"

#ifdef __amigaos4__
	#include "reactionGUI.h"

	Object *objects[LAST_NUM];

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
DBUG("CreatePrefDispGads()\n");

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
// [SHEET GROUP]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_Label,       GetString(&gLocaleInfo,MSG_SHEET_BORDER),//"Sheet"
				LAYOUT_AddChild, OBJ(OID_D_GRID) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
					GA_ID,         0,//OID_D_GRID,
					GA_RelVerify,  TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					CHOOSER_LabelArray, sGridLabels,
					CHOOSER_Selected,   pd->pd_Rasta & PDR_CYCLES,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_GRID_LABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_D_CELLWIDTH) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
					GA_ID,        0,//OID_D_CELLWIDTH,
					GA_RelVerify, TRUE,
					GA_Selected,  pd->pd_Rasta & PDR_CELLWIDTH,
					GA_Disabled,  TRUE,
					//GA_HintInfo,  "",
					GA_Text,      GetString(&gLocaleInfo,MSG_GRID_CELL_WIDTH_GAD),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_D_HEADINGS) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
					GA_ID,        0,//OID_D_HEADINGS,
					GA_RelVerify, TRUE,
					GA_Selected,  pd->pd_ShowAntis,
					//GA_HintInfo,  "",
					GA_Text,      GetString(&gLocaleInfo,MSG_SHOW_HEADINGS_GAD),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_D_FONT) = NewObject(GetFontClass, NULL, //"getfont.gadget",
					GA_ID,        OID_D_FONT,
					GA_RelVerify, TRUE,
					GETFONT_TextAttr,  pd->pd_AntiAttr,
					GETFONT_MaxHeight, 100,
				TAG_DONE),
				CHILD_WeightedHeight, 0,
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_FONT_LABEL),
				TAG_DONE),
			TAG_DONE), // END [SHEET GROUP]
// [TOOLBAR GROUP]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation,  LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,   BVS_GROUP,
				LAYOUT_Label,        GetString(&gLocaleInfo,MSG_BARS_BORDER),//"Toolbars"
				LAYOUT_InnerSpacing, 20, // small gap/space between columns
// TOOLBAR GROUP: [COLUMN 1]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_AddChild, OBJ(OID_D_HELPBAR) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
						GA_ID,        0,//OID_D_HELPBAR,
						GA_RelVerify, TRUE,
						GA_Selected,  pd->pd_HelpBar,
						//GA_HintInfo,  "",
						GA_Text,      GetString(&gLocaleInfo,MSG_HELP_BAR_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_D_TOOLBAR) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
						GA_ID,        0,//OID_D_TOOLBAR,
						GA_RelVerify, TRUE,
						GA_Selected,  pd->pd_ToolBar,
						//GA_HintInfo,  "",
						GA_Text,      GetString(&gLocaleInfo,MSG_TOOL_BAR_GAD),
					TAG_DONE),
				TAG_DONE), // END TOOLBAR GROUP: [COLUMN 1]
// TOOLBAR GROUP: [COLUMN 2]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_AddChild, OBJ(OID_D_ICONBAR) = NewObject(ChooserClass, NULL, //"chooser.gadget",
						GA_ID,         0,//OID_D_ICONBAR,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						CHOOSER_LabelArray, sIconBarLabels,
						CHOOSER_Selected,   pd->pd_IconBar,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_ICON_BAR_LABEL),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_D_FORMULABAR) = NewObject(ChooserClass, NULL, //"chooser.gadget",
						GA_ID,         0,//OID_D_FORMULABAR,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						CHOOSER_LabelArray, sFormulaBarLabels,
						CHOOSER_Selected,   pd->pd_FormBar,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_FORMULA_BAR_LABEL),
					TAG_DONE),
				TAG_DONE), // END TOOLBAR GROUP: [COLUMN 2]
			TAG_DONE), // END [TOOLBAR GROUP]
// [BUTTONS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_TEST,//10
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_TEST_GAD),//"Test"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_CANCEL,//11
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefDispGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreatePrefScreenGads(REG(a0, struct winData *wd))
{
  struct List *l, *pslist = (struct List *)AllocSysObject(ASOT_LIST, NULL);
  struct Node *sn = NULL, *node = NULL;
  //uint32 maxwidth = 0; // set max. OID_S_SCREENNAME gadget length
DBUG("CreatePrefScreenGads()\n");

  // Create public screens list
  if( (l=LockPubScreenList()) != 0 )
  {
   for( sn=GetHead(l); sn; sn=GetSucc(sn) )
   {
    if( strcmp(pubname,sn->ln_Name) )
    {
     node = AllocChooserNode(CNA_CopyText,TRUE, CNA_Text,sn->ln_Name, TAG_DONE);
     AddTail(pslist, node);
DBUG("SCREEN '%s' (0x%08lx)\n", sn->ln_Name,node);//sn->ln_Name,sn);
     //if(TLn(sn->ln_Name) > maxwidth)
     //{
     // maxwidth = TLn(sn->ln_Name);
     //}
//DBUG("SCREEN '%s' %ldpx\n", sn->ln_Name,maxwidth);
    }
   }
   UnlockPubScreenList();
  }
//DBUG("SCREEN (0x%08lx)\n", sn);
	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
	    LAYOUT_Orientation,  LAYOUT_ORIENT_VERT,
	    //LAYOUT_BevelStyle,   BVS_GROUP,
// [SCREEN GROUP]
	    LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
	      LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
	      LAYOUT_BevelStyle,  BVS_GROUP,
	      LAYOUT_Label,       GetString(&gLocaleInfo,MSG_SCREEN_BORDER),//"Screen"
	      LAYOUT_AddChild, OBJ(OID_S_SCREENTYPE) = NewObject(RadiobuttonClass, NULL, //"radiobutton.gadget",
	       GA_ID,        OID_S_SCREENTYPE,
	       GA_RelVerify, TRUE,
	       GA_Text,      sScreenLabels,
	       RADIOBUTTON_Selected, prefs.pr_Screen->ps_Type,
	      TAG_DONE),
	      CHILD_WeightedWidth, 0,
// SCREEN GROUP: [COLUMN RIGHT]
	      LAYOUT_AddChild, OBJ(OID_S_SCRGROUP) = NewObject(LayoutClass, NULL, //"layout.gadget",
	       LAYOUT_Orientation,  LAYOUT_ORIENT_VERT,
	       LAYOUT_BevelStyle,   BVS_NONE,
	       //LAYOUT_InnerSpacing, 20, // small gap/space between gadgets
	       LAYOUT_AddChild, OBJ(OID_S_SCREENNAME) = NewObject(ChooserClass, NULL, //"chooser.gadget",
	        GA_ID,         OID_S_SCREENNAME,
	        GA_RelVerify,  TRUE,
	        GA_Disabled,   prefs.pr_Screen->ps_Type==0? FALSE:TRUE,
	        //GA_HintInfo,   "",
	        GA_Underscore, 0,
	        CHOOSER_Title,    prefs.pr_Screen->ps_PubName,
	        CHOOSER_DropDown, TRUE,
	        CHOOSER_Labels,   pslist, // public screens list
	        CHOOSER_Selected, 0,
	        //CHOOSER_Width, maxwidth+40,
	       TAG_DONE),
	       //CHILD_MinWidth, maxwidth+40, // to fit widest string
	       LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
	        GA_ID,       0,
	        GA_ReadOnly, TRUE,
	        SPACE_MinHeight, fontheight>>1, // '>> 1' = '/ 2'
	        //SPACE_Transparent, TRUE,
	       TAG_DONE),
	       LAYOUT_AddChild, OBJ(OID_S_SELECTSCR) = NewObject(GetScreenModeClass, NULL, //"getscreenmode.gadget",
	        GA_ID,        OID_S_SELECTSCR,
	        GA_RelVerify, TRUE,
	        GA_Disabled,  prefs.pr_Screen->ps_Type==2? FALSE:TRUE,
	        GETSCREENMODE_DisplayID, prefs.pr_Screen->ps_ModeID,
	        GETSCREENMODE_DoWidth,        TRUE,
	        GETSCREENMODE_DoHeight,       TRUE,
	        GETSCREENMODE_DoDepth,        TRUE,
	        GETSCREENMODE_DoOverscanType, TRUE,
	       TAG_DONE),
	       //CHILD_WeightedHeight, 0,
	       //CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
	       // LABEL_Text, GetString(&gLocaleInfo,MSG_SCREEN_MODE_GAD),
	       //TAG_DONE),
	       LAYOUT_AddChild, OBJ(OID_S_SELECTFONT) = NewObject(GetFontClass, NULL, //"getfont.gadget",
	        GA_ID,        OID_S_SELECTFONT,
	        GA_RelVerify, TRUE,
	        GA_Disabled,  prefs.pr_Screen->ps_Type==2? FALSE:TRUE,
	        GETFONT_MaxHeight, 100,
	        GETFONT_TextAttr,  prefs.pr_Screen->ps_TextAttr,
	       TAG_DONE),
	       //CHILD_WeightedHeight, 0,
	       //CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
	       // LABEL_Text, GetString(&gLocaleInfo,MSG_FONT_ELLIPSIS_GAD),
	       //TAG_DONE),
	      TAG_DONE), // END SCREEN GROUP: [COLUMN RIGHT]
	      CHILD_WeightedHeight, 0,
	    TAG_DONE), // END [SCREEN GROUP]
// [MONITOR GROUP]
	    LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
	      //LAYOUT_Orientation,  LAYOUT_ORIENT_HORIZ,
	      LAYOUT_BevelStyle,   BVS_GROUP,
	      LAYOUT_Label,        GetString(&gLocaleInfo,MSG_MONITOR_BORDER),//"Monitor"
	      //LAYOUT_InnerSpacing, 20, // small gap/space between columns
	      LAYOUT_AddChild, OBJ(OID_S_MONITORW) = NewObject(IntegerClass, NULL, //"integer.gadget",
	       GA_ID,        0,//OID_S_MONITORW,
	       GA_RelVerify, TRUE,
	       //GA_HintInfo,  "",
	       INTEGER_Number,   prefs.pr_Screen->ps_mmWidth / 10240,
	       INTEGER_Minimum,  1,
	       INTEGER_Maximum,  999,
	       INTEGER_MaxChars, 4,
	      TAG_DONE),
	      CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
	       LABEL_Text, GetString(&gLocaleInfo,MSG_WIDTH_LABEL),
	      TAG_DONE),
	      LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
	       GA_ID,       0,
	       GA_ReadOnly, TRUE,
	       GA_Text,     "cm",
	       BUTTON_BevelStyle,    BVS_NONE,
	       BUTTON_Transparent,   TRUE,
	       BUTTON_Justification, BCJ_LEFT,
	      TAG_DONE),
	      CHILD_WeightedWidth,  0,
	      CHILD_WeightedHeight, 0,
	      LAYOUT_AddChild, OBJ(OID_S_MONITORH) = NewObject(IntegerClass, NULL, //"integer.gadget",
	       GA_ID,        0,//OID_S_MONITORH,
	       GA_RelVerify, TRUE,
	       //GA_HintInfo,  "",
	       INTEGER_Number,   prefs.pr_Screen->ps_mmHeight / 10240,
	       INTEGER_Minimum,  1,
	       INTEGER_Maximum,  999,
	       INTEGER_MaxChars, 4,
	      TAG_DONE),
	      CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
	       LABEL_Text, GetString(&gLocaleInfo,MSG_HEIGHT_LABEL),
	      TAG_DONE),
	      LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
	       GA_ID,       0,
	       GA_ReadOnly, TRUE,
	       GA_Text,     "cm",
	       BUTTON_BevelStyle,    BVS_NONE,
	       BUTTON_Transparent,   TRUE,
	       BUTTON_Justification, BCJ_LEFT,
	      TAG_DONE),
	      CHILD_WeightedWidth,  0,
	      CHILD_WeightedHeight, 0,
	    TAG_DONE),// END [MONITOR GROUP]
// [BACKGROUND GROUP]
	    LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
	      //LAYOUT_Orientation,  LAYOUT_ORIENT_HORIZ,
	      LAYOUT_BevelStyle,   BVS_GROUP,
	      LAYOUT_Label,        GetString(&gLocaleInfo,MSG_BACKGROUND_BORDER),//"Background"
	      //LAYOUT_InnerSpacing, 20, // small gap/space between columns
	      LAYOUT_AddChild, OBJ(OID_S_BGPATTERN) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
	       GA_ID,        OID_S_BGPATTERN,
	       GA_RelVerify, TRUE,
	       GA_Selected,  prefs.pr_Screen->ps_BackFill,
	       //GA_HintInfo,  "",
	       GA_Text,      GetString(&gLocaleInfo,MSG_PATTERN_UGAD),
	      TAG_DONE),
	      CHILD_WeightedWidth, 0,
	      LAYOUT_AddChild, OBJ(OID_S_PALETTE) = NewObject(PaletteClass, NULL, //"palette.gadget",
	       GA_ID,        OID_S_PALETTE,
	       GA_RelVerify, TRUE,
	       GA_Disabled, !prefs.pr_Screen->ps_BackFill,
	       //GA_ReadOnly,  TRUE,
	       //GA_HintInfo,  "",
	       //PALETTE_Color,      prefs.pr_Screen->ps_BFColor,
	       PALETTE_NumColours, 8,
	      TAG_DONE),
	    TAG_DONE), // END [BACKGROUND GROUP]
// [BUTTONS]
	    LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
	      LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
	      LAYOUT_SpaceOuter,  TRUE,
	      LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
	       GA_ID,        OID_OK,//9
	       GA_RelVerify, TRUE,
	       GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
	      TAG_DONE),
	      CHILD_WeightedWidth, 0,
	      LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
	       GA_ID,        OID_CANCEL,//11
	       GA_RelVerify, TRUE,
	       GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
	      TAG_DONE),
	      CHILD_WeightedWidth, 0,
	    TAG_DONE),
	    CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefScreenGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void
prefmenuToRALB(struct MinList *min_l, struct List* lst) // RALB = ReAction ListBrowser
{
	struct Node *n = NULL;
	struct IgnAppMenu *am;
DBUG("prefmenuToRALB():\n");
	FreeListBrowserList(lst);

	foreach(min_l, am)
	{
		if( (n=AllocListBrowserNode(1,
		                            LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,am->am_Node.ln_Name,
		                            LBNA_UserData, am, // prefs menu list [NATIVE] item
		                           TAG_DONE)) )
		{
			AddTail(lst, n);
DBUG("\t'%s' (0x%08lx)\n",am->am_Node.ln_Name,n);
		}
	}
}

void
prefmenuitemsToRALB(struct IgnAppMenu *am, struct IgnAppMenuEntry *ame, struct List* lst) // RALB = ReAction ListBrowser
{
	struct Node *n = NULL;
	struct IgnAppMenuEntry *same;
DBUG("prefmenuitemsToRALB():\n");
	if(am==NULL && ame==NULL) return; // invalid both args empty
	if(am!=NULL && ame!=NULL) return; // invalid both args set

	FreeListBrowserList(lst);
	if(am != NULL)
	{
		foreach(&am->am_Items, ame)
		{
			if( (n=AllocListBrowserNode(1,
			                            LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,ame->am_Node.ln_Name,
			                            LBNA_UserData, ame, // prefs menu list [NATIVE] items
			                           TAG_DONE)) )
			{
				AddTail(lst, n);
DBUG("\t'%s' '%s' '%s' (0x%08lx) [item]\n",ame->am_Node.ln_Name,ame->am_AppCmd,ame->am_ShortCut,n);
			}
		}
	}
	else
	{
		foreach(&ame->am_Subs, same)
		{
			if( (n=AllocListBrowserNode(1,
			                            LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,same->am_Node.ln_Name,
			                            LBNA_UserData, same, // prefs menu list [NATIVE] (sub)items
			                           TAG_DONE)) )
			{
				AddTail(lst, n);
DBUG("\t'%s' '%s' '%s' (0x%08lx) [subitem]\n",same->am_Node.ln_Name,same->am_AppCmd,same->am_ShortCut,n);
			}
		}
	}
DBUG("\tn=0x%08lx\n",n);
}

void ASM CreatePrefMenuGads(REG(a0, struct winData *wd))
{
// wd->wd_ExtData[0] -> menu title list(browser) [REACTION] (was menu title list gadget [NATIVE])
// wd->wd_ExtData[1] -> menu items list(browser) [REACTION] (was menu items list gadget [NATIVE])
// wd->wd_ExtData[2] -> menu subitems list(browser) [REACTION] (was menu subitems list gadget [NATIVE])
// wd->wd_ExtData[3] -> menu list [NATIVE]
// wd->wd_ExtData[4] -> PopUp list(browser) "normal" (was list gadget [NATIVE])
// wd->wd_ExtData[5] -> PopUp list(browser) "internal" (was context chooser index [NATIVE])
DBUG("CreatePrefMenuGads()\n");
	prefmenuToRALB( (struct MinList *)wd->wd_ExtData[3], (struct List *)wd->wd_ExtData[0] );

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
		LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		//LAYOUT_BevelStyle,  BVS_GROUP,
// [MENUS][ITEMS][SUBITEMS]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
			//LAYOUT_BevelStyle,  BVS_GROUP,
			LAYOUT_SpaceOuter,  TRUE,
// [LISTMENUS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_Label, GetString(&gLocaleInfo,MSG_MENU_TITLE_GAD), // "Title"
				LAYOUT_AddChild, OBJ(OID_M_LISTMENUS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
					GA_ID,        OID_M_LISTMENUS,
					GA_RelVerify, TRUE,
					//GA_HintInfo,  "",
					//LISTBROWSER_AutoFit,      TRUE,
					//LISTBROWSER_Striping,     LBS_ROWS,
					//LISTBROWSER_MinVisible,   10,
					LISTBROWSER_ShowSelected, TRUE,
					LISTBROWSER_Labels,       wd->wd_ExtData[0],
				TAG_DONE),
				//CHILD_MinWidth, 200, // in pixels
// [/\][\/][NEW][DELETE]
				LAYOUT_AddChild, OBJ(OID_M_BTNGROUP_M) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, OBJ(OID_M_UP_M) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_UP_M,
						GA_RelVerify, TRUE,
						//GA_Text,      "/\\",
						BUTTON_AutoButton, BAG_UPARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_M_DOWN_M) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_DOWN_M,
						GA_RelVerify, TRUE,
						//GA_Text,      "\\/",
						BUTTON_AutoButton, BAG_DNARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
						GA_ID,       0,
						GA_ReadOnly, TRUE,
						//SPACE_Transparent, TRUE,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_NEW_M,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_NEW_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_M_DEL_M) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_DEL_M,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
					TAG_DONE),
				TAG_DONE), // END [/\][\/][NEW][DELETE]
				CHILD_WeightedHeight, 0,
				LAYOUT_AddChild, OBJ(OID_M_TITLE_M) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_M_TITLE_M,
					GA_RelVerify, TRUE,
				TAG_DONE),
			TAG_DONE), // END [LISTMENUS]
			LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
				GA_ID,       0,
				GA_ReadOnly, TRUE,
				//SPACE_Transparent, TRUE,
			TAG_DONE),
			CHILD_WeightedWidth, 0,
// [LISTITEMS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_Label, GetString(&gLocaleInfo,MSG_MENU_ITEMS_GAD), // "Items"
				LAYOUT_AddChild, OBJ(OID_M_LISTITEMS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
					GA_ID,        OID_M_LISTITEMS,
					GA_RelVerify, TRUE,
					//GA_HintInfo,  "",
					//LISTBROWSER_AutoFit,      TRUE,
					//LISTBROWSER_Striping,     LBS_ROWS,
					LISTBROWSER_MinVisible,   10,
					LISTBROWSER_ShowSelected, TRUE,
					//LISTBROWSER_Labels,       wd->wd_ExtData[5],
				TAG_DONE),
				//CHILD_MinWidth, 200, // in pixels
// [/\][\/][>][NEW][DELETE]
				LAYOUT_AddChild, OBJ(OID_M_BTNGROUP_I) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, OBJ(OID_M_UP_I) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_UP_I,
						GA_RelVerify, TRUE,
						//GA_Text,      "/\\",
						BUTTON_AutoButton, BAG_UPARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_M_DOWN_I) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_DOWN_I,
						GA_RelVerify, TRUE,
						//GA_Text,      "\\/",
						BUTTON_AutoButton, BAG_DNARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					/*LAYOUT_AddChild, OBJ(OID_M_TO_SUBITEM) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_TO_SUBITEM,
						GA_RelVerify, TRUE,
						//GA_Text,      ">",
						BUTTON_AutoButton, BAG_RTARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,*/
					LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
						GA_ID,       0,
						GA_ReadOnly, TRUE,
						//SPACE_Transparent, TRUE,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_M_NEW_I) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_NEW_I,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_NEW_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_M_DEL_I) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_DEL_I,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
					TAG_DONE),
				TAG_DONE), // END [/\][\/][<][NEW][DELETE]
				CHILD_WeightedHeight, 0,
				LAYOUT_AddChild, OBJ(OID_M_TITLE_I) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_M_TITLE_I,
					GA_RelVerify, TRUE,
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_M_HOTKEY_I) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_M_HOTKEY_I,
					GA_RelVerify, TRUE,
					STRINGA_Justification, GACT_STRINGRIGHT,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_SHORT_CUT_LABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_M_COMMAND_I) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_M_COMMAND_I,
					GA_RelVerify, TRUE,
				TAG_DONE),
// [TYPE][\/]
				LAYOUT_AddChild, OBJ(OID_M_GROUP_I) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, OBJ(OID_M_TYPE_I) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
						GA_ID,         0,//OID_M_TYPE_I,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						CHOOSER_LabelArray, &gAppCommandLabels,
						//CHOOSER_Selected,   0,
					TAG_DONE),
					//CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_M_POPUPBTN_I) = NewObject(ButtonClass, NULL, //"button.gadget", 
						GA_ID,        OID_M_POPUPBTN_I,
						GA_RelVerify, TRUE,
						//GA_Text,      "\\/",
						BUTTON_AutoButton, BAG_POPUP,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
				TAG_DONE), // END [TYPE][\/]
				CHILD_WeightedHeight, 0,
			TAG_DONE), // END [LISTITEMS]
			LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
				GA_ID,       0,
				GA_ReadOnly, TRUE,
				//SPACE_Transparent, TRUE,
			TAG_DONE),
			CHILD_WeightedWidth, 0,
// [LISTSUBITEMS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_Label, GetString(&gLocaleInfo,MSG_MENU_SUBITEMS_GAD), // "Sub-Items"
				LAYOUT_AddChild, OBJ(OID_M_LISTSUBITEMS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
					GA_ID,        OID_M_LISTSUBITEMS,
					GA_RelVerify, TRUE,
					//GA_HintInfo,  "",
					//LISTBROWSER_AutoFit,      TRUE,
					//LISTBROWSER_Striping,     LBS_ROWS,
					LISTBROWSER_MinVisible,   10,
					LISTBROWSER_ShowSelected, TRUE,
					//LISTBROWSER_Labels,       wd->wd_ExtData[5],
				TAG_DONE),
				//CHILD_MinWidth, 200, // in pixels
// [/\][\/][<][NEW][DELETE]
				LAYOUT_AddChild, OBJ(OID_M_BTNGROUP_SI) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, OBJ(OID_M_UP_SI) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_UP_SI,
						GA_RelVerify, TRUE,
						//GA_Text,      "/\\",
						BUTTON_AutoButton, BAG_UPARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_M_DOWN_SI) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_DOWN_SI,
						GA_RelVerify, TRUE,
						//GA_Text,      "\\/",
						BUTTON_AutoButton, BAG_DNARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					/*LAYOUT_AddChild, OBJ(OID_M_TO_ITEM) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_TO_ITEM,
						GA_RelVerify, TRUE,
						//GA_Text,      "<",
						BUTTON_AutoButton, BAG_LFARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,*/
					LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
						GA_ID,       0,
						GA_ReadOnly, TRUE,
						//SPACE_Transparent, TRUE,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_M_NEW_SI) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_NEW_SI,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_NEW_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_M_DEL_SI) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_M_DEL_SI,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
					TAG_DONE),
				TAG_DONE), // END [/\][\/][<][NEW][DELETE]
				CHILD_WeightedHeight, 0,
				LAYOUT_AddChild, OBJ(OID_M_TITLE_SI) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_M_TITLE_SI,
					GA_RelVerify, TRUE,
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_M_HOTKEY_SI) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_M_HOTKEY_SI,
					GA_RelVerify, TRUE,
					STRINGA_Justification, GACT_STRINGRIGHT,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_SHORT_CUT_LABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_M_COMMAND_SI) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_M_COMMAND_SI,
					GA_RelVerify, TRUE,
				TAG_DONE),
// [TYPE][\/]
				LAYOUT_AddChild, OBJ(OID_M_GROUP_SI) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, OBJ(OID_M_TYPE_SI) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
						GA_ID,         0,//OID_M_TYPE_SI,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						CHOOSER_LabelArray, &gAppCommandLabels,
						//CHOOSER_Selected,   0,
					TAG_DONE),
					//CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_M_POPUPBTN_SI) = NewObject(ButtonClass, NULL, //"button.gadget", 
						GA_ID,        OID_M_POPUPBTN_SI,
						GA_RelVerify, TRUE,
						//GA_Text,      "\\/",
						BUTTON_AutoButton, BAG_POPUP,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
				TAG_DONE), // END [TYPE][\/]
				CHILD_WeightedHeight, 0,
			TAG_DONE), // END [LISTSUBITEMS]
		TAG_DONE), // END [MENUS][ITEMS][SUBITEMS]
// [BUTTONS]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
			LAYOUT_BevelStyle,  BVS_SBAR_VERT,
			LAYOUT_SpaceOuter,  TRUE,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_OK,//9
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_CANCEL,//11
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
		TAG_DONE),
		CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefMenuGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreateFilePrefsGadgets(REG(a0, struct winData *wd))
{
	struct PrefFile *pf = wd->wd_Data;

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
// [PATH GROUP]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_Label,       GetString(&gLocaleInfo,MSG_PATHS_BORDER),//"Path"
				LAYOUT_AddChild, OBJ(OID_F_PATHDOCS) = NewObject(GetFileClass, NULL, //"getfile.gadget",
					GA_ID,        OID_F_PATHDOCS,
					GA_RelVerify, TRUE,
					GETFILE_DrawersOnly, TRUE,
					GETFILE_Drawer,      wd->wd_ExtData[0],
				TAG_DONE),
				CHILD_WeightedHeight, 0,
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_DOCUMENTS_LABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_F_PATHGFX) = NewObject(GetFileClass, NULL, //"getfile.gadget",
					GA_ID,        OID_F_PATHGFX,
					GA_RelVerify, TRUE,
					GETFILE_DrawersOnly, TRUE,
					GETFILE_Drawer,      wd->wd_ExtData[1],
				TAG_DONE),
				CHILD_WeightedHeight, 0,
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_GRAPHICS_LABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_F_PATHICONS) = NewObject(GetFileClass, NULL, //"getfile.gadget",
					GA_ID,        OID_F_PATHICONS,
					GA_RelVerify, TRUE,
					GETFILE_DrawersOnly, TRUE,
					GETFILE_Drawer,      wd->wd_ExtData[2],
				TAG_DONE),
				CHILD_WeightedHeight, 0,
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_ICONS_LABEL),
				TAG_DONE),
			TAG_DONE), // END [SHEET GROUP]
// [SAVE GROUP]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation,  LAYOUT_ORIENT_VERT,
				LAYOUT_BevelStyle,   BVS_GROUP,
				LAYOUT_Label,        GetString(&gLocaleInfo,MSG_SAVE_BORDER),//"Save"
				LAYOUT_InnerSpacing, 10, // small gap/space between [col1][col2] and [autosave]
// SAVE GROUP: [COL1][COL2]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation,  LAYOUT_ORIENT_HORIZ,
					//LAYOUT_BevelStyle,   BVS_GROUP,
					LAYOUT_InnerSpacing, 20, // small gap/space between [column 1] and [column 2]
// SAVE GROUP: [COLUMN 1]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						//LAYOUT_BevelStyle,  BVS_GROUP,
						LAYOUT_AddChild, OBJ(OID_F_CREATEICONS) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_F_CREATEICONS,
							GA_RelVerify, TRUE,
							GA_Selected,  pf->pf_Flags & PFF_ICONS,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_CREATE_ICONS_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_F_BACKUP) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_F_BACKUP,
							GA_RelVerify, TRUE,
							GA_Selected,  pf->pf_Flags & PFF_BACKUP,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_SECURITY_BACKUPS_GAD),
						TAG_DONE),
					TAG_DONE), // END SAVE GROUP: [COLUMN 1]
// SAVE GROUP: [COLUMN 2]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						//LAYOUT_BevelStyle,  BVS_GROUP,
						LAYOUT_AddChild, OBJ(OID_F_NOEXT) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_F_NOEXT,
							GA_RelVerify, TRUE,
							GA_Selected,  pf->pf_Flags & PFF_NOSUFFIX,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_NO_FILE_NAME_SUFFIX_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_F_WARNIOFMT) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_F_WARNIOFMT,
							GA_RelVerify, TRUE,
							GA_Selected,  pf->pf_Flags & PFF_WARN_IOTYPE,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_WARN_IOTYPE_NO_DEFAULT_GAD),
						TAG_DONE),
					TAG_DONE), // END SAVE GROUP: [COLUMN 2]
				TAG_DONE), // END SAVE GROUP: [COL1][COL2]
// SAVE GROUP: [AUTOSAVE]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_AddChild, OBJ(OID_F_AUTOSAVE) = NewObject(ChooserClass, NULL, //"chooser.gadget",
						GA_ID,         OID_F_AUTOSAVE,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						CHOOSER_LabelArray, sAutoSaveLabels,
						CHOOSER_Selected,   pf->pf_AutoSave,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_AUTO_SAVE_LABEL),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_F_AUTOSAVETIME) = NewObject(IntegerClass, NULL, //"integer.gadget",
						GA_ID,        0,//OID_F_AUTOSAVETIME,
						GA_RelVerify, TRUE,
						GA_Disabled,  !pf->pf_AutoSave,
						//GA_HintInfo,  "",
						INTEGER_Number,   pf->pf_AutoSaveIntervall / 60,
						INTEGER_Minimum,  0,
						INTEGER_Maximum,  99,
						INTEGER_MaxChars, 3,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_INTERVAL_LABEL),
					TAG_DONE),
					LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,       0,
						GA_ReadOnly, TRUE,
						GA_Text,     GetString(&gLocaleInfo,MSG_MINUTE_UNIT),
						BUTTON_BevelStyle,    BVS_NONE,
						BUTTON_Transparent,   TRUE,
						BUTTON_Justification, BCJ_LEFT,
					TAG_DONE),
					CHILD_WeightedWidth,  0,
					CHILD_WeightedHeight, 0,
				TAG_DONE), // END SAVE GROUP: [AUTOSAVE]
			TAG_DONE), // END [SAVE GROUP]
// [BUTTONS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_CANCEL,//11
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreateFilePrefsGadgets() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreateTablePrefsGadgets(REG(a0, struct winData *wd))
{
	struct PrefTable *pt = wd->wd_ExtData[0];
	uint32 maxwidth = TLn(GetString(&gLocaleInfo,MSG_CORNER_MODE_COPY_CELLS_GAD)) + 40;
//wd->wd_ExtData[3] -> OBJ(OID_T_QUALFUNC) list [REACTION] (was popup list [NATIVE])
//wd->wd_ExtData[4] -> OBJ(OID_T_QUALIFIER) index [REACTION] (was imsg.Code value [NATIVE])
DBUG("CreateTablePrefsGadgets()\n");

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
		LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		//LAYOUT_BevelStyle,  BVS_GROUP,
// [ERRORMSG GROUP]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			LAYOUT_BevelStyle,  BVS_GROUP,
			LAYOUT_Label,       GetString(&gLocaleInfo,MSG_ERROR_MESSAGES_BORDER),//"Error Messages",
			LAYOUT_AddChild, OBJ(OID_T_OPENERRREQ) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
				GA_ID,        0,//OID_T_OPENERRREQ,
				GA_RelVerify, TRUE,
				GA_Selected,  calcflags & CF_REQUESTER,
				//GA_HintInfo,  "",
				GA_Text,      GetString(&gLocaleInfo,MSG_OPEN_ERROR_REQUESTER_GAD),
			TAG_DONE),
			LAYOUT_AddChild, OBJ(OID_T_ZEROUNKNOWN) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
				GA_ID,        0,//OID_T_ZEROUNKNOWN,
				GA_RelVerify, TRUE,
				GA_Selected,  calcflags & CF_ZERONAMES,
				//GA_HintInfo,  "",
				GA_Text,      GetString(&gLocaleInfo,MSG_ZERO_FOR_UNKNOWN_NAMES_GAD),
			TAG_DONE),
		TAG_DONE), // END [ERRORMSG GROUP]
// [DRAGGABLEEDGE GROUP]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation,  LAYOUT_ORIENT_VERT,
			LAYOUT_BevelStyle,   BVS_GROUP,
			LAYOUT_InnerSpacing, 10, // small gap/space between rows
			LAYOUT_Label,        GetString(&gLocaleInfo,MSG_DRAGGABLE_CORNER_BORDER),//"Draggable Edge"
// DRAGGABLEEDGE GROUP: [ROW 1]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_AddChild, OBJ(OID_T_QUALIFIER) = NewObject(ChooserClass, NULL, //"chooser.gadget",
					GA_ID,         OID_T_QUALIFIER,
					GA_RelVerify,  TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					CHOOSER_LabelArray, sQualifierLabels,
					CHOOSER_Selected,   wd->wd_ExtData[4],
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_QUALIFIER_LABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_T_QUALFUNC) = NewObject(ChooserClass, NULL, //"chooser.gadget",
					GA_ID,         OID_T_QUALFUNC,
					GA_RelVerify,  TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					CHOOSER_DropDown, TRUE,
					CHOOSER_Labels,   wd->wd_ExtData[3],
					CHOOSER_Selected, pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_FUNCMASK,
				TAG_DONE),
				CHILD_MinWidth, maxwidth, // to fit widest string
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_FUNCTION_LABEL),
				TAG_DONE),
			TAG_DONE), // END DRAGGABLEEDGE GROUP: [ROW 1]
// DRAGGABLEEDGE GROUP: [ROW 2]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_AddChild, OBJ(OID_T_SHOWDRAGGABLE) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
					GA_ID,        0,//OID_T_SHOWDRAGGABLE,
					GA_RelVerify, TRUE,
					GA_Selected,  pt->pt_Flags & PTF_EDITFUNC,
					//GA_HintInfo,  "",
					GA_Text,      GetString(&gLocaleInfo,MSG_SHOW_DRAGGABLE_CORNER_GAD),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_T_ONLYTEXT) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
					GA_ID,        0,//OID_T_ONLYTEXT,
					GA_RelVerify, TRUE,
					GA_Selected,  pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_TEXTONLY,
					//GA_HintInfo,  "",
					GA_Text,      GetString(&gLocaleInfo,MSG_ONLY_CELL_TEXT_GAD),
				TAG_DONE),
			TAG_DONE), // END DRAGGABLEEDGE GROUP: [ROW 2]
		TAG_DONE), // [DRAGGABLEEDGE GROUP]
// [MISC GROUP]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			LAYOUT_BevelStyle,  BVS_GROUP,
			LAYOUT_Label,       GetString(&gLocaleInfo,MSG_MISC_BORDER),
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation,  LAYOUT_ORIENT_VERT,
				LAYOUT_InnerSpacing, 10, // small gap/space between [col1+col2] and [autosave]
// MISC GROUP: [COL1][COL2]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_InnerSpacing, 20, // small gap/space between [col1] and [col2]
// MISC GROUP: [COLUMN 1]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						//LAYOUT_BevelStyle,  BVS_GROUP,
						LAYOUT_AddChild, OBJ(OID_T_SHOWZEROS) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_T_SHOWZEROS,
							GA_RelVerify, TRUE,
							GA_Selected,  pt->pt_Flags & PTF_SHOWZEROS,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_SHOW_ZEROS_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_T_SHOWFORMULAS) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_T_SHOWFORMULAS,
							GA_RelVerify, TRUE,
							GA_Selected,  pt->pt_Flags & PTF_SHOWFORMULA,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_SHOW_FORMULAS_GAD),
						TAG_DONE),
					TAG_DONE), // END SAVE GROUP: [COLUMN 1]
// MISC GROUP: [COLUMN 2]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						//LAYOUT_BevelStyle,  BVS_GROUP,
						LAYOUT_AddChild, OBJ(OID_T_ADJCELL) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_T_ADJCELL,
							GA_RelVerify, TRUE,
							GA_Selected,  pt->pt_Flags & PTF_AUTOCELLSIZE,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_AUTO_CHANGE_CELL_SIZE_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_T_CENTURY) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        0,//OID_T_CENTURY,
							GA_RelVerify, TRUE,
							GA_Selected,  pt->pt_Flags & PTF_AUTOCENTURY,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_AUTO_COMPLETE_CENTURY_GAD),
						TAG_DONE),
					TAG_DONE), // END MISC GROUP: [COLUMN 2]
				TAG_DONE), // END MISC GROUP: [COL1][COL2]
// MISC GROUP: [SHOWBLOCK]
				LAYOUT_AddChild, OBJ(OID_T_SHOWBLOCK) = NewObject(ChooserClass, NULL, //"chooser.gadget",
					GA_ID,         0,//OID_T_SHOWBLOCK,
					GA_RelVerify,  TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					CHOOSER_LabelArray, sBlockDisplayLabels,
					CHOOSER_Selected,   (pt->pt_Flags & (PTF_MARKSUM|PTF_MARKAVERAGE)) >> 4,
				TAG_DONE), // END SAVE GROUP: [SHOWBLOCK]
				CHILD_WeightedWidth, 0,
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_BLOCK_DISPLAY_LABEL),
				TAG_DONE),
			TAG_DONE), // END [COL1][COL2] + [SHOWBLOCK]
		TAG_DONE),// END [MISC GROUP]
// [BUTTONS]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
			LAYOUT_SpaceOuter,  TRUE,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_OK,//9
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_CANCEL,//11
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
		TAG_DONE),
		CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreateTablePrefsGadgets() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreateKeyboardPrefsGadgets(struct winData *wd, long width, long height)
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
// wd->wd_ExtData[0] -> keys list(browser) [REACTION] (was list gadget)
// wd->wd_ExtData[1] -> AppKey struct
// wd->wd_ExtData[2] -> keys list [NATIVE]
// wd->wd_ExtData[3] -> keys_LB struct ColumnInfo
// wd->wd_ExtData[4] -> PopUp list(browser) "normal"
// wd->wd_ExtData[5] -> PopUp_LB struct ColumnInfo (changed to globa var ra_popup_ci)
DBUG("CreateKeyboardPrefsGadgets(): width=%ld   height=%ld\n",width,height);
	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
		LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		//LAYOUT_BevelStyle,  BVS_GROUP,
		LAYOUT_Label,       GetString(&gLocaleInfo,MSG_SHORT_CUT_GAD),//"Short cut"
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			LAYOUT_SpaceOuter,  TRUE,
			LAYOUT_AddChild, OBJ(OID_K_LISTKEYS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
				GA_ID,        OID_K_LISTKEYS,
				GA_RelVerify, TRUE,
				//GA_HintInfo,  "",
				LISTBROWSER_SortColumn,   0,
				LISTBROWSER_ColumnInfo,   wd->wd_ExtData[3],
				LISTBROWSER_ColumnTitles, FALSE,
				LISTBROWSER_AutoFit,      TRUE,
				LISTBROWSER_VertSeparators, FALSE,
				LISTBROWSER_Striping,     LBS_ROWS,
				LISTBROWSER_MinVisible,   12,
				LISTBROWSER_ShowSelected, TRUE,
				LISTBROWSER_Labels,       wd->wd_ExtData[0],
			TAG_DONE),
			CHILD_MinWidth, 300, // in pixels
// [NEW][DELETE]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_K_NEW,
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_NEW_GAD),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_K_DEL) = NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_K_DEL,
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
				TAG_DONE),
			TAG_DONE), // END [NEW][DELETE]
			CHILD_WeightedHeight, 0,
		TAG_DONE),
// [RECORD][\/]
		LAYOUT_AddChild, OBJ(OID_K_KBGROUP) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			LAYOUT_SpaceOuter,  TRUE,
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, OBJ(OID_K_RECORD) = NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_K_RECORD,
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_RECORD_GAD),
				TAG_DONE),
				/*CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, OBJ(OID_K_KEYPRESS) = NewObject(StringClass, NULL, //"string.gadget", 
					GA_ID,        0,//OID_K_KEYPRESS,
					GA_RelVerify, TRUE,
					STRINGA_HookType, SHK_HOTKEY,
					//STRINGA_DisablePopup, TRUE,
				TAG_DONE),
//				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
//					LABEL_Text, GetString(&gLocaleInfo,MSG_RECORD_GAD),
//				TAG_DONE),*/
				//LAYOUT_AddChild, OBJ(OID_K_POPUPBTN) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
				LAYOUT_AddChild, OBJ(OID_K_POPUPBTN) = NewObject(ButtonClass, NULL, //"button.gadget", 
					GA_ID,        OID_K_POPUPBTN,
					GA_RelVerify, TRUE,
					//GA_Text,      "\\/",
					BUTTON_AutoButton, BAG_POPUP,
					//CHOOSER_Labels,   poplist,
					//CHOOSER_DropDown, TRUE,
					//CHOOSER_MaxLabels, 50,
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE), // END [RECORD][\/]
			CHILD_WeightedHeight, 0,
			LAYOUT_AddChild, OBJ(OID_K_KEYLABEL) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
				GA_ID,         OID_K_KEYLABEL,
				GA_RelVerify,  TRUE,
				//GA_HintInfo,   "",
				GA_Underscore, 0,
				CHOOSER_LabelArray, sKeyLabels,
			TAG_DONE),
		TAG_DONE),
		CHILD_WeightedHeight, 0,
// [BUTTONS]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
			LAYOUT_SpaceOuter,  TRUE,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_OK,//9
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_CANCEL,//11
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
		TAG_DONE),
		CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreateKeyboardPrefsGadgets() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreatePrefIconGads(REG(a0, struct winData *wd))
{
//wd->wd_ExtData[0] -> commands_with_icon list(browser) [REACTION] (was commands_with_icon gadget)
//wd->wd_ExtData[1] -> toolbar list(browser) [REACTION] (was toolbar gadget)
//wd->wd_ExtData[2] -> toolbar list [NATIVE]
//wd->wd_ExtData[3] -> separator object/gadget [NATIVE]
//wd->wd_ExtData[4] -> commands_with_icon list [NATIVE]
//wd->wd_ExtData[5] -> commands_with_icon_LB struct ColumnInfo [REACTION] (was AddLockNode(...))
//wd->wd_ExtData[6] -> NOT USED
//wd->wd_ExtData[7] -> toolbar_LB struct ColumnInfo [REACTION] (was AddListViewLock(...))
DBUG("CreatePrefIconGads()\n");

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
// [LISTICONS][LISTTOOLBAR]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_SpaceOuter,  TRUE,
// [LISTICONS]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_Label, GetString(&gLocaleInfo,MSG_COMMANDS_WITH_ICONS_GAD), // "Commands with icon"
					LAYOUT_AddChild, OBJ(OID_I_LISTICONS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
						GA_ID,        0,//OID_I_LISTICONS,
						GA_RelVerify, TRUE,
						//GA_HintInfo,  "",
						LISTBROWSER_ColumnInfo,   wd->wd_ExtData[5],
						LISTBROWSER_ColumnTitles, FALSE,
						LISTBROWSER_AutoFit,      TRUE,
						LISTBROWSER_Labels, wd->wd_ExtData[0],
						LISTBROWSER_ShowSelected,    TRUE,
						LISTBROWSER_VertSeparators,  FALSE,
						//LISTBROWSER_HorizSeparators, TRUE,
						LISTBROWSER_MinVisible,      8,
						LISTBROWSER_Spacing,         2,
					TAG_DONE),
					CHILD_MinWidth, 250, // in pixels
// [>]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation,    LAYOUT_ORIENT_HORIZ,
						//LAYOUT_SpaceOuter,     TRUE,
						LAYOUT_HorizAlignment, LALIGN_RIGHT,
						LAYOUT_AddChild, OBJ(OID_I_TO_TB) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_I_TO_TB,
							GA_RelVerify, TRUE,
							//GA_Text,      ">",
							BUTTON_AutoButton, BAG_RTARROW,
						TAG_DONE),
						CHILD_WeightedWidth, 0,
					TAG_DONE), // END [>]
					CHILD_WeightedHeight, 0,
				TAG_DONE), // END [LISTICONS]
				LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
					GA_ID,       0,
					GA_ReadOnly, TRUE,
					//SPACE_Transparent, TRUE,
				TAG_DONE),
				CHILD_WeightedWidth, 0,
// [LISTTOOLBAR]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_Label, GetString(&gLocaleInfo,MSG_TOOL_BAR_GAD), // "Toolbar"
					LAYOUT_AddChild, OBJ(OID_I_LISTTOOLBAR) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
						GA_ID,        0,//OID_I_LISTTOOLBAR,
						GA_RelVerify, TRUE,
						//GA_HintInfo,  "",
						LISTBROWSER_ColumnInfo,   wd->wd_ExtData[7],
						LISTBROWSER_ColumnTitles, FALSE,
						LISTBROWSER_AutoFit,      TRUE,
						LISTBROWSER_Labels, wd->wd_ExtData[1],
						//LISTBROWSER_Selected,        0,
						LISTBROWSER_ShowSelected,    TRUE,
						LISTBROWSER_VertSeparators,  FALSE,
						//LISTBROWSER_HorizSeparators, TRUE,
						LISTBROWSER_MinVisible,      8,
						LISTBROWSER_Spacing,         2,
					TAG_DONE),
					CHILD_MinWidth, 250, // in pixels
// [/\][\/][SEPARATOR][DELETE]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
						//LAYOUT_SpaceOuter,  TRUE,
						LAYOUT_AddChild, OBJ(OID_I_UP) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_I_UP,
							GA_RelVerify, TRUE,
							//GA_Text,      "/\\",
							BUTTON_AutoButton, BAG_UPARROW,
						TAG_DONE),
						CHILD_WeightedWidth, 0,
						LAYOUT_AddChild, OBJ(OID_I_DOWN) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_I_DOWN,
							GA_RelVerify, TRUE,
							//GA_Text,      "\\/",
							BUTTON_AutoButton, BAG_DNARROW,
						TAG_DONE),
						CHILD_WeightedWidth, 0,
						LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
							GA_ID,       0,
							GA_ReadOnly, TRUE,
							//SPACE_Transparent, TRUE,
						TAG_DONE),
						CHILD_WeightedWidth, 0,
						LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_I_SEPARATOR,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_EMPTY_FIELD_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_I_DEL) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_I_DEL,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
						TAG_DONE),
					TAG_DONE), // END [/\][\/][SEPARATOR][DELETE]
					CHILD_WeightedHeight, 0,
				TAG_DONE), // END [LISTTOOLBAR]
			TAG_DONE),// END [LISTICONS][LISTTOOLBAR]
// [BUTTONS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,  BVS_SBAR_VERT,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_TEST,//10
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_TEST_GAD),//"Test"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_CANCEL,//11
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefIconGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreatePrefCmdsGads(REG(a0, struct winData *wd))
{
//wd->wd_ExtData[0] -> command list(browser) (was list gadget)
//wd->wd_ExtData[1] -> command_LB struct ColumnInfo
//wd->wd_ExtData[5] -> AddLockNode(&pr->pr_AppCmds,..) (windows.c)
DBUG("CreatePrefCmdsGads()\n");
	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_AddChild, OBJ(OID_P_LISTCMDS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
					GA_ID,        OID_P_LISTCMDS,
					GA_RelVerify, TRUE,
					//GA_HintInfo,  "",
					LISTBROWSER_ColumnInfo,   wd->wd_ExtData[1],
					LISTBROWSER_ColumnTitles, FALSE,
					LISTBROWSER_AutoFit,      TRUE,
					LISTBROWSER_Labels, wd->wd_ExtData[0],
					LISTBROWSER_ShowSelected,    TRUE,
					LISTBROWSER_VertSeparators,  FALSE,
					//LISTBROWSER_HorizSeparators, TRUE,
					LISTBROWSER_MinVisible,      10,
					LISTBROWSER_Spacing,         2,
				TAG_DONE),
				CHILD_MinWidth, 250, // in pixels
// [NEW][COPY][DELETE]
				LAYOUT_AddChild, /*OBJ(OID_P_GROUP_BTN) =*/ NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_P_NEW,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_NEW_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_P_DEL) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_P_DEL,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_P_COPY) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_P_COPY,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_COPY_GAD),
					TAG_DONE),
				TAG_DONE), // END [NEW][COPY][DELETE]
				CHILD_WeightedHeight, 0,
			TAG_DONE),
// [BUTTONS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation,    LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,     BVS_SBAR_VERT,
				LAYOUT_SpaceOuter,     TRUE,
				LAYOUT_HorizAlignment, LALIGN_CENTER,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefCmdsGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void commandsToRALB(struct AppCmd *from_ac, struct List *to_lst)
{
	struct Node *n = NULL;
	struct Command *cmd;

	foreach(&from_ac->ac_Cmds, cmd)
	{
DBUG("\t[CMD]'%s' (0x%08lx)\n",cmd->cmd_Name,cmd);
		n = AllocListBrowserNode(1,
		                         LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,cmd->cmd_Name,
		                         LBNA_UserData, cmd,
		                        TAG_DONE);
		if(n)
		{
			AddTail(to_lst, n);
//DBUG("\tnode=0x%08lx\n",n);
		}
	}
}

void ASM
CreateDefineCmdGads(REG(a0, struct winData *wd))
{
	struct AppCmd *ac = wd->u.definecmd.wd_AppCmd;
	struct Image *im = NULL;
	int16 w = scr->Font->ta_YSize<<1; // icon-image button width*2 of system's font
//wd->wd_ExtData[5] -> PopUp list(browser) "internal"
//wd->wd_ExtData[6] -> commands list(browser) [REACTION]
//wd->wd_ExtData[7] -> outputs chooserlist [REACTION]
DBUG("CreateDefineCmdGads() ac=0x%08lx (0x%08lx)\n",ac,wd->wd_Data);

	if(wd->wd_Data) // == 0 -> NEW button/command
	{
		commandsToRALB(ac, wd->wd_ExtData[6]);
		im = ((struct AppCmd *)wd->wd_Data)->ac_Node.in_Image;
DBUG("\tim=0x%08lx\n",im);
		/*if(im)
		{// "resizing" icon/image button
			w = im->Width + 15;
			h = im->Height + 15;
		}*/
	}
DBUG("\tw=h=%ld\n",w);

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
// [GROUP01]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, OBJ(OID_D_LABEL) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        0,//OID_D_LABEL,
					GA_RelVerify, TRUE,
					//GA_TabCycle,  TRUE,
					//GA_HintInfo,  "",
					STRINGA_Buffer, ac->ac_Node.in_Name,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_NAME_ULABEL),
				TAG_DONE),
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation,   LAYOUT_ORIENT_HORIZ,
					//LAYOUT_BevelStyle,    BVS_GROUP,
					//LAYOUT_SpaceOuter,    TRUE,
					LAYOUT_VertAlignment, LALIGN_CENTER,
					LAYOUT_AddChild, OBJ(OID_D_ICONIMG) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        0,//OID_D_ICONIMG
						GA_RelVerify, TRUE,
						GA_ReadOnly,  TRUE,
						//GA_Disabled,  im? FALSE : TRUE,
						BUTTON_RenderImage, im,//((struct AppCmd *)wd->wd_Data)->ac_Node.in_Image,
					TAG_DONE),
					CHILD_MinWidth,  w, // To show icon-image..
					CHILD_MinHeight, w, // ..button with a nice..
					CHILD_MaxWidth,  w, // ..border around.
					CHILD_MaxHeight, w, //
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_ICON_LABEL),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_D_ICONSEL) = NewObject(GetFileClass, NULL, //"getfile.gadget",
						GA_ID,        OID_D_ICONSEL,
						GA_RelVerify, TRUE,
						GETFILE_TitleText, GetString(&gLocaleInfo,MSG_LOAD_ICON_TITLE),
						GETFILE_Drawer,    iconpath,
						GETFILE_File,      FilePart(ac->ac_ImageName),
						//GETFILE_FullFile,  ac->ac_ImageName,
						//GETFILE_FullFileExpand, TRUE,
					TAG_DONE),
				TAG_DONE),
			TAG_DONE), // END [GROUP01]
			CHILD_WeightedHeight, 0,
// [GROUP02]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_Label, GetString(&gLocaleInfo,MSG_COMMANDS_BORDER), // "Toolbar"
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, OBJ(OID_D_LISTCMDS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
						GA_ID,        OID_D_LISTCMDS,
						GA_RelVerify, TRUE,
						//GA_HintInfo,  "",
						//LISTBROWSER_Striping,     LBS_ROWS,
						LISTBROWSER_MinVisible,   5,
						LISTBROWSER_ShowSelected, TRUE,
						LISTBROWSER_Labels,       wd->wd_ExtData[6],
						//LISTBROWSER_HorizontalProp, TRUE,
					TAG_DONE),
					//CHILD_MinWidth, 200, // in pixels
// [NEW][DELETE]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
						//LAYOUT_SpaceOuter,  TRUE,
						LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_D_NEW,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_NEW_UGAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_D_DEL) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_D_DEL,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_UGAD),
						TAG_DONE),
					TAG_DONE), // END [NEW][DELETE]
					CHILD_WeightedHeight, 0,
				TAG_DONE),
				CHILD_WeightedWidth, 35,
				LAYOUT_AddChild, OBJ(OID_D_GROUP_CMD) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					//LAYOUT_SpaceOuter,  TRUE,
// [COMMAND][\/]
					LAYOUT_AddChild, OBJ(OID_M_GROUP_I) = NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
						//LAYOUT_SpaceOuter,  TRUE,
						LAYOUT_AddChild, OBJ(OID_D_COMMAND) = NewObject(StringClass, NULL, //"chooser.gadget", 
							GA_ID,         OID_D_COMMAND,
							GA_RelVerify,  TRUE,
							//GA_HintInfo,   "",
							//GA_Underscore, 0,
							//STRINGA_TextVal, NULL,
							//STRINGA_MaxChars, 256,
						TAG_DONE),
						//CHILD_WeightedWidth, 0,
						LAYOUT_AddChild, OBJ(OID_D_POPUPBNT) = NewObject(ButtonClass, NULL, //"button.gadget", 
							GA_ID,        OID_D_POPUPBNT,
							GA_RelVerify, TRUE,
							//GA_Text,      "\\/",
							BUTTON_AutoButton, BAG_POPUP,//BAG_POPFILE,
						TAG_DONE),
						CHILD_WeightedWidth, 0,
					TAG_DONE), // END [COMMAND][\/]
					CHILD_WeightedHeight, 0,
					LAYOUT_AddChild, OBJ(OID_D_CMDTYPE) = NewObject(RadiobuttonClass, NULL, //"radiobutton.gadget",
						GA_ID,        OID_D_CMDTYPE,
						GA_RelVerify, TRUE,
						GA_Text,      sCommandLabels,
						RADIOBUTTON_Selected, 0,
					TAG_DONE),
				TAG_DONE),
				CHILD_WeightedWidth, 65,
			TAG_DONE), // END [GROUP02]
			CHILD_MinWidth, 350,
// [GROUP03]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_Label, GetString(&gLocaleInfo,MSG_OUTPUT_BORDER), // "Toolbar"
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation,   LAYOUT_ORIENT_HORIZ,
					//LAYOUT_BevelStyle,    BVS_GROUP,
					//LAYOUT_SpaceOuter,    TRUE,
					LAYOUT_AddChild, OBJ(OID_D_OUTPUT) = NewObject(StringClass, NULL, //"string.gadget",
						GA_ID,        0,//OID_D_OUTPUT,
						GA_RelVerify, TRUE,
						//GA_TabCycle,  TRUE,
						//GA_HintInfo,  "",
						STRINGA_Buffer, ac->ac_Output,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_OUTPUT_ULABEL),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_D_OUTPUTSEL) = NewObject(ChooserClass, NULL, //"chooser.gadget",
						GA_ID,         OID_D_OUTPUTSEL,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						CHOOSER_DropDown, TRUE,
						CHOOSER_Labels,   wd->wd_ExtData[7],
						//CHOOSER_Selected, 0,
					TAG_DONE),
					//CHILD_WeightedWidth, 0,
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_D_HELPTEXT) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        0,//OID_D_HELPTEXT,
					GA_RelVerify, TRUE,
					//GA_TabCycle,  TRUE,
					//GA_HintInfo,  "",
					STRINGA_Buffer, ac->ac_HelpText,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_HELP_TEXT_ULABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_D_HYPERTEXT) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        0,//OID_D_HYPERTEXT,
					GA_RelVerify, TRUE,
					//GA_TabCycle,  TRUE,
					//GA_HintInfo,  "",
					STRINGA_Buffer, ac->ac_Guide,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_HYPER_TEXT_ULABEL),
				TAG_DONE),
			TAG_DONE), // END [GROUP03]
			CHILD_WeightedHeight, 0,
// [BUTTONS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_BevelStyle,  BVS_SBAR_VERT,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_CANCEL,//11
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreateDefineCmdGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreatePrefNamesGads(REG(a0, struct winData *wd))
{
	struct Mappe *mp = GetRealMap(wd->wd_Data);

	wd->wd_ExtData[1] = NULL;
//wd->wd_ExtData[0] -> list(browser) [REACTION] (was list gadget)
//wd->wd_ExtData[2] -> list [NATIVE])

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
		LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		//LAYOUT_BevelStyle,  BVS_GROUP,
// [COL1][COL2]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
			//LAYOUT_BevelStyle,  BVS_GROUP,
			LAYOUT_InnerSpacing, 10, // small gap/space between columns
// [COLUMN 1]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_AddChild, OBJ(OID_N_LISTNAMES) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
					GA_ID,        OID_N_LISTNAMES,
					GA_RelVerify, TRUE,
					//GA_HintInfo,  "",
					//LISTBROWSER_HorizontalProp, TRUE,
					//LISTBROWSER_Striping,       LBS_ROWS,
					LISTBROWSER_ShowSelected,   TRUE,
					LISTBROWSER_Labels, wd->wd_ExtData[0],
				TAG_DONE),
				CHILD_MinHeight, 120, // in pixels
				LAYOUT_AddChild, OBJ(OID_N_LNBUTTONS) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_N_NEW,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_NEW_UGAD),//"New"
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_N_COPY) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_N_COPY,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_COPY_UGAD),//"Copy"
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_N_DELETE) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_N_DELETE,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_UGAD),//"Delete"
					TAG_DONE),
				TAG_DONE), // END OID_N_LNBUTTONS: [New][Copy][Delete]
				CHILD_WeightedHeight, 0,
			TAG_DONE), // END [COLUMN 1]]
// [COLUMN 2]
			LAYOUT_AddChild, OBJ(OID_N_COL2GROUP) = NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_AddChild, OBJ(OID_N_NAME) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_N_NAME,
					GA_RelVerify, TRUE,
					GA_TabCycle,  TRUE,
					//GA_HintInfo,  "",
					//STRINGA_Buffer, "test",
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_NAME_ULABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_N_CONTENTS) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_N_CONTENTS,
					GA_RelVerify, TRUE,
					GA_TabCycle,  TRUE,
					//GA_HintInfo,  "",
					//STRINGA_Buffer, "test",
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_CONTENTS_ULABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_N_TYPE) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
					GA_ID,         OID_N_TYPE,
					GA_RelVerify,  TRUE,
					GA_TabCycle,   TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					CHOOSER_LabelArray, sNameTypeLabels,
					//CHOOSER_Selected,   0,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_TYPE_ULABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_N_REFERENCE) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
					GA_ID,         OID_N_REFERENCE,
					GA_RelVerify,  TRUE,
					GA_TabCycle,   TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					//GA_ReadOnly,      TRUE,//
					CHOOSER_DropDown, TRUE,
					CHOOSER_Title,    mp? mp->mp_actPage->pg_Node.ln_Name : "-",
					//CHOOSER_Selected,   0,
				TAG_DONE),
				CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
					LABEL_Text, GetString(&gLocaleInfo,MSG_REFERENCE_ULABEL),
				TAG_DONE),
			TAG_DONE), // END [COLUMN 2]
			CHILD_WeightedHeight, 0,
		TAG_DONE), // END [COL1][COL2]
// [BUTTONS]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
			LAYOUT_BevelStyle,  BVS_SBAR_VERT,
			LAYOUT_SpaceOuter,  TRUE,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_OK,//9
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_CANCEL,//11
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
		TAG_DONE),
		CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefNamesGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void prefformatsToRALB(struct MinList *from_lst, struct List *to_lst)
{
	struct FormatVorlage *fv = NULL;
	struct Node *n = NULL;
	UWORD *ra_pens = dri->dri_Pens; // to "obtain" HIGHLIGHTTEXTPEN
DBUG("prefformatsToRALB()\n");
DBUG("\tra_pens=0x%08lx -> HIGHLIGHTTEXTPEN=0x%08lx\n",ra_pens,ra_pens[HIGHLIGHTTEXTPEN]);
	// Clear list(browser)
	FreeListBrowserList(to_lst);

	foreach(from_lst, fv)
	{
		if( (n=AllocListBrowserNode(2,
		                            LBNA_Flags,LBFLG_CUSTOMPENS,
		                            LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,fv->fv_Node.ln_Name,
		                                           LBNCA_FGPen,ra_pens[HIGHLIGHTTEXTPEN],
		                            LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,fv->fv_Preview,
		                            LBNA_UserData, fv, // format list [NATIVE] item
		                           TAG_DONE)) )
		{
			AddTail(to_lst, n);
DBUG("\tn=0x%08lx (0x%08lx)\n",n,fv);
		}
	}
}

void ASM
CreateFormatPrefsGadgets(REG(a0, struct winData *wd))
{
	UWORD *ra_pens = dri->dri_Pens; // to "obtain" TEXTPEN
//wd->wd_ExtData[0] -> SLIDER_LevelHook [REACTION] (was defined_format gadget [NATIVE])
//wd->wd_ExtData[1] -> color pen chooserlist [REACTION] (was decimal_places gadget [NATIVE])
//wd->wd_ExtData[2] -> DATE chooserlist [REACTION] (was DATE list [NATIVE])
//wd->wd_ExtData[3] -> TIME chooserlist [REACTION] (was TIME list [NATIVE])
//wd->wd_ExtData[4] -> NOT USED (was struct FormatVorlage [NATIVE])
//wd->wd_ExtData[5] -> format list [NATIVE]
//wd->wd_ExtData[6] -> format_LB struct ColumnInfo [REACTION]
//wd->wd_ExtData[7] -> format list(browser) [REACTION]
DBUG("CreateFormatPrefsGadgets()\n");
	prefformatsToRALB(wd->wd_ExtData[5], wd->wd_ExtData[7]);

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
// [LISTFORMATS][FORMAT SETTINGS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_SpaceOuter,  TRUE,
// [LISTFORMATS]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_Label, GetString(&gLocaleInfo,MSG_DEFINED_FORMATS_GAD), // "Defined Formats"
					LAYOUT_AddChild, OBJ(OID_F_LISTFORMATS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
						GA_ID,        OID_F_LISTFORMATS,
						GA_RelVerify, TRUE,
						//GA_HintInfo,  "",
						LISTBROWSER_ColumnInfo,   wd->wd_ExtData[6],
						LISTBROWSER_ColumnTitles, FALSE,
						LISTBROWSER_AutoFit,      TRUE,
						LISTBROWSER_Labels, wd->wd_ExtData[7],
						//LISTBROWSER_Selected,        0,
						LISTBROWSER_ShowSelected,    TRUE,
						LISTBROWSER_MinVisible,      16,
						//LISTBROWSER_Spacing,         2,
						LISTBROWSER_HorizontalProp,  TRUE,
					TAG_DONE),
					CHILD_MinWidth, 250, // in pixels
// [NEW][COPY][DEL]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
						//LAYOUT_SpaceOuter,  TRUE,
						LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_F_NEW,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_NEW_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_F_COPY) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_F_COPY,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_COPY_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_F_DEL) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_F_DEL,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
						TAG_DONE),
					TAG_DONE), // END [NEW][COPY][DEL]
					CHILD_WeightedHeight, 0,
				TAG_DONE), // END [LISTFORMATS]
// [FORMAT SETTINGS]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_Label, " ", // to "align" with list(browser)
// [PROPERTIES]
					LAYOUT_AddChild, OBJ(OID_F_GROUP_PROP) = NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						LAYOUT_BevelStyle,  BVS_GROUP,
						LAYOUT_Label, GetString(&gLocaleInfo,MSG_PROPERTIES_BORDER), // "Properties"
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							//LAYOUT_BevelStyle,  BVS_GROUP,
							LAYOUT_AddChild, OBJ(OID_F_TYPE) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
								GA_ID,         OID_F_TYPE,
								GA_RelVerify,  TRUE,
								//GA_HintInfo,   "",
								GA_Underscore, 0,
								CHOOSER_LabelArray, &sFormatTypeLabels,
								CHOOSER_Selected,   0,
							TAG_DONE),
							CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
								LABEL_Text, GetString(&gLocaleInfo,MSG_TYPE_LABEL),
							TAG_DONE),
						TAG_DONE),
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							//LAYOUT_BevelStyle,  BVS_GROUP,
							LAYOUT_AddChild, OBJ(OID_F_PRIORITY) = NewObject(SliderClass, NULL, //"slider.gadget",
								GA_ID,        OID_F_PRIORITY,
								GA_RelVerify, TRUE,
								//GA_HintInfo,  "",
								SLIDER_Min,          -128,
								SLIDER_Max,          127,
								//SLIDER_Ticks,        17,
								//SLIDER_Level,        0,
								SLIDER_Orientation,  SORIENT_HORIZ,
								SLIDER_LevelFormat,  "%ld",
								SLIDER_LevelPlace,   PLACETEXT_IN,
								SLIDER_LevelJustify, SLJ_CENTER,
								SLIDER_LevelMaxLen,  4,
							TAG_DONE),
							CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
								LABEL_Text, GetString(&gLocaleInfo,MSG_PRIORITY_SLIDER_GAD2),
							TAG_DONE),
						TAG_DONE),
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							//LAYOUT_BevelStyle,  BVS_GROUP,
							LAYOUT_AddChild, OBJ(OID_F_FORMAT) = NewObject(StringClass, NULL, //"string.gadget",
								GA_ID,        OID_F_FORMAT,
									GA_RelVerify, TRUE,
									//GA_HintInfo,  "",
									//STRINGA_TextVal, "",
								TAG_DONE),
							CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
							LABEL_Text, GetString(&gLocaleInfo,MSG_FORMAT_LABEL),
							TAG_DONE),
							LAYOUT_AddChild, OBJ(OID_F_FMT_POPBTN) = NewObject(ChooserClass, NULL, //"chooser.gadget",
								GA_ID,         OID_F_FMT_POPBTN,
								GA_RelVerify,  TRUE,
								//GA_HintInfo,   "",
								GA_Underscore, 0,
								//CHOOSER_Title,    "",
								CHOOSER_DropDown, TRUE,
								//CHOOSER_Labels, NULL, // DATE/TIME chooserlist (wd->wd_ExtData[2|3])
								//CHOOSER_Selected, 0,
							TAG_DONE),
						TAG_DONE),
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							//LAYOUT_BevelStyle,  BVS_GROUP,
							LAYOUT_AddChild, OBJ(OID_F_DECIMALS) = NewObject(SliderClass, NULL, //"slider.gadget",
								GA_ID,        OID_F_DECIMALS,
								GA_RelVerify, TRUE,
								//GA_HintInfo,  "",
								SLIDER_Min,          -1,
								SLIDER_Max,          10,
								//SLIDER_Level,        0,
								SLIDER_Ticks,        5,
								SLIDER_Orientation,  SORIENT_HORIZ,
								SLIDER_LevelFormat,  "%s",
								SLIDER_LevelPlace,   PLACETEXT_LEFT,
								SLIDER_LevelJustify, SLJ_LEFT,
								SLIDER_LevelMaxLen,  Strlen(GetString(&gLocaleInfo,MSG_AUTO_VALUE_GAD)),
								SLIDER_LevelHook,    wd->wd_ExtData[0],
							TAG_DONE),
							CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
								LABEL_Text, GetString(&gLocaleInfo,MSG_DECIMAL_PLACES_LABEL),
							TAG_DONE),
						TAG_DONE),
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							//LAYOUT_BevelStyle,  BVS_GROUP,
							LAYOUT_AddChild, OBJ(OID_F_ALIGN) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
								GA_ID,         OID_F_ALIGN,
								GA_RelVerify,  TRUE,
								//GA_HintInfo,   "",
								GA_Underscore, 0,
								CHOOSER_LabelArray, &sAlignLabels,
								CHOOSER_Selected,   0,
							TAG_DONE),
							CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
								LABEL_Text, GetString(&gLocaleInfo,MSG_ALIGNMENT_LABEL),
							TAG_DONE),
						TAG_DONE),
					TAG_DONE), // END [PROPERTIES]
// [OPTIONS]
					LAYOUT_AddChild, OBJ(OID_F_GROUP_OPTS) = NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						LAYOUT_BevelStyle,  BVS_GROUP,
						LAYOUT_Label, GetString(&gLocaleInfo,MSG_MARKED_BORDER), // "Options"
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
							LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
							//LAYOUT_HorizAlignment, LALIGN_LEFT,
							//LAYOUT_BevelStyle,  BVS_GROUP,
							LAYOUT_AddChild, OBJ(OID_F_NEGCOLOR) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
								GA_ID,        OID_F_NEGCOLOR,
								GA_RelVerify, TRUE,
								//GA_Selected,  FALSE,
								//GA_Disabled,  TRUE,
								//GA_HintInfo,  "",
								GA_Text,      GetString(&gLocaleInfo,MSG_NEG_VALUES_LABEL),
							TAG_DONE),
							LAYOUT_AddChild, OBJ(OID_F_NEGCOL_SAMPLE) = NewObject(ButtonClass, NULL, //"button.class
								GA_ID,        0,//OID_F_NEGCOL_SAMPLE
								GA_RelVerify, TRUE,
								GA_ReadOnly,  TRUE,
								BUTTON_DomainString,  "WWW",
								BUTTON_BackgroundPen, ra_pens[TEXTPEN],
							TAG_DONE),
							LAYOUT_AddChild, OBJ(OID_F_NEGCOL_POPBTN) = NewObject(ButtonClass, NULL, //"button.gadget",
								GA_ID,        OID_F_NEGCOL_POPBTN,
								GA_RelVerify, TRUE,
								BUTTON_AutoButton, BAG_POPUP,
							TAG_DONE),
							CHILD_WeightedWidth, 0,
						TAG_DONE),
						CHILD_WeightedWidth, 0,
						LAYOUT_AddChild, OBJ(OID_F_NEGVAL) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        OID_F_NEGVAL,
							GA_RelVerify, TRUE,
							//GA_Selected,  FALSE,
							//GA_Disabled,  TRUE,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_NEG_VALUES_GAD),
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_F_NEGSEPARATOR) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
							GA_ID,        OID_F_NEGSEPARATOR,
							GA_RelVerify, TRUE,
							//GA_Selected,  FALSE,
							//GA_Disabled,  TRUE,
							//GA_HintInfo,  "",
							GA_Text,      GetString(&gLocaleInfo,MSG_THOUSANDS_SEPARATOR_GAD),
						TAG_DONE),
					TAG_DONE), // END [OPTIONS]
				TAG_DONE), // END [FORMAT SETTINGS]
				CHILD_WeightedHeight, 0,
			TAG_DONE), // END [LISTFORMATS][FORMAT SETTINGS]
// [BUTTONS]
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,  BVS_SBAR_VERT,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_CANCEL,//11
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreateFormatPrefsGadgets() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}


void ASM
CreatePrefColorsGads(REG(a0, struct winData *wd))
{
	struct ColorWheelRGB rgb;
	struct ColorWheelHSB hsb;
	//short *pens, numPens = 0;
	//wd->wd_ExtData[0] -> colors list(browser) [REACTION] (was gradientslider gadget [NATIVE])
	//wd->wd_ExtData[1] -> colors_LB struct ColumnInfo [REACTION] (was colorwheel gadget [NATIVE])
	//wd->wd_ExtData[2] -> scrcolors list(browser) [REACTION] (was R slider [NATIVE])
	//wd->wd_ExtData[3] -> NOT USED (was G slider [NATIVE])
	//wd->wd_ExtData[4] -> NOT USED (was B slider [NATIVE])
	//wd->wd_ExtData[5] -> scrcolors [NATIVE];
	//wd->wd_ExtData[6] -> pens [NATIVE]
	//wd->wd_ExtData[7] -> numPens [NATIVE]
	//wd->wd_Data -> colors list [NATIVE]

	/*if( (pens=wd->wd_ExtData[6]) != 0 )
	{
		pens[MAXGRADPENS+1] = ObtainPen(scr->ViewPort.ColorMap, -1, 0, 0, 0, PEN_EXCLUSIVE|PEN_NO_SETCOLOR);

		GetRGB32(scr->ViewPort.ColorMap, 0, 1, (ULONG *)&rgb);
		ConvertRGBToHSB(&rgb, &hsb);
		while(numPens < MAXGRADPENS)
		{
			hsb.cw_Brightness = 0xffffffff - ((0xffffffff/MAXGRADPENS)*numPens);
			ConvertHSBToRGB(&hsb, &rgb);
			if( (pens[numPens]=ObtainPen(scr->ViewPort.ColorMap, -1, rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue, PEN_EXCLUSIVE)) == -1 )
			{
				break;
			}
			numPens++;
		}
		pens[numPens] = ~0;
		wd->wd_ExtData[7] = (APTR)((long)numPens);
		if(!numPens)
		{
			pens[0] = FindColor(scr->ViewPort.ColorMap, 0xffffffff, 0xffffffff, 0xffffffff, 7);
			pens[1] = FindColor(scr->ViewPort.ColorMap, 0x88888888, 0x88888888, 0x88888888, 7);
			pens[2] = FindColor(scr->ViewPort.ColorMap, 0x00000000, 0x00000000, 0x00000000, 7);
			pens[3] = ~0;
		}
	}*/


	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
// [COLORS][PALETTE]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_BevelStyle,  BVS_GROUP,
// [COLOR GROUP]
				LAYOUT_AddChild, OBJ(OID_P_COLGROUP) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_Label,       GetString(&gLocaleInfo,MSG_COLOR_BORDER),//"Color"
					//LAYOUT_VertAlignment, LALIGN_CENTER,
					LAYOUT_InnerSpacing, 10,
					LAYOUT_AddChild, OBJ(OID_P_COLNAME) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        0,//OID_P_COLORNAME,
						GA_RelVerify, TRUE,
						GA_ReadOnly,  TRUE,
						//GA_Text,      cp->cp_Node.ln_Name,
						BUTTON_BevelStyle,  BVS_NONE,
						BUTTON_Transparent, TRUE,
					TAG_DONE),
					CHILD_WeightedHeight, 0,
					LAYOUT_AddChild, OBJ(OID_P_COLOR) = NewObject(GetColorClass, NULL, //"getcolor.gadget",
						GA_ID,        OID_P_COLOR,
						GA_RelVerify, TRUE,
						GETCOLOR_TitleText,  GetString(&gLocaleInfo,MSG_COLOR_BORDER),
						GETCOLOR_Screen,     scr,
						GETCOLOR_ColorWheel, TRUE,
						GETCOLOR_RGBSliders, TRUE,
						//GETCOLOR_ShowRGB,    TRUE,
					TAG_DONE),
					//CHILD_MaxHeight, 50,
					LAYOUT_AddChild, OBJ(OID_P_COLRGB) = NewObject(ButtonClass, NULL, //"string.gadget",
						GA_ID,        0,//OID_P_COLRGB,
						GA_RelVerify, TRUE,
						GA_ReadOnly,  TRUE,
						//GA_HintInfo,  "",
						BUTTON_BevelStyle,   BVS_NONE,
						BUTTON_Transparent,  TRUE,
						BUTTON_DomainString, "WWW: 888 / 888 / 888", // widest string
					TAG_DONE),
					CHILD_WeightedHeight, 0,
					LAYOUT_AddChild, OBJ(OID_P_COLHSB) = NewObject(ButtonClass, NULL, //"string.gadget",
						GA_ID,        0,//OID_P_COLHSB,
						GA_RelVerify, TRUE,
						GA_ReadOnly,  TRUE,
						//GA_HintInfo,  "",
						BUTTON_BevelStyle,   BVS_NONE,
						BUTTON_Transparent,  TRUE,
						BUTTON_DomainString, "WWW: 888 / 888 / 888", // widest string
					TAG_DONE),
					CHILD_WeightedHeight, 0,
				TAG_DONE), // END [COLOR GROUP]
				CHILD_WeightedWidth, 0,
// [PALETTE GROUP]
				LAYOUT_AddChild, OBJ(OID_P_PALGROUP) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					LAYOUT_BevelStyle,  BVS_GROUP,
					LAYOUT_Label,       GetString(&gLocaleInfo,MSG_PALETTE_BORDER),//"Palette"
					LAYOUT_AddChild, OBJ(OID_P_PALETTE) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
						GA_ID,         OID_P_PALETTE,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						CHOOSER_LabelArray, sColorViewLabels,
						CHOOSER_Selected,   0,
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_P_LISTCOLORS) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
						GA_ID,        OID_P_LISTCOLORS,
						GA_RelVerify, TRUE,
						//GA_HintInfo,  "",
						LISTBROWSER_AutoFit,        TRUE,
						LISTBROWSER_ColumnInfo,     wd->wd_ExtData[1],
						LISTBROWSER_ColumnTitles,   FALSE,
						LISTBROWSER_VertSeparators, FALSE,
						//LISTBROWSER_Striping,     LBS_ROWS,
						LISTBROWSER_Spacing,      2,
						LISTBROWSER_MinVisible,   7,
						LISTBROWSER_ShowSelected, TRUE,
						LISTBROWSER_Labels,       wd->wd_ExtData[0],
					TAG_DONE),
					//CHILD_MinHeight, 120, // in pixels
// PALETTE GROUP: [NEW][COPY][DELETE]
					LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
						//LAYOUT_BevelStyle,  BVS_GROUP,
						LAYOUT_AddChild, OBJ(OID_P_PAL_NEW) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_P_PAL_NEW,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_NEW_UGAD),//"New"
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_P_PAL_COPY) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_P_PAL_COPY,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_COPY_UGAD),//"Copy"
						TAG_DONE),
						LAYOUT_AddChild, OBJ(OID_P_PAL_DEL) = NewObject(ButtonClass, NULL, //"button.gadget",
							GA_ID,        OID_P_PAL_DEL,
							GA_RelVerify, TRUE,
							GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_UGAD),//"Delete"
						TAG_DONE),
					TAG_DONE), // END PALETTE GROUP: [NEW][COPY][DELETE]
					CHILD_WeightedHeight, 0,
					LAYOUT_AddChild, OBJ(OID_P_PAL_NAME) = NewObject(StringClass, NULL, //"string.gadget",
						GA_ID,        OID_P_PAL_NAME,
						GA_RelVerify, TRUE,
						//GA_HintInfo,  "",
						//STRINGA_TextVal, cp->cp_Node.ln_Name,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_NAME_ULABEL),
					TAG_DONE),
				TAG_DONE), // END [PALETTE GROUP]
				CHILD_MinWidth, 200,
			TAG_DONE), // END [COLORS][PALETTE]
// [BUTTONS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_CANCEL,//11
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefColorsGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
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


void ASM
CreateSystemPrefsGadgets(REG(a0, struct winData *wd))
{
	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
		LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		//LAYOUT_BevelStyle,  BVS_GROUP,
		LAYOUT_AddChild, NewObject(ClickTabClass, NULL, //"clicktab.gadget",
			//GA_ID,        0,
			GA_RelVerify, TRUE,
			GA_Text,      sSystemPageLabels,
			CLICKTAB_PageGroup, NewObject(NULL, "page.gadget",
// [MISC GROUP/PAGE]
				PAGE_Add, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_SpaceOuter,  TRUE,
					//LAYOUT_SpaceInner,  TRUE,
					LAYOUT_AddChild, OBJ(OID_S_APPICON) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
						GA_ID,        0,//OID_S_APPICON,
						GA_RelVerify, TRUE,
						GA_Selected,  prefs.pr_Flags & PRF_APPICON,
						//GA_HintInfo,  "",
						GA_Text,      GetString(&gLocaleInfo,MSG_ICON_ON_WORKBENCH_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_S_SESSIONS) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
						GA_ID,        0,//OID_S_SESSIONS,
						GA_RelVerify, TRUE,
						GA_Selected,  prefs.pr_Flags & PRF_USESESSION,
						//GA_HintInfo,  "",
						GA_Text,      GetString(&gLocaleInfo,MSG_USE_SESSIONS_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_S_CHECKSEC) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
						GA_ID,        0,//OID_S_CHECKSEC,
						GA_RelVerify, TRUE,
						GA_Selected,  prefs.pr_Flags & PRF_SECURESCRIPTS,
						//GA_HintInfo,  "",
						GA_Text,      GetString(&gLocaleInfo,MSG_CHECK_SECURITY_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_S_CTXTMENU) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
						GA_ID,        0,//OID_S_CTXTMENU,
						GA_RelVerify, TRUE,
						GA_Selected,  prefs.pr_Flags & PRF_CONTEXTMENU,
						//GA_HintInfo,  "",
						GA_Text,      GetString(&gLocaleInfo,MSG_CONTEXT_MENUS_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_S_REFRESH) = NewObject(ChooserClass, NULL, //"chooser.gadget",
						GA_ID,         0,//OID_S_REFRESH,
						GA_RelVerify,  TRUE,
						//GA_HintInfo,   "",
						GA_Underscore, 0,
						GA_Disabled,   gIsBeginner,
						CHOOSER_LabelArray, sWindowRefreshLabels,
						CHOOSER_Selected,   prefs.pr_Flags & PRF_SIMPLEWINDOWS? (prefs.pr_Flags & PRF_SIMPLEPROJS? 2 : 1) : 0,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_WINDOW_REFRESH_GAD),
					TAG_DONE),
				TAG_DONE), // END [MISC GROUP/PAGE]
// [CLIPBOARD GROUP/PAGE]
				PAGE_Add, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation,   LAYOUT_ORIENT_VERT,
					//LAYOUT_SpaceOuter,    TRUE,
					//LAYOUT_SpaceInner,    TRUE,
					LAYOUT_VertAlignment, LALIGN_CENTER,
					LAYOUT_AddChild, OBJ(OID_S_CLIMBLST) = NewObject(CheckBoxClass, NULL, //"checkbox.gadget",
						GA_ID,        0,//OID_S_CLIMBLST,
						GA_RelVerify, TRUE,
						GA_Selected,  prefs.pr_Flags & PRF_USESESSION,
						//GA_HintInfo,  "",
						GA_Text,      GetString(&gLocaleInfo,MSG_CLIMB_LIST_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_S_CLIPUNIT) = NewObject(IntegerClass, NULL, //"integer.gadget",
						GA_ID,        0,//OID_S_CLIPUNIT,
						GA_RelVerify, TRUE,
						//GA_HintInfo,  "",
						INTEGER_Number,   clipunit,
						INTEGER_Minimum,  0,
						INTEGER_Maximum,  255,
						//INTEGER_MaxChars, 4,
					TAG_DONE),
					CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
						LABEL_Text, GetString(&gLocaleInfo,MSG_CLIPBOARD_UNIT_LABEL),
					TAG_DONE),
				TAG_DONE), // END [CLIPBOARD GROUP/PAGE]
			TAG_DONE), // END PAGES
		TAG_DONE),
// [BUTTONS]
		LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
			LAYOUT_SpaceOuter,  TRUE,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_OK,//9
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
			LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
				GA_ID,        OID_CANCEL,//11
				GA_RelVerify, TRUE,
				GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
			TAG_DONE),
			CHILD_WeightedWidth, 0,
		TAG_DONE),
		CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreateSystemPrefsGadgets() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
}
//#warning: "psysPages dummy"
//struct Gadget *psysPages[3];


void
contextmenuToRALB(struct MinList *min_l, struct List* lst) // RALB = ReAction ListBrowser
{
	struct Node *n = NULL;
	struct ContextMenu *cm;

	FreeListBrowserList(lst);
	foreach(min_l, cm)
	{
		if( (n=AllocListBrowserNode(1,
		                            LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,cm->cm_Node.ln_Name,
		                            LBNA_UserData, cm, // context menu list [NATIVE] item
		                           TAG_DONE)) )
		{
			AddTail(lst, n);
DBUG("contextmenuToRALB(): '%s' (0x%08lx)\n",cm->cm_Node.ln_Name,n);
		}
	}
}

void ASM
CreatePrefContextGads(REG(a0, struct winData *wd))
{
// wd->wd_ExtData[0..NUM_CMT-1=4] -> contextmenu list [NATIVE]
// wd->wd_ExtData[NUM_CMT=5] -> contextmenu list(browser) (was struct ContextMenu [NATIVE])
// wd->wd_ExtData[6] -> PopUp list(browser) "normal" (was list gadget [NATIVE])
// wd->wd_ExtData[7] -> PopUp list(browser) "internal" (was context chooser index [NATIVE])
DBUG("CreatePrefContextGads()\n");
	contextmenuToRALB( (struct MinList *)wd->wd_ExtData[0], (struct List *)wd->wd_ExtData[5] );

	OBJ(OID_TEMPORAL) = NewObject(LayoutClass, NULL, //"layout.gadget",
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			//LAYOUT_BevelStyle,  BVS_GROUP,
// [LISTCONTEXT]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				//LAYOUT_BevelStyle,  BVS_GROUP,
				LAYOUT_Label, GetString(&gLocaleInfo,MSG_CONTEXT_LABEL), // "Context"
				LAYOUT_AddChild, OBJ(OID_C_CONTEXT) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
					GA_ID,         OID_C_CONTEXT,
					GA_RelVerify,  TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					CHOOSER_LabelArray, &sContextMenuLabels,
					//CHOOSER_Selected,   0,
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_C_LISTCONTEXT) = NewObject(ListBrowserClass, NULL, //"listbrowser.gadget",
					GA_ID,        OID_C_LISTCONTEXT,
					GA_RelVerify, TRUE,
					//GA_HintInfo,  "",
					//LISTBROWSER_AutoFit,      TRUE,
					//LISTBROWSER_Striping,     LBS_ROWS,
					LISTBROWSER_MinVisible,   10,
					LISTBROWSER_ShowSelected, TRUE,
					LISTBROWSER_Labels,       wd->wd_ExtData[5],
				TAG_DONE),
				CHILD_MinWidth, 250, // in pixels
// [/\][\/][NEW][DELETE]
				LAYOUT_AddChild, OBJ(OID_C_BTNGROUP) = NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_AddChild, OBJ(OID_C_UP) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_C_UP,
						GA_RelVerify, TRUE,
						//GA_Text,      "/\\",
						BUTTON_AutoButton, BAG_UPARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, OBJ(OID_C_DOWN) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_C_DOWN,
						GA_RelVerify, TRUE,
						//GA_Text,      "\\/",
						BUTTON_AutoButton, BAG_DNARROW,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, NewObject(SpaceClass, NULL, //"space.gadget",
						GA_ID,       0,
						GA_ReadOnly, TRUE,
						//SPACE_Transparent, TRUE,
					TAG_DONE),
					CHILD_WeightedWidth, 0,
					LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_C_NEW,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_NEW_GAD),
					TAG_DONE),
					LAYOUT_AddChild, OBJ(OID_C_DEL) = NewObject(ButtonClass, NULL, //"button.gadget",
						GA_ID,        OID_C_DEL,
						GA_RelVerify, TRUE,
						GA_Text,      GetString(&gLocaleInfo,MSG_DELETE_GAD),
					TAG_DONE),
				TAG_DONE), // END [/\][\/][NEW][DELETE]
				CHILD_WeightedHeight, 0,
			TAG_DONE),
			LAYOUT_AddChild, OBJ(OID_C_TITLE) = NewObject(StringClass, NULL, //"string.gadget",
				GA_ID,        OID_C_TITLE,
				GA_RelVerify, TRUE,
			TAG_DONE),
			CHILD_Label, NewObject(LabelClass, NULL, //"label.image",
				LABEL_Text, GetString(&gLocaleInfo,MSG_TITLE_LABEL),
			TAG_DONE),
// [COMMAND][TYPE][\/]
			LAYOUT_AddChild, OBJ(OID_C_CMDGROUP) = NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				//LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, OBJ(OID_C_COMMAND) = NewObject(StringClass, NULL, //"string.gadget",
					GA_ID,        OID_C_COMMAND,
					GA_RelVerify, TRUE,
					//GA_Text,      GetString(&gLocaleInfo,MSG_TITLE_LABEL),
				TAG_DONE),
				LAYOUT_AddChild, OBJ(OID_C_TYPE) = NewObject(ChooserClass, NULL, //"chooser.gadget", 
					GA_ID,         0,//OID_C_TYPE,
					GA_RelVerify,  TRUE,
					//GA_HintInfo,   "",
					GA_Underscore, 0,
					CHOOSER_LabelArray, &gAppCommandLabels,
					//CHOOSER_Selected,   0,
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, OBJ(OID_C_POPUPBTN) = NewObject(ButtonClass, NULL, //"button.gadget", 
					GA_ID,        OID_C_POPUPBTN,
					GA_RelVerify, TRUE,
					//GA_Text,      "\\/",
					BUTTON_AutoButton, BAG_POPUP,
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE), // END [COMMAND][TYPE][\/]
			CHILD_WeightedHeight, 0,
// [BUTTONS]
			LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,  BVS_SBAR_VERT,
				LAYOUT_SpaceOuter,  TRUE,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_OK,//9
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_OK_GAD),//"OK"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
				LAYOUT_AddChild, NewObject(ButtonClass, NULL, //"button.gadget",
					GA_ID,        OID_CANCEL,//11
					GA_RelVerify, TRUE,
					GA_Text,      GetString(&gLocaleInfo,MSG_CANCEL_GAD),//"Cancel"
				TAG_DONE),
				CHILD_WeightedWidth, 0,
			TAG_DONE),
			CHILD_WeightedHeight, 0,
	TAG_DONE);
DBUG("CreatePrefContextGads() obj=0x%08lx\n", OBJ(OID_TEMPORAL));
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
