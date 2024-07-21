/* Preferences window handling functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include "reactionGUI.h" // RA
	#include <proto/gtdrag.h>
#endif


extern void CreatePrefsGadgets(struct winData *wd, long width, long height);
extern void CreateKeyboardPrefsGadgets(struct winData *wd, long width, long height);
// RA
extern void contextmenuToRALB(struct MinList *min_l, struct List* lst); // prefgadgets.c
extern void prefmenuToRALB(struct MinList *min_l, struct List* lst); // prefgadgets.c
extern void prefmenuitemsToRALB(struct IgnAppMenu *am, struct IgnAppMenuEntry *ame, struct List* lst); // prefgadgets.c
extern void prefformatsToRALB(struct MinList *min_l, struct List* lst); // prefgadgets.c
extern BOOL isRAwinobjEmpty(void); // windows.c
// RA
//extern struct Gadget *PageGadget(struct Gadget *gad, long num);

extern struct RastPort *doublerp;
extern struct ColumnInfo *ra_popup_ci; // ReAction ListBrowser's ColumnInfo PopUp


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
#ifndef __amigaos4__
                if (ToggleTree(gad,tn,&imsg))
                {
                    if (tn->tn_Flags & (TNF_ADD | TNF_REPLACE))
                        RefreshPrefsTreeNode(tn);
                }
                else 
#endif
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
#ifdef __amigaos4__
							ttn->tn_Flags |= TNF_OPEN; //this is a bad workaround, because gtdrag has only a dummy function which is needed for handle it right
#endif
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
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	struct PrefDisp *pd = pr->pr_Disp;

	while( (wresult=IDoMethod(winobj[WDT_PREFDISP], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("DISPLAY wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("DISPLAY WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("DISPLAY WMHI_CLOSEWINDOW\n");
				ClosePrefDispWindow(pr);
			break;

			case WMHI_GADGETUP:
DBUG("DISPLAY WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_D_FONT:
						if( IDoMethod(OBJ(OID_D_FONT), GFONT_REQUEST, win) )//winptr) )
						{
							struct TextAttr *ta = NULL;

							GetAttr(GETFONT_TextAttr, OBJ(OID_D_FONT), (APTR)&ta);
DBUG("\tDISPLAY OID_D_FONT: %s/%ld\n",ta->ta_Name,ta->ta_YSize);
							FreeString( (STRPTR)pd->pd_AntiAttr.ta_Name );
							pd->pd_AntiAttr.ta_Name = AllocString(ta->ta_Name);
							pd->pd_AntiAttr.ta_YSize = ta->ta_YSize;
							UpdateAntiFont(pr);
							wd->wd_ExtData[0] = (APTR)TRUE;
						}
					break;
					case OID_CANCEL://11
DBUG("\tDISPLAY OID_CANCEL\n");
						ClosePrefDispWindow(pr);
					break;
					case OID_TEST://10
					case OID_OK://9
						GetAttr(CHOOSER_Selected, OBJ(OID_D_GRID), &res_val);
						pd->pd_Rasta = res_val;
						GetAttr(GA_Selected, OBJ(OID_D_CELLWIDTH), &res_val);
						pd->pd_Rasta |= res_val;
						GetAttr(GA_Selected, OBJ(OID_D_HEADINGS), &res_val);
						pd->pd_ShowAntis = res_val;
						GetAttr(GA_Selected, OBJ(OID_D_HELPBAR), &res_val);
						pd->pd_HelpBar = res_val;
						GetAttr(GA_Selected, OBJ(OID_D_TOOLBAR), &res_val);
						pd->pd_ToolBar = res_val;
						GetAttr(CHOOSER_Selected, OBJ(OID_D_FORMULABAR), &res_val);
						pd->pd_FormBar = res_val;
						GetAttr(CHOOSER_Selected, OBJ(OID_D_ICONBAR), &res_val);
						pd->pd_IconBar = res_val;
DBUG("\tpd->pd_Rasta=0x%lx   pd->pd_ShowAntis=0x%lx   pd->pd_HelpBar=0x%lx\n",pd->pd_Rasta,pd->pd_ShowAntis,pd->pd_HelpBar);
DBUG("\tpd->pd_ToolBar=0x%lx   pd->pd_IconBar=0x%lx   pd->pd_FormBar=0x%lx\n",pd->pd_ToolBar,pd->pd_IconBar,pd->pd_FormBar);

						if(wd->wd_ExtData[0] && (wresult & WMHI_GADGETMASK) == OID_TEST)
						{
DBUG("\tDISPLAY OID_TEST\n");
							RefreshPrefsModule(pr, NULL, WDT_PREFDISP);
						}

						if( (wresult&WMHI_GADGETMASK) == OID_OK )
						{
DBUG("\tDISPLAY OID_OK\n");
							FreePooled( pool, wd->wd_ExtData[1], sizeof(struct PrefDisp) );
							wd->wd_ExtData[1] = NULL;
							SetPrefsModule(pr, WDT_PREFDISP, TRUE);
							ClosePrefDispWindow(pr);
						}
					break;
					default:
						wd->wd_ExtData[0] = (APTR)TRUE;
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("DISPLAY handlePrefDispIDCMP() END\n\n");
}


void ASM
closePrefScreenWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct List *pslist;
DBUG("closePrefScreenWin()\n");
	if(clean == FALSE)
	{
		return;
	}

	// Clear/Free public screens list
	GetAttr(CHOOSER_Labels, OBJ(OID_S_SCREENNAME), (uint32 *)&pslist);
	if(pslist)
	{
		struct Node *nextnode, *node = GetHead(pslist);

		while(node)
		{
DBUG("closePrefScreenWin(): Free'ing public screen (0x%08lx) from list (0x%08lx)\n", node,pslist);
			nextnode = GetSucc(node);
			FreeChooserNode(node);
			node = nextnode;
		}
		FreeSysObject(ASOT_LIST, pslist);
	}
}

void ASM
handlePrefScreenIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	STRPTR res_string;

	while( (wresult=IDoMethod(winobj[WDT_PREFSCREEN], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("SCREEN wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("SCREEN WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("SCREEN WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("SCREEN WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_S_SCREENTYPE:
DBUG("\tSCREEN OID_S_SCREENTYPE: %ld\n",wcode);
						SetAttrs(OBJ(OID_S_SCREENNAME), GA_Disabled,wcode, TAG_DONE);
						if(wcode == 2) // user defined
						{
							SetAttrs(OBJ(OID_S_SELECTSCR), GA_Disabled,FALSE, TAG_DONE);
							SetAttrs(OBJ(OID_S_SELECTFONT), GA_Disabled,FALSE, TAG_DONE);
						}
						else // public & like WB
						{
							SetAttrs(OBJ(OID_S_SELECTSCR), GA_Disabled,TRUE, TAG_DONE);
							SetAttrs(OBJ(OID_S_SELECTFONT), GA_Disabled,TRUE, TAG_DONE);
						}
						RefreshGadgets(GAD(OID_S_SCRGROUP), win, NULL);
					break;
					case OID_S_SCREENNAME:
					{
						struct Node *n = NULL;

						GetAttr(CHOOSER_SelectedNode, OBJ(OID_S_SCREENNAME), (uint32 *)&n);
						GetChooserNodeAttrs(n, CNA_Text,&res_string, TAG_DONE);
DBUG("\tSCREEN OID_S_SCREENNAME '%s'\n",res_string);
						//RefreshSetGadgetAttrs(GAD(OID_S_SCREENNAME), win, NULL, CHOOSER_Title,res_string, TAG_DONE); // if 'maxwidth' used in prefsgadgets.c:CreatePrefScreenGads()
						SetAttrs(OBJ(OID_S_SCREENNAME), CHOOSER_Title,res_string, TAG_DONE);
						IDoMethod(winobj[WDT_PREFSCREEN], WM_RETHINK);
					}
					break;
					case OID_S_SELECTSCR:
DBUG("\tSCREEN OID_S_SELECTSCR\n");
						IDoMethod(OBJ(OID_S_SELECTSCR), GSM_REQUEST, win);
					break;
					case OID_S_SELECTFONT:
DBUG("\tSCREEN OID_S_SELECTFONT\n");
						IDoMethod(OBJ(OID_S_SELECTFONT), GFONT_REQUEST, win);
					break;
					case OID_S_BGPATTERN:
DBUG("\tSCREEN OID_S_BGPATTERN (0x%08lx)\n",wcode);
						RefreshSetGadgetAttrs(GAD(OID_S_PALETTE), win, NULL, GA_Disabled,!wcode, TAG_DONE);
					break;
					case OID_S_PALETTE:
DBUG("\tSCREEN OID_S_PALETTE (0x%08lx)\n",wcode);
						res_val = wcode;
						wd->wd_Data = (APTR)res_val; // index of color
					break;
					case OID_OK:
DBUG("\tSCREEN OID_OK\n");
						SetPrefsModule(&prefs, WDT_PREFSCREEN, TRUE);

						GetAttr(RADIOBUTTON_Selected, OBJ(OID_S_SCREENTYPE), &res_val);
						prefs.pr_Screen->ps_Type = res_val;
						if(res_val == 0)
						{
							GetAttr(CHOOSER_SelectedNode, OBJ(OID_S_SCREENNAME), &res_val);
							GetChooserNodeAttrs( (struct Node *)res_val, CNA_Text,&res_string, TAG_DONE );
							strcpy(prefs.pr_Screen->ps_PubName, (STRPTR)res_string);
DBUG("\tSCREEN pubscreen='%s'\n", res_string);
						}
						else if(res_val == 2)
						{
							struct TextAttr *ta = NULL;

							GetAttr(GETSCREENMODE_DisplayID, OBJ(OID_S_SELECTSCR), &res_val);
							prefs.pr_Screen->ps_ModeID = res_val;//scrReq->sm_DisplayID;
							GetAttr(GETSCREENMODE_DisplayWidth, OBJ(OID_S_SELECTSCR), &res_val);
							prefs.pr_Screen->ps_Width = res_val;//scrReq->sm_DisplayWidth;
							GetAttr(GETSCREENMODE_DisplayHeight, OBJ(OID_S_SELECTSCR), &res_val);
							prefs.pr_Screen->ps_Height = res_val;//scrReq->sm_DisplayHeight;
							GetAttr(GETSCREENMODE_DisplayDepth, OBJ(OID_S_SELECTSCR), &res_val);
							prefs.pr_Screen->ps_Depth = res_val;//scrReq->sm_DisplayDepth;
DBUG("\tSCREEN SCREENMODE: %4ldx%4ld@%ld (0x%08lx)\n",prefs.pr_Screen->ps_Width,prefs.pr_Screen->ps_Height,prefs.pr_Screen->ps_Depth,prefs.pr_Screen->ps_ModeID);
							GetAttr(GETFONT_TextAttr, OBJ(OID_S_SELECTFONT), (APTR)&ta);
							FreeString( (STRPTR)prefs.pr_Screen->ps_TextAttr.ta_Name );
							prefs.pr_Screen->ps_TextAttr.ta_Name = AllocString(ta->ta_Name);
							prefs.pr_Screen->ps_TextAttr.ta_YSize = ta->ta_YSize;
DBUG("\tSCREEN FONT: %s/%ld\n",ta->ta_Name,ta->ta_YSize);
						}
						GetAttr(INTEGER_Number, OBJ(OID_S_MONITORW), &res_val);
						//prefs.pr_Screen->ps_mmWidth = (long)(ConvertNumber((STRPTR)i,CNT_MM)*1024.0);
						prefs.pr_Screen->ps_mmWidth = (long)(res_val*10*1024.0);
						GetAttr(INTEGER_Number, OBJ(OID_S_MONITORH), &res_val);
						//prefs.pr_Screen->ps_mmHeight = (long)(ConvertNumber((STRPTR)i,CNT_MM)*1024.0);
						prefs.pr_Screen->ps_mmHeight = (long)(res_val*10*1024.0);
						GetAttr(GA_Selected, OBJ(OID_S_BGPATTERN), &res_val);
						prefs.pr_Screen->ps_BackFill = res_val;
						//GetAttr(PALETTE_Color, OBJ(OID_S_PALETTE), &res_val); // color value
						//prefs.pr_Screen->ps_BFColor = res_val;
						prefs.pr_Screen->ps_BFColor = (LONG)wd->wd_Data; // color index
DBUG("\tps_mmWidth=%ld   ps_mmHeight=%ld   ps_BackFill=%ld   ps_BFColor=0x%08lx\n",
						prefs.pr_Screen->ps_mmWidth,prefs.pr_Screen->ps_mmHeight,prefs.pr_Screen->ps_BackFill,prefs.pr_Screen->ps_BFColor);

						PropagatePrefsModule(&prefs, WDT_PREFSCREEN);
						CloseAppWindow(win, TRUE);
						ChangeAppScreen(TRUE);
					break;
					case OID_CANCEL:
DBUG("\tSCREEN OID_CANCEL\n");
//DBUG("\tConvertNumber=%ld (val*10*1024)\n",(long)(ConvertNumber("20,5 cm",CNT_MM)*1024.0));
						CloseAppWindow(win, TRUE);
					break;
				}
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod(..
DBUG("SCREEN handlePrefScreen() END\n\n");
}


void ASM closePrefMenuWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;

	if (clean)
	{
		FreeAppMenus(wd->wd_ExtData[3]);
		FreePooled(pool,wd->wd_ExtData[3],sizeof(struct List));

		// RA
		if(isRAwinobjEmpty() == TRUE)
		{
			FreeLBColumnInfo(ra_popup_ci); // PopUp
			ra_popup_ci = NULL;
		}
		FreeListBrowserList( (struct List *)wd->wd_ExtData[4] );    // PopUp
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[4]); // PopUp
		FreeListBrowserList( (struct List *)wd->wd_ExtData[5] );    // PopUp
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[5]); // PopUp

		FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[0]);
		FreeListBrowserList( (struct List *)wd->wd_ExtData[1] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[1]);
		FreeListBrowserList( (struct List *)wd->wd_ExtData[2] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[2]);
		// RA
	}
}

void updatePrefMenuGads(struct IgnAppMenu *am, struct IgnAppMenuEntry *ame, struct IgnAppMenuEntry *same)
{
	UBYTE dame, dsame;

	if(!am)
	{
		ame = NULL;
	}
	if(!ame)
	{
		same = NULL;
	}
	(GAD(OID_M_LISTMENUS))->UserData = am;
	(GAD(OID_M_LISTITEMS))->UserData = ame;
	(GAD(OID_M_LISTSUBITEMS))->UserData = same;

	SetAttrs(OBJ(OID_M_LISTMENUS),
	         LISTBROWSER_Selected,FindListEntry(wd->wd_ExtData[3],(struct MinNode *)am), TAG_DONE);
	RefreshGadgets(GAD(OID_M_LISTMENUS), win, NULL);
	SetAttrs(OBJ(OID_M_LISTITEMS),
	         LISTBROWSER_Selected,am? FindListEntry(&am->am_Items,(struct MinNode *)ame) : -1, TAG_DONE);
	RefreshGadgets(GAD(OID_M_LISTITEMS), win, NULL);
	SetAttrs(OBJ(OID_M_LISTSUBITEMS),
	         LISTBROWSER_Selected,ame? FindListEntry(&ame->am_Subs,(struct MinNode *)same) : -1, TAG_DONE);
	RefreshGadgets(GAD(OID_M_LISTSUBITEMS), win, NULL);

	dame = !ame || !IsListEmpty((struct List *)&ame->am_Subs);// || !strcmp(ame->am_Node.ln_Name,"-");
	dsame = !same;// || !strcmp(same->am_Node.ln_Name,"-");

	// Clear/Reset menu title's gadgets
	SetAttrs(OBJ(OID_M_UP_M), GA_Disabled,!am, TAG_DONE);
	SetAttrs(OBJ(OID_M_DOWN_M), GA_Disabled,!am, TAG_DONE);
	SetAttrs(OBJ(OID_M_DEL_M), GA_Disabled,!am, TAG_DONE);
	RefreshGadgets(GAD(OID_M_BTNGROUP_M), win, NULL);
	RefreshSetGadgetAttrs(GAD(OID_M_TITLE_M), win, NULL, STRINGA_TextVal,am? am->am_Node.ln_Name : NULL, GA_Disabled,!am, TAG_DONE);

	// Clear/Reset menu item's gadgets
	SetAttrs(OBJ(OID_M_UP_I), GA_Disabled,!am, TAG_DONE);
	SetAttrs(OBJ(OID_M_DOWN_I), GA_Disabled,!am, TAG_DONE);
	//SetAttrs(OBJ(OID_M_TO_SUBITEM), GA_Disabled,dame, TAG_DONE);
	SetAttrs(OBJ(OID_M_NEW_I), GA_Disabled,!am, TAG_DONE);
	SetAttrs(OBJ(OID_M_DEL_I), GA_Disabled,dame, TAG_DONE);
	RefreshGadgets(GAD(OID_M_BTNGROUP_I), win, NULL);
	RefreshSetGadgetAttrs(GAD(OID_M_TITLE_I), win, NULL, STRINGA_TextVal,ame? ame->am_Node.ln_Name : NULL, GA_Disabled,!ame, TAG_DONE);
	RefreshSetGadgetAttrs(GAD(OID_M_HOTKEY_I), win, NULL, STRINGA_TextVal,ame? ame->am_ShortCut : NULL, GA_Disabled,dame, TAG_DONE);
	RefreshSetGadgetAttrs(GAD(OID_M_COMMAND_I), win, NULL, STRINGA_TextVal,ame? ame->am_AppCmd : NULL, GA_Disabled,dame, TAG_DONE);
	RefreshSetGadgetAttrs(GAD(OID_M_GROUP_I), win, NULL, GA_Disabled,dame, TAG_DONE);

	// Clear/Reset menu subitem's gadgets
	SetAttrs(OBJ(OID_M_UP_SI), GA_Disabled,!ame, TAG_DONE);
	SetAttrs(OBJ(OID_M_DOWN_SI), GA_Disabled,!ame, TAG_DONE);
	//SetAttrs(OBJ(OID_M_TO_ITEM), GA_Disabled,dsame, TAG_DONE);
	SetAttrs(OBJ(OID_M_NEW_SI), GA_Disabled,!ame, TAG_DONE);
	SetAttrs(OBJ(OID_M_DEL_SI), GA_Disabled,dsame, TAG_DONE);
	RefreshGadgets(GAD(OID_M_BTNGROUP_SI), win, NULL);
	RefreshSetGadgetAttrs(GAD(OID_M_TITLE_SI), win, NULL, STRINGA_TextVal,same? same->am_Node.ln_Name : NULL, GA_Disabled,!same, TAG_DONE);
	RefreshSetGadgetAttrs(GAD(OID_M_HOTKEY_SI), win, NULL, STRINGA_TextVal,same? same->am_ShortCut : NULL, GA_Disabled,dsame, TAG_DONE);
	RefreshSetGadgetAttrs(GAD(OID_M_COMMAND_SI), win, NULL, STRINGA_TextVal,same? same->am_AppCmd : NULL, GA_Disabled,dsame, TAG_DONE);
	RefreshSetGadgetAttrs(GAD(OID_M_GROUP_SI), win, NULL, GA_Disabled,dsame, TAG_DONE);
}

void ASM handlePrefMenuIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 wresult = WMHI_LASTMSG, res_val;
	STRPTR res_string;
	struct Node *res_n = NULL;
	struct IgnAppMenu *am;
	struct IgnAppMenuEntry *ame, *same;

	am = (GAD(OID_M_LISTMENUS))->UserData;
	ame = (GAD(OID_M_LISTITEMS))->UserData;
	same = (GAD(OID_M_LISTSUBITEMS))->UserData;

	while( (wresult=IDoMethod(winobj[WDT_PREFMENU], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("MENU wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("MENU WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("MENU WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("MENU WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_M_LISTMENUS:
DBUG("\tMENU OID_M_LISTMENUS %ld:\n",wcode);
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTMENUS), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&am, TAG_DONE); // context menu list [NATIVE] item
DBUG("\t(am=0x%08lx) '%s'\n",am,am->am_Node.ln_Name);
						// Detach items list(browser)
						SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
						prefmenuitemsToRALB(am, NULL, (struct List*)wd->wd_ExtData[1]);
						// Re-attach the list(browser)
						SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,wd->wd_ExtData[1], TAG_DONE);

						// Clean/Reset menu subitem's list(browser)
						SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
						FreeListBrowserList( (struct List *)wd->wd_ExtData[2] );
						SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,wd->wd_ExtData[2], TAG_DONE);

						updatePrefMenuGads(am, NULL, NULL);
					break;
					case OID_M_LISTITEMS:
DBUG("\tMENU OID_M_LISTITEMS %ld:\n",wcode);
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTITEMS), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ame, TAG_DONE); // context menu list [NATIVE] item
DBUG("\t(ame=0x%08lx) '%s'\n",ame,ame->am_Node.ln_Name);
						// Detach subitems list(browser)
						SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
						prefmenuitemsToRALB(NULL, ame, (struct List*)wd->wd_ExtData[2]);
						// Re-attach the list(browser)
						SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,wd->wd_ExtData[2], TAG_DONE);

						updatePrefMenuGads(am, ame, NULL);
					break;
					case OID_M_LISTSUBITEMS:
DBUG("\tMENU OID_M_LISTSUBITEMS %ld:\n",wcode);
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTSUBITEMS), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&same, TAG_DONE); // context menu list [NATIVE] subitem
DBUG("\t(same=0x%08lx) '%s'\n",same,same->am_Node.ln_Name);

						updatePrefMenuGads(am, ame, same);
					break;

					case OID_M_UP_M:
						GetAttrs(OBJ(OID_M_LISTMENUS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, TAG_DONE);
DBUG("\tMENU OID_M_UP_M: n=0x%08lx %ld\n",res_n,res_val);
						if(res_val != 0)
						{
							struct IgnAppMenu *sel_am, *mov_am;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_am, TAG_DONE); // selected
							if(res_val != 1)
							{// Insert a node into a doubly linked list AFTER a given node...
								res_n = GetPred( GetPred(res_n) );
								//res_n = GetPred(res_n);
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&mov_am, TAG_DONE); // new position (previous)
							}

							SetAttrs(OBJ(OID_M_LISTMENUS), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_am);
							Insert( (struct List *)wd->wd_ExtData[3], (struct Node *)sel_am, res_val==1? 0:(struct Node *)mov_am );
							prefmenuToRALB( (struct MinList *)wd->wd_ExtData[3], (struct List *)wd->wd_ExtData[0] );
							RefreshSetGadgetAttrs(GAD(OID_M_LISTMENUS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
							                      LISTBROWSER_Selected,--res_val, TAG_DONE);
						}
					break;
					case OID_M_DOWN_M:
					{
						int32 total;

						GetAttrs(OBJ(OID_M_LISTMENUS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tMENU OID_M_DOWN_M: n=0x%08lx %ld (tot=%ld)\n",res_n,res_val,total);
						if(res_val != total-1)
						{
							struct IgnAppMenu *sel_am, *mov_am;
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_am, TAG_DONE); // selected
							GetListBrowserNodeAttrs(GetSucc(res_n), LBNA_UserData,&mov_am, TAG_DONE); // new position (next)

							SetAttrs(OBJ(OID_M_LISTMENUS), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_am);
							Insert( (struct List *)wd->wd_ExtData[3], (struct Node *)sel_am, (struct Node *)mov_am );
							prefmenuToRALB( (struct MinList *)wd->wd_ExtData[3], (struct List *)wd->wd_ExtData[0] );
							RefreshSetGadgetAttrs(GAD(OID_M_LISTMENUS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
							                      LISTBROWSER_Selected,++res_val, TAG_DONE);
						}
					}
					break;
					case OID_M_NEW_M:
DBUG("\tMENU OID_M_NEW_M:\n");
						RefreshSetGadgetAttrs(GAD(OID_M_LISTMENUS), win, NULL, LISTBROWSER_Selected,-1, TAG_DONE);
						SetAttrs(OBJ(OID_M_UP_M), GA_Disabled,TRUE, TAG_DONE);
						SetAttrs(OBJ(OID_M_DOWN_M), GA_Disabled,TRUE, TAG_DONE);
						SetAttrs(OBJ(OID_M_DEL_M), GA_Disabled,TRUE, TAG_DONE);
						RefreshGadgets(GAD(OID_M_BTNGROUP_M), win, NULL);
						RefreshSetGadgetAttrs(GAD(OID_M_TITLE_M), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,FALSE, TAG_DONE);
					break;
					case OID_M_DEL_M:
DBUG("\tMENU OID_M_DEL_M:\n");
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTMENUS), (uint32 *)&res_n);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&am, TAG_DONE); // context menu list [NATIVE] titles
DBUG("\tam=0x%08lx '%s' (0x%08lx)\n",am,am->am_Node.ln_Name,res_n);
							MyRemove(am);
							FreeString(am->am_Node.ln_Name);
							FreePooled( pool, am, sizeof(struct IgnAppMenu) );
							// Remove menu title's list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_M_LISTMENUS), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshGadgets(GAD(OID_M_LISTMENUS), win, NULL);

							// Clear menu items' list(browser)
							SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							FreeListBrowserList( (struct List *)wd->wd_ExtData[1] );
							SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,wd->wd_ExtData[1], TAG_DONE);
							// Clear menu subitem's list(browser)
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							FreeListBrowserList( (struct List *)wd->wd_ExtData[2] );
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,wd->wd_ExtData[2], TAG_DONE);

							updatePrefMenuGads(NULL, NULL, NULL);
						}
					break;
					case OID_M_TITLE_M:
						GetAttrs(OBJ(OID_M_TITLE_M), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tMENU OID_M_TITLE_M: '%s'\n",res_string);
						if(*(res_string) != '\0')
						{
							am = NULL;

							GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTMENUS), (uint32 *)&res_n);
DBUG("\tres_n=0x%08lx\n",res_n);
							if(res_n != NULL)
							{
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&am, TAG_DONE); // context menu list [NATIVE] title
DBUG("\tam=0x%08lx old:'%s'   new:'%s'\n",am,am->am_Node.ln_Name,res_string);
							}

							if(am)
							{
								FreeString(am->am_Node.ln_Name);
								am->am_Node.ln_Name = AllocString(res_string);
							}
							else
							{
								am = AllocPooled( pool, sizeof(struct IgnAppMenu) );
								am->am_Node.ln_Name = AllocString(res_string);
								MyNewList(&am->am_Items);
								MyAddTail(wd->wd_ExtData[3], am);
							}
DBUG("\tam=0x%08lx\n",am);
							SetAttrs(OBJ(OID_M_LISTMENUS), LISTBROWSER_Labels,NULL, TAG_DONE);
							prefmenuToRALB( (struct MinList *)wd->wd_ExtData[3], (struct List *)wd->wd_ExtData[0] );
							SetAttrs(OBJ(OID_M_LISTMENUS), LISTBROWSER_Labels,wd->wd_ExtData[0], TAG_DONE);
							// Clear/Reset menu item's list(browser)
							SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							FreeListBrowserList( (struct List *)wd->wd_ExtData[1] );
							SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,wd->wd_ExtData[1], TAG_DONE);
							// Clear/Reset menu subitem's list(browser)
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							FreeListBrowserList( (struct List *)wd->wd_ExtData[2] );
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,wd->wd_ExtData[2], TAG_DONE);

							updatePrefMenuGads(am, NULL, NULL);
						}
					break;

					case OID_M_UP_I:
						GetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, TAG_DONE);
DBUG("\tMENU OID_M_UP_I: n=0x%08lx %ld\n",res_n,res_val);
						if(res_val != 0)
						{
							struct IgnAppMenu *sel_am, *mov_am;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_am, TAG_DONE); // selected
							if(res_val != 1)
							{// Insert a node into a doubly linked list AFTER a given node...
								res_n = GetPred( GetPred(res_n) );
								//res_n = GetPred(res_n);
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&mov_am, TAG_DONE); // new position (previous)
							}

							SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_am);
							Insert( (struct List *)&am->am_Items, (struct Node *)sel_am, res_val==1? 0:(struct Node *)mov_am );
							prefmenuitemsToRALB(am, NULL, (struct List*)wd->wd_ExtData[1]);
							RefreshSetGadgetAttrs(GAD(OID_M_LISTITEMS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[1],
							                      LISTBROWSER_Selected,--res_val, TAG_DONE);
						}
					break;
					case OID_M_DOWN_I:
					{
						int32 total;

						GetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tMENU OID_M_DOWN_M: n=0x%08lx %ld (tot=%ld)\n",res_n,res_val,total);
						if(res_val != total-1)
						{
							struct IgnAppMenu *sel_am, *mov_am;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_am, TAG_DONE); // selected
							GetListBrowserNodeAttrs(GetSucc(res_n), LBNA_UserData,&mov_am, TAG_DONE); // new position (next)

							SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_am);
							Insert( (struct List *)&am->am_Items, (struct Node *)sel_am, (struct Node *)mov_am );
							prefmenuitemsToRALB(am, NULL, (struct List*)wd->wd_ExtData[1]);
							RefreshSetGadgetAttrs(GAD(OID_M_LISTITEMS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[1],
							                      LISTBROWSER_Selected,++res_val, TAG_DONE);
						}
					}
					break;
					/*case OID_M_TO_SUBITEM:
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTITEMS), (uint32 *)&res_n);
DBUG("\tMENU OID_M_TO_SUBITEM 0x%08lx:\n",res_n);
						if(res_n != NULL)
						{
							struct IgnAppMenu *sel_am;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_am, TAG_DONE); // context menu list [NATIVE] title
//MyRemove(dm->dm_Object.od_Object);
//InsertAt(&ame->am_Subs,dm->dm_Object.od_Object,dm->dm_TargetEntry);
							MyRemove(sel_am);
							InsertAt(&ame->am_Subs, (struct Node *)sel_am, -1); // add to tail/bottom

							updatePrefMenuGads(am, ame, same);
						}
					break;*/
					case OID_M_NEW_I:
DBUG("\tMENU OID_M_NEW_I:\n");
						RefreshSetGadgetAttrs(GAD(OID_M_LISTITEMS), win, NULL, LISTBROWSER_Selected,-1, TAG_DONE);
						SetAttrs(OBJ(OID_M_UP_I), GA_Disabled,TRUE, TAG_DONE);
						SetAttrs(OBJ(OID_M_DOWN_I), GA_Disabled,TRUE, TAG_DONE);
						SetAttrs(OBJ(OID_M_DEL_I), GA_Disabled,TRUE, TAG_DONE);
						RefreshGadgets(GAD(OID_M_BTNGROUP_I), win, NULL);
						RefreshSetGadgetAttrs(GAD(OID_M_TITLE_I), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,FALSE, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_M_HOTKEY_I), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,TRUE, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_M_COMMAND_I), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,TRUE, TAG_DONE);
					break;
					case OID_M_DEL_I:
DBUG("\tMENU OID_M_DEL_I:\n");
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTITEMS), (uint32 *)&res_n);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ame, TAG_DONE); // context menu list [NATIVE] items
DBUG("\tame=0x%08lx '%s' (0x%08lx)\n",ame,ame->am_Node.ln_Name,res_n);
							MyRemove(ame);
							FreeString(ame->am_Node.ln_Name);
							FreeString(ame->am_ShortCut);
							FreeString(ame->am_AppCmd);
							while( (same=(struct IgnAppMenuEntry *)MyRemHead(&ame->am_Subs)) != NULL )
							{
								FreeString(same->am_Node.ln_Name);
								FreeString(same->am_ShortCut);
								FreeString(same->am_AppCmd);
							}
							FreePooled( pool, ame, sizeof(struct IgnAppMenuEntry) );
							// Remove menu items's list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_M_LISTITEMS), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshGadgets(GAD(OID_M_LISTITEMS), win, NULL);

							// Clear menu subitem's list(browser)
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							FreeListBrowserList( (struct List *)wd->wd_ExtData[2] );
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,wd->wd_ExtData[2], TAG_DONE);

							updatePrefMenuGads(am, NULL, NULL);
						}
					break;

					case OID_M_UP_SI:
						GetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, TAG_DONE);
DBUG("\tMENU OID_M_UP_SI: n=0x%08lx %ld\n",res_n,res_val);
						if(res_val != 0)
						{
							struct IgnAppMenuEntry *sel_ame, *mov_ame;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_ame, TAG_DONE); // selected
							if(res_val != 1)
							{// Insert a node into a doubly linked list AFTER a given node...
								res_n = GetPred( GetPred(res_n) );
								//res_n = GetPred(res_n);
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&mov_ame, TAG_DONE); // new position (previous)
							}

							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_ame);
							Insert( (struct List *)(&ame->am_Subs), (struct Node *)sel_ame, res_val==1? 0:(struct Node *)mov_ame );
							prefmenuitemsToRALB(NULL, ame, (struct List*)wd->wd_ExtData[2]);
							RefreshSetGadgetAttrs(GAD(OID_M_LISTSUBITEMS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[2],
							                      LISTBROWSER_Selected,--res_val, TAG_DONE);
						}
					break;
					case OID_M_DOWN_SI:
					{
						int32 total;

						GetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tMENU OID_M_DOWN_SI: n=0x%08lx %ld (tot=%ld)\n",res_n,res_val,total);
						if(res_val != total-1)
						{
							struct IgnAppMenuEntry *sel_ame, *mov_ame;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_ame, TAG_DONE); // selected
							GetListBrowserNodeAttrs(GetSucc(res_n), LBNA_UserData,&mov_ame, TAG_DONE); // new position (next)

							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_ame);
							Insert( (struct List *)(&ame->am_Subs), (struct Node *)sel_ame, (struct Node *)mov_ame );
							prefmenuitemsToRALB(NULL, ame, (struct List*)wd->wd_ExtData[2]);
							RefreshSetGadgetAttrs(GAD(OID_M_LISTSUBITEMS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[2],
							                      LISTBROWSER_Selected,++res_val, TAG_DONE);
						}
					}
					break;
					/*case OID_M_TO_ITEM:
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTITEMS), (uint32 *)&res_n);
DBUG("\tMENU OID_M_TO_ITEM 0x%08lx:\n",res_n);
						if(res_n != NULL)
						{
							struct IgnAppMenuEntre *sel_ame;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_ame, TAG_DONE); // selected
//MyRemove(dm->dm_Object.od_Object);
//InsertAt(&am->am_Items, dm->dm_Object.od_Object, dm->dm_TargetEntry);
							MyRemove(sel_ame);
							InsertAt(&am->am_Items, (struct Node *)sel_ame, -1); // add to tail/bottom

							updatePrefMenuGads(am, ame, same);
						}
					break;*/
					case OID_M_NEW_SI:
DBUG("\tMENU OID_M_NEW_SI:\n");
						RefreshSetGadgetAttrs(GAD(OID_M_LISTSUBITEMS), win, NULL, LISTBROWSER_Selected,-1, TAG_DONE);
						SetAttrs(OBJ(OID_M_UP_SI), GA_Disabled,TRUE, TAG_DONE);
						SetAttrs(OBJ(OID_M_DOWN_SI), GA_Disabled,TRUE, TAG_DONE);
						SetAttrs(OBJ(OID_M_DEL_SI), GA_Disabled,TRUE, TAG_DONE);
						RefreshGadgets(GAD(OID_M_BTNGROUP_SI), win, NULL);
						RefreshSetGadgetAttrs(GAD(OID_M_TITLE_SI), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,FALSE, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_M_HOTKEY_SI), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,TRUE, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_M_COMMAND_SI), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,TRUE, TAG_DONE);
					break;
					case OID_M_DEL_SI:
DBUG("\tMENU OID_M_DEL_SI:\n");
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_M_LISTSUBITEMS), (uint32 *)&res_n);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&same, TAG_DONE); // context menu list [NATIVE] subitems
DBUG("\tame=0x%08lx '%s' (0x%08lx)\n",same,same->am_Node.ln_Name,res_n);
							MyRemove(same);
							FreeString(same->am_Node.ln_Name);
							FreeString(same->am_ShortCut);
							FreeString(same->am_AppCmd);
							FreePooled( pool, same, sizeof(struct IgnAppMenuEntry) );
							// Remove menu subitems's list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_M_LISTSUBITEMS), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshGadgets(GAD(OID_M_LISTSUBITEMS), win, NULL);

							updatePrefMenuGads(am, ame, NULL);
						}
					break;

					case OID_M_TITLE_SI:
DBUG("\tMENU OID_M_TITLE_SI:\n");
					//break;
					case OID_M_TITLE_I:
DBUG("\tMENU OID_M_TITLE_I:\n");
						wcode = wresult & WMHI_GADGETMASK; // (sub)item OID_M_TITLE_[S]I value

						GetAttrs(OBJ(wcode), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\t'%s'\n",res_string);
						if(*(res_string) != '\0')
						{
							struct IgnAppMenuEntry *iam = NULL;

							GetAttr(LISTBROWSER_SelectedNode, OBJ(wcode-7), (uint32 *)&res_n); // OID_M_TITLE_[S]I - 7 = OID_M_LISTITEMS_[S]I
DBUG("\tres_n=0x%08lx\n",res_n);
							if(res_n != NULL)
							{
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&iam, TAG_DONE); // context menu list [NATIVE] (sub)item
DBUG("\tame=0x%08lx old:'%s'   new:'%s'\n",iam,iam->am_Node.ln_Name,res_string);
							}

							if(iam)
							{
								FreeString(iam->am_Node.ln_Name);
								iam->am_Node.ln_Name = AllocString(res_string);
								updatePrefMenuGads(am, ame, same);
							}
							else
							{
								iam = AllocPooled( pool, sizeof(struct IgnAppMenuEntry) );
								iam->am_Node.ln_Name = AllocString(res_string);
								MyNewList(&iam->am_Subs);
								if(wcode == OID_M_TITLE_I)
								{
									MyAddTail(&am->am_Items, ame=iam);
								}
								else
								{
									MyAddTail(&ame->am_Subs, same=iam);
									FreeString(ame->am_AppCmd); // "clear" AppCmd
									ame->am_AppCmd = NULL;
								}
							}
DBUG("\tiam=0x%08lx\n",iam);
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
							if(wcode == OID_M_TITLE_I)
							{
								SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,NULL, TAG_DONE);
								prefmenuitemsToRALB(am, NULL, (struct List*)wd->wd_ExtData[1]);
								SetAttrs(OBJ(OID_M_LISTITEMS), LISTBROWSER_Labels,wd->wd_ExtData[1],
								         LISTBROWSER_MakeVisible,999, TAG_DONE); // go to bottom/last item
								// Clean/Reset menu subitem's list(browser)
								FreeListBrowserList( (struct List *)wd->wd_ExtData[2] );
							}
							else
							{
								prefmenuitemsToRALB(NULL, ame, (struct List*)wd->wd_ExtData[2]);
							}
							SetAttrs(OBJ(OID_M_LISTSUBITEMS), LISTBROWSER_Labels,wd->wd_ExtData[2],
							         LISTBROWSER_MakeVisible,999, TAG_DONE); // go to bottom/last item

							updatePrefMenuGads(am, ame, wcode==OID_M_TITLE_SI? same : NULL);
						}
					break;
					case OID_M_HOTKEY_I:
DBUG("\tMENU OID_M_HOTKEY_I:\n");
						same = ame;
					//break;
					case OID_M_HOTKEY_SI:
DBUG("\tMENU OID_M_HOTKEY_SI:\n");
						wcode = wresult & WMHI_GADGETMASK; // (sub)item OID_M_HOTKEY_[S]I value

						GetAttrs(OBJ(wcode), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tsame=0x%08lx old:'%s'   new:'%s'\n",same,same->am_ShortCut,res_string);
						FreeString(same->am_ShortCut);
						same->am_ShortCut = AllocString(res_string);
					break;
					case OID_M_COMMAND_I:
DBUG("\tMENU OID_M_COMMAND_I:\n");
						same = ame;
					//break;
					case OID_M_COMMAND_SI:
DBUG("\tMENU OID_M_COMMAND_SI:\n");
						wcode = wresult & WMHI_GADGETMASK; // (sub)item OID_M_COMMAND_[S]I value

						GetAttrs(OBJ(wcode), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tsame=0x%08lx old:'%s'   new:'%s'\n",same,same->am_AppCmd,res_string);
						FreeString(same->am_AppCmd);
						same->am_AppCmd = AllocString(res_string);

						updatePrefMenuGads(am, same==ame? NULL : ame, NULL);
					break;
					case OID_M_POPUPBTN_SI:
DBUG("\tMENU OID_M_POPUPBTN_SI:\n");
						//ame = same;
					//break;
					case OID_M_POPUPBTN_I:
					{
DBUG("\tMENU OID_M_POPUPBTN_I:\n");
						struct Mappe *mp = GetPrefsMap( GetLocalPrefs(wd->wd_Data) );
						struct MinList *list;
						uint32 i = 0;

						wcode = wresult & WMHI_GADGETMASK; // (sub)item OID_M_POPUPBTN_[S]I value

						GetAttr(CHOOSER_Selected, OBJ(wcode-1), &res_val); // wcode-1 = OID_M_TYPE_[S]I
DBUG("\t%ld   mp=0x%08lx   wcode=%ld\n",res_val,mp,wcode);
						list = res_val==0? (mp? &mp->mp_AppCmds : &prefs.pr_AppCmds) : &intcmds;
						i = PopUpRAList(ra_popup_ci, (struct List *)wd->wd_ExtData[4+res_val], // res_val=0|1
						                POPA_MaxItems, res_val==0? 5:15,
						                //res_val? TAG_IGNORE:POPA_MaxItems, 5,
						                POPA_HorizProp, res_val,
						                //POPA_Selected, GetListNumberOfName(list,ame->am_AppCmd,res_val,!res_val&&mp);
						                POPA_Width, (GAD(OID_M_TITLE_I))->Width, // OID_M_TITLE_SI has same width
						               TAG_DONE);
DBUG("\ti=0x%08lx\n",i);
						if(i != 0)
						{
							struct Node *ln = (struct Node *)i;

							if(!res_val && mp)
							{
								ln = ((struct Link *)ln)->l_Link;
							}
DBUG("\tmp=0x%08lx '%s'\n",mp,ln->ln_Name);
							GetAttr(LISTBROWSER_SelectedNode, OBJ(wcode-12), (uint32 *)&res_n); // wcode-12 = OID_M_LIST[SUB]ITEMS
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ame, TAG_DONE); // context menu list [NATIVE] (sub)item
DBUG("\tame=0x%08lx old:'%s'   new:'%s'  res_val=%ld\n",ame,ame->am_AppCmd,ln->ln_Name,res_val);
							FreeString(ame->am_AppCmd);
							ame->am_AppCmd = AllocString(ln->ln_Name);
							RefreshSetGadgetAttrs(GAD(wcode-2), win, NULL, STRINGA_TextVal,ame->am_AppCmd, TAG_DONE); // wcode-2 = OID_M_COMMAND_[S]I
						}
					}
					break;

					case OID_OK://9
					{
						struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

						if( !IsListEmpty((struct List *)wd->wd_ExtData[3]) )
						{
							AddPrefsModuleToLocalPrefs(GetPrefsMap(pr), WDT_PREFMENU);
						}

						SetPrefsModule(pr, WDT_PREFMENU, TRUE);

						swapLists(wd->wd_ExtData[3], &pr->pr_AppMenus);
						CloseAppWindow(win, TRUE);
						RefreshPrefsModule(pr, NULL, WDT_PREFMENU);
					}
					break;
					case OID_CANCEL://11
DBUG("\tMENU OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("SYSTEM handlePrefMenuIDCMP() END\n\n");
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
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct PrefFile *pf = wd->wd_Data;
	STRPTR res_string;

	while( (wresult=IDoMethod(winobj[WDT_PREFFILE], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("FILE wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("FILE WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("FILE WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("FILE WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_F_PATHDOCS:
					case OID_F_PATHGFX:
					case OID_F_PATHICONS:
						res_val = wcode;//wresult & WMHI_GADGETMASK;
DBUG("\tFILE OID_F_%04ld [%ld]:\n",res_val,res_val-OID_F_PATHDOCS);
						if( IDoMethod(OBJ(res_val), GFILE_REQUEST, win) )//winptr) )
						{
							//GetAttrs(OBJ(res_val), GETFILE_FullFile,&res_string, TAG_DONE);
							GetAttrs(OBJ(res_val), GETFILE_Drawer,&res_string, TAG_DONE);
DBUG("\t'%s'\n",res_string);
							res_val -= OID_F_PATHDOCS; // res_val - OID_F_PATHDOCS = 0|1|2
							FreeString(wd->wd_ExtData[res_val]);
							wd->wd_ExtData[res_val] = AllocString(res_string);
DBUG("\twd->wd_ExtData[%ld]='%s'\n",res_val,wd->wd_ExtData[res_val]);
						}
					break;
					case OID_F_AUTOSAVE:
						//GetAttrs(OBJ(OID_F_AUTOSAVE), CHOOSER_Selected,&res_val, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_F_AUTOSAVETIME), win, NULL, GA_Disabled,wcode? FALSE:TRUE, TAG_DONE);
						pf->pf_AutoSave = wcode;
DBUG("\tFILE OID_F_AUTOSAVE: %ld\n",pf->pf_AutoSave);
					break;
					case OID_OK:
DBUG("\tFILE OID_OK\n");
						GetAttr(GA_Selected, OBJ(OID_F_CREATEICONS), &res_val);
						//pf->pf_Flags = (pf->pf_Flags & ~PFF_ICONS) | (res_val? PFF_ICONS:0);
						pf->pf_Flags = res_val;
						GetAttr(GA_Selected, OBJ(OID_F_BACKUP), &res_val);
						//pf->pf_Flags = (pf->pf_Flags & ~PFF_BACKUP) | (res_val? PFF_BACKUP:0);
						if(res_val) pf->pf_Flags |= PFF_BACKUP;
						GetAttr(GA_Selected, OBJ(OID_F_NOEXT), &res_val);
						//pf->pf_Flags = (pf->pf_Flags & ~PFF_NOSUFFIX) | (res_val? PFF_NOSUFFIX:0);
						if(res_val) pf->pf_Flags |= PFF_NOSUFFIX;
						GetAttr(GA_Selected, OBJ(OID_F_WARNIOFMT), &res_val);
						//pf->pf_Flags = (pf->pf_Flags & ~PFF_WARN_IOTYPE) | (res_val? PFF_WARN_IOTYPE:0);
						if(res_val) pf->pf_Flags |= PFF_WARN_IOTYPE;

						GetAttr(GA_Selected, OBJ(OID_F_AUTOSAVETIME), &res_val);
						pf->pf_AutoSaveIntervall = res_val*60;
						//if( (pf->pf_AutoSaveIntervall=ConvertTime((STRPTR)res_val)) == -1L )
						//{
						//	ErrorRequest( GetString(&gLocaleInfo,MSG_INVALID_TIME_ERR) );
						//}

						FreeString(projpath);
						FreeString(graphicpath);
						FreeString(iconpath);
						GetAttrs(OBJ(OID_F_PATHDOCS), GETFILE_Drawer,&res_string, TAG_DONE);
						projpath = AllocString(res_string);
						GetAttrs(OBJ(OID_F_PATHGFX), GETFILE_Drawer,&res_string, TAG_DONE);
						graphicpath = AllocString(res_string);
						GetAttrs(OBJ(OID_F_PATHICONS), GETFILE_Drawer,&res_string, TAG_DONE);
						iconpath = AllocString(res_string);

						CopyMem( pf, prefs.pr_File, sizeof(struct PrefFile) );
						if(prefs.pr_File->pf_AutoSaveIntervall < 60)
						{
							prefs.pr_File->pf_AutoSaveIntervall = 60;
						}
						CurrentTime(&lastsecs, (ULONG *)&res_val); // Zähler zurücksetzen

						SetPrefsModule(&prefs, WDT_PREFFILE, TRUE);
						PropagatePrefsModule(&prefs, WDT_PREFFILE);
DBUG("\t           pf ->pf_Flags=0x%08lx   ->pf_AutoSave=%ld   ->pf_AutoSaveIntervall=%ld\n",pf->pf_Flags,pf->pf_AutoSave,pf->pf_AutoSaveIntervall);
DBUG("\tprefs.pr_File ->pf_Flags=0x%08lx   ->pf_AutoSave=%ld   ->pf_AutoSaveIntervall=%ld\n",prefs.pr_File->pf_Flags,prefs.pr_File->pf_AutoSave,prefs.pr_File->pf_AutoSaveIntervall);
						CloseAppWindow(win, TRUE);
					break;
					case OID_CANCEL:
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("FILE HandleFilePrefsIDCMP() END\n\n");
}


void ASM
CloseTablePrefsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;

	if (!clean)
		return;

	if (wd->wd_ExtData[0])
		FreePooled(pool, wd->wd_ExtData[0], sizeof(struct PrefTable));

	FreeSysObject(ASOT_LIST, wd->wd_ExtData[3]);
	//FreeStringList(wd->wd_ExtData[3]);
}

void ASM HandleTablePrefsIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct PrefTable *pt = wd->wd_ExtData[0];
	struct Node *res_n = NULL;
	STRPTR res_string;

	while( (wresult=IDoMethod(winobj[WDT_PREFTABLE], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("TABLE wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("TABLE WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("TABLE WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("TABLE WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_T_QUALIFIER:
DBUG("\tTABLE OID_T_QUALIFIER: %ld   Function=%ld   OnlyText=%ld\nn",wcode,pt->pt_EditFunc[wcode]&PTEF_FUNCMASK,pt->pt_EditFunc[wcode]&PTEF_TEXTONLY);
						RefreshSetGadgetAttrs(GAD(OID_T_ONLYTEXT), win, NULL, GA_Selected,pt->pt_EditFunc[wcode]&PTEF_TEXTONLY, TAG_DONE);
						wd->wd_ExtData[4] = (APTR)((ULONG)wcode);
						wcode = pt->pt_EditFunc[wcode] & PTEF_FUNCMASK;
						SetAttrs(OBJ(OID_T_QUALFUNC), CHOOSER_Selected,wcode, TAG_DONE);
					//break; // OID_T_QUALIFIER changes OID_T_QUALFUNC "automagically"
					case OID_T_QUALFUNC:
DBUG("\tTABLE OID_T_QUALFUNC: %ld\n",wcode);
						pt->pt_EditFunc[(long)wd->wd_ExtData[4]] = (pt->pt_EditFunc[(long)wd->wd_ExtData[4]]&PTEF_TEXTONLY) | wcode;

						GetAttr(CHOOSER_SelectedNode, OBJ(OID_T_QUALFUNC), (uint32 *)&res_n);
						GetChooserNodeAttrs(res_n, CNA_Text,&res_string, TAG_DONE);
DBUG("TABLE OID_T_QUALFUNC: '%s'\n", res_string);
						RefreshSetGadgetAttrs(GAD(OID_T_QUALFUNC), win, NULL, CHOOSER_Title,res_string, TAG_DONE);
					break;
					case OID_OK:
					{
						BOOL century = FALSE;
DBUG("\tTABLE OID_OK\n");
						GetAttr(GA_Selected, OBJ(OID_T_SHOWFORMULAS), &res_val);
						//pt->pt_Flags = (pt->pt_Flags & ~PTF_SHOWFORMULA) | (imsg.Code ? PTF_SHOWFORMULA : 0);
						pt->pt_Flags = res_val;
						GetAttr(GA_Selected, OBJ(OID_T_SHOWZEROS), &res_val);
						//pt->pt_Flags = (pt->pt_Flags & ~PTF_SHOWZEROS) | (imsg.Code ? PTF_SHOWZEROS : 0);
						if(res_val) pt->pt_Flags |= PTF_SHOWZEROS;
						GetAttr(GA_Selected, OBJ(OID_T_ADJCELL), &res_val);
						//pt->pt_Flags = (pt->pt_Flags & ~PTF_AUTOCELLSIZE) | (imsg.Code ? PTF_AUTOCELLSIZE : 0);
						if(res_val) pt->pt_Flags |= PTF_AUTOCELLSIZE;
						GetAttr(GA_Selected, OBJ(OID_T_CENTURY), &res_val);
						//pt->pt_Flags = (pt->pt_Flags & ~PTF_AUTOCENTURY) | (imsg.Code ? PTF_AUTOCENTURY : 0);
						if(res_val) pt->pt_Flags |= PTF_AUTOCENTURY;

						GetAttr(GA_Selected, OBJ(OID_T_SHOWDRAGGABLE), &res_val);
						//pt->pt_Flags = (pt->pt_Flags & ~PTF_EDITFUNC) | (imsg.Code ? PTF_EDITFUNC : 0);
						if(res_val) pt->pt_Flags = PTF_EDITFUNC;
						GetAttr(GA_Selected, OBJ(OID_T_ONLYTEXT), &res_val);
						//pt->pt_EditFunc[(long)wd->wd_ExtData[4]] = (pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_FUNCMASK) | (imsg.Code ? PTEF_TEXTONLY : 0);
						pt->pt_EditFunc[(long)wd->wd_ExtData[4]] = (pt->pt_EditFunc[(long)wd->wd_ExtData[4]] & PTEF_FUNCMASK) | (res_val? PTEF_TEXTONLY : 0);

						GetAttr(GA_Selected, OBJ(OID_T_SHOWBLOCK), &res_val);
						//pt->pt_Flags = (pt->pt_Flags & ~(PTF_MARKSUM | PTF_MARKAVERAGE)) | (imsg.Code*PTF_MARKSUM);
						if(res_val) pt->pt_Flags |= res_val * PTF_MARKSUM;

						if( (pt->pt_Flags&PTF_AUTOCENTURY)!=(prefs.pr_Table->pt_Flags&PTF_AUTOCENTURY) && (pt->pt_Flags&PTF_AUTOCENTURY) )
						{
							century = TRUE;
						}

						calcflags &= ~(CF_REQUESTER | CF_ZERONAMES);
						//calcflags |= GetCheckBoxFlag(GadgetAddress(win,1),win,CF_REQUESTER) | GetCheckBoxFlag(GadgetAddress(win,2),win,CF_ZERONAMES);
						GetAttr(GA_Selected, OBJ(OID_T_OPENERRREQ), &res_val);
						if(res_val) calcflags |= CF_REQUESTER;
						GetAttr(GA_Selected, OBJ(OID_T_ZEROUNKNOWN), &res_val);
						if(res_val) calcflags |= CF_ZERONAMES;

						CopyMem(pt, prefs.pr_Table, sizeof(struct PrefTable));

						CloseAppWindow(win, TRUE);
						SetPrefsModule(&prefs, WDT_PREFTABLE, TRUE);
						PropagatePrefsModule(&prefs, WDT_PREFTABLE);

						if(century) RefreshMapTexts(NULL, FALSE);
						else RecalcMap(NULL);
					}
					break;
					case OID_CANCEL:
DBUG("\tTABLE OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod(..
DBUG("TABLE HandleTablePrefs() END\n\n");
}


void ASM handlePressKeyIDCMP(REG(a0, struct TagItem *tag))
{
    struct AppKey *ak = wd->wd_Data;
    struct Window *swin;
    struct winData *swd;
    struct Node *res_n = NULL;
    struct TagItem ti[4];

    switch(imsg.Class)
    {
        case IDCMP_RAWKEY:
            if (imsg.Code > 95 && imsg.Code < 105)   /* Qualifier */
                break;
        case IDCMP_VANILLAKEY:
            /*bug("code: %ld - qualifier: %ld\n",imsg.Code,imsg.Qualifier);*/
            if ((swin = GetAppWindow(WDT_PREFKEYS)) != 0)
            {
                swd = (struct winData *)swin->UserData;
                //GT_SetGadgetAttrs((swd = (struct winData *)swin->UserData)->wd_ExtData[0],swin,NULL,GTLV_Labels,~0L,TAG_END);
            }
            ak->ak_Class = imsg.Class;
            ak->ak_Code = imsg.Code;
            ak->ak_Qualifier = imsg.Qualifier;
            SetAppKeyName(ak);
            if (swin)
            {
                sortList(swd->wd_ExtData[2]);

                GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_K_LISTKEYS), (uint32 *)&res_n);
DBUG("\tKEYS IDCMP_VANILLAKEY: n=0x%08lx '%s' '%s'\n",res_n,ak->ak_Node.ln_Name,ak->ak_AppCmd);
                // Edit node, and update list(browser)
                ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 0;
                ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
                ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)ak->ak_Node.ln_Name;//ak->ak_AppCmd;
                ti[3].ti_Tag = TAG_DONE;
                DoGadgetMethod(GAD(OID_K_LISTKEYS), swin, NULL, LBM_EDITNODE, NULL, res_n, ti);
                // Sort new entry and refresh the gadget imagery
                DoGadgetMethod(GAD(OID_K_LISTKEYS), swin, NULL,
                               LBM_SORT, NULL, 0, LBMSORT_FORWARD, NULL);
                RefreshGadgets(GAD(OID_K_LISTKEYS), swin, NULL);
                //GT_SetGadgetAttrs(swd->wd_ExtData[0],swin,NULL,GTLV_Labels,swd->wd_ExtData[2],GTLV_Selected,FindListEntry(swd->wd_ExtData[2],(struct MinNode *)ak),TAG_END);
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
	//GTD_RemoveWindow(win); // gtdrag

	if(clean)
	{
		FreeAppKeys(wd->wd_ExtData[2]);
		FreePooled( pool, wd->wd_ExtData[2], sizeof(struct List) );

		if(isRAwinobjEmpty() == TRUE)
		{
			FreeLBColumnInfo(ra_popup_ci); // PopUp
			ra_popup_ci = NULL;
		}
		FreeListBrowserList( (struct List *)wd->wd_ExtData[4] );    // PopUp
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[4]); // PopUp

		FreeLBColumnInfo( (struct ColumnInfo *)wd->wd_ExtData[3] );
		FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[0]);
	}
}

void ASM
HandleKeyboardPrefsIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct AppKey *ak = wd->wd_ExtData[1];
	struct Node *res_n = NULL;
	STRPTR res_string;

	while( (wresult=IDoMethod(winobj[WDT_PREFKEYS], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("KEYS wresult=0x%08lx (wcode=0x%08lx) ak=0x%08lx\n", wresult,wcode,ak);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("KEYS WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("KEYS WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("KEYS WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_K_LISTKEYS:
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_K_LISTKEYS), (uint32 *)&res_n);
DBUG("\tKEYS OID_K_LISTKEYS: n=0x%08lx\n",res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ak,  TAG_DONE); // list [NATIVE] selected item
DBUG("\t'%s' '%s' %ld (0x%08lx)\n",ak->ak_Node.ln_Name,ak->ak_AppCmd,ak->ak_Node.ln_Type,ak);
						wd->wd_ExtData[1] = ak;
						RefreshSetGadgetAttrs(GAD(OID_K_DEL), win, NULL, GA_Disabled,FALSE, TAG_DONE);
						SetAttrs(OBJ(OID_K_RECORD), GA_Disabled,FALSE, TAG_DONE);
						SetAttrs(OBJ(OID_K_POPUPBTN), GA_Disabled,FALSE, TAG_DONE);
						SetAttrs(OBJ(OID_K_KEYLABEL), GA_Disabled,FALSE, CHOOSER_Selected,ak->ak_Node.ln_Type, TAG_DONE);
						RefreshGadgets(GAD(OID_K_KBGROUP), win, NULL);
					break;
					case OID_K_NEW:
DBUG("\tTABLE OID_K_NEW\n");
						if( (ak=AllocPooled(pool,sizeof(struct AppKey))) != 0 )
						{
							ak->ak_Node.ln_Name = AllocString("## n.def.");
							wd->wd_ExtData[1] = ak;
							MyAddTail(wd->wd_ExtData[2], ak);
							sortList(wd->wd_ExtData[2]);
DBUG("\tFindListEntry(): %ld   ak=0x%08lx\n",FindListEntry(wd->wd_ExtData[2],(struct MinNode *)ak),ak);
							res_n = AllocListBrowserNode(2,
							                             LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,ak->ak_Node.ln_Name,
							                             LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,NULL,
							                             LBNA_UserData, ak, // AppKey list [NATIVE] new item
							                            TAG_DONE);
							if(res_n)
							{// Detach the list(browser)
								SetAttrs(OBJ(OID_K_LISTKEYS), LISTBROWSER_Labels,NULL, TAG_DONE);
								// Add node to list(browser)
								AddTail(wd->wd_ExtData[0], res_n);
								// Re-attach the list(browser)
								SetAttrs(GAD(OID_K_LISTKEYS), LISTBROWSER_Labels,wd->wd_ExtData[0],
								         LISTBROWSER_SelectedNode,res_n, TAG_DONE);
								// Sort new entry and refresh the gadget imagery
								DoGadgetMethod(GAD(OID_K_LISTKEYS), win, NULL, LBM_SORT, NULL, 0, LBMSORT_FORWARD, NULL);
								RefreshSetGadgetAttrs(GAD(OID_K_LISTKEYS), win, NULL,
								                      LISTBROWSER_MakeNodeVisible,res_n, TAG_DONE);
							}
						}
						else
						{
							wd->wd_ExtData[1] = NULL;
						}

						RefreshSetGadgetAttrs(GAD(OID_K_DEL), win, NULL, GA_Disabled,!ak, TAG_DONE);
						SetAttrs(OBJ(OID_K_RECORD), GA_Disabled,!ak, TAG_DONE);
						SetAttrs(OBJ(OID_K_POPUPBTN), GA_Disabled,!ak, TAG_DONE);
						SetAttrs(OBJ(OID_K_KEYLABEL), GA_Disabled,!ak, CHOOSER_Selected,ak? ak->ak_Node.ln_Type : 0, TAG_DONE);
						RefreshGadgets(GAD(OID_K_KBGROUP), win, NULL);
					break;
					case OID_K_DEL:
DBUG("\tTABLE OID_K_DEL\n");
						if(ak)
						{
							GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_K_LISTKEYS), (uint32 *)&res_n);
DBUG("\tKEYS OID_K_DEL: n=0x%08lx\n",res_n);
							MyRemove(ak);
							FreeString(ak->ak_Node.ln_Name);
							FreeString(ak->ak_AppCmd);
							FreePooled(pool, ak, sizeof(struct AppKey) );
							wd->wd_ExtData[1] = NULL;
							RefreshSetGadgetAttrs(GAD(OID_K_DEL), win, NULL, GA_Disabled,TRUE, TAG_DONE);
							SetAttrs(OBJ(OID_K_RECORD), GA_Disabled,TRUE, TAG_DONE);
							SetAttrs(OBJ(OID_K_POPUPBTN), GA_Disabled,TRUE, TAG_DONE);
							SetAttrs(OBJ(OID_K_KEYLABEL), GA_Disabled,TRUE, CHOOSER_Selected,0, TAG_DONE);
							RefreshGadgets(GAD(OID_K_KBGROUP), win, NULL);
							// Remove list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_K_LISTKEYS), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshGadgets(GAD(OID_K_LISTKEYS), win, NULL);
						}
					break;
					case OID_K_RECORD:
DBUG("\tTABLE OID_K_RECORD\n");
						if(ak)
						{
							OpenAppWindow(WDT_PRESSKEY, WA_Data,ak, TAG_DONE);
						}
					break;
					case OID_K_POPUPBTN:
						if(ak)
						{
							struct Mappe *mp = GetPrefsMap( GetLocalPrefs(wd->wd_Data) );
							struct MinList *list;
							uint32 i = 0;
DBUG("\tKEYS OID_K_POPUPBTN: mp=0x%08lx (wcode=%ld)\n",mp,wcode);
							list = mp? &mp->mp_AppCmds : &prefs.pr_AppCmds;
							i = PopUpRAList(ra_popup_ci, (struct List *)wd->wd_ExtData[4],
							                POPA_MaxItems, 5,
							                POPA_Selected, GetListNumberOfName(list,ak->ak_AppCmd,0,1&&mp),//mode,!mode && mp),
							                POPA_Width, (GAD(OID_K_RECORD))->Width,
							               TAG_DONE);
DBUG("\ti=0x%08lx\n",i);
							if(i != 0)
							{
								struct Node *ln = (struct Node *)i;
								struct TagItem ti[4];

								if(mp)
								{
									ln = ((struct Link *)ln)->l_Link;
								}
								FreeString(ak->ak_AppCmd);
								ak->ak_AppCmd = AllocString(ln->ln_Name);
								GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_K_LISTKEYS), (uint32 *)&res_n);
DBUG("\tKEYS OID_K_POPUPBTN: '%s' (n=0x%08lx)\n",ln->ln_Name,res_n);
								// Edit node, and update list(browser)
								ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 1;
								ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
								ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)ln->ln_Name;
								ti[3].ti_Tag = TAG_DONE;
								DoGadgetMethod(GAD(OID_K_LISTKEYS), win, NULL, LBM_EDITNODE, NULL, res_n, ti);
								//RefreshGadgets(GAD(OID_K_LISTKEYS), win, NULL);
							}
						}
					break;
					case OID_K_KEYLABEL:
DBUG("\tKEYS OID_K_KEYLABEL\n");
						if(ak)
						{
							ak->ak_Node.ln_Type = wcode; // wcode = selected (sKeyLabels) item index
						}
					break;
					case OID_OK:
DBUG("\tKEYS OID_OK\n");
					{
						struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

						if( !IsListEmpty((struct List *)wd->wd_ExtData[2]) )
						{
							AddPrefsModuleToLocalPrefs(GetPrefsMap(pr), WDT_PREFKEYS);
						}
						SetPrefsModule(pr, WDT_PREFKEYS, TRUE);

						FreeAppKeys(&pr->pr_AppKeys);
						moveList(wd->wd_ExtData[2], &pr->pr_AppKeys);
					}
					//break;
					case OID_CANCEL:
DBUG("\tKEYS OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod(..
DBUG("KEYS HandleKeyboardPrefsIDCMP() END\n\n");
}


void ASM
closePrefIconWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

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
		// RA
		FreeLBColumnInfo( (struct ColumnInfo *)wd->wd_ExtData[5] );
		FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[0]);
		FreeLBColumnInfo( (struct ColumnInfo *)wd->wd_ExtData[7] );
		FreeListBrowserList( (struct List *)wd->wd_ExtData[1] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[1]);
		// RA
	}
}


//#warning "PrefIconAppCmdLock() dummy"
//void PUBLIC PrefIconAppCmdLock(REG(a0, struct LockNode *ln),REG(a1, struct MinNode *node),REG(d0, UBYTE flags)) { return; }

void updatePrefIconRALB(int32 ra_sel)
{
	struct Node *n;
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	struct IconObj *io;

	// Detach and clear list(browser)
	SetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_Labels,NULL, TAG_DONE);
	FreeListBrowserList( (struct List *)wd->wd_ExtData[1] );
	// (Re)create list(browser)
	foreach(&pr->pr_IconObjs, io)
	{
DBUG("\t[toolbar]'%s' 0x%08lx\n",io->io_Node.in_Name,io->io_Node.in_Image);
		n = AllocListBrowserNode(2,
		                         LBNA_Column,0, LBNCA_Image,io->io_Node.in_Image,
		                         LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,io->io_Node.in_Name,
		                         LBNA_UserData, io,
		                        TAG_DONE);
		if(n)
		{
			AddTail( (struct List *)wd->wd_ExtData[1], n );
//DBUG("\tnode=0x%08lx\n",n);
		}
	}
	// Re-attach the list(browser)
	SetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_Labels,wd->wd_ExtData[1],
	         LISTBROWSER_Selected,ra_sel, LISTBROWSER_MakeVisible,ra_sel, TAG_DONE);
	RefreshGadgets(GAD(OID_I_LISTTOOLBAR), win, NULL);
//	RefreshSetGadgetAttrs(GAD(OID_I_LISTTOOLBAR), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[1],
//	                      LISTBROWSER_Selected,ra_sel, TAG_DONE);
}

void ASM
handlePrefIconIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	struct IconObj *io;
	static BYTE tested;
	struct Node *res_n = NULL;

	while( (wresult=IDoMethod(winobj[WDT_PREFICON], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("ICON wresult=0x%08lx (wcode=0x%08lx) tested=0x%08lx\n", wresult,wcode,tested);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("ICON WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("ICON WMHI_CLOSEWINDOW\n");
						CloseAppWindow(win, TRUE);
						if(tested)
						{
							RefreshProjWindows(TRUE);
							tested = FALSE;
						}
			break;

			case WMHI_GADGETUP:
DBUG("ICON WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_I_LISTICONS:
DBUG("\tICON OID_I_LISTICONS:\n");
						/*GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_I_LISTICONS), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&io, TAG_DONE); // commands_with_icon list [NATIVE] item
DBUG("\t(io=0x%08lx) '%s'\n",io,io->io_Node.in_Name);*/
					break;
					case OID_I_LISTTOOLBAR:
DBUG("\tICON OID_I_LISTTOOLBAR:\n");
						/*GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_I_LISTTOOLBAR), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&io, TAG_DONE); // toolbar list [NATIVE] item
DBUG("\t(io=0x%08lx) '%s'\n",io,io->io_Node.in_Name);
						(GAD(OID_I_LISTTOOLBAR))->UserData = io;*/
					break;
					case OID_I_TO_TB:
DBUG("\tICON OID_I_TO_TB:\n");
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_I_LISTICONS), (uint32 *)&res_n);
DBUG("\t[ORIG] n=0x%08lx\n",res_n);
						if(res_n != 0)
						{
							struct AppCmd *ac = NULL;
							int32 total;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ac, TAG_DONE); // commands_with_icon list [NATIVE] item
DBUG("\t(ac=0x%08lx) '%s'\n",ac,ac->ac_Node.in_Name);
							GetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_SelectedNode,&res_n,
							         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\t[DEST] n=0x%08lx   res_val=%ld   total=%ld\n",res_n,res_val,total);
							if(res_n != 0)
							{
								if(res_val == total-1)
								{
									res_val = -1; // at bottom
								}
								else
								{
									res_val++; // add below selected item
									total = res_val;
								}
							}
							else
							{
								res_val = -1; // at bottom
							}

							AddIconObj(&pr->pr_IconObjs, (struct AppCmd *)ac, res_val);

							updatePrefIconRALB(total);
						}
					break;
					case OID_I_UP:
						GetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, TAG_DONE);
DBUG("\tICON OID_I_UP: n=0x%08lx %ld\n",res_n,res_val);
						if(res_n!=0 && res_val!=0)
						{
							struct IconObj *sel_io, *mov_io;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_io, TAG_DONE); // selected
							if(res_val != 1)
							{// Insert a node into a doubly linked list AFTER a given node...
								res_n = GetPred( GetPred(res_n) );
								//res_n = GetPred(res_n);
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&mov_io, TAG_DONE); // new position (previous)
							}
							MyRemove(sel_io);
							Insert( (struct List *)&pr->pr_IconObjs, (struct Node *)sel_io, res_val==1? 0:(struct Node *)mov_io );

							updatePrefIconRALB(--res_val);
						}
					break;
					case OID_I_DOWN:
					{
						int32 total;

						GetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tICON OID_I_DOWN: n=0x%08lx %ld (tot=%ld)\n",res_n,res_val,total);
						if(res_n!=0 && res_val!=total-1)
						{
							struct IconObj *sel_io, *mov_io;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_io, TAG_DONE); // selected
							GetListBrowserNodeAttrs(GetSucc(res_n), LBNA_UserData,&mov_io, TAG_DONE); // new position (next)
							MyRemove(sel_io);
							Insert( (struct List *)&pr->pr_IconObjs, (struct Node *)sel_io, (struct Node *)mov_io );

							updatePrefIconRALB(++res_val);
						}
					}
					break;
					case OID_I_SEPARATOR:
					{
						int32 total;
DBUG("\tICON OID_I_SEPARATOR:\n");
						GetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tres_n=0x%08lx   res_val=%ld   total=%ld\n",res_n,res_val,total);
						if(res_n != 0)
						{
							if(res_val == total-1)
							{
								res_val = -1; // at botttom
							}
							else
							{
								res_val++; // add below selected item
								total = res_val;
							}
						}
						else
						{
							res_val = -1; // at bottom
						}

						AddIconObj(&pr->pr_IconObjs, wd->wd_ExtData[3], res_val);

						updatePrefIconRALB(total);
					}
					break;
					case OID_I_DEL:
					{
						int32 total;
DBUG("\tICON OID_I_DEL:\n");// (io=0x%08lx):\n",(GAD(OID_I_LISTTOOLBAR))->UserData);
						//GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_I_LISTTOOLBAR), (uint32 *)&res_n);
						GetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tres_n=0x%08lx   res_val=%ld   total=%ld\n",res_n,res_val,total);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&io, TAG_DONE); // toolbar list [NATIVE] item
DBUG("\t(io=0x%08lx) '%s'\n",io,io->io_Node.in_Name);
							MyRemove(io);
							FreeString(io->io_AppCmd);
							FreePooled( pool, io, sizeof(struct IconObj) );
							if(res_val == total-1)
							{
								res_val--;
							}
							//(GAD(OID_I_LISTTOOLBAR))->UserData = NULL;
							// Remove toolbar's list(browser) node, free it and update display
							/*GetAttrs(OBJ(OID_I_LISTTOOLBAR), LISTBROWSER_Selected,&res_val, TAG_DONE);*/
							DoGadgetMethod(GAD(OID_I_LISTTOOLBAR), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshSetGadgetAttrs(GAD(OID_I_LISTTOOLBAR), win, NULL, LISTBROWSER_Selected,res_val, TAG_DONE);
						}
					}
					break;
					case OID_OK:
DBUG("\tICON OID_OK\n");
						if( !IsListEmpty((struct List *)wd->wd_ExtData[2]) )
						{
							struct Mappe *mp = wd->wd_Data;

							AddPrefsModuleToLocalPrefs(mp, WDT_PREFICON);
						}
						SetPrefsModule(pr, WDT_PREFICON, TRUE);

						FreeIconObjs(wd->wd_ExtData[2]);
						FreePooled( pool, wd->wd_ExtData[2], sizeof(struct MinList) );
						wd->wd_ExtData[2] = NULL;
						CloseAppWindow(win, TRUE);
						RefreshPrefsModule(pr, NULL, WDT_PREFICON);
					break;
					case OID_TEST:
DBUG("\tICON OID_TEST\n");
						RefreshProjWindows(TRUE);
						tested = TRUE;
					break;
					case OID_CANCEL:
DBUG("\tICON OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
						if(tested)
						{
							RefreshProjWindows(TRUE);
							tested = FALSE;
						}
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod(..
DBUG("ICON handlePrefIconIDCMP() END\n\n");
}


void ASM
closePrefCmdsWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;

	FreeLBColumnInfo( (struct ColumnInfo *)wd->wd_ExtData[1] );
	FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );
	FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[0]);
	//GTD_RemoveGadget(wd->wd_ExtData[0]);

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
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	struct Node *res_n = NULL;//, *sn;
	struct AppCmd *ac;
	//long   i, id;

	while( (wresult=IDoMethod(winobj[WDT_PREFCMDS], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("PREFCMDS wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("PREFCMDS WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("PREFCMDS WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("PREFCMDS WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_P_LISTCMDS:
DBUG("\tPREFCMDS OID_P_LISTCMDS (%ld):\n",wcode);
						GetAttrs(OBJ(OID_P_LISTCMDS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_RelEvent,&res_val, TAG_DONE);
DBUG("\tn=0x%08lx %ld\n",res_n,res_val);
						if(res_n)
						{
							RefreshSetGadgetAttrs(GAD(OID_P_DEL), win, NULL, GA_Disabled,FALSE, TAG_DONE);
							RefreshSetGadgetAttrs(GAD(OID_P_COPY), win, NULL, GA_Disabled,FALSE, TAG_DONE);
							if(res_val==LBRE_DOUBLECLICK)
							{
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ac, TAG_DONE); // command list [NATIVE] item
DBUG("\t(ac=0x%08lx) '%s'\n",ac,ac->ac_Node.in_Name);
								if(winobj[WDT_DEFINECMD] == NULL)
								{
									OpenAppWindow(WDT_DEFINECMD, WA_Data,ac, WA_Map,wd->wd_Data, TAG_DONE);
								}
							}
						}
					break;
					case OID_P_NEW:
DBUG("\tPREFCMDS OID_P_NEW (WDT_DEFINECMD=0x%08lx)\n",winobj[WDT_DEFINECMD]);
						if(winobj[WDT_DEFINECMD] == NULL)
						{// Only allow 1 WDT_DEFINECMD prefs window
							OpenAppWindow(WDT_DEFINECMD, WA_Map,wd->wd_Data, TAG_DONE);
						}
					break;
					case OID_P_DEL:
DBUG("\tPREFCMDS OID_P_DEL (%ld):\n",wcode);
						GetAttrs(OBJ(OID_P_LISTCMDS), LISTBROWSER_SelectedNode,&res_n,  TAG_DONE);
DBUG("\tn=0x%08lx\n",res_n);
						if(res_n)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ac, TAG_DONE); // command list [NATIVE] item
DBUG("\tSEL: ac=0x%08lx '%s'\n",ac,ac->ac_Node.in_Name);
							RemoveFromLockedList(&pr->pr_AppCmds, (struct MinNode *)ac);
							RemoveAppCmdsIconObjs(pr, ac);
							FreeAppCmd(ac);

							// Remove command's list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_P_LISTCMDS), win, NULL, LBM_REMNODE, NULL, res_n);
							res_val = wcode;
							if(res_val != 0) res_val--;
							RefreshSetGadgetAttrs(GAD(OID_P_LISTCMDS), win, NULL, LISTBROWSER_Selected,res_val, TAG_DONE);
	
							if( IsListEmpty((struct List *)&pr->pr_AppCmds) )
							{
								RefreshSetGadgetAttrs(GAD(OID_P_DEL), win, NULL, GA_Disabled,TRUE, TAG_DONE);
								RefreshSetGadgetAttrs(GAD(OID_P_COPY), win, NULL, GA_Disabled,TRUE, TAG_DONE);
								//RefreshSetGadgetAttrs(GAD(OID_P_GROUP_BTN), win, NULL);
							}
						}
					break;
					case OID_P_COPY:
DBUG("\tPREFCMDS OID_P_COPY (%ld):\n",wcode); // wcode = index in list(browser)
						GetAttrs(OBJ(OID_P_LISTCMDS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
DBUG("\tn=0x%08lx\n",res_n);
						if(res_n)
						{
							struct Node *n = NULL;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&ac, TAG_DONE); // command list [NATIVE] item
DBUG("\tSEL: ac=0x%08lx '%s'\n",ac,ac->ac_Node.in_Name);
							if( (n=(struct Node *)NewAppCmd(ac)) != 0 )
							{
								struct TagItem ti[7];

								MakeUniqueName(&pr->pr_AppCmds, &n->ln_Name);
								if(!n->ln_Name)
								{
									n->ln_Name = AllocString( GetString(&gLocaleInfo,MSG_UNNAMED_IN_PARENTHESIS) );
								}
								AddLockedTail(&pr->pr_AppCmds, (struct MinNode *)n);
DBUG("\tCOPY: ac=0x%08lx '%s'\n",n,n->ln_Name);
								// Add node and update list(browser)
								ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 0;
								ti[1].ti_Tag = LBNCA_Image;    ti[1].ti_Data = (uint32)ac->ac_Node.in_Image;
								ti[2].ti_Tag = LBNA_Column;    ti[2].ti_Data = 1;
								ti[3].ti_Tag = LBNCA_CopyText; ti[3].ti_Data = TRUE;
								ti[4].ti_Tag = LBNCA_Text;     ti[4].ti_Data = (uint32)n->ln_Name;
								ti[5].ti_Tag = LBNA_UserData;  ti[5].ti_Data = (uint32)(struct AppCmd *)n;
								ti[6].ti_Tag = TAG_DONE;
								DoGadgetMethod(GAD(OID_P_LISTCMDS), win, NULL, LBM_ADDNODE, NULL, res_n, ti);
								RefreshSetGadgetAttrs(GAD(OID_P_LISTCMDS), win, NULL, LISTBROWSER_Selected,wcode, TAG_DONE);
							}
						}
					break;

					case OID_OK:
DBUG("\tPREFCMDS OID_OK\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod(..
DBUG("ICON handlePrefCmdsIDCMP() END\n\n");
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
	{
		FreeAppCmd(wd->u.definecmd.wd_AppCmd);
		// RA
		struct Node *nn, *n = GetHead( (struct List *)wd->wd_ExtData[7] );
		while(n)
		{
DBUG("closeDefineCmdWin(): Free'ing chooser (n=0x%08lx) from list (0x%08lx)\n", n,wd->wd_ExtData[7]);
			nn = GetSucc(n);
			FreeChooserNode(n);
			n = nn;
		}
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[7]);

		FreeListBrowserList( (struct List *)wd->wd_ExtData[6] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[6]);

		if(isRAwinobjEmpty() == TRUE)
		{
			FreeLBColumnInfo(ra_popup_ci); // PopUp
			ra_popup_ci = NULL;
		}
		FreeListBrowserList( (struct List *)wd->wd_ExtData[5] );    // PopUp
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[5]); // PopUp
		// RA
		}
}

void ASM
handleDefineCmdIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	STRPTR res_string;
	struct Node *res_n = NULL;
	struct AppCmd *ac = wd->u.definecmd.wd_AppCmd;
	struct Command *cmd = NULL;
	struct IntCmd *ic = NULL;
	long i;

	while( (wresult=IDoMethod(winobj[WDT_DEFINECMD], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("DEFINECMD wresult=0x%08lx (wcode=0x%08lx) ac=0x%08lx\n", wresult,wcode,ac);
DBUG("o:'%s' h:'%s' g:'%s'\n",ac->ac_Output,ac->ac_HelpText,ac->ac_Guide);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("DEFINECMD WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("DEFINECMD WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("DEFINECMD WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_D_LABEL:
DBUG("\tDEFINECMD OID_D_LABEL ('%s')\n",ac->ac_Node.in_Name);
					break;
//					case OID_D_ICONIMG:
//DBUG("\tDEFINECMD OID_D_ICONIMG\n");
//					break;
					case OID_D_ICONSEL:
DBUG("\tDEFINECMD OID_D_ICONSEL\n");
						if( IDoMethod(OBJ(OID_D_ICONSEL), GFILE_REQUEST, win) )
						{
							int32 pos;
							char txt[256];

							/*if(ac->ac_ImageName)
							{
								strcpy(txt, ac->ac_ImageName);
								*( PathPart(txt) ) = 0;
							}
							else
							{
								strcpy(txt, iconpath);
							}*/

							GetAttrs(OBJ(OID_D_ICONSEL), GETFILE_FullFile,&res_string, TAG_DONE);
DBUG("\t'%s' '%s'\n",iconpath,res_string);
							pos = SplitName(res_string, '/', txt, 0, 255);
DBUG("\tpos=%ld ('%s')\n",pos,(res_string+pos));
							ac->ac_ImageName = AllocString(res_string+pos);
							if( (ac->ac_Node.in_Image=LoadImage(ac->ac_ImageName)) != 0 )
							{
DBUG("\tim=0x%08lx\n",ac->ac_Node.in_Image);
								RefreshSetGadgetAttrs(GAD(OID_D_ICONIMG), win, NULL,
								                      BUTTON_RenderImage,ac->ac_Node.in_Image, TAG_DONE);
							}
						}
					break;
					case OID_D_LISTCMDS:
DBUG("\tDEFINECMD OID_D_LISTCMDS %ld:\n",wcode);
						GetAttrs(OBJ(OID_D_LISTCMDS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
DBUG("\tres_n=0x%08lx cmd=0x%08lx\n",res_n,cmd);
						if(res_n != 0)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cmd, TAG_DONE); // selected
DBUG("\tcmd=0x%08lx '%s'\n",cmd,cmd->cmd_Name);
							SetAttrs(OBJ(OID_D_COMMAND), STRINGA_TextVal,cmd->cmd_Name, TAG_DONE);
							RefreshSetGadgetAttrs(GAD(OID_D_DEL), win, NULL, GA_Disabled,FALSE, TAG_DONE);
							RefreshSetGadgetAttrs(GAD(OID_D_GROUP_CMD), win, NULL, GA_Disabled,FALSE, TAG_DONE);
						}
					break;
					case OID_D_NEW:
DBUG("\tDEFINECMD OID_D_NEW\n");
						RefreshSetGadgetAttrs(GAD(OID_D_DEL), win, NULL, GA_Disabled,FALSE, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_D_LISTCMDS), win, NULL, LISTBROWSER_Selected,-1, TAG_DONE);
						SetAttrs(OBJ(OID_D_COMMAND), STRINGA_TextVal,NULL, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_D_GROUP_CMD), win, NULL, GA_Disabled,FALSE, TAG_DONE);
					break;
					case OID_D_DEL:
					{
						int32 total;
DBUG("\tDEFINECMD OID_D_DEL:\n");
						GetAttrs(OBJ(OID_D_LISTCMDS), LISTBROWSER_SelectedNode,&res_n, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tn=0x%08lx %ld\n",res_n,total);
						if(res_n != 0)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cmd, TAG_DONE); // selected
DBUG("\tcmd=0x%08lx '%s'\n",cmd,cmd->cmd_Name);
							MyRemove(cmd);
							FreeString(cmd->cmd_Name);
							FreePooled( pool, cmd, sizeof(struct Command) );
							// Remove command's list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_D_LISTCMDS), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshGadgets(GAD(OID_D_LISTCMDS), win, NULL);

							SetAttrs(OBJ(OID_D_COMMAND), STRINGA_TextVal,NULL, TAG_DONE);
							if(total == 1)
							{
								RefreshSetGadgetAttrs(GAD(OID_D_DEL), win, NULL, GA_Disabled,TRUE, TAG_DONE);
								SetAttrs(OBJ(OID_D_GROUP_CMD), win, NULL, GA_Disabled,TRUE, TAG_DONE);
							}
							RefreshGadgets(GAD(OID_D_GROUP_CMD), win, NULL);
						}
					}
					break;
					case OID_D_COMMAND:
DBUG("\tDEFINECMD OID_D_COMMAND:\n");
						GetAttr(RADIOBUTTON_Selected, OBJ(OID_D_CMDTYPE), &res_val);
						GetAttrs(OBJ(OID_D_COMMAND), STRINGA_TextVal,&res_string, TAG_DONE);
						GetAttrs(OBJ(OID_D_LISTCMDS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
DBUG("\t'%s' %ld (0x%08lx)\n",res_string,res_val,res_n);
						if(res_n != 0)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cmd, TAG_DONE); // selected
							FreeString(cmd->cmd_Name);
							cmd->cmd_Name = AllocString(res_string);
						}
						else if( (cmd=AllocPooled(pool, sizeof(struct Command))) != 0 )
						{
							MyAddTail(&ac->ac_Cmds, cmd);
							cmd->cmd_Name = AllocString(res_string);
							//GetAttr(RADIOBUTTON_Selected, OBJ(OID_D_CMDTYPE), &res_val);
							cmd->cmd_Type = res_val;
						}
DBUG("\tcmd=0x%08lx '%s' %ld\n",cmd,cmd->cmd_Name,cmd->cmd_Type);
						if(cmd)// && checkDefinedCmd(cmd->cmd_Name) )
						{
							struct TagItem ti[5];

							//if(cmd->cmd_Type == CMDT_INTERN)
							if(res_val == CMDT_INTERN)
							{
								checkDefinedCmd(cmd->cmd_Name);
							}
							// Add/Edit node and update list(browser)
							ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 0;
							ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
							ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)cmd->cmd_Name;//res_string;
							ti[3].ti_Tag = LBNA_UserData;  ti[3].ti_Data = (uint32)cmd;
							ti[4].ti_Tag = TAG_DONE;
							if(res_n == NULL)
							{
								res_n = (struct Node *)DoGadgetMethod(GAD(OID_D_LISTCMDS), win, NULL, LBM_ADDNODE, NULL, ~0, ti);
							}
							else
							{
								DoGadgetMethod(GAD(OID_D_LISTCMDS), win, NULL, LBM_EDITNODE, NULL, res_n, ti);
							}
							RefreshSetGadgetAttrs(GAD(OID_D_LISTCMDS), win, NULL, LISTBROWSER_SelectedNode,res_n,
							                      LISTBROWSER_MakeNodeVisible,res_n, TAG_DONE);
						}
					break;
					case OID_D_POPUPBNT:
DBUG("\tDEFINECMD OID_D_POPUPBNT:\n");
						GetAttrs(OBJ(OID_D_LISTCMDS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						GetAttr(RADIOBUTTON_Selected, OBJ(OID_D_CMDTYPE), &res_val);
DBUG("\t%ld (0x%08lx)\n",res_val,res_n);
						if(res_n != 0)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cmd, TAG_DONE); // selected
DBUG("\tOLD:'%s' (0x%08lx)\n",cmd->cmd_Name,cmd);
						}
						if(res_val == CMDT_INTERN)
						{// CMDT_INTERN -> PopUp
							i = PopUpRAList(ra_popup_ci, (struct List *)wd->wd_ExtData[5],
							                POPA_MaxItems, 12,
							                POPA_HorizProp, 1,
							                POPA_Width, (GAD(OID_D_COMMAND))->Width,
							               TAG_DONE);
DBUG("\ti=0x%08lx (cmd)\n",i);
							if(i != 0)
							{
								//cmd = (wd->u.definecmd.wd_ListView)->UserData;
								//if( !(cmd=gad->UserData) && (cmd=AllocPooled(pool,sizeof(struct Command))) )
								if( !cmd && (cmd=AllocPooled(pool,sizeof(struct Command))) )
								{
									MyAddTail(&ac->ac_Cmds, cmd);
								}
								if(cmd)
								{
									FreeString(cmd->cmd_Name);
									cmd->cmd_Name = AllocString( ((struct Command *)i)->cmd_Name );
									//cmd->cmd_Name = ((struct Command *)i)->cmd_Name;
DBUG("\tNEW:'%s' (0x%08lx)\n",cmd->cmd_Name,cmd);
									SetAttrs(OBJ(OID_D_COMMAND), STRINGA_TextVal,cmd->cmd_Name, TAG_DONE);
								}
							}
						}
						else
						{// ASL requester
							char txt[256];
DBUG("\tASL requester\n");
							if(cmd && cmd->cmd_Name)
							{
DBUG("\tOLD:'%s' 0x%08lx (type=%ld)\n",cmd->cmd_Name,cmd,res_val);
								strcpy(txt, cmd->cmd_Name);
								*(PathPart(txt)) = 0;
							}
							else
							{
								txt[0] = 0;
							}

							if( AslRequestTags(fileReq, ASLFR_Window,win, ASLFR_InitialDrawer,txt,
							                   ASLFR_TitleText,res_val==CMDT_AREXX? GetString(&gLocaleInfo,MSG_LOAD_AREXX_SCRIPT_TITLE) : GetString(&gLocaleInfo,MSG_LOAD_COMMAND_TITLE),
							                  TAG_DONE) )
							{
								strcpy(txt, fileReq->fr_Drawer);
								AddPart(txt, fileReq->fr_File, 255);
								if( !cmd && (cmd=AllocPooled(pool,sizeof(struct Command))) )
								{
DBUG("\tMyAddTail()\n");
									cmd->cmd_Type = res_val;
									MyAddTail(&ac->ac_Cmds, cmd);
								}
DBUG("\tNEW:'%s' (0x%08lx)\n",txt,cmd);
								FreeString(cmd->cmd_Name);
								cmd->cmd_Name = AllocString(txt);
								SetAttrs(OBJ(OID_D_COMMAND), STRINGA_TextVal,txt, TAG_DONE);
							}
						}
						RefreshGadgets(GAD(OID_D_COMMAND), win, NULL);
						if(cmd)
						{
							struct TagItem ti[5];
							// Add/Edit node and update list(browser)
							ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 0;
							ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
							ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)cmd->cmd_Name;
							ti[3].ti_Tag = LBNA_UserData;  ti[3].ti_Data = (uint32)cmd;
							ti[4].ti_Tag = TAG_DONE;
							if(res_n == NULL)
							{
								res_n = (struct Node *)DoGadgetMethod(GAD(OID_D_LISTCMDS), win, NULL, LBM_ADDNODE, NULL, ~0, ti);
							}
							else
							{
								DoGadgetMethod(GAD(OID_D_LISTCMDS), win, NULL, LBM_EDITNODE, NULL, res_n, ti);
							}
							RefreshSetGadgetAttrs(GAD(OID_D_LISTCMDS), win, NULL, LISTBROWSER_SelectedNode,res_n,
							                      LISTBROWSER_MakeNodeVisible,res_n, TAG_DONE);
						}
					break;
					case OID_D_CMDTYPE:
DBUG("\tDEFINECMD OID_D_CMDTYPE %ld\n",wcode);
						SetAttrs(OBJ(OID_D_POPUPBNT), BUTTON_AutoButton,wcode? BAG_POPFILE:BAG_POPUP, TAG_DONE);
						RefreshGadgets(GAD(OID_D_POPUPBNT), win, NULL);
					break;
					case OID_D_OUTPUT:
DBUG("\tDEFINECMD OID_D_OUTPUT ('%s')\n",ac->ac_Output);
//						GetAttrs(OBJ(OID_D_OUTPUT), STRINGA_TextVal,&res_string, TAG_DONE);
//FreeString(ac->ac_Output);
//						ac->ac_Output = AllocString(res_string);
					break;
					case OID_D_OUTPUTSEL:
DBUG("\tDEFINECMD OID_D_OUTPUTSEL %ld\n",wcode);
							GetAttrs(OBJ(OID_D_OUTPUTSEL), CHOOSER_SelectedNode,&res_n, TAG_DONE);
							GetChooserNodeAttrs(res_n, CNA_Text,&res_string, TAG_DONE); // selected
							RefreshSetGadgetAttrs(GAD(OID_D_OUTPUT), win, NULL, STRINGA_TextVal,res_string, TAG_DONE);
DBUG("\tres_string='%s'\n",res_string);
					break;
					case OID_D_HELPTEXT:
DBUG("\tDEFINECMD OID_D_HELPTEXT ('%s')\n",ac->ac_HelpText);
//						GetAttrs(OBJ(OID_D_HELPTEXT), STRINGA_TextVal,&res_string, TAG_DONE);
//FreeString(ac->ac_HelpText);
//						ac->ac_HelpText = AllocString(res_string);
					break;
					case OID_D_HYPERTEXT:
DBUG("\tDEFINECMD OID_D_HYPERTEXT ('%s')\n",ac->ac_Guide);
//						GetAttrs(OBJ(OID_D_HYPERTEXT), STRINGA_TextVal,&res_string, TAG_DONE);
//FreeString(ac->ac_Guide);
//						ac->ac_Guide = AllocString(res_string);
					break;

					case OID_OK:
					{
DBUG("\tDEFINECMD OID_OK (ac=0x%08lx)\n",ac);
						struct Prefs *pr = GetLocalPrefs(wd->u.definecmd.wd_Map);
						struct Mappe *mp = GetPrefsMap(pr);
						struct IconObj *io;
						struct Node *n = NULL;

						AddPrefsModuleToLocalPrefs(mp, WDT_PREFCMDS);
						SetPrefsModule(pr, WDT_PREFCMDS, TRUE);

						GetAttrs(OBJ(OID_D_LABEL), STRINGA_TextVal,&res_string, TAG_DONE);
						cmd = (struct Command *)MyFindName(&pr->pr_AppCmds, res_string);
						if(!strlen(res_string) || cmd && !wd->wd_Data || cmd && wd->wd_Data!=cmd)
						{
							DisplayBeep(NULL);
							//ActivateGadget(GadgetAddress(win, 1), win, NULL);
							break;
						}

						if(wd->wd_Data)
						{
							MyRemove(wd->wd_Data);
							FreeAppCmd(wd->wd_Data);
							wd->wd_Data = NULL;
						}

DBUG("\tLABEL: '%s'\n",res_string);
						ac->ac_Node.in_Name = AllocString(res_string);
						GetAttrs(OBJ(OID_D_OUTPUT), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tOUTPUT: '%s'\n",res_string);
						ac->ac_Output = AllocString(res_string);
						GetAttrs(OBJ(OID_D_HELPTEXT), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tHELP: '%s'\n",res_string);
						ac->ac_HelpText = AllocString(res_string);
						GetAttrs(OBJ(OID_D_HYPERTEXT), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tGUIDE: '%s'\n",res_string);
						ac->ac_Guide = AllocString(res_string);

						for(i=0,io=(APTR)pr->pr_IconObjs.mlh_Head; io->io_Node.in_Succ; io=(APTR)io->io_Node.in_Succ)
						{
//DBUG("\ti=%ld (0) '%s' '%s'\n",i,io->io_AppCmd,ac->ac_Node.in_Name);
							if( !strcmp(io->io_AppCmd,ac->ac_Node.in_Name) )
							{
								i++;
								io->io_Node.in_Image = ac->ac_Node.in_Image;
//DBUG("\ti=%ld (1) 0x%08lx\n",i,io->io_Node.in_Image);
							}
						}
						wd->u.definecmd.wd_AppCmd = NULL;
						CloseAppWindow(win, TRUE);

						AddLockedTail(&pr->pr_AppCmds, (struct MinNode *)ac);
DBUG("\ti=%ld\n",i);
						if(i)
						{
							RefreshProjWindows(TRUE);
						}
						// Position in list of new entry/node
						res_val = FindListEntry(&prefs.pr_AppCmds, (struct MinNode *)ac);
DBUG("\tpos=%ld\n",res_val);
						wd = (struct winData *)(win->Parent)->UserData; // win->Parent: PREFCMDS prefs window
						// Detach items from list(browser), free'em and re(generate) list(browser)
						SetAttrs(OBJ(OID_P_LISTCMDS), LISTBROWSER_Labels,NULL, TAG_DONE);
						FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );
						foreach(&prefs.pr_AppCmds, ac)
						{
DBUG("\t'%s' (0x%08lx)\n",ac->ac_Node.in_Name,ac->ac_Node.in_Image);
							n = AllocListBrowserNode(2,
							                         LBNA_Column,0, LBNCA_Image,ac->ac_Node.in_Image,
							                         LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,ac->ac_Node.in_Name,
							                         LBNA_UserData, ac,
							                        TAG_DONE);
							if(n)
							{
								AddTail( (struct List *)wd->wd_ExtData[0], n );
//DBUG("\tnode=0x%08lx\n",n);
							}
						}
							// Re-attach the list(browser) and refresh PREFSCMD prefs window
						SetAttrs(OBJ(OID_P_LISTCMDS), LISTBROWSER_Labels,wd->wd_ExtData[0],
						         LISTBROWSER_Selected,res_val, LISTBROWSER_MakeVisible,res_val, TAG_DONE);
						RefreshGadgets(GAD(OID_P_LISTCMDS), win->Parent, NULL);
					}
					break;
					case OID_CANCEL:
DBUG("\tDEFINECMD OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod(..
DBUG("DEFINECMD handleDefineCmdIDCMP() END\n\n");
}


void ASM
closePrefNamesWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	struct Name *nm;
DBUG("closePrefNamesWin()\n");
	if (clean)
	{
		while ((nm = (struct Name *)MyRemHead(wd->wd_ExtData[2])) != 0)
			FreeName(nm);

		FreePooled(pool,wd->wd_ExtData[2],sizeof(struct List));
		// RA
		FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );    // allocated in support.c:CopyRALBListItems()
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[0]); // allocated in support.c:CopyRALBListItems()
		// RA
	}
}

void
UpdateNamesGadgets(struct Name *nm, BOOL activate)
{
	//(GAD(OID_N_LISTNAMES))->UserData = nm;
	//((struct Gadget *)wd->wd_ExtData[0])->UserData = nm;
DBUG("UpdateNamesGadgets(0x%08lx):\n",nm);
	if(nm != NULL)
	{
DBUG("\t'%s' '%s' %ld\n",nm->nm_Node.ln_Name,nm->nm_Content,nm->nm_Node.ln_Type&NMT_TYPEMASK);
		SetAttrs(OBJ(OID_N_COPY), GA_Disabled,FALSE, TAG_DONE);
		SetAttrs(OBJ(OID_N_DELETE), GA_Disabled,FALSE, TAG_DONE);
		SetAttrs(OBJ(OID_N_COL2GROUP), GA_Disabled,FALSE, TAG_DONE);
		SetAttrs(OBJ(OID_N_NAME), STRINGA_TextVal,nm->nm_Node.ln_Name, TAG_DONE);
		SetAttrs(OBJ(OID_N_CONTENTS), STRINGA_TextVal,nm->nm_Content, TAG_DONE);
		SetAttrs(OBJ(OID_N_TYPE), CHOOSER_Selected,nm->nm_Node.ln_Type&NMT_TYPEMASK, TAG_DONE);
		SetAttrs(OBJ(OID_N_REFERENCE), CHOOSER_Title,nm->nm_Page? nm->nm_Page->pg_Node.ln_Name : "-", GA_Disabled,wd->wd_Data? FALSE : TRUE, TAG_DONE);
DBUG("\t'%s' %ld\n",nm->nm_Page? nm->nm_Page->pg_Node.ln_Name : "-",wd->wd_Data? FALSE : TRUE);
		/*if(activate)
		{
			//ActivateGadget(GAD(OID_N_NAME), win, NULL);
			//ActivateLayoutGadget( GAD(OID_N_COL2GROUP), win, NULL, (uint32)OBJ(OID_N_NAME) );
			IDoMethod( winobj[WDT_PREFNAMES], WM_ACTIVATEGADGET, GAD(OID_N_NAME) );
		}*/
	}
	else
	{
		struct Mappe *mp = GetRealMap(wd->wd_Data);
DBUG("\tmp=0x%08lx\n",mp);
		SetAttrs(OBJ(OID_N_COPY), GA_Disabled,TRUE, TAG_DONE);
		SetAttrs(OBJ(OID_N_DELETE), GA_Disabled,TRUE, TAG_DONE);
		SetAttrs(OBJ(OID_N_COL2GROUP), GA_Disabled,TRUE, TAG_DONE);
		SetAttrs(OBJ(OID_N_NAME), STRINGA_TextVal,NULL, TAG_DONE);
		SetAttrs(OBJ(OID_N_CONTENTS), STRINGA_TextVal,NULL, TAG_DONE);
		SetAttrs(OBJ(OID_N_TYPE), CHOOSER_Selected,0, TAG_DONE);
		if(mp)
		{
			SetAttrs(OBJ(OID_N_REFERENCE), CHOOSER_Title,nm->nm_Page? nm->nm_Page->pg_Node.ln_Name : "-", TAG_DONE);
		}
	}

	RefreshGadgets(GAD(OID_N_LNBUTTONS), win, NULL);
	RefreshGadgets(GAD(OID_N_COL2GROUP), win, NULL);
}

void updatePrefNamesRALB(struct Name *nm)
{
	//struct Name *nm = (GAD(OID_N_LISTNAMES))->UserData;
	struct Node *n = NULL;

	n = AllocListBrowserNode(1,
	                         LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,nm->nm_Node.ln_Name,
	                         LBNA_UserData, nm, // list [NATIVE] new item
	                        TAG_DONE);
	if(n)
	{
		AddTail(wd->wd_ExtData[0], n);
	}
}

void ASM handlePrefNamesIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct Mappe *mp = GetRealMap(wd->wd_Data);
	struct Name *nm = NULL;//(GAD(OID_N_LISTNAMES))->UserData;
	struct Node *res_n = NULL;
	STRPTR res_string;

	while( (wresult=IDoMethod(winobj[WDT_PREFNAMES], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("NAMES wresult=0x%08lx (wcode=0x%08lx) nm=0x%08lx\n", wresult,wcode,nm);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("NAMES WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("NAMES WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("NAMES WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_N_LISTNAMES:
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_N_LISTNAMES), (uint32 *)&res_n);
DBUG("\tNAMES OID_N_LISTNAMES: n=0x%08lx\n",res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&nm, TAG_DONE); // list [NATIVE] selected item
DBUG("\t'%s' '%s' %ld (0x%08lx)\n",nm->nm_Node.ln_Name,nm->nm_Content,nm->nm_Node.ln_Type,nm);
						UpdateNamesGadgets(nm, TRUE);
					break;
					case OID_N_NEW:
DBUG("\tNAMES OID_N_NEW:\n");
						if( (nm=AddName(wd->wd_ExtData[2], GetString(&gLocaleInfo,MSG_NEW_NAME), NULL, NMT_NONE|NMT_DETACHED, mp? (struct Page *)mp->mp_Pages.mlh_Head : NULL)) != 0 )
						{
DBUG("\t[NEW] nm=0x%08lx\n",nm);
							// Detach the list(browser)
							SetAttrs(OBJ(OID_N_LISTNAMES), LISTBROWSER_Labels,NULL, TAG_DONE);
							updatePrefNamesRALB(nm);
							res_val = FindListEntry(wd->wd_ExtData[2], (struct MinNode *)nm);
							// Re-attach the list(browser)
//							RefreshSetGadgetAttrs(GAD(OID_N_LISTNAMES), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
//							                      LISTBROWSER_Selected,FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm), TAG_DONE);
							SetAttrs(OBJ(OID_N_LISTNAMES), LISTBROWSER_Labels,wd->wd_ExtData[0],
							         LISTBROWSER_Selected,res_val, LISTBROWSER_MakeVisible,res_val, TAG_DONE);
							RefreshGadgets(GAD(OID_N_LISTNAMES), win, NULL);
//DBUG("\tFindListEntry()=%ld\n",FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm));
							UpdateNamesGadgets(nm, TRUE);
						}
					break;
					case OID_N_COPY:
DBUG("\tNAMES OID_N_COPY:\n");
						//GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_N_LISTNAMES), (uint32 *)&res_n);
						GetAttrs(OBJ(OID_N_LISTNAMES), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_TotalNodes,&res_val, TAG_DONE);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&nm, TAG_DONE); // list [NATIVE] selected item (to duplicate)
DBUG("\t[ORIG] nm=0x%08lx\n",nm);
						if( (nm=AddName(wd->wd_ExtData[2], nm->nm_Node.ln_Name, nm->nm_Content, nm->nm_Node.ln_Type, nm->nm_Page)) != 0 )
						{
DBUG("\t[COPY] nm=0x%08lx\n",nm);
							// Detach the list(browser)
							SetAttrs(OBJ(OID_N_LISTNAMES), LISTBROWSER_Labels,NULL, TAG_DONE);
							updatePrefNamesRALB(nm);
//							res_n = GetTail( (struct List *)wd->wd_ExtData[0] );
//DBUG("\tres_n=0x%08lx (new)\n",res_n);
							// Re-attach the list(browser)
							SetAttrs(OBJ(OID_N_LISTNAMES), LISTBROWSER_Labels,wd->wd_ExtData[0],
							         LISTBROWSER_Selected,res_val, LISTBROWSER_MakeVisible,res_val, TAG_DONE);
							RefreshGadgets(GAD(OID_N_LISTNAMES), win, NULL);
//							RefreshSetGadgetAttrs(GAD(OID_N_LISTNAMES), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
//							                      LISTBROWSER_SelectedNode,res_n, LISTBROWSER_Position,LBP_BOTTOM, TAG_DONE);
//							RefreshSetGadgetAttrs(GAD(OID_N_LISTNAMES), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
//							                      LISTBROWSER_Selected,FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm), TAG_DONE);
//DBUG("\tFindListEntry()=%ld\n",FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm));
							UpdateNamesGadgets(nm, TRUE);
						}
					break;
					case OID_N_DELETE:
DBUG("\tNAMES OID_N_DELETE\n");
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_N_LISTNAMES), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&nm, TAG_DONE); // list [NATIVE] selected item
						MyRemove(nm);
						FreeName(nm);
						// Remove list(browser) node, free it and update display
						DoGadgetMethod(GAD(OID_N_LISTNAMES), win, NULL, LBM_REMNODE, NULL, res_n);
						RefreshGadgets(GAD(OID_N_LISTNAMES), win, NULL);

						UpdateNamesGadgets(NULL, FALSE);
					break;
					case OID_N_NAME:
DBUG("\tNAMES OID_N_NAME:\n");
						GetAttrs(OBJ(OID_N_NAME), STRINGA_TextVal,&res_string, TAG_DONE);
						if( IsValidName(wd->wd_ExtData[2],res_string) )
						{
							struct Name *snm;

							GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_N_LISTNAMES), (uint32 *)&res_n);
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&nm, TAG_DONE); // list [NATIVE] selected item
DBUG("\tname:'%s' (was/old)\n",nm->nm_Node.ln_Name);
							FreeString(nm->nm_Node.ln_Name);
							nm->nm_Node.ln_Name = AllocString(res_string);
DBUG("\tname:'%s' (updated)\n",nm->nm_Node.ln_Name);
							sortList(wd->wd_ExtData[2]); // list [NATIVE]
							//sortLBList(wd->wd_ExtData[0]); // list(browser) [REACTION]

							// Detach and clear/free the list(browser)
							SetAttrs(OBJ(OID_N_LISTNAMES), LISTBROWSER_Labels,NULL, TAG_DONE);
							FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );
							foreach(wd->wd_ExtData[2], snm)
							{
								updatePrefNamesRALB(snm);
							}
							// Re-attach the list(browser)
							RefreshSetGadgetAttrs(GAD(OID_N_LISTNAMES), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
							                      LISTBROWSER_Selected,FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm), TAG_DONE);
DBUG("\tFindListEntry()=%ld\n",FindListEntry(wd->wd_ExtData[2],(struct MinNode *)nm));
						}
					break;
					case OID_N_CONTENTS:
DBUG("\tNAMES OID_N_CONTENTS:\n");
						GetAttrs(OBJ(OID_N_CONTENTS), STRINGA_TextVal,&res_string, TAG_DONE);
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_N_LISTNAMES), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&nm, TAG_DONE); // list [NATIVE] selected item
DBUG("\tcontent:'%s' (was/old)\n",nm->nm_Content);
						SetNameContent( nm, AllocString(res_string) );
DBUG("\tcontent:'%s' (updated)\n",nm->nm_Content);
					break;
					case OID_N_TYPE:
DBUG("\tNAMES OID_N_TYPE:\n");
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_N_LISTNAMES), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&nm, TAG_DONE); // list [NATIVE] selected item
DBUG("\ttype:%ld (was/old)\n",nm->nm_Node.ln_Type);
						nm->nm_Node.ln_Type = (UBYTE)wcode; // wcode -> chooser's index
						RefreshName(nm);
DBUG("\ttype:%ld (updated)\n",nm->nm_Node.ln_Type);
					break;
					case OID_N_REFERENCE:
DBUG("\tNAMES OID_N_REFERENCE\n");
					break;
					case OID_OK://9
						{
DBUG("\tNAMES OID_OK\n");
							struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

							if( !IsListEmpty((struct List *)wd->wd_ExtData[2]) )
							{
								AddPrefsModuleToLocalPrefs(mp, WDT_PREFNAMES);
							}

							SetPrefsModule(pr, WDT_PREFNAMES, TRUE);

							if( LockList(&pr->pr_Names,LNF_REFRESH) )
							{
								DetachNameList(&pr->pr_Names);
								swapLists(wd->wd_ExtData[2], &pr->pr_Names);
								AttachNameList(&pr->pr_Names);

								UnlockList(&pr->pr_Names, LNF_REFRESH);
							}

							CloseAppWindow(win, TRUE);

							RefreshPrefsModule(pr, NULL, WDT_PREFNAMES);
						}
					break;
					case OID_CANCEL://11
DBUG("\tNAMES OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("NAMES handlePrefNamesIDCMP() END\n\n");
}


void ASM
CloseFormatPrefsWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;

	if (!clean)
		return;

	FreeSysObject(ASOT_LIST, wd->wd_ExtData[2]); // chooser's DATE list/items
	FreeSysObject(ASOT_LIST, wd->wd_ExtData[3]); // chooser's TIME list/items
	//FreeStringList(wd->wd_ExtData[2]);  // fvt_zeit
	//FreeStringList(wd->wd_ExtData[3]);	// fvt_datum

	if (wd->wd_ExtData[5])
	{
		struct Node *ln;
		while ((ln = MyRemHead(wd->wd_ExtData[5])) != 0)
			FreeFormat((struct FormatVorlage *)ln);

		FreePooled(pool, wd->wd_ExtData[5], sizeof(struct List));
	}

	FreeSysObject(ASOT_HOOK, (struct Hook *)wd->wd_ExtData[0]);
	FreeLBColumnInfo( (struct ColumnInfo *)wd->wd_ExtData[6] );
	FreeListBrowserList( (struct List *)wd->wd_ExtData[7] );
	FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[7]);

	if(isRAwinobjEmpty() == TRUE)
	{
		FreeLBColumnInfo(ra_popup_ci); // PopUp
		ra_popup_ci = NULL;
	}
	FreeListBrowserList( (struct List *)wd->wd_ExtData[1] );    // PopUp
	FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[1]); // PopUp
}


void
makeFormatPreview(struct FormatVorlage *my_fv, struct Node *my_n)
{
	struct TagItem ti[6];
	double value = 0.0;
DBUG("makeFormatPreview()\n");
	FreeString(my_fv->fv_Preview);
	switch(my_fv->fv_Node.ln_Type)
	{
		case FVT_DATE:
			value = FORMAT_DATE_PREVIEW;
		break;
		case FVT_TIME:
			value = FORMAT_TIME_PREVIEW;
		break;
	}
	my_fv->fv_Preview = AllocString( FitValueInFormat(value,(struct Node *)my_fv,NULL,0,0) );
/*
	SetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_Labels,NULL, TAG_DONE);
	SetListBrowserNodeAttrs(my_n,
	                        LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,my_fv->fv_Node.ln_Name,
	                        LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,my_fv->fv_Preview,
	                       TAG_DONE);
	RefreshSetGadgetAttrs(GAD(OID_F_LISTFORMATS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[7],
	                      LISTBROWSER_SelectedNode,my_n, LISTBROWSER_AutoFit,TRUE, TAG_DONE);
*/
	// Edit node, and update list(browser)
	ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 0;
	ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
	ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)my_fv->fv_Node.ln_Name;
	ti[3].ti_Tag = LBNA_Column;    ti[3].ti_Data = 1;
	ti[4].ti_Tag = LBNCA_CopyText; ti[4].ti_Data = TRUE;
	ti[5].ti_Tag = LBNCA_Text;     ti[5].ti_Data = (uint32)my_fv->fv_Preview;
	ti[6].ti_Tag = TAG_DONE;
	DoGadgetMethod(GAD(OID_F_LISTFORMATS), win, NULL, LBM_EDITNODE, NULL, my_n, ti);
}

void
UpdatePrefFormatGadgets(struct FormatVorlage *fv)
{
//wd->wd_ExtData[4] = fv;
DBUG("UpdatePrefFormatGadgets(0x%08lx)\n",fv);
	if(fv)
	{
		SetAttrs(OBJ(OID_F_TYPE), GA_Disabled,FALSE, CHOOSER_Selected,fv->fv_Node.ln_Type-1, TAG_DONE);
		SetAttrs(OBJ(OID_F_PRIORITY), GA_Disabled,FALSE, SLIDER_Level,fv->fv_Node.ln_Pri, TAG_DONE);
		SetAttrs(OBJ(OID_F_FORMAT), GA_Disabled,FALSE, STRINGA_TextVal,fv->fv_Node.ln_Name, TAG_DONE);
//DBUG("\tOID_F_TYPE=%ld\n",fv->fv_Node.ln_Type);
		SetAttrs(OBJ(OID_F_FMT_POPBTN), CHOOSER_Labels,NULL, TAG_DONE);
		SetAttrs(OBJ(OID_F_DECIMALS), GA_Disabled,FALSE, SLIDER_Level,fv->fv_Komma, TAG_DONE);
		if(fv->fv_Node.ln_Type == FVT_DATE)
		{
			SetAttrs(OBJ(OID_F_FMT_POPBTN), GA_Disabled,FALSE, CHOOSER_Labels,wd->wd_ExtData[2], TAG_DONE);
			SetAttrs(OBJ(OID_F_DECIMALS), GA_Disabled,TRUE, TAG_DONE);
		}
		else if(fv->fv_Node.ln_Type == FVT_TIME)
		{
			SetAttrs(OBJ(OID_F_FMT_POPBTN), GA_Disabled,FALSE, CHOOSER_Labels,wd->wd_ExtData[3], TAG_DONE);
		}
		else
		{
			SetAttrs(OBJ(OID_F_FMT_POPBTN), GA_Disabled,TRUE, TAG_DONE);
		}
		SetAttrs(OBJ(OID_F_ALIGN), GA_Disabled,FALSE, CHOOSER_Selected,fv->fv_Alignment-1, TAG_DONE);

		SetAttrs(OBJ(OID_F_NEGCOLOR), GA_Disabled,FALSE, GA_Selected,fv->fv_NegativePen&FVF_NEGATIVEPEN, TAG_DONE);
		if( !(fv->fv_Flags&FVF_NEGATIVEPEN) )
		{
			SetAttrs(OBJ(OID_F_NEGCOL_POPBTN), GA_Disabled,TRUE, TAG_DONE);
		}
		SetAttrs(OBJ(OID_F_NEGVAL), GA_Disabled,FALSE, GA_Selected,fv->fv_NegativePen&FVF_NEGPARENTHESES, TAG_DONE);
		SetAttrs(OBJ(OID_F_NEGSEPARATOR), GA_Disabled,FALSE, GA_Selected,fv->fv_NegativePen&FVF_SEPARATE, TAG_DONE);
//ActivateGadget(gad,win,NULL);
	}
	else
	{
		SetAttrs(OBJ(OID_F_FORMAT), STRINGA_TextVal,NULL, TAG_DONE);

		SetAttrs(OBJ(OID_F_GROUP_PROP), GA_Disabled,TRUE, TAG_DONE);
		SetAttrs(OBJ(OID_F_GROUP_OPTS), GA_Disabled,TRUE, TAG_DONE);

		//RefreshSetGadgetAttrs(GAD(OID_F_LISTFORMATS), win, NULL, LISTBROWSER_Selected,-1, TAG_DONE);
		//RefreshSetGadgetAttrs(GAD(OID_F_COPY), win, NULL, GA_Disabled,TRUE, TAG_DONE);
		//RefreshSetGadgetAttrs(GAD(OID_F_DEL), win, NULL, GA_Disabled,TRUE, TAG_DONE);
	}

		RefreshGadgets(GAD(OID_F_GROUP_PROP), win, NULL);
		RefreshGadgets(GAD(OID_F_GROUP_OPTS), win, NULL);
}

void ASM
HandleFormatPrefsIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	struct FormatVorlage *fv = NULL;//wd->wd_ExtData[4];
	//double value = 0.0;
	//STRPTR t, s;
	//long   i;
	struct Node *res_n = NULL;
	STRPTR res_string;

	while( (wresult=IDoMethod(winobj[WDT_PREFFORMAT], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("FORMAT wresult=0x%08lx (wcode=0x%08lx) fv=0x%08lx\n", wresult,wcode,fv);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("FORMAT WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("FORMAT WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("FORMAT WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_F_LISTFORMATS:
DBUG("\tFORMAT OID_F_LISTFORMATS:\n");
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
DBUG("\t'%s' fmt:'%s' (0x%08lx)\n",fv->fv_Node.ln_Name,fv->fv_Preview,fv);
DBUG("\ttype:%ld   pri:%ld   comma:%ld   align:%ld\n",fv->fv_Node.ln_Type-1,fv->fv_Node.ln_Pri,fv->fv_Komma,fv->fv_Alignment);
DBUG("\tFlags (0x%08lx):\n",fv->fv_Flags);
DBUG( "FVF_NEGATIVEPEN=%ld(0x%08lx)   FVF_NEGPARENTHESES=%ld   FVF_SEPARATE=%ld\n",fv->fv_Flags&FVF_NEGATIVEPEN,fv->fv_NegativePen,fv->fv_Flags&FVF_NEGPARENTHESES,fv->fv_Flags&FVF_SEPARATE);
							UpdatePrefFormatGadgets(fv);

							RefreshSetGadgetAttrs(GAD(OID_F_COPY), win, NULL, GA_Disabled,FALSE, TAG_DONE);
							RefreshSetGadgetAttrs(GAD(OID_F_DEL), win, NULL, GA_Disabled,FALSE, TAG_DONE);
						}
					break;
					case OID_F_NEW:
DBUG("\tFORMAT OID_F_NEW\n");
						GetAttr(LISTBROWSER_TotalNodes, OBJ(OID_F_LISTFORMATS), &res_val);

						fv = AddFormat(wd->wd_ExtData[5], "0", 0, -1, -1, -1, ITA_NONE, FVT_VALUE);

						// Detach the list(browser)
						SetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_Labels,NULL, TAG_DONE);
						// (Re)create list(browser)
						prefformatsToRALB(wd->wd_ExtData[5], wd->wd_ExtData[7]);
						// Re-attach the list(browser)
						SetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_Labels,wd->wd_ExtData[7],
						         LISTBROWSER_Selected,res_val, LISTBROWSER_MakeVisible,res_val, TAG_DONE);
						RefreshGadgets(GAD(OID_F_LISTFORMATS), win, NULL);

						UpdatePrefFormatGadgets(fv);

						RefreshSetGadgetAttrs(GAD(OID_F_COPY), win, NULL, GA_Disabled,FALSE, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_F_DEL), win, NULL, GA_Disabled,FALSE, TAG_DONE);
					break;
					case OID_F_COPY:
DBUG("\tFORMAT OID_F_COPY:\n");
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_TotalNodes,&res_val, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
DBUG("\t'%s' fmt:'%s' (0x%08lx)\n",fv->fv_Node.ln_Name,fv->fv_Preview,fv);
							if( (fv=CopyFormat(fv)) != 0 )
							{
								MyAddTail(wd->wd_ExtData[5], fv);

								// Detach the list(browser)
								SetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_Labels,NULL, TAG_DONE);
								// (Re)create list(browser)
								prefformatsToRALB(wd->wd_ExtData[5], wd->wd_ExtData[7]);
								// Re-attach the list(browser)
								SetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_Labels,wd->wd_ExtData[7],
								         LISTBROWSER_Selected,res_val, LISTBROWSER_MakeVisible,res_val, TAG_DONE);
								RefreshGadgets(GAD(OID_F_LISTFORMATS), win, NULL);
							}
						}
					break;
					case OID_F_DEL:
DBUG("\tFORMAT OID_F_DEL\n");
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
							MyRemove(fv);
							FreeFormat(fv);
							// Remove format list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_F_LISTFORMATS), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshGadgets(GAD(OID_F_LISTFORMATS), win, NULL);

							UpdatePrefFormatGadgets(NULL);

							RefreshSetGadgetAttrs(GAD(OID_F_COPY), win, NULL, GA_Disabled,TRUE, TAG_DONE);
							RefreshSetGadgetAttrs(GAD(OID_F_DEL), win, NULL, GA_Disabled,TRUE, TAG_DONE);
						}
					break;
					case OID_F_TYPE:
DBUG("\tFORMAT OID_F_TYPE (%ld)\n",wcode);
						SetAttrs(OBJ(OID_F_DECIMALS), GA_Disabled,FALSE, TAG_DONE);
						if(wcode==2 || wcode==3) // DATE or TIME
						{
							SetAttrs(OBJ(OID_F_FMT_POPBTN), CHOOSER_Labels,NULL, TAG_DONE);
							SetAttrs(OBJ(OID_F_FMT_POPBTN), CHOOSER_Labels,wd->wd_ExtData[wcode], // wcode=index->DATE[2]|TIME[3]
							         GA_Disabled,FALSE, TAG_DONE);
							if(wcode == 2) // DATE
							{
								SetAttrs(OBJ(OID_F_DECIMALS), GA_Disabled,TRUE, TAG_DONE);
							}
						}
						else
						{
							SetAttrs(OBJ(OID_F_FMT_POPBTN), GA_Disabled,TRUE, TAG_DONE);
						}
						RefreshGadgets(GAD(OID_F_FMT_POPBTN), win, NULL);
						RefreshGadgets(GAD(OID_F_DECIMALS), win, NULL);
					break;
					case OID_F_PRIORITY:
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							struct FormatVorlage *sel_fv = NULL;
							//int32 sel;
DBUG("\tFORMAT OID_F_PRIORITY: %ld\n",(int16)wcode);
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
DBUG("\told:%ld new:%ld\n",fv->fv_Node.ln_Pri,(int16)wcode);
							fv->fv_Node.ln_Pri = (int16)wcode;
							SortFormatList(wd->wd_ExtData[5]);

							// Get new position of selected item
							sel_fv = (APTR)((struct List *)wd->wd_ExtData[5])->lh_Head;
							for(res_val=0; sel_fv!=fv; res_val++,sel_fv=(APTR)sel_fv->fv_Node.ln_Succ);
DBUG("\t[fv]0x%08lx == 0x%08lx[sel_fv] (%ld)\n",fv,sel_fv,res_val);

							// Detach the list(browser)
							SetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_Labels,NULL, TAG_DONE);
							prefformatsToRALB(wd->wd_ExtData[5], wd->wd_ExtData[7]);
							// Re-attach the list(browser)
							RefreshSetGadgetAttrs(GAD(OID_F_LISTFORMATS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[7],
							                      LISTBROWSER_Selected,res_val, TAG_DONE);
						}
					break;
					case OID_F_FORMAT:
DBUG("\tFORMAT OID_F_FORMAT:\n");
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
							GetAttrs(OBJ(OID_F_FORMAT), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\told:'%s' new:'%s' (0x%08lx)\n",fv->fv_Node.ln_Name,res_string,fv);
							FreeString(fv->fv_Node.ln_Name);
							fv->fv_Node.ln_Name = AllocString(res_string);

							makeFormatPreview(fv, res_n);
							RefreshSetGadgetAttrs(GAD(OID_F_LISTFORMATS), win, NULL, LISTBROWSER_AutoFit,TRUE, TAG_DONE);
						}
					break;
					case OID_F_FMT_POPBTN:
DBUG("\tFORMAT OID_F_FMT_POPBTN %ld\n",wcode);
						//GetAttrs(OBJ(OID_F_FMT_POPBTN), CHOOSER_Selected,&res_val, TAG_DONE);
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							char fmt[3];// = "XX";
							STRPTR t;
							struct Node *n = NULL;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item

							GetAttrs(OBJ(OID_F_FMT_POPBTN), CHOOSER_SelectedNode,&n, TAG_DONE);
							GetChooserNodeAttrs(n, CNA_Text,&res_string, TAG_DONE);
//DBUG("\tfull='%s'\n",res_string);
							fmt[0] = *(res_string);  // only want/need the 2 first..
							fmt[1] = *(res_string+1);// ..characters (format)
							fmt[2] = '\0';
DBUG("\tfmt='%s'\n",fmt);

							res_val = 3 + Strlen(fv->fv_Node.ln_Name); // 3 = sizeof(fmt)
							if( (t=AllocPooled(pool,res_val)) !=0 )
							{
								//Strlcpy(t, res_val, fv->fv_Node.ln_Name);
								//Strlcat(t, res_val, fmt);
								SNPrintf(t, res_val, "%s%s",fv->fv_Node.ln_Name,fmt);
DBUG("\told:'%s' new='%s' (%ld)\n",fv->fv_Node.ln_Name,t,res_val);
								FreeString(fv->fv_Node.ln_Name);
								fv->fv_Node.ln_Name = AllocString(t);
								RefreshSetGadgetAttrs(GAD(OID_F_FORMAT), win, NULL, STRINGA_TextVal,fv->fv_Node.ln_Name, TAG_DONE);
								FreePooled(pool, t, res_val);

								makeFormatPreview(fv, res_n);
								RefreshSetGadgetAttrs(GAD(OID_F_LISTFORMATS), win, NULL, LISTBROWSER_AutoFit,TRUE, TAG_DONE);
							}
						}
					break;
					case OID_F_DECIMALS:
DBUG("\tFORMAT OID_F_DECIMALS %ld\n",wcode);
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
							fv->fv_Komma = wcode;

							makeFormatPreview(fv, res_n);
						}
					break;
					case OID_F_ALIGN:
DBUG("\tFORMAT OID_F_ALIGN %ld\n",wcode);
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
							fv->fv_Alignment = wcode + 1;
						}
					break;
					case OID_F_NEGCOLOR:
DBUG("\tFORMAT OID_F_NEGCOLOR: %ld\n",wcode);
						RefreshSetGadgetAttrs(GAD(OID_F_NEGCOL_POPBTN), win, NULL, GA_Disabled,!wcode, TAG_DONE);
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
							fv->fv_Flags &= (~FVF_NEGATIVEPEN);
							fv->fv_Flags |= (wcode? FVF_NEGATIVEPEN : 0);
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
						}
					break;
					case OID_F_NEGCOL_POPBTN:
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							uint32 i = 0;
DBUG("\tFORMAT OID_F_NEGCOL_POPBTN (0x%08lx)\n",res_n);
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item

							res_val = max( TLn(GetString(&gLocaleInfo,MSG_NEG_VALUES_LABEL)), TLn(GetString(&gLocaleInfo,MSG_NEG_VALUES_UNDO)) );
							res_val = max(TLn(GetString(&gLocaleInfo,MSG_NUM_SEPARATOR_UNDO)), res_val);
							i = PopUpRAList(ra_popup_ci, (struct List *)wd->wd_ExtData[1l],
							                POPA_MaxItems, 5,
							                POPA_Width, res_val,//(GAD(OID_F_NEGCOLOR))->Width,
							                POPA_ItemHeight, 2, // "gap" between items
							               TAG_DONE);
DBUG("\ti=0x%08lx\n",i);
							if(i != 0)
							{
								struct colorPen *cp = (struct colorPen *)i;
DBUG("\t0x%08lx '%s' (0x%08lx)\n",cp,cp->cp_Node.ln_Name,cp->cp_Pen);
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
								fv->fv_Flags |= FVF_NEGATIVEPEN;
								fv->fv_NegativePen = cp->cp_ID;
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
								RefreshSetGadgetAttrs(GAD(OID_F_NEGCOL_SAMPLE), win, NULL, BUTTON_BackgroundPen,cp->cp_Pen, TAG_DONE);
							}
						}
					break;
					case OID_F_NEGVAL:
DBUG("\tFORMAT OID_F_NEGVAL: %ld\n",wcode);
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
							fv->fv_Flags &= (~FVF_NEGPARENTHESES);
							fv->fv_Flags |= (wcode? FVF_NEGPARENTHESES : 0);
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
						}
					break;
					case OID_F_NEGSEPARATOR:
DBUG("\tFORMAT OID_F_NEGSEPARATOR\n");
						GetAttrs(OBJ(OID_F_LISTFORMATS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n != NULL)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&fv, TAG_DONE); // format list [NATIVE] item
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
							fv->fv_Flags &= (~FVF_SEPARATE);
							fv->fv_Flags |= (wcode? FVF_SEPARATE : 0);
DBUG("\tfv->fv_Flags=0x%08lx\n",fv->fv_Flags);
						}
					break;
					case OID_OK://9
DBUG("\tFORMAT OID_OK\n");
					{
						struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
						struct Mappe *mp = wd->wd_Data;

						if( !IsListEmpty((struct List *)wd->wd_ExtData[5]) )
						{
							AddPrefsModuleToLocalPrefs(mp, WDT_PREFFORMAT);
						}
						SetPrefsModule(pr, WDT_PREFFORMAT, TRUE);

						SortFormatList(wd->wd_ExtData[5]);

						if( LockList(&pr->pr_Formats,LNF_REFRESH) )
						{
							while( (fv=(APTR)MyRemHead(&pr->pr_Formats)) != 0 )
							{
								FreeFormat(fv);
							}
							moveList(wd->wd_ExtData[5], &pr->pr_Formats);

							UnlockList(&pr->pr_Formats, LNF_REFRESH);
						}
						CloseAppWindow(win, TRUE);

						RefreshPrefsModule(pr, NULL, WDT_PREFFORMAT);
					}
					break;
					case OID_CANCEL://11
DBUG("\tFORMAT OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("FORMAT HandleFormatPrefsIDCMP() END\n\n");
}


void
setGradientColors(struct colorPen *cp, BOOL findname)
{
	struct ColorWheelRGB rgb;
	struct ColorWheelHSB hsb;
	short  *pens = (short *)wd->wd_ExtData[6],
	       numPens = (short)((long)wd->wd_ExtData[7]);
	long   i = 0;
DBUG("setGradientColors():\n");
	GetAttrs(OBJ(OID_P_COLOR), GETCOLOR_HSB,&hsb, TAG_DONE);
	while(i < numPens)
	{
		hsb.cw_Brightness = 0xffffffff - ((0xffffffff / numPens) * i);
		ConvertHSBToRGB(&hsb, &rgb);
		SetRGB32(&scr->ViewPort, pens[i], rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue);
		i++;
	}

	if(cp)
	{
		// update pen
		GetAttrs(OBJ(OID_P_COLOR), GETCOLOR_RGB,&rgb, TAG_DONE);
		cp->cp_Red   = rgb.cw_Red >> 24;
		cp->cp_Green = rgb.cw_Green >> 24;
		cp->cp_Blue  = rgb.cw_Blue >> 24;
		cp->cp_ID = (cp->cp_Red << 16) | (cp->cp_Green << 8) | cp->cp_Blue;
		if(cp->cp_Pen != -1)
		{
			SetRGB32(&scr->ViewPort, cp->cp_Pen, rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue);
		}

		// update name
		if (findname && cp->cp_Node.ln_Type)
		{
			struct TagItem ti[4];
			struct Node *res_n = NULL;
DBUG("\t'%s' (old)\n",cp->cp_Node.ln_Name);
			FindColorName(cp);
DBUG("\t'%s' (new)\n",cp->cp_Node.ln_Name);
			RefreshSetGadgetAttrs(GAD(OID_P_PAL_NAME), win, NULL, STRINGA_TextVal,cp->cp_Node.ln_Name, TAG_DONE);
			RefreshSetGadgetAttrs(GAD(OID_P_COLNAME), win, NULL, GA_Text,cp->cp_Node.ln_Name, TAG_DONE);
			GetAttrs(OBJ(OID_P_LISTCOLORS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
			// Edit node, and update list(browser)
			ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 2;
			ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
			ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)cp->cp_Node.ln_Name;
			ti[3].ti_Tag = TAG_DONE;
			DoGadgetMethod(GAD(OID_P_LISTCOLORS), win, NULL, LBM_EDITNODE, NULL, res_n, ti);
			//RefreshGadgets(GAD(OID_P_LISTCOLORS), win, NULL);
		}
	}
}

//#warning "showPrefColorsMode() dummy"
//void showPrefColorsMode(struct Window *win, long mode) { return; }

//#warning "remapColorPen() dummy"
//void remapColorPen(struct colorPen *cp) { return; }

void ASM
closePrefColorsWin(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	short  numPens = (short)((long)wd->wd_ExtData[7]),
	       *pens = (short *)wd->wd_ExtData[6];
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
		// RA
		FreeLBColumnInfo( (struct ColumnInfo *)wd->wd_ExtData[1] );
		FreeListBrowserList( (struct List *)wd->wd_ExtData[0] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[0]);

		FreeListBrowserList( (struct List *)wd->wd_ExtData[2] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[2]);
		// RA
	}
}

void refreshColorRAWin(struct Window *ra_win)
{
	char tmp_str[22];// = "WWW: 888 / 888 / 888"
	struct ColorWheelRGB rgb;
	struct ColorWheelHSB hsb;
	struct Node *res_n = NULL;
	struct colorPen *cp = NULL;

	GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_P_LISTCOLORS), (uint32 *)&res_n);
	if(res_n)
	{
DBUG("\trefreshColorRAWin(0x%08lx): n=0x%08lx\n",ra_win,res_n);
		GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cp, TAG_DONE); // list [NATIVE] selected color
DBUG("\trefreshColorRAWin(): R=%ld G=%ld B=%ld (0x%08lx)\n",cp->cp_Red,cp->cp_Green,cp->cp_Blue,cp);
		rgb.cw_Red   = RGB32(cp->cp_Red);
		rgb.cw_Green = RGB32(cp->cp_Green);
		rgb.cw_Blue  = RGB32(cp->cp_Blue);
		ConvertRGBToHSB(&rgb, &hsb);
DBUG("\trefreshColorRAWin(): H=%ld S=%ld B=%ld\n", (hsb.cw_Saturation>>24)? (hsb.cw_Hue>>24):0,hsb.cw_Saturation>>24,hsb.cw_Brightness>>24);

		SetAttrs(OBJ(OID_P_COLNAME), GA_Text,cp->cp_Node.ln_Name, TAG_DONE);
		SetAttrs(OBJ(OID_P_COLOR), GETCOLOR_RGB,rgb, TAG_DONE);
		sprintf(tmp_str, "%s: %03ld / %03ld / %03ld",GetString(&gLocaleInfo,MSG_COLOR_SPACE_RGB_GAD),cp->cp_Red,cp->cp_Green,cp->cp_Blue);
		SetAttrs(OBJ(OID_P_COLRGB), GA_Text,tmp_str, TAG_DONE);
		sprintf(tmp_str, "%s: %03ld / %03ld / %03ld",GetString(&gLocaleInfo,MSG_COLOR_SPACE_HSB_GAD),(hsb.cw_Saturation>>24)? (hsb.cw_Hue>>24):0,hsb.cw_Saturation>>24,hsb.cw_Brightness>>24);
		SetAttrs(OBJ(OID_P_COLHSB), GA_Text,tmp_str, TAG_DONE);
		RefreshSetGadgetAttrs(GAD(OID_P_COLGROUP), ra_win, NULL, GA_Disabled,FALSE, TAG_DONE);
		RefreshSetGadgetAttrs(GAD(OID_P_PAL_NAME), ra_win, NULL, STRINGA_TextVal,cp->cp_Node.ln_Name, TAG_DONE);
	}
	else
	{
		RefreshSetGadgetAttrs(GAD(OID_P_COLGROUP), ra_win, NULL, GA_Disabled,TRUE, TAG_DONE);
		RefreshSetGadgetAttrs(GAD(OID_P_PAL_NAME), ra_win, NULL, STRINGA_TextVal,NULL, TAG_DONE);
	}
}

void ASM
handlePrefColorsIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 res_val, wresult = WMHI_LASTMSG;
	STRPTR res_string;
	struct Node *res_n = NULL;
	struct colorPen *cp = NULL;

	while( (wresult=IDoMethod(winobj[WDT_PREFCOLORS], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("COLORS/PALETTE wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("COLORS/PALETTE WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("COLORS/PALETTE WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);

				if( (APTR)scr->UserData == iport )
				{
					LoadRGB32(&scr->ViewPort, standardPalette);
				}
				ReleaseAppColors(scr);
				ObtainAppColors(scr, TRUE);
				RefreshProjWindows(FALSE);
			break;

			case WMHI_GADGETUP:
DBUG("COLORS/PALETTE WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_P_PALETTE:
DBUG("\tCOLORS/PALETTE OID_P_PALETTE (%ld)\n",wcode);
						RefreshSetGadgetAttrs(GAD(OID_P_COLGROUP), win, NULL, GA_Disabled,wcode, TAG_DONE);
						SetAttrs(OBJ(OID_P_PAL_NEW), GA_Disabled,wcode, TAG_DONE);
						SetAttrs(OBJ(OID_P_PAL_COPY), GA_Disabled,wcode, TAG_DONE);
						SetAttrs(OBJ(OID_P_PAL_DEL), GA_Disabled,wcode, TAG_DONE);
						SetAttrs(OBJ(OID_P_PAL_NAME), GA_Disabled,wcode, TAG_DONE);
						RefreshGadgets(GAD(OID_P_PALGROUP), win, NULL);

						GetAttr(LISTBROWSER_Selected, OBJ(OID_P_LISTCOLORS), (uint32 *)&res_val);
DBUG("\tindex=%ld  wcode=%ld: 0x%08lx\n",res_val,wcode,wcode? wd->wd_ExtData[5]:wd->wd_ExtData[0]);
						if(wcode==1 && res_val>7)
						{// scrcolors choosed -> max. 8 colors/items
							res_val = 7;
						}
						// Detach the list(browser)
						SetAttrs(OBJ(OID_P_LISTCOLORS), LISTBROWSER_Labels,NULL, TAG_DONE);
						// Re-attach the list(browser) choosed
						RefreshSetGadgetAttrs(GAD(OID_P_LISTCOLORS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[2*wcode], // wcode=0|1 -> [0] or [2]
						                      LISTBROWSER_Selected,res_val, TAG_DONE);
						if(wcode == 0) 
						{
							refreshColorRAWin(win);
						}
						break;
					case OID_P_LISTCOLORS:
						GetAttr(CHOOSER_Selected, OBJ(OID_P_PALETTE), &res_val);
DBUG("\tCOLORS/PALETTE OID_P_LISTCOLORS (%ld)\n",res_val);
						if(res_val == 0)
						{
							refreshColorRAWin(win);
						}
					break;
					case OID_P_COLOR:
						GetAttrs(OBJ(OID_P_LISTCOLORS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);

						if( IDoMethod(OBJ(OID_P_COLOR), GCOLOR_REQUEST, win) )
						{
							struct TagItem ti[3];
//							struct ColorWheelRGB rgb;

//							GetAttrs(OBJ(OID_P_COLOR), GETCOLOR_RGB,&rgb, TAG_DONE);
							GetAttr(GETCOLOR_Color, OBJ(OID_P_COLOR), &res_val);
//DBUG("\tCOLORS/PALETTE OID_P_COLOR: R=%ld G=%ld B=%ld\n", rgb.cw_Red>>24,rgb.cw_Green>>24,rgb.cw_Blue>>24);
DBUG("\tCOLORS/PALETTE OID_P_COLOR: R=%ld G=%ld B=%ld (0x%08lx)\n",res_val>>16,(res_val&0xFF00)>>8,res_val&0xFF,res_val);

							GetListBrowserNodeAttrs(res_n,
							                        //LBNA_Column,0, LBNCA_Image,(uint32 *)&myimg,
							                        //LBNA_Column,1, LBNCA_Integer,(uint32 *)&cp, // list [NATIVE] selected color
							                        LBNA_UserData, &cp, // list [NATIVE] selected color
							                       TAG_DONE);
							//cp = (APTR)res_val;
DBUG("\tCOLORS/PALETTE OID_P_COLOR: R=0x%08lx G=0x%08lx B=0x%08lx (old)\n",cp->cp_Red,cp->cp_Green,cp->cp_Blue);
//							cp->cp_Red   = rgb.cw_Red>>24;
//							cp->cp_Green = rgb.cw_Green>>24;
//							cp->cp_Blue  = rgb.cw_Blue>>24;
							cp->cp_Red   = res_val >> 16;
							cp->cp_Green = (res_val & 0xFF00) >> 8;
							cp->cp_Blue  = res_val & 0xFF;
DBUG("\tCOLORS/PALETTE OID_P_COLOR: R=0x%08lx G=0x%08lx B=0x%08lx (new)\n",cp->cp_Red,cp->cp_Green,cp->cp_Blue);

DBUG("\tCOLORS/PALETTE OID_P_COLOR: cp->cp_Pen=0x%08lx (release)\n",cp->cp_Pen);
							ReleasePen(scr->ViewPort.ColorMap, cp->cp_Pen);
							cp->cp_Pen = ObtainPen(scr->ViewPort.ColorMap, -1, RGB32(cp->cp_Red), RGB32(cp->cp_Green), RGB32(cp->cp_Blue), 0);
DBUG("\tCOLORS/PALETTE OID_P_COLOR: cp->cp_Pen=0x%08lx (obtain)\n",cp->cp_Pen);

							Object *myimg = NewObject(BevelClass, NULL,
							                          IA_Width,  scr->Font->ta_YSize*2,
							                          IA_Height, scr->Font->ta_YSize,
							                          BEVEL_Style,   BVS_BOX,
							                          BEVEL_FillPen, cp->cp_Pen==-1? TAG_IGNORE : cp->cp_Pen,
							                         TAG_DONE);
							// Edit node, and update list(browser)
							ti[0].ti_Tag = LBNA_Column; ti[0].ti_Data = 0;
							ti[1].ti_Tag = LBNCA_Image; ti[1].ti_Data = (uint32)myimg;
							ti[2].ti_Tag = TAG_DONE;
							DoGadgetMethod(GAD(OID_P_LISTCOLORS), win, NULL, LBM_EDITNODE, NULL, res_n, ti);
							//RefreshGadgets(GAD(OID_P_LISTCOLORS), win, NULL);

							refreshColorRAWin(win);
							setGradientColors(cp, TRUE);
						}
					break;
					case OID_P_PAL_NEW:
					{
DBUG("\tCOLORS/PALETTE OID_P_PAL_NEW\n");
						//if(!cp)
						//{
							cp = AddColor(wd->wd_Data, NULL, 255, 255, 255); // White
							cp->cp_Pen = ObtainPen(scr->ViewPort.ColorMap, -1, ~0, ~0, ~0, 0);
DBUG("\tCOLORS/PALETTE OID_P_PAL_NEW: cp->cp_Pen=0x%08lx (obtain)\n",cp->cp_Pen);
							//cp->cp_Pen = ((short *)wd->wd_ExtData[6])[MAXGRADPENS+1];

							Object *myimg = NewObject(BevelClass, NULL,
							                          IA_Width,  scr->Font->ta_YSize*2,
							                          IA_Height, scr->Font->ta_YSize,
							                          BEVEL_Style,   BVS_BOX,
							                          BEVEL_FillPen, cp->cp_Pen==-1? TAG_IGNORE : cp->cp_Pen,
							                         TAG_DONE);
//DBUG("SET myimg=0x%08lx\n",myimg);
							if( (res_n=AllocListBrowserNode(2,
							                                LBNA_Column,0, LBNCA_Image,myimg,
							                                LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,cp->cp_Node.ln_Name,
							                                LBNA_UserData, cp, // color list [NATIVE] item
							                               TAG_DONE)) )
							{// Detach the list(browser)
								SetAttrs(OBJ(OID_P_LISTCOLORS), LISTBROWSER_Labels,NULL, TAG_DONE);
								// Add node to list(browser)
								AddTail( (struct List *)wd->wd_ExtData[0], res_n );
DBUG("\tCOLORS/PALETTE OID_P_PAL_NEW: node(0x%08lx) '%s'\n", res_n,cp->cp_Node.ln_Name);
								// Re-attach, refresh the list(browser) and select last/bottom entry
								RefreshSetGadgetAttrs(GAD(OID_P_LISTCOLORS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
								                      LISTBROWSER_SelectedNode,res_n, LISTBROWSER_Position,LBP_BOTTOM, TAG_DONE);
								refreshColorRAWin(win);
							}
						//}
					}
					break;
					case OID_P_PAL_COPY:
DBUG("\tCOLORS/PALETTE OID_P_PAL_COPY\n");
						GetAttrs(OBJ(OID_P_LISTCOLORS), LISTBROWSER_SelectedNode,&res_n, TAG_DONE);
						if(res_n)
						{
							Object *myimg = NULL;
DBUG("\tCOLORS/PALETTE OID_P_PAL_COPY: node=0x%08lx\n",res_n);
							GetListBrowserNodeAttrs(res_n,
							                        LBNA_Column,0, LBNCA_Image,&myimg,
							                        LBNA_UserData, &cp, // list [NATIVE] selected color
							                       TAG_DONE);
DBUG("\tCOLORS/PALETTE OID_P_PAL_COPY: cp=0x%08lx\n",cp);
							//if(cp)
							//{
								cp = AddColor(wd->wd_Data, cp->cp_Node.ln_Name, cp->cp_Red, cp->cp_Green, cp->cp_Blue);
								cp->cp_Node.ln_Type = 1;
								cp->cp_Pen = ObtainPen(scr->ViewPort.ColorMap, -1, RGB32(cp->cp_Red), RGB32(cp->cp_Green), RGB32(cp->cp_Blue), 0);
DBUG("\tCOLORS/PALETTE OID_P_PAL_NEW: cp->cp_Pen=0x%08lx (obtain)\n",cp->cp_Pen);
								//cp->cp_Pen = ((short *)wd->wd_ExtData[6])[MAXGRADPENS+1];

							/*Object *myimg = NewObject(BevelClass, NULL,
							                          IA_Width,  scr->Font->ta_YSize*2,
							                          IA_Height, scr->Font->ta_YSize,
							                          BEVEL_Style,   BVS_BOX,
							                          BEVEL_FillPen, cp->cp_Pen==-1? TAG_IGNORE : cp->cp_Pen,
							                         TAG_DONE);*/
//DBUG("SET myimg=0x%08lx\n",myimg);
								if( (res_n=AllocListBrowserNode(2,
								                                LBNA_Column,0, LBNCA_Image,myimg,
								                                LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,cp->cp_Node.ln_Name,
								                                LBNA_UserData, cp, // color list [NATIVE] item
								                               TAG_DONE)) )
								{// Detach the list(browser)
									SetAttrs(OBJ(OID_P_LISTCOLORS), LISTBROWSER_Labels,NULL, TAG_DONE);
									// Add node to list(browser)
									AddTail( (struct List *)wd->wd_ExtData[0], res_n );
DBUG("\tCOLORS/PALETTE OID_P_PAL_NEW: node(0x%08lx) '%s'\n", res_n,cp->cp_Node.ln_Name);
									// Re-attach, refresh the list(browser) and select last/bottom entry
									RefreshSetGadgetAttrs(GAD(OID_P_LISTCOLORS), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[0],
									                      LISTBROWSER_SelectedNode,res_n, LISTBROWSER_Position,LBP_BOTTOM, TAG_DONE);
									//refreshColorRAWin(win); // not needed, it's a copy of color/item selected
								}
							//}
						}
					break;
					case OID_P_PAL_DEL:
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_P_LISTCOLORS), (uint32 *)&res_n);
DBUG("\tCOLORS/PALETTE OID_P_PAL_DEL: node=0x%08lx\n",(uint32 *)res_n);
						if(res_n)
						{
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cp, TAG_DONE); // list [NATIVE] selected color
DBUG("\tCOLORS/PALETTE OID_P_PAL_DEL: cp=0x%08lx\n",cp);
							if(cp)
							{
								struct Node *n2 = NULL;

								MyRemove(cp);
								FreeString(cp->cp_Node.ln_Name);
								FreePooled( pool, cp, sizeof(struct colorPen) );

								// Get next(previous) node to mark on list(browser)
								if( !(n2=GetSucc(res_n)) )
								{
									n2 = GetPred(res_n);
								}
								// Remove node, free it and update list(browser)
								DoGadgetMethod(GAD(OID_P_LISTCOLORS), win, NULL, LBM_REMNODE, NULL, res_n);
								RefreshSetGadgetAttrs(GAD(OID_P_LISTCOLORS), win, NULL,
								                      LISTBROWSER_SelectedNode,n2, TAG_DONE);
								refreshColorRAWin(win);
							}
						}
					break;
					case OID_P_PAL_NAME:
					{
						struct TagItem ti[4];

						GetAttrs(OBJ(OID_P_PAL_NAME), STRINGA_TextVal,&res_string, TAG_DONE);
						//GetAttr(STRINGA_TextVal,OBJ(OID_P_PAL_NAME), (uint32 *)&res_string);
DBUG("\tCOLORS/PALETTE OID_P_PAL_NAME: '%s'\n",res_string);
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_P_LISTCOLORS), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cp, TAG_DONE); // list [NATIVE] selected color
						FreeString(cp->cp_Node.ln_Name);
						cp->cp_Node.ln_Name = AllocString(res_string);
						cp->cp_Node.ln_Type = (cp->cp_Node.ln_Name && cp->cp_Node.ln_Name[0])? 0 : 1;
						// determines if a color name should be automatically choosen
						if(cp->cp_Node.ln_Type == 1)
						{
							FindColorName(cp);
						}

						// Edit node, and update list(browser)
						ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 1;
						ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
						ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)cp->cp_Node.ln_Name;//res_string;
						ti[3].ti_Tag = TAG_DONE;
						DoGadgetMethod(GAD(OID_P_LISTCOLORS), win, NULL, LBM_EDITNODE, NULL, res_n, ti);
						//RefreshGadgets(GAD(OID_P_LISTCOLORS), win, NULL);
					}
					break;
					case OID_OK:
					{
						long col;
DBUG("COLORS/PALETTE OID_OK\n");
						SetPrefsModule(&prefs, WDT_PREFCOLORS, TRUE);
						//for(i=0, cp=(APTR)((struct List *)wd->wd_ExtData[5])->lh_Head;cp->cp_Node.ln_Succ; cp=(APTR)cp->cp_Node.ln_Succ, i++)
						for(res_val=0, cp=(APTR)((struct List *)wd->wd_ExtData[5])->lh_Head; cp->cp_Node.ln_Succ; cp=(APTR)cp->cp_Node.ln_Succ)
						{
							col = cp->cp_Red;
							res_val++;
							standardPalette[res_val] = col | (col << 8) | (col << 16) | (col << 24);
							col = cp->cp_Green;
							res_val++;
							standardPalette[res_val] = col | (col << 8) | (col << 16) | (col << 24);
							col = cp->cp_Blue;
							res_val++;
							standardPalette[res_val] = col | (col << 8) | (col << 16) | (col << 24);
						}
						swapLists(&colors, wd->wd_Data);
						UniqueColors(&colors);
						RefreshProjWindows(TRUE); //Übernahme der neuen Farben in den Fenstern
					}
					case OID_CANCEL:
DBUG("COLORS/PALETTE OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
						if( (APTR)scr->UserData == iport )
						{
							LoadRGB32(&scr->ViewPort, standardPalette);
						}
						ReleaseAppColors(scr);
						ObtainAppColors(scr, TRUE);
						RefreshProjWindows(FALSE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("COLORS/PALETTE handlePrefColorsIDCMP() END\n\n");
}


void ASM HandleSystemPrefsIDCMP(REG(a0, struct TagItem *tag))
{
	uint16 wcode = 0;
	uint32 wresult = WMHI_LASTMSG, res_val;
	long old;

	while( (wresult=IDoMethod(winobj[WDT_PREFSYS], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("SYSTEM wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("SYSTEM WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("SYSTEM WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("SYSTEM WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_CANCEL://11
DBUG("\tSYSTEM OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
					case OID_OK://9
DBUG("\tSYSTEM OID_OK\n");
						old = prefs.pr_Flags;
						GetAttr(GA_Selected, OBJ(OID_S_APPICON), &res_val);
						prefs.pr_Flags = res_val;//PRF_APPICON;
						GetAttr(GA_Selected, OBJ(OID_S_SESSIONS), &res_val);
						if(res_val == 1) prefs.pr_Flags |= PRF_USESESSION;
						GetAttr(GA_Selected, OBJ(OID_S_CHECKSEC), &res_val);
						if(res_val == 1) prefs.pr_Flags |= PRF_SECURESCRIPTS;
						GetAttr(GA_Selected, OBJ(OID_S_CTXTMENU), &res_val);
						if(res_val == 1) prefs.pr_Flags |= PRF_CONTEXTMENU;
						/*GetAttr(GA_Selected, OBJ(OID_S_), &res_val);
						if(res_val == 1) prefs.pr_Flags |= PRF_USEDBUFFER;*/
						GetAttr(GA_Selected, OBJ(OID_S_CLIMBLST), &res_val);
						if(res_val == 1) prefs.pr_Flags |= PRF_CLIPGO;
						GetAttr(CHOOSER_Selected, OBJ(OID_S_REFRESH), &res_val);
						if(res_val == 1)
						{
							prefs.pr_Flags |= PRF_SIMPLEWINDOWS;
						}
						if(res_val == 2)
						{
							prefs.pr_Flags |= PRF_SIMPLEWINDOWS | PRF_SIMPLEPROJS;
						}
						GetAttr(INTEGER_Number, OBJ(OID_S_CLIPUNIT), &res_val);
						clipunit = res_val;
DBUG("\tprefs.pr_Flags=0x%08lx   clipunit=%ld\n",prefs.pr_Flags,clipunit);
						/**************** Update AppIcon ********************/
						if(prefs.pr_Flags&PRF_APPICON)
						{
							InitAppIcon();
						}
						else
						{
							FreeAppIcon();
						}
						/**************** Update prefs module ***************/
						SetPrefsModule(&prefs, WDT_PREFSYS, TRUE);
						PropagatePrefsModule(&prefs, WDT_PREFSYS);
						CloseAppWindow(win, TRUE); // and close SYSTEM prefs window
						/**************** Update window type ****************/
						if( (old&(PRF_SIMPLEPROJS|PRF_SIMPLEWINDOWS)) != (prefs.pr_Flags&(PRF_SIMPLEPROJS|PRF_SIMPLEWINDOWS)) )
						{
							ChangeAppScreen(FALSE);
						}
						/**************** Update DoubleBuffer ***************/
						if( (prefs.pr_Flags&PRF_USEDBUFFER) && !doublerp )
						{
							InitDoubleBuffer();
							AllocDoubleBufferBitMap(scr);
						}
						else
						{
							if( !(prefs.pr_Flags&PRF_USEDBUFFER) && doublerp )
							{
								FreeDoubleBuffer();
							}
						}
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("SYSTEM HandleSystemPrefsIDCMP() END\n\n");
}

//#warning: "psysPages dummy"
//extern struct Gadget *psysPages[];


void
UpdateContextGadgets(struct ContextMenu *cm)
{
	//struct MinList *list = NULL;
	//struct Node *res_n = NULL;
	//uint32 res_val;

	//GetAttr(CHOOSER_Selected, OBJ(OID_C_CONTEXT), &res_val);
	//list = wd->wd_ExtData[res_val];
DBUG("UpdateContextGadgets(): cm=0x%08lx\n",cm);
	if(cm)
	{
		UBYTE dis = cm->cm_Node.ln_Name? !strcmp(cm->cm_Node.ln_Name,"-") : TRUE;
DBUG("UpdateContextGadgets(): dis=%ld\n",dis);
		//SetAttrs(OBJ(OID_C_LISTCONTEXT), LISTBROWSER_Labels,list, LISTBROWSER_Selected,FindListEntry(list,(struct MinNode *)cm), TAG_DONE);

		SetAttrs(OBJ(OID_C_UP), GA_Disabled,FALSE, TAG_DONE);
		SetAttrs(OBJ(OID_C_DOWN), GA_Disabled,FALSE, TAG_DONE);
		SetAttrs(OBJ(OID_C_DEL), GA_Disabled,FALSE, TAG_DONE);
		RefreshGadgets(GAD(OID_C_BTNGROUP), win, NULL);

		RefreshSetGadgetAttrs(GAD(OID_C_TITLE), win, NULL, STRINGA_TextVal,cm->cm_Node.ln_Name, GA_Disabled,FALSE, TAG_DONE);

		SetAttrs(OBJ(OID_C_COMMAND), STRINGA_TextVal,cm->cm_AppCmd, TAG_DONE);
		RefreshSetGadgetAttrs(GAD(OID_C_CMDGROUP), win, NULL, GA_Disabled,dis, TAG_DONE);
	}
	else
	{
		//SetAttrs(OBJ(OID_C_LISTCONTEXT), LISTBROWSER_Labels,list, LISTBROWSER_Selected,-1, TAG_DONE);

		SetAttrs(OBJ(OID_C_UP), GA_Disabled,TRUE, TAG_DONE);
		SetAttrs(OBJ(OID_C_DOWN), GA_Disabled,TRUE, TAG_DONE);
		SetAttrs(OBJ(OID_C_DEL), GA_Disabled,TRUE, TAG_DONE);
		RefreshGadgets(GAD(OID_C_BTNGROUP), win, NULL);

		RefreshSetGadgetAttrs(GAD(OID_C_TITLE), win, NULL, STRINGA_TextVal,NULL, GA_Disabled,TRUE, TAG_DONE);

		SetAttrs(OBJ(OID_C_COMMAND), STRINGA_TextVal,NULL, TAG_DONE);
		RefreshSetGadgetAttrs(GAD(OID_C_CMDGROUP), win, NULL, GA_Disabled,TRUE, TAG_DONE);
	}
	//RefreshGadgets(GAD(OID_C_LISTCONTEXT), win, NULL);
}


void ASM
closePrefContextWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;

	if (clean)
	{
		long i;

		for (i = 0; i < NUM_CMT; i++)
		{
			FreeContextMenu(wd->wd_ExtData[i]);
			FreePooled(pool, wd->wd_ExtData[i], sizeof(struct MinList));
		}
		// RA
		if(isRAwinobjEmpty() == TRUE)
		{
			FreeLBColumnInfo(ra_popup_ci); // PopUp
			ra_popup_ci = NULL;
		}
		FreeListBrowserList( (struct List *)wd->wd_ExtData[6] );    // PopUp
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[6]); // PopUp
		FreeListBrowserList( (struct List *)wd->wd_ExtData[7] );    // PopUp
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[7]); // PopUp

		FreeListBrowserList( (struct List *)wd->wd_ExtData[5] );
		FreeSysObject(ASOT_LIST, (struct List *)wd->wd_ExtData[5]);
		// RA
	}
}

void ASM
handlePrefContextIDCMP( REG(a0, struct TagItem *tag) )
{
	uint16 wcode = 0;
	uint32 wresult = WMHI_LASTMSG, res_val;
	STRPTR res_string;
	struct Node *res_n = NULL;
	struct ContextMenu *cm = NULL;// = wd->wd_ExtData[NUM_CMT];

	while( (wresult=IDoMethod(winobj[WDT_PREFCONTEXT], WM_HANDLEINPUT, &wcode)) != WMHI_LASTMSG )
	{
DBUG("CONTEXT wresult=0x%08lx (wcode=0x%08lx)\n", wresult,wcode);
		switch(wresult & WMHI_CLASSMASK)
		{
			case WMHI_MENUPICK:
DBUG("CONTEXT WMHI_MENUPICK (wcode=0x%08lx) (imsg.Code=0x%08lx)\n", wcode,imsg.Code);
				imsg.Code = wcode; // so HandleMenu() "knows" menu option
				HandleMenu();
			break;

			case WMHI_CLOSEWINDOW:
DBUG("CONTEXT WMHI_CLOSEWINDOW\n");
				CloseAppWindow(win, TRUE);
			break;

			case WMHI_GADGETUP:
DBUG("CONTEXT WMHI_GADGETUP (0x%08lx)\n", (wresult&WMHI_CLASSMASK));
				switch(wresult & WMHI_GADGETMASK)
				{
					case OID_C_CONTEXT:
DBUG("\tCONTEXT OID_C_CONTEXT (%ld)\n",wcode);
						// Detach the list(browser)
						SetAttrs(OBJ(OID_C_LISTCONTEXT), LISTBROWSER_Labels,NULL, TAG_DONE);
						// Copy selected contextmenu list to ReAction ListBrowser
						contextmenuToRALB( (struct MinList *)wd->wd_ExtData[wcode], (struct List *)wd->wd_ExtData[5] );
						// Re-attach and refresh the list(browser)
						RefreshSetGadgetAttrs(GAD(OID_C_LISTCONTEXT), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[5],
						                      LISTBROWSER_Selected,-1, TAG_DONE);

						UpdateContextGadgets(NULL);
					break;
					case OID_C_LISTCONTEXT:
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_C_LISTCONTEXT), (uint32 *)&res_n);
DBUG("\tCONTEXT OID_C_LISTCONTEXT n=0x%08lx\n",res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cm,  TAG_DONE); // context menu list [NATIVE] item
DBUG("\t'%s' ('%s'=0x%08lx) cm=0x%08lx\n",cm->cm_Node.ln_Name,cm->cm_AppCmd,cm->cm_AppCmd,cm);
						UpdateContextGadgets(cm);
					break;
					case OID_C_UP:
						GetAttrs(OBJ(OID_C_LISTCONTEXT), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, TAG_DONE);
DBUG("\tCONTEXT OID_C_UP: n=0x%08lx %ld\n",res_n,res_val);
						if(res_val != 0)
						{
							struct ContextMenu *sel_cm, *mov_cm;
							uint32 res_lst;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_cm, TAG_DONE); // selected
							if(res_val != 1)
							{// Insert a node into a doubly linked list AFTER a given node...
								res_n = GetPred( GetPred(res_n) );
								//res_n = GetPred(res_n);
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&mov_cm, TAG_DONE); // new position (previous)
							}

							GetAttr(CHOOSER_Selected, OBJ(OID_C_CONTEXT), &res_lst);
							SetAttrs(OBJ(OID_C_LISTCONTEXT), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_cm);
							Insert( (struct List *)wd->wd_ExtData[res_lst], (struct Node *)sel_cm, res_val==1? 0:(struct Node *)mov_cm );
							contextmenuToRALB( (struct MinList *)wd->wd_ExtData[res_lst], (struct List *)wd->wd_ExtData[5] );
							RefreshSetGadgetAttrs(GAD(OID_C_LISTCONTEXT), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[5],
							                      LISTBROWSER_Selected,--res_val, TAG_DONE);
						}
					break;
					case OID_C_DOWN:
					{
						int32 total;

						GetAttrs(OBJ(OID_C_LISTCONTEXT), LISTBROWSER_SelectedNode,&res_n,
						         LISTBROWSER_Selected,&res_val, LISTBROWSER_TotalNodes,&total, TAG_DONE);
DBUG("\tCONTEXT OID_C_DOWN: n=0x%08lx %ld (tot=%ld)\n",res_n,res_val,total);
						if(res_val != total-1)
						{
							struct ContextMenu *sel_cm, *mov_cm;
							uint32 res_lst;

							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&sel_cm, TAG_DONE); // selected
							GetListBrowserNodeAttrs(GetSucc(res_n), LBNA_UserData,&mov_cm, TAG_DONE); // new position (next)

							GetAttr(CHOOSER_Selected, OBJ(OID_C_CONTEXT), &res_lst);
							SetAttrs(OBJ(OID_C_LISTCONTEXT), LISTBROWSER_Labels,NULL, TAG_DONE);
							MyRemove(sel_cm);
							Insert( (struct List *)wd->wd_ExtData[res_lst], (struct Node *)sel_cm, (struct Node *)mov_cm );
							contextmenuToRALB( (struct MinList *)wd->wd_ExtData[res_lst], (struct List *)wd->wd_ExtData[5] );
							RefreshSetGadgetAttrs(GAD(OID_C_LISTCONTEXT), win, NULL, LISTBROWSER_Labels,wd->wd_ExtData[5],
							                      LISTBROWSER_Selected,++res_val, TAG_DONE);
						}
					}
					break;
					case OID_C_NEW:
DBUG("\tCONTEXT OID_C_NEW\n");
//prefcontext_new:
						UpdateContextGadgets(NULL);
						//ActivateGadget(GAD(OID_C_TITLE), win, NULL);
						IDoMethod( winobj[WDT_PREFCONTEXT], WM_ACTIVATEGADGET, GAD(OID_C_TITLE) );
						RefreshSetGadgetAttrs(GAD(OID_C_LISTCONTEXT), win, NULL, LISTBROWSER_Selected,-1, TAG_DONE);
						RefreshSetGadgetAttrs(GAD(OID_C_TITLE), win, NULL, GA_Disabled,FALSE, TAG_DONE);
					break;
					case OID_C_DEL:
							GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_C_LISTCONTEXT), (uint32 *)&res_n);
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cm, TAG_DONE); // context menu list [NATIVE] item
DBUG("\tCONTEXT OID_C_DEL: cm=0x%08lx '%s' '%s'\n",cm,cm->cm_Node.ln_Name,cm->cm_AppCmd);
							MyRemove(cm);
							FreeString(cm->cm_Node.ln_Name);
							FreeString(cm->cm_AppCmd);
							FreePooled( pool, cm, sizeof(struct ContextMenu) );
							// Remove list(browser) node, free it and update display
							DoGadgetMethod(GAD(OID_C_LISTCONTEXT), win, NULL, LBM_REMNODE, NULL, res_n);
							RefreshGadgets(GAD(OID_C_LISTCONTEXT), win, NULL);

							UpdateContextGadgets(NULL);
					break;
					case OID_C_TITLE:
						GetAttrs(OBJ(OID_C_TITLE), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tCONTEXT OID_C_TITLE: '%s'\n",res_string);
						if(*(res_string) != '\0')
						{
							struct TagItem ti[5];

							cm = NULL;

							GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_C_LISTCONTEXT), (uint32 *)&res_n);
DBUG("\tres_n=0x%08lx\n",res_n);
							if(res_n != NULL)
							{
								GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cm, TAG_DONE); // context menu list [NATIVE] item
DBUG("\tcm=0x%08lx old:'%s'   new:'%s'\n",cm,cm->cm_AppCmd,res_string);
							}

							if(cm)
							{
								FreeString(cm->cm_Node.ln_Name);
								cm->cm_Node.ln_Name = AllocString(res_string);
							}
							else
							{
								GetAttr(CHOOSER_Selected, OBJ(OID_C_CONTEXT), &res_val);
								cm = AddContextMenu(wd->wd_ExtData[res_val], res_string, NULL);
							}
DBUG("\tcm=0x%08lx\n",cm);
							// Add/Edit node and update list(browser)
							ti[0].ti_Tag = LBNA_Column;    ti[0].ti_Data = 0;
							ti[1].ti_Tag = LBNCA_CopyText; ti[1].ti_Data = TRUE;
							ti[2].ti_Tag = LBNCA_Text;     ti[2].ti_Data = (uint32)cm->cm_Node.ln_Name;//res_string;
							ti[3].ti_Tag = LBNA_UserData;  ti[3].ti_Data = (uint32)cm;
							ti[4].ti_Tag = TAG_DONE;
							if(res_n == NULL)
							{
								res_n = (struct Node *)DoGadgetMethod(GAD(OID_C_LISTCONTEXT), win, NULL, LBM_ADDNODE, NULL, ~0, ti);
							}
							else
							{
								DoGadgetMethod(GAD(OID_C_LISTCONTEXT), win, NULL, LBM_EDITNODE, NULL, res_n, ti);
							}
							RefreshSetGadgetAttrs(GAD(OID_C_LISTCONTEXT), win, NULL, LISTBROWSER_SelectedNode,res_n, TAG_DONE);

							//UpdateContextGadgets(cm);

							if(cm && *(cm->cm_Node.ln_Name)!='-')//strcmp(cm->cm_Node.ln_Name,"-") )
							{
								UpdateContextGadgets(cm);
								//ActivateGadget(GAD(OID_C_COMMAND), win, NULL);
								//ActivateLayoutGadget( GAD(OID_C_CMDGROUP), win, NULL, (uint32)OBJ(OID_C_COMMAND) );
								IDoMethod( winobj[WDT_PREFCONTEXT], WM_ACTIVATEGADGET, GAD(OID_C_COMMAND) );
							}
							else
							{
								//goto prefcontext_new;
								UpdateContextGadgets(NULL);
							}
						}
					break;
					case OID_C_COMMAND:
						GetAttrs(OBJ(OID_C_COMMAND), STRINGA_TextVal,&res_string, TAG_DONE);
DBUG("\tCONTEXT OID_C_COMMAND: '%s'\n",res_string);
						GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_C_LISTCONTEXT), (uint32 *)&res_n);
						GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cm, TAG_DONE); // context menu list [NATIVE] item
DBUG("\tCONTEXT OID_C_COMMAND: cm=0x%08lx old:'%s'   new:'%s'\n",cm,cm->cm_AppCmd,res_string);
						if(cm && *(res_string)!='\0')
						{
							FreeString(cm->cm_AppCmd);
							cm->cm_AppCmd = AllocString(res_string);
						}
						//goto prefcontext_new;
						UpdateContextGadgets(NULL);
					break;
					/*case OID_C_TYPE:
DBUG("\tCONTEXT OID_C_TYPE (%ld)\n",wcode);
					break;*/
					case OID_C_POPUPBTN:
					{
						struct Mappe *mp = GetPrefsMap( GetLocalPrefs(wd->wd_Data) );
						struct MinList *list;
						uint32 i = 0;

						GetAttr(CHOOSER_Selected, OBJ(OID_C_TYPE), &res_val);
DBUG("\tCONTEXT OID_C_POPUPBTN: %ld   mp=0x%08lx   wcode=%ld\n",res_val,mp,wcode);
						list = res_val==0? (mp? &mp->mp_AppCmds : &prefs.pr_AppCmds) : &intcmds;
						i = PopUpRAList(ra_popup_ci, (struct List *)wd->wd_ExtData[6+res_val],// list, // res_val=0|1
						                POPA_MaxItems, res_val==0? 5:15,
						                //res_val? TAG_IGNORE:POPA_MaxItems, 5,
						                POPA_HorizProp, res_val,
						                //POPA_Selected, GetListNumberOfName(list,cm->cm_AppCmd,res_val,!res_val&&mp),
						                POPA_Width, (GAD(OID_C_TITLE))->Width,
						               TAG_DONE);
DBUG("\tCONTEXT OID_C_POPUPBTN i=0x%08lx\n",i);
						if(i != 0)
						{
							struct Node *ln = (struct Node *)i;

							if(!res_val && mp)
							{
								ln = ((struct Link *)ln)->l_Link;
							}

							GetAttr(LISTBROWSER_SelectedNode, OBJ(OID_C_LISTCONTEXT), (uint32 *)&res_n);
							GetListBrowserNodeAttrs(res_n, LBNA_UserData,&cm, TAG_DONE); // context menu list [NATIVE] item
DBUG("\tcm=0x%08lx old:'%s'   new:'%s'  res_val=%ld\n",cm,cm->cm_AppCmd,ln->ln_Name,res_val);
							FreeString(cm->cm_AppCmd);
							cm->cm_AppCmd = AllocString(ln->ln_Name);
							RefreshSetGadgetAttrs(GAD(OID_C_COMMAND), win, NULL, STRINGA_TextVal,cm->cm_AppCmd, TAG_DONE);
						}

					}
					break;
					case OID_OK://9
					{
DBUG("\tCONTEXT OID_OK\n");
						struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
						struct Mappe *mp = wd->wd_Data;
						long   i;

						for(i=0; i<NUM_CMT; i++)
						{
							if( !IsListEmpty((struct List *)wd->wd_ExtData[i]) )
							{
								AddPrefsModuleToLocalPrefs(mp, WDT_PREFCONTEXT);
								break; // exit for(i=0...
							}
						}

						SetPrefsModule(pr, WDT_PREFCONTEXT, TRUE);

						for(i=0; i<NUM_CMT; i++)
						{
							swapLists(wd->wd_ExtData[i], &pr->pr_Contexts[i]);
						}

						RefreshPrefsModule(pr, NULL, WDT_PREFCONTEXT);
					}
					//break;
					case OID_CANCEL://11
DBUG("\tCONTEXT OID_CANCEL\n");
						CloseAppWindow(win, TRUE);
					break;
				} // END switch(wresult & WMHI_GADGETMASK)
			break;
		} // END switch(wresult & WMHI_CLASSMASK)

	} // END while( (wresult=IDoMethod..
DBUG("SYSTEM handlePrefContextIDCMP() END\n\n");
}
