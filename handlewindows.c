/* window-handling routines
 *
 * Copyright 1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include <gadgets/texteditor.h>
#endif

extern void CreateGClassesGadgets(struct winData *wd, long wid, long hei);
extern void CreateScriptsGadgets(struct winData *wd, long wid, long hei);
extern void CreateNotesGadgets(struct winData *wd, long wid, long hei);

extern ULONG weightvalues[];
extern UWORD kPredefinedFrames[];
extern STRPTR wdt_cmdstr;


void PUBLIC
RxMapLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags))
{
    if (!rxpage)
		CloseAppWindow(ln->ln_Data[0], TRUE);
}


static void
GT_SetString(struct Gadget *gad, STRPTR *string)
{
    STRPTR t;

	if (GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END)) {
        FreeString(*string);
        *string = AllocString(t);
    }
}


void ASM
CloseBorderWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

	if (clean) {
		FreePooled(pool, wd->wd_ExtData[0], sizeof(struct wdtBorder));
		FreePooled(pool, wd->wd_ExtData[1], sizeof(struct wdtBorder));
		FreePooled(pool, wd->wd_ExtData[2], sizeof(struct wdtBorder));
		FreePooled(pool, wd->wd_ExtData[3], sizeof(struct wdtBorder));
    }
}

#define wbwd(x) ((struct wdtBorder *)wd->wd_ExtData[x])

static void
updateBorderGadgets(struct wdtBorder *wb)
{
    char tt[20];

    if (!wb)
        return;

    GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTCB_Checked,wb->wb_Adopt,TAG_END);
    sprintf(tt,"%s pt",ita(wb->wb_Point/65536.0,-3,FALSE));
    GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GTST_String,tt,TAG_END);
    DrawColorField(win->RPort,GadgetAddress(win,3),wb->wb_Color,TRUE);
}


void ASM
HandleBorderIDCMP(REG(a0, struct TagItem *tag))
{
    struct wdtBorder *wb;
    struct Node *ln;
    char   *t,tt[20];
    long   i,j,id;

    wb = (struct wdtBorder *)wd->wd_ExtData[(long)wd->wd_Data];
	switch (imsg.Class) {
        case IDCMP_GADGETDOWN:
			switch ((gad = (struct Gadget *)imsg.IAddress)->GadgetID) {
                case 3:
					i = PopUpList(win, gad, &colors,
							POPA_CallBack,	&colorHook,
							POPA_MaxPen,	~0L,
							POPA_ItemHeight,fontheight + 4,
							POPA_Width,		GetListWidth(&colors) + boxwidth + 5,
                            TAG_END);
                    if (i != ~0L)
                    {
                        for(j = 0,ln = (struct Node *)colors.mlh_Head;j < i;j++,ln = ln->ln_Succ);
                        wb->wb_Color = ((struct colorPen *)ln)->cp_ID;
                        wb->wb_Adopt = TRUE;
                        DrawColorField(win->RPort,gad,wb->wb_Color,TRUE);
						GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTCB_Checked, TRUE, TAG_END);
                    }
                    break;
                case 5:
                case 6:
                    wd->wd_ExtData[4] = (APTR)((long)gad->GadgetID);
                    wd->wd_ExtData[5] = NULL;
                    break;
            }
            break;
        case IDCMP_GADGETUP:
			switch (id = (gad = (struct Gadget *)imsg.IAddress)->GadgetID) {
                case 1:   // Border-Cycle
                    wd->wd_Data = (APTR)((long)imsg.Code);
                    wb = (struct wdtBorder *)wd->wd_ExtData[imsg.Code];
                    updateBorderGadgets(wb);
                /*  GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTCB_Checked,wb->wb_Adopt,TAG_END);
                    sprintf(tt,"%s pt",ita(wb->wb_Point/65536.0,-3,FALSE));
                    GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GTST_String,tt,TAG_END);
                    DrawColorField(win->RPort,GadgetAddress(win,3),wb->wb_Color,TRUE);*/
                    break;
                case 2:   // Übernehmen?
                    wb->wb_Adopt = (BOOL)imsg.Code;
                    break;
                case 4:   // Stärke
                    wb->wb_Adopt = TRUE;
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
                    if (t)
                        wb->wb_Point = (long)(ConvertNumber(t,CNT_POINT)*65536.0);
                    GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTCB_Checked,TRUE,TAG_END);
                    break;
                case 5:   // -
                case 6:   // +
                    if (!wd->wd_ExtData[5])
                        goto border_tooshortforintuiticks;
                    break;
                case 10:  // FramesGadget
					if (imsg.Code != 0xffff) {
						bool addOnly;
						long point;

                        if (IsDoubleClick(imsg.Code))
                            goto border_doubleclick;

						addOnly = GetCheckBoxFlag(GadgetAddress(win, 11), win, TRUE) && imsg.Code != 0;
							// if "add only" is set, the first default frame (the empty frame)
							// will still remove all frames for convenience

						for (i = 0; i < 4; i++) {
                            wb = (struct wdtBorder *)wd->wd_ExtData[i];
							point = ((kPredefinedFrames[imsg.Code] & (0x3 << i*2)) >> i*2) << 16;
							if (!addOnly || point)
                            {
                                wb->wb_Point = point;
                                wb->wb_Adopt = TRUE;
								wb->wb_Color = kPredefinedFrames[imsg.Code] & (0x100 << i) ? FindColorPen(255,255,255) : FindColorPen(0,0,0);
                            }
                            else
                                wb->wb_Adopt = FALSE;
                        }
                        updateBorderGadgets((struct wdtBorder *)wd->wd_ExtData[(long)wd->wd_Data]);
                    }
                    break;
            }
        case IDCMP_VANILLAKEY:
            if (imsg.Class == IDCMP_GADGETUP)
                id = gad->GadgetID;
            else
            {
                id = imsg.Code;
                gad = NULL;
            }
            if (id == wd->wd_ShortCuts[1] || id == wd->wd_ShortCuts[4]) // "Ok" und "Zuweisen"
            {
                long border = 0;

                border_doubleclick: /********************************************************/

                GT_GetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[0]),win,NULL,GTCB_Checked,&border,TAG_END);

                SetBorder(rxpage, border,wbwd(0)->wb_Adopt ? wbwd(0)->wb_Color : ~0L,wbwd(0)->wb_Point >> 11, wbwd(1)->wb_Adopt ? wbwd(1)->wb_Color : ~0L,wbwd(1)->wb_Point >> 11,wbwd(2)->wb_Adopt ? wbwd(2)->wb_Color : ~0L,wbwd(2)->wb_Point >> 11,wbwd(3)->wb_Adopt ? wbwd(3)->wb_Color : ~0L,wbwd(3)->wb_Point >> 11);
                if (id == wd->wd_ShortCuts[1])  // Fenster schließen bei "Ok", nicht bei Doppelklick oder "Zuweisen"
                    CloseAppWindow(win,TRUE);
            }
            else if (id == wd->wd_ShortCuts[2] || id == ESCAPE_KEY)  // Abbrechen
                CloseAppWindow(win,TRUE);
            else if (!gad && id == wd->wd_ShortCuts[0])  // Block umrahmen
            {
                long border = 0;

				GT_GetGadgetAttrs(gad = GadgetAddress(win, wd->wd_ShortCuts[0]), win, NULL, GTCB_Checked, &border, TAG_END);
				GT_SetGadgetAttrs(gad, win, NULL, GTCB_Checked, !border, TAG_END);
            }
            else if (id == wd->wd_ShortCuts[3])  // Rahmen von aktueller Zelle übernehmen
            {
                if (rxpage && rxpage->pg_Gad.DispPos > PGS_NONE)
                {
                    struct tableField *tf = rxpage->pg_Gad.tf;

                    for(i = 0;i < 4;i++)
                    {
                        wb = (struct wdtBorder *)wd->wd_ExtData[i];
                        wb->wb_Adopt = TRUE;

                        if (tf)
                        {
                            wb->wb_Point = tf->tf_Border[i] << 11;
                            wb->wb_Color = tf->tf_BorderColor[i];
                        }
                        else
                        {
                            wb->wb_Point = 0;
                            wb->wb_Color = 0;
                        }
                    }
                    updateBorderGadgets((struct wdtBorder *)wd->wd_ExtData[(long)wd->wd_Data]);
                }
            }
            break;
        case IDCMP_INTUITICKS:
            if (wd->wd_ExtData[4])
            {
                if ((gad = GadgetAddress(win,(long)wd->wd_ExtData[4])) && gad->Flags & GFLG_SELECTED)
                {
                    border_tooshortforintuiticks: /********************************************************/
                    if (!wb->wb_Adopt)
                    {
                        wb->wb_Adopt = TRUE;
                        GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTCB_Checked,TRUE,TAG_END);
                    }
                    wb->wb_Point += gad->GadgetID == 5 ? -65536 : 65536;
                    if (wb->wb_Point < 0)
                        wb->wb_Point = 0;
                    if (wb->wb_Point > (7 << 16))
                        wb->wb_Point = 7 << 16;
                    sprintf(tt,"%s pt",ita(wb->wb_Point/65536.0,-3,FALSE));
                    wd->wd_ExtData[5] = (APTR)TRUE;
                    GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GTST_String,tt,TAG_END);
                }
                else
                    wd->wd_ExtData[4] = NULL;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
CloseFormelWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    if (clean)
		FreeStringList(wd->wd_ExtData[2]);
}


void ASM
HandleFormelIDCMP(REG(a0, struct TagItem *tag))
{
    struct Function *f;
    struct Node *ln;
    long   i;

    switch(imsg.Class)
    {
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
        case IDCMP_GADGETUP:
            switch(((struct Gadget *)imsg.IAddress)->GadgetID)
            {
                case 1:
                    fewftype = imsg.Code;
                    GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,~0L,TAG_END);
                    MakeFewFuncs();
                    GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,&fewfuncs,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTTX_Text,NULL,TAG_END);
                    break;
                case 2:
                    for(ln = (struct Node *)fewfuncs.mlh_Head,i = 0;i < imsg.Code;i++,ln = ln->ln_Succ);
                    ((struct Gadget *)wd->wd_ExtData[1])->UserData = ln;
                    if ((f = (struct Function *)MyFindName(&funcs, ln->ln_Name)) != 0)
                        GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTTX_Text,f->f_Help,TAG_END);
                    break;
                case 4:
                    if ((gad = wd->wd_ExtData[1])->UserData)
                        insertFormel(rxpage,((struct Node *)gad->UserData)->ln_Name);
                    CloseAppWindow(win,TRUE);
                    break;
                case 5:
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;
    }
}


void ASM
HandlePageSetupIDCMP(REG(a0, struct TagItem *tag))
{
    struct Mappe *mp = (struct Mappe *)wd->wd_Data;

    switch(imsg.Class)
    {
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
            {
                case 1:
                    if (imsg.Code < pageSizes)
                    {
                        char t[16];

                        strcpy(t,ita(pageWidth[imsg.Code]/10240.0,2,FALSE));
                        strcat(t," cm");
                        GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTST_String,t,TAG_END);
                        strcpy(t,ita(pageHeight[imsg.Code]/10240.0,2,FALSE));
                        strcat(t," cm");
                        GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTST_String,t,TAG_END);
                    }
                    break;
                case 2:
                case 5:
                case 6:
                case 7:
                    ActivateGadget(GadgetAddress(win,gad->GadgetID+1),win,NULL);
                    break;
                case 9:   // Ok
                {
                    STRPTR t;
                    LONG   storage,width,height;

                    GT_GetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTST_String,&t,TAG_END);
                    width = (ULONG)(ConvertNumber(t,CNT_MM)*1024.0);
                    GT_GetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTST_String,&t,TAG_END);
                    height = (ULONG)(ConvertNumber(t,CNT_MM)*1024.0);

                    if (height < 1024 || width < 1024)
                    {
                        ErrorRequest(GetString(&gLocaleInfo, MSG_INVALID_PAPER_SIZE_ERR));
                        break;
                    }
                    mp->mp_mmWidth = width;
                    mp->mp_mmHeight = height;

                    mp->mp_Flags &= ~(MPF_PAGEMARKED | MPF_PAGEONLY);
                    GT_GetGadgetAttrs(GadgetAddress(win,4),win,NULL,GTCY_Active,&storage,TAG_END);
                    if (storage == 1)
                        mp->mp_Flags |= MPF_PAGEMARKED;
                    if (storage == 2)
                        mp->mp_Flags |= MPF_PAGEONLY;

                    GT_GetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTST_String,&t,TAG_END);
                    mp->mp_BorderLeft = (ULONG)(ConvertNumber(t,CNT_MM)*1024.0);
                    GT_GetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,&t,TAG_END);
                    mp->mp_BorderRight = (ULONG)(ConvertNumber(t,CNT_MM)*1024.0);
                    GT_GetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,&t,TAG_END);
                    mp->mp_BorderTop = (ULONG)(ConvertNumber(t,CNT_MM)*1024.0);
                    GT_GetGadgetAttrs(GadgetAddress(win,8),win,NULL,GTST_String,&t,TAG_END);
                    mp->mp_BorderBottom = (ULONG)(ConvertNumber(t,CNT_MM)*1024.0);

                    CloseAppWindow(win,TRUE);

                    if (mp->mp_Window)
                        DrawTable(mp->mp_Window);
                    break;

                    mp->mp_mmWidth = width;
                    mp->mp_mmHeight = height;
                }
                case 10:   // Abbrechen
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
HandleDocInfoIDCMP(REG(a0, struct TagItem *tag))
{
    struct Mappe *mp = wd->wd_Data;
    long   id;

	switch (imsg.Class)
    {
        case IDCMP_GADGETUP:
            if ((id = (gad = imsg.IAddress)->GadgetID) < 4)
            {
                ActivateGadget(GadgetAddress(win,id+1),win,NULL);
                break;
            }
        case IDCMP_VANILLAKEY:
            if (imsg.Class == IDCMP_VANILLAKEY)
            {
                gad = NULL;
                id = imsg.Code;
            }

            if (id == wd->wd_ShortCuts[0])      // Ok
            {
                STRPTR note = NULL;

                GT_SetString(GadgetAddress(win,1),&mp->mp_Author);
                GT_SetString(GadgetAddress(win,2),&mp->mp_Version);
                GT_SetString(GadgetAddress(win,3),&mp->mp_CatchWords);

#ifdef __amigaos4__
				note = (STRPTR)DoGadgetMethod (wd->wd_ExtData[1],win,NULL,GM_TEXTEDITOR_ExportText,NULL);
#else
                GetAttr(EGA_Text, wd->wd_ExtData[1], (IPTR *)&note);
#endif
                if (note != NULL)
                {
                    FreeString(mp->mp_Note);
                    mp->mp_Note = AllocString(note);
                }
            }
            else if (id == wd->wd_ShortCuts[2]) // Autor
				ActivateGadget(GadgetAddress(win, 1), win, NULL);
            else if (id == wd->wd_ShortCuts[3]) // Version
				ActivateGadget(GadgetAddress(win, 2), win, NULL);
            else if (id == wd->wd_ShortCuts[4]) // Stichworte
				ActivateGadget(GadgetAddress(win, 3), win, NULL);
            else if (id == wd->wd_ShortCuts[5]) // Bemerkung
				ActivateGadget(wd->wd_ExtData[1], win, NULL);

			if (id != wd->wd_ShortCuts[1] && id != wd->wd_ShortCuts[0]) // nicht "Ok" oder "Abbrechen"
                break;
        case IDCMP_CLOSEWINDOW:  /********** muß hinter IDCMP_VANILLAKEY stehen bleiben **********/
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
CloseDocumentWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;
    struct Node *ln;
    int    i;

    for(i = 8;i < 12;i++)
    {
        if ((gad = GadgetAddress(win,i)) && gad->UserData)
            FreePooled(pool,gad->UserData,32);
    }
    if (clean)
    {
        while((ln = MyRemHead(wd->wd_ExtData[3])) != 0)
            FreePooled(pool,ln,sizeof(struct Node));

        FreePooled(pool,wd->wd_ExtData[3],sizeof(struct List));
    }
}


STRPTR
TestPasswords(struct Window *win, LONG id)
{
    struct Gadget *ga,*gb;
    STRPTR a,b;

    if ((ga = GadgetAddress(win,id)) && (a = ga->UserData) && (gb = GadgetAddress(win,id+1)) && (b = gb->UserData))
    {
        if (strcmp(a,b))
        {
            DisplayBeep(NULL);
            ErrorRequest(GetString(&gLocaleInfo, MSG_PASSWORDS_DONT_MATCH_ERR));
            GT_SetGadgetAttrs(ga,win,NULL,GTST_String,NULL,TAG_END);
            GT_SetGadgetAttrs(gb,win,NULL,GTST_String,NULL,TAG_END);
            *a = 0;  *b = 0;
            ActivateGadget(ga,win,NULL);
			return NULL;
        }
    }
    else
        ErrorRequest("Interner Fehler %ld",42);  /* MERKER: was ist denn das??? */

	return a;
}


void
UpdateEventGadgets(struct Mappe *mp, UBYTE event)
{
    char t[16];

    if (event == (UBYTE)~0L)
        return;

    GT_SetGadgetAttrs(wd->wd_ExtData[4],win,NULL,GTLV_Labels,~0L,TAG_END);
    GT_SetGadgetAttrs(wd->wd_ExtData[4],win,NULL,GTLV_Labels,wd->wd_ExtData[3],GTLV_Selected,event,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GA_Disabled,FALSE,GTST_String,mp->mp_Events[event].ev_Command,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,14),win,NULL,GA_Disabled,FALSE,TAG_END);
    if (event == EVT_TIME)
        sprintf(t,"%ld s",mp->mp_Events[EVT_TIME].ev_Intervall);
    else
        t[0] = 0;
    GT_SetGadgetAttrs(GadgetAddress(win,16),win,NULL,GA_Disabled,event != EVT_TIME,GTST_String,t,TAG_END);
}


void ASM
HandleDocumentIDCMP(REG(a0, struct TagItem *tag))
{
    struct Mappe *mp = (struct Mappe *)wd->wd_Data;
    struct Node *ln;
	long   i,evt = (long)((struct Gadget *)wd->wd_ExtData[4])->UserData;
#ifdef __amigaos4__
    STRPTR t;
#endif
	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
			switch (((struct Gadget *)imsg.IAddress)->GadgetID)
            {
                case 15:
                    if (evt != ~0L)
                    {
						long mode, j;

                        GT_GetGadgetAttrs(GadgetAddress(win,14),win,NULL,GTCY_Active,&mode,TAG_END);

                        gad = GadgetAddress(win,13);
						i = PopUpList(win, imsg.IAddress, mode ? &intcmds : &mp->mp_AppCmds,
								mode ? TAG_IGNORE : POPA_ItemHeight,itemheight,
								mode ? TAG_IGNORE : POPA_CallBack,&linkHook,
								mode ? TAG_IGNORE : POPA_MaxItems,5,
								POPA_Left,  TRUE,
								POPA_Width, ((struct Gadget *)imsg.IAddress)->LeftEdge-(gad ? gad->LeftEdge+6 : 100),
								TAG_END);
                        if (i != ~0L)
                        {
                            for(j = 0,ln = (struct Node *)(mode ? intcmds.mlh_Head : mp->mp_AppCmds.mlh_Head);j < i;j++,ln = ln->ln_Succ);
                            if (!mode)
                                ln = ((struct Link *)ln)->l_Link;
                            mp->mp_Events[evt].ev_Flags |= EVF_ACTIVE;
                            mp->mp_Events[evt].ev_Command = AllocString(ln->ln_Name);
                            if ((ln = FindListNumber(wd->wd_ExtData[3],evt)) != 0)
                                ln->ln_Type = TRUE;
                            UpdateEventGadgets(mp,evt);
                        }
                    }
                    break;
            }
            break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(imsg.Code == ESCAPE_KEY)
			{
				CloseAppWindow(win, true);
				break;
			}
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
            {
                /********** Schutz ***********/

                case 8:
                case 10:
                    ActivateGadget(GadgetAddress(win,gad->GadgetID+1),win,NULL);
                    break;
                case 9:
                case 11:
                    TestPasswords(win,gad->GadgetID-1);
                    break;

                /********** Ereignisse ***********/

                case 12:   // Liste
                    ((struct Gadget *)wd->wd_ExtData[4])->UserData = (APTR)((long)imsg.Code);
                    if (imsg.Code == evt)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[4],win,NULL,GTLV_Labels,~0L,TAG_END);
                        if ((ln = FindListNumber(wd->wd_ExtData[3],evt = imsg.Code)) != 0)
                            ln->ln_Type = (!ln->ln_Type) & 1;
                        if (ln->ln_Type && mp->mp_Events[evt].ev_Command)
                            mp->mp_Events[evt].ev_Flags |= EVF_ACTIVE;
                        else
                            mp->mp_Events[evt].ev_Flags &= ~EVF_ACTIVE;
                    }
                    UpdateEventGadgets(mp,imsg.Code);
                    break;
                case 13:  // Command
#ifndef __amigaos4__
                {
                    STRPTR t;
#endif

                    GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END);
                    if (!(ln = FindListNumber(wd->wd_ExtData[3],evt)))
                        break;
                    if ((mp->mp_Events[evt].ev_Command = AllocString(t)) != 0)
                    {
                        ln->ln_Type = TRUE;
                        mp->mp_Events[evt].ev_Flags |= EVF_ACTIVE;
                    }
                    else
                    {
                        ln->ln_Type = FALSE;
                        mp->mp_Events[evt].ev_Flags &= ~EVF_ACTIVE;
                    }
                    UpdateEventGadgets(mp,evt);
                    break;
                case 16:   // Intervalle
                    if (evt == EVT_TIME)
                    {
                        GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END);
                        if ((i = ConvertTime(t) < 1) != 0)
                            i = 1;
                        mp->mp_Events[evt].ev_Intervall = i;
                    }
                    break;
#ifndef __amigaos4__
                }
#endif

                /********** Ok ***********/

                case 17:
                {
                    STRPTR t;

                    if ((t = TestPasswords(win, 8)) != 0)
                    {
                        FreeString(mp->mp_Password);
                        mp->mp_Password = AllocString(t);
                    }
                    else
                        break;
                    if ((t = TestPasswords(win, 10)) != 0)
                    {
                        FreeString(mp->mp_CellPassword);
                        mp->mp_CellPassword = AllocString(t);
                    }
                    else
                        break;

                    mp->mp_Flags &= ~(MPF_NOTES | MPF_SCRIPTS | MPF_READONLY | MPF_SHOWHIDDEN | MPF_SAVEWINPOS);

                    mp->mp_Flags |= GetCheckBoxFlag(GadgetAddress(win,1),win,MPF_SCRIPTS) |
                                                    GetCheckBoxFlag(GadgetAddress(win,2),win,MPF_NOTES) |
                                                    GetCheckBoxFlag(GadgetAddress(win,3),win,MPF_READONLY) |
                                                    GetCheckBoxFlag(GadgetAddress(win,4),win,MPF_SHOWHIDDEN) |
                                                    GetCheckBoxFlag(GadgetAddress(win,5),win,MPF_SAVEWINPOS);

                    CloseAppWindow(win,TRUE);
                    UpdateInteractive(mp,FALSE);

                    if (mp->mp_Window)
                        DrawTable(mp->mp_Window);
                    break;
                }
            }
            break;
        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;
    }
}


void ASM
HandlePageIDCMP(REG(a0, struct TagItem *tag))
{
    struct Page *page = wd->wd_Data;
    struct tableField *tf;
    struct Node *ln;
    STRPTR t;
	long   j, i = 0;
#ifdef __amigaos4__
	bool ok_pressed = FALSE;  //Notwendig, bis das mit den Dokumentenschutz geklärt ist. Welcher zusammenhang besteht mit übernahme der Änderungen!
#endif

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            switch((gad = imsg.IAddress)->GadgetID)
            {
                case 3:
                case 4:
                    i = PopUpList(win, gad, &colors, POPA_CallBack, &colorHook, POPA_MaxPen, ~0L,
                            POPA_ItemHeight, fontheight + 4, POPA_Width, GetListWidth(&colors) + boxwidth + 5,
                            TAG_END);
                    if (i != ~0L)
                    {
                        for(j = 0,ln = (struct Node *)colors.mlh_Head;j < i;j++,ln = ln->ln_Succ);
                        wd->wd_ExtData[gad->GadgetID-3] = (APTR)(i = ((struct colorPen *)ln)->cp_ID);
                        DrawColorField(win->RPort,gad,i,TRUE);
                    }
                    break;
            }
            break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
            {
                case 5:     // Ok (apply page changes)
#ifdef __amigaos4__
					ok_pressed = TRUE;
#endif
                    GT_GetGadgetAttrs(GadgetAddress(win, 1), win, NULL, GTST_String, &t, TAG_END);
                    if (!page->pg_Node.ln_Name || strcmp(page->pg_Node.ln_Name,t))
                    {
                        FreeString(page->pg_Node.ln_Name);
                        page->pg_Node.ln_Name = AllocString(t);
                        if (page->pg_Window && ((struct winData *)page->pg_Window->UserData)->wd_Data == page)
                            GT_SetGadgetAttrs(GadgetAddress(page->pg_Window,GID_PAGE),page->pg_Window,NULL,GTTX_Text,page->pg_Node.ln_Name,TAG_END);
                    }
                    if ((ULONG)wd->wd_ExtData[0] != page->pg_APen || (ULONG)wd->wd_ExtData[1] != page->pg_BPen)
                    {
                        struct tableField *stf;
                        struct UndoNode *un;
                        char   s[100];

						/* Create UndoNode and string */

                        i = 42;
						strcpy(s, GetString(&gLocaleInfo, MSG_PAGE_COLOR));
                        if ((ln = (APTR)GetColorPen((ULONG)wd->wd_ExtData[0])) != 0)
							strcat(s, ln->ln_Name);
                        else
                            strcat(s,"-");
                        strcat(s,"/");
                        if ((ln = (APTR)GetColorPen((ULONG)wd->wd_ExtData[1])) != 0)
                            strcat(s,ln->ln_Name);
                        else
                            strcat(s,"-");
						if ((un = CreateUndo(page, UNDO_PRIVATE, s)) != 0)
                        {
							un->un_Type = UNT_CELLS_CHANGED;
                            un->un_Mode = UNM_PAGECOLORS;
                            un->un_TablePos.tp_Col = page->pg_APen;               /* undo: apen */
                            un->un_TablePos.tp_Row = page->pg_BPen;               /*       bpen */
                            un->un_TablePos.tp_Width = (ULONG)wd->wd_ExtData[0];  /* redo: apen */
                            un->un_TablePos.tp_Height = (ULONG)wd->wd_ExtData[1]; /*       bpen */
                        }
			
						/* Actually perform the changes - set every color that matches
						 * the old colors to the newly chosen ones.
                         */

                        foreach (&page->pg_Table, tf)
                        {
                            if (un && (stf = CopyCell(page, tf)))
								MyAddTail(&un->un_UndoList, stf);

                            if (tf->tf_APen == page->pg_APen)
                                tf->tf_APen = (ULONG)wd->wd_ExtData[0];
                            if (tf->tf_ReservedPen == page->pg_APen)
                                tf->tf_ReservedPen = (ULONG)wd->wd_ExtData[0];
                            if (tf->tf_BPen == page->pg_BPen)
                                tf->tf_BPen = (ULONG)wd->wd_ExtData[1];

                            if (un && (stf = CopyCell(page, tf)))
								MyAddTail(&un->un_RedoList, stf);
                        }
                        page->pg_APen = (ULONG)wd->wd_ExtData[0];
                        page->pg_BPen = (ULONG)wd->wd_ExtData[1];

						/* Set the page protection flags */

						GT_GetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTCY_Active, &i, TAG_END);
						page->pg_Flags &= ~PGF_SECURITY;
						if (i == 1)
							page->pg_Flags |= PGF_IMMUTABLE;
                    }
                case 6:
                    CloseAppWindow(win, TRUE);
#ifdef __amigaos4__
					if(ok_pressed)
#else
                    if (i)
#endif
                        DrawTable(page->pg_Window);
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win, TRUE);
            break;
    }
}


void
updateCellSizeGads(long w, long h)
{
    if (w != ~0L)
    {
        GT_SetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTCY_Active,w,TAG_END);
        GT_SetGadgetAttrs(gad = GadgetAddress(win,2),win,NULL,GA_Disabled,w == 3 || w == 0,TAG_END);
        if (w > 0 && w < 3)
            ActivateGadget(gad,win,NULL);
    }
    if (h != ~0L)
    {
        GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTCY_Active,h,TAG_END);
        GT_SetGadgetAttrs(gad = GadgetAddress(win,4),win,NULL,GA_Disabled,h == 3 || h == 0,TAG_END);
        if (h > 0 && h < 3)
            ActivateGadget(gad,win,NULL);
    }
}


void
SetCellSizeGadget(struct Window *win,long gad,long mm)
{
    char t[32];

    if (mm != -1L)
    {
        strcpy(t,ita(mm/1024.0,2,FALSE));
        strcat(t," mm");
        GT_SetGadgetAttrs(GadgetAddress(win,gad),win,NULL,GTST_String,t,TAG_END);
    }
}


void ASM
handleCellSizeIDCMP(REG(a0, struct TagItem *tag))
{
    long   i,id;

	switch (imsg.Class)
    {
        case IDCMP_VANILLAKEY:
            id = imsg.Code;
        case IDCMP_GADGETUP:
            if (imsg.Class == IDCMP_GADGETUP)
                id = (gad = imsg.IAddress)->GadgetID;

            switch(id)
            {
                case 1:
                    updateCellSizeGads(imsg.Code,~0L);
                    break;
                case 3:
                    updateCellSizeGads(~0L,imsg.Code);
                    break;
                case 2:
                    if (imsg.Qualifier & IEQUALIFIER_SHIFT)
                        break;

                    GT_GetGadgetAttrs(gad = GadgetAddress(win,4),win,NULL,GA_Disabled,&i,TAG_END);

                    if (gad && !i)
                        ActivateGadget(gad,win,NULL);
                    break;
                case ' ':
                    ActivateGadget(GadgetAddress(win,2),win,NULL);
                    break;
                default:
                    if (tolower(id) == wd->wd_ShortCuts[0] || tolower(id) == wd->wd_ShortCuts[1])
                    {
                        GT_GetGadgetAttrs(gad = GadgetAddress(win,tolower(id) == wd->wd_ShortCuts[0] ? 1 : 3),win,NULL,GTCY_Active,&i,TAG_END);

                        if (isupper(id))   // Cycle-Gadget zurück
                        {
                            i--;
                            if (i < 0) i = 3;
                        }
                        else               // Cycle-Gadget weiter
                        {
                            i++;
                            if (i > 3) i = 0;
                        }
                        GT_SetGadgetAttrs(gad,win,NULL,GTCY_Active,i,TAG_END);
                        updateCellSizeGads(gad->GadgetID == 1 ? i : ~0L,gad->GadgetID == 3 ? i : ~0L);
                    }
                    else if (id == wd->wd_ShortCuts[5]) // Größe von aktueller Zelle übernehmen
                    {
                        if (rxpage && rxpage->pg_Gad.DispPos > PGS_NONE)
                        {
                            long mm = -1L;

                            if (rxpage->pg_Gad.cp.cp_Col > rxpage->pg_Cols)
                                mm = rxpage->pg_mmStdWidth;
                            else if (rxpage->pg_tfWidth)
                                mm = (rxpage->pg_tfWidth+rxpage->pg_Gad.cp.cp_Col-1)->ts_mm;

                            SetCellSizeGadget(win,2,mm);

                            if (rxpage->pg_Gad.cp.cp_Row > rxpage->pg_Rows)
                                mm = rxpage->pg_mmStdHeight;
                            else if (rxpage->pg_tfHeight)
                                mm = (rxpage->pg_tfHeight+rxpage->pg_Gad.cp.cp_Row-1)->ts_mm;
                            else
                                mm = -1L;

                            SetCellSizeGadget(win,4,mm);
                            updateCellSizeGads(1,1);
                        }
                        else
                            DisplayBeep(NULL);
                    }
                    else if (id == wd->wd_ShortCuts[2] || id == wd->wd_ShortCuts[3]) // Ok & Zuweisen
                    {
                        STRPTR width = NULL,height = NULL;
                        long j;

                        GT_GetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTCY_Active,&i,TAG_END);
                        if (i)  // neue Breite
                        {
                            GT_GetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTST_String,&width,TAG_END);
                            width = AllocString(width);
                        }
                        GT_GetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTCY_Active,&j,TAG_END);
                        if (j)  // neue Höhe
                        {
                            GT_GetGadgetAttrs(GadgetAddress(win,4),win,NULL,GTST_String,&height,TAG_END);
                            height = AllocString(height);
                        }
                        if (id == wd->wd_ShortCuts[2])
                            CloseAppWindow(win,TRUE);

                        ChangeCellSize(rxpage,width,height,(i == 2 ? CCS_MINWIDTH : (i == 3 ? CCS_OPTWIDTH : 0)) | (id == 2 ? CCS_MINHEIGHT : (id == 3 ? CCS_OPTHEIGHT : 0)),NULL);
                        FreeString(width);  FreeString(height);
                    }
                    else if (id == wd->wd_ShortCuts[4] || id == ESCAPE_KEY)  // Abbrechen
                        CloseAppWindow(win,TRUE);
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
closeNotesWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;
    struct wdtNote *wn;

    if (clean)
    {
        while((wn = (struct wdtNote *)MyRemHead((struct List *)wd->wd_ExtData[0])) != 0)
        {
            FreeString(wn->wn_Node.ln_Name);
            FreeString(wn->wn_Note);
            FreePooled(pool,wn,sizeof(struct wdtNote));
        }
    }
}


void ASM
handleNotesIDCMP(REG(a0, struct TagItem *tag))
{
    struct tableField *tf = NULL;
    char   t[256];
    long   id,i;

    switch(imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 2)
            {
                i = PopUpList(win,gad,wd->wd_ExtData[0],POPA_Width,win->Width-16-boxwidth,TAG_END);
                if (i != ~0L)
                {
                    struct wdtNote *wn;
                    STRPTR s,t;
                    long   len;

                    wn = (struct wdtNote *)FindListNumber(wd->wd_ExtData[0],i);
#ifdef __amigaos4__
					s = (STRPTR)DoGadgetMethod ((gad = wd->wd_ExtData[1]),win,NULL,GM_TEXTEDITOR_ExportText,TAG_END);
#else
                    GetAttr(EGA_Text, (Object*)(gad = wd->wd_ExtData[1]), (IPTR *)&s);
#endif
                    if (s && wn->wn_Note && (t = AllocPooled(pool,len = strlen(s)+strlen(wn->wn_Note)+1)))
                    {
                        strcpy(t,s);
                        strcat(t,wn->wn_Note);
#ifdef __amigaos4__
                        SetGadgetAttrs(gad, win, NULL, GA_TEXTEDITOR_Contents, t, TAG_END);
#else
                        SetGadgetAttrs(gad,win,NULL,EGA_Text,t,TAG_END);
#endif
                        FreePooled(pool,t,len);
                    }
                    else
#ifdef __amigaos4__
                        SetGadgetAttrs(gad, win, NULL, GA_TEXTEDITOR_Contents, wn->wn_Note, TAG_END);
#else
                        SetGadgetAttrs(gad,win,NULL,EGA_Text,wn->wn_Note,TAG_END);
#endif
                    ActivateGadget(gad,win,NULL);
                }
                break;
            }
            break;
        case IDCMP_GADGETUP:
            id = ((struct Gadget *)imsg.IAddress)->GadgetID;
        case IDCMP_VANILLAKEY:
            if (imsg.Class == IDCMP_VANILLAKEY)
                id = imsg.Code;
            if (wd->wd_ShortCuts[0] == id)
                ActivateGadget(wd->wd_ExtData[1],win,NULL);
            else if (wd->wd_ShortCuts[1] == id || wd->wd_ShortCuts[2] == id)
            {
                STRPTR note = NULL;

                id = wd->wd_ShortCuts[1] == id ? 1 : 2;
#ifdef __amigaos4__
				note = (STRPTR)DoGadgetMethod (wd->wd_ExtData[1],win,NULL,GM_TEXTEDITOR_ExportText,NULL);
#else
                GetAttr(EGA_Text, wd->wd_ExtData[1], (IPTR *)&note);
#endif
                if (id == 1 && note && (i = strlen(note)))
                {
                    strcpy(t,GetString(&gLocaleInfo, MSG_ATTACH_NOTE_UNDO));
                    strcat(t,"\"");
                    strncat(t,note,222);
                    if (i > 222)
                        strcat(t,"...");
                    strcat(t,"\"");
                }
                else
                {
                    strcpy(t,GetString(&gLocaleInfo, MSG_DELETE_NOTE_UNDO));
                    id = 2;
                }
                BeginUndo(rxpage,UNDO_BLOCK,t);
                while ((tf = GetMarkedFields(rxpage, tf, id == 1 ? TRUE : FALSE)) != 0)
                {
                    FreeString(tf->tf_Note);
                    if (id == 1)
                        tf->tf_Note = AllocString(note);
                    else
                        tf->tf_Note = NULL;
                }
                EndUndo(rxpage);
                CloseAppWindow(win,TRUE);
            }
            else if (wd->wd_ShortCuts[3] == id || id == ESCAPE_KEY)
                CloseAppWindow(win,TRUE);
            break;
        case IDCMP_NEWSIZE:
			StandardNewSize(CreateNotesGadgets);
            RefreshAppWindow(win,wd);
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void PUBLIC
ZoomWindowLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *n), REG(d0, UBYTE flags))
{
    char   txt[64];

    if (rxpage)
    {
        ProcentToString(rxpage->pg_Zoom,txt);
        GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTST_String,txt,TAG_END);
    }
    else
        CloseAppWindow(ln->ln_Data[0],TRUE);
}


void ASM
handleZoomIDCMP(REG(a0, struct TagItem *tag))
{
    struct Node *ln;
	long   i, j;

	switch (imsg.Class)
    {
        case IDCMP_VANILLAKEY:
            if (imsg.Code != ESCAPE_KEY)
            {
				ActivateGadget(GadgetAddress(win, 1), win, NULL);
                break;
            }
        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 2)
            {
                i = PopUpList(win,GadgetAddress(win,1),&zooms,TAG_END);
                if (i != ~0L)
                {
                    for(j = 0,ln = (struct Node *)zooms.mlh_Head;j < i;j++,ln = ln->ln_Succ);
					if (ln->ln_Succ != (struct Node *)&zooms.mlh_Tail) {
						if (ln->ln_Type != POPUP_NO_SELECT_BARLABEL)
							SetZoom(rxpage, (atol(ln->ln_Name) << 10) / 100, FALSE, TRUE);
					} else
						SetZoom(rxpage, -1, FALSE, TRUE);
                }
            }
            break;
        case IDCMP_GADGETUP:
            if ((gad = imsg.IAddress)->GadgetID == 1)
            {
                GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&ln,TAG_END);
                if (ln)
                    SetZoom(rxpage,(atol((STRPTR)ln) << 10)/100,FALSE,TRUE);
                break;
            }
            break;
    }
}


void ASM
closeGClassesWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    RemLockNode(wd->wd_ExtData[0]);
}


void ASM
handleGClassesIDCMP(REG(a0, struct TagItem *tag))
{
    struct gClass *gc;
    long   i;

	switch (imsg.Class)
    {
        case IDCMP_VANILLAKEY:
			if(imsg.Code == ESCAPE_KEY)
			{
				CloseAppWindow(win, true);
				break;
			}
			break;		    
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
            {
                case 1:
					for (gc = (APTR)gclasses.mlh_Head, i = 0; i < imsg.Code; i++, gc = (APTR)gc->gc_Node.in_Succ);
					if (!GetCheckBoxFlag(GadgetAddress(win, 2), win, TRUE))
						CloseAppWindow(win, TRUE);
                    else
						SetMousePointer(win, POINTER_OBJECT);

					PrepareCreateObject(rxpage, gc, FALSE);
                    break;
            }
            break;
        case IDCMP_NEWSIZE:
			StandardNewSize(CreateGClassesGadgets);
            break;
        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;
    }
}


void
UpdateGInterface(struct Window *interfaceWindow, struct MinList *list, struct gObject *go, UBYTE page)
{
    struct gGadget *gg;
    struct Window *win;
    long   width;
    char   t[64];

    if (!list || !go)
        return;

	foreach (list, gg)
    {
        if (page != gg->gg_Page)
            win = NULL;
        else
			win = interfaceWindow;

		switch (gg->gg_Type)
        {
            case GIT_WEIGHT:
				if (gg->gg_Kind == STRING_KIND && GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&width))
                {
					sprintf(t, "%s pt", ita(width / 65536.0, -3, FALSE));
					GT_SetGadgetAttrs(gg->gg_Gadget, win, NULL, GTST_String, t, TAG_END);
                }
                break;
            case GIT_FONT:
            {
                struct FontInfo *fi;

				if (GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&fi))
                {
					switch (gg->gg_Kind)
                    {
                        case STRING_KIND:
							sprintf(t, "%ld pt", fi->fi_FontSize->fs_PointHeight >> 16);
							GT_SetGadgetAttrs(gg->gg_Gadget, win, NULL, GTST_String, t, TAG_END);
                            break;
                        case TEXT_KIND:
							GT_SetGadgetAttrs(gg->gg_Gadget, win, NULL, GTTX_Text, fi->fi_Family->ln_Name, TAG_END);
                            break;
                    }
                }
                break;
            }
            case GIT_TEXT:
            case GIT_FORMULA:
            case GIT_FILENAME:
				if (gg->gg_Kind == STRING_KIND && GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&width))
					GT_SetGadgetAttrs(gg->gg_Gadget, win, NULL, GTST_String, width, TAG_END);
                break;
            case GIT_PEN:
				if (win && gg->gg_Kind == POPUP_KIND && GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&width))
					DrawColorField(win->RPort, gg->gg_Gadget, width, TRUE);
                break;
            case GIT_CYCLE:
				if (gg->gg_Kind == CYCLE_KIND && GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&width))
					GT_SetGadgetAttrs(gg->gg_Gadget, win, NULL, GTCY_Active, width, TAG_END);
                break;
            case GIT_CHECKBOX:
				if (gg->gg_Kind == CHECKBOX_KIND && GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&width))
					GT_SetGadgetAttrs(gg->gg_Gadget, win, NULL, GTCB_Checked, width, TAG_END);
                break;
			case GIT_DATA_PAGE:
			{
				/* this is a special type for WDT_DIAGRAM */
				struct winData *wd = (struct winData *)interfaceWindow->UserData;
				struct Page *dataPage, *page = go->go_Page;

				if (gg->gg_Kind == POPUP_KIND && GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&dataPage))
				{
					struct Gadget *gad = PageGadget(wd->wd_Pages[0], gg->gg_Gadget->GadgetID - 1);
					
					if (gad == NULL)
						break;
																
					foreach (&page->pg_Document->mp_Pages, page)
					{
						if (dataPage == page)
							break;
					}
					if (page != dataPage)
					{
						// ToDo: something went very wrong, did it?
						page = (struct Page *)page->pg_Document->mp_Pages.mlh_Head;
					}

					GT_SetGadgetAttrs(gad, win, NULL, GTTX_Text, page->pg_Node.ln_Name, TAG_END);
					gad->UserData = page;
				}
			}
        }
    }
}


void
UpdateObjectGadgets(struct Window *win)
{
    struct gObject *go;
    struct winData *wd;
    long   width,height;
    char   t[64];

    if (!win)
        return;
    wd = (APTR)win->UserData;
    go = wd->wd_Data;

    if (wd->wd_Type == WDT_DIAGRAM)
    {
        UpdateDiagramGadgets(win);
        return;
    }
	if ((gad = GadgetAddress(win, 1)) != 0)         /* Name */
		GT_SetGadgetAttrs(gad, win, NULL, GTST_String, go->go_Node.ln_Name, TAG_END);

    width = go->go_mmRight;
    height = go->go_mmBottom;
    if (!(go->go_Flags & GOF_LINEMODE))
    {
        width -= go->go_mmLeft;
        height -= go->go_mmTop;
    }

	if ((gad = GadgetAddress(win, 2)) != 0)         /* von links */
    {
		sprintf(t, "%s cm", ita(go->go_mmLeft / 10240.0, -3, FALSE));
		GT_SetGadgetAttrs(gad, win, NULL, GTST_String, t, TAG_END);
    }
	if ((gad = GadgetAddress(win, 3)) != 0)         /* von oben */
    {
		sprintf(t, "%s cm", ita(go->go_mmTop / 10240.0, -3, FALSE));
		GT_SetGadgetAttrs(gad, win, NULL, GTST_String, t, TAG_END);
    }
	if ((gad = GadgetAddress(win, 4)) != 0)         /* Breite/von rechts */
    {
		sprintf(t, "%s cm", ita(width / 10240.0, -3, FALSE));
		GT_SetGadgetAttrs(gad, win, NULL, GTST_String, t, TAG_END);
    }
	if ((gad = GadgetAddress(win, 5)) != 0)         /* Höhe/von unten */
    {
		sprintf(t, "%s cm", ita(height / 10240.0, -3, FALSE));
		GT_SetGadgetAttrs(gad, win, NULL, GTST_String, t, TAG_END);
    }
	UpdateGInterface(win, wd->wd_ExtData[4], go, 1);
}


void ASM
CloseObjectWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;
    struct gGadget *gg;

    ((struct gObject *)wd->wd_Data)->go_Window = NULL;

	while ((gg = (struct gGadget *)MyRemHead(wd->u.object.wd_Gadgets)) != 0)
		FreePooled(pool, gg, sizeof(struct gGadget));

    if (clean)
		FreePooled(pool, wd->u.object.wd_Gadgets, sizeof(struct MinList));

	FreeStringList(wd->u.object.wd_WeightStrings);
}


void
HandleGGadget(struct Page *page, struct gObject *go)
{
    struct gGadget *gg;
    ULONG  tags[3];
	bool   set = false, freeString = false;

    if (imsg.Class != IDCMP_GADGETDOWN && imsg.Class != IDCMP_GADGETUP)
        return;

    if (!(gg = (gad = imsg.IAddress)->UserData))
        return;

    tags[0] = gg->gg_Tag;
    tags[2] = TAG_END;

    if (imsg.Class == IDCMP_GADGETDOWN)
    {
        struct Node *ln;
		long   i, j;

		switch (gg->gg_Type)
        {
            case GIT_WEIGHT:
				i = PopUpList(win, GadgetAddress(win, gad->GadgetID - 1), wd->wd_ExtData[1], TAG_END);
                if (i != ~0L)
                {
                    tags[1] = weightvalues[i];
					set = true;
                }
                break;
            case GIT_PEN:
				i = PopUpList(win, gad, &colors, POPA_CallBack, &colorHook, POPA_MaxPen, ~0L, POPA_ItemHeight, fontheight + 4,
						POPA_Width, GetListWidth(&colors) + boxwidth + 5, TAG_END);
                if (i != ~0L)
                {
					for (j = 0, ln = (struct Node *)colors.mlh_Head; j < i; j++, ln = ln->ln_Succ);
                    tags[1] = ((struct colorPen *)ln)->cp_ID;
					set = true;
                }
                break;
            case GIT_FONT:
            {
                struct FontInfo *fi;

				if (GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&fi))
                {
                    if (gg->gg_Kind == POPUP_KIND)
                    {
						i = PopUpList(win, GadgetAddress(win, gad->GadgetID-1), &families, TAG_END);
                        if (i != ~0L)
                        {
							for (j = 0, ln = (struct Node *)families.mlh_Head; j < i; j++, ln = ln->ln_Succ);
							tags[1] = (ULONG)NewFontInfo(fi, page->pg_DPI, FA_Family, ln, TAG_END);
							set = true;
                        }
                    }
                    else if (gg->gg_Kind == POPUPPOINTS_KIND)
                    {
						i = PopUpList(win, GadgetAddress(win, gad->GadgetID - 1), &sizes, TAG_END);
                        if (i != ~0L)
                        {
							for (j = 0, ln = (struct Node *)sizes.mlh_Head; j < i; j++, ln = ln->ln_Succ);
							tags[1] = (ULONG)NewFontInfo(fi, page->pg_DPI, FA_PointHeight, atol(ln->ln_Name) << 16, TAG_END);
							set = true;
                        }
                    }
                }
                break;
            }
            case GIT_TEXT:
            case GIT_FORMULA:
                if (gg->gg_Tag == GOA_Command)
                {
                    struct Mappe *mp = page->pg_Document;
                    long   mode;

					GT_GetGadgetAttrs(GadgetAddress(win, gad->GadgetID - 1), win, NULL, GTCY_Active, &mode, TAG_END);

					i = PopUpList(win, GadgetAddress(win, gad->GadgetID - 2), mode ? &intcmds : &mp->mp_AppCmds,
							mode ? TAG_IGNORE : POPA_ItemHeight, itemheight, mode ? TAG_IGNORE : POPA_CallBack, &linkHook,
							mode ? TAG_IGNORE : POPA_MaxItems, 5, TAG_END);
                    if (i != ~0L)
                    {
						for (j = 0,ln = (struct Node *)(mode ? intcmds.mlh_Head : mp->mp_AppCmds.mlh_Head); j < i; j++, ln = ln->ln_Succ);
                        if (!mode)
                            ln = ((struct Link *)ln)->l_Link;
                        tags[1] = (ULONG)ln->ln_Name;
						set = true;
                    }
                }
                break;
			case GIT_DATA_PAGE:
				// happens in HandleDiagramIDCMP()
				break;
        }
    }
    else  // IDCMP_GADGETUP
    {
        set = TRUE;

		switch (gg->gg_Type)
        {
            case GIT_FILENAME:
                if (gg->gg_Kind == POPUP_KIND)
                {
                    char path[400];
                    STRPTR name = NULL;

					GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&name);
                    if (name)
                    {
                        STRPTR u;

						zstrcpy(path, name);
                        name = (STRPTR)FilePart(name);

                        if ((u = PathPart(path)) != 0)
                            *u = 0;
                    }
                    else
                        path[0] = 0;

					if (AslRequestTags(fileReq, 
                                                ASLFR_TitleText,     GetString(&gLocaleInfo, MSG_SELECT_FILE_NAME_TITLE),
                                                ASLFR_InitialDrawer, path,
                                                ASLFR_InitialFile,   name ? name : (STRPTR)"",
                                                ASLFR_DoSaveMode,    FALSE,
#ifdef __amigaos4__
												ASLFR_Window,		 page->pg_Window,
												ASLFR_AcceptPattern, "#?.(iff|png|gif|jpg)", 
												ASLFR_InitialPattern, "#?.(iff|png|gif|jpg)",
												ASLFR_DoPatterns, 	 TRUE,
#else
												ASLFR_Window,		 scr->FirstWindow,
                                                ASLFR_InitialPattern,"#?",
                                                ASLFR_DoPatterns,    FALSE,
#endif
                                                ASLFR_DrawersOnly,   FALSE,
                                                TAG_END))
                    {
						strcpy(path, fileReq->fr_Drawer);
						AddPart(path, fileReq->fr_File, 400);
                        tags[1] = (ULONG)AllocString(path);
						freeString = true;
                    }
                    else
						set = false;
                    break;
                }
            case GIT_TEXT:
            case GIT_FORMULA:
            {
                STRPTR t;

				GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
                tags[1] = (ULONG)t;
                break;
            }
            case GIT_WEIGHT:
            {
                STRPTR t;
                LONG   p;

				GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
				p = (LONG)(ConvertNumber(t, CNT_POINT) * 65536.0 + 0.5);
                tags[1] = abs(p);
                break;
            }
            case GIT_CHECKBOX:
            case GIT_CYCLE:
                tags[1] = imsg.Code;
                break;
            case GIT_BUTTON:
                set = FALSE;
                tags[1] = ~0L;
				gSetObjectAttrsA(page, go, (struct TagItem *)tags);
                break;
            case GIT_FONT:
            {
                struct FontInfo *fi;

				if (GetGObjectAttr(go, gg->gg_Tag, (ULONG *)&fi))
                {
                    STRPTR t;

					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
					tags[1] = (ULONG)NewFontInfo(fi, page->pg_DPI, FA_PointHeight, (long)(ConvertNumber(t, CNT_POINT)*65536 + 0.5), TAG_END);
                    break;
                }
            }
            default:
				set = false;
        }
    }
    if (set)
    {
		if (gObjectIsDiagram(go))
			SetDiagramAttrsA(win, (struct gDiagram *)go, (struct TagItem *)tags);
        else
			SetGObjectAttrsA(page, go, (struct TagItem *)tags);
    }
	if (freeString)
        FreeString((STRPTR)tags[1]);
}


void ASM
HandleObjectIDCMP(REG(a0, struct TagItem *tag))
{
    struct Page *page = wd->wd_ExtData[0];
    struct gObject *go = wd->wd_Data;

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
			HandleGGadget(page, go);
            break;
        case IDCMP_VANILLAKEY:
            if (imsg.Code == ESCAPE_KEY)
				CloseAppWindow(win, TRUE);
            break;
        case IDCMP_GADGETUP:
        {
            STRPTR t;

            switch((gad = imsg.IAddress)->GadgetID)
            {
                case 2:  /* Left */
                case 3:  /* Top */
                {
                    struct UndoNode *un;
                    long   x,y;

					GT_GetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTST_String, &t, TAG_END);
					x = (long)(ConvertNumber(t, CNT_MM) * 1024) - go->go_mmLeft;
					GT_GetGadgetAttrs(GadgetAddress(win, 3), win, NULL, GTST_String, &t, TAG_END);
					y = (long)(ConvertNumber(t, CNT_MM) * 1024) - go->go_mmTop;

					if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_MOVE_OBJECT_UNDO))) != 0)
                    {
						un->un_Type = UNT_OBJECTS_MOVE;
						un->un_MoveDeltaX = x;
						un->un_MoveDeltaY = y;

						AddUndoLink(&un->un_UndoList, go);
						ApplyObjectsMoveUndoRedo(page, un, TYPE_REDO);
                    }
                    break;
                }
                case 4:  /* Width */
                case 5:  /* Height */
                {
                    long   w,h;

                    GT_GetGadgetAttrs(GadgetAddress(win,4),win,NULL,GTST_String,&t,TAG_END);
                    w = (long)(ConvertNumber(t,CNT_MM)*1024);
                    GT_GetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTST_String,&t,TAG_END);
                    h = (long)(ConvertNumber(t,CNT_MM)*1024);
                    if (go->go_Flags & GOF_LINEMODE)
                        w = abs(w-go->go_mmLeft),  h = abs(w-go->go_mmTop);

					SizeGObject(page, go, w, h);
                    break;
                }
                default:
					HandleGGadget(page, go);
                    break;
            }
            break;
        }
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
handleCommandIDCMP(REG(a0, struct TagItem *tag))
{
    STRPTR t = NULL;
    struct Node *ln;
    long   i,j,mode;

    switch(imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 3)
            {
                GT_GetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTCY_Active,&mode,TAG_END);

                i = PopUpList(win,gad = GadgetAddress(win,1),mode ? &intcmds : &rxpage->pg_Document->mp_AppCmds,mode ? TAG_IGNORE : POPA_ItemHeight,itemheight,mode ? TAG_IGNORE : POPA_CallBack,&linkHook,mode ? TAG_IGNORE : POPA_MaxItems,5,TAG_END);
                if (i != ~0L)
                {
                    for(j = 0,ln = (struct Node *)(mode ? intcmds.mlh_Head : rxpage->pg_Document->mp_AppCmds.mlh_Head);j < i;j++,ln = ln->ln_Succ);
                    if (!mode)
                        ln = ((struct Link *)ln)->l_Link;
                    GT_SetGadgetAttrs(gad,win,NULL,GTST_String,ln->ln_Name,TAG_END);
                }
            }
            break;
        case IDCMP_VANILLAKEY:
            i = imsg.Code;
        case IDCMP_GADGETUP:
            if (imsg.Class == IDCMP_GADGETUP)
                i = ((struct Gadget *)imsg.IAddress)->GadgetID;
            if (i == wd->wd_ShortCuts[0])
                ActivateGadget(GadgetAddress(win,1),win,NULL);
            else if (i == wd->wd_ShortCuts[1])
            {
                GT_GetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTST_String,&t,NULL);
                FreeString(wdt_cmdstr);
                wdt_cmdstr = AllocString(t);
            }
            if (i == wd->wd_ShortCuts[1] || i == wd->wd_ShortCuts[2] || i == ESCAPE_KEY)
            {
                char ok = wd->wd_ShortCuts[1];

                CloseAppWindow(win,TRUE);
                if (i == ok)
					ProcessAppCmd(rxpage, wdt_cmdstr);
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
CloseFileTypeWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	if (clean) {
        struct winData *wd = (struct winData *)win->UserData;
		struct IOTypeLink *iol;
        struct IOType *io;

		foreach(&iotypes, io) {
            if (io->io_ClosePrefsGUI)
                io->io_ClosePrefsGUI();
        }

		while ((iol = (struct IOTypeLink *)MyRemHead(wd->wd_ExtData[1])) != 0) {
            struct Node *ln;

			while ((ln = MyRemHead((APTR)&iol->iol_Description)) != 0) {
                FreeString(ln->ln_Name);
				FreePooled(pool, ln, sizeof(struct Node));
            }
			FreePooled(pool, iol, sizeof(struct IOTypeLink));
        }

		FreePooled(pool, wd->wd_ExtData[1], sizeof(struct MinList));
    }
}

#ifdef __amigaos4__
//Notlösung, da das genutzte verfahren unter OS4.x nicht mehr geht und zu crahses führt.
//muß eine bessere Lösung gefunden werden.
//Allerdings geht unter OS4.x auch die 68k Version nicht!
void PUBLIC
HandleFileTypeIDCMP(REG(a0, struct Hook *h), REG(a2, struct FileRequester *fr), REG(a1, struct IntuiMessage *msg))
{
   	struct winData *wd;
	struct Window *typeWindow = msg->IDCMPWindow;
    struct IOType *io;
    long   i;
    static long pos = 0;		//Aktuell gewähltes Modul
    long npos;					//Anz. der vorhandenen Module

	if ((wd = (APTR)typeWindow->UserData) == NULL || wd->wd_Type != WDT_FILETYPE)
		return;
		
	switch (msg->Class) {
		case IDCMP_RAWKEY:
			if (msg->Code == 95) {
				win = typeWindow;
				imsg.MouseX = msg->MouseX;
				imsg.MouseY = msg->MouseY;

				ProcessAppCmd(rxpage, "HELP");
				}
			break;

        case IDCMP_GADGETUP:
            if ((gad = msg->IAddress)->GadgetID == 1)
            {
				struct IOTypeLink *iol;
				UWORD value;
				
				//Anz. der speicher-module bestimmen
				npos = CountNodes(((struct MinList *)wd->wd_ExtData[1])) - 1;

				//Ein Option weiter und beim Ende, wieder auf die Erste
				if(++pos > npos)
					pos = 0;
				StripIntuiMessages(NULL, typeWindow);
				for (iol = (APTR)((struct List *)wd->wd_ExtData[1])->lh_Head, i = 0; i < pos; iol = (APTR)iol->iol_Node.ln_Succ, i++);
				wd->wd_ExtData[0] = io = iol->iol_Link;

				GT_SetGadgetAttrs(GadgetAddress(typeWindow, 1), typeWindow, NULL, GTLV_Selected ,pos, TAG_END);
				GT_SetGadgetAttrs(GadgetAddress(typeWindow, 2), typeWindow, NULL, GTLV_Labels, &iol->iol_Description, TAG_END);
				GT_SetGadgetAttrs(GadgetAddress(typeWindow, 3), typeWindow, NULL, GA_Disabled, !(io->io_Flags & IOF_HASPREFSGUI), TAG_END);
            }
            else if (gad->GadgetID == 3)  // Einstellungen...
            {
				struct IOType *io = wd->wd_ExtData[0];
				
				if (io == NULL)
					break;

                InitIOType(io);

                if (io->io_OpenPrefsGUI)
                    io->io_OpenPrefsGUI(scr);
            }
            break;
    }
    /* Tastaturbedienung... */
}
#else
void PUBLIC
HandleFileTypeIDCMP(REG(a0, struct Hook *h), REG(a2, struct FileRequester *fr), REG(a1, struct IntuiMessage *msg))
{
    struct winData *wd;
	struct Window *typeWindow = msg->IDCMPWindow;
    struct IOType *io;
    long   i;

	if ((wd = (APTR)typeWindow->UserData) == NULL || wd->wd_Type != WDT_FILETYPE)
	return;

	switch (msg->Class) {
		case IDCMP_RAWKEY:
			if (msg->Code == 95) {
				win = typeWindow;
				imsg.MouseX = msg->MouseX;
				imsg.MouseY = msg->MouseY;

				ProcessAppCmd(rxpage, "HELP");
				}
			break;

        case IDCMP_GADGETUP:
            if ((gad = msg->IAddress)->GadgetID == 1)
            {
				struct IOTypeLink *iol;

				for (iol = (APTR)((struct List *)wd->wd_ExtData[1])->lh_Head, i = 0; i < msg->Code; iol = (APTR)iol->iol_Node.ln_Succ, i++);
				wd->wd_ExtData[0] = io = iol->iol_Link;

				GT_SetGadgetAttrs(GadgetAddress(typeWindow, 2), typeWindow, NULL, GTLV_Labels, &iol->iol_Description, TAG_END);
				GT_SetGadgetAttrs(GadgetAddress(typeWindow, 3), typeWindow, NULL, GA_Disabled, !(io->io_Flags & IOF_HASPREFSGUI), TAG_END);
            }
            else if (gad->GadgetID == 3)  // Einstellungen...
            	{
				struct IOType *io = wd->wd_ExtData[0];
				
				if (io == NULL)
					break;

                InitIOType(io);

                if (io->io_OpenPrefsGUI)
                    io->io_OpenPrefsGUI(scr);
            }
            break;
    }
    /* Tastaturbedienung... */
}
#endif

extern struct Gadget *cellPages[];

void PUBLIC
CellWindowLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *n), REG(d0, UBYTE flags))
{
    if (ln->ln_List == RXMAP)  // Lock auf aktuelle Mappe setzen
    {
        struct winData *wd = (APTR)((struct Window *)ln->ln_Data[0])->UserData;

        if (!rxpage)
        {
            CloseAppWindow(ln->ln_Data[0],TRUE);
            return;
        }
        RemLockNode(wd->wd_ExtData[5]);
        wd->wd_ExtData[5] = AddLockNode(&rxpage->pg_Document->mp_Formats,0,CellWindowLock,sizeof(APTR),ln->ln_Data[0]);
    }
    GT_SetGadgetAttrs(PageGadget(cellPages[0],1),ln->ln_Data[0],NULL,GTLV_Labels,&rxpage->pg_Document->mp_Formats,TAG_END);
}


void ASM closeCellWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    RemLockNode(wd->wd_ExtData[5]);
}


void ASM handleCellIDCMP(REG(a0, struct TagItem *tag))
{
    struct Node *ln;
    long   j,i = 0;
    BYTE   ok = 0;

    switch(imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            switch((gad = imsg.IAddress)->GadgetID)
            {
            /****** Farbe & Ausrichtung ******/
                case 6:  // APen
                case 7:  // BPen
                    if ((i = PopColors(win,gad)) != ~0L)
                    {
                        wd->wd_ExtData[gad->GadgetID-6] = (APTR)i;
                        DrawColorField(win->RPort,gad,i,TRUE);
                        GT_SetGadgetAttrs(PageGadget(cellPages[1],gad->GadgetID-2),win,NULL,GTCB_Checked,TRUE,TAG_END);
                    }
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 2);  // Änderungen auf Seite markieren
                    break;
                case 23:  // Pattern
                    if ((i = PopUpTable(win,gad,4,4,PatternPopper,TAG_END)) != ~0L)
                    {
                        wd->wd_Data = (APTR)i;
                        DrawPatternField(win->RPort,gad,(ULONG)wd->wd_ExtData[7],(UBYTE)i);
                        GT_SetGadgetAttrs(PageGadget(cellPages[1],22),win,NULL,GTCB_Checked,TRUE,TAG_END);
                    }
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 2);  // Änderungen auf Seite markieren
                    break;
                case 24:  // Pattern-Color
                    if ((i = PopColors(win,gad)) != ~0L)
                    {
                        wd->wd_ExtData[7] = (APTR)i;
                        DrawPatternField(win->RPort,PageGadget(cellPages[1],23),i,(UBYTE)((long)wd->wd_Data));
                        GT_SetGadgetAttrs(PageGadget(cellPages[1],22),win,NULL,GTCB_Checked,TRUE,TAG_END);
                    }
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 2);  // Änderungen auf Seite markieren
                    break;
            /****** Schrift ******/
                case 11: // Schrift-PopUp
                    i = PopUpList(win,gad = PageGadget(cellPages[2],10),&families,TAG_END);
                    if (i != ~0L)
                    {
                        for(j = 0,ln = (struct Node *)families.mlh_Head;j < i;j++,ln = ln->ln_Succ);
                        wd->wd_ExtData[4] = ln;
                        wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 4);  // Änderungen auf Seite markieren
                        GT_SetGadgetAttrs(gad,win,NULL,GTTX_Text,ln->ln_Name,TAG_END);
                    }
                    break;
                case 13: // Größe-PopUp
                    i = PopUpList(win,gad = PageGadget(cellPages[2],12),&sizes,TAG_END);
                    if (i != ~0L)
                    {
                        for(j = 0,ln = (struct Node *)sizes.mlh_Head;j < i;j++,ln = ln->ln_Succ);
                        wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 4);  // Änderungen auf Seite markieren
                        GT_SetGadgetAttrs(gad,win,NULL,GTST_String,ln->ln_Name,TAG_END);
                    }
                    break;
            }
            break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
            {
            /****** Format ******/
                case 3:
                    GT_SetGadgetAttrs(PageGadget(cellPages[0],2),win,NULL,GTCB_Checked,TRUE,TAG_END);
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 1);  // Änderungen auf Seite markieren
                    break;
                case 1:
                    for(ln = (struct Node *)rxpage->pg_Document->mp_Formats.mlh_Head,i = 0;i < imsg.Code;ln = ln->ln_Succ,i++);
                    wd->wd_ExtData[3] = (ln = ((struct Link *)ln)->l_Link);
                    GT_SetGadgetAttrs(PageGadget(cellPages[0],3),win,NULL,GTSL_Level,((struct FormatVorlage *)ln)->fv_Komma,TAG_END);
#ifdef __amigaos4__
  					DrawPointSliderValue(win->RPort,PageGadget(cellPages[0],3),((struct FormatVorlage *)ln)->fv_Komma);
#endif
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 1);  // Änderungen auf Seite markieren
                    if (IsDoubleClick(imsg.Code))
                        goto handlecellzuweisen;
                    break;
                case 26:  // Format von aktueller Zelle übernehmen
                {
                    struct tableField *tf;

                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 1);  // Änderungen auf Seite markieren

                    if (rxpage->pg_MarkCol != -1)
                        tf = GetFirstCell(rxpage,(struct tablePos *)&rxpage->pg_MarkCol);
                    else
                        tf = rxpage->pg_Gad.tf;

                    if (!tf)  // keine Zelle gefunden
                    {
                        DisplayBeep(NULL);
                        break;
                    }

                    if (tf->tf_Format != NULL)
                    {
                        ln = (struct Node *)FindLinkWithName(&rxpage->pg_Document->mp_Formats,tf->tf_Format);
                        i = FindListEntry(&rxpage->pg_Document->mp_Formats,(struct MinNode *)ln);
                    }
                    else
                    {
                        i = 0;
                        ln = (struct Node *)rxpage->pg_Document->mp_Formats.mlh_Head;
                    }
                    wd->wd_ExtData[3] = ln ? ((struct Link *)ln)->l_Link : NULL;
                    GT_SetGadgetAttrs(PageGadget(cellPages[0],1),win,NULL,GTLV_Selected,i,GTLV_MakeVisible,i,TAG_END);
                    GT_SetGadgetAttrs(PageGadget(cellPages[0],3),win,NULL, GTSL_Level,tf->tf_Komma, TAG_END);
#ifdef __amigaos4__
  					DrawPointSliderValue(win->RPort,PageGadget(cellPages[0],3),tf->tf_Komma);
#endif
                    break;
                }
            /****** Farbe & Ausrichtung ******/
                case 4:
                case 5:
                case 8:
                case 9:
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 2);  // Änderungen auf Seite markieren
                    break;
                case 27:  // Farbe & Ausrichtung von selektierten Zellen übernehmen
                {
                    struct tableField *tf = NULL;
                    ULONG  apen,bpen,patcol;
                    LONG   alignH,alignV;
                    BYTE   pattern,first = TRUE;

                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 2);  // Änderungen auf Seite markieren

                    while ((tf = GetMarkedFields(rxpage, tf, FALSE)) != 0)
                    {
                        if (first)
                        {
                            apen = tf->tf_ReservedPen;
                            bpen = tf->tf_BPen;
                            patcol = tf->tf_PatternColor;
                            pattern = tf->tf_Pattern;
                            alignH = tf->tf_Alignment & TFA_HCENTER;
                            alignV = tf->tf_Alignment & TFA_VCENTER;

                            first = FALSE;
                        }
                        else
                        {
                            if (tf->tf_ReservedPen != apen)
                                apen = ~0L;
                            if (tf->tf_BPen != bpen)
                                bpen = ~0L;
                            if (tf->tf_PatternColor != patcol)
                                patcol = ~0L;
                            if (tf->tf_Pattern != pattern)
                                pattern = -1;
                            if ((tf->tf_Alignment & TFA_HCENTER) != alignH)
                                alignH = -1;
                            if ((tf->tf_Alignment & TFA_VCENTER) != alignV)
                                alignV = -1;
                        }
                    }
                    if (first)  // keine Zelle gefunden
                    {
                        DisplayBeep(NULL);
                        break;
                    }

                    if (apen != ~0L)
                    {
                        wd->wd_ExtData[0] = (APTR)apen;
                        DrawColorField(win->RPort,PageGadget(cellPages[1],6),apen,TRUE);
                    }
                    GT_SetGadgetAttrs(PageGadget(cellPages[1],4),win,NULL,GTCB_Checked,apen != ~0L,TAG_END);

                    if (bpen != ~0L)
                    {
                        wd->wd_ExtData[1] = (APTR)bpen;
                        DrawColorField(win->RPort,PageGadget(cellPages[1],7),bpen,TRUE);
                    }
                    GT_SetGadgetAttrs(PageGadget(cellPages[1],5),win,NULL,GTCB_Checked,bpen != ~0L,TAG_END);

                    if (pattern != -1)
                    {
                        if (patcol == ~0L)
                            wd->wd_ExtData[7] = (APTR)patcol;

                        DrawPatternField(win->RPort,PageGadget(cellPages[1],23),(ULONG)wd->wd_ExtData[7],pattern);
                    }
                    GT_SetGadgetAttrs(PageGadget(cellPages[1],22),win,NULL,GTCB_Checked,pattern != -1,TAG_END);

					switch (alignH)
                    {
                        case -1:          alignH = 0;  break;
                        case TFA_LEFT:    alignH = 2;  break;
                        case TFA_HCENTER: alignH = 3;  break;
                        case TFA_RIGHT:   alignH = 4;  break;
                    }
                    GT_SetGadgetAttrs(PageGadget(cellPages[1],8),win,NULL,GTCY_Active,alignH,TAG_END);

					switch (alignV)
                    {
                        case -1:          alignV = 0;  break;
                        case TFA_TOP:     alignV = 1;  break;
                        case TFA_VCENTER: alignV = 2;  break;
                        case TFA_BOTTOM:  alignV = 3;  break;
                    }
                    GT_SetGadgetAttrs(PageGadget(cellPages[1],9),win,NULL,GTCY_Active,alignV,TAG_END);
                    break;
                }
            /****** Schrift ******/
                case 16:
                case 29:
                case 30:  // bei [doppelt] unter-(/durchge-)strichen darf jeweils nur eines gleichzeitig aktiv sein!
                {
                    int id[] = {16,29,30};

                    for(i = 0;i < 3;i++)
                        if (gad->GadgetID != id[i])
                            GT_SetGadgetAttrs(PageGadget(cellPages[2],id[i]),win,NULL,GTCB_Checked,FALSE,TAG_END);
                }
                case 12:
#ifdef __amigaos4__
				{
					STRPTR t;
				    
					if (GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END))
					{
					    char gt[15];
						    
					    Strlcpy(gt, t, 15);
					    if(!strstr(gt, "pt"))
					    {
					    	Strlcat(gt, " pt", 10);
					    	GT_SetGadgetAttrs(gad, win, NULL, GTST_String, &gt, TAG_END);
					    }
					}
				}
#endif 
                case 14:
                case 15:
                case 17:
                case 18:
                case 19:
                case 20:
                case 21:
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 4);  // Änderungen auf Seite markieren
                    break;
                case 28:  // Schrift von aktueller Zelle übernehmen
                {
                    struct tableField *tf;
                    struct FontInfo *fi;

                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 4);  // Änderungen auf Seite markieren

                    if (rxpage->pg_MarkCol != -1)
                        tf = GetFirstCell(rxpage,(struct tablePos *)&rxpage->pg_MarkCol);
                    else
                        tf = rxpage->pg_Gad.tf;

                    if (!tf)  // keine Zelle gefunden
                    {
                        DisplayBeep(NULL);
                        break;
                    }
                    if (!(fi = tf->tf_FontInfo))
                    {
                        D(bug("handleCellIDCMP: id = 28: no FontInfo!\n"));
                        break;
                    }

                    wd->wd_ExtData[4] = fi->fi_Family;
                    GT_SetGadgetAttrs(PageGadget(cellPages[2],10),win,NULL,GTTX_Text,fi->fi_Family->ln_Name,TAG_END);
                    {
                        char t[20];

                        strcpy(t,ita(fi->fi_FontSize->fs_PointHeight/65536.0,-3,ITA_NONE));
                        strcat(t," pt");

                        GT_SetGadgetAttrs(PageGadget(cellPages[2],12),win,NULL,GTST_String,t,TAG_END);
                    }
                    /* style */
                    GT_SetGadgetAttrs(PageGadget(cellPages[2],14),win,NULL,GTCB_Checked,fi->fi_Style & FS_BOLD,TAG_END);
                    GT_SetGadgetAttrs(PageGadget(cellPages[2],15),win,NULL,GTCB_Checked,fi->fi_Style & FS_ITALIC,TAG_END);
                    GT_SetGadgetAttrs(PageGadget(cellPages[2],16),win,NULL,GTCB_Checked,fi->fi_Style & FS_UNDERLINED,TAG_END);
                    GT_SetGadgetAttrs(PageGadget(cellPages[2],29),win,NULL,GTCB_Checked,fi->fi_Style & FS_STRIKE_THROUGH,TAG_END);
                    GT_SetGadgetAttrs(PageGadget(cellPages[2],30),win,NULL,GTCB_Checked,fi->fi_Style & FS_DOUBLE_UNDERLINED,TAG_END);

                    /** rotate/shear/width **/
                    {
                        char t[16];

                        sprintf(t,"%ld t",fi->fi_FontSize->fs_Shear);
                        GT_GetGadgetAttrs(PageGadget(cellPages[2],18),win,NULL,GTST_String,t,TAG_END);
                    }

                    /** kerning/space **/
                    GT_SetGadgetAttrs(PageGadget(cellPages[2],20),win,NULL,GTCY_Active,fi->fi_Kerning,TAG_END);
                    {
                        char t[20];

                        strcpy(t,ita(fi->fi_CharSpace/65536.0,-3,ITA_NONE));
                        strcat(t," pt");

                        GT_SetGadgetAttrs(PageGadget(cellPages[2],21),win,NULL,GTST_String,t,TAG_END);
                    }
                    break;
                }
            /****** Diverses ******/
                case 25:
                    wd->wd_ExtData[2] = (APTR)((LONG)wd->wd_ExtData[2] | 8);  // Änderungen auf Seite markieren
                    break;
            /****** Ok, Zuweisen & Abbrechen ******/
                /*case 32:
                    bug("page: %ld\n",imsg.Code);
					wd->wd_CurrentPage = (APTR)imsg.Code;
                    break;*/
                case 33:
                    ok = TRUE;
                case 34:
                handlecellzuweisen:
                {
                    struct colorPen *apen,*bpen;
                    struct Node *ln,*font;
                    long   alignH,alignV,size,style,space,width,patcol,security;
                    WORD   rotate,shear;
                    BYTE   iskomma,komma,pages = (BYTE)((long)wd->wd_ExtData[2]),kerning,pattern = -1;

                    /************* remember changes **************/

                    if (pages & 2) /* Farbe & Ausrichtung */
                    {
                        /** Farbe **/

                        apen = GetCheckBoxFlag(PageGadget(cellPages[1],4),win,TRUE) ? GetColorPen((ULONG)wd->wd_ExtData[0]) : NULL;
                        bpen = GetCheckBoxFlag(PageGadget(cellPages[1],5),win,TRUE) ? GetColorPen((ULONG)wd->wd_ExtData[1]) : NULL;

                        /** Muster **/

                        if (GetCheckBoxFlag(PageGadget(cellPages[1],22),win,TRUE))
                        {
                            pattern = (UBYTE)((long)wd->wd_Data);
                            patcol = (long)wd->wd_ExtData[7];
                        }

                        /** Ausrichtung **/

                        GT_GetGadgetAttrs(PageGadget(cellPages[1],8),win,NULL,GTCY_Active,&alignH,TAG_END);
                        switch(alignH)
                        {
                            case 1: alignH = TFA_VIRGIN;  break;
                            case 2: alignH = TFA_LEFT;    break;
                            case 3: alignH = TFA_HCENTER; break;
                            case 4: alignH = TFA_RIGHT;   break;
                        }
                        GT_GetGadgetAttrs(PageGadget(cellPages[1],9),win,NULL,GTCY_Active,&alignV,TAG_END);
                        switch(alignV)
                        {
                            case 1: alignV = TFA_TOP;     break;
                            case 2: alignV = TFA_VCENTER; break;
                            case 3: alignV = TFA_BOTTOM;  break;
                        }
                    }
                    if (pages & 1) /* Format */
                    {
                        iskomma = (BYTE)GetCheckBoxFlag(PageGadget(cellPages[0],2),win,TRUE);
                        GT_GetGadgetAttrs(PageGadget(cellPages[0],3),win,NULL,GTSL_Level,&i,TAG_END);
                        komma = (BYTE)i;
                        ln = wd->wd_ExtData[3];
                    }
                    if (pages & 4) /* Schrift */
                    {
                        font = wd->wd_ExtData[4];  size = ~0L;  space = ~0L;
                        GT_GetGadgetAttrs(PageGadget(cellPages[2],12),win,NULL,GTST_String,&i,TAG_END);
                        if (i)
                            size = (long)(ConvertNumber((STRPTR)i,CNT_POINT)*65536+0.5);

                        /** style **/
                        style = GetCheckBoxFlag(PageGadget(cellPages[2],14),win,TRUE) ? FS_BOLD : FS_PLAIN;
                        style |= GetCheckBoxFlag(PageGadget(cellPages[2],15),win,FS_ITALIC) |
                                         GetCheckBoxFlag(PageGadget(cellPages[2],16),win,FS_UNDERLINED) |
                                         GetCheckBoxFlag(PageGadget(cellPages[2],29),win,FS_STRIKE_THROUGH) |
                                         GetCheckBoxFlag(PageGadget(cellPages[2],30),win,FS_DOUBLE_UNDERLINED);

                        /** rotate/shear/width **/
                        rotate = ~0L;
                        GT_GetGadgetAttrs(PageGadget(cellPages[2],18),win,NULL,GTST_String,&i,TAG_END);
                        if (i)
                            shear = atol((STRPTR)i);
                        width = ~0L;

                        /** kerning/space **/
                        GT_GetGadgetAttrs(PageGadget(cellPages[2],20),win,NULL,GTCY_Active,&i,TAG_END);  kerning = i;
                        GT_GetGadgetAttrs(PageGadget(cellPages[2],21),win,NULL,GTST_String,&i,TAG_END);
                        if (i)
                            space = (long)(ConvertNumber((STRPTR)i,CNT_POINT)*65536.0);
                    }
                    if (pages & 8) /* Diverses */
                    {
                        /** Zellschutz **/

                        GT_GetGadgetAttrs(PageGadget(cellPages[3],25),win,NULL,GTCY_Active,&security,TAG_END);
                        switch(security)
                        {
                            case 0: security = -1;              break;
                            case 1: security = 0;               break;
                            case 2: security = TFF_STATIC;      break;
                            case 3: security = TFF_HIDEFORMULA; break;
                            case 4: security = TFF_HIDETEXT;    break;
                        }
                    }
                    if (ok)
                        CloseAppWindow(win,TRUE);

                    /************ make changes ************/

                    if (pages & 1 && ln) /* Format */
                    {
                        struct tableField *tf = NULL;
                        long   maxcol = 0;

                        BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_SET_FORMAT_UNDO));
                        while ((tf = GetMarkedFields(rxpage, tf, TRUE)) != 0)
                        {
                            FreeString(tf->tf_Format);
                            if (ln->ln_Type == FVT_NONE)
                            {
                                tf->tf_Format = NULL;
                                tf->tf_Flags &= ~(TFF_KOMMASET | TFF_FORMATSET);
                            }
                            else
                            {
                                tf->tf_Format = AllocString(ln->ln_Name);
                                if (iskomma)
                                    tf->tf_Komma = komma;
                                if (((struct FormatVorlage *)ln)->fv_NegativePen != ~0L && tf->tf_Value < 0.0)
                                    tf->tf_APen = ((struct FormatVorlage *)ln)->fv_NegativePen;
                                tf->tf_Flags |= (iskomma ? TFF_KOMMASET : 0) | TFF_FORMATSET;
                            }
                            if (maxcol < tf->tf_Col+tf->tf_Width)
                                maxcol = tf->tf_Col+tf->tf_Width;
                            SetTFText(rxpage,tf,(STRPTR)~0L);
                            if (maxcol < tf->tf_Col+tf->tf_Width)
                                maxcol = tf->tf_Col+tf->tf_Width;
                        }
                        EndUndo(rxpage);
                        RefreshMarkedTable(rxpage,maxcol,TRUE);
                    }
                    if (pages & 2) /* Farbe & Ausrichtung */
                    {
                        SetPageColor(rxpage,apen,bpen);
						if (pattern != -1)
                            SetCellPattern(rxpage,patcol,pattern);
                        if (alignH || alignV)
                        {
                            SetAlignment(rxpage,(BYTE)alignH,(BYTE)alignV);
                            if (alignH == TFA_VIRGIN)
                                RecalcTableFields(rxpage);
                        }
                    }
                    if (pages & 4) /* Schrift */
                    {
						UpdatePageFont(rxpage,
							FA_Family,      (struct FontFamily *)font,
							FA_Style,       style | FS_ALLBITS,
							FA_PointHeight, size,
							FA_Rotate,      rotate,
							FA_Shear,       shear,
							FA_Kerning,     kerning,
							FA_Space,       space,
							TAG_IGNORE,     width,
							TAG_END);
                    }
                    if (pages & 8) /* Diverses*/
						SetCellSecurity(rxpage, security);
                    break;
                }
                case 35:
					CloseAppWindow(win, TRUE);
                    break;
            }
            break;
        case IDCMP_MOUSEMOVE:
            if ((struct Gadget *)imsg.IAddress == PageGadget(cellPages[0],3))
                DrawPointSliderValue(win->RPort,(struct Gadget *)imsg.IAddress,imsg.Code);
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
handleSetNameIDCMP(REG(a0, struct TagItem *tag))
{
	switch (imsg.Class)
    {
        case IDCMP_GADGETUP:
            if (rxpage)
            {
                STRPTR t;

                if (GT_GetGadgetAttrs(imsg.IAddress,win,NULL,GTST_String,&t,TAG_END))
                {
                    char s[256];

                    strcpy(s,"CELLNAME SET=");
                    strcat(s,t);
                    processIntCmd(s);
                }
            }
            else
                DisplayBeep(NULL);
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM handleSetTitleIDCMP(REG(a0, struct TagItem *tag))
{
    switch(imsg.Class)
    {
        case IDCMP_GADGETUP:
            if (rxpage)
            {
                STRPTR t;

                if (GT_GetGadgetAttrs(imsg.IAddress,win,NULL,GTST_String,&t,TAG_END))
                {
                    char s[256];

                    strcpy(s,"TITLE SET=\"");
                    strcat(s,t);
                    strcat(s,"\"");
                    if (wd->wd_Data)
                        strcat(s," VERT");

                    processIntCmd(s);
                }
            }
            else
                DisplayBeep(NULL);
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void
SetScriptText(struct winData *wd, struct RexxScript *rxs)
{
    STRPTR t;

    if (!rxs)
        return;

#ifdef __amigaos4__
	if(t = (STRPTR)DoGadgetMethod (wd->wd_ExtData[1], wd->wd_Mother, NULL, GM_TEXTEDITOR_ExportText, NULL)){
#else
	if (GetAttr(EGA_Text, wd->wd_ExtData[1], (IPTR *)&t)) {
#endif
        FreeRexxScriptData(rxs);
        rxs->rxs_Data = AllocString(t);
		rxs->rxs_DataLength = zstrlen(t) + 1;
    }
}

void
UpdateScriptsGadgets(struct Window *win, struct RexxScript *rxs)
{
    struct RexxScript *oldrxs;
    struct winData *wd;

    if (!win)
        return;

    wd = (struct winData *)win->UserData;

    if ((oldrxs = ((struct Gadget *)wd->wd_ExtData[0])->UserData) && rxs != oldrxs)
		SetScriptText(wd, oldrxs);

    ((struct Gadget *)wd->wd_ExtData[0])->UserData = rxs;

	if (rxs) {
        struct Mappe *mp = wd->wd_Data;

		GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Selected, FindListEntry(&mp->mp_RexxScripts, (struct MinNode *)rxs), TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 3), win, NULL, GA_Disabled, FALSE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 4), win, NULL, GTST_String, rxs->rxs_Node.ln_Name, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 5), win, NULL, GTST_String, rxs->rxs_Description, GA_Disabled, FALSE, TAG_END);
#ifdef __amigaos4__
		SetGadgetAttrs (wd->wd_ExtData[1], win, NULL, GA_Disabled, FALSE, GA_TEXTEDITOR_Contents, rxs->rxs_Data, GA_TEXTEDITOR_CursorX, 0, GA_TEXTEDITOR_CursorY, 1, TAG_END);
#else
		SetGadgetAttrs(wd->wd_ExtData[1], win, NULL, EGA_Text, rxs->rxs_Data, GA_Disabled, FALSE, TAG_END);
#endif
		GT_SetGadgetAttrs(GadgetAddress(win, 7),win, NULL, GA_Disabled, FALSE, TAG_END);
	} else {
		GT_SetGadgetAttrs(GadgetAddress(win, 3),win, NULL, GA_Disabled, TRUE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 4),win, NULL, GTST_String, NULL, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 5),win, NULL, GA_Disabled, TRUE, TAG_END);
		SetGadgetAttrs(wd->wd_ExtData[1], win, NULL, GA_Disabled, TRUE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 7),win, NULL, GA_Disabled, TRUE, TAG_END);
    }
}


void PUBLIC
ScriptsWindowLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *n), REG(d0, UBYTE flags))
{
	ListViewLock(ln, n, flags);

	if ((flags & (LNCMD_UNLOCK | LNF_REMOVE)) == (LNCMD_UNLOCK | LNF_REMOVE)) {
        if (!((struct Gadget *)ln->ln_Data[1])->UserData)
			UpdateScriptsGadgets(ln->ln_Data[0], NULL);
	} else if (flags & LNCMD_REFRESH)
		UpdateScriptsGadgets(ln->ln_Data[0], ((struct Gadget *)ln->ln_Data[1])->UserData);
}


void ASM
CloseScriptsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    SetScriptText(wd,((struct Gadget *)wd->wd_ExtData[0])->UserData);

    RemLockNode(wd->wd_ExtData[5]);
}


void ASM
HandleScriptsIDCMP(REG(a0, struct TagItem *tag))
{
    struct Mappe *mp = wd->wd_Data;
	struct RexxScript *rxs = ((struct Gadget *)wd->wd_ExtData[0])->UserData;

	switch (imsg.Class) {
        case IDCMP_RAWKEY:
			rxs = (APTR)HandleLVRawKeys(wd->wd_ExtData[0], win, &mp->mp_RexxScripts, fontheight);
			UpdateScriptsGadgets(win, rxs);
            break;

#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(imsg.Code == ESCAPE_KEY)
			{
				CloseAppWindow(win, true);
				break;
			}
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
			{
				case 1:	// listview
					UpdateScriptsGadgets(win, (APTR)FindListNumber(&mp->mp_RexxScripts, imsg.Code));
                    break;
				case 2:	// new
                    if (rxs)
						UpdateScriptsGadgets(win, NULL);
					ActivateGadget(GadgetAddress(win, 4), win, NULL);
                    break;
				case 3:	// delete
					DeleteRexxScript(mp, rxs);
                    break;
				case 4:	// name
                {
                    STRPTR t;
					if (!GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END) || t == NULL || !t[0])
						break;

					if (rxs == NULL) {
						// create new script

						if (!(rxs = (struct RexxScript *)FindTag(&mp->mp_RexxScripts, t)))
						    rxs = NewRexxScript(mp, t, TRUE);

						UpdateScriptsGadgets(win, rxs);
					} else {
						// change script name

						GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, ~0L, TAG_END);
						FreeString(rxs->rxs_Node.ln_Name);
						rxs->rxs_Node.ln_Name = AllocString(t);
						GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, &mp->mp_RexxScripts, TAG_END);
					}

					ActivateGadget(GadgetAddress(win, 5), win, NULL);
                    break;
                }
				case 5:	// description
					if (rxs) {
                        STRPTR t;

						if (GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END)) {
                            FreeString(rxs->rxs_Description);
                            rxs->rxs_Description = AllocString(t);
                        }
						ActivateGadget(wd->wd_ExtData[1], win, NULL);
                    }
                    break;
                case 6: // Ok
					CloseAppWindow(win, TRUE);
                    break;
                case 7: // Editor
					SetScriptText(wd, rxs);
					EditRexxScript(mp, rxs);
                    break;
            }
            break;
        case IDCMP_NEWSIZE:
			StandardNewSize(CreateScriptsGadgets);
            break;
        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;
    }
}
