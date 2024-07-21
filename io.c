/* I/O routines and the standard file format implementation (old and current)
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "classes.h"

#ifdef __amigaos4__
	#include <proto/elf.h>
#endif
extern struct FontInfo *ChangeFontInfoA(struct FontInfo *ofi,ULONG thisdpi, struct TagItem *ti, UBYTE freeref);
 
//#define DUMP_CELLS
//#define ENABLE_OLD_FORMAT

struct MinList io_fvs, io_fonts;
struct Node *io_lastfont;


CONST_STRPTR
IFFErrorText(long err)
{
	switch (err) {
		case -3L:  return GetString(&gLocaleInfo, MSG_WRONG_SCOPUS_IFFERR);
		case -4L:  return GetString(&gLocaleInfo, MSG_NO_MEMORY_IFFERR);
		case IFFERR_READ:  return GetString(&gLocaleInfo, MSG_READ_IFFERR);
		case IFFERR_WRITE:  return GetString(&gLocaleInfo, MSG_WRITE_IFFERR);
		case -7L:  return GetString(&gLocaleInfo, MSG_SEEK_IFFERR);
		case -8L:  return GetString(&gLocaleInfo, MSG_CORRUPTED_IFFERR);
		case -9L:  return GetString(&gLocaleInfo, MSG_BAD_SYNTAX_IFFERR);
		case -10L: return GetString(&gLocaleInfo, MSG_NO_IFF_IFFERR);
		default:
			return GetString(&gLocaleInfo, MSG_UNKNOWN_IFFERR);
	}
	return NULL;
}


#if 0
#define DUMPED_BLOCK_SIZE 16

void
dump_block(const char *buffer,int size)
{
	int i;

	for (i = 0;i < size;) {
		int start = i;

		for(;i < start+DUMPED_BLOCK_SIZE;i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x",*(unsigned char *)(buffer+i));
		}
		printf("  ");

		for(i = start;i < start + DUMPED_BLOCK_SIZE;i++) {
			if (i < size) {
				char c = *(buffer+i);

				if (c < 30)
					printf(".");
				else
					printf("%c",c);
			}
			else
				break;
		}
		printf("\n");
	}
}
#endif
	 

STRPTR
ReadChunkString(struct IFFHandle *iff, STRPTR buffer, ULONG len)
{
	STRPTR s;

	if (!(s = buffer) || !len)
		return NULL;

	while (len > 1 && ReadChunkBytes(iff, s, 1) > 0)
	{
		if (*s)
		{
			s++;
			len--;
		}
		else
			break;
	}
	if (*s)
	{
		while(ReadChunkBytes(iff, s, 1) > 0 && *s);
		*s = 0;
	}
	
	return buffer;
}


static STRPTR
DecryptString(STRPTR s)
{
    int length;

	/* MERKER: vielleicht doch mal was vernünftiges ausdenken... */
	if(length = strlen(s))
	{
		STRPTR decrypt = AllocString(s);
		int    i;

		for(i = 0;i < length;i++)
			decrypt[i] += 13;
		return(decrypt);
	}
	else
		return(NULL);
}


static void
CryptString(STRPTR target, STRPTR source)
{
    uint8 i = 0;

	if(source)
		while (*source && i < 63)
		{
			*(target++) = *(source++) - 13;
			i++;
		}
	*(target) = 0;
}

/*
Not used and wrong function for io-functable
STRPTR PUBLIC
ioita(REG(d0, ULONG d1), REG(d1, ULONG d2), REG(d2, long komma), REG(d3, UBYTE flags))
{
	ULONG ld[2];
	double *d = (double *)ld;

	ld[0] = d1;  ld[1] = d2;

	return ita(*d,komma,flags);
}
*/

void PUBLIC
ioUpdateTFText(REG(a0, struct Page *page), REG(a1, struct tableField *tf))
{
	if (!page || !tf)
		return;

	tf->tf_FontInfo = SetFontInfoA(tf->tf_FontInfo,page->pg_DPI,NULL);
	//UpdateCellText(page,tf);
}


static ULONG
lpGetColorPen(struct MinList *list,long id)
{
	struct Node *ln;

	if ((ln = FindListNumber(list, id)) != 0)
		return (ULONG)ln->ln_Name;

	return (ULONG)id;
}


static struct colorPen *
lpFindColorPen(UBYTE red, UBYTE green, UBYTE blue)
{
	struct colorPen *cp;

	foreach (&colors,cp)
	{
		if (red == cp->cp_Red && green == cp->cp_Green && blue == cp->cp_Blue)
			return(cp);
	}
	return NULL;
}


struct colorPen * PUBLIC
ioAddPen(REG(a0, STRPTR name), REG(d0, UBYTE r), REG(d1, UBYTE g), REG(d2, UBYTE b))
{
	struct colorPen *cp;

	if (!(cp = lpFindColorPen(r,g,b)))
		cp = AddColor(&colors,name,r,g,b);
	return(cp);
}

#ifdef ENABLE_OLD_FORMAT

static void
AddGObjectTag(struct TagItem **tags,long *numtags,long *tagpos,ULONG tag,APTR data)
{
	if (*tagpos+1 > *numtags)
	{
		struct TagItem *newtags;

		if (newtags = AllocPooled(pool,(*numtags+14)*sizeof(struct TagItem)))
		{
			if (*tags)
			{
				CopyMem(*tags,newtags,*numtags * sizeof(struct TagItem));
				FreePooled(pool,*tags,*numtags * sizeof(struct TagItem));
			}
			*tags = newtags;
			*numtags += 14;
		}
		else
			return;
	}
	if (!*tags)
		return;
	(*tags)[*tagpos].ti_Tag = tag;
	(*tags)[*tagpos].ti_Data = (ULONG)data;
	(*tagpos)++;
}


struct gObject *
OldLoadGObject(struct Page *page,STRPTR t,struct MinList *list,struct MinList *cols)
{
	struct gObject *go = NULL;
	struct gClass *gc;
	struct point2d *p;
	struct TagItem *tags = NULL;
	long   i,num,numtags = 0,tagpos = 0,tag;
	STRPTR s,name = NULL;

	if (t[0] != 'o' || t[1] != '=')
		return NULL;

	t += 2;
	for(s = t;*s && *s != ';';s++);
	*(s++) = 0;

	if (gc = FindGClass(t))
	{
		if (!(gc->gc_Node.in_Type & GCT_INTERNAL) && !gc->gc_Segment && !LoadGClass(gc))
			return NULL;

		while(*s)
		{
			t = s+2;
			switch(*s)
			{
				case 'p':   // points
					if ((num = atol(t)) && (p = AllocPooled(pool,sizeof(struct point2d)*num)))
					{
						for(i = 0;i < num;i++)
						{
							while(*t && *t++ != ':');

							if (!*t || sscanf(t,"%ld/%ld",&p[i].x,&p[i].y) != 2)
							{
								ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_CLASS_ERR),gc->gc_ClassName);
								FreePooled(pool,p,sizeof(struct point2d)*num);
								if (tags)
									FreePooled(pool,tags,numtags*sizeof(struct TagItem));

								return(NULL);
							}
						}
					}
					break;
				case 'f':   // font
					if (sscanf(t,"%lx",&tag))
					{
						long   size,style;

						while(*t && *t++ != '=');
						if (*t && sscanf(t,"%lx/%lx/%lx",&i,&size,&style) == 3)
						{
							struct Node *ln;

							if (ln = FindListNumber(list,i))
							{
								struct FontInfo *fi;

								if (fi = NewFontInfo(NULL,dpi,FA_Family,      ln->ln_Name,
																							FA_PointHeight, size,
																							FA_Style,       style,
																							TAG_END))
									AddGObjectTag(&tags,&numtags,&tagpos,tag,fi);
							}
						}
					}
					break;
				case 't':   // text
					if (sscanf(t,"%lx",&tag))
					{
						while(*t && *t++ != '"');
						for(s = t;*s && *s != '"';s++);
						if (*s == '"')
							AddGObjectTag(&tags,&numtags,&tagpos,tag,AllocStringLength(t,s-t));
					}
					break;
				case 'v':   // value
					if (sscanf(t,"%lx=%lx",&tag,&i) == 2)
						AddGObjectTag(&tags,&numtags,&tagpos,tag,(APTR)i);
					break;
				case 'c':   // color
					if (sscanf(t,"%lx=%lx",&tag,&i) == 2)
						AddGObjectTag(&tags,&numtags,&tagpos,tag,(APTR)lpGetColorPen(cols,i));
					break;
				case 'n':   // name
					name = t;
					break;
			}
			for(s = t;*s && *s++ != ';';);
		}
		if (tags)
			AddGObjectTag(&tags,&numtags,&tagpos,TAG_END,NULL);

		if (p)
		{
			struct point2d *sp;

			gDoClassMethod(gc,NULL,GCM_ENDPOINTS,p,i = num,&sp,&num);
			if (sp != p)
				FreePooled(pool,p,i*sizeof(struct point2d));
			if (sp && num && (go = (struct gObject *)gDoClassMethod(gc,gc,GCM_NEW,tags,page)))
			{
				go->go_Node.ln_Name = AllocString(name);

				go->go_Knobs = sp;
				go->go_NumKnobs = num;

				RefreshGObjectBounds(page, go);
			}
			if (!go && sp)
				FreePooled(pool, sp, num * sizeof(struct point2d));
		}
		if (tags)
			FreePooled(pool, tags, numtags * sizeof(struct TagItem));
	}
	return go;
}

#endif  // ENABLE_OLD_FORMAT

#define BUFLEN 512

static ULONG
ReadColor(UBYTE **p)
{
	ULONG c = 0L;

	memcpy((UBYTE *)&c+1,*p,3);
	*p += 3;
	return(c);
}


static STRPTR
ReadString(UBYTE **p)
{
	STRPTR t = *p;

	*p += strlen(t)+1;
	return AllocString(t);
}


static LONG
ReadValue(UBYTE **p, UBYTE num)
{
	LONG size,storage = 0;
 
	size = 1L << num;
	ASSERT(size <= 4);

	memcpy((UBYTE *)&storage + 4 - size, *p, size);
	*p += size;
	return storage;
}


static LONG
ReadLong(UBYTE **p)
{
	LONG storage;

	storage = *(LONG *)*p;  *p += 4;
	return storage;
}


long
LoadCells(struct IFFHandle *iff, LONG context, struct Page *page, struct MinList *list)
{
	struct StoredProperty *sp;
	struct tableField *tf = NULL,*ntf;
	UBYTE  *a,*b,*end,tag,skip_row,unknownTag = 0;

	if (!(sp = FindProp(iff, context, ID_CELL)))
		return IFFERR_READ;

	a = sp->sp_Data;  end = a+sp->sp_Size;
	if (!(*a & TAG_NewCell))
		return IFFERR_MANGLED;

	if (!(ntf = MakeTableField(page, 1, 1)))
		return IFFERR_NOMEM;

	while (a < end)
	{
		if (*a & TAG_NewCell)
		{
			ULONG col, row;

#ifdef DUMP_CELLS
			bug("newcell: %lx (%s)\n",(long)*a,(*a & TAG_FromAbove) ? "above" : ((*a & TAG_FromLeft) ? "left" : "new"));
			dump("  ",a,16);
#endif

			if (tf && !tf->tf_Text && !(tf->tf_Flags & TFF_FONTSET)) // alte Zelle zurücksetzen
			{
				FreeFontInfo(tf->tf_FontInfo);
				tf->tf_FontInfo = NewFontInfo(NULL,page->pg_DPI,FA_Family,     page->pg_Family,
																FA_PointHeight,page->pg_PointHeight,
																TAG_END);
			}
			b = a;
			tag = *a++;
			if (tag & TAG_FromLeft)
				tf = CopyCell(page, tf);
			else if (!(tag & TAG_FromAbove))
				tf = CopyCell(page, ntf);

			if (tf)
			{
				col = tf->tf_Col;
				row = tf->tf_Row;
			}
			else
			{
				col = ((struct tableField *)list->mlh_TailPred)->tf_Col;
				row = ((struct tableField *)list->mlh_TailPred)->tf_Row;
			}
			skip_row = FALSE;

			switch (tag & TAG_Position)
			{
				case TAG_BytePos:
					col = *a++;
					row = *a++;
					break;
				case TAG_WordPos:
					col = *(UWORD *)a;  a += 2;
					row = *(UWORD *)a;  a += 2;
					break;
				case TAG_LongPos:
					col = *(ULONG *)a;  a += 4;
					row = *(ULONG *)a;  a += 4;
					break;
				default:
				{
					switch (tag & TAG_Position)
					{
						/*case TAG_Empty:
							break;*/
						case TAG_ByteCol:
							col = *a++;
							break;
						case TAG_WordCol:
							col = *(UWORD *)a;  a += 2;
							break;
						case TAG_LongCol:
							col = *(ULONG *)a;  a += 4;
							break;
						default:
							if (tag & (TAG_FromAbove | TAG_FromLeft))
								col++;
							if ((tag & TAG_Position) == TAG_Empty)
								break;
#ifdef DUMP_CELLS
							bug("  -> %ld\n",tag & TAG_Position);
#endif
							a--;
							break;
					}
					if ((!(*a & TAG_NewCell) && *a > TAG_LongRow) || (tag & TAG_Position) == TAG_Empty || (b != a && (*a & TAG_NewCell)))
					{
						skip_row = TRUE;
#ifdef DUMP_CELLS
						bug("  skip rows.\n");
#endif
						break;
					}
					switch (*a++ & TAG_Position)
					{
						case TAG_ByteRow:
							row = *a++;
							break;
						case TAG_WordRow:
							row = *(UWORD *)a;  a += 2;
							break;
						case TAG_LongRow:
							row = *(ULONG *)a;  a += 4;
							break;
					}
#ifdef DUMP_CELLS
	bug("  row: %ld\n",row);
#endif
					break;
				}
			}
			if (tag & TAG_FromAbove)
			{
				for(tf = (APTR)list->mlh_TailPred;tf->tf_Node.mln_Pred && tf->tf_Col != col;tf = (APTR)tf->tf_Node.mln_Pred);
				if (!tf->tf_Node.mln_Pred)
				{
					ErrorRequest(GetString(&gLocaleInfo, MSG_REFERENCE_CELL_NOT_FOUND_ERR),col,row);
					tf = ntf;
				}
				if (!(tf = CopyCell(page,tf)))
					return IFFERR_NOMEM;

				if (skip_row)
					row = tf->tf_Row+1;
			}
			MyAddTail(list,tf);
			tf->tf_Col = col;
			tf->tf_Row = row;
#ifdef DUMP_CELLS
			bug("  pos: %ld:%ld\n",tf->tf_Col,tf->tf_Row);
#endif
			{
				struct tableField *ptf = (struct tableField *)tf->tf_Node.mln_Pred;

				if (ptf->tf_Row == tf->tf_Row)
					ptf->tf_MaxWidth = tf->tf_Col-ptf->tf_Col-1;
			}
			if (!(tag & TAG_WithText))
			{
				FreeString(tf->tf_Text);
				tf->tf_Text = NULL;
			}
			if (a >= end)
				break;
		}
		if (*a & TAG_NewCell)
			continue;

		tag = *a++;

#ifdef DUMP_CELLS
		bug("  tag: %ld ",tag);
#endif
		switch (tag)          // MERKER: viel Raum zum Optimieren (switch?)...
		{
			case CELL_NOFLAGS:
				tf->tf_Flags = 0;
				break;
			case CELL_BYTEFLAGS:
			case CELL_WORDFLAGS:
			case CELL_LONGFLAGS:
				tf->tf_Flags = ReadValue(&a, tag - CELL_BYTEFLAGS);
				break;
			case CELL_APENPAGE:
				tf->tf_ReservedPen = tf->tf_APen = page->pg_APen;
				break;
			case CELL_BPENPAGE:
				tf->tf_BPen = page->pg_BPen;
				break;
			case CELL_APEN:
				tf->tf_ReservedPen = tf->tf_APen = ReadColor(&a);
				break;
			case CELL_BPEN:
				tf->tf_BPen = ReadColor(&a);
				break;
			case CELL_NEGPEN:
				tf->tf_NegativePen = ReadColor(&a);
				break;
			case CELL_PATTERN:
				tf->tf_Pattern = *a++;
				break;
			case CELL_PATTERNCOLOR:
				tf->tf_PatternColor = ReadColor(&a);
				break;
			case CELL_TEXT:
				tf->tf_Text = ReadString(&a);
#ifdef DUMP_CELLS
				bug("text: '%s'",tf->tf_Text);
				if (tf->tf_Col == 2 && tf->tf_Row == 14)
					bug(" [width: %ld - widthset: %ld - maxwidth: %ld - oldwidth: %ld]",tf->tf_Width,tf->tf_WidthSet,tf->tf_MaxWidth,tf->tf_OldWidth);
#endif
				break;
			case CELL_FORMULA:
				tf->tf_Text = AllocString(a-1);
				*tf->tf_Text = '=';
				a += strlen(a)+1;
#ifdef DUMP_CELLS
				bug("formula: %s",tf->tf_Text);
#endif
				break;
			case CELL_POINT:
				tf->tf_Komma = *a++;
				break;
			case CELL_ALIGN:
				tf->tf_Alignment = *a++;
				break;
			case CELL_NOTE:
				tf->tf_Note = ReadString(&a);
				break;
			case CELL_NOFORMAT:
				tf->tf_Format = NULL;
				break;
			case CELL_LEFTBORDER:
			case CELL_RIGHTBORDER:
			case CELL_TOPBORDER:
			case CELL_BOTTOMBORDER:
				tf->tf_Border[tag - CELL_LEFTBORDER] = *a++;
				break;
			case CELL_LBORDERCOLOR:
			case CELL_RBORDERCOLOR:
			case CELL_TBORDERCOLOR:
			case CELL_BBORDERCOLOR:
				tf->tf_BorderColor[tag - CELL_LBORDERCOLOR] = ReadColor(&a);
				break;
			case CELL_BYTEFORMAT:
			case CELL_WORDFORMAT:
			case CELL_LONGFORMAT:
			{
				LONG   num = ReadValue(&a, tag - CELL_BYTEFORMAT);
				struct Node *ln;

				if ((ln = FindListNumber(&io_fvs, num)) != 0)
					ln = (struct Node *)ln->ln_Name;

				tf->tf_Format = AllocString((STRPTR)ln);
				break;
			}
			case CELL_FORMATNAME:
			{
				tf->tf_Format = AllocString(a);
				/* Format zum Projekt hinzufügen, wenn noch nicht vorhanden */

				if (!FindLinkName(&page->pg_Mappe->mp_CalcFormats,a))
				{
					struct Mappe *mp = page->pg_Mappe;
					struct Prefs *pr = GetLocalPrefs(mp);

					AddPrefsModuleToLocalPrefs(mp,WDT_PREFFORMAT);
					SetPrefsModule(pr,WDT_PREFFORMAT,TRUE);

					if (LockList(&pr->pr_Formats,LNF_ADD))
					{
						AddFormat(&pr->pr_Formats,a,0,-1,-1,0L,FVF_NONE,FVT_NOTSPECIFIED);
						SortFormatList(&pr->pr_Formats);

						UnlockList(&pr->pr_Formats,LNF_ADD);
					}
					RefreshPrefsModule(pr,NULL,WDT_PREFFORMAT);
				}
				a += strlen(a)+1;
				break;
			}
			case CELL_BYTEFONT:
			case CELL_WORDFONT:
			case CELL_LONGFONT:
			{
				LONG num = ReadValue(&a, tag - CELL_BYTEFONT);
				struct Node *ln;

				if ((ln = FindListNumber(&io_fonts, num)) != 0)
					ln = (struct Node *)ln->ln_Name;

				io_lastfont = ln;
				tf->tf_FontInfo = SetFontInfo(tf->tf_FontInfo, page->pg_DPI, FA_Family, ln, TAG_END);
				break;
			}
			case CELL_LASTFONT:
				tf->tf_FontInfo = SetFontInfo(tf->tf_FontInfo, page->pg_DPI, FA_Family, io_lastfont, TAG_END);
				break;
			case CELL_FONTNAME:
			{
				struct Node *ln;

				if (!(ln = FindTag(&families, a)))
				{
					ErrorRequest(GetString(&gLocaleInfo, MSG_FONT_NOT_FOUND_ERR), a);

					/* MERKER: Auswahlmöglichkeit für neue Fonts schaffen */

				}
				else
					tf->tf_FontInfo = SetFontInfo(tf->tf_FontInfo, page->pg_DPI, FA_Family, ln, TAG_END);

				a += strlen(a) + 1;
				break;
			}
			case CELL_POINTHEIGHT:
			case CELL_STYLE:
			{
				LONG value = *(LONG *)a;

				tf->tf_FontInfo = SetFontInfo(tf->tf_FontInfo, page->pg_DPI, tag == CELL_STYLE ? FA_Style : FA_PointHeight, value, TAG_END);
				a += 4;
				break;
			}
			case CELL_NOWIDTH:
				tf->tf_WidthSet = (UWORD)~0;
				break;
			case CELL_BYTEWIDTH:
			case CELL_WORDWIDTH:
			case CELL_LONGWIDTH:
				tf->tf_WidthSet = ReadValue(&a, tag - CELL_BYTEWIDTH);
				break;
			case CELL_TYPE_BYTE:  
				unknownTag = *a;  a += 1 + 1;
				break;
			case CELL_TYPE_WORD:
				unknownTag = *a;  a += 1 + 2;
				break;
			case CELL_TYPE_LONG:
				unknownTag = *a;  a += 1 + 4;
				break;
			case CELL_TYPE_COLOR:
				unknownTag = *a;  a += 1 + 3;
				break;
			case CELL_TYPE_STRING:
				unknownTag = *a++;
				a += strlen(a)+1;
				break;
			case CELL_TYPE_BYTES:
			{
				LONG value = *(LONG *)a;

				a += 4;
				unknownTag = *a;
				a += 1 + value;
				break;
			}
			default:
				return IFFERR_MANGLED;
		}

#ifdef DUMP_CELLS
		bug("\n");
		if (unknownTag != 0)
			bug("  unknown tag: %ld\n",(long)unknownTag);
			struct tableField
#endif
	}
	if (unknownTag != 0)
		ErrorRequest(GetString(&gLocaleInfo, MSG_UNKNOWN_CELL_TAGS_ERR));

	return 0L;
}


int32
LoadObject(struct IFFHandle *iff, LONG context, struct Page *page)
{
	struct gObject *go;

	if (!(go = (struct gObject *)gDoClassMethod(FindGClass("root"), NULL, GCM_LOAD, iff, &io_fonts, page)))
		return 0L;

	if (!(go->go_Class->gc_Node.in_Type & GCT_DIAGRAM))
	{
		MyAddTail(&page->pg_gObjects, go);
		MyAddTail(&page->pg_gGroups, OBJECTGROUP(go));
	}
	else
		MyAddTail(&page->pg_gDiagrams, go);

	//AddGObject(page,NULL,go,ADDREM_NONE);

	return 0L;
}


static int32
LoadPage(struct IFFHandle *iff, struct Mappe *mp)
{
	struct ContextNode *cn;
	struct StoredProperty *sp;
	struct Page *page;
	long   error = 0;
	UBYTE  *t;

	if (!(page = NewPage(mp)))
		return IFFERR_NOMEM;

	//PropChunk(iff,ID_PAGE,ID_OBJ);
	PropChunk(iff, ID_PAGE, ID_CELL);
	PropChunk(iff, ID_PAGE, ID_PGHD);

	while (!error || error == IFFERR_EOC)
	{
		if ((error = ParseIFF(iff,IFFPARSE_STEP)) && error != IFFERR_EOC)
			break;

		if ((cn = CurrentChunk(iff)) != 0)
		{
			if (error == IFFERR_EOC)  // leaving a chunk
			{
				if (cn->cn_ID == ID_PGHD && (sp = FindProp(iff,ID_PAGE,ID_PGHD)) && (t = sp->sp_Data))
				{
					struct Node *ln;
					ULONG  i;

					if (*t)
					{
						FreeString(page->pg_Node.ln_Name);
						page->pg_Node.ln_Name = AllocString(t);
						t += strlen(t);
					}
					t++;
					page->pg_APen = ReadColor(&t);
					page->pg_BPen = ReadColor(&t);
					page->pg_Zoom = *(ULONG *)t;  t += 4;
					RecalcPageDPI(page);

					if ((ln = FindListNumber(&io_fonts, *(ULONG *)t)) != 0)
						page->pg_Family = (struct Node *)ln->ln_Name;

					t += 4;
					page->pg_PointHeight = *(ULONG *)t;  t += 4;
					io_lastfont = page->pg_Family;

					page->pg_mmStdWidth = *(ULONG *)t;  t += 4;
					while ((i = *(ULONG *)t) != 0)
					{
						AllocTableSize(page,i,0);  t += 4;
						(page->pg_tfWidth-1+i)->ts_mm = *(ULONG *)t;
						t += 4;
					}
					t += 4;

					page->pg_mmStdHeight = *(ULONG *)t;  t += 4;
					while ((i = *(ULONG *)t) != 0)
					{
						AllocTableSize(page,0,i);  t += 4;
						(page->pg_tfHeight-1+i)->ts_mm = *(ULONG *)t;
						t += 4;
					}
					t += 4;

					/*** read titles ***/

					if ((t-(UBYTE *)sp->sp_Data) < sp->sp_Size)
					{
						while ((i = *(ULONG *)t) != 0)
						{
							AllocTableSize(page,i,0);  t += 4;
							(page->pg_tfWidth-1+i)->ts_Title = ReadString(&t);
						}
						t += 4;

						if ((t-(UBYTE *)sp->sp_Data) < sp->sp_Size)
						{
							while ((i = *(ULONG *)t) != 0)
							{
								AllocTableSize(page,0,i);  t += 4;
								(page->pg_tfHeight-1+i)->ts_Title = ReadString(&t);
							}
						}
					}
				}
				else if (cn->cn_ID == ID_CELL)
				{
					struct tableField *tf;
					long col = 0,row = 0;

					error = LoadCells(iff, ID_PAGE, page, &page->pg_Table);
					foreach (&page->pg_Table, tf)
					{
						D(  if (tf->tf_Col == 0 || tf->tf_Row == 0)
							{
								struct tableField *dtf = tf;

								bug("**** warning: cell at %ld:%ld\n",tf->tf_Col,tf->tf_Row);
								tf = (struct tableField *)tf->tf_Node.mln_Succ;
								MyRemove(dtf);
								FreeTableField(dtf);

								if (!tf->tf_Node.mln_Succ)
									break;
							}
						 );
						if (tf->tf_Col > col)
							col = tf->tf_Col;
						if (tf->tf_Row > row)
							row = tf->tf_Row;
					}
					AllocTableSize(page, col, row);
					LinkCellsToTableSize(page);
				}
				else if (cn->cn_ID == ID_OBJ)
					error = LoadObject(iff, ID_PAGE, page);
				else if (cn->cn_Type == ID_PAGE && cn->cn_ID == ID_FORM)
					break;
			}
		}
	}
	return error;
}


static int32
LoadDatabase(struct IFFHandle *iff,struct Mappe *mp)
{
	struct StoredProperty *sp;
	struct MaskField *mf;
	struct Database *db;
	struct Field *fi;
	struct Index *in;
	struct Mask *ma;
	long   i;
	UBYTE  b, *d, *stop;

	if (!(sp = FindProp(iff, ID_TABL, ID_DB)))
		return 0L;

	if (!(db = AllocPooled(pool, sizeof(struct Database))))
		return IFFERR_NOMEM;

	d = sp->sp_Data;  stop = d + sp->sp_Size;

	db->db_Node.ln_Name = ReadString(&d);
	db->db_Node.ln_Type = NMT_DATABASE;
	db->db_Content = ReadString(&d);
	db->db_PageNumber = ReadLong(&d);

	MyNewList(&db->db_Fields);
	MyNewList(&db->db_Indices);
	MyNewList(&db->db_Filters);
	MyAddTail(&mp->mp_Databases, db);

	for (i = ReadLong(&d); i > 0; i--)
	{
		if (!(fi = AllocPooled(pool, sizeof(struct Field))))
			return IFFERR_NOMEM;

		fi->fi_Node.ln_Type = *d++;
		fi->fi_Special = ReadString(&d);
		MyAddTail(&db->db_Fields, fi);
	}

	while (d < stop)
	{
		b = *d++;
		switch (b)
		{
			case 1:  /* read indices */
				while ((b = *d++) != 0)
				{
					if (!(in = AllocPooled(pool, sizeof(struct Index))))
						return IFFERR_NOMEM;

					if (b == 2)   /* active index */
					{
						db->db_Index = in;
						in->in_Node.ln_Type = 1;
					}
					in->in_Node.ln_Name = ReadString(&d);
					MyAddTail(&db->db_Indices, in);
				}
				break;
			case 2:  /* read mask */
				if (!(ma = AllocPooled(pool, sizeof(struct Mask))))
					return IFFERR_NOMEM;

				ma->ma_Node.ln_Name = AllocString(db->db_Node.ln_Name);
				ma->ma_Page = (APTR)ReadLong(&d);
				MyNewList(&ma->ma_Fields);
				MyAddTail(&mp->mp_Masks, ma);

				for (i = ReadLong(&d); i > 0; i--) {
					if (!(mf = AllocPooled(pool, sizeof(struct MaskField))))
						return IFFERR_NOMEM;

					mf->mf_Col = ReadLong(&d);
					mf->mf_Row = ReadLong(&d);
					mf->mf_Node.ln_Name = ReadString(&d);
					MyAddTail(&ma->ma_Fields, mf);
				}
				break;
			case 3:		/* read filter */
			{
				struct Filter *filter;

#ifdef __amigaos4__
				while (d < stop)
				{
#endif
					if ((filter = AllocPooled(pool, sizeof(struct Filter))) == NULL)
						return IFFERR_NOMEM;
				
					filter->fi_Node.ln_Type = *d++;
					filter->fi_Node.ln_Name = ReadString(&d);
					filter->fi_Filter = ReadString(&d);
					MyAddTail(&db->db_Filters, filter);
#ifdef __amigaos4__
				}
#endif
				break;
			}
		}
	}
	return 0L;
}

#ifdef __amigaos4__
static int32
LoadScripts(struct IFFHandle *iff, struct Mappe *mp)
{
	struct StoredProperty *sp;
	struct RexxScript *rxs;
	long   i;
	UBYTE  b, *d, *stop;

	if (!(sp = FindProp(iff, ID_TABL, ID_SCRIPT)))
		return 0L;

	d = sp->sp_Data;  stop = d + sp->sp_Size;
	MyNewList(&mp->mp_RexxScripts);
	for (i = ReadLong(&d); i > 0; i--)
	{
		if (!(rxs = AllocPooled(pool, sizeof(struct RexxScript))))
			return IFFERR_NOMEM;
		rxs->rxs_Node.ln_Name = ReadString(&d);
		rxs->rxs_Description = ReadString(&d);
		rxs->rxs_Data = ReadString(&d);
		rxs->rxs_DataLength = ReadLong(&d);
		rxs->rxs_Map = mp;
		MyAddTail(&mp->mp_RexxScripts, rxs);
	}
	return 0L;
}
#endif

void
LoadFonts(struct IFFHandle *iff)
{
	struct StoredProperty *sp;
	struct Node *ln;
	ULONG  pos = 0;

	if (!(sp = FindProp(iff, ID_TABL, ID_FONTS)))
		return;

	while (pos < sp->sp_Size)
	{
		STRPTR family = (UBYTE *)sp->sp_Data + pos;

		if ((ln = AllocPooled(pool, sizeof(struct Node))))
		{
			// gibt es diesen Schriftsatz auf diesem System?
			if (!(ln->ln_Name = (char *)MyFindName(&families, family)))
			{
				/* MERKER: nette Auswahlmöglichkeit schaffen */
				/* for now: find known good font */
				struct Node *font;

				// search for a known good font
				foreach (&families, font) {
					if (!strcmp(font->ln_Name, "Arial")
						|| !strcmp(font->ln_Name, "CG Triumvirate")
						|| !strcmp(font->ln_Name, "Helvetica")
						|| !strcmp(font->ln_Name, "Times")
						|| !strcmp(font->ln_Name, "Times New Roman")) {
							ln->ln_Name = (char *)font;
							break;
						}	
					}
				if (ln->ln_Name == NULL) {
					// still nothing found, be a bit more generic
					foreach (&families, font) {
						if (strstr(font->ln_Name, "Arial")
							|| strstr(font->ln_Name, "CG Triumvirate")
							|| strstr(font->ln_Name, "Helvetica")
							|| strstr(font->ln_Name, "Times")) {
								ln->ln_Name = (char *)font;
								break;
							}
						}
					if (ln->ln_Name == NULL) {
						// well, just take the first one
						ln->ln_Name = (char *)families.mlh_Head;
						}
					}
				}
			MyAddTail(&io_fonts, ln);
		}
		pos += strlen(family) + 1;
	}
}

static long ASM
StandardLoadProject(REG(d0, BPTR dat), REG(a0, struct Mappe *mp))
{
	struct IFFHandle *iff;
	struct Node *ln;
	long   error;
#ifdef __amigaos4__
	LONG   chunks[] = {ID_TABL,ID_MAP, ID_TABL,ID_FONTS, ID_TABL,ID_DB, ID_TABL,ID_SCRIPT};
#else
	LONG   chunks[] = {ID_TABL,ID_MAP, ID_TABL,ID_FONTS, ID_TABL,ID_DB};
#endif
	if (!(iff = AllocIFF()))
		return RC_FAIL;

	InitIFFasDOS(iff);
	iff->iff_Stream = (IPTR)dat;

#ifdef __amigaos4__
	PropChunks(iff, chunks, 4);
#else
	PropChunks(iff, chunks, 3);
#endif
	PropPrefsChunks(iff, ID_TABL,PRF_NAMES | PRF_FORMAT | PRF_DISPLAY | PRF_KEYS | PRF_ICON | PRF_CMDS | PRF_MENU | PRF_CONTEXT);

	if (!(error = OpenIFF(iff, IFFF_READ)))
	{
		struct ContextNode *cn;

		while (!error || error == IFFERR_EOC)
		{
			/*{
				struct ContextNode *node = CurrentChunk(iff);
				if (node != NULL) {
					printf("1. Current ContextNode: type = %lx, id = %lx, size = %ld, scan = %ld\n", node->cn_Type, node->cn_ID, node->cn_Size, node->cn_Scan);
				} else
					printf("1. current ContextNode == NULL\n");
			}*/
			
            if ((error = ParseIFF(iff, IFFPARSE_STEP)) && error != IFFERR_EOC)
				break;

			if ((cn = CurrentChunk(iff)) != 0)
			{
				if (error == IFFERR_EOC)      // leaving a chunk
				{
					if (cn->cn_Type == ID_TABL)
					{
						switch (cn->cn_ID)
						{
							case ID_FONTS: LoadFonts(iff); break;
							// preferences
							// lpVersion(iff); /* MERKER: Version berücksichtigen */
							case ID_FORMAT:  lpFormat(iff, ID_TABL, &mp->mp_Prefs, &io_fvs); break;
							case ID_NAMES:   lpNames(iff, ID_TABL, &mp->mp_Prefs); break;
							case ID_CMDS:    lpCmds(iff, ID_TABL, &mp->mp_Prefs, FALSE, TRUE/*keep*/); break;
							case ID_MENU:    lpMenu(iff, ID_TABL, &mp->mp_Prefs); break;
							case ID_ICON:    lpIcon(iff, ID_TABL, &mp->mp_Prefs); break;
							case ID_DISP:    lpDisp(iff, ID_TABL, &mp->mp_Prefs); break;
							case ID_KEYS:    lpKeys(iff, ID_TABL, &mp->mp_Prefs); break;
							case ID_CONTEXT: lpContext(iff, ID_TABL, &mp->mp_Prefs); break;

							case ID_DB: LoadDatabase(iff, mp); break;
#ifdef __amigaos4__
							case ID_SCRIPT: LoadScripts(iff, mp); break;
#endif
							case ID_MAP:
							{
								struct StoredProperty *sp;
								UBYTE  pos,*t;

								if (!(sp = FindProp(iff, ID_TABL, ID_MAP)))
									break;

								t = sp->sp_Data;
								CopyMem(t, &mp->mp_mmWidth, 8);  t += 8;
								CopyMem(t, &mp->mp_Flags, 4);  t += 4;  pos = 12;

								while (pos < sp->sp_Size)
								{
									int i;

									if (*t == 255)       // Read window position
									{
										CopyMem(++t,&mp->mp_WindowBox,8);
										t += 8;  pos += 9;
									}
									else if (*t == 254)  // Read Author/Version/...
									{
										mp->mp_Author = AllocString(++t);
										i = strlen(t)+1;  t += i;  pos += i+1;
										mp->mp_Version = AllocString(t);
										i = strlen(t)+1;  t += i;  pos += i;
										mp->mp_CatchWords = AllocString(t);
										i = strlen(t)+1;  t += i;  pos += i;
										mp->mp_Note = AllocString(t);
										i = strlen(t)+1;  t += i;  pos += i;
									}
									else if (*t == 253)  // Read printer flags and page borders
									{
										CopyMem(++t,&mp->mp_PrinterFlags,4);  t += 4;
										CopyMem(t,&mp->mp_BorderLeft,16);
										t += 16;  pos += 21;
									}
									else if (*t == 252)  // Read Passwords
									{
										mp->mp_Password = DecryptString(++t);
										i = strlen(t)+1;  t += i;  pos += i+1;
										mp->mp_CellPassword = DecryptString(t);
										i = strlen(t)+1;  t += i;  pos += i;
									}
									else if (*t < NUM_EVENTS)  /* events */
									{
										i = *t++;
										mp->mp_Events[i].ev_Flags = *t++;  pos += 2;
										mp->mp_Events[i].ev_Command = AllocString(t);
										i = strlen(t)+1;  t += i;  pos += i;
									}
									else
									{
										ErrorRequest(GetString(&gLocaleInfo, MSG_UNKNOWN_CHUNKS_ERR),(long)*t);
										break;
									}
								}
								break;
							}
						}
					}
				}
				else if (!error)  // entering a chunk
				{
					if (cn->cn_Type == ID_PAGE && cn->cn_ID == ID_FORM)
						error = LoadPage(iff, mp);
				}
			}
			/*{
				struct ContextNode *node = CurrentChunk(iff);
				if (node != NULL) {
					printf("2. Current ContextNode: type = %lx, id = %lx, size = %ld, scan = %ld\n", node->cn_Type, node->cn_ID, node->cn_Size, node->cn_Scan);
				} else
					printf("2. current ContextNode == NULL\n");
			}*/
		}
		CloseIFF(iff);
	}

	FreeIFF(iff);

	while ((ln = MyRemHead(&io_fonts)) != 0)
		FreePooled(pool, ln, sizeof(struct Node));

	while ((ln = MyRemHead(&io_fvs)) != 0)
	{
		FreeString(ln->ln_Name);
		FreePooled(pool, ln, sizeof(struct Node));
	}

	if (error < 0 && error != IFFERR_EOF)
		ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_DOCUMENT_ERR), IFFErrorText(error));

	return RC_OK;
}


long  sc_size, sc_abovePos, sc_leftPos;
UBYTE *sc_aboveBuffer, *sc_leftBuffer, *sc_buffer;
long  sc_left, sc_above;


static bool
ExtendSCBuffer(uint8 **_buffer, uint8 *which, size_t newSize)
{
	uint8 *newBuffer;

	if (newSize <= sc_size)
		return true;

	newSize = ((newSize / 256) + 1) * 256;

	if (!(newBuffer = AllocPooled(pool, newSize * 2)))
		return false;

	*_buffer = *_buffer - sc_buffer + newBuffer + (which == sc_aboveBuffer ? 0 : newSize);
	sc_buffer = newBuffer + (which == sc_aboveBuffer ? 0 : newSize);

	if (sc_aboveBuffer)
	{
		CopyMem(sc_aboveBuffer, newBuffer, sc_size);
		CopyMem(sc_leftBuffer, newBuffer + newSize, sc_size);

		FreePooled(pool, sc_aboveBuffer, sc_size * 2);
	}
	sc_aboveBuffer = newBuffer;
	sc_leftBuffer = newBuffer + newSize;
	sc_size = newSize;

	return true;
}


static void
WriteBytes(UBYTE tag, APTR p, long size, UBYTE **_buffer)
{
	if (!ExtendSCBuffer(_buffer, sc_buffer, *_buffer + size + 1 - sc_buffer))
		return;

	if (tag)
		*(*_buffer)++ = tag;

	if (!size || !p)
		return;

	CopyMem(p, *_buffer, size);
	*_buffer += size;
}


static void
WriteValue(UBYTE tag, ULONG v, UBYTE **_buffer)
{
	if (v < 256)
		WriteBytes(tag, ((UBYTE *)&v) + 3, 1, _buffer);
	else if (v < 65536)
		WriteBytes(tag + 1, ((UWORD *)&v) + 1, 2, _buffer);
	else
		WriteBytes(tag + 2, &v, 4, _buffer);
}


static void
WriteString(UBYTE tag, STRPTR s, UBYTE **_buffer)
{
	long length = s ? strlen(s) + 1 : 1;

	if (!ExtendSCBuffer(_buffer, sc_buffer, *_buffer + length + 1 - sc_buffer))
		return;

	*(*_buffer)++ = tag;
	if (s)
	{
		CopyMem(s, *_buffer, length);
		*_buffer += length;
	}
	else
		*(*_buffer)++ = 0;
}


struct Node *
MakeCellStream(struct Page *page, struct tableField *tf, uint8 tag, struct tableField *reftf, uint8 *buffer, long *_pos, bool fullnames)
{
	struct Node *font = io_lastfont;
	struct tableField *ptf = (APTR)tf->tf_Node.mln_Pred;

	sc_buffer = buffer;

	/** position **/

	if (tf->tf_Original && reftf->tf_Original && !strcmp(reftf->tf_Original, tf->tf_Original))
		tag |= TAG_WithText;

	tag |= TAG_NewCell;

	if ((tag & TAG_FromLeft && (tf->tf_Row == reftf->tf_Row || tf->tf_Col+1 == reftf->tf_Col)) ||
			(tag & TAG_FromAbove && (tf->tf_Col == ptf->tf_Col+1 || tf->tf_Row == reftf->tf_Row+1)) ||
			(!(tag & (TAG_FromAbove | TAG_FromLeft)) && (tf->tf_Col == 1 || tf->tf_Row == 1)))
	{
		if ((tag & TAG_FromAbove && ptf->tf_Col+1 != tf->tf_Col)
			|| (tag & TAG_FromLeft && tf->tf_Row == reftf->tf_Row && tf->tf_Col > reftf->tf_Col+1)
			|| !(tag & (TAG_FromLeft | TAG_FromAbove)) && tf->tf_Row == 1 && tf->tf_Col != 1)
		{
			if (tf->tf_Col < 256)
				WriteBytes(tag | TAG_ByteCol, (UBYTE *)&tf->tf_Col + 3, 1, &buffer);
			else if (tf->tf_Col < 65536)
				WriteBytes(tag | TAG_WordCol, (UWORD *)&tf->tf_Col + 1, 2, &buffer);
			else
				WriteBytes(tag | TAG_LongCol, &tf->tf_Col, 4, &buffer);
		}
		else if ((tag & TAG_FromLeft && tf->tf_Row != reftf->tf_Row) || (tag & TAG_FromAbove && tf->tf_Row != reftf->tf_Row+1) || !(tag & (TAG_FromLeft | TAG_FromAbove)) && tf->tf_Col == 1 && tf->tf_Row != 1)
		{
			if (tf->tf_Row < 256)
				WriteBytes(tag | TAG_ByteRow, (UBYTE *)&tf->tf_Row + 3, 1, &buffer);
			else if (tf->tf_Row < 65536)
				WriteBytes(tag | TAG_WordRow, (UWORD *)&tf->tf_Row + 1, 2, &buffer);
			else
				WriteBytes(tag | TAG_LongRow, &tf->tf_Row, 4, &buffer);
		}
		else
			WriteBytes(tag, NULL, 0, &buffer);
	}
	else
	{
		if (tf->tf_Col < 256 && tf->tf_Row < 256)
		{
			WriteBytes(tag | TAG_BytePos, (UBYTE *)&tf->tf_Col + 3, 1, &buffer);
			WriteBytes(0, (UBYTE *)&tf->tf_Row + 3, 1, &buffer);
		}
		else if (tf->tf_Col < 65536 && tf->tf_Row < 65536)
		{
			WriteBytes(tag | TAG_WordPos, (UWORD *)&tf->tf_Col + 1, 2, &buffer);
			WriteBytes(0, (UWORD *)&tf->tf_Row + 1, 2, &buffer);
		}
		else if (tf->tf_Col < 65536)
		{
			if (tf->tf_Col < 256)
				WriteBytes(tag | TAG_ByteCol, (UBYTE *)&tf->tf_Col + 3, 1, &buffer);
			else
				WriteBytes(tag | TAG_WordCol, (UWORD *)&tf->tf_Col + 1, 2, &buffer);
			WriteBytes(TAG_LongRow, &tf->tf_Row, 4, &buffer);
		}
		else if (tf->tf_Row < 65536)
		{
			WriteBytes(tag | TAG_LongCol, &tf->tf_Col, 4, &buffer);
			if (tf->tf_Row < 256)
				WriteBytes(TAG_ByteRow, (UBYTE *)&tf->tf_Row + 3, 1, &buffer);
			else
				WriteBytes(TAG_WordRow, (UWORD *)&tf->tf_Row + 1, 2, &buffer);
		}
		else
			WriteBytes(tag | TAG_LongPos, &tf->tf_Col, 8, &buffer);
	}

	/** flags **/

	if (tf->tf_Flags != reftf->tf_Flags)
	{
		if (!tf->tf_Flags)
			WriteBytes(CELL_NOFLAGS, NULL, 0, &buffer);
		else
			WriteValue(CELL_BYTEFLAGS, tf->tf_Flags, &buffer);
	}

	/** colors **/

	if (reftf->tf_ReservedPen != tf->tf_ReservedPen)
	{
		if (tf->tf_ReservedPen == page->pg_APen)
			WriteBytes(CELL_APENPAGE, NULL, 0, &buffer);
		else
			WriteBytes(CELL_APEN, (UBYTE *)&tf->tf_ReservedPen + 1, 3, &buffer);
	}
	if (reftf->tf_BPen != tf->tf_BPen)
		WriteBytes(CELL_BPEN, (UBYTE *)&tf->tf_BPen + 1, 3, &buffer);
	if (reftf->tf_NegativePen != tf->tf_NegativePen)
		WriteBytes(CELL_NEGPEN, (UBYTE *)&tf->tf_NegativePen + 1, 3, &buffer);

	/** pattern **/

	if (reftf->tf_Pattern != tf->tf_Pattern)
		WriteBytes(CELL_PATTERN, &tf->tf_Pattern, 1, &buffer);
	if (tf->tf_Pattern && (!reftf->tf_Pattern || reftf->tf_PatternColor != tf->tf_PatternColor))
		WriteBytes(CELL_PATTERNCOLOR, (UBYTE *)&tf->tf_PatternColor + 1, 3, &buffer);

	/** border **/
	{
		UBYTE i;

		for(i = 0;i < 4;i++)
		{
			if (reftf->tf_Border[i] != tf->tf_Border[i])
				WriteBytes(CELL_LEFTBORDER + i, &tf->tf_Border[i], 1, &buffer);
			if (tf->tf_Border[i] && reftf->tf_BorderColor[i] != tf->tf_BorderColor[i])
				WriteBytes(CELL_LBORDERCOLOR + i, ((UBYTE *)&(tf->tf_BorderColor[i])) + 1, 3, &buffer);
		}
	}
	/** misc **/

#ifdef __amigaos4__
//	if (tf->tf_Original)
#else
	if (tf->tf_Original)
#endif
	{
		if (reftf->tf_Komma != tf->tf_Komma)
			WriteBytes(CELL_POINT, &tf->tf_Komma, 1, &buffer);
		if (reftf->tf_Alignment != tf->tf_Alignment)
			WriteBytes(CELL_ALIGN, &tf->tf_Alignment, 1, &buffer);

	}
	if (reftf->tf_WidthSet != tf->tf_WidthSet)
	{
		if (tf->tf_WidthSet == (uint16)~0)
			WriteBytes(CELL_NOWIDTH, NULL, 0, &buffer);
		else
			WriteValue(CELL_BYTEWIDTH, tf->tf_WidthSet, &buffer);
	}

	/** font **/

	// does this cell have a font setting that has to be preserved?
	if (tf->tf_Original || tf->tf_Flags & TFF_FONTSET)
	{
		struct FontInfo *rfi = reftf->tf_FontInfo,*fi = tf->tf_FontInfo;
		BOOL referenceHasFont = reftf->tf_Original || (reftf->tf_Flags & TFF_FONTSET) != 0 || tag == TAG_Empty;
		
		// If the reference cell has no valid font setting or if its family
		// is different from this cell's family, go, save it
		if (!referenceHasFont || rfi->fi_Family != fi->fi_Family)
		{
			if (fullnames)
				WriteString(CELL_FONTNAME, fi->fi_Family->ln_Name, &buffer);
			else if (io_lastfont != fi->fi_Family)
			{
				struct NumberLink *nl;

				if ((nl = FindLink(&io_fonts, fi->fi_Family)) != 0)
				{
					font = fi->fi_Family;
					WriteValue(CELL_BYTEFONT, nl->nl_Number, &buffer);
				}
			}
			else
				WriteBytes(CELL_LASTFONT, NULL, 0, &buffer);
		}
		// if the reference cell doesn't have the attribute, only save if it's different
		// from the standard setting
		if (!referenceHasFont && fi->fi_FontSize->fs_PointHeight != page->pg_PointHeight
			|| rfi->fi_FontSize->fs_PointHeight != fi->fi_FontSize->fs_PointHeight)
			WriteBytes(CELL_POINTHEIGHT, &fi->fi_FontSize->fs_PointHeight, 4, &buffer);

		if (!referenceHasFont && fi->fi_Style != FS_PLAIN || rfi->fi_Style != fi->fi_Style)
			WriteBytes(CELL_STYLE, &fi->fi_Style, 4, &buffer);

		/* MERKER: da fehlt noch einiges (Shear, Rotate, Kerning, ...) */
	}

	/** format **/

	if ((tf->tf_Original || (tf->tf_Flags & TFF_FORMATSET) != 0) && reftf->tf_Format != tf->tf_Format)
	{
		struct NumberLink *nl;
		//bug("write: (%ld:%ld) \"%s\" (text = \"%s\"), %s\n", tf->tf_Col, tf->tf_Row, tf->tf_Format, tf->tf_Original, fullnames ? "full names" : "links");
		if (tf->tf_Format && fullnames)
			WriteString(CELL_FORMATNAME, tf->tf_Format, &buffer);
		else if (tf->tf_Format && (nl = (APTR)MyFindName(&io_fvs, tf->tf_Format)))
			WriteValue(CELL_BYTEFORMAT, nl->nl_Number, &buffer);
		else
			WriteBytes(CELL_NOFORMAT,NULL, 0, &buffer);
	}

	/** text/formula **/

	if (tf->tf_Original && !(tag & TAG_WithText))
	{
		if (tf->tf_Type & TFT_FORMULA)
		{
			tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
			WriteString(CELL_FORMULA, StaticTreeTerm(tf->tf_Root, false), &buffer);
		}
		else
		{
			WriteString(CELL_TEXT, tf->tf_Original, &buffer);
		}
	}
	if ((!reftf->tf_Note && tf->tf_Note) || (reftf->tf_Note && !tf->tf_Note) || (reftf->tf_Note && strcmp(reftf->tf_Note, tf->tf_Note)))
		WriteString(CELL_NOTE, tf->tf_Note, &buffer);

	*_pos = buffer - sc_buffer;
	return font;
}


int32
SaveCells(struct IFFHandle *iff, struct Page *pg, ULONG handle, int32 mode)
{
	struct tableField *tf, **refs, *ntf;
	long   error = 0L, max, min, current, len;
	struct Node *aboveFont, *leftFont;
	uint8  l, *buffer, first = TRUE;
	bool fullNames = (mode & IO_SAVE_FULL_NAMES) != 0;

	// create buffer to be able to save a complete row of cells
	{
		struct CellIterator *ci = GetCellIteratorStruct(handle);

		if (!(refs = AllocPooled(pool, sizeof(APTR) * ((max = ci->ci_MaxCol) + 1 - (min = ci->ci_Col)))))
			return IFFERR_NOMEM;
	}
	if (!(ntf = MakeTableField(pg, 0, 0)))
	{
		FreePooled(pool, refs, sizeof(APTR) * (max + 1 - min));
		return IFFERR_NOMEM;
	}
	sc_size = 0;  sc_aboveBuffer = NULL;  sc_leftBuffer = (APTR)~0L;
	calcflags |= CF_SHORTFUNCS;

	while (!error && (tf = NextCell(handle)) != NULL)
	{
		// don't save protected cells if we're not allowed to
		if ((mode & IO_IGNORE_PROTECTED_CELLS) != 0 && (tf->tf_Flags & TFF_HIDEFORMULA) != 0)
			continue;

		if (first && (error = PushChunk(iff, 0, ID_CELL, IFFSIZE_UNKNOWN)))
			break;

		first = FALSE;
		current = tf->tf_Col - min;

		if (current > 0 && refs[current - 1])
		{
			leftFont = MakeCellStream(pg, tf, TAG_FromLeft, refs[current - 1], sc_leftBuffer, &sc_leftPos, fullNames);
			l = TRUE;
		}
		else
			l = FALSE;

		aboveFont = MakeCellStream(pg, tf, refs[current] ? TAG_FromAbove : TAG_Empty,
			refs[current] ? refs[current] : ntf, sc_aboveBuffer, &sc_abovePos, fullNames);

		if (refs[current])
		{
			if (l && sc_leftPos < sc_abovePos)
				aboveFont = MakeCellStream(pg, tf, TAG_Empty, ntf, sc_aboveBuffer, &sc_abovePos, fullNames);
			else
				leftFont = MakeCellStream(pg, tf, TAG_Empty, ntf, sc_leftBuffer, &sc_leftPos, fullNames);
		}
		if (l && sc_leftPos < sc_abovePos)
			sc_left++, buffer = sc_leftBuffer,  len = sc_leftPos, io_lastfont = leftFont;
		else
			sc_above++, buffer = sc_aboveBuffer,  len = sc_abovePos, io_lastfont = aboveFont;

		refs[current] = tf;

#ifdef DUMP_CELLS
		bug("new: %lx (%s) - %ld:%ld\n", *buffer, *buffer & TAG_FromAbove ? "above" : (*buffer & TAG_FromLeft ? "left" : "new"),tf->tf_Col,tf->tf_Row);
		dump("  ", buffer, len);
#endif
		if (WriteChunkBytes(iff, buffer, len) != len)
			error = IFFERR_WRITE;
	}
	if (!first && !error)
		error = PopChunk(iff);

	calcflags &= ~CF_SHORTFUNCS;

	if (sc_aboveBuffer) {
		FreePooled(pool, sc_aboveBuffer, sc_size * 2);
		sc_aboveBuffer = NULL;
	}

	FreeTableField(ntf);
	FreePooled(pool, refs, sizeof(APTR) * (max + 1 - min));

	return error;
}


static int32
SaveObject(struct IFFHandle *iff, struct gObject *go)
{
	long error;

	if ((error = PushChunk(iff, 0, ID_OBJ, IFFSIZE_UNKNOWN)) != 0)
		return error;

	gDoMethod(go, GCM_SAVE, iff, &io_fonts);

	return PopChunk(iff);
}


static int32
SaveObjects(struct IFFHandle *iff, struct Page *page)
{
	struct gObject *go;
	long   error = 0L;

	/* MERKER: gGroup muß irgendwie berücksichtigt werden... */
	/*foreach(&page->pg_gGroups,gg)
		bug("ggroup: (%lx)\n",gg);*/

	/* Diagramme */

	foreach (&page->pg_gDiagrams, go)
		SaveObject(iff, go);

	/* normale Objekte */

	foreach(&page->pg_gObjects, go)
		SaveObject(iff, go);

	return error;
}


static int32
SavePage(struct IFFHandle *iff,struct Page *pg)
{
	long error;

	if ((error = PushChunk(iff, ID_PAGE, ID_FORM, IFFSIZE_UNKNOWN)) != 0)
		return error;

	if (!(error = PushChunk(iff,0,ID_PGHD,IFFSIZE_UNKNOWN)))
	{
		struct NumberLink *nl;
		long   i,j;

		WriteChunkString(iff,pg->pg_Node.ln_Name);
		WriteChunkBytes(iff,(UBYTE *)&pg->pg_APen+1,3);
		WriteChunkBytes(iff,(UBYTE *)&pg->pg_BPen+1,3);
		WriteChunkBytes(iff,&pg->pg_Zoom,4);

		if ((nl = (APTR)MyFindName(&io_fonts, pg->pg_Family->ln_Name)) != 0)
		{
			j = nl->nl_Number;
			WriteChunkBytes(iff,&j,4);
		}
		else
			WriteChunkBytes(iff,&nl,4);   // null

		io_lastfont = pg->pg_Family;
		WriteChunkBytes(iff,&pg->pg_PointHeight,4);

		/***  write field size (columns) ***/

		WriteChunkBytes(iff,&pg->pg_mmStdWidth,4);
		for(i = 0;i < pg->pg_Cols;i++)
		{
			if (pg->pg_mmStdWidth != (pg->pg_tfWidth+i)->ts_mm)
			{
				j = i+1;
				WriteChunkBytes(iff,&j,4);
				WriteChunkBytes(iff,&(pg->pg_tfWidth+i)->ts_mm,4);
			}
		}
		j = 0;  WriteChunkBytes(iff,&j,4);

		/***  write field size (rows) ***/

		WriteChunkBytes(iff,&pg->pg_mmStdHeight,4);
		for(i = 0;i < pg->pg_Rows;i++)
		{
			if (pg->pg_mmStdHeight != (pg->pg_tfHeight+i)->ts_mm)
			{
				j = i+1;
				WriteChunkBytes(iff,&j,4);
				WriteChunkBytes(iff,&(pg->pg_tfHeight+i)->ts_mm,4);
			}
		}
		j = 0;  WriteChunkBytes(iff,&j,4);

		/***  write field titles (columns) ***/

		{
			long colcount = 0;
			long rowcount = 0;

			for(i = 0;i < pg->pg_Cols;i++)
			{
				if ((pg->pg_tfWidth+i)->ts_Title)
				{
					j = i+1;
					WriteChunkBytes(iff,&j,4);
					WriteChunkString(iff,(pg->pg_tfWidth+i)->ts_Title);
					colcount++;
				}
			}
			if (colcount)
			{
				j = 0;
				WriteChunkBytes(iff,&j,4);
			}

			/***  write field titles (rows) ***/

			for(i = 0;i < pg->pg_Rows;i++)
			{
				if ((pg->pg_tfHeight+i)->ts_Title)
				{
					if (!colcount)
					{
						j = 0;  WriteChunkBytes(iff,&j,4);
						colcount++;
					}

					j = i+1;
					WriteChunkBytes(iff,&j,4);
					WriteChunkString(iff,(pg->pg_tfHeight+i)->ts_Title);
					rowcount++;
				}
			}
			if (rowcount)
			{
				j = 0;
				WriteChunkBytes(iff,&j,4);
			}
		}
		error = PopChunk(iff);
	}
	if (error)
		return error;

	{
		ULONG handle;

		if ((handle = GetCellIterator(pg, NULL, FALSE)) != 0)
		{
#ifdef __amigaos4__
			error = SaveCells(iff, pg, handle, IO_STANDARD_SAVE|IO_SAVE_FULL_NAMES); //Sonst keine Speicherung von merkmalen einer Zelle
#else
			error = SaveCells(iff, pg, handle, IO_STANDARD_SAVE);
#endif
			FreeCellIterator(handle);
		}
	}
	if (!error)
		error = SaveObjects(iff,pg);
	if (!error)
		error = PopChunk(iff);

	return error;
}


static int32
SaveDatabases(struct IFFHandle *iff, struct Mappe *mp)
{
	struct Database *db;
	struct Field *fi;
	long error, i;
	UBYTE b;

	foreach (&mp->mp_Databases, db)
	{
		if ((error = PushChunk(iff, 0, ID_DB, IFFSIZE_UNKNOWN)) != 0)
			return error;

		WriteChunkString(iff, db->db_Node.ln_Name);
		WriteChunkString(iff, db->db_Content);
		i = FindListEntry(&mp->mp_Pages, (struct MinNode *)db->db_Page);
		WriteChunkBytes(iff, &i, sizeof(LONG));

		i = CountNodes(&db->db_Fields);
		WriteChunkBytes(iff, &i, sizeof(LONG));
		foreach(&db->db_Fields, fi)
		{
			WriteChunkBytes(iff, &fi->fi_Node.ln_Type, 1);
			WriteChunkString(iff, fi->fi_Special);
		}

		/*************************** Indizes *******************************/
		{
			struct Index *in;

			b = 0;
			if (!IsListEmpty((struct List *)&db->db_Indices))
			{
				b = 1;
				WriteChunkBytes(iff, &b, 1);
			}

			foreach(&db->db_Indices, in)
			{
				if (in == db->db_Index)
					b = 2;
				else
					b = 1;
				WriteChunkBytes(iff, &b, 1);
				WriteChunkString(iff, in->in_Node.ln_Name);
			}
			if (b)
			{
				b = 0;
				WriteChunkBytes(iff, &b, 1);
			}
		}

		/*************************** Masken *******************************/
		{
			struct MaskField *mf;
			struct Mask *ma;

			foreach (&mp->mp_Masks, ma)
			{
				if (stricmp(db->db_Node.ln_Name, ma->ma_Node.ln_Name))
					continue;

				if (!b)
				{
					b = 2;
					WriteChunkBytes(iff, &b, 1);
				}
				i = ma->ma_Page ? FindListEntry(&mp->mp_Pages, (struct MinNode *)ma->ma_Page) : 0;
				WriteChunkBytes(iff, &i, 4);

				i = CountNodes(&ma->ma_Fields);
				WriteChunkBytes(iff, &i, sizeof(LONG));

				foreach (&ma->ma_Fields, mf)
				{
					WriteChunkBytes(iff, &mf->mf_Col, 4);
					WriteChunkBytes(iff, &mf->mf_Row, 4);
					WriteChunkString(iff, mf->mf_Node.ln_Name);
				}
			}
		}

		/*************************** Filter *******************************/
		{
			struct Filter *filter;

			if (!IsListEmpty((struct List *)&db->db_Filters))
			{
				b = 3;
				WriteChunkBytes(iff, &b, 1);
			}

			foreach (&db->db_Filters, filter)
			{
				WriteChunkBytes(iff, &filter->fi_Node.ln_Type, 1);
				WriteChunkString(iff, filter->fi_Node.ln_Name);
				WriteChunkString(iff, filter->fi_Filter);
			}
		}

		if ((error = PopChunk(iff)) != 0)
			return error;
	}
	return 0L;
}


#ifdef __amigaos4__
static int32
SaveScripts(struct IFFHandle *iff, struct Mappe *mp)
{
	struct RexxScript *rxs;
	long error, i;
	UBYTE b;

	if(!IsMinListEmpty(&mp->mp_RexxScripts))
	{
		if ((error = PushChunk(iff, 0, ID_SCRIPT, IFFSIZE_UNKNOWN)) != 0)
			return error;

		i = CountNodes(&mp->mp_RexxScripts);
		WriteChunkBytes(iff, &i, sizeof(LONG));
		foreach(&mp->mp_RexxScripts, rxs)
		{
			WriteChunkString(iff, rxs->rxs_Node.ln_Name);		    
			WriteChunkString(iff, rxs->rxs_Description);
			WriteChunkString(iff, rxs->rxs_Data);
			WriteChunkBytes(iff, &rxs->rxs_DataLength, 4);
		}

		if ((error = PopChunk(iff)) != 0)
			return error;
	}
	return 0L;
}
#endif

static long ASM
StandardSaveProject(REG(d0, BPTR dat), REG(a0, struct Mappe *mp))
{
	struct IFFHandle *iff;
	long   error;

	if (!(iff = AllocIFF()))
		return(RC_FAIL);

	InitIFFasDOS(iff);
	iff->iff_Stream = (IPTR)dat;

	if (!(error = OpenIFF(iff,IFFF_WRITE)))
	{
		if (!(error = PushChunk(iff,ID_TABL,ID_FORM,IFFSIZE_UNKNOWN)))
		{
			struct Page *pg;

			if (!(error = PushChunk(iff,0,ID_MAP,IFFSIZE_UNKNOWN)))
			{
				UBYTE  i;

				WriteChunkBytes(iff,&mp->mp_mmWidth,sizeof(ULONG)*2);
				WriteChunkBytes(iff,&mp->mp_Flags,sizeof(ULONG));

				/** Author/Version/... **/

				if (mp->mp_Author || mp->mp_Version || mp->mp_CatchWords || mp->mp_Note)
				{
					i = 254;
					WriteChunkBytes(iff,&i,1);
					WriteChunkString(iff,mp->mp_Author);
					WriteChunkString(iff,mp->mp_Version);
					WriteChunkString(iff,mp->mp_CatchWords);
					WriteChunkString(iff,mp->mp_Note);
				}

				/** Printer flags and page borders **/

				i = 253;
				WriteChunkBytes(iff,&i,1);
				WriteChunkBytes(iff,&mp->mp_PrinterFlags,4);
				WriteChunkBytes(iff,&mp->mp_BorderLeft,16);

				/** Passwords **/

				if (mp->mp_Password || mp->mp_CellPassword)
				{
					char t[64];

					i = 252;
					WriteChunkBytes(iff,&i,1);
					CryptString(t,mp->mp_Password);
					WriteChunkString(iff,t);
					CryptString(t,mp->mp_CellPassword);
					WriteChunkString(iff,t);
				}

				/** Window position **/

				if (mp->mp_Window && mp->mp_Flags & MPF_SAVEWINPOS)
				{
					i = 255;
					WriteChunkBytes(iff,&i,1);
					WriteChunkBytes(iff,&mp->mp_Window->LeftEdge,2*sizeof(ULONG));

					/* MERKER: vielleicht noch etwas elegantere Berechnungen der inneren Breite?? */
				}

				/** events **/

				for (i = 0; i < NUM_EVENTS; i++)
				{
					if (!mp->mp_Events[i].ev_Command)
						continue;

					WriteChunkBytes(iff,&i,sizeof(UBYTE));
					WriteChunkBytes(iff,&mp->mp_Events[i].ev_Flags,sizeof(UBYTE));
					WriteChunkString(iff,mp->mp_Events[i].ev_Command);
				}
				if (!error)
					error = PopChunk(iff);
			}

			/************ Fonts ************/
			if (!error && !(error = PushChunk(iff, 0, ID_FONTS,IFFSIZE_UNKNOWN)))
			{
				struct NumberLink *nl;

				foreach (&io_fonts, nl)
					WriteChunkString(iff, ((struct Node *)nl->nl_Link)->ln_Name);

				if (!error)
					error = PopChunk(iff);
			}

			/************ Prefs ************/

			if (!error)
				error = SaveProjectPrefs(iff, &mp->mp_Prefs);

			/************ Databases ************/

			if (!error)
				error = SaveDatabases(iff, mp);

#ifdef __amigaos4__
			/************ Rexx-Scripts ************/

			if(!error)
				error = SaveScripts(iff, mp);
#endif
			/************ Pages ************/

			foreach(&mp->mp_Pages, pg)
				SavePage(iff, pg);

			if (!error)
				error = PopChunk(iff);
		}
		if (error)
			ErrorRequest(GetString(&gLocaleInfo, MSG_SAVE_DOCUMENT_ERR), IFFErrorText(error));

		CloseIFF(iff);
	}
	FreeIFF(iff);

	return error;
}

#ifdef ENABLE_OLD_FORMAT

static long ASM
OldLoadProject(REG(d0, BPTR dat), REG(a0, struct Mappe *mp))
{
	struct tableField *tf;
	struct Page *page = NULL;
	struct MinList list,forms,cols;
	struct gObject *go;
	struct gGroup *gg = NULL;
	struct colorPen *cp;
	struct Database *db;
	struct Field *fi;
	struct Node *ln;
	char   t[BUFLEN],*s;
	ULONG  pen;
	long   i,a,b,c,mode = 3;

	MyNewList(&list);  MyNewList(&forms);  MyNewList(&cols);
	FGets(dat,t,BUFLEN);
	if (!strcmp(t,"#ign-pre\n"))
	{
		mp->mp_Flags &= ~MPF_UNNAMED;
		while(FGets(dat,t,BUFLEN))
		{
			t[strlen(t)-1] = 0;
			if (*t == '#')
			{
				if (!strcmp(t,"#fonts"))
					mode = 1;
				else if (!strcmp(t,"#table"))
					mode = 2;
				else if (!strcmp(t,"#object"))
					mode = 7;
				else if (!strcmp(t,"#objects"))
					mode = 6;
				else if (!strncmp(t,"#page",5))
				{
					mode = 0;
					if (page = NewPage(mp))
					{
						FreeString(page->pg_Node.ln_Name);
						page->pg_Node.ln_Name = AllocString(t+6);
					}
				}
				else if (!strncmp(t,"#formats",5))
					mode = 4;
				else if (!strncmp(t,"#colors",5))
					mode = 5;
				else
				{
					D(bug("unsupported chunk\n"));
					mode = -1;
				}
			}
			else
			{
				switch(mode)
				{
					case 0:  /* #page */
						s = t;
						if (sscanf(t,"v=%ld;h=%ld;z=%ld\n",&a,&b,&c))
						{
							page->pg_APen = lpGetColorPen(&cols,a);
							page->pg_BPen = lpGetColorPen(&cols,b);
							page->pg_Zoom = c*1024/100;
							page->pg_StdWidth = pixel(page,page->pg_mmStdWidth,TRUE);
							page->pg_StdHeight = pixel(page,page->pg_mmStdHeight,FALSE);
							if (!IsListEmpty((struct List *)&list))
								page->pg_Family = (struct Node *)((struct Node *)list.mlh_Head)->ln_Name;
							RecalcPageDPI(page);
						}
						else if (*t == 'w')
						{
							while(*s)
							{
								a = atol(s);
								while(*s++ != ':');
								b = atol(s);
								while(*s && *s++ != ';');
								if (a)
								{
									AllocTableSize(page,a,0);
									(page->pg_tfWidth+a-1)->ts_mm = b;
									(page->pg_tfWidth+a-1)->ts_Pixel = pixel(page,b,TRUE);
								}
								else
								{
									page->pg_mmStdWidth = b;
									page->pg_StdWidth = pixel(page,b,TRUE);
								}
							}
						}
						else if (*t == 'h')
						{
							while(*s)
							{
								a = atol(s);
								while(*s++ != ':');
								b = atol(s);
								while(*s && *s++ != ';');
								if (a)
								{
									AllocTableSize(page,0,a);
									(page->pg_tfHeight+a-1)->ts_mm = b;
									(page->pg_tfHeight+a-1)->ts_Pixel = pixel(page,b,FALSE);
								}
								else
								{
									page->pg_mmStdHeight = b;
									page->pg_StdHeight = pixel(page,b,FALSE);
								}
							}
						}
						break;
					case 1:     /* #fonts */
						if (ln = AllocPooled(pool,sizeof(struct Node)))
						{
							if (!(ln->ln_Name = (char *)MyFindName(&families, t)))
								ln->ln_Name = (char *)families.mlh_Head;
							MyAddTail(&list, ln);
						}
						break;
					case 2:     /* #table */
					{
						struct Node *ff = (APTR)~0L;
						ULONG  height = ~0L;
						LONG   style = ~0L;

						s = t;
						for(a = 0;isalpha(*s);s++)
							a = 26*a+*s-'a'+1;
						b = atol(s);
						if (tf = AllocTableField(page,a,b))
						{
							while(*s++ != ';');
							while(*s)
							{
								if (*s == 'v')
								{
									tf->tf_APen = lpGetColorPen(&cols,atol(s+2));
									tf->tf_ReservedPen = tf->tf_APen;
								}
								else if (*s == 'h')
									tf->tf_BPen = lpGetColorPen(&cols,atol(s+2));
								else if (*s == 'b')
								{
									s++;
									for(i = 0;i < 4;i++)
									{
										s++;
										if (sscanf(s,"%ld.%ld/",&a,&b) > 0)
										{
											tf->tf_BorderColor[i] = lpGetColorPen(&cols,a);
											tf->tf_Border[i] = b;
											while(*++s && *s != '/' && *s != ';');
										}
									}
								}
								else if (*s == 'f')
								{
									if (*++s == 'n' && (ln = FindListNumber(&list,atol(s+2))))
										ff = (struct Node *)ln->ln_Name;
									else if (*s == 's')
										height = atol(s+2) << 16;
									else if (*s == 'f')
										style = atol(s+2);
									else if (*s == '=' && (ln = FindListNumber(&forms,atol(s+1))))
									{
										tf->tf_Format = AllocString(ln->ln_Name);
										tf->tf_Flags |= TFF_FORMATSET;
									}
								}
								else if (*s == 'n')
								{
									tf->tf_Komma = atol(s+2);
									tf->tf_Flags |= TFF_KOMMASET;
								}
								else if (*s == 'a')
									tf->tf_Alignment = atol(s+2);
								else if (*s == 'w')
									tf->tf_WidthSet = atol(s+2);
								else if (*s == 't')
								{
									tf->tf_Text = AllocString(s+2);
									*s = 0;
								}
								while(*s && *s++ != ';');
							}
							tf->tf_FontInfo = SetFontInfo(tf->tf_FontInfo,page->pg_DPI,FA_Family,      ff,
																																				 FA_PointHeight, height,
																																				 FA_Style,       style,
																																				 TAG_END);
							ioUpdateTFText(page,tf);
						}
						break;
					}
					case 3:  /* Mappenübergreifendes */
						s = t;
						sscanf(s,"s=%lu:%lu",&mp->mp_mmWidth,&mp->mp_mmHeight);
						if (sscanf(s,"f=%lu",&a) > 0)
							mp->mp_Flags = a;
						if (*s == 'e')
						{
							s++;
							for(a = 0;a < NUM_EVENTS;a++)
							{
								s++;
								for(b = 0;*(s+b) && *(s+b) != ';';b++);
								if (b && (mp->mp_Events[a].ev_Command = AllocPooled(pool,b)))
								{
									if (*s == '+') mp->mp_Events[a].ev_Flags |= EVF_ACTIVE;
									//if (*s == '$') mp->mp_Events[a].ev_Type = EVT_EXTERN;
									strncpy(mp->mp_Events[a].ev_Command,s+1,b-1);
									s += b;
								}
							}
						}
						else if (*s == 'n')  /* names */
						{
							while(*s && *s++ != ';');
							a = s-1-t;  b = 0;  c = 0;
							while(*s)
							{
								if (*s == 't')
									b = atol(s+2);
								else if (*s == 'p')
									c = atol(s+2);
								else if (*s == 'c')
									break;
								while(*s && *s++ != ';');
							}
							t[a] = 0;
							AddName(&mp->mp_Prefs.pr_Names,t+2,*s == 'c' ? s+2 : NULL,b | NMT_UNDEFINED,(struct Page *)c);
							AddPrefsModuleToLocalPrefs(mp,WDT_PREFNAMES);
						}
						else if (*s == 'd')  /* database */
						{
							while(*s && *s++ != ';');
							/*a = s-1-t;*/  b = 0;  c = 0;
							*(s-1) = 0;  a = 0;
							if (db = AllocPooled(pool,sizeof(struct Database)))
							{
								db->db_Node.ln_Name = AllocString(t+2);
								db->db_Node.ln_Type = NMT_DATABASE;
								db->db_PageNumber = ~0L;
								MyNewList(&db->db_Fields);
								MyNewList(&db->db_Indices);
								MyNewList(&db->db_Filters);
								while(*s)
								{
									if (*s == 'p')
										db->db_PageNumber = (atol(s+2));
									else if (*s == 'c')
									{
										a = 1;
										b = (long)(s-t+2);
									}
									else if (*s == 'f')
									{
										a = 2;
										b = (long)(s-t+2);
									}
									while(*s && *s++ != ';');
									if (a == 1)
									{
										if (*s)
											*(s-1) = 0;
										a = 0;
										db->db_Content = AllocString(t+b);
									}
									else if (a == 2)
									{
										if (fi = AllocPooled(pool,sizeof(struct Field)))
										{
											fi->fi_Node.ln_Type = atol(t+b);
											if (*(t+1+b) == '.')
											{
												if (*s)
													*(s-1) = 0;
												fi->fi_Special = AllocString(t+2+b);
											}
											MyAddTail(&db->db_Fields, fi);
										}
										b = (long)(s-t);
									}
								}
								MyAddTail(&mp->mp_Databases, db);
							}
						}
						else if (*s == 'i')  /* index */
						{
							struct Index *in;

							if ((db = (APTR)FindListNumber(&mp->mp_Databases,atol(t+2))) && (in = AllocPooled(pool,sizeof(struct Index))))
							{
								while(*s && *s++ != ';');
								while(*s)
								{
									if (*s == '.')
										db->db_Index = in, s++;
									for(a = 0;*(s+a) && *(s+a++) != ';';);
									in->in_Node.ln_Name = AllocStringLength(s,a-(*(s+a) ? 1 : 0));
									s += a;
								}
								MyAddTail(&db->db_Indices, in);
							}
						}
						else if (*s == 'm')  /* mask */
						{
							struct MaskField *mf;
							struct Mask *ma;

							while(*s && *s++ != ';');
							if ((db = (APTR)FindListNumber(&mp->mp_Databases,atol(t+2))) && (ma = AllocPooled(pool,sizeof(struct Mask))))
							{
								ma->ma_Node.ln_Name = AllocString(db->db_Node.ln_Name);
								ma->ma_Page = (struct Page *)~0L;
								MyNewList(&ma->ma_Fields);
								a = 0;  // Modus 0: Maske, 1: Maskenfeld
								b = 0;  c = 0;
								while(*s)
								{
									if (!a)
									{
										if (*s == 'p')
											ma->ma_Page = (struct Page *)(atol(s+2));
										else if (*s == 'f')
										{
											a = 1;
											b = (long)(s-t+2);
										}
									}
									while(*s && *s++ != ';');
									if (a == 1)
									{
										if (mf = AllocPooled(pool,sizeof(struct MaskField)))
										{
											String2Coord(t+b,(long *)&mf->mf_Col,(long *)&mf->mf_Row);
											while(*(t+b) && *(t+b++) != '.');
											if (*(t+b))
											{
												if (*s)
													*(s-1) = 0;
												mf->mf_Node.ln_Name = AllocString(t+b);
											}
											MyAddTail(&ma->ma_Fields,mf);
										}
										b = (long)(s-t);
									}
								}
								MyAddTail(&mp->mp_Masks, ma);
							}
						}
						break;
					case 4:  /* #formats */
						for(s = t;*s != 'f';s++)
							while(*s != ';') s++;
						if (ln = AllocPooled(pool,sizeof(struct Node)))
						{
							ln->ln_Name = AllocString(s+2);
							MyAddTail(&forms, ln);
						}
						if (!MyFindName(&prefs.pr_Formats, s + 2) && !MyFindName(&mp->mp_Prefs.pr_Formats, s + 2))
						{
							i = s+2-t;  s = t;  a = 1;  b = -1;  c = -1;  pen = ~0L;
							while(*s)
							{
								if (*s == 't')
									a = atol(s+2);
								else if (*s == 'k')
									b = atol(s+2);
								else if (*s == 'a')
									c = atol(s+2);
								else if (*s == 'n')
									stch_l(s+2,(long *)&pen);
								else if (*s == 'f')
									*s = 0;
								while(*s && *s++ != ';');
							}
							AddFormat(&mp->mp_Prefs.pr_Formats,t+i,0,b,c,lpGetColorPen(&cols,pen),ITA_NONE,a);
						}
						break;
					case 5:  /* #colors */
						s = t;
						stch_l(s,(long *)&a);
						while(*s && *s++ != '/');
						stch_l(s,(long *)&b);
						while(*s && *s++ != '/');
						stch_l(s,(long *)&c);
						while(*s && *s++ != ';');
						if (!(cp = lpFindColorPen(a,b,c)))
							cp = AddColor(&colors,*s ? s : NULL,a,b,c);

						if (cp && (ln = AllocPooled(pool,sizeof(struct Node))))
						{
							/*bug("color: %s (%ld/%ld/%ld)\n",cp->cp_Node.ln_Name,cp->cp_Red,cp->cp_Green,cp->cp_Blue);*/
							ln->ln_Name = (STRPTR)cp->cp_ID;
							MyAddTail(&cols,ln);
						}
						break;
					case 7:   /* #object */
						if (!strcmp(t,"f"))
						{

							/* create a group */

						}
						else if (!strcmp(t,"/f"))
							gg = NULL;
						else if (go = OldLoadGObject(page,t,&list,&cols))
							AddGObject(page,gg,go,ADDREM_NONE);
						break;
				}
			}
		}
	}

	while ((ln = MyRemHead(&list)) != 0)
		FreePooled(pool,ln,sizeof(struct Node));
	while ((ln = MyRemHead(&cols) != 0)
		FreePooled(pool,ln,sizeof(struct Node));
	while ((ln = MyRemHead(&forms) != 0)
	{
		FreeString(ln->ln_Name);
		FreePooled(pool,ln,sizeof(struct Node));
	}

	return RC_OK;
}

#endif  // ENABLE_OLD_FORMAT

const APTR io_functable[] = {
	/*  0 */ ErrorRequestA,
	/*  1 */ NewPage,
	/*  2 */ RecalcPageDPI,
	/*  3 */ AllocTableField,
	/*  4 */ ioUpdateTFText,
	/*  5 */ AllocTableSize,
	/*  6 */ FindColorPen,
	/*  7 */ ioAddPen,
	/*  8 */ AbsCoord2String,
	/*  9 */ pixel,
	/* 10 */ mm,
	/* 11 */ AllocStringLength,
	/* 12 */ AllocString,
	/* 13 */ FreeString,
	/* 14 */ ita, //ioita,
	/* 15 */ ChangeFontInfoA,
	/* 16 */ AddName,
	NULL
};  /* to be continued */

#ifdef __amigaos4__
void InitIOType(struct IOType *io)
{
	BOOL * ASM (*initIOSegment)(REG(a0, APTR), REG(a1, APTR *), REG(a2, APTR), REG(a3, APTR), REG(a6, APTR), REG(d0, APTR), REG(d1, APTR), REG(d2, APTR), REG(d3, APTR), REG(d4, APTR), REG(d5, APTR), REG(d6, long));
	BPTR dir,olddir;//,segment;
	Elf32_Handle elfhandle;
	Elf32_Handle filehandle;
	struct Elf32_SymbolQuery query;

	if (io->io_Segment || !io->io_Filename)
		return;

	if ((dir = Lock(CLASSES_PATH,ACCESS_READ)) != 0)
	{
		olddir = SetCurrentDir(dir);
		if ((io->io_Segment = LoadSeg(io->io_Filename)) != 0)
		{
			//Get ELF handler
			GetSegListInfoTags(io->io_Segment,GSLI_ElfHandle,&elfhandle,TAG_DONE);
	
			if (elfhandle != NULL) {//Find the ELF handler?
				//Reopen the ELF file
				if((filehandle = OpenElfTags(OET_ElfHandle,elfhandle,TAG_DONE))) {
					//Check version first
					query.Flags = ELF32_SQ_BYNAME;
					query.Name = "Version";
					if (SymbolQuery(filehandle, 1, &query) != 0) {
						if ((*(LONG*)query.Value) == 1) {//If the version is 1 then this plugin is compatible
							//Let's query for the symbol name "Operator"
							query.Name = "InitModule";
							if (SymbolQuery(filehandle, 1, &query) != 0) {
								//Close the opened ELF file
								CloseElfTags(filehandle, CET_CloseInput, TRUE, TAG_DONE);
								//Store the address of the Function
								initIOSegment = (void*)query.Value;
								if (!initIOSegment((APTR)io, (APTR)io_functable, (APTR)pool, (APTR)IExec, (APTR)IDOS, (APTR)IUtility, (APTR)ILocale, (APTR)IIntuition, (APTR)IGadTools, NULL, NULL, MAKE_ID('I','G','N',0)))
									UnLoadSeg(io->io_Segment);
							}
							else
								UnLoadSeg(io->io_Segment);
						}
						else
							UnLoadSeg(io->io_Segment);
					}
					else
						UnLoadSeg(io->io_Segment);
				}
				else
					UnLoadSeg(io->io_Segment);
			}
		}
		else
			ErrorRequest(GetString(&gLocaleInfo, MSG_ADD_ON_NOT_FOUND_ERR),io->io_Node.ln_Name,io->io_Filename);
		SetCurrentDir(olddir);
		UnLock(dir);
	}
}
#else
void InitIOType(struct IOType *io)
{
	BOOL * ASM (*initIOSegment)(REG(a0, APTR), REG(a1, APTR *), REG(a2, APTR), REG(a3, APTR), REG(a6, APTR),
		REG(d0, APTR), REG(d1, APTR), REG(d2, APTR), REG(d3, APTR), REG(d4, long));
	BPTR dir,olddir,segment;

	if (io->io_Segment || !io->io_Filename)
		return;

	if ((dir = Lock(CLASSES_PATH,ACCESS_READ)) != 0)
	{
		olddir = CurrentDir(dir);
		if ((segment = LoadSeg(io->io_Filename)) != 0)
		{
			initIOSegment = MKBADDR(segment) + sizeof(APTR);
			if (initIOSegment(io, io_functable, pool, DOSBase,
					SysBase, MathIeeeDoubBasBase, MathIeeeDoubTransBase, UtilityBase, LocaleBase, MAKE_ID('I','G','N',0)))
				io->io_Segment = segment;
			else
				UnLoadSeg(segment);
		}
		else
			ErrorRequest(GetString(&gLocaleInfo, MSG_ADD_ON_NOT_FOUND_ERR),io->io_Node.ln_Name,io->io_Filename);
		CurrentDir(olddir);
		UnLock(dir);
	}
}
#endif

static bool
BuildGObjectOrder(struct Page *page)
{
	struct gObject **gos;
	struct gObject *go;
	int    i;

	page->pg_NumObjects = CountNodes(&page->pg_gObjects);

	if (!page->pg_NumObjects)
		return true;

	if (!(gos = AllocPooled(pool,sizeof(APTR)*page->pg_NumObjects)))
		return false;

	page->pg_OrderSize = page->pg_NumObjects;  // tatsächliche Größe des Arrays
	page->pg_ObjectOrder = gos;

	/** alle Positionen wieder herstellen **/

	i = 0;
	foreach(&page->pg_gObjects,go)
	{
		if (go->go_Pos == -1)
			continue;

		gos[go->go_Pos] = go;
		i++;
	}

	/** falls Objekte ohne Position übriggeblieben sind **/

	if (i == page->pg_NumObjects)
		return true;

	i = 0;
	foreach(&page->pg_gObjects,go)
	{
		if (go->go_Pos != -1)
			continue;

		while(gos[i]) i++;

		go->go_Pos = i;
		gos[i] = go;
	}
	return true;
}


int32
LoadProject(struct Mappe *mp, struct IOType *type)
{
	struct Database *db;
	struct tableField *tf;
	struct Field *fi;
	struct Mask *ma;
	struct Name *nm;
	struct IOType *io;
	char   t[256], s[128], c;
	long   rc, len, i;
	BYTE   fits, ascii;
	BPTR   dat;

	t[0] = 0;
	if (mp->mp_Path)
		strcpy(t, mp->mp_Path);
	AddPart(t, mp->mp_Node.ln_Name, 256);

	rc = RC_WARN;
	if ((dat = Open(t, MODE_OLDFILE)) != 0) {
		if ((len = FRead(dat, s, 1, 128)) != 0) {
			ascii = TRUE;
			for (i = 0; i < len; i++) {
#ifdef __amigaos4__
				if (!IsPrint(loc, c = *(s+i)) && (c < 32 || c > 126) && c != 10 && c != 9 && c != 13)  //because correct identification of ¤
#else
				if (!IsAlNum(loc, c = *(s+i)) && (c < 32 || c > 126) && c != 10 && c != 9 && c != 13) 
#endif
				{
					ascii = FALSE;
					break;
				}
			}

			foreach (&iotypes, io) {
#ifdef __amigaos4__
				fits = FALSE;
//				if(!Strnicmp(io->io_Suffix, &t[Strlen(t) - 3], 3))
//printf("Suffix=<%s> strchr=<%s> len=%d\n", io->io_Suffix, strchr(t, '.') + 1, Strlen(io->io_Suffix));
				if(!Strnicmp(io->io_Suffix, strrchr(t, '.') + 1, Strlen(io->io_Suffix)))
					fits = TRUE;
#else
				fits = TRUE;
				if (type != NULL && io != type)
					fits = FALSE;
				if (fits && !io->io_Load && io->io_Segment)
					fits = FALSE;

				if (fits && io->io_Pattern) {
					ParsePattern(io->io_Pattern, &t[128], 128);
					if (!MatchPattern(&t[128], t))
						fits = FALSE;
				}

				if (fits && io->io_Flags & IOF_ASCII)
					fits = ascii;

				if (fits && io->io_BytesUsed) {
					for (i = 0; i < 32; i++) {
						if ((io->io_BytesUsed & (1 << i)) && io->io_Bytes[i] != *(s+i)) {
							fits = FALSE;
							break;
						}
					}
				}
#endif
				if (fits && !io->io_Load)
					InitIOType(io);
				if (fits && io->io_Load) {
					struct Page *page;
					UWORD  oldcalcflags = calcflags;

					D(bug("Load-DataType %s.\n", io->io_Node.ln_Name));
#ifdef __amigaos4__
					ChangeFilePosition(dat, 0, OFFSET_BEGINNING);
#else
					Seek(dat, 0, OFFSET_BEGINNING);
#endif

					MyNewList(&io_fonts);
					MyNewList(&io_fvs);
					calcflags = (calcflags & ~CF_REQUESTER) | CF_SUSPEND; /* setzt Berechnung der Zellen aus */

					if (!(rc = io->io_Load(dat, mp))) {
						// initialize format template preferences
						if (!IsListEmpty((struct List *)&mp->mp_Prefs.pr_Formats))
						{
							struct PrefsModule *module = GetPrefsModule(&mp->mp_Prefs, WDT_PREFFORMAT);

							if (module && (module->pm_Flags & PMF_ADD_REPLACE_MASK) == PMF_REPLACE)
							{
								struct FormatVorlage *format;
								bool found = false;

								// check if the format list is anywhere near complete
								// (check for the "0" format, and add it if it's not there)
								foreach (&mp->mp_Prefs.pr_Formats, format) {
									if (!strcmp(format->fv_Node.ln_Name, "0")) {
										found = true;
										break;
									}
								}
								if (!found)
									AddFormat(&mp->mp_Prefs.pr_Formats, "0", 0, -1, -1, 0L, FVF_NONE, FVT_VALUE);
							}
							AddPrefsModuleToLocalPrefs(mp, WDT_PREFFORMAT);
						}

						RefreshMapPrefs(mp);

						foreach (&mp->mp_Pages, page)
						{
							page->pg_StdWidth = pixel(page, page->pg_mmStdWidth, TRUE);
							page->pg_StdHeight = pixel(page, page->pg_mmStdHeight, FALSE);

							RecalcTableSize(page);
							UpdateGGroups(page);

							if (!BuildGObjectOrder(page))
								ErrorRequest(GetString(&gLocaleInfo, MSG_INIT_OBJECT_ERR));

							{
								struct gObject *go;

								foreach (&page->pg_gDiagrams, go)
									gDoMethod(go, GCM_INITAFTERLOAD);

								foreach (&page->pg_gObjects, go)
									gDoMethod(go, GCM_INITAFTERLOAD);
							}

							foreach (&page->pg_Table, tf)
							{
								// Gespeichert werden nur die internen kurzen Funktionsnamen
								calcflags |= CF_SHORTFUNCS;  /* MERKER: sollte später wieder rein, gell? */
								UpdateCellText(page, tf);

								if (tf->tf_Type & TFT_FORMULA)
								{
									// Aber der Benutzer soll diese nach Möglichkeit nicht sehen;
									// die Bedeutung wird dadurch allerdings nicht geändert, eine
									// Neuberechnung muß also an dieser Stelle nicht durchgeführt
									// werden
									calcflags &= ~CF_SHORTFUNCS;
									FreeString(tf->tf_Original);
									tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
									tf->tf_Original = TreeTerm(tf->tf_Root, TRUE);
								}
							}
						}
						calcflags = oldcalcflags;

						UpdateMapTitle(mp);
						mp->mp_FileType = io;

						if (!(io->io_Flags & IOF_NODEFAULT))
							mp->mp_Flags &= ~MPF_UNNAMED;

						ObtainAppColors(scr, FALSE);

						foreach (&mp->mp_Prefs.pr_Names, nm) {
							if (nm->nm_PageNumber != ~0L) {
								nm->nm_Page = (struct Page *)FindListNumber(&mp->mp_Pages, nm->nm_PageNumber);
								nm->nm_Root = CreateTree(nm->nm_Page, nm->nm_Content);
							}
						}

						foreach (&mp->mp_Databases, db) {
							struct Filter *filter;
							struct Index *in;

							if (db->db_PageNumber != ~0L)
								db->db_Page = (struct Page *)FindListNumber(&mp->mp_Pages,db->db_PageNumber);

							db->db_Root = CreateTree(db->db_Page,db->db_Content);
							FillTablePos(&db->db_TablePos,db->db_Root);

							for (i = 0, fi = (APTR)db->db_Fields.mlh_Head; fi->fi_Node.ln_Succ; fi = (APTR)fi->fi_Node.ln_Succ, i++)
							{
								if ((tf = GetFields(db,i)) != 0)
									fi->fi_Node.ln_Name = AllocFieldText(db, tf, i);
							}

							foreach (&db->db_Indices, in)
								MakeIndex(db, in);        /* generate index data */
							
							foreach (&db->db_Filters, filter) {
								// create filter term (and replace short field names
								// with full database references)
								if ((filter->fi_Root = CreateTree(db->db_Page, filter->fi_Filter)) != 0)
									PrepareFilter(db, filter->fi_Root);

								// generate filter index & set current filter
								if (filter->fi_Node.ln_Type) {
									MakeFilter(db, filter);

									if (db->db_Filter == NULL)
										db->db_Filter = filter;
								}
							}
						}

						foreach(&mp->mp_Masks, ma)
						{
							if (ma->ma_Page != (APTR)~0L)
								ma->ma_Page = (struct Page *)FindListNumber(&mp->mp_Pages, (ULONG)ma->ma_Page);
							else
								ma->ma_Page = NULL;
						}
 
						// update database masks if necessary

						if (mp->mp_Flags & MPF_SCRIPTS) {
							foreach (&mp->mp_Databases, db) {
								UpdateDBCurrent(db, db->db_Current);
							}
						}

						if (!IsListEmpty((struct List *)&mp->mp_Pages))
						{
							SetMainPage(mp->mp_actPage = (struct Page *)mp->mp_Pages.mlh_Head);

							RecalcTableFields(rxpage);
							RecalcTableFields(rxpage);  /* MERKER: irgendwie ist das so nicht der Hit... */

							if ((prefs.pr_Flags & PRF_SECURESCRIPTS)
								&& (mp->mp_Flags & MPF_SCRIPTS)
								&& !FindInSession(mp)
								&& DoRequest(GetString(&gLocaleInfo, MSG_INTERACTIVE_MODE_IN_NEW_DOCUMENT_REQ),
										GetString(&gLocaleInfo, MSG_YES_NO_REQ), mp->mp_Node.ln_Name))
								mp->mp_Flags &= ~MPF_SCRIPTS;
						}
						if (mp->mp_CellPassword)
							mp->mp_Flags |= MPF_CELLSLOCKED;

						AddToSession(mp);
					}
					else
						calcflags = oldcalcflags;
					break;
				}
			}
			if (IsListEmpty((struct List *)&mp->mp_Pages))
				NewPage(mp);
		}
		Close(dat);
	}
	return rc;
}


static void
AddNumberLink(struct MinList *list, APTR link)
{
	struct NumberLink *nl;

	if ((nl = AllocPooled(pool, sizeof(struct NumberLink))) != 0)
	{
		if (IsListEmpty((struct List *)list))
			nl->nl_Number = 0;
		else
			nl->nl_Number = ((struct NumberLink *)list->mlh_TailPred)->nl_Number + 1;

		nl->nl_Link = link;
		MyAddTail(list, nl);
	}
}


int32
SaveProject(struct Mappe *mp, struct IOType *io, bool confirmOverwrite)
{
	struct tableField *tf;
	struct Page *page;
	struct Node *ln;
	char   t[256], *s;
	BPTR   dat;
	int32  rc;

	// if no type is selected yet, chose the one with the highest priority
	if (io == NULL)
		io = (struct IOType *)iotypes.mlh_Head;

	if ((io->io_Flags & IOF_WRITEABLE) == 0)
		return RC_WARN;
	
	// add suffix if required

	if (io->io_Suffix != NULL && !(prefs.pr_File->pf_Flags & PFF_NOSUFFIX)) {
		int32 nameLength = strlen(mp->mp_Node.ln_Name);
		const char *suffix = mp->mp_Node.ln_Name + nameLength - 1;
													
		// get old suffix, if any
		while (suffix > mp->mp_Node.ln_Name && isalnum(suffix[0]))
			suffix--;
		if (suffix <= mp->mp_Node.ln_Name || suffix[0] != '.')
			suffix = NULL;
		else
			suffix++;

		if (suffix == NULL || (suffix != NULL && strcmp(suffix, io->io_Suffix))) {
			// needs new suffix

			strcpy(t, mp->mp_Node.ln_Name);
			
			// if there already is a suffix, cut it up
			if (suffix != NULL)
				t[suffix - 1 - mp->mp_Node.ln_Name] = '\0';

			strcat(t, ".");
			strcat(t, io->io_Suffix);

			SetMapName(mp, t);
		}
	}

	t[0] = 0;
	if (mp->mp_Path)
		strcpy(t, mp->mp_Path);
	AddPart(t, mp->mp_Node.ln_Name, 256);

	// create backup if we should

	if ((prefs.pr_File->pf_Flags & PFF_BACKUP) && (s = AllocPooled(pool,strlen(t) + 5))) {
		strcpy(s, t);
		strcat(s, ".bak");
#ifdef __amigaos4__
		Delete(s);
#else
		DeleteFile(s);
#endif
		Rename(t, s);
		FreeString(s);
	}

	// Should we just overwrite the file?

	if (confirmOverwrite && (dat = Lock(t, ACCESS_READ))) {
		rc = DoRequest(GetString(&gLocaleInfo, MSG_FILE_EXISTS_ERR),GetString(&gLocaleInfo, MSG_YES_NO_REQ),t);
		UnLock(dat);

		if (rc == 0) // Die Datei soll nicht überschrieben werden
			return -1;
	}

	if (!io->io_Save && !io->io_Segment)
		InitIOType(io);
						 
	if (io->io_Save && (dat = Open(t, MODE_NEWFILE))) {
		struct gObject *go;
		struct Node *fv;

		/*************************** Listen vorbereiten *******************************/
		MyNewList(&io_fonts);  MyNewList(&io_fvs);
										
		// add map-specific formats
		foreach (&mp->mp_Prefs.pr_Formats, fv)
			AddNumberLink(&io_fvs, fv->ln_Name);

		foreach (&mp->mp_Pages, page) {
			foreach (&page->pg_Table, tf) {
				// add fonts used in the project
				if (tf->tf_Text && tf->tf_FontInfo && !FindLink(&io_fonts, tf->tf_FontInfo->fi_Family))
					AddNumberLink(&io_fonts, tf->tf_FontInfo->fi_Family);
												
				// add global formats used in the project
				if (tf->tf_Format && !MyFindName(&io_fvs, tf->tf_Format))
					AddNumberLink(&io_fvs, tf->tf_Format);
			}

			// also, add all the fonts used by the objects
			foreach (&page->pg_gObjects, go) {
				struct gInterface *gi;

				for (gi = go->go_Class->gc_Interface; gi->gi_Tag; gi++) {
					if (gi->gi_Type == GIT_FONT) {
						struct FontInfo *fi;

						if (GetGObjectAttr(go, gi->gi_Tag, (ULONG *)&fi)) {
#ifdef __amigaos4__
							if (FindListEntry(&io_fonts, (struct MinNode *)fi->fi_Family) == -1)
#else
							if (!FindListEntry(&io_fonts, (struct MinNode *)fi->fi_Family))
#endif
								AddNumberLink(&io_fonts, fi->fi_Family);
						}
					}
				}
			}
			// also, add all the fonts used by the diagrams
#ifdef __amigaos4__
			foreach (&page->pg_gDiagrams, go) {
				struct FontInfo *fi;
				struct gInterface *gi;
				struct gcpGet gs = {GCM_GET, GAA_FontInfo, (ULONG *)&fi};

				if(gDiagramDispatch(go->go_Class, (struct gDiagram *)go, (Msg)&gs) == 1)
				{
					if (FindListEntry(&io_fonts, (struct MinNode *)fi->fi_Family) == -1)
						AddNumberLink(&io_fonts, fi->fi_Family);
				}
			}
#endif
		}
//      SortFormatList((struct List *)&io_fvs);

		rc = io->io_Save(dat, mp);
		Close(dat);

		if (!rc)
			mp->mp_FileType = io;

		while ((ln = MyRemHead(&io_fonts)))
			FreePooled(pool, ln, sizeof(struct Node));
		while ((ln = MyRemHead(&io_fvs)))
			FreePooled(pool, ln, sizeof(struct Node));

		AddToSession(mp);

		if (prefs.pr_File->pf_Flags & PFF_ICONS) {
			struct DiskObject *dio;

			if ((dio = GetDiskObject(t)) != 0)  // is there an icon attached to the file?
				FreeDiskObject(dio);
			else {                       // Create a new one
				BPTR olddir,dir;
				STRPTR tool;
				char tpath[256];
				if ((dir = Lock(iconpath, SHARED_LOCK)) != 0) {
					char s[256];
#ifdef __amigaos4__
					olddir = SetCurrentDir(dir);
#else
					olddir = CurrentDir(dir);
#endif
					if ((dio = GetDiskObject("icons/def_ign")) != 0)
						tool = dio->do_DefaultTool;
#ifdef __amigaos4__
					SetCurrentDir(olddir);
#else
					CurrentDir(olddir);
#endif
					UnLock(dir);

					if (!dio) {
						if ((dio = GetDefDiskObject(WBPROJECT)) != 0) {
							tool = dio->do_DefaultTool;
							dio->do_DefaultTool = NULL;
						}
					}
					if (dio) {
						if (!dio->do_DefaultTool || !strlen(dio->do_DefaultTool)) {
#ifdef __amigaos4__
							if (GetCliCurrentDirName(s, 256)) 
#else
							if (GetCurrentDirName(s, 256)) 
#endif
							{
								AddPart(s, "ignition", 256);
								dio->do_DefaultTool = s;
							} else
								dio->do_DefaultTool = "ignition";
						}
						PutDiskObject(t,dio);
						dio->do_DefaultTool = tool;
						FreeDiskObject(dio);
					}
				}
			}
		}
		return rc;
	}
	return RC_WARN;
}


void
initStandardIOTypes(void)
{
	struct IOType *io;

	if ((io = AllocPooled(pool, sizeof(struct IOType))) != 0) {
		io->io_Node.ln_Name = AllocString(GetString(&gLocaleInfo, MSG_DEFAULT_FILE_FORMAT_NAME));
		io->io_Node.ln_Pri = 42;
		io->io_Flags = IOF_WRITEABLE | IOF_READABLE;
		io->io_BytesUsed = 0x00000f0f;
		strcpy(io->io_Bytes, "FORM????TABL");
		io->io_Load = StandardLoadProject;
		io->io_Save = StandardSaveProject;
		io->io_Short = "igs";
		io->io_Suffix = "igs";
		
		MakeLocaleStrings(&io->io_Description,
			MSG_IO_STANDARD_FORMAT_1_DESCR,
			MSG_IO_STANDARD_FORMAT_2_DESCR,
			TAG_END);

		MyEnqueue(&iotypes, io);
	}

#ifdef ENABLE_OLD_FORMAT
	if (io = AllocPooled(pool, sizeof(struct IOType))) {
		io->io_Node.ln_Name = AllocString("ignition-Text");
		io->io_Node.ln_Pri = 41;
		io->io_Flags = IOF_READABLE | IOF_NODEFAULT;
		io->io_BytesUsed = 0x000001ff;
		strcpy(io->io_Bytes, "#ign-pre\n");
		io->io_Load = OldLoadProject;
		io->io_Short = "ign";
		io->io_Suffix = "ign";

		MakeStrings(&io->io_Description,
			"ehemaliges Format in den ignition-Betas.",
			"Unterstützt nicht unbedingt alle Fähigkeiten.",
			NULL);
		
		MyEnqueue(&iotypes, io);
	}
#endif
}


static void
WriteIOTag(BPTR file, STRPTR tag, STRPTR data)
{
	if (!data)
		return;

	FPuts(file, tag);
	FPutC(file, '=');
	FPuts(file, data);
	FPutC(file, '\n');
}


static void UNUSED
SaveIOTypeDescription(struct IOType *io)
{
	BPTR dir,olddir;
	char *s;

	if (!io->io_GetPrefs || !io->io_Filename)
		return;

	s = AllocString(io->io_GetPrefs());

	if (!zstrcmp(s,io->io_Prefs)) {
		FreeString(s);
		return;
	}

	if ((dir = Lock(CLASSES_PATH, ACCESS_READ)) != 0) {
		char t[256];
		BPTR file;

#ifdef __amigaos4__
		olddir = SetCurrentDir(dir);
#else
		olddir = CurrentDir(dir);
#endif
		D(bug("save I/O prefs for \"%s\"\n", io->io_Filename));
		strcpy(t, io->io_Filename);
		strcat(t, "descr");

		if ((file = Open(t, MODE_NEWFILE)) != 0) {
			struct Node *ln;

			WriteIOTag(file, "NAME", io->io_Node.ln_Name);
			WriteIOTag(file, "SHORT", io->io_Short);

			// flags
			if (io->io_Flags & IOF_ASCII)
				FPuts(file, "ASCII\n");
			if (io->io_Flags & IOF_READABLE)
				FPuts(file, "READABLE\n");
			if (io->io_Flags & IOF_WRITEABLE) {
				FPuts(file, "WRITEABLE\n");
				if (io->io_Flags & IOF_NODEFAULT)
					FPuts(file, "NODEFAULT\n");
			}
			if (io->io_Flags & IOF_HASPREFSGUI)
				FPuts(file, "HASPREFSGUI\n");

			sprintf(t, "%ld", io->io_Node.ln_Pri);
			WriteIOTag(file, "PRI", t);

			WriteIOTag(file, "PATTERN", io->io_Pattern);
			WriteIOTag(file, "BYTES", io->io_OriginalBytes);
			WriteIOTag(file, "PREFS", s);

			FPuts(file, "--\n");

			foreach (&io->io_Description, ln) {
				FPuts(file, ln->ln_Name);
				if (ln->ln_Succ->ln_Succ)
					FPutC(file, '\n');
			}
		#if defined __MORPHOS__
			Flush(file);
		#else
			FFlush(file);
		#endif
			Close(file);
		}
#ifdef __amigaos4__
		SetCurrentDir(olddir);
#else
		CurrentDir(olddir);
#endif
		UnLock(dir);
	}
}


static void
LoadIOTypeDescription(struct IOType *io, BPTR file)
{
	STRPTR localizedNames[10];
	int language;
	long i, mode;
	char t[512], c = '?', *s = NULL;
	struct Node *ln;
	bool readDescription = false;

	for (i = 0; i < 10; i++)
		localizedNames[i] = NULL;

	mode = 0;

	while (FGets(file, t, 512)) {
		t[strlen(t) - 1] = '\0';
			// kill Linefeed

		if (!mode) {
			if (!strnicmp(t, "NAME", 4) && (t[4] == '=' || t[4] == ':'))
				SetLocalizedName(localizedNames, &io->io_Node.ln_Name, t + 4);
			else if (!strnicmp(t, "PATTERN=", 8))
				io->io_Pattern = AllocString(t + 8);
			else if (!stricmp(t, "ASCII"))
				io->io_Flags |= IOF_ASCII;
			else if (!stricmp(t, "READABLE"))
				io->io_Flags |= IOF_READABLE;
			else if (!stricmp(t, "WRITEABLE"))
				io->io_Flags |= IOF_WRITEABLE;
			else if (!stricmp(t, "NODEFAULT"))
				io->io_Flags |= IOF_NODEFAULT;
			else if (!stricmp(t, "HASPREFSGUI"))
				io->io_Flags |= IOF_HASPREFSGUI;
			else if (!strnicmp(t, "PRI=", 4))
				io->io_Node.ln_Pri = (BYTE)atol(t + 4);
			else if (!strnicmp(t, "SHORT=", 6))
				io->io_Short = AllocString(t + 6);
			else if (!strnicmp(t, "SUFFIX=", 7))
				io->io_Suffix = AllocString(t + 7);
			else if (!strnicmp(t, "READOVER=", 9))
				c = io->io_ReadOver = t[9];
			else if (!strnicmp(t, "BYTES=", 6))
				s = AllocString(t + 6);
			else if (!strnicmp(t, "PREFS=", 6))
				io->io_Prefs = AllocString(t + 6);
			else if (!strcmp(t, "--")) {
				GetLocalizedName(localizedNames, &io->io_Node.ln_Name, &language);
				if (language == -1 || !stricmp(loc->loc_PrefLanguages[language], "deutsch"))
					readDescription = true;

				mode = 1;
			}
		} else if (t[0] == ':') {
			// have we already read the description?
			if (readDescription)
				break;

			// check for language

			if (!stricmp(t + 1, loc->loc_PrefLanguages[language]))
				readDescription = true;
		} else if (t[0] == ' ' && readDescription && (ln = AllocPooled(pool, sizeof(struct Node)))) {
			if ((ln->ln_Name = AllocString(t + 1)) != 0)
				MyAddTail(&io->io_Description, ln);
			else
				FreePooled(pool, ln, sizeof(struct Node));
		}
	}
 
	if (s) {
		io->io_OriginalBytes = AllocString(s);

		for (i = 0; *s; s++, i++) {
			if (*s != c) {
				if (s[0] == 92 && s[1]) {
					// special commands
					s++;
					if (*s == 'n')
						io->io_Bytes[i] = '\n';
					else if (*s >= '0' && *s <= '9')
						io->io_Bytes[i] = *s-'0';
					else if (*s == 92)
						io->io_Bytes[i] = 92;
				} else
					io->io_Bytes[i] = *s;
				io->io_BytesUsed |= 1 << i;
			}
		}
	}
		
	if (io->io_Suffix == NULL)
		io->io_Suffix = AllocString(io->io_Short);

	if (!(io->io_Flags & IOF_WRITEABLE))
		io->io_Flags |= IOF_NODEFAULT;
}


void
closeIO(void)
{
	struct IOType *io;

	while ((io = (APTR)MyRemHead(&iotypes))) {
		//SaveIOTypeDescription(io);

		FreeString(io->io_Node.ln_Name);
		FreeString(io->io_Pattern);

		if (io->io_Segment)
			UnLoadSeg(io->io_Segment);

		FreePooled(pool, io, sizeof(struct IOType));
	}
}


void
initIO(void)
{
#ifdef __amigaos4__
	struct AnchorPath *ap;
#else
	struct AnchorPath ALIGNED ap;
#endif
	BPTR   dir,olddir,file;
	long   rc,i;
	struct IOType *io;

	MyNewList(&iotypes);
	initStandardIOTypes();
	if ((dir = Lock(CLASSES_PATH,ACCESS_READ)) != 0) {
#ifdef __amigaos4__
		olddir = SetCurrentDir(dir);

		ap = AllocDosObjectTags(DOS_ANCHORPATH, ADO_Mask, SIGBREAKF_CTRL_C, ADO_Strlen, 1024L, TAG_END ); 
		for (rc = MatchFirst("#?.iodescr",ap); !rc; rc = MatchNext(ap)) {
			if ((file = Open(ap->ap_Info.fib_FileName, MODE_OLDFILE)) != 0) {
				if ((io = AllocPooled(pool, sizeof(struct IOType))) != 0) {
					MyNewList(&io->io_Description);
					if ((io->io_Filename = AllocPooled(pool,i = strlen(ap->ap_Info.fib_FileName)-4)) != 0)
						CopyMem(ap->ap_Info.fib_FileName,io->io_Filename,i-1);
#else
		olddir = CurrentDir(dir);
		memset(&ap,0,sizeof(struct AnchorPath));
		for (rc = MatchFirst("#?.iodescr",&ap); !rc; rc = MatchNext(&ap)) {
			if ((file = Open(ap.ap_Info.fib_FileName, MODE_OLDFILE)) != 0) {
				if ((io = AllocPooled(pool, sizeof(struct IOType))) != 0) {
					MyNewList(&io->io_Description);
					if ((io->io_Filename = AllocPooled(pool,i = strlen(ap.ap_Info.fib_FileName)-4)) != 0)
						CopyMem(ap.ap_Info.fib_FileName,io->io_Filename,i-1);
#endif
					LoadIOTypeDescription(io,file);
					MyEnqueue(&iotypes, io);
				}
				Close(file);
			}
		}
#ifdef __amigaos4__
		MatchEnd(ap);
		FreeDosObject(DOS_ANCHORPATH,ap);
		SetCurrentDir(olddir);
#else
		MatchEnd(&ap);
		CurrentDir(olddir);
#endif
		UnLock(dir);
	}
}
