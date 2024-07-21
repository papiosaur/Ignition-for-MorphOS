/* Create gadget routines for ignition
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "classes.h"
#ifdef __amigaos4__
	#include <proto/texteditor.h>
	#include <gadgets/texteditor.h>
	#include <proto/scroller.h>
	#include <gadgets/scroller.h>
#endif


const char *gAppCommandLabels[3];
const char *gDatabaseFieldLabels[5];
//const char *gShortAppCommandLabels[3];
STRPTR wdt_cmdstr;

static const char *sFrameLabels[5];
static const char *sAlignLabels[6];
static const char *sVerticalAlignLabels[5];
static const char *sKerningLabels[4];
static const char *sSecurityLabels[6];
static const char *sCellPageLabels[5];
static const char *sOrderLabels[3];
static const char *sDiagramPageLabels[6];
static const char *sAxesGridLabels[4];
static const char *sPageLimitLabels[4];
static const char *sPageProtectLabels[4];
static const char *sCellSizeLabels[5];
//static const char *sEdgeLabels[4];
//static const char *sEndsLabels[4];


void
AddGGadget(struct MinList *list, struct Gadget *gad, ULONG tag, ULONG type, ULONG kind, UBYTE page)
{
    struct gGadget *gg;

	if (gad && (gg = AllocPooled(pool, sizeof(struct gGadget)))) {
        gad->UserData = gg;

        gg->gg_Gadget = gad;
        gg->gg_Tag = tag;
        gg->gg_Type = type;
        gg->gg_Kind = kind;
        gg->gg_Page = page;

		MyAddTail(list, gg);
    }
}


void
AddObjectGadget(struct MinList *list, struct Gadget *gad, ULONG tag, ULONG type, ULONG kind)
{
	AddGGadget(list, gad, tag, type, kind, 1);
}


CONST_STRPTR
GetGLabel(struct gInterface *gi)
{
    if (!gi)
		return NULL;

    if (gi->gi_Label)
		return gi->gi_Label;

	switch (gi->gi_Type)
    {
        case GIT_PEN:
            if (gi->gi_Tag == GOA_Pen)
				return GetString(&gLocaleInfo, MSG_COLOR_LABEL);
            else if (gi->gi_Tag == GOA_FillPen)
				return GetString(&gLocaleInfo, MSG_FILL_COLOR_LABEL);
            else if (gi->gi_Tag == GOA_OutlinePen)
				return GetString(&gLocaleInfo, MSG_BORDER_COLOR_LABEL);
            break;
        case GIT_WEIGHT:
			return (STRPTR)GetString(&gLocaleInfo, MSG_WEIGHT_LABEL);
        case GIT_CYCLE:
            if (!gi->gi_Special)
                break;
            break;
        case GIT_CHECKBOX:
            if (gi->gi_Tag == GOA_ContinualCommand)
				return GetString(&gLocaleInfo, MSG_CONTINUAL_COMMAND_GAD);
            else if (gi->gi_Tag == GOA_HasOutline)
				return GetString(&gLocaleInfo, MSG_DISPLAY_FRAME_GAD);
			break;

        case GIT_TEXT:
        case GIT_FORMULA:
            if (gi->gi_Tag == GOA_Help)
				return GetString(&gLocaleInfo, MSG_HELP_TEXT_LABEL);
            else if (gi->gi_Tag == GOA_Command)
				return (STRPTR)GetString(&gLocaleInfo, MSG_COMMAND_LABEL);
			return GetString(&gLocaleInfo, MSG_TEXT_LABEL);

        case GIT_BUTTON:
            if (gi->gi_Tag == GEA_References)
				return GetString(&gLocaleInfo, MSG_REFERENCE_OBJECT_GAD);
			break;

        case GIT_FONT:
			return GetString(&gLocaleInfo, MSG_FONT_LABEL);
    }
	return NULL;
}


void ASM
CreateInfoGadgets(REG(a0, struct winData *wd))
{
	gWidth = 296 + lborder + rborder;
	gHeight = barheight + fontheight + 240 + bborder;

    if (wd->wd_Data)
        return;

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_OK_GAD)) + 60;
	ngad.ng_TopEdge = gHeight - fontheight - 7 - bborder;
	ngad.ng_LeftEdge = (gWidth >> 1) - (ngad.ng_Width >> 1);
    ngad.ng_Flags = PLACETEXT_IN;
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
}


void ASM
CreateBorderGadgets(REG(a0, struct winData *wd))
{
    struct gpDomain gpd;

	gWidth = TLn(GetString(&gLocaleInfo, MSG_OK_GAD)) + TLn(GetString(&gLocaleInfo, MSG_FRAME_BLOCK_GAD)) + TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD)) + 104;
	gHeight = barheight + 8 * fontheight + 67 + bborder;

	//wd->wd_ShortCuts = "boarz";
	MakeShortCutString(wd->wd_ShortCuts, MSG_FRAME_WHOLE_BLOCK_UGAD, MSG_OK_UGAD, MSG_CANCEL_UGAD,
		MSG_ADOPT_FRAME_FROM_CELL_UGAD, MSG_ASSIGN_UGAD, TAG_END);

    gpd.MethodID = GM_DOMAIN;
    gpd.gpd_GInfo = NULL;
    gpd.gpd_RPort = &scr->RastPort;
    gpd.gpd_Which = GDOMAIN_NOMINAL;
    gpd.gpd_Attrs = NULL;
	if (DoClassMethodA(framesgclass, (Msg)&gpd))
		gWidth = max(gWidth, gpd.gpd_Domain.Width + 32), gHeight += gpd.gpd_Domain.Height;

	ngad.ng_TopEdge += gpd.gpd_Domain.Height + 5 + fontheight;
	ngad.ng_LeftEdge = lborder + 8;
    ngad.ng_Width = boxwidth;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADD_ONLY_GAD);
    ngad.ng_Flags = PLACETEXT_RIGHT;
    ngad.ng_GadgetID = 11;                  /* 11 */
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE, GTCB_Checked, TRUE, TAG_END);

	ngad.ng_TopEdge += fontheight + 12;
    ngad.ng_LeftEdge = lborder;
	ngad.ng_Width = gWidth - lborder - rborder;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID = 1;                   /* 1 */
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sFrameLabels, TAG_END);

    ngad.ng_LeftEdge += 8;
	ngad.ng_TopEdge += fontheight + 8;
    ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADOPT_CHANGES_GAD);
    ngad.ng_Flags = PLACETEXT_RIGHT;
    ngad.ng_GadgetID++;                     /* 2 */
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE, TAG_END);

    ngad.ng_LeftEdge += 2*boxwidth;
	ngad.ng_TopEdge += fontheight + 7;
    ngad.ng_GadgetID++;                     /* 3 */
	if ((gad = CreatePopGadget(wd, gad, FALSE)) != 0)
		gad->UserData = (APTR)FindColorPen(0, 0, 0);

	ngad.ng_LeftEdge += 14 + boxwidth + TLn(GetString(&gLocaleInfo, MSG_WEIGHT_LABEL));
	ngad.ng_Width = gWidth - 2*TLn("<") - ngad.ng_LeftEdge - 32;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_WEIGHT_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID++;                     /* 4 */
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, "1 pt", TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn("<") + 8;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetText = "<";
    ngad.ng_GadgetID++;                     /* 5 */
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GA_Immediate, TRUE, GA_RelVerify, TRUE, TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetText = ">";
    ngad.ng_GadgetID++;                     /* 6 */
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GA_Immediate, TRUE, GA_RelVerify, TRUE, TAG_END);

    ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight + 11;
	ngad.ng_Width = gWidth - lborder - rborder;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADOPT_FRAME_FROM_CELL_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[3];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
    ngad.ng_Width = boxwidth;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FRAME_WHOLE_BLOCK_UGAD);
    ngad.ng_Flags = PLACETEXT_RIGHT;
    ngad.ng_GadgetID = wd->wd_ShortCuts[0];
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE, GT_Underscore, '_', TAG_END);

    ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_OK_GAD)) + 20;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += 6 + ngad.ng_Width;
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - rborder - 26 - TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD));
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ASSIGN_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[4];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += 6 + ngad.ng_Width;
	ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[2];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	gad = NewObj(wd, WOT_GADGET, framesgclass, NULL, GA_Top,       barheight + fontheight + 5,
                                                     GA_Left,      (gWidth-gpd.gpd_Domain.Width) >> 1,
                                                     GA_Width,     gpd.gpd_Domain.Width,
                                                     GA_Height,    gpd.gpd_Domain.Height,
                                                     GA_Previous,  gad,
                                                     GA_ID,        10,
                                                     TAG_END);
}


void PUBLIC
RefreshCellPageGadgets(REG(a0, struct Window *win), REG(d0, long active))
{
    struct winData *wd = (struct winData *)win->UserData;
    struct RastPort *rp = win->RPort;
    struct Gadget *gad;
    long   i,w = lborder+rborder+16;

    switch(active)
    {
        case 0:  // Format
            if ((gad = GadgetAddress(win, 3)) != 0)
            {
                ULONG komma;

                GT_GetGadgetAttrs(gad,win,NULL,GTSL_Level,&komma,TAG_END);
                DrawPointSliderValue(rp,gad,komma);
            }
            break;
        case 1:  // Farbe & Ausrichtung
#ifdef __amigaos4__
			if(!wd)	//wd not yet set
				break;
#endif
            if ((gad = GadgetAddress(win, 6)) != 0)
            {
                ULONG apen = 0,bpen = 0;

                if (wd)
                    apen = (ULONG)wd->wd_ExtData[0], bpen = (ULONG)wd->wd_ExtData[1];
                else if (rxpage)
                    apen = rxpage->pg_APen, bpen = rxpage->pg_BPen;
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_COLORS_BORDER),8+lborder,gad->TopEdge-fontheight-2,win->Width-w,3*fontheight+18);
                DrawColorField(rp,gad,apen,TRUE);
				if((gad = GadgetAddress(win, 7)) != 0)
	                DrawColorField(rp,gad,bpen,TRUE);
            }
            if ((gad = GadgetAddress(win, 23)) != 0)
            {
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_PATTERN_BORDER),8+lborder,gad->TopEdge-fontheight-2,win->Width-w,2*fontheight+11);
                DrawPatternField(rp,gad,(ULONG)wd->wd_ExtData[7],(BYTE)((long)wd->wd_Data));
            }
            if ((gad = GadgetAddress(win, 8)) != 0)
            {
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_ALIGNMENT_BORDER),8+lborder,gad->TopEdge-fontheight-2,win->Width-w,3*fontheight+18);
            }
            break;
        case 2:  // Schrift
            if ((gad = GadgetAddress(win, 10)) != 0)
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_FONT_BORDER),8+lborder,gad->TopEdge-fontheight-2,win->Width-w,3*fontheight+18);
            if ((gad = GadgetAddress(win,14)))
            {
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_STYLE_BORDER),8+lborder,gad->TopEdge-fontheight-2,(i = boxwidth+24+TLn(GetString(&gLocaleInfo, MSG_DOUBLE_UNDERLINED_GAD))),6*fontheight+39);
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_SPECIAL_BORDER),14+lborder+i,gad->TopEdge-fontheight-2,win->Width-i-6-w,6*fontheight+39);
            }
            if ((gad = GadgetAddress(win, 20)) != 0)
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_TYPOGRAPHY_BORDER),8+lborder,gad->TopEdge-fontheight-2,win->Width-w,3*fontheight+18);
            break;
        case 3:  // Diverses
            if ((gad = GadgetAddress(win, 25)) != 0)
                DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_CELL_SECURITY_BORDER),8+lborder,gad->TopEdge-fontheight-2,win->Width-w,2*fontheight+11);
            break;
    }
}


struct Gadget *cellPages[5];


void ASM
CreateCellGadgets(REG(a0, struct winData *wd))
{
    struct Gadget *pgad;
	char buffer[64];
	long i, w, h, n;
					  
	sprintf(buffer, "%s %s", GetString(&gLocaleInfo, MSG_DECIMAL_PLACES_LABEL), GetString(&gLocaleInfo, MSG_AUTO_VALUE_GAD));
	gWidth = TLn(buffer) + TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD)) + 150;
    if (gWidth < (w = TLn(GetString(&gLocaleInfo, MSG_ADOPT_FONT_STYLE_FROM_CELL_GAD))))
        gWidth = w;
#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_ASSIGN_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 33; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 34; //Set Gadgetnumber 
	wd->wd_ShortCutsID[2] = 35; //Set Gadgetnumber 
#endif
#ifdef __amigaos4__
	gWidth += 16 + rborder + lborder + 75;
#else
	gWidth += 16 + rborder + lborder;
#endif
	gHeight = barheight + fontheight * 15 + 114 + bborder;  h = gHeight - barheight - 2 * fontheight - 23 - bborder;
    cellPages[4] = NULL;

    /******* Format *******/

    pgad = CreateContext(&cellPages[0]);

    i = (h-29-3*fontheight)/fontheight;
    ngad.ng_LeftEdge = 8+lborder;
    ngad.ng_TopEdge = barheight+2*fontheight+16;
    ngad.ng_Width = gWidth-16-lborder-rborder;
    ngad.ng_Height = fontheight*i+4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DEFINED_FORMATS_GAD);
    ngad.ng_Flags = PLACETEXT_ABOVE;        // 1
    pgad = CreateGadget(LISTVIEW_KIND,pgad,&ngad,GTLV_Labels,&rxpage->pg_Mappe->mp_Formats,GTLV_ShowSelected,NULL,GTLV_CallBack,&linkHook,GTLV_MaxPen,7,TAG_END);

    ngad.ng_TopEdge += ngad.ng_Height+3;
    ngad.ng_Width = boxwidth;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID++;                     // 2
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Checked,TRUE,GTCB_Scaled,TRUE,TAG_END);

	ngad.ng_LeftEdge += boxwidth + 14 + (n = TLn(buffer) + 4);
    ngad.ng_TopEdge++;
	ngad.ng_Width = gWidth - 8 - rborder - ngad.ng_LeftEdge;
	ngad.ng_Height = fontheight + 1;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID++;                     // 3
	pgad = CreateGadget(SLIDER_KIND, pgad, &ngad, GTSL_Min, -1, GTSL_Max, 10,
				GTSL_LevelFormat, GetString(&gLocaleInfo, MSG_DECIMAL_PLACES_LABEL), 
				GTSL_Level, 0, GTSL_MaxPixelLen, n, GTSL_MaxLevelLen, 20, GA_RelVerify, TRUE, TAG_END);

    ngad.ng_LeftEdge = 8+lborder;
    ngad.ng_TopEdge = gHeight-2*fontheight-18-bborder;
    ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
    ngad.ng_Height = fontheight+4;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADOPT_FORMAT_FROM_CELL_GAD);
    ngad.ng_GadgetID = 26;
    /*pgad =*/ CreateGadget(BUTTON_KIND,pgad,&ngad,TAG_END);

    /******* Farbe & Ausrichtung *******/

    pgad = CreateContext(&cellPages[1]);

    n = TLn(GetString(&gLocaleInfo, MSG_FOREGROUND_LABEL));
    w = TLn(GetString(&gLocaleInfo, MSG_BACKGROUND_LABEL));
    w = max(w,n)+16+4*boxwidth;

    wd->wd_ExtData[0] = (APTR)((struct Page *)wd->wd_Data)->pg_APen;
    wd->wd_ExtData[1] = (APTR)((struct Page *)wd->wd_Data)->pg_BPen;

    ngad.ng_LeftEdge = (gWidth-w)/2;
    ngad.ng_TopEdge = barheight+2*fontheight+14+(h-9*fontheight-67)/2;
    ngad.ng_Width = boxwidth;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FOREGROUND_LABEL);
    ngad.ng_Flags = PLACETEXT_RIGHT;
    ngad.ng_GadgetID = 4;                   // 4
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_BACKGROUND_LABEL);
    ngad.ng_GadgetID++;                     // 5
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge += w-boxwidth;
    ngad.ng_TopEdge -= fontheight+7;
    ngad.ng_GadgetID++;                     // 6
    pgad = CreatePopGadget(wd,pgad,FALSE);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetID++;                     // 7
    pgad = CreatePopGadget(wd,pgad,FALSE);

    w = TLn(GetString(&gLocaleInfo, MSG_PATTERN_LABEL))+16+5*boxwidth;
    ngad.ng_LeftEdge = (gWidth-w)/2;
    ngad.ng_TopEdge += 2*fontheight+15;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PATTERN_LABEL);
    ngad.ng_GadgetID = 22;                  // 22
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge += w-2*boxwidth;
    ngad.ng_GadgetID++;                     // 23
    pgad = CreatePopGadget(wd,pgad,FALSE);

    ngad.ng_LeftEdge += boxwidth;
    ngad.ng_GadgetID++;                     // 24
    pgad = CreatePopGadget(wd,pgad,FALSE);

    ngad.ng_LeftEdge = 24+lborder+TLn(GetString(&gLocaleInfo, MSG_HORIZONTAL_LABEL));
    ngad.ng_TopEdge += 2*fontheight+15;
    ngad.ng_Width = gWidth-16-rborder-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_HORIZONTAL_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID = 8;                   // 8
	pgad = CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &sAlignLabels, TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_VERTICAL_LABEL);
    ngad.ng_GadgetID++;                     // 9
	pgad = CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &sVerticalAlignLabels, TAG_END);

    ngad.ng_LeftEdge = 8+lborder;
    ngad.ng_TopEdge = gHeight-2*fontheight-18-bborder;
    ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
    ngad.ng_Height = fontheight+4;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADOPT_ATTRIBUTES_FROM_CELLS_GAD);
    ngad.ng_GadgetID = 27;
   /*pgad =*/ CreateGadget(BUTTON_KIND,pgad,&ngad,TAG_END);

    /******* Schrift *******/

    pgad = CreateContext(&cellPages[2]);

    wd->wd_ExtData[4] = ((struct Page *)wd->wd_Data)->pg_Family;

	ngad.ng_LeftEdge = w = 24 + lborder + TLn(GetString(&gLocaleInfo, MSG_FONT_LABEL));
    ngad.ng_TopEdge = barheight+2*fontheight+14;
    ngad.ng_Width = i = gWidth-16-rborder-boxwidth-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID = 10;                  // 10
    pgad = CreateGadget(TEXT_KIND,pgad,&ngad,GTTX_Border,TRUE,GTTX_Text,((struct Page *)wd->wd_Data)->pg_Family ? ((struct Page *)wd->wd_Data)->pg_Family->ln_Name : "-",TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetID++;                     // 11
    pgad = CreatePopGadget(wd,pgad,FALSE);

    ngad.ng_LeftEdge = w;
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width = i;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL);
    ngad.ng_GadgetID++;                     // 12
    {
        char t[16];

        sprintf(t,"%ld pt",((struct Page *)wd->wd_Data)->pg_PointHeight >> 16);
        pgad = CreateGadget(STRING_KIND,pgad,&ngad,GTST_String,t,TAG_END);
    }
    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetID++;                     // 13
    pgad = CreatePopGadget(wd,pgad,FALSE);

    ngad.ng_LeftEdge = 16+lborder;
    n = ngad.ng_TopEdge += 2*fontheight+13;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_BOLD_GAD);
    ngad.ng_Flags = PLACETEXT_RIGHT;
    ngad.ng_GadgetID++;                     // 14
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ITALICS_GAD);
    ngad.ng_GadgetID++;                     // 15
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_UNDERLINED_GAD);
    ngad.ng_GadgetID++;                     // 16
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_STRIKE_THROUGH_GAD);
    ngad.ng_GadgetID = 29;                  // 29
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DOUBLE_UNDERLINED_GAD);
    ngad.ng_GadgetID++;                     // 30
    pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = boxwidth+54+lborder+TLn(GetString(&gLocaleInfo, MSG_DOUBLE_UNDERLINED_GAD))+TLn(GetString(&gLocaleInfo, MSG_ROTATION_LABEL));
    ngad.ng_TopEdge = n;
    ngad.ng_Width = gWidth-16-rborder-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ROTATION_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID = 17;                  // 17
    pgad = CreateGadget(STRING_KIND,pgad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHEAR_LABEL);
    ngad.ng_GadgetID++;                     // 18
    pgad = CreateGadget(STRING_KIND,pgad,&ngad,GTST_String,"0°",GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_WIDTH_LABEL);
    ngad.ng_GadgetID++;                     // 19
    pgad = CreateGadget(STRING_KIND,pgad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = 24+lborder+TLn(GetString(&gLocaleInfo, MSG_FONT_TRACKING_LABEL));
    ngad.ng_TopEdge += 4*fontheight+27;
    ngad.ng_Width = gWidth-16-rborder-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_KERNING_LABEL);
    ngad.ng_GadgetID++;                     // 20
	pgad = CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &sKerningLabels, GT_Underscore, '_', TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_TRACKING_LABEL);
    ngad.ng_GadgetID++;                     // 21
    pgad = CreateGadget(STRING_KIND,pgad,&ngad,GTST_String,"0 pt",STRINGA_Justification,GTJ_RIGHT,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = 8+lborder;
    ngad.ng_TopEdge = gHeight-2*fontheight-18-bborder;
    ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
    ngad.ng_Height = fontheight+4;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADOPT_FONT_STYLE_FROM_CELL_GAD);
    ngad.ng_GadgetID = 28;
    /*pgad =*/ CreateGadget(BUTTON_KIND,pgad,&ngad,TAG_END);

    /******* Diverses *******/

    pgad = CreateContext(&cellPages[3]);

	ngad.ng_LeftEdge = 24+lborder+TLn(GetString(&gLocaleInfo, MSG_PROPERTIES_LABEL));
    ngad.ng_TopEdge = barheight+2*fontheight+14+(h-2*fontheight-11)/2;
    ngad.ng_Width = gWidth-16-rborder-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PROPERTIES_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID = 25;                   // 25
	/*pgad =*/ CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &sSecurityLabels, TAG_END);

    /******* Page & Index, Ok & Abbrechen *******/

	gad = NewObj(wd, WOT_GADGET, pagegclass, NULL, PAGEGA_Pages,  &cellPages,
												   PAGEGA_RefreshFunc, RefreshCellPageGadgets,
												   PAGEGA_Active, wd->wd_CurrentPage,
                                                   GA_Previous,   gad,
												   GA_Left,       8 + lborder,
												   GA_Top,        barheight + fontheight + 9,
												   GA_Width,      gWidth - 16 - lborder - rborder,
                                                   GA_Height,     h,
                                                   GA_ID,         31,
                                                   TAG_END);
	gad = NewObj(wd, WOT_GADGET, indexgclass, NULL, GA_Top,       barheight + 3,
                                                    GA_Left,      lborder,
													GA_Width,     gWidth - rborder - lborder,
													GA_Height,    gHeight - barheight - fontheight - 13 - bborder,
                                                    GA_DrawInfo,  dri,
                                                    GA_Previous,  gad,
                                                    ICA_TARGET,   gad,
                                                    GA_ID,        32,
													IGA_Labels,   &sCellPageLabels,
													IGA_Active,   wd->wd_CurrentPage,
                                                    TAG_END);
	wd->wd_PageHandlingGadget = gad;

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = gHeight-fontheight-7-bborder;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID = 33;                   // 33
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = (gWidth-ngad.ng_Width) >> 1;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ASSIGN_UGAD);
    ngad.ng_GadgetID++;                      // 34
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_GadgetID++;                      // 35
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void
RemoveDiagramGadgets(struct winData *wd)
{
	struct gGadget *gg, *ngg;

	RemoveGadgets(&wd->wd_Pages[2], FALSE); // alle nicht GadTools-Gadgets entfernen
	FreeGadgets(wd->wd_Pages[2]);		    // GadTools-Gadgets freigeben
	wd->wd_Pages[2] = NULL;

	for (gg = (APTR)wd->u.diagram.wd_Gadgets->mlh_Head; (ngg = (APTR)gg->gg_Node.mln_Succ) != 0; gg = ngg)
    {
        if (gg->gg_Page == 2)
        {
            MyRemove(gg);
			FreePooled(pool, gg, sizeof(struct gGadget));
        }
    }
}


void
AddDiagramGadgets(struct gDiagram *gd, struct Gadget *pgad, struct MinList *list, int width, int height)
{
    struct gInterface *gi;
	int lines = 0, h;

    if (!gd || !pgad || !list)
        return;

	for (gi = gd->gd_Object.go_Class->gc_Interface; gi && gi->gi_Tag; gi++)
    {
		switch (gi->gi_Type)
        {
            case GIT_PEN:
            case GIT_WEIGHT:
            case GIT_CHECKBOX:
            case GIT_BUTTON:
            case GIT_FILENAME:
            case GIT_TEXT:
            case GIT_FORMULA:
            case GIT_FONT:
                lines++;
        }
    }

	h = height - barheight - 2*fontheight - 23 - bborder;
	ngad.ng_TopEdge = barheight + fontheight + 12 + (h - lines*(fontheight + 7)) / 2;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID = 42;

	for (gi = gd->gd_Object.go_Class->gc_Interface; gi && gi->gi_Tag; gi++)
    {
        ngad.ng_GadgetText = (STRPTR)GetGLabel(gi);

		switch (gi->gi_Type)
        {
            case GIT_PEN:
				ngad.ng_LeftEdge = lborder + 16 + 2*boxwidth + TLn(ngad.ng_GadgetText);
                ngad.ng_GadgetID++;
				pgad = CreatePopGadget(wd, pgad, FALSE);
				AddGGadget(list, pgad, gi->gi_Tag, GIT_PEN, POPUP_KIND, 2);
                break;

            case GIT_WEIGHT:
                ngad.ng_LeftEdge = lborder+TLn(ngad.ng_GadgetText)+16;
                ngad.ng_Width = width-8-rborder-boxwidth-ngad.ng_LeftEdge;
                ngad.ng_GadgetID++;
                pgad = CreateGadget(STRING_KIND,pgad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',TAG_END);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_WEIGHT,STRING_KIND,2);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
                pgad = CreatePopGadget(wd,gad,FALSE);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_WEIGHT,POPUP_KIND,2);
                break;
/*
            case GIT_CYCLE:
                if (!gi->gi_Special)
                    break;
                break;
*/
            case GIT_CHECKBOX:
                ngad.ng_LeftEdge = lborder+8;
                ngad.ng_Width = boxwidth;
                ngad.ng_Flags = PLACETEXT_RIGHT;
                ngad.ng_GadgetID++;
                pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,FALSE,TAG_END);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_CHECKBOX,CHECKBOX_KIND,2);
                ngad.ng_Flags = PLACETEXT_LEFT;
                break;

            case GIT_BUTTON:
				ngad.ng_LeftEdge = lborder + 8;
				ngad.ng_Width = width - lborder - rborder - 16;
                ngad.ng_Flags = PLACETEXT_IN;
                ngad.ng_GadgetID++;
				pgad = CreateGadget(BUTTON_KIND, pgad, &ngad, TAG_END);
				AddGGadget(list, pgad, gi->gi_Tag, GIT_BUTTON, BUTTON_KIND, 2);
                ngad.ng_Flags = PLACETEXT_LEFT;
                break;

            case GIT_FILENAME:
                ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
                ngad.ng_Width = width-8-rborder-boxwidth-ngad.ng_LeftEdge;
                ngad.ng_GadgetID++;
                pgad = CreateGadget(STRING_KIND,pgad,&ngad,GT_Underscore,'_',TAG_END);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_FILENAME,STRING_KIND,2);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
				pgad = CreatePopGadget(wd, pgad, FALSE);
				AddGGadget(list, pgad, gi->gi_Tag, GIT_FILENAME, POPUP_KIND, 2);
                break;

            case GIT_TEXT:
            case GIT_FORMULA:
                if (gi->gi_Tag == GOA_Help)
                {
                    ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
                    ngad.ng_Width = width-8-rborder-ngad.ng_LeftEdge;
                    ngad.ng_GadgetID++;
                    pgad = CreateGadget(STRING_KIND,pgad,&ngad,GT_Underscore,'_',TAG_END);
                    AddGGadget(list,pgad,gi->gi_Tag,GIT_TEXT,STRING_KIND,2);
                }
                else if (gi->gi_Tag == GOA_Command)
                {
                    ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
					ngad.ng_Width = width - 8 - rborder - boxwidth - ngad.ng_LeftEdge - TLn(gAppCommandLabels[0]) - 42;
                    ngad.ng_GadgetID++;
                    pgad = CreateGadget(STRING_KIND,pgad,&ngad,GT_Underscore,'_',TAG_END);
					AddGGadget(list, pgad, gi->gi_Tag, GIT_TEXT,STRING_KIND, 2);

                    ngad.ng_LeftEdge += ngad.ng_Width;
					ngad.ng_Width = TLn(gAppCommandLabels[0]) + 42;
                    ngad.ng_GadgetText = NULL;
                    ngad.ng_GadgetID++;
					pgad = CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &gAppCommandLabels, TAG_END);
					AddGGadget(list, pgad, gi->gi_Tag, GIT_NONE, CYCLE_KIND, 2);

                    ngad.ng_LeftEdge += ngad.ng_Width;
                    ngad.ng_GadgetID++;
                    pgad = CreatePopGadget(wd,pgad,FALSE);
                    AddGGadget(list,pgad,gi->gi_Tag,GIT_TEXT,POPUP_KIND,2);
                }
                else
                {
                    ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
                    ngad.ng_Width = width-8-rborder-ngad.ng_LeftEdge;
                    ngad.ng_GadgetID++;
                    pgad = CreateGadget(STRING_KIND,pgad,&ngad,GT_Underscore,'_',TAG_END);
                    AddGGadget(list,pgad,gi->gi_Tag,GIT_TEXT,STRING_KIND,2);
                }
                break;

            case GIT_FONT:
                ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
                ngad.ng_Width = width-48-rborder-TLn(GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL))-TLn("999 pt")-2*boxwidth-ngad.ng_LeftEdge;
                ngad.ng_GadgetID++;
                pgad = CreateGadget(TEXT_KIND,pgad,&ngad,GTTX_Border,TRUE,GT_Underscore,'_',TAG_END);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_FONT,TEXT_KIND,2);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
                pgad = CreatePopGadget(wd,pgad,FALSE);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_FONT,POPUP_KIND,2);

                ngad.ng_LeftEdge += boxwidth+14+TLn(GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL));
                ngad.ng_Width = width-rborder-boxwidth-ngad.ng_LeftEdge;
                ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL);
                ngad.ng_GadgetID++;
                pgad = CreateGadget(STRING_KIND,pgad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',TAG_END);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_FONT,STRING_KIND,2);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
                pgad = CreatePopGadget(wd,pgad,FALSE);
                AddGGadget(list,pgad,gi->gi_Tag,GIT_FONT,POPUPPOINTS_KIND,2);
                break;

            default:
                ngad.ng_TopEdge -= fontheight+7;
        }
        ngad.ng_TopEdge += fontheight+7;
    }
}


void PUBLIC
RefreshDiagramPageGadgets(REG(a0, struct Window *win), REG(d0, long active))
{
    struct winData *wd = (struct winData *)win->UserData;
    struct gDiagram *gd = NULL;
    struct RastPort *rp = win->RPort;
    struct Gadget *gad;

    if (wd)
		gd = wd->u.diagram.wd_CurrentDiagram;

	switch (active)
    {
        case 0:  // Inhalt
        case 1:  // Diagrammtyp
            break;
        case 2:  // Darstellung
        {
            STRPTR t = NULL;

            if (gd)
            {
                struct gGadget *gg;
                int count = 0;

				foreach (wd->u.diagram.wd_Gadgets, gg)
                {
                    if (gg->gg_Type == GIT_PEN)
                    {
                        struct gInterface *gi;

						for (gi = gd->gd_Object.go_Class->gc_Interface;gi && gi->gi_Tag;gi++)
                        {
                            if (gg->gg_Tag == gi->gi_Tag)
                            {
                                itext.IText = GetGLabel(gi);
                                PrintIText(rp,&itext,gg->gg_Gadget->LeftEdge-8-2*boxwidth-IntuiTextLength(&itext),gg->gg_Gadget->TopEdge+3);
                            }
                        }
                    }
                    count++;
                }
                if (count == 0)
                    t = GetString(&gLocaleInfo, MSG_DIAGRAM_HAS_NO_DISPLAY_OPTIONS_ERR);
            }
            else
                t = GetString(&gLocaleInfo, MSG_NO_DIAGRAM_TYPE_SELECTED_ERR);

            if (t != NULL)
            {
                itext.IText = t;
				itext.ITextFont = scr->Font;
                PrintIText(rp,&itext,(win->Width-IntuiTextLength(&itext)) >> 1,(win->Height-fontheight) >> 1);
            }
            break;
        }
        case 3:  // Werte
            if ((gad = GadgetAddress(win, 10)) != 0)
            {
                long x,y;

                itext.IText = GetString(&gLocaleInfo, MSG_ROW_GAD);
                PrintIText(rp,&itext,x = gad->LeftEdge,y = gad->TopEdge-fontheight-1);
                itext.IText = GetString(&gLocaleInfo, MSG_COLOR_GAD);
                PrintIText(rp,&itext,x+2*boxwidth+8,y);
                itext.IText = GetString(&gLocaleInfo, MSG_MARK_GAD);
                PrintIText(rp,&itext,x+gad->Width-IntuiTextLength(&itext),y);

                if ((gad = GadgetAddress(win, 11)) != 0)
                    DrawColorField(rp,gad,(ULONG)gad->UserData,TRUE);
                //GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&gd->gd_Values,TAG_END);
            }
            /*if (gd && gd->gd_Object.go_Class->gc_Interface)
            {
            }
            else
            {
                if (gd)
                    itext.IText = "keine Attribute definiert";
                else
                    itext.IText = "noch kein Diagrammtyp gewählt";
                PrintIText(win->RPort,&itext,(win->Width-IntuiTextLength(&itext)) >> 1,(win->Height-fontheight)/2);
            }*/
            break;
        case 4:  // Achsen
			if ((gad = GadgetAddress(win, 14)) != 0)
				DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_LABELS_BORDER),8+lborder,gad->TopEdge-fontheight-2,win->Width-16-lborder-rborder,3*fontheight+18);
			if ((gad = GadgetAddress(win, 15)) != 0)
            {
                ULONG pen = 0x0;

                GetGObjectAttr((struct gObject *)gd,GAA_NumberPen,&pen);
                DrawColorField(rp,gad,pen,TRUE);
                itext.IText = GetString(&gLocaleInfo, MSG_COLOR_LABEL);
                PrintIText(rp,&itext,gad->LeftEdge-2*boxwidth-8-IntuiTextLength(&itext),gad->TopEdge+2);
            }
			if ((gad = GadgetAddress(win, 20)) != 0)
				DrawGroupBorder(rp, GetString(&gLocaleInfo, MSG_BACKGROUND_BORDER), 8 + lborder, gad->TopEdge - fontheight - 2, win->Width - 16 - lborder - rborder, 4*fontheight + 25);
             
			if ((gad = GadgetAddress(win, 21)) != 0)
            {
                ULONG pen = 0x0;

                GetGObjectAttr((struct gObject *)gd,GAA_BPen,&pen);
                DrawColorField(rp,gad,pen,TRUE);
                itext.IText = GetString(&gLocaleInfo, MSG_BACKGROUND_COLOR_LABEL);
                PrintIText(rp,&itext,gad->LeftEdge-2*boxwidth-8-IntuiTextLength(&itext),gad->TopEdge+2);
            }
            break;
    }
}

#define GD_MAXROTATION 45
		
 
void ASM
CreateDiagramGadgets(REG(a0, struct winData *wd))
{
	struct gDiagram *gd = wd->u.diagram.wd_CurrentDiagram;
    struct Gadget *pgad;
	long   i, h, top, w, a, b;
    char   t[32];

	D({if (gd != NULL) {
		check_page_pointer(gd->gd_DataPage);
		check_page_pointer(gd->gd_Object.go_Page);
	}});

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 33; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 35; //Set Gadgetnumber 
#endif

	gWidth = 460 + rborder + lborder;
	gHeight = barheight + fontheight*13 + 95 + bborder + (gd ? 0 : fontheight + 7);
	h = gHeight - barheight - fontheight - 16 - bborder - (gd ? 0 : fontheight + 7);
	top = barheight + fontheight + 10;
	wd->wd_Pages[5] = NULL;

    /******* Inhalt *******/

	pgad = CreateContext(&wd->wd_Pages[0]);

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_LABEL);
	ngad.ng_LeftEdge = 16 + lborder+TLn(GetString(&gLocaleInfo, MSG_RANGE_LABEL));
/*  ngad.ng_TopEdge = top+((h-4*fontheight-25) >> 1);*/  // von der Legende...
	ngad.ng_TopEdge = top + ((h - 3*fontheight - 18) >> 1);
	ngad.ng_Width = gWidth - 8 - rborder - ngad.ng_LeftEdge;
    ngad.ng_Flags = PLACETEXT_LEFT;         // 1
	pgad = CreateGadget(STRING_KIND, pgad, &ngad, GTST_String, gd ? (STRPTR)gd->gd_Object.go_Node.ln_Name : GetString(&gLocaleInfo, MSG_NEW_DIAGRAM_NAME), TAG_END);
	AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GOA_Name, GIT_TEXT, STRING_KIND, 0);

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_RANGE_LABEL);
	ngad.ng_LeftEdge = 16 + lborder + TLn(ngad.ng_GadgetText);
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = gWidth - 38 - rborder - ngad.ng_LeftEdge - TLn(GetString(&gLocaleInfo, MSG_FROM_BLOCK_GAD))
		- TLn(GetString(&gLocaleInfo, MSG_VALUES_LABEL)) - (i = TLn(GetString(&gLocaleInfo, MSG_HORIZONTAL_GAD))+42);
    ngad.ng_GadgetID++;                     // 2
    {
        struct Page *page;

        if (gd)
			zstrcpy(t, gd->gd_Range);
		else if ((page = wd->wd_Data) != 0) {
            if (page->pg_MarkCol != -1)
				TablePos2String(page, (struct tablePos *)&page->pg_MarkCol, t);
            else
                t[0] = 0;
        }
		pgad = CreateGadget(STRING_KIND, pgad, &ngad, GTST_String, t, TAG_END);
    }
	AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GDA_Range, GIT_FORMULA, STRING_KIND, 0);

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FROM_BLOCK_GAD);
	ngad.ng_Width = TLn(ngad.ng_GadgetText) + 16;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;                     // 3
	pgad = CreateGadget(BUTTON_KIND, pgad, &ngad, GT_Underscore, '_', TAG_END);

    w = ngad.ng_LeftEdge+ngad.ng_Width;

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_VALUES_LABEL);
	ngad.ng_LeftEdge += ngad.ng_Width + 14 + TLn(ngad.ng_GadgetText);
    ngad.ng_Width = i;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID++;                     // 4
	pgad = CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &sOrderLabels, GTCY_Active, gd ? gd->gd_ReadData : 0, TAG_END);
	AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GDA_ReadData, GIT_CYCLE, CYCLE_KIND, 0);

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PAGE_LABEL);
	ngad.ng_LeftEdge = 16 + lborder + TLn(GetString(&gLocaleInfo, MSG_RANGE_LABEL));
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = w - ngad.ng_LeftEdge - boxwidth;
    ngad.ng_GadgetID++;                     // 5
    {
		struct Page *pg = gd ? gd->gd_DataPage : rxpage;

        if (!pg)
            pg = rxpage;

		pgad = CreateGadget(TEXT_KIND, pgad, &ngad, GTTX_Text, pg->pg_Node.ln_Name, GTTX_Border, TRUE, TAG_END);
        pgad->UserData = pg;
    }

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetID++;                     // 6
	pgad = CreatePopGadget(wd, pgad, FALSE);
	AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GDA_DataPage, GIT_DATA_PAGE, POPUP_KIND, 0);

/*  ngad.ng_LeftEdge = 8+lborder;
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width = boxwidth;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetText = "Legende darstellen";
    ngad.ng_Flags = PLACETEXT_RIGHT;
    ngad.ng_GadgetID++;                     // 7
    CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GA_Disabled,TRUE,TAG_END);
*/
    /******* Diagrammtyp *******/

	pgad = CreateContext(&wd->wd_Pages[1]);

	i = (h - 6 - fontheight) / itemheight;
	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_TopEdge = top + fontheight + 6 + ((h - itemheight*i - 10 - fontheight) >> 1);
	ngad.ng_Width = gWidth - 16 - lborder - rborder;
	ngad.ng_Height = itemheight * i + 4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DIAGRAM_TYPES_GAD);
    ngad.ng_Flags = PLACETEXT_ABOVE;
    ngad.ng_GadgetID = 8;                   // 8
	/*pgad =*/ CreateGadget(LISTVIEW_KIND, pgad, &ngad, GTLV_Labels, &gdiagrams, GTLV_ShowSelected, NULL,
		GTLV_Selected, gd ? FindListEntry(&gdiagrams, (struct MinNode *)gd->gd_Object.go_Class) : ~0L,
		GTLV_CallBack, &renderHook, GTLV_MaxPen, 7, GTLV_ItemHeight, itemheight, TAG_END);
    ngad.ng_Height = fontheight+4;

    /******* Darstellung *******/

	pgad = CreateContext(&wd->wd_Pages[2]);
	AddDiagramGadgets(gd, pgad, wd->u.diagram.wd_Gadgets, gWidth, gHeight);

    /******* Werte *******/

	pgad = CreateContext(&wd->wd_Pages[3]);

	i = (h - 13 - 2*fontheight) / (fontheight + 4);
	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_TopEdge = top + 6 + fontheight + ((h - (fontheight + 4)*i - 17 - 2*fontheight) >> 1);
    ngad.ng_Width = gWidth-16-lborder-rborder;
    ngad.ng_Height = (fontheight+4)*i+4;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID = 10;                  // 10
	pgad = CreateGadget(LISTVIEW_KIND, pgad, &ngad, GTLV_Labels,		gd ? &gd->gd_Values : NULL,
													GTLV_ShowSelected,	NULL,
													GTLV_CallBack,		&glinkHook,
													GTLV_MaxPen,		7,
													GTLV_ItemHeight,	fontheight + 4,
													GA_Disabled,		!gd,
                                                    TAG_END);

	ngad.ng_LeftEdge = 8 + lborder + 2*boxwidth;
	ngad.ng_TopEdge += ngad.ng_Height + 3;
	ngad.ng_Height = fontheight + 4;
    ngad.ng_GadgetID++;                     // 11
	if ((pgad = CreatePopGadget(wd, pgad, !gd)) != 0)
        pgad->UserData = (APTR)0x0;

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_VALUES_LABEL);
    ngad.ng_LeftEdge += boxwidth+14+TLn(ngad.ng_GadgetText);
    ngad.ng_Width = gWidth-ngad.ng_LeftEdge-38-rborder-TLn(GetString(&gLocaleInfo, MSG_MARK_LABEL))-boxwidth;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID++;                     // 12
    pgad = CreateGadget(TEXT_KIND,pgad,&ngad,GTTX_Border,TRUE,TAG_END);

    ngad.ng_LeftEdge = gWidth-8-rborder-boxwidth;
    ngad.ng_Width = boxwidth;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_MARK_LABEL);
    ngad.ng_GadgetID++;                     // 13
    /*pgad =*/ CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GA_Disabled,!gd,TAG_END);

    /******* Achsen *******/

    {
        BOOL disabled = TRUE;
        struct gClass *gc;
        struct gAxes *ga = NULL;

        if (gd && (gc = FindGClass("axes")))
			disabled = !gIsSubclassFrom(gd->gd_Object.go_Class, gc);
        if (!disabled)
			ga = GINST_DATA(gc, gd);

		pgad = CreateContext(&wd->wd_Pages[4]);

		ngad.ng_LeftEdge = 16 + lborder;
		ngad.ng_TopEdge = top + ((h - 7*fontheight - 35) >> 1);
        ngad.ng_Width = boxwidth;
        ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DISPLAY_LABELS_GAD);
        ngad.ng_Flags = PLACETEXT_RIGHT;
        ngad.ng_GadgetID++;                     // 14
        pgad = CreateGadget(CHECKBOX_KIND,pgad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,ga ? ga->ga_Flags & GAF_SHOWNUMBERS: TRUE,GA_Disabled,disabled,TAG_END);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_ShowNumbers, GIT_CHECKBOX, CHECKBOX_KIND, 4);

		ngad.ng_LeftEdge = gWidth - 16 - rborder - boxwidth;
        ngad.ng_GadgetID++;                     // 15
		if ((pgad = CreatePopGadget(wd, pgad, disabled)))
            pgad->UserData = (APTR)(ga ? ga->ga_NumberPen : 0x0);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_NumberPen, GIT_PEN,POPUP_KIND, 4);

        //ngad.ng_TopEdge += 2*fontheight+10;
		ngad.ng_TopEdge += fontheight + 7;
        ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_LABEL);
        ngad.ng_LeftEdge = 24+lborder+TLn(ngad.ng_GadgetText);
        ngad.ng_Width = gWidth-64-rborder-TLn(GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL))-TLn("999 pt")-2*boxwidth-ngad.ng_LeftEdge;
        ngad.ng_Flags = PLACETEXT_LEFT;
        ngad.ng_GadgetID++;                     // 16
        pgad = CreateGadget(TEXT_KIND,pgad,&ngad,GTTX_Border,TRUE,GT_Underscore,'_',GA_Disabled,disabled,TAG_END);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_FontInfo, GIT_FONT, TEXT_KIND, 4);

        ngad.ng_LeftEdge += ngad.ng_Width;
        ngad.ng_GadgetID++;                     // 17
		pgad = CreatePopGadget(wd, pgad, disabled);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_FontInfo, GIT_FONT, POPUP_KIND, 4);

		ngad.ng_LeftEdge += boxwidth + 14 + TLn(GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL));
		ngad.ng_Width = gWidth - 16 - rborder - boxwidth - ngad.ng_LeftEdge;
        ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL);
        ngad.ng_GadgetID++;                     // 18
        pgad = CreateGadget(STRING_KIND,pgad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',GA_Disabled,disabled,TAG_END);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_FontInfo, GIT_FONT, STRING_KIND, 4);

        ngad.ng_LeftEdge += ngad.ng_Width;
        ngad.ng_GadgetID++;                     // 19
        pgad = CreatePopGadget(wd,pgad,disabled);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_FontInfo, GIT_FONT, POPUPPOINTS_KIND, 4);

		// grid string length
		a = TLn(sAxesGridLabels[0]);
		b = TLn(sAxesGridLabels[1]);
		a = max(a, b);
		b = TLn(sAxesGridLabels[2]);
		a = max(a, b);

        ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_GRID_LABEL);
		ngad.ng_LeftEdge = 24 + lborder + TLn(ngad.ng_GadgetText);
		ngad.ng_TopEdge += 2 * fontheight + 13;
		ngad.ng_Width = 2 * boxwidth + a + 32;
        ngad.ng_GadgetID++;                     // 20
		pgad = CreateGadget(CYCLE_KIND, pgad, &ngad, GTCY_Labels, &sAxesGridLabels, GA_Disabled, disabled, TAG_END);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_Raster, GIT_CYCLE, CYCLE_KIND, 4);

		ngad.ng_LeftEdge = 24 + lborder + TLn(GetString(&gLocaleInfo, MSG_BACKGROUND_COLOR_LABEL)) + 2*boxwidth;
		ngad.ng_TopEdge += fontheight + 7;
        ngad.ng_GadgetID++;                     // 21
		if ((pgad = CreatePopGadget(wd, pgad, disabled)) != 0)
            pgad->UserData = (APTR)(ga ? ga->ga_BPen : 0xffffff);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_BPen, GIT_PEN, POPUP_KIND, 4);
		
		ngad.ng_LeftEdge = 16 + lborder;
		ngad.ng_TopEdge += fontheight + 7;
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TRANSPARENT_GAD);
        ngad.ng_Flags = PLACETEXT_RIGHT;
		ngad.ng_GadgetID++;                     // 22
		pgad = CreateGadget(CHECKBOX_KIND, pgad, &ngad, GTCB_Scaled, TRUE, GTCB_Checked,
					ga ? ga->ga_Flags & GAF_TRANSPARENT_BACKGROUND : FALSE, GA_Disabled, disabled, TAG_END);
		AddGGadget(wd->u.diagram.wd_Gadgets, pgad, GAA_TransparentBackground, GIT_CHECKBOX, CHECKBOX_KIND, 4);
    }

    /******* Page & Index, Ok, Vorschau & Abbrechen *******/

	gad = NewObj(wd, WOT_GADGET, pagegclass, NULL,
			PAGEGA_Pages,		 wd->wd_Pages,
			PAGEGA_RefreshFunc,	 RefreshDiagramPageGadgets,
			GA_Previous,		 gad,
			GA_Left,			 8 + lborder,
			GA_Top,				 barheight + fontheight + 10,
			GA_Width,			 gWidth - 16 - rborder - lborder,
			GA_Height,			 h,
			GA_ID,				 31,
			TAG_END);
	wd->u.diagram.wd_PageGadget = gad;
	gad = NewObj(wd, WOT_GADGET, indexgclass, NULL,
			GA_Top,       barheight + 3,
			GA_Left,      lborder,
			GA_Width,     gWidth - lborder - rborder,
			GA_Height,    h+fontheight + 10,
			GA_DrawInfo,  dri,
			GA_Previous,  gad,
			ICA_TARGET,   gad,
			GA_ID,        32,
			IGA_Labels,   &sDiagramPageLabels,
			TAG_END);
	wd->wd_PageHandlingGadget = gad;

	if (!gd)
	{
		a = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD));
		b = TLn(GetString(&gLocaleInfo, MSG_OK_GAD));
		a = max(a, b);
		b = TLn(GetString(&gLocaleInfo, MSG_PREVIEW_GAD));
		a = max(a, b);

		ngad.ng_LeftEdge = lborder;
		ngad.ng_TopEdge = gHeight - fontheight - 7 - bborder;
		ngad.ng_Width = a + 16;
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
		ngad.ng_Flags = PLACETEXT_IN;
		ngad.ng_GadgetID = 33;                   // 33
		gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

		ngad.ng_LeftEdge = (gWidth - ngad.ng_Width) >> 1;
        ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PREVIEW_GAD);
        ngad.ng_GadgetID++;                      // 34
		gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

		ngad.ng_LeftEdge = gWidth - rborder - ngad.ng_Width;
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
		ngad.ng_GadgetID = 35;                     // 35
		gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
	}
}


void ASM
CreateFormelGadgets(REG(a0, struct winData *wd))
{
    long i = 14;

    gWidth = 4*TLn(GetString(&gLocaleInfo, MSG_MATHEMATICS_GAD))+216+lborder+rborder;
    gHeight = fontheight*i+3*fontheight+barheight+32+bborder;
    while(gHeight > scr->Height)
    {
        gHeight -= fontheight;
        i--;
    }
    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = barheight+fontheight+9;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_MATHEMATICS_GAD))+100;
    ngad.ng_Height = fontheight*i+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FUNCTION_CATEGORY_GAD);
    ngad.ng_Flags = PLACETEXT_ABOVE;        // 1
    gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,wd->wd_ExtData[2],GTLV_ShowSelected,NULL,GTLV_CallBack,&formelHook,GTLV_MaxPen,7,GTLV_Selected,fewftype,TAG_END);

    ngad.ng_LeftEdge += 6+ngad.ng_Width;
    ngad.ng_Width += 2*TLn(GetString(&gLocaleInfo, MSG_MATHEMATICS_GAD))+10;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FUNCTIONS_GAD);
    ngad.ng_GadgetID++;                     // 2
    gad = wd->wd_ExtData[1] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,&fewfuncs,GTLV_ShowSelected,NULL,TAG_END);

    ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_DESCRIPTION_LABEL));
    ngad.ng_TopEdge += ngad.ng_Height+3;
    ngad.ng_Width = gWidth-ngad.ng_LeftEdge-rborder;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DESCRIPTION_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID++;                     // 3
    gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge += fontheight+9;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_GAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;                     // 4
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = gWidth-ngad.ng_Width-rborder;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_GAD);
    ngad.ng_GadgetID++;                     // 5
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}
			

// ToDo: that should be done differently (better localized page sizes)!
STRPTR pageLabels[] = {"A3", "A4", "A5", "US Letter", "US Legal", NULL, NULL};
#ifdef __amigaos4__
	const ULONG pageWidth[] = {304128, 215040, 151552, 221184, 221184};  /* in mm*1024 */
	const ULONG pageHeight[] = {430080, 304128, 215040, 285696, 363520};
#else
	const ULONG pageWidth[] = {305664, 215142, 152166, 221081, 221081};  /* in mm*1024 */
	const ULONG pageHeight[] = {432332, 304640, 216166, 286105, 364134};
#endif
const ULONG pageSizes = 5;


void ASM
CreatePageSetupGadgets(REG(a0, struct winData *wd))
{
    struct Mappe *mp = (struct Mappe *)wd->wd_Data;
    char   t[36];
    long   i;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 9; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 10; //Set Gadgetnumber 
#endif
    gWidth = TLn(GetString(&gLocaleInfo, MSG_PAGE_FORMAT_GAD))+TLn(GetString(&gLocaleInfo, MSG_MARKED_GAD))+116+rborder+lborder;
	gHeight = barheight + fontheight * 11 + 81 + bborder;
	for (i = 0; i < pageSizes; i++)
    {
        if (pageWidth[i] == mp->mp_mmWidth && pageHeight[i] == mp->mp_mmHeight)
            break;
    }
    ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_PAGE_SIZE_LABEL));
    ngad.ng_TopEdge = barheight+fontheight+5;
    ngad.ng_Width = gWidth-rborder-8-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PAGE_SIZE_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    gad = CreateGadget(CYCLE_KIND,gad,&ngad,GT_Underscore,'_',GTCY_Labels,&pageLabels,GTCY_Active,i,TAG_END);

    ngad.ng_LeftEdge += boxwidth;
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width -= boxwidth;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PAGE_WIDTH_LABEL);
    ngad.ng_GadgetID++;                     // 2
    strcpy(t,ita(mp->mp_mmWidth/10240.0,2,FALSE));
    strcat(t," cm");
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',GTST_String,t,TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PAGE_HEIGHT_LABEL);
    ngad.ng_GadgetID++;                     // 3
    strcpy(t,ita(mp->mp_mmHeight/10240.0,2,FALSE));
    strcat(t," cm");
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',GTST_String,t,TAG_END);

    i = 0;
    if (mp->mp_Flags & MPF_PAGEMARKED)
        i = 1;
    if (mp->mp_Flags & MPF_PAGEONLY)
        i = 2;
    ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_LIMITS_LABEL));
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width = gWidth-rborder-8-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_LIMITS_LABEL);
    ngad.ng_GadgetID++;                     // 4
	gad = CreateGadget(CYCLE_KIND, gad, &ngad,GT_Underscore, '_', GTCY_Labels, &sPageLimitLabels, GTCY_Active, i, TAG_END);

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_LEFT_MARGIN_LABEL);
    ngad.ng_LeftEdge = lborder+36+TLn(GetString(&gLocaleInfo, MSG_BOTTOM_MARGIN_LABEL));
    ngad.ng_TopEdge += 2*fontheight+15;
    ngad.ng_Width = gWidth-rborder-18-ngad.ng_LeftEdge;
    ngad.ng_GadgetID++;                     // 5
    strcpy(t,ita(mp->mp_BorderLeft/10240.0,2,FALSE));
    strcat(t," cm");
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',GTST_String,t,TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_RIGHT_MARGIN_LABEL);
    ngad.ng_GadgetID++;                     // 6
    strcpy(t,ita(mp->mp_BorderRight/10240.0,2,FALSE));
    strcat(t," cm");
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',GTST_String,t,TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TOP_MARGIN_LABEL);
    ngad.ng_GadgetID++;                     // 7
    strcpy(t,ita(mp->mp_BorderTop/10240.0,2,FALSE));
    strcat(t," cm");
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',GTST_String,t,TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_BOTTOM_MARGIN_LABEL);
    ngad.ng_GadgetID++;                     // 8
    strcpy(t,ita(mp->mp_BorderBottom/10240.0,2,FALSE));
    strcat(t," cm");
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',GTST_String,t,TAG_END);

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge += fontheight+12;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;                     // 9
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = gWidth-ngad.ng_Width-rborder;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_GadgetID++;                     // 10
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

}


void ASM
CreateDocInfoGadgets(REG(a0, struct winData *wd))
{
    struct Mappe *mp = (struct Mappe *)wd->wd_Data;

	gWidth = 3 * TLn(GetString(&gLocaleInfo, MSG_COMMENTS_LABEL)) + 140;
	gHeight = barheight + bborder + 9 * fontheight + 39;

	//wd->wd_ShortCuts = "oauvsb";
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, MSG_AUTHOR_ULABEL,
		MSG_VERSION_ULABEL, MSG_KEYWORDS_ULABEL, MSG_COMMENTS_ULABEL, TAG_END);

	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_AUTHOR_ULABEL);
	ngad.ng_LeftEdge = lborder + 8 + TLn(GetString(&gLocaleInfo, MSG_COMMENTS_LABEL));
    ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge;
    ngad.ng_Flags = PLACETEXT_LEFT;         // 1
    gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,mp->mp_Author,TAG_END);

	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_VERSION_ULABEL);
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetID++;                     // 2
    gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,mp->mp_Version,TAG_END);

	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_KEYWORDS_ULABEL);
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetID++;                     // 3
    gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,mp->mp_CatchWords,TAG_END);

#ifdef __amigaos4__
	gad = NewObj(wd, WOT_GADGET, TEXTEDITOR_GetClass(), "editorgadget", 
			GA_Left,      ngad.ng_LeftEdge,
            GA_Top,       ngad.ng_TopEdge+fontheight+7,
            GA_Width,     ngad.ng_Width,
            GA_Height,    fontheight*5+4,
            GA_DrawInfo,  dri,
            GA_Previous,  gad,
//          EGA_Scroller, TRUE,
          	GA_TEXTEDITOR_Contents,     mp->mp_Note,
            GA_ID,        4,
            TAG_END);
#else
	gad = NewObj(wd, WOT_GADGET, NULL, "pinc-editgadget",
			GA_Left,      ngad.ng_LeftEdge,
            GA_Top,       ngad.ng_TopEdge+fontheight+7,
            GA_Width,     ngad.ng_Width,
            GA_Height,    fontheight*5+4,
            GA_DrawInfo,  dri,
            GA_Previous,  gad,
            EGA_Scroller, TRUE,
            EGA_Text,     mp->mp_Note,
            GA_ID,        4,
            TAG_END);
#endif
    wd->wd_ExtData[1] = gad;

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))-linelen+20;
    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = gHeight-bborder-fontheight-7;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID = wd->wd_ShortCuts[0];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_LeftEdge = gWidth - rborder - ngad.ng_Width;
    ngad.ng_GadgetID = wd->wd_ShortCuts[1];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreateDocumentGadgets(REG(a0, struct winData *wd))
{
    struct Mappe *mp = (struct Mappe *)wd->wd_Data;
    long   i;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 17; //Set Gadgetnumber 
#endif
	gWidth = 114 + TLn(GetString(&gLocaleInfo, MSG_PROTECT_DOCUMENT_LABEL))
		+ TLn(GetString(&gLocaleInfo, MSG_SHOW_NOTES_GAD)) + lborder + rborder;
    gHeight = barheight+fontheight*17+90+bborder;

    ngad.ng_LeftEdge = lborder+8;
    ngad.ng_TopEdge = barheight+fontheight+5;
    ngad.ng_Width = boxwidth;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_INTERACTIVE_MODE_GAD);
    ngad.ng_Flags = PLACETEXT_RIGHT;        // 1
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad,
			GTCB_Scaled,	TRUE,
			GTCB_Checked,	mp->mp_Flags & MPF_SCRIPTS,
            TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHOW_NOTES_GAD);
    ngad.ng_GadgetID++;                     // 2
    gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,mp->mp_Flags & MPF_NOTES,TAG_END);

    ngad.ng_LeftEdge = gWidth >> 1;
    ngad.ng_TopEdge -= fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_IMMUTABLE_GAD);
    ngad.ng_GadgetID++;                     // 3
    gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,mp->mp_Flags & MPF_READONLY,TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SHOW_HIDDEN_CELLS_GAD);
    ngad.ng_GadgetID++;                     // 4
    gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,mp->mp_Flags & MPF_SHOWHIDDEN,TAG_END);

    ngad.ng_LeftEdge = lborder+8;
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SAVE_WINDOW_SIZE_GAD);
    ngad.ng_GadgetID++;                     // 5
    gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,mp->mp_Flags & MPF_SAVEWINPOS,TAG_END);

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PROTECT_DOCUMENT_LABEL);
    ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
    ngad.ng_TopEdge += fontheight*2+15;
    ngad.ng_Width = gWidth-ngad.ng_LeftEdge-8-rborder;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID = 8;                   // 8
    if ((gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,mp->mp_Password ? "··········" : NULL,GTST_EditHook,&passwordEditHook,GTST_MaxChars,32,TAG_END)) != 0)
    {
        if ((gad->UserData = AllocPooled(pool,32)) && mp->mp_Password)
            strcpy(gad->UserData,mp->mp_Password);
    }

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_RETYPE_PASSWORD_LABEL);
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetID++;                     // 9
    if ((gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,mp->mp_Password ? "··········" : NULL,GTST_EditHook,&passwordEditHook,GTST_MaxChars,32,TAG_END)) != 0)
    {
        if ((gad->UserData = AllocPooled(pool,32)) && mp->mp_Password)
            strcpy(gad->UserData,mp->mp_Password);
    }

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PROTECT_SECURITY_WITH_PASSWORD_LABEL);
    ngad.ng_LeftEdge = 16+lborder+TLn(ngad.ng_GadgetText);
    ngad.ng_TopEdge += fontheight+8;
    ngad.ng_Width = gWidth-ngad.ng_LeftEdge-8-rborder;
    ngad.ng_GadgetID++;                     // 10
    if ((gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,mp->mp_CellPassword ? "··········" : NULL,GTST_EditHook,&passwordEditHook,GTST_MaxChars,32,GA_Disabled,mp->mp_Flags & MPF_CELLSLOCKED ? TRUE : FALSE,TAG_END)) != 0)
    {
        if ((gad->UserData = AllocPooled(pool,32)) && mp->mp_CellPassword)
            strcpy(gad->UserData,mp->mp_CellPassword);
    }
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_RETYPE_PASSWORD_LABEL);
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetID++;                     // 11
    if ((gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,mp->mp_CellPassword ? "··········" : NULL,GTST_EditHook,&passwordEditHook,GTST_MaxChars,32,GA_Disabled,mp->mp_Flags & MPF_CELLSLOCKED ? TRUE : FALSE,TAG_END)) != 0)
    {
        if ((gad->UserData = AllocPooled(pool,32)) && mp->mp_CellPassword)
            strcpy(gad->UserData,mp->mp_CellPassword);
    }
    ngad.ng_LeftEdge = 8+lborder;
    ngad.ng_TopEdge += fontheight*2+15;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_INPUT_ENDED_EVENT))+64;
    ngad.ng_Height = fontheight*6+4;
    ngad.ng_GadgetText = NULL;
    ngad.ng_UserData = (APTR)~0L;
    ngad.ng_GadgetID++;                     // 12
	gad = wd->wd_ExtData[4] = CreateGadget(LISTVIEW_KIND, gad, &ngad,
			GTLV_Labels,		wd->wd_ExtData[3],
			GTLV_ShowSelected,	NULL,
			GTLV_CallBack,		&selectHook,
            TAG_END);

    i = (ngad.ng_LeftEdge += 6+ngad.ng_Width);
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - TLn(gAppCommandLabels[0]) - boxwidth - 42 - rborder;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetID++;                     // 13
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, NULL, GA_Disabled, TRUE, GT_Underscore, '_', TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(gAppCommandLabels[0]) + 34;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID++;                     // 14
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &gAppCommandLabels, GA_Disabled, TRUE, TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetID++;                     // 15
    gad = CreatePopGadget(wd,gad,FALSE);

    ngad.ng_LeftEdge = i+lborder+TLn(GetString(&gLocaleInfo, MSG_INTERVAL_LABEL));
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_INTERVAL_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetID++;                     // 16
    gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,"1s",GA_Disabled,TRUE,TAG_END);

    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Width = TLn(ngad.ng_GadgetText)-linelen+42;
    ngad.ng_LeftEdge = (gWidth >> 1)-(ngad.ng_Width >> 1);
    ngad.ng_TopEdge = gHeight-bborder-fontheight-7;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;                     // 17
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreatePageGadgets(REG(a0, struct winData *wd))
{
    struct Page *page = wd->wd_Data;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 5; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 6; //Set Gadgetnumber 
#endif
	gWidth = TLn(GetString(&gLocaleInfo, MSG_FORE_BACKGROUND_COLOR_GAD)) + boxwidth * 6 + 14 + lborder + rborder;
	gHeight = barheight + fontheight * 4 + 31 + bborder;

	ngad.ng_LeftEdge = lborder + 8 + TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL));
	ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_LABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;         // 1
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, ((struct Node *)wd->wd_Data)->ln_Name, TAG_END);

	ngad.ng_LeftEdge = lborder + 8 + TLn(GetString(&gLocaleInfo, MSG_PROTECTION_PROPERTIES_LABEL));
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PROTECTION_PROPERTIES_LABEL);
    ngad.ng_GadgetID++;                     // 2
	gad = CreateGadget(CYCLE_KIND, gad, &ngad,
		GTCY_Labels,	&sPageProtectLabels,
		GTCY_Active,	(page->pg_Flags & PGF_IMMUTABLE) != 0 ? 1 : 0,
		GA_Disabled,	true,
        TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_LeftEdge = lborder+8+boxwidth*2+TLn(GetString(&gLocaleInfo, MSG_FORE_BACKGROUND_COLOR_GAD));
    ngad.ng_GadgetID++;                     // 3
    gad = CreatePopGadget(wd,gad,FALSE);

    ngad.ng_LeftEdge += boxwidth*3+6;
    ngad.ng_GadgetID++;                     // 4
    gad = CreatePopGadget(wd,gad,FALSE);

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+16;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;                     // 5
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_GadgetID++;                     // 6
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreateCellSizeGadgets(REG(a0, struct winData *wd))
{
    char t[32];
	int a, b;

	a = TLn(GetString(&gLocaleInfo, MSG_ADOPT_SIZE_FROM_CELL_UGAD)) + 32;
	b = 3 * TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD)) + 64;
	gWidth = max(a, b) + rborder + lborder;
	gHeight = barheight +fontheight * 6 + 39 + bborder;

	//wd->wd_ShortCuts = "bhozag";
	MakeShortCutString(wd->wd_ShortCuts, MSG_WIDTH_ULABEL, MSG_HEIGHT_ULABEL, MSG_OK_UGAD,
		MSG_ASSIGN_UGAD, MSG_CANCEL_UGAD, MSG_ADOPT_SIZE_FROM_CELL_UGAD, TAG_END);

	a = TLn(GetString(&gLocaleInfo, MSG_WIDTH_LABEL));
	b = TLn(GetString(&gLocaleInfo, MSG_HEIGHT_LABEL));

	ngad.ng_LeftEdge = 8 + lborder + max(a, b);
	ngad.ng_TopEdge = barheight + 3;
	ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_WIDTH_ULABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;         // 1
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sCellSizeLabels, GT_Underscore, '_', TAG_END);

    ngad.ng_TopEdge += fontheight+4;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID++;                     // 2
    strcpy(t,ita(((struct Page *)wd->wd_Data)->pg_mmStdWidth/1024.0,2,FALSE));
    strcat(t," mm");
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, t, GA_Disabled, TRUE, GT_Underscore, '_', TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_HEIGHT_ULABEL);
    ngad.ng_GadgetID++;                     // 3
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &sCellSizeLabels, GT_Underscore, '_', TAG_END);

    ngad.ng_TopEdge += fontheight+4;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID++;                     // 4
    strcpy(t,ita(((struct Page *)wd->wd_Data)->pg_mmStdHeight/1024.0,2,FALSE));
    strcat(t," mm");
    gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',GTST_String,t,GA_Disabled,TRUE,TAG_END);

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width = gWidth-rborder-lborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ADOPT_SIZE_FROM_CELL_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID = wd->wd_ShortCuts[5];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[2];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = (gWidth-ngad.ng_Width) >> 1;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ASSIGN_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[3];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[4];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void
CreateNotesGadgets(struct winData *wd, long wid, long hei)
{
    STRPTR t = NULL;
	long rows = (hei-barheight-2*fontheight-24 - leftImg->Height) / fontheight;

	//wd->wd_ShortCuts = "nola";
	MakeShortCutString(wd->wd_ShortCuts, MSG_NOTE_UGAD, MSG_OK_UGAD, MSG_DELETE_UGAD, MSG_CANCEL_UGAD, TAG_END);
		// ToDo: MSG_NOTE_UGAD doesn't have any visible label, and is only defined for this shortcut

    ngad.ng_TopEdge = barheight+3;

    if (rxpage && rxpage->pg_Gad.tf)
        t = rxpage->pg_Gad.tf->tf_Note;

#ifdef __amigaos4__
	gad = NewObj(wd, WOT_GADGET, TEXTEDITOR_GetClass(), "editorgadget", 
			GA_Left,      lborder,
            GA_Top,       ngad.ng_TopEdge,
			GA_Width,     wid - rborder - lborder,
			GA_Height,    fontheight * rows + 4,
            GA_DrawInfo,  dri,
            GA_Previous,  gad,
//          EGA_Scroller, TRUE,
          	GA_TEXTEDITOR_Contents,     t,
            GA_ID,        1,
            TAG_END);
#else
	gad = NewObj(wd, WOT_GADGET, NULL, "pinc-editgadget",
			GA_Left,      lborder,
            GA_Top,       ngad.ng_TopEdge,
			GA_Width,     wid - rborder - lborder,
			GA_Height,    fontheight * rows + 4,
            GA_DrawInfo,  dri,
            GA_Previous,  gad,
            EGA_Scroller, TRUE,
            EGA_Text,     t,
            GA_ID,        1,
            TAG_END);
#endif
    wd->wd_ExtData[1] = gad;

	ngad.ng_LeftEdge = wid - boxwidth - rborder;
	ngad.ng_TopEdge += rows * fontheight + 7;
    ngad.ng_GadgetID = 2;                     // 2
    gad = CreatePopGadget(wd,gad,FALSE);

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = hei - fontheight - leftImg->Height - 7;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID = wd->wd_ShortCuts[1];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = (wid-ngad.ng_Width) >> 1;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[2];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = wid-rborder-ngad.ng_Width;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[3];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreateZoomGadgets(REG(a0, struct winData *wd))
{
    char t[32];

    gWidth = TLn(GetString(&gLocaleInfo, MSG_ZOOM_LABEL))+TLn("1000.0%")+48+boxwidth+lborder+rborder;
    gHeight = barheight+fontheight+10+bborder;

    ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_ZOOM_LABEL));
    ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge-boxwidth;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ZOOM_ULABEL);
    ngad.ng_Flags = PLACETEXT_LEFT;         // 1
    if (rxpage)
        ProcentToString(rxpage->pg_Zoom,t);
    else
        strcpy(t,"-");
    gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,t,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetID++;                     // 2
    gad = CreatePopGadget(wd,gad,FALSE);
}


void
CreateGClassesGadgets(struct winData *wd, long wid, long hei)
{
    long h = (hei-barheight-fontheight-17-leftImg->Height)/itemheight;

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = barheight+3;
    ngad.ng_Width = wid-lborder-rborder;
    ngad.ng_Height = h*itemheight+4;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID = 1;                   // 1
    gad = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,&gclasses,GTLV_CallBack,&renderHook,GTLV_MaxPen,7,GTLV_ItemHeight,itemheight,GTLV_ShowSelected,NULL,TAG_END);

    ngad.ng_TopEdge += ngad.ng_Height+3;
    ngad.ng_Width = boxwidth;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_KEEP_WINDOWS_OPEN_GAD);
    ngad.ng_Flags = PLACETEXT_RIGHT;
    ngad.ng_GadgetID++;                     // 2
    gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GT_Underscore,'_',TAG_END);
}


void ASM
CreateObjectGadgets(REG(a0, struct winData *wd))
{
    struct gObject *go = wd->wd_Data;
    BYTE   linemode = go->go_Flags & GOF_LINEMODE;
    long   lines = (long)wd->wd_ExtData[2],w,h;
    struct gInterface *gi;

    {
        long   gw;
        STRPTR s;

        gWidth = 0;
        for(gi = go->go_Class->gc_Interface;gi && gi->gi_Tag;gi++)
        {
            if (!(s = (STRPTR)GetGLabel(gi)))
                continue;

            gw = TLn(s);
            switch(gi->gi_Type)
            {
                case GIT_PEN:
                case GIT_WEIGHT:
                    gw += 3*boxwidth;
                    break;
                case GIT_CYCLE:
                case GIT_CHECKBOX:
                    gw += boxwidth;
                    break;
            }
            if (gWidth < gw)
                gWidth = gw;
        }
    }
	gWidth = gWidth*2 + 114 + lborder + rborder;
    if (lines)
        w = gWidth >> 1;
    else
    {
		gWidth = (gWidth >> 1) + 4 + rborder + lborder;
		gWidth = max(220, gWidth);
		w = gWidth + 4 - rborder;
    }

    wd->wd_ExtData[3] = (APTR)w;

    ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_LeftEdge = TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL)) + 8 + lborder;
	ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_LABEL);
	gad = CreateGadget(STRING_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
	AddObjectGadget(wd->wd_ExtData[4], gad, GOA_Name, GIT_TEXT, STRING_KIND);

	h = (ngad.ng_TopEdge += 2*fontheight + 9);
    if (lines > 4)
		ngad.ng_TopEdge += ((lines - 4) * (fontheight + 7)) >> 1;
	ngad.ng_LeftEdge = 16 + lborder + (linemode ? TLn(GetString(&gLocaleInfo, MSG_BEGIN_FROM_LEFT_LABEL)) : TLn(GetString(&gLocaleInfo, MSG_FROM_LEFT_LABEL)));
	ngad.ng_Width = w - 12 - ngad.ng_LeftEdge;
    ngad.ng_GadgetText = linemode ? GetString(&gLocaleInfo, MSG_BEGIN_FROM_LEFT_LABEL) : GetString(&gLocaleInfo, MSG_FROM_LEFT_LABEL);
    ngad.ng_GadgetID++;                   // 2
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FROM_TOP_LABEL);
    ngad.ng_GadgetID++;                   // 3
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
    ngad.ng_GadgetText = linemode ? GetString(&gLocaleInfo, MSG_END_FROM_LEFT_LABEL) : GetString(&gLocaleInfo, MSG_WIDTH_LABEL);
    ngad.ng_GadgetID++;                   // 4
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
    ngad.ng_GadgetText = linemode ? GetString(&gLocaleInfo, MSG_FROM_TOP_LABEL) : GetString(&gLocaleInfo, MSG_HEIGHT_LABEL);
    ngad.ng_GadgetID++;                   // 5
    gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',TAG_END);

    ngad.ng_TopEdge = h;
    if (lines < 4)
        ngad.ng_TopEdge += ((4-lines)*(fontheight+7)) >> 1;

	for (gi = go->go_Class->gc_Interface; gi && gi->gi_Tag; gi++)
    {
		switch (gi->gi_Type)
        {
            case GIT_PEN:
                ngad.ng_LeftEdge = gWidth-8-rborder-boxwidth;
                ngad.ng_GadgetID++;
                gad = CreatePopGadget(wd,gad,FALSE);
				AddObjectGadget(wd->wd_ExtData[4], gad, gi->gi_Tag, GIT_PEN, POPUP_KIND);
                break;
            case GIT_WEIGHT:
                ngad.ng_GadgetText = (STRPTR)GetGLabel(gi);
				ngad.ng_LeftEdge = w + TLn(ngad.ng_GadgetText) + 16;
				ngad.ng_Width = gWidth - 8 - rborder - boxwidth - ngad.ng_LeftEdge;
                ngad.ng_GadgetID++;
				gad = CreateGadget(STRING_KIND, gad, &ngad, STRINGA_Justification, GACT_STRINGRIGHT, GT_Underscore, '_', TAG_END);
				AddObjectGadget(wd->wd_ExtData[4], gad, gi->gi_Tag, GIT_WEIGHT, STRING_KIND);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
				gad = CreatePopGadget(wd, gad, FALSE);
				AddObjectGadget(wd->wd_ExtData[4], gad, gi->gi_Tag, GIT_WEIGHT, POPUP_KIND);
                break;
            case GIT_CYCLE:
                if (!gi->gi_Special)
                    break;

                ngad.ng_GadgetText = (STRPTR)GetGLabel(gi);
                ngad.ng_LeftEdge = w+TLn(ngad.ng_GadgetText)+16;
                ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
                ngad.ng_GadgetID++;

                gad = CreateGadget(CYCLE_KIND,gad,&ngad,GTCY_Labels,gi->gi_Special,GT_Underscore,'_',TAG_END);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_CYCLE,CYCLE_KIND);
                break;
            case GIT_CHECKBOX:
                ngad.ng_GadgetText = (STRPTR)GetGLabel(gi);
                ngad.ng_LeftEdge = gWidth-8-rborder-boxwidth;
                ngad.ng_Width = boxwidth;
                ngad.ng_GadgetID++;
                gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,FALSE,TAG_END);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_CHECKBOX,CHECKBOX_KIND);
                break;
            default:
                ngad.ng_TopEdge -= fontheight+7;
        }
        ngad.ng_TopEdge += fontheight+7;
    }

    ngad.ng_TopEdge = h+max(lines,4)*(fontheight+7)+5;

    for(gi = go->go_Class->gc_Interface;gi && gi->gi_Tag;gi++)
    {
        ngad.ng_GadgetText = (STRPTR)GetGLabel(gi);
        switch(gi->gi_Type)
        {
            case GIT_BUTTON:
                ngad.ng_GadgetText = (STRPTR)GetGLabel(gi);
                ngad.ng_LeftEdge = lborder;
                ngad.ng_Width = gWidth-lborder-rborder;
                ngad.ng_Flags = PLACETEXT_IN;
                ngad.ng_GadgetID++;
                gad = CreateGadget(BUTTON_KIND,gad,&ngad,TAG_END);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_BUTTON,BUTTON_KIND);
                ngad.ng_Flags = PLACETEXT_LEFT;

                ngad.ng_TopEdge += fontheight+7;
                break;
            case GIT_FILENAME:
                ngad.ng_LeftEdge = 8+lborder+TLn(ngad.ng_GadgetText);
                ngad.ng_Width = gWidth-rborder-boxwidth-ngad.ng_LeftEdge;
                ngad.ng_GadgetID++;
                gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_FILENAME,STRING_KIND);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
                gad = CreatePopGadget(wd,gad,FALSE);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_FILENAME,POPUP_KIND);

                ngad.ng_TopEdge += fontheight+7;
                break;
            case GIT_TEXT:
            case GIT_FORMULA:
                if (gi->gi_Tag == GOA_Help)
                {
                    ngad.ng_LeftEdge = 8+lborder+TLn(ngad.ng_GadgetText);
                    ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge;
                    ngad.ng_GadgetID++;
                    gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
                    AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_TEXT,STRING_KIND);

                    ngad.ng_TopEdge += fontheight+7;
                }
                else if (gi->gi_Tag == GOA_Command)
                {
					ngad.ng_LeftEdge = 8 + lborder + TLn(ngad.ng_GadgetText);
					ngad.ng_Width = gWidth - rborder - boxwidth - ngad.ng_LeftEdge - TLn(gAppCommandLabels[0]) - 42;
                    ngad.ng_GadgetID++;
                    gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
                    AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_TEXT,STRING_KIND);

                    ngad.ng_LeftEdge += ngad.ng_Width;
					ngad.ng_Width = TLn(gAppCommandLabels[0]) + 42;
                    ngad.ng_GadgetText = NULL;
                    ngad.ng_GadgetID++;
					gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &gAppCommandLabels, TAG_END);
					AddObjectGadget(wd->wd_ExtData[4], gad, gi->gi_Tag, GIT_NONE, CYCLE_KIND);

                    ngad.ng_LeftEdge += ngad.ng_Width;
                    ngad.ng_GadgetID++;
                    gad = CreatePopGadget(wd,gad,FALSE);
                    AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_TEXT,POPUP_KIND);
                    ngad.ng_TopEdge += fontheight+7;
                }
                else
                {
                    ngad.ng_LeftEdge = 8+lborder+TLn(ngad.ng_GadgetText);
                    ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge;
                    ngad.ng_GadgetID++;
                    gad = CreateGadget(STRING_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
                    AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_TEXT,STRING_KIND);
                    ngad.ng_TopEdge += fontheight+7;
                }
                break;
            case GIT_FONT:
                ngad.ng_LeftEdge = 8+lborder+TLn(ngad.ng_GadgetText);
                ngad.ng_Width = gWidth-40-rborder-TLn(GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL))-TLn("999 pt")-2*boxwidth-ngad.ng_LeftEdge;
                ngad.ng_GadgetID++;
                gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GT_Underscore,'_',TAG_END);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_FONT,TEXT_KIND);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
                gad = CreatePopGadget(wd,gad,FALSE);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_FONT,POPUP_KIND);

                ngad.ng_LeftEdge += boxwidth+14+TLn(GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL));
                ngad.ng_Width = gWidth-rborder-boxwidth-ngad.ng_LeftEdge;
                ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_SIZE_LABEL);
                ngad.ng_GadgetID++;
                gad = CreateGadget(STRING_KIND,gad,&ngad,STRINGA_Justification,GACT_STRINGRIGHT,GT_Underscore,'_',TAG_END);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_FONT,STRING_KIND);

                ngad.ng_LeftEdge += ngad.ng_Width;
                ngad.ng_GadgetID++;
                gad = CreatePopGadget(wd,gad,FALSE);
                AddObjectGadget(wd->wd_ExtData[4],gad,gi->gi_Tag,GIT_FONT,POPUPPOINTS_KIND);
                ngad.ng_TopEdge += fontheight+7;
                break;
        }
    }
	gHeight = ngad.ng_TopEdge + bborder;
}


void ASM
CreateCommandGadgets(REG(a0, struct winData *wd))
{
	gWidth = TLn(GetString(&gLocaleInfo, MSG_COMMAND_LABEL))
		+ 2 * TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))
		+ TLn(gAppCommandLabels[0]) + boxwidth + 160 + lborder + rborder;
	gHeight = barheight + fontheight * 2 + 18 + bborder;

	//wd->wd_ShortCuts = "boa";
	MakeShortCutString(wd->wd_ShortCuts, MSG_COMMAND_ULABEL, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);

    ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_COMMAND_LABEL));
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - TLn(gAppCommandLabels[0]) - boxwidth - 42 - lborder;
	ngad.ng_Height = fontheight + 4;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_COMMAND_ULABEL);         // 1
    gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,wdt_cmdstr,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(gAppCommandLabels[0]) + 42;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID++;                      // 2
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &gAppCommandLabels, TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetID++;                      // 3
    gad = CreatePopGadget(wd,gad,FALSE);

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge += fontheight+8;
    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+16;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID = wd->wd_ShortCuts[1];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
    ngad.ng_GadgetID = wd->wd_ShortCuts[2];
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}

void ASM
CreateFileTypeGadgets(REG(a0, struct winData *wd))
{
	struct IOTypeLink *iol;
    struct IOType *io;
    long   i = -1;
    struct Node *n;

	gWidth = scr->Width / 3;
	gHeight = barheight + 17 * fontheight + 35 + bborder;

	io = ((struct Page *)wd->wd_Data)->pg_Mappe->mp_FileType;

	foreach (wd->wd_ExtData[1], iol) {
        i++;
		if (io == NULL || iol->iol_Link == io)
            break;
    }
    
	// if no type is set yet, use the one with the highest priority
	if (io == NULL) 
		io = iol->iol_Link;
	wd->wd_ExtData[2] = iol;
	
#ifdef __amigaos4__
	iol = (APTR)((struct List *)wd->wd_ExtData[1])->lh_Head; //Wieder auf startwert setzen, sonst zeigt der Zeiger in die irre...
	wd->wd_ExtData[0] = io = iol->iol_Link;
#endif
	
    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge += fontheight+6;
    ngad.ng_Width = gWidth-lborder-rborder;
    ngad.ng_Height = 8*fontheight+4;
    ngad.ng_GadgetText = wd->wd_ExtData[0] ? GetString(&gLocaleInfo, MSG_AVAILABLE_LOAD_MODULES_GAD) : GetString(&gLocaleInfo, MSG_AVAILABLE_SAVE_MODULES_GAD);
    ngad.ng_Flags = PLACETEXT_ABOVE;                     // 1
#ifdef __amigaos4__
	gad = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, wd->wd_ExtData[1], GTLV_ShowSelected, NULL, GTLV_Selected, 0, TAG_END);
#else
	gad = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, wd->wd_ExtData[1], GTLV_ShowSelected, NULL, GTLV_Selected, i, TAG_END);
    wd->wd_ExtData[0] = (APTR)((struct Page *)wd->wd_Data)->pg_Mappe->mp_FileType;
#endif

	ngad.ng_TopEdge += fontheight * 9 + 12;
	ngad.ng_Height = 6 * fontheight + 4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NOTES_GAD);
    ngad.ng_GadgetID++;                                  // 2
	gad = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, (i != ~0L ? &iol->iol_Description : NULL), GTLV_ReadOnly, TRUE, TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height + 3;
	ngad.ng_Height = fontheight + 4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PREFERENCES_GAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GA_Disabled, i == ~0L || !(io->io_Flags & IOF_HASPREFSGUI), TAG_END);
}


void ASM
CreateSetNameGadgets(REG(a0, struct winData *wd))
{
    gWidth = scr->Width >> 1;
    gHeight = barheight+fontheight+10+bborder;

    ngad.ng_LeftEdge = lborder+8+TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL));
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - rborder;
    ngad.ng_Height = fontheight+4;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_LABEL);           // 1
    gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,NULL,GT_Underscore,'_',TAG_END);
}


void
CreateScriptsGadgets(struct winData *wd, long wid, long hei)
{
    struct Mappe *mp = wd->wd_Data;
	long   w, top, h;
    long   rows = (hei-barheight-2*fontheight-21 - leftImg->Height) / fontheight;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 6; //Set Gadgetnumber 
#endif
    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = barheight+3;
    ngad.ng_Width = 2*TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+32;
    ngad.ng_Height = fontheight*rows+4;
    ngad.ng_GadgetText = NULL;               // 1
    ngad.ng_GadgetID = 1;
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND,gad, &ngad, GTLV_Labels, &mp->mp_RexxScripts, GTLV_ShowSelected, NULL, TAG_END);
	gad->UserData = NULL;
		// pointer to the currently edited script

    top = ngad.ng_TopEdge += ngad.ng_Height;
    ngad.ng_Width >>= 1;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;                      // 2
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
    ngad.ng_GadgetID++;                      // 3
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

    w = ngad.ng_LeftEdge+ngad.ng_Width+6;

	ngad.ng_LeftEdge = w + 8 + TLn(GetString(&gLocaleInfo, MSG_DESCRIPTION_LABEL));
	ngad.ng_TopEdge = barheight + 3;
	ngad.ng_Width = wid - rborder - ngad.ng_LeftEdge;
    ngad.ng_Flags = PLACETEXT_LEFT;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_LABEL);
    ngad.ng_GadgetID++;                      // 4
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, NULL, GT_Underscore, '_', TAG_END);

    ngad.ng_TopEdge += fontheight+7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DESCRIPTION_LABEL);
    ngad.ng_GadgetID++;                      // 5
    gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,NULL,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

    h = ((top-ngad.ng_TopEdge-fontheight-11)/fontheight)*fontheight+4;

    ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_OK_UGAD))+42;
    ngad.ng_LeftEdge = (wid >> 1)-(ngad.ng_Width >> 1);
    ngad.ng_TopEdge = hei - fontheight - leftImg->Height - 7;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID++;                      // 6
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

    ngad.ng_LeftEdge = w;
    ngad.ng_TopEdge = top;
    ngad.ng_Width = wid-w-rborder;
    ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OPEN_IN_EDITOR_GAD);
    ngad.ng_GadgetID++;                      // 7
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,TRUE,TAG_END);

#ifdef __amigaos4__
	gad = NewObj(wd, WOT_GADGET, TEXTEDITOR_GetClass(), "editorgadget", GA_Left,      w,
                                                          				GA_Top,       top-h,
                                                          				GA_Width,     wid-w-rborder,
                                                          				GA_Height,    h,
                                                          				GA_DrawInfo,  dri,
                                                          				GA_Previous,  gad,
                                                          				GA_Disabled,  TRUE,
                                                          				GA_ID,        8,
                                                          				TAG_END);
#else
	gad = NewObj(wd, WOT_GADGET, NULL, "pinc-editgadget", GA_Left,      w,
                                                          GA_Top,       top-h,
                                                          GA_Width,     wid-w-rborder,
                                                          GA_Height,    h,
                                                          GA_DrawInfo,  dri,
                                                          GA_Previous,  gad,
                                                          EGA_Scroller, TRUE,
                                                          //EGA_ShowControls,TRUE,
                                                          //EGA_AutoIndent,TRUE,
                                                          GA_Disabled,  TRUE,
                                                          GA_ID,        8,
                                                          TAG_END);
#endif
    wd->wd_ExtData[1] = gad;
}

 
void
InitGadgetLabels(void)
{
	MakeLocaleLabels(gAppCommandLabels, MSG_NORMAL_COMMAND_GAD, MSG_INTERNAL_COMMAND_GAD, TAG_END);
	//MakeLocaleLabels(gShortAppCommandLabels, "n", "i");

	MakeLocaleLabels(gDatabaseFieldLabels, MSG_DBASE_FIELD_STANDARD_GAD, MSG_DBASE_FIELD_REFERENCE_GAD, MSG_DBASE_FIELD_ENUM_GAD, MSG_DBASE_FIELD_COUNTER_GAD, TAG_END);

	MakeLocaleLabels(sFrameLabels, MSG_FRAME_LEFT_GAD, MSG_FRAME_RIGHT_GAD, MSG_FRAME_BOTTOM_GAD, MSG_FRAME_TOP_GAD, TAG_END);

	// WDT_CELL
	MakeLocaleLabels(sAlignLabels, MSG_KEEP_GAD, MSG_ALIGN_REMOVE_GAD, MSG_ALIGN_LEFT_GAD,
		MSG_ALIGN_CENTER_GAD, MSG_ALIGN_RIGHT_GAD, TAG_END);
	MakeLocaleLabels(sVerticalAlignLabels, MSG_KEEP_GAD, MSG_VALIGN_TOP_GAD, MSG_VALIGN_MIDDLE_GAD, MSG_VALIGN_BOTTOM_GAD, TAG_END);
	MakeLocaleLabels(sKerningLabels, MSG_NO_KERNING_GAD, MSG_TEXT_KERNING_GAD, MSG_DESIGN_KERNING_GAD, TAG_END);
	MakeLocaleLabels(sSecurityLabels, MSG_KEEP_GAD, MSG_NO_SECURITY_GAD, MSG_NOT_EDITABLE_GAD,
		MSG_HIDE_FORMULA_GAD, MSG_HIDE_CONTENTS_GAD, TAG_END);
	MakeLocaleLabels(sCellPageLabels, MSG_CELL_FORMAT_BORDER, MSG_CELL_COLOR_ALIGNMENT_BORDER,
		MSG_CELL_FONT_BORDER, MSG_MISC_BORDER, TAG_END);

	// WDT_DIAGRAM
	MakeLocaleLabels(sOrderLabels, MSG_ORDER_VERTICAL_GAD, MSG_ORDER_HORIZONTAL_GAD, TAG_END);
	MakeLocaleLabels(sDiagramPageLabels, MSG_DIAGRAM_CONTENTS_BORDER, MSG_DIAGRAM_TYPE_BORDER,
		MSG_DIAGRAM_DISPLAY_BORDER, MSG_DIAGRAM_VALUES_BORDER, MSG_DIAGRAM_AXES_BORDER, TAG_END);
	MakeLocaleLabels(sAxesGridLabels, MSG_NO_AXES_GRID_GAD, MSG_POINTS_AXES_GRID_GAD, MSG_LINE_AXES_GRID, TAG_END);

	MakeLocaleLabels(sPageLimitLabels, MSG_NO_PAGE_LIMITS_GAD, MSG_MARKED_PAGE_LIMITS_GAD, MSG_FIXED_PAGE_LIMITS_GAD, TAG_END);
	pageLabels[5] = GetString(&gLocaleInfo, MSG_OTHER_PAGE_SIZE_GAD);
																																
	// ToDo: the commented protection mode is not yet implemented!
	MakeLocaleLabels(sPageProtectLabels, MSG_NO_PAGE_PROTECTION_GAD, MSG_PAGE_IMMUTABLE_GAD /*, MSG_PAGE_HIDDEN_GAD*/, TAG_END);

	MakeLocaleLabels(sCellSizeLabels, MSG_KEEP_CELL_SIZE_GAD, MSG_EXACT_CELL_SIZE_GAD,
		MSG_MINIMAL_CELL_SIZE_GAD, MSG_OPTIMAL_CELL_SIZE_GAD, TAG_END);

	//MakeLocaleLabels(sEdgeLabels, "standard", "rund", "schräg", TAG_END);
	//MakeLocaleLabels(sEndsLabels, "standard", "rund", "Pfeil", TAG_END);
}
