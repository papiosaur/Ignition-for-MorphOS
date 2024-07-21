/* References for the automatic computations
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"


// TODO: what about external references to pages that don't exist?

#define TRACE_REFERENCES 0
#undef D

#if TRACE_REFERENCES
#   define D(x) x
#else
#   define D(x) ;
#endif

struct ArrayList gUnresolvedRefs, gTimedRefs;


void
ReferenceError(void)
{
	ErrorRequest(GetString(&gLocaleInfo, MSG_REFERENCES_FATAL_ERR));
}


__inline uint8
ReferenceType(struct Reference *r)
{
	return (uint8)(r->r_Type & RTYPE_TYPEMASK);
}


#ifdef DEBUG

void
DumpReference(struct Reference *r)
{
	if (!r) {
		bug("null reference!!!\n");
		return;
	}

	bug("%08lx: reference, page = \"%s\"",r, r->r_Page->pg_Node.ln_Name);

	if (r->r_Type & RTYPE_TIMED)
		bug(", *TIMED*");

	bug(", type = ");
	switch (ReferenceType(r)) {
		case RTYPE_CELL:
			bug("cell at %ld:%ld\n", ((struct tableField *)r->r_This)->tf_Col,((struct tableField *)r->r_This)->tf_Row);
			break;
		case RTYPE_NAME:
			bug("name \"%ss\"\n", ((struct Name *)r->r_This)->nm_Node.ln_Name);
			break;
		case RTYPE_UNRESOLVED_CELL:
			bug("unresolved cell at %ld:%ld\n", r->r_Col, r->r_Row);
			break;
		case RTYPE_UNRESOLVED_RANGE:
			bug("unresolved range at %ld:%ld, size = %ld:%ld, count = %ld\n", r->r_Range, r->r_Count);
			break;
	}
}


void
VerboseDumpReference(struct Reference *r)
{
	int index;

	DumpReference(r);
	if (!r)
		return;

	index = r->r_References.al_Last;
	bug("   references:%s\n",index ? "" : "\n\t<empty>");
	while (index-- > 0)
	{
		bug("%5ld:\t",index);
		DumpReference(r->r_References.al_List[index]);
	}

	index = r->r_ReferencedBy.al_Last;
	bug("   referenced by:%s\n",index ? "" : "\n\t<empty>");
	while (index-- > 0)
	{
		bug("%5ld:\t",index);
		DumpReference(r->r_ReferencedBy.al_List[index]);
	}
}

#endif

/** Überträgt alle Referenzen der "source" Reference auf das
 *  Ziel. "source" wird danach auf Wunsch freigegeben.
 *
 *  @param dest destination Reference
 *  @param source source Reference
 *  @param freeSource free source reference
 */

void
MergeReferences(struct Reference *dest, struct Reference *source, bool freeSource)
{
	D({
		int count = source->r_ReferencedBy.al_Last;

		bug("mergeReferences from ");
		DumpReference(source);
		bug("				to   ");
		DumpReference(dest);

		bug("1.");
		if (!count)
			bug("\t<empty>\n");
		while (count-- > 0)
		{
			bug("\t");
			DumpReference(source->r_ReferencedBy.al_List[count]);
		}
		bug("2.");
		count = dest->r_ReferencedBy.al_Last;
		if (!count)
			bug("\t<empty>\n");
		while (count-- > 0)
		{
			bug("\t");
			DumpReference(dest->r_ReferencedBy.al_List[count]);
		}
	});

	AddListToArrayList(&source->r_ReferencedBy,&dest->r_ReferencedBy);

	if (freeSource)
	{
		MakeEmptyArrayList(&source->r_ReferencedBy);
		FreeReference(source, false);   // TODO: is "false" correct here?
	}
}


void
AssignUnresolvedReferencesForName(struct Page *page, struct Name *nm)
{
	int count = gUnresolvedRefs.al_Last;
	struct Reference *thisr = nm->nm_Reference;

	D(bug("assignUnresolvedReferencesForName()\n"));

	for (; count-- > 0;)
	{
		struct Reference *r;

		// correct page?
		if ((r = gUnresolvedRefs.al_List[count])->r_Page != page)
			continue;

		/* TODO: after the first reference has been found, collect only... */

		if (ReferenceType(r) == RTYPE_UNRESOLVED_NAME
			&& zstricmp(r->r_Name, nm->nm_Node.ln_Name)) {
			D(bug("found unresolved name for %s\n", nm->nm_Node.ln_Name));

			RemoveFromArrayList(&gUnresolvedRefs, r);

			if (!thisr) {
				// recycle the old reference if we need one

				r->r_Type = RTYPE_NAME;
				r->r_This = nm;
				FreeString(r->r_Name);
				//r->r_Name = NULL;

				nm->nm_Reference = thisr = r;
			} else
				MergeReferences(thisr, r, true);
		}
	}
}


void
AssignUnresolvedReferencesForCell(struct Page *page, struct tableField *tf)
{
	/* MERKER: da ist wohl noch Raum für Verbesserungen... */
	// z.B. koennte die Liste sortiert werden, und vielleicht auch noch
	// verschiedene Listen fuer die verschiedenen Typen

	int count = gUnresolvedRefs.al_Last;
	struct Reference *thisr = tf->tf_Reference;

	D(bug("assignUnresolvedReferencesForCell()\n"));

	while (count-- > 0)
	{
		struct Reference *r;

		// correct page?
		if ((r = gUnresolvedRefs.al_List[count])->r_Page != page)
			continue;

		/* TODO: after the first reference has been found, collect only... */

		if (ReferenceType(r) == RTYPE_UNRESOLVED_CELL
			&& r->r_Col == tf->tf_Col && r->r_Row == tf->tf_Row) {
			D(bug("found unresolved cell at %ld:%ld\n", tf->tf_Col, tf->tf_Row));

			RemoveFromArrayList(&gUnresolvedRefs, r);

			if (!thisr) {
				r->r_Type = RTYPE_CELL;
				r->r_This = tf;

				tf->tf_Reference = thisr = r;
			} else
				MergeReferences(thisr, r, true);
		} else if (ReferenceType(r) == RTYPE_UNRESOLVED_RANGE
				 && IsCellInRange(&r->r_Range, tf)) {
			D(bug("found unresolved range at %ld:%ld\n", tf->tf_Col, tf->tf_Row));

			if (!--r->r_Count) {
				RemoveFromArrayList(&gUnresolvedRefs, r);

				if (!thisr) {
					// recycle special reference

					r->r_Type = RTYPE_CELL;
					r->r_This = tf;

					tf->tf_Reference = thisr = r;
				} else
					MergeReferences(thisr, r, true);
			} else {
				if (!thisr)
					tf->tf_Reference = thisr = MakeReference(page, RTYPE_CELL, tf, tf->tf_Root);

				MergeReferences(thisr, r, false);
//				if (!CopyArrayList(&r->r_ReferencedBy,&tf->tf_Reference->r_ReferencedBy))
//					ReferenceError();
			}
		}
	}
}


/** Geht rekursiv durch die Liste der referenzierten Objekte
 *  und testet auf Vorhandensein der neuen Referenz.
 */
static bool
CheckForLoops(struct Reference *r, struct Reference *source)
{
	struct Reference **refs = (struct Reference **)r->r_References.al_List;
	int32 index;

	if(!r)
		return false;
//printf("r=0x%08X source=0x%08X\n", r, source);
#ifdef __amigaos4__
//if(r->r_References.al_Last > 10 || r->r_Type > 10 || r->r_References.al_Size > 10)
// printf("index=%ld %d %ld\n", r->r_References.al_Last, r->r_Type, r->r_References.al_Size);
	if(r->r_References.al_Last > r->r_References.al_Size)
		{
//   		 printf("Wrong size!\n");
			return false;
		}
	if(r->r_References.al_Last > 10000)
	{
//    	printf("Catch!\n");
    	return false;
	}
#endif
	for (index = r->r_References.al_Last; index-- > 0;)
	{
		bool loop;

		if (refs[index] == source)
		{
			return true;
		}
		
		loop = CheckForLoops(refs[index], source);
		if (loop)
		{
			return true;
		}
	}
	return false;
}


bool
ConnectReferences(struct Reference *r, struct Reference *rb)
{
	// loops won't be connected
	if (CheckForLoops(rb, r)) {
		r->r_Type |= RTYPE_LOOP;
		D(bug("connectReferences: loop detected: %lx -> %lx\n", r, rb));
		return false;
	}

	// check if the references are already connected
	{
		struct Reference **refs = (struct Reference **)GetArrayListArray(&r->r_References);
		int32 index = CountArrayListItems(&r->r_References);

		while (index-- > 0) {
			if (refs[index] == rb) {
				D(bug("connection already found...\n"));
				return true;
			}
		}
	}

	AddToArrayList(&rb->r_ReferencedBy, r);
	AddToArrayList(&r->r_References, rb);

	return true;
}

void
AddReferences(struct Reference *r, struct Term *t)
{
	if (!t)
		return;

	if (t->t_Op != OP_RANGE) { 
		AddReferences(r, t->t_Left);
		AddReferences(r, t->t_Right);
	}

	switch (t->t_Op) {
		case OP_RANGE:
		{
			struct tableField *tf;
			struct tablePos tp;
			uint32 handle, count;
			uint32 x,y;

#ifdef __amigaos4__
			ClearMem(&tp, sizeof(struct tablePos));	//Init. Structure to prevent Crash if range for example 11:a6
#endif
			FillTablePos(&tp, t);

#ifdef __amigaos4__
			if(tp.tp_Width == 0 && tp.tp_Height == 0) //if no range is there terminate function
				return;
			for(x = tp.tp_Col; x < tp.tp_Width + 2; x++) //prefill range, to avoid a loop-problem
				for(y = tp.tp_Row; y < tp.tp_Height + 2; y++)
				{
					tf = GetTableField(r->r_Page, x, y);
					if(tf == NULL)
						AllocTableField(r->r_Page, x, y);
				}
#endif
			count = (tp.tp_Width + 1) * (tp.tp_Height + 1);
			handle = GetCellIterator(r->r_Page, &tp, false);
			while ((tf = NextCell(handle)) != 0) {
#ifdef __amigaos4__
				if (tf == r->r_This)	//if it is the same, then is it a loop contition
				{
					return;
				}
#endif
				if (!tf->tf_Reference && !(tf->tf_Reference = MakeReference(r->r_Page, RTYPE_CELL, tf, tf->tf_Root))) {
					ReferenceError();
					continue;
				}
				if (!ConnectReferences(r, tf->tf_Reference)) //ConnectReferences detect a loop
				{
					return;
				}

				count--;
			}
			if (count) {
				// there are unresolved cells left
				struct Reference *tpr = MakeReference(r->r_Page, RTYPE_UNRESOLVED_RANGE, NULL, NULL);

				D(bug("make unresolved range reference\n"));
				if (!tpr) {
					ReferenceError();
					return;
				}
				CopyMem(&tp, &tpr->r_Range, sizeof(struct tablePos));
				tpr->r_Count = count;

				AddToArrayList(&gUnresolvedRefs, tpr);
				ConnectReferences(r, tpr);
			}
			break;
		}
		case OP_CELL:
		case OP_EXTCELL:
		{
			struct tableField *tf;
			int32 col = t->t_Col + (t->t_AbsCol ? 0 : tf_col);
			int32 row = t->t_Row + (t->t_AbsRow ? 0 : tf_row);

			if (t->t_Op == OP_EXTCELL)
				tf = GetExtTableField(t);
			else
				tf = GetTableField(r->r_Page, col, row);

			if (tf == r->r_This)
			{
				D(bug("addReferences: loop detected\n"));
				break;
			}
			if (tf)
			{
				if (!tf->tf_Reference && !(tf->tf_Reference = MakeReference(r->r_Page, RTYPE_CELL, tf, tf->tf_Root)))
				{
					ReferenceError();
					break;
				}
				ConnectReferences(r, tf->tf_Reference);
				return;
			}

			// handle unresolved cell
			{
				int count = gUnresolvedRefs.al_Last;
				struct Reference *searchr;

				// perhaps the cell is already referenced somewhere else...

				while (count-- > 0)
				{
					if ((searchr = gUnresolvedRefs.al_List[count])->r_Page != r->r_Page)
						continue;

					if (ReferenceType(searchr) == RTYPE_UNRESOLVED_CELL
						&& searchr->r_Col == col
						&& searchr->r_Row == row)
					{
						ConnectReferences(r,searchr);
						return;
					}
				}

				// perhaps not
				{
					struct Reference *tpr = MakeReference(r->r_Page, RTYPE_UNRESOLVED_CELL, NULL, NULL);

					if (!tpr)
						break;

					if (t->t_Op == OP_EXTCELL)
						tpr->r_Page = GetExtCalcPage(t);

					tpr->r_Col = col;
					tpr->r_Row = row;

					// save link to unresolved reference
					AddToArrayList(&gUnresolvedRefs, tpr);

					ConnectReferences(r, tpr);
				}
			}
			break;
		}
		case OP_NAME:
		{
			struct Database *db;
			struct Field *fi;
			int32 fiPos;

			if (!GetNameAndField(t->t_Text, &db, &fi, &fiPos))
				break;

			if (db == r->r_This)
			{
				D(bug("addReferences: loop detected (name = %s)\n", db->db_Node.ln_Name));
				break;
			}

			if (db)
			{
				// name was found, we just have to create a reference to it and connect the two

				if (!db->db_Reference && !(db->db_Reference = MakeReference(r->r_Page, RTYPE_NAME, db, db->db_Root)))
				{
					ReferenceError();
					break;
				}
				ConnectReferences(r, db->db_Reference);
				return;
			}

			// handle unresolved name
			{
				int count = gUnresolvedRefs.al_Last;
				struct Reference *searchr;

				// perhaps the name is already referenced somewhere else...

				while (count-- > 0)
				{
					if ((searchr = gUnresolvedRefs.al_List[count])->r_Page != r->r_Page)
						continue;

					if (ReferenceType(searchr) == RTYPE_UNRESOLVED_NAME
						&& zstricmp(searchr->r_Name, t->t_Text))
					{
						ConnectReferences(r, searchr);
						return;
					}
				}

				// ... perhaps not
				{
					struct Reference *tpr = MakeReference(r->r_Page, RTYPE_UNRESOLVED_NAME, NULL, NULL);

					if (!tpr)
						break;

					tpr->r_Name = AllocString(t->t_Text);

					// save link to unresolved reference
					AddToArrayList(&gUnresolvedRefs, tpr);

					ConnectReferences(r, tpr);
				}
			}
			break;
		}

			break;
		case OP_FUNC:
		{
			struct Function *f = t->t_Function;

			if (f == (APTR)~0L || f == NULL)
				break;

			if (f->f_Code) {
				struct FuncArg *fa;

				foreach (&t->t_Args, fa)
					AddReferences(r, fa->fa_Root);
			}
			
			/* Special treatment of references and today()/now() */

			switch (f->f_ID) {
				case MAKE_ID('t','d','y','\0'):  // today()
				case MAKE_ID('n','o','w','\0'):  // now()
					if ((r->r_Type & RTYPE_TIMED) == 0) {
						// only add the timed reference, if it hasn't been added
						// already (there might be more than one timed reference
						// per object)
						D(bug("** time reference detected (0x%08lx)!\n", r));
						AddToArrayList(&gTimedRefs, r);
						r->r_Type |= RTYPE_TIMED;
					}
					break;
				case MAKE_ID('e','x','t','\0'):  // extern()
					D(bug("not implemented reference: extern()\n"));
					break;
				case MAKE_ID('p','a','g','\0'):  // page()
					D(bug("not implemented reference: page()\n"));
					break;
				case MAKE_ID('r','e','f','\0'):  // reference()
					D(bug("not implemented reference: reference()\n"));
					break;
				case MAKE_ID('c','o','l','\0'):  // column()
					D(bug("not implemented reference: column()\n"));
					break;
				case MAKE_ID('r','o','w','\0'):  // row()
					D(bug("not implemented reference: row()\n"));
					break;
			}
			break;
		}
		default:
			break;
	}
}

void
RemoveReferences(struct Reference *r)
{
	/* MERKER: leere Referenzen könnten auch gelöscht werden... */

	int index = r->r_References.al_Last;

	D(bug("removeReferences(%lx) index = %ld\n", r, index));

	// remove this reference from the timer array, if necessary
	if (r->r_Type & RTYPE_TIMED)
	{
		D(bug("** remove timed reference!\n"));
		RemoveFromArrayList(&gTimedRefs, r);
	}

	r->r_Type &= ~(RTYPE_LOOP | RTYPE_TIMED);

	// remove this reference from other's lists
	while (index-- > 0)
	{
		struct Reference *refr = r->r_References.al_List[index];

		RemoveFromArrayList(&refr->r_ReferencedBy, r);
	}

	// remove all references (but they still remain valid pointers)
	MakeEmptyArrayList(&r->r_References);
}


void
UpdateReferences(struct Reference *r, struct Term *t)
{
	D(bug("updateReferences(%lx)\n", r));

	RemoveReferences(r);

	if (ReferenceType(r) == RTYPE_CELL)
	{
		tf_col = ((struct tableField *)r->r_This)->tf_Col;
		tf_row = ((struct tableField *)r->r_This)->tf_Row;
	}
	else
	{
		tf_col = 0;
		tf_row = 0;
	}

	AddReferences(r, t);
}


void
FreeReference(struct Reference *r, bool recalc)
{
	D(bug("freeReferences(%lx)\n", r));

	if (!r)
		return;

	RemoveReferences(r);

	// If anyone is still referencing this object, move to unresolved list, so that
	// they still be happy (somewhat :-))

	if (r->r_ReferencedBy.al_Last > 0)
	{
		void *this = r->r_This;

		r->r_This = NULL;
		if (recalc)
			RecalcReferencingObjects(r, false);

		// first, search for any unresolved ranges that point
		// to this cell and just increment its reference counter
		// if one could be found
		// Of course, that only applies to RTYPE_CELL

		if (ReferenceType(r) == RTYPE_CELL)
		{
			struct tableField *tf = this;
			struct Reference *searchr;
			int32 index = gUnresolvedRefs.al_Last;
			bool found = false, otherReferences = false;

			while (index-- > 0)
			{
#ifdef __amigaos4__
				if(!(searchr = (struct Reference *)gUnresolvedRefs.al_List[index]))
					continue;
#else
				searchr = (struct Reference *)gUnresolvedRefs.al_List[index];
#endif
				if (ReferenceType(searchr) == RTYPE_UNRESOLVED_CELL
					&& searchr->r_Col == tf->tf_Col && searchr->r_Row == tf->tf_Row)
				{
					otherReferences = true;
				}
				else if (ReferenceType(searchr) == RTYPE_UNRESOLVED_RANGE
					&& IsCellInRange(&searchr->r_Range, tf))
				{
					D(bug("found unresolved range containing the cell\n"));
					searchr->r_Count++;
					found = true;
				}
			}
			// TODO: that's not correct, is it? If any other cell is still referencing us
			//  that one won't be able to see us anymore...
			if (found && !otherReferences)
				goto leave;
		}

		if (ReferenceType(r) == RTYPE_CELL)
		{
			r->r_Type = RTYPE_UNRESOLVED_CELL;
			r->r_Col = ((struct tableField *)this)->tf_Col;
			r->r_Row = ((struct tableField *)this)->tf_Row;
		}
		else if (ReferenceType(r) == RTYPE_NAME)
		{
			r->r_Type = RTYPE_UNRESOLVED_NAME;
			r->r_Name = AllocString(((struct Name *)this)->nm_Node.ln_Name);
		}

		AddToArrayList(&gUnresolvedRefs, r);

		// we don't have to free the reference because we could reuse it
		return;
	}

leave:
	FreePooled(pool, r, sizeof(struct Reference));
}


struct Reference *
MakeReference(struct Page *page, uint8 type, void *this, struct Term *t)
{
	struct Reference *r;

	if (!(r = AllocPooled(pool, sizeof(struct Reference))))
		return NULL;

	D(bug("make reference for %08lx type=%ld (size = %ld, 0x%lx)\n", this, (long)type, (long)sizeof(struct Reference), r));

	r->r_Page = page;
	r->r_Type = type;
	r->r_This = this;

	if (t)
		UpdateReferences(r, t);

	return r;
}

