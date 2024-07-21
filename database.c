/* Database functionality
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"


void UpdateDBCurrent(struct Database *db, ULONG current);


BOOL
IsDBEmpty(struct Database *db)
{
	struct Index *in;
	BOOL empty = FALSE;

	if (!db)
		return TRUE;

	if (!GetFirstCell(db->db_Page, &db->db_TablePos))
		empty = TRUE;

	if (((in = (struct Index *)db->db_Filter) || (in = db->db_Index)) && !in->in_Index || !db->db_TablePos.tp_Height && db->db_Page && empty)
		return TRUE;

	return FALSE;
}


struct tableField *
GetFields(struct Database *db, long pos)
{
	if (!db || db->db_Root->t_Op != OP_RANGE && db->db_Root->t_Op != OP_CELL)
		return NULL;

	if (!pos) {
		tf_col = tf_row = 0;
		FillTablePos(&db->db_TablePos,db->db_Root);
	}
	if (pos > db->db_TablePos.tp_Width)
		return NULL;

	if (db->db_TablePos.tp_Row > 1)
		return GetTableField(db->db_Page, db->db_TablePos.tp_Col + pos, db->db_TablePos.tp_Row - 1);

	return (APTR)~0L;   // wenn Datenbank-Titel außerhalb der Tabelle
}


struct Term *
GetField(struct Database *db, STRPTR t)
{
	struct tablePos tp;
	struct Field *fi;
	char pos[42];
	long i, j;

	if (!db)
		return NULL;
	if (!t)
		return db->db_Root;

	for (i = 0; *(t+i) && *(t+i) != '.'; i++);
	tf_col = tf_row = 0;
	if (*(t+i++) != '.' || !FillTablePos(&tp, db->db_Root))
		return db->db_Root;

	if (db->db_Node.ln_Type == NMT_DATABASE && (fi = (APTR)FindTag(&db->db_Fields, t + i)))
		j = FindListEntry(&db->db_Fields, (struct MinNode *)fi);
	else
		return NULL;

	strcpy(pos,AbsCoord2String(TRUE,tp.tp_Col+j,TRUE,tp.tp_Row));
	strcat(pos,":");
	strcat(pos,AbsCoord2String(TRUE,tp.tp_Col+j,TRUE,tp.tp_Row+tp.tp_Height));

	return CreateTree(db->db_Page, pos);
}


STRPTR
AllocFieldText(struct Database *db,struct tableField *tf,long pos)
{
	if (tf != (APTR)~0L)
		return AllocString(tf->tf_Text);

	{
		struct Page *page;
		long i = db->db_TablePos.tp_Col+pos;

		if ((page = db->db_Page) && page->pg_Cols >= i && (page->pg_tfWidth+i-1)->ts_Title)
			return AllocString((page->pg_tfWidth+i-1)->ts_Title);
		else {
			char t[8];

			Pos2String(i,t);
			return AllocString(t);
		}
	}
}


void
CreateFields(struct Database *db)
{
	struct Field *fi;
	struct tableField *tf;
	long pos;

	if (!db)
		return;

	while ((fi = (struct Field *)MyRemHead(&db->db_Fields)) != 0) {
		FreeString(fi->fi_Node.ln_Name);
		FreeString(fi->fi_Special);
		FreePooled(pool,fi,sizeof(struct Field));
	}

	for (pos = 0; (tf = GetFields(db, pos)) != 0; pos++) {
		if ((fi = AllocPooled(pool, sizeof(struct Field))) != 0) {
			fi->fi_Node.ln_Name = AllocFieldText(db, tf, pos);
			MyAddTail(&db->db_Fields, fi);
		}
	}
}


struct Mask *
IsOverMask(struct Page *page)
{
	struct MaskField *mf;
	struct Mask *ma;

	if (!page || page->pg_Gad.DispPos < PGS_FRAME)
		return NULL;

	foreach(&page->pg_Mappe->mp_Masks,ma) {
		if (ma->ma_Page == page) {
			foreach(&ma->ma_Fields,mf) {
				if (mf->mf_Col == page->pg_Gad.cp.cp_Col && mf->mf_Row == page->pg_Gad.cp.cp_Row)
					return ma;
			}
		}
	}

	return NULL;
}


struct Mask *
GuessMask(struct Page *page)
{
	struct Mask *ma,*sma;
	long count = 0;

	foreach (&page->pg_Mappe->mp_Masks, ma) {
		if (ma->ma_Page == page) {
			sma = ma;
			count++;
		}
	}
	if (count > 1)
		return IsOverMask(page);
	if (count == 1)
		return sma;

	return NULL;
}


ULONG
GetDBPos(struct Database *db,ULONG pos)
{
	struct Index *in;
	ULONG  i;

	if (!db || (!((in = (struct Index *)db->db_Filter) || (in = db->db_Index))))
		return pos;
	if (!in->in_Index)
		return 0L;

	for (i = 0;i < in->in_Size;i++) {
		/* aktuellen Eintrag aus Index/Filter heraussuchen */
		if (in->in_Index[i] == pos)
			break;
	}
	return i;
}

struct Database *refdb;

long
refcmp(const ULONG *a,const ULONG *b)
{
	return((long)GetDBPos(refdb,*a)-(long)GetDBPos(refdb,*b));
}


long *
GetDBRefs(STRPTR t)
{
	long   *stack,count = 1,*temp,size = 4096,i,j,k;
	struct Index *in = NULL;

	if (!t)
		return(NULL);
	if (refdb && ((in = (struct Index *)refdb->db_Filter) || (in = refdb->db_Index)) && !in->in_Index)
		in = NULL;
	if ((stack = AllocPooled(pool, size)) != 0)
	{
		while(*t)
		{
			while(*t && !isdigit(*t)) t++;
			if (*t)
			{
				*(stack+count++) = i = atol(t)-1;
				if (in)
				{
					for(j = k = 0;j < in->in_Size;j++)
					{
						if (i == in->in_Index[j])
						{
							k = 1;
							break;
						}
					}
					if (!k)
						count--;
				}
				while(isdigit(*t))
					t++;
				if (count*sizeof(long) > size)
				{
					if ((temp = AllocPooled(pool, size += 4096)) != 0)
					{
						CopyMem(stack,temp,size);
						FreePooled(pool,stack,size);
						stack = temp;
					}
					else
						goto getdbrefs_end;
				}
			}
		}
		getdbrefs_end:
		if (count > 1 && (temp = AllocPooled(pool,count*sizeof(long))))
		{
			CopyMem(stack,temp,count*sizeof(long));
			qsort(temp+1,count-1,sizeof(ULONG),(APTR)refcmp);

			*temp = count-1;
		}
		else
			temp = NULL;
		FreePooled(pool,stack,size);
	}
	return(temp);
}


long *GetDBReferences(struct Database *db,struct Database *pdb)
{
	struct tableField *tf;
	struct Field *fi;
	long   i;

	if (!db || !pdb)
		return(NULL);
	for(i = 0,fi = (APTR)pdb->db_Fields.mlh_Head;fi->fi_Node.ln_Succ;fi = (APTR)fi->fi_Node.ln_Succ,i++)
	{
		if (fi->fi_Node.ln_Type == FIT_REFERENCE && !strcmp(fi->fi_Special,db->db_Node.ln_Name))
		{
			if ((tf = GetTableField(pdb->db_Page, pdb->db_TablePos.tp_Col + i, pdb->db_TablePos.tp_Row + pdb->db_Current)) != 0)
			{
				refdb = db;
				return(GetDBRefs(tf->tf_Text));
			}
		}
	}
	return(NULL);
}


struct Database *GetMaskDatabase(struct Mappe *mp,STRPTR dbase,STRPTR *mask,struct tableField *tf)
{
	struct Database *db;
	struct Field *fi;
	long   len,count = 0,*refs;

	if ((db = (APTR)MyFindName(&mp->mp_Databases, dbase)) != 0)
	{
		refdb = db;
		if (tf && (refs = GetDBRefs(tf->tf_Text)))
		{
			for(len = *refs;len;len--)           // Aktuellen Eintrag aus Referenzen heraussuchen
				if (db->db_Current == *(refs+len))
					break;
			if (!len && *refs && *(refs+1) >= 0 && *(refs+1) <= db->db_TablePos.tp_Height)
			{                                    /* Position aktualisieren */
				db->db_Current = *(refs+1);
				db->db_IndexPos = GetDBPos(db,db->db_Current);
			}
			FreePooled(pool,refs,((len = *refs)+1)*sizeof(long));
			if (!len)
				refs = NULL;
		}
		if (tf && !refs)  /* empty references */
		{
			*mask += strlen(*mask);   /* -> empty string */
			return(db);
		}
		foreach(&db->db_Fields,fi)
		{
			if (fi->fi_Node.ln_Name && !strncmp(fi->fi_Node.ln_Name,*mask,len = strlen(fi->fi_Node.ln_Name)) && *(*mask+len) == '.')
			{
				if (fi->fi_Node.ln_Type == FIT_REFERENCE && (tf = GetTableField(db->db_Page,db->db_TablePos.tp_Col+count,db->db_TablePos.tp_Row+db->db_Current)) && tf->tf_Text)
				{
					*mask = *mask+len+1;
					return(db = GetMaskDatabase(mp,fi->fi_Special,mask,tf));
				}
			}
			count++;
		}
	}
	else
		ErrorRequest(GetString(&gLocaleInfo, MSG_DATABASE_NOT_FOUND_ERR),dbase);
	return(db);
}


void FreeIndex(struct MinList *mlh)
{
	struct Index *in;

	while((in = (APTR)MyRemHead(mlh)) != 0)
	{
		if (in->in_Index)
			FreePooled(pool,in->in_Index,in->in_Size*sizeof(ULONG));
		FreeString(in->in_Node.ln_Name);
		FreePooled(pool,in,sizeof(struct Index));
	}
}


void
FreePreparedIndex(struct MinList *mlh)
{
	struct Node *ln;

	while((ln = MyRemHead(mlh)) != 0)
		FreePooled(pool,ln,sizeof(struct Node)+sizeof(ULONG));

	FreePooled(pool,mlh,sizeof(struct MinList));
}


struct MinList *
PrepareIndex(struct Database *db, struct Index *in)
{
	struct MinList *list;
	struct Node *ln;
	struct Field *fi;
	STRPTR name,part;
	ULONG  i;

	if ((list = AllocPooled(pool, sizeof(struct MinList))) != 0)
	{
		if ((name = AllocString(in->in_Node.ln_Name)) != 0)
		{
			MyNewList(list);
			while(strlen(name))
			{
				part = (STRPTR)FilePart(name);
				if ((ln = AllocPooled(pool, sizeof(struct Node) + sizeof(ULONG))) != 0)
				{
					if (*part == '-')                        /* sort direction */
						part++,  ln->ln_Type = 1;

					i = 0;
					foreach(&db->db_Fields,fi)
					{
						if (!zstricmp(part,fi->fi_Node.ln_Name))
						{
							*(ULONG *)(ln+1) = i;
							break;
						}
						i++;
					}
					MyAddHead(list, ln);
				}
				*PathPart(name) = 0;
			}
		}
	}

	return list;
}


APTR  *mitable;   /* MakeIndex()-Table */
struct MinList *milist;
ULONG misize,mipos,mitype,micmps;
long  midirection;

long micmptext(const ULONG *a,const ULONG *b);
long micmpvalue(const ULONG *a,const ULONG *b);


long
NextIndexSort(const ULONG *a, const ULONG *b)
{
	long rc;

	if (mipos >= micmps-1)
		return 0L;

	mipos++;  mitable += misize;
	if (mitype & (1 << mipos))
		rc = micmptext(a,b);
	else
		rc = micmpvalue(a,b);
	mitable -= misize;  mipos--;

	return rc;
}


long
micmptext(const ULONG *a,const ULONG *b)   /* MakeIndex()-Textsort */
{
	STRPTR sa = mitable[*a],sb = mitable[*b];
	long rc = stricmp(sa ? sa : (STRPTR)"",sb ? sb : (STRPTR)"");

	if (!rc)
		rc = NextIndexSort(a,b);
	return(midirection & (1 << mipos) ? -rc : rc);
}


long micmpvalue(const ULONG *a,const ULONG *b)  /* MakeIndex()-Valuesort */
{
	double da,db;
	long   rc;

	if ((da = (mitable[*a] ? *(double *)mitable[*a] : 0.0)) == (db = (mitable[*b] ? *(double *)mitable[*b] : 0.0)))
		rc = NextIndexSort(a,b);
	else if (da < db)
		rc = -1;
	return midirection & (1 << mipos) ? -rc : rc;
}


BOOL
MakeIndex(struct Database *db,struct Index *in)
{
	struct tablePos ALIGNED tp;
	struct tableField *tf;
	struct Node *ln;
	long   i,row;

	if (!db || !in)
		return(FALSE);

	if (in->in_Index)
		FreePooled(pool,in->in_Index,in->in_Size*sizeof(ULONG));

	CopyMem(&db->db_TablePos,&tp,sizeof(struct tablePos));
	misize = tp.tp_Height+1;
	if ((milist = PrepareIndex(db, in)) != 0)
	{
		if ((mitable = AllocPooled(pool, misize * sizeof(APTR) * (micmps = CountNodes(milist)))) != 0)
		{
			if ((in->in_Index = AllocPooled(pool, misize * sizeof(ULONG))) != 0)
			{
				in->in_Size = misize;      /* index initialisation */
				for(i = 0;i < misize;i++)
					in->in_Index[i] = i;

				tp.tp_Width = 0;           /* sorting */
				i = 0;  mipos = 0;  midirection = 0;  mitype = 0;
				foreach(milist,ln)
				{
					ULONG handle;

					if (ln->ln_Type)                                      /* sort direction: */
						midirection |= 1 << mipos;                          /* Z-A or A-Z      */
					tp.tp_Col = db->db_TablePos.tp_Col+*(ULONG *)(ln+1);  /* sort column */
					row = tp.tp_Row;

					if ((handle = GetCellIterator(db->db_Page, &tp, FALSE)) != 0)
					{
						while((tf = NextCell(handle)) && (!tf->tf_Text));
						FreeCellIterator(handle);

						if (tf->tf_Text)
						{
							if (tf->tf_Type & TFT_TEXT)
								mitype |= (1 << mipos);

							if ((handle = GetCellIterator(db->db_Page, &tp, FALSE)) != 0)
							{
								for(; (tf = NextCell(handle)) != 0; row = tf->tf_Row)
								{
									i += tf->tf_Row-row;  /* empty cells */
									mitable[i] = tf->tf_Type & TFT_TEXT ? (ULONG *)tf->tf_Text : (ULONG *)&tf->tf_Value;
								}
								FreeCellIterator(handle);
								i++;
							}
						}
						else
							i += misize;
					}
					mipos++;
				}
				mipos = 0;                 /* wird in NextIndexSort() gebraucht */
				qsort(in->in_Index,misize,sizeof(ULONG),(APTR)(mitype & 1 ? micmptext : micmpvalue));
				/*for(i = 0;i < misize;i++)
					bug(":: %ld (%s)\n",in->in_Index[i],mitable[in->in_Index[i]]);*/

				FreePooled(pool,mitable,misize*sizeof(APTR));
				FreePreparedIndex(milist);
				mitable = NULL;  milist = NULL;

				return TRUE;
			}
			FreePooled(pool,mitable,misize*sizeof(APTR));
		}
		FreePreparedIndex(milist);
	}
	ErrorRequest(GetString(&gLocaleInfo, MSG_CREATE_INDEX_ERR));

	return FALSE;
}


/** Walks through a term, and replaces all simple references to a database field
 *	of the form "field" with the complete form (ie. "database.field").
 */

void
PrepareFilter(struct Database *db, struct Term *t)
{
	if (!t)
		return;

	PrepareFilter(db, t->t_Left);
	PrepareFilter(db, t->t_Right);

	if (t->t_Op == OP_NAME && t->t_Text)    /* term is a name */
	{
		if (FindTag(&db->db_Fields, t->t_Text))  /* references a field in the database */
		{
            char s[256];

			strcpy(s, db->db_Node.ln_Name);
			strcat(s, ".");
			strcat(s, t->t_Text);
			FreeString(t->t_Text);
			t->t_Text = AllocString(s);
		}
	}
}


/**	Creates the index for the filter \fi of the specified database \a db.
 *	If the current filter should not be used as a basis for this filtering,
 *	db_Filter must be set to \c NULL before.
 */

BOOL
MakeFilter(struct Database *db, struct Filter *fi)
{
	ULONG  i,j,size;
	ULONG  *dest;

	if (!db || !fi)
		return FALSE;

	if (fi->fi_Index) {
		FreePooled(pool, fi->fi_Index, fi->fi_Size * sizeof(ULONG));
		fi->fi_Index = NULL;
		fi->fi_Size = 0;
	}

	calcpage = db->db_Page;  tf_col = tf_row = 0;
	size = db->db_TablePos.tp_Height + 1;

	if ((dest = AllocPooled(pool, size * sizeof(ULONG))) != 0) {
		for (i = j = 0; i < size; i++) {
			SetDBCurrent(db, DBC_ABS, i);
			if (TreeValue(fi->fi_Root))      /* criteria matches */
				dest[j++] = db->db_Current;    /* adopt order of the index */
		}

		// create an array of the correct size and copy the big one into that
		if ((fi->fi_Size = j) && (fi->fi_Index = AllocPooled(pool, j * sizeof(ULONG))))
			CopyMem(dest, fi->fi_Index, j * sizeof(ULONG));

		FreePooled(pool, dest, size * sizeof(ULONG));
	}

	return TRUE;
}


void
MakeSearchFilter(struct Database *db, struct Mask *ma)
{
	struct tableField *tf;
	struct MaskField *mf;
	struct Filter *fi;
	struct List list;
	struct MSF {struct Node msf_Node; struct MinList msf_List;} *msf;
	struct Node *ln;
	long   count = 0;
	STRPTR t;

	if (!db || !ma)
		return;

	ma->ma_Node.ln_Type = 0;
	MyNewList(&list);

	// collect all mask field entries

	foreach (&ma->ma_Fields, mf) {
		t = mf->mf_Node.ln_Name;
		if ((db = GetMaskDatabase(ma->ma_Page->pg_Mappe, ma->ma_Node.ln_Name, &t, NULL)) != 0) {
			if ((tf = GetTableField(ma->ma_Page, mf->mf_Col, mf->mf_Row)) && tf->tf_Text) {
				count++;
				for (msf = (APTR)list.lh_Head; msf->msf_Node.ln_Succ && msf->msf_Node.ln_Name != (APTR)db; msf = (APTR)msf->msf_Node.ln_Succ);
				if (!msf->msf_Node.ln_Succ) {
					if ((msf = AllocPooled(pool, sizeof(struct MSF))) != 0) {
						MyNewList(&msf->msf_List);
						msf->msf_Node.ln_Name = (APTR)db;
						MyAddTail(&list, msf);
					}
				}
				if (msf && (ln = AllocPooled(pool, sizeof(struct Node) + sizeof(APTR)))) {
					ln->ln_Name = AllocString(t);
					*(STRPTR *)(ln + 1) = (APTR)tf->tf_Text;
					MyAddTail(&msf->msf_List, ln);
				}
			}
		}
	}

	if (IsListEmpty(&list)) {
		if (count)
			ErrorRequest(GetString(&gLocaleInfo, MSG_CREATE_FILTER_ERR));
		return;
	}

	// create search filter

	if ((t = AllocPooled(pool, 2048)) == NULL) {
		ErrorRequest(GetString(&gLocaleInfo, MSG_CREATE_FILTER_ERR));
		return;
	}

	while ((msf = (struct MSF *)MyRemHead(&list)) != 0) {
		db = (APTR)msf->msf_Node.ln_Name;
		memset(t, 0, 2048);

		if ((fi = AllocPooled(pool, sizeof(struct Filter))) != 0) {
			fi->fi_Type = FIT_SEARCH;
			while ((ln = MyRemHead(&msf->msf_List)) != 0) {
				if (t[0])
					strcat(t, " && ");

				strcat(t, "pat(\"");			/* function to use (pat == pattern) */
				strcat(t, *(STRPTR *)(ln + 1));	/* contents of the cell (pattern) */
				strcat(t, "#?\";");
				strcat(t, db->db_Node.ln_Name);	/* name of the database */
				t[strlen(t)] = '.';
				strcat(t, ln->ln_Name);			/* name of the field */
				t[strlen(t)] = ')';
				FreeString(ln->ln_Name);
			}

			if ((fi->fi_Filter = AllocString(t)) != 0)
				fi->fi_Root = CreateTree(db->db_Page, t);

			MakeFilter(db, fi);
			db->db_Filter = fi;
			UpdateDBCurrent(db, 0);   /* first matching entry */
		}
	}
	FreePooled(pool, t, 2048);
}


void
SetFilter(struct Database *db, struct Filter *fi)
{
	struct Filter *sfi;

	if (!db)
		return;

	if (!fi || fi->fi_Type != FIT_SEARCH)
	{
		foreach (&db->db_Filters, sfi)    /* alten aktuellen entfernen */
			sfi->fi_Node.ln_Type = 0;

		db->db_Filter = NULL;
	}

	// ToDo: here seems to be a memory leak for FIT_SEARCH filters...

	if (MakeFilter(db, fi))    /* Filter erstellt */
		db->db_Filter = fi;
	else if (!fi && db->db_Filter && db->db_Filter->fi_Type != FIT_SEARCH)  /* Filter auflösen */
		db->db_Filter = NULL;
	else
	{
		foreach (&db->db_Filters, sfi)
		{
			if (sfi->fi_Node.ln_Type)
			{
				db->db_Filter = sfi;
				break;
			}
		}
	}
}


void
UpdateDBCurrent(struct Database *db, ULONG current)
{
	struct Index *in;

	if ((in = (struct Index *)db->db_Filter) || (in = db->db_Index))
	{
		if (in->in_Index)
		{
			long i;

			for(i = 0;i < in->in_Size;i++)     /* aktuellen Eintrag aus Index heraussuchen */
				if (in->in_Index[i] == current)
					break;
			if (i == in->in_Size)              /* der Eintrag wurde nicht gefunden */
			{
				db->db_Current = in->in_Index[0];
				db->db_IndexPos = 0;
			}
			else
				db->db_Current = current,  db->db_IndexPos = i;
		}
		else
			db->db_Current = 0,  db->db_IndexPos = -1;
	}
	else
		db->db_Current = db->db_IndexPos = current;
}


void
UpdateIndices(struct Database *db)
{
	struct Filter *fi;
	struct Index *in;
	ULONG  current;

	if (!db)
		return;

	current = db->db_Current;

	if ((in = db->db_Index) != 0)
		MakeIndex(db,in);

	if ((fi = db->db_Filter) && fi->fi_Type != FIT_SEARCH)
	{
		db->db_Filter = NULL;   /* fool SetDBCurrent() */
		MakeFilter(db,(struct Filter *)fi);
		db->db_Filter = fi;
	}
	UpdateDBCurrent(db, current);
}


void
FreeFilter(struct Filter *fi)
{
	if (!fi)
		return;

	if (fi->fi_Index)
		FreePooled(pool, fi->fi_Index, fi->fi_Size * sizeof(ULONG));

	FreeString(fi->fi_Node.ln_Name);
	FreeString(fi->fi_Filter);
	DeleteTree(fi->fi_Root);

	FreePooled(pool, fi, sizeof(struct Filter));
}


void
FreeFilters(struct MinList *mlh)
{
	struct Filter *fi;

	while ((fi = (APTR)MyRemHead(mlh)) != 0)
		FreeFilter(fi);
}


void RefreshMaskFields(struct Mappe *mp,BOOL refresh)
{
	struct tableField *tf,*dbtf;
	struct Database *db;
	struct Field *fi;
	struct Mask *ma;
	struct MaskField *mf;
	STRPTR t;
	BYTE   leer;
	long   i;

	if (!(mp->mp_Flags & MPF_SCRIPTS))
		return;
	for(ma = (APTR)mp->mp_Masks.mlh_Head;ma->ma_Node.ln_Succ;ma = (APTR)ma->ma_Node.ln_Succ)
	{
		if (ma->ma_Node.ln_Type)   /* search mode is active */
			continue;
		leer = FALSE;
		if ((db = (APTR)MyFindName(&mp->mp_Databases, ma->ma_Node.ln_Name)) != 0)
			leer = IsDBEmpty(db);

		for(mf = (APTR)ma->ma_Fields.mlh_Head;mf->mf_Node.ln_Succ;mf = (APTR)mf->mf_Node.ln_Succ)
		{
			t = mf->mf_Node.ln_Name;
			if (leer || (db = GetMaskDatabase(mp,ma->ma_Node.ln_Name,&t,NULL)))
			{
				if ((tf = AllocTableField(ma->ma_Page, mf->mf_Col, mf->mf_Row)) != 0)
				{
					if (!leer)
						for(i = 0,fi = (APTR)db->db_Fields.mlh_Head;fi->fi_Node.ln_Succ && zstrcmp(fi->fi_Node.ln_Name,t);fi = (APTR)fi->fi_Node.ln_Succ,i++);
					if (!leer && fi->fi_Node.ln_Succ)
					{
						dbtf = GetTableField(db->db_Page,db->db_TablePos.tp_Col+i,db->db_TablePos.tp_Row+db->db_Current);
						if (ma->ma_Page->pg_Gad.DispPos <= PGS_FRAME || ma->ma_Page->pg_Gad.tf != tf)
						{
							t = AllocString(tf->tf_Original);
							if (dbtf)
								SetTFText(ma->ma_Page,tf,db->db_Page->pg_Gad.tf == dbtf && db->db_Page->pg_Gad.DispPos > PGS_FRAME ? dbtf->tf_Text : dbtf->tf_Original);
							else
								SetTFText(ma->ma_Page,tf,NULL);
							if (refresh && strcmp(t ? t : (STRPTR)"",tf->tf_Original ? tf->tf_Original : (STRPTR)""))
								DrawTableField(ma->ma_Page,tf);
							FreeString(t);
						}
					}
					else if (strcmp(tf->tf_Text ? tf->tf_Text : (STRPTR)"","-"))
					{
						SetTFText(ma->ma_Page,tf,"-");
						DrawTableField(ma->ma_Page,tf);
					}
				}
			}
		}
	}
}


void UpdateMaskCell(struct Mappe *mp,struct Page *page,struct tableField *tf,struct UndoNode *un)
{
	struct tableField *dbtf;
	struct Database *db;
	struct Field *fi;
	struct MaskField *mf = NULL;
	struct Mask *ma;
	STRPTR t;

	foreach(&mp->mp_Masks,ma)
	{
		if (ma->ma_Page == page)
		{
			foreach(&ma->ma_Fields,mf)
			{
				if (tf->tf_Col == mf->mf_Col && tf->tf_Row == mf->mf_Row)
					break;
			}
			if (mf->mf_Node.ln_Succ)
				break;
		}
	}
	if (!mf)
		return;
	if (mf->mf_Node.ln_Succ)
	{
		if (!ma->ma_Node.ln_Type)  /* search mode is not active */
		{
			t = mf->mf_Node.ln_Name;
			if ((db = GetMaskDatabase(mp, ma->ma_Node.ln_Name, &t, NULL)) != 0)
			{
				if ((fi = (APTR)MyFindName(&db->db_Fields, t)) != 0)
				{
					if ((dbtf = AllocTableField(db->db_Page, db->db_TablePos.tp_Col + FindListEntry(&db->db_Fields, (struct MinNode *)fi), db->db_TablePos.tp_Row+db->db_Current)) != 0)
					{
						SetTFText(db->db_Page,dbtf,tf->tf_Original);
						if (un)
							un->un_Node.ln_Type |= UNDO_MASK;
						DrawTableField(db->db_Page,dbtf);
					}
				}
				else
					ErrorRequest(GetString(&gLocaleInfo, MSG_FIELD_NOT_FOUND_IN_DBASE_ERR),t,db->db_Node.ln_Name);
			}
		}
#if 0
	 /* if (!mf->mf_Node.ln_Succ->ln_Succ)  last mask field */
		{
			db = (struct Database *)MyFindName(&mp->mp_Databases,ma->ma_Node.ln_Name);
			if (ma->ma_Node.ln_Type)
				MakeSearchFilter(db,ma);
			else
				UpdateIndices(db);
		}
#endif
	}
}


void SetDBCurrent(struct Database *db,UBYTE mode,long pos)
{
	struct Index *in;

	if (!db)
		return;
	if ((in = (struct Index *)db->db_Filter) || (in = db->db_Index)) {
		/* indexed/filtered */
		if (!in->in_Index) {
			db->db_IndexPos = -1;
			return;
		}
		if (mode == DBC_REL) {
			/* no check for DBC_ABS!! */
			long i;

			/* search current entry in the index */
			for (i = 0; i < in->in_Size; i++) {
				if (in->in_Index[i] == db->db_Current)
					break;
			}
			D({if (i != db->db_IndexPos)
				bug("setdbcurrent: verschieden!!\n");});
			pos += i;
		}
		if (pos < 0)
			pos = 0;
		if (pos >= in->in_Size)  /* size depends on the index, not the database */
			pos = in->in_Size - 1;
		db->db_Current = in->in_Index[pos];
		db->db_IndexPos = pos;   /* store position in the index */
	} else {
		/* not filtered or indexed */
		if (mode == DBC_REL)
			pos += db->db_Current;
		if (pos < 0)
			pos = 0;
		if (pos > db->db_TablePos.tp_Height)
			pos = db->db_TablePos.tp_Height;
		db->db_Current = pos;
		db->db_IndexPos = pos;
	}
}


/**************************************************************************\
* gadget creation routines                                                 *
\**************************************************************************/

extern const char *gDatabaseFieldLabels[];


void ASM
CreateDatabaseGadgets(REG(a0, struct winData *wd))
{
	long   w;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 9; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 10; //Set Gadgetnumber 
#endif

	w = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+120+lborder;
	gWidth = w + 48 + TLn(GetString(&gLocaleInfo, MSG_TYPE_LABEL)) + TLn(GetString(&gLocaleInfo, MSG_REFERENCE_DBASE_GAD)) + rborder;
	gHeight = barheight+fontheight*12+49+bborder;

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = barheight+fontheight+9;
	ngad.ng_Width = w-6-lborder;
	ngad.ng_Height = fontheight*6+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DATABASE_GAD);
	ngad.ng_Flags = PLACETEXT_ABOVE;        // 1
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,wd->wd_ExtData[2],GTLV_ShowSelected,NULL,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+57;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 2
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+57;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;                     // 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_NAME_GAD));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = w-6-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_GAD);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;                     // 5
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_REFERENCE_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = w-6-ngad.ng_LeftEdge-boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_REFERENCE_LABEL);
	ngad.ng_GadgetID++;                     // 6
	gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GTTX_Text,((struct Page *)wd->wd_Data)->pg_Node.ln_Name,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;                     // 7
	gad = CreatePopGadget(wd,gad,FALSE);

#ifdef __amigaos4__
	ngad.ng_LeftEdge = lborder;
#else
	ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_REFERENCE_LABEL));
#endif
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = w-22-ngad.ng_LeftEdge-TLn(GetString(&gLocaleInfo, MSG_FROM_BLOCK_GAD));
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;                     // 8
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FROM_BLOCK_GAD);
	ngad.ng_Width = TLn(ngad.ng_GadgetText)+16;
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 9
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+8;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 10
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;                     // 11
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = w;
	ngad.ng_TopEdge = barheight+fontheight+9;
	ngad.ng_Width = gWidth-w-rborder;
	ngad.ng_Height = fontheight*8+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FIELDS_GAD);
	ngad.ng_Flags = PLACETEXT_ABOVE;
	ngad.ng_GadgetID++;                     // 12
	gad = wd->wd_ExtData[1] = CreateGadget(LISTVIEW_KIND,gad,&ngad,GA_Disabled,TRUE,GTLV_Labels,NULL,GTLV_ShowSelected,NULL,TAG_END);

	ngad.ng_LeftEdge = w+8+TLn(GetString(&gLocaleInfo, MSG_TYPE_LABEL));
	ngad.ng_TopEdge += ngad.ng_Height+4;
	ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TYPE_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;                     // 13
	gad = CreateGadget(CYCLE_KIND, gad, &ngad, GTCY_Labels, &gDatabaseFieldLabels, GA_Disabled, TRUE, GT_Underscore, '_', TAG_END);

	ngad.ng_TopEdge += fontheight+4;
	ngad.ng_Width -= boxwidth;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;                     // 14
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;                     // 15
	gad = CreatePopGadget(wd,gad,FALSE);
}


void ASM
CreateMaskGadgets(REG(a0, struct winData *wd))
{
	struct Database *db = wd->wd_ExtData[0];
	long   w,h;

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 10; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 11; //Set Gadgetnumber 
#endif
	w = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+120+lborder;
	gWidth = w+48+TLn(GetString(&gLocaleInfo, MSG_TYPE_LABEL))+TLn(GetString(&gLocaleInfo, MSG_REFERENCE_DBASE_GAD))+rborder;
	gHeight = barheight+fontheight*13+54+bborder;

	ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_DATABASE_LABEL));
	ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge-boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DATABASE_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;         // 1
	gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GTTX_Text,db ? db->db_Node.ln_Name : "-",GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;                     // 2
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_TopEdge += 2*fontheight+8;
	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_TO_EXISTING_PAGE_LABEL);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;                     // 3
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,FALSE,GA_Disabled,!db,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width+TLn(GetString(&gLocaleInfo, MSG_TO_EXISTING_PAGE_LABEL))+16;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge-boxwidth;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;                     // 4
	gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;                     // 5
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_AUTO_GENERATE_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;                     // 6
	gad = CreateGadget(CHECKBOX_KIND,gad,&ngad,GTCB_Scaled,TRUE,GTCB_Checked,FALSE,GA_Disabled,TRUE /*!db*/,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 8+lborder;
	h = (ngad.ng_TopEdge += 2*fontheight+13);
	ngad.ng_Width = w-14-lborder;
	ngad.ng_Height = fontheight*6+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;                     // 7
	gad = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,NULL,GTLV_ShowSelected,NULL,GA_Disabled,!db,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+53;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 8
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,!db,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+53;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;                     // 9
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+12;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 10
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;                     // 11
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = w+8+TLn(GetString(&gLocaleInfo, MSG_FIELD_LABEL));
	ngad.ng_TopEdge = h;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FIELD_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;                     // 12
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = w+8+TLn(GetString(&gLocaleInfo, MSG_CELL_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CELL_LABEL);
	ngad.ng_GadgetID++;                     // 13
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = w;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-8-rborder-w;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_GENERATE_FIELDS_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 14
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,!db,GT_Underscore,'_',TAG_END);
}


void ASM
CreateIndexGadgets(REG(a0, struct winData *wd))
{
	struct Database *db = wd->wd_ExtData[0];

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 8; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 9; //Set Gadgetnumber 
#endif

	gWidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+156+lborder+rborder;
	gHeight = barheight+fontheight*13+41+bborder;

	if (!wd->wd_ExtData[6])         /***** predefined database */
	{
		ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_DATABASE_LABEL));
		ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge-boxwidth;
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DATABASE_LABEL);
		ngad.ng_Flags = PLACETEXT_LEFT;         // 1
		gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GTTX_Text,db ? db->db_Node.ln_Name : "-",GT_Underscore,'_',TAG_END);

		ngad.ng_LeftEdge += ngad.ng_Width;
		ngad.ng_GadgetID++;                     // 2
		gad = CreatePopGadget(wd,gad,FALSE);
	}
	else
		ngad.ng_TopEdge -= fontheight+6, gHeight -= fontheight+6;

	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_TopEdge += 2*fontheight+8;
	ngad.ng_Width = gWidth-lborder-rborder-16;
	ngad.ng_Height = fontheight*8+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;                     // 3
	gad = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,db ? &db->db_Indices : NULL,GTLV_ShowSelected,NULL,GTLV_CallBack,&selectHook,GA_Disabled,!db,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+70;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,!db,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+70;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;                     // 5
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_INDEX_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-8-rborder-boxwidth-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_INDEX_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;                     // 6
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;                     // 7
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+12;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 9
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;                     // 10
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


void ASM
CreateFilterGadgets(REG(a0, struct winData *wd))
{
	struct Database *db = wd->wd_ExtData[0];

#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_OK_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 9; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 10; //Set Gadgetnumber 
#endif

	gWidth = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+156+lborder+rborder;
	gHeight = barheight+fontheight*14+48+bborder;

	if (!wd->wd_ExtData[6])         /***** predefined database */
	{
		ngad.ng_LeftEdge = 8+lborder+TLn(GetString(&gLocaleInfo, MSG_DATABASE_LABEL));
		ngad.ng_Width = gWidth-rborder-ngad.ng_LeftEdge-boxwidth;
		ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DATABASE_LABEL);
		ngad.ng_Flags = PLACETEXT_LEFT;         // 1
		gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GTTX_Text,db ? db->db_Node.ln_Name : "-",GT_Underscore,'_',TAG_END);

		ngad.ng_LeftEdge += ngad.ng_Width;
		ngad.ng_GadgetID++;                     // 2
		gad = CreatePopGadget(wd,gad,FALSE);
	}
	else
		ngad.ng_TopEdge -= fontheight+6, gHeight -= fontheight+6;

	ngad.ng_LeftEdge = 8+lborder;
	ngad.ng_TopEdge += 2*fontheight+8;
	ngad.ng_Width = gWidth-16-lborder-rborder;
	ngad.ng_Height = fontheight*8+4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID = 3;                   // 3
	gad = CreateGadget(LISTVIEW_KIND,gad,&ngad,GTLV_Labels,db ? &db->db_Filters : NULL,GTLV_ShowSelected,NULL,GA_Disabled,!db,GTLV_CallBack,&selectHook,TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_NEW_GAD))+70;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NEW_GAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 4
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',GA_Disabled,!db,TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD))+70;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_DELETE_GAD);
	ngad.ng_GadgetID++;                     // 5
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_NAME_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-8-rborder-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NAME_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;                     // 6
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = 16+lborder+TLn(GetString(&gLocaleInfo, MSG_INDEX_LABEL));
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = gWidth-8-rborder-boxwidth-ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FILTER_LABEL);
	ngad.ng_GadgetID++;                     // 7
	gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Disabled,TRUE,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;                     // 8
	gad = CreatePopGadget(wd,gad,FALSE);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+12;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_UGAD))+16;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;                     // 9
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = gWidth - rborder - ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;                     // 10
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
}


/****************************************************************************
 ** window handling routines                                               **
 ****************************************************************************/


void ASM
CloseDatabaseWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	struct Database *db;
	struct Field *fi;

	if (clean)
	{
		if (wd->wd_ExtData[2])
		{
			while((db = (struct Database *)MyRemHead(wd->wd_ExtData[2])) != 0)
			{
				while((fi = (struct Field *)MyRemHead(&db->db_Fields)) != 0)
				{
					FreeString(fi->fi_Node.ln_Name);
					FreeString(fi->fi_Special);
				}
				FreeString(db->db_Node.ln_Name);
				FreeString(db->db_Content);
				FreePooled(pool,db,sizeof(struct Database));
			}
			FreePooled(pool,wd->wd_ExtData[2],sizeof(struct List));
		}
	}
}


void
UpdateDatabaseGadgets(struct Mappe *mp,struct Database *db,struct Field *fi)
{
	wd->wd_ExtData[3] = db;  wd->wd_ExtData[4] = fi;
	if (db)
	{
		GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],GTLV_Selected,FindListEntry(wd->wd_ExtData[2],(struct MinNode *)db),TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GA_Disabled,FALSE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GA_Disabled,FALSE,GTST_String,db->db_Node.ln_Name,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTTX_Text,db->db_Page->pg_Node.ln_Name,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,FALSE,GTST_String,db->db_Content,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,FALSE,TAG_END);
		GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,&db->db_Fields,GTLV_Selected,fi ? FindListEntry(&db->db_Fields,(struct MinNode *)fi) : ~0L,GA_Disabled,FALSE,TAG_END);
	}
	else
	{
		GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,wd->wd_ExtData[2],GTLV_Selected,~0L,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GA_Disabled,TRUE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GA_Disabled,TRUE,GTST_String,NULL,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GTTX_Text,NULL,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,TRUE,GTST_String,NULL,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,TRUE,TAG_END);
		GT_SetGadgetAttrs(wd->wd_ExtData[1],win,NULL,GTLV_Labels,NULL,GA_Disabled,TRUE,TAG_END);
		fi = NULL;
	}
	if (fi)
	{
		GT_SetGadgetAttrs(GadgetAddress(win,12),win,NULL,GTCY_Active,fi->fi_Node.ln_Type,GA_Disabled,FALSE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GTST_String,fi->fi_Special,GA_Disabled,!fi->fi_Node.ln_Type || fi->fi_Node.ln_Type == FIT_COUNTER,TAG_END);
	}
	else
	{
		GT_SetGadgetAttrs(GadgetAddress(win,12),win,NULL,GTCY_Active,0,GA_Disabled,TRUE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GTST_String,NULL,GA_Disabled,TRUE,TAG_END);
	}
}


void ASM
HandleDatabaseIDCMP(REG(a0, struct TagItem *tag))
{
	struct Database *db;
	struct Field *fi;
	struct Mappe *mp;
	struct Page *page;
	struct List *list;
	char   t[42];
	long   i;

	db = (struct Database *)wd->wd_ExtData[3];
	fi = (struct Field *)wd->wd_ExtData[4];
	mp = ((struct Page *)wd->wd_Data)->pg_Mappe;

	switch(imsg.Class)
	{
		case IDCMP_GADGETDOWN:
			switch((gad = imsg.IAddress)->GadgetID)
			{
				case 6:  // Seite
					if (!db)
						break;
					i = PopUpList(win,gad = GadgetAddress(win,5),&mp->mp_Pages,TAG_END);
					if (i != ~0L)
					{
						for(page = (struct Page *)mp->mp_Pages.mlh_Head;i && page->pg_Node.ln_Succ;page = (APTR)page->pg_Node.ln_Succ,i--);
						db->db_Page = page;
						FreeString(db->db_Content);  db->db_Content = NULL;  db->db_Root = NULL;
						UpdateDatabaseGadgets(mp,db,fi);
					}
					break;
				case 14:  // fi_Special-Popper
					if (!db || !fi || !fi->fi_Node.ln_Type || fi->fi_Node.ln_Type == FIT_COUNTER)
						break;
					list = fi->fi_Node.ln_Type == FIT_REFERENCE ? wd->wd_ExtData[2] : &mp->mp_Prefs.pr_Names;
					i = PopUpList(win,gad = GadgetAddress(win,13), (struct MinList *)list,TAG_END);
					if (i != ~0L)
					{
						for(page = (struct Page *)list->lh_Head;i && page->pg_Node.ln_Succ;page = (APTR)page->pg_Node.ln_Succ,i--);
						fi->fi_Special = AllocString(page->pg_Node.ln_Name);
						UpdateDatabaseGadgets(mp,db,fi);
					}
					break;
			}
			break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
			{
				case 1:  // Database-Liste
					if ((WORD)imsg.Code < 0)
						break;
					for(i = 0,db = (APTR)((struct List *)wd->wd_ExtData[2])->lh_Head;i < imsg.Code;db = (APTR)db->db_Node.ln_Succ,i++);
					UpdateDatabaseGadgets(mp,db,NULL);
					break;
				case 2:  // Neu
					if ((db = AllocPooled(pool, sizeof(struct Database))) != 0)
					{
						GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
						db->db_Node.ln_Name = AllocString(GetString(&gLocaleInfo, MSG_NEW_DBASE_NAME));
						db->db_Node.ln_Type = NMT_DATABASE;
						if (rxpage && rxpage->pg_Mappe == mp)
							db->db_Page = rxpage;
						else
							db->db_Page = wd->wd_Data;
						MyNewList(&db->db_Fields);
						MyNewList(&db->db_Indices);
#ifdef __amigaos4__
						MyNewList(&db->db_Filters);
#endif
						MyAddTail(wd->wd_ExtData[2], db);
					}
					UpdateDatabaseGadgets(mp,db,NULL);
					ActivateGadget(GadgetAddress(win,4),win,NULL);
					break;
				case 3:  // Löschen
					if (db)
					{
						GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
						MyRemove(db);
						FreeString(db->db_Node.ln_Name);
						FreeString(db->db_Content);
						while((fi = (struct Field *)MyRemHead(&db->db_Fields)) != 0)
						{
							FreeString(fi->fi_Node.ln_Name);
							FreeString(fi->fi_Special);
							FreePooled(pool,fi,sizeof(struct Field));
						}
						FreePooled(pool,db,sizeof(struct Database));
					}
					UpdateDatabaseGadgets(mp,NULL,NULL);
					break;
				case 4:  // Name
					if (db)
					{
						GT_SetGadgetAttrs(wd->wd_ExtData[0],win,NULL,GTLV_Labels,~0L,TAG_END);
						FreeString(db->db_Node.ln_Name);
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&db->db_Node.ln_Name,TAG_END);
						db->db_Node.ln_Name = AllocString(db->db_Node.ln_Name);
						sortList(wd->wd_ExtData[2]);
						if (!IsValidName(wd->wd_ExtData[2],db->db_Node.ln_Name))
							i = 4;
						else
							i = 7;
						ActivateGadget(GadgetAddress(win,i),win,NULL);
					}
					UpdateDatabaseGadgets(mp,db,fi);
					break;
				case 7:  // Bezug
					if (db)
					{
						FreeString(db->db_Content);
#ifdef __amigaos4__
					    STRPTR tp;
					    
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String, &tp,TAG_END);
						db->db_Content = AllocString(tp);
						db->db_Root = CreateTree(db->db_Page, tp);
#else
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&db->db_Content,TAG_END);
						db->db_Content = AllocString(db->db_Content);
						db->db_Root = CreateTree(db->db_Page,db->db_Content);
#endif
#ifdef __amigaos4__
						if(db->db_Root)
#endif
							CreateFields(db);
					}
					UpdateDatabaseGadgets(mp,db, NULL);
					break;
				case 8:  // Block
					if (db && (page = db->db_Page))
					{
						if (page->pg_MarkCol != -1)
						{
							FreeString(db->db_Content);
							TablePos2String(page,(struct tablePos *)&page->pg_MarkCol,t);
						}
						else if (page->pg_Gad.DispPos > PGS_NONE)
						{
#ifdef __amigaos4__
							strcpy(t,Coord2String(page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row));
							if(!strchr(t, ':'))   //is there a : as a marker for a block, when not show a error and end
							{
								ErrorRequest(GetString(&gLocaleInfo, MSG_NO_BLOCK_ERR));
								break;
							}
							FreeString(db->db_Content);
#else
							FreeString(db->db_Content);
							strcpy(t,Coord2String(page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row));
#endif
						}
						else
						{
							ErrorRequest(GetString(&gLocaleInfo, MSG_NO_BLOCK_ERR));
							break;
						}
						db->db_Content = AllocString(t);
						db->db_Root = CreateTree(db->db_Page,t);
#ifdef __amigaos4__
						if(db->db_Root)
#endif
							CreateFields(db);
					}
					UpdateDatabaseGadgets(mp,db,NULL);
					break;
				case 9:  // Ok
#ifdef __amigaos4__
					moveList(wd->wd_ExtData[2],&mp->mp_Databases);
#else
					swapLists(&mp->mp_Databases, wd->wd_ExtData[2]);
#endif
				case 10:  // Abbrechen
					CloseAppWindow(win,TRUE);

					if (i == 9)  // Ok
					{
						MakeNamesLinks(mp);
						RecalcTableFields((struct Page *)mp->mp_Pages.mlh_Head);
					}
					break;
				case 11:  // Felder-Liste
					if ((WORD)imsg.Code < 0)
						break;
					if (db)
					{
						for(i = 0,fi = (APTR)db->db_Fields.mlh_Head;i < imsg.Code;fi = (APTR)fi->fi_Node.ln_Succ,i++);
						UpdateDatabaseGadgets(mp,db,fi);
					}
					break;
				case 12:  // Feld-Typ
					if (fi)
						fi->fi_Node.ln_Type = imsg.Code;
					UpdateDatabaseGadgets(mp,db,fi);
					break;
				case 13:  // Feld-Special
					if (fi)
					{
						FreeString(fi->fi_Special);
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&fi->fi_Special,TAG_END);
						fi->fi_Special = AllocString(fi->fi_Special);
					}
					break;
			}
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
			break;
	}
}


void
FreeMaskFields(struct Mask *ma)
{
	struct MaskField *mf;

	while((mf = (APTR)MyRemHead(&ma->ma_Fields)) != 0)
	{
		FreeString(mf->mf_Node.ln_Name);
		FreePooled(pool,mf,sizeof(struct MaskField));
	}
}


void
AddMaskFields(struct Mappe *mp,struct Mask *ma,struct Database *db,STRPTR pre)
{
	struct MaskField *mf;
	struct Field *fi;
	STRPTR newpre,name;

	if (db)
	{
		for(fi = (APTR)db->db_Fields.mlh_Head;fi->fi_Node.ln_Succ;fi = (APTR)fi->fi_Node.ln_Succ)
		{
			if (!(name = fi->fi_Node.ln_Name))
				name = "";
			if (fi->fi_Node.ln_Type == FIT_REFERENCE)
			{
				if ((newpre = AllocPooled(pool, (pre ? strlen(pre) : 0) + strlen(name) + 2)) != 0)
				{
					if (pre)
						strcpy(newpre,pre);
					strcat(newpre,name);
					newpre[strlen(newpre)] = '.';
				}
				AddMaskFields(mp, ma, (APTR)MyFindName(&mp->mp_Databases, fi->fi_Special), newpre);
				FreeString(newpre);
			}
			else if ((mf = AllocPooled(pool, sizeof(struct MaskField))) != 0)
			{
				if ((mf->mf_Node.ln_Name = AllocPooled(pool, strlen(name) + (pre ? strlen(pre) : 0) + 1)) != 0)
				{
					if (pre)
						strcpy(mf->mf_Node.ln_Name,pre);
					strcat(mf->mf_Node.ln_Name,name);
				}
				MyAddTail(&ma->ma_Fields, mf);
			}
		}
	}
}


static void
GenerateMaskFields(struct Mappe *mp,struct Mask *ma)
{
	struct Database *db;

	if ((db = (APTR)MyFindName(&mp->mp_Databases, ma->ma_Node.ln_Name)) != 0)
	{
		FreeMaskFields(ma);
		AddMaskFields(mp,ma,db,NULL);
	}
}


void
FreeMask(struct Mask *ma)
{
	FreeMaskFields(ma);
	FreeString(ma->ma_Node.ln_Name);
	FreePooled(pool,ma,sizeof(struct Mask));
}


void
FreeMasks(struct MinList *list)
{
	struct Mask *ma;

	while ((ma = (APTR)MyRemHead(list)) != 0)
		FreeMask(ma);
}


void ASM
CloseMaskWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;

	if (clean)
	{
		FreeMasks(wd->wd_ExtData[1]);
		FreePooled(pool,wd->wd_ExtData[1],sizeof(struct MinList));
	}
}


void
UpdateMaskGadgets(struct Mask *ma,struct MaskField *mf)
{
	wd->wd_ExtData[0] = ma;  wd->wd_ExtData[2] = mf;
	if (ma)
	{
		GT_SetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTTX_Text,ma->ma_Node.ln_Name,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTCB_Checked,ma->ma_Page,GA_Disabled,FALSE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,4),win,NULL,GTTX_Text,ma->ma_Page ? ma->ma_Page->pg_Node.ln_Name : NULL,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTCB_Checked,ma->ma_Node.ln_Type,GA_Disabled,FALSE,TAG_END);
		if (ma->ma_Node.ln_Type)
			wd->wd_ExtData[2] = mf = NULL;
		GT_SetGadgetAttrs(gad = GadgetAddress(win,7),win,NULL,GA_Disabled,ma->ma_Node.ln_Type,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,8),win,NULL,GA_Disabled,ma->ma_Node.ln_Type,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,14),win,NULL,GA_Disabled,ma->ma_Node.ln_Type,TAG_END);
		if (!mf)
		{
			GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&ma->ma_Fields,GTLV_Selected,~0L,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,9),win,NULL,GA_Disabled,TRUE,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,12),win,NULL,GA_Disabled,TRUE,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GA_Disabled,TRUE,TAG_END);
		}
		else if (mf != (APTR)~0L)
		{
			GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&ma->ma_Fields,GTLV_Selected,FindListEntry((struct MinList *)&ma->ma_Fields,(struct MinNode *)mf),TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,9),win,NULL,GA_Disabled,FALSE,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,12),win,NULL,GTST_String,mf->mf_Node.ln_Name,GA_Disabled,FALSE,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GTST_String,mf->mf_Col && mf->mf_Row ? Coord2String(mf->mf_Col,mf->mf_Row) : NULL,GA_Disabled,FALSE,TAG_END);
		}
	}
}


void ASM
HandleMaskIDCMP(REG(a0, struct TagItem *tag))
{
	struct Mask *ma = wd->wd_ExtData[0];
	struct MaskField *mf = wd->wd_ExtData[2];
	struct Page *page;
	long   i;
	static uint8 init = FALSE;

#ifdef __amigaos4__
	if(!init)  //Fast solution to set Mask-list
	{
		ma = (struct Mask *)((struct List *)wd->wd_ExtData[1])->lh_Head;
		if(ma)
			UpdateMaskGadgets(ma,NULL);
		init = TRUE;
	}
#endif
	switch (imsg.Class) {
		case IDCMP_GADGETDOWN:
			if (!ma)
				break;
			switch ((gad = imsg.IAddress)->GadgetID) {
				case 2:    // Mask-PopUp
					i = PopUpList(win,gad = GadgetAddress(win,1),wd->wd_ExtData[1],TAG_END);
					if (i != ~0L) {
						for(ma = (struct Mask *)((struct List *)wd->wd_ExtData[1])->lh_Head;i && ma->ma_Node.ln_Succ;ma = (APTR)ma->ma_Node.ln_Succ,i--);
						UpdateMaskGadgets(ma,NULL);
					}
					break;
				case 5:    // Page-PopUp
					i = PopUpList(win,gad = GadgetAddress(win,4),&((struct Mappe *)wd->wd_Data)->mp_Pages,TAG_END);
					if (i != ~0L) {
						for(page = (struct Page *)((struct Mappe *)wd->wd_Data)->mp_Pages.mlh_Head;i && page->pg_Node.ln_Succ;page = (APTR)page->pg_Node.ln_Succ,i--);
						ma->ma_Page = page;
						UpdateMaskGadgets(ma,(APTR)~0L);
					}
					break;
			}
			break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
			{
    			case 3:    // on this page?
					if (ma) {
						if (!imsg.Code)
							ma->ma_Page = NULL;
						else
							ma->ma_Page = (struct Page *)((struct Mappe *)wd->wd_Data)->mp_Pages.mlh_Head;
						UpdateMaskGadgets(ma,(APTR)~0L);
					}
					break;
				case 6:    // create automatically?
					if (ma) {
						ma->ma_Node.ln_Type = (BYTE)imsg.Code;
						UpdateMaskGadgets(ma,mf);
					}
					break;
				case 7:    // field list
					if ((WORD)imsg.Code < 0)
						break;
					if (ma) {
						for(mf = (APTR)ma->ma_Fields.mlh_Head,i = imsg.Code;i && mf->mf_Node.ln_Succ;mf = (APTR)mf->mf_Node.ln_Succ,i--);
						UpdateMaskGadgets(ma,mf);
						ActivateGadget(GadgetAddress(win,12),win,NULL);
					}
					break;
				case 8:    // new field
					if (ma && (mf = AllocPooled(pool,sizeof(struct MaskField)))) {
						mf->mf_Node.ln_Name = AllocString(GetString(&gLocaleInfo, MSG_NEW_FIELD_NAME));
						GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTLV_Labels,~0L,TAG_END);
						MyAddTail(&ma->ma_Fields, mf);
					}
					UpdateMaskGadgets(ma,mf);
					if (mf)
						ActivateGadget(GadgetAddress(win,12),win,NULL);
					break;
				case 9:    // delete field
					if (ma && mf) {
						GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTLV_Labels,~0L,TAG_END);
						MyRemove(mf);
						FreeString(mf->mf_Node.ln_Name);
						FreePooled(pool,mf,sizeof(struct MaskField));
						UpdateMaskGadgets(ma,NULL);
					}
					break;
				case 12:   // field name
					if (ma && mf) {
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&i,TAG_END);
						FreeString(mf->mf_Node.ln_Name);
						mf->mf_Node.ln_Name = AllocString((STRPTR)i);
						UpdateMaskGadgets(ma,mf);
						ActivateGadget(GadgetAddress(win,13),win,NULL);
					}
					break;
				case 13:   // cell
					if (ma && mf) {
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&i,TAG_END);
						String2Coord((STRPTR)i,(long *)&mf->mf_Col,(long *)&mf->mf_Row);
						if (mf->mf_Node.ln_Succ->ln_Succ) {
							mf = (APTR)mf->mf_Node.ln_Succ;
							UpdateMaskGadgets(ma,mf);
							ActivateGadget(GadgetAddress(win,13),win,NULL);
						}
					}
					break;
				case 14:   // generate fields
					if (ma) {
						GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTLV_Labels,~0L,TAG_END);
						GenerateMaskFields(wd->wd_Data,ma);
						UpdateMaskGadgets(ma,NULL);
					}
					break;
				case 10:   // Ok
					FreeMasks(&((struct Mappe *)wd->wd_Data)->mp_Masks);
					while ((ma = (struct Mask *)MyRemHead(wd->wd_ExtData[1])) != 0) {
						if (ma->ma_Node.ln_Type) {
							// create automatically
							ma->ma_Node.ln_Type = 0;   /* proposed for search mode */
							/* to be implemented */
						} else if (IsListEmpty((struct List *)&ma->ma_Fields)) {
							FreeMask(ma);
						} else {
							if (!ma->ma_Page) {
								page = rxpage;
								ma->ma_Page = NewPage(wd->wd_Data);
								FreeString(ma->ma_Page->pg_Node.ln_Name);
								if ((ma->ma_Page->pg_Node.ln_Name = AllocPooled(pool, strlen(ma->ma_Node.ln_Name) + strlen(GetString(&gLocaleInfo, MSG_MASK_NAME)) + 1)) != 0) {
									strcpy(ma->ma_Page->pg_Node.ln_Name,ma->ma_Node.ln_Name);
									strcat(ma->ma_Page->pg_Node.ln_Name,GetString(&gLocaleInfo, MSG_MASK_NAME));
								}
								rxpage = page;
							}
							MyAddTail(&((struct Mappe *)wd->wd_Data)->mp_Masks, ma);
						}
					}
				case 11:   // Abbrechen
					CloseAppWindow(win,TRUE);
#ifdef __amigaos4__
					init = FALSE;
#endif
			}
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win,TRUE);
#ifdef __amigaos4__
			init = FALSE;
#endif
			break;
	}
}


void ASM
CloseIndexWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	struct Database *db;

	if (clean) {
		while ((db = (APTR)MyRemHead(wd->wd_ExtData[1])) != 0) {
			FreeIndex(&db->db_Indices);
			FreeString(db->db_Node.ln_Name);
			FreePooled(pool, db, sizeof(struct Database));
		}
		FreePooled(pool, wd->wd_ExtData[1], sizeof(struct MinList));
	}
}


void
UpdateIndexGadgets(struct Database *db, struct Index *in)
{
	wd->wd_ExtData[0] = db;
	wd->wd_ExtData[2] = in;
	if (db) {
		GT_SetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTTX_Text,db->db_Node.ln_Name,TAG_END);
		if (!in) {
			GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,&db->db_Indices,GTLV_Selected,~0L,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GA_Disabled,TRUE,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GA_Disabled,TRUE,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,TRUE,TAG_END);
		} else if (in != (APTR)~0L) {
			GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,&db->db_Indices,GTLV_Selected,FindListEntry((struct MinList *)&db->db_Indices, (struct MinNode *)in),TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GA_Disabled,FALSE,TAG_END);
			GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,in->in_Node.ln_Name,GA_Disabled,FALSE,TAG_END);
			// GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,FALSE,TAG_END);
		}
	}
}


void ASM
HandleIndexIDCMP(REG(a0, struct TagItem *tag))
{
	struct Database *db = wd->wd_ExtData[0];
	struct Index *in = wd->wd_ExtData[2];
	long   i;

	switch (imsg.Class) {
		case IDCMP_GADGETDOWN:
			if (!db)
				break;
			switch ((gad = imsg.IAddress)->GadgetID) {
				case 2:    // Databasepopper
					i = PopUpList(win,GadgetAddress(win,1),wd->wd_ExtData[1],TAG_END);
					if (i != ~0L) {
						for (db = (struct Database *)((struct List *)wd->wd_ExtData[1])->lh_Head;i && db->db_Node.ln_Succ;db = (APTR)db->db_Node.ln_Succ,i--);
						UpdateIndexGadgets(db, NULL);
					}
					break;
				case 7:    // fieldpopper
					i = PopUpList(win,GadgetAddress(win,6),&db->db_Fields,TAG_END);
					if (i != ~0L) {
						struct Field *fi;
						STRPTR t;

						for (fi = (APTR)db->db_Fields.mlh_Head;i && fi->fi_Node.ln_Succ;fi = (APTR)fi->fi_Node.ln_Succ,i--);
						GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,~0L,TAG_END);
						if ((t = AllocPooled(pool, 512)) != 0) {
							if (strcmp(in->in_Node.ln_Name,GetString(&gLocaleInfo, MSG_NEW_FIELD_NAME)))
								strcpy(t,in->in_Node.ln_Name);
							AddPart(t,fi->fi_Node.ln_Name,512);
							FreeString(in->in_Node.ln_Name);
							in->in_Node.ln_Name = AllocString(t);
							FreePooled(pool,t,512);
						}
						UpdateIndexGadgets(db, in);
					}
					break;
			}
			break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
			{
				case 3:    // index list
					if (db) {
						struct Index *oin = in;

						for (in = (APTR)db->db_Indices.mlh_Head,i = imsg.Code;i && in->in_Node.ln_Succ;in = (APTR)in->in_Node.ln_Succ,i--);
						if (oin == in) {
							/* toggle activation */
							if (!in->in_Node.ln_Type) {
								foreach(&db->db_Indices,oin)  /* remove old active */
									oin->in_Node.ln_Type = 0;
								in->in_Node.ln_Type = 1;
							} else
								in->in_Node.ln_Type = 0;
						}
						UpdateIndexGadgets(db,in);
						ActivateGadget(GadgetAddress(win,6),win,NULL);
					}
					break;
				case 4:    // new index
					if (db && (in = AllocPooled(pool,sizeof(struct Index)))) {
						in->in_Node.ln_Name = AllocString(GetString(&gLocaleInfo, MSG_NEW_INDEX_NAME));
						GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,~0L,TAG_END);
						MyAddTail(&db->db_Indices, in);
					}
					UpdateIndexGadgets(db,in);
					if (in)
						ActivateGadget(GadgetAddress(win,6),win,NULL);
					break;
				case 5:    // delete index
					if (db && in) {
						GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,~0L,TAG_END);
						MyRemove(in);
						FreeString(in->in_Node.ln_Name);
						FreePooled(pool,in,sizeof(struct Index));
						UpdateIndexGadgets(db,NULL);
					}
					break;
				case 6:    // index name
					if (db && in) {
						GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&i,TAG_END);
						FreeString(in->in_Node.ln_Name);
						in->in_Node.ln_Name = AllocString((STRPTR)i);
						UpdateIndexGadgets(db,in);
					}
					break;
				case 8:   // Ok
				{
					struct Database *cdb;
					struct Mappe *mp = wd->wd_Data;

					SetBusy(TRUE,BT_APPLICATION);
					foreach (wd->wd_ExtData[1], cdb) {
						if (!(db = (APTR)MyFindName(&mp->mp_Databases, cdb->db_Node.ln_Name))) {
							ErrorRequest(GetString(&gLocaleInfo, MSG_DATABASE_NOT_FOUND_ERR),cdb->db_Node.ln_Name);
							continue;
						}
						FreeIndex(&db->db_Indices);
						db->db_Index = NULL;
						while ((in = (APTR)MyRemHead(&cdb->db_Indices)) != 0) {
							if (in->in_Node.ln_Type) {
								/* active index */
								MakeIndex(db,in);            /* generate index data */
								db->db_Index = in;
							}
							MyAddTail(&db->db_Indices, in);
						}
					}
					RecalcTableFields((struct Page *)mp->mp_Pages.mlh_Head);
					SetBusy(FALSE,BT_APPLICATION);
				}
				case 9:   // cancel
					CloseAppWindow(win, TRUE);
			}
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
			break;
	}
}


void ASM
CloseFilterWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	struct Database *db;

	if (clean) {
		while ((db = (APTR)MyRemHead(wd->wd_ExtData[1])) != 0) {
			FreeFilters(&db->db_Filters);
			FreeString(db->db_Node.ln_Name);
			FreePooled(pool,db,sizeof(struct Database));
		}
		FreePooled(pool,wd->wd_ExtData[1],sizeof(struct MinList));
	}
}


static void
UpdateFilterGadgets(struct Database *db, struct Filter *fi)
{
	wd->wd_ExtData[0] = db;  wd->wd_ExtData[2] = fi;

	if (db == NULL)
		return;

	GT_SetGadgetAttrs(GadgetAddress(win,1),win,NULL,GTTX_Text,db->db_Node.ln_Name,TAG_END);
	if (!fi) {
		GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,&db->db_Filters,GTLV_Selected,~0L,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GA_Disabled,TRUE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GA_Disabled,TRUE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GA_Disabled,TRUE,TAG_END);
	} else if (fi != (APTR)~0L) {
		GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,&db->db_Filters,GTLV_Selected,FindListEntry((struct MinList *)&db->db_Filters, (struct MinNode *)fi),TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,5),win,NULL,GA_Disabled,FALSE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,6),win,NULL,GTST_String,fi->fi_Node.ln_Name,GA_Disabled,FALSE,TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win,7),win,NULL,GTST_String,fi->fi_Filter,GA_Disabled,FALSE,TAG_END);
	}
}


void ASM
HandleFilterIDCMP(REG(a0, struct TagItem *tag))
{
	struct Database *db = wd->wd_ExtData[0];
	struct Filter *fi = wd->wd_ExtData[2];
	long   i;

	switch (imsg.Class) {
		case IDCMP_GADGETDOWN:
			if (!db)
				break;

			switch ((gad = imsg.IAddress)->GadgetID) {
				case 2:    // database-popup
					i = PopUpList(win,GadgetAddress(win,1),wd->wd_ExtData[1],TAG_END);
					if (i != ~0L) {
						for (db = (struct Database *)((struct List *)wd->wd_ExtData[1])->lh_Head;i && db->db_Node.ln_Succ;db = (APTR)db->db_Node.ln_Succ,i--);
						UpdateFilterGadgets(db,NULL);
					}
					break;
				case 8:    // field-popup
					i = PopUpList(win,GadgetAddress(win,7),&db->db_Fields,TAG_END);
					if (i != ~0L) {
						struct Field *field;
						STRPTR t;

						for (field = (APTR)db->db_Fields.mlh_Head;i && field->fi_Node.ln_Succ;field = (APTR)field->fi_Node.ln_Succ,i--);
						GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,~0L,TAG_END);
						if ((t = AllocPooled(pool, 1024)) != 0) {
							if (fi->fi_Filter)
								strcpy(t,fi->fi_Filter);
							FreeString(fi->fi_Filter);
							strcat(t,field->fi_Node.ln_Name);
							fi->fi_Filter = AllocString(t);
							FreePooled(pool,t,1024);
						}
						UpdateFilterGadgets(db,fi);
					}
					break;
			}
			break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
			{
				case 3:    // Filterliste
					if (db) {
						struct Filter *ofi = fi;

						for (fi = (APTR)db->db_Filters.mlh_Head,i = imsg.Code;i && fi->fi_Node.ln_Succ;fi = (APTR)fi->fi_Node.ln_Succ,i--);
						if (ofi == fi) {
							/* toggle activation */
							if (!fi->fi_Node.ln_Type) {
								foreach (&db->db_Filters, ofi)  /* remove old active */
									ofi->fi_Node.ln_Type = 0;
								fi->fi_Node.ln_Type = 1;
							}
							else
								fi->fi_Node.ln_Type = 0;
						}
						UpdateFilterGadgets(db, fi);
						ActivateGadget(GadgetAddress(win,6),win,NULL);
					}
					break;
				case 4:    // new filter
					if (db && (fi = AllocPooled(pool,sizeof(struct Filter)))) {
						fi->fi_Node.ln_Name = AllocString(GetString(&gLocaleInfo, MSG_NEW_FILTER_NAME));
						GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,~0L,TAG_END);
						MyAddTail(&db->db_Filters, fi);
					}
					UpdateFilterGadgets(db, fi);

					if (fi)
						ActivateGadget(GadgetAddress(win, 6), win, NULL);
					break;

				case 5:    // delete filter
					if (db && fi) {
						GT_SetGadgetAttrs(GadgetAddress(win,3),win,NULL,GTLV_Labels,~0L,TAG_END);
						MyRemove(fi);
						FreeFilter(fi);
						UpdateFilterGadgets(db, NULL);
					}
					break;
				case 6:    // filter name
					if (db && fi) {
						GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &i, TAG_END);
						FreeString(fi->fi_Node.ln_Name);
						fi->fi_Node.ln_Name = AllocString((STRPTR)i);
						UpdateFilterGadgets(db, fi);
					}
					break;
				case 7:   // filter definition
					if (db && fi) {
						GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &i, TAG_END);
						FreeString(fi->fi_Filter);
						DeleteTree(fi->fi_Root);  fi->fi_Root = NULL;
						fi->fi_Filter = AllocString((STRPTR)i);
						if ((fi->fi_Root = CreateTree(db->db_Page, fi->fi_Filter)) != 0)
							PrepareFilter(db, fi->fi_Root);
						else
							ErrorRequest(GetString(&gLocaleInfo, MSG_BAD_FILTER_ERR));

						UpdateFilterGadgets(db, fi);
					}
					break;

				case 9:   // Ok
				{
					struct Mappe *mp = wd->wd_Data;
					struct Database *cdb;
					struct Filter *filter = NULL;
					ULONG  current = db->db_Current;

					SetBusy(TRUE, BT_APPLICATION);

					foreach (wd->wd_ExtData[1], cdb) {
						if (!(db = (APTR)MyFindName(&mp->mp_Databases, cdb->db_Node.ln_Name))) {
							ErrorRequest(GetString(&gLocaleInfo, MSG_DATABASE_NOT_FOUND_ERR), cdb->db_Node.ln_Name);
							continue;
						}

						if (db->db_Filter && db->db_Filter->fi_Type == FIT_SEARCH)
							filter = db->db_Filter;
						db->db_Filter = NULL;
						FreeFilters(&db->db_Filters);

						while ((fi = (APTR)MyRemHead(&cdb->db_Filters)) != 0) {
							if (fi->fi_Node.ln_Type)	/* active filter */
								MakeFilter(db, fi);		/* generate filter data */
							MyAddTail(&db->db_Filters, fi);
						}

						foreach (&db->db_Filters, fi) {
							if (fi->fi_Node.ln_Type) {
								db->db_Filter = fi;
								break;
							}
						}

						if (db->db_Filter)          /* free search filter */
							FreeFilter(filter);
						else                        /* search filter stays active */
							db->db_Filter = filter;

						UpdateDBCurrent(db, current);
					}
					RecalcTableFields((struct Page *)mp->mp_Pages.mlh_Head);
					SetBusy(FALSE, BT_APPLICATION);
				}

				case 10:   // cancel
					CloseAppWindow(win, TRUE);
			}
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
			break;
	}
}
