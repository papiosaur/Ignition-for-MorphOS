/* Internal computation functions, and management
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"


#define BUILTIN_FUNCTIONLANGUAGE "__ign"


#ifdef __amigaos4__
	extern struct Name *GetName(STRPTR t);
#endif
extern struct tableField *GetRangeCells(struct Term *t,struct tableField *tf);
extern long GetRangeSize(struct Term *t);
extern struct Page *GetExtCalcPage(struct Term *t);
extern struct tableField *GetExtTableField(struct Term *t);
extern double fak(double val);
extern BOOL schaltjahr(long jahr);
extern void tagedatum(long days,long *tag,long *monat,long *jahr);
extern long monthlength(long m,long jahr);
extern long weekday(long days);

#ifdef __amigaos4__
extern long mp_flags, mday[];
extern UWORD calcerr;
#else
extern long mp_flags, mday[], calcerr;
#endif
extern STRPTR itaPoint;
extern struct MinList flangs;


void
MakeTermAbsolute(struct Term *t)
{
    if (t->t_Left)
        MakeTermAbsolute(t->t_Left);
    if (t->t_Right)
        MakeTermAbsolute(t->t_Right);

    if (t->t_Op == OP_CELL)
    {
        if (!t->t_AbsCol)
        {
            t->t_AbsCol = TRUE;
            t->t_Col += tf_col;
        }
        if (!t->t_AbsRow)
        {
            t->t_AbsRow = TRUE;
            t->t_Row += tf_row;
        }
    }
}


/******************************** alle internen Funktionen ********************************/


static double
perzentile(double perzentil,struct FuncArg *arg)
{
    struct FuncArg *fa;
    struct tableField *tf = NULL;
    double *values;
    long   num,i;

    if (perzentil > 1.0)
        perzentil = 1.0;
    else if (perzentil < 0.0)
        perzentil = 0.0;

    for(num = 0,fa = arg;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
        num += GetRangeSize(fa->fa_Root);

    if ((values = AllocPooled(pool, sizeof(double)*num)) != 0)
    {
        for(i = 0,fa = arg;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
        {
            while ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
                values[i++] = tf->tf_Value;
        }
        dqsort(values,num);
        i = (long)(perzentil *= num+1);

        if (i < 1)
            perzentil = values[0];
        else if (i > num)
            perzentil = values[num-1];
        else if (perzentil != (double)i)
            perzentil = (values[i-1]+values[i])/2.0;
        else
            perzentil = values[i-1];

		FreePooled(pool, values, sizeof(double) * num);
		return perzentil;
    }
	return 0.0;
}


static double
cfPerzentil(struct MinList *args)
{
	return perzentile(TreeValue(((struct FuncArg *)args->mlh_Head)->fa_Root),(APTR)args->mlh_Head->mln_Succ);
}


static double
cfMedian(struct MinList *args)
{
    return perzentile(0.5,(APTR)args->mlh_Head);
}


static double
cfVarianz(struct MinList *args)
{
    struct tableField *tf = NULL;
    struct FuncArg *fa;
    double val = 0.0,mittel = 0.0;
    long   num = 0;

#ifndef __amigaos4__
    foreach(args,fa)
        num += GetRangeSize(fa->fa_Root);
#else
    foreach(args,fa)
    {
        if(GetRangeSize(fa->fa_Root))
			num += GetRangeSize(fa->fa_Root);
		else
		{
			num++;
			val += TreeValue(fa->fa_Root) * TreeValue(fa->fa_Root);
		}
    }
#endif
    foreach(args,fa)
    {
        mittel += TreeValue(fa->fa_Root);
    }
    mittel = (mittel*mittel)/num;
    foreach(args,fa)
    {
        while((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
            val += tf->tf_Value * tf->tf_Value;
    }
    return (val - mittel) / (num - 1);
}


static double
cfSumme(struct MinList *args)
{
    struct FuncArg *fa;
    double val = 0.0;

    foreach(args,fa)
        val += TreeValue(fa->fa_Root);

    return val;
}

#ifdef __amigaos4__
static double
cfMode(struct MinList *args)
{
    struct FuncArg *fa;
    double aval = DBL_MAX, mval = DBL_MAX;
    long  acount = 0, mcount = 0;
    long maxargs = 0;
    double *values;
	int i;
    struct tableField *tf = NULL;
    
    for(fa = (APTR)args->mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ) //count arguments
        if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
        {
            do
            {
                if(tf->tf_Type != 0 && tf->tf_Type != TFT_EMPTY) //do not count empty fields !
                    maxargs++;
            }
            while((tf = GetRangeCells(fa->fa_Root, tf)) != 0);
        }
        else
            maxargs++;


	if (!(values = AllocPooled(pool, sizeof(double) * maxargs))) //alloc array for values
    {
        calcerr = CTERR_ARGS;
        return 0;
    }
		
	maxargs = 0; //reset counter, to use it as index
	tf = NULL;

    for(fa = (APTR)args->mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ) //read values
    {
        if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
        {
            do
            {
                if(tf->tf_Type != 0 && tf->tf_Type != TFT_EMPTY) //do not count empty fields !
                	values[maxargs++] = tf->tf_Value;
            }
            while((tf = GetRangeCells(fa->fa_Root, tf)) != 0);
        }
        else
            values[maxargs++] = TreeValue(fa->fa_Root);
    }

	dqsort(values, maxargs); //sort values
	
	for(i = maxargs - 1; i >= 0; i--) //determine the lowest values with the most appears
	{
	    if(values[i] < aval) //found a new value
	    {
	    	aval = values[i]; //set actual parameters
	    	acount = 1;
	    }
	    else
	    	acount++;	//increment counter
        if(aval <= mval && acount >= mcount) //check if a new maximum is available
        {
            mval = aval; //set new result values
            mcount = acount;
        }
	}
	FreePooled(pool, values, sizeof(double) * maxargs);
    return mval;
}
#endif

static double
cfProdukt(struct MinList *args)
{
    struct tableField *tf = NULL;
    struct FuncArg *fa;
    double val = 1.0;

    foreach(args,fa)
    {
        if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
        {
            do
            {
                val *= tf->tf_Value;
            }
            while((tf = GetRangeCells(fa->fa_Root, tf)) != 0);
        }
        else
            val *= TreeValue(fa->fa_Root);
    }
    return val;
}


static double
cfAnzahl(struct MinList *args)
{
    struct tableField *tf = NULL;
    struct FuncArg *fa;
    double val = 0.0;
#ifdef __amigaos4__ 
	struct Name *name[50];
	int8 ncount = 0;

	name[0] = NULL;
#endif


    foreach(args,fa)
    {
#ifdef __amigaos4__
		if(fa->fa_Root->t_Op == OP_NAME && (name[ncount] = GetName(fa->fa_Root->t_Text))) 
	    	ncount++;
#endif
        while ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
        {
           if (tf->tf_Text)
                val += 1.0;
        }
    }
#ifdef __amigaos4__
	//the processing of the NAME's must be done external and not in the loop
	//because it corrupts the calculation-engine and generates grim's
	//when including other parameters then NAME's after a NAME
	if(ncount) 
	{
	    uint32 col, row;
		    
		for(ncount--; ncount >= 0; ncount--)
	    {
				if(strchr(name[ncount]->nm_Content, ':'))
		    		for(row = name[ncount]->nm_Root->t_Left->t_type.t_cell.t_row; row <= name[ncount]->nm_Root->t_Right->t_type.t_cell.t_row; row++)
		    		{
				    	for(col = name[ncount]->nm_Root->t_Left->t_type.t_cell.t_col; col <= name[ncount]->nm_Root->t_Right->t_type.t_cell.t_col; col++)
					    { 
			    			if(tf = GetTableField(name[ncount]->nm_Page, col, row))
			    			{
					            if (tf->tf_Text)
            					    val += 1.0;
		    				}
			    		}
			    	}
			    else
			    {
			        String2Coord(name[ncount]->nm_Content, &col, &row);
	    			if(tf = GetRealTableField(name[ncount]->nm_Page, col, row))
	    			{
			            if (tf->tf_Text)
           				    val += 1.0;
	    			}
	    		}
		}
	}
#endif
	return val;
}


static double
cfFak(struct MinList *args)
{
    struct FuncArg *fa;
#ifdef __amigaos4__
	double value;
	
    fa = (APTR)args->mlh_Head;
    value = TreeValue(fa->fa_Root);
    if(value < 0.0 || (double)((int32)value) != value || value > 83.0)
    {
        calcerr = CTERR_ARGS;
        return 0;
    }
    return fak(value);
#else
    fa = (APTR)args->mlh_Head;
	return fak(TreeValue(fa->fa_Root));
#endif
}

#ifdef __amigaos4__
static double
cfIstLeer(struct MinList *args)
{
    struct FuncArg *fa;
    struct tableField *tf = NULL;
	
    fa = (APTR)args->mlh_Head;
    if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
    {
    	if(tf->tf_Type == 0 || tf->tf_Type == TFT_EMPTY)
            return 1.0;
        return 0.0;
    }
    return 0.0;
}

static double
cfIstDatum(struct MinList *args)
{
    struct FuncArg *fa;
    struct tableField *tf = NULL;
	
    fa = (APTR)args->mlh_Head;
    if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
    {
    	if((tf->tf_Type == 2 || tf->tf_Type == 6) && tf->tf_Format != NULL) 
    	{
	    	if((strstr(tf->tf_Format, "#Y") || strstr(tf->tf_Format, "#y"))) 
    	        return 1.0;
    	}
        return 0.0;
    }
    return 0.0;
}

static double
cfIstZeit(struct MinList *args)
{
    struct FuncArg *fa;
    struct tableField *tf = NULL;
	
    fa = (APTR)args->mlh_Head;
    if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
    {
    	if((tf->tf_Type == 2 || tf->tf_Type == 6) && tf->tf_Format != NULL) 
    		if((strstr(tf->tf_Format, "#M") || strstr(tf->tf_Format, "#m")) && (strstr(tf->tf_Format, "#H") || strstr(tf->tf_Format, "#h"))) 
    		{
            	return 1.0;
            }
        return 0.0;
    }
    return 0.0;
}

static double
cfIstZahl(struct MinList *args)
{
    struct FuncArg *fa;
    struct tableField *tf = NULL;
	double value;
	STRPTR t;
	
    fa = (APTR)args->mlh_Head;
    tf = GetRangeCells(fa->fa_Root, tf);
  	t = TreeText(fa->fa_Root);
   	value = TreeValue(fa->fa_Root);
   	calcerr = 0;
	return((t == NULL && (tf == NULL ? 0 : tf->tf_Text != NULL)) || value != 0.0  ? 1.0 : 0.0);
}

static double
cfIstText(struct MinList *args)
{
    struct FuncArg *fa;
    struct tableField *tf = NULL;
	double value;
	STRPTR t;
	
    fa = (APTR)args->mlh_Head;
    tf = GetRangeCells(fa->fa_Root, tf);
  	t = TreeText(fa->fa_Root);
   	value = TreeValue(fa->fa_Root);
   	calcerr = 0;
	return((value == 0.0 && (tf == NULL ? 0 : tf->tf_Text != NULL)) || t != NULL  ? 1.0 : 0.0);
    //if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
    //{
////printf("type=%d text=<%s>\n", tf->tf_Type, tf->tf_Text);
		//if((tf->tf_Type == 1 || tf->tf_Type == 3) && Strlen(tf->tf_Text))
            //return 1.0;
        //return 0.0;
    //}
    //return 0.0;
}

static double
cfVorzeichen(struct MinList *args)
{
    struct FuncArg *fa;
	double value;
	
    fa = (APTR)args->mlh_Head;
    value = TreeValue(fa->fa_Root);
    if(value < 0.0)
    	return -1.0;
    else if(value > 0.0)
    	return 1.0;
    else
    	return 0.0;
}

static double
cfInt(struct MinList *args)
{
    struct FuncArg *fa;
	double value;
	
    fa = (APTR)args->mlh_Head;
    value = TreeValue(fa->fa_Root);
    return((int32)value);
}

static double
cfQrt(struct MinList *args)
{
    struct FuncArg *fa;
	double value;
	
    fa = (APTR)args->mlh_Head;
    value = TreeValue(fa->fa_Root);
    return(value * value);
}

static double
cfRunden(struct MinList *args)
{
    struct FuncArg *fa;
    long   len = 0, offset, i;
    double value = 0.0;
    char str[80], fstr[80];

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        value = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
	        len = (long)TreeValue(fa->fa_Root);
	    if(len < 0) 
	    	len = 0;
        sprintf(fstr, "%%.%ldlf", len);
   	    sprintf(str, fstr, value + 0.000000000000001);
   	    if(len = 0)
   	    	value = round(value);
   	    else
	       	value = atof(str);
    }
	return value;
}
#endif



static double
cfMittelwert(struct MinList *args)
{
    struct FuncArg *fa;
    struct tableField *tf = NULL;
    double val = 0.0;
    long   num = 0;

	foreach(args, fa)
    {
        if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
        {
            do
            {
                if (tf->tf_Flags & TFF_LOCKED)
                    calcerr = CTERR_LOOP;
                val += tf->tf_Value;
#ifdef __amigaos4__
                if(tf->tf_Type != 0 && tf->tf_Type != TFT_EMPTY) //do not count empty fields !
                	num++;
#endif
            }
            while((tf = GetRangeCells(fa->fa_Root, tf)) != 0);

#ifndef __amigaos4__
            num += GetRangeSize(fa->fa_Root);
#endif
        }
        else
        {
            val += TreeValue(fa->fa_Root);
            num++;
        }
    }
    if (num)
		return val / num;

	return 0.0;
}


static double
cfMinMax(BOOL isMax, struct MinList *args)
{
    struct tableField *tf = NULL;
    struct FuncArg *fa;
    double val = 0.0,op;
    long   pos;
    BOOL   isFirst = TRUE;

    for(fa = (APTR)args->mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
    {
        if ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
        {
            pos = 0;
            do
            {
#ifdef __amigaos4__
                if(tf->tf_Type != 0 && tf->tf_Type != TFT_EMPTY) //do not count empty fields !
                {
#endif
                op = tf->tf_Value;
                if (isFirst || isMax && val < op || !isMax && val > op)
                    val = op;
#ifdef __amigaos4__
                }
#endif
                isFirst = FALSE;
                pos++;
            }
            while((tf = GetRangeCells(fa->fa_Root, tf)) != 0);

            if (!isMax && pos < GetRangeSize(fa->fa_Root) && val > 0.0)
                val = 0.0;
        }
        else
        {
            op = TreeValue(fa->fa_Root);
            if (isFirst || isMax && val < op || !isMax && val > op)
                val = op;
        }
        isFirst = FALSE;
    }
	return val;
}


static double
cfMin(struct MinList *args)
{
    return cfMinMax(FALSE, args);
}


static double
cfMax(struct MinList *args)
{
    return cfMinMax(TRUE, args);
}


static double
cfGreatestCommonDivisor(struct MinList *args)
{
    struct FuncArg *fa = (struct FuncArg *)args->mlh_Head;
    int32 u = fabs(TreeValue(fa->fa_Root));
    int32 v = fabs(TreeValue(((struct FuncArg *)fa->fa_Node.mln_Succ)->fa_Root));

    while (v != 0)
    {
        int32 r = u % v;

        u = v;
        v = r;
    }
    return (double)u;
}


static double
cfSummeAus(struct MinList *args)
{
    struct Term *term;
    struct tablePos tp;
    long   col,row;
    double val = 0.0;

    term = ((struct FuncArg *)args->mlh_TailPred)->fa_Root;

    if (FillTablePos(&tp,((struct FuncArg *)args->mlh_Head)->fa_Root))
    {
        tp.tp_Width += tp.tp_Col;  tp.tp_Height += tp.tp_Row;
        col = tf_col;  row = tf_row;

        for(tf_row = tp.tp_Row;tf_row <= tp.tp_Height;tf_row++)
        {
            for(tf_col = tp.tp_Col;tf_col <= tp.tp_Width;tf_col++)
                val += TreeValue(term);
        }
        tf_col = col;  tf_row = row;
    }
	return val;
}


static double
cfGrad(struct MinList *args)
{
	struct FuncArg *fa = (APTR)args->mlh_Head;

	return TreeValue(fa->fa_Root) * 180 / PI;
}


static double
cfBogen(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return TreeValue(fa->fa_Root) * PI/180;
}


static double
cfSin(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return sin(TreeValue(fa->fa_Root));
}


static double
cfCos(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return cos(TreeValue(fa->fa_Root));
}


static double
cfTan(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return tan(TreeValue(fa->fa_Root));
}


static double
cfSinHyp(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return sinh(TreeValue(fa->fa_Root));
}


static double
cfCosHyp(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return cosh(TreeValue(fa->fa_Root));
}


static double
cfTanHyp(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return tanh(TreeValue(fa->fa_Root));
}


static double
cfArcSin(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return asin(TreeValue(fa->fa_Root));
}


static double
cfArcCos(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return acos(TreeValue(fa->fa_Root));
}


static double
cfArcTan(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

    return atan(TreeValue(fa->fa_Root));
}


static double
cfRandom(struct MinList *args)
{
	return drand48();
}


static double
cfAbs(struct MinList *args)
{
    struct FuncArg *fa;

    fa = (APTR)args->mlh_Head;
	return fabs(TreeValue(fa->fa_Root));
}


static double
cfExp(struct MinList *args)
{
    struct FuncArg *fa;

    fa = (APTR)args->mlh_Head;
	return exp(TreeValue(fa->fa_Root));
}


static double
cfLn(struct MinList *args)
{
    struct FuncArg *fa;
#ifdef __amigaos4__
	double val;
#endif

    fa = (APTR)args->mlh_Head;
#ifdef __amigaos4__
	if((val = TreeValue(fa->fa_Root)) <= 0.0)
	{
	    calcerr = CTERR_ARGS;
	    return 0.0;
	}
	return log(val);
#else
	return log(TreeValue(fa->fa_Root));
#endif
}

#ifdef __amigaos4__
static double
cfSqr(struct MinList *args)
{
    struct FuncArg *fa;
	double val;

    fa = (APTR)args->mlh_Head;
	if((val = TreeValue(fa->fa_Root)) < 0.0)
	{
	    calcerr = CTERR_ARGS;
	    return 0.0;
	}
	return sqrt(val);
}
#endif

static double
cfLog10(struct MinList *args)
{
    struct FuncArg *fa;
#ifdef __amigaos4__
	double val;
#endif

    fa = (APTR)args->mlh_Head;
#ifdef __amigaos4__
	if((val = TreeValue(fa->fa_Root)) <= 0.0)
	{
	    calcerr = CTERR_ARGS;
	    return 0.0;
	}
	return log10(val);
#else
	return log10(TreeValue(fa->fa_Root));
#endif
}


static double
cfNicht(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

	return (double)(TreeValue(fa->fa_Root) == 0.0);
}


static double
cfUnd(struct MinList *args)
{
    struct tableField *tf = NULL;
    struct FuncArg *fa;
    long   pos;

    for(fa = (APTR)args->mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
    {
        if (fa->fa_Root->t_Op == OP_RANGE)
        {
            for (pos = 0; (tf = GetRangeCells(fa->fa_Root, tf)) != 0; pos++)
            {
                if (!tf->tf_Value)
					return 0.0;
            }
            if (pos < GetRangeSize(fa->fa_Root))
				return 0.0;
        }
        else if (!TreeValue(fa->fa_Root))
			return 0.0;
    }
	return 1.0;
}


static double
cfOder(struct MinList *args)
{
    struct tableField *tf = NULL;
    struct FuncArg *fa;
    long   pos;

    for(fa = (APTR)args->mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
    {
        if (fa->fa_Root->t_Op == OP_RANGE)
        {
            for (pos = 0; (tf = GetRangeCells(fa->fa_Root, tf)) != 0; pos++)
            {
                if (tf->tf_Value)
					return 1.0;
            }
            if (pos < GetRangeSize(fa->fa_Root))
				return 0.0;
        }
        else if (TreeValue(fa->fa_Root))
			return 1.0;
    }
	return 0.0;
}

#ifdef __amigaos4__
static double
cfXOder(struct MinList *args)
{
    struct tableField *tf = NULL;
    struct FuncArg *fa;
    long   pos, count = 0;

    for(fa = (APTR)args->mlh_Head;fa->fa_Node.mln_Succ;fa = (APTR)fa->fa_Node.mln_Succ)
    {
        if (fa->fa_Root->t_Op == OP_RANGE)
        {
            for (pos = 0; (tf = GetRangeCells(fa->fa_Root, tf)) != 0; pos++)
            {
                if (tf->tf_Value)
					count++;;
            }
            if (pos < GetRangeSize(fa->fa_Root))
				return 0.0;
        }
        else if (TreeValue(fa->fa_Root))
			count++;
    }
	return (count == 1 ? 1.0 : 0.0);
}
#endif

static long
cfAuswahl(struct Result *r, struct List *args)
{
    struct FuncArg *fa;

    if ((fa = (APTR)FindListNumber((struct MinList *)args, 0)) != 0)
    {
#ifdef __amigaos4__
	    long num = 0;

        num = (long)TreeValue(fa->fa_Root);
        if (num > 0 && (fa = (APTR)FindListNumber((struct MinList *)args, num)) != 0)
#else
        if ((fa = (APTR)FindListNumber((struct MinList *)args, (long)TreeValue(fa->fa_Root))) != 0)
#endif
			return CalcTree(r, fa->fa_Root);
#ifdef __amigaos4__
	    else
    		return CTERR_ARGS;
#endif
    }
	return CT_OK;
}

#ifdef __amigaos4__
static long
cfErstelleString(struct Result *r, struct List *args)
{
    struct FuncArg *fa;
    long argno = 0;
    char str[256] = "";
    
    while((fa = (APTR)FindListNumber((struct MinList *)args, argno++)) != 0)
    {
        CalcTree(r, fa->fa_Root);
        if(r->r_Type == 1)	//formel
        {
            char str2[256];
            
            sprintf(str2, "%lf", r->r_Value); 
  	      	Strlcat(str, str2, 255);
        }
        else if(r->r_Type == 5)  //Cell with number
        {
	        Strlcat(str, r->r_Cell->tf_Text, 255);
        }
        else
	        Strlcat(str, r->r_Text, 255);
//printf("%d) <%s> <%lf> <%d> str=<%s> rcell=<%s>\n", argno, r->r_Text, r->r_Value, r->r_Type, str, (r->r_Cell != NULL ? r->r_Cell->tf_Text : "")); 
    }
    FreeString(r->r_Text);
    r->r_Type = 2; //result is string
    r->r_Text = AllocString(str);
    return CT_OK;
}
#endif

static long
cfWenn(struct Result *r,struct MinList *args)
{
    struct FuncArg *fa;

    if ((fa = (APTR)FindListNumber((struct MinList *)args, 0)) != 0)
    {
        long num = 2;

        if (TreeValue(fa->fa_Root))
            num = 1;
        if ((fa = (APTR)FindListNumber((struct MinList *)args, num)) != 0)
			return CalcTree(r, fa->fa_Root);

		return CT_OK;
    }
	return CTERR_ARGS;
}


static long
cfExtern(struct Result *r,struct MinList *args)
{
    struct FuncArg *fa;
    struct Page *page;
    struct Mappe *mp;
    struct Result pr;
    long   rc = CTERR_ARGS;
    STRPTR t;

    if ((fa = (APTR)args->mlh_Head) && fa->fa_Node.mln_Succ && (t = TreeText(fa->fa_Root)))
    {
		if ((mp = (APTR)FindTag(&gProjects, t)) && (fa = (APTR)fa->fa_Node.mln_Succ) && fa->fa_Node.mln_Succ)
        {
            memset(&pr,0,sizeof(struct Result));
            if (CalcTree(&pr,fa->fa_Root) == CT_OK && (fa = (APTR)fa->fa_Node.mln_Succ) && fa->fa_Node.mln_Succ)
            {
                page = calcpage;  calcpage = NULL;
                if (pr.r_Type & RT_TEXT)
                {
                    calcpage = (APTR)FindTag(&mp->mp_Pages,pr.r_Text);
                    FreeString(pr.r_Text);
                }
                else
                    calcpage = (APTR)FindListNumber(&mp->mp_Pages,(long)pr.r_Value-1);
                if (calcpage)
                    rc = CalcTree(r,fa->fa_Root);
                calcpage = page;
            }
        }
        FreeString(t);
    }
	return rc;
}


static long
cfSeite(struct Result *r,struct MinList *args)
{
    struct FuncArg *fa;
    struct Page *page;
    struct Result pr;
    long   rc = CTERR_ARGS;

    if ((fa = (APTR)args->mlh_Head) && fa->fa_Node.mln_Succ && fa->fa_Root)
    {
        memset(&pr,0,sizeof(struct Result));
        if (CalcTree(&pr,fa->fa_Root) == CT_OK && (fa = (APTR)fa->fa_Node.mln_Succ) && fa->fa_Node.mln_Succ)
        {
            page = calcpage;  calcpage = NULL;
            if (pr.r_Type & RT_TEXT)
            {
                calcpage = (APTR)FindTag(&page->pg_Mappe->mp_Pages,pr.r_Text);
                FreeString(pr.r_Text);
            }
            else
                calcpage = (APTR)FindListNumber(&page->pg_Mappe->mp_Pages,(long)pr.r_Value-1);
            if (calcpage)
                rc = CalcTree(r,fa->fa_Root);
            calcpage = page;
        }
    }
	return rc;
}


static void
SetTermCol(struct Term *relpos,struct Term *t)
{
    struct Result r;

    memset(&r,0,sizeof(struct Result));

    if (CalcTree(&r,relpos) == CT_OK)
    {
        if (r.r_Type & RT_TEXT)
        {
            struct tableSize *ts;
            long   col = 0;

            for(ts = calcpage->pg_tfWidth;col < calcpage->pg_Cols;col++,ts++)
            {
                if (ts->ts_Title && !StrnCmp(loc,r.r_Text,ts->ts_Title,-1,SC_COLLATE1))
                {
                    t->t_Col = col+1;
                    t->t_AbsCol = TRUE;
                    FreeString(r.r_Text);
                    return;
                }
            }
            FreeString(r.r_Text);
        }
        if (r.r_Type & RT_VALUE)
            t->t_Col = r.r_Value;
    }
}


static void
SetTermRow(struct Term *relpos,struct Term *t)
{
    struct Result r;

    memset(&r,0,sizeof(struct Result));

    if (CalcTree(&r,relpos) == CT_OK)
    {
        if (r.r_Type & RT_TEXT)
        {
            struct tableSize *ts;
            long   row = 0;

            for(ts = calcpage->pg_tfHeight;row < calcpage->pg_Rows;row++,ts++)
            {
                if (ts->ts_Title && !StrnCmp(loc,r.r_Text,ts->ts_Title,-1,SC_COLLATE1))
                {
                    t->t_Row = row+1;
                    t->t_AbsRow = TRUE;
                    FreeString(r.r_Text);
                    return;
                }
            }
            FreeString(r.r_Text);
        }
        if (r.r_Type & RT_VALUE)
            t->t_Row = r.r_Value;
    }
    return;
}


static long
cfSpalte(struct Result *r,struct MinList *args)
{
    struct FuncArg *fa;
    struct Term t;

    if ((fa = (APTR)args->mlh_Head) && fa->fa_Node.mln_Succ)
    {
        memset(&t,0,sizeof(struct Term));
        t.t_Op = OP_CELL;
        SetTermCol(fa->fa_Root,&t);

        if ((fa = (APTR)fa->fa_Node.mln_Succ) && fa->fa_Node.mln_Succ && TreeValue(fa->fa_Root))
        {
            if (t.t_Col < 1)
                t.t_Col = 1;
            t.t_AbsCol = 1;
        }
		return CalcTree(r, &t);
    }
	return CTERR_ARGS;
}


static long
cfReihe(struct Result *r,struct MinList *args)
{
    struct FuncArg *fa;
    struct Term t;

    if ((fa = (APTR)args->mlh_Head) && fa->fa_Node.mln_Succ)
    {
        memset(&t,0,sizeof(struct Term));
        t.t_Op = OP_CELL;
        SetTermRow(fa->fa_Root,&t);

        if ((fa = (APTR)fa->fa_Node.mln_Succ) && fa->fa_Node.mln_Succ && TreeValue(fa->fa_Root))
        {
            if (t.t_Row < 1)
                t.t_Row = 1;
            t.t_AbsRow = 1;
        }
		return CalcTree(r, &t);
    }
	return CTERR_ARGS;
}


static long
cfCurrent(struct Result *r,struct MinList *args)
{
    struct Term t;

    memset(&t,0,sizeof(struct Term));
    t.t_Op = OP_CELL;

	return CalcTree(r, &t);
}


static long
cfBezug(struct Result *r,struct MinList *args)
{
    struct FuncArg *fa;
    struct Term t;

    if ((fa = (APTR)args->mlh_Head) && fa->fa_Node.mln_Succ)
    {
        memset(&t,0,sizeof(struct Term));
        t.t_Op = OP_CELL;
        SetTermCol(fa->fa_Root,&t);

        if ((fa = (APTR)fa->fa_Node.mln_Succ) && fa->fa_Node.mln_Succ)
        {
            SetTermRow(fa->fa_Root,&t);

            if ((fa = (APTR)fa->fa_Node.mln_Succ) && fa->fa_Node.mln_Succ && TreeValue(fa->fa_Root))
            {
                if (t.t_Col < 1)
                    t.t_Col = 1;
                if (t.t_Row < 1)
                    t.t_Row = 1;
                t.t_AbsCol = t.t_AbsRow = 1;
            }
			return CalcTree(r, &t);
        }
    }
	return CTERR_ARGS;
}


static double
cfEndKapitalEinfach(struct MinList *args)
{
    struct FuncArg *fa;
    double start,zins,jahre;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        start = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            zins = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 2)) != 0)
            {
                jahre = TreeValue(fa->fa_Root);
				return start * (1 + zins * jahre);
            }
        }
    }
	return 0.0;
}


static double
cfEndKapitalZZ(struct MinList *args)
{
    struct FuncArg *fa;
    double start,zins,jahre;
#ifdef __amigaos4__
	double periode = 1.0;
#endif

    if ((fa = (APTR)FindListNumber(args,0)))
    {
        start = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args,1)))
        {
            zins = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args,2)))
            {
                jahre = TreeValue(fa->fa_Root);
#ifdef __amigaos4__
	            if ((fa = (APTR)FindListNumber(args,3)))
	            	periode = TreeValue(fa->fa_Root);
				if(periode == 0.0)
				{
			    	calcerr = CTERR_NULLP;
			    	return 0.0;
			    }
			    return start * (pow(1 + zins / periode, periode * jahre));
#else
				return start * pow(1 + zins, jahre);
#endif
            }
        }
    }
	return 0.0;
}


static double
cfStartKapitalZZ(struct MinList *args)
{
    struct FuncArg *fa;
	double start, zins, jahre, periode = 1.0;

    if ((fa = (APTR)FindListNumber(args,0)))
    {
        start = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args,1)))
        {
            zins = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args,2)))
            {
                jahre = TreeValue(fa->fa_Root);
#ifdef __amigaos4__
	            if ((fa = (APTR)FindListNumber(args,3)))
	            	periode = TreeValue(fa->fa_Root);
				if(periode == 0.0)
				{
			    	calcerr = CTERR_NULLP;
			    	return 0.0;
			    }
			    return start / (pow(1 + zins / periode, periode * jahre));
#else
				return start / pow(1 + zins, jahre);
#endif
            }
        }
    }
	return 0.0;
}

static double
cfLaufzeitZZ(struct MinList *args)
{
    struct FuncArg *fa;
	double start, end, zins, periode = 1.0;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        start = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            end = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 2)) != 0)
            {
                zins = TreeValue(fa->fa_Root);
#ifdef __amigaos4__
	            if ((fa = (APTR)FindListNumber(args, 3)) != 0)
                	periode = TreeValue(fa->fa_Root);
				if(start <= 0.0 || periode == 0.0 || end <= 0.0 || zins == 0.0)
				{
			    	calcerr = CTERR_NULLP;
			    	return 0.0;
			    }
#endif
				return (double)((log(end/start)/log(1 + zins/periode)) / periode);
            }
        }
    }
	return 0.0;
}

#ifdef __amigaos4__
static double
cfZinssatz(struct MinList *args)
{
    struct FuncArg *fa;
	double start, end, jahre, periode = 1.0;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        start = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            end = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 2)) != 0)
            {
                jahre = TreeValue(fa->fa_Root);
	            if ((fa = (APTR)FindListNumber(args, 3)) != 0)
           		    periode = TreeValue(fa->fa_Root);
				if(jahre <= 0.0 || end == 0.0 || start == 0.0 || periode == 0.0)
				{
			    	calcerr = CTERR_NULLP;
			    	return 0.0;
			    }
				return (double)((pow(end,(1/(jahre*periode))) / pow(start,(1/(jahre*periode)))) - 1)*periode ;
            }
        }
    }
	return 0.0;
}

static double
cfRatenendkapital(struct MinList *args)
{
    struct FuncArg *fa;
	double rate, zins, laufzeit, periode = 1.0, p;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        rate = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            zins = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 2)) != 0)
            {
                laufzeit = TreeValue(fa->fa_Root);
	            if ((fa = (APTR)FindListNumber(args, 3)) != 0)
                	periode = TreeValue(fa->fa_Root);
				if(periode == 0.0)
				{
			    	calcerr = CTERR_NULLP;
			    	return 0.0;
			    }
			    p = (1.0 + zins * (1.0/periode));
				return (double)(rate * ((pow(p, periode * laufzeit) - 1) / (p - 1)));
            }
        }
    }
	return 0.0;
}

static double
cfRatenhoehe(struct MinList *args)
{
    struct FuncArg *fa;
	double end, zins, laufzeit, periode = 1.0, p;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        end = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            zins = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 2)) != 0)
            {
                laufzeit = TreeValue(fa->fa_Root);
	            if ((fa = (APTR)FindListNumber(args, 3)) != 0)
                	periode = TreeValue(fa->fa_Root);
				if(periode == 0.0)
				{
			    	calcerr = CTERR_NULLP;
			    	return 0.0;
			    }
			    p = (1.0 + zins * (1.0/periode));
				return (double)(end / ((pow(p, periode * laufzeit) - 1) / (p - 1)));
            }
        }
    }
	return 0.0;
}

static double
cfRatenlaufzeit(struct MinList *args)
{
    struct FuncArg *fa;
	double end, zins, rate, periode = 1.0, p;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        end = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            rate = TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 2)) != 0)
            {
                zins = TreeValue(fa->fa_Root);
	            if ((fa = (APTR)FindListNumber(args, 3)) != 0)
                	periode = TreeValue(fa->fa_Root);
				if(periode == 0.0)
				{
			    	calcerr = CTERR_NULLP;
			    	return 0.0;
			    }
			    p = (1.0 + zins * (1.0/periode));
				return (double)log(end/rate * (p - 1) + 1) / log(p) / periode;
            }
        }
    }
	return 0.0;
}
#endif


static STRPTR
cfGross(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t,s;

    fa = (APTR)args->mlh_Head;
    if ((t = s = TreeText(fa->fa_Root)) != 0)
    {
        for(;*s;s++)
            *s = ToUpper(*s);
    }
	return t;
}


static STRPTR
cfKlein(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t,s;

    fa = (APTR)args->mlh_Head;
    if ((t = s = TreeText(fa->fa_Root)) != 0)
    {
        for(;*s;s++)
            *s = ToLower(*s);
    }
	return t;
}


static STRPTR
cfLinks(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t,s = NULL;
    long   len;

    fa = (APTR)args->mlh_Head;
    if ((t = TreeText(fa->fa_Root)) != 0)
    {
        fa = (APTR)fa->fa_Node.mln_Succ;
        len = (long)TreeValue(fa->fa_Root);
        if (len > strlen(t))
            len = strlen(t);
        if (len < 0)
            len = 0;
        if ((s = AllocPooled(pool,len+1)) && len)
            strncpy(s,t,len);
        FreeString(t);
    }
	return s;
}


static STRPTR
cfRechts(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t,s = NULL;
    long   len;

    fa = (APTR)args->mlh_Head;
    if ((t = TreeText(fa->fa_Root)) != 0)
    {
        fa = (APTR)fa->fa_Node.mln_Succ;
        len = (long)TreeValue(fa->fa_Root);
        if (len > strlen(t))
            len = strlen(t);
        if (len < 0)
            len = 0;
        if ((s = AllocPooled(pool,len+1)) && len)
            strcpy(s,t+strlen(t)-len);
        FreeString(t);
    }
	return s;
}


static STRPTR
cfMitte(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t,s = NULL;
    long   len,pos;

    fa = (APTR)args->mlh_Head;
    if ((t = TreeText(fa->fa_Root)) != 0)
    {
        fa = (APTR)fa->fa_Node.mln_Succ;
        pos = (long)TreeValue(fa->fa_Root);
        if (pos > strlen(t))
            pos = strlen(t);
        if (pos < 1)
            pos = 1;
        fa = (APTR)fa->fa_Node.mln_Succ;
        len = (long)TreeValue(fa->fa_Root);
        if (len+pos > strlen(t))
            len = strlen(t)-pos;
        if (len < 0)
            len = 0;
        if ((s = AllocPooled(pool,len+1)) && len > 0)
            strncpy(s,t+pos-1,len);
        FreeString(t);
    }
	return s;
}


static double
cfPattern(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t,pattern;
    long   cmp = 0;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        pattern = TreeText(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            char dest[256];

            t = TreeText(fa->fa_Root);
            if (pattern && t && ParsePatternNoCase(pattern,dest,256))
            {
                if (!(cmp = MatchPatternNoCase(dest,t)) && IoErr() == ERROR_TOO_MANY_LEVELS)
                {
                    if (Fault(IoErr(),NULL,dest,256))
                        ErrorRequest(dest);
                }
            }
            else
                cmp = !stricmp(pattern ? pattern : (STRPTR)"",t ? t : (STRPTR)"");
            FreeString(t);
        }
        FreeString(pattern);
    }
	return (double)cmp;
}


static double
cfLength(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t;
    long   l = 0;

    fa = (APTR)args->mlh_Head;
    if ((t = TreeText(fa->fa_Root)) != 0)
    {
        l = strlen(t);
        FreeString(t);
    }

	return (double)l;
}


static double
cfWert(struct MinList *args)
{
    struct FuncArg *fa;
    struct Node *fv;
    STRPTR t = NULL,fvt = NULL;
    double val = 0.0;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
        t = TreeText(fa->fa_Root);
    if (t && (fa = (APTR)FindListNumber(args,1)))
        fvt = TreeText(fa->fa_Root);
    if (!(fvt && (fv = (APTR)FindLinkName(&calcpage->pg_Mappe->mp_Formats,fvt)) && CheckFormat(fv,t,&val) == 1L))
        GetFormatOfValue(t,&val);
#ifdef __amigaos4__
	if(calcerr != CT_OK) //no longer an error if argument is a number
	{
	    calcerr = CT_OK;
		val = TreeValue(fa->fa_Root);
	}
#endif
	FreeString(t);
    FreeString(fvt);

	return val;
}

#ifdef __amigaos4__
static STRPTR
cfTrim(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL,fvt = NULL;
    int8 i;
    char rstr[11] = "          ";

    fa = (APTR)args->mlh_Head;
    if ((fvt = TreeText(fa->fa_Root)))
    {
    	t = AllocString(fvt);
    	for(i = 10; i > 1; i--)
    	{
    	    rstr[i] = '\0';
    	    ReplaceTextString(t, rstr, " ", 0);
    	}
    	FreeString(fvt);
    }
	return t;
}

static STRPTR
cfGross2(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL, ti = NULL, fvt = NULL;
    int8 i;

    fa = (APTR)args->mlh_Head;
    if ((fvt = TreeText(fa->fa_Root)))
    {
    	t = ti = AllocString(fvt);
    	*ti = ToUpper(*ti);
    	for(ti++; *ti; ti++)
    		if(*(ti - 1) == ' ')
		    	*ti = ToUpper(*ti);
    	FreeString(fvt);
    }
	return t;
}

static STRPTR
cfKomprimieren(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL, t2 = NULL, fvt = NULL, fvt2 = NULL;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        fvt = TreeText(fa->fa_Root);
    	t = AllocString(fvt);
	    if (t && (fa = (APTR)FindListNumber(args,1)))
        {
        	fvt2 = t2 = TreeText(fa->fa_Root);
        	if(t2)
        	{
        	    char rstr[] = {" "};
        	    
        	    for(; *t2; t2++)
        	    {
        	        rstr[0] = *t2;
		    	    ReplaceTextString(t, rstr, "", 0);
		    	}
        		FreeString(fvt2);
        	}
        }
        else
			if(t)
	    	    ReplaceTextString(t, " ", "", 0);
    	FreeString(fvt);
    }
	return t;
}

static STRPTR
cfsaeubern(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL, t2 = NULL, ti = NULL, fvt = NULL;

    fa = (APTR)args->mlh_Head;
    if ((fvt = TreeText(fa->fa_Root)))
    {
    	t = t2 = AllocString(fvt);
   		for(ti = t; *ti; ti++)
   			if(isprint(*ti))
   			{
	    		*t2 = *ti;
		    	t2++;
		    }
   		*t2 = '\0';
   		FreeString(fvt);
    }
	return t;
}

static STRPTR
cfSchiebenL(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL, t2 = NULL, fvt = NULL;

    fa = (APTR)args->mlh_Head;
    if ((fvt = TreeText(fa->fa_Root)))
    {
        char c;
        
    	t = t2 = AllocString(fvt);
    	if(Strlen(t) > 1)
    	{
	    	c = t[0];
   			for(++t2; *t2; t2++)
   			{
   				*(t2 - 1) = *t2;
   			}
			*(t2 - 1) = c;
   			//*t2 = '\0';
   		}
   		FreeString(fvt);
    }
	return t;
}

static STRPTR
cfSchiebenR(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL, t2 = NULL, fvt = NULL;

    fa = (APTR)args->mlh_Head;
    if ((fvt = TreeText(fa->fa_Root)))
    {
        char c;
        
    	t = AllocString(fvt);
    	if(Strlen(t) > 1)
    	{
	    	c = t[Strlen(t) - 1];
   			for(t2 = t + Strlen(t) - 1 ; t2 != t; t2--)
   			{
   				*t2 = *(t2 - 1);
   			}
			*t = c;
   			//*t2 = '\0';
   		}
   		FreeString(fvt);
    }
	return t;
}

static STRPTR
cfSpiegeln(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL, t2 = NULL, ti = NULL, fvt = NULL;

    fa = (APTR)args->mlh_Head;
    if ((fvt = TreeText(fa->fa_Root)))
    {
        char c;
        
    	t = ti = AllocString(fvt);
    	if(Strlen(t) > 1)
    	{
   			for(t2 = fvt + Strlen(t) - 1 ; fvt != t2; t2--)
   			{
   				*(ti++) = *t2;
   			}
   			*ti = *t2; 
   		}
   		FreeString(fvt);
    }
	return t;
}

static STRPTR
cfWiederholen(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL, fvt = NULL;;
    uint16 c = 0, len = 0;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        fvt = TreeText(fa->fa_Root);
	    if (fvt && (fa = (APTR)FindListNumber(args,1)))
        {
        	if((c = TreeValue(fa->fa_Root)) > 0)
        	{
		    	t = AllocStringLength(fvt, len = Strlen(fvt) * c + 1);
        	    for(c--; c; c--)
        	    	Strlcat(t, fvt, len);
        	}	
	    	FreeString(fvt);
    	}
    }
	return t;
}
#endif

static STRPTR
cfText(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t = NULL,fvt = NULL;
    double val;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        val = TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args,1)))
            fvt = TreeText(fa->fa_Root);
        t = AllocString(FitValueInFormat(val,NULL,fvt,-1,ITA_NONE));
        FreeString(fvt);
    }
	return t;
}


static double
cfPosition(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t1,t2;
    long   l = 0,i,len1,len2;

    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        if ((t1 = TreeText(fa->fa_Root)) && (fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            if ((t2 = TreeText(fa->fa_Root)) != 0)
            {
                len1 = strlen(t1);  len2 = strlen(t2);
                for(i = 0;i < len2-len1+1;i++)
                {
                    if (!strncmp(t1,t2+i,len1))
                    {
                        l = i+1;
                        break;
                    }
                }
                FreeString(t2);
            }
        }
        FreeString(t1);
    }

	return (double)l;
}


#ifdef __amigaos4__
static double
cfASCII(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t,s;

    fa = (APTR)args->mlh_Head;
    if ((t = s = TreeText(fa->fa_Root)) != 0)
    	return (double)s[0];
	return 0.0;
}
#endif


static STRPTR
cfChar(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR t;

    if ((t = AllocPooled(pool, 2)) != 0)
    {
        fa = (APTR)args->mlh_Head;
        *t = (UBYTE)TreeValue(fa->fa_Root);
    }
	return t;
}


static STRPTR
cfColorName(struct MinList *args)
{
    struct colorPen *cp;
	char t[32];
	ULONG id;

    id = (ULONG)TreeValue(((struct FuncArg *)args->mlh_Head)->fa_Root);

	foreach (&colors, cp)
    {
        if (cp->cp_ID == id)
			return AllocString(cp->cp_Node.ln_Name);
    }

	GetColorName(t,(id >> 16) & 0xff,(id >> 8) & 0xff,id & 0xff);
	return AllocString(t);
}


static double
cfAPen(struct MinList *args)
{
    struct tableField *tf;
    ULONG  id;

    calcflags |= CF_NOLOOPS;
    if ((tf = TreeCell(((struct FuncArg *)args->mlh_Head)->fa_Root)) != 0)
        id = tf->tf_APen;
    else
        id = calcpage->pg_APen;
    calcflags &= ~CF_NOLOOPS;

	return (double)id;
}


static double
cfBPen(struct MinList *args)
{
    struct tableField *tf;
    ULONG  id;

    calcflags |= CF_NOLOOPS;
    if ((tf = TreeCell(((struct FuncArg *)args->mlh_Head)->fa_Root)) != 0)
        id = tf->tf_BPen;
    else
        id = calcpage->pg_BPen;
    calcflags &= ~CF_NOLOOPS;

	return (double)id;
}


static STRPTR
cfFont(struct MinList *args)
{
    struct tableField *tf;
    STRPTR t;

    calcflags |= CF_NOLOOPS;
    if ((tf = TreeCell(((struct FuncArg *)args->mlh_Head)->fa_Root)) != 0)
        t = tf->tf_FontInfo->fi_Family->ln_Name;
    else
        t = calcpage->pg_Family->ln_Name;
    calcflags &= ~CF_NOLOOPS;

	return AllocString(t);
}


static double
cfPointHeight(struct MinList *args)
{
    struct tableField *tf;
    long   points;

    calcflags |= CF_NOLOOPS;
    if ((tf = TreeCell(((struct FuncArg *)args->mlh_Head)->fa_Root)) != 0)
        points = tf->tf_FontInfo->fi_FontSize->fs_PointHeight;
    else
        points = calcpage->pg_PointHeight;
    calcflags &= ~CF_NOLOOPS;

	return points / 65536.0;
}


static double
cfWidth(struct MinList *args)
{
    struct Page *page = NULL;
    struct FuncArg *fa;
    ULONG  mm = 0,col;

    fa = (struct FuncArg *)args->mlh_Head;
    if (fa->fa_Root)
    {
        mp_flags |= MPF_CUSIZE;
        col = fa->fa_Root->t_Col;
        if (fa->fa_Root->t_Op == OP_CELL)
        {
            page = calcpage;
            col += fa->fa_Root->t_AbsCol ? 0 : tf_col;
        }
        if (fa->fa_Root->t_Op == OP_EXTCELL)
            page = GetExtCalcPage(fa->fa_Root);
        if (page)
            mm = (page->pg_tfWidth+col-1)->ts_mm;
    }

	return (double)mm / 1024.0;
}


static double
cfHeight(struct MinList *args)
{
    struct FuncArg *fa;
    struct Page *page = NULL;
    ULONG  mm = 0,row;

    fa = (struct FuncArg *)args->mlh_Head;
    if (fa->fa_Root)
    {
        mp_flags |= MPF_CUSIZE;
        row = fa->fa_Root->t_Row;
        if (fa->fa_Root->t_Op == OP_CELL)
        {
            page = calcpage;
            row += fa->fa_Root->t_AbsRow ? 0 : tf_row;
        }
        if (fa->fa_Root->t_Op == OP_EXTCELL)
            page = GetExtCalcPage(fa->fa_Root);
        if (page)
            mm = (page->pg_tfHeight+row-1)->ts_mm;
    }

	return (double)mm / 1024.0;
}


static double
cfMonat(struct MinList *args)
{
    struct FuncArg *fa;
    long   monat = 1;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
        tagedatum((long)TreeValue(fa->fa_Root),NULL,&monat,NULL);

	return (double)monat;
}


static double
cfTag(struct MinList *args)
{
    struct FuncArg *fa;
    long   tag = 1;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
        tagedatum((long)TreeValue(fa->fa_Root),&tag,NULL,NULL);

	return (double)tag;
}


static double
cfJahr(struct MinList *args)
{
    struct FuncArg *fa;
    long   jahr = 0;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
        tagedatum((long)TreeValue(fa->fa_Root),NULL,NULL,&jahr);

	return (double)jahr;
}


static double
cfSchaltjahr(struct MinList *args)
{
    struct FuncArg *fa;
    long   jahr = 0;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
        tagedatum((long)TreeValue(fa->fa_Root),NULL,NULL,&jahr);

	return (double)schaltjahr(jahr);
}


#ifdef __amigaos4__
static double
cfOstern(struct MinList *args)
{
    struct FuncArg *fa;
    long jahr;
    long k, m, s, a, d, r, og, sz, oe;
    
    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
    {
        jahr = (long)TreeValue(fa->fa_Root);
        k = jahr / 100;
        m = 15 + (3 * k + 3) / 4 - (8 * k + 13) / 25;
        s = 2 - (3 * k + 3) / 4;
        a = jahr % 19;
        d = (19 * a + m) % 30;
        r = (d + a / 11) / 29;
        og = 21 + d - r;
        sz = 7 - (jahr + jahr / 4 + s) % 7;
        oe = 7 - (og - sz) % 7;
        return (double)((!(jahr % 4) && (jahr % 100 || !(jahr % 400)) ? 1 : 0) + og + oe + 31 + 28 + --jahr*365 + jahr/4 - jahr/100 + jahr/400);
    }
	return (double)0;
}
static STRPTR
cfStringErsetzen(struct MinList *args)
{
    struct FuncArg *fa;
	STRPTR text = NULL, stext = NULL, rtext = NULL, buff = NULL;
	int32 pos = 0;
	
    if ((fa = (APTR)FindListNumber(args, 0)) != 0)
    {
        text = TreeText(fa->fa_Root);
		if(!(buff = AllocStringLength(text, 1024)))
			return text;
        if ((fa = (APTR)FindListNumber(args, 1)) != 0)
        {
            stext = TreeText(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 2)) != 0)
            {
                rtext = TreeText(fa->fa_Root);
	            if ((fa = (APTR)FindListNumber(args, 3)) != 0)
                	pos = TreeValue(fa->fa_Root);
				if(text == NULL || stext == NULL || rtext == NULL)
				{
			    	calcerr = CTERR_ARGS;
			    	return text;
			    }
			    ReplaceTextString(buff, stext, rtext, pos);
            }
        }
    }
	return buff;
}

STRPTR convertval(uint32 val, uint8 base, uint32 len)
{
    STRPTR s = NULL;
    char fs[8];

    if(len == 0)
       	for(len = 0; pow(base, len) < (double)val; len++);
    else if(len < 0 || len > (base == 8 ? 10 : 8) || (double)val > (pow(base, len) - 1)) 
  	{
  		calcerr = CTERR_ARGS;
        return s;
   	}
   	if(val < 0)
   		len = (base == 8 ? 10 : 8);
	if(!(s = AllocStringLength("H", len + 2)))
	{
	    calcerr = CTERR_NULLP;
	}
	else
	{
		sprintf(fs, "%%0%d%c", len, (base == 8 ? 'o' : 'X'));
		sprintf(s, fs, val);
		if(Strlen(s) > 10)
			Strlcpy(s, s + 1, 11);
	}
	return s;
}

static STRPTR
cfdez2hex(struct MinList *args)
{
    struct FuncArg *fa;
    long   val, len = 0;

    fa = (APTR)args->mlh_Head;
    val = TreeValue(fa->fa_Root);
    if(calcerr)
    	return NULL;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
    {
      	len = TreeValue(fa->fa_Root);
	    if(calcerr)
	    	return NULL;
    }
	return convertval(val, 16, len);
}

static STRPTR
cfdez2oct(struct MinList *args)
{
    struct FuncArg *fa;
    long   val, len = 0;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    val = TreeValue(fa->fa_Root);
    if(calcerr)
    	return NULL;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
    {
      	len = TreeValue(fa->fa_Root);
	    if(calcerr)
	    	return NULL;
    }
	return convertval(val, 8, len);
}

static STRPTR
cfdez2bin(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL;
    long   val, len = 0, i;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    val = TreeValue(fa->fa_Root);
    if(calcerr)
    	return s;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
    {
      	len = TreeValue(fa->fa_Root);
	    if(calcerr)
	    	return s;
    }
    if(len == 0 && val > 0)
       	for(len = 0; pow(2, len) <= (double)val; len++);
    else if(len < 0 || (double)val > (pow(2, len) - 1) || val < -512) 
  	{
  		calcerr = CTERR_ARGS;
        return s;
   	}
   	if(val < 0)
   		len = 10;
	if(!(s = AllocStringLength("H", len + 2)))
	    calcerr = CTERR_NULLP;
	else
	{
	    s[len] = '\0';
	    for(i = 0; i < len; i++)
	        if((0x1 << i) & val)
	        	s[len - i - 1] = '1';
	       	else
	        	s[len - i - 1] = '0';
	}
	return s;
}

int32 bin2value(char *bin, uint32 len)
{
    int32 result = 0, i;
    
    for(i = 0; i < (len == 10 ? 9 : len); i++)
    	if(bin[len - i - 1] == '1')
    		result |= 0x1 << i; 
    if(bin[0] == '1' && len == 10)
    	for(i = 9; i < 32; i++)
	    	result |= 1 << i;
  	return result;  
}

static double
cfbin2dez(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL;
    long   i, val = 0, len;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    s = TreeText(fa->fa_Root);
    if (calcerr != CTERR_TYPE)
    {
	    if((len = Strlen(s)) <= 10)
	    {
	        for(i = len - 1; i >= 0; i--)
	        	if(s[i] != '0' && s[i] != '1')
	        		calcerr = CTERR_ARGS;
	        if(calcerr != CTERR_ARGS)
	        {
	            val = bin2value(s, len);
	        }
	    }
	    else
	    	calcerr = CTERR_ARGS;   
    }
	return (double)val;
}

static STRPTR
cfbin2oct(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL;
    long   i, val = 0, len, dlen = 0;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    s = TreeText(fa->fa_Root);
    if (calcerr != CT_OK)
        return NULL;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
      	dlen = TreeValue(fa->fa_Root);
    if (calcerr != CTERR_TYPE)
    {
	    if((len = Strlen(s)) <= 10)
	    {
	        for(i = len - 1; i >= 0; i--)
	        	if(s[i] != '0' && s[i] != '1')
	        		calcerr = CTERR_ARGS;
	        if(calcerr != CTERR_ARGS)
	        {
	            val = bin2value(s, len);
				s = convertval(val, 8, (dlen > 10 ? 10 : dlen));
	        }
	    }
	    else
	    	calcerr = CTERR_ARGS;   
    }
	return s;
}

static STRPTR
cfbin2hex(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL;
    long   i, val = 0, len, dlen;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    s = TreeText(fa->fa_Root);
    if (calcerr != CT_OK)
        return NULL;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
      	dlen = TreeValue(fa->fa_Root);
    if (calcerr != CTERR_TYPE)
    {
	    if((len = Strlen(s)) <= 10)
	    {
	        for(i = len - 1; i >= 0; i--)
	        	if(s[i] != '0' && s[i] != '1')
	        		calcerr = CTERR_ARGS;
	        if(calcerr != CTERR_ARGS)
	        {
	            val = bin2value(s, len);
				s = convertval(val, 16, (dlen ? dlen : 8));
	        }
	    }
	    else
	    	calcerr = CTERR_ARGS;   
    }
	return s;
}

int32 oct2value(STRPTR oct, uint32 len)
{
    int32 result = 0, i;
    
    for(i = 0; i < (len == 10 ? 9 : len); i++)
    	result += (int32)(pow(8, i) * (double)(oct[len - i - 1] - 0x30));
    if(oct[0] == '7' && len == 10)
    	for(i = 9; i < 32; i++)
	    	result |= 1 << i;
  	return result;  
}

static STRPTR
cfoct2bin(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL, p = NULL;
    long   i, val = 0, slen, dlen = 10;

    fa = (APTR)args->mlh_Head;
    p = TreeText(fa->fa_Root);
    if (calcerr != CT_OK)
        return NULL;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
      	dlen = TreeValue(fa->fa_Root);
    if (calcerr != CTERR_TYPE)
    {
	    if((slen = Strlen(p)) <= 10)
	    {
	        for(i = slen - 1; i >= 0; i--)
	        	if((p[i] - 0x30) < 0 || (p[i] - 0x30) > 7)
	        		calcerr = CTERR_ARGS;
            val = oct2value(p, slen);
			if(pow(2.0, dlen) < val)
			    calcerr = CTERR_ARGS;
	        if(calcerr != CTERR_ARGS)
	        {
   				if(val < 0)
			   		dlen = 10;
				if(!(s = AllocStringLength("H", dlen + 2)))
	    			calcerr = CTERR_NULLP;
				else
				{
	    			s[dlen] = '\0';
	    			for(i = 0; i < dlen; i++)
	        			if((0x1 << i) & val)
	        				s[dlen - i - 1] = '1';
	       				else
	        				s[dlen - i - 1] = '0';
				}
	        }
	    }
	    else
	    	calcerr = CTERR_ARGS;   
    }
	return s;
}

static STRPTR
cfoct2hex(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL, p = NULL;
    long   i, val = 0, slen, dlen = 8;

    fa = (APTR)args->mlh_Head;
    p = TreeText(fa->fa_Root);
    if (calcerr != CT_OK)
        return NULL;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
      	dlen = TreeValue(fa->fa_Root);
    if (calcerr != CTERR_TYPE)
    {
	    if((slen = Strlen(p)) <= 10)
	    {
	        for(i = slen - 1; i >= 0; i--)
	        	if((p[i] - 0x30) < 0 || (p[i] - 0x30) > 7)
	        		calcerr = CTERR_ARGS;
            val = oct2value(p, slen);
	        if(calcerr != CTERR_ARGS)
				s = convertval(val, 16, dlen);
	    }
	    else
	    	calcerr = CTERR_ARGS;   
    }
	return s;
}

static double
cfoct2dez(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR p = NULL;
    long   i, val = 0, slen;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    p = TreeText(fa->fa_Root);
    if (calcerr != CT_OK)
        return 0.0;
    if((slen = Strlen(p)) <= 10)
    {
        for(i = slen - 1; i >= 0; i--)
        	if((p[i] - 0x30) < 0 || (p[i] - 0x30) > 7)
        	{
        		calcerr = CTERR_ARGS;
        		return 0.0;
        	}
        return (double)oct2value(p, slen);
    }
    else
    	calcerr = CTERR_ARGS;   
	return 0,0;
}

int32 hex2value(char *hex, uint32 len)
{
    int32 result = 0, i, v;
    
    for(i = 0; i < (len == 10 ? 9 : len); i++)
    {
        v = ((hex[len - i - 1] - 0x30) > 9 ? toupper((hex[len - i - 1])) - 'A' + 10 : (hex[len - i - 1] - 0x30)); 
    	result += (int32)(pow(16, i) * (double)v);
    }
    if(toupper(hex[0]) == 'F' && len == 8)
    {
    	for(i = 9; i < 32; i++)
	    	result |= 1 << i;
	    result += 1;
	}
  	return result;  
}

static double
cfhex2dez(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR p = NULL;
    long   i, val = 0, slen;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    p = TreeText(fa->fa_Root);
    if (calcerr != CT_OK)
        return 0.0;
    if((slen = Strlen(p)) <= 10)
    {
        for(i = slen - 1; i >= 0; i--)
        	if(!strchr("ABCDEF0123456789", toupper(p[i]))) 
        	{
        		calcerr = CTERR_ARGS;
        		return 0.0;
        	}
        return (double)hex2value(p, slen);
    }
    else
    	calcerr = CTERR_ARGS;   
	return 0,0;
}

static STRPTR
cfhex2bin(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL, p = NULL;
    long   val, len = 0, i;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    p = TreeText(fa->fa_Root);
    if(calcerr)
    	return s;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
    {
      	len = TreeValue(fa->fa_Root);
	    if(calcerr)
	    	return s;
    }

    for(i = Strlen(p) - 1; i >= 0; i--)
	   	if(!strchr("ABCDEF0123456789", toupper(p[i])) || i > 9) 
       	{
       		calcerr = CTERR_ARGS;
       		return NULL;
       	}
    val = hex2value(p, Strlen(p));
    if(len == 0 && val > 0)
       	for(len = 0; pow(2, len) <= (double)val; len++);
    if(len < 0 || (double)val > (pow(2, len) - 1) || val < -512 || len > 10) 
  	{
  		calcerr = CTERR_ARGS;
        return s;
   	}
   	if(val < 0)
   		len = 10;
	if(!(s = AllocStringLength("H", len + 2)))
	    calcerr = CTERR_NULLP;
	else
	{
	    s[len] = '\0';
	    for(i = 0; i < len; i++)
	        if((0x1 << i) & val)
	        	s[len - i - 1] = '1';
	       	else
	        	s[len - i - 1] = '0';
	}
	return s;
}

static STRPTR
cfhex2oct(struct MinList *args)
{
    struct FuncArg *fa;
    STRPTR s = NULL, p = NULL;
    long   val, len = 0, i;
    char fs[8];

    fa = (APTR)args->mlh_Head;
    p = TreeText(fa->fa_Root);
    if(calcerr)
    	return s;
    if ((fa = (APTR)FindListNumber(args, 1)) != 0)
    {
      	len = TreeValue(fa->fa_Root);
	    if(calcerr)
	    	return s;
    }

    for(i = Strlen(p) - 1; i >= 0; i--)
	   	if(!strchr("ABCDEF0123456789", toupper(p[i])) || i > 9) 
       	{
       		calcerr = CTERR_ARGS;
       		return NULL;
       	}
    val = hex2value(p, Strlen(p));
    if(len == 0 && val > 0)
       	for(len = 0; pow(8, len) <= (double)val; len++);
    if(len < 0 || (double)val > (pow(8, len) - 1) || val < -512 || len > 10) 
  	{
  		calcerr = CTERR_ARGS;
        return s;
   	}
   	if(val < 0)
   		len = 10;
	if(!(s = AllocStringLength("H", len + 2)))
	    calcerr = CTERR_NULLP;
	else
		s = convertval(val, 8, len);
	return s;
}
#endif


static double
cfTagImJahr(struct MinList *args)
{
    struct FuncArg *fa;
    long   tag = 1,monat,jahr;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
    {
        tagedatum((long)TreeValue(fa->fa_Root),&tag,&monat,&jahr);
        tag += mday[monat-1]+(monat > 2 && schaltjahr(jahr) ? 1 : 0);
    }
	return (double)tag;
}


static double
cfWoche(struct MinList *args)
{
    struct FuncArg *fa;
    long   datum,tag = 1,monat,jahr;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
    {
        tagedatum(datum = (long)TreeValue(fa->fa_Root),&tag,&monat,&jahr);
		tag += mday[monat - 1] + (monat > 2 && schaltjahr(jahr) ? 1 : 0);
		monat = weekday(datum - tag + 1);
		tag = ((tag + 4 - monat) / 7) + 1;
    }
	return (double)tag;
}


static double
cfWochentag(struct MinList *args)
{
    struct FuncArg *fa;
    long   tag = 1;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
        tag = weekday((long)TreeValue(fa->fa_Root));

	return (double)tag;
}


static double
cfHeute(struct MinList *args)
{
    struct ClockData cd;
    uint32 sec,day;

	CurrentTime(&sec, &day);
	Amiga2Date(sec, &cd);

	day = mday[cd.month-1] + cd.mday;
    if (cd.month > 2 && !(cd.year % 4) && (cd.year % 100 || !(cd.year % 400)))
        day++;
	day += --cd.year*365 + cd.year/4 - cd.year/100 + cd.year/400;
    return (double)day;
}


static double
cfJetzt(struct MinList *args)
{
    struct ClockData cd;
    uint32 sec,msec;

    CurrentTime(&sec,&msec);
    Amiga2Date(sec,&cd);

    return (double)(cd.hour*3600 + cd.min*60 + cd.sec);
}

	
static double
cfGMTOffset(struct MinList *args)
{
	return (double)(loc->loc_GMTOffset * 60);
}

 
static double
cfDatumRechnen(struct MinList *args)
{
    struct FuncArg *fa;
    long   datum = 0,month,jahr,i;

    if ((fa = (struct FuncArg *)args->mlh_Head) != 0)
    {
        datum = (long)TreeValue(fa->fa_Root);
        tagedatum(datum,NULL,&month,&jahr);
        if ((fa = (APTR)FindListNumber(args, 1)) != 0) // Tag
            datum += (long)TreeValue(fa->fa_Root);
        if ((fa = (APTR)FindListNumber(args, 2)) != 0) // Monat
        {
            i = (long)TreeValue(fa->fa_Root);
            if ((fa = (APTR)FindListNumber(args, 3)) != 0) // Jahr
                i += (long)TreeValue(fa->fa_Root)*12;
            if (i > 0)
            {
                for(;i;i--)
                {
                    datum += monthlength(month++,jahr);
                    if (month > 12)
                        month = 1, jahr++;
                }
            }
            else if (i < 0)
            {
                for(;i;i++)
                {
                    if (!--month)
                    {
                        month = 12;
                        jahr--;
                    }
                    datum -= monthlength(month,jahr);
                }
            }
        }
    }
	return (double)datum;
}


static double
cfMonthlength(struct MinList *args)
{
    struct FuncArg *fa;
    long   m = 1,j = 1;

    if ((fa = (struct FuncArg *)FindListNumber(args, 0)) != 0)
        m = (long)TreeValue(fa->fa_Root);
    if ((fa = (struct FuncArg *)FindListNumber(args, 1)) != 0)
        j = (long)TreeValue(fa->fa_Root);

	return (double)monthlength(m,j);
}


static double
cfHour(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

	return (double)((long)TreeValue(fa->fa_Root) / 3600);
}


static double
cfMinute(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

	return (double)(((long)(TreeValue(fa->fa_Root)+0.5) / 60) % 60);
}


static double
cfSecond(struct MinList *args)
{
    struct FuncArg *fa = (APTR)args->mlh_Head;

	return (double)((long)TreeValue(fa->fa_Root) % 60);
}


static double
cfDbPosition(struct MinList *args)
{
    struct FuncArg *fa;
    struct Database *db = NULL,*pdb = NULL;
    long   pos = 0,*refs;

    if ((fa = (struct FuncArg *)FindListNumber(args, 0)) != 0)
    {
        if (fa->fa_Root && (fa->fa_Root->t_Op == OP_TEXT || fa->fa_Root->t_Op == OP_NAME))
            db = (APTR)FindTag(&calcpage->pg_Mappe->mp_Databases,fa->fa_Root->t_Text);
    }
    if (db && (fa = (struct FuncArg *)FindListNumber(args,1)))
    {
        if (fa->fa_Root && (fa->fa_Root->t_Op == OP_TEXT || fa->fa_Root->t_Op == OP_NAME))
            pdb = (APTR)FindTag(&calcpage->pg_Mappe->mp_Databases,fa->fa_Root->t_Text);
    }
    if (db)
    {
        if (pdb)
        {
            if (!IsDBEmpty(pdb) && (refs = GetDBReferences(db,pdb)))
            {
                for(pos = 1;pos <= *refs;pos++)
                    if (*(refs+pos) == db->db_Current)
                        break;
                FreePooled(pool,refs,(*refs+1)*sizeof(long));
            }
            else
                pos = 0;
        }
        else
            pos = db->db_IndexPos+1;
    }

	return (double)pos;
}

static double
cfDbSize(struct MinList *args)
{
    struct FuncArg *fa;
    struct Database *db = NULL,*pdb = NULL;
    long   size = 0,*refs;

    if ((fa = (struct FuncArg *)FindListNumber(args, 0)) != 0)
    {
        if (fa->fa_Root && (fa->fa_Root->t_Op == OP_TEXT || fa->fa_Root->t_Op == OP_NAME))
            db = (APTR)FindTag(&calcpage->pg_Mappe->mp_Databases,fa->fa_Root->t_Text);
    }
    if (db && (fa = (struct FuncArg *)FindListNumber(args,1)))
    {
        if (fa->fa_Root && (fa->fa_Root->t_Op == OP_TEXT || fa->fa_Root->t_Op == OP_NAME))
            pdb = (APTR)FindTag(&calcpage->pg_Mappe->mp_Databases,fa->fa_Root->t_Text);
    }
    if (db)
    {
        struct Index *in;

        if (pdb)
        {
            if (!IsDBEmpty(pdb) && (refs = GetDBReferences(db,pdb)))
            {
                size = *refs;
                FreePooled(pool,refs,(*refs+1)*sizeof(long));
            }
            else
                size = 0;
        }
        else if ((in = (struct Index *)db->db_Filter) || (in = (struct Index *)db->db_Index))
            size = in->in_Size;
        else
            size = db->db_TablePos.tp_Height+1;
    }
	return (double)size;
}

static STRPTR
cfDbFilter(struct MinList *args)
{
    struct FuncArg *fa;
    struct Database *db;
    STRPTR t = NULL;

    if ((fa = (APTR)args->mlh_Head) && fa->fa_Node.mln_Succ && fa->fa_Root)
    {
        if (fa->fa_Root && (fa->fa_Root->t_Op == OP_TEXT || fa->fa_Root->t_Op == OP_NAME))
            db = (APTR)FindTag(&calcpage->pg_Mappe->mp_Databases,fa->fa_Root->t_Text);
        if (db)
        {
            if (db->db_Filter)
            {
                if (db->db_Filter->fi_Type == FIT_SEARCH)
                    t = "search_filter";
                else
                    t = db->db_Filter->fi_Node.ln_Name;
            }
        }
        else
            calcerr = CTERR_NAME;
    }
    else
        calcerr = CTERR_ARGS;

	return AllocString(t);
}


#ifdef __amigaos4__
static double
cfDbTableRow(struct MinList *args)
{
    struct FuncArg *fa;
    struct Database *db;

    if ((fa = (struct FuncArg *)FindListNumber(args, 0)) != 0)
    {
        if (fa->fa_Root && (fa->fa_Root->t_Op == OP_TEXT || fa->fa_Root->t_Op == OP_NAME))
            db = (APTR)FindTag(&calcpage->pg_Mappe->mp_Databases,fa->fa_Root->t_Text);
    }
    if (db)
	{
		return (double)db->db_Current + 1;
	}
	return 0.0;
}
#endif

static long
cfRealDBFiltered(struct Database *db, double *v, struct Term *term, struct Term *filter, void ASM (*func)(REG(a0, double *), REG(a1, struct tableField *tf)))
{
    long   size,i,current,c,r,count = 0;
    struct Filter *fil;
    struct tableField *tf;
    struct Field *fi;
    struct Page *page;
    STRPTR t;

    if ((filter->t_Op == OP_TEXT || filter->t_Op == OP_NAME) && (fil = (APTR)FindTag(&db->db_Filters,filter->t_Text)))
    {
        if (!(filter = fil->fi_Root))
			return 0;
    }
    if (!(t = strchr(term->t_Text,'.')))
    {
        calcerr = CTERR_ARGS;
		return 0;
    }
    c = db->db_TablePos.tp_Col;
    r = db->db_TablePos.tp_Row;
    t++;
    foreach(&db->db_Fields,fi)
    {
        if (!stricmp(fi->fi_Node.ln_Name,t))
            break;
        c++;
    }
    if (!fi || !fi->fi_Node.ln_Succ)
    {
        calcerr = CTERR_NAME;
		return 0;
    }
    page = calcpage;
    calcpage = db->db_Page;

    size = db->db_TablePos.tp_Height+1;
    current = db->db_Current;

    for(i = 0;i < size;i++,r++)
    {
        SetDBCurrent(db,DBC_ABS,i);
        if (TreeValue(filter))        /* Kriterium trifft zu */
        {
            count++;
            if ((tf = GetTableField(calcpage, c, r)) != 0)
                func(v,tf);
        }
    }
    UpdateDBCurrent(db,current);

    calcpage = page;
    return count;
}


static long
cfDbFiltered(struct MinList *args, double *v, void ASM (*func)(REG(a0, double *), REG(a1, struct tableField *tf)))
{
    struct FuncArg *fa,*tail = (APTR)args->mlh_TailPred;
    struct Term *t,*filter;
    struct tableField *tf = NULL;
    long   col,row,count;

    if ((count = CountNodes(args)) > 1 && (filter = tail->fa_Root))
    {
        if (count == 2 && (t = ((struct FuncArg *)args->mlh_Head)->fa_Root))
        {
            struct Database *db;

            if ((t->t_Op == OP_TEXT || t->t_Op == OP_NAME) && (db = (APTR)FindCommand(&calcpage->pg_Mappe->mp_Databases,t->t_Text)))
                return cfRealDBFiltered(db,v,t,filter,func);
        }
        MakeTermAbsolute(filter);  count = 0;
        col = tf_col;  row = tf_row;

        for (fa = (APTR)args->mlh_Head;fa != tail;fa = (APTR)fa->fa_Node.mln_Succ)
        {
            tf_col = col;  tf_row = row;
            while ((tf = GetRangeCells(fa->fa_Root, tf)) != 0)
            {
                tf_col = tf->tf_Col;  tf_row = tf->tf_Row;

                if (TreeValue(filter))
                {
                    count++;
                    func(v,tf);
                }
            }
        }
        tf_col = col;  tf_row = row;
        return count;
    }
    calcerr = CTERR_ARGS;
    return 0;
}


static void ASM
cfdbsumme(REG(a0, double *v), REG(a1, struct tableField *tf))
{
    *v += tf->tf_Value;
}


static double
cfDbSumme(struct MinList *args)
{
    double v = 0.0;

	cfDbFiltered(args, &v, cfdbsumme);
    return(v);
}


static void ASM
cfdbprodukt(REG(a0, double *v), REG(a1, struct tableField *tf))
{
    *v *= tf->tf_Value;
}


static double
cfDbProdukt(struct MinList *args)
{
    double v = 1.0;

	cfDbFiltered(args, &v, cfdbprodukt);
    return v;
}


static void ASM
cfdbmin(REG(a0, double *v), REG(a1, struct tableField *tf))
{
    if (*v > tf->tf_Value)
        *v = tf->tf_Value;
}


static double
cfDbMin(struct MinList *args)
{
    double v = DBL_MAX;

    cfDbFiltered(args, &v, cfdbmin);
    return v;
}


static void ASM
cfdbmax(REG(a0, double *v), REG(a1, struct tableField *tf))
{
    if (*v < tf->tf_Value)
        *v = tf->tf_Value;
}


static double
cfDbMax(struct MinList *args)
{
    double v = DBL_MIN;

    cfDbFiltered(args,&v,cfdbmax);
    return v;
}


static void ASM
cfdbanzahl(REG(a0, double *v), REG(a1, struct tableField *tf))
{
    if (tf->tf_Text)
        *v += 1.0;
}


static double
cfDbAnzahl(struct MinList *args)
{
    double v = 0.0;

    cfDbFiltered(args, &v, cfdbanzahl);
    return v;
}


static double
cfDbMittelwert(struct MinList *args)
{
    double v = 0.0;
    long   num;

    num = cfDbFiltered(args, &v, cfdbsumme);
    if (!num)
        return 0.0;

    return v / num;
}

struct oan {ULONG tag;STRPTR name;} objattrnames[] =
{
    {GOA_Text,             "text"},
    {GOA_Help,             "help"},
    {GOA_Pen,              "pen"},
    {GOA_FillPen,          "fillpen"},
    {GOA_OutlinePen,       "open"},
    {GOA_Command,          "cmd"},
    {GOA_ContinualCommand, "contcmd"},
    {GOA_FontInfo,         "font"},
    {GOA_FontInfo,         "fontsize"},
    {GOA_HasOutline,       "outlined"},
    {GOA_Weight,           "weight"},
    {0,0}
};


static long
cfObjectAttr(struct Result *r,struct MinList *args)
{
    struct gInterface *gi,*sgi = NULL;
    struct gObject *go;
    struct FuncArg *fa;
    STRPTR obj,t;

    fa = (struct FuncArg *)args->mlh_Head;
    if (!(obj = TreeText(fa->fa_Root)))
		return CTERR_ARGS;

    r->r_Type = RT_VALUE;
    r->r_Value = 0.0;

    /*** Object heraussuchen ***/

    if (!(go = MyFindName(&calcpage->pg_gObjects, obj)))
        go = MyFindName(&calcpage->pg_gDiagrams, obj);

    FreeString(obj);  obj = NULL;

    if (!go)
		return CT_OK;

    /*** Attribut suchen ***/

    fa = (struct FuncArg *)fa->fa_Node.mln_Succ;
    if (!(t = TreeText(fa->fa_Root)))
        return(CTERR_ARGS);

    for(gi = go->go_Class->gc_Interface;gi->gi_Tag;gi++) // bei gInterface suchen
    {
        if (!zstricmp(gi->gi_Name,t))
        {
            sgi = gi;
            break;
        }
    }
    if (!sgi)  // bei den vorgegebenen Tags suchen
    {
        int i;

        for(i = 0;objattrnames[i].tag;i++)
        {
            if (!zstricmp(objattrnames[i].name,t))
            {
                sgi = GetGInterfaceTag(go->go_Class,objattrnames[i].tag);
                break;
            }
        }
    }

    if (!sgi)
    {
        if (!zstricmp("name",t))
        {
            r->r_Text = AllocString(go->go_Node.ln_Name);
            r->r_Type = RT_TEXT;
        }
        else if (!zstricmp("left",t))
            r->r_Value = go->go_mmLeft / 10240.0;
        else if (!zstricmp("top",t))
            r->r_Value = go->go_mmTop / 10240.0;
        else if (!zstricmp("width",t))
            r->r_Value = (go->go_mmRight - go->go_mmLeft) / 10240.0;
        else if (!zstricmp("height",t))
            r->r_Value = (go->go_mmBottom - go->go_mmTop) / 10240.0;
        else
            gDoMethod(go,GCM_GETNAMEDATTR,t,r);

        FreeString(t);

        return(CT_OK);
    }

    /*** Wert ermitteln ***/

    GetGObjectAttr(go,sgi->gi_Tag,(ULONG *)&obj);

	switch (sgi->gi_Type)
    {
        case GIT_WEIGHT:
            r->r_Value = (ULONG)obj / 65536.0;
            break;
        case GIT_FONT:
        {
            struct FontInfo *fi = (APTR)obj;

            if (!stricmp("fontsize",t))
                r->r_Value = fi->fi_FontSize->fs_PointHeight / 65536.0;
            else
            {
                r->r_Text = AllocString(fi->fi_Family->ln_Name);
                r->r_Type = RT_TEXT;
            }
            break;
        }
        case GIT_TEXT:
        case GIT_FORMULA:
        case GIT_FILENAME:
            r->r_Text = AllocString(obj);
            r->r_Type = RT_TEXT;
            break;
        case GIT_PEN:
        case GIT_CYCLE:
        case GIT_CHECKBOX:
            r->r_Value = (ULONG)obj;
            break;
    }
    FreeString(t);

    return CT_OK;
}


static double
cfNop(struct MinList *args)
{
    return 0.0;
}

static double
cfPi(struct MinList *args)
{
    return (double)3.141592654;
}

#ifdef __amigaos4__ 
static STRPTR
cfTabellenname(struct MinList *args)
{
    STRPTR t = NULL;
    
   	t = AllocString(calcpage->pg_Node.ln_Name);
    return t;
}
#endif 

/******************************** Verwaltung für die Funktionen ********************************/


void
AddFunction(STRPTR abb,STRPTR args,UBYTE ftype,APTR code,BYTE type)
{
    struct Function *f;

    if ((f = AllocPooled(pool, sizeof(struct Function))) != 0)
    {
        long min,max,mode;

        strcpy((char *)&f->f_ID,abb);
        f->f_Args = args;
        f->f_Node.ln_Type = type;
        f->f_Node.ln_Pri = ftype & FT_IGNOREFORMAT;
        f->f_Code = code;
        f->f_Type = ftype & ~FT_IGNOREFORMAT;

        /* Anzahl der Funktionsargumente festlegen */

        min = max = 0;  mode = 1;

        while(*args)
        {
            if (*args == '[')      /* optionale Argumente */
                mode = 2;
            else if (*args == ']')
                mode = 1;
            else if (*args == ';')
                args++;

            if (isalpha(*args))
            {
                if (mode && max != -1)
                    max++;
                if (mode == 1)
                    min++;
            }
            else if (*args == '.') /* beliebige Anzahl */
                max = -1;
            args++;
        }
        f->f_MinArgs = min;  f->f_MaxArgs = max;

        MyAddTail(&funcs, f);
    }
}


int
CompareFunctionNames(struct FunctionName *fna,struct FunctionName *fnb)
{
    /*if (!fna->fn_Name)
    {
        if (!fnb->fn_Name)
            return(0);
        else
            return(-1);
    }
    else if (!fnb->fn_Name)
        return(1);*/

    return cmdcmp((STRPTR *)fna,(STRPTR *)fnb);
}


struct Function *
FindFunctionWithLanguage(struct FunctionLanguage *fl,STRPTR name)
{
    struct FunctionName *fn;

    if ((fn = bsearch(&name, fl->fl_Array, fl->fl_Length, sizeof(struct FunctionName), (APTR)cmdcmp)) != 0)
        return fn->fn_Function;

    return NULL;
}


struct Function *
FindFunction(STRPTR name, struct Term *t)
{
    struct FunctionLanguage *fl;
    struct Function *f;

    if (!name)
        return NULL;

    if (calcflags & CF_SHORTFUNCS)
    {
        if ((f = FindFunctionWithLanguage((APTR)flangs.mlh_TailPred, name)) && t)
            t->t_Function = f;
        return f;
    }

    foreach (&flangs, fl)
    {
        if ((f = FindFunctionWithLanguage(fl, name)) != 0)
        {
            if (t)
                t->t_Function = f;

            return f;
        }
    }
    return NULL;
}


void
FreeFunctionLanguage(struct FunctionLanguage *fl)
{
    FreeString(fl->fl_Node.ln_Name);
    FreePooled(pool, fl->fl_Array, fl->fl_Bytes);
    FreePooled(pool, fl, sizeof(struct FunctionLanguage));
}


struct FunctionLanguage *
AddFunctionLanguage(STRPTR name, APTR array, ULONG length, ULONG bytes)
{
    struct FunctionLanguage *fl;

    if (!name || !array || !length)
        return NULL;

    if ((fl = AllocPooled(pool, sizeof(struct FunctionLanguage))) != 0)
    {
        fl->fl_Node.ln_Name = AllocString(name);
        fl->fl_Array = array;
        fl->fl_Length = length;
        fl->fl_Bytes = bytes;
        qsort(array, length, sizeof(struct FunctionName), (APTR)CompareFunctionNames);

        MyAddTail(&flangs, fl);
    }
    return fl;
}


void
LoadFunctionLanguage(STRPTR name)
{
    struct FunctionLanguage *fl = (APTR)flangs.mlh_Head;
    struct FunctionName *fn;
    struct Function *f = NULL;
    char   line[256],mode = 0,warnings = 0;
    BPTR   file;
    int    i = 0,length;

	if (!strcmp(name, BUILTIN_FUNCTIONLANGUAGE))
        return;

	if (!(fn = AllocPooled(pool, sizeof(struct FunctionName)*(length = fl->fl_Length))))
        return;

	strcpy(line, CONFIG_PATH);
	AddPart(line, name, 230);
	strcat(line, ".functions");

	if ((file = Open(line, MODE_OLDFILE)) != 0)
    {
		while (FGets(file, line, 256))
        {
            if (line[0] == '#')
            {
                if (!strncmp("# warnings",line+1,10))
                    warnings = TRUE;
                continue;
            }
            if (line[0] == ';' || line[0] == '\n')
                continue;

            line[strlen(line)-1] = 0;

            if (mode == 0)
            {
                if (!(f = FindFunctionWithLanguage(fl,line)) && warnings)
					ErrorRequest(GetString(&gLocaleInfo, MSG_FUNCTABLE_FUNCTION_NOT_FOUND_ERR), name, line);
            }
            else if (mode == 1 && f)
            {
                fn[i].fn_Name = AllocString(line);
                fn[i].fn_Function = f;

                if (!f->f_Node.ln_Name)
                    f->f_Node.ln_Name = fn[i].fn_Name;
                if (++i > length-1)
                    break;
            }
            else if (mode == 2 && f && !f->f_Help)
                f->f_Help = AllocString(line);

            mode = (mode+1) % 3;
        }
        if (i)
            AddFunctionLanguage(name,fn,i,fl->fl_Bytes);

        if (warnings && i < fl->fl_Length)
			ErrorRequest(GetString(&gLocaleInfo, MSG_FUNCTABLE_FUNCTIONS_MISSING_ERR), name);

        Close(file);
    }
}


void
MakeFunctionArray(void)
{
    struct FunctionLanguage *fl;
    struct FunctionName *array;
    struct Function *f;
    ULONG  num,i = 0;

    MyNewList(&flangs);
    num = CountNodes(&funcs);

	if ((array = AllocPooled(pool, num * sizeof(struct FunctionName))) == NULL)
	{
        ErrorRequest(GetString(&gLocaleInfo, MSG_FUNCTABLE_ERR));
		return;
	}
     

	foreach(&funcs,f)
	{
		array[i].fn_Name = (char *)&f->f_ID;
		array[i++].fn_Function = f;
	}

	fl = AddFunctionLanguage(BUILTIN_FUNCTIONLANGUAGE, array, num, num * sizeof(struct FunctionName));
	if (fl && loc)
	{
		for (i = 0; loc->loc_PrefLanguages[i]; i++)
			LoadFunctionLanguage(loc->loc_PrefLanguages[i]);

		MyRemove(fl);          // to have the right order

		if (IsListEmpty((struct List *)&flangs))
			ErrorRequest(GetString(&gLocaleInfo, MSG_NO_FUNCTABLE_ERR));

		MyAddTail(&flangs,fl);

		foreach (&funcs, f)
		{
			if (!f->f_Node.ln_Name)
				f->f_Node.ln_Name = (char *)&f->f_ID;
		}
	}
}


void
initFuncs(void)
{
    MyNewList(&funcs);  MyNewList(&fewfuncs);  MyNewList(&refs);

    itaPoint = loc->loc_DecimalPoint;
    if (!itaPoint || (*itaPoint != '.' && *itaPoint != ','))
        itaPoint = ".";

    /** b=bereich, z=zahl, t=text, w=wert, a=ausdruck, r=bezug, c=condition **/

    AddFunction("abs","z",RT_VALUE,cfAbs,FTM_ALGEBRA);
    AddFunction("num","b;.",RT_VALUE | FT_IGNOREFORMAT,cfAnzahl,FT_TABLE);
    AddFunction("sum","w;.",RT_VALUE,cfSumme,FT_TABLE);
    AddFunction("smt","b;a",RT_VALUE,cfSummeAus,FT_TABLE);
    AddFunction("prt","w;.",RT_VALUE,cfProdukt,FT_TABLE);
    AddFunction("fak","z",RT_VALUE,cfFak,FTM_ALGEBRA);
#ifdef __amigaos4__
    AddFunction("int", "z",RT_VALUE,cfInt,FTM_ALGEBRA);
    AddFunction("qrt", "z",RT_VALUE,cfQrt,FTM_ALGEBRA);
    AddFunction("xor", "w;.",RT_VALUE,cfXOder,FTM_LOGIK);
    AddFunction("rou", "z[;z]",RT_VALUE,cfRunden,FT_MISC);
    AddFunction("sgn", "z",RT_VALUE,cfVorzeichen,FTM_ALGEBRA);
    AddFunction("iec", "r",RT_VALUE | FT_IGNOREFORMAT,cfIstLeer,FTM_LOGIK);
    AddFunction("idt", "z",RT_VALUE | FT_IGNOREFORMAT,cfIstDatum,FTM_LOGIK);
    AddFunction("iti", "z",RT_VALUE | FT_IGNOREFORMAT,cfIstZeit,FTM_LOGIK);
    AddFunction("inm", "z",RT_VALUE | FT_IGNOREFORMAT,cfIstZahl,FTM_LOGIK);
    AddFunction("itx", "z",RT_VALUE | FT_IGNOREFORMAT,cfIstText,FTM_LOGIK);
#endif
    AddFunction("exp","z",RT_VALUE,cfExp,FTM_ALGEBRA);
    AddFunction("ln", "z",RT_VALUE,cfLn,FTM_ALGEBRA);
#ifdef __amigaos4__
    AddFunction("sqr", "z",RT_VALUE,cfSqr,FTM_ALGEBRA);
#endif
    AddFunction("l10","z",RT_VALUE,cfLog10,FTM_ALGEBRA);
    AddFunction("not","z",RT_VALUE,cfNicht,FTM_LOGIK);
    AddFunction("and","w;.",RT_VALUE,cfUnd,FTM_LOGIK);
    AddFunction("or", "w;.",RT_VALUE,cfOder,FTM_LOGIK);
    AddFunction("if", "c;w;w",RT_RESULT,cfWenn,FTM_LOGIK);
    AddFunction("nop",".",RT_VALUE,cfNop,FTM_LOGIK);
    AddFunction("sel","z;w;.",RT_RESULT,cfAuswahl,FTM_LOGIK);
#ifdef __amigaos4__
    AddFunction("csc","t;t;.",RT_RESULT,cfErstelleString,FT_TEXT);
    AddFunction("pi","",RT_VALUE,cfPi,FTM_ALGEBRA);  
    AddFunction("tnm","",RT_TEXT,cfTabellenname,FT_TABLE);
    AddFunction("sst","t;t;t[;z]",RT_TEXT,cfStringErsetzen,FT_TEXT);
    AddFunction("d2h","z[;z]",RT_TEXT,cfdez2hex,FT_MISC);
    AddFunction("d2o","z[;z]",RT_TEXT,cfdez2oct,FT_MISC);
    AddFunction("d2b","z[;z]",RT_TEXT,cfdez2bin,FT_MISC);
    AddFunction("b2d","t",RT_VALUE,cfbin2dez,FT_MISC);
    AddFunction("b2o","t[;z]",RT_TEXT,cfbin2oct,FT_MISC);
    AddFunction("b2h","t[;z]",RT_TEXT,cfbin2hex,FT_MISC);
    AddFunction("o2d","t",RT_VALUE,cfoct2dez,FT_MISC);
    AddFunction("o2b","t[;z]",RT_TEXT,cfoct2bin,FT_MISC);
    AddFunction("o2h","t[;z]",RT_TEXT,cfoct2hex,FT_MISC);
    AddFunction("h2d","t",RT_VALUE,cfhex2dez,FT_MISC);
    AddFunction("h2b","t[;z]",RT_TEXT,cfhex2bin,FT_MISC);
    AddFunction("h2o","t[;z]",RT_TEXT,cfhex2oct,FT_MISC);
#endif
    AddFunction("rad","z",RT_VALUE,cfBogen,FTM_TRIGO);
    AddFunction("deg","z",RT_VALUE,cfGrad,FTM_TRIGO);
    AddFunction("sin","z",RT_VALUE,cfSin,FTM_TRIGO);
    AddFunction("cos","z",RT_VALUE,cfCos,FTM_TRIGO);
    AddFunction("tan","z",RT_VALUE,cfTan,FTM_TRIGO);
    AddFunction("snh","z",RT_VALUE,cfSinHyp,FTM_TRIGO);
    AddFunction("csh","z",RT_VALUE,cfCosHyp,FTM_TRIGO);
    AddFunction("tnh","z",RT_VALUE,cfTanHyp,FTM_TRIGO);
    AddFunction("asn","z",RT_VALUE,cfArcSin,FTM_TRIGO);
    AddFunction("acs","z",RT_VALUE,cfArcCos,FTM_TRIGO);
    AddFunction("atn","z",RT_VALUE,cfArcTan,FTM_TRIGO);
    AddFunction("ran","",RT_VALUE,cfRandom,FT_MISC);
    AddFunction("upc","t",RT_TEXT,cfGross,FT_TEXT);
    AddFunction("lwc","t",RT_TEXT,cfKlein,FT_TEXT);
    AddFunction("lef","t;z",RT_TEXT,cfLinks,FT_TEXT);
    AddFunction("rig","t;z",RT_TEXT,cfRechts,FT_TEXT);
    AddFunction("mid","t;z;z",RT_TEXT,cfMitte,FT_TEXT);
	AddFunction("pat", "t;t", RT_VALUE, cfPattern, FT_TEXT);
    AddFunction("len","t",RT_VALUE,cfLength,FT_TEXT);
    AddFunction("pos","t;t",RT_VALUE,cfPosition,FT_TEXT);
    AddFunction("val","t[;t]",RT_VALUE,cfWert,FT_TEXT);
    AddFunction("txt","z[;t]",RT_TEXT,cfText,FT_TEXT);
    AddFunction("chr","z",RT_TEXT,cfChar,FT_TEXT);
#ifdef __amigaos4__
    AddFunction("asc","t",RT_VALUE,cfASCII,FT_TEXT);
    AddFunction("tri","t",RT_TEXT,cfTrim,FT_TEXT);
    AddFunction("up2","t",RT_TEXT,cfGross2,FT_TEXT);
    AddFunction("cpr","t[;t]",RT_TEXT,cfKomprimieren,FT_TEXT);
    AddFunction("cln","t",RT_TEXT,cfsaeubern,FT_TEXT);
    AddFunction("shl","t",RT_TEXT,cfSchiebenL,FT_TEXT);
    AddFunction("shr","t",RT_TEXT,cfSchiebenR,FT_TEXT);
    AddFunction("mir","t",RT_TEXT,cfSpiegeln,FT_TEXT);
    AddFunction("rep","t;z]",RT_TEXT,cfWiederholen,FT_TEXT);
    AddFunction("czv","z;z;z[;z]",RT_VALUE,cfZinssatz,FTM_FINANZ);
    AddFunction("cci","z;z;z[;z]",RT_VALUE,cfEndKapitalZZ,FTM_FINANZ);
    AddFunction("trm","z;z;z[;z]",RT_VALUE,cfLaufzeitZZ,FTM_FINANZ);
    AddFunction("cre","z;z;z[;z]",RT_VALUE,cfRatenendkapital,FTM_FINANZ);
    AddFunction("cri","z;z;z[;z]",RT_VALUE,cfRatenhoehe,FTM_FINANZ);
    AddFunction("crr","z;z;z[;z]",RT_VALUE,cfRatenlaufzeit,FTM_FINANZ);
    AddFunction("cap","z;z;z[;z]",RT_VALUE,cfStartKapitalZZ,FTM_FINANZ);
#else
    AddFunction("cci","z;z;z",RT_VALUE,cfEndKapitalZZ,FTM_FINANZ);
    AddFunction("cap","z;z;z",RT_VALUE,cfStartKapitalZZ,FTM_FINANZ);
#endif
    AddFunction("cai","z;z;z",RT_VALUE,cfEndKapitalEinfach,FTM_FINANZ);
    AddFunction("bpn","r",RT_VALUE | FT_IGNOREFORMAT,cfBPen,FT_LOOK);
    AddFunction("apn","r",RT_VALUE | FT_IGNOREFORMAT,cfAPen,FT_LOOK);
    AddFunction("pnm","z",RT_TEXT | FT_IGNOREFORMAT,cfColorName,FT_LOOK);
    AddFunction("fnt","r",RT_TEXT | FT_IGNOREFORMAT,cfFont,FT_LOOK);
    AddFunction("fpt","r",RT_VALUE | FT_IGNOREFORMAT,cfPointHeight,FT_LOOK);
    AddFunction("wid","r",RT_VALUE | FT_IGNOREFORMAT,cfWidth,FT_LOOK);
    AddFunction("hei","r",RT_VALUE | FT_IGNOREFORMAT,cfHeight,FT_LOOK);
    AddFunction("mon","z",RT_VALUE | FT_IGNOREFORMAT,cfMonat,FT_DATE);
    AddFunction("day","z",RT_VALUE | FT_IGNOREFORMAT,cfTag,FT_DATE);
    AddFunction("yer","z",RT_VALUE | FT_IGNOREFORMAT,cfJahr,FT_DATE);
    AddFunction("lep","z",RT_VALUE | FT_IGNOREFORMAT,cfSchaltjahr,FT_DATE);
    AddFunction("doy","z",RT_VALUE | FT_IGNOREFORMAT,cfTagImJahr,FT_DATE);
    AddFunction("wek","z",RT_VALUE | FT_IGNOREFORMAT,cfWoche,FT_DATE);
    AddFunction("wkd","z",RT_VALUE | FT_IGNOREFORMAT,cfWochentag,FT_DATE);
    AddFunction("tdy","",RT_VALUE,cfHeute,FT_DATE);
    AddFunction("dc", "z;z[;z;z]",RT_VALUE,cfDatumRechnen,FT_DATE);
    AddFunction("tsc","z",RT_VALUE | FT_IGNOREFORMAT,cfSecond,FT_DATE);
    AddFunction("tmn","z",RT_VALUE | FT_IGNOREFORMAT,cfMinute,FT_DATE);
    AddFunction("thr","z",RT_VALUE | FT_IGNOREFORMAT,cfHour,FT_DATE);
	AddFunction("now", "", RT_VALUE, cfJetzt, FT_DATE);
	AddFunction("gmt", "", RT_VALUE, cfGMTOffset, FT_DATE);
#ifdef __amigaos4__
    AddFunction("esd","z",RT_VALUE,cfOstern,FT_DATE);
#endif
    AddFunction("mln","z[;z]",RT_VALUE | FT_IGNOREFORMAT,cfMonthlength,FT_DATE);
    AddFunction("dps","w[;w]",RT_VALUE,cfDbPosition,FT_DATABASE);
    AddFunction("dsz","w[;w]",RT_VALUE,cfDbSize,FT_DATABASE);
    AddFunction("dfi","w",RT_TEXT,cfDbFilter,FT_DATABASE);
#ifdef __amigaos4__
    AddFunction("dtr","w",RT_VALUE,cfDbTableRow,FT_DATABASE);
#endif
    AddFunction("dnm","b;.;c",RT_VALUE,cfDbAnzahl,FT_DATABASE);
    AddFunction("dpt","b;.;c",RT_VALUE,cfDbProdukt,FT_DATABASE);
    AddFunction("dsm","b;.;c",RT_VALUE,cfDbSumme,FT_DATABASE);
    AddFunction("dmi","b;.;c",RT_VALUE,cfDbMin,FT_DATABASE);
    AddFunction("dma","b;.;c",RT_VALUE,cfDbMax,FT_DATABASE);
    AddFunction("dav","b;.;c",RT_VALUE,cfDbMittelwert,FT_DATABASE);
    AddFunction("ext","t;w;a",RT_RESULT,cfExtern,FT_TABLE);
    AddFunction("pag","w;a",RT_RESULT,cfSeite,FT_TABLE);
    AddFunction("ref","a;a[;z]",RT_RESULT,cfBezug,FT_TABLE);
    AddFunction("cur","",RT_RESULT,cfCurrent,FT_TABLE);
    AddFunction("row","a[;z]",RT_RESULT,cfReihe,FT_TABLE);
    AddFunction("col","a[;z]",RT_RESULT,cfSpalte,FT_TABLE);
    AddFunction("avg","w;.",RT_VALUE,cfMittelwert,FTM_STATISTIK);
    AddFunction("med","b;.",RT_VALUE,cfMedian,FTM_STATISTIK);
    AddFunction("per","z;b;.",RT_VALUE,cfPerzentil,FTM_STATISTIK);
    AddFunction("var","b;.",RT_VALUE,cfVarianz,FTM_STATISTIK);
    AddFunction("min","w;.",RT_VALUE,cfMin,FTM_STATISTIK);
    AddFunction("max","w;.",RT_VALUE,cfMax,FTM_STATISTIK);
#ifdef __amigaos4__
    AddFunction("mod","w;.",RT_VALUE,cfMode,FTM_STATISTIK);
#endif
    AddFunction("gcd","z;z", RT_VALUE, cfGreatestCommonDivisor, FTM_ALGEBRA);
    AddFunction("obj","t;t",RT_RESULT,cfObjectAttr,FT_LOOK);

    MakeFunctionArray();
}
