/* Tree and calculation functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"

#ifdef __amigaos4__
	#include <string.h>
	#include <clib/macros.h>
#endif
 
#define TREESIZE 2048

struct Term *createTree(STRPTR t, APTR *stack, ULONG *size);


struct Page *calcpage;
long tf_col, tf_row, mp_flags, gTextBufferLength;
STRPTR tf_format, gTextBuffer, itaPoint;
UWORD calcerr, calcflags = CF_REQUESTER;
long mday[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
#if defined(__AROS__) && !(AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
#warning FIXME when V1 ABI is out
struct FormatVorlage empty_fv = {{NULL, NULL, NULL, FVT_NONE, 0}, NULL, 0, 0, 0, 0};
#else
struct FormatVorlage empty_fv = {{NULL, NULL, FVT_NONE, 0, NULL}, NULL, 0, 0, 0, 0};
#endif
struct MinList flangs;
APTR tree_stack;
ULONG tree_size;


/** Entfernt alle Leerzeichen aus dem übergebenen String, solange sie
 *  nicht in Anführungszeichen enthalten sind.
 *  Gibt einen neuen String zurück, der von der aufrufenden Funktion
 *  nach Gebrauch freizugeben ist.
 */

STRPTR
RemoveSpaces(STRPTR st)
{
    STRPTR t;
    char   c,intext;
    long   i,l;

    for (i = 0, intext = FALSE, l = 0; (c = *(st + i)) != 0; i++)
    {
        if (c == intext)
            intext = 0;
        else if (!intext && (c == '"' || c == '\'' || c == '`'))
            intext = c;
        if (!intext && *(st+i) == ' ')
            l++;
    }
    if (!(t = AllocPooled(pool, strlen(st) - l + 1)))
        return NULL;

    for (i = 0, l = 0, intext = FALSE; (c = *(st+i)) != 0; *(t+l++) = *(st+i++))
    {
        if (c == intext)
            intext = 0;
        else if (!intext && (c == '"' || c == '\'' || c == '`'))
            intext = c;
        if (!intext)
            while (*(st+i) == ' ') i++;
        if (!*(st+i))
            break;
    }
    return t;
}


static bool
EnlargeTextBuffer(int32 pos)
{
    if (gTextBuffer == NULL)
    {
        gTextBuffer = AllocPooled(pool, gTextBufferLength = 256);
        return (bool)(gTextBuffer != 0);
    }

    if ((pos > 0 ? pos : strlen(gTextBuffer)) > gTextBufferLength - 92)
    {
        // enlarge buffer
        STRPTR t;

        if ((t = AllocPooled(pool, gTextBufferLength + 256)) != 0)
        {
            strcpy(t, gTextBuffer);
            FreePooled(pool, gTextBuffer, gTextBufferLength);
            gTextBufferLength += 256;
#ifdef __amigaos4__
			gTextBuffer = t;
#endif
            return true;
        }
    }
    else
        return true;

    return false;
}


void
Pos2String(long pos, STRPTR t)
{
    long i,j;

    for(j = 26,i = 0;pos>j;j += j*26,i++);
    *(t+i+1) = 0;
    for(;i>=0;i--,pos /= 26)
    {
        pos--;
        *(t+i) = (pos % 26)+97;
    }
}


/** Konvertiert eine Spalten-/Zeilenangabe zu einem String
 *  @note Diese Funktion ist nicht reentrant.
 */

STRPTR
Coord2String(long col, long row)
{
    static char t[16];

    Pos2String(col,t);
    sprintf(t+strlen(t),"%ld",row);
    return t;
}


void
TablePos2String(struct Page *page,struct tablePos *tp,STRPTR t)
{
    strcpy(t,Coord2String(tp->tp_Col,tp->tp_Row));
    if (tp->tp_Width || tp->tp_Height)
    {
        strcat(t,":");
        strcat(t,Coord2String(tp->tp_Width != -1 ? tp->tp_Col+tp->tp_Width : page->pg_Cols,tp->tp_Height != -1 ? tp->tp_Row+tp->tp_Height : page->pg_Rows));
    }
}


/** Konvertiert eine (absolute) Spalten-/Zeilenangabe zu einem String
 *  @note Diese Funktion ist nicht reentrant.
 */

STRPTR PUBLIC
AbsCoord2String(REG(d0, BOOL abscol), REG(d1, long col), REG(d2, BOOL absrow), REG(d3, long row))
{
    static char t[16];

    t[0] = 0;

    if (abscol)
        strcpy(t, "$");
    Pos2String(col, t + strlen(t));

    if (absrow)
        strcat(t, "$");
    sprintf(t + strlen(t), "%ld", row);

    return t;
}


long
String2Coord(STRPTR s, long *col, long *row)
{
    long abs = 0;

    if (!s)
        return 0;

    if (*s == '$')
    {
        abs = 1;
        s++;
    }

    for (*col = 0; isalpha(*s); s++)
#ifdef __amigaos4__
        *col = 26 * (*col) + ToLower(*s) - 'a' + 1;
#else
        *col = 26 * (*col) + *s - 'a' + 1;
#endif

    if (*s == '$')
    {
        abs |= 2;
        s++;
    }
    *row = atol(s);
    return abs;
}


struct Page *
GetExtCalcPage(struct Term *t)
{
    /*struct Mappe *mp;*/
    struct Page *page = NULL;

    if (!t || t->t_Op != OP_EXTCELL)
        return NULL;
    /*if (!t->t_Mappe)*/
    {
        if (t->t_Page)
            page = (struct Page *)MyFindName(&calcpage->pg_Mappe->mp_Pages, t->t_Page);
#ifdef __amigaos4__
        if (!page && t->t_NumPage != -1)
#else
        if (!page)
#endif
            page = (struct Page *)FindListNumber(&calcpage->pg_Mappe->mp_Pages,t->t_NumPage-1);
    }
/*    else
    {
        D(bug("not implemented\n"));
       if (mp = (struct Mappe *)MyFindName(&calcpage->pg_Mappe->mp_Projects,t->t_Mappe))
        {
            if (t->t_NumPage == -1)
                page = (struct Page *)MyFindName(&mp->mp_Pages,t->t_Page);
            else
                page = (struct Page *)FindListNumber(&mp->mp_Pages,t->t_NumPage);
        }
        if (mp = (struct Mappe *)FindListNumber(&calcpage->pg_Mappe->mp_Projects,t->t_Mappe-1))
            page = (struct Page *)FindListNumber(&mp->mp_Pages,t->t_Page-1);
    }
*/
    return page;
}


struct tableField *
GetExtTableField(struct Term *t)
{
    struct Page *page;

    if ((page = GetExtCalcPage(t)) != 0)
        return GetTableField(page,t->t_Col,t->t_Row);
	
    return NULL;
}


struct Name *
GetName(STRPTR t)
{
    if (!t)
        return NULL;

    /*if (!(nm = (struct Name *)FindLinkCommand(&calcpage->pg_Mappe->mp_Names,t)))
        nm = (struct Name *)FindCommand(&calcpage->pg_Mappe->mp_Databases,t);*/
   	return (struct Name *)FindLinkCommand(&calcpage->pg_Mappe->mp_Names, t);
}


bool
IsValidName(struct List *list, STRPTR t)
{
    struct Name *nm;
    STRPTR s;
    long   i = 0;

    if (!t)
        return false;

    if (!IsAlpha(loc,*t))
        i++;
    for (s = t;*s;s++)
    {
        if (!IsAlNum(loc,*s) && *s != '_')
            i++;
    }
    if (!IsAlpha(loc,*--s) && *s != '_')
        i++;
    if (i)
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_INVALID_NAME_ERR));
        return false;
    }
    if (!list)
        return true;

    i = 0;
    foreach (list, nm)
    {
        if (!stricmp(nm->nm_Node.ln_Name,t))
            i++;
    }
    if (i > 1)
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_NAME_IN_USE_ERR));
        return FALSE;
    }
    return TRUE;
}


long
GetRangeSize(struct Term *t)
{
    struct Term *tr,*tl;

    if (!t || (!(tl = t->t_Left)) || (!(tr = t->t_Right)))
        return 0L;

   return (tr->t_Col + (tr->t_AbsCol ? 0 : tf_col) + 1 - tl->t_Col - (tl->t_AbsCol ? 0 : tf_col))
        *(tr->t_Row + (tr->t_AbsRow ? 0 : tf_row) + 1 - tl->t_Row-(tl->t_AbsRow ? 0 : tf_row));
}


/* Für Schachtelungen von GetRangeCells() */

ULONG *grc_base,*grc_ci;
long  grc_size,grc_count;


void
FreeRangeCellsData(void)
{
    grc_count--;
    if (*grc_ci)
        FreeCellIterator(*grc_ci);
    grc_ci--;
}


void
AllocRangeCellsData(struct Page *page, struct tablePos *tp)
{
    if ((grc_count + 1) * sizeof(ULONG) > grc_size)
    {
        APTR temp;

        if ((temp = AllocPooled(pool, (grc_count + 2) * sizeof(ULONG))) != 0)
        {
            if (grc_base)
            {
                CopyMem(grc_base, temp, grc_size);
                FreePooled(pool, grc_base, grc_size);
            }
            grc_base = temp;
            grc_size = (grc_count + 2) * sizeof(ULONG);
        }
        else
            ErrorRequest(GetString(&gLocaleInfo, MSG_NO_MEMORY_FOR_NESTING_ERR));
    }
    grc_ci = grc_base + grc_count;

    *grc_ci = GetCellIterator(page, tp, FALSE);
    grc_count++;
}


struct tableField *
GetRangeCells(struct Term *t,struct tableField *tf)
{
    if (!tf)
    {
        struct Term *tl,*tr;
        struct tablePos tp;
        struct Page *page;

        if (!t)
        {
            return NULL;
        }
        if ((tl = t->t_Left) && (tr = t->t_Right) && (tl->t_Op != OP_CELL || tr->t_Op != OP_CELL))
        {
#ifdef __amigaos4__
            if (tl->t_Op == OP_EXTCELL && tr->t_Op == OP_EXTCELL && ((tl->t_Page != NULL && !strcmp(tl->t_Page,tr->t_Page)) || (tl->t_NumPage == tr->t_NumPage && tl->t_NumPage > -1)))
#else
            if (tl->t_Op == OP_EXTCELL && tr->t_Op == OP_EXTCELL && !stricmp(tl->t_Page,tr->t_Page))
#endif
            {
                if (tl->t_Col > tr->t_Col)
                    swmem((UBYTE *)&tl->t_Col, (UBYTE *)&tr->t_Col, sizeof(LONG));
                if (tl->t_Row > tr->t_Row)
                    swmem((UBYTE *)&tl->t_Row, (UBYTE *)&tr->t_Row, sizeof(LONG));

                tp.tp_Col = tl->t_Col;
                tp.tp_Row = tl->t_Row;
                tp.tp_Width = tr->t_Col-tl->t_Col;
                tp.tp_Height = tr->t_Row-tl->t_Row;
                page = GetExtCalcPage(tl);
#ifdef __amigaos4__
				if(page == NULL) //Unknown page!
				{
					calcerr = CTERR_ARGS;
					return NULL;
				}
#endif
            }
            else
                return NULL;
        }
        else if (FillTablePos(&tp,t))
        {
            page = calcpage;
        }
        else
        {
            return NULL;
        }
    	AllocRangeCellsData(page,&tp);
    }
    if (*grc_ci && (tf = NextCell(*grc_ci)))
    {
        if (tf->tf_Flags & TFF_LOCKED)
        {
            calcerr = CTERR_LOOP;
            return NULL; 
        }
        if (!tf_format)
        {
            tf_format = tf->tf_Format;
        }
    }
    else
    {
        FreeRangeCellsData();
    }
    return tf;
}


bool
PosInTablePos(struct tablePos *tp,ULONG col,ULONG row)
{
    if (!tp)
        return false;

    if (col >= tp->tp_Col && row >= tp->tp_Row && col <= tp->tp_Col+tp->tp_Width && row <= tp->tp_Row+tp->tp_Height)
        return true;

    return false;
}


bool
InTablePos(struct tablePos *tp,struct tablePos *itp)
{
    if (!tp || !itp)
        return false;

    if (tp->tp_Col >= itp->tp_Col && tp->tp_Row >= itp->tp_Row && tp->tp_Col+tp->tp_Width <= itp->tp_Col+itp->tp_Width && tp->tp_Row+tp->tp_Height <= itp->tp_Row+itp->tp_Height)
        return true;

    return false;
}


bool
FillTablePos(struct tablePos *tp,struct Term *t)
{
    struct Term *tl,*tr;
    struct Name *nm;

    if (!tp || !t)
        return false;

    if (t->t_Op == OP_NAME)
    {
        calcpage = rxpage;
        if ((nm = GetName(t->t_Text)) != 0)
            t = nm->nm_Root;
    }
    if (t->t_Op == OP_CELL)
    {
        tp->tp_Col = t->t_Col+(t->t_AbsCol ? 0 : tf_col);
        tp->tp_Row = t->t_Row+(t->t_AbsRow ? 0 : tf_row);
        if (tp->tp_Col < 0 || tp->tp_Row < 0)
            return false;

        tp->tp_Width = tp->tp_Height = 0;
    }
#ifdef __amigaos4__
    else if (t->t_Op == OP_RANGE && (tr = t->t_Right) && (tr->t_Op == OP_CELL || tr->t_Op == OP_EXTCELL) && (tl = t->t_Left) && (tl->t_Op == OP_CELL || tl->t_Op == OP_EXTCELL))
#else
    else if (t->t_Op == OP_RANGE && (tr = t->t_Right) && tr->t_Op == OP_CELL && (tl = t->t_Left) && tl->t_Op == OP_CELL)
#endif
    {
        if ((tl->t_AbsCol ? 0 : tf_col)+tl->t_Col > (tr->t_AbsCol ? 0 : tf_col)+tr->t_Col)
        {
            swmem((UBYTE *)&tl->t_Col, (UBYTE *)&tr->t_Col, sizeof(LONG));
            swmem(&tl->t_AbsCol, &tr->t_AbsCol, sizeof(BOOL));
        }
        if ((tl->t_AbsRow ? 0 : tf_row)+tl->t_Row > (tr->t_AbsRow ? 0 : tf_row)+tr->t_Row)
        {
            swmem((UBYTE *)&tl->t_Row,(UBYTE *)&tr->t_Row,sizeof(LONG));
            swmem(&tl->t_AbsRow,&tr->t_AbsRow,sizeof(BOOL));
        }
        tp->tp_Col = tl->t_Col+(tl->t_AbsCol ? 0 : tf_col);
        tp->tp_Row = tl->t_Row+(tl->t_AbsRow ? 0 : tf_row);
        tp->tp_Width = tr->t_Col+(tr->t_AbsCol ? 0 : tf_col)-tp->tp_Col;
        tp->tp_Height = tr->t_Row+(tr->t_AbsRow ? 0 : tf_row)-tp->tp_Row;
        if (tp->tp_Col < 0 || tp->tp_Row < 0 || tp->tp_Width < 0 || tp->tp_Height < 0)
            return false;
    }
    else if (t->t_Op == OP_FUNC)
    {
        struct Result r;

		memset(&r, 0, sizeof(struct Result));
		if (CalcTree(&r, t) != CT_OK || !r.r_Cell)
            return false;

        tp->tp_Col = r.r_Cell->tf_Col;
        tp->tp_Row = r.r_Cell->tf_Row;
        tp->tp_Width = tp->tp_Height = 0;
    }
    else
    {
        return false;
    }

    return true;
}


bool
schaltjahr(long jahr)
{
	return (bool)(!(jahr % 4) && (jahr % 100 || !(jahr % 400)));
}


#define YEARS400 146097  // 400 Jahre haben soviel Tage
#define YEARS100 36524
#define YEARS4 1461


static long
floorDivide(long numerator, long denominator, long *remainder)
{
    long quotient;

    if (numerator >= 0)
    {
        *remainder = numerator % denominator;
		return numerator/denominator;
    }

    quotient = ((numerator+1) % denominator)-1;
    *remainder = numerator - (quotient * denominator);
	return quotient;
}


void
tagedatum(long days, long *tag, long *monat, long *jahr)
{
    UBYTE isLeap;
    long  i;

	// ToDo: ist das hier richtig???
	days -= 1;

    if (days < 0)
    {
        if (tag)
            *tag = days;
        if (monat)
            *monat = 1;
        if (jahr)
            *jahr = 1;
        return;
    }
    else
    {
		long n400, n100, n4, n1;
		
		n400 = floorDivide(days, YEARS400, &days);
		n100 = floorDivide(days, YEARS100, &days);
		n4 = floorDivide(days, YEARS4, &days);
		n1 = floorDivide(days, 365, &days);

		i = n400 * 400 + n100 * 100 + n4 * 4 + n1;
        if (n100 == 4 || n1 == 4)
            days = 365;
        else
            i++;
    }
    isLeap = schaltjahr(i);

    if (jahr)
        *jahr = i;

    {
        UBYTE correction = 0;
        int   march1 = isLeap ? 60 : 59; // zero-based DOY for March 1

        if (days >= march1)
            correction = isLeap ? 1 : 2;

        i = (12 * (days + correction) + 6) / 367; // zero-based month

        if (monat)
			*monat = i + 1;
        if (tag)
			*tag = days - mday[i] - (isLeap && days >= march1 ? 1 : 0) + 1;
    }
}


long
monthlength(long m, long jahr)
{
    // Der Dezember hat keinen Folgemonat
    if (m == 12)
        return 31;

    // Der Februar hat in Schaltjahren 29 Tage
    if (m == 2 && schaltjahr(jahr))
        return 29;

    // Differenz des ersten Tages zum Folgemonat
    return mday[m] - mday[m - 1];
}


static long
LocaleWeekday(long days)
{
	return days % 7;
}


long
weekday(long days)
{
	return ((days + 6) % 7) + 1;
}


double
fak(double val)
{
    double f = 1.0;
    long   i = 1;

    if (val <= 0.0)
        return 0.0;

	for (; i <= val; f *= i, i++);

    return f;
}


static double
betterfmod(double v, long m)
{
    v /= m;
    if (v > 0.0)
        v = v - floor(v);
    else
        v = v - ceil(v);

    return v * m;
}


bool
GetNameDatabaseField(struct Mappe *mp, struct Database **db, STRPTR t, struct Field **sfi, long *pos)
{
    struct Field *fi;
    long   len;

    if (!*db)
        return false;

    *pos = 0;
    foreach (&(*db)->db_Fields, fi)
    {
        len = strlen(fi->fi_Node.ln_Name);
        if (!strnicmp(fi->fi_Node.ln_Name,t,len))
        {
            if (t[len] == '.' && fi->fi_Node.ln_Type == FIT_REFERENCE)
            {
                *db = (APTR)MyFindName(&mp->mp_Databases, fi->fi_Special);
                return GetNameDatabaseField(mp, db, t + len + 1, sfi, pos);
            }
            else if (!t[len])
            {
                *sfi = fi;
                return true;
            }
        }
        (*pos)++;
    }
    *sfi = NULL;
    return false;
}


bool
GetNameAndField(STRPTR t, struct Database **_db, struct Field **_fi, int32 *_fiPos)
{
    struct Database *db;
    struct Field *fi = NULL;
    int32  fiPos;

    if (!t)
        return false;

    if (!(db = (struct Database *)GetName(t)))      /* kein Name oder Datenbank */
    {
        // Aber wir sind ja nett, also suchen wir nach Feldern in
        // Datenbanken, die auf den Namen hören könnten

        foreach (&calcpage->pg_Mappe->mp_Databases, db)
        {
            fiPos = 0;
            foreach (&db->db_Fields, fi)
            {
                if (!stricmp(fi->fi_Node.ln_Name, t))
                    break;
                fiPos++;
            }
            if (fi->fi_Node.ln_Succ)
                break;
        }
        if (!fi || !fi->fi_Node.ln_Succ)
            return false;
    }
    else if ((t = strchr(t, '.')) != 0) /* Der Punkt markiert Felder in Datenbanken - bei normalen Namen ist er unzulässig */
    {
        if (db->db_Node.ln_Type != NMT_DATABASE)
            return false;

        if (!GetNameDatabaseField(calcpage->pg_Mappe, &db, t + 1, &fi, &fiPos))
            return false;
    }

    *_db = db;
    *_fi = fi;
    *_fiPos = fiPos;

    return true;
}


long
NameValue(struct Result *r, STRPTR t)
{
    struct Page *oldpage;
    struct Database *db;
    struct Field *fi;
    long   col, row, rc, fiPos;

    if (!GetNameAndField(t, &db, &fi, &fiPos))
    {
		// nichts gefunden - wenn unbekannte Namen zu 0.0 aufgelöst werden sollen,
        // machen wir das hier, sonst wird ein Fehler zurückgegeben.
        if (t && (calcflags & CF_ZERONAMES))
        {
            r->r_Value = 0.0;
            r->r_Type = RT_VALUE;
            return CT_OK;
        }
        return CTERR_NAME;
    }

    oldpage = calcpage;                             /* globale Var. aufbereiten */

    if (db->db_Page)
        calcpage = db->db_Page;

    if (db->db_Node.ln_Type != NMT_SEARCH)
    {
        col = tf_col;  row = tf_row;
        tf_col = 0;  tf_row = 0;
    }

    if (fi)                                         /* aktuelles Feld zurückgeben */
    {
        struct Term term;

		memset(&term, 0, sizeof(struct Term));
        term.t_Op = OP_CELL;
        term.t_Col = db->db_TablePos.tp_Col + fiPos;
        term.t_Row = db->db_TablePos.tp_Row + db->db_Current;
        term.t_AbsCol = term.t_AbsRow = TRUE;
        rc = CalcTree(r, &term);
    }
    else                                            /* Standardberechnung */
    {
        rc = CalcTree(r, db->db_Root);
	}
    calcpage = oldpage;                             /* globale Var. auf alte Werte */
    if (db->db_Node.ln_Type != NMT_SEARCH)
        tf_col = col,  tf_row = row;

    return rc;
}


void
CalcError(long err)
{
    CONST_STRPTR t = NULL;

    if (!err || !(calcflags & CF_REQUESTER))
        return;

    switch (err)
    {
        case CTERR_TYPE: t = GetString(&gLocaleInfo, MSG_WRONG_TYPE_CALCERR);  break;
        case CTERR_DIV: t = GetString(&gLocaleInfo, MSG_DIVISION_BY_ZERO_CALCERR);  break;
        case CTERR_FUNC: t = GetString(&gLocaleInfo, MSG_UNKNOWN_FUNCTION_CALCERR);  break;
        case CTERR_ARGS: t = GetString(&gLocaleInfo, MSG_WRONG_ARGS_COUNT_CALCERR);  break;
        case CTERR_NAME: t = GetString(&gLocaleInfo, MSG_UNDEFINED_NAME_CALCERR);  break;
        case CTERR_SYNTAX: t = GetString(&gLocaleInfo, MSG_BAD_SYNTAX_CALCERR);  break;
        case CTERR_LOOP:
            ErrorRequest(GetString(&gLocaleInfo, MSG_LOOP_CALCERR),Coord2String(tf_col,tf_row));
            break;
        default:
            t = GetString(&gLocaleInfo, MSG_UNKNOWN_CALCERR);
    }
    if (t)
        ErrorRequest(t);
}


STRPTR CalcErrorString(int32 err)
{
    STRPTR t;

    if (!err)
        return NULL;

    switch (err)
    {
        case CTERR_TYPE: t = "#err-type"; break;
        case CTERR_DIV:  t = "#err-divzero"; break;
        case CTERR_LOOP: t = "#err-loop"; break;
        case CTERR_FUNC: t = "#err-func"; break;
        case CTERR_ARGS: t = "#err-args"; break;
        case CTERR_NAME: t = "#err-name"; break;
        case CTERR_SYNTAX: t = "#err-syntax"; break;
#ifdef __amigaos4__
        case CTERR_NULLP: t = "#err-nullvalue"; break;
#endif
        default:
            t = "#err-unknown";
    }
    return t;
}


int32 PUBLIC
CalcTree(REG(a0, struct Result *r), REG(a1, struct Term *t))
{
    struct Result r1,r2;
    struct tableField *tf;
    int32  rc = CT_OK;

    if (!t)
        return CT_OK;

    memset(&r1,0,sizeof(struct Result));  memset(&r2,0,sizeof(struct Result));
    if (t->t_Op != OP_RANGE)
    {
        if ((rc = CalcTree(&r1, t->t_Left)) != 0)
        {
			return rc;
		}
        if ((rc = CalcTree(&r2, t->t_Right)) != 0)
        {
            FreeString(r1.r_Text);
            return rc;
        }
    }

    switch(t->t_Op)
    {
        case OP_VALUE:
            r->r_Value = t->t_Value;
            r->r_Type = RT_VALUE;
            break;
        case OP_TEXTVALUE:
            r->r_Value = t->t_Value;
            r->r_Text = AllocString(t->t_Text);
            r->r_Type = RT_VALUE | RT_TEXT;
            if (!tf_format)
                tf_format = t->t_Format;
            break;
        case OP_TEXT:
            r->r_Text = AllocString(t->t_Text);
            r->r_Type = RT_TEXT;
            break;
        case OP_EXTCELL:
            tf = GetExtTableField(t);
#ifdef __amigaos4__
  			if(tf == NULL)
  			{
            	return CTERR_ARGS;
  			}
#endif
        case OP_CELL:
            if (t->t_Op == OP_CELL)
            {
                tf = GetTableField(calcpage,t->t_Col + (t->t_AbsCol ? 0 : tf_col),
                                            t->t_Row + (t->t_AbsRow ? 0 : tf_row));
            }

            r->r_Cell = tf;
            r->r_Type = RT_CELL;

            if (tf && tf->tf_Original)
            {
                if (!tf_format)
                    tf_format = tf->tf_Format;
                if (!(calcflags & CF_NOLOOPS))
                {
                    if (tf->tf_Flags & TFF_LOCKED)
                    {
                        tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
                        rc = CTERR_LOOP;
                        break;
                    }
                    else if (!(tf->tf_Flags & TFF_ACTUAL))
                    {
                        struct Page *page;

                        if (t->t_Op == OP_EXTCELL)
                        {
                            page = calcpage;
                            calcpage = GetExtCalcPage(t);
                        }
                        CalcTableField(tf);
                        if (t->t_Op == OP_EXTCELL)
                            calcpage = page;
                    }
                }
                if (tf->tf_Type & TFT_VALUE)
                    r->r_Value = tf->tf_Value,  r->r_Type |= RT_VALUE;
                if (tf->tf_Type & TFT_TEXT)
                    r->r_Text = AllocString(tf->tf_Text),  r->r_Type |= RT_TEXT;
            }
            else
                r->r_Type |= RT_TEXT | RT_VALUE;
            break;
        case OP_NAME:
            rc = NameValue(r,t->t_Text);
            break;
        case OP_ADD:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = r1.r_Value+r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            if (r1.r_Type & RT_TEXT && r2.r_Type & RT_TEXT)
            {
                if ((r->r_Text = AllocPooled(pool,(r1.r_Text ? strlen(r1.r_Text) : 0)+(r2.r_Text ? strlen(r2.r_Text) : 0)+1)) != 0)
                {
                    if (r1.r_Text)
                        strcpy(r->r_Text,r1.r_Text);
                    if (r2.r_Text)
                        strcat(r->r_Text,r2.r_Text);
                    r->r_Type |= RT_TEXT;
                }
            }
            else if (!(r1.r_Type & r2.r_Type))
                rc = CTERR_TYPE;
            break;
        case OP_SUB:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = r1.r_Value-r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_MUL:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = r1.r_Value*r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_DIV:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                if (r2.r_Value)
                    r->r_Value = r1.r_Value/r2.r_Value;
                else
                    rc = CTERR_DIV;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_MOD:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = fmod(r1.r_Value,r2.r_Value);
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_NEG:
            if (r2.r_Type & RT_VALUE)
            {
                r->r_Value = -r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_POT:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = pow(r1.r_Value,r2.r_Value);
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_RANGE:
            r1.r_Value = 0.0;
            calcerr = CT_OK;
            for (tf = NULL; (tf = GetRangeCells(t, tf)) != 0; r1.r_Value += tf->tf_Value);
            rc = calcerr;
            r->r_Value = r1.r_Value;
            r->r_Type = RT_VALUE;
            break;
        case OP_FUNC:
        {
            struct Function *f = t->t_Function;
            STRPTR oldformat = tf_format;

            if (f && f != (APTR)~0L && f->f_Code)
            {
                calcerr = CT_OK;

                switch(f->f_Type)
                {
                    case RT_RESULT:
                        rc = ((long (*)(struct Result *,struct MinList *))f->f_Code)(r,&t->t_Args);
                        break;
                    case RT_VALUE:
                        r->r_Value = ((double (*)(struct MinList *))f->f_Code)(&t->t_Args);
                        r->r_Type = RT_VALUE;
                        rc = calcerr;
                        break;
                    case RT_TEXT:
                        r->r_Text = ((STRPTR (*)(struct MinList *))f->f_Code)(&t->t_Args);
                        r->r_Type = RT_TEXT;
                        rc = calcerr;
                        break;
                }
                if (f->f_Node.ln_Pri & FT_IGNOREFORMAT)
                    tf_format = oldformat;
            }
            else if (f)
            {
#ifdef __amigaos4__
                t->t_Function = NULL;
#endif
                rc = CTERR_ARGS;
                }
            else
                rc = CTERR_FUNC;
            break;
        }
        case OP_FAK:
            if (r2.r_Type & RT_VALUE)
            {
                r->r_Value = fak(r2.r_Value);
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_GREATER:
        case OP_EQGREATER:
        case OP_LESS:
        case OP_EQLESS:
        case OP_EQUAL:
        case OP_NOTEQUAL:
            r->r_Type = RT_VALUE;
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                switch(t->t_Op)
                {
                    case OP_GREATER: r->r_Value = r1.r_Value > r2.r_Value;    break;
                    case OP_EQGREATER: r->r_Value = r1.r_Value >= r2.r_Value; break;
                    case OP_LESS: r->r_Value = r1.r_Value < r2.r_Value;       break;
                    case OP_EQLESS: r->r_Value = r1.r_Value <= r2.r_Value;    break;
                    case OP_EQUAL: r->r_Value = r1.r_Value == r2.r_Value;     break;
                    case OP_NOTEQUAL: r->r_Value = r1.r_Value != r2.r_Value;  break;
                    default: break;
                }
            }
            else if (r1.r_Type & RT_TEXT && r2.r_Type & RT_TEXT)
            {
                long diff = strcmp(r1.r_Text ? r1.r_Text : (STRPTR)"",r2.r_Text ? r2.r_Text : (STRPTR)"");

                switch(t->t_Op)
                {
                    case OP_GREATER: r->r_Value = diff > 0;    break;
                    case OP_EQGREATER: r->r_Value = diff >= 0; break;
                    case OP_LESS: r->r_Value = diff < 0;       break;
                    case OP_EQLESS: r->r_Value = diff <= 0;    break;
                    case OP_EQUAL: r->r_Value = diff == 0;     break;
                    case OP_NOTEQUAL: r->r_Value = diff != 0;  break;
                    default: break;
                }
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_BITNOT:
            if (r2.r_Type & RT_VALUE)
            {
                r->r_Value = !(long)r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_BITAND:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = (long)r1.r_Value & (long)r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_BITOR:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = (long)r1.r_Value | (long)r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_AND:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = r1.r_Value && r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
        case OP_OR:
            if (r1.r_Type & RT_VALUE && r2.r_Type & RT_VALUE)
            {
                r->r_Value = r1.r_Value || r2.r_Value;
                r->r_Type = RT_VALUE;
            }
            else
                rc = CTERR_TYPE;
            break;
		default:
			break;
    }
    FreeString(r1.r_Text);  FreeString(r2.r_Text);
    return rc;
}


double TreeValue(struct Term *t)
{
    struct Result r;

    memset(&r,0,sizeof(struct Result));
    if (!(calcerr = CalcTree(&r,t)) && !(r.r_Type & RT_VALUE))
        calcerr = CTERR_TYPE;
    FreeString(r.r_Text);

    return r.r_Value;
}


STRPTR
TreeText(struct Term *t)
{
    struct Result r;

    memset(&r,0,sizeof(struct Result));
    if (!(calcerr = CalcTree(&r,t)) && !(r.r_Type & RT_TEXT))
        calcerr = CTERR_TYPE;
    return(r.r_Text);
}


struct tableField *
TreeCell(struct Term *t)
{
    struct Result r;

    if (!t)
        return NULL;

    memset(&r,0,sizeof(struct Result));
    if (!(calcerr = CalcTree(&r,t)) && !(r.r_Type & RT_CELL))
        calcerr = CTERR_TYPE;

    return r.r_Cell;
}


void
FreeName(struct Name *nm)
{
    if (!nm)
        return;

    FreeString(nm->nm_Node.ln_Name);
    FreeString(nm->nm_Content);
    DeleteTree(nm->nm_Root);

    FreeReference(nm->nm_Reference, true);

    FreePooled(pool, nm, sizeof(struct Name));
}


struct Name *
CopyName(struct Name *nm)
{
    struct Name *cnm;

    if ((cnm = AllocPooled(pool, sizeof(struct Name))) != 0)
    {
        CopyMem(nm,cnm,sizeof(struct Name));
        cnm->nm_Node.ln_Name = AllocString(nm->nm_Node.ln_Name);
        cnm->nm_Node.ln_Type |= NMT_DETACHED;
        cnm->nm_Content = AllocString(nm->nm_Content);
        cnm->nm_Root = CopyTree(nm->nm_Root);
        cnm->nm_Reference = NULL;
    }
    return cnm;
}


void
RefreshName(struct Name *nm)
{
    if (!nm || nm->nm_Node.ln_Type != NMT_CELL)
        return;

    FillTablePos(&nm->nm_TablePos, nm->nm_Root);
}


void
UpdateName(struct Name *nm)
{
    if (!nm || (nm->nm_Node.ln_Type & NMT_DETACHED) != 0)
        return;

    if (!nm->nm_Reference)
        nm->nm_Reference = MakeReference(nm->nm_Page, RTYPE_NAME, nm, nm->nm_Root);
    else
        UpdateReferences(nm->nm_Reference, nm->nm_Root);
}


uint8
TypeOfName(struct Name *nm)
{
    return (uint8)(nm->nm_Node.ln_Type & ~(NMT_UNDEFINED | NMT_DETACHED));
}


void
DetachNameList(struct MinList *list)
{
    struct Name *nm;

    foreach (list, nm)
    {
        nm->nm_Node.ln_Type |= NMT_DETACHED;

        FreeReference(nm->nm_Reference, false); // ToDo: is "false" correct here?
        nm->nm_Reference = NULL;
    }
}


void
AttachNameList(struct MinList *list)
{
    struct Name *nm;

    foreach (list, nm)
    {
        nm->nm_Node.ln_Type &= ~NMT_DETACHED;
        D({if (nm->nm_Reference)
            bug("reference is already set for name: 0x%lx\n", nm);});

        UpdateName(nm);
    }
}


void
SetNameContent(struct Name *nm, STRPTR content)
{
    if (nm == NULL)
        return;

    // alte "Verbindlichkeiten" loswerden
    FreeString(nm->nm_Content);
    DeleteTree(nm->nm_Root);

    // neue erstellen
    nm->nm_Content = content;
    nm->nm_Root = CreateTree(nm->nm_Page, content);

    RefreshName(nm);
    UpdateName(nm);
}


struct Name *
AddName(struct MinList *list, CONST_STRPTR name, STRPTR content, BYTE type, struct Page *page)
{
    struct Name *nm;

    if ((nm = AllocPooled(pool, sizeof(struct Name))) != 0)
    {
        nm->nm_Node.ln_Name = AllocString(name);
        nm->nm_Node.ln_Type = type & (~NMT_UNDEFINED);
        nm->nm_Content = AllocString(content);
        nm->nm_PageNumber = ~0L;

        if (type & NMT_UNDEFINED)
            nm->nm_PageNumber = (ULONG)page;
        else
        {
            nm->nm_Page = page;
            if (content)
            {
                nm->nm_Root = CreateTree(page, content);
                RefreshName(nm);
                UpdateName(nm);
            }
        }
        MyAddTail(list, nm);

        if ((nm->nm_Node.ln_Type & NMT_DETACHED) == 0)
            AssignUnresolvedReferencesForName(page, nm);
    }
    return nm;
}


void FreeFormat(struct FormatVorlage *fv)
{
    if (!fv)
        return;

    FreeString(fv->fv_Node.ln_Name);
    FreeString(fv->fv_Preview);
    FreePooled(pool,fv,sizeof(struct FormatVorlage));
}


struct FormatVorlage *
CopyFormat(struct FormatVorlage *fv)
{
    struct FormatVorlage *cfv;

    if (!fv)
        return NULL;

    if ((cfv = AllocPooled(pool, sizeof(struct FormatVorlage))) != 0)
    {
        CopyMem(fv,cfv,sizeof(struct FormatVorlage));
        cfv->fv_Node.ln_Name = AllocString(fv->fv_Node.ln_Name);
        cfv->fv_Preview = AllocString(fv->fv_Preview);
    }
    return cfv;
}


int32
FormatSort(struct Node **lna,struct Node **lnb)
{
    long i;

    if (!(i = (*lna)->ln_Type - (*lnb)->ln_Type))
    {
        if (!(i = (*lna)->ln_Pri - (*lnb)->ln_Pri))
        {
            if ((*lna)->ln_Type == FVT_VALUE && !strcmp((*lna)->ln_Name,"0"))
            {
                if (!strcmp((*lna)->ln_Name,(*lnb)->ln_Name))
                    return(0);
                return(-1);
            }
            else if ((*lnb)->ln_Type == FVT_VALUE && !strcmp((*lnb)->ln_Name,"0"))
                return(1);
            else /*if (!(i = StrnCmp(loc,(*lna)->ln_Name,(*lnb)->ln_Name,-1,SC_COLLATE1)))*/
                return(strcmp((*lna)->ln_Name,(*lnb)->ln_Name));
        }
    }
    return i;
}


void
SortFormatList(struct MinList *list)
{
    SortListWith(list,FormatSort);
}


uint8
SpecifyFormatType(STRPTR t)
{
    bool unsure = false;
    char  c;

    for(;*t;t++)
    {
        if (*t == '#')
        {
            t++;  c = tolower(*t);
            if (c == 'y' || c == 'd')
                return(FVT_DATE);
            else if (c == 'h' || c == 's')
                return(FVT_TIME);
            else if (c == 'm')
                unsure = true;
        }
        else if (*t == '%')
        {
            t++;  c = tolower(*t);
            if (c == 'w' || c == 'm')
                return FVT_DATE;
        }
    }
    if (unsure)
        return FVT_DATE;

    return FVT_VALUE;
}


struct FormatVorlage *
AddFormat(struct MinList *list, STRPTR t, BYTE pri, BYTE komma, BYTE align, ULONG pen, UBYTE flags, UBYTE type)
{
    struct FormatVorlage *fv;
    double value = 0.0;

	if ((fv = AllocPooled(pool, sizeof(struct FormatVorlage))) != 0)
    {
        fv->fv_Node.ln_Name = AllocString(t);
        fv->fv_Node.ln_Pri = pri;
        if (type == FVT_NOTSPECIFIED)
            type = SpecifyFormatType(t);
        fv->fv_Node.ln_Type = type;
        fv->fv_Komma = komma;
        fv->fv_Flags = flags;

        switch (type)
        {
            case FVT_DATE:
				value = FORMAT_DATE_PREVIEW;
                break;
            case FVT_TIME:
				value = FORMAT_TIME_PREVIEW;
             break;
        }
		fv->fv_Preview = AllocString(FitValueInFormat(value, (struct Node *)fv, NULL, 0, 0));
        fv->fv_Alignment = align == -1 ? TFA_RIGHT : align;
        fv->fv_NegativePen = pen;
		MyAddTail(list, fv);
    }
    return fv;
}


void
CompleteCentury(long *year)
{
    struct ClockData cd;
    ULONG  sec,msec;

    CurrentTime(&sec,&msec);
    Amiga2Date(sec,&cd);
    cd.year += 15;
    *year += (cd.year/100)*100;
    if (*year > cd.year)
        *year -= 100;
}


STRPTR
ita(double f,long nach,uint8 flags)
{
    static char buff[256];
    char   *b1;
    ULONG  restganz,stelle;
    long   i,j;
    BOOL   cut = FALSE;
    double rest,ganz;

    b1 = &buff[128];
    if (f >= 0.0)
        ganz = floor(f);
    else
        ganz = ceil(f);
    if (nach)
    {
        rest = f-ganz;
        restganz = (ULONG)(fabs(rest)*1000000000.0+0.5);
        if (nach < 0)
            cut = TRUE;
        if (nach < -1)
            nach = abs(nach)-1;
        if (nach != -1)
        {
            for(i = 9,stelle = 1;i > nach;i--,stelle *= 10);
            if ((restganz % stelle) > (5*stelle/10))
                restganz += stelle;
        }
        if (restganz >= 1000000000)
        {
            restganz -= 1000000000;
            ganz += f >= 0.0 ? 1.0 : -1.0;
        }
        sprintf(b1,"%09.9lu",restganz);
    }
    else
        ganz = floor(f+0.5);
    if (f < 0.0 && ganz == 0.0 && !(flags & ITA_NEGPARENTHESES))
        strcpy(buff,"-0");
    else
    {
        buff[0] = 0;  j = 0;  buff[127] = 0;
        if (!(flags & ITA_NEGPARENTHESES) && ganz < 0.0)
            strcpy(buff,"-"), j = 1;
        ganz = fabs(ganz);
        if (ganz != 0.0)
        {
            if (ganz > ((ULONG)~0L))
            {
                for(;ganz >= 1.0;ganz /= 10.0)
                {
                    i = betterfmod(ganz,10)+0.5;
                    ganz -= i;
                    buff[126] = i+'0';
                    strins(&buff[j],&buff[126]);
                }
            }
            else
                sprintf(buff+j,"%lu",(ULONG)ganz);
        }
        else
            strcat(buff,"0");
    }
    if (flags & ITA_SEPARATE)
    {
        j = buff[0] == '-' ? 1 : 0;
        for(i = strlen(buff)-3;i > j;i -= 3)
            strins(&buff[i],loc->loc_GroupSeparator);
    }
    if (nach)
    {
        strcat(buff,itaPoint);
        j = strlen(buff);
        if (!cut)
            strncat(buff,b1,nach);
        else
        {
            if (nach == -1)
                strcat(buff,b1);
            else
                strncat(buff,b1,nach);
            i = strlen(buff)-1;
            while(buff[i] == '0') buff[i--] = 0;
            if (buff[i] == *itaPoint) buff[i] = 0;
        }
        if (flags & ITA_SEPARATE)
        {
            for(i = 0;buff[j+i];i++)
            {
                if (i && !(i % 3))
                {
                    strins(&buff[j+i]," ");
                    j++;
                }
            }
        }
    }
    if ((flags & ITA_NEGPARENTHESES) && f < 0.0)
    {
        for(i = strlen(buff)+1;i;i--)
            buff[i] = buff[i-1];
        buff[0] = '(';
        strcat(buff,")");
    }
    return(buff);
}


int8
GetFVType(struct Node *fv, long *pos)
{
	STRPTR t = fv->ln_Name;
	long i = *pos;
	char c;

    (*pos)++;

	if (!t || (c = t[i]) == '\0')
		return 0;

	if (c == ' ')
		return FVT_SPACE;
	if (c == '0')
		return FVT_ZAHL;
	if (c == '#')
    {
		(*pos)++;
	
		switch (t[++i]) {
			case 'd':
				return FVT_SDAY;
			case 'D':
				return FVT_DAY;
			case 'm':
				return FVT_SMIN;
			case 'M':
				return FVT_MIN;
			case 'y':
				return FVT_SYEAR;
			case 'Y':
				return FVT_YEAR;
			case 'h':
				return FVT_SHOUR;
			case 'H':
				return FVT_HOUR;
			case 's':
				return FVT_SSEC;
			case 'S':
				return FVT_SEC;
			case '#':
			case '!':
				return FVT_CROSS;
			case '?':
				return FVT_ALL_THE_REST;
		}
    }
	else if (c == '%')
    {
		(*pos)++;

		switch (t[++i]) {
			case 'm':
				return FVT_SMON;
			case 'M':
				return FVT_MON;
			case 'w':
				return FVT_SWEEK;
			case 'W':
				return FVT_WEEK;
			case '%':
			case '!':
				return FVT_PROZENT;
			default:
				(*pos)--;
		}
    }
	else if (c == '?')
    {
		(*pos)++;
			
		switch (t[++i]) {
			case '?':
				return FVT_EVERYCHAR;
			case '!':
				return FVT_QUESTIONMARK;
			case '*':
				return FVT_ALL_THE_REST;
			default:
				return FVT_CHAR_OR_NOTHING;
		}
    }

	return FVT_CHAR;
}


/** Benutzt globalen String-Puffer
 */

STRPTR
FitValueInFormat(double val, struct Node *fv, STRPTR fvt, long komma, UBYTE flags)
{
	double v1, v2, v3;
    char   minus = 0;
	long   type = 0, i, pos = 0, days;
    STRPTR s;

    if (!(prefs.pr_Table->pt_Flags & PTF_SHOWZEROS) && val == 0.0)
		return NULL;

    if (!fv && fvt)
    {
		// No FormatVorlage	given, so find it
		if (!(fv = (APTR)FindLinkName(&calcpage->pg_Mappe->mp_CalcFormats, fvt))
			&& (fv = AllocPooled(pool, sizeof(struct FormatVorlage))))
        {
			// create a new FormatVorlage for this unknown format
            fv->ln_Name = fvt;
            fv->ln_Type = FVT_VALUE;
            ((struct FormatVorlage *)fv)->fv_Komma = komma;
            ((struct FormatVorlage *)fv)->fv_Flags = flags | FVF_ONTHEFLY;
			while ((i = GetFVType(fv, &pos)) != 0)
            {
				switch (i)
                {
                    case FVT_DAY:
                    case FVT_SDAY:
                    case FVT_YEAR:
                    case FVT_SYEAR:
                    case FVT_WEEK:
                    case FVT_SWEEK:
                    case FVT_MON:
                    case FVT_SMON:
                        fv->ln_Type = FVT_DATE;
                        break;
                    case FVT_SEC:
                    case FVT_SSEC:
                    case FVT_HOUR:
                    case FVT_SHOUR:
                        fv->ln_Type = FVT_TIME;
                        break;
                }
            }
            pos = 0;
        }
    }
    else if (fv)
    {
        komma = ((struct FormatVorlage *)fv)->fv_Komma;
        flags = ((struct FormatVorlage *)fv)->fv_Flags;
    }

    if (!fv)
		return ita(val, komma, flags);

    v1 = val;
    if (fv->ln_Type == FVT_DATE || fv->ln_Type == FVT_TIME)
    {
        if (fv->ln_Type == FVT_DATE)
        {
            long tag, monat, jahr;

            tagedatum(days = (long)val, &tag, &monat, &jahr);
            v1 = val = tag;
            v2 = monat;
            v3 = jahr;
        }

        while ((i = GetFVType(fv, &pos)))
        {
            switch (i)
            {
                case FVT_DAY:
                case FVT_SDAY:
                    type |= 1;
                    break;
                case FVT_YEAR:
                case FVT_SYEAR:
                    type |= 4;
                    break;
                case FVT_SEC:
                case FVT_SSEC:
                    type |= 1;
                    break;
                case FVT_MIN:
                case FVT_SMIN:
                    type |= 2;
                    break;
                case FVT_HOUR:
                case FVT_SHOUR:
                    type |= 4;
                    break;
            }
        }
    }

    if (gTextBuffer)
        *gTextBuffer = 0;

    pos = 0;
    if (fv->ln_Type == FVT_TIME && val < 0.0)
    {
        minus = 1;
        val = abs(val);
    }

    while ((i = GetFVType(fv, &pos)))
    {
        if (!EnlargeTextBuffer(-1))
            return NULL;
 
        switch (i)
        {
            case FVT_ZAHL:
                if (fv->ln_Type == FVT_PERCENT)
                    v1 *= 100;
                strcat(gTextBuffer, ita(v1, komma, flags));
                break;
            case FVT_DAY:
            case FVT_SDAY:
                if (i == FVT_DAY && (long)v1 < 10 && v1 >= 0.0)
                    strcat(gTextBuffer, "0");
                sprintf(gTextBuffer + strlen(gTextBuffer), "%ld", (long)v1);
                break;
            case FVT_SEC:
            case FVT_SSEC:
                if (type & 4)
                    v1 = betterfmod(val, 3600);
                if (type & 2)
                    v1 = betterfmod(val, 60);
                if (type == 1 && minus)
                    strcat(gTextBuffer, "-");
                s = ita(v1, komma, FALSE);
                if (i == FVT_SEC && (!*(s+1) || *(s+1) == ',' || *(s+1) == '.'))
                    strcat(gTextBuffer, "0");
                strcat(gTextBuffer, s);
                break;
            case FVT_MIN:
            case FVT_SMIN:
                if (fv->ln_Type == FVT_TIME)
                {
                    v2 = val/60.0;
                    if (type & 4)
                        v2 = ((long)v2) % 60;
                    else if (minus)
						strcat(gTextBuffer, "-");
                    if (type & 1)
                        sprintf(gTextBuffer + strlen(gTextBuffer), i == FVT_MIN ? "%02ld" : "%ld",(long)v2);
                    else
                    {
                        s = ita(v2, komma, FALSE);
                        if (i == FVT_MIN && (!*(s+1) || *(s+1) == ',' || *(s+1) == '.'))
                            strcat(gTextBuffer, "0");
                        strcat(gTextBuffer, s);
                    }
                }
                else
                {
                    if (i == FVT_MIN && (long)v2 < 10)
                        strcat(gTextBuffer, "0");
                    sprintf(gTextBuffer + strlen(gTextBuffer), "%ld", (long)v2);
                }
                break;
            case FVT_SMON:
                strcat(gTextBuffer, GetLocaleStr(loc, ABMON_1 + (long)v2 - 1));
                break;
            case FVT_MON:
                strcat(gTextBuffer, GetLocaleStr(loc, MON_1 + (long)v2-1));
                break;
            case FVT_SWEEK:
				strcat(gTextBuffer, GetLocaleStr(loc, ABDAY_1 + LocaleWeekday(days)));
                break;
            case FVT_WEEK:
				strcat(gTextBuffer, GetLocaleStr(loc, DAY_1 + LocaleWeekday(days)));
                break;
            case FVT_YEAR:
                sprintf(gTextBuffer + strlen(gTextBuffer), "%04ld", (long)v3);
                break;
            case FVT_SYEAR:
                sprintf(gTextBuffer + strlen(gTextBuffer), "%02ld", ((long)v3) % 100);
                break;
            case FVT_HOUR:
            case FVT_SHOUR:
                v3 = val / 3600.0;
                if (minus)
                    strcat(gTextBuffer, "-");
                if (type & 2)
                    sprintf(gTextBuffer + strlen(gTextBuffer), i == FVT_HOUR ? "%02ld" : "%ld", (long)v3);
                else
                {
                    s = ita(v3,komma,FALSE);
                    if (i == FVT_HOUR && (!*(s+1) || *(s+1) == ',' || *(s+1) == '.'))
                        strcat(gTextBuffer, "0");
                    strcat(gTextBuffer, s);
                }
                break;            
            case FVT_CHAR:
            case FVT_SPACE:
            case FVT_CHAR_OR_NOTHING:
                gTextBuffer[strlen(gTextBuffer) + 1] = 0;
                gTextBuffer[strlen(gTextBuffer)] = *(fv->ln_Name + pos - 1);
                break;
            case FVT_QUESTIONMARK:
                strcat(gTextBuffer, "?");
                break;
            case FVT_PROZENT:
                strcat(gTextBuffer, "%");
                break;
            case FVT_CROSS:
                strcat(gTextBuffer, "#");
                break;
        }
    }
    if (((struct FormatVorlage *)fv)->fv_Flags & FVF_ONTHEFLY)
        FreePooled(pool, fv, sizeof(struct FormatVorlage));

    return gTextBuffer;
}


int32
t2d(STRPTR t, double *d, bool *minus)
{
    double e = 0.0;
    long   i = 0;

    if (*t == '-' || *t == '+')
        t++, i++, *minus = true;
    for(; *t && isdigit(*t); t++, i++)
    {
        e *= 10.0;
        e += *t-'0';
    }
    *d = ceil(e);
    return i;
}


long
nachkommastellen(STRPTR t, double *value)
{
    long   i = 0, l;
    double nach;
    UBYTE  store;

    if (*t == '.' || *t == ',')
    {
        for (l = 0; isdigit(*(t + l + 1)); l++);
        if (l > 0)
        {
            i = 1+l;
            store = *(t+i);  *(t+i) = 0;
            for(nach = strtod(t+1,NULL);l;nach /= 10.0,l--);
            *(t+i) = (UBYTE)store;
            *value += nach;
        }
    }
    return i;
}


const STRPTR abdayname[] = {"So","Mo","Di","Mi","Do","Fr","Sa",
                            "Sun","Mon","Tue","Wed","Thu","Fri","Sat",
                            "Dim","Lun","Mar","Mer","Jeu","Ven","Sam",
                            "dom","lun","mar","mie","jue","vie","sab",
                            "Dom","Lun","Mar","Mer","Gio","Ven","Sab"};
const STRPTR dayname[] = {"Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag",
                          "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday",
                          "Dimanche","Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi",
                          "domingo","lunes","martes","miércoles","jueves","viernes","sábado",
                          "Domenica","Lunedì","Martedì","Mercoledì","Giovedì","Venerdì","Sabato"};
const STRPTR abmonthname[] = {"Jan","Feb","Mär","Apr","Mai","Jun","Jul","Aug","Sep","Okt","Nov","Dez",
                              "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec",
                              "Jan","Fév","Mars","Avr","Mai","Juin","Juil","Août","Sep","Oct","Nov","Déc",
                              "ene","feb","mar","abr","may","jun","jul","ago","sep","oct","nov","dic",
                              "Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago","Set","Ott","Nov","Dic"};
const STRPTR monthname[] = {"Januar","Februar","März","April","Mai","Juni","Juli","August","September","Oktober","November","Dezember",
                            "January","February","March","April","May","June","July","August","September","October","November","December",
                            "Janvier","Février","Mars","Avril","Mai","Juin","Juillet","Août","Septembre","Octobre","Novembre","Décembre",
                            "enero","febrero","marzo","abril","mayo","junio","julio","agosto","septiembre","octubre","noviembre","diciembre",
                            "Gennaio","Febbraio","Marzo","Aprile","Maggio","Giugno","Luglio","Agosto","Settembre","Ottobre","Novembre","Dicembre"};
#define NUM_LANGS 5

#define CFR_NO_MATCH 0
#define CFR_FITS 1
#define CFR_FITS_PARTIALLY 2
#define CFR_CENTURY 4


int32
CheckFormat(struct Node *fv, STRPTR t, double *_value)
{
	long index, type, result = CFR_NO_MATCH;
	long formatIndex = 0;
	double zahl, sec;
	long   hour, m, day, year;
//	  char   *s;
	bool   minus = false;

    if (!fv)
		return 0L;

	index = 0;  zahl = 0.0;  sec = 0.0;  hour = 0;  m = 1;  day = 1;  year = 1;

	while ((type = GetFVType(fv, &formatIndex)) != 0)
    {
		// ignore spaces
		while (t[index] == ' ')
			index++;

		switch (type)
        {
            case FVT_ZAHL:
			{
				long length = t2d(t + index, &zahl, &minus);

				if (length != 0 && !(length == 1 && (t[index] == '-' || t[index] == '+')))
                {
					index += length;
					if ((length = nachkommastellen(t + index, &zahl)) != 0)
						index += length;
                }
                else
					return CFR_NO_MATCH;
                break;
			}
            case FVT_MON:
            case FVT_SMON:
			{
				bool found = false;
				int32 length;
				const char *s;

				// iterate over the current locale
				for (m = 0; m < 12; m++)
				{
					uint32 month1 = type == FVT_SMON ? ABMON_1 : MON_1;

					s = GetLocaleStr(loc, month1 + m);
					length = strlen(s);

					if (!strnicmp(s, t + index, length) && !isalpha(t[index + length]))
					{
						index += length;
						found = true;
						break;
					}
				}

				if (!found)	{
					// iterate over all known day names in several languages
					for (m = 0; m < 12 * NUM_LANGS; m++)
	                {
						s = type == FVT_SMON ? abmonthname[m] : monthname[m];
						length = strlen(s);

						if (!strnicmp(s, t + index, length) && !isalpha(t[index + length]))
	                    {
							index += length;
							found = true;
							break;
	                    }
	                }
				}

				m = (m % 12) + 1;
				
				if (!found)
					return CFR_NO_MATCH;
                break;
			}
            case FVT_WEEK:
            case FVT_SWEEK:
			{
				bool found = false;
				int32 length, i;
				char *s;
																		
				// iterate over the current locale
				for (i = 0; i < 7; i++)
				{
					uint32 day1 = type == FVT_SWEEK ? ABDAY_1 : DAY_1;

					s = (char *)GetLocaleStr(loc, day1 + i);
					length = strlen(s);

					if (!strnicmp(s, t + index, length) && !isalpha(t[index + length]))
					{
						index += length;
						found = true;
						break;
					}
				}

				if (!found)	{
					// iterate over all known day names in several languages
					for (i = 0; i < 7 * NUM_LANGS; i++)
					{
						s = (type == FVT_SWEEK ? abdayname[i] : dayname[i]);
						length = strlen(s);

						if (!strnicmp(s, t + index, length) && !isalpha(t[index + length]))
						{
							index += strlen(s);
							found = true;
							break;
						}
					}
				}

				day = (i % 7);

				if (!found)
					return CFR_NO_MATCH;
                break;
			}
            case FVT_CHAR:
				if (fv->ln_Name[formatIndex - 1] != t[index++])
					return CFR_NO_MATCH;
                break;
            case FVT_EVERYCHAR:
                index++;
                break;
            case FVT_CHAR_OR_NOTHING:
				// only take the next character, if the current one fits
				if (fv->ln_Name[formatIndex - 1] == t[index])
                    index++;
                break;
            case FVT_QUESTIONMARK:
				if (t[index++] != '?')
					return CFR_NO_MATCH;
                break;
            case FVT_CROSS:
				if (t[index++] != '#')
					return CFR_NO_MATCH;
                break;
            case FVT_PROZENT:
				if (t[index++] != '%')
					return CFR_NO_MATCH;
                break;
            case FVT_SPACE:
                break;
            default:
			{
				long longValue;
#ifdef __amigaos4__
				long length;
				
				for(length = 0; isdigit(t[index + length]) && &t[index + length] != NULL; length++);
				longValue  = atol(t+ index);
#else
				long length = stcd_l(t + index, &longValue);
#endif
				if (length == 0)
					return CFR_NO_MATCH;

				// number at current position that has to be interpreted in the context

				index += length;
				if (longValue < 0)
                {
					longValue = -longValue;
					minus = true;
                }

				switch (type)
                {
                    case FVT_SMIN:
                    case FVT_MIN:
						if (fv->ln_Type == FVT_DATE && longValue < 1)
							return CFR_NO_MATCH;

						m = longValue;
                        break;
                    case FVT_SHOUR:
                    case FVT_HOUR:
						hour = longValue;
                        break;
                    case FVT_SDAY:
                    case FVT_DAY:
						if (longValue < 1)
							return CFR_NO_MATCH;

						day = longValue;
                        break;
                    case FVT_SSEC:
                    case FVT_SEC:
						sec = longValue;
						if ((length = nachkommastellen(t + index, &sec)) != 0)
							index += length;
                        break;
                    case FVT_SYEAR:
						// if the year has more than two characters, it's not the best choice
						// for this type
						if (length > 2)
							result |= CFR_FITS_PARTIALLY;
                    case FVT_YEAR:
						year = longValue;
                        if (prefs.pr_Table->pt_Flags & PTF_AUTOCENTURY && year < 100)
                        {
                            result |= CFR_CENTURY;
                            CompleteCentury(&year);
                        }
                        break;
                }
				break;
			}
        }
    }

	// we haven't found any reasons to deny the current format

	// Spaces übergehen
	while (t[index] == ' ')
		index++;

	if (t[index]) {
		// es folgen noch andere Zeichen
		result |= CFR_FITS_PARTIALLY;
	} else if ((result & CFR_FITS_PARTIALLY) == 0) {
		// dieses Format paßt perfekt, wenn es nicht schon vorher auf CFR_FITS_PARTIALLY
		// gesetzt wurde
        result |= CFR_FITS;
	}

    tf_format = fv->ln_Name;

    if (minus)
        zahl = -zahl;

	if (_value) 
	{
		switch (fv->ln_Type)
	    {
    	    case FVT_PERCENT:
				*_value = zahl / 100.0;
            	break;
	        case FVT_TIME:
				*_value = hour * 3600.0 + m * 60.0 + sec;  /* day removed */
   	        	 break;
   	     case FVT_DATE:
				day += mday[m - 1];
            	if (m > 2 && !(year % 4) && (year % 100 || !(year % 400)))
	                day++;
				// FIXME: GCC warning: operation on 'year' may be undefined
				m = (long)--year * 365 + year / 4 - year / 100 + year / 400;
				*_value = (double)(day + m);
	            break;
    	    default:
				*_value = zahl;
	    }
	}
	
    return result;
}


long
GetFormatOfValue(STRPTR t, double *value)
{
    struct MinList *list;
    struct Link *l;
	long   result = CFR_NO_MATCH;

    if (!t)
        return 0L;

    tf_format = NULL;

    if (calcpage)
        list = &calcpage->pg_Mappe->mp_CalcFormats;
    else
        list = &prefs.pr_Formats;

	// search the format list backwards, until a CFR_FITS format is found - a CFR_FITS_PARTIALLY will be saved in tf_format

	for (l = (struct Link *)list->mlh_TailPred; l->l_Node.mln_Pred; l = (APTR)l->l_Node.mln_Pred)
    {
		long tempResult = CheckFormat(list == &prefs.pr_Formats ? (APTR)l : l->l_Link, t, value);
		if (tempResult) {
			result = tempResult;  
			if (result & CFR_FITS)
				break;
		}
    }
	return result;
}


void
SetTFAPen(struct tableField *tf,struct FormatVorlage *fv)
{
    tf->tf_APen = tf->tf_ReservedPen;
    if (tf->tf_Value < 0.0)
    {
        if (tf->tf_Flags & TFF_NEGPENSET)
        {
            if (tf->tf_NegativePen != ~0L)
                tf->tf_APen = tf->tf_NegativePen;
        }
        else if (fv && (fv->fv_Flags & FVF_NEGATIVEPEN))
            tf->tf_APen = fv->fv_NegativePen;
    }
}


void
GetValue(struct Page *page, struct tableField *tf)
{
    struct FormatVorlage *fv;
    struct Mappe *mp = page->pg_Mappe;
    long   rc;

    if (!tf->tf_Text)
        return;

    if (!(tf->tf_Flags & TFF_FORMATSET))
    {
        FreeString(tf->tf_Format);
        tf->tf_Format = NULL;
    }
    calcpage = page;

	if (tf->tf_Format
		&& (fv = (struct FormatVorlage *)FindLinkName(&mp->mp_CalcFormats, tf->tf_Format)) != NULL
		&& (rc = CheckFormat((struct Node *)fv,tf->tf_Text, &tf->tf_Value)) & CFR_FITS)
        tf_format = NULL;
    else
		rc = GetFormatOfValue(tf->tf_Text, &tf->tf_Value);

    if (rc)
    {
        tf->tf_Type = TFT_VALUE;
		if (tf_format && (!tf->tf_Format || !FindLinkName(&mp->mp_CalcFormats, tf->tf_Format)))
        {
			if ((fv = (struct FormatVorlage *)FindLinkName(&mp->mp_CalcFormats, tf_format)) != 0)
            {
                FreeString(tf->tf_Format);
                tf->tf_Format = AllocString(tf_format);
                if (!(tf->tf_Flags & TFF_KOMMASET))
                    tf->tf_Komma = fv->fv_Komma;
            }
        }
        if (rc & CFR_FITS)
        {
            if (rc & CFR_CENTURY)    // Jahrhundert auch im Text vervollständigen
            {
				if (tf_format || (GetFormatOfValue(tf->tf_Text, NULL) & CFR_FITS))
                {
                    STRPTR t;
                    long   i;

					if ((t = AllocString(tf_format)) != 0)
                    {
                        for(i = 0;*(t+i) && !(*(t+i) == '#' && tolower(*(t+i+1)) == 'y');i++);
                        if (*(t+ ++i) == 'y')
                            *(t+i) = 'Y';
                        FreeString(tf->tf_Original);
                        tf->tf_Original = AllocString(FitValueInFormat(tf->tf_Value,NULL,t,tf->tf_Komma,0));
                        FreeString(t);
                    }
                }
            }

            FreeString(tf->tf_Text);
			tf->tf_Text = AllocString(FitValueInFormat(tf->tf_Value, NULL, tf->tf_Format, tf->tf_Komma,
								tf->tf_Flags & (TFF_SEPARATE | TFF_NEGPARENTHESES)));

			if ((fv = (struct FormatVorlage *)FindLinkName(&mp->mp_CalcFormats, tf->tf_Format)) != 0)
            {
                if (tf->tf_Alignment & TFA_VIRGIN)
                    tf->tf_Alignment = (tf->tf_Alignment & TFA_VCENTER) | fv->fv_Alignment | TFA_VIRGIN;
            }
			SetTFAPen(tf, fv);
        }
        else
            tf->tf_Type |= TFT_TEXT;
    }
}


static OPs
Operator(int op)
{
    switch (op)
    {
        case '+': return OP_ADD;
        case '-': return OP_SUB;
        case '*': return OP_MUL;
        case '/': return OP_DIV;
        case '%': return OP_MOD;
        case '_': return OP_NEG;
        case '^': return OP_POT;
        case ':': return OP_RANGE;
        case '!': return OP_FAK;
        case '~': return OP_BITNOT;
        case '>': return OP_GREATER;
        case '<': return OP_LESS;
        case '=': return OP_EQUAL;
        case '&': return OP_BITAND;
        case '|': return OP_BITOR;
        default: return OP_NONE;
    }
}

#define PRI_NEG 9
#define PRI_POT 10
#define PRI_TOP 12

static int
Priority(OPs op)
{
    switch (op)
    {
        case OP_AND:
            return 1;
        case OP_OR:
            return 2;
        case OP_BITOR:
            return 3;
        case OP_BITAND:
            return 4;
        case OP_EQUAL:
        case OP_NOTEQUAL:
            return 5;
        case OP_GREATER:
        case OP_EQGREATER:
        case OP_LESS:
        case OP_EQLESS:
            return 6;
        case OP_ADD:
        case OP_SUB:
            return 7;
        case OP_MUL:
        case OP_MOD:
        case OP_DIV:
            return 8;
        case OP_FAK:
        case OP_BITNOT:
        case OP_NEG:
            return PRI_NEG;
        case OP_POT:
            return PRI_POT;
        case OP_RANGE:
            return 11;
        default:
            return PRI_TOP;
    }
}


static int32
CheckFuncArgs(struct Function *f,struct MinList *args)
{
    struct FuncArg *fa;
    struct Result r;
    long   type = -2;
    STRPTR t;

    if (!f || !args)
        return CT_OK;
    /*** Argumentanzahl prüfen ***/
    {
        long num = CountNodes((struct MinList *)args);

        if (num < f->f_MinArgs || f->f_MaxArgs != -1 && num > f->f_MaxArgs)
            return CTERR_ARGS;
    }

    /*** Argumenttyp prüfen ***/

    t = f->f_Args;

    if (*t && !(calcflags & CF_SUSPEND))
    {
        for (fa = (APTR)args->mlh_Head;*t && fa->fa_Node.mln_Succ;t++,fa = (APTR)fa->fa_Node.mln_Succ)
        {
            /** b=bereich, z=zahl, t=text, w=wert, a=ausdruck, r=bezug, c=condition **/

            if (IsAlpha(loc,*t))
            {
                if (*t == 'z')
                    type = RT_VALUE;
                else if (*t == 't')
                    type = RT_TEXT;
                else if (*t == 'r')
                    type = RT_CELL;
                else
                    type = RT_TEXT | RT_VALUE;

                if (type != (RT_TEXT | RT_VALUE))
                {
                    memset(&r,0,sizeof(struct Result));
                    if (CalcTree(&r,fa->fa_Root) || !(r.r_Type & type))
                        return CTERR_TYPE;
                }
            }
            else if (*t == '.' && type != (RT_VALUE | RT_TEXT))
            {
                for (;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
                {
                    if (CalcTree(&r,fa->fa_Root) || !(r.r_Type & type))
                        return CTERR_TYPE;
                }
            }
            while (*t && *t != ';')
            {
                t++;
            }
        }
    }
    return CT_OK;
}


/** Übersetzt den angegebenen Text in Anführungszeichen in einen einfachen
 *  String.
 *  Der String-Pointer wird aufs letzt Anführungszeichen gesetzt.
 *  Benutzt den globalen String-Buffer.
 */

static STRPTR
parseText(char **t)
{
    char *string = *t;
    char quote = *string++;
    uint32 pos = 0;

    if (!EnlargeTextBuffer(0))
        return NULL;

    while (*string != quote && *string)
    {
        if (string[0] == '\\' && string[1] == quote)
            string++;

        if (pos >= gTextBufferLength - 2)
            EnlargeTextBuffer(pos);

        gTextBuffer[pos++] = *string++;
    }
    gTextBuffer[pos] = '\0';
    *t = string;

    return AllocString(gTextBuffer);
}


static struct Term *
neuerOperand(char **t)
{
    struct Term *k;
    char   zahl[32];
    STRPTR s;

    if ((k = AllocPooled(pool,sizeof(struct Term))) != 0)
    {
        s = *t;
        while(*++s == '.' || *s == ',' || IsAlNum(loc,*s))
            if (*s == ',') *s = '.';
        if (isdigit(**t))
        {
#ifdef __amigaos4__
			Strlcpy(zahl, *t, (long)((char *)s - *t) + 1);
#else
            stccpy(zahl, *t, (long)((char *)s - *t) + 1);
#endif
            k->t_Value = strtod(zahl,NULL);
            k->t_Op = OP_VALUE;
            k->t_Pri = PRI_TOP;
        }
        *t = s;
    }
    return k;
}


static struct Term *
newText(char *text, char quotationMark)
{
    struct Term *term;

    if ((term = AllocPooled(pool,sizeof(struct Term))) == NULL)
        return NULL;

    term->t_Text = text;
    term->t_Pri = PRI_TOP;

    if (quotationMark == '\'' && GetFormatOfValue(text, &term->t_Value))
    {
        term->t_Op = OP_TEXTVALUE;
        term->t_Format = AllocString(tf_format);
    }
    else
        term->t_Op = OP_TEXT;

    return term;
}


static struct Term *
neuerName(STRPTR t)
{
    struct Term *k;

    if ((k = AllocPooled(pool, sizeof(struct Term))) != 0)
    {
        k->t_Op = OP_NAME;
        k->t_Pri = PRI_TOP;
        k->t_Text = AllocString(t);
    }
    return k;
}

#ifdef __amigaos4__
static struct Term *
neuerSeitensprung(char **t)
{
    struct Term *k, *l, *r;
    char   *text = *t;
    char   page[64]; 
    char   coord[21];
    uint8 i; 

    //check if it link to another page and reserve term
    if (*text != '@' || (k = AllocPooled(pool, sizeof(struct Term))) == NULL)
        return NULL;

	//seperate page number or pagename
	text++; //ignore open @
	for(i = 0; text[0] != '@' && i < 63; text++)
		if(i < 63)
			page[i++] = text[0];
	text++; //ignore closing @ 
	page[(i < 63 ? i : 63)] = '\0';

    // copy coord-string
    i = 0;
    if (text[0] == '$') coord[i++] = *(text++);
    for (; isalpha(text[0]) && i < 20; coord[i++] = *(text++));
    if (text[0] == '$') coord[i++] = *(text++);
    for (; isdigit(text[0]) && i < 20; coord[i++] = *(text++));
    *t = text;
	coord[i] = '\0';
	//Is it a range in format @x@coord1:coord2, so convert to ignition format @x@coord1:@x@coord2
	if(text[0] == ':' && text[1] != '@')
	{
	    char tmp[67];
	    
	    sprintf(tmp,":@%s@", page);
		*t = *t -  strlen(tmp) + 1;
		CopyMem(tmp, *t, strlen(tmp));
	}
    //set attributes
    k->t_Op = OP_EXTCELL;
    k->t_Pri = Priority(OP_EXTCELL);

    //Is Page identified by number or name
	if (isdigit(page[0]))
       	k->t_NumPage = atol(page);
   	else
       	k->t_NumPage = -1;
   	k->t_Page = AllocString(page);//NULL;
	
    // ToDo: could accept much more advanced terms here...
    String2Coord(coord, &k->t_Col, &k->t_Row);

    return k;
}
#else
static struct Term *
neuerSeitensprung(char *page, char **t)
{
    struct Term *k;
    char   *text = *t;

    if (*text != '@' || (k = AllocPooled(pool, sizeof(struct Term))) == NULL) 
    {
        FreeString(page);
        return NULL;
    }

    k->t_Op = OP_EXTCELL;
    k->t_Pri = PRI_TOP;

    if (isdigit(page[0]))
        k->t_NumPage = atol(page + 1);
    else
        k->t_NumPage = -1;

    k->t_Page = page;

   // ToDo: could accept much more advanced terms here...
    String2Coord(++text, &k->t_Col, &k->t_Row);

    // jump to the end of the coord string
    if (text[0] == '$') text++;
    for (; isalpha(text[0]); text++);
    if (text[0] == '$') text++;
    for (; isdigit(text[0]); text++);
    *t = text - 1;

    return k;
}
#endif

static struct Term *
neuesFeld(char *t)
{
    struct Term *k;
    long   abs;

    if ((k = AllocPooled(pool, sizeof(struct Term))) != 0)
    {
        abs = String2Coord(t,&k->t_Col,&k->t_Row);
        k->t_Op = OP_CELL;
        k->t_Pri = PRI_TOP;
        if (abs & 1)  // Absolute Column
            k->t_AbsCol = TRUE;
        else
            k->t_Col -= tf_col;

        if (abs & 2)  // Absolute Row
            k->t_AbsRow = TRUE;
        else
            k->t_Row -= tf_row;
    }
    return k;
}

static struct Term *
neueFunktion(char *name, char *t)
{
    struct Term *k = NULL;
    struct FuncArg *fa;
    struct Function *f;
    STRPTR buff;
	long   i, j, size;

    if ((buff = AllocPooled(pool,size = strlen(t)+1)) != 0)
    {
        if ((k = AllocPooled(pool, sizeof(struct Term))) != 0)
        {
            k->t_Op = OP_FUNC;
            f = FindFunction(name,k);

            MyNewList(&k->t_Args);
            strcpy(buff, t);
            for (i = 1,j = 0;i && *t;t++,j++)
            {
                if (*t == ')')
                    i--;
                if (*t == '(')
                    i++;
                if (i == 1 && *t == ';')
                    buff[j] = 0;
            }
            if (i)
                calcerr = CTERR_SYNTAX;
#ifdef __amigaos4__
            buff[(j > 0 ? --j : (j = 0))] = 0; //verhindert, das j auf -1 gesetzt wird (zb =summe( ). dadurch wird hier wild in speicher geschrieben und die schleife unten läuft auch amok
#else
            buff[--j] = 0;
#endif
            if (f)
            {
                APTR stack;
                ULONG size = 1024;
                if ((stack = AllocPooled(pool, size)) != 0)
                {
                    for(i = 0;i < j;i += strlen(buff+i)+1)
                    {
#ifdef __amigaos4__
 						if(strlen(buff+i) == 1 && strchr((char *)"<>+-#?:*\\^[", *(buff+i)) != NULL)	//isolate "crash" characters
							continue;
#endif
                        if ((fa = AllocPooled(pool, sizeof(struct FuncArg))) != 0)
                        {
                            fa->fa_Root = createTree(buff+i,&stack,&size);
                            MyAddTail(&k->t_Args, fa);
                        }
                    }
                    if ((calcerr = CheckFuncArgs(f,&k->t_Args)) == CTERR_ARGS)
                        k->t_Function = (APTR)~0L;

                    FreePooled(pool, stack, size);
                }
            }
        }
        FreePooled(pool, buff, size);
    }
    return k;
}


static void
verwendeOperator(OPs op, struct Term **num_stack, long *sp_num)
{
    struct Term *t;

    if ((t = AllocPooled(pool, sizeof(struct Term))) != 0)
    {
        t->t_Right = num_stack[--(*sp_num)];
        t->t_Op = op;
        t->t_Pri = Priority(op);
        if (t->t_Pri > 0 && t->t_Pri < PRI_TOP && t->t_Pri != PRI_NEG && *sp_num)
            t->t_Left = num_stack[--(*sp_num)];
        num_stack[(*sp_num)++] = t;
    }
}


/** Berechnet den Zelltext, ändert die Zellbreite entsprechend, und aktualisiert
 *  die Textausrichtung und -farbe.
 */

void
CalcTableField(struct tableField *tf)
{
    int32  col = tf_col,row = tf_row,rc;
    struct FormatVorlage *fv = NULL;
    struct Result r;
    STRPTR oldtext;

    // only TFT_FORMULA cells must be calculated, don't calculate if the field gadget
    // is currently working on us
    if (!(tf->tf_Type & TFT_FORMULA)
        || (calcflags & CF_SUSPEND)
        || calcpage->pg_Gad.DispPos > PGS_FRAME && calcpage->pg_Gad.tf == tf && !tf->tf_Original)
        return;

    // if the references contains a loop, we don't need to
    // calculate this cell's value
    if (tf->tf_Reference && (tf->tf_Reference->r_Type & RTYPE_LOOP))
    {
        tf->tf_Text = AllocString(CalcErrorString(calcerr = CTERR_LOOP));
        tf->tf_Value = 0;
        return;
    }

    tf_col = tf->tf_Col;  tf_row = tf->tf_Row;  tf_format = NULL;  oldtext = tf->tf_Text;
    tf->tf_Flags |= TFF_LOCKED;
    memset(&r,0,sizeof(struct Result));
    tf->tf_Text = NULL;

    if ((rc = calcerr = CalcTree(&r, tf->tf_Root)) != 0)  /* an error */
    {
        tf->tf_Text = AllocString(CalcErrorString(rc));
        tf->tf_Value = 0;
    }
    else if (prefs.pr_Table->pt_Flags & PTF_SHOWFORMULA)
    {
        tf->tf_Text = AllocString(tf->tf_Original);
        tf->tf_Value = r.r_Value;
    }
    else
    {
        if (r.r_Type & RT_VALUE)
        {
            tf->tf_Value = r.r_Value;
            if (!(tf->tf_Flags & TFF_FORMATSET))
            {
                FreeString(tf->tf_Format);
                tf->tf_Format = AllocString(tf_format);
                if (!(tf->tf_Flags & TFF_KOMMASET) && tf_format && (fv = (APTR)FindLinkName(&calcpage->pg_Mappe->mp_CalcFormats,tf_format)))
                    tf->tf_Komma = fv->fv_Komma;
            }
            tf->tf_Text = AllocString(FitValueInFormat(r.r_Value,NULL,tf->tf_Format,tf->tf_Komma,tf->tf_Flags & (TFF_SEPARATE | TFF_NEGPARENTHESES)));
        }
        else if (r.r_Type & RT_TEXT)
            tf->tf_Text = r.r_Text;
    }
    tf->tf_Type = TFT_FORMULA | (r.r_Type & RT_VALUE ? TFT_VALUE : 0) | (r.r_Type & RT_TEXT ? TFT_TEXT : 0);

    if (!fv && tf->tf_Format)
        fv = (struct FormatVorlage *)FindLinkName(&calcpage->pg_Mappe->mp_CalcFormats,tf->tf_Format);
    tf->tf_Flags &= ~TFF_LOCKED;
    tf_col = col;  tf_row = row;

    tf->tf_Flags |= TFF_ACTUAL;
    if (!oldtext || strcmp(oldtext,tf->tf_Text ? tf->tf_Text : (STRPTR)""))
    {
        tf->tf_Flags |= TFF_REFRESH;
        SetTFWidth(calcpage,tf);
    }
    FreeString(oldtext);

    if (tf->tf_Alignment & TFA_VIRGIN)
    {
        if (tf->tf_Type & (TFT_TEXT | TFT_VALUE) == TFT_TEXT)
            tf->tf_Alignment = (tf->tf_Alignment & TFA_VCENTER) | TFA_LEFT | TFA_VIRGIN;
        else if (fv)
            tf->tf_Alignment = (tf->tf_Alignment & TFA_VCENTER) | fv->fv_Alignment | TFA_VIRGIN;
        else
            tf->tf_Alignment = (tf->tf_Alignment & TFA_VCENTER) | TFA_RIGHT | TFA_VIRGIN;
    }
    SetTFAPen(tf,fv);
}


struct Term * PUBLIC
CopyTree(REG(a0, struct Term *t))
{
    struct Term *ckn;
    struct FuncArg *fa,*sfa;

    if (!t)
        return NULL;

    if ((ckn = AllocPooled(pool, sizeof(struct Term))) != 0)
    {
        *ckn = *t;
        ckn->t_Left = CopyTree(t->t_Left);
        ckn->t_Right = CopyTree(t->t_Right);

        switch (ckn->t_Op)
        {
            case OP_TEXT:
            case OP_TEXTVALUE:
            case OP_NAME:
                ckn->t_Text = AllocString(t->t_Text);
                break;
            case OP_FUNC:
                MyNewList(&ckn->t_Args);
                for(fa = (APTR)t->t_Args.mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
                {
                    if ((sfa = AllocPooled(pool, sizeof(struct FuncArg))) != 0)
                    {
                        sfa->fa_Root = CopyTree(fa->fa_Root);
                        MyAddTail(&ckn->t_Args, sfa);
                    }
                }
                break;
            case OP_EXTCELL:
                ckn->t_Page = AllocString(t->t_Page);
                /*ckn->t_Mappe = AllocString(t->t_Mappe);*/
                break;
			default:
				break;
        }
    }
    return ckn;
}


static STRPTR
Ops(OPs op)
{
    static const STRPTR c[] = {">",">=","<","<=","=","!=","&","|","||","&&","+","-","*","/","%","-","!","~","^",":"};

    if (op >= 0 && op < OP_FUNC)
        return c[op-1];

    return NULL;
}


struct Term * PUBLIC
CopyTerm(REG(a0, struct Term *term))
{
    return CopyTree(term);
}


void PUBLIC
DeleteTerm(REG(a0, struct Term *term))
{
    DeleteTree(term);
}


struct Term * PUBLIC
CreateTerm(REG(a0, struct Page *page), REG(a1, STRPTR text))
{
    if (!text || *text != '=')
        return NULL;

    return CreateTree(page, text + 1);
}


/** Berechnet aus dem übergebenen Text bzw. der Formel den Ergebnistext.
 *  Das Ergebnis wird nach Benutzung mit FreeString() freigegeben.
 *  Ist (term == NULL) wird der Berechnungsterm aus der Formel, wenn nötig
 *  on-the-fly erstellt.
 *  Formeln müssen mit '=' beginnen, ein '#' am Anfang wird ignoriert und
 *  der Rest als normaler Text interpretiert.
 *
 *  @param page Seite des Objektes
 *  @param text Eingabetext oder Formel (darf NULL sein)
 *  @param term der der Formel entsprechende Term oder NULL
 *  @param format das Format für die Ausgabe oder NULL für eine automatische Wahl
 *
 *  @return result der berechnete Text, oder NULL wenn dieser leer ist. Das Ergebnis
 *     muß mit FreeString() freigegeben werden.
 */

STRPTR PUBLIC
CalcTerm(REG(a0, struct Page *page), REG(a1, STRPTR text), REG(a2, struct Term *term), REG(a3, STRPTR format))
{
    if (!text)
        return NULL;

    if (*text == '=')
    {
        struct Result r;
        long   rc;
        BYTE   treeowner = FALSE;

        tf_col = 0;  tf_row = 0;  memset(&r, 0, sizeof(struct Result));
        if (!term)
        {
            term = CreateTree(page, text + 1);
            treeowner = TRUE;
        }
        rc =CalcTree(&r, term);
        if (treeowner)
            DeleteTree(term);

        if (rc)
        {
            CalcError(rc);
            return AllocString(CalcErrorString(rc));
        }
        else
        {
            if (r.r_Type & RT_TEXT)
                return r.r_Text;
            else if (r.r_Type & RT_VALUE)
                return AllocString(FitValueInFormat(r.r_Value, NULL, format ? format : tf_format, -1, 0));
        }
        return NULL;
    }
    else if (*text == '#')
        return AllocString(text + 1);

    return AllocString(text);
}


bool
openBracket(struct Term *k, struct Term *s, uint32 *pos)
{
#ifdef __amigaos4__
    if (!k || !s)
    {
        D(bug("openBracket() - fehler!\n"));
    }
	else
	    if (k->t_Pri > s->t_Pri && s->t_Pri != 0 || k->t_Pri == s->t_Pri && k->t_Pri == PRI_POT)
    	{
        	strcpy(gTextBuffer + (*pos)++, "(");
        	return true;
    	}
#else
    if (!k || !s)
        D(bug("openBracket() - fehler!\n"));
    if (k->t_Pri > s->t_Pri && s->t_Pri != 0 || k->t_Pri == s->t_Pri && k->t_Pri == PRI_POT)
    {
        strcpy(gTextBuffer + (*pos)++, "(");
        return true;
    }
#endif
    return false;
}


void
closeBracket(BOOL isOpen, uint32 *pos)
{
    if (isOpen)
        strcpy(gTextBuffer + (*pos)++, ")");
}


static void
TermToText(struct Term *term, uint32 *_pos)
{
    struct FuncArg *fa;
    uint32  length = *_pos;
    bool   klammer;

    // ToDo: has no real checking of the length of the term!!!

    if (!term)
        return;

    if (!EnlargeTextBuffer(length))
        return;

    switch (term->t_Op)
    {
        case OP_VALUE:
            strcpy(gTextBuffer + length, ita(term->t_Value, -1, FALSE));
            break;
        case OP_CELL:
            if ((term->t_AbsCol || tf_col + term->t_Col > 0) && (term->t_AbsRow || tf_row + term->t_Row > 0))
                strcat(gTextBuffer, AbsCoord2String(term->t_AbsCol, (term->t_AbsCol ? 0 : tf_col) + term->t_Col,
                                                 term->t_AbsRow, (term->t_AbsRow ? 0 : tf_row) + term->t_Row));
            else
                strcat(gTextBuffer, "0");
            break;
        case OP_EXTCELL:
            // ToDo: don't use '"' if not necessary
#ifdef __amigaos4__
            gTextBuffer[length++] = '@';
#else
            gTextBuffer[length++] = '"';
#endif
            strcpy(gTextBuffer + length, term->t_Page ? term->t_Page : (STRPTR)"0");
            length += strlen(gTextBuffer + length);
#ifdef __amigaos4__
            gTextBuffer[length++] = '@';
#else
            gTextBuffer[length++] = '"';
            gTextBuffer[length++] = '#';
#endif
            strcpy(gTextBuffer + length, Coord2String(term->t_Col, term->t_Row));
            break;
        case OP_TEXT:
            gTextBuffer[length++] = '"';
            strcpy(gTextBuffer + length, term->t_Text);
            strcat(gTextBuffer + length, "\"");
            break;
        case OP_TEXTVALUE:
            gTextBuffer[length++] = '\'';
            strcpy(gTextBuffer + length, term->t_Text);
            strcat(gTextBuffer + length, "'");
            break;
        case OP_NAME:
            strcpy(gTextBuffer + length, term->t_Text);
            break;
        case OP_FUNC:
        {
            struct Function *f;

            if ((f = term->t_Function) != 0)
            {
                int32 nameLength;
                char *name;

                if (calcflags & CF_SHORTFUNCS)
                {
                    name = (char *)&f->f_ID;  // maximal 3 Buchstaben + Null-Byte
                    nameLength = strlen(name);
                }
                else
                {
                    name = f->f_Node.ln_Name;
                    nameLength = cmdlen(name);
                }
                memcpy(gTextBuffer + length, name, nameLength);
                length += nameLength;
                strcpy(gTextBuffer + length++, "(");
            }
            else
            {
                strcpy(gTextBuffer + length, "##unknown(");
                length += strlen(gTextBuffer + length);
            }

            foreach (&term->t_Args, fa)
            {
                if (fa != (struct FuncArg *)term->t_Args.mlh_Head)
                    strcat(gTextBuffer + length++, ";");

                TermToText(fa->fa_Root, &length);
            }
            strcpy(gTextBuffer + length, ")");
            break;
        }
        case OP_NEG:
            strcpy(gTextBuffer + length++, "-");
            klammer = openBracket(term, term->t_Right, &length);
            TermToText(term->t_Right, &length);
            closeBracket(klammer, &length);
            break;
        default:
            // left part of the term
            klammer = openBracket(term, term->t_Left, &length);
            if (!klammer && term->t_Op == OP_POT && term->t_Left->t_Op == OP_POT)
            {
                strcpy(gTextBuffer + length++, "(");
                klammer = TRUE;
            }
            TermToText(term->t_Left, &length);

            closeBracket(klammer, &length);

#ifdef __amigaos4__
			//Die if-Abfrage verhindert zwei = in Objekten wie zB das Text-Objekt.
			//ist erstmal eine Notlösung!
			if(!(gTextBuffer[0] == '=' && term->t_Op == OP_EQUAL && length == 1))
            	strcpy(gTextBuffer + length, Ops(term->t_Op));
#else
            strcpy(gTextBuffer + length, Ops(term->t_Op));
#endif
            length += strlen(gTextBuffer + length);

            // right part of the term

            klammer = openBracket(term, term->t_Right, &length);
            if (!klammer && (term->t_Op == OP_SUB || term->t_Op == OP_DIV) && term->t_Right->t_Pri == term->t_Pri)
            {
                strcpy(gTextBuffer + length++, "(");
                klammer = TRUE;
            }
            TermToText(term->t_Right, &length);

            closeBracket(klammer, &length);
            break;
    }

    // update the position to the current length
    length += strlen(gTextBuffer + length);
    *_pos = length;
}


/** Gibt die zu einem Term gehörende Formel als automatisch
 *  generierten String aus.
 *  Der zurückgegebene String bleibt bis zum nächsten Aufruf
 *  dieser Funktion oder von TreeTerm() erhalten.
 *  Wenn im Term Bezüge vorhanden sind, müssen tf_col und
 *  tf_row korrekt gesetzt sein.
 *
 *  @param t Zeiger auf den Term
 *  @param formula wenn TRUE wird dem String ein '=' vorangestellt
 *  @return s die Formel
 *
 *  @note Diese Funktion ist nicht reentrant; sie benutzt den
 *      globalen String-Puffer.
 */

STRPTR StaticTreeTerm(struct Term *t, bool formula)
{
    if (EnlargeTextBuffer(0))
    {
        uint32 pos;

        if (formula)
            strcpy(gTextBuffer, "=");
        else
            *gTextBuffer = '\0';

        pos = strlen(gTextBuffer);
        TermToText(t, &pos);
    }
    return gTextBuffer;
}


/** Gibt die zu einem Term gehörende Formel als automatisch
 *  generierten String aus.
 *  Der zurückgegebene String wird dynamisch erstellt und
 *  muß mit FreeString() freigegeben werden.
 *  Wenn im Term Bezüge vorhanden sind, müssen tf_col und
 *  tf_row korrekt gesetzt sein.
 *
 *  @param t Zeiger auf den Term
 *  @param formula wenn TRUE wird dem String ein '=' vorangestellt
 *  @return s die Formel
 *
 *  @note Diese Funktion ist nicht reentrant.
 */

STRPTR
TreeTerm(struct Term *t, BOOL formula)
{
    return AllocString(StaticTreeTerm(t, formula));
}


/** Gibt alle zu dem übergebenen Term gehörenden
 *  Speicherbereiche frei.
 *
 *  @param t der freizugebende Term (darf NULL sein)
 */

void PUBLIC
DeleteTree(REG(a0, struct Term *t))
{
    struct FuncArg *fa;

    if (!t)
        return;

    DeleteTree(t->t_Left);
    DeleteTree(t->t_Right);

    switch(t->t_Op)
    {
        case OP_NAME:
        case OP_TEXT:
            FreeString(t->t_Text);
            break;
        case OP_EXTCELL:
            FreeString(t->t_Page);
            break;
        case OP_FUNC:
            while ((fa = (struct FuncArg *)MyRemHead(&t->t_Args)) != 0)
            {
                DeleteTree(fa->fa_Root);
                FreePooled(pool,fa,sizeof(struct FuncArg));
            }
            break;
		default:
			break;
    }
    FreePooled(pool, t, sizeof(struct Term));
}


struct Term *
CreateTreeFrom(struct Page *page, long col, long row, STRPTR t)
{
	calcpage = page;
	tf_col = col;
	tf_row = row;
	
	return createTree(t, &tree_stack, &tree_size);
}


struct Term * PUBLIC
CreateTree(REG(a0, struct Page *page),REG(a1, STRPTR t))
{
    struct Term *te;
    
    calcpage = page;
    tf_col = 0;
    tf_row = 0;
    te =  createTree(t, &tree_stack, &tree_size);
	return te;
}


#ifdef __amigaos4__
struct Term *
createTree(STRPTR t, void **stack, uint32 *_size)
{
    struct Term **num_stack;
    long   *op_stack,sp_num,sp_op;
    long   i,pri;
    STRPTR st = t;
    uint32 size = *_size;
    char   c,buff[64];
    OPs    op,op2;
    BOOL   negoper = FALSE; //wenn true ist der operand der potenz-funktion negativ

    if (!t)
        return NULL;

    if (*stack)
    {
        num_stack = *stack;
        op_stack = (int32 *)((uint8 *)*stack + (size >> 1));
    }
    sp_num = 0;  sp_op = 0;

    for (; (c = *t) != 0; t++)
    {
        // allocate or enlarge operator stack
        if (!*stack || sp_num > (size >> 1))
        {
            APTR temp;

            if ((temp = AllocPooled(pool, size + TREESIZE)) != 0)
            {
                if (*stack)
                {
                    CopyMem(*stack, temp, size >> 1);
                    CopyMem((uint8 *)*stack + (size >> 1), (uint8 *)temp + ((size + TREESIZE) >> 1), size >> 1);
                    FreePooled(pool, *stack, size);
                }
                *_size += TREESIZE;  size = *_size;
                *stack = temp;

                num_stack = *stack;
                op_stack = (int32 *)((uint8 *)*stack + (size >> 1));
            }
            else
                return NULL;
        }

        // white space is ignored
        if (c == ' ' || c == '\t')
            continue;

        if (c == '(')
            op_stack[sp_op++] = OP_OPEN;
        else if (IsAlNum(loc, c) || c == '$' || c == '@')
        {
            if (IsAlpha(loc, c) || c == '$')
            {
                // If it's a letter, it can only be a name, a reference, a function, or a page reference
                bool specialCharacter = false;

                for (i = 0; IsAlNum(loc, c) || c == '.' || c == '$' || c == '_'; c = *(++t))
                {
                    if (i < sizeof(buff) - 1)
                        buff[i++] = ToLower(c);
                    if (c == '.' || c == '_') 
                        specialCharacter = true;      /* Punkt oder Unterstrich? */
                }
                buff[i] = '\0';

                if (*t != '(')
                {
                    if (!specialCharacter && isdigit(*(t - 1)))        /* Dann kann es schon kein Bezug mehr sein */
                        num_stack[sp_num++] = neuesFeld(buff);
                    else
                        num_stack[sp_num++] = neuerName(buff);
                }
                else
                {
                    num_stack[sp_num++] = neueFunktion(buff, ++t);
                    for(i = 1;i && *t;t++)
                    {
                        if (*t == ')')
                            i--;
                        if (*t == '(')
                            i++;
                    }
                }
            }
            else if(c == '@')
            {
                num_stack[sp_num++] = neuerSeitensprung(&t);
            }
            else
            {
                num_stack[sp_num++] = neuerOperand(&t);
            }
            t--;
        }
        else if (c == '"' || c == '\'' || c == '`')
        {
            char *text = parseText(&t);

            if (text)
            {
                if (!*t)
                {
                    FreeString(text);
                    t--;
                    continue;
                }

                if (t[1] == '@')
                {
                    t--;
                    num_stack[sp_num++] = neuerSeitensprung(&t);//Problem?!?!
                }
                else
                    num_stack[sp_num++] = newText(text, *t);
            }
        }
        else if (c == ')')
        {
            for (;;)
            {
                if (sp_op <= 0)
                    return NULL;

                op = op_stack[--sp_op];
                if (op == OP_OPEN)
                    break;

                verwendeOperator(op, num_stack, &sp_num);
            }
        }
        else
        {
            op = op2 = Operator(c);
            t++;

            // filter out the two-character operators
            // ToDo: let the Operator() function handle this!
            switch (op)
            {
                case OP_GREATER:
                    if (*t == '=') op = OP_EQGREATER;
                    break;
                case OP_LESS:
                    if (*t == '=') op = OP_EQLESS;
                    break;
                case OP_FAK:
                    if (*t == '=') op = OP_NOTEQUAL;
                    break;
                case OP_EQUAL:
                    if (*t == '=') t++;
                    break;
                case OP_BITAND:
                    if (*t == '&') op = OP_AND;
                    break;
                case OP_BITOR:
                    if (*t == '|') op = OP_OR;
                    break;
				case OP_POT:
				    if (*t == '-') //negative potenzvalue
				    {
				        t++;  //ignore - sign
				        negoper = TRUE;
				    }
				    break;
				default:
					break;
            }
            if (op == op2)
                t--;
            pri = Priority(op);
            if (op == OP_SUB && (t == st || *(t-1) == '(' || Operator(*(t-1)) != OP_NONE))
            {
                op = OP_NEG;
                pri = PRI_NEG;
            }
/******************************************************************************************/
            while (sp_op > 0)
            {
                int32 p;

                op2 = op_stack[sp_op-1];
                p = Priority(op2);
                if (op2 == OP_OPEN || p < pri || p == pri && p == PRI_POT || !sp_op)
                    break;
                sp_op--;
                verwendeOperator(op2, num_stack, &sp_num);
            }
            op_stack[sp_op++] = op;
			if(negoper)		//schreibt OP_NEG auf den stack, wenn ein negativer Exp wert vorliegt
			{
				op_stack[sp_op++] = OP_NEG;
				negoper = FALSE;
			}
        }
    }
    while (sp_op > 0)
    {
        op2 = op_stack[--sp_op];
        if (op2 == OP_CLOSE)
            return NULL;
        verwendeOperator(op2, num_stack, &sp_num);
    }
    if (sp_num == 0)
        return NULL;

    return *num_stack;
}
#else
struct Term *
createTree(STRPTR t, void **stack, uint32 *_size)
{
    struct Term **num_stack;
    long   *op_stack,sp_num,sp_op;
    long   i,pri;
    STRPTR st = t;
    uint32 size = *_size;
    char   c,buff[64];
    OPs    op,op2;

    if (!t)
        return NULL;

    if (*stack)
    {
        num_stack = *stack;
        op_stack = (int32 *)((uint8 *)*stack + (size >> 1));
    }
    sp_num = 0;  sp_op = 0;

    for (; (c = *t) != 0; t++)
    {
        // allocate or enlarge operator stack
        if (!*stack || sp_num > (size >> 1))
        {
            APTR temp;

            if ((temp = AllocPooled(pool, size + TREESIZE)) != 0)
            {
                if (*stack)
                {
                    CopyMem(*stack, temp, size >> 1);
                    CopyMem((uint8 *)*stack + (size >> 1), (uint8 *)temp + ((size + TREESIZE) >> 1), size >> 1);
                    FreePooled(pool, *stack, size);
                }
                *_size += TREESIZE;  size = *_size;
                *stack = temp;

                num_stack = *stack;
                op_stack = (int32 *)((uint8 *)*stack + (size >> 1));
            }
            else
                return NULL;
        }

        // white space is ignored
        if (c == ' ' || c == '\t')
            continue;

        if (c == '(')
            op_stack[sp_op++] = OP_OPEN;
        else if (IsAlNum(loc, c) || c == '$')
        {
            if (IsAlpha(loc, c) || c == '$')
            {
                // If it's a letter, it can only be a name, a reference, a function, or a page reference
                bool specialCharacter = false;

                for (i = 0; IsAlNum(loc, c) || c == '.' || c == '$' || c == '_'; c = *(++t))
                {
                    if (i < sizeof(buff) - 1)
                        buff[i++] = ToLower(c);
                    if (c == '.' || c == '_')
                        specialCharacter = true;      /* Punkt oder Unterstrich? */
                }
                buff[i] = '\0';
                if (*t != '(')
                {
                    if (!specialCharacter && isdigit(*(t - 1)))        /* Dann kann es schon kein Bezug mehr sein */
                        num_stack[sp_num++] = neuesFeld(buff);
                    else if (*t == '#')
                        num_stack[sp_num++] = neuerSeitensprung(AllocString(buff), &t);
                    else
                        num_stack[sp_num++] = neuerName(buff);
                }
                else
                {
                    num_stack[sp_num++] = neueFunktion(buff, ++t);
                    for(i = 1;i && *t;t++)
                    {
                        if (*t == ')')
                            i--;
                        if (*t == '(')
                            i++;
                    }
                }
            }
            else
                num_stack[sp_num++] = neuerOperand(&t);
            t--;
        }
        else if (c == '"' || c == '\'' || c == '`')
        {
            char *text = parseText(&t);

            if (text)
            {
                if (!*t)
                {
                    FreeString(text);
                    t--;
                    continue;
                }

                if (t[1] == '#')
                {
                    t++;
                    num_stack[sp_num++] = neuerSeitensprung(text, &t);
                }
                else
                    num_stack[sp_num++] = newText(text, *t);
            }
        }
        else if (c == ')')
        {
            for (;;)
            {
                if (sp_op <= 0)
                    return NULL;

                op = op_stack[--sp_op];
                if (op == OP_OPEN)
                    break;

                verwendeOperator(op, num_stack, &sp_num);
            }
        }
        else
        {
            op = op2 = Operator(c);
            t++;

            // filter out the two-character operators
            // ToDo: let the Operator() function handle this!
            switch (op)
            {
                case OP_GREATER:
                    if (*t == '=') op = OP_EQGREATER;
                    break;
                case OP_LESS:
                    if (*t == '=') op = OP_EQLESS;
                    break;
                case OP_FAK:
                    if (*t == '=') op = OP_NOTEQUAL;
                    break;
                case OP_EQUAL:
                    if (*t == '=') t++;
                    break;
                case OP_BITAND:
                    if (*t == '&') op = OP_AND;
                    break;
                case OP_BITOR:
                    if (*t == '|') op = OP_OR;
                    break;
				default:
					break;
            }
            if (op == op2)
                t--;
            pri = Priority(op);
            if (op == OP_SUB && (t == st || *(t-1) == '(' || Operator(*(t-1)) != OP_NONE))
            {
                op = OP_NEG;
                pri = PRI_NEG;
            }
/******************************************************************************************/
            while (sp_op > 0)
            {
                int32 p;

                op2 = op_stack[sp_op-1];
                p = Priority(op2);
                if (op2 == OP_OPEN || p < pri || p == pri && p == PRI_POT || !sp_op)
                    break;
                sp_op--;
                verwendeOperator(op2, num_stack, &sp_num);
            }
            op_stack[sp_op++] = op;
        }
    }
    while (sp_op > 0)
    {
        op2 = op_stack[--sp_op];
        if (op2 == OP_CLOSE)
            return NULL;
        verwendeOperator(op2, num_stack, &sp_num);
    }
    if (sp_num != 1)
        return NULL;

    return *num_stack;
}
#endif

void
GetFormula(struct Page *page,struct tableField *tf)
{
    tf->tf_Type = TFT_FORMULA;
    tf->tf_Flags &= TFF_FORMATSET | TFF_KOMMASET;

    tf_col = tf->tf_Col;  tf_row = tf->tf_Row;
    calcpage = page;
    if ((tf->tf_Root = createTree(tf->tf_Text + 1, &tree_stack, &tree_size)) != 0)
    {
        FreeString(tf->tf_Text);  tf->tf_Text = NULL;

        calcerr = CT_OK;
        CalcTableField(tf);
        CalcError(calcerr);
    }
    else
    {
        if (calcflags & CF_REQUESTER)
            DisplayBeep(NULL);
        tf->tf_Text = AllocString("#err-syntax");
    }
}


void
RecalcObject(struct Page *page, struct gObject *go)
{
    if (!page || !go)
        return;

    if (gDoMethod(go,GCM_RECALC) == GCPR_REDRAW)
    {
        struct gObject *rgo;
        struct Link *l;

        DrawSingleGObject(page, go);

        foreach(&go->go_ReferencedBy, l)
        {
            if ((rgo = l->l_Link) != 0)
                DrawSingleGObject(rgo->go_Page,rgo);
        }
    }
}


static void
RecalcTableField(struct tableField *tf)
{
    CalcTableField(tf);

    if (tf->tf_Flags & TFF_REFRESH)
    {
        tf->tf_Flags &= ~TFF_REFRESH;

        DrawTableField(calcpage, tf);
    }
}


void
RecalcMapPages(struct Mappe *mp)
{
    struct tableField *tf;
#ifdef __amigaos4__
	struct Page *actpage;			//meiner meinung nach darf calcpage nicht verändert werden, da globale Variable. Sie wird aber global in unterfkt. benutzt. deshalb sichere ich hier startzustand
									//um Crashes unter AOS4.x beim löschen von Namensreferencen zu verhindern. diese nutzung von glob. variablen muss geändert werden.
	actpage = calcpage;
#endif
    D(bug("recalcMapPages()\n"));

    mp_flags = 0;
	foreach (&mp->mp_Pages, calcpage)      // Alle Zellen als überarbeitungswürdig markieren
    {
		if (handleEvent(calcpage, EVT_CALC, 0, 0))
            return;

		foreach (&calcpage->pg_Table, tf)
            tf->tf_Flags &= ~TFF_ACTUAL;
    }
	RefreshMaskFields(mp, TRUE);
	foreach (&mp->mp_Pages, calcpage)
    {
		foreach (&calcpage->pg_Table, tf)
        {
            if (tf->tf_Type & TFT_FORMULA)
            {
                if (!(tf->tf_Flags & TFF_ACTUAL))
                    CalcTableField(tf);
                if (tf->tf_Flags & TFF_REFRESH)
                {
                    tf->tf_Flags &= ~TFF_REFRESH;
                    DrawTableField(calcpage,tf);
                }
            }
        }
    }
    foreach(&mp->mp_Pages,calcpage)
    {
        struct gObject *go;

        foreach(&calcpage->pg_gObjects,go)
            RecalcObject(calcpage,go);

        foreach(&calcpage->pg_gDiagrams,go)
            RecalcObject(calcpage,go);
    }
    mp->mp_Flags = (mp->mp_Flags & ~(MPF_CUSIZE | MPF_CUTIME)) | mp_flags;
#ifdef __amigaos4__
	calcpage = actpage;
#endif
}


void
RecalcMap(struct Mappe *mp)
{
    if (mp)
        RecalcMapPages(mp);
    else
    {
		foreach(&gProjects, mp)
            RecalcMapPages(mp);
    }
}


void
RecalcTableFields(struct Page *page)
{
    RecalcMapPages(page->pg_Mappe);
}


void
RecalcReferencesList(struct ArrayList *al)
{
    int32 count = al->al_Last;

#ifdef __amigaos4__
	if(al->al_Last > al->al_Size || al->al_Size % 8 || al->al_Last > 10000 || al->al_Size > 10000 || al->al_Last < 0 || al->al_Size < 0)	//catch some error-cases that could appear in other functions
	{
	    DebugPrintF("RecalcReferencesList: Pointer: 0x%08X Last:%8d Size:%8d\n", al, al->al_Last, al->al_Size);
	    al->al_Last = al->al_Size = 0; al->al_List = NULL;
		return;
	}
#endif
	while (count-- > 0)
        RecalcReferencingObjects(al->al_List[count], true);
}


void
RecalcReferencingObjects(struct Reference *r, bool recalcThis)
{
    struct tableField *tf;

    if (!r)
        return;

    // recalc the object which belongs to this reference
    if (r->r_This != NULL && recalcThis)
    {
        if (ReferenceType(r) == RTYPE_CELL && (tf = r->r_This)->tf_Type & TFT_FORMULA)
        {
            calcpage = r->r_Page;
            RecalcTableField(tf);
        }
        else if (ReferenceType(r) == RTYPE_OBJECT)
        {
            calcpage = r->r_Page;
            RecalcObject(calcpage, (struct gObject *)r->r_This);
        }
    }

    // recalc all objects referencing the current
    RecalcReferencesList(&r->r_ReferencedBy);
}


void
InitCalc(void)
{
	empty_fv.fv_Node.ln_Name = GetString(&gLocaleInfo, MSG_EMPTY_FORMAT);
}
 
