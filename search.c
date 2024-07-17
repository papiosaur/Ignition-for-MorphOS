/* Cell search and replace functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"


UWORD searchMode;
char  word[256];
long  msfirst,mslen;


static bool
CharType(char c, BYTE *type)
{
	BYTE t = *type;

	if (isdigit(c))
		*type = CHAR_DIGIT;
	else if (isupper(c))
		*type = CHAR_UPPER;
	else if (islower(c))
		*type = CHAR_LOWER;
	else
		*type = CHAR_NONE;

	if (t == CHAR_FIRST)
		return true;
	if (t == CHAR_NONE && *type != t)
		return false;
	if (t == CHAR_LOWER && *type == CHAR_UPPER)
		return false;
	if (t != CHAR_NONE && *type == CHAR_NONE)
		return false;

	return true;
}


static bool
IsEndOfWord(STRPTR t,long pos)
{
	BYTE type = CHAR_FIRST;

	if (!*(t+pos+1))
		return true;

	CharType(*(t+pos), &type);

	return (bool)!CharType(*(t+pos+1), &type);
}


static bool
IsBeginOfWord(STRPTR t, long pos)
{
	BYTE type = CHAR_FIRST;

	if (!pos)
		return true;

	CharType(*(t+pos-1),&type);

	return (bool)!CharType(*(t+pos),&type);
}


STRPTR
NextWord(STRPTR t, long pos)
{
	BYTE type;
	long oldpos;

	word[0] = 0;
	oldpos = pos;  type = CHAR_FIRST;
	if (!*(t+pos))
		return NULL;

	while(*(t+pos) && CharType(*(t+pos),&type))
		pos++;
#ifdef __amigaos4__
	Strlcpy(word,t+oldpos,pos-oldpos+1);
#else
	stccpy(word,t+oldpos,pos-oldpos+1);
#endif

	return word;
}


static long
smr(UWORD sm, long cmp)
{
	switch (sm & SMR_MASK) {
		case SMR_EQUAL: return !cmp;
		case SMR_LESS: return cmp < 0;
		case SMR_EQUALLESS: return cmp <= 0;
		case SMR_GREATER: return cmp > 0;
		case SMR_EQUALGREATER: return cmp >= 0;
	}
	return 0; /* suppress compiler warning */
}


static bool
MatchString(STRPTR t, STRPTR cmp, UWORD sm)
{
	STRPTR s;
	long   i,len;

	if (!t) t = "";
	if (!cmp) cmp = "";

	if (sm & SM_PATTERN)
	{
		if (sm & SM_WORDS)
		{
			for (i = 0; (s = NextWord(t,i)) != 0; i += mslen)
			{
				msfirst = i;  mslen = strlen(s);
				if (sm & SM_IGNORECASE && MatchPatternNoCase(cmp,s) || MatchPattern(cmp,s))
					return true;
			}
			return false;
		}
		else
			return (bool)(sm & SM_IGNORECASE && MatchPatternNoCase(cmp,t) || MatchPattern(cmp,t));
	}
	else if ((sm & SMR_MASK) == SMR_EQUAL)
	{
		len = strlen(cmp);
		for(i = 0;*(t+i);i++)
		{
			if (ToLower(*(t+i)) == ToLower(*cmp))
			{
				if ((sm & SM_IGNORECASE && smr(sm,strnicmp(t+i,cmp,len))) || smr(sm,strncmp(t+i,cmp,len)))
				{
					msfirst = i;  mslen = len;
					if (sm & SM_WORDS)
					{
						if (IsBeginOfWord(t,i) && IsEndOfWord(t,i+len-1))
							return true;
					}
					else
						return true;
				}
			}
		}
		return false;
	}
	return (bool)smr(sm,stricmp(t,cmp));
}


void
ReplaceString(struct Page *page, struct tableField *tf, struct SearchNode *sn, UWORD sm)
{
	struct SearchNode *asn;
	STRPTR s,t;

	if (sn->sn_Type == SNT_TEXT)
		t = tf->tf_Text;
	else
		t = tf->tf_Note;
	for(asn = (APTR)search.mlh_Head;asn->sn_Node.ln_Succ && asn->sn_Type != sn->sn_Type;asn = (APTR)asn->sn_Node.ln_Succ);
	if (((sm & SM_PATTERN) && !(sm & SM_WORDS)) || !asn->sn_Node.ln_Type || !t)
		s = sn->sn_Text;
	else
	{
		if (MatchString(t, asn->sn_Text, sm) != 0) /* setzen von msfirst & mslen */
		{
			if ((s = AllocPooled(pool,strlen(t)-mslen+strlen(sn->sn_Text)+1)))
			{
#ifdef __amigaos4__
				Strlcpy(s,t,msfirst+1);
#else
				stccpy(s,t,msfirst+1);
#endif
				strcat(s,sn->sn_Text);
				strcat(s,t+msfirst+mslen);
			}
		}
	}
	switch(sn->sn_Type)
	{
		case SNT_TEXT:
			FreeString(tf->tf_Text);
			FreeString(tf->tf_Original);
			tf->tf_Text = AllocString(s);
			UpdateCellText(page, tf);
			break;
		case SNT_NOTE:
			FreeString(tf->tf_Note);
			tf->tf_Note = AllocString(s);
			break;
	}
	if (s != sn->sn_Text)
		FreeString(s);
}


BOOL
MatchSearched(struct tableField *tf,UWORD sm)
{
	struct SearchNode *sn;
	BOOL   match = TRUE;

	for(sn = (APTR)search.mlh_Head;sn->sn_Node.ln_Succ;sn = (APTR)sn->sn_Node.ln_Succ)
	{
		if (match && sn->sn_Node.ln_Type)
		{
			switch(sn->sn_Type)
			{
				case SNT_APEN:
					if (tf->tf_ReservedPen != sn->sn_Number)
						match = FALSE;
					break;
				case SNT_BPEN:
					if (tf->tf_BPen != sn->sn_Number)
						match = FALSE;
					break;
				case SNT_FORMAT:
					if (strcmp(tf->tf_Format,sn->sn_Text))
						match = FALSE;
					break;
				case SNT_TEXT:
					if (sn->sn_Number && sm & SM_PATTERN)
					{
						if (!MatchString(tf->tf_Original,(STRPTR)sn->sn_Number,sm))
							match = FALSE;
					}
					else if (!MatchString(tf->tf_Original,sn->sn_Text,sm & ~SM_PATTERN))
						match = FALSE;
					break;
				case SNT_STYLE:
					if (tf->tf_FontInfo->fi_Style != sn->sn_Number)
						match = FALSE;
					break;
				case SNT_FONT:
					/*if (!MatchString(tf->tf_Family->ln_Name,sn->sn_Text,sm))
						match = FALSE;*/
					if (strcmp(tf->tf_FontInfo->fi_Family->ln_Name,sn->sn_Text))
						match = FALSE;
					break;
				case SNT_SIZE:
					if (!smr(sm,tf->tf_FontInfo->fi_FontSize->fs_PointHeight-sn->sn_Number))
						match = FALSE;
					break;
				case SNT_HALIGN:
					if ((tf->tf_Alignment & TFA_HCENTER) != sn->sn_Number)
						match = FALSE;
					break;
				case SNT_VALIGN:
					if ((tf->tf_Alignment & TFA_VCENTER) != sn->sn_Number)
						match = FALSE;
					break;
				case SNT_NOTE:
					if (sn->sn_Number && sm & SM_PATTERN)
					{
						if (!MatchString(tf->tf_Note,(STRPTR)sn->sn_Number,sm))
							match = FALSE;
					}
					else if (!MatchString(tf->tf_Note,sn->sn_Text,sm & ~SM_PATTERN))
						match = FALSE;
					break;
			}
		}
	}
	return match;
}


long
PrepareSearchReplace(struct MinList *list,UWORD sm)
{
	struct SearchNode *sn;
	char   dest[512];
	long   num = 0;

	for(sn = (APTR)list->mlh_Head;sn->sn_Node.ln_Succ;sn = (APTR)sn->sn_Node.ln_Succ)
	{
		if (sn->sn_Node.ln_Type)
		{
			num++;
			switch(sn->sn_Type)
			{
				case SNT_STYLE:
					sn->sn_Number = (sn->sn_Number & 2 ? FS_BOLD : 0) | (sn->sn_Number & 4 ? FS_ITALIC : 0) | (sn->sn_Number & 8 ? FS_UNDERLINED : 0);
					break;
				case SNT_FONT:
					sn->sn_Number = (ULONG)MyFindName(&families, sn->sn_Text);
					break;
				case SNT_SIZE:
					sn->sn_Number = (ULONG)(ConvertNumber(sn->sn_Text,CNT_POINT)*65536.0+0.5);
					break;
				case SNT_HALIGN:
					switch(sn->sn_Number)
					{
						case 0:
							sn->sn_Number = TFA_LEFT;
							break;
						case 1:
							sn->sn_Number = TFA_HCENTER;
							break;
						case 2:
							sn->sn_Number = TFA_RIGHT;
							break;
					}
					break;
				case SNT_VALIGN:
					switch(sn->sn_Number)
					{
						case 0:
							sn->sn_Number = TFA_TOP;
							break;
						case 1:
							sn->sn_Number = TFA_VCENTER;
							break;
						case 2:
							sn->sn_Number = TFA_BOTTOM;
							break;
					}
					break;
				case SNT_TEXT:
				case SNT_NOTE:
					sn->sn_Number = 0;
					if (sm & SM_PATTERN && sn->sn_Text && (sn->sn_Type == SNT_TEXT || sn->sn_Type == SNT_NOTE))
					{
						if (sm & SM_IGNORECASE)
						{
							if (ParsePatternNoCase(sn->sn_Text,dest,512) == 1)
								sn->sn_Number = (ULONG)AllocString(dest);
						}
						else if (ParsePattern(sn->sn_Text,dest,512) != 1)
							sn->sn_Number = (ULONG)AllocString(dest);
					}
					break;
			}
		}
	}
	return num;
}


void
SearchReplace(struct Page *page,UWORD sm)
{
	struct tableField *tf,*stf;
	struct SearchNode *sn;
	struct UndoNode *un;
	long   col = 0,row = 0,found = 0,changed;
	BOOL   valid;

	if (!page)
		return;

	if ((sm & SM_BLOCK) && page->pg_MarkCol == -1)
	{
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_BLOCK_ERR));
		return;
	}

	if ((sm & SM_CELL) && page->pg_Gad.DispPos != PGS_NONE)
	{
		col = page->pg_Gad.cp.cp_Col;
		row = page->pg_Gad.cp.cp_Row;
	}
	if (!PrepareSearchReplace(&search,sm))
	{
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_SEARCH_CRITERIA_ERR));
		return;
	}
	if ((sm & SM_REPLACE) != 0 && !PrepareSearchReplace(&replace, sm))
	{
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_REPLACE_PROPERTIES_ERR));
		return;
	}
	if ((sm & SM_REPLACE) && (un = CreateUndo(page,UNDO_PRIVATE,GetString(&gLocaleInfo, MSG_REPLACE_UNDO))))
		un->un_Type = UNT_CELLS_CHANGED;

	foreach (&page->pg_Table, tf)
	{
		valid = TRUE;
		if (tf->tf_Row < row || tf->tf_Col <= col && tf->tf_Row == row)
			valid = FALSE;
		if ((sm & SM_BLOCK) && ((page->pg_MarkWidth != -1 && (tf->tf_Col < page->pg_MarkCol || tf->tf_Col > page->pg_MarkCol+page->pg_MarkWidth)) || (page->pg_MarkHeight != -1 && (tf->tf_Row < page->pg_MarkRow || tf->tf_Row > page->pg_MarkRow+page->pg_MarkHeight))))
			valid = FALSE;

		if (valid && MatchSearched(tf, sm))
		{
			found++;
			if (sm & SM_REPLACE)
			{
				/* insert replace code here :-) */
				if (sm & SM_ASK)
				{
					CreateTabGadget(page,tf->tf_Col,tf->tf_Row,TRUE);
					if (!(changed = DoRequest(GetString(&gLocaleInfo, MSG_REPLACE_CELL_PROPERTIES_REQ),GetString(&gLocaleInfo, MSG_YES_NO_CANCEL_REQ))))
					{
						DrawTable(page->pg_Window);
						RecalcTableFields(page);
						return;
					}
				}
				else
					changed = 1;
				if (changed == 1)
				{
					struct Node *ff = (APTR)~0L;
					ULONG  height = ~0L;
					LONG   style = ~0L;

					changed = FALSE;
					if (un && (stf = CopyCell(page,tf)))
						MyAddTail(&un->un_UndoList, stf);

					for (sn = (APTR)replace.mlh_Head;sn->sn_Node.ln_Succ;sn = (APTR)sn->sn_Node.ln_Succ)
					{
						if (sn->sn_Node.ln_Type) switch(sn->sn_Type)
						{
							case SNT_TEXT:
							case SNT_NOTE:
								ReplaceString(page, tf, sn, sm);
								break;
							case SNT_APEN:
								tf->tf_ReservedPen = tf->tf_APen = sn->sn_Number;
								break;
							case SNT_BPEN:
								tf->tf_BPen = sn->sn_Number;
								break;
							case SNT_HALIGN:
								tf->tf_Alignment = (tf->tf_Alignment & TFA_VCENTER) | sn->sn_Number;
								break;
							case SNT_VALIGN:
								tf->tf_Alignment = (tf->tf_Alignment & TFA_HCENTER) | sn->sn_Number;
								break;
							case SNT_FONT:
								ff = (struct Node *)sn->sn_Number;
								changed = TRUE;
								break;
							case SNT_SIZE:
								height = sn->sn_Number;
								changed = TRUE;
								break;
							case SNT_STYLE:
								style = sn->sn_Number;
								changed = TRUE;
								break;
							case SNT_FORMAT:
								FreeString(tf->tf_Format);
								tf->tf_Format = AllocString(sn->sn_Text);
								break;
						}
					}
					if (changed)
					{
						tf->tf_FontInfo = SetFontInfo(tf->tf_FontInfo, page->pg_DPI, FA_Family,	   ff,
																					 FA_PointHeight,  height,
																					 FA_Style,		style,
																					 TAG_END);
					}
					if (un && (stf = CopyCell(page, tf)))
						MyAddTail(&un->un_RedoList, stf);
				}
			}
			else
			{
				CreateTabGadget(page, tf->tf_Col, tf->tf_Row, TRUE);
				return;
			}
		}
	}
	if (found)
	{
		if (sm & SM_REPLACE)
		{
			DrawTable(page->pg_Window);
			RecalcTableFields(page);
		}
	}
	else
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_CELL_FOUND_ERR));
}


void
SetFindReplace(struct SearchNode *sn,struct Gadget *gad)
{
	struct Gadget *sgad;
	struct MinList *list;
	struct Hook *hook;
	BOOL   activate = FALSE;

	list = NULL;  hook = &popUpHook;
	switch(sn->sn_Type)
	{
		case SNT_TEXT:
		case SNT_NOTE:
			list = &history;
			activate = TRUE;
			break;
		case SNT_SIZE:
			list = &sizes;
			activate = TRUE;
			break;
		case SNT_STYLE:
			list = wd->wd_ExtData[2];
			activate = TRUE;
			break;
		case SNT_FONT:
			list = &families;
			break;
		case SNT_APEN:
		case SNT_BPEN:
			list = &colors;
			hook = &colorHook;
			break;
		case SNT_FORMAT:
			list = &prefs.pr_Formats;
			hook = &formatHook;
			break;
		case SNT_HALIGN:
			list = wd->wd_ExtData[3];
			break;
		case SNT_VALIGN:
			list = wd->wd_ExtData[4];
			break;
	}
	GT_SetGadgetAttrs(GadgetAddress(win,gad->GadgetID+1),win,NULL,GTST_String,sn->sn_Text,GA_Disabled,FALSE,TAG_END);
	if ((sgad = GadgetAddress(win, gad->GadgetID + 2)) != 0)
		sgad->UserData = list;
	wd->wd_ExtData[gad->GadgetID == 1 ? 5 : 6] = hook;
	if (activate)
		ActivateGadget(GadgetAddress(win,gad->GadgetID+1),win,NULL);
}


void
SetSearchStyle(struct SearchNode *sn, ULONG num)
{
	STRPTR t;
	char   s[256];

	if (num) {
		if (num != 1)
			sn->sn_Number &= ~1L;
		if (sn->sn_Number & num)
			sn->sn_Number &= ~num;
		else
			sn->sn_Number |= num;
	} else {
		char plain = ToLower(*GetString(&gLocaleInfo, MSG_FONT_PLAIN_CHAR));
		char bold = ToLower(*GetString(&gLocaleInfo, MSG_FONT_BOLD_CHAR));
		char italics = ToLower(*GetString(&gLocaleInfo, MSG_FONT_ITALICS_CHAR));
		char underlined = ToLower(*GetString(&gLocaleInfo, MSG_FONT_UNDERLINED_CHAR));

		sn->sn_Number = 0;

		for (t = sn->sn_Text; t[0]; t++) {
			char c = ToLower(t[0]);

			if (IsAlpha(loc, c)) {
				if (c == plain)
					sn->sn_Number = 1;
				else if (c == bold)
					sn->sn_Number |= 2;
				else if (c == italics)
					sn->sn_Number |= 4;
				else if (c == underlined)
					sn->sn_Number |= 8;

				while (*++t && IsAlpha(loc, t[0]));
				if (!*t)
					t--;
			}
		}
		FreeString(sn->sn_Text);
	}

	if (!sn->sn_Number || sn->sn_Number & 1) {
		sn->sn_Number = 1;
		strcpy(s, GetString(&gLocaleInfo, MSG_FONT_PLAIN_NAME));
	} else {
		s[0] = '\0';

		// bold
		if (sn->sn_Number & 2)
			strcpy(s, GetString(&gLocaleInfo, MSG_FONT_BOLD_NAME));	
		
		// italics
		if (sn->sn_Number & 4) {
			if (s[0])
				strcat(s, " |");

			strcat(s, GetString(&gLocaleInfo, MSG_FONT_ITALICS_NAME));
		}
		
		// underlined
		if (sn->sn_Number & 8) {
			if (s[0])
				strcat(s, " |");

			strcat(s, GetString(&gLocaleInfo, MSG_FONT_UNDERLINED_NAME));
		}
	}

	sn->sn_Text = AllocString(s);
}


void ASM
CreateFindReplaceGadgets(REG(a0, struct winData *wd))
{
	static const STRPTR searchLabels[] = {"<","<=","=",">=",">",NULL};
	long i = 0, w, buttonWidth;

	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD,
		MSG_START_AT_CELL_UGAD, MSG_WITHIN_BLOCK_UGAD, TAG_END);

	// find bottom button line with
	buttonWidth = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD));
	w = TLn(GetString(&gLocaleInfo, MSG_OK_GAD));
	buttonWidth = max(buttonWidth, w);
	w = TLn(GetString(&gLocaleInfo, MSG_START_AT_CELL_UGAD));
	buttonWidth = max(buttonWidth, w);
	w = TLn(GetString(&gLocaleInfo, MSG_WITHIN_BLOCK_UGAD));
	buttonWidth = max(buttonWidth, w);

	gWidth = 4 * buttonWidth + 82 + lborder + rborder;
	if (wd->wd_Type == WDT_REPLACE)
	{
		i = 1;  gWidth += TLn(GetString(&gLocaleInfo, MSG_WITH_SEARCH_PATTERN_GAD));
		w = (gWidth - 38 - lborder - rborder) / 2;
	}
	else
		w = gWidth - 16 - lborder - rborder;

	gHeight = barheight + fontheight * (15 - i) + 66 - i * 7 + bborder;

	for (; i > -1; i--)
	{
		ngad.ng_LeftEdge = 8 + lborder + i * (22 + w);
		ngad.ng_TopEdge = barheight + fontheight + 5;
		ngad.ng_Width = w;
		ngad.ng_Height = fontheight * 7 + 4;
		ngad.ng_GadgetText = NULL;			  // 1 - 6
		ngad.ng_GadgetID = i*5+1;
		gad = wd->wd_ExtData[i] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,i ? &replace : &search,GTLV_CallBack,&selectHook,GTLV_ShowSelected,NULL,TAG_END);

		ngad.ng_TopEdge += ngad.ng_Height+3;
		ngad.ng_Width -= boxwidth;
		ngad.ng_Height = fontheight+4;
		ngad.ng_GadgetID++;					 // 2 - 7
		gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,TAG_END);

		ngad.ng_LeftEdge += ngad.ng_Width;
		ngad.ng_GadgetID++;					 // 3 - 8
		gad = CreatePopGadget(wd,gad,FALSE);
	}
	ngad.ng_LeftEdge = 8 + lborder;
	ngad.ng_TopEdge += fontheight * 2 + 14;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_IGNORE_CASE_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID = 9;					 // 9
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Checked, searchMode & SM_IGNORECASE, GTCB_Scaled, TRUE, TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ONLY_WHOLE_WORDS_GAD);
	ngad.ng_GadgetID++;					   // 10
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Checked, searchMode & SM_WORDS, GTCB_Scaled, TRUE, TAG_END);

	if (wd->wd_Type == WDT_FIND)
		ngad.ng_TopEdge += fontheight + 7;
	else {
		long a = TLn(GetString(&gLocaleInfo, MSG_IGNORE_CASE_GAD)), b = TLn(ngad.ng_GadgetText);

		ngad.ng_LeftEdge += max(a, b) + 14 + boxwidth;
		a = w + lborder + 22;
		ngad.ng_LeftEdge = max(ngad.ng_LeftEdge, a);
	}

	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_WITH_SEARCH_PATTERN_GAD);
	ngad.ng_GadgetID++;					   // 11
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Checked, searchMode & SM_PATTERN, GTCB_Scaled, TRUE, TAG_END);

	if (wd->wd_Type == WDT_REPLACE)
	{
		ngad.ng_TopEdge -= fontheight+7;
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ACKNOWLEDGE_GAD);
		ngad.ng_GadgetID++;					 // 12
		gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Checked,searchMode & SM_ASK,GTCB_Scaled,TRUE,TAG_END);

		ngad.ng_TopEdge += fontheight+7;
	}
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_SEARCH_RELATION_GAD);
	ngad.ng_LeftEdge = 16 + lborder + TLn(ngad.ng_GadgetText);
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width = 42 + TLn("===");
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID = 13;					// 13
	gad = CreateGadget(CYCLE_KIND,gad,&ngad,GTCY_Labels,&searchLabels,GTCY_Active,(searchMode >> SMR_SHIFT)-1,GA_Disabled,searchMode & (SM_WORDS | SM_PATTERN),TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight + 12;
	ngad.ng_Width = buttonWidth + 16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = wd->wd_ShortCuts[0];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = lborder + (i = gWidth - ngad.ng_Width - lborder - rborder) / 3;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_START_AT_CELL_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[2];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = lborder + i * 2 / 3;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_WITHIN_BLOCK_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[3];
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth - ngad.ng_Width - rborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	wd->wd_ExtData[5] = NULL;
}


void ASM
CloseFindReplaceWindow(REG(a0, struct Window *win), REG(d0, bool clean))
{
	if (!clean)
		return;

	FreeStringList(wd->wd_ExtData[2]);
	FreeStringList(wd->wd_ExtData[3]);
	FreeStringList(wd->wd_ExtData[4]);
}

 
void ASM
HandleFindReplaceIDCMP(REG(a0, struct TagItem *tag))
{
	struct SearchNode *sn;
	struct MinList *list;
	struct Node *ln;
	long   i, j, id;

	if (imsg.Class == IDCMP_GADGETUP || imsg.Class == IDCMP_GADGETDOWN)
	{
		if ((gad = imsg.IAddress)->GadgetID < 6 || gad->GadgetID > 8)
		{
			sn = ((struct Gadget *)wd->wd_ExtData[0])->UserData;
			list = &search;
		}
		else
		{
			sn = ((struct Gadget *)wd->wd_ExtData[1])->UserData;
			list = &replace;
		}
	}

	switch (imsg.Class)
	{
		case IDCMP_GADGETDOWN:
			if (sn && ((i = (gad = imsg.IAddress)-> GadgetID) == 3 || i == 8) && gad->UserData)
			{
				i = PopUpList(win,GadgetAddress(win,gad->GadgetID-1),gad->UserData,POPA_CallBack,wd->wd_ExtData[i < 6 ? 5 : 6],POPA_MaxPen,~0L,wd->wd_ExtData[i < 6 ? 5 : 6] == &colorHook ? POPA_ItemHeight : TAG_IGNORE,fontheight+4,TAG_END);
				if (i != ~0L)
				{
					for(j = 0,ln = ((struct List *)gad->UserData)->lh_Head;j < i;j++,ln = ln->ln_Succ);
					FreeString(sn->sn_Text);
					if (sn->sn_Type == SNT_STYLE)
						SetSearchStyle(sn,1 << i);
					else
						sn->sn_Text = AllocString(ln->ln_Name);
					switch(sn->sn_Type)
					{
						case SNT_APEN:
						case SNT_BPEN:
							sn->sn_Number = ((struct colorPen *)ln)->cp_ID;
							break;
						case SNT_STYLE:   // do not remove! sn_Number was set in SetSearchStyle()!
							break;
						default:
							sn->sn_Number = i;
							break;
					}
					GT_SetGadgetAttrs(GadgetAddress(win,gad->GadgetID-1),win,NULL,GTST_String,sn->sn_Text,TAG_END);
					GT_SetGadgetAttrs(gad = wd->wd_ExtData[gad->GadgetID < 6 ? 0 : 1],win,NULL,GTLV_Labels,~0L,TAG_END);
					sn->sn_Node.ln_Type = 1;
					GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,list,TAG_END);
				}
			}
			break;
		case IDCMP_GADGETUP:
		case IDCMP_VANILLAKEY:
			if (imsg.Class == IDCMP_GADGETUP)
				id = gad->GadgetID;
			else {
				id = imsg.Code;
#ifndef __amigaos4__
				gad = NULL;
#endif
			}

			switch (id) {
				case 1:
				case 6:
					if (sn && imsg.Code == FindListEntry(list, (struct MinNode *)sn))
					{
						GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,~0L,TAG_END);
						sn->sn_Node.ln_Type = (!sn->sn_Node.ln_Type) & 1;
						GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,list,TAG_END);
					}
					else
					{
						for(i = 0,sn = (APTR)list->mlh_Head;i < imsg.Code && sn->sn_Node.ln_Succ;i++,sn = (APTR)sn->sn_Node.ln_Succ);
						gad->UserData = sn;
						SetFindReplace(sn,gad);
					}
					break;
				case 2:
				case 7:
					if (sn)
					{
						FreeString(sn->sn_Text);
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&sn->sn_Text,TAG_END);
						sn->sn_Text = AllocString(sn->sn_Text);

						switch (sn->sn_Type)
						{
							case SNT_TEXT:
							case SNT_NOTE:
								if (!MyFindName(&history, sn->sn_Text))
								{
									if ((ln = AllocPooled(pool, sizeof(struct Node))) != 0)
									{
										ln->ln_Name = AllocString(sn->sn_Text);
										MyAddTail(&history, ln);
									}
								}
								break;
							case SNT_FONT:
								if (!MyFindName(&families, sn->sn_Text))
								{
									DisplayBeep(NULL);
									sn->sn_Text = AllocString(((struct Node *)families.mlh_Head)->ln_Name);
								}
								break;
							case SNT_FORMAT:
								if (!MyFindName(&prefs.pr_Formats, sn->sn_Text))
								{
									DisplayBeep(NULL);
									sn->sn_Text = AllocString(((struct Node *)prefs.pr_Formats.mlh_Head)->ln_Name);
								}
								break;
							case SNT_APEN:
							case SNT_BPEN:
								if ((ln = MyFindName(&colors, sn->sn_Text)) != 0)
									sn->sn_Number = ((struct colorPen *)ln)->cp_ID;
								else
								{
									DisplayBeep(NULL);
									ln = (struct Node *)colors.mlh_Head;
									sn->sn_Text = AllocString(ln->ln_Name);
									sn->sn_Number = ((struct colorPen *)ln)->cp_ID;
								}
								break;
							case SNT_STYLE:
								SetSearchStyle(sn,0);
								break;
						}
						GT_SetGadgetAttrs(gad,win,NULL,GTST_String,sn->sn_Text,TAG_END);
						GT_SetGadgetAttrs(gad = wd->wd_ExtData[gad->GadgetID < 6 ? 0 : 1],win,NULL,GTLV_Labels,~0L,TAG_END);
						sn->sn_Node.ln_Type = 1;
						GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,list,TAG_END);
					}
					break;
				case 10:   // Wörter?
				case 11:   // Suchmuster?
					GT_GetGadgetAttrs(GadgetAddress(win, gad->GadgetID == 10 ? 11 : 10), win, NULL, GTCB_Checked, &i, TAG_END);
					GT_SetGadgetAttrs(GadgetAddress(win, 13), win, NULL, GA_Disabled, imsg.Code | i, TAG_END);
					break;
				default:
					if (id == wd->wd_ShortCuts[0] || id == wd->wd_ShortCuts[2] || id == wd->wd_ShortCuts[3]) {
						// Ok, Ab Zelle, Im Block
						GT_GetGadgetAttrs(GadgetAddress(win, 9), win, NULL, GTCB_Checked, &i, TAG_END);
						searchMode = i ? searchMode | SM_IGNORECASE : searchMode & ~SM_IGNORECASE;
						GT_GetGadgetAttrs(GadgetAddress(win, 10), win, NULL, GTCB_Checked, &i, TAG_END);
						searchMode = i ? searchMode | SM_WORDS : searchMode & ~SM_WORDS;
						GT_GetGadgetAttrs(GadgetAddress(win, 11), win, NULL, GTCB_Checked, &i, TAG_END);
						searchMode = i ? searchMode | SM_PATTERN : searchMode & ~SM_PATTERN;
						searchMode &= ~SMR_MASK;
						if (searchMode & (SM_PATTERN | SM_WORDS))
							searchMode |= SMR_EQUAL;
						else {
							GT_GetGadgetAttrs(GadgetAddress(win, 13), win, NULL, GTCY_Active, &i, TAG_END);
							searchMode |= (i+1) << SMR_SHIFT;
						}
						searchMode &= ~(SM_BLOCK | SM_CELL);

						if (gad->GadgetID == 15)
							searchMode |= SM_CELL;
						else if (gad->GadgetID == 16)
							searchMode |= SM_BLOCK;

						if ((gad = GadgetAddress(win, 12)) != 0) {
							GT_GetGadgetAttrs(gad, win, NULL, GTCB_Checked, &i, TAG_END);
							searchMode = i ? searchMode | SM_ASK : searchMode & ~SM_ASK;
						}
						searchMode &= ~SM_REPLACE;
						if (wd->wd_Type == WDT_REPLACE)
							searchMode |= SM_REPLACE;

						CloseAppWindow(win, TRUE);
						SearchReplace(rxpage, searchMode);
					} else if (id == wd->wd_ShortCuts[1]) {
						// Abbrechen
						CloseAppWindow(win, TRUE);
					}
#ifdef __amigaos4__
					gad = NULL;
#endif
					break;
			}
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win,TRUE);
			break;
	}
}


void
InitSearch(void)
{
	const LONG searchNames[] = {MSG_TEXT_SEARCH, MSG_FOREGROUND_COLOR_SEARCH, MSG_BACKGROUND_COLOR_SEARCH,
		MSG_FORMAT_SEARCH, MSG_FONT_SEARCH, MSG_FONT_HEIGHT_SEARCH, MSG_FONT_STYLE_SEARCH, MSG_ALIGNMENT_SEARCH,
		MSG_VALIGNMENT_SEARCH, MSG_NOTE_SEARCH};
	struct SearchNode *sn, *rn;
	long i;

	MyNewList(&search);  MyNewList(&replace);  MyNewList(&history);

	for (i = 1; i < NUM_SNT; i++)
	{
		if ((sn = AllocPooled(pool, sizeof(struct SearchNode))) != 0)
		{
			sn->sn_Node.ln_Name = GetString(&gLocaleInfo, searchNames[i - 1]);
			sn->sn_Type = i;
			MyAddTail(&search, sn);
			if ((rn = AllocPooled(pool, sizeof(struct SearchNode))) != 0)
			{
				*rn = *sn;
				MyAddTail(&replace, rn);
			}
			if (i == SNT_TEXT)
				sn->sn_Node.ln_Type = 1;
		}
	}
	searchMode = SM_IGNORECASE | SMR_EQUAL;
}


