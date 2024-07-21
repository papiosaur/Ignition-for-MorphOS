/* Table related functions
 *
 * Copyright 1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"

#ifdef __amigaos4__
	#include <clib/macros.h>
#endif

#define TRACE_TABLE 1
#if TRACE_TABLE
#	define TRACE(x) D(bug x)
#else
#	define TRACE(x) ;
#endif

#define IsMarked(p,col,row) ((p)->pr_MarkCol != -1 && ((p)->pr_MarkWidth == -1 || (p)->pr_MarkCol <= col && (p)->pr_MarkCol+(p)->pr_MarkWidth >= col) && ((p)->pr_MarkHeight == -1 || (p)->pr_MarkRow <= row && (p)->pr_MarkRow+(p)->pr_MarkHeight >= row))

extern void RefreshDiagram(struct gDiagram *gd);

extern struct RastPort *doublerp,*irp;
const  ULONG pagePtrn = 0x44441111;


void
RecalcTableSize(struct Page *page)
{
	long i,j,k;

	if (!page)
		return;

	for (i = j = k = 0;i < page->pg_Cols;i++) {
		(page->pg_tfWidth+i)->ts_Pixel = pixel(page,(page->pg_tfWidth+i)->ts_mm+j,TRUE)-k;
		j += (page->pg_tfWidth+i)->ts_mm;
		k = pixel(page, j, TRUE);
	}
	for (i = j = k = 0;i < page->pg_Rows;i++) {
		(page->pg_tfHeight+i)->ts_Pixel = pixel(page,(page->pg_tfHeight+i)->ts_mm+j,FALSE)-k;
		j += (page->pg_tfHeight+i)->ts_mm;
		k = pixel(page, j, FALSE);
	}
}


void PUBLIC
AllocTableSize(REG(a0, struct Page *page), REG(d0, long w),REG(d1, long h))
{
	struct tableSize *tsw,*tsh;
	long i;

	tsw = page->pg_tfWidth;  tsh = page->pg_tfHeight;
	if (w > page->pg_Cols && (page->pg_tfWidth = AllocPooled(pool,sizeof(struct tableSize)*w)))
	{
#ifndef __amigaos4__
		for(i = page->pg_Cols;i < w;i++)
		{
			(page->pg_tfWidth+i)->ts_Pixel = page->pg_StdWidth;
			(page->pg_tfWidth+i)->ts_mm = page->pg_mmStdWidth;
		}
#endif
		if (tsw)
		{
			CopyMem(tsw,page->pg_tfWidth,sizeof(struct tableSize)*page->pg_Cols);
			FreePooled(pool,tsw,sizeof(struct tableSize)*page->pg_Cols);
		}
#ifdef __amigaos4__
		for(i = page->pg_Cols;i < w;i++)
		{
			(page->pg_tfWidth+i)->ts_Pixel = page->pg_StdWidth;
			(page->pg_tfWidth+i)->ts_mm = page->pg_mmStdWidth;
		}
#endif
		page->pg_Cols = w;
	}
	if (h > page->pg_Rows && (page->pg_tfHeight = AllocPooled(pool,sizeof(struct tableSize)*h)))
	{
#ifndef __amigaos4__
		for(i = page->pg_Rows;i < h;i++)
		{
			(page->pg_tfHeight+i)->ts_Pixel = page->pg_StdHeight;
			(page->pg_tfHeight+i)->ts_mm = page->pg_mmStdHeight;
		}
#endif
		if (tsh)
		{
			CopyMem(tsh,page->pg_tfHeight,sizeof(struct tableSize)*page->pg_Rows);
			FreePooled(pool,tsh,sizeof(struct tableSize)*page->pg_Rows);
		}
#ifdef __amigaos4__
		for(i = page->pg_Rows;i < h;i++)
		{
			(page->pg_tfHeight+i)->ts_Pixel = page->pg_StdHeight;
			(page->pg_tfHeight+i)->ts_mm = page->pg_mmStdHeight;
		}
#endif
		page->pg_Rows = h;
	}
	SetPageProps(page);
}


void
LinkCellsToTableSize(struct Page *page)
{
	struct tableField *tf;
	ULONG  lastrow = 0,i;

	if (!page->pg_tfHeight)
		return;

	/* remove old entries */

		for (i = 0; i < page->pg_Rows; i++)
		(page->pg_tfHeight+i)->ts_Cell = NULL;

	/* create new ones */

	foreach (&page->pg_Table, tf) {
		if (lastrow != tf->tf_Row) {
			lastrow = tf->tf_Row;
			(page->pg_tfHeight + lastrow - 1)->ts_Cell = tf;
		}
	}
}


STRPTR
GetHorizTitle(struct Page *page,long num)
{
	if (!num || num > page->pg_Cols)
		return NULL;

	return (page->pg_tfWidth + num - 1)->ts_Title;
}


STRPTR
GetVertTitle(struct Page *page, long num)
{
	if (!num || num > page->pg_Rows)
		return NULL;

	return (page->pg_tfHeight + num - 1)->ts_Title;
}


int32
GetTFWidth(struct Page *page, long num)
{
	DTESTR(!page,1);

	if (!num || num > page->pg_Cols)
		return page->pg_StdWidth;

	DTESTR(!page->pg_tfWidth, 1);

	return (page->pg_tfWidth + num - 1)->ts_Pixel;
}


int32
GetTFHeight(struct Page *page,long num)
{
	DTESTR(!page,1);

	if (!num || num > page->pg_Rows)
		return page->pg_StdHeight;

	DTESTR(!page->pg_tfHeight,1);

	return (page->pg_tfHeight+num-1)->ts_Pixel;
}


struct tableField *
NextTableField(struct tableField *tf)
{
	tf = (APTR)tf->tf_Node.mln_Succ;
	if (tf->tf_Node.mln_Succ)
		return tf;

	return NULL;
}


struct tableField *
PrevTableField(struct tableField *tf)
{
	tf = (APTR)tf->tf_Node.mln_Pred;
	if (tf->tf_Node.mln_Pred)
		return tf;

	return NULL;
}


void
SetBorder(struct Page *page,BOOL block,long col,long point,long col1,long point1,long col2,long point2,long col3,long point3)
{
	struct tableField *tf = NULL;
	long   i;

	if (!page)
		return;
	BeginUndo(page,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_CHANGE_FRAME_UNDO));
	if (block && (page->pg_MarkCol != -1 || page->pg_Gad.tf))
	{
		if (page->pg_MarkCol == -1)
			SetMark(page,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row,page->pg_Gad.tf->tf_Width,0);

		if (page->pg_MarkWidth != -1)
		{
			if (col3 != ~0L)
			{
				for(i = page->pg_MarkCol;i <= page->pg_MarkCol+page->pg_MarkWidth;i++)
				{
					if ((tf = AllocTableField(page, i, page->pg_MarkRow)) != 0)
					{
						tf->tf_BorderColor[3] = col3;
						tf->tf_Border[3] = (point3) > 255 ? 255 : point3;
					}
				}
			}
			if (col2 != ~0L && page->pg_MarkHeight != -1)
			{
				for(i = page->pg_MarkCol;i <= page->pg_MarkCol+page->pg_MarkWidth;i++)
				{
					if ((tf = AllocTableField(page, i, page->pg_MarkRow+page->pg_MarkHeight)) != 0)
					{
						tf->tf_BorderColor[2] = col2;
						tf->tf_Border[2] = (point2) > 255 ? 255 : point2;
					}
				}
			}
		}
		if (page->pg_MarkHeight != -1)
		{
			if (col != ~0L)
			{
				for(i = page->pg_MarkRow;i <= page->pg_MarkRow+page->pg_MarkHeight;i++)
				{
					if ((tf = AllocTableField(page,page->pg_MarkCol,i)) != 0)
					{
						tf->tf_BorderColor[0] = col;
						tf->tf_Border[0] = (point) > 255 ? 255 : point;
					}
				}
			}
			if (col1 != ~0L && page->pg_MarkWidth != -1)
			{
				for(i = page->pg_MarkRow;i <= page->pg_MarkRow+page->pg_MarkHeight;i++)
				{
					if ((tf = AllocTableField(page,page->pg_MarkCol+page->pg_MarkWidth,i)) != 0)
					{
						tf->tf_BorderColor[1] = col1;
						tf->tf_Border[1] = (point1) > 255 ? 255 : point1;
					}
				}
			}
		}
	}
	else //Not a block 
	{
#ifdef __amigaos4__
	    long column[4], points[4];
	    
		column[0] = col; column[1] = col1; column[2] = col2; column[3] = col3;
		points[0] = point; points[1] = point1; points[2] = point2; points[3] = point3;
#endif
		while ((tf = GetMarkedFields(page, tf, TRUE)) != 0)
		{
			for(i = 0;i < 4;i++)
			{
#ifdef __amigaos4__
				if (column[i] != ~0L)
				{
					tf->tf_BorderColor[i] = column[i];
					tf->tf_Border[i] = points[i] > 255 ? 255 : points[i];
				}
#else
				if (*(&col+i*2) != ~0L)
				{
					tf->tf_BorderColor[i] = *(&col+i*2);
					tf->tf_Border[i] = *(&point+i*2) > 255 ? 255 : *(&point+i*2);
				}
#endif
			}
		}
	}
	EndUndo(page);

	DrawMarkedCells(page, -1);
}


void
InReCeKn(struct Term *t,long offset,long diff,long comp,long first,long last)
{
	struct FuncArg *fa;
	long   col,row;

	if (!t)
		return;
	InReCeKn(t->t_Left,offset,diff,comp,first,last);
	InReCeKn(t->t_Right,offset,diff,comp,first,last);

	if (t->t_Op == OP_CELL && !(offset == 0 ? t->t_AbsCol : t->t_AbsRow))
	{
		col = *(&tf_col+offset)+*(&t->t_Col+offset);
		row = *(&tf_row-offset)+*(&t->t_Row-offset);
		if (*(&tf_col+offset) > comp)
		{
			if (col < comp && row >= first && row <= last)
				*(&t->t_Col+offset) -= diff;
		}
		else
		{
			if (col > comp+abs(diff)-1 && row >= first && row <= last)
				*(&t->t_Col+offset) += diff;
		}
	}
	else if (t->t_Op == OP_FUNC)
	{
		for(fa = (APTR)t->t_Args.mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
			InReCeKn(fa->fa_Root,offset,diff,comp,first,last);
	}
}


//Insert and Remove Cells
long
InReCells(struct Page *page, UBYTE mode, long col, long row, long width, long height, LONG *mm)
{
	struct tableField *tf,*stf;
	long   comp = col,first = 1,last,diff;
	struct Database *db;
	struct MaskField *mf;
	struct Mask *ma;
	short  offset = 0;

	if (!page || DocumentSecurity(page, NULL) || (!page->pg_tfHeight &&  mode == UNT_REMOVE_VERT_CELLS) ||  (!page->pg_tfWidth &&  mode == UNT_REMOVE_HORIZ_CELLS))
		return 0;

#ifdef __amigaos4__	
	if(mode == UNT_INSERT_HORIZ_CELLS && col > page->pg_Cols)			//kein insert nötig
		return 0;
	if(mode == UNT_INSERT_VERT_CELLS && row > page->pg_Rows)			//kein insert nötig
		return 0;
#endif
		
	if (mode == UNT_INSERT_VERT_CELLS || mode == UNT_REMOVE_VERT_CELLS)
	{
		offset = 1;
		diff = height + 1;
		comp = row;
		if (width == -1)
			last = page->pg_Cols+1;
		else
		{
			first = col;
			last = col + width;
		}
	}
	else
	{
		diff = width + 1;
		if (height == -1)
			last = page->pg_Rows+1;
		else
		{
			first = row;
			last = row + height;
		}
	}
//printf("InReCells: col:%d row:%d width:%d height:%d mode:%d\n", col, row, width, height, mode);
//printf("InReCells: pg_Cols:%d pg_Rows:%d\n",page->pg_Cols, page->pg_Rows);
//printf("InReCells: offset:%d first:%d last:%d diff:%d comp:%d\n", offset, first, last, diff, comp);

	/********************* Zellgrößen aktualisieren ***********************/

	if (mode == UNT_INSERT_VERT_CELLS || mode == UNT_INSERT_HORIZ_CELLS)
	{
		comp--;
		if (!mm)   /* beim Undo hat die Tabelle bereits die passende Größe */
		{
			AllocTableSize(page, page->pg_Cols + (mode == UNT_INSERT_HORIZ_CELLS ? diff : 0),
				page->pg_Rows + (mode == UNT_INSERT_VERT_CELLS ? diff : 0));
		}

		/* AllocTableSize ist bei SHIFTxxx nicht unbedingt notwendig */

		if (mode == UNT_INSERT_VERT_CELLS && width == -1)
		{
#ifdef __amigaos4__
			MoveMem(page->pg_tfHeight+comp, page->pg_tfHeight + comp + diff, (page->pg_Rows-comp-diff) * sizeof(struct tableSize)); //src / dest bei memmove falsch, hier korrigiert
#else
			memmove(page->pg_tfHeight+comp, page->pg_tfHeight + comp + diff, (page->pg_Rows-comp-diff) * sizeof(struct tableSize));
#endif
			for(col = 0;col < diff;col++)
			{
				if (mm)
				{
					(page->pg_tfHeight+comp+col)->ts_Pixel = pixel(page,*(mm+col),FALSE);
					(page->pg_tfHeight+comp+col)->ts_mm = *(mm+col);
				}
				else
				{
					(page->pg_tfHeight+comp+col)->ts_Pixel = pixel(page,page->pg_mmStdHeight,FALSE);
					(page->pg_tfHeight+comp+col)->ts_mm = page->pg_mmStdHeight;
				}
				(page->pg_tfHeight+comp)->ts_Title = NULL; 
			}
		}
		else if (mode == UNT_INSERT_HORIZ_CELLS && height == -1)
		{
#ifdef __amigaos4__
			MoveMem(page->pg_tfWidth + comp, page->pg_tfWidth+comp + diff, (page->pg_Cols-comp-diff) * sizeof(struct tableSize)); //src / dest bei memmove falsch, hier korrigiert 
#else
			memmove(page->pg_tfWidth + comp, page->pg_tfWidth+comp + diff, (page->pg_Cols-comp-diff) * sizeof(struct tableSize));
#endif
			for (col = 0; col < diff; col++)
			{
				if (mm)
				{
					(page->pg_tfWidth+comp+col)->ts_Pixel = pixel(page,*(mm+col),TRUE);
					(page->pg_tfWidth+comp+col)->ts_mm = *(mm+col);
				}
				else
				{
					(page->pg_tfWidth+comp+col)->ts_Pixel = pixel(page,page->pg_mmStdWidth,TRUE);
					(page->pg_tfWidth+comp+col)->ts_mm = page->pg_mmStdWidth;
				}
				(page->pg_tfWidth+comp)->ts_Title = NULL; 
			}
		}
	}
	else if (mode == UNT_REMOVE_VERT_CELLS && width == -1)
	{
#ifdef __amigaos4__
		MoveMem(page->pg_tfHeight + comp + diff - 1, page->pg_tfHeight + comp - 1, (page->pg_Rows-comp-diff + 1) * sizeof(struct tableSize)); //src / dest bei memmove falsch, hier korrigiert
#else
		memmove(page->pg_tfHeight + comp + diff - 1, page->pg_tfHeight + comp - 1, (page->pg_Rows-comp-diff) * sizeof(struct tableSize));
#endif
		for(col = 0;col < diff;col++)
		{
			(page->pg_tfHeight+page->pg_Rows-1-col)->ts_Pixel = page->pg_StdHeight;
			(page->pg_tfHeight+page->pg_Rows-1-col)->ts_mm = page->pg_mmStdHeight;
			(page->pg_tfHeight+page->pg_Rows-1-col)->ts_Title = NULL;
		}
	}
	else if (mode == UNT_REMOVE_HORIZ_CELLS && height == -1)
	{
#ifdef __amigaos4__
		MoveMem(page->pg_tfWidth + comp + diff - 1, page->pg_tfWidth+comp - 1, (page->pg_Cols- comp - diff + 1)*sizeof(struct tableSize)); //src / dest bei memmove falsch, hier korrigiert
#else
		memmove(page->pg_tfWidth+comp+diff-1,page->pg_tfWidth+comp-1,(page->pg_Cols-comp-diff)*sizeof(struct tableSize));
#endif
		for(col = 0;col < diff;col++)
		{
			(page->pg_tfWidth+page->pg_Cols-1-col)->ts_Pixel = page->pg_StdWidth;
			(page->pg_tfWidth+page->pg_Cols-1-col)->ts_mm = page->pg_mmStdWidth;
			(page->pg_tfWidth+page->pg_Cols-1-col)->ts_Title = NULL;
		}
	}
	if (mode == UNT_REMOVE_VERT_CELLS || mode == UNT_REMOVE_HORIZ_CELLS)
		diff = -diff;

	/********************* Zellen aktualisieren ***********************/

	for(tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ)
	{
		if (tf->tf_Type & TFT_FORMULA)
		{
			tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
			InReCeKn(tf->tf_Root,offset,diff,comp,first,last);
		}
	}

	/********************* Namen & Datenbanken aktualisieren ***********************/

	tf_col = tf_row = 0;
	for(col = 2,ma = (APTR)&page->pg_Document->mp_Databases;col;col--)
	{
		if (col == 1)
			ma = (APTR)&page->pg_Document->mp_Prefs.pr_Names;
		for(db = (APTR)((struct List *)ma)->lh_Head;db->db_Node.ln_Succ;db = (APTR)db->db_Node.ln_Succ)
		{
			if (db->db_Page == page)
			{
				if (db->db_Node.ln_Type == NMT_DATABASE
					&& mode == UNT_INSERT_VERT_CELLS
					&& width == db->db_TablePos.tp_Width
					&& first == db->db_TablePos.tp_Col
					&& comp+1 == db->db_TablePos.tp_Row)
				{
					struct Term *t;

					if ((t = db->db_Root) && t->t_Op == OP_RANGE)
					{
						if (t->t_Right
							&& t->t_Right->t_Op == OP_CELL
							&& t->t_Right->t_Col == db->db_TablePos.tp_Col+db->db_TablePos.tp_Width)
							t->t_Right->t_Row++;
						else if (t->t_Left && t->t_Left->t_Op == OP_CELL)
							t->t_Left->t_Row++;
					}
				}
				else
					InReCeKn(db->db_Root, offset, diff, comp, first, last);

				FreeString(db->db_Content);
				db->db_Content = TreeTerm(db->db_Root, FALSE);
				if (db->db_Node.ln_Type == NMT_DATABASE)
					FillTablePos(&db->db_TablePos, db->db_Root);
			}
		}
	}

	/********************* Masken aktualisieren ***********************/

	foreach(&page->pg_Document->mp_Masks,ma)
	{
		if (ma->ma_Page == page)
		{
			foreach(&ma->ma_Fields,mf)
			{
				row = *(&mf->mf_Row-offset);
				if (row >= first && row <= last && *(&mf->mf_Col+offset) > comp)
					*(&mf->mf_Col+offset) += diff;
			}
		}
	}

	/********************* Zellen verschieben/entfernen ***********************/

	if (mode == UNT_REMOVE_VERT_CELLS || mode == UNT_REMOVE_HORIZ_CELLS)
	{
		for(tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ)
		{
			col = *(&tf->tf_Col+offset);
			row = *(&tf->tf_Row-offset);

			while (tf->tf_Node.mln_Succ && col >= comp && col <= comp-diff-1 && row >= first && row <= last)
			{
				stf = (struct tableField *)tf->tf_Node.mln_Succ;
				RemoveCell(page, tf, true);
				FreeTableField(tf);
				if ((tf = stf)->tf_Node.mln_Succ)
				{
					col = *(&tf->tf_Col+offset);
					row = *(&tf->tf_Row-offset);
				}
			}
			if (tf->tf_Node.mln_Succ)
			{
				if (col > comp-diff-1 && row >= first && row <= last)
				{
					*(&tf->tf_Col+offset) += diff;
					if (offset && width != -1 || !offset && height != -1)
					{
						RemoveCell(page, tf, false);
						InsertCell(page, tf, false);
					}
				}
			}
			else
				break;
		}
	}
	else
	{
		for(tf = (APTR)page->pg_Table.mlh_TailPred;tf->tf_Node.mln_Pred;tf = (APTR)tf->tf_Node.mln_Pred)
		{
			row = *(&tf->tf_Row-offset);
			if (*(&tf->tf_Col+offset) > comp && row >= first && row <= last)
			{
				*(&tf->tf_Col+offset) += diff;
				if (offset && width != -1 || !offset && height != -1)
				{
					RemoveCell(page, tf, false);
					InsertCell(page, tf, false);
				}
			}
		}
	}

	for(tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ)
	{
		if ((mode == UNT_INSERT_HORIZ_CELLS || mode == UNT_REMOVE_HORIZ_CELLS)
			&& tf->tf_Col <= comp
			&& (row = tf->tf_Row) >= first
			&& row <= last)
			SetTFWidth(page,tf);

		if (tf->tf_Type & TFT_FORMULA)
		{
			FreeString(tf->tf_Original);
			tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
			tf->tf_Original = TreeTerm(tf->tf_Root,TRUE);
		}
	}
	setCoordPkt(page,&page->pg_Gad.cp,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);
	page->pg_Gad.cp.cp_X += page->pg_wTabX;  page->pg_Gad.cp.cp_Y += page->pg_wTabY;
	if (page->pg_MarkCol != -1)
		setTableCoord(page,(struct Rect32 *)&page->pg_MarkX1,page->pg_MarkCol,page->pg_MarkRow,page->pg_MarkWidth,page->pg_MarkHeight);
	RecalcTableSize(page);

	/********************* Objekte & Diagramme aktualisieren ***********************/
	{
		struct Page *pg;

		foreach(&page->pg_Document->mp_Pages,pg)
		{
			struct gObject *go;

			foreach(&pg->pg_gObjects,go)
				gDoMethod(go,GCM_INSERTREMOVECELLS,page,(long)offset,diff,comp,first,last);
			foreach(&pg->pg_gDiagrams,go)
				gDoMethod(go,GCM_INSERTREMOVECELLS,page,(long)offset,diff,comp,first,last);
		}
	}
	RecalcTableFields(page);
	DrawTable(page->pg_Window);
	return 0;
}

void
MakeCellSizeUndo(struct UndoNode *un, UBYTE flags, ULONG mm, long pixel, long pos)
{
	struct UndoCellSize *ucs;

	if (!un)
		return;

	if ((ucs = AllocPooled(pool, sizeof(struct UndoCellSize))) != 0)
	{
		ucs->ucs_Flags = flags;
		ucs->ucs_Position = pos;
		ucs->ucs_mm = mm;
		ucs->ucs_Pixel = pixel;
		MyAddTail((struct List *)&un->un_UndoList, (struct Node *)ucs);
	}
}


void
ChangeCellSize(struct Page *page, STRPTR widthString, STRPTR heightString, UWORD mode, struct UndoNode *undo)
{
	struct tableField *tf;
	struct UndoNode *un = NULL;
	struct tablePos tp;
	long   minWidth = 0, minHeight = 0, mmWidth = 0;
	long   mmHeight = 0, width, height, i;

	 if (!page || widthString == heightString && mode == CCS_STANDARD && !undo || DocumentSecurity(page, NULL))
		return;

	if (!undo && (un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_CHANGE_CELL_SIZE_UNDO)))) {
		un->un_Type = UNT_CELL_SIZE;
		un->un_Mode = mode;
	}

	if (undo) {
		// Undo changes
		if ((mmWidth = minWidth = undo->un_mmWidth) != 0)
			widthString = (STRPTR)~0L;
		if ((mmHeight = minHeight = undo->un_mmHeight) != 0)
			heightString = (STRPTR)~0L;

		mode = undo->un_Mode;
		tp = undo->un_TablePos;
	} else {
		// Make changes
		if (widthString) {
			if (widthString != (STRPTR)~0L)
				mmWidth = minWidth = (long)(1024 * ConvertNumber(widthString, CNT_MM));
			if (un)
				un->un_mmWidth = mmWidth;
		}
		if (heightString) {
			if (heightString != (STRPTR)~0L)
				mmHeight = minHeight = (long)(1024 * ConvertNumber(heightString, CNT_MM));
			if (un)
				un->un_mmHeight = mmHeight;
		}
		tp = *((struct tablePos *)&page->pg_MarkCol);
	}

	if (mode & CCS_OPTWIDTH)   // prepare in case of optimum size
		widthString = (STRPTR)~0L;
	if (mode & CCS_OPTHEIGHT)
		heightString = (STRPTR)~0L;

	width = pixel(page, mmWidth, TRUE);
	height = pixel(page, mmHeight, FALSE);

	if (tp.tp_Col != -1) {
		/* More than a single cell is selected */
		if (un)
			un->un_TablePos = tp;

		if (widthString && tp.tp_Width == -1) {
			// change all widths (incl. std)
			tp.tp_Width = page->pg_Cols-tp.tp_Col;
			if (mode == CCS_STANDARD) {
				MakeCellSizeUndo(un, UCSF_STANDARD | UCSF_HORIZ, page->pg_mmStdWidth, page->pg_StdWidth, -1);
				page->pg_mmStdWidth = mmWidth;
				page->pg_StdWidth = width;
			}
		}
		if (heightString && tp.tp_Height == -1) {
			// change all heights (incl. std)
			tp.tp_Height = page->pg_Rows-tp.tp_Row;
			if (mode == CCS_STANDARD) {
				MakeCellSizeUndo(un, UCSF_STANDARD | UCSF_VERT, page->pg_mmStdHeight, page->pg_StdHeight, -1);
				page->pg_mmStdHeight = mmHeight;
				page->pg_StdHeight = height;
			}
		}
	} else if (page->pg_Gad.DispPos > PGS_NONE) {
		/* change just a single cell */
		tp.tp_Col = page->pg_Gad.cp.cp_Col;
		tp.tp_Row = page->pg_Gad.cp.cp_Row;
		tp.tp_Width = tp.tp_Height = 0;

		if (un)
			un->un_TablePos = tp;
	} else {
		/* No cell selected at all */
		FreeUndo(page, un);
		return;
	}

	AllocTableSize(page, widthString ? tp.tp_Col + tp.tp_Width : 0, heightString ? tp.tp_Row+tp.tp_Height : 0);

	if (widthString) {
		for (i = tp.tp_Col; i <= tp.tp_Col + tp.tp_Width; i++) {
			if (mode & CCS_OPTWIDTH) {
				width = 0;
				for (tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ) {
					if (tf->tf_Col == i && width < (mmWidth = OutlineLength(tf->tf_FontInfo,tf->tf_Text,tf->tf_Text ? strlen(tf->tf_Text) : 0)))
						width = mmWidth;
				}

				if (width != 0) {
					width = (width + 1) * 21 / 20;
					mmWidth = mm(page, width, TRUE);
				} else {
					// there obviously were no cells (with contents) in this region
					mmWidth = (page->pg_tfWidth+i-1)->ts_mm;
					width = pixel(page, mmWidth, TRUE);
				}
			} else if (mode & CCS_MINWIDTH) {
				mmWidth = max((page->pg_tfWidth + i - 1)->ts_mm, minWidth);
				width = pixel(page, mmWidth, TRUE);
			}

			if ((page->pg_tfWidth+i-1)->ts_mm != mmWidth) {
				MakeCellSizeUndo(un, UCSF_HORIZ, (page->pg_tfWidth+i-1)->ts_mm, (page->pg_tfWidth+i-1)->ts_Pixel, i-1);
				(page->pg_tfWidth+i-1)->ts_Pixel = width;
				(page->pg_tfWidth+i-1)->ts_mm = mmWidth;
			}
		}

		for (tf = (APTR)page->pg_Table.mlh_Head; tf->tf_Node.mln_Succ; tf = (APTR)tf->tf_Node.mln_Succ) {
			if ((tf->tf_Col >= tp.tp_Col || tf->tf_Width+tf->tf_Col >= tp.tp_Col) && tf->tf_Col <= tp.tp_Col+tp.tp_Width)
				SetTFWidth(page, tf);
		}
	}

	if (heightString) {
		for (i = tp.tp_Row; i <= tp.tp_Row + tp.tp_Height; i++) {
			if (mode & CCS_OPTHEIGHT) {
				height = 0;
				for (tf = (APTR)page->pg_Table.mlh_Head; tf->tf_Node.mln_Succ; tf = (APTR)tf->tf_Node.mln_Succ) {
					if (tf->tf_Row == i && height < (mmHeight = tf->tf_FontInfo->fi_FontSize->fs_PointHeight >> 14))
						height = mmHeight;
				}
				
				if (height != 0) {
					mmHeight = (long)(1024 * height * 25.4/72) / 3;
						// convert DPI to MM, multiply with 4/3	(*4 in the loop, /3 here)
					height = pixel(page, mmHeight, FALSE);
				} else {
					// there obviously were no cells (with contents) in this region
					mmHeight = (page->pg_tfHeight+i-1)->ts_mm;
				}
			} else if (mode & CCS_MINHEIGHT) {
				mmHeight = max((page->pg_tfHeight+i-1)->ts_mm, minHeight);
				height = pixel(page, mmHeight, FALSE);
			}

			if ((page->pg_tfHeight+i-1)->ts_mm != mmHeight) {
				MakeCellSizeUndo(un, UCSF_VERT, (page->pg_tfHeight+i-1)->ts_mm, (page->pg_tfHeight+i-1)->ts_Pixel, i-1);
				(page->pg_tfHeight+i-1)->ts_Pixel = height;
				(page->pg_tfHeight+i-1)->ts_mm = mmHeight;
			}
		}
	}

	RecalcTableSize(page);

	if (page->pg_Gad.DispPos > PGS_NONE) {
		setCoordPkt(page,&page->pg_Gad.cp,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);
		page->pg_Gad.cp.cp_X += page->pg_wTabX;
		page->pg_Gad.cp.cp_Y += page->pg_wTabY;
	}

	if (page->pg_MarkCol != -1)
		setTableCoord(page,(APTR)&page->pg_MarkX1,page->pg_MarkCol,page->pg_MarkRow,page->pg_MarkWidth,page->pg_MarkHeight);

	//if (!(un && IsListEmpty((struct List *)&un->un_UndoCells))) /* MERKER: wäre schönm, wenn das Neuzeichnen konditioniert wäre */
	DrawTable(page->pg_Window);
	/* else if (page->pg_Gad.DispPos > PGS_NONE) überflüssig zu refreshen?? */

	if (page->pg_Document->mp_Flags & MPF_CUSIZE)
		RecalcTableFields(page);
}


long PUBLIC
pixel(REG(a0, struct Page *page), REG(d0, long mm), REG(d1, BOOL width))
{
	if (width)
		return (long)(mm * page->pg_SizeFactorX + 0.5);

	return (long)(mm * page->pg_SizeFactorY + 0.5);
}


long PUBLIC
mm(REG(a0, struct Page *page), REG(d0, long pixel), REG(d1, BOOL width))
{
	if (width)
		return (long)(pixel/page->pg_SizeFactorX + 0.5);

	return (long)(pixel/page->pg_SizeFactorY + 0.5);
}


void
DrawTFText(struct RastPort *rp, struct Page *page, struct Rectangle *bounds, struct tableField *tf, long x, long y)
{
	long   w = GetTotalWidth(page, tf), h = GetTFHeight(page, tf->tf_Row);
	struct Rectangle rect;
	struct Region *region;
	APTR   layer;
	short  tw,th;
	STRPTR t;

	if (!(t = tf->tf_Text)) // || (tf->tf_Flags & TFF_SECURITY) == TFF_HIDETEXT)
		return;

	SetColors(rp, tf->tf_APen, tf->tf_BPen);

	if ((tf->tf_Flags & TFF_SECURITY) == TFF_HIDETEXT) {
		if (!(page->pg_Document->mp_Flags & MPF_SHOWHIDDEN))  // do really hide them
		{
			if ((tf->tf_Flags & TFF_SECURITY) != TFF_HIDEFORMULA || (page->pg_Document->mp_Prefs.pr_Table->pt_Flags & PTF_SHOWFORMULA))
				return;
		}
		else
		{
			if (tf->tf_BPen >= 0x7f7f7f)  // darken or lighten primary pen
				SetHighColor(rp, TintColor(tf->tf_BPen, 0.75f));
			else if (tf->tf_BPen != 0x000000)
				SetHighColor(rp, TintColor(tf->tf_BPen, 1.25f));
			else
				SetHighColor(rp,0x444444);
		}
	}

	if ((layer = rp->Layer) != 0)
	{
		region = NewRegion();
		if (bounds)
			rect = *bounds;
		else
		{
			rect.MinX = page->pg_wTabX;  rect.MinY = page->pg_wTabY;
			rect.MaxX = page->pg_wTabX + page->pg_wTabW;  rect.MaxY = page->pg_wTabY + page->pg_wTabH;
		}
		OrRectRegion(region,&rect);
		rect.MinX = x;  rect.MinY = y;  rect.MaxX = x + w - 1;  rect.MaxY = y + h - 1;
		AndRectRegion(region, &rect);
		InstallClipRegion(layer, region);
	}
	SetDrMd(rp, JAM1);
	if (tf->tf_Original)
	{
		if ((tf->tf_Alignment & TFA_HCENTER) != TFA_LEFT)
		{
			tw = OutlineLength(tf->tf_FontInfo,t,-1);
			if ((tf->tf_Alignment & TFA_HCENTER) == TFA_RIGHT)
			{
				struct tableField *ntf;

				x += w-2-tw-page->pg_CellTextSpace;

				for(ntf = tf;ntf && tf->tf_Col+tf->tf_Width > ntf->tf_Col && ntf->tf_Row == tf->tf_Row;ntf = NextTableField(ntf));
				if (ntf && ntf->tf_Col == tf->tf_Col+tf->tf_Width && ntf->tf_Row == tf->tf_Row && ntf->tf_Border[1])
					x -= ((ntf->tf_Border[1]*(page->pg_DPI >> 16)/72)+1)/32;
			}
			else
				x += (w-2-tw) >> 1;
		}
		else
		{
			x += page->pg_CellTextSpace;
			if (tf->tf_Border[0])
				x += ((tf->tf_Border[0]*(page->pg_DPI >> 16)/72)+1)/32;
		}
	}
	else
	{
		x += page->pg_CellTextSpace;
		t += page->pg_Gad.FirstChar;
	}
	if ((tf->tf_Alignment & TFA_VCENTER) != TFA_TOP)
	{
		th = tf->tf_FontInfo->fi_FontSize->fs_EMHeight;
		if ((tf->tf_Alignment & TFA_VCENTER) == TFA_BOTTOM)
			y += h-1-th;
		else
			y += (h-1-th) >> 1;
	}
	DrawText(rp, tf->tf_FontInfo, t, x, y);

	if (layer)
		freeLayerClip(layer);
}


void
DrawRastaCellWidth(struct RastPort *rp,long x,long y,long w,long h)
{
	Move(rp,x,y+h-1);
	Draw(rp,x+w-1,y+h-1);
	Draw(rp,x+w-1,y);
}


void
DrawPattern(struct RastPort *rp,UBYTE pattern,WORD x1,WORD y1,WORD x2,WORD y2,UBYTE a,UBYTE b)
{
	SetPattern(rp,pattern,a,b);  SetDrMd(rp,JAM2);
	RectFill(rp,x1,y1,x2,y2);
	SetAfPt(rp,NULL,0);
}


void
DrawTableRegion(struct Window *win,struct Page *page,struct Rectangle *refresh,BOOL vertgads,BOOL horizgads)
{
	struct RastPort *rp;
	struct gObject *go;
	struct tableField *tf;
	struct Rectangle rect;
	struct PrefDisp *pd;
	long   tabLeft,tabTop,tabXOff,tabYOff;
	long   pw,ph,x,y,i,flags,border,w,h;
	char   txt[10],nobackpen = TRUE;

	if (!win || !page || page->pg_Locked)
		return;

	rect = *refresh;  pd = page->pg_Document->mp_Prefs.pr_Disp;

	if (rect.MinX == -1)
		nobackpen = FALSE;
	rect.MinX = max(rect.MinX,page->pg_wTabX);  rect.MaxX = min(rect.MaxX,page->pg_wTabX+page->pg_wTabW);
	rect.MinY = max(rect.MinY,page->pg_wTabY);  rect.MaxY = min(rect.MaxY,page->pg_wTabY+page->pg_wTabH);
	if (rect.MaxX < rect.MinX || rect.MaxY < rect.MinY)
		return;

	flags = page->pg_Document->mp_Flags;
	if (flags & (MPF_PAGEONLY | MPF_PAGEMARKED))
	{
#ifdef __amigaos4__	//subtract temporary border from printable area.
		pw = (int32)(pixel(page,page->pg_Document->mp_mmWidth - page->pg_Document->mp_BorderLeft - page->pg_Document->mp_BorderRight /*+ page->pg_wTabX*/, TRUE)); 
		ph = (int32)(pixel(page,page->pg_Document->mp_mmHeight - page->pg_Document->mp_BorderTop - page->pg_Document->mp_BorderBottom,FALSE) /** 0.991*/); //-page->pg_TabY+page->pg_wTabY;
#else
		pw = pixel(page,page->pg_Document->mp_mmWidth,TRUE); //-page->pg_TabX+page->pg_wTabX;   /* Vielleicht FP-Berechnung in Page auslagern */
		ph = pixel(page,page->pg_Document->mp_mmHeight,FALSE); //-page->pg_TabY+page->pg_wTabY;
#endif
	}
	rp = win->RPort;

	if (doublerp)
	{
		irp->BitMap = doublerp->BitMap;
		irp->Layer = doublerp->Layer;
	}
	else
	{
		irp->BitMap = rp->BitMap;
		irp->Layer = rp->Layer;
	}
	/**************************** Erstes Feld im Bereich in X-Richtung ****************************/

	border = page->pg_TabX+rect.MinX-page->pg_wTabX;
	for(x = 0,i = 0,tabLeft = 1;x <= border;x += i)
	{
		i = GetTFWidth(page,tabLeft);
		if (x+i<=border)
			tabLeft++;
	}
	tabXOff = (x-i)-border;
	if (!(i+tabXOff))
		tabXOff = 0;

	/**************************** Erstes Feld im Bereich in Y-Richtung ****************************/

	border = page->pg_TabY+rect.MinY-page->pg_wTabY;
	for(y = 0,i = 0,tabTop = 1;y <= border;y += i)
	{
		i = GetTFHeight(page,tabTop);
		if (y+i <= border)
			tabTop++;
	}
	tabYOff = (y-i)-border;
	if (!(i+tabYOff))
		tabYOff = 0;

	/**************************** Titel und Linien zeichnen ****************************/

	if (!pd->pd_ShowAntis)
	{
		vertgads = FALSE;
		horizgads = FALSE;
	}
	{
		ULONG rasterColor = page->pg_BPen;

		// brighten raster if the BPen is near black
		if (rasterColor < 0x00090909)
		{
			if (rasterColor < 0x00030303)
				rasterColor = 0x444444;
			else
				rasterColor = TintColor(rasterColor,3.0f);
		}
		else
			rasterColor = TintColor(rasterColor,0.84f);

		SetColors(irp,rasterColor,page->pg_BPen);
	}
	SetDrMd(irp,JAM2);

	if (!nobackpen)
	{
		ULONG oldpen = GetAPen(irp);
		ULONG bpen = GetBPen(irp);

		SetAPen(irp,bpen);
		RectFill(irp,rect.MinX,rect.MinY,rect.MaxX,rect.MaxY);
		SetAPen(irp,oldpen);
	}

	itext.ITextFont = scr->Font;
	irp->linpatcnt = 15;  irp->Flags |= FRST_DOT;

	if (pd->pd_Rasta & PDR_POINTS)
	{
		if ((rect.MinY+page->pg_TabY) % 2)
			irp->LinePtrn = 0x5555;
		else
			irp->LinePtrn = 0xaaaa;
	}
	else
		irp->LinePtrn = 0xffff;

	itext.FrontPen = 1;
	itext.ITextFont = &pd->pd_AntiAttr;
	//itext.IText = txt;

	y = page->pg_wTabY-pd->pd_AntiHeight;
	makeClip(win,rect.MinX,y,rect.MaxX-rect.MinX,rect.MaxY-y);
	for(x = rect.MinX+tabXOff,i = tabLeft;x <= rect.MaxX;x += w,i++)
	{
		w = GetTFWidth(page,i);
		if ((pd->pd_Rasta & PDR_CYCLES) && !(pd->pd_Rasta & PDR_CELLWIDTH) && x+w-1 <= rect.MaxX) // && !((flags & MPF_PAGEONLY) && rect.MaxY < rect.MinY))
		{
			Move(irp,x+w-1,rect.MinY);
			Draw(irp,x+w-1,rect.MaxY);
		}
		if (horizgads)
		{
			if (!(itext.IText = GetHorizTitle(page,i)))
			{
				Pos2String(i,txt);
				itext.IText = txt;
			}
			EraseRect(rp,x+2,y+1,x-3+w,y+pd->pd_AntiHeight-2);
			DrawBevelBox(rp,x,y,w,pd->pd_AntiHeight,GT_VisualInfo,vi,/*GTBB_FrameType,BBFT_BUTTON,*/TAG_END);
			if ((border = IntuiTextLength(&itext)) < w-3)
				PrintIText(rp,&itext,x+(w >> 1)-(border >> 1),y+2);
		}
	}
	freeClip(win);

	if (pd->pd_Rasta & PDR_POINTS)
	{
		if ((rect.MinX+page->pg_TabX) % 2)
			irp->LinePtrn = 0x5555;
		else
			irp->LinePtrn = 0xaaaa;
	}
	else
		irp->LinePtrn = 0xffff;

	x = page->pg_wTabX-pd->pd_AntiWidth;
	makeClip(win,x,rect.MinY,rect.MaxX-x,rect.MaxY-rect.MinY);
	for(y = rect.MinY+tabYOff,i = tabTop;y <= rect.MaxY;y += h,i++)
	{
		h = GetTFHeight(page,i);
		if ((pd->pd_Rasta & PDR_CYCLES) && !(pd->pd_Rasta & PDR_CELLWIDTH))
		{
			if (y+h-1 <= rect.MaxY) // && !((flags & MPF_PAGEONLY) && rect.MaxX < rect.MinX))
			{
				Move(irp,rect.MinX,y+h-1);
				Draw(irp,rect.MaxX,y+h-1);
			}
		}
		if (vertgads)
		{
			if (!(itext.IText = GetVertTitle(page,i)))
			{
				sprintf(txt,"%ld",i);
				itext.IText = txt;
			}
			EraseRect(rp,x+2,y+1,x-3+pd->pd_AntiWidth,y-2+h);
			DrawBevelBox(rp,x,y,pd->pd_AntiWidth,h,GT_VisualInfo,vi,GTBB_FrameType,BBFT_BUTTON,TAG_END);
			if (h > pd->pd_AntiHeight-3)
				PrintIText(rp,&itext,x+(pd->pd_AntiWidth >> 1)-(IntuiTextLength(&itext) >> 1),y+((h-pd->pd_AntiHeight+4) >> 1));
		}
	}
	itext.ITextFont = scr->Font;
	freeClip(win);

	if (doublerp)
		rp = doublerp;

	/**************************** Zellen zeichnen ****************************/

	for(y = rect.MinY+tabYOff;y <= rect.MaxY;y += h,tabTop++)
	{
		long nextleft;

		h = GetTFHeight(page,tabTop);  tf = NULL;
		makeLayerClip(rp->Layer,rect.MinX,rect.MinY,rect.MaxX-rect.MinX,rect.MaxY-rect.MinY);

		for(x = rect.MinX+tabXOff,i = nextleft = tabLeft;/*(i <= page->pg_Cols) &&*/ (x <= rect.MaxX);x += w,i++)
		{
			w = GetTFWidth(page,i);
			if (!tf)
				tf = GetTableField(page,i,tabTop);
			else while(tf && (tf->tf_Col < i && tf->tf_Row == tabTop || tf->tf_Row < tabTop))
				tf = NextTableField(tf);

			if (tf && tf->tf_Col == i && tf->tf_Row == tabTop)
			{
				nextleft += tf->tf_Width+2;

				if (tf->tf_BPen != page->pg_BPen || tf->tf_Pattern && tf->tf_PatternColor != tf->tf_BPen)
				{
					if (tf->tf_Pattern)
					{
						ULONG bpen = GetBPen(rp);

						SetColors(rp,tf->tf_PatternColor,tf->tf_BPen);
						DrawPattern(rp,tf->tf_Pattern,x,y,x+w-1,y+h-1,page->pg_TabX % 16,page->pg_TabY % 4);
						SetHighColor(rp,tf->tf_BPen);
						SetBPen(rp,bpen);
					}
					else
					{
						SetHighColor(rp,tf->tf_BPen);
						RectFill(rp,x,y,x+w-1,y+h-1);
					}
				}
				else if (nobackpen)
				{
					SetHighColor(rp,tf->tf_BPen);
					if ((pd->pd_Rasta & PDR_CYCLES) && !(pd->pd_Rasta & PDR_CELLWIDTH))
						RectFill(rp,x,y,x+w-2,y+h-2);
					else
						RectFill(rp,x,y,x+w-1,y+h-1);
				}

				if (tf->tf_Border[0])
				{
					SetHighColor(rp,tf->tf_BorderColor[0]);
					/*border = ((tf->tf_Border[0]*(page->pg_DPI >> 16)/72)+1)/32;
					RectFill(rp,x,y,x+border,y+h-1);*/
					DrawVertBlock(rp,page->pg_DPI,x,y,y+h-1,tf->tf_Border[0]*8,0);
				}
				if (tf->tf_Border[1])
				{
					SetHighColor(rp,tf->tf_BorderColor[1]);
					DrawVertBlock(rp,page->pg_DPI,x+w-1,y+h-1,y,tf->tf_Border[1]*8,0);
				}
				if (tf->tf_Border[2])
				{
					SetHighColor(rp,tf->tf_BorderColor[2]);
					DrawHorizBlock(rp,page->pg_DPI,x+w-1,y+h-1,x,tf->tf_Border[2]*8,0);
				}
				if (tf->tf_Border[3])
				{
					SetHighColor(rp,tf->tf_BorderColor[3]);
					DrawHorizBlock(rp,page->pg_DPI,x,y,x+w-1,tf->tf_Border[3]*8,0);
				}
				if (pd->pd_Rasta & PDR_CELLWIDTH)
					DrawRastaCellWidth(irp,x,y,!tf->tf_Width ? w : GetTotalWidth(page,tf),h);

				tf = NextTableField(tf);
			}
			else
			{
				if (nextleft < i)
					nextleft++;

				if (nobackpen)
				{
					SetHighColor(rp,page->pg_BPen);
					if ((pd->pd_Rasta & PDR_CYCLES) && !(pd->pd_Rasta & PDR_CELLWIDTH))
						RectFill(rp,x,y,x+w-2,y+h-2);
					else
						RectFill(rp,x,y,x+w-1,y+h-1);
				}
				if ((pd->pd_Rasta & PDR_CELLWIDTH) && nextleft <= i)
					DrawRastaCellWidth(irp,x,y,w,h);
			}
		}
		freeLayerClip(rp->Layer);

		tf = GetRealTableField(page,tabLeft,tabTop);
		for(x = rect.MinX+tabXOff,i = tabLeft;(i <= page->pg_Cols) && (x <= rect.MaxX);x += GetTFWidth(page,i++))
		{
			if (!tf)
				tf = GetTableField(page,i,tabTop);
			else while(tf && (tf->tf_Col+tf->tf_Width < i && tf->tf_Row == tabTop || tf->tf_Row < tabTop))
				tf = NextTableField(tf);

			for(;tf && tf->tf_Col <= i && tf->tf_Col+tf->tf_Width >= i && tf->tf_Row == tabTop;tf = NextTableField(tf))
			{
				if (tf->tf_Col == i)
					DrawTFText(rp,page,&rect,tf,x,y);
				else
				{
					for(w = x,border = i;border > tf->tf_Col;w -= GetTFWidth(page,--border));
					DrawTFText(rp,page,&rect,tf,w,y);
				}
			}
		}
	}

	if (flags & MPF_PAGEMARKED)
	{
		long p = pw-page->pg_TabX+page->pg_wTabX;
		SetABPenDrMd(rp,0,0,JAM1);
		SetPattern(rp,3,page->pg_TabX % 16,page->pg_TabY % 4);

		for(;p-2 < rect.MaxX;p += pw)
		{
			if (p+3 > rect.MinX)
			{
				long xmin = max(p-2,rect.MinX);
				long xmax = min(p+3,rect.MaxX);

				RectFill32(rp,xmin,rect.MinY,xmax,rect.MaxY);
				SetAPen(rp,1);
				Move(rp,p,rect.MinY);  Draw(rp,p,rect.MaxY);
				SetAPen(rp,0);
			}
		}
		p = ph-page->pg_TabY+page->pg_wTabY;

		for(;p-2 < rect.MaxY;p += ph)
		{
			if (p+2 > rect.MinY)
			{
				long ymin = max(p-2,rect.MinY);
				long ymax = min(p+2,rect.MaxY);

				RectFill32(rp,rect.MinX,ymin,rect.MaxX,ymax);
				SetAPen(rp,1);
				Move(rp,rect.MinX,p);  Draw(rp,rect.MaxX,p);
				SetAPen(rp,0);
			}
		}
		SetAfPt(rp,NULL,0);
	}
	makeLayerClip(rp->Layer,rect.MinX,rect.MinY,rect.MaxX-rect.MinX,rect.MaxY-rect.MinY);

	/**************************** Eingaberahmen & Cursorposition zeichnen ****************************/

	if (page->pg_Gad.DispPos != PGS_NONE)
	{
		SetAPen(rp,1);
		DrawRect(rp,x = page->pg_Gad.cp.cp_X,y = page->pg_Gad.cp.cp_Y,page->pg_Gad.cp.cp_W-2,page->pg_Gad.cp.cp_H-2);
		DrawRect(rp,x-1,y-1,page->pg_Gad.cp.cp_W,page->pg_Gad.cp.cp_H);

		if (prefs.pr_Table->pt_Flags & PTF_EDITFUNC && page->pg_Gad.cp.cp_W > 3 && page->pg_Gad.cp.cp_H > 3 && !(page->pg_Document->mp_Flags & MPF_SCRIPTS))
		{
			x += page->pg_Gad.cp.cp_W - 3;
			y += page->pg_Gad.cp.cp_H - 3;
			RectFill(rp, x, y, x + 2, y + 2);
			SetAPen(rp,0);
			Move(rp,x-1,y+1);  Draw(rp,x-1,y+2);
			Move(rp,x+1,y-1);  Draw(rp,x+2,y-1);
		}
		if (page->pg_Gad.DispPos > PGS_FRAME && page->pg_Gad.tf)
		{
			x = page->pg_Gad.cp.cp_X+page->pg_CellTextSpace+OutlineLength(page->pg_Gad.tf->tf_FontInfo,page->pg_Gad.tf->tf_Text+page->pg_Gad.FirstChar,page->pg_Gad.DispPos-page->pg_Gad.FirstChar);
			if (page->pg_Gad.tf->tf_BPen == page->pg_APen)
				SetHighColor(rp,page->pg_Gad.tf->tf_APen);
			else
				SetHighColor(rp,page->pg_APen);
			RectFill(rp,x,page->pg_Gad.cp.cp_Y+1,x+1,page->pg_Gad.cp.cp_Y+page->pg_Gad.cp.cp_H-3);
		}
	}

	/**************************** Objekte zeichnen ****************************/

	SetGRastPort(rp);
	for(i = 0;i < page->pg_NumObjects;i++)
	{
		if (!(go = page->pg_ObjectOrder[i]))
			continue;

		if ((x = go->go_Left-page->pg_TabX+page->pg_wTabX) <= rect.MaxX && x+go->go_Right-go->go_Left >= rect.MinX && (y = go->go_Top-page->pg_TabY+page->pg_wTabY) <= rect.MaxY && y+go->go_Bottom-go->go_Top >= rect.MinY)
			DrawGObject(grp, page, go, x, y);
	}
	freeLayerClip(rp->Layer);

	/**************************** Seitenbegrenzung zeichnen ****************************/

	if (flags & MPF_PAGEONLY)
	{
		long xmin = max(pw-page->pg_TabX+page->pg_wTabX,rect.MinX);
		long ymin = max(ph-page->pg_TabY+page->pg_wTabY,rect.MinY);

		/*if (CyberGfxBase && GetCyberMapAttr(irp->BitMap,CYBRMATTR_PIXFMT) >= PIXFMT_ARGB32)
		{
			if (xmin < rect.MaxX)
				FillPixelArray(irp,xmin,rect.MinY,rect.MaxX+1-xmin,rect.MaxY+1-rect.MinY,0x80ffffff);
			if (ymin < rect.MaxY)
			{
				if (xmin < rect.MaxX)
					FillPixelArray(irp,rect.MinX,ymin,xmin+1-rect.MinX,rect.MaxY+1-ymin,0x80ffffff);
				else
					FillPixelArray(irp,rect.MinX,ymin,rect.MaxX+1-rect.MinX,rect.MaxY+1-ymin,0x80ffffff);
			}
		}
		else*/
		{
			SetABPenDrMd(irp,0,0,JAM1);
			SetPattern(irp,3,page->pg_TabX % 16,page->pg_TabY % 4);

			if (xmin < rect.MaxX)
				RectFill32(irp,xmin,rect.MinY,rect.MaxX,rect.MaxY);
			if (ymin < rect.MaxY)
			{
				if (xmin < rect.MaxX)
					RectFill32(irp,rect.MinX,ymin,xmin,rect.MaxY);
				else
					RectFill32(irp,rect.MinX,ymin,rect.MaxX,rect.MaxY);
			}
			SetAfPt(irp,NULL,0);
		}
	}

	/**************************** Zellen-Markierung darstellen ****************************/

	if (page->pg_MarkCol != -1 && page->pg_MarkX1 != page->pg_MarkX2)
	{
		long xmin = max(page->pg_MarkX1,rect.MinX);
		long xmax = min(page->pg_MarkX2,rect.MaxX);
		long ymin = max(page->pg_MarkY1,rect.MinY);
		long ymax = min(page->pg_MarkY2,rect.MaxY);

		SetDrMd(irp,COMPLEMENT);
		RectFill32(irp,xmin,ymin,xmax,ymax);
		SetDrMd(irp,JAM2);
	}

	x = page->pg_wTabX-page->pg_TabX;
	y = page->pg_wTabY-page->pg_TabY;

	/**************************** Objektanker/-rahmen zeichnen ****************************/

	foreach(&page->pg_gObjects,go)
	{
		if ((go->go_Flags & GOF_SELECTED) && go->go_Left+x < rect.MaxX && go->go_Right+x > rect.MinX && go->go_Top+y < rect.MaxY && go->go_Bottom+y > rect.MinY)
			DrawGObjectKnobs(page,go);
	}

	/**************************** DoubleBuffer kopieren ****************************/

	if (doublerp)
		ClipBlit(doublerp,rect.MinX,rect.MinY,win->RPort,rect.MinX,rect.MinY,rect.MaxX+1-rect.MinX,rect.MaxY+1-rect.MinY,0xc0);
}


void
DrawTableTitles(struct Page *page, UBYTE horiz)
{
	struct Rectangle rect;

	if (!page)
		return;

	if (horiz)
	{
		rect.MinX = 0;
		rect.MaxX = page->pg_wTabW+page->pg_wTabX;
		rect.MinY = 0;
		rect.MaxY = page->pg_wTabY+1;
	}
	else
	{
		rect.MinY = 0;
		rect.MaxY = page->pg_wTabH+page->pg_wTabY;
		rect.MinX = 0;
		rect.MaxX = page->pg_wTabX+1;
	}
	DrawTableRegion(page->pg_Window,page,&rect,!horiz,horiz);
}


void
DrawTableCoord(struct Page *page, long x1, long y1, long x2, long y2)
{
	struct Rectangle rect;
	struct Window *win;

	if (page && (win = page->pg_Window))
	{
		SetBusy(TRUE,BT_PROJECT);
		rect.MinX = x1;  rect.MaxX = x2;
		rect.MinY = y1;  rect.MaxY = y2;
		DrawTableRegion(win,page,&rect,FALSE,FALSE);
		SetBusy(FALSE,BT_PROJECT);
	}
}


void
DrawTablePos(struct Page *page, long col, long row, long width, long height)
{
	struct Rect32 rect;

	if (!page)
		return;

	setTableCoord(page,&rect,col,row,width,height);
	DrawTableCoord(page,rect.MinX,rect.MinY,rect.MaxX,rect.MaxY);
}


void
DrawTableField(struct Page *page, struct tableField *tf)
{
	struct coordPkt cp;
	long   i;

	if (!page || !page->pg_Window || !tf)
		return;

	SetCellCoordPkt(page,&cp,tf,tf->tf_Col,tf->tf_Row);
	cp.cp_X += page->pg_wTabX;  cp.cp_Y += page->pg_wTabY;
	if (tf->tf_Width < tf->tf_OldWidth)
	{
		if ((i = tf->tf_OldWidth) != 0)
		{
			for(cp.cp_W = 0;i+1;i--)
				cp.cp_W += GetTFWidth(page,tf->tf_Col+i);
		}
	}
	if (page->pg_Gad.tf == tf)
	{
		cp.cp_X -= 2;  cp.cp_Y -= 2;
		cp.cp_W += 4;  cp.cp_H += 4;
	}
	DrawTableCoord(page, cp.cp_X, cp.cp_Y, cp.cp_X + cp.cp_W, cp.cp_Y + cp.cp_H);
	tf->tf_OldWidth = tf->tf_Width;
}


void
DrawMarkedCells(struct Page *page, int32 maxColumn)
{
	if (!page)
		return;

	if (page->pg_MarkCol != -1 || maxColumn != -1) {
		if (page->pg_MarkCol != -1) {
			// block with or without maxColumn set
			DrawTablePos(page, page->pg_MarkCol, page->pg_MarkRow,
				maxColumn == -1 ? page->pg_MarkWidth : maxColumn - page->pg_MarkCol, page->pg_MarkHeight);
		} else {
			// single cell with maxColumn set
			DrawTablePos(page, page->pg_Gad.cp.cp_Col, page->pg_Gad.cp.cp_Row, maxColumn - page->pg_Gad.cp.cp_Col, 0);
		}
	} else if (page->pg_Gad.tf)
		DrawTableField(page, page->pg_Gad.tf);
	else
		DrawTablePos(page, page->pg_Gad.cp.cp_Col, page->pg_Gad.cp.cp_Row, 0, 0);
}


void
DrawTable(struct Window *win)
{
	struct Rectangle rect;

	if (!win)
		return;

	SetBusy(TRUE,BT_PROJECT);
	rect.MinX = -1;  rect.MaxX = 32767;
	rect.MinY = -1;  rect.MaxY = 32767;
	DrawTableRegion(win,win->UserData ? ((struct winData *)win->UserData)->wd_Data : NULL,&rect,TRUE,TRUE);
	SetBusy(FALSE,BT_PROJECT);
}


void
RefreshMarkedTable(struct Page *page, long maxcol, BOOL end)
{
	struct tableField *tf;
	struct Rect32 rect;

	if ((tf = page->pg_Gad.tf) != 0)
	{
		ULONG col = tf->tf_Col+tf->tf_Width;

		if (end)
			SetTabGadget(page,(STRPTR)~0,PGS_FRAME);
		else if (col >= maxcol)
			DrawTableField(page,tf);

		if (page->pg_MarkCol == -1 && col < maxcol)
		{
			setTableCoord(page,&rect,col,tf->tf_Row,maxcol-col,0);
			DrawTableCoord(page,rect.MinX,rect.MinY-1,rect.MaxX,rect.MaxY);
		}
	}
	else if (page->pg_Gad.DispPos != PGS_NONE)
		page->pg_Gad.tf = GetTableField(page,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);

	if (maxcol && page->pg_MarkCol != -1)
	{
		setTableCoord(page, &rect, page->pg_MarkCol, page->pg_MarkRow, maxcol - page->pg_MarkCol, page->pg_MarkHeight);
		DrawTableCoord(page, rect.MinX, rect.MinY - 1, rect.MaxX, rect.MaxY);
	}

	RecalcTableFields(page);
	RefreshToolBar(page);
}


void
ScrollTable(struct Window *win, long xoff, long yoff)
{
	struct Rectangle rect;
	struct PrefDisp *pd;
	struct Page *page;

#ifdef __amigaos4__
	struct gObject *go;
#endif
	/* Scrollt BitMap des Fensters */

	wd = (struct winData *)win->UserData;  page = wd->wd_Data;
	pd = page->pg_Document->mp_Prefs.pr_Disp;

	xoff = page->pg_TabX-xoff;  yoff = page->pg_TabY-yoff;
	if (!xoff && !yoff)
		return;

	if (pd->pd_ShowAntis && !(xoff && yoff)) // || xoff && yoff)
	{
		long anti;

		if (xoff)
		{
			anti = pd->pd_AntiHeight;
			makeClip(win,wd->wd_TabX,wd->wd_TabY-anti,wd->wd_TabW,wd->wd_TabH+anti);
			ClipBlit(win->RPort,wd->wd_TabX,wd->wd_TabY-anti,win->RPort,wd->wd_TabX+xoff,wd->wd_TabY-anti+yoff,wd->wd_TabW+1,wd->wd_TabH+1+anti,0xc0);
		}
		else
		{
			anti = pd->pd_AntiWidth;
			makeClip(win,wd->wd_TabX-anti,wd->wd_TabY,wd->wd_TabW+anti,wd->wd_TabH);
			ClipBlit(win->RPort,wd->wd_TabX-anti,wd->wd_TabY,win->RPort,wd->wd_TabX-anti+xoff,wd->wd_TabY+yoff,wd->wd_TabW+1+anti,wd->wd_TabH+1,0xc0);
		}
	}
	else
	{
		makeClip(win,wd->wd_TabX,wd->wd_TabY,wd->wd_TabW,wd->wd_TabH);
		ClipBlit(win->RPort,wd->wd_TabX,wd->wd_TabY,win->RPort,wd->wd_TabX+xoff,wd->wd_TabY+yoff,wd->wd_TabW+1,wd->wd_TabH+1,0xc0);
	}
	freeClip(win);
	page->pg_TabX -= xoff;  page->pg_TabY -= yoff;
	page->pg_Gad.cp.cp_X += xoff;
	page->pg_Gad.cp.cp_Y += yoff;
	page->pg_MarkX1 += xoff;  page->pg_MarkY1 += yoff;
	page->pg_MarkX2 += xoff;  page->pg_MarkY2 += yoff;
	page->pg_Select.MinX += xoff;  page->pg_Select.MinY += yoff;
	page->pg_Select.MaxX += xoff;  page->pg_Select.MaxY += yoff;

	if (xoff)
	{
		rect.MinY = wd->wd_TabY;
		rect.MaxY = wd->wd_TabY+wd->wd_TabH;
		if (xoff < 0)							   /* nach links */
		{
			rect.MinX = max(wd->wd_TabX+wd->wd_TabW+xoff,wd->wd_TabX);
			rect.MaxX = wd->wd_TabX+wd->wd_TabW;
		}
		else										/* nach rechts */
		{
			rect.MinX = wd->wd_TabX;
			rect.MaxX = min(wd->wd_TabX+xoff,wd->wd_TabX+wd->wd_TabW);
		}
		DrawTableRegion(win,page,&rect,(BOOL)yoff,(BOOL)xoff);
#ifdef __amigaos4__
		foreach (&page->pg_gDiagrams, go)
			RefreshDiagram((struct gDiagram *)go);
#endif
	}
	if (yoff)
	{
		rect.MinX = wd->wd_TabX;
		rect.MaxX = wd->wd_TabX+wd->wd_TabW;
		if (yoff < 0)							   /* nach oben */
		{
			rect.MinY = max(wd->wd_TabY+wd->wd_TabH+yoff,wd->wd_TabY);
			rect.MaxY = wd->wd_TabY+wd->wd_TabH;
		}
		else										/* nach unten */
		{
			rect.MinY = wd->wd_TabY;
			rect.MaxY = min(wd->wd_TabY+yoff,wd->wd_TabY+wd->wd_TabH);
		}
		DrawTableRegion(win,page,&rect,(BOOL)yoff,(BOOL)xoff);
#ifdef __amigaos4__
		foreach (&page->pg_gDiagrams, go)
			RefreshDiagram((struct gDiagram *)go);
#endif
	}

}


void
DrawTableFrame(struct Window *win, struct winData *wd)
{
	struct Page *page = wd->wd_Data;
	struct PrefDisp *pd;
	long x,y;

	if ((pd = page->pg_Document->mp_Prefs.pr_Disp)->pd_ShowAntis)
	{
		x = wd->wd_TabX - pd->pd_AntiWidth;
		y = wd->wd_TabY - pd->pd_AntiHeight;
		DrawBevelBox(win->RPort,x,y,pd->pd_AntiWidth,pd->pd_AntiHeight,GT_VisualInfo,vi,GTBB_FrameType,BBFT_BUTTON,TAG_END);
	}
	else if (pd->pd_IconBar != PDIB_NONE || pd->pd_FormBar != PDFB_NONE || pd->pd_HelpBar || pd->pd_ToolBar)
	{
		x = wd->wd_TabX;
		y = wd->wd_TabY;
	}
	else
		return;

	SetAPen(win->RPort,1);
	win->RPort->AreaPtrn = (unsigned short *)&dithPtrn;
	win->RPort->AreaPtSz = 1;
	RectFill(win->RPort,x-4,y-3,win->Width-1-win->BorderRight,y-1);
	RectFill(win->RPort,x-4,wd->wd_TabY+wd->wd_TabH+1,win->Width-1-win->BorderRight,wd->wd_TabY+wd->wd_TabH+3);
	RectFill(win->RPort,x-4,y,x-1,wd->wd_TabY+wd->wd_TabH);
	RectFill(win->RPort,win->Width-5-win->BorderRight,y,win->Width-1-win->BorderRight,wd->wd_TabY+wd->wd_TabH);
	win->RPort->AreaPtrn = NULL;
	win->RPort->AreaPtSz = 0;
}


void
setTableCoord(struct Page *page, struct Rect32 *target, long col, long row, long width, long height)
{
	struct coordPkt cp;
	struct Rect32 rect;

	TRACE(("setTableCoord(col = %ld, row = %ld, width = %ld, height = %ld)\n", col, row, width, height));
		
	if (!page)
		return;

	setCoordPkt(page, &cp, col, row);
	rect.MinX = cp.cp_X + page->pg_wTabX;
	rect.MinY = cp.cp_Y + page->pg_wTabY;

	if (width == -1)
		rect.MaxX = page->pg_wTabX + page->pg_wTabW;
	else
		for (rect.MaxX = rect.MinX-1;width >= 0;rect.MaxX += GetTFWidth(page, col + width--));

	if (height == -1)
		rect.MaxY = page->pg_wTabY+page->pg_wTabH;
	else
		for (rect.MaxY = rect.MinY-1;height >= 0;rect.MaxY += GetTFHeight(page, row + height--));

	CopyMem(&rect, target, sizeof(struct Rect32));
}
						  

void
SetCellCoordPkt(struct Page *page, struct coordPkt *cp, struct tableField *tf, long col, long row)
{
	long   pos, i;

	TRACE(("SetCellCoordPkt(cell = 0x%08lx, col = %ld, row = %ld)\n", tf, col, row));
	
	if (tf)
		cp->cp_W = GetTotalWidth(page, tf);
	else
		cp->cp_W = GetTFWidth(page, col);

	for (i = -page->pg_TabX, pos = 1; pos < col; pos++)
		i += GetTFWidth(page, pos);

	cp->cp_Col = col;
	cp->cp_X = i;

	for (i = -page->pg_TabY, pos = 1; pos < row; pos++)
		i += GetTFHeight(page, pos);

	cp->cp_Row = row;
	cp->cp_Y = i;
	cp->cp_H = GetTFHeight(page,row); 
}


void
setCoordPkt(struct Page *page, struct coordPkt *cp, long col, long row)
{
	SetCellCoordPkt(page, cp, GetTableField(page, col, row), col, row);
}


struct coordPkt
getCoordPkt(struct Page *page, long mouseX, long mouseY)
{
	struct coordPkt cp;
	struct tableField *tf;
	long pos, j;

	/* Set cp_X and cp_Col */

	for (j = -page->pg_TabX, pos = 1; j <= mouseX; pos++) {
		cp.cp_X = j;
		j += cp.cp_W = GetTFWidth(page, pos);
	}
	cp.cp_Col = pos - 1;

	/* Set cp_Y and cp_Row */

	for (j = -page->pg_TabY, pos = 1; j <= mouseY; pos++) {
		cp.cp_Y = j;
		j += cp.cp_H = GetTFHeight(page, pos);
	}
	cp.cp_Row = pos - 1;
	if (!cp.cp_Col) {
		cp.cp_Col = 1;
		cp.cp_X = -page->pg_TabX;
		cp.cp_W = GetTFWidth(page, 1);
	}
	if (!cp.cp_Row) {
		cp.cp_Row = 1;
		cp.cp_Y = -page->pg_TabY;
		cp.cp_H = GetTFHeight(page, 1);
	}
	if ((tf = GetTableField(page, cp.cp_Col, cp.cp_Row)) != 0)
		cp.cp_W = GetTotalWidth(page, tf);
//printf("getCoordPkt: row=%d col=%d width=%d height=%d\n",cp.cp_Y,cp.cp_X,cp.cp_W,cp.cp_H);
	return cp;
}


void
FreeTable(struct Page *page)
{
	struct tableField *tf, *nextCell;

	for (tf = (struct tableField *)page->pg_Table.mlh_Head; tf->tf_Node.mln_Succ; tf = nextCell) {
		nextCell = (struct tableField *)tf->tf_Node.mln_Succ;

		RemoveCell(page, tf, false);
			// ToDo: recalc external references!
		FreeTableField(tf);
	}

	if (page->pg_tfWidth)
		FreePooled(pool, page->pg_tfWidth, sizeof(struct tableSize) * page->pg_Cols);
	if (page->pg_tfHeight)
		FreePooled(pool, page->pg_tfHeight, sizeof(struct tableSize) * page->pg_Rows);
}


void
ShowTable(struct Page *page,struct coordPkt *cp,long col,long row)
{
	struct coordPkt scp;
	long   xoff,yoff;

	if (!page || !page->pg_Window)
		return;
	if (!cp) {
		cp = &scp;
		setCoordPkt(page,cp,col,row);
	}
	xoff = page->pg_TabX;  yoff = page->pg_TabY;

	if (cp->cp_X > page->pg_wTabW || cp->cp_X+(cp->cp_W >> 1) > page->pg_wTabW && cp->cp_W < page->pg_wTabW)
		xoff += cp->cp_X-page->pg_wTabW+min(cp->cp_W-1,page->pg_wTabW-10);
	else if (cp->cp_X < 0)
		xoff += cp->cp_X;

	if (cp->cp_Y > page->pg_wTabH || cp->cp_Y+cp->cp_H > page->pg_wTabH && cp->cp_H < page->pg_wTabH)
		yoff += cp->cp_Y-page->pg_wTabH+min(cp->cp_H-1,page->pg_wTabH-4);
	else if (cp->cp_Y < 0)
		yoff += cp->cp_Y;

	if (cp != &scp) {
		cp->cp_X += page->pg_wTabX;
		cp->cp_Y += page->pg_wTabY;
	}
	ScrollTable(page->pg_Window,xoff,yoff);
	SetPageProps(page);
}


void PUBLIC
RecalcPageDPI(REG(a0, struct Page *page))
{
	struct Mappe *mp = page->pg_Document;
	long zoom = page->pg_Zoom;
	long xdpi, ydpi;

	page->pg_SizeFactorX = (zoom*mp->mp_MediumWidth)/(mp->mp_mmMediumWidth*1024.0);
	page->pg_SizeFactorY = (zoom*mp->mp_MediumHeight)/(mp->mp_mmMediumHeight*1024.0);
	page->pg_StdWidth = pixel(page,page->pg_mmStdWidth,TRUE);
	page->pg_StdHeight = pixel(page,page->pg_mmStdHeight,FALSE);
	xdpi = ((LONG)(mp->mp_MediumWidth*zoom*25.4)/mp->mp_mmMediumWidth);
	ydpi = ((LONG)(mp->mp_MediumHeight*zoom*25.4)/mp->mp_mmMediumHeight);
	page->pg_DPI = (xdpi << 16) | ydpi;
}


void
SetZoom(struct Page *page, ULONG zoom, BOOL force, BOOL draw)
{
#ifdef __amigaos4__
	double factor;
#else
	double factor = zoom*1.0/page->pg_Zoom;
#endif
	struct Mappe *mp = page->pg_Document;
	struct Gadget *gad;
	struct tableField *tf;
	long   i;

	if (zoom == -1) {
		long x,y;

		for (x = 0,i = 0;i < page->pg_Cols;x += (page->pg_tfWidth+i++)->ts_mm);
		for (y = 0,i = 0;i < page->pg_Rows;y += (page->pg_tfHeight+i++)->ts_mm);

		if (x && y) {
			x = (long)((1024.0*mp->mp_mmMediumWidth*(page->pg_wTabW+2))/(x*mp->mp_MediumWidth)+0.5);
			y = (long)((1024.0*mp->mp_mmMediumHeight*(page->pg_wTabH+3))/(y*mp->mp_MediumHeight)+0.5);
			zoom = min(x,y);
		} else
			zoom = 1 << 10;  // 100%
	}
	if (zoom > 10240) zoom = 10240;  // 1000%
	if (zoom < 256) zoom = 256;	  // 25%
	/* refresh zoom gadgets */
	{
		char   txt[64];

		ProcentToString(zoom,txt);
		if ((gad = GadgetAddress(page->pg_Window,GID_ZOOM)) != 0)
		{
			gad->UserData = (APTR)zoom;
			GT_SetGadgetAttrs(gad,page->pg_Window,NULL,GTST_String,txt,TAG_END);
		}
		if (rxpage == page && (tf = (APTR)GetAppWindow(WDT_ZOOM)) && (gad = GadgetAddress((struct Window *)tf,1)))
			GT_SetGadgetAttrs(gad,(struct Window *)tf,NULL,GTST_String,txt,TAG_END);
	}
	if (page->pg_Zoom == zoom && !force)
		return;

#ifdef __amigaos4__
	factor = zoom*1.0/page->pg_Zoom;
#endif
	page->pg_Zoom = zoom;
	RecalcPageDPI(page);
	RecalcTableSize(page);

	page->pg_TabX = (long)page->pg_TabX*factor;
	page->pg_TabY = (long)page->pg_TabY*factor;
 	page->pg_CellTextSpace = pixel(page,TF_BORDERSPACE,TRUE);
	UpdateGGroups(page);
	if (page->pg_Gad.DispPos != PGS_NONE) {
		SetCellCoordPkt(page,&page->pg_Gad.cp,page->pg_Gad.tf,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);
		page->pg_Gad.cp.cp_X += page->pg_wTabX;
		page->pg_Gad.cp.cp_Y += page->pg_wTabY;
	}

	for (tf = (APTR)page->pg_Table.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ)
		tf->tf_FontInfo = SetFontInfoA(tf->tf_FontInfo,page->pg_DPI,NULL);

	if (draw) {
		if (page->pg_MarkCol != -1)
		{
			setTableCoord(page,(APTR)&page->pg_MarkX1,page->pg_MarkCol,page->pg_MarkRow,page->pg_MarkWidth,page->pg_MarkHeight);
		}
		DrawTable(page->pg_Window);
		SetPageProps(page);
	} else {
		for (page->pg_TabW = 0,i = 0;i < page->pg_Cols;page->pg_TabW += GetTFWidth(page,++i));
		for (page->pg_TabH = 0,i = 0;i < page->pg_Rows;page->pg_TabH += GetTFHeight(page,++i));
	}
}


