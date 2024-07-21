/* Preferences window handling functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#if defined __amigaos4__ || defined __MORPHOS__
	#include <proto/gtdrag.h>
	#include <ctype.h>
#endif


extern void CreatePrefsGadgets(struct winData *wd, long width, long height);
extern void CreateKeyboardPrefsGadgets(struct winData *wd, long width, long height);
extern struct Gadget *PageGadget(struct Gadget *gad, long num);

extern struct RastPort *doublerp;


void ASM
closePrefsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	if (clean) {
		while ((win = GetAppWindow(WDT_PREFCHOICE)) != 0) {
			CloseAppWindow(win, TRUE);
		}
    }
}


void
DisenablePrefsGadgets(long disable)
{
	GT_SetGadgetAttrs(GadgetAddress(win, wd->wd_ShortCuts[0]), win, NULL, GA_Disabled, disable, TAG_END);
	GT_SetGadgetAttrs(GadgetAddress(win, wd->wd_ShortCuts[1]), win, NULL, GA_Disabled, disable, TAG_END);
}


void ASM
handlePrefsIDCMP(REG(a0, struct TagItem *tag))
{
    struct TreeNode *tn;
	long   i, id;

	switch (imsg.Class) {
        case IDCMP_RAWKEY:
            if (HandleLVRawKeys(GadgetAddress(win,1),win,&prefstree.tl_View,itemheight))
                DisenablePrefsGadgets(FALSE);
            break;
        case IDCMP_GADGETUP:
#ifndef __amigaos4__
			if ((WORD)imsg.Code < 0)
				break;
#endif
            if ((gad = imsg.IAddress)->GadgetID == 1)
            {
                for(tn = (APTR)prefstree.tl_View.mlh_Head,i = 0;i < imsg.Code;tn = (APTR)tn->tn_Node.in_Succ,i++);
                gad->UserData = tn;  tn = TREENODE(tn);

                DisenablePrefsGadgets(FALSE);
                if (ToggleTree(gad,tn,&imsg))
                {
                    if (tn->tn_Flags & (TNF_ADD | TNF_REPLACE))
                        RefreshPrefsTreeNode(tn);
                }
                else 
                if (IsDoubleClick(imsg.Code))
                {
                    struct PrefsModule *pm;

					pm = tn->tn_Special;
					if(pm == NULL || pm == (APTR)0xFFFFFFFF)
						break;
                    if (pm->pm_Node.in_Type == LNT_PREFSMODULE)
                        OpenAppWindow(pm->pm_Type,WA_Data,(tn = GetTreeContainer(tn)) ? tn->tn_Special : NULL,TAG_END);
                }
                break;
            }
            id = gad->GadgetID;
        case IDCMP_VANILLAKEY:
            if (imsg.Code == 13 && (gad = GadgetAddress(win,1)) && (tn = gad->UserData))
            {
                struct PrefsModule *pm;

                tn = TREENODE(tn);
                if ((pm = tn->tn_Special) && pm->pm_Node.in_Type == LNT_PREFSMODULE)
                    OpenAppWindow(pm->pm_Type,WA_Data,(tn = GetTreeContainer(tn)) ? tn->tn_Special : NULL,TAG_END);
                else if (tn->tn_Flags & TNF_CONTAINER)
                {
                    long top = 0;

                    GT_GetGadgetAttrs(gad,win,NULL,GTLV_Top,&top,TAG_END);
                    GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,~0L,TAG_END);
                    if ((i = ToggleTreeNode(&prefstree.tl_Tree,tn)) < (gad->Height-4)/itemheight)
                        top += i;
                    GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&prefstree.tl_View,GTLV_Top,top,TAG_END);
                }
                break;
            }
            if (imsg.Class == IDCMP_VANILLAKEY)
                id = imsg.Code;
            if (id == wd->wd_ShortCuts[0])       // Hinzufügen
            {
                if ((gad = GadgetAddress(win,1)) && (tn = gad->UserData))
                {
                    tn = TREENODE(tn);
                    if ((tn->tn_Flags & TNF_CONTAINER || (tn = GetTreeContainer(tn))) && tn->tn_Depth)
                        OpenAppWindow(WDT_PREFCHOICE,WA_Data,tn->tn_Special,TAG_END);
                    else if (tn && !tn->tn_Special)
                        ErrorRequest(GetString(&gLocaleInfo, MSG_GLOBAL_PREFS_HAVE_ALL_MODULES_ERR));
                    else
                        DisplayBeep(NULL);
                }
                break;
            }
            else if (id == wd->wd_ShortCuts[1])  // Entfernen
            {
                struct PrefsModule *pm;
                struct Gadget *gad;

                if ((gad = GadgetAddress(win,1)) && (tn = TREENODE(gad->UserData)) && (pm = tn->tn_Special) && pm != (APTR)~0L && pm->pm_Node.in_Type == LNT_PREFSMODULE)
                {
                    struct TreeNode *ctn;

                    if ((ctn = GetTreeContainer(tn)) != 0)
                    {
                        struct Mappe *mp;

                        if (ctn == prefs.pr_TreeNode)   // global
                        {
                            if (DoRequest(GetString(&gLocaleInfo, MSG_REMOVE_GLOBAL_PREFS_REQ),GetString(&gLocaleInfo, MSG_YES_NO_REQ)))
                            {
                                FreePrefsModuleData(&prefs,pm);
                                AllocPrefsModuleData(&prefs,pm->pm_Type);
                                RefreshPrefsModule(&prefs,pm,0);
                                pm->pm_Flags |= PMF_MODIFIED;
                            }
                            else
                                break;
                        }
                        else if (ctn->tn_Special == (APTR)~0L)  // Prefs-Ablage
                            RemPrefsModule(&recycledprefs,pm);
                        else if ((mp = ctn->tn_Special) != 0)          // lokal
                            RemPrefsModule(&mp->mp_Prefs,pm);

                        gad->UserData = NULL;
                        GT_SetGadgetAttrs(gad,win,NULL,GTLV_Selected,~0L,TAG_END);
                        DisenablePrefsGadgets(TRUE);
                    }
                }
                break;
            }
            else if (id != wd->wd_ShortCuts[2] && id != ESCAPE_KEY)  // nicht "Ok"
                break;
        case IDCMP_CLOSEWINDOW:       // muß hinter IDCMP_VANILLAKEY stehen
            CloseAppWindow(win,TRUE);
            break;
        case IDCMP_NEWSIZE:
        {
            struct Gadget *gad;

            GTD_RemoveGadgets(win);
            RemLockNode(wd->wd_Lock);
			StandardNewSize(CreatePrefsGadgets);

            gad = GadgetAddress(win,1);
            GTD_AddGadget(LISTVIEW_KIND,gad,win,GTDA_Same,        TRUE,
	                                            GTDA_AcceptTypes, DRAGT_PREFS,
	                                            GTDA_InternalType,DRAGT_PREFS,
	                                            GTDA_ItemHeight,  itemheight,
	                                            GTDA_TreeView,    TRUE,
	                                            GTDA_InternalOnly,TRUE,
	                                            TAG_END);
            wd->wd_Lock = AddTreeLock((struct MinList *)&prefstree,win,gad);
            break;
        }
        case IDCMP_OBJECTDROP:
        {
            struct DropMessage *dm = imsg.IAddress;

            if (dm && !dm->dm_Object.od_Owner && dm->dm_Object.od_Type == ODT_TREENODE)
            {
                if ((tn = TREENODE(dm->dm_Object.od_Object))->tn_Flags & TNF_CONTAINER)
                    ErrorRequest(GetString(&gLocaleInfo, MSG_MOVE_PREFS_FOLDER_ERR));
                else
                {
                    struct TreeNode *ttn,*ctn;

                    for(ttn = (APTR)prefstree.tl_View.mlh_Head,i = 0;i < dm->dm_TargetEntry;ttn = (APTR)ttn->tn_Node.in_Succ,i++);

                    if (TREENODE(ttn)->tn_Flags & TNF_CONTAINER && !(dm->dm_Flags & DMF_DROPOVER))
                        ttn = (APTR)ttn->tn_Node.in_Pred;

                    if (!((ttn = TREENODE(ttn))->tn_Flags & TNF_CONTAINER))
                        ttn = GetTreeContainer(ttn);

                    if ((ctn = GetTreeContainer(tn)) != ttn)
                    {
                        if ((imsg.Qualifier & IEQUALIFIER_SHIFT) && ctn == (struct TreeNode *)prefstree.tl_Tree.mlh_Head)
                            ErrorRequest(GetString(&gLocaleInfo, MSG_MOVE_FROM_GLOBAL_PREFS_ERR));
                        else
						{
                            CopyPrefsModule(ctn,tn,ttn);
						}
                    }
                    else
                        ErrorRequest(GetString(&gLocaleInfo, MSG_MOVE_PREFS_IN_FOLDER_ERR));
                }
            }
            break;
        }
    }
}


void ASM
ClosePrefChoiceWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    if (clean && wd->wd_ExtData[1])
    {
        struct PrefsModule *pm;

        while((pm = (APTR)MyRemHead(wd->wd_ExtData[1])) != 0)      // flat copy
            FreePooled(pool,pm,sizeof(struct PrefsModule));
        FreePooled(pool,wd->wd_ExtData[1],sizeof(struct MinList));
    }
}


void ASM HandlePrefChoiceIDCMP(REG(a0, struct TagItem *tag))
{
    struct PrefsModule *pm;
    long   id,i;

    switch(imsg.Class)
    {
        case IDCMP_RAWKEY:
            HandleLVRawKeys(GadgetAddress(win,1),win,wd->wd_ExtData[1],itemheight);
            break;
        case IDCMP_GADGETUP:
            if ((gad = imsg.IAddress)->GadgetID == 1)
            {
                for(pm = (APTR)((struct MinList *)wd->wd_ExtData[1])->mlh_Head,i = 0;i < imsg.Code;pm = (APTR)pm->pm_Node.in_Succ,i++);
                gad->UserData = pm;
                goto prefchoiceok;
                break;
            }
            id = gad->GadgetID;
        case IDCMP_VANILLAKEY:
            if (imsg.Class == IDCMP_VANILLAKEY)
                id = imsg.Code;
            if (id == 13 || id == wd->wd_ShortCuts[0])  // Ok
            {
                prefchoiceok:    // von IDCMP_GADGETUP

                if ((gad = GadgetAddress(win,1)) && (pm = gad->UserData))
                {
                    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

                    if (HasPrefsModule(pr,pm->pm_Type))
                        ErrorRequest(GetString(&gLocaleInfo, MSG_PREFS_ALREADY_EXIST_IN_FOLDER_ERR),pm->pm_Node.in_Name,((struct Node *)wd->wd_Data)->ln_Name);
                    else if (LockList((struct MinList *)&prefstree,LNF_REFRESH))
                    {
                        struct PrefsModule *cpm;

                        if ((cpm = AddPrefsModule(pr, pm->pm_Node.in_Name, pm->pm_ImageName, pm->pm_Type, pm->pm_Flags)) != 0)
                            AddPrefsModuleToTree(pr,cpm,&pr->pr_TreeNode->tn_Nodes);

                        UnlockList((struct MinList *)&prefstree,LNF_REFRESH);
                        RefreshPrefsModule(pr,cpm,cpm->pm_Type);
                    }
                }
            }
            else if (id != wd->wd_ShortCuts[1] && id != ESCAPE_KEY)
                break;
        case IDCMP_CLOSEWINDOW:            // muß hinter IDCMP_VANILLAKEY stehen
            CloseAppWindow(win,TRUE);
            break;
    }
}


void CopyPrefDisp(struct PrefDisp *from,struct Prefs *pr)
{
    APTR   font;

    if (!from)
        return;

    font = pr->pr_Disp->pd_AntiFont;  // es soll der alte Font freigegeben werden
    CopyMem(from,pr->pr_Disp,sizeof(struct PrefDisp));
    pr->pr_Disp->pd_AntiFont = font;
    UpdateAntiFont(pr);
}


void ASM closePrefDispWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    if (!clean)
        return;
    if (wd->wd_ExtData[1])
    {
        struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

        if (wd->wd_ExtData[2])  // new local prefs
            RemPrefsModule(pr,GetPrefsModule(pr,WDT_PREFDISP));
        else  // restore old prefs
            CopyPrefDisp(wd->wd_ExtData[1],pr);

        FreePooled(pool,wd->wd_ExtData[1],sizeof(struct PrefDisp));
    }
}


void ClosePrefDispWindow(struct Prefs *pr)
{
    BYTE refresh;

    refresh = (BYTE)((long)(wd->wd_ExtData[0]));
    CloseAppWindow(win,TRUE);
    if (refresh)
        RefreshPrefsModule(pr,NULL,WDT_PREFDISP);
}


void ASM handlePrefDispIDCMP(REG(a0, struct TagItem *tag))
{
    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
    struct PrefDisp *pd = pr->pr_Disp;
    struct Gadget *gad;
    char   t[256];
    long   i;
#ifdef __amigaos4__	
    STRPTR aktfont;
#endif

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
                case 4:
#ifdef __amigaos4__	
 					aktfont = AllocString((STRPTR)pd->pd_AntiAttr.ta_Name);
#endif
                    if (AslRequestTags(fontReq,	ASLFO_Window, 		win,
#ifdef __amigaos4__	
                                              	ASLFO_InitialName, 	aktfont,
#else
                                               	ASLFO_InitialName, 	pd->pd_AntiAttr.ta_Name,
#endif
                                                ASLFO_InitialSize, 	pd->pd_AntiAttr.ta_YSize,
                                                TAG_END))
                    {
                        strcpy(t,fontReq->fo_Attr.ta_Name);
#ifdef __amigaos4__	
                        FreeString((STRPTR)pd->pd_AntiAttr.ta_Name);
                        pd->pd_AntiAttr.ta_Name = AllocString(aktfont);
                        FreeString(aktfont);
#else
                        FreeString((STRPTR)pd->pd_AntiAttr.ta_Name);
#endif
                        pd->pd_AntiAttr.ta_Name = AllocString(fontReq->fo_Attr.ta_Name);
                        pd->pd_AntiAttr.ta_YSize = fontReq->fo_Attr.ta_YSize;
                        UpdateAntiFont(pr);

                        sprintf(t,"%s/%ldpt",pd->pd_AntiAttr.ta_Name,pd->pd_AntiAttr.ta_YSize);
                        GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTTX_Text,t,TAG_END);
                        wd->wd_ExtData[0] = (APTR)TRUE;
                    }
                    break;
                case 9:      // Ok
                case 10:     // Test
                    GT_GetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTCY_Active,&i,TAG_END);
                    pd->pd_Rasta = i | GetCheckBoxFlag(GadgetAddress(win,12),win,PDR_CELLWIDTH);
                    pd->pd_ShowAntis = GetCheckBoxFlag(GadgetAddress(win,2),win,TRUE);
                    pd->pd_HelpBar = GetCheckBoxFlag(GadgetAddress(win,5),win,TRUE);
                    pd->pd_ToolBar = GetCheckBoxFlag(GadgetAddress(win,6),win,TRUE);
                    GT_GetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTCY_Active,&i,TAG_END);
                    pd->pd_FormBar = i;
                    GT_GetGadgetAttrs(GadgetAddress(win,8),win,NULL,GTCY_Active,&i,TAG_END);
                    pd->pd_IconBar = i;

                    if (wd->wd_ExtData[0] && gad->GadgetID == 10)  // Testen
                    {
                    	RefreshPrefsModule(pr,NULL,WDT_PREFDISP);
                    }

                    if (gad->GadgetID == 9)  // Ok
                    {
                        FreePooled(pool,wd->wd_ExtData[1],sizeof(struct PrefDisp));
                        wd->wd_ExtData[1] = NULL;

                        SetPrefsModule(pr,WDT_PREFDISP,TRUE);
                        ClosePrefDispWindow(pr);
                    }
                    break;
                case 11:     // Cancel
                    ClosePrefDispWindow(pr);
                    break;
                default:
                    wd->wd_ExtData[0] = (APTR)TRUE;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            ClosePrefDispWindow(pr);
            break;
    }
}

void ASM
handlePrefScreenIDCMP(REG(a0, struct TagItem *tag))
{
    struct Gadget *gad;
    struct MinList list;
    struct List *l;
    long   i;
    UWORD  id;
    struct Node *sn,*ln;

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 2)
            {
                if ((l = LockPubScreenList()) != 0)
                {
                    MyNewList(&list);
                    for(sn = l->lh_Head;sn->ln_Succ;sn = sn->ln_Succ)
                    {
                        if (strcmp(pubname,sn->ln_Name) && (ln = AllocPooled(pool,sizeof(struct Node))))
                        {
                            ln->ln_Name = AllocString(sn->ln_Name);
                            MyAddTail(&list, ln);
                        }
                    }
                    UnlockPubScreenList();
                    i = PopUpList(win,gad = GadgetAddress(win,1),(struct MinList *)&list,POPA_MaxItems,5,TAG_END);
                    if (i != ~0L)
                    {
                        for(ln = (struct Node *)list.mlh_Head;i && ln->ln_Succ;ln = ln->ln_Succ,i--);
                        GT_SetGadgetAttrs(gad,win,NULL,GTST_String,ln->ln_Name,TAG_END);
                        GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTMX_Active,0,TAG_END);
                    }
                    while((ln = MyRemHead(&list)) != 0)
                    {
                        FreeString(ln->ln_Name);
                        FreePooled(pool,ln,sizeof(struct Node));
                    }
                }
            }
            else if (gad->GadgetID == 9)
            {
                i = PopUpList(win,gad,&scrcolors,POPA_CallBack,&colorHook,POPA_MaxPen,~0L,POPA_ItemHeight,fontheight+4,POPA_Width,GetListWidth(&scrcolors)+boxwidth+5,TAG_END);
                if (i != ~0L)
                {
                    wd->wd_Data = (APTR)i;
                    DrawColorField(win->RPort,GadgetAddress(win,9),i,FALSE);
                }
            }
            break;
        case IDCMP_VANILLAKEY:
            id = imsg.Code;
        case IDCMP_GADGETUP:
            if (imsg.Class == IDCMP_GADGETUP)
                id = ((struct Gadget *)imsg.IAddress)->GadgetID;

			switch (id)
            {
                case 6:
					ActivateGadget(GadgetAddress(win, 7), win, NULL);
                    break;
                default:
                    wd->wd_ExtData[0] = (APTR)TRUE;
                    if (id == wd->wd_ShortCuts[0])
                    {
                        GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTMX_Active,0,TAG_END);
                        ActivateGadget(GadgetAddress(win,1),win,NULL);
                    }
                    else if (id == wd->wd_ShortCuts[1])
                        GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTMX_Active,1,TAG_END);
                    else if (id == wd->wd_ShortCuts[2])
                        GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTMX_Active,2,TAG_END);
                    else if (id == wd->wd_ShortCuts[5])
                        ActivateGadget(GadgetAddress(win,6),win,NULL);
                    else if (id == wd->wd_ShortCuts[6])
                        ActivateGadget(GadgetAddress(win,7),win,NULL);
                    else if (id == wd->wd_ShortCuts[7] && imsg.Class == IDCMP_VANILLAKEY)
                    {
						// toggle pattern check box
						GT_GetGadgetAttrs(GadgetAddress(win, wd->wd_ShortCuts[7]), win, NULL, GTCB_Checked, &i, TAG_END);
						GT_SetGadgetAttrs(GadgetAddress(win, wd->wd_ShortCuts[7]), win, NULL, GTCB_Checked, !i, TAG_END);
                    }
                    else if (id == wd->wd_ShortCuts[3])       // Screen-Req
                    {
                        if (AslRequestTags(scrReq,ASLSM_Window,win,TAG_END))
                            GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTMX_Active,2,TAG_END);
                    }
                    else if (id == wd->wd_ShortCuts[4])       // Font-Req
                    {
                        if (AslRequestTags(fontReq,ASLFO_Window,win,TAG_END))
                            GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTMX_Active,2,TAG_END);
                    }
                    else if (id == wd->wd_ShortCuts[8])       // Ok
                    {
                        SetPrefsModule(&prefs,WDT_PREFSCREEN,TRUE);
                        GT_GetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTST_String,&i,TAG_END);
                        strcpy(prefs.pr_Screen->ps_PubName,(STRPTR)i);
                        GT_GetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTMX_Active,&i,TAG_END);
                        prefs.pr_Screen->ps_Type = i;
                        GT_GetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,&i,TAG_END);
                        prefs.pr_Screen->ps_mmWidth = (long)(ConvertNumber((STRPTR)i,CNT_MM)*1024.0);
                        GT_GetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,&i,TAG_END);
                        prefs.pr_Screen->ps_mmHeight = (long)(ConvertNumber((STRPTR)i,CNT_MM)*1024.0);
                        prefs.pr_Screen->ps_BackFill = (BOOL)GetCheckBoxFlag(GadgetAddress(win,wd->wd_ShortCuts[7]),win,TRUE);
                        prefs.pr_Screen->ps_BFColor = (LONG)wd->wd_Data;
                        prefs.pr_Screen->ps_ModeID = scrReq->sm_DisplayID;
                        prefs.pr_Screen->ps_Width = scrReq->sm_DisplayWidth;
                        prefs.pr_Screen->ps_Height = scrReq->sm_DisplayHeight;
                        prefs.pr_Screen->ps_Depth = scrReq->sm_DisplayDepth;
                        FreeString((STRPTR)prefs.pr_Screen->ps_TextAttr.ta_Name);
                        prefs.pr_Screen->ps_TextAttr.ta_Name = AllocString(fontReq->fo_Attr.ta_Name);
                        prefs.pr_Screen->ps_TextAttr.ta_YSize = fontReq->fo_Attr.ta_YSize;

                        PropagatePrefsModule(&prefs,WDT_PREFSCREEN);
                        CloseAppWindow(win,TRUE);
                        ChangeAppScreen(TRUE);
                    }
                    else if (id == wd->wd_ShortCuts[9] || id == ESCAPE_KEY)
                        CloseAppWindow(win,TRUE);
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM closePrefMenuWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    GTD_RemoveGadgets(win);

    if (clean)
    {
        FreeAppMenus(wd->wd_ExtData[3]);
        FreePooled(pool,wd->wd_ExtData[3],sizeof(struct List));
    }
}


void updatePrefMenuGads(struct IgnAppMenu *am,struct IgnAppMenuEntry *ame,struct IgnAppMenuEntry *same)
{
    UBYTE dame,dsame;

    if (!am)
        ame = NULL;
    if (!ame)
        same = NULL;
    ((struct Gadget *)wd->wd_ExtData[0])->UserData = am;
    ((struct Gadget *)wd->wd_ExtData[1])->UserData = ame;
    ((struct Gadget *)wd->wd_ExtData[2])->UserData = same;

    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,   wd->wd_ExtData[3],
                                                                                             GTLV_Selected, FindListEntry(wd->wd_ExtData[3],(struct MinNode *)am),
                                                                                             GA_Disabled,   IsListEmpty((struct List *)wd->wd_ExtData[3]),
                                                                                             TAG_END);
    GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,   am ? &am->am_Items : NULL,
                                                                                             GA_Disabled,   (am && !IsListEmpty((struct List *)&am->am_Items)) ? FALSE : TRUE,
                                                                                             GTLV_Selected, am ? FindListEntry(&am->am_Items,(struct MinNode *)ame) : ~0L,
                                                                                             TAG_END);
    GT_SetGadgetAttrs(wd->wd_ExtData[2],win,NULL,GTLV_Labels,   ame ? &ame->am_Subs : NULL,
                                                                                             GA_Disabled,   (ame && !IsListEmpty((struct List *)&ame->am_Subs)) ? FALSE : TRUE,
                                                                                             GTLV_Selected, ame ? FindListEntry(&ame->am_Subs,(struct MinNode *)same) : ~0L,
                                                                                             TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GA_Disabled,!am,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GA_Disabled,!ame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,!am,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,!ame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,9),win,NULL,GA_Disabled,!same,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,10),win,NULL,GTST_String,am ? am->am_Node.ln_Name : NULL,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,11),win,NULL,GTST_String,ame ? ame->am_Node.ln_Name : NULL,GA_Disabled,!am,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,12),win,NULL,GTST_String,same ? same->am_Node.ln_Name : NULL,GA_Disabled,!ame,TAG_END);
    dame = !ame || !IsListEmpty((struct List *)&ame->am_Subs) || !strcmp(ame->am_Node.ln_Name,"-");
    dsame = !same || !strcmp(same->am_Node.ln_Name,"-");
    GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GTST_String,ame ? ame->am_ShortCut : NULL,GA_Disabled,dame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,14),win,NULL,GTST_String,same ? same->am_ShortCut : NULL,GA_Disabled,dsame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,15),win,NULL,GTST_String,!dame ? ame->am_AppCmd : NULL,GA_Disabled,dame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,16),win,NULL,GTST_String,!dsame ? same->am_AppCmd : NULL,GA_Disabled,dsame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,17),win,NULL,GA_Disabled,dame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,18),win,NULL,GA_Disabled,dsame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,19),win,NULL,GA_Disabled,dame,TAG_END);
    GT_SetGadgetAttrs(GadgetAddress(win,20),win,NULL,GA_Disabled,dsame,TAG_END);
}


void ASM handlePrefMenuIDCMP(REG(a0, struct TagItem *tag))
{
    struct IgnAppMenu *am;
    struct IgnAppMenuEntry *ame,*same;
    STRPTR t;
    long   i;

    am = ((struct Gadget *)wd->wd_ExtData[0])->UserData;
    ame = ((struct Gadget *)wd->wd_ExtData[1])->UserData;
    same = ((struct Gadget *)wd->wd_ExtData[2])->UserData;

    switch(imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            i = (gad = imsg.IAddress)->GadgetID;
            if (i == 17 && ame || i == 18 && same)
            {
                struct Mappe *mp = GetPrefsMap(GetLocalPrefs(wd->wd_Data));
                struct MinList *list;
                long   mode,selected;

                if (i == 18)
                    ame = same;

                GT_GetGadgetAttrs(GadgetAddress(win,i+2),win,NULL,GTCY_Active,&mode,TAG_END);
                list = mode ? &intcmds : (mp ? &mp->mp_AppCmds : &prefs.pr_AppCmds);

                selected = GetListNumberOfName(list,ame->am_AppCmd,mode,!mode && mp);

                selected = PopUpList(win,gad,list,mode ? TAG_IGNORE : POPA_ItemHeight, itemheight,
                                                                                    mode ? TAG_IGNORE : POPA_CallBack,   mp ? &linkHook : &renderHook,
                                                                                    mode ? TAG_IGNORE : POPA_MaxItems,   5,
                                                                                    POPA_Selected, selected,
                                                                                    POPA_Width,    ((struct Gadget *)wd->wd_ExtData[0])->Width-boxwidth,
                                                                                    POPA_Left,     TRUE,
                                                                                    TAG_END);
                if (selected != ~0L)
                {
                    struct Node *ln;
                    long   j;

                    for(j = 0,ln = (struct Node *)list->mlh_Head;j < selected;j++,ln = ln->ln_Succ);
                    if (!mode && mp)
                        ln = ((struct Link *)ln)->l_Link;

                    FreeString(ame->am_AppCmd);
                    ame->am_AppCmd = AllocString(ln->ln_Name);
                    GT_SetGadgetAttrs(GadgetAddress(win,i-2),win,NULL,GTST_String,ame->am_AppCmd,TAG_END);
                }
            }
            break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (i = gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch (i = (gad = imsg.IAddress)->GadgetID)
#endif
            {
                case 1:
                    for(am = (APTR)((struct List *)wd->wd_ExtData[3])->lh_Head,i = 0;i < imsg.Code;am = (APTR)am->am_Node.ln_Succ,i++);

                    if (gad->UserData != am)
                        updatePrefMenuGads(am,NULL,NULL);
                    ActivateGadget(GadgetAddress(win,11),win,NULL);
                    break;
                case 2:
                    if (am)
                    {
                        for(ame = (APTR)am->am_Items.mlh_Head,i = 0;i < imsg.Code;ame = (APTR)ame->am_Node.ln_Succ,i++);
                        if (gad->UserData != ame)
                            updatePrefMenuGads(am,ame,NULL);
                        ActivateGadget(GadgetAddress(win,11),win,NULL);
                    }
                    break;
                case 3:
                    if (am && ame)
                    {
                        for(same = (APTR)ame->am_Subs.mlh_Head,i = 0;i < imsg.Code;same = (APTR)same->am_Node.ln_Succ,i++);
                        if (gad->UserData != same)
                            updatePrefMenuGads(am,ame,same);
                        ActivateGadget(GadgetAddress(win,12),win,NULL);
                    }
                    break;
                case 4:
                case 5:
                case 6:
                    if (i == 4) am = NULL;
                    if (i == 5) ame = NULL;
                    if (i == 6) same = NULL;
                    updatePrefMenuGads(am,ame,same);
                    ActivateGadget(GadgetAddress(win,i+6),win,NULL);
                    break;
                case 7:
                    if (am)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,GTLV_Selected,~0L,TAG_END);
                        MyRemove(am);
                        FreeString(am->am_Node.ln_Name);
                        FreePooled(pool,am,sizeof(struct IgnAppMenu));
                        updatePrefMenuGads(NULL,NULL,NULL);
                    }
                    break;
                case 8:
                    if (ame)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,~0L,GTLV_Selected,~0L,TAG_END);
                        MyRemove(ame);
                        FreeString(ame->am_Node.ln_Name);
                        FreeString(ame->am_ShortCut);
                        FreeString(ame->am_AppCmd);
                        while((same = (struct IgnAppMenuEntry *)MyRemHead(&ame->am_Subs)) != NULL)
                        {
                            FreeString(same->am_Node.ln_Name);
                            FreeString(same->am_ShortCut);
                            FreeString(same->am_AppCmd);
                        }
                        FreePooled(pool,ame,sizeof(struct IgnAppMenuEntry));
                        updatePrefMenuGads(am,NULL,NULL);
                    }
                    break;
                case 9:
                    if (same)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,~0L,GTLV_Selected,~0L,TAG_END);
                        MyRemove(same);
                        FreeString(same->am_Node.ln_Name);
                        FreeString(same->am_ShortCut);
                        FreeString(same->am_AppCmd);
                        FreePooled(pool,same,sizeof(struct IgnAppMenuEntry));
                        updatePrefMenuGads(am,ame,NULL);
                    }
                    break;
                case 10:
                    GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END);
                    if (strlen(t))
                    {
                        if ((am = ((struct Gadget *)wd->wd_ExtData[0])->UserData) != 0)
                        {
                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                            FreeString(am->am_Node.ln_Name);
                            am->am_Node.ln_Name = AllocString(t);
                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[3],TAG_END);
                        }
                        else if ((am = AllocPooled(pool,sizeof(struct IgnAppMenu))) != 0)
                        {
                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                            am->am_Node.ln_Name = AllocString(t);
                            MyNewList(&am->am_Items);
                            MyAddTail(wd->wd_ExtData[3], am);
                            updatePrefMenuGads(am,NULL,NULL);
                            ActivateGadget(GadgetAddress(win,11),win,NULL);
                        }
                    }
                    else
                        ActivateGadget(imsg.IAddress,win,NULL);
                    break;
                case 11:
                {
                    struct IgnAppMenuEntry *iam;
                    UBYTE  separator;

                    iam = ame;
                case 12:
                    if (i == 12)
                        iam = same;
                    GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END);
                    if (strlen(t))
                    {
                        separator = (UBYTE)!strcmp(t,"-");
                        if (iam)
                        {
                            GT_SetGadgetAttrs(wd->wd_ExtData[i-10],win,NULL,GTLV_Labels,~0L,TAG_END);
                            FreeString(iam->am_Node.ln_Name);
                            iam->am_Node.ln_Name = AllocString(t);
                            updatePrefMenuGads(am,ame,same);
                        }
                        else if ((iam = AllocPooled(pool, sizeof(struct IgnAppMenuEntry))) != 0)
                        {
                            GT_SetGadgetAttrs(wd->wd_ExtData[i-10],win,NULL,GTLV_Labels,~0L,GA_Disabled,FALSE,GTLV_Selected,~0L,TAG_END);
                            iam->am_Node.ln_Name = AllocString(t);
                            MyNewList(&iam->am_Subs);
                            if (i == 11)                    // an Menü einhängen
                                MyAddTail(&am->am_Items, ame = iam);
                            else                            // am Submenü einhängen
                            {
                                MyAddTail(&ame->am_Subs, same = iam);
                                FreeString(ame->am_AppCmd);   // AppCmd löschen
                                ame->am_AppCmd = NULL;
                            }
                            updatePrefMenuGads(am,i == 11 && separator ? NULL : ame,i == 12 ? (separator ? NULL : same) : NULL);
                        }
                        if (i == 11 && ame || i == 12 && same)
                            ActivateGadget(separator ? gad : GadgetAddress(win,i+2),win,NULL);
                    }
                    break;
                }
                case 13:
                    same = ame;
                case 14:
                    if (same)
                    {
                        GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END);
                        FreeString(same->am_ShortCut);
                        same->am_ShortCut = AllocString(t);
                        ActivateGadget(GadgetAddress(win,i+2),win,NULL);
                    }
                    break;
                case 15:  // Item-Command
                    same = ame;
                case 16:
                    if (same)
                    {
                        GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END);
                        FreeString(same->am_AppCmd);
                        same->am_AppCmd = AllocString(t);
                        updatePrefMenuGads(am,same == ame ? NULL : ame,NULL);
                        ActivateGadget(GadgetAddress(win,i-4),win,NULL);
                    }
                    break;
                case 21:  // Ok
                {
                    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

                    if (!IsListEmpty((struct List *)wd->wd_ExtData[3]))
                        AddPrefsModuleToLocalPrefs(GetPrefsMap(pr),WDT_PREFMENU);
                    SetPrefsModule(pr,WDT_PREFMENU,TRUE);

                    swapLists(wd->wd_ExtData[3],&pr->pr_AppMenus);
                    CloseAppWindow(win,TRUE);
                    RefreshPrefsModule(pr,NULL,WDT_PREFMENU);
                    break;
                }
                case 22:  // Abbrechen
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
        case IDCMP_OBJECTDROP:
        {
            struct DropMessage *dm = imsg.IAddress;

            if (dm && !dm->dm_Object.od_Owner && dm->dm_Target && dm->dm_Object.od_Object)
            {
                if (dm->dm_Window == win)
                {
                    long target,source;

                    source = dm->dm_Gadget->GadgetID;  target = dm->dm_Target->GadgetID;

                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                    GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,~0L,TAG_END);
                    GT_SetGadgetAttrs(wd->wd_ExtData[2],win,NULL,GTLV_Labels,~0L,TAG_END);

                    if (source == 1 && target == 1)
                        MoveTo((struct Node *)dm->dm_Object.od_Object,wd->wd_ExtData[3],dm->dm_SourceEntry,wd->wd_ExtData[3],dm->dm_TargetEntry);
                    else if (am && source == 2 && target == 2)
                        MoveTo((struct Node *)dm->dm_Object.od_Object,&am->am_Items,dm->dm_SourceEntry,&am->am_Items,dm->dm_TargetEntry);
                    else if (ame && source == 3 && target == 3)
                        MoveTo((struct Node *)dm->dm_Object.od_Object,&ame->am_Subs,dm->dm_SourceEntry,&ame->am_Subs,dm->dm_TargetEntry);
                    else if (am && source == 3 && target == 2)
                    {
                        MyRemove(dm->dm_Object.od_Object);
                        InsertAt(&am->am_Items,dm->dm_Object.od_Object,dm->dm_TargetEntry);
                    }
                    else if (ame && source == 2 && target == 3)
                    {
                        MyRemove(dm->dm_Object.od_Object);
                        InsertAt(&ame->am_Subs,dm->dm_Object.od_Object,dm->dm_TargetEntry);
                    }
                    updatePrefMenuGads(am,ame,same);
                }
                else if (dm->dm_Object.od_InternalType == DRAGT_SUBMENU)
                {
                    struct IgnAppMenuEntry *iam,*dam = dm->dm_Object.od_Object;

                    if ((iam = AllocPooled(pool, sizeof(struct IgnAppMenuEntry))) != 0)
                    {
                        GT_SetGadgetAttrs(dm->dm_Target,win,NULL,GTLV_Labels,~0L,GA_Disabled,FALSE,GTLV_Selected,~0L,TAG_END);
                        iam->am_Node.ln_Name = AllocString(dam->am_Node.ln_Name);
                        iam->am_AppCmd = AllocString(dam->am_AppCmd);
                        MyNewList(&iam->am_Subs);

                        if (wd->wd_ExtData[1] == dm->dm_Target)  // einhängen
                            InsertAt(&am->am_Items,(struct Node *)(ame = iam),dm->dm_TargetEntry);
                        else
                            InsertAt(&ame->am_Subs,(struct Node *)(same = iam),dm->dm_TargetEntry);

                        updatePrefMenuGads(am,ame,same);         // aktivieren
                    }
                }
            }
            break;
        }
    }
}


void ASM
CloseFilePrefsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

	if (clean) {
        FreeString(wd->wd_ExtData[0]);
        FreeString(wd->wd_ExtData[1]);
        FreeString(wd->wd_ExtData[2]);
		FreePooled(pool, wd->wd_Data, sizeof(struct PrefFile));
    }
}


void ASM
HandleFilePrefsIDCMP(REG(a0, struct TagItem *tag))
{
    struct PrefFile *pf = wd->wd_Data;
    long   i;

	switch (imsg.Class) {
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
                case 1:   // Pfad-Eingabe
                case 2:
                case 3:
                    FreeString(wd->wd_ExtData[gad->GadgetID-1]);
                    GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&i,TAG_END);
                    wd->wd_ExtData[gad->GadgetID-1] = AllocString((STRPTR)i);
                    if (gad->GadgetID < 3)
                        ActivateGadget(GadgetAddress(win,gad->GadgetID+1),win,NULL);
                    break;
                case 4:   // Pfad-Requester
                case 5:
                case 6:
					if (AslRequestTags(fileReq,
							ASLFR_Window,        win,
							ASLFR_TitleText,     "Pfad festlegen",  /*i == 0 ? "" : "",*/
							ASLFR_InitialDrawer, wd->wd_ExtData[i = gad->GadgetID-4],
							ASLFR_InitialPattern,"#?",
							ASLFR_DoSaveMode,    FALSE,
							ASLFR_DoPatterns,    FALSE,
							ASLFR_DrawersOnly,   TRUE,
							TAG_END))
                    {
                        FreeString(wd->wd_ExtData[i]);
                        wd->wd_ExtData[i] = AllocString(fileReq->fr_Drawer);
                        GT_SetGadgetAttrs(GadgetAddress(win,i+1),win,NULL,GTST_String,fileReq->fr_Drawer,TAG_END);
                    }
                    break;
                case 7:   // Icons?
                    pf->pf_Flags = (pf->pf_Flags & ~PFF_ICONS) | (imsg.Code ? PFF_ICONS : 0);
                    break;
                case 8:   // Suffix?
                    pf->pf_Flags = (pf->pf_Flags & ~PFF_NOSUFFIX) | (imsg.Code ? PFF_NOSUFFIX : 0);
                    break;
                case 9:   // Backup?
                    pf->pf_Flags = (pf->pf_Flags & ~PFF_BACKUP) | (imsg.Code ? PFF_BACKUP : 0);
                    break;
				case 10:   // IOType warning?
					pf->pf_Flags = (pf->pf_Flags & ~PFF_WARN_IOTYPE) | (imsg.Code ? PFF_WARN_IOTYPE : 0);
                    break;
                case 11:  // AutoSave?
                    GT_SetGadgetAttrs(GadgetAddress(win,12),win,NULL,GA_Disabled,!imsg.Code,TAG_END);
                    pf->pf_AutoSave = imsg.Code;
                    break;
                case 12:  // Intervall
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &i, TAG_END);
                    if ((pf->pf_AutoSaveIntervall = ConvertTime((STRPTR)i)) == -1L)
						ErrorRequest(GetString(&gLocaleInfo, MSG_INVALID_TIME_ERR));
                    break;
                case 13:  // Ok
                    FreeString(projpath);  FreeString(graphicpath);  FreeString(iconpath);
                    GT_GetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTST_String,&projpath,TAG_END);
                    projpath = AllocString(projpath);
                    GT_GetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTST_String,&graphicpath,TAG_END);
                    graphicpath = AllocString(graphicpath);
                    GT_GetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTST_String,&iconpath,TAG_END);
                    iconpath = AllocString(iconpath);

					CopyMem(pf, prefs.pr_File, sizeof(struct PrefFile));
                    if (prefs.pr_File->pf_AutoSaveIntervall < 60)
                        prefs.pr_File->pf_AutoSaveIntervall = 60;
					CurrentTime(&lastsecs, (ULONG *)&i);  // Zähler zurücksetzen

					SetPrefsModule(&prefs, WDT_PREFFILE,TRUE);
					PropagatePrefsModule(&prefs, WDT_PREFFILE);

					// supposed to fall through
                case 14:  // Abbrechen
					CloseAppWindow(win, TRUE);
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;
    }
}

void ASM
CloseTablePrefsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

	if (!clean)
		return;

	if (wd->wd_ExtData[0])
		FreePooled(pool, wd->wd_ExtData[0], sizeof(struct PrefTable));

	FreeStringList(wd->wd_ExtData[3]);
}


void ASM
HandleTablePrefsIDCMP(REG(a0, struct TagItem *tag))
{
    struct PrefTable *pt = wd->wd_ExtData[0];
    struct Node *ln;
    long   i;

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if (((struct Gadget *)imsg.IAddress)->GadgetID == 9)
            {
                i = PopUpList(win,gad = GadgetAddress(win,8),wd->wd_ExtData[3],TAG_END);
                if (i != ~0L)
                {
                    pt->pt_EditFunc[(long)wd->wd_ExtData[4]] = (pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_TEXTONLY) | i;
                    for(ln = ((struct List *)wd->wd_ExtData[3])->lh_Head;i && ln->ln_Succ;ln = ln->ln_Succ,i--);
                    GT_SetGadgetAttrs(gad,win,NULL,GTTX_Text,ln->ln_Name,TAG_END);
                }
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
                case 7:  // Qualifier
                    if ((ln = FindListNumber(wd->wd_ExtData[3], pt->pt_EditFunc[imsg.Code] & PTEF_FUNCMASK)) != 0)
                        GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GTTX_Text,ln->ln_Name,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,11),win,NULL,GTCB_Checked,pt->pt_EditFunc[imsg.Code] & PTEF_TEXTONLY,TAG_END);
                    wd->wd_ExtData[4] = (APTR)((ULONG)imsg.Code);
                    break;
                case 10:  // Ziehecke darstellen
                    pt->pt_Flags = (pt->pt_Flags & ~PTF_EDITFUNC) | (imsg.Code ? PTF_EDITFUNC : 0);
                    break;
                case 11:  // Nur Texte
                    pt->pt_EditFunc[(long)wd->wd_ExtData[4]] = (pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_FUNCMASK) | (imsg.Code ? PTEF_TEXTONLY : 0);
                    break;
                case 12: // Nullwerte
                    pt->pt_Flags = (pt->pt_Flags & ~PTF_SHOWZEROS) | (imsg.Code ? PTF_SHOWZEROS : 0);
                    break;
                case 13: // Automatische Größenänderung
                    pt->pt_Flags = (pt->pt_Flags & ~PTF_AUTOCELLSIZE) | (imsg.Code ? PTF_AUTOCELLSIZE : 0);
                    break;
                case 14: // Formeln zeigen
                    pt->pt_Flags = (pt->pt_Flags & ~PTF_SHOWFORMULA) | (imsg.Code ? PTF_SHOWFORMULA : 0);
                    break;
                case 15: // Jahrhundert vervollständigen
                    pt->pt_Flags = (pt->pt_Flags & ~PTF_AUTOCENTURY) | (imsg.Code ? PTF_AUTOCENTURY : 0);
                    break;
                case 16: // Blockmarkierung
                    pt->pt_Flags = (pt->pt_Flags & ~(PTF_MARKSUM | PTF_MARKAVERAGE)) | (imsg.Code*PTF_MARKSUM);
                    break;
                case 17: // Ok
                {
                    BOOL century = FALSE;

                    if ((pt->pt_Flags & PTF_AUTOCENTURY) != (prefs.pr_Table->pt_Flags & PTF_AUTOCENTURY) && (pt->pt_Flags & PTF_AUTOCENTURY))
                        century = TRUE;

                    calcflags &= ~(CF_REQUESTER | CF_ZERONAMES);
                    calcflags |= GetCheckBoxFlag(GadgetAddress(win,1),win,CF_REQUESTER) | GetCheckBoxFlag(GadgetAddress(win,2),win,CF_ZERONAMES);

                    CopyMem(pt,prefs.pr_Table,sizeof(struct PrefTable));

                    CloseAppWindow(win,TRUE);
                    SetPrefsModule(&prefs,WDT_PREFTABLE,TRUE);
                    PropagatePrefsModule(&prefs,WDT_PREFTABLE);

                    if (century)
                        RefreshMapTexts(NULL,FALSE);
                    else
                        RecalcMap(NULL);
                    break;
                }
                case 18: // Abbrechen
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}

void ASM handlePressKeyIDCMP(REG(a0, struct TagItem *tag))
{
    struct AppKey *ak = wd->wd_Data;
    struct Window *swin;
    struct winData *swd;

    switch(imsg.Class)
    {
        case IDCMP_RAWKEY:
            if (imsg.Code > 95 && imsg.Code < 105)   /* Qualifier */
                break;
        case IDCMP_VANILLAKEY:
            /*bug("code: %ld - qualifier: %ld\n",imsg.Code,imsg.Qualifier);*/
            if ((swin = GetAppWindow(WDT_PREFKEYS)) != 0)
                GT_SetGadgetAttrs((swd = (struct winData *)swin->UserData)->wd_ExtData[0],swin,NULL,GTLV_Labels,~0L,TAG_END);
            ak->ak_Class = imsg.Class;
            ak->ak_Code = imsg.Code;
            ak->ak_Qualifier = imsg.Qualifier;
            SetAppKeyName(ak);
            if (swin)
            {
                sortList(swd->wd_ExtData[2]);
                GT_SetGadgetAttrs(swd->wd_ExtData[0],swin,NULL,GTLV_Labels,swd->wd_ExtData[2],GTLV_Selected,FindListEntry(swd->wd_ExtData[2],(struct MinNode *)ak),TAG_END);
            }
            else
                sortList(&prefs.pr_AppKeys);
        case IDCMP_GADGETUP:
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
CloseKeyboardPrefsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    wd->wd_ExtData[1] = NULL;
	GTD_RemoveWindow(win);

    if (clean)
    {
        FreeAppKeys(wd->wd_ExtData[2]);
		FreePooled(pool,wd->wd_ExtData[2], sizeof(struct List));
    }
}


void ASM
HandleKeyboardPrefsIDCMP(REG(a0, struct TagItem *tag))
{
	struct AppKey *ak = wd->wd_ExtData[1];
	long i, j;

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 5 && ak)
            {
                struct Mappe *mp = GetPrefsMap(GetLocalPrefs(wd->wd_Data));
                struct MinList *list;
                long   mode = 0;

                //GT_GetGadgetAttrs(GadgetAddress(win,i+2),win,NULL,GTCY_Active,&mode,TAG_END);
                list = mode ? &intcmds : (mp ? &mp->mp_AppCmds : &prefs.pr_AppCmds);

                i = PopUpList(win,gad,list,mode ? TAG_IGNORE : POPA_ItemHeight, itemheight,
                                                                     mode ? TAG_IGNORE : POPA_CallBack,   mp ? &linkHook : &renderHook,
                                                                     mode ? TAG_IGNORE : POPA_MaxItems,   5,
                                                                     POPA_Selected, GetListNumberOfName(list,ak->ak_AppCmd,mode,!mode && mp),
                                                                     POPA_Width,    gad->LeftEdge-8,
                                                                     POPA_Left,     TRUE,
                                                                     TAG_END);
                if (i != ~0L)
                {
                    struct Node *ln;

                    for(j = 0,ln = (struct Node *)list->mlh_Head;j < i;j++,ln = ln->ln_Succ);
                    if (!mode && mp)
                        ln = ((struct Link *)ln)->l_Link;

                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                    FreeString(ak->ak_AppCmd);
                    ak->ak_AppCmd = AllocString(ln->ln_Name);
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],TAG_END);
                }
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
                case 1:  // Key-Liste
                    for(i = 0,ak = (APTR)((struct List *)wd->wd_ExtData[2])->lh_Head;i < imsg.Code && ak->ak_Node.ln_Succ;ak = (APTR)ak->ak_Node.ln_Succ,i++);
                    wd->wd_ExtData[1] = ak;
                    for(j = 2;j <= 4;j++)
                        GT_SetGadgetAttrs(GadgetAddress(win,j),win,NULL,GA_Disabled,FALSE,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTCY_Active,ak->ak_Node.ln_Type,GA_Disabled,FALSE,TAG_END);
                    break;
                case 2:  // Neu
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                    if ((ak = AllocPooled(pool, sizeof(struct AppKey))) != 0)
                    {
                        ak->ak_Node.ln_Name = AllocString("## n.def.");
                        wd->wd_ExtData[1] = ak;
                        MyAddTail(wd->wd_ExtData[2], ak);
                        sortList(wd->wd_ExtData[2]);
                        i = FindListEntry(wd->wd_ExtData[2],(struct MinNode *)ak);
                    }
                    else
                    {
                        wd->wd_ExtData[1] = NULL;
                        i = ~0L;
                    }
                    for(j = 2;j <= 4;j++)
                        GT_SetGadgetAttrs(GadgetAddress(win,j),win,NULL,GA_Disabled,!ak,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTCY_Active,ak ? ak->ak_Node.ln_Type : 0,GA_Disabled,!ak,TAG_END);
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],GTLV_Selected,i,TAG_END);
                    break;
                case 3:  // Delete
                    if (ak)
                    {
                        MyRemove(ak);
                        FreeString(ak->ak_Node.ln_Name);
                        FreeString(ak->ak_AppCmd);
                        FreePooled(pool,ak,sizeof(struct AppKey));
                        wd->wd_ExtData[1] = NULL;
                        for(j = 2;j <= 4;j++)
                            GT_SetGadgetAttrs(GadgetAddress(win,j),win,NULL,GA_Disabled,TRUE,TAG_END);
                        GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTCY_Active,0,GA_Disabled,TRUE,TAG_END);
						GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, wd->wd_ExtData[2], GTLV_Selected, ~0L, TAG_END);
                    }
                    break;
                case 4:  // Aufnehmen
                    if (ak)
                        OpenAppWindow(WDT_PRESSKEY,WA_Data,ak,TAG_END);
                    break;
                case 6:  // Key-Modus
                    if (ak)
                        ak->ak_Node.ln_Type = imsg.Code;
                    break;
                case 7:   // Ok
                {
                    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

                    if (!IsListEmpty((struct List *)wd->wd_ExtData[2]))
                        AddPrefsModuleToLocalPrefs(GetPrefsMap(pr),WDT_PREFKEYS);
                    SetPrefsModule(pr,WDT_PREFKEYS,TRUE);

                    FreeAppKeys(&pr->pr_AppKeys);
                    moveList(wd->wd_ExtData[2],&pr->pr_AppKeys);

                    /* RefreshPrefsModule() bislang nicht notwendig */
                }
                case 8:   // Abbrechen
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;

        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;

        case IDCMP_NEWSIZE:
			StandardNewSize(CreateKeyboardPrefsGadgets);
            break;

        case IDCMP_OBJECTDROP:
        {
            struct DropMessage *dm = imsg.IAddress;

			if (ak && dm && !dm->dm_Object.od_Owner)
            {
				GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, ~0L, TAG_END);

				if (dm->dm_Object.od_InternalType & DRAGT_APPCMD)
                {
					FreeString(ak->ak_AppCmd);
					ak->ak_AppCmd = AllocString(((struct Node *)dm->dm_Object.od_Object)->ln_Name);
				} else
					DisplayBeep(NULL);

				GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, wd->wd_ExtData[2], GTLV_Selected, FindListEntry(wd->wd_ExtData[2], (struct MinNode *)ak), TAG_END);
            }
            break;
        }
    }
}


void ASM
closePrefIconWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;
    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

    ((struct Gadget *)wd->wd_ExtData[1])->UserData = NULL;
    GTD_RemoveGadgets(win);

    if (clean)
    {
        if (wd->wd_ExtData[2])
        {
            FreeIconObjs(&pr->pr_IconObjs);
            moveList(wd->wd_ExtData[2],&pr->pr_IconObjs);
            FreePooled(pool,wd->wd_ExtData[2],sizeof(struct MinList));
        }
        FreePooled(pool,wd->wd_ExtData[3],sizeof(struct ImageNode));

        if (wd->wd_ExtData[4])
        {
            struct AppCmd *ac;

            while((ac = (APTR)MyRemHead(wd->wd_ExtData[4])) != 0)
                FreeAppCmd(ac);
            FreePooled(pool,wd->wd_ExtData[4],sizeof(struct MinList));
        }
    }
    RemLockNode(wd->wd_ExtData[5]);
    RemLockNode(wd->wd_ExtData[7]);
}


void PUBLIC
PrefIconAppCmdLock(REG(a0, struct LockNode *ln),REG(a1, struct MinNode *node),REG(d0, UBYTE flags))
{
	switch (flags & LNCMDS)
    {
        case LNCMD_LOCK:
            GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,~0L,TAG_END);
            break;
        case LNCMD_FREE:
            break;
        default:
        {
            struct winData *wd = ln->ln_Data[2];

            MakePrefIconAppCmds(GetLocalPrefs(wd->wd_Data),wd->wd_ExtData[4]);
            GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,wd->wd_ExtData[4],TAG_END);
            break;
        }
    }
}


void ASM
handlePrefIconIDCMP(REG(a0, struct TagItem *tag))
{
    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
    struct IconObj *io;
    static BYTE tested;

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
                case 2:
                {
                    long i;

                    for(io = (APTR)pr->pr_IconObjs.mlh_Head,i = 0;i < imsg.Code;io = (APTR)io->io_Node.in_Succ,i++);
                    ((struct Gadget *)wd->wd_ExtData[1])->UserData = io;
                    break;
                }
                case 3:   // Leerfeld
                    GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,~0L,TAG_END);
                    AddIconObj(&pr->pr_IconObjs,wd->wd_ExtData[3],-1);
                    GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,&pr->pr_IconObjs,GTLV_Selected,~0L,TAG_END);
                    break;
                case 4:   // Löschen
                    if ((io = ((struct Gadget *)wd->wd_ExtData[1])->UserData) != 0)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,~0L,TAG_END);
                        if (io->io_Node.in_Succ != (APTR)&pr->pr_IconObjs.mlh_Tail)
                            ((struct Gadget *)wd->wd_ExtData[1])->UserData = io->io_Node.in_Succ;
                        else
                            ((struct Gadget *)wd->wd_ExtData[1])->UserData = io->io_Node.in_Pred;
                        MyRemove(io);
                        if (IsListEmpty((struct List *)&pr->pr_IconObjs))
                            ((struct Gadget *)wd->wd_ExtData[1])->UserData = NULL;
                        FreeString(io->io_AppCmd);
                        FreePooled(pool,io,sizeof(struct IconObj));
                        GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,&pr->pr_IconObjs,TAG_END);
                    }
                    break;
                case 5:    // Ok
                {
                    struct Mappe *mp = wd->wd_Data;

                    if (!IsListEmpty((struct List *)wd->wd_ExtData[2]))
                        AddPrefsModuleToLocalPrefs(mp,WDT_PREFICON);
                    SetPrefsModule(pr,WDT_PREFICON,TRUE);

                    FreeIconObjs(wd->wd_ExtData[2]);
                    FreePooled(pool,wd->wd_ExtData[2],sizeof(struct MinList));
                    wd->wd_ExtData[2] = NULL;
                    CloseAppWindow(win,TRUE);
                    RefreshPrefsModule(pr,NULL,WDT_PREFICON);
                    break;
                }
                case 6:    // Test
                    RefreshProjWindows(TRUE);
                    tested = TRUE;
                    break;
                case 7:    // Abbrechen
                    CloseAppWindow(win,TRUE);
                    if (tested)
                    {
                        RefreshProjWindows(TRUE);
                        tested = FALSE;
                    }
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            if (tested)
            {
                RefreshProjWindows(TRUE);
                tested = FALSE;
            }
            break;
        case IDCMP_OBJECTDROP:
        {
            struct DropMessage *dm = imsg.IAddress;

            if (dm && !dm->dm_Object.od_Owner)
            {
                long source,target;

                source = dm->dm_Gadget->GadgetID;  target = dm->dm_Target->GadgetID;

                GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,~0L,TAG_END);
                if (dm->dm_Object.od_InternalType & (DRAGT_APPCMD | DRAGT_ICONSEPARATOR) && target == 2)
                {
                    if (dm->dm_Object.od_InternalType != DRAGT_ICONSEPARATOR && !((struct ImageNode *)dm->dm_Object.od_Object)->in_Image)
                        DisplayBeep(NULL);
                    else
                        AddIconObj(&pr->pr_IconObjs,(struct AppCmd *)dm->dm_Object.od_Object,dm->dm_TargetEntry);
                }
                if (dm->dm_Window == win)
                {
                    if (source == 2 && (target == 4 || target == 1))
                    {
                        MyRemove(dm->dm_Object.od_Object);
                        FreeString(((struct IconObj *)dm->dm_Object.od_Object)->io_AppCmd);
                        FreePooled(pool,dm->dm_Object.od_Object,sizeof(struct IconObj));
                    }
                    if (source == 2 && target == 2)
                        MoveTo((struct Node *)dm->dm_Object.od_Object,&pr->pr_IconObjs,dm->dm_SourceEntry,&pr->pr_IconObjs,dm->dm_TargetEntry);
                }
                GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,&pr->pr_IconObjs,GTLV_Selected,~0L,TAG_END);
                ((struct Gadget *)wd->wd_ExtData[1])->UserData = NULL;
            }
            break;
        }
    }
}


void ASM
closePrefCmdsWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    GTD_RemoveGadget(wd->wd_ExtData[0]);

    RemLockNode(wd->wd_ExtData[5]);
}


void
RemoveAppCmdsIconObjs(struct Prefs *pr,struct AppCmd *ac)
{
    struct IconObj *sio;
    int    count = 0;

    /* Entfernen der IconObjs müßte eigentlich prefsweit geschehen */

    foreach(&pr->pr_IconObjs,sio)
    {
        if (!strcmp(sio->io_AppCmd,ac->ac_Node.in_Name))
        {
            struct IconObj *io = sio;

            count++;  sio = (APTR)sio->io_Node.in_Pred;
            RemoveFromLockedList(&pr->pr_IconObjs,(struct MinNode *)io);

            FreeString(io->io_AppCmd);
            FreePooled(pool,io,sizeof(struct IconObj));
        }
    }
    if (count)
        RefreshProjWindows(TRUE);
}


void ASM
handlePrefCmdsIDCMP(REG(a0, struct TagItem *tag))
{
    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
    struct Node *n,*sn;
    long   i,id;

    switch(imsg.Class)
    {
        case IDCMP_RAWKEY:
            HandleLVRawKeys(wd->wd_ExtData[0],win,&pr->pr_AppCmds,itemheight);
            break;
        case IDCMP_GADGETUP:
            id = ((struct Gadget *)imsg.IAddress)->GadgetID;
            if (id == 1)
            {
				for (n = (struct Node *)pr->pr_AppCmds.mlh_Head,i = 0;i<imsg.Code;n = n->ln_Succ,i++);
                ((struct Gadget *)imsg.IAddress)->UserData = n;
                if (IsDoubleClick(imsg.Code) && !((struct AppCmd *)n)->ac_Locked)
					OpenAppWindow(WDT_DEFINECMD, WA_Data, n, WA_Map, wd->wd_Data, TAG_END);
            }
		case IDCMP_VANILLAKEY:
            if (imsg.Class == IDCMP_VANILLAKEY)
                id = imsg.Code;
            if (id == wd->wd_ShortCuts[0])
				OpenAppWindow(WDT_DEFINECMD, WA_Map, wd->wd_Data, TAG_END);
            else if (id == wd->wd_ShortCuts[1])   // Copy
            {
                if (((struct Gadget *)wd->wd_ExtData[0])->UserData)
                {
                    if ((n = (struct Node *)NewAppCmd(((struct Gadget *)wd->wd_ExtData[0])->UserData)) != 0)
                    {
                        MakeUniqueName(&pr->pr_AppCmds,&n->ln_Name);
                        if (!n->ln_Name)
                            n->ln_Name = AllocString(GetString(&gLocaleInfo, MSG_UNNAMED_IN_PARENTHESIS));
                        AddLockedTail(&pr->pr_AppCmds,(struct MinNode *)n);

                        for(i = 0,sn = (struct Node *)pr->pr_AppCmds.mlh_Head;sn != n;sn = sn->ln_Succ,i++);
                        ((struct Gadget *)wd->wd_ExtData[0])->UserData = n;
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Selected,i,TAG_END);
                    }
                    else
                        ((struct Gadget *)wd->wd_ExtData[0])->UserData = NULL;
                }
            }
            else if (id == wd->wd_ShortCuts[2])   // Delete
            {
                if ((n = ((struct Gadget *)wd->wd_ExtData[0])->UserData) && !((struct AppCmd *)n)->ac_Locked)
                {
                    RemoveFromLockedList(&pr->pr_AppCmds,(struct MinNode *)n);
                    RemoveAppCmdsIconObjs(pr,(struct AppCmd *)n);

                    if (n->ln_Succ == (APTR)&pr->pr_AppCmds.mlh_Tail)
                        sn = n->ln_Pred;
                    else
                        sn = n->ln_Succ;
                    FreeAppCmd((struct AppCmd *)n);

                    if (!IsListEmpty((struct List *)&pr->pr_AppCmds))
                    {
                        for(i = 0,n = (struct Node *)pr->pr_AppCmds.mlh_Head;sn != n;n = n->ln_Succ,i++);
                        ((struct Gadget *)wd->wd_ExtData[0])->UserData = sn;
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Selected,i,TAG_END);
                    }
                    else
                        ((struct Gadget *)wd->wd_ExtData[0])->UserData = NULL;
                }
                else if (n)
                    DisplayBeep(NULL);
            }
            else if (id == wd->wd_ShortCuts[3] || id == ESCAPE_KEY)
                CloseAppWindow(win,TRUE);
            else if (id == 13)
            {
                n = ((struct Gadget *)wd->wd_ExtData[0])->UserData;
                if (!((struct AppCmd *)n)->ac_Locked)
					OpenAppWindow(WDT_DEFINECMD, WA_Data, ((struct Gadget *)wd->wd_ExtData[0])->UserData, WA_Map, wd->wd_Data, TAG_END);
            }
            break;
        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;
    }
}


BOOL
checkDefinedCmd(STRPTR t)
{
    struct IntCmd *ic;
    struct CSource cs;
    struct RDArgs *rda;
    STRPTR t2;
    long   opts[MAX_OPTS];

    if (!t)
		return FALSE;

	if (!(ic = (struct IntCmd *)FindCommand(&intcmds, t)))
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_INTERNAL_COMMAND_NOT_FOUND_ERR));
		return FALSE;
    }
	if ((t2 = AllocPooled(pool, strlen(t) + 2)) != 0)
    {
		strcpy(t2, t);
		strcat(t2, "\n");

		cs.CS_Buffer = t2 + cmdlen(ic->ic_Node.ln_Name) + 1;
        cs.CS_Length = strlen(cs.CS_Buffer);
        cs.CS_CurChr = 0;
		memset((char *)opts, 0, sizeof(opts));

		if ((rda = AllocDosObject(DOS_RDARGS, TAG_END)) != 0)
        {
            rda->RDA_Source = cs;
            rda->RDA_Flags |= RDAF_NOPROMPT;

			if (ReadArgs(ic->ic_Node.ln_Name + cmdlen(ic->ic_Node.ln_Name) + 1, opts, rda))
                FreeArgs(rda);
            else
				ErrorRequest(GetString(&gLocaleInfo, MSG_BAD_COMMAND_ARGUMENTS_ERR), t2);

			FreeDosObject(DOS_RDARGS, rda);
        }
    }
	return TRUE;
}


void ASM
closeDefineCmdWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    if (wd->wd_Data)
        ((struct AppCmd *)wd->wd_Data)->ac_Locked--;

    if (clean)
		FreeAppCmd(wd->u.definecmd.wd_AppCmd);
}


void ASM
handleDefineCmdIDCMP(REG(a0, struct TagItem *tag))
{
	struct AppCmd *ac = wd->u.definecmd.wd_AppCmd;
    struct Command *cmd;
    struct IntCmd *ic;
    struct IconObj *io;
    struct Gadget *gad;
    char   *t;
    long   i;
    UWORD  id;

	switch (imsg.Class)
    {
        case IDCMP_RAWKEY:
            if (imsg.Code == CURSORUP || imsg.Code == CURSORDOWN)
            {
				if ((cmd = (struct Command *)HandleLVRawKeys(wd->u.definecmd.wd_ListView, win, &ac->ac_Cmds, fontheight)) != 0)
                {
                    GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[3]),win,NULL,GA_Disabled,FALSE,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,cmd->cmd_Name,GA_Disabled,FALSE,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,FALSE,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,9),win,NULL,GTMX_Active,cmd->cmd_Type,GA_Disabled,FALSE,TAG_END);
                }
            }
            break;
        case IDCMP_GADGETDOWN:
            gad = (struct Gadget *)imsg.IAddress;
            switch(gad->GadgetID)
            {
                case 8:
                    GT_GetGadgetAttrs(GadgetAddress(win,9),win,NULL,GTMX_Active,&i,TAG_END);
                    if (i == CMDT_INTERN)
                    {
						i = PopUpList(win, GadgetAddress(win, 7), &intcmds, POPA_MaxItems, 12, TAG_END);
                        if (i != ~0L)
                        {
							gad = wd->u.definecmd.wd_ListView;
							if (!(cmd = gad->UserData) && (cmd = AllocPooled(pool, sizeof(struct Command))))
                            {
                                gad->UserData = cmd;
								MyAddTail(&ac->ac_Cmds, cmd);
                            }
                            if (cmd)
                            {
                                t = NULL;
                                if ((ic = (struct IntCmd *)FindListNumber(&intcmds, i)) != 0)
                                    t = AllocString(ic->ic_Node.ln_Name);
                                FreeString(cmd->cmd_Name);
                                cmd->cmd_Name = t;
                                GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&ac->ac_Cmds,GTLV_Selected,FindListEntry(&ac->ac_Cmds,(struct MinNode *)cmd),TAG_END);
                                GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,t,TAG_END);
                            }
                        }
                    }
                    break;
                case 9:
					if ((cmd = wd->u.definecmd.wd_ListView->UserData) != NULL)
                        cmd->cmd_Type = imsg.Code;
                    break;
                case 11:  // Output-Popper
					i = PopUpList(win, GadgetAddress(win, 10), &outputs, POPA_MaxItems, 12, TAG_END);
                    if (i != ~0L)
                    {
                        t = NULL;
                        if ((ic = (APTR)FindListNumber(&outputs, i)) != 0)
                            t = ic->ic_Node.ln_Name;
                        FreeString(ac->ac_Output);
                        ac->ac_Output = AllocString(t);
                        GT_SetGadgetAttrs(GadgetAddress(win,10),win,NULL,GTST_String,t,TAG_END);
                    }
                    break;
            }
            break;
        case IDCMP_GADGETUP:
            id = ((struct Gadget *)imsg.IAddress)->GadgetID;
            switch(id)
            {
                case 4:     // Listview
                    for(cmd = (APTR)ac->ac_Cmds.mlh_Head,i = 0;i < imsg.Code;cmd = cmd->cmd_Succ,i++);
					gad = wd->u.definecmd.wd_ListView;
                    gad->UserData = cmd;
                    GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[3]),win,NULL,GA_Disabled,FALSE,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,cmd->cmd_Name,GA_Disabled,FALSE,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,FALSE,TAG_END);
                    GT_SetGadgetAttrs(GadgetAddress(win,9),win,NULL,GTMX_Active,cmd->cmd_Type,GA_Disabled,FALSE,TAG_END);
                    ActivateGadget(GadgetAddress(win,7),win,NULL);
                    break;
                case 7:     // Command
					gad = wd->u.definecmd.wd_ListView;
                    GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,~0L,TAG_END);
                    if ((cmd = gad->UserData) != 0)
                    {
                        FreeString(cmd->cmd_Name);
                        GT_GetGadgetAttrs(imsg.IAddress,win,NULL,GTST_String,&t,TAG_END);
                        cmd->cmd_Name = AllocString(t);
                        GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&ac->ac_Cmds,TAG_END);
                    }
                    else if ((cmd = AllocPooled(pool, sizeof(struct Command))) != 0)
                    {
                        gad->UserData = cmd;
                        MyAddTail(&ac->ac_Cmds, cmd);
                        GT_GetGadgetAttrs(imsg.IAddress,win,NULL,GTST_String,&t,TAG_END);
                        GT_GetGadgetAttrs(GadgetAddress(win,9),win,NULL,GTMX_Active,&i,TAG_END);
                        cmd->cmd_Name = AllocString(t);
                        cmd->cmd_Type = i;
                        GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&ac->ac_Cmds,GTLV_Selected,CountNodes(&ac->ac_Cmds),TAG_END);
                    }
                    if (cmd && cmd->cmd_Type == CMDT_INTERN && !checkDefinedCmd(cmd->cmd_Name))
                    {
                        ActivateGadget(imsg.IAddress,win,NULL);
                    }
                    GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[3]),win,NULL,GA_Disabled,FALSE,TAG_END);
                    break;
                case 8:    // Command-Popper
                    GT_GetGadgetAttrs(GadgetAddress(win,9),win,NULL,GTMX_Active,&i,TAG_END);
                    if (i != CMDT_INTERN)
                    {
                        char txt[256];

						gad = wd->u.definecmd.wd_ListView;
                        if ((cmd = gad->UserData) && cmd->cmd_Name)
                        {
                            strcpy(txt,cmd->cmd_Name);
                            *(PathPart(txt)) = 0;
                        }
                        else
                            txt[0] = 0;

                        if (AslRequestTags(fileReq,ASLFR_Window,        win,
                                                                             ASLFR_TitleText,     i == CMDT_AREXX ? GetString(&gLocaleInfo, MSG_LOAD_AREXX_SCRIPT_TITLE) : GetString(&gLocaleInfo, MSG_LOAD_COMMAND_TITLE),
                                                                             ASLFR_InitialDrawer, txt,
                                                                             ASLFR_InitialPattern,"#?",
                                                                             ASLFR_DoSaveMode,    FALSE,
                                                                             ASLFR_DoPatterns,    FALSE,
                                                                             ASLFR_DrawersOnly,   FALSE,
                                                                             TAG_END))
                        {
                            strcpy(txt,fileReq->fr_Drawer);
                            AddPart(txt,fileReq->fr_File,255);
                            if (!cmd && (cmd = AllocPooled(pool,sizeof(struct Command))))
                            {
                                gad->UserData = cmd;
                                cmd->cmd_Type = i;
                                MyAddTail(&ac->ac_Cmds, cmd);
                            }
                            FreeString(cmd->cmd_Name);
                            cmd->cmd_Name = AllocString(txt);
                            GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&ac->ac_Cmds,GTLV_Selected,FindListEntry(&ac->ac_Cmds,(struct MinNode *)cmd),TAG_END);
                            GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,txt,TAG_END);
                        }
                    }
                    break;
                case 10:   // Output
                    GT_GetGadgetAttrs(imsg.IAddress,win,NULL,GTST_String,&t,TAG_END);
                    if (t && strlen(t) && !MyFindName(&outputs, t))
                    {
                        if ((ic = AllocPooled(pool, sizeof(struct Node))) != 0)
                        {
                            ic->ic_Node.ln_Name = AllocString(t);
                            MyAddTail(&outputs, ic);
                        }
                    }
                    ActivateGadget(GadgetAddress(win,12),win,NULL);
                    break;
            }
        case IDCMP_VANILLAKEY:
            if (imsg.Class == IDCMP_VANILLAKEY)
                id = imsg.Code;
            if (id == wd->wd_ShortCuts[0])
                ActivateGadget(GadgetAddress(win,1),win,NULL);
            else if (id == wd->wd_ShortCuts[1])     // Load Image
            {
                char   txt[256];

                if (ac->ac_ImageName)
                {
                    strcpy(txt,ac->ac_ImageName);
                    *(PathPart(txt)) = 0;
                }
                else
                    strcpy(txt,iconpath);
                if (AslRequestTags(fileReq,ASLFR_Window,        win,
                                                                     ASLFR_TitleText,     GetString(&gLocaleInfo, MSG_LOAD_ICON_TITLE),
                                                                     ASLFR_InitialDrawer, txt,
#ifdef __amigaos4__
																	 ASLFR_InitialPattern,"#?",
																	 ASLFR_AcceptPattern, "#?.icon",
#endif
                                                                     ASLFR_DoSaveMode,    FALSE,
                                                                     ASLFR_DoPatterns,    TRUE,
                                                                     ASLFR_DrawersOnly,   FALSE,
                                                                     TAG_END))
                {
                    strcpy(txt,fileReq->fr_Drawer);
                    AddPart(txt,fileReq->fr_File,255);
                    ac->ac_ImageName = AllocString(txt);
                    GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTTX_Text,ac->ac_ImageName,TAG_END);
                    i = TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL))+26;
                    EraseRect(win->RPort,i,barheight+16+fontheight,i+boxwidth*2-5,barheight+fontheight*3+21);
                    makeClip(win,i,barheight+16+fontheight,boxwidth*2-5,fontheight*2+5);
					if ((ac->ac_Node.in_Image = LoadImage(ac->ac_ImageName)) != 0) {
						SetAttrs(ac->ac_Node.in_Image, /*(DTA_Dummy + 253)PDTA_TransRemapPen, 0,*/PDTA_Screen, scr, TAG_END);
							// this actually renders the image into the bitmap for the current screen
                        DrawImage(win->RPort,ac->ac_Node.in_Image,i-2-(ac->ac_Node.in_Image->Width >> 1)+boxwidth,barheight+19+2*fontheight-(ac->ac_Node.in_Image->Height >> 1));
					}
                    freeClip(win);
                }
            }
            else if (id == wd->wd_ShortCuts[2])     // New Command
            {
                GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,NULL,GA_Disabled,FALSE,TAG_END);
                GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,FALSE,TAG_END);
                GT_SetGadgetAttrs(GadgetAddress(win,9),win,NULL,GTMX_Active,0,GA_Disabled,FALSE,TAG_END);
				GT_SetGadgetAttrs(wd->u.definecmd.wd_ListView, win, NULL, GTLV_Selected, ~0L, TAG_END);
                GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[3]),win,NULL,GA_Disabled,TRUE,TAG_END);
                ActivateGadget(GadgetAddress(win,7),win,NULL);
				gad = wd->u.definecmd.wd_ListView;
                gad->UserData = NULL;
            }
            else if (id == wd->wd_ShortCuts[3])     // Delete Command
            {
                GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,NULL,GA_Disabled,TRUE,TAG_END);
                GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,TRUE,TAG_END);
                GT_SetGadgetAttrs(GadgetAddress(win,9),win,NULL,GTMX_Active,0,GA_Disabled,TRUE,TAG_END);
				gad = wd->u.definecmd.wd_ListView;
				GT_SetGadgetAttrs(gad, win, NULL, GTLV_Labels, ~0L, TAG_END);
                if ((cmd = gad->UserData) != 0)
                {
                    MyRemove(cmd);
                    FreeString(cmd->cmd_Name);
					FreePooled(pool, cmd, sizeof(struct Command));
                    gad->UserData = NULL;
                }
                GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&ac->ac_Cmds,GTLV_Selected,~0L,TAG_END);
                GT_SetGadgetAttrs(imsg.IAddress,win,NULL,GA_Disabled,TRUE,TAG_END);
            }
            else if (id == wd->wd_ShortCuts[i = 4] || id == wd->wd_ShortCuts[i = 5] || id == wd->wd_ShortCuts[i = 6])     // Command Types
            {
                i -= 4;
				if ((gad = wd->u.definecmd.wd_ListView) && (cmd = gad->UserData) != NULL)
                {
                    cmd->cmd_Type = i;
					GT_SetGadgetAttrs(GadgetAddress(win, 9), win, NULL, GTMX_Active, i, TAG_END);
                }
            }
            else if (id == wd->wd_ShortCuts[7])
                ActivateGadget(GadgetAddress(win,10),win,NULL);
            else if (id == wd->wd_ShortCuts[8])   // Hilfetext
                ActivateGadget(GadgetAddress(win,12),win,NULL);
            else if (id == wd->wd_ShortCuts[11])  // Hypertext
                ActivateGadget(GadgetAddress(win,13),win,NULL);
            else if (id == wd->wd_ShortCuts[9])   // Ok
            {
				struct Prefs *pr = GetLocalPrefs(wd->u.definecmd.wd_Map);
                struct Mappe *mp = GetPrefsMap(pr);

				AddPrefsModuleToLocalPrefs(mp, WDT_PREFCMDS);
				SetPrefsModule(pr, WDT_PREFCMDS, TRUE);

				GT_GetGadgetAttrs(GadgetAddress(win, 1), win, NULL, GTST_String, &t, TAG_END);
                cmd = (struct Command *)MyFindName(&pr->pr_AppCmds, t);
                if (!strlen(t) || cmd && !wd->wd_Data || cmd && wd->wd_Data != cmd)
                {
                    DisplayBeep(NULL);
					ActivateGadget(GadgetAddress(win, 1), win, NULL);
                    break;
                }
                if (wd->wd_Data)
                {
                    MyRemove(wd->wd_Data);
                    FreeAppCmd(wd->wd_Data);
                    wd->wd_Data = NULL;
                }
				GT_GetGadgetAttrs(GadgetAddress(win, 1), win, NULL, GTST_String, &t, TAG_END);
                ac->ac_Node.in_Name = AllocString(t);
				GT_GetGadgetAttrs(GadgetAddress(win, 10), win, NULL, GTST_String, &t, TAG_END);
                ac->ac_Output = AllocString(t);
				GT_GetGadgetAttrs(GadgetAddress(win, 12), win, NULL, GTST_String, &t, TAG_END);
                ac->ac_HelpText = AllocString(t);
				GT_GetGadgetAttrs(GadgetAddress(win, 13), win, NULL, GTST_String, &t, TAG_END);
                ac->ac_Guide = AllocString(t);
				for (id = 0, io = (APTR)pr->pr_IconObjs.mlh_Head; io->io_Node.in_Succ; io = (APTR)io->io_Node.in_Succ)
                {
//if(io->io_AppCmd == NULL || ac->ac_Node.in_Name == NULL)
//	continue;
                    if (!strcmp(io->io_AppCmd,ac->ac_Node.in_Name))
                    {
                        id++;
                        io->io_Node.in_Image = ac->ac_Node.in_Image;
                    }
                }
				wd->u.definecmd.wd_AppCmd = NULL;
				CloseAppWindow(win, TRUE);

				AddLockedTail(&pr->pr_AppCmds, (struct MinNode *)ac);
                if (id)
                    RefreshProjWindows(TRUE);
            }
            else if (id == wd->wd_ShortCuts[10] || id == ESCAPE_KEY)
				CloseAppWindow(win, TRUE);
            else if (imsg.Class == IDCMP_VANILLAKEY && id == 13 && (cmd = (gad = wd->wd_ExtData[1])->UserData))
				ActivateGadget(GadgetAddress(win, 7), win, NULL);
            break;
        case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
            break;
    }
}


void ASM
closePrefNamesWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;
    struct Name *nm;

    if (clean)
    {
        while ((nm = (struct Name *)MyRemHead(wd->wd_ExtData[2])) != 0)
            FreeName(nm);
        FreePooled(pool,wd->wd_ExtData[2],sizeof(struct List));
    }
}


void
UpdateNamesGadgets(struct Name *nm, BOOL activate)
{
    ((struct Gadget *)wd->wd_ExtData[0])->UserData = nm;

    if (nm)
    {
        GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[1]), win, NULL, GA_Disabled, FALSE, TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[2]), win, NULL, GA_Disabled, FALSE, TAG_END);
        GT_SetGadgetAttrs(gad = GadgetAddress(win,5),win,NULL,GTST_String,nm->nm_Node.ln_Name,GA_Disabled,FALSE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,nm->nm_Content,GA_Disabled,FALSE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTCY_Active, (long)(nm->nm_Node.ln_Type & NMT_TYPEMASK), GA_Disabled, FALSE, TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GTTX_Text,nm->nm_Page ? nm->nm_Page->pg_Node.ln_Name : "-",GA_Disabled,wd->wd_Data ? FALSE : TRUE,TAG_END);
#ifdef __amigaos4__
        //SetAttrs((struct Object *)GadgetAddress(win,9),GA_Disabled, FALSE,TAG_END);
        SetGadgetAttrs(GadgetAddress(win,9), win, NULL, GA_Disabled, FALSE,TAG_END);
        RefreshGList(GadgetAddress(win,9), win, NULL, 1);
#endif
        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],GTLV_Selected,FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm),TAG_END);
        if (activate)
            ActivateGadget(gad, win, NULL);
    }
    else
    {
        struct Mappe *mp = GetRealMap(wd->wd_Data);

        GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[1]),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,wd->wd_ShortCuts[2]),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTST_String,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTCY_Active,0,GA_Disabled,TRUE,TAG_END);
        if (mp)
            GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GTTX_Text,((struct Node *)mp->mp_Pages.mlh_Head)->ln_Name,GA_Disabled,TRUE,TAG_END);
#ifdef __amigaos4__
        SetGadgetAttrs(GadgetAddress(win,9), win, NULL, GA_Disabled, TRUE,TAG_END);
        RefreshGList(GadgetAddress(win,9), win, NULL, 1);
#endif
        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],GTLV_Selected,~0L,TAG_END);
    }
}


void ASM
handlePrefNamesIDCMP(REG(a0, struct TagItem *tag))
{
    struct Mappe *mp = GetRealMap(wd->wd_Data);
    struct Name *nm = ((struct Gadget *)wd->wd_ExtData[0])->UserData;
    long   i,id;

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 9 && mp)
            {
                i = PopUpList(win,gad = GadgetAddress(win,8),&mp->mp_Pages,TAG_END);
                if (i != ~0L)
                {
                    struct Page *page;

                    for(page = (struct Page *)mp->mp_Pages.mlh_Head;i && page->pg_Node.ln_Succ;page = (APTR)page->pg_Node.ln_Succ,i--);
                    nm->nm_Page = page;
                    GT_SetGadgetAttrs(gad,win,NULL,GTTX_Text,nm->nm_Page->pg_Node.ln_Name,TAG_END);
                }
            }
            break;
        case IDCMP_RAWKEY:
            nm = (APTR)HandleLVRawKeys(wd->wd_ExtData[0],win,wd->wd_ExtData[2],fontheight);
            UpdateNamesGadgets(nm,FALSE);
            break;
        case IDCMP_VANILLAKEY:
            id = imsg.Code;
            if (id == wd->wd_ShortCuts[i = 3] || id == wd->wd_ShortCuts[i = 4])
            {
				if (nm)
					ActivateGadget(GadgetAddress(win,i + 2), win, NULL);
                break;
            }
			else if (ConvToLower(loc, id) == wd->wd_ShortCuts[5] && (gad = GadgetAddress(win, 7)) && !(gad->Flags & GFLG_DISABLED))
            {
                if (GT_GetGadgetAttrs(gad,win,NULL,GTCY_Active,&i,TAG_END))
                {
                    if (imsg.Qualifier & IEQUALIFIER_SHIFT)
                    {
                        i--;
                        if (i < 0)
                            i = 2;
                    }
                    else
						i = (i + 1) % 3;

					GT_SetGadgetAttrs(gad, win, NULL, GTCY_Active, i, TAG_END);
                }
            }
#ifdef __amigaos4__
			else if (ConvToLower(loc, id) == wd->wd_ShortCuts[5] && mp && (gad = GadgetAddress(win, 8)) && !(gad->Flags & GFLG_DISABLED))
#else
			else if (ConvToLower(loc, id) == wd->wd_ShortCuts[6] && mp && (gad = GadgetAddress(win, 8)) && !(gad->Flags & GFLG_DISABLED))
#endif
            {
                struct Page *page = nm->nm_Page;

                if (imsg.Qualifier & IEQUALIFIER_SHIFT)
                {
                    if (page->pg_Node.ln_Pred->ln_Pred)
                        page = (struct Page *)page->pg_Node.ln_Pred;
                    else
                        page = (struct Page *)mp->mp_Pages.mlh_TailPred;
                }
                else
                {
                    if (page->pg_Node.ln_Succ->ln_Succ)
                        page = (struct Page *)page->pg_Node.ln_Succ;
                    else
                        page = (struct Page *)mp->mp_Pages.mlh_Head;
                }
				GT_SetGadgetAttrs(gad, win, NULL, GTTX_Text, (nm->nm_Page = page)->pg_Node.ln_Name, TAG_END);
            }
        case IDCMP_GADGETUP:
            if (imsg.Class == IDCMP_GADGETUP)
            {
                id = (gad = imsg.IAddress)->GadgetID;
				switch (id)
                {
                    case 1:  // Namen-Liste
						if ((WORD)imsg.Code < 0)
							break;
                        for(i = 0,nm = (APTR)((struct List *)wd->wd_ExtData[2])->lh_Head;i < imsg.Code;nm = (APTR)nm->nm_Node.ln_Succ,i++);
                        UpdateNamesGadgets(nm,TRUE);
                        break;
                    case 5:  // Name
                        if (nm)
                        {
#ifdef __amigaos4__
							FreeString(nm->nm_Node.ln_Name);
#endif
                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                            GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&nm->nm_Node.ln_Name,TAG_END);
#ifdef __amigaos4__
                            if(!Strlen(nm->nm_Node.ln_Name))
                            	nm->nm_Node.ln_Name = "#";
#endif
                            nm->nm_Node.ln_Name = AllocString(nm->nm_Node.ln_Name);
                            sortList(wd->wd_ExtData[2]);
                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],GTLV_Selected,FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm),TAG_END);
							if (IsValidName(wd->wd_ExtData[2], nm->nm_Node.ln_Name))
                                i = 6;
                            else
                                i = 5;
							ActivateGadget(GadgetAddress(win, i), win, NULL);
                        }
                        break;
                    case 6:  // Inhalt
                        if (nm)
                        {
#ifdef __amigaos4__
                            STRPTR t;
                            char tmp[500];

                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                            GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END);
							if(!isdigit(t[0]))//text musst be in ""
							{							    
							    sprintf(tmp, "\"%s\"", t);
							    t = tmp;
							}
							
                            SetNameContent(nm, AllocString(t));
#else                            
                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                            GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&i,TAG_END);

                            SetNameContent(nm, AllocString((STRPTR)i));
#endif
                            GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],GTLV_Selected,FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm),TAG_END);
                        }
                        break;
                    case 7:  // Typ
                        if (nm)
                        {
                            nm->nm_Node.ln_Type = (UBYTE)imsg.Code;
                            RefreshName(nm);
                        }
                        break;
                }
            }
            if (id == wd->wd_ShortCuts[0])             // Neu
            {
                GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                if ((nm = AddName(wd->wd_ExtData[2], GetString(&gLocaleInfo, MSG_NEW_NAME), NULL,NMT_NONE | NMT_DETACHED,mp ? (struct Page *)mp->mp_Pages.mlh_Head : NULL)) != 0)
                    UpdateNamesGadgets(nm, TRUE);
                else
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],TAG_END);
            }
            else if (id == wd->wd_ShortCuts[1] && nm)  // Kopieren
            {
                GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                if (nm && (nm = AddName(wd->wd_ExtData[2],nm->nm_Node.ln_Name,nm->nm_Content,nm->nm_Node.ln_Type,nm->nm_Page)))
                    UpdateNamesGadgets(nm,TRUE);
                else
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],TAG_END);
            }
            else if (id == wd->wd_ShortCuts[2] && nm)  // Löschen
            {
                GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                MyRemove(nm);
                FreeName(nm);
                UpdateNamesGadgets(NULL,FALSE);
            }
#ifdef __amigaos4__
            else if (id == wd->wd_ShortCuts[6])  // Ok
#else
            else if (id == wd->wd_ShortCuts[7])  // Ok
#endif
            {
                struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

                if (!IsListEmpty((struct List *)wd->wd_ExtData[2]))
                    AddPrefsModuleToLocalPrefs(mp,WDT_PREFNAMES);
                SetPrefsModule(pr,WDT_PREFNAMES,TRUE);

                if (LockList(&pr->pr_Names,LNF_REFRESH))
                {
                    DetachNameList(&pr->pr_Names);
                    swapLists(wd->wd_ExtData[2],&pr->pr_Names);
                    AttachNameList(&pr->pr_Names);

                    UnlockList(&pr->pr_Names,LNF_REFRESH);
                }
                CloseAppWindow(win,TRUE);

                RefreshPrefsModule(pr,NULL,WDT_PREFNAMES);
            }
#ifdef __amigaos4__
            else if (id == wd->wd_ShortCuts[7] || id == ESCAPE_KEY)  // Abbrechen
#else
            else if (id == wd->wd_ShortCuts[8] || id == ESCAPE_KEY)  // Abbrechen
#endif
                CloseAppWindow(win,TRUE);
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void ASM
CloseFormatPrefsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

	if (!clean)
		return;

	FreeStringList(wd->wd_ExtData[2]);  // fvt_zeit
	FreeStringList(wd->wd_ExtData[3]);	// fvt_datum

	if (wd->wd_ExtData[5]) {
		struct Node *ln;
		while ((ln = MyRemHead(wd->wd_ExtData[5])) != 0)
			FreeFormat((struct FormatVorlage *)ln);

		FreePooled(pool, wd->wd_ExtData[5], sizeof(struct List));
    }
}


void
UpdatePrefFormatGadgets(struct FormatVorlage *fv)
{
    wd->wd_ExtData[4] = fv;

    if (fv)
    {
        /*struct Gadget *cycgad;
        long   active;*/

        GT_SetGadgetAttrs(gad = GadgetAddress(win,5),win,NULL,GA_Disabled,FALSE,GTCY_Active,fv->fv_Node.ln_Type-1,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GA_Disabled,FALSE,GTSL_Level,fv->fv_Node.ln_Pri,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,FALSE,GTST_String,fv->fv_Node.ln_Name,TAG_END);
        GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GA_Disabled,fv->fv_Node.ln_Type == FVT_DATE,GTSL_Level,fv->fv_Komma,TAG_END);
        DrawPointSliderValue(win->RPort,wd->wd_ExtData[1],fv->fv_Komma);
        GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GA_Disabled,FALSE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GA_Disabled,FALSE,TAG_END);
        /*if ((cycgad = GadgetAddress(win,10)) && GT_GetGadgetAttrs(cycgad,win,NULL,GTCY_Active,&active,TAG_END) && active != fv->fv_Alignment)
            GT_SetGadgetAttrs(cycgad,win,NULL,GA_Disabled,FALSE,GTCY_Active,fv->fv_Alignment-1,TAG_END);*/
        GT_SetGadgetAttrs(GadgetAddress(win,10),win,NULL,GA_Disabled,FALSE,GTCY_Active,fv->fv_Alignment-1,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,11),win,NULL,GA_Disabled,FALSE,GTCB_Checked,fv->fv_Flags & FVF_NEGATIVEPEN,TAG_END);
        DrawColorField(win->RPort,GadgetAddress(win,12),fv->fv_NegativePen,TRUE);
        DisableGadget(GadgetAddress(win,8),win,FALSE);
        DisableGadget(GadgetAddress(win,12),win,FALSE);
        GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GA_Disabled,fv->fv_Node.ln_Type == FVT_DATE,GTCB_Checked,fv->fv_Flags & FVF_NEGPARENTHESES,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,14),win,NULL,GA_Disabled,fv->fv_Node.ln_Type == FVT_DATE || fv->fv_Node.ln_Type == FVT_TIME,GTCB_Checked,fv->fv_Flags & FVF_SEPARATE,TAG_END);
        ActivateGadget(gad,win,NULL);
    }
    else
    {
        int i;

        GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,NULL,TAG_END);
        for(i = 3;i < 15;i++)
        {
            DisableGadget(GadgetAddress(win,i),win,TRUE);
            /*if (i == 6)
                i = 10;*/
        }

        /*GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,10),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,11),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,14),win,NULL,GA_Disabled,TRUE,TAG_END);*/
        GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GA_Disabled,TRUE,TAG_END);
        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[5],GTLV_Selected,~0L,TAG_END);
    }
}


void ASM
HandleFormatPrefsIDCMP(REG(a0, struct TagItem *tag))
{
    struct FormatVorlage *fv = wd->wd_ExtData[4];
    double value = 0.0;
    STRPTR t,s;
    long   i;
#if defined  __amigaos4__  || defined __MORPHOS__
	static BOOL flag = FALSE;
#endif

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 8 && fv && (fv->fv_Node.ln_Type == FVT_TIME || fv->fv_Node.ln_Type == FVT_DATE))
            {
                i = PopUpList(win,gad = GadgetAddress(win,7),wd->wd_ExtData[2+(long)(fv->fv_Node.ln_Type == FVT_DATE)],TAG_END);
                if (i != ~0L)
                {
                    for(fv = (APTR)((struct List *)wd->wd_ExtData[2+(long)(fv->fv_Node.ln_Type == FVT_DATE)])->lh_Head;i-- && fv->fv_Node.ln_Succ;fv = (APTR)fv->fv_Node.ln_Succ);
                    GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&s,TAG_END);
                    if ((t = AllocPooled(pool, 3 + (s ? strlen(s) : 0))) != 0)
                    {
                        if (s) strcpy(t,s);
                        strncat(t,fv->fv_Node.ln_Name,2);
                    }
                    GT_SetGadgetAttrs(gad,win,NULL,GTST_String,t,TAG_END);
                    ActivateGadget(gad,win,NULL);
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);

                    fv = wd->wd_ExtData[4];
                    FreeString(fv->fv_Node.ln_Name);  fv->fv_Node.ln_Name = AllocString(t);
                    goto makeFormatPreview;
                }
            }
            else if (fv && gad->GadgetID == 12)   /* NegativePen-PopUp */
            {
                i = PopUpList(win,gad,&colors,POPA_CallBack,&colorHook,POPA_MaxPen,~0L,POPA_ItemHeight,fontheight+4,POPA_Width,GetListWidth(&colors)+boxwidth+5,TAG_END);
                if (i != ~0L)
                {
                    struct colorPen *cp;

                    for(cp = (APTR)colors.mlh_Head;i-- && cp->cp_Node.ln_Succ;cp = (APTR)cp->cp_Node.ln_Succ);

                    //wd->wd_ExtData[7] = (APTR)i;
                    fv->fv_Flags |= FVF_NEGATIVEPEN;
                    fv->fv_NegativePen = cp->cp_ID;
                    DrawColorField(win->RPort,gad,cp->cp_ID,TRUE);
                    GT_SetGadgetAttrs(GadgetAddress(win,11),win,NULL,GTCB_Checked,TRUE,TAG_END);
                }
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
                case 1:  // Liste
                    for(i = 0,fv = (APTR)((struct List *)wd->wd_ExtData[5])->lh_Head;i < imsg.Code;i++,fv = (APTR)fv->fv_Node.ln_Succ);
                    UpdatePrefFormatGadgets(fv);
                    break;
                case 2:  // Neu
#ifdef __amigaos4__
					if(flag)
						goto setFormatData;
#endif
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                    fv = AddFormat(wd->wd_ExtData[5],NULL,0,-1,-1,-1,ITA_NONE,FVT_VALUE);
                    GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[5],GTLV_MakeVisible,i = CountNodes(wd->wd_ExtData[5]),GTLV_Selected,i,TAG_END);
                    UpdatePrefFormatGadgets(fv);
					flag = TRUE;
                    break;
                case 3:  // Kopieren
#ifdef __amigaos4__
					if(flag)
						goto setFormatData;
#endif
                    if (fv)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                        if ((wd->wd_ExtData[4] = (fv = CopyFormat(fv))) != 0)
                            MyAddTail(wd->wd_ExtData[5], fv);
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[5],GTLV_MakeVisible,i = CountNodes(wd->wd_ExtData[5]),GTLV_Selected,i,TAG_END);
                    }
                    break;
                case 4:  // Löschen
                    if (fv)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                        MyRemove(fv);
                        FreeFormat(fv);
                        UpdatePrefFormatGadgets(NULL);
                    }
                    break;
                case 5:  // Typ-Cycle
                    if (fv)
                    {
                        fv->fv_Node.ln_Type = imsg.Code+1;
                        GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GA_Disabled,fv->fv_Node.ln_Type == FVT_DATE,TAG_END);
                        ActivateGadget(GadgetAddress(win,6),win,NULL);
                    }
                    break;
                case 6:  // Priorität
                    if (fv)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                        fv->fv_Node.ln_Pri = imsg.Code;
                        SortFormatList(wd->wd_ExtData[5]);
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[5],GTLV_Selected,i = FindListEntry(wd->wd_ExtData[5],(struct MinNode *)fv),GTLV_MakeVisible,i,TAG_END);
                    }
                    break;
                case 7:  // Format
#ifdef __amigaos4__
				setFormatData:
					flag = FALSE;
#endif
                    if (fv)
                    {
                        GT_GetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTCY_Active,&i,TAG_END);
                        fv->fv_Node.ln_Type = i+1;
                        GT_GetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,&s,TAG_END);
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                        FreeString(fv->fv_Node.ln_Name);
                        fv->fv_Node.ln_Name = AllocString(s);
                makeFormatPreview:  /************************************** makeFormatPreview ****************************/
                        FreeString(fv->fv_Preview);
						switch (fv->fv_Node.ln_Type)
                        {
                            case FVT_DATE:
								value = FORMAT_DATE_PREVIEW;
                                break;
                            case FVT_TIME:
								value = FORMAT_TIME_PREVIEW;
                             break;
                        }
                        fv->fv_Preview = AllocString(FitValueInFormat(value,(struct Node *)fv,NULL,0,0));
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[5],TAG_END);
                    }
                    break;
                case 9:  // Nachkommastellen
                    if (fv)
                    {
                        GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
                        DrawPointSliderValue(win->RPort,wd->wd_ExtData[1],imsg.Code);
                        fv->fv_Komma = imsg.Code;
                        goto makeFormatPreview;
                    }
                    break;
                case 10:  // Ausrichtung
                    if (fv)
                        fv->fv_Alignment = imsg.Code+1;
                    break;
                case 11:  // NegativePen-Checkbox
                    if (fv)
                        fv->fv_Flags = (fv->fv_Flags & ~FVF_NEGATIVEPEN) | (imsg.Code ? FVF_NEGATIVEPEN : 0);
                    break;
                case 13:  // Neg. Klammern
                    if (fv)
                        fv->fv_Flags = (fv->fv_Flags & ~FVF_NEGPARENTHESES) | (imsg.Code ? FVF_NEGPARENTHESES : 0);
                    break;
                case 14:  // Tausender trennen
                    if (fv)
                        fv->fv_Flags = (fv->fv_Flags & ~FVF_SEPARATE) | (imsg.Code ? FVF_SEPARATE : 0);
                    break;
                case 15:  // Ok
                {
                    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
                    struct Mappe *mp = wd->wd_Data;

#ifdef __amigaos4__
					if(flag)
						goto setFormatData;
#endif
                    if (!IsListEmpty((struct List *)wd->wd_ExtData[5]))
                        AddPrefsModuleToLocalPrefs(mp,WDT_PREFFORMAT);
                    SetPrefsModule(pr,WDT_PREFFORMAT,TRUE);

                    SortFormatList(wd->wd_ExtData[5]);

                    if (LockList(&pr->pr_Formats,LNF_REFRESH))
                    {
                        while ((fv = (APTR)MyRemHead(&pr->pr_Formats)) != 0)
                            FreeFormat(fv);
                        moveList(wd->wd_ExtData[5],&pr->pr_Formats);

                        UnlockList(&pr->pr_Formats,LNF_REFRESH);
                    }
                    CloseAppWindow(win,TRUE);

                    RefreshPrefsModule(pr,NULL,WDT_PREFFORMAT);
                    break;
                }
                case 16:
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;
        case IDCMP_MOUSEMOVE:
            if ((struct Gadget *)imsg.IAddress == wd->wd_ExtData[1])
                DrawPointSliderValue(win->RPort,wd->wd_ExtData[1],imsg.Code);
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}


void
setGradientColors(struct colorPen *cp, BOOL findname)
{
    struct ColorWheelRGB rgb;
    struct ColorWheelHSB hsb;
	short  *pens = (short *)wd->wd_ExtData[6], numPens = (short)((long)wd->wd_ExtData[7]);
    long   i = 0;

	GetAttr(WHEEL_HSB, wd->wd_ExtData[1], (IPTR *)&hsb);
	while (i < numPens)
    {
		hsb.cw_Brightness = 0xffffffff - ((0xffffffff / numPens) * i);
        ConvertHSBToRGB(&hsb,&rgb);
		SetRGB32(&scr->ViewPort, pens[i], rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue);
        i++;
    }
	GT_GetGadgetAttrs(GadgetAddress(win, 12), win, NULL, GTMX_Active, &i, TAG_END);
	GetAttr(i ? WHEEL_HSB : WHEEL_RGB, wd->wd_ExtData[1], (IPTR *)&rgb);
	GT_SetGadgetAttrs(wd->wd_ExtData[2], win, NULL, GTSL_Level, rgb.cw_Red >> 24, TAG_END);
	GT_SetGadgetAttrs(wd->wd_ExtData[3], win, NULL, GTSL_Level, rgb.cw_Green >> 24, TAG_END);
	GT_SetGadgetAttrs(wd->wd_ExtData[4], win, NULL, GTSL_Level, rgb.cw_Blue >> 24, TAG_END);

	// ToDo: nur auf True-Color Screens nötig
	RefreshGList(wd->wd_ExtData[0], win, NULL, 1);

    if (cp)
    {
#ifdef __amigaos4__
		ULONG no;
#endif
		// update pen

		GetAttr(WHEEL_RGB, wd->wd_ExtData[1], (IPTR *)&rgb);
        cp->cp_Red = rgb.cw_Red >> 24;
        cp->cp_Green = rgb.cw_Green >> 24;
        cp->cp_Blue = rgb.cw_Blue >> 24;
        cp->cp_ID = (cp->cp_Red << 16) | (cp->cp_Green << 8) | cp->cp_Blue;
        if (cp->cp_Pen != -1)
			SetRGB32(&scr->ViewPort, cp->cp_Pen, rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue);

		// update name & view

		GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTLV_Labels, ~0L, TAG_END);
		if (findname && cp->cp_Node.ln_Type) {
			FindColorName(cp);
			GT_SetGadgetAttrs(GadgetAddress(win, 6), win, NULL, GTST_String, cp->cp_Node.ln_Name, TAG_END);
		}
#ifdef __amigaos4__
		GT_GetGadgetAttrs(GadgetAddress(win, 1), win, NULL, GTCY_Active, &no, TAG_END); //Farbpalette bestimmen
		GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTLV_Labels, no ? wd->wd_ExtData[5] : wd->wd_Data, TAG_END);
#else
		GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTLV_Labels, wd->wd_Data, TAG_END);
#endif
    }
}

extern char *gColorModeLabels[];

void
showPrefColorsMode(struct Window *win, long mode)
{
    struct ColorWheelRGB rgb;
    struct winData *wd = (APTR)win->UserData;
	long   x, y, i = 0;

    if (mode == -1)
		GT_GetGadgetAttrs(GadgetAddress(win, 12), win, NULL, GTMX_Active, &mode, TAG_END);

	if ((gad = GadgetAddress(win, 7)) != 0)
    {
		x = gad->LeftEdge - 10 - TLn("999");
		y = gad->TopEdge + 1;
		EraseRect(win->RPort, 16, y, x, y + 3 * fontheight + 10);
        itext.FrontPen = 1;

		while (i < 3) {
			itext.IText = gColorModeLabels[mode * 3 + i];
			PrintIText(win->RPort, &itext, x - IntuiTextLength(&itext), y);
            y += fontheight+4;  i++;
        }
    }
	GetAttr(mode ? WHEEL_HSB : WHEEL_RGB, wd->wd_ExtData[1], (IPTR *)&rgb);

	GT_SetGadgetAttrs(wd->wd_ExtData[2], win, NULL, GTSL_Level, rgb.cw_Red >> 24, TAG_END);
	GT_SetGadgetAttrs(wd->wd_ExtData[3], win, NULL, GTSL_Level, rgb.cw_Green >> 24, TAG_END);
	GT_SetGadgetAttrs(wd->wd_ExtData[4], win, NULL, GTSL_Level, rgb.cw_Blue >> 24, TAG_END);
}


void
remapColorPen(struct colorPen *cp)
{
	ULONG r, g, b;
#ifdef __amigaos4__
	ULONG no;
#endif

    if (!cp || cp->cp_Pen != ((short *)wd->wd_ExtData[6])[MAXGRADPENS+1])
        return;

	GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTLV_Labels, ~0L, TAG_END);
	r = cp->cp_Red;
	g = cp->cp_Green;
	b = cp->cp_Blue;
    r = r | (r << 8) | (r << 16) | (r << 24);
    g = g | (g << 8) | (g << 16) | (g << 24);
    b = b | (b << 8) | (b << 16) | (b << 24);
	cp->cp_Pen = ObtainBestPen(scr->ViewPort.ColorMap, r, g, b, TAG_END);
#ifdef __amigaos4__
	GT_GetGadgetAttrs(GadgetAddress(win, 1), win, NULL, GTCY_Active, &no, TAG_END); //Farbpalette bestimmen
	GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTLV_Labels, no ? wd->wd_ExtData[5] : wd->wd_Data, TAG_END);
#else
	GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTLV_Labels, wd->wd_Data, TAG_END);
#endif
}


void ASM
closePrefColorsWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;
	short  numPens = (short)((long)wd->wd_ExtData[7]), *pens = (short *)wd->wd_ExtData[6];
    struct colorPen *cp;

	ReleasePen(scr->ViewPort.ColorMap, pens[MAXGRADPENS + 1]);
	while (numPens--)
		ReleasePen(scr->ViewPort.ColorMap, pens[numPens]);

    if (clean)
    {
		FreePooled(pool, wd->wd_ExtData[6], sizeof(short) * (MAXGRADPENS + 2));
        while((cp = (struct colorPen *)MyRemHead(wd->wd_Data)) != 0)
        {
            FreeString(cp->cp_Node.ln_Name);
			FreePooled(pool, cp, sizeof(struct colorPen));
        }
		FreePooled(pool, wd->wd_Data, sizeof(struct List));
    }
}


void ASM
handlePrefColorsIDCMP(REG(a0, struct TagItem *tag))
{
    struct colorPen *cp = NULL;
    long   i, col;

    if ((gad = GadgetAddress(win, 2)) != 0)
    {
        cp = (struct colorPen *)gad->UserData;
	}
	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 12)
				showPrefColorsMode(win, imsg.Code);
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
				case 1:		// cycle: document palette / screen palette
					for (i = (scr->UserData != (APTR)iport ? 2 : 3); i < 7; i++)
						GT_SetGadgetAttrs(GadgetAddress(win, i), win, NULL, GA_Disabled, imsg.Code, TAG_END);
					if ((gad = GadgetAddress(win, 2)) != 0)
                    {
                        remapColorPen(cp);
						GT_SetGadgetAttrs(gad, win, NULL, GTLV_Labels, imsg.Code ? wd->wd_ExtData[5] : wd->wd_Data, GTLV_Selected, ~0L, TAG_END);
                        gad->UserData = NULL;
                    }
                    break;
				case 2:		// color listview
					GT_GetGadgetAttrs(gad, win, NULL, GTLV_Labels, &col, TAG_END);
                    remapColorPen(cp);
					for (i = 0, cp = (APTR)((struct List *)col)->lh_Head; i < imsg.Code && cp->cp_Node.ln_Succ;cp = (APTR)cp->cp_Node.ln_Succ,i++);
                    gad->UserData = cp;
                    if ((struct List *)col == wd->wd_Data)
                    {
                        if (cp->cp_Pen != -1 && cp->cp_Pen != ((short *)wd->wd_ExtData[6])[MAXGRADPENS+1])
							ReleasePen(scr->ViewPort.ColorMap, cp->cp_Pen);
                        cp->cp_Pen = ((short *)wd->wd_ExtData[6])[MAXGRADPENS+1];
						GT_SetGadgetAttrs(GadgetAddress(win, 6), win, NULL, GTST_String, cp->cp_Node.ln_Name, TAG_END);
                    }
#ifdef __amigaos4__
					SetGadgetAttrs(wd->wd_ExtData[1], win, NULL,
						WHEEL_Red,   (cp->cp_Red)  | (cp->cp_Red << 8)   | (cp->cp_Red << 16)    | (cp->cp_Red << 24),
						WHEEL_Green, (cp->cp_Green)| (cp->cp_Green << 8) | (cp->cp_Green << 16) | (cp->cp_Green << 24),
						WHEEL_Blue,  (cp->cp_Blue) | (cp->cp_Blue << 8)  | (cp->cp_Blue << 16)   | (cp->cp_Blue << 24),
						TAG_END);
#else
					SetGadgetAttrs(wd->wd_ExtData[1], win, NULL,
						WHEEL_Red,   (col = cp->cp_Red)   | (col << 8) | (col << 16) | (col << 24),
						WHEEL_Green, (col = cp->cp_Green) | (col << 8) | (col << 16) | (col << 24),
						WHEEL_Blue,  (col = cp->cp_Blue)  | (col << 8) | (col << 16) | (col << 24),
						TAG_END);
#endif
					setGradientColors(cp, FALSE);
                    break;
				case 3:		// new color
				case 4:		// copy color
                    remapColorPen(cp);
                    GT_SetGadgetAttrs(gad = GadgetAddress(win,2),win,NULL,GTLV_Labels,~0L,TAG_END);
                    if (!cp || ((struct Gadget *)imsg.IAddress)->GadgetID == 3)
						gad->UserData = cp = AddColor(wd->wd_Data, NULL, 255, 255, 255);
                    else
                    {
						gad->UserData = cp = AddColor(wd->wd_Data, cp->cp_Node.ln_Name, cp->cp_Red, cp->cp_Green, cp->cp_Blue);
                        cp->cp_Node.ln_Type = 1;
                    }
                    cp->cp_Pen = ((short *)wd->wd_ExtData[6])[MAXGRADPENS+1];
                    GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,wd->wd_Data,GTLV_Selected,i = CountNodes(wd->wd_Data),GTLV_MakeVisible,i,TAG_END);
                    GT_SetGadgetAttrs(gad = GadgetAddress(win,6),win,NULL,GTST_String,cp->cp_Node.ln_Name,TAG_END);
#ifdef __amigaos4__
					SetGadgetAttrs(wd->wd_ExtData[1], win, NULL,
						WHEEL_Red,   (cp->cp_Red)  | (cp->cp_Red << 8)   | (cp->cp_Red << 16)    | (cp->cp_Red << 24),
						WHEEL_Green, (cp->cp_Green)| (cp->cp_Green << 8) | (cp->cp_Green << 16) | (cp->cp_Green << 24),
						WHEEL_Blue,  (cp->cp_Blue) | (cp->cp_Blue << 8)  | (cp->cp_Blue << 16)   | (cp->cp_Blue << 24),
						TAG_END);
#else
					SetGadgetAttrs(wd->wd_ExtData[1], win, NULL,
						WHEEL_Red,   (col = cp->cp_Red)   | (col << 8) | (col << 16) | (col << 24),
						WHEEL_Green, (col = cp->cp_Green) | (col << 8) | (col << 16) | (col << 24),
						WHEEL_Blue,  (col = cp->cp_Blue)  | (col << 8) | (col << 16) | (col << 24),
						TAG_END);
#endif
					setGradientColors(cp, TRUE);
					ActivateGadget(gad, win, NULL);
                    break;
				case 5:		// remove color
                    if (cp)
                    {
                        GT_SetGadgetAttrs(gad = GadgetAddress(win,2),win,NULL,GTLV_Labels,~0L,TAG_END);
                        gad->UserData = NULL;
                        MyRemove(cp);
                        FreeString(cp->cp_Node.ln_Name);
						FreePooled(pool, cp, sizeof(struct colorPen));
						GT_SetGadgetAttrs(gad, win, NULL, GTLV_Labels, wd->wd_Data, GTLV_Selected, ~0L, TAG_END);
                    }
                    break;
				case 6:		// set color name
					GT_SetGadgetAttrs(GadgetAddress(win, 2), win, NULL, GTLV_Labels, ~0L, TAG_END);

                    FreeString(cp->cp_Node.ln_Name);
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &cp->cp_Node.ln_Name, TAG_END);
                    cp->cp_Node.ln_Name = AllocString(cp->cp_Node.ln_Name);
					cp->cp_Node.ln_Type = (cp->cp_Node.ln_Name && cp->cp_Node.ln_Name[0]) ? 0 : 1;
						// determines if a color name should be automatically chosen
					if (cp->cp_Node.ln_Type == 1)
						FindColorName(cp);

                    GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTLV_Labels,wd->wd_Data,TAG_END);
                    break;
				case 7:		// RGB / HSB
                case 8:
                case 9:
                    goto slider_action;
                    break;
                case 10:    // Ok
					SetPrefsModule(&prefs, WDT_PREFCOLORS, TRUE);
                    for(i = 0,cp = (APTR)((struct List *)wd->wd_ExtData[5])->lh_Head;cp->cp_Node.ln_Succ;cp = (APTR)cp->cp_Node.ln_Succ,i++)
                    {
                        col = cp->cp_Red;
                        standardPalette[i * 3 + 1] = col | (col << 8) | (col << 16) | (col << 24);
                        col = cp->cp_Green;
                        standardPalette[i * 3 + 2] = col | (col << 8) | (col << 16) | (col << 24);
                        col = cp->cp_Blue;
                        standardPalette[i * 3 + 3] = col | (col << 8) | (col << 16) | (col << 24);
                    }
                    swapLists(&colors,wd->wd_Data);
                    UniqueColors(&colors);
#ifdef __amigaos4__
                    RefreshProjWindows(TRUE); //Übernahme der neuen Farben in den Fenstern
#endif
                case 11:    // Abbrechen
                    if ((gad = GadgetAddress(win, 2)) != 0)
                        remapColorPen(gad->UserData);
					CloseAppWindow(win, TRUE);
                    if ((APTR)scr->UserData == iport)
                        LoadRGB32(&scr->ViewPort,standardPalette);
					ReleaseAppColors(scr);
					ObtainAppColors(scr, TRUE);
                    RefreshProjWindows(FALSE);
                    break;
            }
            break;
        case IDCMP_MOUSEMOVE:
        slider_action:
            if ((gad = imsg.IAddress) != 0)
            {
                GT_GetGadgetAttrs(GadgetAddress(win,12),win,NULL,GTMX_Active,&i,TAG_END);
                if (gad->GadgetID >= 7 && gad->GadgetID <= 9)
                {
                    GT_GetGadgetAttrs(gad,win,NULL,GTSL_Level,&col,TAG_END);
                    if (i)
                        col = ((col+1) << 24)-1;
                    else
                        col = col | col << 8 | col << 16 | col << 24;
					switch (gad->GadgetID)
                    {
                        case 7:
							SetGadgetAttrs(wd->wd_ExtData[1], win, NULL, (i ? WHEEL_Hue : WHEEL_Red), col, TAG_END);
                            break;
                        case 8:
							SetGadgetAttrs(wd->wd_ExtData[1], win, NULL, (i ? WHEEL_Saturation : WHEEL_Green), col, TAG_END);
                            break;
                        case 9:
							SetGadgetAttrs(wd->wd_ExtData[1], win, NULL, (i ? WHEEL_Brightness : WHEEL_Blue), col, TAG_END);
                            break;
                    }
                }
				setGradientColors(cp, TRUE);
            }
            break;
        case IDCMP_IDCMPUPDATE:
			setGradientColors(cp, TRUE);
            break;
        case IDCMP_CLOSEWINDOW:
            remapColorPen(cp);
			CloseAppWindow(win, TRUE);

            if ((APTR)scr->UserData == iport)
				LoadRGB32(&scr->ViewPort, standardPalette);
			ReleaseAppColors(scr);
			ObtainAppColors(scr, TRUE);
            RefreshProjWindows(FALSE);
            break;
    }
}

extern struct Gadget *psysPages[];

void ASM
HandleSystemPrefsIDCMP(REG(a0, struct TagItem *tag))
{
	long i, old;

	switch (imsg.Class)
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
                case 8:  // Clipboard-Unit
                    GT_GetGadgetAttrs(gad,win,NULL,GTIN_Number,&i,TAG_END);
                    if (i < 0 || i > 255)
                    {
                        DisplayBeep(NULL);
                        if (i < 0)
                            i = 0;
                        else
                            i = 255;
                        GT_SetGadgetAttrs(gad,win,NULL,GTIN_Number,i,TAG_END);
                    }
                    break;
                case 33: // Ok
                    old = prefs.pr_Flags;
                    prefs.pr_Flags = GetCheckBoxFlag(PageGadget(psysPages[0],1),win,PRF_APPICON) |
                                                     GetCheckBoxFlag(PageGadget(psysPages[0],2),win,PRF_USESESSION) |
                                                     GetCheckBoxFlag(PageGadget(psysPages[0],3),win,PRF_SECURESCRIPTS) |
                                                     GetCheckBoxFlag(PageGadget(psysPages[0],4),win,PRF_CONTEXTMENU) |
                                                     GetCheckBoxFlag(PageGadget(psysPages[0],5),win,PRF_USEDBUFFER) |
                                                     GetCheckBoxFlag(PageGadget(psysPages[0],7),win,PRF_CLIPGO);

                    GT_GetGadgetAttrs(PageGadget(psysPages[0],6),win,NULL,GTCY_Active,&i,TAG_END);
                    if (i == 1)
                        prefs.pr_Flags |= PRF_SIMPLEWINDOWS;
                    else if (i == 2)
                        prefs.pr_Flags |= PRF_SIMPLEWINDOWS | PRF_SIMPLEPROJS;

                    GT_GetGadgetAttrs(PageGadget(psysPages[1],8),win,NULL,GTIN_Number,&i,TAG_END);
                    clipunit = i;

                    /**************** Update AppIcon ****************/

                    if (prefs.pr_Flags & PRF_APPICON)
                        InitAppIcon();
                    else
                        FreeAppIcon();

                    /**************** Update prefs module ****************/

                    SetPrefsModule(&prefs,WDT_PREFSYS,TRUE);
                    PropagatePrefsModule(&prefs,WDT_PREFSYS);
                    CloseAppWindow(win,TRUE);

                    /**************** Update window type ****************/

                    if ((old & (PRF_SIMPLEPROJS | PRF_SIMPLEWINDOWS)) != (prefs.pr_Flags & (PRF_SIMPLEPROJS | PRF_SIMPLEWINDOWS)))
                        ChangeAppScreen(FALSE);

                    /**************** Update DoubleBuffer ****************/

                    if ((prefs.pr_Flags & PRF_USEDBUFFER) && !doublerp)
                    {
                        InitDoubleBuffer();
                        AllocDoubleBufferBitMap(scr);
                    }
                    else if (!(prefs.pr_Flags & PRF_USEDBUFFER) && doublerp)
                        FreeDoubleBuffer();
                    break;
                case 34: // Abbrechen
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}

void
UpdateContextGadgets(struct ContextMenu *cm)
{
    struct MinList *list = wd->wd_ExtData[(LONG)wd->wd_ExtData[7]];

    wd->wd_ExtData[NUM_CMT] = cm;

    if (cm)
    {
        UBYTE dis = cm->cm_Node.ln_Name ? !strcmp(cm->cm_Node.ln_Name,"-") : TRUE;

        GT_SetGadgetAttrs(wd->wd_ExtData[6],win,NULL,GTLV_Labels,list,GTLV_Selected,FindListEntry(list,(struct MinNode *)cm),TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GA_Disabled,FALSE,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTST_String,cm->cm_Node.ln_Name,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,cm->cm_AppCmd,GA_Disabled,dis,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,dis,TAG_END);
        GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,dis,TAG_END);
    }
    else
    {
		GT_SetGadgetAttrs(wd->wd_ExtData[6], win, NULL, GTLV_Labels, list, GTLV_Selected, ~0L, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 4), win, NULL, GA_Disabled, TRUE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 5), win, NULL, GTST_String, NULL, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 6), win, NULL, GTST_String, NULL, GA_Disabled, TRUE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 7), win, NULL, GA_Disabled, TRUE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 8), win, NULL, GA_Disabled, TRUE, TAG_END);
    }
}


void ASM
closePrefContextWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
    struct winData *wd = (struct winData *)win->UserData;

    GTD_RemoveGadgets(win);

    if (clean)
    {
        long i;

		for (i = 0; i < NUM_CMT; i++)
        {
            FreeContextMenu(wd->wd_ExtData[i]);
			FreePooled(pool, wd->wd_ExtData[i], sizeof(struct MinList));
        }
    }
}


void ASM
handlePrefContextIDCMP(REG(a0, struct TagItem *tag))
{
    struct ContextMenu *cm = wd->wd_ExtData[NUM_CMT];
	long i;

	switch (imsg.Class)
    {
        case IDCMP_GADGETDOWN:
            if ((gad = imsg.IAddress)->GadgetID == 8 && cm)
            {
                struct Mappe *mp = GetPrefsMap(GetLocalPrefs(wd->wd_Data));
                struct MinList *list;
                long   mode;

                GT_GetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTCY_Active,&mode,TAG_END);
                list = mode ? &intcmds : (mp ? &mp->mp_AppCmds : &prefs.pr_AppCmds);

                i = PopUpList(win,gad,list,mode ? TAG_IGNORE : POPA_ItemHeight, itemheight,
                                                                     mode ? TAG_IGNORE : POPA_CallBack,   mp ? &linkHook : &renderHook,
                                                                     mode ? TAG_IGNORE : POPA_MaxItems,   5,
                                                                     POPA_Selected, GetListNumberOfName(list,cm->cm_AppCmd,mode,!mode && mp),
                                                                     POPA_Width,    win->Width-boxwidth-16,
                                                                     POPA_Left,     TRUE,
                                                                     TAG_END);
                if (i != ~0L)
                {
                    struct Node *ln;
                    long   j;

                    for(j = 0,ln = (struct Node *)list->mlh_Head;j < i;j++,ln = ln->ln_Succ);
                    if (!mode && mp)
                        ln = ((struct Link *)ln)->l_Link;

                    FreeString(cm->cm_AppCmd);
                    cm->cm_AppCmd = AllocString(ln->ln_Name);
                    GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,cm->cm_AppCmd,TAG_END);
                }
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
                case 1:
                    GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTLV_Labels,wd->wd_ExtData[imsg.Code],TAG_END);
                    wd->wd_ExtData[7] = (APTR)((long)imsg.Code);
                    UpdateContextGadgets(NULL);
                    break;
                case 2:
                    if ((cm = (APTR)FindListNumber(wd->wd_ExtData[(LONG)wd->wd_ExtData[7]], imsg.Code)) != 0)
                        UpdateContextGadgets(cm);
                    break;
                case 3:
                prefcontext_new:
                    UpdateContextGadgets(NULL);
                    ActivateGadget(GadgetAddress(win,5),win,NULL);
                    break;
                case 4:
                    GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTLV_Labels,~0L,TAG_END);
                    MyRemove(cm);
                    FreeString(cm->cm_Node.ln_Name);
                    FreeString(cm->cm_AppCmd);
                    FreePooled(pool,cm,sizeof(struct ContextMenu));
                    UpdateContextGadgets(NULL);
                    break;
                case 5:
                {
                    STRPTR t;

                    if (GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END))
                    {
#ifdef __amigaos4__
						if(!Strlen(t)) //prevent crash with empty label
							break;
#endif
                        GT_SetGadgetAttrs(GadgetAddress(win,2),win,NULL,GTLV_Labels,~0L,TAG_END);
                        if (cm)
                        {
                            FreeString(cm->cm_Node.ln_Name);
                            cm->cm_Node.ln_Name = AllocString(t);
                        }
                        else
                            cm = AddContextMenu(wd->wd_ExtData[(LONG)wd->wd_ExtData[7]],t,NULL);

                        UpdateContextGadgets(cm);

                        if (cm && strcmp(cm->cm_Node.ln_Name,"-"))
                            ActivateGadget(GadgetAddress(win,6),win,NULL);
                        else
                            goto prefcontext_new;
                    }
                    break;
                }
                case 6: // Befehl
                {
                    STRPTR t;

                    if (cm && GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&t,TAG_END))
                    {
                        FreeString(cm->cm_AppCmd);
                        cm->cm_AppCmd = AllocString(t);
                    }
                    goto prefcontext_new;
                    break;
                }
                case 9: // Ok
                {
                    struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
                    struct Mappe *mp = wd->wd_Data;

                    for(i = 0;i < NUM_CMT;i++)
                    {
                        if (!IsListEmpty((struct List *)wd->wd_ExtData[i]))
                        {
                            AddPrefsModuleToLocalPrefs(mp,WDT_PREFCONTEXT);
                            break;
                        }
                    }
                    SetPrefsModule(pr,WDT_PREFCONTEXT,TRUE);

                    for(i = 0;i < NUM_CMT;i++)
                        swapLists(wd->wd_ExtData[i],&pr->pr_Contexts[i]);

                    RefreshPrefsModule(pr,NULL,WDT_PREFCONTEXT);
                }
                case 10: // Abbrechen
                    CloseAppWindow(win,TRUE);
                    break;
            }
            break;
        case IDCMP_OBJECTDROP:
        {
            struct DropMessage *dm = imsg.IAddress;

            if (dm && !dm->dm_Object.od_Owner && dm->dm_Target == wd->wd_ExtData[6] && (cm = dm->dm_Object.od_Object))
            {
                struct MinList *list = wd->wd_ExtData[(LONG)wd->wd_ExtData[7]];

                if (dm->dm_Window == win)
                {
                    GT_SetGadgetAttrs(wd->wd_ExtData[6],win,NULL,GTLV_Labels,~0L,TAG_END);
                    MoveTo((struct Node *)dm->dm_Object.od_Object,list,dm->dm_SourceEntry,list,dm->dm_TargetEntry);
                }
                else if (dm->dm_Object.od_Object && dm->dm_Object.od_InternalType == DRAGT_SUBMENU)
                {
                    struct IgnAppMenuEntry *am = (struct IgnAppMenuEntry *)dm->dm_Object.od_Object;

                    GT_SetGadgetAttrs(wd->wd_ExtData[6],win,NULL,GTLV_Labels,~0L,TAG_END);

                    cm = AddContextMenu(list,am->am_Node.ln_Name,am->am_AppCmd);
                    MyRemove(cm);
                    InsertAt(list,(struct Node *)cm,dm->dm_TargetEntry);
                }
                else
                    break;

                UpdateContextGadgets(cm);
            }
            break;
        }
        case IDCMP_CLOSEWINDOW:
            CloseAppWindow(win,TRUE);
            break;
    }
}
