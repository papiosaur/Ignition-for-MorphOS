/* Cell related functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"


static struct CellIterator *ci_stack;
static uint32 ci_num,ci_current,gci;


struct tableField *
GetFirstCellInRow(struct Page *page, uint32 row)
{
	if (row <= page->pg_Rows && row > 0)
		return (page->pg_tfHeight + row - 1)->ts_Cell;

	return NULL;
}


int
CompareCellPositions(struct tableField **atf, struct tableField **btf)
{
	int i;

	if (!(i = (*atf)->tf_Row - (*btf)->tf_Row))
		return (*atf)->tf_Col - (*btf)->tf_Col;

	return i;
}


void
NormalizeTablePos(struct Page *page, struct tablePos *tp, LONG *norm)
{
	if (!tp) {
		norm[0] = norm[1] = 0;
		if (page) {
			norm[2] = page->pg_Cols;
			norm[3] = page->pg_Rows;
		} else
			norm[2] = norm[3] = 0x7fffffff;

		return;
	}
	norm[0] = tp->tp_Col;  norm[1] = tp->tp_Row;
	norm[2] = tp->tp_Col+tp->tp_Width;
	norm[3] = tp->tp_Row+tp->tp_Height;

	if (tp->tp_Width == -1 || tp->tp_Height == -1) {
		if (!page) {
			norm[2] = norm[3] = 0x7fffffff;
			return;
		}
		if (page->pg_Window) {
			struct coordPkt cp;

			cp = getCoordPkt(page,((struct winData *)page->pg_Window->UserData)->wd_TabW,((struct winData *)page->pg_Window->UserData)->wd_TabH);
			if (tp->tp_Width == -1)
				norm[2] = max(cp.cp_Col,page->pg_Cols);
			if (tp->tp_Height == -1)
				norm[3] = max(cp.cp_Row,page->pg_Rows);
		} else {
			if (tp->tp_Width == -1)
				norm[2] = page->pg_Cols;
			if (tp->tp_Height == -1)
				norm[3] = page->pg_Rows;
		}
	}
}


struct CellIterator *
GetCellIteratorStruct(uint32 handle)
{
	if (!handle)
		return NULL;

	return ci_stack + handle - 1;
}


struct tableField *
NextAllocatedCell(struct CellIterator *ci)
{
	struct tableField *tf = ci->ci_Current;
	uint32 col,row;

	if (tf) {
		col = tf->tf_Col+1;
		row = tf->tf_Row;
	} else {
		col = ci->ci_Col;
		row = ci->ci_Row;
	}

	if (col > ci->ci_MaxCol) {
		// determine position of the next cell
		if (++row > ci->ci_MaxRow)
			return NULL;

		col = ci->ci_Col;
	}

	if (tf) {
		// return next cell, if there is already one
		if (tf->tf_Node.mln_Succ->mln_Succ) {
			// next entry is valid
			tf = (struct tableField *)tf->tf_Node.mln_Succ;
			if (tf->tf_Row < row)
				tf = GetFirstCellInRow(ci->ci_Page,row);

			if (tf) {
				while(tf->tf_Node.mln_Succ->mln_Succ && (tf->tf_Col < ci->ci_Col || tf->tf_Col > ci->ci_MaxCol) && tf->tf_Row == row)
					tf = (struct tableField *)tf->tf_Node.mln_Succ;

				if (col == tf->tf_Col && row == tf->tf_Row)
					return ci->ci_Current = tf;
			}
		}
	}
	if (!(tf = AllocTableField(ci->ci_Page,col,row)))
		ErrorRequest(GetString(&gLocaleInfo, MSG_ALLOC_CELL_FAILED_ERR),Coord2String(col,row));

	return ci->ci_Current = tf;
}


struct tableField *
NextCell(uint32 handle)
{
	struct CellIterator *ci;
	struct tableField *tf;
	struct Page *page;

	if (!(ci = GetCellIteratorStruct(handle)))
		return NULL;

	if (ci->ci_Allocate)
		return NextAllocatedCell(ci);

	tf = ci->ci_Current;
	if (!tf || !tf->tf_Node.mln_Succ)  // already at the end of the list
		return NULL;

	if ((page = ci->ci_Page) != 0) {
		if (tf->tf_Col > ci->ci_MaxCol) {
			ULONG row = tf->tf_Row;

			/* find the correct row */

			while ((!(tf = GetFirstCellInRow(page,++row)) || tf->tf_Col > ci->ci_MaxCol) && row < ci->ci_MaxRow);

			if (!tf || row > ci->ci_MaxRow)
				return NULL;
		}
	}
	while (tf->tf_Node.mln_Succ && (tf->tf_Col < ci->ci_Col || tf->tf_Col > ci->ci_MaxCol || tf->tf_Row < ci->ci_Row))
		tf = (struct tableField *)tf->tf_Node.mln_Succ;

	if (tf->tf_Node.mln_Succ && tf->tf_Row <= ci->ci_MaxRow) {
		ci->ci_Current = (struct tableField *)tf->tf_Node.mln_Succ;
		return tf;
	}
	return NULL;
}


uint32
GetCellIteratorHandle(void)
{
	if (++ci_current > ci_num) {
		// Stack must be enlarged
		struct CellIterator *ci;

		if ((ci = AllocPooled(pool, sizeof(struct CellIterator) * (ci_num + 1))) != 0) {
			if (ci_stack) {
				ULONG size = sizeof(struct CellIterator)*ci_num;

				CopyMem(ci_stack, ci, size);
				FreePooled(pool, ci_stack, size);
			}
			ci_stack = ci;
			ci_num++;
		} else
			return 0L;
	}
	return ci_current;
}


void
FreeCellIterator(uint32 current)
{
	D({if (current != ci_current)
		bug("  handles do not match!\n");});

	ci_current--;
}


void
RewindCellIterator(uint32 handle)
{
	struct CellIterator *ci;

	if (!(ci = GetCellIteratorStruct(handle)))
		return;

	if (!ci->ci_Allocate) {
		struct tableField *cell = NULL;
		ULONG  row = ci->ci_Row;

		while (!(cell = GetFirstCellInRow(ci->ci_Page, row)) && ++row <= ci->ci_MaxRow)
			;

		ci->ci_Current = cell;
	} else
		ci->ci_Current = NULL;
}


uint32
GetCellIterator(struct Page *page, struct tablePos *tp, UBYTE alloc)
{
	struct CellIterator *ci;
	uint32 handle;
	if (!(ci = GetCellIteratorStruct(handle = GetCellIteratorHandle())))
		return 0;
	memset(ci, 0, sizeof(struct CellIterator));

	NormalizeTablePos(page,tp,&ci->ci_Col);
	//bug("GetCellIterator: tablePos = (%ld;%ld - %ld;%ld)\n",ci->ci_Col,ci->ci_Row,ci->ci_MaxCol,ci->ci_MaxRow);

	if (!alloc) {
		struct tableField *tf = NULL;
		ULONG  row = ci->ci_Row;

		while (!(tf = GetFirstCellInRow(page,row)) && ++row <= ci->ci_MaxRow);
			//D(bug("GetCellIterator: tf = %lx at row %ld\n",tf,row-1));

		ci->ci_Current = tf;
		//D(bug("GetCellIterator: ci_Current = %lx (%ld:%ld)\n",tf,tf ? tf->tf_Col : -1,tf ? tf->tf_Row : -1));
	}
	ci->ci_Page = page;
	ci->ci_Allocate = alloc;

	return handle;
}


uint32
GetCellIteratorFromList(struct MinList *list,struct tablePos *tp)
{
	struct CellIterator *ci;
	uint32 handle;

	if (!(ci = GetCellIteratorStruct(handle = GetCellIteratorHandle())))
		return 0;

	memset(ci,0,sizeof(struct CellIterator));

	ci->ci_Current = (struct tableField *)list->mlh_Head;
	NormalizeTablePos(NULL,tp,&ci->ci_Col);

	if (!tp) {
		struct tableField *tf;

		ULONG mincol = ~0,maxcol = 0,minrow = ~0,maxrow = 0;

		foreach(list, tf) {
			if (tf->tf_Col < mincol)
				mincol = tf->tf_Col;
			if (tf->tf_Col > maxcol)
				maxcol = tf->tf_Col;
			if (tf->tf_Row < minrow)
				minrow = tf->tf_Row;
			if (tf->tf_Row > maxrow)
				maxrow = tf->tf_Row;
		}
		ci->ci_Col = mincol;  ci->ci_Row = minrow;
		ci->ci_MaxCol = maxcol;  ci->ci_MaxRow = maxrow;
	}
	return handle;
}


struct tableField *
GetFirstCell(struct Page *page,struct tablePos *tp)
{
	struct tableField *tf;
	uint32 handle;

	if ((handle = GetCellIterator(page, tp, FALSE)) != 0) {
		tf = NextCell(handle);
		FreeCellIterator(handle);
	}
	return tf;
}


struct tableField *
GetMarkedFields(struct Page *page, struct tableField *tf, BOOL alloc)
{
	if (page->pg_MarkCol == -1) {
		// No block is marked
		if (!tf) {
			if (alloc && !page->pg_Gad.tf && page->pg_Gad.DispPos != PGS_NONE)
				page->pg_Gad.tf = AllocTableField(page,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);
			return(page->pg_Gad.tf);
		}
		return NULL;
	} 
	else if (!tf)
		gci = GetCellIterator(page,(struct tablePos *)&page->pg_MarkCol,alloc);

	if (gci) {
		if ((tf = NextCell(gci)) != 0)
			return tf;

		FreeCellIterator(gci);
		gci = 0;
	}
	return NULL;
}


extern struct MinList *merker;

struct tableField *
CopyCell(struct Page *page,struct tableField *tf)
{
	struct tableField *ctf;

	if (!page || !tf)
		return NULL;

	if ((ctf = AllocPooled(pool, sizeof(struct tableField))) != 0) {
		*ctf = *tf;
		ctf->tf_Original = AllocString(tf->tf_Original);
		ctf->tf_Text = AllocString(tf->tf_Text);
		ctf->tf_Format = AllocString(tf->tf_Format);
		ctf->tf_Note = AllocString(tf->tf_Note);
		ctf->tf_FontInfo = NewFontInfo(tf->tf_FontInfo,page->pg_DPI,TAG_END);   /* take new DPI into account */
		ctf->tf_Root = CopyTree(tf->tf_Root);
		ctf->tf_Reference = NULL;
	}
	return ctf;
}


void
FreeTableField(struct tableField *tf)
{
	FreeString(tf->tf_Text);
	FreeString(tf->tf_Original);
	FreeFontInfo(tf->tf_FontInfo);
	FreeString(tf->tf_Format);
	FreeString(tf->tf_Note);
	DeleteTree(tf->tf_Root);
	
	if (tf->tf_Reference)
		FreeReference(tf->tf_Reference, true);

	FreePooled(pool, tf, sizeof(struct tableField));
}


struct tableField *
EmptyTableField(LONG col, LONG row, ULONG width)
{
	struct tableField *tf;

	if (!(tf = AllocPooled(pool,sizeof(struct tableField))))
		return NULL;

	tf->tf_Type = TFT_EMPTY;
	tf->tf_Col = col;
	tf->tf_Row = row;
	tf->tf_Width = width;
	tf->tf_MaxWidth = tf->tf_WidthSet = 0xffff;

	return tf;
}


struct tableField *
MakeTableField(struct Page *page,long col,long row)
{
	struct tableField *tf;
	struct FontInfo *fi;
	
	if ((fi = NewFontInfo(NULL, page->pg_DPI,
			FA_Family, page->pg_Family,
			FA_PointHeight, page->pg_PointHeight,
			TAG_END)) != 0) {
		if ((tf = AllocPooled(pool, sizeof(struct tableField))) != 0) {
			tf->tf_Col = col;  tf->tf_Row = row;
			tf->tf_APen = tf->tf_ReservedPen = page->pg_APen;
			tf->tf_BPen = page->pg_BPen;
			tf->tf_NegativePen = ~0L;
			tf->tf_Alignment = TFA_LEFT | TFA_BOTTOM | TFA_VIRGIN;
			tf->tf_FontInfo = fi;
			tf->tf_MaxWidth = tf->tf_WidthSet = 0xffff;
			tf->tf_Komma = -1;
			return tf;
		}
		FreeFontInfo(fi);
	}
	return NULL;
}


struct tableField * PUBLIC
AllocTableField(REG(a0, struct Page *page), REG(d0, long x), REG(d1, long y))
{
	struct tableField *tf;

	if ((tf = GetTableField(page,x,y)) != 0)
		return tf;

	if ((tf = MakeTableField(page,x,y)) != 0) {
		if (x > page->pg_Cols || y > page->pg_Rows)
			AllocTableSize(page, x, y);

		InsertCell(page, tf, false);
	}
	return tf;
}


void
RemoveCell(struct Page *page,struct tableField *tf, bool recalc)
{
	uint32 row = tf->tf_Row;

	// is this cell the first in the row?
	// if so, then update the vertical cell array
	if (tf == (page->pg_tfHeight + row - 1)->ts_Cell) {
		struct tableField *ntf = (struct tableField *)tf->tf_Node.mln_Succ;

		if (!ntf->tf_Node.mln_Succ || ntf->tf_Row != row)
			ntf = NULL;

		(page->pg_tfHeight + row - 1)->ts_Cell = ntf;
	}

	MyRemove(tf);

	// FreeReference() berechnet die Zellen eventuell neu, also muß
	// diese Zelle bereits aus der Tabelle entfernt worden sein
	FreeReference(tf->tf_Reference, recalc);
	tf->tf_Reference = NULL;

	if (page->pg_Gad.tf == tf)
		page->pg_Gad.tf = NULL;
}


void
InsertCell(struct Page *page,struct tableField *tf, bool updateText)
{
	struct tableField *stf;
#ifdef DEBUG
	if (tf->tf_Col < 1 || tf->tf_Col > page->pg_Cols ||
			tf->tf_Row < 1 || tf->tf_Row > page->pg_Rows)
		bug("InsertTableField(): cell has invalid position: %ld:%ld\n",tf->tf_Col,tf->tf_Row);
#endif

	for(stf = (APTR)page->pg_Table.mlh_Head;stf->tf_Node.mln_Succ && stf->tf_Row < tf->tf_Row;stf = (APTR)stf->tf_Node.mln_Succ);
	if (stf->tf_Node.mln_Succ && !(stf->tf_Col > tf->tf_Col || stf->tf_Row > tf->tf_Row))
		for(;stf->tf_Node.mln_Succ && stf->tf_Col < tf->tf_Col && stf->tf_Row == tf->tf_Row;stf = (APTR)stf->tf_Node.mln_Succ);
	Insert((struct List *)&page->pg_Table,(struct Node *)tf,(struct Node *)stf->tf_Node.mln_Pred);

#ifdef DEBUG
	if (stf->tf_Col == tf->tf_Col && stf->tf_Row == tf->tf_Row)
		bug("InsertTableField(): Doppelbelegung %ld:%ld\n",tf->tf_Col,tf->tf_Row);
#endif

	if (!tf->tf_Node.mln_Pred->mln_Pred || ((struct tableField *)tf->tf_Node.mln_Pred)->tf_Row != tf->tf_Row)
	{
#ifdef DEBUG
		if (!page->pg_tfHeight)
			bug("InsertTableField(): pg_tfHeight is not proper initialized\n");
#endif
		(page->pg_tfHeight+tf->tf_Row-1)->ts_Cell = tf;
	}
	SetTFWidth(page,tf);

	/* update references */

	D({if (tf->tf_Reference)
		bug("*** Reference was not removed from cell %ld:%ld (0x%lx)!\n",tf->tf_Col,tf->tf_Row,tf);});

	if (!updateText && !tf->tf_Reference && (tf->tf_Type & TFT_FORMULA) != 0 && tf->tf_Root)
		tf->tf_Reference = MakeReference(page, RTYPE_CELL, tf, tf->tf_Root);

	AssignUnresolvedReferencesForCell(page, tf);

	if (updateText)
		UpdateCellText(page, tf);
}



void
SetTFTextWidth(struct Page *page,struct tableField *tf)
{
	long   width,w;

	tf->tf_OldWidth = tf->tf_Width;

	if (tf->tf_WidthSet != 0xffff)
		tf->tf_Width = tf->tf_WidthSet;
	else if (tf->tf_Text && tf->tf_MaxWidth > 0) {
		tf->tf_Width = 0;
		width = (21 * OutlineLength(tf->tf_FontInfo,tf->tf_Text,strlen(tf->tf_Text)) ) / 20;  /* 105% Breite */
		for (w = GetTFWidth(page,tf->tf_Col);width > w && tf->tf_Width < tf->tf_MaxWidth;w += GetTFWidth(page,tf->tf_Col+tf->tf_Width))
			tf->tf_Width++;

		if (tf->tf_Col+tf->tf_Width > page->pg_Cols)
			AllocTableSize(page,tf->tf_Col+tf->tf_Width,0);
	} else
		tf->tf_Width = 0;
}


void
SetTFWidth(struct Page *page,struct tableField *tf)
{
	struct tableField *stf,*ptf;

	/*** Search next cell with text in this row ***/
	for (stf = (APTR)tf->tf_Node.mln_Succ;
		stf->tf_Node.mln_Succ && !stf->tf_Text && stf->tf_Row == tf->tf_Row;
		stf = (APTR)stf->tf_Node.mln_Succ);

	/*** set tf_MaxWidth depending on it ***/

	if (stf != (struct tableField *)&page->pg_Table.mlh_Tail && tf->tf_Row == stf->tf_Row && stf->tf_Text) {
		tf->tf_MaxWidth = stf->tf_Col-tf->tf_Col-1;
		if (tf->tf_Text /*|| page->pg_Gad.tf == tf*/)
			stf = tf;
	} else {
		tf->tf_MaxWidth = 0xffff;
		stf = tf;
	}
	SetTFTextWidth(page, tf);

	for (ptf = (APTR)tf->tf_Node.mln_Pred;
		ptf->tf_Node.mln_Pred && !ptf->tf_Text && ptf->tf_Row == tf->tf_Row;
		ptf = (APTR)ptf->tf_Node.mln_Pred);

	if (ptf != (struct tableField *)&page->pg_Table && tf->tf_Row == ptf->tf_Row && ptf->tf_Text) {
		if (stf->tf_Text/* || page->pg_Gad.tf == stf*/)
			ptf->tf_MaxWidth = stf->tf_Col-ptf->tf_Col-1;
		else
			ptf->tf_MaxWidth = 0xffff;
		SetTFTextWidth(page,ptf);
	}

	if (page->pg_Gad.tf == tf)
		page->pg_Gad.cp.cp_W = GetTotalWidth(page, tf);
}


bool
TextIsFormula(STRPTR text)
{
	return (bool)(text[0] == '=' && text[1] != '=' && text[1]);
}


void
UpdateCellText(struct Page *page, struct tableField *tf)
{
#ifdef __amigaos4__
    struct Prefs *pr = GetLocalPrefs(page->pg_Document);
    struct Name *nm;
#endif

	if (!tf)
		return;

	tf->tf_Type = 0;  tf->tf_Value = 0.0;

	if (tf->tf_Alignment & TFA_VIRGIN)
		tf->tf_Alignment = (tf->tf_Alignment & TFA_VCENTER) | TFA_LEFT | TFA_VIRGIN;

	if (tf->tf_Text) {
		tf->tf_Original = AllocString(tf->tf_Text);
		tf->tf_Type = TFT_TEXT;

		if (TextIsFormula(tf->tf_Text)) {
			// formula
			GetFormula(page, tf);
		} else if (tf->tf_Text[0] != '#') {
			// number
			GetValue(page, tf);
		} else {
			// normal text
			FreeString(tf->tf_Text);
			tf->tf_Text = AllocString(tf->tf_Original + 1);
		}
	}
	SetTFWidth(page, tf);
#ifdef __amigaos4__
	foreach (&pr->pr_Names, nm)  //refresh the names with the selected cells, so the referencing is added
        if(nm->nm_Reference)
        	UpdateReferences(nm->nm_Reference, nm->nm_Root);
#endif
	if (!tf->tf_Reference && tf->tf_Type & TFT_FORMULA)
		tf->tf_Reference = MakeReference(page, RTYPE_CELL, tf, tf->tf_Root);
	else if (tf->tf_Reference)
		UpdateReferences(tf->tf_Reference, tf->tf_Root);

	RecalcReferencingObjects(tf->tf_Reference, true);
}


void
SetTFText(struct Page *page, struct tableField *tf, STRPTR t)
{
	if (tf == NULL || DocumentSecurity(page, tf))
		return;

	if (page->pg_Gad.tf != tf || page->pg_Gad.DispPos <= PGS_FRAME) {
		FreeString(tf->tf_Text);

		if (t == (STRPTR)~0L)
			tf->tf_Text = tf->tf_Original;
		else {
			tf->tf_Text = AllocString(t);
			FreeString(tf->tf_Original);
		}
		tf->tf_Original = NULL;

		DeleteTree(tf->tf_Root);
		tf->tf_Root = NULL;

		UpdateCellText(page, tf);
	} else
		SetTabGadget(page, t, PGS_IGNORE);
}


struct tableField * PUBLIC
GetTableField(REG(a0, struct Page *page), REG(d0, long x), REG(d1, long y))
{
	struct tableField *tf;

	if (!page || x > page->pg_Cols || y > page->pg_Rows)
		return NULL;

#ifdef DEBUG
	// find cell row
	for(tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ && tf->tf_Row < y;tf = (APTR)tf->tf_Node.mln_Succ);
	// find cell column
	if (tf->tf_Node.mln_Succ && tf->tf_Row == y) {
		struct tableField *rtf = GetFirstCellInRow(page,y);

		if (rtf != tf) {
			if (rtf)
				bug("GetTableField: cells do not match: %lx (%ld:%ld), %lx (%ld:%ld)\n",tf,tf->tf_Col,tf->tf_Row,rtf,rtf->tf_Col,rtf->tf_Row);
			else
				bug("GetTableField: cell not found: %lx (%ld:%ld)\n",tf,tf->tf_Col,tf->tf_Row);
		}
		if (!rtf)
			return NULL;
#else
	{
		if (!(tf = GetFirstCellInRow(page,y)))
			return NULL;
#endif

		for (;tf->tf_Node.mln_Succ && tf->tf_Col < x && tf->tf_Row == y;tf = (APTR)tf->tf_Node.mln_Succ);
		if (tf->tf_Node.mln_Succ && tf->tf_Col == x && tf->tf_Row == y)
			return tf;
	}
	return NULL;
}


struct tableField *
GetTableFieldCoord(struct Page *page,long x,long y)
{
	struct coordPkt cp;

	if (!page)
		return NULL;

	cp = getCoordPkt(page,x,y);
	return GetTableField(page,cp.cp_Col,cp.cp_Row);
}


/** Find the cell which is assigned to the specified position
 *  @param page
 *  @param x
 *  @param y
 */

struct tableField *
GetRealTableField(struct Page *page,long x,long y)
{
	struct tableField *tf;

	if (!(tf = GetFirstCellInRow(page,y)))
		return NULL;

	for (;tf->tf_Node.mln_Succ && tf->tf_Col+tf->tf_Width < x;tf = (APTR)tf->tf_Node.mln_Succ);

	if (y == tf->tf_Row && x <= tf->tf_Col+tf->tf_Width)
		return tf;

	return NULL;
}


long
GetTotalWidth(struct Page *page,struct tableField *tf)
{
	long width = 0,i;

	if ((i = tf->tf_Width) != 0) {
		for(;i;i--)
			width += GetTFWidth(page,tf->tf_Col+i);
	}
	return GetTFWidth(page,tf->tf_Col) + width;
}
