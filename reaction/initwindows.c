/* Window initialization code
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include "reactionGUI.h"
#endif

extern struct wdtPrintStatus *asp_wps;


/************************** Misc **************************/


void ASM
InitFileTypeWindow(REG(a0, struct winData *wd))
{
	struct IOTypeLink *iol;
	struct IOType *io;
	struct Node *ln;

	if ((wd->wd_ExtData[1] = AllocPooled(pool, sizeof(struct MinList))) != 0) {
		MyNewList(wd->wd_ExtData[1]);
		foreach(&iotypes, io) {
			if (io->io_Flags & (wd->wd_ExtData[0] ? IOF_READABLE : IOF_WRITEABLE)
				&& (iol = AllocPooled(pool, sizeof(struct IOTypeLink))) != 0) {
				iol->iol_Node.ln_Name = io->io_Node.ln_Name;
				iol->iol_Link = io;
				MyNewList(&iol->iol_Description);

				foreach(&io->io_Description, ln)
					WordWrapText((struct List *)&iol->iol_Description, ln->ln_Name, scr->Width / 3 - 40);

				MyAddTail(wd->wd_ExtData[1], iol);
			}
		}
	}
}


void ASM
InitDocumentWindow(REG(a0, struct winData *wd))
{
	struct Node *ln,*cln;
	struct Mappe *mp;
	long   i = 0;

	mp = wd->wd_Data = rxpage->pg_Mappe;
	if ((wd->wd_ExtData[3] = AllocPooled(pool, sizeof(struct List))) != 0) {
		MyNewList(wd->wd_ExtData[3]);

		foreach(&events,ln) {
			if ((cln = AllocPooled(pool, sizeof(struct Node))) != 0) {
				cln->ln_Name = ln->ln_Name;
				cln->ln_Type = mp->mp_Events[i].ev_Flags & EVF_ACTIVE ? TRUE : FALSE;
				MyAddTail(wd->wd_ExtData[3], cln);
			}
			i++;
		}
	}
}


void ASM
InitPrinterWindow(REG(a0, struct winData *wd))
{
	struct wdtPrinter *wp;
	struct Page *page;

	if ((wd->wd_ExtData[1] = AllocPooled(pool, sizeof(struct List))) != 0) {
		MyNewList(wd->wd_ExtData[1]);
		for (page = (APTR)((struct Page *)wd->wd_Data)->pg_Mappe->mp_Pages.mlh_Head;page->pg_Node.ln_Succ;page = (APTR)page->pg_Node.ln_Succ) {
			if ((wp = AllocPooled(pool, sizeof(struct wdtPrinter))) != 0) {
				wp->wp_Node.ln_Name = AllocString(page->pg_Node.ln_Name);
				wp->wp_Node.ln_Type = 1;
				wp->wp_PageMode = PRTPM_ALL;
				wp->wp_Copies = 1;
				wp->wp_Zoom = 1 << 10;
				wp->wp_Page = page;

				MyAddTail(wd->wd_ExtData[1],wp);
			}
		}
	}
	wd->wd_ExtData[2] = ((struct List *)wd->wd_ExtData[1])->lh_Head;
	wd->wd_ExtData[3] = (APTR)TRUE;   // Einstellungen gelten für alle
	wd->wd_ExtData[4] = NULL;		 // Drucker-Unit
	{
		char *filename = "env:sys/printer.prefs";
		char name[32];

		if (!ReadUnitName(filename, name, 0))
			strcpy(name, GetString(&gLocaleInfo, MSG_DEFAULT_PRINTER_GAD));

		wd->wd_ExtData[5] = AllocString(name);
	}
}


void ASM
InitPrintStatusWindow(REG(a0, struct winData *wd))
{
	wd->wd_ExtData[0] = AllocPooled(pool,sizeof(struct wdtPrintStatus));
	asp_wps = wd->wd_ExtData[0];
}


void ASM
InitFindReplaceWindow(REG(a0, struct winData *wd))
{
	// font style
	wd->wd_ExtData[2] = MakeLocaleStringList(MSG_FONT_PLAIN_NAME, MSG_FONT_BOLD_NAME, MSG_FONT_ITALICS_NAME, MSG_FONT_UNDERLINED_NAME, TAG_END);

	// alignment
	wd->wd_ExtData[3] = MakeLocaleStringList(MSG_ALIGN_LEFT_NAME, MSG_ALIGN_CENTER_NAME, MSG_ALIGN_RIGHT_NAME, TAG_END);
	wd->wd_ExtData[4] = MakeLocaleStringList(MSG_VALIGN_TOP_NAME, MSG_VALIGN_MIDDLE_NAME, MSG_VALIGN_BOTTOM_NAME, TAG_END);
}


/************************** Preferences **************************/


void ASM
initPrefChoiceWindow(REG(a0, struct winData *wd))
{
	struct PrefsModule *pm,*cpm;

	if ((wd->wd_ExtData[1] = AllocPooled(pool, sizeof(struct MinList))) != 0)
	{
		MyNewList(wd->wd_ExtData[1]);

		foreach(&prefs.pr_Modules,pm)
		{
			if (!(pm->pm_Flags & PMF_GLOBAL) && (cpm = AllocPooled(pool,sizeof(struct PrefsModule))))
			{
				CopyMem(pm,cpm,sizeof(struct PrefsModule));
				MyAddTail(wd->wd_ExtData[1], cpm);
			}
		}
	}
}


void ASM
InitKeyboardPrefsWindow(REG(a0, struct winData *wd))
{
	wd->wd_ExtData[0] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE);
	wd->wd_ExtData[3] = AllocLBColumnInfo(2,
	                                      LBCIA_Column,0, /*LBCIA_Weight,30,*/ LBCIA_AutoSort,TRUE, LBCIA_Sortable,TRUE,
	                                      LBCIA_Column,1, /*LBCIA_Weight,70,*/ TAG_DONE);

	if ((wd->wd_ExtData[2] = AllocPooled(pool, sizeof(struct List))) != 0)
	{
		struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

		MyNewList(wd->wd_ExtData[2]);
		CopyRALBAppKeys(&pr->pr_AppKeys, wd->wd_ExtData[0], wd->wd_ExtData[2]); // last tag is [NATIVE] list
//		CopyAppKeys(&pr->pr_AppKeys,wd->wd_ExtData[2]);
	}
}


void ASM
initPrefContext(REG(a0, struct winData *wd))
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	long   i;

	for(i = 0;i < NUM_CMT;i++)
	{
		if ((wd->wd_ExtData[i] = AllocPooled(pool,sizeof(struct MinList))))
		{
			MyNewList(wd->wd_ExtData[i]);
			CopyContextMenu(&pr->pr_Contexts[i],wd->wd_ExtData[i]);
		}
	}

	wd->wd_ExtData[5] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
}


void ASM
initPrefMenu(REG(a0, struct winData *wd))
{
	if ((wd->wd_ExtData[3] = AllocPooled(pool,sizeof(struct List))) != 0)
	{
		struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

		MyNewList(wd->wd_ExtData[3]);
		CopyAppMenus(&pr->pr_AppMenus,wd->wd_ExtData[3]);
	}

	wd->wd_ExtData[0] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
	wd->wd_ExtData[1] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
	wd->wd_ExtData[2] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
}


static void
AddPrefIconAppCmd(struct List *list,struct AppCmd *ac)
{
	struct AppCmd *cac;

	if (ac->ac_Node.in_Image)  // has image
	{
		if ((cac = NewAppCmd(ac)) != 0)
			MyAddTail(list, cac);
	}
}


void
MakePrefIconAppCmdsRALB(struct Prefs *pr, struct List *list, struct List *ra_list)
{
	struct PrefsModule *pm = GetPrefsModule(pr, WDT_PREFCMDS);
	struct AppCmd *ac;
	struct Node *n = NULL;

	while( (ac=(APTR)MyRemHead(list)) != 0 )
	{
		FreeAppCmd(ac);
	}

	foreach(&pr->pr_AppCmds, ac)
	{
		AddPrefIconAppCmd(list, ac);
	}

	if( &prefs!=pr && (!pm || (pm->pm_Flags&PMF_ADD)) )
	{
		foreach(&prefs.pr_AppCmds, ac)
		{
			AddPrefIconAppCmd(list, ac);
		}
	}

	sortList( (struct MinList *)list );

	foreach(list, ac)
	{// Create/Build ReAction ListBrowser (Commands with icon)
DBUG("\t[cmds]'%s' 0x%08lx\n",ac->ac_Node.in_Name,ac->ac_Node.in_Image);
		n = AllocListBrowserNode(2,
		                         LBNA_Column,0, LBNCA_Image,ac->ac_Node.in_Image,
		                         LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,ac->ac_Node.in_Name,
		                         LBNA_UserData, ac,
		                        TAG_DONE);
		if(n)
		{
			AddTail(ra_list, n);
//DBUG("\tnode=0x%08lx\n",n);
		}
	}
}
/*void
MakePrefIconAppCmds(struct Prefs *pr,struct List *list)
{
	struct PrefsModule *pm = GetPrefsModule(pr,WDT_PREFCMDS);
	struct AppCmd *ac;

	while ((ac = (APTR)MyRemHead(list)) != 0)
		FreeAppCmd(ac);

	foreach(&pr->pr_AppCmds,ac)
		AddPrefIconAppCmd(list,ac);

	if (&prefs != pr && (!pm || (pm->pm_Flags & PMF_ADD)))
	{
		foreach(&prefs.pr_AppCmds,ac)
			AddPrefIconAppCmd(list,ac);
	}
	sortList((struct MinList *)list);
}*/


void ASM
initPrefIcon(REG(a0, struct winData *wd))
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);
	struct IconObj *io,*sio;
	// RA
	struct Node *n = NULL;

	wd->wd_ExtData[0] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
	wd->wd_ExtData[5] = AllocLBColumnInfo(2,
	                                      LBCIA_Column,0, //LBCIA_Weight,20,
	                                      LBCIA_Column,1, //LBCIA_Weight,80,
	                                     TAG_DONE);
	wd->wd_ExtData[1] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
	wd->wd_ExtData[7] = AllocLBColumnInfo(2,
	                                      LBCIA_Column,0, //LBCIA_Weight,20,
	                                      LBCIA_Column,1, //LBCIA_Weight,80,
	                                     TAG_DONE);
	// RA
	if ((wd->wd_ExtData[2] = AllocPooled(pool, sizeof(struct MinList))) != 0)
	{
		MyNewList(wd->wd_ExtData[2]);
		foreach(&pr->pr_IconObjs,io)
		{
			if ((sio = CopyIconObj(io)) != 0)
			{
				MyAddTail(wd->wd_ExtData[2], sio);
			}
			// RA
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
			// RA
		}
	}
	if ((wd->wd_ExtData[4] = AllocPooled(pool,sizeof(struct MinList))) != 0)
	{
		MyNewList(wd->wd_ExtData[4]);
		MakePrefIconAppCmdsRALB(pr, wd->wd_ExtData[4], wd->wd_ExtData[0]); // last tag is ReAction ListBrowser
		//MakePrefIconAppCmds(pr,wd->wd_ExtData[4]);
	}
	if ((wd->wd_ExtData[3] = AllocPooled(pool, sizeof(struct ImageNode))) != 0)
	{
		((struct ImageNode *)wd->wd_ExtData[3])->in_Name = GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD);
	}
}


// RA
void ASM
initPrefCmdsWindow(REG(a0, struct winData *wd))
{
	struct Node *n = NULL;
	struct AppCmd *ac;

	wd->wd_ExtData[0] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
	wd->wd_ExtData[1] = AllocLBColumnInfo(2,
	                                      LBCIA_Column,0, //LBCIA_Weight,20,
	                                      LBCIA_Column,1, //LBCIA_Weight,80,
	                                     TAG_DONE);

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
}
//RA


void ASM
initPrefColors(REG(a0, struct winData *wd))
{
	struct colorPen *cp, *scp;
	struct Node *node = NULL;

	wd->wd_ExtData[0] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE);
	wd->wd_ExtData[1] = AllocLBColumnInfo(2,
	                                      LBCIA_Column,0, LBCIA_Weight,30,
	                                      LBCIA_Column,1, LBCIA_Weight,70, TAG_DONE);

	wd->wd_ExtData[2] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE);

	wd->wd_ExtData[6] = AllocPooled(pool,sizeof(short)*(MAXGRADPENS+2));

	if ((wd->wd_Data = AllocPooled(pool, sizeof(struct List))) != 0)
	{
		MyNewList(wd->wd_Data);
		for(scp=(APTR)colors.mlh_Head; scp->cp_Node.ln_Succ; scp=(APTR)scp->cp_Node.ln_Succ)
		{
			if( (cp=AllocPooled(pool,sizeof(struct colorPen))) != 0 )
			{
				*cp = *scp;
				cp->cp_Node.ln_Name = AllocString(cp->cp_Node.ln_Name);
				cp->cp_Pen = ObtainBestPen(scr->ViewPort.ColorMap, RGB32(cp->cp_Red), RGB32(cp->cp_Green), RGB32(cp->cp_Blue), 0);
				MyAddTail(wd->wd_Data, cp);
DBUG("initPrefColors(): cp(0x%08lx): R=%ld G=%ld B=%ld\n",cp,cp->cp_Red,cp->cp_Green,cp->cp_Blue);
DBUG("initPrefColors(): cp->cp_Pen=0x%08lx (obtain)\n",cp->cp_Pen);
				Object *myimg = NewObject(BevelClass, NULL,
				                          IA_Width,  scr->Font->ta_YSize*2,//40,
				                          IA_Height, scr->Font->ta_YSize,
				                          BEVEL_Style,   BVS_BOX,
				                          BEVEL_FillPen, cp->cp_Pen==-1? TAG_IGNORE : cp->cp_Pen,
				                         TAG_DONE);
				if( (node=AllocListBrowserNode(2,
				                               LBNA_Column,0, LBNCA_Image,myimg,
				                               LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,cp->cp_Node.ln_Name,
				                               LBNA_UserData, cp, // color list [NATIVE] item
				                              TAG_DONE)) )
				{
					AddTail( (struct List *)wd->wd_ExtData[0], node );
DBUG("initPrefColors(): node(0x%08lx) '%s'\n", node,cp->cp_Node.ln_Name);
				}
			}
		}
	}

	wd->wd_ExtData[5] = &scrcolors;
	for(scp=(APTR)scrcolors.mlh_Head; scp->cp_Node.ln_Succ; scp=(APTR)scp->cp_Node.ln_Succ)
	{
		scp->cp_Pen = ObtainPen(scr->ViewPort.ColorMap, -1, RGB32(scp->cp_Red), RGB32(scp->cp_Green), RGB32(scp->cp_Blue), 0);
DBUG("initPrefColors(): scp(0x%08lx): R=%ld G=%ld B=%ld\n", scp,scp->cp_Red,scp->cp_Green,scp->cp_Blue);
DBUG("initPrefColors(): scp->cp_Pen=0x%08lx (obtain)\n",scp->cp_Pen);
		Object *myimg = NewObject(BevelClass, NULL,
		                          IA_Width,  scr->Font->ta_YSize*2,//40,
		                          IA_Height, scr->Font->ta_YSize,
		                          BEVEL_Style,   BVS_BOX,
		                          BEVEL_FillPen, scp->cp_Pen==-1? TAG_IGNORE : scp->cp_Pen,
		                          TAG_DONE);
		if( (node=AllocListBrowserNode(2,
		                               LBNA_Column,0, LBNCA_Image,myimg,
		                               LBNA_Column,1, LBNCA_Text,scp->cp_Node.ln_Name,
		                               LBNA_UserData, scp, // scrcolors list [NATIVE] item
		                              TAG_DONE)) )
		{
			AddTail( (struct List *)wd->wd_ExtData[2], node );
DBUG("initPrefColors(): node(0x%08lx) '%s'\n", node,scp->cp_Node.ln_Name);
		}
	}

DBUG("initPrefColors(): [0]=0x%08lx   [2]=0x%08lx END\n",(uint32 *)wd->wd_ExtData[0],wd->wd_ExtData[2]);
}
/*void ASM
initPrefColors(REG(a0, struct winData *wd))
{
	struct colorPen *cp,*scp;

	if (!GradientSliderBase)
		GradientSliderBase = OpenLibrary("gadgets/gradientslider.gadget",39);

	if (ColorWheelBase && GradientSliderBase)
		wd->wd_ExtData[6] = AllocPooled(pool,sizeof(short)*(MAXGRADPENS+2));

	if ((wd->wd_Data = AllocPooled(pool, sizeof(struct List))) != 0)
	{
		MyNewList(wd->wd_Data);
		for(scp = (APTR)colors.mlh_Head;scp->cp_Node.ln_Succ;scp = (APTR)scp->cp_Node.ln_Succ)
		{
			if ((cp = AllocPooled(pool, sizeof(struct colorPen))) != 0)
			{
				*cp = *scp;
				cp->cp_Node.ln_Name = AllocString(cp->cp_Node.ln_Name);
				MyAddTail(wd->wd_Data, cp);
			}
		}
	}
	wd->wd_ExtData[5] = &scrcolors;
}*/


void ASM
InitFilePrefsWindow(REG(a0, struct winData *wd))
{
	if ((wd->wd_Data = AllocPooled(pool, sizeof(struct PrefFile))) != 0)
	{
		CopyMem(prefs.pr_File, wd->wd_Data, sizeof(struct PrefFile));
		wd->wd_ExtData[0] = AllocString(projpath);
		wd->wd_ExtData[1] = AllocString(graphicpath);
		wd->wd_ExtData[2] = AllocString(iconpath);
	}
}


void ASM
InitTablePrefsWindow(REG(a0, struct winData *wd))
{
	if ((wd->wd_ExtData[0] = AllocPooled(pool, sizeof(struct PrefTable))) != 0)
		CopyMem(prefs.pr_Table, wd->wd_ExtData[0], sizeof(struct PrefTable));

	wd->wd_ExtData[3] = MakeLocaleStringRACList(MSG_CORNER_MODE_NONE_GAD, MSG_CORNER_MODE_COPY_CELLS_GAD,
	                                            MSG_CORNER_MODE_COPY_GAD, MSG_CORNER_MODE_MOVE_GAD, MSG_CORNER_MODE_SWAP_GAD, TAG_END);
	wd->wd_ExtData[4] = NULL;
}


void ASM
initPrefNames(REG(a0, struct winData *wd))
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

	wd->wd_ExtData[0] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
DBUG("initPrefNames() 0x%08lx:\n", (uint32 *)wd->wd_ExtData[0]);
	if( (wd->wd_ExtData[2]=AllocPooled(pool, sizeof(struct List)))!=0
	   && wd->wd_ExtData[0]!=NULL )
	{
		MyNewList(wd->wd_ExtData[2]);
		CopyRALBListItems(&pr->pr_Names, (struct List *)wd->wd_ExtData[0], CopyName, (struct MinList *)wd->wd_ExtData[2]); // last tag is "old" (native) list
DBUG("initPrefNames() 0x%08lx:\n", (uint32 *)wd->wd_ExtData[2]);
	}
}
/*void ASM
initPrefNames(REG(a0, struct winData *wd))
{
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

	if ((wd->wd_ExtData[2] = AllocPooled(pool, sizeof(struct List))) != 0)
	{
		MyNewList(wd->wd_ExtData[2]);

		CopyListItems(&pr->pr_Names, (struct MinList *)wd->wd_ExtData[2], CopyName);
	}
}*/


// RA
char ra_disp[3];
APTR FormatPrefsSliderDecimals(struct Hook *hook, APTR obj, struct TagItem *tags)
{
	int32 level = 0;

	if(tags != NULL)
	{
		level = (int32)GetTagData(SLIDER_Level, 0L, tags);
DBUG("FormatPrefsSliderPri(): %ld\n",level);
		if(level == -1)
		{
			return( (APTR)GetString(&gLocaleInfo,MSG_AUTO_VALUE_GAD) );
		}
		else
		{
			SNPrintf(ra_disp, 3, "%ld",level);
			return( (APTR)ra_disp );
		}
	}

	return( (APTR)0L );
}
// RA


void ASM
InitFormatPrefsWindow(REG(a0, struct winData *wd))
{
	struct FormatVorlage *fv,*sfv;
	struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

	// RA
	wd->wd_ExtData[3] = MakeLocaleStringRACList(MSG_TIME_H_FORMAT, MSG_TIME_HH_FORMAT, MSG_TIME_M_FORMAT,
	                                            MSG_TIME_MM_FORMAT, MSG_TIME_S_FORMAT, MSG_TIME_SS_FORMAT, TAG_END);
	wd->wd_ExtData[2] = MakeLocaleStringRACList(MSG_DATE_Y_FORMAT, MSG_DATE_YY_FORMAT, MSG_DATE_MNAME_FORMAT,
	                                            MSG_DATE_MMNAME_FORMAT, MSG_DATE_M_FORMAT, MSG_DATE_MM_FORMAT,
	                                            MSG_DATE_W_FORMAT, MSG_DATE_WW_FORMAT, MSG_DATE_D_FORMAT, MSG_DATE_DD_FORMAT, TAG_END);

	wd->wd_ExtData[0] = (struct Hook *)AllocSysObjectTags(ASOT_HOOK, ASO_NoTrack,TRUE,
	                                                      ASOHOOK_Entry,FormatPrefsSliderDecimals,
	                                                     TAG_DONE);

	wd->wd_ExtData[7] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE);
	wd->wd_ExtData[6] = AllocLBColumnInfo(2,
	                                      LBCIA_Column,0,
	                                      LBCIA_Column,1,
	                                     TAG_DONE);

	wd->wd_ExtData[1] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE);
	// RA

	if ((wd->wd_ExtData[5] = AllocPooled(pool, sizeof(struct List))) != 0)
	{
		MyNewList(wd->wd_ExtData[5]);
		for(sfv = (APTR)pr->pr_Formats.mlh_Head;sfv->fv_Node.ln_Succ;sfv = (APTR)sfv->fv_Node.ln_Succ)
		{
			if ((fv = CopyFormat(sfv)) != 0)
			{
DBUG("\t'%s' '%s' (0x%08lx)\n",fv->fv_Node.ln_Name,fv->fv_Preview,fv);
				MyAddTail(wd->wd_ExtData[5],fv);
			}
		}
	}
}


/************************** Objects **************************/


const ULONG  weightvalues[] = {0,0x4000,0x8000,0x10000,0x20000,0x40000,0x80000};


void ASM
InitObjectWindow(REG(a0, struct winData *wd))
{
	struct gObject *go = (struct gObject *)wd->wd_Data;
	struct gInterface *gi;
	BYTE weight = FALSE, lines = 0;

	wd->u.object.wd_Page = rxpage;

	for (gi = go->go_Class->gc_Interface; gi && gi->gi_Tag; gi++) {
		switch (gi->gi_Type) {
			case GIT_WEIGHT:
				weight = TRUE;
			case GIT_PEN:
			case GIT_CYCLE:
			case GIT_CHECKBOX:
				lines++;
				break;
		}
	}
	if (weight)
		wd->u.object.wd_WeightStrings = MakeStringList(GetString(&gLocaleInfo, MSG_HAIR_LINE_GAD), "¼ pt", "½ pt", "1 pt", "2 pt", "4 pt", "8 pt", NULL);
	else
		wd->u.object.wd_WeightStrings = NULL;

	wd->u.object.wd_Lines = lines;

	if ((wd->u.object.wd_Gadgets = AllocPooled(pool, sizeof(struct MinList))) != 0)
		MyNewList(wd->u.object.wd_Gadgets);
}


void ASM
InitDiagramWindow(REG(a0, struct winData *wd))
{
#ifdef __amigaos4__
	wd->wd_Pages = AllocVecTags(sizeof(struct Gadget *) * 6, AVT_ClearWithValue, 0, TAG_DONE);
#else
	wd->wd_Pages = AllocVec(sizeof(struct Gadget *) * 6, MEMF_CLEAR);
#endif

	if ((wd->u.diagram.wd_Gadgets = AllocPooled(pool, sizeof(struct MinList))) != 0)
		MyNewList(wd->u.diagram.wd_Gadgets);
}


/************************** Cell properties **************************/


static const uint8 kFunctionCategoryLevels[] = {
	0, // "Zuletzt benutzte"
	0, // "Alle"
	1, //	  "Mathematik"
	2, //		 "Algebra"
	2, //		 "Finanz"
	2, //		 "Logik"
	2, //		 "Matrizen"
	2, //		 "Trigonometrie"
	2, //		 "Statistik"
	1, //	 "Datum & Zeit"
	1, //	 "Datenbank"
	1, //	 "Text"
	1, //	 "Erscheinung"
	1, //	 "Tabelle"
	1, //	 "Diverse"
};


void ASM
InitFormelWindow(REG(a0, struct winData *wd))
{
	if ((wd->wd_ExtData[2] = MakeLocaleStringList(MSG_RECENTLY_USED_FUNCCAT, MSG_ALL_FUNCCAT, MSG_MATH_FUNCCAT, MSG_ALGEBRA_FUNCCAT, MSG_FINANCE_FUNCCAT,
								MSG_LOGIC_FUNCCAT, MSG_MATRICES_FUNCCAT, MSG_TRIGO_FUNCCAT, MSG_STATISTICS_FUNCCAT,
								MSG_DATE_TIME_FUNCCAT, MSG_DBASE_FUNCCAT, MSG_TEXT_FUNCCAT, MSG_LOOK_FUNCCAT, MSG_SHEET_FUNCCAT, MSG_MISC_FUNCCAT,
								TAG_END)) != 0)
	{
		struct Node *ln;
		int i = 0;

		foreach (wd->wd_ExtData[2], ln)
		{
			ln->ln_Pri = kFunctionCategoryLevels[i];
			ln->ln_Type = i++;
		}
	}
	MakeFewFuncs();
}


BOOL
IsInNotes(struct List *list,STRPTR t)
{
	struct wdtNote *wn;

	foreach(list,wn)
		if (!zstrcmp(wn->wn_Note,t))
			return(TRUE);

	return(FALSE);
}

#define NOTELEN 48

void AddNotesFromPage(struct Page *page, struct List *list, struct List *ra_list)
//void AddNotesFromPage(struct Page *page,struct List *list)
{
	struct tableField *tf;
	struct Node *node = NULL; // RA

	if (!page)
		return;

	foreach(&page->pg_Table,tf)
	{
		STRPTR s;

		if ((s = tf->tf_Note) && !IsInNotes(list,s))
		{
			struct wdtNote *wn;

			if ((wn = AllocPooled(pool, sizeof(struct wdtNote))) != 0)
			{
				char t[NOTELEN];
				int  i = 0;

				if (page == rxpage)
				{
					strcpy(t,Coord2String(tf->tf_Col,tf->tf_Row));
					i = strlen(t);
					strcpy(t+i,": ");
					i += 2;
				}
				t[NOTELEN-1] = '\0';
				strncpy(t+i,s,NOTELEN-i);
				if (t[NOTELEN-1])	  // add ellipsis
					strcpy(t+NOTELEN-4,"...");

				for(i = 0;t[i];i++)	// convert tabs and newlines in a single space
				{
					if (isspace(t[i]))
						t[i] = ' ';
				}
				wn->wn_Node.ln_Name = AllocString(t);
				// RA
				// Add "shortened" note string to ReAction chooser (OID_CN_INSERT)
				node = AllocChooserNode(CNA_Text,wn->wn_Node.ln_Name, CNA_CopyText,TRUE, TAG_DONE);
				AddTail(ra_list, node);
				// RA

DBUG("AddNotesFromPage(): wn->wn_Node.ln_Name='%s' (0x%08lx)\n",wn->wn_Node.ln_Name,node);
				wn->wn_Note = AllocString(s);
DBUG("AddNotesFromPage(): wn->wn_Note='%s'\n",wn->wn_Note);
				MyAddTail(list, wn);
			}
		}
	}
}


void ASM
initNotes(REG(a0, struct winData *wd))
{
	struct Page *page;
	struct Mappe *mp;
	long count = 0;
	// RA
DBUG("initNotes()\n");
	// "Shortened" notes list
	wd->wd_ExtData[1] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE);
	// RA

	if ((wd->wd_ExtData[0] = AllocPooled(pool, sizeof(struct List))) != 0)
	{
		MyNewList(wd->wd_ExtData[0]);

		AddNotesFromPage(rxpage, wd->wd_ExtData[0], wd->wd_ExtData[1]);
		//AddNotesFromPage(rxpage,wd->wd_ExtData[0]);
		count = CountNodes(wd->wd_ExtData[0]);

		foreach(&gProjects, mp)
		{
			foreach(&mp->mp_Pages,page)
			{
				if (page != rxpage)
					AddNotesFromPage(page, wd->wd_ExtData[0], wd->wd_ExtData[1]);
					//AddNotesFromPage(page,wd->wd_ExtData[0]);
			}
		}
		if (count && count != CountNodes(wd->wd_ExtData[0]))
		{
			struct wdtNote *wn;

			if ((wn = AllocPooled(pool,sizeof(struct wdtNote))) != 0)
			{
				wn->wn_Node.ln_Name = AllocString("-");
				InsertAt(wd->wd_ExtData[0],(struct Node *)wn,count);
			}
		}
	}

}


void ASM
InitBorderWindow(REG(a0, struct winData *wd))
{
	struct wdtBorder *wb;
	long i = 0;

	wd->wd_Data = NULL;
	for(;i < 4;i++)
	{
		if ((wb = wd->wd_ExtData[i] = AllocPooled(pool,sizeof(struct wdtBorder))) != 0)
		{
			wb->wb_Color = FindColorPen(0,0,0);
			wb->wb_Point = 1 << 16;
		}
	}
}


/************************** Database **************************/


void ASM
InitDatabaseWindow(REG(a0, struct winData *wd))
{
	struct Mappe *mp;
	struct Database *db,*sdb;
	struct Field *fi,*sfi;
	struct Index *in,*sin;

	if (!wd->wd_Data)
		return;
	mp = ((struct Page *)wd->wd_Data)->pg_Mappe;
	if ((wd->wd_ExtData[2] = AllocPooled(pool, sizeof(struct List))) != 0)
	{
		MyNewList(wd->wd_ExtData[2]);
		for(sdb = (APTR)mp->mp_Databases.mlh_Head;sdb->db_Node.ln_Succ;sdb = (APTR)sdb->db_Node.ln_Succ)
		{
			if ((db = AllocPooled(pool,sizeof(struct Database))) != 0)
			{
				*db = *sdb;
				db->db_Node.ln_Name = AllocString(db->db_Node.ln_Name);
				db->db_Content = AllocString(db->db_Content);
				db->db_Root = CopyTree(db->db_Root);
				MyNewList(&db->db_Fields);
				MyNewList(&db->db_Indices);
				for(sfi = (APTR)sdb->db_Fields.mlh_Head;sfi->fi_Node.ln_Succ;sfi = (APTR)sfi->fi_Node.ln_Succ)
				{
					if ((fi = AllocPooled(pool,sizeof(struct Field))) != 0)
					{
						fi->fi_Node.ln_Name = AllocString(sfi->fi_Node.ln_Name);
						fi->fi_Node.ln_Type = sfi->fi_Node.ln_Type;
						fi->fi_Special = AllocString(sfi->fi_Special);
						MyAddTail(&db->db_Fields, fi);
					}
				}
				for(sin = (APTR)sdb->db_Indices.mlh_Head;sin->in_Node.ln_Succ;sin = (APTR)sin->in_Node.ln_Succ)
				{
					if ((in = AllocPooled(pool,sizeof(struct Index))) != 0)
					{
						in->in_Node.ln_Name = AllocString(sin->in_Node.ln_Name);
						/* Index has to be generated again */
						MyAddTail(&db->db_Indices, in);
					}
				}
				MyAddTail(wd->wd_ExtData[2], db);
			}
		}
	}
}


struct Database *
CurrentInitDB(struct MinList *mlh,struct Database *db)
{
	struct Database *cdb = NULL;

	if (!IsListEmpty((struct List *)mlh))
	{
		if (db)
			cdb = (struct Database *)MyFindName(mlh, db->db_Node.ln_Name);
		else
			cdb = (struct Database *)mlh->mlh_Head;
	}
	return cdb;
}


void ASM
InitMaskWindow(REG(a0, struct winData *wd))
{
	struct Database *db;
	struct Mask *ma,*sma;
	struct MaskField *mf,*smf;

	wd->wd_Data = rxpage->pg_Mappe;
	if ((wd->wd_ExtData[1] = AllocPooled(pool,sizeof(struct MinList))) != 0)
	{
		MyNewList(wd->wd_ExtData[1]);
		for(db = (APTR)rxpage->pg_Mappe->mp_Databases.mlh_Head;db->db_Node.ln_Succ;db = (APTR)db->db_Node.ln_Succ)
		{
			if ((ma = AllocPooled(pool,sizeof(struct Mask))) != 0)
			{
				ma->ma_Node.ln_Name = AllocString(db->db_Node.ln_Name);
				MyNewList(&ma->ma_Fields);
				if ((sma = (struct Mask *)MyFindName(&rxpage->pg_Mappe->mp_Masks,db->db_Node.ln_Name)) != 0)
				{
					ma->ma_Page = sma->ma_Page;
					for(smf = (APTR)sma->ma_Fields.mlh_Head;smf->mf_Node.ln_Succ;smf = (APTR)smf->mf_Node.ln_Succ)
					{
						if ((mf = AllocPooled(pool,sizeof(struct MaskField))) != 0)
						{
							*mf = *smf;
							mf->mf_Node.ln_Name = AllocString(smf->mf_Node.ln_Name);
							MyAddTail(&ma->ma_Fields, mf);
						}
					}
				}
				MyAddTail(wd->wd_ExtData[1],ma);
			}
		}
		wd->wd_ExtData[0] = CurrentInitDB(wd->wd_ExtData[1],wd->wd_ExtData[0]);
	}
}


void ASM
InitIndexWindow(REG(a0, struct winData *wd))
{
	struct Database *db,*cdb;
	struct Index *in,*cin;

	wd->wd_Data = rxpage->pg_Mappe;
	if (wd->wd_ExtData[0])
		wd->wd_ExtData[6] = (APTR)TRUE;
	if ((wd->wd_ExtData[1] = AllocPooled(pool,sizeof(struct MinList))) != 0)
	{
		MyNewList(wd->wd_ExtData[1]);
		foreach(&rxpage->pg_Mappe->mp_Databases,db)
		{
			if ((cdb = AllocPooled(pool,sizeof(struct Database))) != 0)
			{
				CopyMem(db,cdb,sizeof(struct Database));
				cdb->db_Node.ln_Name = AllocString(db->db_Node.ln_Name);
				MyNewList(&cdb->db_Indices);
				foreach(&db->db_Indices,in)
				{
					if ((cin = AllocPooled(pool,sizeof(struct Index))) != 0)
					{
						CopyMem(in,cin,sizeof(struct Index));
						cin->in_Node.ln_Name = AllocString(in->in_Node.ln_Name);
						cin->in_Index = NULL;
						MyAddTail(&cdb->db_Indices, cin);
					}
				}
				/* all other fields remain unchanged: field-list, etc */

				MyAddTail(wd->wd_ExtData[1], cdb);
			}
		}
		wd->wd_ExtData[0] = CurrentInitDB(wd->wd_ExtData[1],wd->wd_ExtData[0]);
	}
}


void ASM
InitFilterWindow(REG(a0, struct winData *wd))
{
	struct Database *db, *cdb;
	struct Filter *fi, *cfi;

	wd->wd_Data = rxpage->pg_Mappe;
	if (wd->wd_ExtData[0])
		wd->wd_ExtData[6] = (APTR)TRUE;

	if ((wd->wd_ExtData[1] = AllocPooled(pool,sizeof(struct MinList))) != 0)
	{
		MyNewList(wd->wd_ExtData[1]);
		foreach(&rxpage->pg_Mappe->mp_Databases,db)
		{
			if ((cdb = AllocPooled(pool,sizeof(struct Database))) != 0)
			{
				CopyMem(db,cdb,sizeof(struct Database));
				cdb->db_Node.ln_Name = AllocString(db->db_Node.ln_Name);
				MyNewList(&cdb->db_Filters);
				foreach(&db->db_Filters,fi)
				{
					if ((cfi = AllocPooled(pool,sizeof(struct Filter))) != 0)
					{
						CopyMem(fi,cfi,sizeof(struct Filter));
						cfi->fi_Node.ln_Name = AllocString(fi->fi_Node.ln_Name);
						cfi->fi_Filter = AllocString(fi->fi_Filter);
						cfi->fi_Root = CopyTree(fi->fi_Root);
						cfi->fi_Index = NULL;
						MyAddTail(&cdb->db_Filters, cfi);
					}
				}
				/* all other fields remain unchanged: field-list, etc */

				MyAddTail(wd->wd_ExtData[1], cdb);
			}
		}
		wd->wd_ExtData[0] = CurrentInitDB(wd->wd_ExtData[1],wd->wd_ExtData[0]);
	}
}
