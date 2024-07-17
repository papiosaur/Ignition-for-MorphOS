/* Project related functions
 *
 * Copyright 1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
 

#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include <proto/gtdrag.h>
	#include <stdarg.h>

	extern char IgnTitle[100];		//ScreenTitle for ignition, must be unique for PubScreen handling
	extern bool IsOverTabGadget(struct Page *page);
#endif


struct coordPkt actcp;
int mousepointer;
LONG propadd = 0;


static int
MouseOverTable(struct winData *wd)
{
	if (!wd)
		return 0;

	return imsg.MouseX >= wd->wd_TabX && imsg.MouseX <= wd->wd_TabX+wd->wd_TabW
		&& imsg.MouseY >= wd->wd_TabY && imsg.MouseY <= wd->wd_TabY+wd->wd_TabH;
}


void
SetMainPage(struct Page *page)
{
	if (page == rxpage || page && rxpage && page->pg_Mappe == rxpage->pg_Mappe) {
		rxpage = page;
		return;
	}
//Correct the grim-error when closing igntion with more the two window
#ifdef __amigaos4__
	if (rxpage && rxpage->pg_Window && rxpage->pg_Window->UserPort == iport)
#else
	if (rxpage && rxpage->pg_Window)
#endif
		SetWindowTitles(rxpage->pg_Window,rxpage->pg_Mappe->mp_Title+3,(STRPTR)~0L);
#ifdef __amigaos4__
	if (page && page->pg_Window && page->pg_Window->UserPort == iport)
#else
	if (page && page->pg_Window)
#endif
		SetWindowTitles(page->pg_Window,page->pg_Mappe->mp_Title,(STRPTR)~0L);

	calcpage = rxpage = page;   // zur Sicherheit auch calcpage

	RefreshLockList(RXMAP);
	/** set menu correctly for all windows **/
	{
		struct Window *win;
		struct winData *wd;
		struct Menu *menu = rxpage ? rxpage->pg_Mappe->mp_Prefs.pr_Menu : prefs.pr_Menu;

		for (win = scr->FirstWindow;win;win = win->NextWindow) {
			wd = (struct winData *)win->UserData;
			if (win->UserPort == iport && wd->wd_Type != WDT_PROJECT && win->MenuStrip != menu) {
				ClearMenuStrip(win);
				SetMenuStrip(win,menu);
			}
		}
	}
}


void
UpdateMapTitle(struct Mappe *mp)
{
	long len = 4;

	if (!mp)
		return;

	FreeString(mp->mp_Title);
	if (mp->mp_Path)
		len += strlen(mp->mp_Path)+5;

	if ((mp->mp_Title = AllocPooled(pool,strlen(mp->mp_Node.ln_Name)+len)) != 0) {
		strcpy(mp->mp_Title,"** ");
		strcpy(mp->mp_Title+3,mp->mp_Node.ln_Name ? mp->mp_Node.ln_Name : "");
		if (mp->mp_Path) {
			strcat(mp->mp_Title,"  -  ");
			strcat(mp->mp_Title,mp->mp_Path);
		}
	}
}


STRPTR
GetPageTitle(struct Page *page)
{
	if (!page)
		return(NULL);
	return(page->pg_Mappe->mp_Title+(page == rxpage ? 0 : 3));
}


void
UpdateModified(struct Mappe *mp)
{
	struct Page *page;
	BYTE changed = 0;

	foreach (&mp->mp_Pages,page) {
		if (page->pg_Modified)
			changed++;
	}
	mp->mp_Modified = changed ? TRUE : FALSE;
}


void
SetPageProps(struct Page *page)
{
	struct Window *win;
	long i,top,total,visible;
	double factor;

	if (!(win = page->pg_Window))
		return;

	page->pg_PropFactorX = 1.0;
	page->pg_PropFactorY = 1.0;
	for (page->pg_TabW = 0, i = 0; i < page->pg_Cols; page->pg_TabW += GetTFWidth(page,++i));
	for (page->pg_TabH = 0, i = 0; i < page->pg_Rows; page->pg_TabH += GetTFHeight(page,++i));

	total = page->pg_TabW;
	if (page->pg_TabX+page->pg_wTabW > page->pg_TabW)
		total = page->pg_TabX+page->pg_wTabW;
	top = page->pg_TabX;
	visible = page->pg_wTabW;

	if (total > 65535) {
		factor = total/65536.0;
		total = 65535;
		top = (long)top/factor;
		visible = (long)visible/factor;
		page->pg_PropFactorX = factor;
	}
	SetGadgetAttrs(GadgetAddress(win, PROPGAD_HORIZ_ID), win, NULL,
		PGA_Total, total, PGA_Visible, visible, PGA_Top, top, TAG_END);

	total = page->pg_TabH;
	if (page->pg_TabY+page->pg_wTabH > page->pg_TabH)
		total = page->pg_TabY+page->pg_wTabH;

	top = page->pg_TabY;
	visible = page->pg_wTabH;
	if (total > 65535) {
		factor = total/65536.0;
		total = 65535;
		top = (long)top/factor;
		visible = (long)visible/factor;
		page->pg_PropFactorY = factor;
	}

	SetGadgetAttrs(GadgetAddress(win, PROPGAD_VERT_ID), win, NULL,
		PGA_Total, total, PGA_Visible, visible, PGA_Top, top, TAG_END);
}


void
UpdateProjPage(struct Window *win,struct Page *page)
{
	struct winData *wd;
	if (win && page && (wd = (struct winData *)win->UserData) && wd->wd_Data != page) {
		((struct Page *)wd->wd_Data)->pg_Window = NULL;
		wd->wd_Data = page;
		page->pg_Window = page->pg_Mappe->mp_Window = win;
		page->pg_wTabX = wd->wd_TabX;  page->pg_wTabY = wd->wd_TabY;
		page->pg_wTabW = wd->wd_TabW;  page->pg_wTabH = wd->wd_TabH;

		SetWindowTitles(win,GetPageTitle(page),(UBYTE *)~0L);
		DrawTable(win);
		SetPageProps(page);
		GT_SetGadgetAttrs(GadgetAddress(win, GID_PAGE), win, NULL,
			GTTX_Text, page->pg_Node.ln_Name, TAG_END);
		RefreshToolBar(page);
		DisplayTablePos(page);
		DrawStatusFlags(page->pg_Mappe,win);
	}
}


void
SetPageColor(struct Page *page, struct colorPen *apen, struct colorPen *bpen)
{
	struct tableField *tf;
	char t[128];
	long maxcol = 0;

	if (!page)
		return;

	SetBusy(TRUE, BT_APPLICATION);

	strcpy(t, GetString(&gLocaleInfo, MSG_COLOR_CHANGE_UNDO));
	if (apen)
		strcat(t, apen->cp_Node.ln_Name);
	else
		strcat(t, "-");
	strcat(t,"/");
	if (bpen)
		strcat(t,bpen->cp_Node.ln_Name);
	else
		strcat(t,"-");
	BeginUndo(page,UNDO_BLOCK,t);

	tf = NULL;
	while ((tf = GetMarkedFields(page, tf, TRUE)) != 0) {
		if (apen) {
			if (tf->tf_APen == tf->tf_ReservedPen)
				tf->tf_APen = apen->cp_ID;
			tf->tf_ReservedPen = apen->cp_ID;
		}

		if (bpen)
			tf->tf_BPen = bpen->cp_ID;

		if (maxcol < tf->tf_Col + tf->tf_Width)
			maxcol = tf->tf_Col + tf->tf_Width;
		tf->tf_Flags |= TFF_PENSET;
	}
	EndUndo(page);
	RefreshMarkedTable(page, maxcol, FALSE);
	SetBusy(FALSE, BT_APPLICATION);
}


void
SetCellPattern(struct Page *page, long col, UBYTE pattern)
{
	struct tableField *tf;

	if (!page)
		return;

	SetBusy(TRUE, BT_APPLICATION);
	BeginUndo(page, UNDO_BLOCK, pattern ? GetString(&gLocaleInfo, MSG_SET_PATTERN_UNDO)
		: GetString(&gLocaleInfo, MSG_REMOVE_PATTERN_UNDO));
	tf = NULL;

	while ((tf = GetMarkedFields(page, tf, TRUE)) != 0) {
		tf->tf_Pattern = pattern;
		tf->tf_PatternColor = col;
	}

	EndUndo(page);
	DrawMarkedCells(page, -1);
	SetBusy(FALSE, BT_APPLICATION);
}


void
SetCellSecurity(struct Page *page, LONG security)
{
	struct tableField *tf = NULL;
	struct Mappe *mp = page->pg_Mappe;
	long  maxcol = 0;

	if (!page || security == -1)
		return;

	SetBusy(TRUE, BT_APPLICATION);

	if (mp->mp_Flags & MPF_CELLSLOCKED) {
		if (!QueryPassword(GetString(&gLocaleInfo, MSG_CHANGE_SECURITY_ATTRIBUTES_REQ), page->pg_Mappe->mp_CellPassword)) {
			SetBusy(FALSE, BT_APPLICATION);
			return;
		}
		mp->mp_Flags &= ~MPF_CELLSLOCKED;
	}

	BeginUndo(page, UNDO_BLOCK, GetString(&gLocaleInfo, MSG_SET_SECURITY_ATTRIBUTES_UNDO));

	while ((tf = GetMarkedFields(page, tf, FALSE)) != 0) {
		tf->tf_Flags = (tf->tf_Flags & ~(TFF_SECURITY)) | security;

		if (maxcol < tf->tf_Col + tf->tf_Width)
			maxcol = tf->tf_Col + tf->tf_Width;
	}
	EndUndo(page);

	RefreshMarkedTable(page, maxcol, FALSE);
	SetBusy(FALSE, BT_APPLICATION);
}


void
SetAlignment(struct Page *page, BYTE alignH, BYTE alignV)
{
	struct tableField *tf = NULL;
	long   maxcol = 0;
	char   t[128];

	if (!page)
		return;

	SetBusy(TRUE, BT_APPLICATION);

	strcpy(t,GetString(&gLocaleInfo, MSG_ALIGNMENT_UNDO));
	if (alignH) {
		if ((alignH & TFA_HCENTER) == TFA_HCENTER)
			strcat(t,GetString(&gLocaleInfo, MSG_ALIGN_CENTER_UNDO));
		else if (alignH & TFA_LEFT)
			strcat(t,GetString(&gLocaleInfo, MSG_ALIGN_LEFT_UNDO));
		else if (alignH & TFA_RIGHT)
			strcat(t,GetString(&gLocaleInfo, MSG_ALIGN_RIGHT_UNDO));
		if (alignV)
			strcat(t,", ");
	}
	if (alignV) {
		if ((alignV & TFA_VCENTER) == TFA_VCENTER)
			strcat(t,GetString(&gLocaleInfo, MSG_VALIGN_MIDDLE_UNDO));
		else if (alignV & TFA_TOP)
			strcat(t,GetString(&gLocaleInfo, MSG_VALIGN_TOP_UNDO));
		else if (alignV & TFA_BOTTOM)
			strcat(t,GetString(&gLocaleInfo, MSG_VALIGN_BOTTOM_UNDO));
	}

	BeginUndo(page,UNDO_BLOCK,t);
	while ((tf = GetMarkedFields(page, tf, TRUE)) != 0) {
		if (alignV)
			tf->tf_Alignment = (tf->tf_Alignment & (TFA_HCENTER | TFA_VIRGIN)) | alignV;
		if (alignH)
			tf->tf_Alignment = (tf->tf_Alignment & TFA_VCENTER) | alignH;
		if (maxcol < tf->tf_Col+tf->tf_Width)
			maxcol = tf->tf_Col+tf->tf_Width;
	}
	EndUndo(page);

	RefreshMarkedTable(page, maxcol, TRUE);
	SetBusy(FALSE, BT_APPLICATION);
}

#ifdef __amigaos4__
void UpdatePageFontA(struct Page *page, struct TagItem *tags)
{
	struct FontFamily *ff;
	struct TagItem *styletag;
	ULONG style,height;
	WORD rotate,shear;
	struct tableField *tf = NULL;
	long maxcol = 0;
	char t[256],s[16];

	if (!page)
		return;

	SetBusy(TRUE, BT_APPLICATION);

	ff = (struct FontFamily *)GetTagData(FA_Family, ~0L, (struct TagItem *)tags);
	height = GetTagData(FA_PointHeight, ~0L, (struct TagItem *)tags);

	if (ff != (APTR)~0L)
		page->pg_Family = (struct Node *)ff;		/* Auswahl für folgende übernehmen */
	if (height != ~0L) {
		// limit maximal font size to 128 pt
		if (height > (128L << 16)) 
			height = 128L << 16;
#if  0
		// limit maximal font size to 72 pt (because of MorphOS broken ft2.library)
		if (height > (72L << 16)) 
			height = 72L << 16;
#endif
		{

			styletag = FindTagItem(FA_PointHeight, (struct TagItem *)tags);
			if (styletag)
				styletag->ti_Data = height;
		}

		page->pg_PointHeight = height;
	}

	if (page->pg_Gad.DispPos != PGS_NONE || page->pg_MarkCol != -1) {
		if ((styletag = FindTagItem(FA_Style, (struct TagItem *)tags)) != 0)
			style = styletag->ti_Data;
		else
			style = ~0L;
		rotate = GetTagData(FA_Rotate, ~0L, (struct TagItem *)tags);
		shear = GetTagData(FA_Shear, ~0L, (struct TagItem *)tags);

		/********** Undo/Redo Text erstellen **********/

		t[0] = 0;
		if (ff != (APTR)~0L) {
			strcpy(t, GetString(&gLocaleInfo, MSG_FONT_UNDO));
			strcat(t, ff->ff_Node.ln_Name);
			if (height != ~0L || style != ~0L || rotate != ~0L || shear != ~0L)
				strcat(t, "/");
		}
		if (height != ~0L) {
			sprintf(t + strlen(t), "%s %ldpt", ff == (APTR)~0L ? "Schrifthöhe" : "Höhe", height >> 16);
			if (style != ~0L || rotate != ~0L || shear != ~0L)
				strcat(t, "/");

			if (prefs.pr_Table->pt_Flags & PTF_AUTOCELLSIZE) {
				while ((tf = GetMarkedFields(page, tf, FALSE)) != 0) {
					if (tf->tf_Text)
						maxcol = 1;
				}

				if (maxcol) {
					sprintf(s, "%ld pt", ((height*4) >> 16)/3);
					ChangeCellSize(page,NULL,s,CCS_MINHEIGHT,NULL);
					maxcol = 0;
				}
			}
		}

		if (style != ~0L) {
			sprintf(t+strlen(t),"%s ",ff == (APTR)~0L && height == ~0L ? "Schriftstil" : "Stil");
			if (rotate != ~0L || shear != ~0L)
				strcat(t,"/");
			if (style == FS_PLAIN)
				strcat(t,GetString(&gLocaleInfo, MSG_FONT_PLAIN_NAME));
			else {
				BOOL last = FALSE;

				if (style & FS_BOLD) {
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_BOLD_NAME));
					last = TRUE;
				}
				if (style & FS_ITALIC) {
					if (last)
						strcat(t, "/");
					else
						last = TRUE;
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_ITALICS_NAME));
				}
				if (style & (FS_UNDERLINED | FS_DOUBLE_UNDERLINED | FS_STRIKE_THROUGH)) {
					if (last)
						strcat(t, "/");
				}
				if (style & FS_UNDERLINED)
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_UNDERLINED_NAME));
				else if (style & FS_DOUBLE_UNDERLINED)
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_DOUBLE_UNDERLINED_UNDO));
				else if (style & FS_STRIKE_THROUGH)
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_STRIKE_THROUGH_UNDO));
			}
		}
		if (rotate != ~0L) {
			if (shear != ~0L)
				strcat(t,"/");
			sprintf(t+strlen(t), GetString(&gLocaleInfo, MSG_FONT_ROTATION_UNDO), rotate);
		}
		if (shear != ~0L)
			sprintf(t+strlen(t), GetString(&gLocaleInfo, MSG_FONT_SHEAR_UNDO), shear);

		/********** Änderungen durchführen **********/
		BeginUndo(page, UNDO_BLOCK, t);

		while ((tf = GetMarkedFields(page, tf, TRUE /*FALSE*/)) != 0) {
			if (style != ~0L) {
				if (style & FS_ALLBITS)
					styletag->ti_Data = style & ~FS_ALLBITS;
				else if (style & FS_UNSET)
					styletag->ti_Data = tf->tf_FontInfo->fi_Style & ~style;
				else if (style & (FS_UNDERLINED | FS_DOUBLE_UNDERLINED | FS_STRIKE_THROUGH))
					styletag->ti_Data = (tf->tf_FontInfo->fi_Style & ~(FS_UNDERLINED | FS_DOUBLE_UNDERLINED | FS_STRIKE_THROUGH)) | style;
				else
					styletag->ti_Data = tf->tf_FontInfo->fi_Style | style;

				if (style == FS_PLAIN)
					styletag->ti_Data = FS_PLAIN;
			}

			tf->tf_FontInfo = SetFontInfoA(tf->tf_FontInfo, page->pg_DPI, (struct TagItem *)tags);
			tf->tf_Flags |= TFF_FONTSET;

			if (maxcol < tf->tf_Col+tf->tf_Width) // Breite vorher
				maxcol = tf->tf_Col+tf->tf_Width;

			SetTFWidth(page, tf);

			if (maxcol < tf->tf_Col+tf->tf_Width) // Breite nachher
				maxcol = tf->tf_Col+tf->tf_Width;
		}
		EndUndo(page);
		RefreshMarkedTable(page, maxcol, TRUE);
	}
	else
		RefreshToolBar(page);

	SetBusy(false, BT_APPLICATION);
}

void VARARGS68K UpdatePageFont(struct Page *page,...);
void UpdatePageFont(struct Page *page,...)
{
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, page);
	tags = va_getlinearva(ap, struct TagItem *);
	UpdatePageFontA(page, tags);
	return;
}

#else
void
UpdatePageFont(struct Page *page, ULONG tag,...)
{
	struct FontFamily *ff;
	struct TagItem *styletag;
	ULONG style,height;
	WORD rotate,shear;
	struct tableField *tf = NULL;
	long maxcol = 0;
	char t[256],s[16];

	if (!page)
		return;

	SetBusy(TRUE, BT_APPLICATION);

	ff = (struct FontFamily *)GetTagData(FA_Family, ~0L, (struct TagItem *)&tag);
	height = GetTagData(FA_PointHeight, ~0L, (struct TagItem *)&tag);

	if (ff != (APTR)~0L)
		page->pg_Family = (struct Node *)ff;		/* Auswahl für folgende übernehmen */
	if (height != ~0L) {
		// limit maximal font size to 128 pt
		if (height > (128L << 16)) 
			height = 128L << 16;
#if  0
		// limit maximal font size to 72 pt (because of MorphOS broken ft2.library)
		if (height > (72L << 16)) 
			height = 72L << 16;
#endif
		{
			styletag = FindTagItem(FA_PointHeight, (struct TagItem *)&tag);
			if (styletag)
				styletag->ti_Data = height;
		}

		page->pg_PointHeight = height;
	}

	if (page->pg_Gad.DispPos != PGS_NONE || page->pg_MarkCol != -1) {
		if ((styletag = FindTagItem(FA_Style, (struct TagItem *)&tag)) != 0)
			style = styletag->ti_Data;
		else
			style = ~0L;
		rotate = GetTagData(FA_Rotate, ~0L, (struct TagItem *)&tag);
		shear = GetTagData(FA_Shear, ~0L, (struct TagItem *)&tag);

		/********** Undo/Redo Text erstellen **********/

		t[0] = 0;
		if (ff != (APTR)~0L) {
			strcpy(t, GetString(&gLocaleInfo, MSG_FONT_UNDO));
			strcat(t, ff->ff_Node.ln_Name);
			if (height != ~0L || style != ~0L || rotate != ~0L || shear != ~0L)
				strcat(t, "/");
		}
		if (height != ~0L) {
			sprintf(t + strlen(t), "%s %ldpt", ff == (APTR)~0L ? "Schrifthöhe" : "Höhe", height >> 16);
			if (style != ~0L || rotate != ~0L || shear != ~0L)
				strcat(t, "/");

			if (prefs.pr_Table->pt_Flags & PTF_AUTOCELLSIZE) {
				while ((tf = GetMarkedFields(page, tf, FALSE)) != 0) {
					if (tf->tf_Text)
						maxcol = 1;
				}

				if (maxcol) {
					sprintf(s, "%ld pt", ((height*4) >> 16)/3);
					ChangeCellSize(page,NULL,s,CCS_MINHEIGHT,NULL);
					maxcol = 0;
				}
			}
		}

		if (style != ~0L) {
			sprintf(t+strlen(t),"%s ",ff == (APTR)~0L && height == ~0L ? "Schriftstil" : "Stil");
			if (rotate != ~0L || shear != ~0L)
				strcat(t,"/");
			if (style == FS_PLAIN)
				strcat(t,GetString(&gLocaleInfo, MSG_FONT_PLAIN_NAME));
			else {
				BOOL last = FALSE;

				if (style & FS_BOLD) {
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_BOLD_NAME));
					last = TRUE;
				}
				if (style & FS_ITALIC) {
					if (last)
						strcat(t, "/");
					else
						last = TRUE;
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_ITALICS_NAME));
				}
				if (style & (FS_UNDERLINED | FS_DOUBLE_UNDERLINED | FS_STRIKE_THROUGH)) {
					if (last)
						strcat(t, "/");
				}
				if (style & FS_UNDERLINED)
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_UNDERLINED_NAME));
				else if (style & FS_DOUBLE_UNDERLINED)
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_DOUBLE_UNDERLINED_UNDO));
				else if (style & FS_STRIKE_THROUGH)
					strcat(t, GetString(&gLocaleInfo, MSG_FONT_STRIKE_THROUGH_UNDO));
			}
		}
		if (rotate != ~0L) {
			if (shear != ~0L)
				strcat(t,"/");
			sprintf(t+strlen(t), GetString(&gLocaleInfo, MSG_FONT_ROTATION_UNDO), rotate);
		}
		if (shear != ~0L)
			sprintf(t+strlen(t), GetString(&gLocaleInfo, MSG_FONT_SHEAR_UNDO), shear);

		/********** Änderungen durchführen **********/

		BeginUndo(page, UNDO_BLOCK, t);

		while ((tf = GetMarkedFields(page, tf, TRUE /*FALSE*/)) != 0) {
			if (style != ~0L) {
				if (style & FS_ALLBITS)
					styletag->ti_Data = style & ~FS_ALLBITS;
				else if (style & FS_UNSET)
					styletag->ti_Data = tf->tf_FontInfo->fi_Style & ~style;
				else if (style & (FS_UNDERLINED | FS_DOUBLE_UNDERLINED | FS_STRIKE_THROUGH))
					styletag->ti_Data = (tf->tf_FontInfo->fi_Style & ~(FS_UNDERLINED | FS_DOUBLE_UNDERLINED | FS_STRIKE_THROUGH)) | style;
				else
					styletag->ti_Data = tf->tf_FontInfo->fi_Style | style;

				if (style == FS_PLAIN)
					styletag->ti_Data = FS_PLAIN;
			}

			tf->tf_FontInfo = SetFontInfoA(tf->tf_FontInfo, page->pg_DPI, (struct TagItem *)&tag);
			tf->tf_Flags |= TFF_FONTSET;

			if (maxcol < tf->tf_Col+tf->tf_Width) // Breite vorher
				maxcol = tf->tf_Col+tf->tf_Width;

			SetTFWidth(page, tf);

			if (maxcol < tf->tf_Col+tf->tf_Width) // Breite nachher
				maxcol = tf->tf_Col+tf->tf_Width;
		}
		EndUndo(page);
		RefreshMarkedTable(page, maxcol, TRUE);
	}
	else
		RefreshToolBar(page);

	SetBusy(false, BT_APPLICATION);
}
#endif

void 
DisposePage(struct Page *page)
{
	struct gGroup *gg;

#ifndef __amigaos4__
	MyRemove(page);
#endif

	if (rxpage == page) {
		// Nachfolger für die aktuelle Seite finden
		struct Mappe *mp = page->pg_Mappe;

		if (!IsListEmpty((struct List *)&mp->mp_Pages))
			SetMainPage((APTR)mp->mp_Pages.mlh_Head);
		else if (mp->mp_Node.ln_Succ->ln_Succ)
			SetMainPage(((struct Mappe *)mp->mp_Node.ln_Succ)->mp_actPage);
		else if (mp->mp_Node.ln_Pred->ln_Pred)
			SetMainPage(((struct Mappe *)mp->mp_Node.ln_Pred)->mp_actPage);
		else
			SetMainPage(NULL);
	}

#ifdef __amigaos4__
	MyRemove(page);
#endif

	if (page->pg_Gad.DispPos >= 0)
		FreeTabGadget(page);

	// Diagramme & Objekte freigeben
	if (page->pg_ObjectOrder) {
		FreePooled(pool, page->pg_ObjectOrder, sizeof(APTR) * page->pg_OrderSize);
		page->pg_ObjectOrder = NULL;
	}

	while ((gg = (struct gGroup *)MyRemHead(&page->pg_gGroups)) != 0)
		FreeGGroup(gg);

	if (!ende)
		FreeTable(page);

	FreePooled(pool, page, sizeof(struct Page));
}


void
SetMapName(struct Mappe *mp, STRPTR name)
{
	struct TreeNode *tn;

	if (!mp || !name)
		return;

	FreeString(mp->mp_Node.ln_Name);
	mp->mp_Node.ln_Name = AllocString(name);

	if ((tn = FindTreeSpecial(&prefstree.tl_Tree, mp)) != 0) {
		tn->tn_Node.in_Name = mp->mp_Node.ln_Name;
		RefreshLockList((struct MinList *)&prefstree);
		RefreshLockList((struct MinList *)&gProjects);
	}
}


bool
DisposeProject(struct Mappe *mp)
{
	struct Page *page;
	struct Window *win,*swin;
	struct TreeNode *tn;
	long rc;

	if (mp->mp_Modified) {
		rc = DoRequest(GetString(&gLocaleInfo, MSG_SAVE_CHANGED_PROJECT_REQ),
				ende ? GetString(&gLocaleInfo, MSG_YES_NO_REQ) : GetString(&gLocaleInfo, MSG_YES_NO_CANCEL_REQ), mp->mp_Node.ln_Name);
		if (rc == 1) {
#ifdef __amigaos4__
			page = rxpage;  rxpage = (struct Page *)mp->mp_Pages.mlh_Head; //make sure that win position is although saved when closing window
			processIntCmd("SAVE");                                         //with close-button of the window
			mp->mp_Window = NULL;
#else
			mp->mp_Window = NULL;
			page = rxpage;  rxpage = (struct Page *)mp->mp_Pages.mlh_Head;
			processIntCmd("SAVE");
#endif
			rxpage = page;
		} else if (!rc && !ende)
			return false;
	}

	RemoveFromLockedList(&gProjects, (struct MinNode *)mp);
	FreeLockList(mp);
		// frees all locks related to this map

	// close all windows relating to this project 

	for (win = scr ? scr->FirstWindow : NULL; win; win = swin) {
		struct winData *wd;
		swin = win->NextWindow;
#ifdef __amigaos4__
		if(win->UserPort != iport)	//Not a ignition then next
			continue;
#endif
		if ((wd = (APTR)win->UserData) && WindowIsProjectDependent(wd->wd_Type)) {
			if (wd->wd_Data == mp)
			{
				CloseAppWindow(win, TRUE);
			}
			else {
				foreach (&mp->mp_Pages, page) {
					if (page == wd->wd_Data)
						CloseAppWindow(win, TRUE);
				}
			}
		}
	}

	while (!IsListEmpty((struct List *)&mp->mp_RexxScripts))
		DeleteRexxScript(mp,(struct RexxScript *)mp->mp_RexxScripts.mlh_Head);

	while (!IsListEmpty((struct List *)&mp->mp_Pages)) {
		(page = (APTR)mp->mp_Pages.mlh_Head)->pg_Window = NULL;
		DisposePage(page);
	}

	if ((tn = FindTreeSpecial(&prefstree.tl_Tree, mp)) != 0) {
		RemoveFromLockedList((struct MinList *)&prefstree, (struct MinNode *)tn);

		FreeTreeNodes(pool, &tn->tn_Nodes);
		FreePooled(pool, tn, sizeof(struct TreeNode));
	}

	{
		struct RexxScript *rxs;

		while ((rxs = (struct RexxScript *)MyRemHead(&mp->mp_RexxScripts)) != 0)
		{
			FreeRexxScript(rxs);
		}
	}
	FreeString(mp->mp_Node.ln_Name);
	FreeString(mp->mp_Path);
	FreeString(mp->mp_Title);
	FreePooled(pool, mp, sizeof(struct Mappe));

	return true;
}


void
SetMediumSize(struct Mappe *mp)
{
	if (!mp)
		return;

	mp->mp_MediumWidth = prefs.pr_Screen->ps_dimWidth;
	mp->mp_mmMediumWidth = prefs.pr_Screen->ps_mmWidth;
	mp->mp_MediumHeight = prefs.pr_Screen->ps_dimHeight;
	mp->mp_mmMediumHeight = prefs.pr_Screen->ps_mmHeight;
}


struct Mappe *
NewProject(void)
{
	struct Mappe *mp;

	if ((mp = AllocPooled(pool,sizeof(struct Mappe))) != 0) {
		struct TreeNode *tn;

		MyNewList(&mp->mp_Pages);  MyNewList(&mp->mp_Projects);  MyNewList(&mp->mp_Formats);
		MyNewList(&mp->mp_AppCmds);  MyNewList(&mp->mp_Names);  MyNewList(&mp->mp_Databases);
		MyNewList(&mp->mp_Masks);  MyNewList(&mp->mp_RexxScripts);  MyNewList(&mp->mp_CalcFormats);

		mp->mp_Node.ln_Name = AllocString(GetString(&gLocaleInfo, MSG_UNNAMED));
		mp->mp_Node.ln_Type = LNT_MAP;
		mp->mp_Flags = MPF_NOTES | MPF_UNNAMED;
		mp->mp_PrinterFlags = MPPRTF_PAGENUMBERS | MPPRTF_NAME | MPPRTF_ASYNCHRON;
		mp->mp_BorderLeft = mp->mp_BorderRight = 10240;
		mp->mp_BorderTop = mp->mp_BorderBottom = 10240;
		mp->mp_mmWidth = pageWidth[PG_DINA4];  mp->mp_mmHeight = pageHeight[PG_DINA4];
		SetMediumSize(mp);
		mp->mp_Events[EVT_TIME].ev_Intervall = 1;
		mp->mp_WindowBox.Left = -1;
		mp->mp_WindowBox.Top = -1;
		mp->mp_WindowBox.Width = -1;
		mp->mp_WindowBox.Height = -1;

		InitPrefs(&mp->mp_Prefs);
		PropagatePrefsToMap(&prefs, mp);

		if (LockList((struct MinList *)&prefstree, LNF_ADD)) {
			if ((tn = AddTreeNode(pool,&((struct TreeNode *)prefstree.tl_Tree.mlh_Head)->tn_Nodes,mp->mp_Node.ln_Name,NULL,TNF_CONTAINER | TNF_NOSUBDIRS)) != 0) {
				mp->mp_Prefs.pr_TreeNode = tn;
				tn->tn_Special = mp;
			}
			UnlockList((struct MinList *)&prefstree,LNF_ADD);
		}

		if ((mp->mp_Title = AllocPooled(pool,strlen(mp->mp_Node.ln_Name)+4)) != 0) {
			strcpy(mp->mp_Title, "** ");
			strcpy(mp->mp_Title + 3, mp->mp_Node.ln_Name);
		}
		RefreshMapPrefs(mp);

		AddLockedTail(&gProjects, (struct MinNode *)mp);
	}
	return mp;
}


struct Page * PUBLIC
NewPage(REG(a0, struct Mappe *mp))
{
	struct Page *page = NULL;
	char   t[42];

	if (mp && (page = AllocPooled(pool,sizeof(struct Page)))) {
		page->pg_Mappe = mp;
		page->pg_Cols = 0;  page->pg_Rows = 0;
		MyNewList(&page->pg_Table);
		MyNewList(&page->pg_Undos);
		MyNewList(&page->pg_gObjects);
		MyNewList(&page->pg_gGroups);
		MyNewList(&page->pg_gDiagrams);
		page->pg_APen = FindColorPen(0,0,0);
		page->pg_BPen = FindColorPen(255,255,255);
		page->pg_SelectCol = -1;
		page->pg_MarkCol = -1;
		page->pg_Zoom = 1 << 10;
		page->pg_SizeFactorX = (double)mp->mp_MediumWidth / mp->mp_mmMediumWidth;
		page->pg_SizeFactorY = (double)mp->mp_MediumHeight / mp->mp_mmMediumHeight;
		page->pg_DPI = gDPI;
		page->pg_Family = stdfamily;
		page->pg_PointHeight = stdpointheight;
		page->pg_mmStdHeight = (page->pg_PointHeight >> 16) * 490;  // 361
		page->pg_mmStdWidth = mm(page,page->pg_Mappe->mp_Prefs.pr_Disp->pd_AntiWidth << 1,TRUE);
		page->pg_StdWidth = pixel(page,page->pg_mmStdWidth,TRUE);
		page->pg_StdHeight = pixel(page,page->pg_mmStdHeight,FALSE);
		page->pg_CellTextSpace = pixel(page,TF_BORDERSPACE,TRUE);
		page->pg_Gad.DispPos = PGS_NONE;
		page->pg_Gad.cp.cp_Col = page->pg_Gad.cp.cp_Row = 1;

		sprintf(t, GetString(&gLocaleInfo, MSG_PAGE), CountNodes(&mp->mp_Pages) + 1);
		page->pg_Node.ln_Name = AllocString(t);

		mp->mp_actPage = page;
		SetMainPage(page);
		MyAddTail(&mp->mp_Pages, page);
	}
	return page;
}


void
drawline(struct Page *page, BOOL horiz, int x, int y)
{
	long i;

	for (i = -1; i < 2; i++) {
		if (horiz) {
			Move(win->RPort, x + i, page->pg_wTabY);
			Draw(win->RPort, x + i, page->pg_wTabY + page->pg_wTabH);
		} else {
			Move(win->RPort, page->pg_wTabX, y + i);
			Draw(win->RPort, page->pg_wTabX + page->pg_wTabW, y + i);
		}
	}
}


void
SetMark(struct Page *page, long col, long row, long width, long height)
{
	struct Rect32 old;
	struct Rectangle rect;
	struct Region *region;

	if (col == page->pg_MarkCol
		&& row == page->pg_MarkRow
		&& width == page->pg_MarkWidth
		&& height == page->pg_MarkHeight || page->pg_MarkCol != -1
		&& page->pg_MarkX1 == page->pg_MarkX2)
		return;

	if (col != -1)
		setTableCoord(page, (struct Rect32 *)&page->pg_MarkX1, col, row, width, height);
	CopyMem(&page->pg_MarkCol, &old, sizeof(struct Rect32));
#ifdef __amigaos4__
	page->pg_MarkCol 	= col;
	page->pg_MarkRow 	= row;
	page->pg_MarkWidth	= width;
	page->pg_MarkHeight = height;
#else
	CopyMem(&col, &page->pg_MarkCol, sizeof(struct Rect32));
#endif	
	// make selection visible if necessary

	if (!page->pg_Window)
		return;

	DisplayTablePos(page);
	col = old.MinX;  row = old.MinY;  width = old.MaxX;  height = old.MaxY;
	if (col != -1)
		setTableCoord(page, &old, col, row, width, height);

	region = NewRegion();
	Rect32ToRect(&old, &rect);
	if (col != -1)
		OrRectRegion(region, &rect);
	Rect32ToRect((struct Rect32 *)&page->pg_MarkX1, &rect);
	if (page->pg_MarkCol != -1)
		XorRectRegion(region, &rect);

	InstallClipRegion(page->pg_Window->WLayer, region);
	SetDrMd(page->pg_Window->RPort, COMPLEMENT);
	RectFill(page->pg_Window->RPort, page->pg_wTabX, page->pg_wTabY,
		page->pg_wTabX + page->pg_wTabW, page->pg_wTabY + page->pg_wTabH);
	SetDrMd(page->pg_Window->RPort, JAM2);
	freeClip(page->pg_Window);
}


void
drawSelect(struct RastPort *rp, long x1, long y1, long x2, long y2)
{
	DrawDithRect(rp,x1-2,y1-2,x2+1,y1+1);
	if (y2-y1 > 3) {
		DrawDithRect(rp,x1-2,y1+2,x1+1,y2-3);
		DrawDithRect(rp,x2-2,y1+2,x2+1,y2-3);
	}
	DrawDithRect(rp,x1-2,y2-2,x2+1,y2+1);
}


void
selectString(struct Page *page, STRPTR t)
{
	char pos[32];

	if (page->pg_SelectLength) {
		t[page->pg_SelectPos] = 0;
		strcat(t,&t[page->pg_SelectPos+page->pg_SelectLength]);
	}
	if (page->pg_SelectCol != -1) {
		strcpy(pos,Coord2String(page->pg_SelectCol,page->pg_SelectRow));
		if (page->pg_SelectWidth || page->pg_SelectHeight) {
			strcat(pos,":");
			strcat(pos,Coord2String(page->pg_SelectCol+page->pg_SelectWidth,page->pg_SelectRow+page->pg_SelectHeight));
		}
		page->pg_SelectLength = strlen(pos);
		strins(&t[page->pg_SelectPos],pos);
	}
}


void
SetSelect(struct Page *page, long col, long row, long width, long height, BYTE ok)
{
	struct Rect32 old;
	struct Gadget *gad;
	char *t,*s;
	long tlen;

	if (col == page->pg_SelectCol && row == page->pg_SelectRow
		&& width == page->pg_SelectWidth && height == page->pg_SelectHeight)
		return;

	if (col != -1)
		setTableCoord(page, &page->pg_Select, col, row, width, height);

	CopyMem(&page->pg_SelectCol,&old,sizeof(struct Rect32));
#ifdef __amigaos4__
	page->pg_SelectCol 		= col;
	page->pg_SelectRow 		= row;
	page->pg_SelectWidth 	= width;
	page->pg_SelectHeight 	= height;
#else
	CopyMem(&col,&page->pg_SelectCol,sizeof(struct Rect32));
#endif
	col = old.MinX;  row = old.MinY;  width = old.MaxX;  height = old.MaxY;
	if (col != -1)
		setTableCoord(page,&old,col,row,width,height);
	makeClip(win,page->pg_wTabX,page->pg_wTabY,page->pg_wTabW,page->pg_wTabH);
	SetDrMd(win->RPort,COMPLEMENT);
	if (col != -1)
		drawSelect(win->RPort,old.MinX,old.MinY,old.MaxX,old.MaxY);
	if (page->pg_SelectCol != -1)
		drawSelect(win->RPort,page->pg_Select.MinX,page->pg_Select.MinY,page->pg_Select.MaxX,page->pg_Select.MaxY);
	SetDrMd(win->RPort,JAM2);
	freeClip(win);
	if (page->pg_SelectCol == -1 && ok == 1 || ok == 2)
		return;

	if (page->pg_Gad.DispPos > PGS_FRAME) {
		if (page->pg_Gad.tf->tf_Text) {
			tlen = strlen(page->pg_Gad.tf->tf_Text)+32;
			if ((t = AllocPooled(pool,tlen)) != 0)
				strcpy(t,page->pg_Gad.tf->tf_Text);
		} else
			t = AllocPooled(pool,tlen = 32);

		if (t) {
			selectString(page,t);
			SetTabGadget(page,t,PGS_IGNORE);
			FreePooled(pool,t,tlen);
		}
	} else if ((gad = GadgetAddress(win, GID_FORM)) != 0) {
		GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &s, TAG_END);
		tlen = strlen(s) + 32;
		if ((t = AllocPooled(pool, tlen)))
			strcpy(t,s);
		if (t) {
			selectString(page,t);
			GT_SetGadgetAttrs(gad, win, NULL, GTST_String, t, TAG_END);
			FreePooled(pool, t, tlen);
		}
	} else
		DisplayBeep(NULL);
}


void
insertFormel(struct Page *page, STRPTR t)
{
	struct Gadget *gad = NULL;
	struct Window *win;
	long pos, size;
	STRPTR in, source;

	if (page->pg_Gad.DispPos == PGS_NONE || !t || !*t) {
		DisplayBeep(NULL);
		return;
	}
	if (page->pg_Gad.DispPos == PGS_FRAME && (win = page->pg_Window) && (gad = GadgetAddress(win,GID_FORM))) {
		// use GIT_FORM as target
		GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&source,TAG_END);
		if (!*source)
			source = NULL;

		pos = ((struct StringInfo *)gad->SpecialInfo)->BufferPos;
	} else {
		// use tableField as target
		if (page->pg_Gad.DispPos == PGS_FRAME)
			SetTabGadget(page,(STRPTR)~0L,0);

		pos = page->pg_Gad.DispPos;
		source = page->pg_Gad.tf ? page->pg_Gad.tf->tf_Text : NULL;
	}

	size = zstrlen(source) + 4 + strlen(t);
	// allocate for source + func + () + = + \0

	if ((in = AllocPooled(pool, size)) != 0) {
		int offset;

		if (source) {
			if (!pos && *source == '=')
				pos++;
#ifdef __amigaos4__
			Strlcpy(in, source, pos + 1);
#else
			stccpy(in, source, pos + 1);
#endif
		}
		offset = pos;

		if (!pos) {
			// no source and no leading '='
			strcpy(in, "=");
		}

		strncat(in, t, cmdlen(t));
		strcat(in, "()");
		pos = strlen(in) - 1;

		if (source)
			strcpy(in + pos + 1, source + offset);

		/* update string in gadget/cell */

		if (gad) {
			GT_SetGadgetAttrs(gad, win, NULL, GTST_String, in, TAG_END);
			((struct StringInfo *)gad->SpecialInfo)->BufferPos = pos;
			ActivateGadget(gad, win, NULL);
		} else
			SetTabGadget(page, in, pos);

		FreePooled(pool, in, size);
	}
}


void
EditFuncCopy(struct Page *page, struct tablePos *tp, BOOL textonly)
{
	struct tableField *ptf, *tf = NULL;
	struct UndoNode *un;
	STRPTR formula;
	ULONG handle;
	LONG maxCol;

	if ((un = CreateUndo(page, UNDO_BLOCK, GetString(&gLocaleInfo, MSG_COPY_CELL_UNDO))) != 0) {
		un->un_TablePos = *tp;
		un->un_Type = UNT_BLOCK_CHANGED;
		MakeUndoRedoList(page, un, &un->un_UndoList);
	}

	maxCol = tp->tp_Col + tp->tp_Width;
	if ((ptf = page->pg_Gad.tf) != 0)
		maxCol += ptf->tf_Width;

	if ((handle = (ULONG)GetCellIterator(page, tp, (UBYTE)((long)(page->pg_Gad.tf)))) != 0) {
		while ((tf = NextCell(handle)) != 0) {
			if (ptf == tf)
				continue;

			if (textonly) {
				if (ptf->tf_Type & TFT_FORMULA) {
					tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
					SetTFText(page,tf,formula = TreeTerm(ptf->tf_Root, TRUE));
					FreeString(formula);
				} else
					SetTFText(page, tf, ptf ? ptf->tf_Original : NULL);
			} else {
				if (ptf) {
					struct tableField *copiedCell;

					if ((copiedCell = CopyCell(page, ptf)) != 0) {
						tf_col = tf->tf_Col;
						tf_row = tf->tf_Row;

						RemoveCell(page, tf, false);
							// the referencing cells don't have to be recalculated -
							// InsertCell() will do this job in a moment
						FreeTableField(tf);

						copiedCell->tf_Col = tf_col;
						copiedCell->tf_Row = tf_row;

						if (copiedCell->tf_Type & TFT_FORMULA) {
							// since we've copied the cell, tf_Root still is correct - we
							// only have to change update the term & the result string
							FreeString(copiedCell->tf_Text);  copiedCell->tf_Text = NULL;
							FreeString(copiedCell->tf_Original);
							tf->tf_Original = TreeTerm(copiedCell->tf_Root, TRUE);
						}

						// updates the text after having inserted the cell
						InsertCell(page, copiedCell, true);
					}
				} else {
					RemoveCell(page, tf, true);
					FreeTableField(tf);
				}
			}
		}
		FreeCellIterator(handle);
	}

	if (!ptf || !(ptf->tf_Type & TFT_FORMULA))
		DrawTablePos(page, tp->tp_Col, tp->tp_Row, maxCol, tp->tp_Height);
	if (un)
		EndUndo(page);
}


void
EditFuncSwapMove(struct Page *page, struct tablePos *tp, BYTE mode, BOOL textonly)
{
	struct tableField *ctf, *ptf, *tf;
	struct UndoNode *un;
	LONG col, row;

	ptf = page->pg_Gad.tf;
	col = page->pg_Gad.cp.cp_Col;
	row = page->pg_Gad.cp.cp_Row;
	tf = GetTableField(page, tp->tp_Col, tp->tp_Row);

	/*** UndoNode erstellen und Undo-Zellen hinzufügen ***/

	un = CreateUndo(page, UNDO_CELL, mode == PTEF_MOVE ? GetString(&gLocaleInfo, MSG_MOVE_CELL_UNDO)
		: GetString(&gLocaleInfo, MSG_SWAP_CELLS_UNDO));
	if (un != NULL) {
		un->un_TablePos = *tp;
		un->un_Type = UNT_CELLS_CHANGED;
		un->un_Node.ln_Type &= ~UNDO_NOREDO; // Redo soll nicht automatisch erstellt werden

		if ((ptf && (ctf = CopyCell(page, ptf))) || (mode == PTEF_SWAP && (ctf = EmptyTableField(col, row, tf ? tf->tf_Width : 0)))) {
			ctf->tf_OldWidth = tf ? tf->tf_Width : 0;
			MyAddTail(&un->un_UndoList, ctf);
		}
		if (tf && (ctf = CopyCell(page,tf)) || (ctf = EmptyTableField(tp->tp_Col,tp->tp_Row,ptf ? ptf->tf_Width : 0))) {
			ctf->tf_OldWidth = ptf ? ptf->tf_Width : 0;
			MyAddTail(&un->un_UndoList, ctf);
		}
		SortListWith(&un->un_UndoList, CompareCellPositions);
	}
	FreeTabGadget(page);  // CreateUndo() benötigt noch das TabGadget

	/*** Tausch/Verschiebung durchführen (nur Text oder alles) ***/

	if (textonly) {
		if (mode == PTEF_MOVE) {
			if (!tf && ptf && ptf->tf_Original)
				tf = AllocTableField(page, tp->tp_Col, tp->tp_Row);
			if (tf) {
				if (ptf && ptf->tf_Type & TFT_FORMULA) {
					STRPTR formula;

					tf_col = tf->tf_Col;
					tf_row = tf->tf_Row;
					SetTFText(page,tf,formula = TreeTerm(ptf->tf_Root, TRUE));
					FreeString(formula);
				} else
					SetTFText(page, tf, ptf ? ptf->tf_Original : NULL);
			}
			SetTFText(page, ptf, NULL);
		} else {
			// PTEF_SWAP
			if (!tf && ptf && ptf->tf_Original)
				tf = AllocTableField(page,tp->tp_Col,tp->tp_Row);
			else if (!ptf && tf && tf->tf_Original)
				ptf = AllocTableField(page,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);

			if (tf && ptf) {
				STRPTR t = AllocString(tf->tf_Original);
				STRPTR formula = NULL,pformula = NULL;

				if (tf->tf_Type & TFT_FORMULA) {
					tf_col = ptf->tf_Col;  tf_row = ptf->tf_Row;
					formula = TreeTerm(tf->tf_Root,TRUE);
				}
				if (ptf->tf_Type & TFT_FORMULA) {
					tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
					pformula = TreeTerm(ptf->tf_Root,TRUE);
				}
				if (pformula)
					SetTFText(page,tf,pformula);
				else
					SetTFText(page,tf,ptf->tf_Original);
				if (formula)
					SetTFText(page,ptf,formula);
				else
					SetTFText(page,ptf,t);

				FreeString(pformula);
				FreeString(formula);
			}
		}
	} else {
		// not textonly
		if (tf) {
			// target cell
			RemoveCell(page, tf, ptf != NULL);
			if (mode == PTEF_SWAP) {
				tf->tf_Col = col;
				tf->tf_Row = row;
				InsertCell(page, tf, false);
			} else {
				// PTEF_MOVE
				FreeTableField(tf);
			}
		}
		if (ptf) {
			// source cell to move/swap
			RemoveCell(page, ptf, true);
			ptf->tf_Col = tp->tp_Col;
			ptf->tf_Row = tp->tp_Row;
			InsertCell(page, ptf, false);
		}
		if (mode == PTEF_MOVE) {
			tf = ptf;
			ptf = NULL;
		}
	}

	/*** add redo-cells to the UndoNode ***/

	if (un) {
		// create redo
		if (ptf && (ctf = CopyCell(page, ptf)) || (ctf = EmptyTableField(col,row,tf ? tf->tf_Width : 0)))
			MyAddTail(&un->un_RedoList, ctf);
		if ((tf && (ctf = CopyCell(page, tf)))
			|| (mode == PTEF_SWAP && (ctf = EmptyTableField(tp->tp_Col,tp->tp_Row,ptf ? ptf->tf_Width : 0))))
			MyAddTail(&un->un_RedoList, ctf);

		SortListWith(&un->un_RedoList, CompareCellPositions);
	}

	/*** Show changes ***/

	if (!ptf)
		DrawTablePos(page, col, row, tf ? tf->tf_Width : 0, 0);
	else if (!(ptf->tf_Type & TFT_FORMULA))  // newly computed ones will be shown automatically
		DrawTableField(page, ptf);

	if (!tf)
		DrawTablePos(page,tp->tp_Col,tp->tp_Row,0,0);
	else if (!(tf->tf_Type & TFT_FORMULA))
		DrawTableField(page,tf);

	if (un)
		EndUndo(page);
}


void
EditFunc(struct Page *page, UBYTE func, struct tablePos *tp)
{
	BOOL textonly = func & PTEF_TEXTONLY;

	if (!page || !tp)
		return;
 
	func &= PTEF_FUNCMASK;
	if (func == PTEF_NONE)
		return;

	if (page->pg_Gad.DispPos > PGS_FRAME)
		SetTabGadget(page, (STRPTR)~0, PGS_FRAME);

	switch (func) {
		case PTEF_COPY:
		case PTEF_SINGLECOPY:
			EditFuncCopy(page, tp, textonly);
			break;
		case PTEF_SWAP:
		case PTEF_MOVE:
			EditFuncSwapMove(page, tp, func, textonly);
			break;
	}

	if (func != PTEF_COPY)
		CreateTabGadget(page, tp->tp_Col, tp->tp_Row, FALSE);

	RecalcTableFields(page);
}


int ha_pos;


/*  Ensures that all elements are hidden that would leave drawing artifacts
 *	during scrolling.
 *	This function is called before and after the actual scrolling action.
 *
 *  @param mode HTS_BEGIN before scrolling, HTS_END after the fact
 */
void
HideTableSpecials(struct Page *page, long mode)
{
	switch ((long)wd->wd_ExtData[6]) {
		case PWA_OBJECT:
			if (mode == HTS_BEGIN && !(page->pg_Action & PGA_REFRESH)) {
				if (page->pg_Action & PGA_CREATE) {
					long ox = page->pg_wTabX-page->pg_TabX, oy = page->pg_wTabY-page->pg_TabY;

					SetGRastPort(page->pg_Window->RPort);
					makeClip(win, page->pg_wTabX, page->pg_wTabY, page->pg_wTabW, page->pg_wTabH);
					SetDrMd(grp, COMPLEMENT);

					/* erase last drawing */

					gDoClassMethod(page->pg_CreateFromClass, NULL, GCM_ADDPOINT, grp,
						page->pg_Points,(ox << 16) | (oy & 0xffff), page->pg_CurrentPoint, GCPAM_REDRAW);

					SetDrMd(grp, JAM2);
					freeClip(win);
				} else if (page->pg_Action & PGA_MULTISELECT)
					DrawMultiSelectFrame(page, DMSF_ALL);
				else if (page->pg_Action & PGA_CHANGE)
					DrawGObjectMove(page);
				page->pg_Action |= PGA_REFRESH;
			}
			break;
		case PWA_CELL:
		case PWA_EDITFUNC:
			if (page->pg_SelectCol != -1) {
				makeClip(win,page->pg_wTabX,page->pg_wTabY,page->pg_wTabW,page->pg_wTabH);
				SetDrMd(win->RPort,COMPLEMENT);
				drawSelect(win->RPort,page->pg_Select.MinX,page->pg_Select.MinY,page->pg_Select.MaxX,page->pg_Select.MaxY);
				SetDrMd(win->RPort,JAM2);
				freeClip(win);
			}
			break;
		case PWA_CELLSIZE:
			drawline(page,(BOOL)((long)wd->wd_ExtData[5]),ha_pos,ha_pos);
			break;
	}
}


/*  Scrolls the table depending on the mouse position, and uses
 *  HideTableSpecials() to remove drawing artifacts.
 */
void
MouseScroll(struct Page *page, struct Window *win, struct winData *wd)
{
	int i, j;

	i = 2*(imsg.MouseX-wd->wd_TabX-wd->wd_TabW);
	if (i < 0 && (i = 2*(imsg.MouseX-wd->wd_TabX)) > 0)
		i = 0;
	j = 2*(imsg.MouseY-wd->wd_TabY-wd->wd_TabH);
	if (j < 0 && (j = 2*(imsg.MouseY-wd->wd_TabY)) > 0)
		j = 0;
	//if ((!page->pg_TabX && i > 0 || page->pg_TabX && i) || (!page->pg_TabY && j > 0 || page->pg_TabY && j))   // scrollt nur, wenn nötig
	if (i || j) {
		HideTableSpecials(page, HTS_BEGIN);
		ScrollTable(win, ((i += page->pg_TabX) > 0) ? i : 0,
			((j += page->pg_TabY) > 0) ? j : 0);
		SetPageProps(page);
		HideTableSpecials(page, HTS_END);
	}
}


void
handleAntis(struct Page *page, BOOL horiz)
{
	struct coordPkt cp;
	struct UndoNode *un;
	struct IntuiMessage *msg;
	struct tableField *tf;
	//struct Rectangle rect;   /* TODO: this should have been for what?? */
	BOOL mark = FALSE;
	BYTE ende = 0;

	wd->wd_ExtData[5] = (APTR)((long)horiz);
	cp = getCoordPkt(page,imsg.MouseX-wd->wd_TabX,imsg.MouseY-wd->wd_TabY);
	cp.cp_X += wd->wd_TabX;  cp.cp_Y += wd->wd_TabY;

	if (horiz) {
		if (cp.cp_X >= imsg.MouseX-1) {
			setCoordPkt(page,&cp,--cp.cp_Col,1);
			cp.cp_X += wd->wd_TabX;  cp.cp_Y += wd->wd_TabY;
		} else if (!(cp.cp_X+GetTFWidth(page,cp.cp_Col) <= imsg.MouseX+2))
			mark = TRUE;
	} else {
		if (cp.cp_Y >= imsg.MouseY-1) {
			setCoordPkt(page,&cp,1,--cp.cp_Row);
			cp.cp_X += wd->wd_TabX;  cp.cp_Y += wd->wd_TabY;
		} else if (!(cp.cp_Y+GetTFHeight(page,cp.cp_Row) <= imsg.MouseY+2))
			mark = TRUE;
	}
	if (mark) {
		wd->wd_ExtData[4] = (APTR)(horiz ? cp.cp_Col : cp.cp_Row);
		wd->wd_ExtData[6] = (APTR)PWA_TITLE;
		SetMark(page,horiz ? cp.cp_Col : 1,horiz ? 1 : cp.cp_Row,horiz ? 0 : -1,horiz ? -1 : 0);
		CreateTabGadget(page,horiz ? cp.cp_Col : 1,horiz ? 1 : cp.cp_Row,FALSE);
		return;
	}
	if (horiz && cp.cp_Col == 0 || !horiz && cp.cp_Row == 0)
		return;

	wd->wd_ExtData[6] = (APTR)PWA_CELLSIZE;
	win->Flags |= WFLG_RMBTRAP;
	SetDrMd(win->RPort,COMPLEMENT);
	if (horiz && cp.cp_X > wd->wd_TabX || !horiz && cp.cp_Y > wd->wd_TabY)
		drawline(page,horiz,cp.cp_X,cp.cp_Y);
	drawline(page,horiz,imsg.MouseX,imsg.MouseY);

	while (!ende) {
		WaitPort(iport);
		while ((msg = GT_GetIMsg(iport)) != 0) {
			switch (msg->Class) {
				case IDCMP_MOUSEBUTTONS:
					ende = msg->Code;
					break;
				case IDCMP_MOUSEMOVE:
					if (horiz) {
						msg->MouseX = max(msg->MouseX,max(cp.cp_X+3,wd->wd_TabX));
						msg->MouseX = min(msg->MouseX,wd->wd_TabX+wd->wd_TabW-1);
					} else {
						msg->MouseY = max(msg->MouseY,max(cp.cp_Y+3,wd->wd_TabY));
						msg->MouseY = min(msg->MouseY,wd->wd_TabY+wd->wd_TabH-1);
					}
					if (horiz && msg->MouseX != imsg.MouseX || !horiz && msg->MouseY != imsg.MouseY) {
						drawline(page,horiz,imsg.MouseX,imsg.MouseY);
						drawline(page,horiz,msg->MouseX,msg->MouseY);
						imsg.MouseX = msg->MouseX;  imsg.MouseY = msg->MouseY;
					}
					break;
				case IDCMP_INTUITICKS:
					if (horiz && (msg->MouseX > wd->wd_TabW+wd->wd_TabX
						|| msg->MouseX < wd->wd_TabX && cp.cp_X < wd->wd_TabX)) {
						ha_pos = imsg.MouseX;
						imsg.MouseX = msg->MouseX;
						MouseScroll(page,win,wd);
						imsg.MouseX = ha_pos;
						setCoordPkt(page,&cp,cp.cp_Col,1);
						cp.cp_X += wd->wd_TabX;  cp.cp_Y += wd->wd_TabY;
					} else if (!horiz && (msg->MouseY > wd->wd_TabH+wd->wd_TabY
						|| msg->MouseY < wd->wd_TabY && cp.cp_Y < wd->wd_TabY)) {
						ha_pos = imsg.MouseY;
						imsg.MouseY = msg->MouseY;
						MouseScroll(page,win,wd);
						imsg.MouseY = ha_pos;
						setCoordPkt(page,&cp,1,cp.cp_Row);
						cp.cp_X += wd->wd_TabX;  cp.cp_Y += wd->wd_TabY;
					}
					break;
			}
			GT_ReplyIMsg(msg);
		}
	}
	drawline(page, horiz, imsg.MouseX, imsg.MouseY);
	if (horiz && cp.cp_X > wd->wd_TabX || !horiz && cp.cp_Y > wd->wd_TabY)
		drawline(page, horiz, cp.cp_X, cp.cp_Y);
	win->Flags &= ~WFLG_RMBTRAP;
	//ReportMouse(FALSE,win);
	SetDrMd(win->RPort, JAM1);
	wd->wd_ExtData[6] = NULL;

	if (ende == MENUDOWN)
		return;

	AllocTableSize(page,horiz ? cp.cp_Col : 0,horiz ? 0 : cp.cp_Row);

	if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_CELL_SIZE_UNDO))) != 0)
		un->un_Type = UNT_CELL_SIZE;

	if (horiz) {
		MakeCellSizeUndo(un, UCSF_HORIZ, (page->pg_tfWidth+cp.cp_Col-1)->ts_mm, (page->pg_tfWidth+cp.cp_Col-1)->ts_Pixel,cp.cp_Col - 1);
		if (un) {
			un->un_TablePos.tp_Col = cp.cp_Col;
			un->un_mmUndo = mm(page,imsg.MouseX-cp.cp_X,TRUE);
		}

		(page->pg_tfWidth+cp.cp_Col - 1)->ts_Pixel = imsg.MouseX - cp.cp_X;
		(page->pg_tfWidth+cp.cp_Col - 1)->ts_mm = mm(page, imsg.MouseX - cp.cp_X, TRUE);

		for (tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ) {
			if (tf->tf_Col <= cp.cp_Col && tf->tf_Width+tf->tf_Col >= cp.cp_Col)
				SetTFWidth(page,tf);
		}
		/*rect.MinX = cp.cp_X; rect.MaxX = wd->wd_TabX+wd->wd_TabW;
		rect.MinY = wd->wd_TabY;  rect.MaxY = wd->wd_TabY+wd->wd_TabH;*/
	} else {
		MakeCellSizeUndo(un,UCSF_VERT,(page->pg_tfHeight+cp.cp_Row-1)->ts_mm,(page->pg_tfHeight+cp.cp_Row-1)->ts_Pixel,cp.cp_Row-1);
		if (un) {
			un->un_TablePos.tp_Row = cp.cp_Row;
			un->un_mmRedo = mm(page,imsg.MouseY-cp.cp_Y,FALSE);
		}

		(page->pg_tfHeight+cp.cp_Row-1)->ts_Pixel = imsg.MouseY-cp.cp_Y;
		(page->pg_tfHeight+cp.cp_Row-1)->ts_mm = mm(page,imsg.MouseY-cp.cp_Y,FALSE);
		/*rect.MinX = wd->wd_TabX;  rect.MaxX = wd->wd_TabX+wd->wd_TabW;
		rect.MinY = cp.cp_Y; rect.MaxY = wd->wd_TabY+wd->wd_TabH;*/
	}

	RecalcTableSize(page);

	if (page->pg_Gad.DispPos > PGS_NONE) {
		SetCellCoordPkt(page,&page->pg_Gad.cp,page->pg_Gad.tf,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);
		page->pg_Gad.cp.cp_X += wd->wd_TabX;
		page->pg_Gad.cp.cp_Y += wd->wd_TabY;
	}
	if (page->pg_MarkCol != -1)
		setTableCoord(page,(APTR)&page->pg_MarkX1,page->pg_MarkCol,page->pg_MarkRow,page->pg_MarkWidth,page->pg_MarkHeight);
	/*DrawTableRegion(win,page,&rect,!horiz,horiz);
	if (horiz) {
		for(tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ) {
			if (tf->tf_Col < cp.cp_Col && tf->tf_Width+tf->tf_Col >= cp.cp_Col)
				DrawTableField(page,tf);
		}
	}*/
	DrawTable(page->pg_Window);
	if (page->pg_Mappe->mp_Flags & MPF_CUSIZE)
		RecalcTableFields(page);
}


void
HandleBars(void)
{
	struct Page *page = wd->wd_Data;
	struct Gadget *gad;
	struct Node *ln;
	long i,j;
#ifdef __amigaos4__
	STRPTR t;
#endif

	switch (imsg.Class) {
		case IDCMP_GADGETUP:
			switch ((gad = (struct Gadget *)imsg.IAddress)->GadgetID) {
				case GID_APEN:
					if (GetAttr(CGA_Color,(Object*)gad,(IPTR *)&i))
						SetPageColor(page,GetColorPen(i),NULL);
					break;
				case GID_BPEN:
					if (GetAttr(CGA_Color,(Object*)gad,(IPTR *)&i))
						SetPageColor(page,NULL,GetColorPen(i));
					break;
				case GID_FORM:
					gad->UserData = NULL;
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &ln, TAG_END);
					if (page->pg_Gad.DispPos != PGS_NONE) {
						struct tableField *tf;

						if ((tf = page->pg_Gad.tf) != 0) {
							if (tf->tf_Flags & TFF_SECURITY) {
								ErrorRequest(GetString(&gLocaleInfo, MSG_PROTECTED_CELL));
								break;
							}
							if (page->pg_Gad.DispPos > PGS_FRAME) {
								FreeTabGadget(page);
								page->pg_Gad.tf = tf;
							}
							page->pg_Gad.Undo = CopyCell(page,tf);
							FreeString(tf->tf_Original);
							FreeString(tf->tf_Text);
							tf->tf_Original = NULL;
						} else if (ln)
							tf = page->pg_Gad.tf = AllocTableField(page,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);

						if (tf) {
							page->pg_Gad.DispPos = 0;
							tf->tf_Text = AllocString((STRPTR)ln);
							CreateTabGadget(page,tf->tf_Col,tf->tf_Row,TRUE);
						}
					}
					break;
				case GID_SIZE:
#ifdef __amigaos4__
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
					if (t)
					{
					    char gt[15];
					    
					    Strlcpy(gt, t, 15);
					    if(!strstr(gt, "pt"))
					    {
					    	Strlcat(gt, " pt", 10);
					    }
						UpdatePageFont(page, FA_PointHeight, (long)(ConvertNumber(gt, CNT_POINT) *65536.0 + 0.5), TAG_END);
					}
#else
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &ln, TAG_END);
					if (ln)
						UpdatePageFont(page, FA_PointHeight, (long)(ConvertNumber((STRPTR)ln, CNT_POINT) *65536.0 + 0.5), TAG_END);
#endif
					break;
				case GID_ZOOM:
#ifdef __amigaos4__
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
					if (t)
					{
					    char gt[15];
					    
					    Strlcpy(gt, t, 15);
					    if(!strchr(gt, '%'))
					    {
					    	Strlcat(gt, "%", 10);
					    }
						SetZoom(page,(ULONG)(ConvertDegreeProcent(gt)*1024+0.5),FALSE,TRUE);
					}
#else
					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &ln, TAG_END);
					if (ln)
						SetZoom(page,(ULONG)(ConvertDegreeProcent((STRPTR)ln)*1024+0.5),FALSE,TRUE);
#endif
					break;





				case GID_CHOOSE:
					OpenAppWindow(WDT_FORMEL, TAG_END);
					break;
			}
			break;
		case IDCMP_GADGETDOWN:
			switch ((gad = (struct Gadget *)imsg.IAddress)->GadgetID) {
				case GID_FORM:
					gad->UserData = (APTR)TRUE;
					break;
				case GID_POPPER:
					i = PopUpList(win, gad, &fewfuncs, POPA_Width, GetListWidth(&fewfuncs) + 20, POPA_Left, TRUE, TAG_END);
					if (i != ~0L) {
						for (j = 0, ln = (struct Node *)fewfuncs.mlh_Head; j < i; j++, ln = ln->ln_Succ);
						insertFormel(page, ln->ln_Name);
					}
					break;
				case GID_PAGEPOPPER:
				{
					struct List *list = (struct List *)&page->pg_Mappe->mp_Pages;
					struct Node sepln, newln;

					// add special entries
					sepln.ln_Name = "-";
					newln.ln_Name = GetString(&gLocaleInfo, MSG_NEW_PAGE);
					sepln.ln_Type = newln.ln_Type = POPUP_NO_SELECT_BARLABEL;
					MyAddTail(list, &sepln);  MyAddTail(list, &newln);

					i = PopUpList(win, GadgetAddress(win, GID_PAGE), (struct MinList *)list, TAG_END);
					ln = FindListNumber((struct MinList *)list, i);

					// remove special entries
					MyRemove(&sepln);  MyRemove(&newln);

					if (ln != NULL) {
						if (ln == &newln)
							ln = (struct Node *)NewPage(page->pg_Mappe);
						if (ln != (struct Node *)&sepln && ln != (struct Node *)page)
							UpdateProjPage(win,rxpage = (struct Page *)ln);
					}
#ifdef __amigaos4__
					RecalcTableFields(rxpage); //recalc the page to refresh it
#endif
					break;
				}
				case GID_POPFONT:
					i = PopUpList(win,GadgetAddress(win,GID_FONT),&families,TAG_END);
					if (i != ~0L) {
						for (j = 0, ln = (struct Node *)families.mlh_Head; j < i; j++, ln = ln->ln_Succ);
						UpdatePageFont(page, FA_Family, (struct FontFamily *)ln, TAG_END);
					}
					break;
				case GID_POPSIZE:
					i = PopUpList(win, GadgetAddress(win, GID_SIZE), &sizes, TAG_END);
					if (i != ~0L) {
						for (j = 0, ln = (struct Node *)sizes.mlh_Head; j < i; j++, ln = ln->ln_Succ);
						UpdatePageFont(page, FA_PointHeight, (long)(atof(ln->ln_Name) * 65536 + 0.5), TAG_END);
					}
					break;
				case GID_APEN:
					if ((i = PopColors(win, gad)) != ~0L) {
						SetPageColor(page, GetColorPen(i), NULL);
						SetGadgetAttrs(gad->UserData, win, NULL, CGA_Color, i, TAG_END);
					}
					break;
				case GID_BPEN:
					if ((i = PopColors(win,gad)) != ~0L) {
						SetPageColor(page, NULL, GetColorPen(i));
						SetGadgetAttrs(gad->UserData, win, NULL, CGA_Color, i, TAG_END);
					}
					break;
				case GID_POPZOOM:
					i = PopUpList(win, GadgetAddress(win, GID_ZOOM), &zooms, TAG_END);
					if (i != ~0L) {
						for (j = 0, ln = (struct Node *)zooms.mlh_Head; j < i; j++, ln = ln->ln_Succ);
						if (ln->ln_Succ != (struct Node *)&zooms.mlh_Tail) {
							if (ln->ln_Type != POPUP_NO_SELECT_BARLABEL)
								SetZoom(page, (atol(ln->ln_Name) << 10)/100, FALSE, TRUE);
						} else
							SetZoom(page, -1, FALSE, TRUE);
					}
					break;
				case GID_PLAIN:
					i = FS_PLAIN;
					goto SetStyle;
				case GID_BOLD:
					i = FS_BOLD;
					goto SetStyle;
				case GID_ITALIC:
					i = FS_ITALIC;
					goto SetStyle;
				case GID_UNDERLINED:
					i = FS_UNDERLINED;
				SetStyle:
/*		  bug("selected? %ld\n",gad->Flags & GFLG_SELECTED);
					bug("unset? %ld\n",gad->Flags & GFLG_SELECTED ? 0 : FS_UNSET);
					{
						ULONG sel;
						bug("-> %ld\n",GetAttr(GA_Selected,gad,&sel));
						bug("---> %ld\n",sel);
					}*/
					UpdatePageFont(page, FA_Style, i | ((gad->Flags & GFLG_SELECTED) ? 0 : FS_UNSET), TAG_END);
					break;
				case GID_JLEFT:
					SetAlignment(page, TFA_LEFT, 0);
					break;
				case GID_JCENTER:
					SetAlignment(page, TFA_HCENTER, 0);
					break;
				case GID_JRIGHT:
					SetAlignment(page, TFA_RIGHT, 0);
					break;
				case GID_JTOP:
					SetAlignment(page, 0, TFA_TOP);
					break;
				case GID_JVCENTER:
					SetAlignment(page, 0, TFA_VCENTER);
					break;
				case GID_JBOTTOM:
					SetAlignment(page, 0, TFA_BOTTOM);
					break;
			}
	}
}


/*  Sorgt für notwendige Änderungen, wenn der Fokus von den Zellen
 *  auf ein Objekt übergeht:
 *  Zell-Markierungen werden aufgelöst, die aktive Zelle deaktiviert
 *  und PWA_OBJECT gesetzt.
 *
 *  @param page die Seite der Fokus-Änderung
 *  @param wd die winData-Struktur des betroffenen Fensters
 */
void
ProjectToGObjects(struct Page *page, struct winData *wd)
{
	if (!(page->pg_Mappe->mp_Flags & MPF_SCRIPTS)) {
		SetMark(page, -1, 0, 0, 0);
		FreeTabGadget(page);
		SetMousePointer(page->pg_Window, POINTER_OBJECT);
	}
	if (wd)
		wd->wd_ExtData[6] = (APTR)PWA_OBJECT;
}


/*  Testet, welcher Mauszeiger an der aktuellen Position benutzt
 *  werden sollte.
 */
int32
MouseOverSpecial(struct Page *page, struct winData *wd)
{
	struct PrefDisp *pd = page->pg_Mappe->mp_Prefs.pr_Disp;
	struct coordPkt cp;

	cp = getCoordPkt(page,imsg.MouseX-wd->wd_TabX,imsg.MouseY-wd->wd_TabY);
	cp.cp_X += wd->wd_TabX;  cp.cp_Y += wd->wd_TabY;

	if (imsg.MouseX >= wd->wd_TabX-pd->pd_AntiWidth && imsg.MouseX <= wd->wd_TabX
		&& imsg.MouseY > wd->wd_TabY && imsg.MouseY < wd->wd_TabY+wd->wd_TabH) {
		if (imsg.MouseY < cp.cp_Y+2 && imsg.MouseY >= cp.cp_Y && cp.cp_Row > 1
			|| imsg.MouseY <= cp.cp_Y+cp.cp_H && imsg.MouseY > cp.cp_Y+cp.cp_H-3)
			return POINTER_ROWHEIGHT;
	} else if (imsg.MouseX > wd->wd_TabX && imsg.MouseX <= wd->wd_TabX+wd->wd_TabW
		&& imsg.MouseY > wd->wd_TabY-pd->pd_AntiHeight && imsg.MouseY < wd->wd_TabY) {
		if (imsg.MouseX < cp.cp_X+2 && imsg.MouseX >= cp.cp_X && cp.cp_Col > 1 || imsg.MouseX <= cp.cp_X+cp.cp_W && imsg.MouseX > cp.cp_X+cp.cp_W-3)
			return POINTER_COLUMNWIDTH;
	} else if (imsg.MouseX > page->pg_wTabX && imsg.MouseX < page->pg_wTabX+page->pg_wTabW
		&& imsg.MouseY > page->pg_wTabY && imsg.MouseY < page->pg_wTabY+page->pg_wTabH) {
		struct gGroup *gg;
		long   pos;

		foreach (&page->pg_gGroups,gg) {
			if (gg->gg_Flags & GOF_SELECTED
				&& CheckGGroup(page, gg, imsg.MouseX - page->pg_wTabX + page->pg_TabX, imsg.MouseY - page->pg_wTabY + page->pg_TabY))
				break;
		}
		if (gg->gg_Node.mln_Succ)
			return POINTER_OBJECTKNOB;

		if (page->pg_HotSpot == PGHS_OBJECT || MouseGGroup(page,&pos))
			return POINTER_OBJECT;
	}
	return STANDARD_POINTER;
}


void
SetProjectMouseReport(struct Page *page, BOOL set)
{
	struct Window *win;

	if (!page || !(win = page->pg_Window))
		return;

	if (set)
		win->Flags |= WFLG_RMBTRAP;
	else if (!(page->pg_Mappe->mp_Flags & MPF_SCRIPTS && page->pg_Mappe->mp_Events[EVT_RBUTTON].ev_Flags & EVF_ACTIVE && MouseOverTable((struct winData *)win->UserData)))
		win->Flags &= ~WFLG_RMBTRAP;
}


ULONG qualrcvd;
#ifdef	__amigaos4__
	BOOL mscroll = FALSE;
#endif

void ASM
handleProjIDCMP(REG(a0, struct TagItem *tags))
{
	struct coordPkt cp;
	struct Page *page = wd->wd_Data;
	struct Gadget *gad;
	struct IconObj *io;
	long i, j, k;
	BYTE interactive = page->pg_Mappe->mp_Flags & MPF_SCRIPTS;

	if (qualrcvd && (imsg.Qualifier & qualrcvd) == 0 && page->pg_Gad.DispPos == PGS_FRAME
		&& !wd->wd_ExtData[6] && page->pg_HotSpot == PGHS_CELL) {
		SetTabGadget(page,(STRPTR)~0L,(long)((ULONG)~0 >> 1));
		qualrcvd = 0;
	}

	switch (imsg.Class) {
		case IDCMP_RAWKEY:
		case IDCMP_VANILLAKEY:
			if (!qualrcvd && imsg.Class == IDCMP_RAWKEY) {
				switch (imsg.Code) {
					case 0x60:
					case 0x61:
						qualrcvd = IEQUALIFIER_SHIFT;
						break;
#ifdef	__amigaos4__
					case 0x63:
						qualrcvd = IEQUALIFIER_CONTROL;
					    break;
#endif
					case 0x64:
					case 0x65:
						qualrcvd = IEQUALIFIER_ALT;
						break;
					default:
						qualrcvd = 0;
				}
			} else
				qualrcvd = 0;

			if (!handleKey(page,NULL) && wd->wd_ExtData[6] == NULL && page->pg_HotSpot == PGHS_CELL)
				HandleTabGadget(page);
			break;
		case IDCMP_GADGETUP:
		case IDCMP_GADGETDOWN:
			HandleBars();
			break;
#ifdef	__amigaos4__
		case IDCMP_EXTENDEDMOUSE:																//Handle Mousescroll
		{
			struct	IntuiWheelData		*wheelData;
					int32				deltaY = 0, deltaX = 0;
					uint16				code = imsg.Code;
//					uint16				qual = imsg.Qualifier;

		//	check that mouse is over the window and ignore if not
//			if ((intuiMsg->MouseX < 0) ||
//				(intuiMsg->MouseX > winFrame->Window->Width) ||
//				(intuiMsg->MouseY < 0) ||
//				(intuiMsg->MouseY > winFrame->Window->Height))
//			{
//				kprintf ("IH %ld Y/X %ld/%ld out of window, no scroll\n", LL,
//						intuiMsg->MouseY, intuiMsg->MouseX);
//				break;
//			}
			wheelData = (struct IntuiWheelData *)imsg.IAddress;

			if ((code == IMSGCODE_INTUIWHEELDATA) &&
				(wheelData != NULL))
			{
				deltaY = wheelData->WheelY;
				deltaX = wheelData->WheelX;
			}
			if (deltaY == 0 && deltaX == 0)
				break;

			if (deltaY < 0)
			{
				i = PROPGAD_UP_ID;
			}
			else
			if (deltaY > 0)
			{
				i = PROPGAD_DOWN_ID;
			}
			else
			if (deltaX > 0)
			{
				i = PROPGAD_RIGHT_ID;
			}
			else
			if (deltaX < 0)
			{
				i = PROPGAD_LEFT_ID;
			}
			if ((gad = GadgetAddress(win,i)) != NULL)
				gad->Flags |= GFLG_SELECTED;
			if(qualrcvd == IEQUALIFIER_CONTROL)
			{
			    uint32 zoom = page->pg_Zoom;
			    char erg[10];
			    
//			    qualrcvd = 0;
				switch(i)
				{
				    case PROPGAD_UP_ID:
				        zoom = ((zoom / 1024.0) + 0.01) * 1024.0;// + 0.5;
				        ProcentToString(zoom ,erg);													//Check with procent value 
				        if(strchr(erg, ','))														//is there a , in string
				        	zoom += 1;																//then increase value to prevent it
				        break;
				    case PROPGAD_DOWN_ID:
				        zoom = ((zoom / 1024.0) - 0.01) * 1024.0 + 0.5;
				        ProcentToString(zoom ,erg);													//Check with procent value 
				        if(strchr(erg, ','))														//is there a , in string
				        	zoom -= 1;																//then decrease value to prevent it
				        break;
				}
				SetZoom(page, zoom, TRUE, TRUE);
			}
			else
			{
				mscroll = TRUE;
				goto	DoScroll;
			}
		}
#endif	//	def	OS4
		case IDCMP_IDCMPUPDATE:
			i = GetTagData(GA_ID,0,tags);
#ifdef	__amigaos4__
DoScroll:
#endif	//	def	OS4
			j = 1;  k = 0;
			switch (i) {
				case GID_ICONOBJ:
					i = GetTagData(GA_UserData, 0, tags);
					if (i && (io = (struct IconObj *)MyFindName(GetIconObjsList(&page->pg_Mappe->mp_Prefs),(STRPTR)i)) && io->io_AppCmd)
					{
#ifdef __amigaos4__			//must be done to save all, when a cell is active
                        if (IsOverTabGadget(wd->wd_Data))
                        {
                            UWORD oldflags = calcflags;

                            calcflags &= ~CF_REQUESTER;
                            SetTabGadget(wd->wd_Data,(STRPTR)~0L,PGS_FRAME);
                            calcflags = oldflags;
                        }
#endif
						ProcessAppCmd(page,io->io_AppCmd);
					}
					break;
				case PROPGAD_UP_ID:
				case PROPGAD_LEFT_ID:
					j = -1;
				case PROPGAD_DOWN_ID:
				case PROPGAD_RIGHT_ID:
					if (i == PROPGAD_DOWN_ID || i == PROPGAD_UP_ID)
						k = 1;
					if ((gad = GadgetAddress(win,i)) && gad->Flags & GFLG_SELECTED) {
						propadd++;
						if ((gad = GadgetAddress(win,k ? PROPGAD_VERT_ID : PROPGAD_HORIZ_ID))) {
							GetAttr(PGA_Top,(Object*)gad,(IPTR *)&i);
							if (propadd*j+i < 0) {
								if (i)
									i = 0;
								else
									break;
							} else
								i += j*propadd;

							if (i >= 0) {
								SetGadgetAttrs(gad,win,NULL,PGA_Top,i,TAG_END);
								GetAttr(PGA_Top,(Object*)gad,(IPTR *)&i);
								if (k)
								{
#ifdef	__amigaos4__
								    if(mscroll)		//Reset the selected flag which is set by mousescroll routine
								    {
								        gad = GadgetAddress(win,PROPGAD_DOWN_ID);
										gad->Flags &= ~GFLG_SELECTED;
								        gad = GadgetAddress(win,PROPGAD_UP_ID);
										gad->Flags &= ~GFLG_SELECTED;
								    }
#endif
									goto handleprojvertprop;
								}
								else
								{
#ifdef	__amigaos4__
								    if(mscroll)		//Reset the selected flag which is set by mousescroll routine
								    {
								        gad = GadgetAddress(win,PROPGAD_LEFT_ID);
										gad->Flags &= ~GFLG_SELECTED;
								        gad = GadgetAddress(win,PROPGAD_RIGHT_ID);
										gad->Flags &= ~GFLG_SELECTED;
								    }
#endif
									goto handleprojhorizprop;
								}
							}
						}
					} else
						propadd = 0;
					break;
				case PROPGAD_HORIZ_ID:
					i = GetTagData(PGA_Top, 0, tags);
				handleprojhorizprop: /***************************************************/
					i = (long)i * page->pg_PropFactorX;
					ScrollTable(win, i, page->pg_TabY);
					break;
				case PROPGAD_VERT_ID:
					i = GetTagData(PGA_Top, 0, tags);
				handleprojvertprop: /***************************************************/
					i = (long)i * page->pg_PropFactorY;
					ScrollTable(win, page->pg_TabX, i);
					break;
			}
			break;
		case IDCMP_ACTIVEWINDOW:
			if (win->Flags & WFLG_WINDOWACTIVE)
				SetMainPage(page);
			break;
		case IDCMP_MOUSEBUTTONS:
			cp = getCoordPkt(page,imsg.MouseX-wd->wd_TabX,imsg.MouseY-wd->wd_TabY);
			if (MouseOverTable(wd) && handleEvent(page,(imsg.Code & IECODE_RBUTTON) == IECODE_RBUTTON ? EVT_RBUTTON : EVT_LBUTTON,cp.cp_Col,cp.cp_Row))
				break;
			if (!(imsg.Code & IECODE_UP_PREFIX))
				SetProjectMouseReport(page,TRUE);
			switch ((long)wd->wd_ExtData[6]) {
				case PWA_OBJECT:   /* Rahmen gedrückt */
					if (!HandleGObjects(page) && page->pg_HotSpot == PGHS_CELL) {
						SetMark(page,-1, 0, 0, 0);
						CreateTabGadget(page, cp.cp_Col, cp.cp_Row, FALSE);
					}
					break;
				case PWA_CELL:   /* Bezug in Feld einfügen */
					gad = GadgetAddress(win,GID_FORM);
					if (imsg.Code == MENUDOWN)
						imsg.Code = MENUUP;
					SetSelect(page, -1, 0, 0, 0, imsg.Code == SELECTUP ? 1 : 0);
					if (imsg.Code != MENUUP) {
						if (page->pg_Gad.DispPos > PGS_FRAME)
							SetTabGadget(page,(STRPTR)~0L,page->pg_Gad.DispPos+page->pg_SelectLength);
						else if (gad) {
							((struct StringInfo *)gad->SpecialInfo)->BufferPos += page->pg_SelectLength;
							ActivateGadget(gad,win,NULL);
						}
					}
					break;
				case PWA_EDITFUNC:  /* Ecke des Eingabefeldes gezogen */
					{
						struct tablePos tp;
						UBYTE  qual = page->pg_SelectPos;

						if (imsg.Code == MENUDOWN)
							imsg.Code = MENUUP;
						CopyMem(&page->pg_SelectCol,&tp,sizeof(struct tablePos));
						SetSelect(page, -1, 0, 0, 0, 2);
						page->pg_SelectPos = 0;
						if (imsg.Code != MENUUP)
							EditFunc(page,qual,&tp);
					}
					break;
				case PWA_MARK:   /* Feld markiert */
					if (imsg.Code == MENUDOWN) {
						imsg.Code = MENUUP;
						SetMark(page,-1,0,0,0);
						if (page->pg_Gad.DispPos <= PGS_FRAME && (cp.cp_Col != page->pg_Gad.cp.cp_Col || cp.cp_Row != page->pg_Gad.cp.cp_Row)
							|| page->pg_Gad.DispPos > PGS_FRAME && cp.cp_Row != page->pg_Gad.cp.cp_Row)
							CreateTabGadget(page, cp.cp_Col, cp.cp_Row, FALSE);
					} else if (imsg.Code == SELECTUP) {
						if (page->pg_MarkCol != -1)
							SetCellCoordPkt(page,&cp,NULL,page->pg_MarkCol,page->pg_MarkRow);
						if (page->pg_Gad.DispPos <= PGS_FRAME && (cp.cp_Col != page->pg_Gad.cp.cp_Col || cp.cp_Row != page->pg_Gad.cp.cp_Row)
							|| page->pg_Gad.DispPos > PGS_FRAME && cp.cp_Row != page->pg_Gad.cp.cp_Row)
							CreateTabGadget(page, cp.cp_Col, cp.cp_Row, FALSE);
						// handleEvent(page,EVT_FIELDSELECT,cp.cp_Col,cp.cp_Row);
					}
					break;
				case PWA_TITLE:   /* Titelbeschriftung markiert */
					if (imsg.Code == MENUDOWN) {
						imsg.Code = MENUUP;
						SetMark(page, -1, 0, 0, 0);
					}
					break;
				default:
					if (imsg.Code == SELECTDOWN) {
						page->pg_Action = PGA_NONE;
						if (HandleGObjects(page))
							ProjectToGObjects(page,wd);
						else if (imsg.MouseX > wd->wd_TabX && imsg.MouseY >= wd->wd_TabY
							&& imsg.MouseX < wd->wd_TabX+wd->wd_TabW && imsg.MouseY < wd->wd_TabY+wd->wd_TabH) {
							actcp = cp;
							i = page->pg_Gad.cp.cp_W;  j = page->pg_Gad.cp.cp_H;
							if (page->pg_Gad.DispPos >= PGS_FRAME && i > 3 && j > 3
								&& page->pg_Gad.cp.cp_X+i >= imsg.MouseX && page->pg_Gad.cp.cp_X+i-3 < imsg.MouseX
								&& page->pg_Gad.cp.cp_Y+j >= imsg.MouseY && page->pg_Gad.cp.cp_Y+j-3 < imsg.MouseY) {
								/**** PWA_EDITFUNC - rechte, untere Ecke der aktiven Zelle ****/

								UBYTE shift = imsg.Qualifier & IEQUALIFIER_SHIFT ? 1 : 0;
								UBYTE alt = imsg.Qualifier & IEQUALIFIER_ALT ? 1 : 0;
								UBYTE ctrl = imsg.Qualifier & IEQUALIFIER_CONTROL ? 1 : 0;
								UBYTE qual;

								if (!ctrl) {
									if (!shift) {
										if (!alt)
											qual = PTEQ_NONE;
										else
											qual = PTEQ_ALT;
									} else if (alt)
										qual = PTEQ_SHIFTALT;
									else
										qual = PTEQ_SHIFT;
								} else if (!shift && !alt)
									qual = PTEQ_CONTROL;

								if (prefs.pr_Table->pt_EditFunc[qual] != PTEF_NONE) {
									CONST_STRPTR t = NULL;

									actcp = page->pg_Gad.cp;
									page->pg_SelectPos = prefs.pr_Table->pt_EditFunc[qual];

									switch(page->pg_SelectPos & PTEF_FUNCMASK)
									{
										case PTEF_MOVE: t = GetString(&gLocaleInfo, MSG_MOVE_CELL_HELP); break;
										case PTEF_SWAP: t = GetString(&gLocaleInfo, MSG_SWAP_CELLS_HELP); break;
										case PTEF_COPY: t = GetString(&gLocaleInfo, MSG_COPY_CELL_HELP); break;
										case PTEF_SINGLECOPY: t = GetString(&gLocaleInfo, MSG_SINGLE_COPY_CELL_HELP); break;
									}
									DrawHelpText(win,NULL,t);
									SetSelect(page,actcp.cp_Col,actcp.cp_Row,0,0,2);
									wd->wd_ExtData[6] = (APTR)PWA_EDITFUNC;
									break;
								}
							}
							if (page->pg_Gad.tf && page->pg_Gad.cp.cp_Col <= cp.cp_Col
								&& page->pg_Gad.cp.cp_Col+page->pg_Gad.tf->tf_Width >= cp.cp_Col
								&& page->pg_Gad.cp.cp_Row == cp.cp_Row && page->pg_Gad.tf->tf_Text) {
								/**** PWA_MARK - Zell-Markierung beginnen ****/

								for (i = strlen(page->pg_Gad.tf->tf_Text), j = imsg.MouseX - page->pg_Gad.cp.cp_X;
										j < OutlineLength(page->pg_Gad.tf->tf_FontInfo, page->pg_Gad.tf->tf_Text
											+ page->pg_Gad.FirstChar, i - page->pg_Gad.FirstChar); i--);

								if (page->pg_Gad.tf->tf_WidthSet != 0xffff || page->pg_Gad.tf->tf_Width == page->pg_Gad.tf->tf_MaxWidth) {
									if (page->pg_Gad.FirstChar && page->pg_Gad.FirstChar+2 >= page->pg_Gad.DispPos)
										page->pg_Gad.FirstChar--;
									else {
										for (j = strlen(page->pg_Gad.tf->tf_Text)-page->pg_Gad.FirstChar, k = GetTotalWidth(page,page->pg_Gad.tf);
												j && k < OutlineLength(page->pg_Gad.tf->tf_FontInfo,page->pg_Gad.tf->tf_Text+page->pg_Gad.FirstChar,j);j--);
										if (j < strlen(page->pg_Gad.tf->tf_Text)-page->pg_Gad.FirstChar && i >= j-2+page->pg_Gad.FirstChar)
											page->pg_Gad.FirstChar++;
									}
								}
								if (i != page->pg_Gad.DispPos)
									SetTabGadget(page,(STRPTR)~0L,i);
								if (!interactive)
									wd->wd_ExtData[6] = (APTR)PWA_MARK;		  // Blockauswahl aktivieren
								break;
							} else if (page->pg_Gad.DispPos != PGS_NONE && !interactive
								&& (page->pg_Gad.cp.cp_Col != cp.cp_Col || page->pg_Gad.cp.cp_Row != cp.cp_Row)) {
								i = 0;
								if (page->pg_Gad.DispPos > PGS_FRAME) {
									struct tableField *tf = page->pg_Gad.tf;

									if (tf && tf->tf_Text && *tf->tf_Text == '=' && *(tf->tf_Text+1) != '=')
										i = page->pg_Gad.DispPos;
								} else if ((gad = GadgetAddress(win, GID_FORM)) && gad->UserData) {
									struct StringInfo *sinfo = gad->SpecialInfo;

									if (sinfo->Buffer && *sinfo->Buffer == '=' && *(sinfo->Buffer+1) != '=')
										i = sinfo->BufferPos;
								}

								if (i) {
									/* if it's a formula and cursor position is not zero */
									if (page->pg_SelectPos+page->pg_SelectLength != i) {
										page->pg_SelectPos = i;
										page->pg_SelectLength = 0;
									}
									SetSelect(page,cp.cp_Col,cp.cp_Row,0,0,1);
									wd->wd_ExtData[6] = (APTR)PWA_CELL;
									break;
								}
							}
							SetMark(page, -1, 0, 0, 0);
							CreateTabGadget(page, cp.cp_Col, cp.cp_Row, FALSE);
							if (!interactive)
								wd->wd_ExtData[6] = (APTR)PWA_MARK;
							else
								handleEvent(page, EVT_FIELDSELECT, cp.cp_Col, cp.cp_Row);
						} else if (!interactive) {
							/**** Title bars ****/

							struct PrefDisp *pd = page->pg_Mappe->mp_Prefs.pr_Disp;

							if (imsg.MouseX >= wd->wd_TabX-pd->pd_AntiWidth && imsg.MouseX <= wd->wd_TabX && imsg.MouseY > wd->wd_TabY && imsg.MouseY < wd->wd_TabY+wd->wd_TabH)
								handleAntis(page,FALSE);
							else if (imsg.MouseX > wd->wd_TabX && imsg.MouseX <= wd->wd_TabX+wd->wd_TabW && imsg.MouseY > wd->wd_TabY-pd->pd_AntiHeight && imsg.MouseY < wd->wd_TabY)
								handleAntis(page,TRUE);
							else if (imsg.MouseX >= wd->wd_TabX-pd->pd_AntiWidth && imsg.MouseX <= wd->wd_TabX && imsg.MouseY > wd->wd_TabY-pd->pd_AntiHeight && imsg.MouseY < wd->wd_TabY) {
								if (page->pg_MarkCol == 1 && page->pg_MarkRow == 1 && page->pg_MarkWidth == -1 && page->pg_MarkHeight == -1)
									SetMark(page,-1,0,0,0);
								else
									SetMark(page,1,1,-1,-1);  // mark all
							}
						}
					}
					else if (imsg.Code == MENUDOWN)
						goto proj_contextmenu;
					break;
			}
			if (imsg.Code & IECODE_UP_PREFIX && !((long)wd->wd_ExtData[6] == PWA_OBJECT && page->pg_Action == PGA_CREATE)) {
				wd->wd_ExtData[6] = NULL;
				SetProjectMouseReport(page,FALSE);
			}
			if ((gad = GadgetAddress(win,GID_FORM)) != 0)
				gad->UserData = NULL;
			break;
		case IDCMP_MENUVERIFY:
			if (imsg.Code != MENUCANCEL || interactive)
				break;
			cp = getCoordPkt(page,imsg.MouseX-wd->wd_TabX,imsg.MouseY-wd->wd_TabY);

		proj_contextmenu:
			if ((prefs.pr_Flags & PRF_CONTEXTMENU) && !interactive) {
				struct PrefDisp *pd = page->pg_Mappe->mp_Prefs.pr_Disp;

				if (imsg.MouseX >= wd->wd_TabX-pd->pd_AntiWidth && imsg.MouseX <= wd->wd_TabX && imsg.MouseY > wd->wd_TabY && imsg.MouseY < wd->wd_TabY+wd->wd_TabH)
					HandleContext(page,CMT_VERTTITLE,cp.cp_Col,cp.cp_Row);
				else if (imsg.MouseX > wd->wd_TabX && imsg.MouseX <= wd->wd_TabX+wd->wd_TabW && imsg.MouseY > wd->wd_TabY-pd->pd_AntiHeight && imsg.MouseY < wd->wd_TabY)
					HandleContext(page,CMT_HORIZTITLE,cp.cp_Col,cp.cp_Row);
				else if (MouseOverTable(wd)) {
					int	type = CMT_CELL;
					struct gGroup *gg;

					if ((gg = MouseGGroup(page,NULL))) {
						SelectGGroup(page,gg,ACTGO_EXCLUSIVE);
						page->pg_HotSpot = PGHS_OBJECT;
						ProjectToGObjects(page,wd);
						type = CMT_OBJECT;
					} else if (page->pg_MarkCol != -1
						&& cp.cp_Col >= page->pg_MarkCol && (page->pg_MarkWidth == -1 || cp.cp_Col <= page->pg_MarkCol+page->pg_MarkWidth)
						&& cp.cp_Row >= page->pg_MarkRow && (page->pg_MarkHeight == -1 || cp.cp_Row <= page->pg_MarkRow+page->pg_MarkHeight))
						type = CMT_MORECELLS;

					HandleContext(page, type, cp.cp_Col, cp.cp_Row);
				}
			}
			break;
		case IDCMP_INTUITICKS:
			if ((k = (long)wd->wd_ExtData[6]) != 0) {
				if ((k == PWA_OBJECT && page->pg_Action != PGA_NONE || k != PWA_OBJECT)
					&& (imsg.MouseX <= wd->wd_TabX || imsg.MouseY <= wd->wd_TabY || imsg.MouseX >= wd->wd_TabX+wd->wd_TabW || imsg.MouseY >= wd->wd_TabY+wd->wd_TabH)) {
					if (!(k == PWA_TITLE && !wd->wd_ExtData[5]))
						MouseScroll(page,win,wd);
				}
				else if (k == PWA_OBJECT)	   /* frame-action? (multi-select) */
					HandleGObjects(page);
			}
			break;
		case IDCMP_MOUSEMOVE:
			if (interactive) {
				if (page->pg_Mappe->mp_Events[EVT_RBUTTON].ev_Flags & EVF_ACTIVE && MouseOverTable(wd))
					win->Flags |= WFLG_RMBTRAP;
				else
					win->Flags &= ~WFLG_RMBTRAP;
			} else if (!wd->wd_ExtData[6] && !(imsg.Qualifier & IEQUALIFIER_RBUTTON)) {
				int mouse = MouseOverSpecial(page,wd);

				if (mouse != mousepointer)
					SetMousePointer(win,mousepointer = mouse);

				break;
			}
			cp = getCoordPkt(page,imsg.MouseX-wd->wd_TabX,imsg.MouseY-wd->wd_TabY);
			if ((long)wd->wd_ExtData[6] == PWA_OBJECT)
				HandleGObjects(page);
			else if ((long)wd->wd_ExtData[6] == PWA_TITLE) {
				if (wd->wd_ExtData[5] && cp.cp_Col > 0) {
					/* horiz */
					i = cp.cp_Col;
					j = (long)wd->wd_ExtData[4];
					if (i > j)
						swmem((UBYTE *)&i, (UBYTE *)&j, sizeof(long));
					SetMark(page,i,1,j-i,-1);
				} else if (!wd->wd_ExtData[5] && cp.cp_Row > 0) {
					i = cp.cp_Row;
					j = (long)wd->wd_ExtData[4];
					if (i > j)
						swmem((UBYTE *)&i, (UBYTE *)&j, sizeof(long));
					SetMark(page,1,i,-1,j-i);
				}
			} else if (cp.cp_Col > 0 && cp.cp_Row > 0) {
				if ((long)wd->wd_ExtData[6] == PWA_MARK) {
					if (actcp.cp_Col == cp.cp_Col && actcp.cp_Row == cp.cp_Row)
						SetMark(page,-1,0,0,0);
					else
						SetMark(page,min(actcp.cp_Col,cp.cp_Col),min(actcp.cp_Row,cp.cp_Row),abs(actcp.cp_Col-cp.cp_Col),abs(actcp.cp_Row-cp.cp_Row));
				} else if ((long)wd->wd_ExtData[6] == PWA_CELL)
					SetSelect(page,min(actcp.cp_Col,cp.cp_Col),min(actcp.cp_Row,cp.cp_Row),abs(actcp.cp_Col-cp.cp_Col),abs(actcp.cp_Row-cp.cp_Row),1);
				else if ((long)wd->wd_ExtData[6] == PWA_EDITFUNC) {
					if ((page->pg_SelectPos & PTEF_FUNCMASK) == PTEF_COPY) {
						if (page->pg_SelectWidth == 0 && cp.cp_Row != actcp.cp_Row)
							cp.cp_Col = actcp.cp_Col;
						else if (page->pg_SelectHeight == 0 && cp.cp_Col != actcp.cp_Col)
							cp.cp_Row = actcp.cp_Row;
						SetSelect(page, min(actcp.cp_Col, cp.cp_Col), min(actcp.cp_Row, cp.cp_Row),
							abs(actcp.cp_Col - cp.cp_Col), abs(actcp.cp_Row - cp.cp_Row), 2);
					} else
						SetSelect(page, cp.cp_Col,cp.cp_Row, 0, 0, 2);
				}
			}
			break;
		case IDCMP_NEWSIZE:
			EraseRect(win->RPort,win->BorderLeft,win->BorderTop,win->Width-1-win->BorderRight,win->Height-1-win->BorderBottom);
#ifdef __amigaos4__
			RefreshProjectWindow(win, TRUE);
#else
			AddGList(win,MakeProjectGadgets(wd,win->Width,win->Height),-1,-1,NULL);
			DrawBars(win);
			RefreshGadgets(wd->wd_Gadgets,win,NULL);
			GT_RefreshWindow(win,NULL);
			SetPageProps(page);
#endif
			DrawTable(win);
			DisplayTablePos(page);
			break;
		case IDCMP_CLOSEWINDOW:
			if (DisposeProject(page->pg_Mappe)) {
				/*page->pg_Window = NULL;
				page->pg_Mappe->mp_Window = NULL;*/
				CloseAppWindow(win, true);
			}
			break;
	}
}


