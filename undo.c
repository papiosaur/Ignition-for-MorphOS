/* Undo/Redo System
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
*/


#include "types.h"
#include "funcs.h"


// enable debug output for Undo/Redo functions
#ifdef DEBUG
#define DEBUG_UNDOREDO
#endif
#ifdef DEBUG_UNDOREDO
#   define DE(x) x
#else
#   define DE(x) ;
#endif


void
AddUndoLink(struct MinList *list, APTR obj)
{
    struct UndoLink *ul;

	if ((ul = AllocPooled(pool, sizeof(struct UndoLink))) != 0) {
        ul->ul_Link = obj;
        MyAddTail(list, ul);
    }
}


void
ApplyObjectsMoveUndoRedo(struct Page *page, struct UndoNode *un, char type)
{
    struct UndoLink *ul;
    struct gObject *go;
    long   x, y, i;

	x = un->un_MoveDeltaX;
	y = un->un_MoveDeltaY;
    if (type == TYPE_UNDO)
        x = -x, y = -y;

	foreach (&un->un_UndoList, ul) {
		if ((go = ul->ul_Link) != 0) {
			for (i = 0; i < go->go_NumKnobs; i++) {
                go->go_Knobs[i].x += x;
                go->go_Knobs[i].y += y;
            }
            RefreshGObjectDrawing(page, go);
            if (go->go_Window)
                UpdateObjectGadgets(go->go_Window);
        }
    }
}


void
ApplyObjectSizeUndoRedo(struct Page *page, struct UndoNode *un, char type)
{
    struct gObject *go;
    struct point2d *p;

	if (!(go = un->un_Object))
        return;

    if (type == TYPE_UNDO)
    {
		p = un->un_UndoKnobs;
		un->un_FreeType = TYPE_UNDO;
    }
    else
    {
		p = un->un_RedoKnobs;
		un->un_FreeType = TYPE_REDO;
    }
    go->go_Knobs = p;

	RefreshGObjectDrawing(page, go);
    if (go->go_Window)
        UpdateObjectGadgets(go->go_Window);
}


static void
ApplyObjectKnobUndoRedo(struct Page *page, struct UndoNode *un, char type)
{
    struct gObject *go;
    long   x,y;

	if (!(go = un->un_Object))
        return;

    if (type == TYPE_UNDO)
    {
		x = un->un_UndoPoint.x;
		y = un->un_UndoPoint.y;
    }
    else
    {
		x = un->un_RedoPoint.x;
		y = un->un_RedoPoint.y;
	}
	gDoMethod(go, GCM_UPDATEPOINT, x, y, un->un_PointNumber);
    RefreshGObjectDrawing(page, go);
}


static void
ApplyObjectDepthUndoRedo(struct Page *page, struct UndoNode *un, char type)
{
    struct gObject *go;
    long   pos;

	if (!(go = un->un_Object))
        return;

    if (type == TYPE_UNDO)
		pos = un->un_UndoPosition;
    else
		pos = un->un_RedoPosition;

	RemoveGObject(page, go, 0);
    go->go_Pos = pos;
    AddGObject(page, NULL, go, ADDREM_DRAW);
}


static void
FreeGTagValue(ULONG type, struct TagItem *ti)
{
    switch (type)
    {
        case GIT_FONT:
            FreeFontInfo((struct FontInfo *)ti->ti_Data);
            break;
        case GIT_TEXT:
        case GIT_FORMULA:
        case GIT_FILENAME:
            FreeString((STRPTR)ti->ti_Data);
            break;
    }
}


static void
FreeObjectAttrsUndo(struct UndoNode *un)
{
	struct TagItem *undoTags, *redoTags;
    struct gInterface *gi;
    struct gObject *go;
    long   i;

	if (!(go = un->un_Object))
        return;

	undoTags = un->un_UndoTags;  redoTags = un->un_RedoTags;

	for (i = 0; undoTags[i].ti_Tag != TAG_END; i++)
    {
		if ((gi = GetGInterfaceTag(go->go_Class, undoTags[i].ti_Tag)) != 0)
        {
			FreeGTagValue(gi->gi_Type, undoTags + i);
			FreeGTagValue(gi->gi_Type, redoTags + i);
        }
    }
	FreeTagItems(undoTags);
	FreeTagItems(redoTags);
}


static void
ApplyDiagramTypeUndoRedo(struct Page *page, struct UndoNode *un, BYTE type)
{
	struct gDiagram *oldDiagram, *newDiagram;
	struct Window *win;

	if (type == TYPE_UNDO)
	{
		oldDiagram = un->un_RedoDiagram;
		newDiagram = un->un_UndoDiagram;
	}
	else
	{
		oldDiagram = un->un_UndoDiagram;
		newDiagram = un->un_RedoDiagram;
	}

	/* update object's window pointer */
	newDiagram->gd_Object.go_Window = win = oldDiagram->gd_Object.go_Window;
	oldDiagram->gd_Object.go_Window = NULL;

	MyRemove(oldDiagram);
	MyAddTail(&page->pg_gDiagrams, newDiagram);

	UpdateObjectReferences((struct gObject *)oldDiagram, (struct gObject *)newDiagram);
	
	if (win != NULL)
	{
		// special case when WDT_DIAGRAM is open!!!
		struct winData *wd = (struct winData *)win->UserData;

		wd->u.diagram.wd_CurrentDiagram = newDiagram;

		UpdateDiagramTypePage(win, wd, newDiagram);
		UpdateDiagramGadgets(win);
	}
}

 
static void
ApplyObjectAttrsUndoRedo(struct Page *page, struct UndoNode *un, BYTE type)
{
    struct gObject *go;
	struct TagItem *tags;

	D(bug("ApplyObjectAttrsUndoRedo()\n"));

	if (!(go = un->un_Object))
        return;

    if (type == TYPE_UNDO)
		tags = un->un_UndoTags;
    else
		tags = un->un_RedoTags;

	{
		long i = 0;
		for (; tags[i].ti_Tag != TAG_DONE && i < 10; i++)
		{
			STRPTR string = (STRPTR)tags[i].ti_Data;
			if ((uint32)string < 0x200000 || !isalpha(string[0]) || !isalpha(string[1]))
				string = NULL;
			D(bug("%2ld. tags: tag = 0x%08lx, value = 0x%08lx (%ld) (%s)\n", i, (tags + i)->ti_Tag, (tags + i)->ti_Data, (tags + i)->ti_Data, string));
		}
	}

	gSetObjectAttrsA(page, go, tags);
    UpdateObjectGadgets(go->go_Window);
}


static void
ApplyRemoveObjects(struct Page *page, struct UndoNode *un)
{
    struct gObject *go;

	if ((go = un->un_Object) != 0)
    {
		RemoveGObject(page, go, ADDREM_DRAW | ADDREM_CLOSE_WINDOW);
        MyAddTail(&un->un_RedoList, OBJECTGROUP(go));
    }
    else
    {
        struct UndoLink *ul;
        struct gGroup *gg;

		foreach (&un->un_UndoList, ul)
        {
            if ((gg = ul->ul_Link) != 0)
            {
                RemoveGGroup(page, gg, ADDREM_DRAW, 0);
                MyAddTail(&un->un_RedoList, gg);
            }
        }
    }
}


static void
ApplyAddObjects(struct Page *page, struct UndoNode *un)
{
    struct gObject *go;

	if ((go = un->un_Object) != 0)
    {
        MyRemove(OBJECTGROUP(go));
		AddGObject(page, NULL, go, ADDREM_DRAW);
    }
    else
    {
        struct UndoLink *ul;
        struct gGroup *gg;

		foreach (&un->un_UndoList, ul)
        {
            if ((gg = ul->ul_Link) != 0)
            {
                MyRemove(gg);
				AddGGroup(page, gg, ADDREM_DRAW, 0);
            }
        }
    }
}


static void
ApplyCellSizeUndo(struct Page *page, struct UndoNode *un)
{
    struct UndoCellSize *ucs;
    struct tableField *tf;
    struct tablePos tp = un->un_TablePos;

	foreach (&un->un_UndoList, ucs)
    {
        if (ucs->ucs_Flags & UCSF_STANDARD)
        {
            if (ucs->ucs_Flags & UCSF_HORIZ)
            {
                page->pg_mmStdWidth = ucs->ucs_mm;
                page->pg_StdWidth = ucs->ucs_Pixel;
            }
            else if (ucs->ucs_Flags & UCSF_VERT)
            {
                page->pg_mmStdHeight = ucs->ucs_mm;
                page->pg_StdHeight = ucs->ucs_Pixel;
            }
        }
        else
        {
            if (ucs->ucs_Flags & UCSF_HORIZ)
            {
				page->pg_tfWidth[ucs->ucs_Position].ts_mm = ucs->ucs_mm;
				page->pg_tfWidth[ucs->ucs_Position].ts_Pixel = ucs->ucs_Pixel;
            }
            else if (ucs->ucs_Flags & UCSF_VERT)
            {
				page->pg_tfHeight[ucs->ucs_Position].ts_mm = ucs->ucs_mm;
				page->pg_tfHeight[ucs->ucs_Position].ts_Pixel = ucs->ucs_Pixel;
            }
        }
    }
	if (un->un_mmWidth)  /* width changed */
    {
        if (tp.tp_Width == -1)
            tp.tp_Width = page->pg_Cols;
        /*if (tp.tp_Height == -1)
            tp.tp_Height = page->pg_Rows;*/

		foreach (&page->pg_Table, tf)
        {
            if ((tf->tf_Col >= tp.tp_Col || tf->tf_Width+tf->tf_Col >= tp.tp_Col) && tf->tf_Col <= tp.tp_Col+tp.tp_Width)
				SetTFWidth(page, tf);
        }
    }
    if (page->pg_MarkCol != -1)
		setTableCoord(page, (APTR)&page->pg_MarkX1, page->pg_MarkCol, page->pg_MarkRow, page->pg_MarkWidth, page->pg_MarkHeight);
    if (page->pg_Gad.DispPos > PGS_NONE)
    {
		setCoordPkt(page, &page->pg_Gad.cp, page->pg_Gad.cp.cp_Col, page->pg_Gad.cp.cp_Row);
        page->pg_Gad.cp.cp_X += page->pg_wTabX;  page->pg_Gad.cp.cp_Y += page->pg_wTabY;
    }
    DrawTable(page->pg_Window);
}

struct MinList *merker = NULL;
 
static void
ApplyCellUndoRedo(struct Page *page, struct UndoNode *un, struct MinList *list)
{
	struct tableField *tf, *ntf, *stf;
	long   maxcol = 0, changedCells = 42;
    BOOL   tabgadget = FALSE;

	merker = list;
    /*** evtl. vorhandenes TabGadget entfernen ***/

    if (page->pg_Gad.DispPos != PGS_NONE)
    {
#ifdef __amigaos4__
        if(page->pg_Gad.DispPos > 0) 	//no undo/redo when a the cursor in the table gadget is activ, this prevents a grim!
			return;
#endif
        FreeTabGadget(page);
        tabgadget = TRUE;
    }

    /*** Zu ändernde Zellen entfernen ***/

	if (un->un_Type == UNT_BLOCK_CHANGED)
    {
        ULONG handle;

		DE(bug("ApplyUndoRedo(): change block, %ld cells\n", CountNodes(list)));

        if ((handle = GetCellIterator(page, &un->un_TablePos, FALSE)) != 0)
        {
            while ((tf = NextCell(handle)))
            {
                if (tf->tf_Col + tf->tf_Width > maxcol)
                    maxcol = tf->tf_Col + tf->tf_Width;

                RemoveCell(page, tf, true);
                    // ToDo: for now, please fix this (we don't know if the cell will
                    //  be replaced at this point, so we have to recalculate it)
                FreeTableField(tf);
            }
            FreeCellIterator(handle);
        }
    }
	else if (un->un_Type == UNT_CELLS_CHANGED)
    {
		DE(bug("ApplyUndoRedo(): change %ld cells\n", CountNodes(list)));

		for (changedCells = 0, tf = (APTR)page->pg_Table.mlh_Head, stf = (APTR)list->mlh_Head;
			stf->tf_Node.mln_Succ && tf->tf_Node.mln_Succ; tf = ntf)
        {
            ntf = (struct tableField *)tf->tf_Node.mln_Succ;
            if (stf->tf_Col == tf->tf_Col && stf->tf_Row == tf->tf_Row)
            {
				DE(bug("ApplyUndoRedo(): remove cell at %ld:%ld\n",tf->tf_Col,tf->tf_Row));

                if (tf->tf_Col+tf->tf_Width > maxcol)
                    maxcol = tf->tf_Col+tf->tf_Width;

                changedCells++;
                RemoveCell(page, tf, stf->tf_Type == TFT_EMPTY);
                    // only recalculate if no new cell will be inserted
                FreeTableField(tf);
                stf = (struct tableField *)stf->tf_Node.mln_Succ;
            }
        }
        if (un->un_Mode == UNM_PAGECOLORS)
            changedCells = 42;
    }

    /*** Neue Zellen einfügen ***/

    foreach (list, tf)
    {
        if (tf->tf_Type == TFT_EMPTY)
        {
            DE(bug("ApplyUndoRedo(): \"insert\" empty cell at %ld:%ld\n", tf->tf_Col, tf->tf_Row));

            if (changedCells < 10)
                DrawTableField(page, tf);
        }
        else if ((ntf = CopyCell(page, tf)) != 0)
        {
            DE(bug("ApplyUndoRedo(): insert cell at %ld:%ld\n", tf->tf_Col, tf->tf_Row));

            InsertCell(page, ntf, false);
            ntf->tf_OldWidth = tf->tf_OldWidth;  // wiederherstellen der darzustellenden Breite

            if (ntf->tf_Col + ntf->tf_Width > maxcol)
                maxcol = ntf->tf_Col + ntf->tf_Width;
            if (changedCells < 10)
                DrawTableField(page, ntf);

            if (un->un_Node.ln_Type & UNDO_MASK)  // special case for mask cells
                UpdateMaskCell(page->pg_Mappe, page, ntf, NULL);
        }
    }
    /*** TabGadget wieder herstellen, wenn nötig ***/

    if (tabgadget)
    {
        page->pg_Gad.tf = GetTableField(page,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);
        CreateTabGadget(page,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row,FALSE);
    }

    /*** Grafik beim Block oder der ganzen Tabelle aktualisieren ***/

	if (un->un_Type == UNT_BLOCK_CHANGED && maxcol)
        DrawTablePos(page,un->un_TablePos.tp_Col,un->un_TablePos.tp_Row,maxcol-un->un_TablePos.tp_Col,un->un_TablePos.tp_Height);
	else if ((changedCells >= 10) && un->un_Type == UNT_CELLS_CHANGED)
    {
        if (un->un_Mode == UNM_PAGECOLORS)
        {
            if (list == &un->un_UndoList)      /* undo page colors */
            {
                page->pg_APen = un->un_TablePos.tp_Col;
                page->pg_BPen = un->un_TablePos.tp_Row;
            }
            else                                /* redo page colors */
            {
                page->pg_APen = un->un_TablePos.tp_Width;
                page->pg_BPen = un->un_TablePos.tp_Height;
            }
        }
        DrawTable(page->pg_Window);
    }
    RecalcTableFields(page);
    RefreshToolBar(page);
}


bool
ApplyRedo(struct Page *page)
{
    struct UndoNode *un;

    if (!page)
        return false;

    if (!(un = page->pg_CurrentUndo) || un->un_Node.ln_Pred == (struct Node *)&page->pg_Undos.mlh_Head)
    {
        DisplayBeep(NULL);
        return false;
    }

    page->pg_CurrentUndo = un = (struct UndoNode *)page->pg_CurrentUndo->un_Node.ln_Pred;
    page->pg_Modified++;
    UpdateModified(page->pg_Mappe);

    switch (un->un_Type)
    {
		case UNT_BLOCK_CHANGED:
		case UNT_CELLS_CHANGED:
			ApplyCellUndoRedo(page, un, &un->un_RedoList);
            break;
		case UNT_INSERT_HORIZ_CELLS:
		case UNT_INSERT_VERT_CELLS:
		case UNT_REMOVE_HORIZ_CELLS:
		case UNT_REMOVE_VERT_CELLS:
            InReCells(page,un->un_Type,un->un_TablePos.tp_Col,un->un_TablePos.tp_Row,un->un_TablePos.tp_Width,un->un_TablePos.tp_Height,0);
            break;
		case UNT_CELL_SIZE:
			ChangeCellSize(page, NULL, NULL, CCS_STANDARD, un);
            break;
		case UNT_OBJECT_ATTRS:
			ApplyObjectAttrsUndoRedo(page, un, TYPE_REDO);
            break;
		case UNT_OBJECTS_MOVE:
			ApplyObjectsMoveUndoRedo(page, un, TYPE_REDO);
            break;
		case UNT_OBJECT_KNOB:
			ApplyObjectKnobUndoRedo(page, un, TYPE_REDO);
            break;
		case UNT_OBJECT_SIZE:
			ApplyObjectSizeUndoRedo(page, un, TYPE_REDO);
            break;
		case UNT_ADD_OBJECTS:
			ApplyAddObjects(page, un);
            break;
		case UNT_REMOVE_OBJECTS:
			ApplyRemoveObjects(page, un);
            break;
		case UNT_OBJECT_DEPTH:
			ApplyObjectDepthUndoRedo(page, un, TYPE_REDO);
            break;
		case UNT_DIAGRAM_TYPE:
			ApplyDiagramTypeUndoRedo(page, un, TYPE_REDO);
			break;
    }
    return true;
}


bool
ApplyUndo(struct Page *page)
{
    struct UndoNode *un;
    UBYTE  old;

    if (!page)
        return false;

    if (!(un = page->pg_CurrentUndo) || !un->un_Node.ln_Succ)
    {
        DisplayBeep(NULL);
        return false;
    }

    page->pg_CurrentUndo = (struct UndoNode *)page->pg_CurrentUndo->un_Node.ln_Succ;
    page->pg_Modified--;
    UpdateModified(page->pg_Mappe);

    switch (un->un_Type)
    {
		case UNT_CELLS_CHANGED:
		case UNT_BLOCK_CHANGED:
			ApplyCellUndoRedo(page,un,&un->un_UndoList);
            break;
		case UNT_INSERT_HORIZ_CELLS:
		case UNT_INSERT_VERT_CELLS:
            InReCells(page,un->un_Type+2,un->un_TablePos.tp_Col,un->un_TablePos.tp_Row,un->un_TablePos.tp_Width,un->un_TablePos.tp_Height,NULL);
            break;
		case UNT_REMOVE_HORIZ_CELLS:
		case UNT_REMOVE_VERT_CELLS:
            InReCells(page,un->un_Type-2,un->un_TablePos.tp_Col,un->un_TablePos.tp_Row,un->un_TablePos.tp_Width,un->un_TablePos.tp_Height,(LONG *)un->un_mmUndo);
			old = un->un_Type;  un->un_Type = UNT_BLOCK_CHANGED;
			ApplyCellUndoRedo(page, un, &un->un_UndoList);
            un->un_Type = old;
            break;
		case UNT_CELL_SIZE:
			ApplyCellSizeUndo(page, un);
            break;
		case UNT_OBJECT_ATTRS:
			ApplyObjectAttrsUndoRedo(page, un, TYPE_UNDO);
            break;
		case UNT_OBJECTS_MOVE:
			ApplyObjectsMoveUndoRedo(page, un, TYPE_UNDO);
            break;
		case UNT_OBJECT_KNOB:
			ApplyObjectKnobUndoRedo(page, un, TYPE_UNDO);
            break;
		case UNT_OBJECT_SIZE:
			ApplyObjectSizeUndoRedo(page, un, TYPE_UNDO);
            break;
		case UNT_ADD_OBJECTS:
			ApplyRemoveObjects(page, un);
            break;
		case UNT_REMOVE_OBJECTS:
			ApplyAddObjects(page, un);
            break;
		case UNT_OBJECT_DEPTH:
			ApplyObjectDepthUndoRedo(page, un, TYPE_UNDO);
            break;
		case UNT_DIAGRAM_TYPE:
			ApplyDiagramTypeUndoRedo(page, un, TYPE_UNDO);
			break;
    }
    return true;
}


void
FreeUndo(struct Page *page,struct UndoNode *un)
{
    if (!page || !un)
        return;

    FreeString(un->un_Node.ln_Name);

	switch (un->un_Type)
    {
		case UNT_CELL_SIZE:
		{
			struct UndoCellSize *ucs;

			while ((ucs = (struct UndoCellSize *)MyRemHead(&un->un_UndoList)) != 0)
				FreePooled(pool, ucs, sizeof(struct UndoCellSize));
			while ((ucs = (struct UndoCellSize *)MyRemHead(&un->un_RedoList)) != 0)
				FreePooled(pool, ucs, sizeof(struct UndoCellSize));
            break;
		}
		case UNT_OBJECTS_MOVE:
		{
			struct UndoLink *ul;
			while ((ul = (struct UndoLink *)MyRemHead(&un->un_UndoList)) != 0)
				FreePooled(pool, ul, sizeof(struct UndoLink));
            break;
		}
		case UNT_OBJECT_KNOB:
            break;
		case UNT_OBJECT_SIZE:
			FreePooled(pool, un->un_FreeType == TYPE_REDO ? un->un_UndoKnobs : un->un_RedoKnobs,
				sizeof(struct point2d) * un->un_Object->go_NumKnobs);
            break;
		case UNT_OBJECT_ATTRS:
            FreeObjectAttrsUndo(un);
            break;
		case UNT_ADD_OBJECTS:
		case UNT_REMOVE_OBJECTS:
        {
            struct gGroup *gg;

			while ((gg = (APTR)MyRemHead(&un->un_RedoList)) != 0)
                FreeGGroup(gg);
            break;
        }
        default:
		{
			struct tableField *tf;

			while ((tf = (struct tableField *)MyRemHead(&un->un_UndoList)) != 0)
                FreeTableField(tf);
			while ((tf = (struct tableField *)MyRemHead(&un->un_RedoList)) != 0)
                FreeTableField(tf);

            if (un->un_mmUndo)
            {
				if (un->un_Type == UNT_REMOVE_VERT_CELLS)
					FreePooled(pool, (APTR)un->un_mmUndo, sizeof(LONG) * (un->un_TablePos.tp_Height + 1));
				else if (un->un_Type == UNT_REMOVE_HORIZ_CELLS)
					FreePooled(pool, (APTR)un->un_mmUndo, sizeof(LONG) * (un->un_TablePos.tp_Width + 1));
            }
            break;
		}
    }
	FreePooled(pool, un, sizeof(struct UndoNode));
}


struct UndoNode *
CreateUndo(struct Page *page, UBYTE type, CONST_STRPTR t)
{
    struct UndoNode *un;

    if (!page)
        return NULL;

	if ((un = AllocPooled(pool, sizeof(struct UndoNode))) != 0)
    {
        if (type & UNDO_BLOCK && page->pg_MarkCol != -1)
            CopyMem(&page->pg_MarkCol,&un->un_TablePos,sizeof(struct tablePos));
        else if (page->pg_Gad.DispPos != PGS_NONE)
        {
            /*if (!(type & UNDO_PRIVATE))
                type = UNDO_CELL;*/
            un->un_TablePos.tp_Col = page->pg_Gad.cp.cp_Col;
            un->un_TablePos.tp_Row = page->pg_Gad.cp.cp_Row;
        }
        else if (!(type & UNDO_PRIVATE))
        {
			FreePooled(pool, un, sizeof(struct UndoNode));
            return NULL;
        }
        un->un_Node.ln_Name = AllocString(t);
        un->un_Node.ln_Type = type | (type & UNDO_PRIVATE ? 0 : UNDO_NOREDO);
        if (page->pg_CurrentUndo && page->pg_CurrentUndo != (struct UndoNode *)page->pg_Undos.mlh_Head)
        {
            while (page->pg_CurrentUndo != (struct UndoNode *)page->pg_Undos.mlh_Head)
                FreeUndo(page,(struct UndoNode *)MyRemHead(&page->pg_Undos));
        }
        MyNewList(&un->un_UndoList);
        MyNewList(&un->un_RedoList);
		MyAddHead(&page->pg_Undos, un);

        page->pg_CurrentUndo = un;
        page->pg_Modified++;
        page->pg_Mappe->mp_Modified = TRUE;
    }
    return un;
}


void
MakeUndoRedoList(struct Page *page,struct UndoNode *un,struct MinList *list)
{
	struct tableField *tf, *stf;

    if (un->un_Node.ln_Type & UNDO_CELL)
    {
        if ((tf = CopyCell(page, GetTableField(page, un->un_TablePos.tp_Col, un->un_TablePos.tp_Row))) != 0)
            MyAddTail(list, tf);
    }
    else
    {
        ULONG handle;

		if ((handle = GetCellIterator(page, &un->un_TablePos, FALSE)) != 0)
        {
            struct CellIterator *ci;

			while ((stf = NextCell(handle)) != 0)
            {
                if ((tf = CopyCell(page, stf)) != 0)
                    MyAddTail(list, tf);
            }

            if ((ci = GetCellIteratorStruct(handle)) != 0)
            {
                if (un->un_TablePos.tp_Width == -1)
                    un->un_TablePos.tp_Width = ci->ci_MaxCol-un->un_TablePos.tp_Col;
                if (un->un_TablePos.tp_Height == -1)
                    un->un_TablePos.tp_Height = ci->ci_MaxRow-un->un_TablePos.tp_Row;
            }
            FreeCellIterator(handle);
        }
    }
}


void
EndUndo(struct Page *page)
{
    struct UndoNode *un;

    if (!page || !(un = page->pg_CurrentUndo) || !(un->un_Node.ln_Type & UNDO_NOREDO))
        return;

    un->un_Node.ln_Type &= ~UNDO_NOREDO;
	MakeUndoRedoList(page, un, &un->un_RedoList);
}


struct UndoNode *
BeginUndo(struct Page *page, UBYTE type, CONST_STRPTR t)
{
    struct UndoNode *un;

    if ((un = CreateUndo(page, type, t)) != 0)
    {
		un->un_Type = UNT_BLOCK_CHANGED;
        MakeUndoRedoList(page, un, &un->un_UndoList);
    }
    return un;
}


