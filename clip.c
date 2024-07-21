/* Clipboard functionality
 *
 * Copyright 1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "classes.h"


#define ID_ABSTRACT MAKE_ID(' ',' ',' ',' ')
ULONG clipunit = 0;


struct gGroup *
CopyGGroup(struct gGroup *sgg, BOOL objects)
{
	struct gGroup *gg;

	if (!sgg)
		return NULL;

	if (sgg->gg_Type == GOT_OBJECT) {
		struct gObject *go;

		if ((go = (struct gObject *)gDoMethod(GROUPOBJECT(sgg), GCM_COPY)) != 0) {
			// deselect the new object
			go->go_Flags &= ~GOF_SELECTED;
			return OBJECTGROUP(go);
		}
		
		return NULL;
	}

	if ((gg = AllocPooled(pool, sizeof(struct gGroup))) != 0) {
		CopyMem(sgg, gg, sizeof(struct gGroup));
		MyNewList(&gg->gg_Objects);
		
		// deselect the new group
		gg->gg_Flags &= ~GOF_SELECTED;

		if (objects) {
			struct gGroup *igg,*sigg;

			foreach (&sgg->gg_Objects, sigg) {
				if ((igg = CopyGGroup(sigg, TRUE)) != 0)
					MyAddTail(&gg->gg_Objects, igg);
			}
		}
	}
	return gg;
}


bool
IsCopyGGroupGranted(struct Page *page, struct gGroup *gg)
{
	if (gg->gg_Type == GOT_OBJECT) {
		struct gClass *refgc = FindGClass("embedded");
		struct gObject *go = GROUPOBJECT(gg);
		struct gEmbedded *ge;

		if (!refgc || !gIsSubclassFrom(go->go_Class,refgc))
			return true;

		ge = GINST_DATA(refgc,go);

		if (!ge->ge_References || !ge->ge_References->go_Page)
			return true;

		return (bool)(ge->ge_References->go_Page->pg_Mappe == page->pg_Mappe);
	} else {
		foreach (&gg->gg_Objects,gg) {
			if (!IsCopyGGroupGranted(page,gg))
				return false;
		}
		return true;
	}
}


void
MoveGGroup(struct Page *page, struct gGroup *gg, long x, long y)
{
	if (gg->gg_Type == GOT_OBJECT)
	{
		struct gObject *go = GROUPOBJECT(gg);
		long   i;

		for (i = 0; i < go->go_NumKnobs; i++)
		{
			go->go_Knobs[i].x += x;
			go->go_Knobs[i].y += y;
		}
		RefreshGObjectBounds(page, go);

		MakeUniqueName(&page->pg_gObjects, &go->go_Node.ln_Name);
	}
	else
	{
		struct gGroup *igg;

		foreach(&gg->gg_Objects, igg)
			MoveGGroup(page, igg, x, y);
	}
}


struct gGroup *
DuplicateGGroup(struct Page *page, struct gGroup *cgg, struct UndoNode *un)
{
	struct gGroup *gg,*sgg;

	if (!page || !cgg || !un)
		return NULL;

	if (!IsCopyGGroupGranted(page, cgg)) {
		ErrorRequest(GetString(&gLocaleInfo, MSG_COPY_OBJECT_TO_PAGE_ERR));
		return NULL;
	}

#ifdef __amigaos4__
	GROUPOBJECT(cgg)->go_Page = page;		//set go_Page to avoid GrimReaper
#endif
	if ((gg = CopyGGroup(cgg, TRUE)) != 0) {
		/* if the original is on the same page */

		foreach (&page->pg_gGroups, sgg) {
			if (sgg == cgg || (sgg->gg_mmLeft == gg->gg_mmLeft && sgg->gg_mmTop == gg->gg_mmTop && sgg->gg_mmRight == gg->gg_mmRight && sgg->gg_mmBottom == gg->gg_mmBottom)) {
				MoveGGroup(page, gg, 5*1024, 5*1024);
				break;
			}
		}
		AddUndoLink(&un->un_UndoList, gg);
		AddGGroup(page, gg, ADDREM_DRAW, 0);
	}
	return gg;
}


void
FreePasteNode(struct PasteNode *pn)
{
	APTR obj;

	if (!pn)
		return;

	FreeString(pn->pn_Node.ln_Name);
	while ((obj = MyRemHead(&pn->pn_List)) != 0) {
		if (pn->pn_Node.ln_Type == PNT_OBJECTS)
			FreeGGroup(obj);
		else
			FreeTableField(obj);
	}
	FreePooled(pool,pn,sizeof(struct PasteNode));
}


void
Objects2Clipboard(struct PasteNode *pn)
{
	struct IFFHandle *iff;
	long   error;

	if (!pn || !(iff = AllocIFF()))
		return;

	InitIFFasClip(iff);

	if ((iff->iff_Stream = (ULONG)OpenClipboard(clipunit)) != 0) {
		if (!(error = OpenIFF(iff,IFFF_WRITE))) {
			if (!(error = PushChunk(iff,ID_FTXT,ID_FORM,IFFSIZE_UNKNOWN))) {
				struct gObject *go;

				foreach (&pn->pn_List, go) {
					if (go->go_Flags & GOF_SELECTED) {
						if (!(error = PushChunk(iff,0,ID_CHRS,IFFSIZE_UNKNOWN))) {
							long len;

							len = go->go_Node.ln_Name ? strlen(go->go_Node.ln_Name) : 0;
							if (WriteChunkBytes(iff,go->go_Node.ln_Name,len) != len)
								error = IFFERR_WRITE;
							if (!error)
								error = PopChunk(iff);
						}
					}
				}
				if (!error)
					error = PopChunk(iff);
			}
			if (error)
				ErrorRequest(GetString(&gLocaleInfo, MSG_COPY_TO_CLIPBOARD_ERR), IFFErrorText(error));
			CloseIFF(iff);
		}
		/* Get current read ClipID to identify PasteNode */

		((struct ClipboardHandle *)iff->iff_Stream)->cbh_Req.io_Command = CBD_CURRENTWRITEID;
		DoIO((struct IORequest *)iff->iff_Stream);
		pn->pn_ID = ((struct ClipboardHandle *)iff->iff_Stream)->cbh_Req.io_ClipID;
		pn->pn_Unit = clipunit;

		CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
	}
	FreeIFF(iff);
}


void
Cells2Clipboard(struct PasteNode *pn)
{
	struct IFFHandle *iff;
	struct tableField *tf;
	long error,row,current;

	if (!pn || !(iff = AllocIFF()))
		return;

	InitIFFasClip(iff);

	if ((iff->iff_Stream = (ULONG)OpenClipboard(clipunit)) != 0) {
		if (!(error = OpenIFF(iff, IFFF_WRITE))) {
			if (!(error = PushChunk(iff, ID_ABSTRACT, ID_CAT, IFFSIZE_UNKNOWN))) {
				// write FTXT chunk for exchanging data with other applications
				if (!(error = PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))) {
					if (!IsListEmpty((struct List *)&pn->pn_List))
					{
						current = row = ((struct tableField *)pn->pn_List.mlh_Head)->tf_Row;

						foreach (&pn->pn_List, tf) {
							if (tf->tf_Row > current) { /* begin composite FORM when entering a new row */
								if (current > row)
									error = PopChunk(iff);

								current = tf->tf_Row;
								if (!error && (error = PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN)))
									break;
							}

							if (!(error = PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN))) {
								char *text = tf->tf_Original;
								long len;

								// cell security
								if ((tf->tf_Flags & TFF_HIDEFORMULA) != 0)
									text = "-";

								len = text ? strlen(text) : 0;

								if (WriteChunkBytes(iff, text, len) != len)
									error = IFFERR_WRITE;
								if (!error)
									error = PopChunk(iff);
							}

							if (error)
								break;
						}
						if (current > row && !error)
							error = PopChunk(iff);
					}
					if (!error)
						error = PopChunk(iff);
				}
																			
				// write chunk in ignition's data format
				if (!error && !(error = PushChunk(iff, ID_TABL, ID_FORM, IFFSIZE_UNKNOWN)))
				{
					ULONG handle;

					if ((handle = GetCellIteratorFromList(&pn->pn_List, NULL)) != 0)
					{
						error = SaveCells(iff, rxpage, handle, IO_SAVE_FULL_NAMES | IO_IGNORE_PROTECTED_CELLS);
						FreeCellIterator(handle);
					}
					if (!error)
						error = PopChunk(iff);
				}
				if (!error)
					error = PopChunk(iff);
			}
			if (error)
				ErrorRequest(GetString(&gLocaleInfo, MSG_COPY_TO_CLIPBOARD_ERR), IFFErrorText(error));
			CloseIFF(iff);
		}
		/* Get current read ClipID to identify PasteNode */

		((struct ClipboardHandle *)iff->iff_Stream)->cbh_Req.io_Command = CBD_CURRENTWRITEID;
		DoIO((struct IORequest *)iff->iff_Stream);
		pn->pn_ID = ((struct ClipboardHandle *)iff->iff_Stream)->cbh_Req.io_ClipID;
		pn->pn_Unit = clipunit;

		CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
	}
	FreeIFF(iff);
}


void
CutCopyObjects(struct Page *page,struct PasteNode *pn,UBYTE mode)
{
	struct gGroup *gg,*ngg;
	struct UndoNode *un;

	if (!pn && !(mode & CCC_DELETE))
		return;

	if (mode & CCC_CUT || mode & CCC_DELETE)
	{
		if ((un = CreateUndo(page, UNDO_PRIVATE, mode & CCC_CUT ? GetString(&gLocaleInfo, MSG_CUT_UNDO) : GetString(&gLocaleInfo, MSG_DELETE_UNDO))) != 0)
			un->un_Type = UNT_REMOVE_OBJECTS;
		else
		{
			ErrorRequest(GetString(&gLocaleInfo, MSG_CUT_DELETE_UNDO_ERR));
			return;
		}
	}

	for (gg = (APTR)page->pg_gGroups.mlh_Head; gg->gg_Node.mln_Succ; gg = ngg)
	{
		ngg = (APTR)gg->gg_Node.mln_Succ;

		if (gg->gg_Flags & GOF_SELECTED)
		{
			if (mode & CCC_DELETE || mode & CCC_CUT)
			{
				RemoveGGroup(page, gg, ADDREM_DRAW, 0);  // remove group
				//RemoveGGroupReferences(page,gg,un);   // remove references

				AddUndoLink(&un->un_UndoList, gg);	// add it to undo
				MyAddTail(&un->un_RedoList, gg);	// it will be freed upon FreeUndo()
			}
			if (mode & (CCC_COPY | CCC_CUT) && (gg = CopyGGroup(gg, TRUE)))
				MyAddTail(&pn->pn_List, gg);
		}
	}
	if (!pn)
		return;

	{
		STRPTR buffer = AllocPooled(pool, 512);

		if (buffer)
		{
			int len = 0;

			foreach(&pn->pn_List,gg)
			{
				CONST_STRPTR t;

				if (gg->gg_Type == GOT_GROUP)
					t = GetString(&gLocaleInfo, MSG_GROUP);
				else
					t = GROUPOBJECT(gg)->go_Node.ln_Name;

				if (!AddToNameList(buffer, (STRPTR)t,&len,512))
					break;
			}
			pn->pn_Node.ln_Name = AllocString(buffer);
			FreePooled(pool, buffer, 512);
		}
	}
}


void
CutCopyCells(struct Page *page, struct PasteNode *pn, uint8 mode)
{
	struct tableField *tf = NULL, *stf = NULL;
	long col, row, maxColumn = -1;
	bool firstSecured = true;
#ifdef __amigaos4__
	bool tfdeleted;			//Flag, das tf schon freigegeben wurde, verhindert so Grim bei OS4
#endif

	if (pn) {
		// set the clip name to something meaningful
		char t[32];
		STRPTR s;

		if (page->pg_MarkCol != -1)
			TablePos2String(page, (struct tablePos *)&page->pg_MarkCol, t);
		else
			strcpy(t, Coord2String(page->pg_Gad.cp.cp_Col, page->pg_Gad.cp.cp_Row));

		if ((s = pn->pn_Node.ln_Name = AllocPooled(pool, col = strlen(page->pg_Mappe->mp_Node.ln_Name) + strlen(page->pg_Node.ln_Name) + strlen(t) + 4)) != 0)
		{
			strcpy(s, t);
			strcat(s, ", ");
			strcat(s, page->pg_Mappe->mp_Node.ln_Name);
			strcat(s, "/");
			strcat(s, page->pg_Node.ln_Name);
		}
	}

	// Cut/Copy/Delete the selected cells into the clip

	SetTabGadget(page, (STRPTR)~0L, PGS_FRAME);

	if (mode & (CCC_CUT | CCC_DELETE))
		BeginUndo(page, UNDO_BLOCK, mode & CCC_CUT ? GetString(&gLocaleInfo, MSG_CUT_UNDO) : GetString(&gLocaleInfo, MSG_DELETE_UNDO));

#ifdef __amigaos4__
	while ((tf = GetMarkedFields(page, tf, TRUE)) != 0)
#else
	while ((tf = GetMarkedFields(page, tf, FALSE)) != 0)
#endif
	{
#ifdef __amigaos4__
	    tfdeleted = FALSE;
#endif
		if ((mode & (CCC_CUT | CCC_DELETE)) != 0 && (tf->tf_Flags & TFF_IMMUTABLE) != 0 && firstSecured) {
			DocumentSecurity(page, tf);
			firstSecured = false;
		}

		if (mode & CCC_TEXTONLY) {
			// only cut the text of the cell
			if (!(mode & CCC_DELETE))
				stf = CopyCell(page, tf);
			if ((mode & (CCC_DELETE | CCC_CUT)) != 0 && (tf->tf_Flags & TFF_IMMUTABLE) == 0)
				SetTFText(page, tf, NULL);
		} else if ((mode & CCC_CUT) != 0 && (tf->tf_Flags & TFF_IMMUTABLE) == 0) {
			// cut cell
			RemoveCell(page, stf = tf, true);
		} else if (mode & CCC_DELETE) {
			// delete cell
			if ((tf->tf_Flags & TFF_IMMUTABLE) == 0) {
				RemoveCell(page, tf, true);
				FreeTableField(tf);
#ifdef __amigaos4__
				tfdeleted = TRUE;
#endif
			}
		} else {
			// copy cell
			stf = CopyCell(page, tf);
		}

#ifdef __amigaos4__
		if (!tfdeleted && (mode & (CCC_CUT | CCC_DELETE)) != 0 && (tf->tf_Flags & TFF_IMMUTABLE) == 0) {
#else
		if ((mode & (CCC_CUT | CCC_DELETE)) != 0 && (tf->tf_Flags & TFF_IMMUTABLE) == 0) {
#endif
			// find the max. column which has to be redrawn
			if (stf->tf_Col + stf->tf_Width > maxColumn)
				maxColumn = stf->tf_Col + stf->tf_Width;
		}
		if (stf && pn != NULL)		// store copied or cut cell
			MyAddTail(&pn->pn_List, stf);
	}

	// get upper left corder of the selection

	if (page->pg_MarkCol == -1) {
		col = page->pg_Gad.cp.cp_Col;
		row = page->pg_Gad.cp.cp_Row;
	} else {
		col = page->pg_MarkCol;
		row = page->pg_MarkRow;
	}

	if (mode & CCC_CUT || mode & CCC_DELETE) {
		page->pg_Gad.tf = NULL;
		DrawMarkedCells(page, maxColumn);

		foreach (&page->pg_Table, tf) {
			// Does this cell touch the block from the left? If so,
			// recompute its width (since it could take more space
			// than before)
			if (tf->tf_Col < col && (page->pg_MarkCol != -1 && tf->tf_Row >= row && tf->tf_Row <= row + page->pg_MarkHeight
				|| tf->tf_Row == page->pg_Gad.cp.cp_Row))
			{
				tf->tf_OldWidth = tf->tf_Width;
				SetTFWidth(page, tf);

				if (tf->tf_OldWidth != tf->tf_Width)
					DrawTableField(page, tf);
			}
		}
		EndUndo(page);
		SetMark(page, -1, 0, 0, 0);
		//RecalcTableFields(page);
	}
}


struct PasteNode *
CutCopyClip(struct Page *page,UBYTE mode)
{
	struct PasteNode *pn = NULL;
	struct Window *cwin;

	if (!page || !(mode & (CCC_COPY | CCC_CUT | CCC_DELETE)))
		return NULL;

	if (mode & CCC_CURRENT || !(mode & CCC_HOTSPOT)) {
		mode &= ~CCC_HOTSPOT;
		if (page->pg_HotSpot == PGHS_OBJECT)
			mode |= CCC_OBJECTS;
		else
			mode |= CCC_CELLS;
	}
	if (mode & CCC_CELLS && page->pg_MarkCol == -1 && page->pg_Gad.DispPos == PGS_NONE) {
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_CELLS_SELECTED_ERR));
		return NULL;
	}
	if (mode & CCC_OBJECTS) {
		struct gGroup *gg;
		long selected = 0;

		foreach (&page->pg_gGroups, gg) {
			if (gg->gg_Flags & GOF_SELECTED)
				selected++;
		}

		if (!selected) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_NO_OBJECT_SELECTED_ERR));
			return NULL;
		}
	}
	if (mode & CCC_HOTSPOT && (mode & CCC_DELETE || (pn = AllocPooled(pool, sizeof(struct PasteNode))))) {
		// A PasteNode is created if the mode is not CCC_DELETE
		if (pn) {
			struct PasteNode *spn;

			if ((cwin = GetAppWindow(WDT_CLIP)) != 0)
				GT_SetGadgetAttrs(GadgetAddress(cwin, 1), cwin, NULL, GTLV_Labels, ~0L, TAG_END);

			foreach (&clips,spn)
				spn->pn_Node.ln_Pri = 0;

			MyAddTail(&clips,pn);
			MyNewList(&pn->pn_List);
			pn->pn_Node.ln_Type = mode & CCC_OBJECTS ? PNT_OBJECTS : PNT_CELLS;
			pn->pn_Node.ln_Pri = PNF_SELECTED;
		}
		if (mode & CCC_OBJECTS) {
			CutCopyObjects(page, pn, mode);
			Objects2Clipboard(pn);
		} else {
			CutCopyCells(page, pn, mode);
			Cells2Clipboard(pn);
		}

		if (pn && (cwin = GetAppWindow(WDT_CLIP)))
			GT_SetGadgetAttrs(GadgetAddress(cwin,1),cwin,NULL,GTLV_Labels,&clips,GTLV_Selected,CountNodes((struct MinList *)&clips),TAG_END);
	}
	return pn;
}


void
PasteObjects(struct Page *page, struct PasteNode *pn, UBYTE mode)
{
	struct UndoNode *un;
	struct gGroup *gg;

	if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_INSERT_OBJECTS_UNDO))) != 0)
	{
		un->un_Type = UNT_ADD_OBJECTS;

		foreach(&pn->pn_List,gg)
			DuplicateGGroup(page, gg, un);
	}
}


void
PasteCells(struct Page *page, struct MinList *list, uint8 mode)
{
	struct tableField *tf, *stf;
	long   col, row, maxCol = 0, maxRow = 0, baseCol = 0, baseRow = 0;
	struct UndoNode *un;

	if (!list || IsListEmpty((struct List *)list))
		return;

	SetTabGadget(page, (STRPTR)~0L, PGS_FRAME);
	if (page->pg_MarkCol == -1) {
		col = page->pg_Gad.cp.cp_Col;
		row = page->pg_Gad.cp.cp_Row;
	} else {
		col = page->pg_MarkCol;
		row = page->pg_MarkRow;
	} 

	foreach (list, tf) {
		if (tf->tf_Col > maxCol)
			maxCol = tf->tf_Col;
		if (tf->tf_Row > maxRow)
			maxRow = tf->tf_Row;
	}

	stf = (struct tableField *)list->mlh_Head;
	baseCol = stf->tf_Col;  baseRow = stf->tf_Row;
	maxCol -= baseCol;  maxRow -= baseRow;

	if ((un = CreateUndo(page, UNDO_BLOCK, GetString(&gLocaleInfo, MSG_INSERT_UNDO))) != 0) {
		un->un_TablePos.tp_Width = maxCol;
		un->un_TablePos.tp_Height = maxRow;
		un->un_Type = UNT_BLOCK_CHANGED;
		MakeUndoRedoList(page, un, &un->un_UndoList);
	}

	if (!(mode & PC_TEXTONLY)) {
		// remove old cells
		foreach (&page->pg_Table, tf) {
			if (tf->tf_Col >= col && tf->tf_Col <= col + maxCol && tf->tf_Row >= row && tf->tf_Row <= row + maxRow) {
				stf = (struct tableField *)tf->tf_Node.mln_Pred;
				RemoveCell(page, tf, true);
					// TODO: merge with insert loop so that we know if we have to recalc this cell
				FreeTableField(tf);
				tf = stf;
			}
		}
	}

	stf = (struct tableField *)list->mlh_Head;
	col -= stf->tf_Col;  row -= stf->tf_Row;

	for (;stf->tf_Node.mln_Succ; stf = (APTR)stf->tf_Node.mln_Succ) {
		// paste the cells over/into the page
		if (mode & PC_TEXTONLY) {
			// Paste only the text of the cell
			if (!stf->tf_Original)
				tf = GetTableField(page, stf->tf_Col + col, stf->tf_Row + row);
			else
				tf = AllocTableField(page, stf->tf_Col + col, stf->tf_Row + row);

			if (tf) {
				if (stf->tf_Type & TFT_FORMULA) {
					tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
					SetTFText(page, tf, StaticTreeTerm(stf->tf_Root, true));
				} else
					SetTFText(page, tf, stf->tf_Original);

				DrawTableField(page, tf);
			}
			else
				DrawTablePos(page, stf->tf_Col + col, stf->tf_Row + row, 0, 0);
		} else if ((tf = CopyCell(page, stf)) != 0) {
			bool isFormula = false;

			// Paste in the whole cell (old cells have already been removed previously)
 
			tf_col = tf->tf_Col = stf->tf_Col + col;
			tf_row = tf->tf_Row = stf->tf_Row + row;
  
			// InsertCell/UpdateCellText() expects the formula to be in tf_Text
			if (tf->tf_Original != NULL) {
				FreeString(tf->tf_Text);
				tf->tf_Text = tf->tf_Original;
				tf->tf_Original = NULL;
			}

			// The TFT_FORMULA flag has not yet been set for cells created from a stream
			if ((mode & PC_CLIPBOARD) != 0)
				isFormula = (tf->tf_Text != NULL && TextIsFormula(tf->tf_Text));
			else
				isFormula = (tf->tf_Type & TFT_FORMULA) != 0;

			if (isFormula) {
				if ((mode & PC_CLIPBOARD) != 0) {
					// we have to build a tree from the base position
					tf->tf_Root = CreateTreeFrom(page, baseCol, baseRow, stf->tf_Text + 1);
				}

				// TODO: what happens if this is independent from PC_CLIPBOARD? Or never?
				tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
				FreeString(tf->tf_Text);
				tf->tf_Text = TreeTerm(tf->tf_Root, TRUE);
			}

			AllocTableSize(page, tf->tf_Col, tf->tf_Row);

			InsertCell(page, tf, true);
				// will recompute this cell and referencing objects

			DrawTableField(page, tf);
		}
	}

	if (un)
		EndUndo(page);
}


void
SetNewTFOrigin(struct MinList *list)
{
	struct tableField *tf;
	uint32 col,row;

	tf = (APTR)list->mlh_Head;
	col = tf->tf_Col;
	row = tf->tf_Row;

	for (;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ) {
		tf->tf_Col -= col;
		tf->tf_Row -= row;
	}
}


long
PasteClipboard(struct Page *page, struct ClipboardHandle *cbh, UBYTE mode)
{
	struct IFFHandle *iff;
	struct tableField *tf;
	struct MinList list;
	long error;

	if (!cbh || !(iff = AllocIFF()))
		return false;

	MyNewList(&list);

	iff->iff_Stream = (ULONG)cbh;
	InitIFFasClip(iff);

	/******************* read ignition-format *******************/

	if (!(error = OpenIFF(iff, IFFF_READ))) {
		PropChunk(iff, ID_TABL, ID_CELL);

		StopOnExit(iff, ID_TABL, ID_FORM);
		ParseIFF(iff, IFFPARSE_SCAN);

		error = LoadCells(iff, ID_TABL, page, &list);

		CloseIFF(iff);
	}

	/******************* read text-format *******************/

	if (IsListEmpty((struct List *)&list) && !(error = OpenIFF(iff, IFFF_READ))) {
		struct ContextNode *cn;
		long col, row;
 
		col = 0;  row = -1;
		PropChunk(iff, ID_FTXT, ID_CHRS);

		while (!error || error == IFFERR_EOC)
		{
			if ((error = ParseIFF(iff, IFFPARSE_STEP)) && error != IFFERR_EOC)
				break;

			if ((cn = CurrentChunk(iff)) != 0)
			{
				if (!error)   // entering a chunk
				{
					if (cn->cn_ID == ID_FORM && cn->cn_Type == ID_FTXT)
					{
						col = 0;
						row++;
					}
				}
				else		  // leaving a chunk
				{
					struct StoredProperty *sp;

					if (cn->cn_ID == ID_CHRS && cn->cn_Type == ID_FTXT)
					{
						if ((sp = FindProp(iff, ID_FTXT,ID_CHRS)) && sp->sp_Size)
						{
							STRPTR s = (STRPTR)sp->sp_Data, t;

							do
							{
								t = strchr(s, '\n');
								if ((uint8 *)t >= (uint8 *)sp->sp_Data + sp->sp_Size)
									break;

								if ((tf = MakeTableField(page, col, row)) != 0)
								{
									int length;
									if (t != NULL)
										length = t - s;
									else
#if defined __amigaos4__ || defined __MORPHOS__
										length = (uint8 *)sp->sp_Data + sp->sp_Size	- (uint8 *)s;
#else
										length = (uint8 *)sp->sp_Data + sp->sp_Size	- s;
#endif

									tf->tf_Text = AllocStringLength(s, length);
									MyAddTail(&list, tf);
								}
								s = t + 1;
								if (t && !*s)
									t = NULL;
								if (t)
									row++;
							}
							while (t);
							col++;
							//PopChunk(iff);
						}
					}
				}
			}
		}
		CloseIFF(iff);
	}

	if (error && error != IFFERR_EOF)
		ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_FROM_CLIPBOARD_ERR), IFFErrorText(error));

	FreeIFF(iff);

	if (IsListEmpty((struct List *)&list))
		return false;

	PasteCells(page, &list, mode | PC_CLIPBOARD);

	while ((tf = (struct tableField *)MyRemHead(&list)) != 0)
		FreeTableField(tf);

	return true;
}


struct PasteNode *
PasteClip(struct Page *page, struct PasteNode *pn, UBYTE mode)
{
	struct PasteNode *spn;
	uint8  selected = FALSE;

	if (!page)
		return NULL;

	if (!pn)
	{
		struct ClipboardHandle *cbh;

		pn = (APTR)~0L;
		if ((cbh = OpenClipboard(clipunit)) != 0)
		{
			cbh->cbh_Req.io_Command = CBD_CURRENTREADID;
			if (!DoIO((struct IORequest *)cbh))
			{
				foreach(&clips,spn)
				{
					if (spn->pn_ID == cbh->cbh_Req.io_ClipID)
					{
						pn = spn;
						break;
					}
				}
				if (pn == (APTR)~0L)
				{
					if (PasteClipboard(page, cbh, mode))
						return NULL;
				}
			}
			CloseClipboard(cbh);
		}
	}
	else if (pn != (APTR)~0L)
		selected = TRUE;

	if (pn == (APTR)~0L) {
		pn = NULL;
		foreach (&clips, spn) {
			if (spn->pn_Node.ln_Pri) {
				pn = spn;
				break;
			}
		}
	}
	if (!pn)				  /* Wurde eine PasteNode zum Einfügen gefunden? */
		return NULL;

	if (pn->pn_Node.ln_Type == PNT_CELLS)
		PasteCells(page, &pn->pn_List, mode);
	else
		PasteObjects(page, pn, mode);

	if (!selected && prefs.pr_Flags & PRF_CLIPGO) {  /* Einfügen ohne selektierte PasteNode -> hochwandern? */
		if (pn != (struct PasteNode *)clips.mlh_Head) {
			struct Window *cwin;

			if ((cwin = GetAppWindow(WDT_CLIP)) != 0) {
				long i = 0;

				foreach (&clips, spn) {
					if (spn == pn)
						break;
					i++;
				}
				pn->pn_Node.ln_Pred->ln_Pri = PNF_SELECTED;
				pn->pn_Node.ln_Pri = 0;
				GT_SetGadgetAttrs(GadgetAddress(cwin, 1), cwin, NULL, GTLV_Selected, i, TAG_END);
			}
		}
	}
	return pn;
}


void
CreateClipboardGadgets(struct winData *wd, long wid, long hei)
{
	long h = (hei - barheight - fontheight - 14 - leftImg->Height) / fontheight;
								
	MakeShortCutString(wd->wd_ShortCuts, MSG_INSERT_UGAD, MSG_DELETE_UGAD, TAG_END);

	ngad.ng_TopEdge = barheight+3;
	ngad.ng_LeftEdge = lborder;
	ngad.ng_Width = wid-rborder-lborder;
	ngad.ng_Height = fontheight*h+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID = 1;						  /* 1 */
	gad = CreateGadget(LISTVIEW_KIND, gad, &ngad,
			GTLV_Labels, &clips, GTLV_Selected, CountNodes((APTR)&clips), GTLV_ShowSelected, NULL, TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width >>= 1;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_INSERT_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = wd->wd_ShortCuts[0];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = wid - rborder - ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_UGAD);
	ngad.ng_GadgetID = wd->wd_ShortCuts[1];
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
}


void ASM
handleClipIDCMP(REG(a0, struct TagItem *tag))
{
	struct PasteNode *pn;
	long   i, id;

	switch (imsg.Class) {
		case IDCMP_GADGETUP:
			id = ((struct Gadget *)imsg.IAddress)->GadgetID;
		case IDCMP_VANILLAKEY:
			if (imsg.Class == IDCMP_VANILLAKEY)
				id = imsg.Code;

			foreach (&clips, pn) {  // selected PasteNode
				if (pn->pn_Node.ln_Pri)
					break;
			}

			if (IsListEmpty((struct List *)&clips) || !pn->pn_Node.ln_Pri)
				pn = NULL;

			switch (id) {
				case 1:   /* Listview */
					if (pn)
						pn->pn_Node.ln_Pri = 0;
					for(i = 0, pn = (APTR)clips.mlh_Head; i < imsg.Code; i++, pn = (APTR)pn->pn_Node.ln_Succ);
					pn->pn_Node.ln_Pri = PNF_SELECTED;
					break;

#ifdef __amigaos4__
				case ESCAPE_KEY:
					CloseAppWindow(win, true);
					break;
#endif
				    
				default:
					if (pn == NULL)
						break;

					if (id == wd->wd_ShortCuts[0]) {
						/* insert */
						PasteClip(rxpage, pn, 0);
					} else if (id == wd->wd_ShortCuts[1]) {
						/* delete */
						GT_SetGadgetAttrs(gad = GadgetAddress(win, 1), win, NULL, GTLV_Labels, ~0L, TAG_END);
						MyRemove(pn);
						FreePasteNode(pn);
						GT_SetGadgetAttrs(gad, win, NULL, GTLV_Labels, &clips, TAG_END);

						GT_GetGadgetAttrs(gad, win, NULL, GTLV_Selected, &i, TAG_END);
						if (i != ~0L && (pn = (APTR)FindListNumber(&clips, i)))
							pn->pn_Node.ln_Pri = PNF_SELECTED;
					} else if (id == ESCAPE_KEY)
						CloseAppWindow(win, true);
					break;
			}
			break;

		case IDCMP_NEWSIZE:
			StandardNewSize(CreateClipboardGadgets);
			break;

		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, true);
			break;
	}
}
