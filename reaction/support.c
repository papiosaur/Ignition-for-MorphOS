/* Miscellaneous support functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "reactionGUI.h" // RA

#include <stdarg.h>



void
Rect32ToRect(struct Rect32 *rect32, struct Rectangle *rect)
{
	rect->MinX = max(rect32->MinX,0);
	rect->MinY = max(rect32->MinY, 0);
	rect->MaxX = min(rect32->MaxX, 32767);
	rect->MaxY = min(rect32->MaxY, 32767);
}


void PUBLIC
FreeString(REG(a0, STRPTR t))
{
#ifdef __amigaos4__
	if (t && pool)
#else
	if (t)
#endif
{
DBUG("FreeString():'%s' (%ld)\n",t,strlen(t));
		FreePooled(pool, t, strlen(t) + 1);
}
}


STRPTR PUBLIC
AllocString(REG(a0, CONST_STRPTR source))
{
	STRPTR t;

	if (!source || !strlen(source))
		return NULL;

	if ((t = AllocPooled(pool, strlen(source) + 1)) != 0)
		strcpy(t, source);

	return t;
}


STRPTR PUBLIC
AllocStringLength(REG(a0, STRPTR source), REG(d0, long len))
{
	STRPTR t;

	if (!source || !strlen(source))
		return NULL;

	if ((t = AllocPooled(pool, len + 1)) != 0)
	{
		strncpy(t, source, len);
		t[len] = '\0';
	}
	return t;
}


void
zstrcpy(STRPTR to, STRPTR from)
{
	if (from)
		strcpy(to, from);
	else
		*to = 0;
}


int
zstrnicmp(STRPTR a,STRPTR b,LONG len)
{
	return strnicmp(a ? a : (STRPTR)"", b ? b : (STRPTR)"",len);
}


int
zstricmp(STRPTR a,STRPTR b)
{
	return stricmp(a ? a : (STRPTR)"", b ? b : (STRPTR)"");
}


int
zstrcmp(STRPTR a,STRPTR b)
{
	return strcmp(a ? a : (STRPTR)"", b ? b : (STRPTR)"");
}


void
strdel(STRPTR t,long len)
{
	STRPTR s;

	for (s = t + len; *s; t++, s++)
		*t = *s;
	*t = 0;
}


int
cmdcmp(STRPTR *s1, STRPTR *s2)
{
	STRPTR c1 = *s1, c2 = *s2;
	int	cmp;

	for (;;c1++,c2++) {
		/* TODO: maybe do a cmdlen() before? */
		if ((*c1 == '(' || *c1 == ' ' || !*c1) && (*c2 == '(' || *c2 == ' ' || !*c2))
			return 0;

		if ((cmp = tolower(*c1) - tolower(*c2)) != 0)
			return cmp;
	}
}


int32
cmdlen(STRPTR t)
{
	long i = 0;

	if (!t)
		return 0;

	while (*(t+i) && i < 30 && (*(t+i) == '_' || IsAlNum(loc,*(t+i))))
		 i++;
	return i;
}


void
StringToUpper(STRPTR t)
{
	for (;*t;t++)
		*t = ToUpper(*t);
}


struct Node *
FindCommand(struct MinList *list,STRPTR name)
{
	struct Node *ln;
	long i;

	if (!name || !list)
		return NULL;

	i = cmdlen(name);
	for (ln = (struct Node *)list->mlh_Head;ln->ln_Succ && (strnicmp(ln->ln_Name,name,i) || IsAlNum(loc,*(ln->ln_Name+i)) || *(ln->ln_Name+i) == '_');ln = ln->ln_Succ);
	if (ln != (struct Node *)&list->mlh_Tail)
		return ln;

	return NULL;
}


struct Node *
FindTag(struct MinList *list, STRPTR t)
{
	struct Node *ln;

	if (!t || !list)
		return NULL;

	for (ln = (struct Node *)list->mlh_Head; ln->ln_Succ && stricmp(ln->ln_Name, t); ln = ln->ln_Succ);
	if (ln->ln_Succ)
		return ln;

	return NULL;
}


int32
CountNodes(struct MinList *l)
{
	struct Node *n;
	long i = 0;

	for (n = (struct Node *)l->mlh_Head; n->ln_Succ; n = n->ln_Succ, i++);
	return i;
}


struct NumberLink *
FindLink(struct MinList *list, APTR link)
{
	struct NumberLink *nl;

	foreach (list, nl) {
		if (nl->nl_Link == link)
			return nl;
	}

	return NULL;
}


struct MinList *
FindList(struct MinNode *mln)
{
	for (; mln->mln_Pred; mln = mln->mln_Pred);
	return (struct MinList *)mln;
}


struct Node *
FindListNumber(struct MinList *l, long num)
{
	struct Node *n;
	long i;
	ULONG listend = ~0L;

	if (IsListEmpty((struct List *)l))
		return NULL;

	for (n = (struct Node *)l->mlh_Head, i = 0; i < num && n->ln_Succ; i++, n = n->ln_Succ) {
		if (n == (struct Node *)l->mlh_TailPred && listend == -1)
			listend = i;
	}
	if (listend < num)
		return NULL;

	return n;
}


long
FindListEntry(struct MinList *l, struct MinNode *n)
{
	struct MinNode *sn;
	long i;

	for (sn = l->mlh_Head, i = 0; sn->mln_Succ; sn = sn->mln_Succ, i++) {
		if (sn == n)
			return i;
	}
	return -1;
}


void
InsertAt(struct MinList *l, struct Node *n, long pos)
{
	struct Node *pn;
	long	i;

	if (pos == -1)
		MyAddTail(l, n);
	else {
		for (pn = (struct Node *)l, i = 0; i < pos && pn->ln_Succ; pn = pn->ln_Succ, i++);
		Insert((struct List *)l, n, pn);
	}
}


void
MoveTo(struct Node *n, struct MinList *l1, long pos1, struct MinList *l2, long pos2)
{
	struct Node *pn;
	long i;

	if (l1 == l2 && pos1 == pos2)
		return;
	MyRemove(n);

	if (l1 == l2 && pos1 < pos2)
		pos2--;
	for (pn = (struct Node *)l2, i = 0; i < pos2 && pn->ln_Succ; pn = pn->ln_Succ, i++);
	Insert((struct List *)l2, n, pn);
}


void
moveList(struct MinList *from, struct MinList *to)
{
	if (!from || IsListEmpty((struct List *)from)) {
		if (to)
			MyNewList(to);
		return;
	}
	to->mlh_Head = from->mlh_Head;
	to->mlh_Tail = NULL;
	to->mlh_TailPred = from->mlh_TailPred;
	to->mlh_Head->mln_Pred = (struct MinNode *)&to->mlh_Head;
	to->mlh_TailPred->mln_Succ = (struct MinNode *)&to->mlh_Tail;
	MyNewList(from);
}


void
swapLists(struct MinList *l1, struct MinList *l2)
{
	struct MinList list;

	moveList(l1, &list);
	moveList(l2, l1);
	moveList(&list, l2);
}


int32
compareNames(const struct Node **lna, const struct Node **lnb)
{
	return StrnCmp(loc, (*lna)->ln_Name, (*lnb)->ln_Name, -1, SC_COLLATE1);
}


void
freeSort(APTR sortBuffer, ULONG len)
{
	if (sortBuffer && len)
		FreePooled(pool, sortBuffer, len * sizeof(struct Node *));
}


APTR
allocSort(struct MinList *l, ULONG *len)
{
	struct Node *ln, **sortBuffer;
	ULONG i = 0;

	if (IsListEmpty((struct List *)l))
		return NULL;

	if ((sortBuffer = AllocPooled(pool, (*len = CountNodes(l)) * sizeof(struct Node *))) != 0) {
		foreach(l, ln)
			*(sortBuffer + i++) = ln;
	}
	return sortBuffer;
}


void
SortListWith(struct MinList *l, APTR func)
{
	struct Node **buffer;
	uint32 len, i;

	if ((buffer = allocSort(l, &len)) != 0) {
		qsort(buffer, len, sizeof(struct Node *), func);
		MyNewList(l);
		for (i = 0; i < len; i++)
			MyAddTail(l, *(buffer + i));

		freeSort(buffer, len);
	}
}


long
compareType(struct Node **lna, struct Node **lnb)
{
	long i;

	i = (*lna)->ln_Type-(*lnb)->ln_Type;
	if (!i) {
		if ((*lna)->ln_Type == FVT_VALUE && !strcmp((*lna)->ln_Name,"0"))
			i = -1;
		else if ((*lnb)->ln_Type == FVT_VALUE && !strcmp((*lnb)->ln_Name,"0"))
			i = 1;
		else
			i = StrnCmp(loc,(*lna)->ln_Name,(*lnb)->ln_Name,-1,SC_COLLATE1);
	}
	return i;
}


void
sortList(struct MinList *l)
{
	SortListWith(l, compareNames);
}


void
SortTypeList(struct MinList *l)
{
	SortListWith(l, compareType);
}


long
GetListWidth(struct MinList *list)
{
	struct Node *ln;
	long width = 0, w;

	for (ln = (struct Node *)list->mlh_Head; ln->ln_Succ; ln = ln->ln_Succ) {
		if (width < (w = TLn(ln->ln_Name)))
			width = w;
	}
	return width;
}


double
PrepareConvert(STRPTR *s)
{
	char t[64];
	double num;
	long i;

	strcpy(t, *s);
	for (i = 0; t[i]; i++) {
		// replace commas with points
		if (t[i] == ',')
			t[i] = '.';
	}
	num = atof(t);

	// Search string backwards for the unit, result in 's'
	for (i--;!isdigit(t[i]) && !isspace(t[i]);i--);
	*s = t+i+1;

	return num;
}


void
ProcentToString(ULONG p,STRPTR t)
{
	strcpy(t, ita(p * 100 / 1024.0, -2, ITA_NONE));
	strcat(t, "%");
}


double
ConvertDegreeProcent(STRPTR s)
{
	double d = PrepareConvert(&s);

	if (*s == '%')
		d /= 100.0;
	else if (*s == '°')
		d = (2*PI*d)/360.0;

	return d;
}


int32
ConvertTime(STRPTR s)
{
	int32 secs = -1L;
	double num;

	num = fabs(PrepareConvert(&s));

	if (s[0] == 'm' || !strncmp(s, GetString(&gLocaleInfo, MSG_MINUTE_UNIT), 3) && (!s[3] || s[3] == ' ' || s[3] == 's'))
		secs = (long)(num * 60);
	else if (s[0] == 's' || !strncmp(s, GetString(&gLocaleInfo, MSG_SECOND_UNIT), 3) && (!s[3] || s[3] == ' ' || s[3] == 's'))
		secs = (long)num;
	else if (s[0] == 'h')
		secs = (long)(num * 3600);

	return secs;
}


struct cnt {STRPTR t;LONG div;};
const struct cnt cntm[] = {{"km",1000000},{"m",1000},{"dm",100},{"cm",10},{NULL,0}};
const struct cnt cnti[] = {{"yd",2592},{"ft",864},{"in",72},{"\"",72},{"pt",1},{NULL,0}};


double
ConvertNumber(STRPTR s, UBYTE targettype)
{
	UBYTE type;
	double num;
	long i;

	num = PrepareConvert(&s);
	for (type = 0, i = 0; cnti[i].t; i++) {
		if (!strcmp(s, cnti[i].t)) {
			// If it's an American/English unit, convert to points
			num *= cnti[i].div;
			type = CNT_POINT;
		}
	}
	if (!type) {
		for (type = 0, i = 0; cntm[i].t; i++) {
			if (!strcmp(s, cntm[i].t)) {
				// If it's a metrical unit, convert to millimeters
				num *= cntm[i].div;
				type = CNT_MM;
			}
		}
	}
	if (!type) {
		// Values without a unit are treated as millimeter
		type = CNT_MM;
	}
	if (type == CNT_MM && targettype >= CNT_POINT) {
		// metric to American/English conversion
		num *= 72 / 25.4;
	}
	if (type == CNT_POINT && targettype < CNT_POINT) {
		// American/English to metric conversion
		num *= 25.4 / 72;
	}
	if (targettype >= CNT_POINT) {
		switch (targettype) {
			case CNT_INCH:
				num /= 72.0;
				break;
			case CNT_FOOT:
				num /= 864.0;
				break;
			case CNT_YARD:
				num /= 2592.0;
				break;
		}
	} else
		for (;--targettype; num /= 10.0);

	return num;
}


void
WriteChunkString(APTR iff, STRPTR t)
{
	UBYTE pad = 0;

	if (t)
		WriteChunkBytes(iff, t, strlen(t) + 1);
	else
		WriteChunkBytes(iff, &pad, 1);
}


static void
MakeLocaleStringsA(struct MinList *list, LONG id, va_list args)
{
	MyNewList(list);

	while (id != TAG_END) {
		struct Node *node = AllocPooled(pool, sizeof(struct Node));
		if (node == NULL)
			break;

		node->ln_Name = GetString(&gLocaleInfo, id);
		MyAddTail(list, node);

		id = va_arg(args, ULONG);
	}
}


void
MakeLocaleStrings(struct MinList *list, LONG id, ...)
{
	va_list args;
	va_start(args, id);

	MakeLocaleStringsA(list, id, args);

	va_end(args);
}


// RA
struct List *MakeLocaleStringRACList(LONG id, ...) // RAC = ReAction Chooser
{
	struct List *list = (struct List *)AllocSysObject(ASOT_LIST, NULL);
	struct Node *n = NULL;
	va_list args;

	va_start(args, id);
	while(id != TAG_END)
	{
//DBUG("MakeLocaleStringRAList(): VA_ARG=%ld '%s'\n", id,GetString(&gLocaleInfo,id));
		n = AllocChooserNode(CNA_CopyText,TRUE, CNA_Text,GetString(&gLocaleInfo,id), TAG_DONE);
		AddTail(list, n);
		id = va_arg(args, ULONG);
	}
	va_end(args);

	return list;
}
// RA

struct List *
MakeLocaleStringList(LONG id, ...)
{
	struct List *list;
	va_list args;

	if ((list = AllocPooled(pool, sizeof(struct List))) == NULL)
		return NULL;

	va_start(args, id);
	MakeLocaleStringsA((struct MinList *)list, id, args);
	va_end(args);

	return list;
}


#ifdef _REACTGUI_
struct List *MakeLocaleStringRAList(LONG id, ...)
{
	struct List *list = (struct List *)AllocSysObject(ASOT_LIST, NULL);
	struct Node *node = NULL;
	va_list args;

	va_start(args, id);
	while(id != TAG_END)
	{
//DBUG("MakeLocaleStringRAList(): VA_ARG=%ld '%s'\n",id,GetString(&gLocaleInfo,id));
		node = AllocChooserNode(CNA_Text,GetString(&gLocaleInfo,id),
		                        CNA_CopyText,TRUE, TAG_DONE);
		AddTail(list, node);
		id = va_arg(args, ULONG);
	}
	va_end(args);

	return list;
}
#endif


void
FreeStringList(struct List *list)
{
	struct Node *node;
	
	if (list == NULL)
		return;

	while ((node = MyRemHead(list)) != NULL) {
		FreePooled(pool, node, sizeof(struct Node));
	}
	
	FreePooled(pool, list, sizeof(struct List));
}

 
static void
MakeStringsA(struct MinList *list, STRPTR string, va_list args)
{
	MyNewList(list);

	while (string != NULL) {
		struct Node *node = AllocPooled(pool, sizeof(struct Node));
		if (node == NULL)
			break;

		node->ln_Name = string;
		if (string && !strcmp(string, "-"))
			node->ln_Type = POPUP_NO_SELECT_BARLABEL;

		MyAddTail(list, node);

		string = va_arg(args, STRPTR);
	}
}


void
MakeStrings(struct MinList *list, const STRPTR string, ...)
{
	va_list args;
	va_start(args, string);

	MakeStringsA(list, string, args);

	va_end(args);
}


struct List *
MakeStringList(const STRPTR	string, ...)
{
	struct List *list;
	va_list args;

	if ((list = AllocPooled(pool, sizeof(struct List))) == NULL)
		return NULL;

	va_start(args, string);
	MakeStringsA((struct MinList *)list, string, args);
	va_end(args);

	return list;
}

 
uint32
DoClassMethodA(Class *cl,Msg msg)
{
	ULONG (ASM *m)(REG(a0, Class *), REG(a2, Object *), REG(a1, Msg));

	if (!cl || !msg)
		return FALSE;

	m = (APTR)cl->cl_Dispatcher.h_Entry;
	return (*m)(cl,(Object *)cl,msg);
}


uint32  lvsec,lvmsec;

bool
IsDoubleClick(int16 entry)
{
	uint32 sec,msec;

	CurrentTime(&sec,&msec);
	if (lventry == entry && DoubleClick(lvsec,lvmsec,sec,msec))
		return true;

	lventry = entry;
	CurrentTime(&lvsec,&lvmsec);

	return false;
}


struct Node *
HandleLVRawKeys(struct Gadget *lvgad,struct Window *win,struct MinList *list,long items)
{
	struct Node *node;
	long   i;

	if (imsg.Code != CURSORUP && imsg.Code != CURSORDOWN)
		return NULL;

	node = (struct Node *)lvgad->UserData;
	if (node && !(imsg.Qualifier & (IEQUALIFIER_SHIFT | IEQUALIFIER_ALT))) {
		if ((imsg.Code == CURSORUP) && ((struct Node *)list->mlh_Head != node))
			node = node->ln_Pred;
		if ((imsg.Code == CURSORDOWN) && ((struct Node *)list->mlh_TailPred != node))
			node = node->ln_Succ;
		lvgad->UserData = node;
	} else if (node && (imsg.Qualifier & IEQUALIFIER_SHIFT)) {
		items = (lvgad->Height-4)/items-1;

		if (imsg.Code == CURSORUP) {
			for (node = (struct Node *)lvgad->UserData;node->ln_Pred && items;node = node->ln_Pred,items--);
			if (!node->ln_Pred)
				node = (struct Node *)list->mlh_Head;
		} else {
			for (node = (struct Node *)lvgad->UserData;node->ln_Succ && items;node = node->ln_Succ,items--);
			if (!node->ln_Succ)
				node = (struct Node *)list->mlh_TailPred;
		}
		lvgad->UserData = node;
	} else if (imsg.Code == CURSORUP || !node)
		lvgad->UserData = list->mlh_Head;
	else if (imsg.Code == CURSORDOWN)
		lvgad->UserData = list->mlh_TailPred;

	GT_SetGadgetAttrs(lvgad,win,NULL,GTLV_Selected,i = FindListEntry(list,lvgad->UserData),GTLV_MakeVisible,i,TAG_END);
	return lvgad->UserData;
}


uint32
GetCheckBoxFlag(struct Gadget *gad,struct Window *win,ULONG flag)
{
	uint32 checked = 0;

	GT_GetGadgetAttrs(gad,win,NULL,GTCB_Checked,&checked,TAG_END);
	return checked ? flag : 0;
}


int32
WordWrapText(struct List *lh,STRPTR t,long width)
{
	struct TextExtent extent;
	struct Node *ln;
	long   fit,len,cols = 0;

	len = strlen(t);
	while (len > 0) {
		fit = TextFit(&scr->RastPort,t,len,&extent,NULL,1,width,fontheight+4);
		if ((ln = AllocPooled(pool, sizeof(struct Node))) != 0) {
			STRPTR s;

			if ((s = strchr(t,'\n')) && s <= t+fit)
				fit = s-t-1;
			else if (fit < len) {
				while(fit && *(t+fit) != ' ')
					fit--;
				if (!fit) {
					while (*(t+fit) && *(t+fit) != ' ')
						fit++;
				}
			}
			if ((ln->ln_Name = AllocPooled(pool, fit+1)) != 0)
				CopyMem(t,ln->ln_Name,fit);
			while (*(t+fit) == ' ' || *(t+fit) == '\n')
				fit++;
			MyAddTail(lh, ln);
			cols++;
		}
		len -= fit;  t += fit;
	}
	return cols;
}


STRPTR
GetUniqueName(struct MinList *list,STRPTR base)
{
	static char t[256];
	char *s;
	long i;

	if (!MyFindName(list,base) || !base || !list)
		return(base);
	if (strlen(base) > 250) {
		ErrorRequest(GetString(&gLocaleInfo, MSG_UNIQUE_NAME_ERR));
		return(base);
	}

	strcpy(t,base);
	for(s = t+strlen(t)-1;s > t && isdigit(*s);s--);
	if (*s == '-')
		s++;
	else
		s = t+strlen(t)+1;
	strcpy(s-1,"-1");  i = 1;
	while (MyFindName(list, t))
		sprintf(s,"%ld",++i);

	return t;
}


void
MakeUniqueName(struct MinList *list, STRPTR *name)
{
	STRPTR t;

	if ((t = GetUniqueName(list,*name)) != *name) {
		FreeString(*name);
		*name = AllocString(t);
	}
}


void
FreeListItems(struct MinList *list, APTR free)
{
	struct MinNode *mln;

	while ((mln = (struct MinNode *)MyRemHead(list)) != 0)
		((void (*)(struct MinNode *))free)(mln);
}


// RA
void
CopyRALBListItems(struct MinList *from, struct List *ra_lst, APTR copy, struct MinList *to) // RALB = ReAction ListBrowser
{
	struct MinNode *mln, *cmln;
	struct Node *n = NULL;

	foreach(from, mln)
	{
		if( (cmln=((struct MinNode * (*)(struct MinNode *))copy)(mln)) != 0 )
		{
			MyAddTail(to, cmln);

			if( (n=AllocListBrowserNode(1,
			                            LBNA_Column,0, LBNCA_CopyText,TRUE, LBNCA_Text,((struct Name *)cmln)->nm_Node.ln_Name,
			                            LBNA_UserData, cmln, // list [NATIVE] item
			                           TAG_DONE)) )
			{
				AddTail(ra_lst, n);
DBUG("CopyRALBListItems(): '%s' '%s' %ld (0x%08lx) (old=0x%08lx)\n",
      ((struct Name *)cmln)->nm_Node.ln_Name,((struct Name *)cmln)->nm_Content,((struct Name *)cmln)->nm_Node.ln_Type,cmln,to);
			}
		}
	}
}
// RA

void
CopyListItems(struct MinList *from, struct MinList *to, APTR copy)
{
	struct MinNode *mln,*cmln;

	foreach (from,mln) {
		if ((cmln = ((struct MinNode * (*)(struct MinNode *))copy)(mln)) != 0)
			MyAddTail(to, cmln);
	}
}


long SAVEDS
LinkNameSort(struct Link **la, struct Link **lb)
{
	return compareNames((const struct Node **)&(*la)->l_Link,(const struct Node **)&(*lb)->l_Link);
}


struct MinNode *
FindLinkCommand(struct MinList *list, STRPTR name)
{
	struct Link *l;
	STRPTR t;
	long i;

	if (!name || !list)
		return NULL;

	i = cmdlen(name);
	for(l = (struct Link *)list->mlh_Head; l->l_Node.mln_Succ && (strnicmp(t = ((struct Node *)l->l_Link)->ln_Name,name,i) || IsAlNum(loc,*(t+i)) || *(t+i) == '_'); l = (APTR)l->l_Node.mln_Succ);
	if (l->l_Node.mln_Succ)
		return l->l_Link;
	return NULL;
}


struct Link *
FindLinkWithName(struct MinList *list,STRPTR name)
{
	struct Link *l;

	if (!name)
		return NULL;

	foreach (list, l) {
		if (((struct Node *)l->l_Link)->ln_Name && !strcmp(((struct Node *)l->l_Link)->ln_Name,name))
			return l;
	}

	return NULL;
}


struct MinNode *
FindLinkName(struct MinList *list, STRPTR name)
{
	struct Link *l;

	if ((l = FindLinkWithName(list, name)) != 0)
		return l->l_Link;

	return NULL;
}


void
AddLink(struct MinList *list, struct MinNode *node, APTR func)
{
	struct Link *l;

	if ((l = AllocPooled(pool, sizeof(struct Link))) != 0) {
		l->l_Link = node;
		l->l_HookFunction = func;
		MyAddTail(list, l);
	}
}


LONG
GetListNumberOfName(struct MinList *mlh, STRPTR name, BOOL cmd, BOOL link)
{
	struct Node *ln;
	long len, pos = 0;
	STRPTR t;

	if (!name)
		return ~0L;

	if (cmd)
		len = cmdlen(name);

	foreach (mlh, ln) {
		t = link ? ((struct Node *)((struct Link *)ln)->l_Link)->ln_Name : ln->ln_Name;
		if (cmd ? !strnicmp(name,t,len) : !stricmp(name,t))
			return pos;
		pos++;
	}
	return ~0L;
}


BOOL
AddToNameList(STRPTR buffer, STRPTR t, int *plen, int maxlength)
{
	int l, len = *plen;

	if (!t)
		return TRUE;

	if (len && (len + 2 < maxlength-1))
		strcpy(buffer+len,", "), len += 2;

	l = strlen(t);

	if (!len && l > maxlength - 5) {
		// write part of first name
		*buffer = '"';
#ifdef __amigaos4__
		Strlcpy(buffer+1,t,maxlength-5);
#else
		stccpy(buffer+1,t,maxlength-5);
#endif
		strcpy(buffer+maxlength-5,"...\"");

		return FALSE;
	}
	if (len + l > maxlength - 8) {
		// add ellipsis to the end
		strcpy(buffer + len, "...");

		return FALSE;
	}

	*(buffer+len) = '"';
	strcpy(buffer+len+1,t);  len += 1+l;
	*(buffer+len) = '"';
	*(buffer+len+1) = 0;

	*plen = len+1;

	return TRUE;
}


struct Library *
IgnOpenClass(STRPTR secondary, STRPTR name, LONG version)
{
	struct Library *class;
	char path[256];

	strcpy(path, CLASSES_PATH);			  // try the "classes"-directory
	AddPart(path, name, 256);
	if ((class = OpenLibrary(path, version)) != 0)
		return class;

	strcpy(path, secondary);				 // try the secondary-directory
	AddPart(path, name, 256);
	if ((class = OpenLibrary(path, version)) != 0)
		return class;

	if ((class = OpenLibrary(name, version)) != 0)  // try the name only
		return class;

	return NULL;
}

#ifdef DEBUG
void KPutChar(char);

void
dumptext(UBYTE *b, int len)
{
	int i;

	bug("  ");
	for (i = 0; i < len; i++) {
		if (*b < ' ')
			KPutChar('.');
		else
			KPutChar(*b);
		b++;
	}
	KPutChar('\n');
}


void
dump(STRPTR pre, UBYTE *a, long len)
{
	UBYTE *b = a;
	int count = 1;

	for (;len; len--, count++) {
		if (count == 1)
			bug(pre);
		bug("%02lx",(long)*a);
		if (!(count % 4))
			KPutChar(' ');
		if (count > 15) {
			dumptext(b, 16);
			b = a+1;
			count = 0;
		}
		a++;
	}
	if (count) {
		int len = count-1;
		for (;count <= 16;count++) {
			bug("  ");
			if (!(count % 4))
				KPutChar(' ');
		}
		dumptext(b, len);
	}
}

#endif // DEBUG


void
RemoveFromArrayList(struct ArrayList *al, void *entry)
{
	void **list;
	int i;

	if (!al || !(list = al->al_List))
		return;
#ifdef __amigaos4__
	if(al->al_Last > al->al_Size || al->al_Size % 8 || al->al_Last > 10000 || al->al_Size > 10000 || al->al_Last < 0 || al->al_Size < 0)		//catch some error-cases that could appear in other functions
	{
DBUG("RemoveFromArrayList: Pointer: 0x%08X Last:%8d Size:%8d\n", al, al->al_Last, al->al_Size);
	    al->al_Last = al->al_Size = 0; al->al_List = NULL;
		return;
	}
#endif

	for (i = 0; i < al->al_Last; i++) {
		if (list[i] == entry)
			break;
	}
	if (i == al->al_Last)
		return;

	if (i < al->al_Last - 1)
		memmove(&list[i], &list[i + 1], (al->al_Last - i) * sizeof(void *));

	al->al_Last--;
}


int32
AddToArrayList(struct ArrayList *al, void *entry)
{
	if (!al->al_List) {
		if (!(al->al_List = AllocPooled(pool, (al->al_Size = 8) * sizeof(void *))))
			return -1;
	} else if (al->al_Last >= al->al_Size) {
		void **list;
		int size = al->al_Size;

		if (!(list = AllocPooled(pool, (al->al_Size += 8) * sizeof(void *))))
			return -1;

		CopyMem(al->al_List, list, size * sizeof(void *));
		FreePooled(pool, al->al_List, size * sizeof(void *));
		al->al_List = list;
	}
	al->al_List[al->al_Last] = entry;

	return al->al_Last++;
}


bool
AddListToArrayList(struct ArrayList *source,struct ArrayList *dest)
{
	int newSize;
	void **list;

	if (!source || !dest)
		return false;

	newSize = source->al_Last + dest->al_Last;
	if (newSize % 8)
		newSize += 8 - (newSize % 8);

	if (!(list = AllocPooled(pool,newSize * 4)))
		return false;

	CopyMem(dest->al_List,list,dest->al_Last * 4);
	CopyMem(source->al_List,list + dest->al_Last,source->al_Last * 4);

	dest->al_Last = source->al_Last + dest->al_Last;
	dest->al_Size = newSize;
	dest->al_List = list;

	return true;
}


bool
CopyArrayList(struct ArrayList *source,struct ArrayList *dest)
{
	if (!source || !dest)
		return false;

	MakeEmptyArrayList(dest);
	dest->al_Last = source->al_Last;
	dest->al_Size = source->al_Size;

	if (!(dest->al_List = AllocPooled(pool,source->al_Size * 4)))
		return false;

	CopyMem(source->al_List,dest->al_List,source->al_Last * 4);

	return true;
}


void
MakeEmptyArrayList(struct ArrayList *al)
{
	if (!al)
		return;

	if (al->al_List) {
		FreePooled(pool,al->al_List,al->al_Size * 4);
		al->al_List = NULL;
	}
	al->al_Last = al->al_Size = 0;
}


void
SetLocalizedName(STRPTR names[10], STRPTR *_name, STRPTR line)
{
	int i;

	if (line[0] == '=') {
		*_name = AllocString(line + 1);

		for (i = 0; i < 10; i++) {
			if (loc->loc_PrefLanguages[i] && !strcmp(loc->loc_PrefLanguages[i], "deutsch")) {
				names[i] = AllocString(*_name);
				break;
			}
		}
	} else {
		// localized name string
		STRPTR end = ++line;

		while (end[0] && end[0] != '=')
			end++;

		if (end[0] == '=')
			end[0] = '\0';

		for (i = 0; i < 10; i++) {
			if (loc->loc_PrefLanguages[i] && !strcmp(loc->loc_PrefLanguages[i], line)) {
				names[i] = AllocString(end + 1);
				break;
			}
		}
	}
}


void
GetLocalizedName(STRPTR names[10], STRPTR *_name, int *_language)
{
	int i;

	for (i = 0; i < 10; i++) {
		if (names[i])
			break;
	}

	if (i < 10) {
		FreeString(*_name);
		*_name = names[i];
		names[i] = NULL;

		if (_language)
			*_language = i;
	} else if (_language)
		*_language = -1;

	for (i = 0; i < 10; i++)
		FreeString(names[i]);
}
