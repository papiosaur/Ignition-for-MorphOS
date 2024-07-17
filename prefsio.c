/* Preferences I/O functions (loading/saving prefs)
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"

#ifdef __amigaos4__
	#include <stdarg.h>
#endif


extern struct MinList io_fvs;


/*************************** ToolTypes ***************************/


void
FreeToolTypes(STRPTR *tooltype)
{
	if (!tooltype)
		return;

	FreeVec(tooltype);
}


STRPTR *
AllocToolTypes(long num)
{
#ifdef __amigaos4__
	return AllocVecTags((num + 1) * sizeof(STRPTR), AVT_Type, MEMF_PRIVATE, AVT_ClearWithValue, 0, TAG_DONE );
#else
	return AllocVec((num + 1) * sizeof(STRPTR), MEMF_CLEAR | MEMF_PUBLIC);
#endif
}


STRPTR *
CopyToolTypes(STRPTR *tooltype)
{
	STRPTR *tt;
	long   i,num;

	if (!tooltype)
		return NULL;

	for (num = 0; tooltype[num]; num++);

	if ((tt = AllocToolTypes(num)) != 0) {
		for (i = 0; i < num; i++)
			tt[i] = AllocString(tooltype[i]);
	}

	return tt;
}


void
SetToolType(STRPTR **tt, STRPTR tooltype, STRPTR data)
{
	long   i = 0,pos = -1,len;
	STRPTR *copy;

	if (!tooltype)
		return;

	if (*tt) {
		for (;(*tt)[i];i++) {
			if (!strnicmp((*tt)[i],tooltype,len = strlen(tooltype))
				&& (*((*tt)[i]+len) == '=' || !*((*tt)[i]+len)) || *((*tt)[i]) == '(' && *((*tt)[i]+len) == ')'
				&& !strnicmp((*tt)[i] + 1, tooltype, len) && strlen((*tt)[i]) == len + 2)
				pos = i;
		}
	}
	if (pos == -1 && data && (copy = AllocToolTypes(i+1))) {
		if (*tt) {
			for (i = 0;(*tt)[i];i++) {
				copy[i] = AllocString((*tt)[i]);
			}
			pos = i;
			FreeToolTypes(*tt);
		} else
			pos = 0;
		*tt = copy;
	}

	if (pos != -1) {
		if (data && ((*tt)[pos] = AllocPooled(pool, strlen(data) + strlen(tooltype) + 2)))
			sprintf((*tt)[pos],"%s=%s",tooltype,data);
		else if (!data && ((*tt)[pos] = AllocPooled(pool,strlen(tooltype)+3)))
			sprintf((*tt)[pos],"(%s)",tooltype);
	}
}


void
SetToolTypes(STRPTR name, STRPTR tt1, ...)
{
	struct DiskObject *dio;
	STRPTR *tt,*oldtt,*tooltype;

	if ((dio = GetDiskObject(name)) != 0)
	{
		if ((tt = CopyToolTypes(oldtt = dio->do_ToolTypes)) != 0)
		{
#ifdef __amigaos4__
			va_list args;
			STRPTR varg, *tooltype2, tt2;
		    char t1[90], t2[90];
			
			va_start(args, tt1);
			tooltype = &tt1;
			while(tt1 != TAG_END)
			{
				tt2 = va_arg(args, STRPTR);
				SetToolType(&tt, tt1, tt2);
				tt1 = va_arg(args, STRPTR);
			}
			//while(*tooltype != (STRPTR)TAG_END)
			//{
			    //
			    //*tooltype2 = va_arg(args, STRPTR);
				//strcpy(t1, *tooltype);
				//strcpy(t2, *tooltype2);
				//SetToolType(&tt, t1, t2);
				//*tooltype = va_arg(args, STRPTR);
			//}
			va_end(args);
#else
			for(tooltype = &tt1;*tooltype;tooltype += 2)
				SetToolType(&tt,*tooltype,*(tooltype+1));
#endif
			dio->do_ToolTypes = tt;
			PutDiskObject(name,dio);
			dio->do_ToolTypes = oldtt;

			FreeToolTypes(tt);
		}
		FreeDiskObject(dio);
	}
}


/*************************** Save Preferences ***************************/


static void
spDisp(struct IFFHandle *iff, struct Prefs *pr)
{
	UWORD store2;

	SetPrefsModule(pr, WDT_PREFDISP, FALSE);

	PushChunk(iff, ID_IGNP, ID_DISP, IFFSIZE_UNKNOWN);
	WriteChunkBytes(iff,&pr->pr_Disp->pd_Rasta,1);
	WriteChunkBytes(iff,&pr->pr_Disp->pd_HelpBar,1);
	WriteChunkBytes(iff,&pr->pr_Disp->pd_ToolBar,1);
	WriteChunkBytes(iff,&pr->pr_Disp->pd_IconBar,1);
	WriteChunkBytes(iff,&pr->pr_Disp->pd_FormBar,1);
	WriteChunkBytes(iff,&pr->pr_Disp->pd_ShowAntis,1);

	WriteChunkString(iff,(STRPTR)pr->pr_Disp->pd_AntiAttr.ta_Name);
	store2 = WORD2BE(pr->pr_Disp->pd_AntiAttr.ta_YSize);
	WriteChunkBytes(iff, &store2, 2);
	PopChunk(iff);
}


static void
spScreen(struct IFFHandle *iff)
{
	UBYTE pad=0;
	ULONG store4;
	UWORD store2;

	SetPrefsModule(&prefs, WDT_PREFSCREEN, FALSE);
	PushChunk(iff, ID_IGNP, ID_SCREEN, IFFSIZE_UNKNOWN);

	store2 = WORD2BE(prefs.pr_Screen->ps_Width);
	WriteChunkBytes(iff, &store2, 2);

	store2 = WORD2BE(prefs.pr_Screen->ps_Height);
	WriteChunkBytes(iff, &store2, 2);

	store2 = WORD2BE(prefs.pr_Screen->ps_Depth);
	WriteChunkBytes(iff, &store2, 2);

	store2 = WORD2BE(prefs.pr_Screen->ps_dimWidth);
	WriteChunkBytes(iff, &store2, 2);

	store2 = WORD2BE(prefs.pr_Screen->ps_dimHeight);
	WriteChunkBytes(iff, &store2, 2);

	store4 = LONG2BE(prefs.pr_Screen->ps_ModeID);
	WriteChunkBytes(iff, &store4, 4);

	store4 = LONG2BE(prefs.pr_Screen->ps_Overscan);
	WriteChunkBytes(iff, &store4, 4);

	WriteChunkBytes(iff, &prefs.pr_Screen->ps_Interleaved, 1);

	WriteChunkBytes(iff, &pad, 1);

	store2 = WORD2BE(prefs.pr_Screen->ps_Type);
	WriteChunkBytes(iff, &store2, 2);

	WriteChunkBytes(iff, prefs.pr_Screen->ps_PubName, PS_NAMELEN);

	WriteChunkBytes(iff, &prefs.pr_Screen->ps_BackFill, 1);

	WriteChunkBytes(iff, &pad, 1);

	store4 = LONG2BE(prefs.pr_Screen->ps_BFColor);
	WriteChunkBytes(iff, &store4, 4);

	store4 = LONG2BE(prefs.pr_Screen->ps_mmWidth);
	WriteChunkBytes(iff, &store4, 4);

	store4 = LONG2BE(prefs.pr_Screen->ps_mmHeight);
	WriteChunkBytes(iff, &store4, 4);

	WriteChunkString(iff, (STRPTR)prefs.pr_Screen->ps_TextAttr.ta_Name);

	store2 = WORD2BE(prefs.pr_Screen->ps_TextAttr.ta_YSize);
	WriteChunkBytes(iff, &store2, 2);

	PopChunk(iff);
}


static void
spFile(struct IFFHandle *iff)
{
	BPTR   olddir;
	ULONG  store4;
	UWORD  store2;
	UBYTE  pad=0;

	SetPrefsModule(&prefs,WDT_PREFFILE,FALSE);
	PushChunk(iff,ID_IGNP,ID_FILE,IFFSIZE_UNKNOWN);

	store2 = WORD2BE(prefs.pr_File->pf_Flags);
	WriteChunkBytes(iff, &store2, 2);
	WriteChunkBytes(iff, &prefs.pr_File->pf_AutoSave, 1);
	WriteChunkBytes(iff, &pad, 1);
	store4 = LONG2BE(prefs.pr_File->pf_AutoSaveIntervall);
	WriteChunkBytes(iff, &store4, 4);

	if (sm)
	{
#ifdef __amigaos4__
		olddir = SetCurrentDir(sm->sm_ArgList[0].wa_Lock);
		SetToolTypes(sm->sm_ArgList[0].wa_Name, "SHEETS", projpath, "GRAPHICS",graphicpath, "ICONS", iconpath, TAG_END);
		SetCurrentDir(olddir);
#else
		olddir = CurrentDir(sm->sm_ArgList[0].wa_Lock);
		SetToolTypes(sm->sm_ArgList[0].wa_Name,
			"SHEETS",  strcmp(projpath,"sheets/") ? projpath : NULL,
			"GRAPHICS",strcmp(graphicpath,"graphic/") ? graphicpath : NULL,
			"ICONS",   strcmp(iconpath,"icons/") ? iconpath : NULL,
			TAG_END);
		CurrentDir(olddir);
#endif
	}
	PopChunk(iff);
}


static void
spIcon(struct IFFHandle *iff,struct Prefs *pr)
{
	struct IconObj *io;

	SetPrefsModule(pr,WDT_PREFICON,FALSE);
	if (IsListEmpty((struct List *)&pr->pr_IconObjs))
		return;
	PushChunk(iff,ID_IGNP,ID_ICON,IFFSIZE_UNKNOWN);
	for(io = (APTR)pr->pr_IconObjs.mlh_Head;io->io_Node.in_Succ;io = (APTR)io->io_Node.in_Succ)
	{
		WriteChunkString(iff,(io->io_AppCmd && strcmp(io->io_AppCmd,GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD))) ? io->io_AppCmd : NULL);
	}
	PopChunk(iff);
}


static void
spColors(struct IFFHandle *iff)
{
	struct colorPen *cp;

	SetPrefsModule(&prefs,WDT_PREFCOLORS,FALSE);
	if (IsListEmpty((struct List *)&colors))
		return;
	PushChunk(iff,ID_IGNP,ID_COLORS,IFFSIZE_UNKNOWN);
	for(cp = (APTR)colors.mlh_Head;cp->cp_Node.ln_Succ;cp = (APTR)cp->cp_Node.ln_Succ)
	{
		WriteChunkBytes(iff,&cp->cp_Red,1);
		WriteChunkBytes(iff,&cp->cp_Green,1);
		WriteChunkBytes(iff,&cp->cp_Blue,1);
		WriteChunkString(iff,!cp->cp_Node.ln_Type ? cp->cp_Node.ln_Name : NULL);
	}
	PopChunk(iff);
}


static void
spKeys(struct IFFHandle *iff,struct Prefs *pr)
{
	struct AppKey *ak;
	ULONG store4;
	UWORD store2;

	SetPrefsModule(pr,WDT_PREFKEYS,FALSE);
	if (IsListEmpty((struct List *)&pr->pr_AppKeys))
		return;
	PushChunk(iff,ID_IGNP,ID_KEYS,IFFSIZE_UNKNOWN);
	foreach(&pr->pr_AppKeys,ak)
	{
		store4 = LONG2BE(ak->ak_Class);
		WriteChunkBytes(iff, &store4, 4);
		store2 = WORD2BE(ak->ak_Code);
		WriteChunkBytes(iff, &store2, 2);
		store2 = WORD2BE(ak->ak_Qualifier);
		WriteChunkBytes(iff, &store2, 2);
		WriteChunkBytes(iff,&ak->ak_Node.ln_Type,1);
		WriteChunkString(iff,ak->ak_AppCmd);
	}
	PopChunk(iff);
}


static void
spMenu(struct IFFHandle *iff,struct Prefs *pr)
{
	struct IgnAppMenu *am;
	struct IgnAppMenuEntry *ame,*same;
	UBYTE  type;

	SetPrefsModule(pr,WDT_PREFMENU,FALSE);
	PushChunk(iff,ID_IGNP,ID_MENU,IFFSIZE_UNKNOWN);
	for(am = (APTR)pr->pr_AppMenus.mlh_Head;am->am_Node.ln_Succ;am = (APTR)am->am_Node.ln_Succ)
	{
		type = NM_TITLE;
		WriteChunkBytes(iff,&type,1);
		WriteChunkString(iff,am->am_Node.ln_Name);

		for(ame = (APTR)am->am_Items.mlh_Head;ame->am_Node.ln_Succ;ame = (APTR)ame->am_Node.ln_Succ)
		{
			type = NM_ITEM;
			WriteChunkBytes(iff,&type,1);
			WriteChunkString(iff,ame->am_Node.ln_Name);
			WriteChunkString(iff,ame->am_ShortCut);
			WriteChunkString(iff,IsListEmpty((struct List *)&ame->am_Subs) ? ame->am_AppCmd : NULL);
			for(same = (APTR)ame->am_Subs.mlh_Head;same->am_Node.ln_Succ;same = (APTR)same->am_Node.ln_Succ)
			{
				type = NM_SUB;
				WriteChunkBytes(iff,&type,1);
				WriteChunkString(iff,same->am_Node.ln_Name);
				WriteChunkString(iff,same->am_ShortCut);
				WriteChunkString(iff,same->am_AppCmd);
			}
		}
	}
	PopChunk(iff);
}


static void
spCmds(struct IFFHandle *iff,struct Prefs *pr)
{
	struct AppCmd *ac;
	struct Command *cmd;
	UBYTE  pad = 0;

	SetPrefsModule(pr,WDT_PREFCMDS,FALSE);
	PushChunk(iff,ID_IGNP,ID_CMDS,IFFSIZE_UNKNOWN);
	for(ac = (APTR)pr->pr_AppCmds.mlh_Head;ac->ac_Node.in_Succ;ac = (APTR)ac->ac_Node.in_Succ)
	{
		WriteChunkString(iff,ac->ac_Node.in_Name);
		WriteChunkString(iff,ac->ac_ImageName);
		WriteChunkString(iff,ac->ac_Output);
		WriteChunkString(iff,ac->ac_HelpText);
		WriteChunkString(iff,ac->ac_Guide);
		for(cmd = (APTR)ac->ac_Cmds.mlh_Head;cmd->cmd_Succ;cmd = cmd->cmd_Succ)
		{
			WriteChunkString(iff,cmd->cmd_Name);
			WriteChunkBytes(iff,&cmd->cmd_Type,1);
		}
		WriteChunkBytes(iff,&pad,1);
	}
	PopChunk(iff);
}


static void
SaveAddReplaceMode(struct IFFHandle *iff, struct PrefsModule *pm)
{
	// save prefs module add/replace setting

	uint8 addReplace = PMF_ADD;

	if (pm)
		addReplace = pm->pm_Flags & PMF_ADD_REPLACE_MASK;

	WriteChunkBytes(iff, &addReplace, 1);
}


static long
spFormat(struct IFFHandle *iff, struct Prefs *pr, struct MinList *list)
{
	struct Mappe *mp = GetPrefsMap(pr);
	struct FormatVorlage *fv;
	struct Node *ln;
	long   error;

	if ((error = PushChunk(iff, 0, ID_FORMAT, IFFSIZE_UNKNOWN)) != 0)
		return error;

	SetPrefsModule(pr, WDT_PREFFORMAT, FALSE);
	SaveAddReplaceMode(iff, mp ? GetPrefsModule(pr, WDT_PREFFORMAT) : NULL);

	foreach (list, ln)
	{
		if (mp)
		{
			if (!(fv = (struct FormatVorlage *)FindLinkName(&mp->mp_Formats,ln->ln_Name)))
				continue;
		}
		else
			fv = (APTR)ln;

		WriteChunkString(iff, fv->fv_Node.ln_Name);
		WriteChunkBytes(iff, &fv->fv_Node.ln_Type, 1);
		WriteChunkBytes(iff, &fv->fv_Node.ln_Pri, 1);
		WriteChunkBytes(iff, &fv->fv_Komma, 1);
		WriteChunkBytes(iff, &fv->fv_Alignment, 1);
		WriteChunkBytes(iff, &fv->fv_Flags, 1);
		if (fv->fv_Flags & FVF_NEGATIVEPEN)
		{
			// FIXME endian?
			WriteChunkBytes(iff, (UBYTE *)&fv->fv_NegativePen + 1, 3);
		}
	}

	return PopChunk(iff);
}


static long
spNames(struct IFFHandle *iff,struct Prefs *pr,struct MinList *list)
{
	struct Mappe *mp = GetPrefsMap(pr);
	struct Name *nm;
	long   error,i;
	LONG store4;

	if ((error = PushChunk(iff,0,ID_NAMES,IFFSIZE_UNKNOWN)) != 0)
		return error;

	SetPrefsModule(pr, WDT_PREFNAMES, FALSE);
	SaveAddReplaceMode(iff, mp ? GetPrefsModule(pr, WDT_PREFFORMAT) : NULL);

	foreach (list, nm)
	{
		WriteChunkString(iff, nm->nm_Node.ln_Name);
		WriteChunkString(iff, nm->nm_Content);
		WriteChunkBytes(iff, &nm->nm_Node.ln_Type, 1);
		if (mp)
			i = FindListEntry(&mp->mp_Pages, (struct MinNode *)nm->nm_Page);
		else
			i = 0L;
		store4 = LONG2BE(i);
		WriteChunkBytes(iff, &store4, 4);
	}
	return PopChunk(iff);
}


static void
spTable(struct IFFHandle *iff,struct Prefs *pr)
{
	UBYTE pad = 0;
	UWORD store2;

	SetPrefsModule(pr, WDT_PREFTABLE, FALSE);
	PushChunk(iff, ID_IGNP, ID_TABLE, IFFSIZE_UNKNOWN);

	store2 = WORD2BE(pr->pr_Table->pt_Flags);
	WriteChunkBytes(iff, &store2, 2);
	WriteChunkBytes(iff, pr->pr_Table->pt_EditFunc, PTEQ_NUM);
	WriteChunkBytes(iff, &pad, 1);
	store2 = WORD2BE(calcflags);
	WriteChunkBytes(iff, &store2, 2);

	PopChunk(iff);
}


static void
spSystem(struct IFFHandle *iff)
{
	ULONG store4;

	SetPrefsModule(&prefs,WDT_PREFSYS,FALSE);
	PushChunk(iff,ID_IGNP,ID_SYSTEM,IFFSIZE_UNKNOWN);
	store4 = LONG2BE(prefs.pr_Flags);
	WriteChunkBytes(iff, &store4, 4);
	store4 = LONG2BE(clipunit);
	WriteChunkBytes(iff, &store4, 4);

	PopChunk(iff);
}


static void
spContext(struct IFFHandle *iff, struct Prefs *pr)
{
	struct ContextMenu *cm;
	long i;

	SetPrefsModule(pr, WDT_PREFCONTEXT, FALSE);
	PushChunk(iff, ID_IGNP, ID_CONTEXT, IFFSIZE_UNKNOWN);

	for (i = 0;i < NUM_CMT;i++) {
		foreach (&pr->pr_Contexts[i], cm) {
			if (cm->cm_Node.ln_Name) {
				WriteChunkString(iff, cm->cm_Node.ln_Name);
				WriteChunkString(iff, cm->cm_AppCmd);
			}
		}
		WriteChunkString(iff, NULL);
	}
	PopChunk(iff);
}


static void
spVersion(struct IFFHandle *iff)
{
	ULONG version = IGNP_VERSION;
	ULONG store4;

	PushChunk(iff, ID_IGNP, ID_VERSION, IFFSIZE_UNKNOWN);
	store4 = LONG2BE(version);
	WriteChunkBytes(iff, &store4, 4);
	PopChunk(iff);
}


long
SaveProjectPrefs(struct IFFHandle *iff, struct Prefs *pr)
{
	ULONG modules = pr->pr_ModuleFlags;
	long  rc = RC_OK;

	// if there are any formats used in this project, we save them with
	// the project itself, even if there was no format module before
	if (!IsListEmpty((struct List *)&io_fvs))
		modules |= PRF_FORMAT;

	if (modules & (PRF_DISPLAY | PRF_KEYS | PRF_ICON | PRF_CMDS | PRF_MENU | PRF_FORMAT | PRF_CONTEXT))
		spVersion(iff);

	if (PRF_DISPLAY & modules) spDisp(iff, pr);
	if (PRF_KEYS & modules)    spKeys(iff, pr);
	if (PRF_ICON & modules)    spIcon(iff, pr);
	if (PRF_CMDS & modules)    spCmds(iff, pr);
	if (PRF_MENU & modules)    spMenu(iff, pr);
	if (PRF_FORMAT & modules)  spFormat(iff, pr, &io_fvs);
	/* MERKER: Globale Namen fehlen da noch komplett... */
	if (PRF_NAMES & modules)   rc = spNames(iff, pr, &pr->pr_Names);
	if (PRF_CONTEXT & modules) spContext(iff, pr);

	return rc;
}


void
SavePrefs(struct Prefs *pr, STRPTR name, long flags)
{
	struct IFFHandle *iff;

	if ((iff = AllocIFF()) != 0) {
		if ((iff->iff_Stream = (IPTR)Open(name,MODE_NEWFILE)) != 0) {
			ULONG modules = pr->pr_ModuleFlags;

			InitIFFasDOS(iff);
			OpenIFF(iff,IFFF_WRITE);

			PushChunk(iff,ID_IGNP,ID_FORM,IFFSIZE_UNKNOWN);

			spVersion(iff);
			if ((flags & PRF_DISPLAY) & modules) spDisp(iff,pr);
			if ((flags & PRF_COLORS) & modules)  spColors(iff);
			if ((flags & PRF_SCREEN) & modules)  spScreen(iff);
			if ((flags & PRF_KEYS) & modules)    spKeys(iff,pr);
			if ((flags & PRF_ICON) & modules)    spIcon(iff,pr);
			if ((flags & PRF_CMDS) & modules)    spCmds(iff,pr);
			if ((flags & PRF_MENU) & modules)    spMenu(iff,pr);
			if ((flags & PRF_FILE) & modules)    spFile(iff);
			if ((flags & PRF_FORMAT) & modules)  spFormat(iff,pr,&pr->pr_Formats);
			if ((flags & PRF_NAMES) & modules)   spNames(iff,pr,&pr->pr_Names);
			if ((flags & PRF_TABLE) & modules)   spTable(iff,pr);
			if ((flags & PRF_SYSTEM) & modules)  spSystem(iff);
			if ((flags & PRF_CONTEXT) & modules) spContext(iff,pr);

			PopChunk(iff);

			CloseIFF(iff);
			Close((BPTR)iff->iff_Stream);
		}
		FreeIFF(iff);
	}
}


/*************************** Load Preferences ***************************/


static void
lpPreamble(struct Prefs *pr,UWORD type,BOOL set)
{
	struct Window *win;

	if ((win = GetPrefsWindow(pr,type)) != 0)
		CloseAppWindow(win,TRUE);

	if (&prefs != pr)
		AddPrefsModuleToLocalPrefs(GetPrefsMap(pr),type);

	if (set)
		SetPrefsModule(pr,type,FALSE);
}


void
lpDisp(struct IFFHandle *iff, LONG context, struct Prefs *pr)
{
	struct StoredProperty *sp;
	STRPTR t;

	if ((sp = FindProp(iff, context, ID_DISP)) != 0) {
		lpPreamble(pr, WDT_PREFDISP, TRUE);

		FreeString((STRPTR)pr->pr_Disp->pd_AntiAttr.ta_Name);

		t = sp->sp_Data;
		pr->pr_Disp->pd_Rasta = *t++;
		pr->pr_Disp->pd_HelpBar = *t++;
		pr->pr_Disp->pd_ToolBar = *t++;
		pr->pr_Disp->pd_IconBar = *t++;
		pr->pr_Disp->pd_FormBar = *t++;
		pr->pr_Disp->pd_ShowAntis = *t++;
		pr->pr_Disp->pd_AntiAttr.ta_Name = AllocString(t);
		t += strlen(t) + 1;
		pr->pr_Disp->pd_AntiAttr.ta_YSize = BE2WORD(*(UWORD *)t);
		UpdateAntiFont(pr);

		RefreshPrefsModule(pr, NULL, WDT_PREFDISP);
	}
}


static void
lpScreen(struct IFFHandle *iff)
{
	struct StoredProperty *sp;
	STRPTR t;

	if ((sp = FindProp(iff, ID_IGNP, ID_SCREEN)) != 0) {
		lpPreamble(&prefs, WDT_PREFSCREEN, TRUE);

		FreeString((STRPTR)prefs.pr_Screen->ps_TextAttr.ta_Name);

		t = sp->sp_Data;
		prefs.pr_Screen->ps_Width = BE2WORD(*(UWORD *)t);
		prefs.pr_Screen->ps_Height = BE2WORD(*(UWORD *)(t+2));
		prefs.pr_Screen->ps_Depth = BE2WORD(*(UWORD *)(t+4));
		prefs.pr_Screen->ps_dimWidth = BE2WORD(*(UWORD *)(t+6));
		prefs.pr_Screen->ps_dimHeight = BE2WORD(*(UWORD *)(t+8));
		prefs.pr_Screen->ps_ModeID = BE2LONG(*(LONG *)(t+10));
		prefs.pr_Screen->ps_Overscan = BE2LONG(*(LONG *)(t+14));
		prefs.pr_Screen->ps_Interleaved = *(t+18);
		prefs.pr_Screen->ps_Type = BE2WORD(*(WORD *)(t+20));
		CopyMem(t + 22, prefs.pr_Screen->ps_PubName, PS_NAMELEN);
		prefs.pr_Screen->ps_BackFill = *(t+86);
		prefs.pr_Screen->ps_BFColor = BE2LONG(*(LONG *)(t+88));
		prefs.pr_Screen->ps_mmWidth = BE2LONG(*(ULONG *)(t+92));
		prefs.pr_Screen->ps_mmHeight = BE2LONG(*(ULONG *)(t+96));
		t += 100;
		prefs.pr_Screen->ps_TextAttr.ta_Name = AllocString(t);
		t += strlen(t)+1;
		prefs.pr_Screen->ps_TextAttr.ta_YSize = BE2WORD(*(UWORD *)t);
		if (scr)
			ChangeAppScreen(TRUE);
	}
}


static void
lpFile(struct IFFHandle *iff)
{
	struct StoredProperty *sp;
	struct DiskObject *dio;
	BPTR   olddir;
	STRPTR t;

	if ((sp = FindProp(iff,ID_IGNP,ID_FILE)) != 0)
	{
		lpPreamble(&prefs,WDT_PREFFILE,TRUE);
		
		t=sp->sp_Data;
		prefs.pr_File->pf_Flags = BE2WORD(*(UWORD *)t);
		prefs.pr_File->pf_AutoSave = *(t+2);
		prefs.pr_File->pf_AutoSaveIntervall = BE2LONG(*(LONG *)(t+4));
	}
	if (sm)
	{
#ifdef __amigaos4__
		olddir = SetCurrentDir(sm->sm_ArgList[0].wa_Lock);
#else
		olddir = CurrentDir(sm->sm_ArgList[0].wa_Lock);
#endif
		if ((dio = GetDiskObject(sm->sm_ArgList[0].wa_Name)) != 0)
		{
			if ((t = FindToolType(dio->do_ToolTypes,"SHEETS")) != 0)
				FreeString(projpath),  projpath = AllocString(t);
			if ((t = FindToolType(dio->do_ToolTypes,"GRAPHICS")) != 0)
				FreeString(graphicpath),  graphicpath = AllocString(t);
			if ((t = FindToolType(dio->do_ToolTypes,"ICONS")) != 0)
				FreeString(iconpath),  iconpath = AllocString(t);
			FreeDiskObject(dio);
		}
#ifdef __amigaos4__
		SetCurrentDir(olddir);
#else
		CurrentDir(olddir);
#endif
	}
}


void
lpCmds(struct IFFHandle *iff, LONG context, struct Prefs *pr, BYTE add, BYTE keep)
{
	struct StoredProperty *sp;
	struct AppCmd *ac,*oac;
	struct Command *cmd;
	struct MinList list;
	STRPTR t;
	ULONG  pos = 0,i;

	if ((sp = FindProp(iff,context,ID_CMDS)) != 0)
	{
		{
			struct Window *win;

			while((win = GetAppWindow(WDT_DEFINECMD)) != 0)
				CloseAppWindow(win,TRUE);
		}
		lpPreamble(pr,WDT_PREFCMDS,!add);

		if (LockList(&pr->pr_AppCmds,LNF_REFRESH))
		{
			if (!add)
			{
				moveList(&pr->pr_AppCmds,&list);
				while((cmd = (struct Command *)MyRemHead(&outputs)) != 0)
				{
					FreeString(cmd->cmd_Name);
					FreePooled(pool,cmd,sizeof(struct Node));
				}
				AddStandardOutput();
			}
			for(t = sp->sp_Data;sp->sp_Size-1 > pos;)
			{
				if ((ac = NewAppCmd(NULL)))
				{
					ac->ac_Node.in_Name = AllocString(t);
					i = strlen(t)+1;  pos += i;  t += i;
					if ((ac->ac_ImageName = AllocString(t)))
						ac->ac_Node.in_Image = LoadImage(t);
					i = strlen(t)+1;  pos += i;  t += i;
					ac->ac_Output = AllocString(t);
					if (ac->ac_Output && !MyFindName(&outputs,t) && (cmd = AllocPooled(pool,sizeof(struct Node))))
					{
						cmd->cmd_Name = AllocString(t);
						MyAddTail(&outputs,(struct Node *)cmd);
					}
					i = strlen(t)+1;  pos += i;  t += i;
					ac->ac_HelpText = AllocString(t);
					i = strlen(t)+1;  pos += i;  t += i;
					ac->ac_Guide = AllocString(t);
					i = strlen(t)+1;  pos += i;  t += i;
					while(*t)
					{
						if ((cmd = AllocPooled(pool,sizeof(struct Command))) != 0)
						{
							cmd->cmd_Name = AllocString(t);
							i = strlen(t)+1;  pos += i;  t += i;
							cmd->cmd_Type = *t++;  pos++;
							MyAddTail(&ac->ac_Cmds, cmd);
						}
					}
					t++;  pos++;
					if (add && (oac = (APTR)MyFindName(&pr->pr_AppCmds, ac->ac_Node.in_Name)))
					{
						if (!keep)
						{
							MyRemove(oac);
							FreeAppCmd(oac);
							MyAddHead(&pr->pr_AppCmds, ac);
						}
					}
					else
						MyAddHead(&pr->pr_AppCmds, ac);
				}
				else
					pos = ~0L;
			}
			if (!add)
			{
				while((ac = (struct AppCmd *)MyRemHead(&list)) != 0)
					FreeAppCmd(ac);
			}
			sortList(&pr->pr_AppCmds); // GURU-MELDUNG
			UnlockList(&pr->pr_AppCmds,LNF_REFRESH);
		}
		RefreshPrefsModule(pr, NULL, WDT_PREFCMDS);
	}
}


void
lpMenu(struct IFFHandle *iff, LONG context, struct Prefs *pr)
{
	struct StoredProperty *sp;
	struct IgnAppMenu *am;
	struct IgnAppMenuEntry *ame,*same;
	STRPTR t;
	long   pos = 0;

	if ((sp = FindProp(iff, context, ID_MENU)) != 0)
	{
		lpPreamble(pr,WDT_PREFMENU,TRUE);
		FreeAppMenus(&pr->pr_AppMenus);

		for(t = sp->sp_Data; sp->sp_Size-1 > pos;)
		{
			if (*t == NM_TITLE && (am = AllocPooled(pool,sizeof(struct IgnAppMenu))))
			{
				am->am_Node.ln_Name = AllocString(t+1);
				pos += strlen(t+1)+2;  t = pos+(UBYTE *)sp->sp_Data;
				MyNewList(&am->am_Items);
				MyAddTail(&pr->pr_AppMenus, am);
			}
			if (*t == NM_ITEM && am && (ame = AllocPooled(pool,sizeof(struct IgnAppMenuEntry))))
			{
				ame->am_Node.ln_Name = AllocString(t+1);
				pos += strlen(t+1)+2;  t = pos+(UBYTE *)sp->sp_Data;
				ame->am_ShortCut = AllocString(t);
				pos += strlen(t)+1;  t = pos+(UBYTE *)sp->sp_Data;
				ame->am_AppCmd = AllocString(t);
				pos += strlen(t)+1;  t = pos+(UBYTE *)sp->sp_Data;
				MyNewList(&ame->am_Subs);
				MyAddTail(&am->am_Items, ame);
			}
			if (*t == NM_SUB && am && ame && (same = AllocPooled(pool,sizeof(struct IgnAppMenuEntry))))
			{
				same->am_Node.ln_Name = AllocString(t+1);
				pos += strlen(t+1)+2;  t = pos+(UBYTE *)sp->sp_Data;
				same->am_ShortCut = AllocString(t);
				pos += strlen(t)+1;  t = pos+(UBYTE *)sp->sp_Data;
				same->am_AppCmd = AllocString(t);
				pos += strlen(t)+1;  t = pos+(UBYTE *)sp->sp_Data;
				MyNewList(&same->am_Subs);
				MyAddTail(&ame->am_Subs, same);
			}
		}
		RefreshPrefsModule(pr, NULL, WDT_PREFMENU);
	}
}


void
lpKeys(struct IFFHandle *iff, LONG context, struct Prefs *pr)
{
	struct StoredProperty *sp;
	struct AppKey *ak;
	STRPTR t;
	long   pos = 0;

	if ((sp = FindProp(iff, context, ID_KEYS)) != 0)
	{
		lpPreamble(pr,WDT_PREFKEYS,TRUE);
		FreeAppKeys(&pr->pr_AppKeys);

		for(t = sp->sp_Data;sp->sp_Size-1 > pos;)
		{
			if ((ak = AllocPooled(pool, sizeof(struct AppKey))) != 0)
			{
				ak->ak_Class = BE2LONG(*(ULONG *)t);
				ak->ak_Code = BE2WORD(*(UWORD *)(t+4));
				ak->ak_Qualifier = BE2WORD(*(UWORD *)(t+6));
				ak->ak_Node.ln_Type = *(t+8);
				t += 9;
				if (*t)
					ak->ak_AppCmd = AllocString(t);
				pos += 10+strlen(t);  t += strlen(t)+1;
				SetAppKeyName(ak);
				MyAddTail(&pr->pr_AppKeys,ak);
			}
		}
		/** RefreshPrefsModule() bislang nicht notwendig **/
	}
}


void
lpColors(struct IFFHandle *iff, struct Prefs *pr)
{
	struct StoredProperty *sp;
	UBYTE  red,green,blue;
	STRPTR t;
	long   pos = 0;

	if ((sp = FindProp(iff,ID_IGNP,ID_COLORS)) != 0)
	{
		lpPreamble(pr,WDT_PREFCOLORS,TRUE);
		FreeAppColors();

		for(t = sp->sp_Data;sp->sp_Size-1 > pos;)
		{
			red = *t++;
			green = *t++;
			blue = *t++;
			AddColor(&colors,*t ? t : NULL,red,green,blue);
			pos += 4+strlen(t);  t += strlen(t)+1;
		}
		ObtainAppColors(scr,TRUE);
	}
}


void
lpIcon(struct IFFHandle *iff, LONG context, struct Prefs *pr)
{
	struct StoredProperty *sp;
	struct IconObj *io,leer;
	STRPTR t;
	long   pos = 0;

	if ((sp = FindProp(iff,context,ID_ICON)))
	{
		lpPreamble(pr,WDT_PREFICON,TRUE);

		leer.io_Node.in_Name = GetString(&gLocaleInfo, MSG_EMPTY_FIELD_GAD);
		leer.io_Node.in_Image = NULL;
		leer.io_AppCmd = NULL;
		FreeIconObjs(&pr->pr_IconObjs);

		for (t = sp->sp_Data; sp->sp_Size-1 > pos;)
		{
			if ((io = AllocPooled(pool, sizeof(struct IconObj))) != 0)
			{
				if (*t)
				{
					struct AppCmd *ac = (APTR)MyFindName(&(pr->pr_AppCmds), t);

					if (!ac && pr != &prefs)
						ac = (APTR)MyFindName(&prefs.pr_AppCmds, t);

					AddIconObj(&pr->pr_IconObjs,ac,-1);
				}
				else
					AddIconObj(&pr->pr_IconObjs,(struct AppCmd *)&leer,-1);
				pos += strlen(t) + 1;  t = pos + (UBYTE *)sp->sp_Data;
			}
		}
		RefreshPrefsModule(pr, NULL, WDT_PREFICON);
	}
}


static void
LoadAddReplaceMode(struct PrefsModule *pm, STRPTR *t)
{
	// load prefs module add/replace setting

	uint8 addReplace = (uint8)(**t) & PMF_ADD_REPLACE_MASK;
	(*t)++;

	if (pm) {
		if ((addReplace & PMF_ADD_REPLACE_MASK) == 0)
			addReplace = PMF_ADD;

		pm->pm_Flags = (pm->pm_Flags & ~PMF_ADD_REPLACE_MASK) | addReplace;
	}
}


void
lpFormat(struct IFFHandle *iff, LONG context, struct Prefs *pr, struct MinList *list)
{
	BYTE   type, komma, align, flags, pri;
	struct Mappe *mp = GetPrefsMap(pr);
	struct StoredProperty *sp;
	struct FormatVorlage *fv;
	long   pos = 0;
	STRPTR s, t;
	ULONG  id;

	if (!(sp = FindProp(iff, context, ID_FORMAT)))
		return;

	lpPreamble(pr, WDT_PREFFORMAT, TRUE);
	/* MERKER: muß das Fenster wirklich noch geschlossen werden? */

	while ((fv = (struct FormatVorlage *)MyRemHead(&pr->pr_Formats)) != 0)
		FreeFormat(fv);

	t = sp->sp_Data;
	LoadAddReplaceMode(mp ? GetPrefsModule(pr, WDT_PREFFORMAT) : NULL, &t);

	for (; sp->sp_Size-1 > pos;)
	{
		s = t;  t += strlen(t)+1;  pos += t-s;
		type = *t++;
		pri = *t++;
		komma = *t++;
		align = *t++;
		flags = *t++;
		pos += 5;
		if (flags & FVF_NEGATIVEPEN)
		{
			id = FindColorPen(t[0],t[1],t[2]);
			pos += 3;  t += 3;
		}
		else
			id = 0L;

		AddFormat(&pr->pr_Formats, s, pri, komma, align ? align : 1, id, flags, type);

		if (list)
		{
			struct Node *ln;

			if ((ln = AllocPooled(pool, sizeof(struct Node))))
			{
				ln->ln_Name = AllocString(s);
				MyAddTail(list, ln);
			}
		}
	}
	SortFormatList(&pr->pr_Formats);

	if (context == ID_TABL)  // doppelte Einträge elimineren (globale und lokale Formate)
	{
		struct FormatVorlage *sfv = (APTR)prefs.pr_Formats.mlh_Head, *nfv;

		for (fv = (APTR)pr->pr_Formats.mlh_Head; (nfv = (APTR)fv->fv_Node.ln_Succ); fv = nfv)  // Vergleich über zwei sortierte Listen
		{
			while (sfv->fv_Node.ln_Succ && FormatSort((struct Node **)&sfv, (struct Node **)&fv) < 0) {
				sfv = (APTR)sfv->fv_Node.ln_Succ;
			}
  
			if (!sfv->fv_Node.ln_Succ)
				break;
 
			if (!FormatSort((struct Node **)&sfv, (struct Node **)&fv)
				&& sfv->fv_Alignment == fv->fv_Alignment
				&& sfv->fv_Flags == fv->fv_Flags
				&& sfv->fv_Komma == fv->fv_Komma
				&& sfv->fv_NegativePen == fv->fv_NegativePen)
			{
				MyRemove(fv);
				FreeFormat(fv);
				continue;
			}
			sfv = (APTR)sfv->fv_Node.ln_Succ;
		}
		if (IsListEmpty((struct List *)&pr->pr_Formats))
			RemPrefsModule(pr, GetPrefsModule(pr, WDT_PREFFORMAT));
	}
	RefreshPrefsModule(pr, NULL, WDT_PREFFORMAT);
}


void
lpNames(struct IFFHandle *iff, LONG context, struct Prefs *pr)
{
	struct StoredProperty *sp;
	struct Mappe *mp = GetPrefsMap(pr);
	struct Name *nm;
	long   pos = 0,i;
	STRPTR s,t;
	BYTE   pages = FALSE, type;
	ULONG  page;

	if ((sp = FindProp(iff, context, ID_NAMES)) == NULL)
		return;

	lpPreamble(pr, WDT_PREFNAMES, TRUE);

	while ((nm = (struct Name *)MyRemHead(&pr->pr_Names)) != 0)
		FreeName(nm);

	if (mp)
		pages = !IsListEmpty((struct List *)&mp->mp_Pages);

	t = sp->sp_Data;
	LoadAddReplaceMode(mp ? GetPrefsModule(pr, WDT_PREFFORMAT) : NULL, &t);

	for (; sp->sp_Size-1 > pos;) {
		s = t;  i = strlen(t)+1;  pos += i;  s += i;
		i = strlen(s)+1;  pos += i + 5;  type = s[i];
		page = BE2LONG(*(ULONG *)(s + i + 1));
		
		if ((type & NMT_TYPEMASK) > NMT_DATABASE || (type & NMT_TYPEMASK) < NMT_NONE)
			break;

		AddName(&pr->pr_Names, t, s, type | (mp && !pages ? NMT_UNDEFINED : NMT_NONE),
			pages ? (APTR)FindListNumber(&mp->mp_Pages, page) : (APTR)page);
		t = s;  t += i + 5;
	}
	sortList(&pr->pr_Names);
	RefreshPrefsModule(pr, NULL, WDT_PREFNAMES);
}


void
lpTable(struct IFFHandle *iff,struct Prefs *pr)
{
	struct StoredProperty *sp;
	STRPTR t;

	if ((sp = FindProp(iff,ID_IGNP,ID_TABLE)) != 0)
	{
		lpPreamble(pr,WDT_PREFTABLE,TRUE);

		t = sp->sp_Data;
		pr->pr_Table->pt_Flags = BE2WORD(*(UWORD *)t);
		CopyMem(t + 2, pr->pr_Table->pt_EditFunc, PTEQ_NUM);
		if (sp->sp_Size > 8)
			calcflags = BE2WORD(*(UWORD *)(t+8));
	}
}


void
lpSystem(struct IFFHandle *iff)
{
	struct StoredProperty *sp;
	STRPTR t;

	if ((sp = FindProp(iff,ID_IGNP,ID_SYSTEM)) != 0)
	{
		lpPreamble(&prefs,WDT_PREFSYS,TRUE);

		t = sp->sp_Data;
		prefs.pr_Flags = BE2LONG(*(ULONG *)t);
		clipunit = BE2LONG(*(ULONG *)(t+4));
	}
}


void
lpContext(struct IFFHandle *iff, LONG context, struct Prefs *pr)
{
	struct StoredProperty *sp;
	long   i,pos,len;
	STRPTR t;

	if ((sp = FindProp(iff,context,ID_CONTEXT)) != 0)
	{
		lpPreamble(pr,WDT_PREFCONTEXT,TRUE);
		for(i = 0;i < NUM_CMT;i++)
			FreeContextMenu(&pr->pr_Contexts[i]);

		for(i = 0,pos = 0,t = sp->sp_Data;i < NUM_CMT && pos < sp->sp_Size;)
		{
			pos += (len = strlen(t));

			if (len)
			{
				AddContextMenu(&pr->pr_Contexts[i],t,t+len+1);
				t += len+1;  pos += (len = strlen(t));  t += len+1;
			}
			else
				i++, pos++, t++;
		}
		RefreshPrefsModule(pr,NULL,WDT_PREFCONTEXT);
	}
}


void
lpVersion(struct IFFHandle *iff)
{
	struct StoredProperty *sp;
	ULONG  version = 0;

	if ((sp = FindProp(iff, ID_IGNP, ID_VERSION)) != 0)
		version = BE2LONG(*(ULONG *)sp->sp_Data);
	if (version > IGNP_VERSION)
		ErrorRequest(GetString(&gLocaleInfo, MSG_PREFERENCES_VERSION_ERR));
}


void
PropPrefsChunks(struct IFFHandle *iff,LONG context,LONG flags)
{
	LONG i;
	LONG chunks[15][2];

	for(i = 0;i < 15;i++)
		chunks[i][0] = context;
	i = 0;
	chunks[i++][1] = ID_VERSION;

	if (flags & PRF_DISPLAY) chunks[i++][1] = ID_DISP;
	if (flags & PRF_SCREEN) chunks[i++][1] = ID_SCREEN;
	if (flags & PRF_COLORS) chunks[i++][1] = ID_COLORS;
	if (flags & PRF_KEYS) chunks[i++][1] = ID_KEYS;
	if (flags & PRF_ICON) chunks[i++][1] = ID_ICON;
	if (flags & PRF_CMDS) chunks[i++][1] = ID_CMDS;
	if (flags & PRF_MENU) chunks[i++][1] = ID_MENU;
	if (flags & PRF_FILE) chunks[i++][1] = ID_FILE;
	if (flags & PRF_FORMAT) chunks[i++][1] = ID_FORMAT;
	if (flags & PRF_NAMES) chunks[i++][1] = ID_NAMES;
	if (flags & PRF_TABLE) chunks[i++][1] = ID_TABLE;
	if (flags & PRF_SYSTEM) chunks[i++][1] = ID_SYSTEM;
	if (flags & PRF_CONTEXT) chunks[i++][1] = ID_CONTEXT;

	PropChunks(iff,(LONG *)chunks,i);
}


void
LoadPrefsWithHandle(struct IFFHandle *iff, struct Prefs *pr, long flags)
{
	BYTE add = flags & PRF_ADDCONTENTS ? TRUE : FALSE;
	BYTE keep = flags & PRF_KEEPOLDCONTENTS ? TRUE : FALSE;

	PropPrefsChunks(iff, ID_IGNP, flags);

	StopOnExit(iff, ID_IGNP, ID_FORM);
	ParseIFF(iff, IFFPARSE_SCAN);

	lpVersion(iff);
	lpDisp(iff, ID_IGNP, pr);
	lpScreen(iff);
	lpColors(iff, pr);
	lpKeys(iff, ID_IGNP, pr);
	lpCmds(iff, ID_IGNP, pr, add, keep);
	lpMenu(iff, ID_IGNP, pr);
	lpIcon(iff, ID_IGNP, pr); 

	lpFile(iff);
	lpFormat(iff, ID_IGNP, pr, NULL);
	lpNames(iff, ID_IGNP, pr);
	lpTable(iff, pr);
	lpSystem(iff);
	lpContext(iff, ID_IGNP, pr);
}


ULONG
LoadPrefs(struct Prefs *prefs, STRPTR name, BPTR lock, long flags)
{
	struct IFFHandle *iff;
	ULONG rc = FALSE;

	if ((iff = AllocIFF()) == NULL)
		return FALSE;

	if ((iff->iff_Stream = (IPTR)(name ? Open(name, MODE_OLDFILE) : OpenFromLock(lock))) != 0)
	{
		InitIFFasDOS(iff);
		if (!OpenIFF(iff, IFFF_READ))
		{
			LoadPrefsWithHandle(iff, prefs, flags);
			CloseIFF(iff);
			rc = TRUE;
		}
		Close((BPTR)iff->iff_Stream);
	}
	else if (!name && lock)
		UnLock(lock);

	FreeIFF(iff);
	return rc;
}
