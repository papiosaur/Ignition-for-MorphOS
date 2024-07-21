/* Preferences management and related functionality
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include <proto/gtdrag.h>

	extern char IgnTitle[100];		//ScreenTitle for ignition, must be unique for PubScreen handling
#endif


extern struct FormatVorlage empty_fv;


struct Window *
GetPrefsWindow(struct Prefs *pr,UWORD type)
{
	struct Window *win;
	struct winData *wd;

	if (!scr)
		return NULL;

	for (win = scr->FirstWindow; win; win = win->NextWindow) {
		if (win->UserPort == iport && (wd = (APTR)win->UserData) && wd->wd_Type == type && GetLocalPrefs(wd->wd_Data) == pr)
			return win;
	}
	return NULL;
}


STRPTR
MakePrefsTitle(struct Mappe *mp, uint8 type, STRPTR t)
{
	struct PrefsModule *pm;
	STRPTR env, title;
	
	if (type == WDT_PREFS)
		return AllocString(GetString(&gLocaleInfo, MSG_PREFS_TITLE));

	// set environment string
	if ((pm = GetPrefsModule(&prefs, type)) && pm->pm_Flags & PMF_GLOBAL)
		env = GetString(&gLocaleInfo, MSG_GLOBAL_ENV_TITLE);
	else if (mp)
		env = mp != (APTR)~0L ? (STRPTR)mp->mp_Node.ln_Name : GetString(&gLocaleInfo, MSG_CLIPBOARD_ENV_TITLE);
	else
		env = GetString(&gLocaleInfo, MSG_GLOBAL_ENV_TITLE);

	if (t && (title = AllocPooled(pool, strlen(env) + strlen(t) + 5 + strlen(GetString(&gLocaleInfo, MSG_PREFERENCES_ENV_TITLE))))) {
		strcpy(title, GetString(&gLocaleInfo, MSG_PREFERENCES_ENV_TITLE));
		strcat(title, t);
		strcat(title, "  (");
		strcat(title, env);
		strcat(title, ")");
	} else if (!t) {
		char buffer[256];
		sprintf(buffer, GetString(&gLocaleInfo, MSG_ADD_PREFS_TITLE), env);

		title = AllocString(buffer);
	}
	return title;
}


uint32
GetPrefsModuleType(uint32 flag)
{
	switch (flag)
	{
		case PRF_DISPLAY:   return WDT_PREFDISP;
		case PRF_FILE:	  return WDT_PREFFILE;
		case PRF_SCREEN:	return WDT_PREFSCREEN;
		case PRF_KEYS:	  return WDT_PREFKEYS;
		case PRF_FORMAT:	return WDT_PREFFORMAT;
		case PRF_NAMES:	 return WDT_PREFNAMES;
		//case PRF_PRINTER: return WDT_PREFPRINTER;
		case PRF_COLORS:	return WDT_PREFCOLORS;
		case PRF_TABLE:	 return WDT_PREFTABLE;
		case PRF_ICON:	  return WDT_PREFICON;
		case PRF_CMDS:	  return WDT_PREFCMDS;
		case PRF_MENU:	  return WDT_PREFMENU;
		case PRF_SYSTEM:	return WDT_PREFSYS;
		case PRF_CONTEXT:   return WDT_PREFCONTEXT;
	}
	return 0;
}


uint32
GetPrefsModuleFlag(uint16 type)
{
	switch (type)
	{
		case WDT_PREFDISP:	  return PRF_DISPLAY;
		case WDT_PREFFILE:	  return PRF_FILE;
		case WDT_PREFSCREEN:	return PRF_SCREEN;
		case WDT_PREFKEYS:	  return PRF_KEYS;
		case WDT_PREFFORMAT:	return PRF_FORMAT;
		case WDT_PREFNAMES:	 return PRF_NAMES;
		//case WDT_PREFPRINTER: return PRF_PRINTER;
		case WDT_PREFCOLORS:	return PRF_COLORS;
		case WDT_PREFTABLE:	 return PRF_TABLE;
		case WDT_PREFICON:	  return PRF_ICON;
		case WDT_PREFCMDS:	  return PRF_CMDS;
		case WDT_PREFMENU:	  return PRF_MENU;
		case WDT_PREFSYS:	   return PRF_SYSTEM;
		case WDT_PREFCONTEXT:   return PRF_CONTEXT;
	}
	return 0; /* suppress compiler warning */
}


bool
HasPrefsModule(struct Prefs *pr,UWORD type)
{
	return (bool)((pr->pr_ModuleFlags & GetPrefsModuleFlag(type)) != 0);
}


struct PrefsModule *
GetPrefsModule(struct Prefs *pr,UWORD type)
{
	struct PrefsModule *pm;

	if (!pr)
		pr = &prefs;

	if (!HasPrefsModule(pr,type))
		return NULL;

	foreach(&pr->pr_Modules,pm)
	{
		if (pm->pm_Type == type)
			return pm;
	}
	return NULL;
}


void
RefreshPrefsModule(struct Prefs *pr,struct PrefsModule *pm,UWORD type)
{
	struct Mappe *mp = GetPrefsMap(pr);

	if (pr == &recycledprefs)
		return;

	if (pm)
		type = pm->pm_Type;
	PropagatePrefsModule(pr,type);

	SetBusy(true, BT_APPLICATION);

	switch (type)
	{
		case WDT_PREFCMDS:
			MakeCmdsLinks(mp);
			break;
		case WDT_PREFFORMAT:
			MakeFormatLinks(mp);
			RefreshMapTexts(mp, true);
			break;
		case WDT_PREFNAMES:
			MakeNamesLinks(mp);
			RecalcMap(mp);
			break;
		case WDT_PREFICON:
			RefreshIconSize(pr);
		case WDT_PREFDISP:
			if (mp)
				RefreshProjectWindow(mp->mp_Window,TRUE);
			else
				RefreshProjWindows(TRUE);
			break;
		case WDT_PREFMENU:
			RefreshMenu(pr);
			break;
		case WDT_PREFCONTEXT:
			RefreshContextMenus(pr);
			break;
	}

	SetBusy(false, BT_APPLICATION);
}


void
RefreshPrefsModules(struct Prefs *pr,LONG modules)
{
	struct PrefsModule *pm;

	if (pr == &recycledprefs)
		return;

	foreach(&pr->pr_Modules,pm)
	{
		if (!(GetPrefsModuleFlag(pm->pm_Type) & modules))
			continue;

		RefreshPrefsModule(pr,pm,0);
	}
}


void
FreePrefsModuleData(struct Prefs *pr,struct PrefsModule *pm)
{
	switch(pm->pm_Type)
	{
		case WDT_PREFCMDS:
			FreeListItems(&pr->pr_AppCmds,FreeAppCmd);
			break;
		case WDT_PREFFORMAT:
			FreeListItems(&pr->pr_Formats,FreeFormat);
			break;
		case WDT_PREFNAMES:
			FreeListItems(&pr->pr_Names,FreeName);
			break;
		case WDT_PREFDISP:
			if (pr->pr_Disp->pd_AntiFont)
				CloseFont(pr->pr_Disp->pd_AntiFont);
			FreeString((STRPTR)pr->pr_Disp->pd_AntiAttr.ta_Name);
			FreePooled(pool,pr->pr_Disp,sizeof(struct PrefDisp));
			break;
		case WDT_PREFICON:
			FreeIconObjs(&pr->pr_IconObjs);
			FreePooled(pool,pr->pr_Icon,sizeof(struct PrefIcon));
			break;
		case WDT_PREFSCREEN:
			FreeString((STRPTR)pr->pr_Screen->ps_TextAttr.ta_Name);
			FreePooled(pool,pr->pr_Screen,sizeof(struct PrefScreen));
			break;
		case WDT_PREFFILE:
			FreePooled(pool,pr->pr_File,sizeof(struct PrefFile));
			break;
		case WDT_PREFTABLE:
			FreePooled(pool,pr->pr_Table,sizeof(struct PrefTable));
			break;
		case WDT_PREFKEYS:
			FreeAppKeys(&pr->pr_AppKeys);
			break;
		case WDT_PREFMENU:
			FreeAppMenus(&pr->pr_AppMenus);
			FreeAppMenu(pr);
			break;
		case WDT_PREFCONTEXT:
		{
			long i;

			for(i = 0;i < NUM_CMT;i++)
				FreeContextMenu(&pr->pr_Contexts[i]);
			break;
		}
	}
}


void
FreePrefsModule(struct Prefs *pr,struct PrefsModule *pm)
{
	FreePrefsModuleData(pr,pm);
	FreeImage(pm->pm_Node.in_Image);
	FreePooled(pool,pm,sizeof(struct PrefsModule));
}


void
AllocPrefsModuleData(struct Prefs *pr,UWORD type)
{
	switch (type)
	{
		case WDT_PREFDISP:
		{
			struct PrefDisp *pd;

			if ((pd = pr->pr_Disp = AllocPooled(pool, sizeof(struct PrefDisp))) != 0)
			{
				pd->pd_Rasta = PDR_POINTS;
				pd->pd_IconBar = PDIB_LEFT;
				pd->pd_FormBar = PDFB_BOTTOM;
				pd->pd_ToolBar = TRUE;
				pd->pd_HelpBar = TRUE;
				pd->pd_ShowAntis = TRUE;
#ifdef __AROS__
				pd->pd_AntiAttr.ta_Name = AllocString("arial.font");
#else
				pd->pd_AntiAttr.ta_Name = AllocString("helvetica.font");
#endif	
				pd->pd_AntiAttr.ta_YSize = 11;
				UpdateAntiFont(pr);
			}
			break;
		}
		case WDT_PREFICON:
			if ((pr->pr_Icon = AllocPooled(pool, sizeof(struct PrefIcon))) != 0)
			{
				pr->pr_Icon->pi_Width = 24;
				pr->pr_Icon->pi_Height = 24;
				pr->pr_Icon->pi_Spacing = TOTSPACE_WIDTH;
			}
			break;
		case WDT_PREFSCREEN:
		{
			struct PrefScreen *ps;

			if ((ps = pr->pr_Screen = AllocPooled(pool, sizeof(struct PrefScreen))) != 0)
			{
				ps->ps_Width = 640;
				ps->ps_Height = 480;
				ps->ps_Type = PST_LIKEWB;
				strcpy(ps->ps_PubName,"Workbench");
				ps->ps_BackFill = TRUE;
				ps->ps_BFColor = 1;
#ifdef __AROS__
				ps->ps_TextAttr.ta_Name = AllocString("arial.font");
#else
				ps->ps_TextAttr.ta_Name = AllocString("helvetica.font");
#endif
				ps->ps_TextAttr.ta_YSize = 11;
				ps->ps_mmWidth = 340 << 10;
				ps->ps_mmHeight = 280 << 10;
			}
			break;
		}
		case WDT_PREFFILE:
			if ((pr->pr_File = AllocPooled(pool, sizeof(struct PrefFile))) != 0) {
				pr->pr_File->pf_Flags = PFF_WARN_IOTYPE | PFF_ICONS;
				pr->pr_File->pf_AutoSaveIntervall = 10*60;  /* 10 min */
			}
			break;
		case WDT_PREFTABLE:
			if ((pr->pr_Table = AllocPooled(pool, sizeof(struct PrefTable))) != 0)
			{
				pr->pr_Table->pt_Flags = PTF_SHOWZEROS | PTF_EDITFUNC;
				pr->pr_Table->pt_EditFunc[PTEQ_NONE] = PTEF_COPY;
			}
			break;
		case WDT_PREFMENU:
			pr->pr_Menu = NULL;
			break;
		case WDT_PREFCONTEXT:
			InitContextMenus(pr);
			break;
		case WDT_PREFSYS:
			// ToDo: PRF_USEDBUFFER is only specified because of the broken Picture-object
			prefs.pr_Flags = PRF_APPICON | PRF_CONTEXTMENU | PRF_USESESSION | PRF_USEDBUFFER;
			break;
	}
}


void
RemPrefsModule(struct Prefs *pr,struct PrefsModule *pm)
{
	struct TreeNode *tn;
	UWORD  type;

	if (!pm)
		return;

	type = pm->pm_Type;
	if ((tn = FindTreeSpecial(&prefstree.tl_Tree,pm)) != 0)
	{
		RemoveFromLockedList((struct MinList *)&prefstree, (struct MinNode *)tn);
		FreePooled(pool,tn,sizeof(struct TreeNode));
	}
	MyRemove(pm);

	FreePrefsModule(pr,pm);
	pr->pr_ModuleFlags &= ~GetPrefsModuleFlag(type);

	PropagatePrefsModuleMap(&prefs,GetPrefsMap(pr),type);
	RefreshPrefsModule(pr,NULL,type);
}


void
PropagatePrefsModuleMap(struct Prefs *pr,struct Mappe *mp,UWORD type)
{
	if (!mp || mp == (APTR)~0L || HasPrefsModule(&mp->mp_Prefs,type))
		return;

	switch (type)
	{
		case WDT_PREFDISP:
			mp->mp_Prefs.pr_Disp = prefs.pr_Disp;
			break;
		case WDT_PREFTABLE:
			mp->mp_Prefs.pr_Table = prefs.pr_Table;
			break;
		case WDT_PREFSCREEN:
			mp->mp_Prefs.pr_Screen = prefs.pr_Screen;
			break;
		case WDT_PREFFILE:
			mp->mp_Prefs.pr_File = prefs.pr_File;
			break;
		case WDT_PREFICON:
			mp->mp_Prefs.pr_Icon = prefs.pr_Icon;
			break;
		case WDT_PREFSYS:
			mp->mp_Prefs.pr_Flags = prefs.pr_Flags;
			break;
		case WDT_PREFMENU:
			mp->mp_Prefs.pr_Menu = prefs.pr_Menu;
			break;
	}
}


void
PropagatePrefsModule(struct Prefs *pr,UWORD type)
{
	struct Mappe *mp;

	if (pr != &prefs)  // macht bei anderen noch keinen Sinn
		return;

	foreach(&gProjects,mp)
		PropagatePrefsModuleMap(pr,mp,type);
}


void
PropagatePrefsToMap(struct Prefs *pr,struct Mappe *mp)
{
	struct PrefsModule *pm;

	foreach(&prefs.pr_Modules,pm)
		PropagatePrefsModuleMap(pr,mp,pm->pm_Type);
}


void
PropagatePrefs(struct Prefs *pr)
{
	struct PrefsModule *pm;

	foreach(&prefs.pr_Modules,pm)
		PropagatePrefsModule(pr,pm->pm_Type);
}


bool
SetPrefsModule(struct Prefs *pr,UWORD type,BYTE modified)
{
	struct PrefsModule *pm;

	if (!pr)
		pr = &prefs;

	foreach(&pr->pr_Modules,pm)
	{
		if (type == 0xff || pm->pm_Type == type)
		{
			pm->pm_Flags = (pm->pm_Flags & ~PMF_MODIFIED) | (modified ? PMF_MODIFIED : 0);
			if (type != 0xff)
				return true;
		}
	}
	return false;
}


struct PrefsModule *
AddPrefsModule(struct Prefs *pr, CONST_STRPTR t, STRPTR iname, UWORD type, UBYTE flags)
{
	struct PrefsModule *pm;

	if (!pr)
		pr = &prefs;
	if (pr != &prefs && flags & PMF_GLOBAL || !t)
		return NULL;

	if ((pm = AllocPooled(pool, sizeof(struct PrefsModule))) != 0) {
		pm->pm_Node.in_Name = (STRPTR)t;
		pm->pm_Node.in_Type = LNT_PREFSMODULE;
		pm->pm_Type = type;
		pm->pm_ImageName = iname;
		pm->pm_Node.in_Image = LoadImage(iname);
		pm->pm_Flags = flags;
		pr->pr_ModuleFlags |= GetPrefsModuleFlag(type);
		AllocPrefsModuleData(pr,type);
		MyAddTail(&pr->pr_Modules, pm);
	}

	return pm;
}


void
AddPrefsModuleToTree(struct Prefs *pr, struct PrefsModule *pm,
	struct MinList *tree)
{
	struct TreeNode *tn;
	long flags;
					
	// In beginner mode, not all preferences are available
	if (!pm || (gIsBeginner && (pm->pm_Type == WDT_PREFCMDS || pm->pm_Type == WDT_PREFMENU || pm->pm_Type == WDT_PREFCONTEXT || pm->pm_Type == WDT_PREFKEYS)))
		return;

	flags = TNF_SORT;
	if (pr != &prefs && pr != &recycledprefs && pm->pm_Flags & (PMF_ADD | PMF_REPLACE))
		flags |= (pm->pm_Flags & PMF_ADD) ? TNF_ADD : TNF_REPLACE;
	if (pm->pm_Flags & PMF_GLOBAL)
		flags |= TNF_STATIC | TNF_HIGHLIGHTED;

	if ((tn = AddTreeNode(pool, tree, pm->pm_Node.in_Name, pm->pm_Node.in_Image, flags)) != 0)
		tn->tn_Special = pm;
}


void
AddPrefsModulesToTree(struct Prefs *pr, struct MinList *tree)
{
	struct PrefsModule *pm;

	if (!pr)
		pr = &prefs;

	foreach (&pr->pr_Modules, pm)
		AddPrefsModuleToTree(pr, pm, tree);
}


void
AddPrefsModuleToLocalPrefs(struct Mappe *mp, UWORD type)
{
	struct PrefsModule *pm;
	struct Prefs *pr;

	if (!mp || mp == (APTR)~0L || HasPrefsModule(pr = &mp->mp_Prefs, type))
		return;

	if (!(pm = GetPrefsModule(&prefs, type)))
		return;

	if ((pm = AddPrefsModule(pr, pm->pm_Node.in_Name, pm->pm_ImageName, pm->pm_Type, pm->pm_Flags)) != 0)
	{
		struct TreeNode *tn;

		if ((tn = FindTreeSpecial(&prefstree.tl_Tree, mp)) && LockList((struct MinList *)&prefstree, LNF_REFRESH))
		{
			AddPrefsModuleToTree(pr, pm, &tn->tn_Nodes);

			UnlockList((struct MinList *)&prefstree, LNF_REFRESH);
		}
	}
}


void
CopyPrefsModule(struct TreeNode *source,struct TreeNode *tn,struct TreeNode *target)
{
	if (!source || !tn || !target)
		return;

	if (LockList((struct MinList *)&prefstree,LNF_REFRESH))
	{
		struct PrefsModule *pm = tn->tn_Special;
		struct TreeNode *ttn;
		long   mode = 3;

		foreach(&target->tn_Nodes,ttn)
		{
			if (((struct PrefsModule *)ttn->tn_Special)->pm_Type == pm->pm_Type)
			{
				if (pm->pm_Flags & (PMF_ADD | PMF_REPLACE))
					mode = DoRequest(GetString(&gLocaleInfo, MSG_PREFS_PART_EXISTS_REQ),GetString(&gLocaleInfo, MSG_APPEND_REPLACE_BACK_GAD));
				else if (!DoRequest(GetString(&gLocaleInfo, MSG_PREFS_PART_LOST_REQ),GetString(&gLocaleInfo, MSG_CONTINUE_BACK_GAD)))
					mode = 0;
				break;
			}
		}

		/* mode
		**  0   nichts
		**  1   anhängen
		**  2   ersetzen
		**  3   normal kopieren
		*/

		if (mode)
		{
			struct PrefsModule *cpm;
			struct Prefs *spr = GetLocalPrefs(source->tn_Special);
			struct Prefs *pr = GetLocalPrefs(target->tn_Special);

			if (pr == spr)
				return;
			if (!ttn->tn_Node.in_Succ && (cpm = AddPrefsModule(pr,pm->pm_Node.in_Name,pm->pm_ImageName,pm->pm_Type,pm->pm_Flags)))
				AddPrefsModuleToTree(pr,cpm,&target->tn_Nodes);

			switch (pm->pm_Type)
			{
				case WDT_PREFFORMAT:
					if (mode == 2)  // ersetzen
						FreeListItems(&pr->pr_Formats,FreeFormat);
					CopyListItems(&spr->pr_Formats,&pr->pr_Formats,CopyFormat);
					if (mode == 1)  // anhängen
						SortFormatList(&pr->pr_Formats);
					break;
				case WDT_PREFNAMES:
					if (mode == 2)  // ersetzen
						FreeListItems(&pr->pr_Names,FreeName);

					CopyListItems(&spr->pr_Names,&pr->pr_Names,CopyName);

					if (mode == 1)  // anhängen
						sortList(&pr->pr_Names);
					break;
				case WDT_PREFCMDS:
					if (mode == 2)  // ersetzen
						FreeListItems(&pr->pr_AppCmds,FreeAppCmd);
					CopyListItems(&spr->pr_AppCmds,&pr->pr_AppCmds,NewAppCmd);
					if (mode == 1)  // anhängen
						sortList(&pr->pr_AppCmds);
					break;
				case WDT_PREFICON:
					if (mode == 2)  // ersetzen
						FreeIconObjs(&pr->pr_IconObjs);
					CopyListItems(&spr->pr_IconObjs,&pr->pr_IconObjs,CopyIconObj);
					break;
				case WDT_PREFMENU:
					FreeAppMenus(&pr->pr_AppMenus);
					CopyAppMenus(&spr->pr_AppMenus,&pr->pr_AppMenus);
					break;
				case WDT_PREFKEYS:
					if (mode == 2)  // ersetzen
						FreeAppKeys(&pr->pr_AppKeys);
					CopyAppKeys(&spr->pr_AppKeys,&pr->pr_AppKeys);
					if (mode == 1)  // anhängen
						sortList(&pr->pr_AppKeys);
					break;
				case WDT_PREFCONTEXT:
				{
					long i;

					for (i = 0;i < NUM_CMT;i++)
						FreeContextMenu(&pr->pr_Contexts[i]);
					for (i = 0;i < NUM_CMT;i++)
						CopyContextMenu(&spr->pr_Contexts[i],&pr->pr_Contexts[i]);
					break;
				}
			}
			RefreshPrefsModule(pr,pm,0);
			if (imsg.Qualifier & IEQUALIFIER_SHIFT)
				RemPrefsModule(GetLocalPrefs(source->tn_Special),pm);
		}
		UnlockList((struct MinList *)&prefstree,LNF_REFRESH);
	}
}


void
RefreshMapPrefs(struct Mappe *mp)
{
	MakeFormatLinks(mp);
	MakeNamesLinks(mp);
	MakeCmdsLinks(mp);
}


void
AddStandardOutput(void)
{
	struct Node *ln;

	if ((ln = AllocPooled(pool, sizeof(struct Node))) != 0)
	{
		ln->ln_Name = AllocString("CON:/14//100/ignition-Output/AUTO/WAIT/SCREEN IGNITION.1");
		MyAddTail(&outputs, ln);
	}
	if ((ln = AllocPooled(pool, sizeof(struct Node))) != 0)
	{
		ln->ln_Name = AllocString("KCON:/14//100/ignition-Output/AUTO/WAIT/SCREEN IGNITION.1");
		MyAddTail(&outputs, ln);
	}
	if ((ln = AllocPooled(pool, sizeof(struct Node))) != 0)
	{
		ln->ln_Name = AllocString("T:ign.out");
		MyAddTail(&outputs, ln);
	}
}


bool
RemoveLinkWithName(struct MinList *list,STRPTR name)
{
	struct Link *l;

	if ((l = FindLinkWithName(list, name)) != 0)
	{
		MyRemove(l);
		FreePooled(pool,l,sizeof(struct Link));

		return true;
	}
	return false;
}


void
MakeCmdsMapLinks(struct Mappe *mp)
{
	if (!mp)
		return;

	if (LockList(&mp->mp_AppCmds,LNF_REFRESH))
	{
		struct PrefsModule *pm = GetPrefsModule(&mp->mp_Prefs,WDT_PREFCMDS);
		struct AppCmd *ac;
		struct Link *l;

		while((l = (APTR)MyRemHead(&mp->mp_AppCmds)) != 0)
			FreePooled(pool,l,sizeof(struct Link));

		if (!pm || (pm->pm_Flags & PMF_ADD))
		{
			foreach(&prefs.pr_AppCmds,ac)
				AddLink(&mp->mp_AppCmds,(struct MinNode *)ac,renderHook.h_Entry);
		}
		foreach(&mp->mp_Prefs.pr_AppCmds,ac)
		{
			RemoveLinkWithName(&mp->mp_AppCmds,ac->ac_Node.in_Name);
			AddLink(&mp->mp_AppCmds,(struct MinNode *)ac,renderHook.h_Entry);
		}
		SortListWith(&mp->mp_AppCmds,LinkNameSort); //GURU-MELDUNG

		UnlockList(&mp->mp_AppCmds,LNF_REFRESH);
	}
}


void
MakeCmdsLinks(struct Mappe *mp)
{
	if (mp == (APTR)~0L)
		return;

	if (mp)
		MakeCmdsMapLinks(mp);
	else
	{
		foreach(&gProjects,mp)
			MakeCmdsMapLinks(mp);
	}
}


void
MakeNamesMapLinks(struct Mappe *mp)
{
	if (!mp)
		return;

	if (LockList(&mp->mp_Names,LNF_REFRESH))
	{
		struct PrefsModule *pm = GetPrefsModule(&mp->mp_Prefs,WDT_PREFNAMES);
		struct Name *nm;
		struct Link *l;

		while ((l = (APTR)MyRemHead(&mp->mp_Names)) != 0)
			FreePooled(pool,l,sizeof(struct Link));

		if (!pm || (pm->pm_Flags & PMF_ADD))  // add global names
		{
			foreach(&prefs.pr_Names,nm)
				AddLink(&mp->mp_Names,(struct MinNode *)nm,popUpHook.h_Entry);
		}
		foreach(&mp->mp_Databases,nm)		 // add local databases
		{
			RemoveLinkWithName(&mp->mp_Names,nm->nm_Node.ln_Name);
			AddLink(&mp->mp_Names,(struct MinNode *)nm,popUpHook.h_Entry);
		}
		foreach(&mp->mp_Prefs.pr_Names,nm)	// add local names
		{
			RemoveLinkWithName(&mp->mp_Names,nm->nm_Node.ln_Name);
			AddLink(&mp->mp_Names,(struct MinNode *)nm,popUpHook.h_Entry);
		}
		SortListWith(&mp->mp_Names,LinkNameSort);

		UnlockList(&mp->mp_Names,LNF_REFRESH);
	}
}


void
MakeNamesLinks(struct Mappe *mp)
{
	if (mp == (APTR)~0L)
		return;

	if (mp)
		MakeNamesMapLinks(mp);
	else
	{
		foreach(&gProjects,mp)
			MakeNamesMapLinks(mp);
	}
}


int32
LinkCalcFormatSort(struct Link **la,struct Link **lb)
{
	int32 i;

	if (!(i = (((struct Node *)(*la)->l_Link)->ln_Pri-((struct Node *)(*lb)->l_Link)->ln_Pri)))
		return FormatSort((struct Node **)&(*la)->l_Link,(struct Node **)&(*lb)->l_Link);

	return i;
}


int32
LinkFormatSort(struct Link **la,struct Link **lb)
{
	return FormatSort((struct Node **)&(*la)->l_Link,(struct Node **)&(*lb)->l_Link);
}


void
MakeFormatMapLinks(struct Mappe *mp)
{
	if (!mp)
		return;

	if (LockList(&mp->mp_Formats,LNF_REFRESH))
	{
		struct FormatVorlage *fv;
		struct PrefsModule *pm = GetPrefsModule(&mp->mp_Prefs,WDT_PREFFORMAT);
		struct Link *l;

		while ((l = (APTR)MyRemHead(&mp->mp_Formats)) != 0)
			FreePooled(pool,l,sizeof(struct Link));
		while ((l = (APTR)MyRemHead(&mp->mp_CalcFormats)) != 0)
			FreePooled(pool,l,sizeof(struct Link));

		if (!pm || (pm->pm_Flags & PMF_ADD))
		{
			foreach(&prefs.pr_Formats,fv)
			{
				AddLink(&mp->mp_Formats,(struct MinNode *)fv,formatHook.h_Entry);
				AddLink(&mp->mp_CalcFormats,(struct MinNode *)fv,formatHook.h_Entry);
			}
		}
		foreach(&mp->mp_Prefs.pr_Formats,fv)
		{
			AddLink(&mp->mp_Formats,(struct MinNode *)fv,formatHook.h_Entry);
			AddLink(&mp->mp_CalcFormats,(struct MinNode *)fv,formatHook.h_Entry);
		}
		AddLink(&mp->mp_Formats,(struct MinNode *)&empty_fv,formatHook.h_Entry);

		SortListWith(&mp->mp_Formats,LinkFormatSort);
		SortListWith(&mp->mp_CalcFormats,LinkCalcFormatSort);

		UnlockList(&mp->mp_Formats,LNF_REFRESH);
	}
}


void
MakeFormatLinks(struct Mappe *mp)
{
	if (mp == (APTR)~0L)
		return;

	if (mp)
		MakeFormatMapLinks(mp);
	else
	{
		foreach(&gProjects,mp)
			MakeFormatMapLinks(mp);
	}
}


void
RefreshPrefsTreeNode(struct TreeNode *tn)
{
	struct TreeNode *container;
	struct Mappe *mp;
	struct PrefsModule *pm = tn->tn_Special;

	if (!(container = GetTreeContainer(tn)))
		return;

	mp = container->tn_Special;
	pm->pm_Flags = (pm->pm_Flags & ~PMF_ADD_REPLACE_MASK) | (tn->tn_Flags & TNF_ADD ? PMF_ADD : PMF_REPLACE);

	RefreshPrefsModule(GetLocalPrefs(mp), pm, 0);
}


void
UpdateAntiFont(struct Prefs *pr)
{
	struct PrefDisp *pd = pr->pr_Disp;

	if (pd->pd_AntiFont)
		CloseFont(pd->pd_AntiFont);

	pd->pd_AntiFont = OpenDiskFont(&pd->pd_AntiAttr);
	itext.ITextFont = &pd->pd_AntiAttr;
	itext.IText = "AAA";
	pd->pd_AntiWidth = IntuiTextLength(&itext)+20;
	pd->pd_AntiHeight = pd->pd_AntiAttr.ta_YSize+4;
}


void PUBLIC
AppCmdLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags))
{
	switch (flags & LNCMDS) {
		case LNCMD_LOCK:
			GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,~0L,TAG_END);
			break;
		case LNCMD_FREE:
			CloseAppWindow(ln->ln_Data[0],TRUE);
			break;

		default:
		{
			long i = ~0L;

			if (flags & LNF_ADD)
			{
				sortList(ln->ln_List);
				if (node)
					i = FindListEntry(ln->ln_List,node);
			}
			if (flags & (LNF_ADD | LNF_REMOVE))
				RefreshItemSize();
			((struct Gadget *)ln->ln_Data[1])->UserData = node;
			GT_SetGadgetAttrs(ln->ln_Data[1],ln->ln_Data[0],NULL,GTLV_Labels,ln->ln_List,GTLV_Selected,i,i != ~0L ? GTLV_MakeVisible : TAG_IGNORE,i,TAG_END);
			MakeCmdsLinks(ln->ln_Data[2]);
			// Tastatur- und Icon-Liste fehlt auch noch...
			break;
		}
	}
}


void
RefreshIconSizeFromList(struct PrefIcon *pi,struct MinList *list,BOOL link)
{
	struct ImageNode *in;
	struct Node *ln;

	foreach(list,ln)
	{
		if (link)
			in = ((struct Link *)ln)->l_Link;
		else
			in = (struct ImageNode *)ln;

		if (in->in_Image && in->in_Image != (APTR)~0L)
		{
			if (in->in_Image->Width+2 > pi->pi_Width)
				pi->pi_Width = in->in_Image->Width+2;
			if (in->in_Image->Height+2 > pi->pi_Height)
				pi->pi_Height = in->in_Image->Height+2;
		}
	}
}


void
RefreshIconSize(struct Prefs *pr)
{
	if (!HasPrefsModule(pr,WDT_PREFICON) || pr == &prefs)
		RefreshIconSizeFromList(prefs.pr_Icon,&prefs.pr_IconObjs,FALSE);
	else
		RefreshIconSizeFromList(pr->pr_Icon,&pr->pr_IconObjs,FALSE);
}


void
InitPrefs(struct Prefs *prefs)
{
	long   i;

	MyNewList(&prefs->pr_Formats);
	MyNewList(&prefs->pr_Names);
	MyNewList(&prefs->pr_AppCmds);
	MyNewList(&prefs->pr_AppMenus);
	MyNewList(&prefs->pr_AppKeys);
	MyNewList(&prefs->pr_IconObjs);
	MyNewList(&prefs->pr_Modules);

	// context menus

	for (i = 0; i < NUM_CMT; i++)
		MyNewList(&prefs->pr_Contexts[i]);

	// window bounds

	for (i = 0; i < NUM_WDT; i++)
	{
		prefs->pr_WinPos[i].Left = -1;
		prefs->pr_WinPos[i].Top = -1;
		prefs->pr_WinPos[i].Width = -1;
		prefs->pr_WinPos[i].Height = -1;
	}

	//prefs->pr_Flags = PRF_APPICON | PRF_CONTEXTMENU;
}


void
InitAppPrefs(STRPTR name)
{
	long i;

	MyNewList(&prefstree.tl_View);  MyNewList(&prefstree.tl_Tree);
	prefs.pr_TreeNode = AddTreeNode(pool,&prefstree.tl_Tree,GetString(&gLocaleInfo, MSG_GLOBAL_PREFS),NULL,TNF_CONTAINER | TNF_OPEN);
	if ((recycledprefs.pr_TreeNode = AddTreeNode(pool, &prefstree.tl_Tree, GetString(&gLocaleInfo, MSG_CLIPBOARD_PREFS), NULL, TNF_CONTAINER)) != 0)
		recycledprefs.pr_TreeNode->tn_Special = (APTR)~0L;

	InitPrefs(&prefs);
	InitPrefs(&recycledprefs);

	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_COMMANDS_PREFS), "icons/prefs_cmds.icon", WDT_PREFCMDS, PMF_ADD);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_SCREEN_PREFS), "icons/prefs_screen.icon", WDT_PREFSCREEN, PMF_GLOBAL);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_DISPLAY_PREFS), "icons/prefs_display.icon", WDT_PREFDISP, 0);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_FILE_PREFS), "icons/prefs_file.icon", WDT_PREFFILE, PMF_GLOBAL);

	//AddPrefsModule(&prefs,"Drucker", "icons/prefs_printer.icon", WDT_PREFPRINTER, 0 /*vorerst*/| PMF_GLOBAL);

	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_FORMATS_PREFS), "icons/prefs_format.icon", WDT_PREFFORMAT, PMF_ADD);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_CONTEXT_MENU_PREFS), "icons/prefs_menu.icon", WDT_PREFCONTEXT, 0);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_MENU_PREFS), "icons/prefs_menu.icon", WDT_PREFMENU, 0);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_NAMES_PREFS), "icons/prefs_names.icon", WDT_PREFNAMES, PMF_ADD);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_PALETTE_PREFS), "icons/prefs_colors.icon", WDT_PREFCOLORS, PMF_GLOBAL);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_ICONBAR_PREFS), "icons/prefs_icon.icon", WDT_PREFICON, 0);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_SYSTEM_PREFS), "icons/prefs_sys.icon", WDT_PREFSYS, PMF_GLOBAL);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_SHEET_PREFS), "icons/prefs_keys.icon", WDT_PREFTABLE, PMF_GLOBAL);
	AddPrefsModule(&prefs, GetString(&gLocaleInfo, MSG_KEYS_PREFS), "icons/prefs_keys.icon", WDT_PREFKEYS, 0);

	//AddPrefsModule(&prefs, "Werkzeugsleiste", "icons/prefs_tool.icon", WDT_PREFTOOL);

	sortList(&prefs.pr_Modules); 
	AddPrefsModulesToTree(&prefs,&prefs.pr_TreeNode->tn_Nodes);
	InitTreeList(&prefstree);

	i = 0;
	if (!strchr(name,':'))
	{
#ifdef __amigaos4__
		BPTR lock;

		lock = Lock(name, ACCESS_READ);
#else
		BPTR progdir = CurrentDir(shelldir), lock;

		lock = Lock(name, ACCESS_READ);
		CurrentDir(progdir);
#endif
		if (lock)
			i = LoadPrefs(&prefs, NULL, lock, PRF_ALL);
	}
	if (!i)
		LoadPrefs(&prefs, name, (BPTR)NULL, PRF_ALL);

	RefreshIconSize(&prefs);

	if (IsListEmpty((struct List *)&prefs.pr_Formats))
	{
		AddFormat(&prefs.pr_Formats,"0",0,-1,-1,0L,FVF_NONE,FVT_VALUE);
		AddFormat(&prefs.pr_Formats,"$0",0,2,-1,0L,FVF_NONE,FVT_VALUE);
		AddFormat(&prefs.pr_Formats,"0 DM",0,2,-1,0L,FVF_NONE,FVT_VALUE);
		AddFormat(&prefs.pr_Formats,"0%",0,-1,-1,0L,FVF_NONE,FVT_PERCENT);
		AddFormat(&prefs.pr_Formats,"0 cm",0,-1,-1,0L,FVF_NONE,FVT_EINHEIT);
		AddFormat(&prefs.pr_Formats,"#d.#m.#y",0,0,-1,0L,FVF_NONE,FVT_DATE);
		AddFormat(&prefs.pr_Formats,"#d.#m.#Y",0,0,-1,0L,FVF_NONE,FVT_DATE);
		AddFormat(&prefs.pr_Formats,"#D.#M.#y",0,0,-1,0L,FVF_NONE,FVT_DATE);
		AddFormat(&prefs.pr_Formats,"#D.#M.#Y",0,0,-1,0L,FVF_NONE,FVT_DATE);
		AddFormat(&prefs.pr_Formats,"#d. %M, #Y",0,0,-1,0L,FVF_NONE,FVT_DATE);
		AddFormat(&prefs.pr_Formats,"#d. %m, #y",0,0,-1,0L,FVF_NONE,FVT_DATE);
		AddFormat(&prefs.pr_Formats,"%W",0,0,-1,0L,FVF_NONE,FVT_DATE);
		AddFormat(&prefs.pr_Formats,"#m:#S",0,0,-1,0L,FVF_NONE,FVT_TIME);
		AddFormat(&prefs.pr_Formats,"#h:#M:#S",0,0,-1,0L,FVF_NONE,FVT_TIME);
		AddFormat(&prefs.pr_Formats,"#H:#M:#S",0,0,-1,0L,FVF_NONE,FVT_TIME);
		AddFormat(&prefs.pr_Formats,"#hh #mm #ss",0,0,-1,0L,FVF_NONE,FVT_TIME);
		SortFormatList(&prefs.pr_Formats);
	}
	if (IsListEmpty((struct List *)&prefs.pr_Names))
	{
		AddName(&prefs.pr_Names,GetString(&gLocaleInfo, MSG_TRUE_NAME),"1",NMT_NONE,NULL);
		AddName(&prefs.pr_Names,GetString(&gLocaleInfo, MSG_FALSE_NAME),"0",NMT_NONE,NULL);
		AddName(&prefs.pr_Names,"pi","3.1415926536",NMT_NONE,NULL);
		AddName(&prefs.pr_Names,"e","2.718281828",NMT_NONE,NULL);
	}
	if (IsListEmpty((struct List *)&outputs))
		AddStandardOutput();
}


/** Aktualisiert alle Zellen sämtlicher Projekte */

void
RefreshPagesTexts(struct Mappe *mp,bool all)
{
	struct Page *page;
	struct tableField *tf;

	for (page = (APTR)mp->mp_Pages.mlh_Head;page->pg_Node.ln_Succ;page = (APTR)page->pg_Node.ln_Succ)
	{
		for (tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ)
		{
			if (all || (tf->tf_Type & (TFT_FORMULA | TFT_VALUE)))
			{
#ifdef __amigaos4__			//do not know the reason for this, but the original solution destroys data ;-)
				if(tf->tf_Original)
				{
					FreeString(tf->tf_Text);
					tf->tf_Text = tf->tf_Original;
					UpdateCellText(page, tf);
				}
#else
				FreeString(tf->tf_Text);
				tf->tf_Text = tf->tf_Original;
				UpdateCellText(page, tf);
#endif
			}
		}
		if (page->pg_Window)
			DrawTable(page->pg_Window);
	}
	if (!IsListEmpty((struct List *)&mp->mp_Pages))
		RecalcTableFields((struct Page *)mp->mp_Pages.mlh_Head);
}


void
RefreshMapTexts(struct Mappe *mp, BOOL all)
{
	if (mp == (APTR)~0L)
		return;

	SetBusy(true, BT_APPLICATION);
	if (mp)
		RefreshPagesTexts(mp, all);
	else {
		for (mp = (struct Mappe *)gProjects.mlh_Head; mp->mp_Node.ln_Succ; mp = (APTR)mp->mp_Node.ln_Succ)
			RefreshPagesTexts(mp, all);
	}
	SetBusy(false, BT_APPLICATION);
}

