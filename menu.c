/* Menu and context menu functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include <proto/gtdrag.h>

	extern char IgnTitle[100];		//ScreenTitle for ignition, must be unique for PubScreen handling
#endif


void PUBLIC ProjectsMenuLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags));


long numsessions = NUM_SESSION;
long cmis_num;


bool
CheckMenuItemSpecial(struct MenuItem *item, STRPTR t)
{
	struct IgnAppMenuEntry *ame;
	long   len;
	STRPTR s;

	if (!item || !(ame = GTMENUITEM_USERDATA(item)))
		return false;

	cmis_num = NUM_SESSION;

	if (!zstrnicmp(t, ame->am_AppCmd, len = strlen(t)) && (!*(s = ame->am_AppCmd+len) || isspace(*s))) {
		STRPTR s = ame->am_AppCmd + len;

		for (; *s; s++) {
			if (isdigit(*s)) {
				cmis_num = atol(s);
				if (cmis_num > 20)
					cmis_num = 20;
				break;
			}
		}
		return true;
	}
	return false;
}


struct MenuItem *
FindMenuSpecial(struct Prefs *pr, STRPTR t, APTR *super, UBYTE *itemtype)
{
	struct Menu *menu;
	struct MenuItem *item,*sub,*special = NULL;
	APTR   parent;
	uint8  type;

	for (menu = pr->pr_Menu; menu; menu = menu->NextMenu) {
		parent = menu;
		type = MST_FIRSTITEM;

		for (item = menu->FirstItem; item; item = item->NextItem) {
			if (!item->SubItem) {
				if (CheckMenuItemSpecial(item, t)) {
					special = item;
					break;
				}
			} else {
				type = MST_FIRSTSUBITEM;
				parent = item;

				for (sub = item->SubItem;sub;sub = sub->NextItem) {
					if (CheckMenuItemSpecial(sub, t)) {
						special = sub;
						break;
					}
					type = MST_ITEM;
					parent = sub;
				}
			}
			if (special)
				break;

			type = MST_ITEM;
			parent = item;
		}
		if (special)
			break;
	}

	if (special) {
		*super = parent;
		*itemtype = type;

		return special;
	}
	return NULL;
}


void
FreeMenuList(struct MenuSpecial *ms)
{
	struct MenuItem *item, *next, *stop, *special = ms->ms_Item;
	APTR parent = ms->ms_Parent;
	UBYTE  type = ms->ms_Type;

	if (!parent || !special)
		return;

	stop = special->NextItem;

	if (type == MST_FIRSTITEM) {
		item = ((struct Menu *)parent)->FirstItem;
		((struct Menu *)parent)->FirstItem = special;
	} else if (type == MST_FIRSTSUBITEM) {
		item = ((struct MenuItem *)parent)->SubItem;
		((struct MenuItem *)parent)->SubItem = special;
	} else {
		item = ((struct MenuItem *)parent)->NextItem;
		((struct MenuItem *)parent)->NextItem = special;
	}

	for (;item != stop; item = next) {
		next = item->NextItem;

		if (item->Flags & ITEMTEXT) {
			FreeString((STRPTR)((struct IntuiText *)item->ItemFill)->IText);
			FreePooled(pool, item->ItemFill, sizeof(struct IntuiText));
		}
		FreePooled(pool, item, sizeof(struct SpecialMenuItem));
	}
}


UNUSED static struct MenuItem *
FindSpecialMenuItem(struct MenuItem *first, STRPTR label)
{
	struct MenuItem *item = first;
	
	while (item) {
		// ToDo: check item type
		//if (item->
		if (!zstrcmp((STRPTR)((struct IntuiText *)item->ItemFill)->IText, label))
			return item;

		item = item->NextItem;
	}
	
	return NULL;
}

 
struct MenuItem *
MakeMenuList(struct MinList *list, APTR parent, UBYTE type, ULONG id, struct MenuItem *next)
{
	struct MenuItem *first = NULL, *item, *old = NULL;
	struct Node *ln;
	LONG   count = 0;

	if (!parent || !list)
		return NULL;

	foreach (list, ln) {
		if (ln->ln_Name && (item = AllocPooled(pool, sizeof(struct SpecialMenuItem)))) {
			if (!first)
				first = item;
			else
				old->NextItem = item;

			if ((item->ItemFill = AllocPooled(pool, sizeof(struct IntuiText))) != 0) {
				// Find entries with the same name
				
				bool equalEntries = false;
				struct Node *node;
                 
				foreach (list, node) {
					if (node != ln && !zstrcmp(node->ln_Name, ln->ln_Name)) {
						equalEntries = true;
						break;
					}
				}

				if (equalEntries) {
					// if there is already an item with the same name, add path
					// informations so that the items can be differentiated better
					char t[256], path[25];

					{
						char *pathSource;
						if (id == SMIM_SESSION)
							pathSource = ((struct Session *)ln)->s_Path;
						else
							pathSource = ((struct Mappe *)ln)->mp_Path;
						
						if (pathSource != NULL)
#ifdef __amigaos4__
							Strlcpy(path, pathSource, sizeof(path));
#else
							stccpy(path, pathSource, sizeof(path));
#endif
						else
							path[0] = '\0';
					}

					if (strlen(path) > sizeof(path) - 2)
						strcpy(path + sizeof(path) - 4, "...");

					sprintf(t, "%s (%s)", ln->ln_Name, path);
					((struct IntuiText *)item->ItemFill)->IText = AllocString(t);
				} else
					((struct IntuiText *)item->ItemFill)->IText = AllocString(ln->ln_Name);

				((struct IntuiText *)item->ItemFill)->TopEdge = 1;
				item->Flags = ITEMTEXT | ITEMENABLED | HIGHCOMP;
				((struct SpecialMenuItem *)item)->smi_Magic = id;
				((struct SpecialMenuItem *)item)->smi_Data = count;
			}
			old = item;
		}
		count++;

		if (id == SMIM_SESSION && count >= numsessions)
			break;
	}

	if (!first)
		return NULL;
	else
		old->NextItem = next;

	switch (type) {
		case MST_FIRSTITEM:
			((struct Menu *)parent)->FirstItem = first;
			break;
		case MST_FIRSTSUBITEM:
			((struct MenuItem *)parent)->SubItem = first;
			break;
		default:
			((struct MenuItem *)parent)->NextItem = first;
			break;
	}

	return first;
}


void
RemoveMenuSpecials(struct Prefs *pr)
{
	/* remove in reverse order */

	if (pr->pr_SessionMenu.ms_Item)
		FreeMenuList(&pr->pr_SessionMenu);

	if (pr->pr_ProjectsMenu.ms_Lock) {
		RemLockNode(pr->pr_ProjectsMenu.ms_Lock);
		pr->pr_ProjectsMenu.ms_Lock = NULL;
		FreeMenuList(&pr->pr_ProjectsMenu);
	}
}


void
InsertMenuSpecials(struct Prefs *pr)
{
	uint8 type;

	if ((pr->pr_ProjectsMenu.ms_Item = FindMenuSpecial(pr, "PROJECTLIST", &pr->pr_ProjectsMenu.ms_Parent, &type)) != 0) {
		pr->pr_ProjectsMenu.ms_Lock = AddLockNode(&gProjects,0,ProjectsMenuLock,0);
		pr->pr_ProjectsMenu.ms_Type = type;

		if (!MakeMenuList(&gProjects, pr->pr_ProjectsMenu.ms_Parent, type, SMIM_PROJECTS, pr->pr_ProjectsMenu.ms_Item->NextItem))
			pr->pr_ProjectsMenu.ms_Item = NULL;   // Items nicht getauscht!
	}

	if ((pr->pr_SessionMenu.ms_Item = FindMenuSpecial(pr, "SESSIONLIST", &pr->pr_SessionMenu.ms_Parent, &type)) != 0) {
		pr->pr_SessionMenu.ms_Type = type;
		numsessions = cmis_num;

		if (!MakeMenuList(&sessions, pr->pr_SessionMenu.ms_Parent, type, SMIM_SESSION, pr->pr_SessionMenu.ms_Item->NextItem))
			pr->pr_SessionMenu.ms_Item = NULL;   // Items nicht getauscht!
	}
}


void
UpdateMenuSpecials(struct Prefs *pr)
{
	RemoveMenuSpecials(pr);
	InsertMenuSpecials(pr);
	LayoutMenus(pr->pr_Menu,vi,GTMN_NewLookMenus,TRUE,TAG_END);

	if (scr)
	{
		struct Window *win;

		for(win = scr->FirstWindow;win;win = win->NextWindow)
		{
#ifdef __amigaos4__
			if(win->UserPort != iport)	//Not a ignition then next
				continue;
#endif
			if (win->MenuStrip == pr->pr_Menu)
			{
				ClearMenuStrip(win);
				SetMenuStrip(win,pr->pr_Menu);
				break;
			}
		}
	}
}


void PUBLIC
ProjectsMenuLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags))
{
	switch(flags & LNCMDS)
	{
		case LNCMD_UNLOCK:
		case LNCMD_REFRESH:
		{
			struct Mappe *mp;

			SetBusy(TRUE,BT_APPLICATION);
			UpdateMenuSpecials(&prefs);

			foreach(&gProjects,mp)
			{
				if (mp->mp_Prefs.pr_Menu != prefs.pr_Menu)
					UpdateMenuSpecials(&mp->mp_Prefs);
			}
			SetBusy(FALSE,BT_APPLICATION);
			break;
		}
	}
}


void
FreeAppMenus(struct MinList *l)
{
	struct IgnAppMenu *am;
	struct IgnAppMenuEntry *ame,*same;

	while ((am = (APTR)MyRemHead(l)) != 0)
	{
		while ((ame = (APTR)MyRemHead(&am->am_Items)) != 0)
		{
			while ((same = (APTR)MyRemHead(&ame->am_Subs)) != 0)
			{
				FreeString(same->am_Node.ln_Name);
				FreeString(same->am_AppCmd);
				FreePooled(pool,same,sizeof(struct IgnAppMenuEntry));
			}
			FreeString(ame->am_Node.ln_Name);
			FreeString(ame->am_AppCmd);
			FreePooled(pool,ame,sizeof(struct IgnAppMenuEntry));
		}
		FreeString(am->am_Node.ln_Name);
		FreePooled(pool,am,sizeof(struct IgnAppMenu));
	}
}


void
CopyAppMenus(struct MinList *from,struct MinList *to)
{
	struct IgnAppMenu *am,*sam;
	struct IgnAppMenuEntry *ame,*same,*mame,*smame;

	if (!from || !to)
		return;

	foreach(from,am)
	{
		if ((sam = AllocPooled(pool,sizeof(struct IgnAppMenu))) != 0)
		{
			sam->am_Node.ln_Name = AllocString(am->am_Node.ln_Name);
			MyNewList(&sam->am_Items);
			MyAddTail(to, sam);

			foreach(&am->am_Items,ame)
			{
				if ((same = AllocPooled(pool,sizeof(struct IgnAppMenuEntry))) != 0)
				{
					same->am_Node.ln_Name = AllocString(ame->am_Node.ln_Name);
					same->am_AppCmd = AllocString(ame->am_AppCmd);
					same->am_ShortCut = AllocString(ame->am_ShortCut);
					MyNewList(&same->am_Subs);
					MyAddTail(&sam->am_Items, same);

					foreach(&ame->am_Subs,mame)
					{
						if ((smame = AllocPooled(pool,sizeof(struct IgnAppMenuEntry))) != 0)
						{
							smame->am_Node.ln_Name = AllocString(mame->am_Node.ln_Name);
							smame->am_AppCmd = AllocString(mame->am_AppCmd);
							smame->am_ShortCut = AllocString(mame->am_ShortCut);
							MyNewList(&smame->am_Subs);
							MyAddTail(&same->am_Subs, smame);
						}
					}
				}
			}
		}
	}
}


struct Menu *
CreateAppMenu(struct Prefs *pr)
{
	struct Menu *menu = NULL;
	struct NewMenu *nmenu;
	struct IgnAppMenu *am;
	struct IgnAppMenuEntry *amItem,*amSub;
	long   i = 1;

	/** NewMenu-Arraygröße berechnen **/

	for(am = (struct IgnAppMenu *)pr->pr_AppMenus.mlh_Head;am->am_Node.ln_Succ;i++, am = (struct IgnAppMenu *)am->am_Node.ln_Succ)
		for(amItem = (struct IgnAppMenuEntry *)am->am_Items.mlh_Head;amItem->am_Node.ln_Succ;i++, amItem = (struct IgnAppMenuEntry *)amItem->am_Node.ln_Succ)
			for(amSub = (struct IgnAppMenuEntry *)amItem->am_Subs.mlh_Head;amSub->am_Node.ln_Succ;i++, amSub = (struct IgnAppMenuEntry *)amSub->am_Node.ln_Succ);

	/** Array nach Preferences füllen **/

	if ((nmenu = AllocPooled(pool, sizeof(struct NewMenu) * i)) != 0)
	{
		for(i = 0,am = (struct IgnAppMenu *)pr->pr_AppMenus.mlh_Head;am->am_Node.ln_Succ;i++, am = (struct IgnAppMenu *)am->am_Node.ln_Succ)
		{
			(*(nmenu+i)).nm_Type = NM_TITLE;
			(*(nmenu+i)).nm_Label = am->am_Node.ln_Name;
			(*(nmenu+i)).nm_Flags = 0;
			(*(nmenu+i)).nm_MutualExclude = 0;
			foreach(&am->am_Items,amItem)
			{
				i++;
				(*(nmenu+i)).nm_Type = NM_ITEM;
				if (strcmp(amItem->am_Node.ln_Name,"-"))
					(*(nmenu+i)).nm_Label = amItem->am_Node.ln_Name;
				else
					(*(nmenu+i)).nm_Label = NM_BARLABEL;
				(*(nmenu+i)).nm_CommKey = amItem->am_ShortCut;
				(*(nmenu+i)).nm_Flags = (amItem->am_ShortCut && strlen(amItem->am_ShortCut) > 1) ? NM_COMMANDSTRING : 0;
				(*(nmenu+i)).nm_MutualExclude = 0;
				(*(nmenu+i)).nm_UserData = amItem;

				foreach(&amItem->am_Subs,amSub)
				{
					i++;
					(*(nmenu+i)).nm_Type = NM_SUB;
					if (strcmp(amSub->am_Node.ln_Name,"-"))
						(*(nmenu+i)).nm_Label = amSub->am_Node.ln_Name;
					else
						(*(nmenu+i)).nm_Label = NM_BARLABEL;
					(*(nmenu+i)).nm_CommKey = amSub->am_ShortCut;
					(*(nmenu+i)).nm_Flags = (amSub->am_ShortCut && strlen(amSub->am_ShortCut) > 1) ? NM_COMMANDSTRING : 0;
					(*(nmenu+i)).nm_MutualExclude = 0;
					(*(nmenu+i)).nm_UserData = amSub;
				}
			}
		}
		nmenu[i].nm_Type = NM_END;
		menu = CreateMenus(nmenu,TAG_END);

		FreePooled(pool, nmenu, (i + 1) * sizeof(struct NewMenu));
	}
	pr->pr_Menu = menu;
	InsertMenuSpecials(pr);

	return menu;
}


void
FreeAppMenu(struct Prefs *pr)
{
	if (&prefs != pr && pr->pr_Menu == prefs.pr_Menu)
		return;

	RemoveMenuSpecials(pr);
	FreeMenus(pr->pr_Menu);
}


void
RefreshMenu(struct Prefs *pr)
{
	struct Mappe *mp = GetPrefsMap(pr),*rxmp = NULL;
	struct Window *win;
	struct winData *wd;

	if (scr && iport)
	{
		if (rxpage)
			rxmp = rxpage->pg_Mappe;

		for(win = scr->FirstWindow;win;win = win->NextWindow)
		{
			wd = (struct winData *)win->UserData;
			if (win->UserPort == iport && (mp && (wd->wd_Type == WDT_PROJECT && ((struct Page *)wd->wd_Data)->pg_Mappe == mp || wd->wd_Type != WDT_PROJECT && rxmp == mp)
																 || !mp && win->MenuStrip == prefs.pr_Menu))
				ClearMenuStrip(win);
		}
	}
	if (pr == &prefs || pr != &prefs && pr->pr_Menu != prefs.pr_Menu)
	{
		FreeAppMenu(pr);
		CreateAppMenu(pr);
		PropagatePrefsModule(pr,WDT_PREFMENU);
	}
	if (scr && iport)
	{
		LayoutMenus(pr->pr_Menu,vi,GTMN_NewLookMenus,TRUE,TAG_END);

		for(win = scr->FirstWindow;win;win = win->NextWindow)
		{
			wd = (struct winData *)win->UserData;
			if (win->UserPort == iport && (mp && (wd->wd_Type == WDT_PROJECT && ((struct Page *)wd->wd_Data)->pg_Mappe == mp || wd->wd_Type != WDT_PROJECT && rxmp == mp)
																 || !mp && (!rxmp || rxmp->mp_Prefs.pr_Menu == prefs.pr_Menu)))
				SetMenuStrip(win,pr->pr_Menu);
		}
	}
}


void
HandleMenu(void)
{
	struct IgnAppMenuEntry *ame;
	struct MenuItem *item;

	if ((item = ItemAddress(imsg.IDCMPWindow->MenuStrip,imsg.Code)) != 0) {
		if ((ame = GTMENUITEM_USERDATA(item)) != 0)
			ProcessAppCmd(rxpage,ame->am_AppCmd);  /* erstmal... (rxpage) */
		else {
			struct SpecialMenuItem *smi = (APTR)item;

			switch (smi->smi_Magic) {
				case SMIM_PROJECTS:
				{
					char t[42];

					sprintf(t,"current num=%ld show",smi->smi_Data);
					processIntCmd(t);
					break;
				}
				case SMIM_SESSION:
				{
					struct Session *s = (APTR)FindListNumber(&sessions,smi->smi_Data);
					struct Mappe *mp;
					long   count = 0;

					foreach (&gProjects, mp) {
						if (!zstricmp(s->s_Path,mp->mp_Path) && !stricmp(s->s_Node.ln_Name,mp->mp_Node.ln_Name))
							break;
						count++;
					}

					if (mp->mp_Node.ln_Succ) {
						char t[42];

						sprintf(t,"current num=%ld show",count);
						processIntCmd(t);
					} else {
						struct Mappe *mp;
						char t[420];

						strcpy(t,"load ");
						if (!(rxpage && (mp = rxpage->pg_Mappe) && (mp->mp_Flags & MPF_UNNAMED) && CountNodes(&mp->mp_Pages) == 1 && !mp->mp_Modified && IsListEmpty((struct List *)&rxpage->pg_Table) && !strcmp(GetString(&gLocaleInfo, MSG_NEW_DOCUMENT_NAME),mp->mp_Node.ln_Name)))
							strcat(t,"new ");
						strcat(t,"name=\"");
						strcat(t,s->s_Path ? s->s_Path : (STRPTR)"");
						AddPart(t,s->s_Node.ln_Name,420);
						strcat(t,"\"");

						if (processIntCmd(t) != RC_OK) {
							MyRemove(s);
							FreeSession(s);
							RefreshSession();
						}
					}
					break;
				}
			}
		}
	}
}

/******************************* Kontext-Menüs *******************************/

const struct NewContextMenu ncm_cell[] = {
	{MSG_CELL_SET_NAME_CONTEXT, "cellname req"},
	{-1, NULL},
	{MSG_CUT_CONTEXT, "cut"},
	{MSG_COPY_CONTEXT, "copy"},
	{MSG_PASTE_CONTEXT, "paste"},
	{0}
};

const struct NewContextMenu ncm_morecells[] = {
	{MSG_CUT_CONTEXT, "cut"},
	{MSG_COPY_CONTEXT, "copy"},
	{MSG_PASTE_CONTEXT, "paste"},
	{0}
};

const struct NewContextMenu ncm_horiztitle[] = {
	{MSG_TITLE_SET_CONTEXT, "title horiz req"},
	{MSG_TITLE_REMOVE_CONTEXT, "title horiz delete"},
	{-1, NULL},
	{MSG_TITLE_INSERT_COLUMN_CONTEXT, "cell insertcol"},
	{MSG_TITLE_OPTIMAL_WIDTH_CONTEXT, "cellsize optwidth"},
	{MSG_TITLE_CELL_SIZE_CONTEXT, "cellsize req"},
	{0}
};

const struct NewContextMenu ncm_verttitle[] = {
	{MSG_TITLE_SET_CONTEXT, "title vert req"},
	{MSG_TITLE_REMOVE_CONTEXT, "title vert delete"},
	{-1, NULL},
	{MSG_TITLE_INSERT_ROW_CONTEXT, "cell insertrow"},
	{MSG_TITLE_OPTIMAL_HEIGHT_CONTEXT, "cellsize optheight"},
	{MSG_TITLE_CELL_SIZE_CONTEXT, "cellsize req"},
	{0, 0}
};

const struct NewContextMenu ncm_object[] = {
//  {"Informationen...","cellname req"},
//  {"-",NULL},
	{MSG_CUT_CONTEXT, "cut"},
	{MSG_COPY_CONTEXT, "copy"},
	{MSG_PASTE_CONTEXT, "paste"},
	{0,0}
};


void
CalculateContextMenu(struct MinList *list)
{
	struct ContextMenu *cm;
	long   y = 0;

	foreach(list, cm) {
		cm->cm_Begin = y;

		if (strcmp("-", cm->cm_Node.ln_Name)) {
			cm->cm_End = y + fontheight;
			itext.IText = cm->cm_Node.ln_Name;
			cm->cm_Width = IntuiTextLength(&itext);
		} else {
			cm->cm_End = y + 5;
			cm->cm_Width = ~0L;
		}
		y = cm->cm_End + 1;
	}
}


void
RefreshContextMenus(struct Prefs *pr)
{
	long i;

	if (!HasPrefsModule(pr, WDT_PREFCONTEXT))
		return;

	// make sure the right font is used in the global itext
	if (scr != NULL)
		itext.ITextFont = scr->Font;

	for (i = 0;i < NUM_CMT;i++)
		CalculateContextMenu(&pr->pr_Contexts[i]);
}


void
FreeContextMenu(struct MinList *list)
{
	struct ContextMenu *cm;

	while ((cm = (APTR)MyRemHead(list)) != 0) {
		FreeString(cm->cm_Node.ln_Name);
		FreeString(cm->cm_AppCmd);
		FreePooled(pool,cm,sizeof(struct ContextMenu));
	}
}


struct ContextMenu *
AddContextMenu(struct MinList *list,STRPTR title,STRPTR cmd)
{
	struct ContextMenu *cm;

	if (!title)
		return NULL;

	if ((cm = AllocPooled(pool,sizeof(struct ContextMenu))) != 0) {
		cm->cm_Node.ln_Name = AllocString(title);
		/*if (!strcmp(title,"-"))
			cm->cm_Node.ln_Type = AMT_BARLABEL;*/
		cm->cm_AppCmd = AllocString(cmd);
		MyAddTail(list, cm);
	}
	return cm;
}


void
CopyContextMenu(struct MinList *from,struct MinList *to)
{
	struct ContextMenu *cm;

	foreach (from,cm)
		AddContextMenu(to,cm->cm_Node.ln_Name,cm->cm_AppCmd);
}


void
AddStandardMenu(struct Prefs *pr, long type, const struct NewContextMenu *ncm)
{
	long i, id;

	for (i = 0; (id = ncm[i].ncm_Title); i++)
		AddContextMenu(&pr->pr_Contexts[type], id > 0 ? GetString(&gLocaleInfo, id) : (STRPTR)"-", ncm[i].ncm_AppCmd);
}


void
InitContextMenus(struct Prefs *pr)
{
	long i;

	for (i = 0;i < NUM_CMT;i++)
		MyNewList(&pr->pr_Contexts[i]);

	if (pr == &prefs) {
		AddStandardMenu(pr,CMT_CELL,ncm_cell);
		AddStandardMenu(pr,CMT_HORIZTITLE,ncm_horiztitle);
		AddStandardMenu(pr,CMT_VERTTITLE,ncm_verttitle);
		AddStandardMenu(pr,CMT_OBJECT,ncm_object);
		AddStandardMenu(pr,CMT_MORECELLS,ncm_morecells);

		RefreshContextMenus(pr);
	}
}


void
DrawContextItem(struct RastPort *rp, struct ContextMenu *cm, long width, bool selected)
{
	long y;

	if (!cm)
		return;

	y = cm->cm_Begin+3;

	if (cm->cm_Width == ~0L) {
		SetAPen(rp, 2);
		RectFill(rp, 4, y, width+3, y+1);
		RectFill(rp, 4, y+4, width+3, y+5);

		SetABPenDrMd(rp, 2, 1, JAM2);
		rp->AreaPtrn = (unsigned short *)&ghostPtrn;
		rp->AreaPtSz = 1;
		RectFill(rp, 4, y+2, width+3, y+3);
		rp->AreaPtrn = NULL;
		rp->AreaPtSz = 0;
	} else {
		if (selected) {
			itext.BackPen = 1;
			itext.FrontPen = 2;
		} else {
			itext.BackPen = 2;
			itext.FrontPen = 1;
		}
		itext.IText = cm->cm_Node.ln_Name;
		itext.ITextFont = scr->Font;
		SetAPen(rp, itext.BackPen);
		RectFill(rp, 4, y, 4, y + fontheight);
		RectFill(rp, cm->cm_Width + 5, y, width + 3, y + fontheight);
		RectFill(rp, 5, y, 4 + cm->cm_Width, y);
		PrintIText(rp, &itext, 5, y + 1);
	}
}


void
PopUpContextMenu(struct MinList *list)
{
	struct Window *cwin;
	struct ContextMenu *cm;
	long   width = 0,height = 0;
	uint32 idcmp;

	if (!list || IsListEmpty((struct List *)list))
		return;

	idcmp = win->IDCMPFlags;
	ModifyIDCMP(win, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE);

	foreach (list,cm) {
		if (cm->cm_Width != ~0L && cm->cm_Width > width)
			width = cm->cm_Width;
		height = cm->cm_End;
	}
	width += 2;
	height++;
	cm = NULL;

	if ((cwin = OpenWindowTags(NULL,
			WA_Left,		imsg.MouseX+win->LeftEdge-(width >> 1),
			WA_Top,			imsg.MouseY+win->TopEdge+4,
			WA_Width,		width+18,
			WA_Height,		height+6,
			WA_BlockPen,	2,
			WA_NoCareRefresh, TRUE,
			WA_PubScreen,	scr,
			WA_Borderless,	TRUE,
#ifdef __amigaos4__
            WA_ScreenTitle,	IgnTitle,
#endif
			TAG_END)) != 0) {
		struct IntuiMessage *msg;
		struct RastPort *rp = cwin->RPort;
		long x, y;
		BYTE doit = TRUE;

		SetAPen(rp,2);
		RectFill(rp,0,0,cwin->Width-1,cwin->Height-1);
		SetAPen(rp,1);
		RectFill(rp,0,0,cwin->Width-1,1);
		RectFill(rp,0,cwin->Height-2,cwin->Width-1,cwin->Height-1);
		RectFill(rp,0,1,1,cwin->Height-2);
		RectFill(rp,cwin->Width-2,1,cwin->Width-1,cwin->Height-2);

		x = cwin->LeftEdge-win->LeftEdge+4;
		y = cwin->TopEdge-win->TopEdge+3;

		itext.DrawMode = JAM2;
		foreach (list,cm)
			DrawContextItem(rp, cm, width, FALSE);
		cm = NULL;

		while (doit) {
			WaitPort(iport);
			while ((msg = GTD_GetIMsg(iport)) != 0) {
				if (msg->Class == IDCMP_MOUSEMOVE) {
					if (msg->MouseX >= x && msg->MouseY >= y && msg->MouseX <= x+width && msg->MouseY < y+height) {
						struct ContextMenu *scm;

						foreach(list, scm) {
							if (msg->MouseY-y >= scm->cm_Begin && msg->MouseY-y <= scm->cm_End)
								break;
						}
						if (!scm->cm_Node.ln_Succ || scm->cm_Width == ~0L)
							scm = NULL;
						if (scm != cm) {
							if (cm)
								DrawContextItem(rp,cm,width,FALSE);

							DrawContextItem(rp,cm = scm,width,TRUE);
						}
					} else if (cm) {
						DrawContextItem(rp, cm, width, FALSE);
						cm = NULL;
					}
				} else if (msg->Class == IDCMP_MOUSEBUTTONS)
					doit = FALSE;

				GTD_ReplyIMsg(msg);
			}
		}
		CloseWindow(cwin);
	}
	ModifyIDCMP(win, idcmp);
	itext.FrontPen = 1;
	itext.BackPen = 0;

	if (cm)
		ProcessAppCmd(rxpage, cm->cm_AppCmd);
}


void
HandleContext(struct Page *page, int32 type, uint32 col, uint32 row)
{
	struct Page *savepage;
	struct Prefs *pr;

	if (!HasPrefsModule(pr = &page->pg_Mappe->mp_Prefs,WDT_PREFCONTEXT))
		pr = &prefs;
	if (IsListEmpty((struct List *)&pr->pr_Contexts[type]))
		return;

	if (type == CMT_CELL || type == CMT_VERTTITLE || type == CMT_HORIZTITLE) {
		SetMark(page,-1,0,0,0);

		if (type == CMT_VERTTITLE)
			col = page->pg_Gad.cp.cp_Col;
		else if (type == CMT_HORIZTITLE)
			row = page->pg_Gad.cp.cp_Row;

		if (page->pg_Gad.cp.cp_Col != col || page->pg_Gad.cp.cp_Row != row)
			CreateTabGadget(page, col, row, FALSE);
	}
	savepage = rxpage;
	rxpage = page;

	PopUpContextMenu(&pr->pr_Contexts[type]);

	rxpage = savepage;
}
