/* ARexx interface and implementation of exported functions
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include <dos/dostags.h>
#ifdef __amigaos4__
	#include <stdarg.h>

	#define SetRexxVar(a, b, c, d)  SetRexxVarFromMsg(b, c, a)
	#define GetRexxVar(a, b, c)		GetRexxVarFromMsg(b, c, a)

	extern char IgnTitle[100];		//ScreenTitle for ignition, must be unique for PubScreen handling
#endif


#define RXPOS_CELL 1
#define RXPOS_BLOCK 2

extern struct Node **intCmdArray;
extern uint32 intCmdArraySize;

struct MinList rexxports,errors;
struct RexxPort *rxp;
uint32 rxmask;
BPTR   rxout;
bool   rxsigbit,ignoreEvent,rxquiet;
STRPTR lasterror;


void
EndRxPos(STRPTR pos)
{
	if (pos)
		rxpage->pg_MarkCol = -1;
}


void
BeginRxPos(int8 mode, STRPTR pos)
{
	struct tablePos tp;
	struct Term *t;
	
	if (pos) {
		tf_col = tf_row = 0;
		if (FillTablePos(&tp, t = CreateTree(rxpage, pos))) {
			SetMark(rxpage, -1, 0, 0, 0);
			if (mode == RXPOS_CELL)
				tp.tp_Width = tp.tp_Height = 0;

			CopyMem(&tp, &rxpage->pg_MarkCol, sizeof(struct tablePos));
			rxpage->pg_MarkX1 = rxpage->pg_MarkX2 = 0;
		}
		DeleteTree(t);
	}
}


BOOL
GetRxTablePos(struct tablePos *tp)
{
	if (!tp)
		return FALSE;

	if (rxpage->pg_MarkCol == -1) {
		if (rxpage->pg_Gad.DispPos == PGS_NONE) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_NO_CELLS_SELECTED_ERR));
			return FALSE;
		}
		tp->tp_Col = rxpage->pg_Gad.cp.cp_Col;
		tp->tp_Row = rxpage->pg_Gad.cp.cp_Row;
		tp->tp_Width = tp->tp_Height = 0;
	} else
		CopyMem(&rxpage->pg_MarkCol,tp,sizeof(struct tablePos));

	return TRUE;
}


BOOL
GetRxPos(LONG *col, LONG *row)
{
	if (rxpage->pg_MarkCol == -1) {
		if (rxpage->pg_Gad.DispPos == PGS_NONE) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_NO_CELLS_SELECTED_ERR));
			return FALSE;
		}
		if (col)
			*col = rxpage->pg_Gad.cp.cp_Col;
		if (row)
			*row = rxpage->pg_Gad.cp.cp_Row;
	} else {
		if (rxpage->pg_MarkWidth || rxpage->pg_MarkHeight)
			return FALSE;

		if (col)
			*col = rxpage->pg_MarkCol;
		if (row)
			*row = rxpage->pg_MarkRow;
	}
	return TRUE;
}


struct colorPen *
GetRxPen(LONG arg)
{
	struct colorPen *pen;

	if (!arg)
		return NULL;

	if (!(pen = (struct colorPen *)FindTag(&colors,(STRPTR)arg)))
	{
		ULONG num;

		if (*(STRPTR)arg == '0' && *((STRPTR)arg+1) == 'x' || *(STRPTR)arg == '#')
#ifdef __amigaos4__
			sscanf((STRPTR)arg, "%x", &num);
#else
			stch_l((STRPTR)arg,(LONG *)&num);
#endif
		else
			num = atol((STRPTR)arg);

		return GetColorPen(num);
	}
	return pen;
}


void
SetRexxResult(STRPTR var, STRPTR s)
{
	if (!s)
		s = "";

	if ((var = AllocString(var)) != 0)
	{
		StringToUpper(var);
		SetRexxVar(rxmsg,var,s,strlen(s));
	}
	else if (rxmsg->rm_Action & RXFF_RESULT)
		rxmsg->rm_Result2 = (long)CreateArgstring(s,strlen(s));
}


void
NotifyRexxScript(struct RexxScript *rxs)
{
	BPTR file;

	if (!rxs)
		return;

	D(bug("notified: %s\n",rxs->rxs_Node.ln_Name));

	if ((file = Open(rxs->rxs_NotifyRequest.nr_Name, MODE_OLDFILE)) != 0)
	{
#ifdef __amigaos4__
		struct ExamineData *edata;
		
		if(edata = ExamineObjectTags(EX_FileHandleInput, file, TAG_END)) {
			FreeRexxScriptData(rxs);
			if ((rxs->rxs_Data = AllocPooled(pool, rxs->rxs_DataLength = edata->FileSize + 1)) != 0)
				Read(file, rxs->rxs_Data, edata->FileSize);

			RefreshLockList(&rxs->rxs_Map->mp_RexxScripts);
			FreeDosObject(DOS_EXAMINEDATA, edata); 
		}
#else
		struct FileInfoBlock ALIGNED fib;

		if (ExamineFH(file,&fib))
		{
			FreeRexxScriptData(rxs);
			if ((rxs->rxs_Data = AllocPooled(pool, rxs->rxs_DataLength = fib.fib_Size+1)) != 0)
				Read(file,rxs->rxs_Data,fib.fib_Size);

			RefreshLockList(&rxs->rxs_Map->mp_RexxScripts);
		}
#endif
		Close(file);
	}
}


void
CreateRexxScriptHeader(struct RexxScript *rxs)
{
	char t[256];

	strcpy(t,"/* ignition-Script - ");
	strcat(t,rxs->rxs_Node.ln_Name);
	strcat(t," */\n\n\n");

	/* MERKER: Header mit Datum!? */

	FreeRexxScriptData(rxs);
	rxs->rxs_Data = AllocString(t);
	rxs->rxs_DataLength = strlen(t)+1;
}


void
EditRexxScript(struct Mappe *mp, struct RexxScript *rxs)
{
	char t[256];
	long i;

	if (!rxs || !mp)
		return;

	strcpy(t, "T:ign");
	sprintf(t + 5, "%08lx", mp);
	strncat(t, rxs->rxs_Node.ln_Name, 92);
	
	// Replace spaces with underscores
	for (i = 0; t[i]; i++) {
		if (t[i] == ' ')
			t[i] = '_';
	}

	if (!rxs->rxs_NotifyRequest.nr_Name && notifyport) {
		// add notification
		BPTR file;

		rxs->rxs_NotifyRequest.nr_Name = AllocString(t);
		rxs->rxs_NotifyRequest.nr_Flags = NRF_SEND_MESSAGE | NRF_WAIT_REPLY;
		rxs->rxs_NotifyRequest.nr_stuff.nr_Msg.nr_Port = notifyport;
		rxs->rxs_NotifyRequest.nr_UserData = (ULONG)rxs;

		if ((file = Open(t, MODE_NEWFILE)) != 0) {
			if (!rxs->rxs_Data)
				CreateRexxScriptHeader(rxs);

			if (rxs->rxs_Data)
				Write(file,rxs->rxs_Data,rxs->rxs_DataLength-1);  // trailing null byte

			Close(file);
		}

		if (StartNotify(&rxs->rxs_NotifyRequest) != DOSTRUE) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_INIT_SCRIPT_NOTIFICATION_ERR), rxs->rxs_Node.ln_Name);
			FreeString((const STRPTR)rxs->rxs_NotifyRequest.nr_Name);
			rxs->rxs_NotifyRequest.nr_Name = NULL;
		}
	}

	if (rxs->rxs_NotifyRequest.nr_Name && gEditor != NULL && gEditor[0]) {
		// run editor

		strins(t, " ");
		strins(t, gEditor);

		SystemTags(t,
			SYS_Asynch,		TRUE,
			SYS_Input,		NULL,
			SYS_Output,		NULL,
			SYS_UserShell,	TRUE,
			TAG_END);
	}
}


void FreeRexxScriptData(struct RexxScript *rxs)
{
	if (rxs && rxs->rxs_Data)
	{
		FreePooled(pool,rxs->rxs_Data,rxs->rxs_DataLength);
		rxs->rxs_DataLength = 0;
	}
}


void DeleteRexxScript(struct Mappe *mp,struct RexxScript *rxs)
{
	if (!mp || !rxs)
		return;

	if (rxs->rxs_NotifyRequest.nr_Name)
	{
		EndNotify(&rxs->rxs_NotifyRequest);
#ifdef __amigaos4__
		Delete(rxs->rxs_NotifyRequest.nr_Name);
#else
		DeleteFile(rxs->rxs_NotifyRequest.nr_Name);
#endif
		FreeString((const STRPTR)rxs->rxs_NotifyRequest.nr_Name);
	}
	RemoveFromLockedList(&mp->mp_RexxScripts,(struct MinNode *)rxs);
	FreeString(rxs->rxs_Node.ln_Name);
	FreeRexxScriptData(rxs);

	FreePooled(pool,rxs,sizeof(struct RexxScript));
}


struct RexxScript *NewRexxScript(struct Mappe *mp,STRPTR name,BYTE header)
{
	struct RexxScript *rxs;

	if (FindTag(&mp->mp_RexxScripts,name))
		return(NULL);

	if ((rxs = AllocPooled(pool, sizeof(struct RexxScript))) != 0)
	{
		rxs->rxs_Node.ln_Name = AllocString(name);
		rxs->rxs_Map = mp;
		if (header)
			CreateRexxScriptHeader(rxs);
		AddLockedTail(&mp->mp_RexxScripts,(struct MinNode *)rxs);
	}
	return(rxs);
}


void setKomma(struct tableField *tf,BYTE add,BYTE sub,LONG komma)
{
	if (add)
		tf->tf_Komma++;
	else if (sub)
		tf->tf_Komma--;
	else if (komma)
		tf->tf_Komma = *(long *)komma;

	if (add || sub || komma)
	{
		if (tf->tf_Komma > 9)
			tf->tf_Komma = 9;
		if (tf->tf_Komma < -1)
			tf->tf_Komma = -1;
		tf->tf_Flags |= TFF_KOMMASET;
	}
}

// FORMAT POS,SET/K,TYPE/N,FRACTION/N,ALIGN/K,PRI/N,NEGPEN/K,NEGPARENTHESES/S,REQ/S,GLOBAL/S
//		0   1	 2	  3		  4	   5	 6		7				8	 9

long changeFormat(struct tableField *tf,long *opts,ULONG pen,UBYTE align)
{
	long maxcol;

	setKomma(tf, 0, 0, opts[3]);

	if (opts[1])
	{
		FreeString(tf->tf_Format);
		tf->tf_Format = AllocString((STRPTR)opts[1]);
		tf->tf_Flags |= TFF_FORMATSET;
	}
	if (opts[4])
		tf->tf_Alignment = (tf->tf_Alignment & ~(TFA_HCENTER | TFA_VIRGIN)) | align;
	if (opts[6])
	{
		tf->tf_NegativePen = pen;
		tf->tf_Flags |= TFF_NEGPENSET;
	}
	if (opts[7])
		tf->tf_Flags |= TFF_NEGPARENTHESES;

	maxcol = tf->tf_Col+tf->tf_Width;
	if (tf != rxpage->pg_Gad.tf || rxpage->pg_Gad.DispPos <= PGS_FRAME)
	{
		SetTFText(rxpage,tf,(STRPTR)~0L);
		if (maxcol < tf->tf_Col+tf->tf_Width)
			maxcol = tf->tf_Col+tf->tf_Width;
		/*if (tf == rxpage->pg_Gad.tf)
			rxpage->pg_Gad.cp.cp_W = GetTotalWidth(rxpage,tf);
		DrawTableField(rxpage,tf);*/
	}
	return maxcol;
}

// NOP - a do nothing function pointer

ULONG
rxNop(long *opts)
{
	return RC_OK;
}


void
LoadBlock(struct Page *page, STRPTR name, long mode)
{
	struct IFFHandle *iff;
	struct MinList list;
	long   error;

	if (!(iff = AllocIFF()))
		return;

	InitIFFasDOS(iff);
	MyNewList(&list);

	if ((iff->iff_Stream = (IPTR)Open(name, MODE_OLDFILE)) != 0)
	{
		if (!(error = OpenIFF(iff, IFFF_READ)))
		{
			PropChunk(iff, ID_TABL, ID_CELL);
			StopOnExit(iff, ID_TABL, ID_FORM);

			if (!(error = ParseIFF(iff, IFFPARSE_SCAN)))
				error = LoadCells(iff, ID_TABL, page, &list);

			if (error && error != IFFERR_EOF)
				ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_BLOCK_ERR), IFFErrorText(error));
			CloseIFF(iff);
		}
		Close((BPTR)iff->iff_Stream);
	}
	FreeIFF(iff);

	if (IsListEmpty((struct List *)&list))
		return;

	PasteCells(page, &list, mode | PC_CLIPBOARD);

	{
		struct tableField *tf;

		while ((tf = (struct tableField *)MyRemHead(&list)) != 0)
			FreeTableField(tf);
	}
}


void
SaveBlock(struct Page *page, STRPTR name)
{
	struct IFFHandle *iff;
	long   error;

	if (!(iff = AllocIFF()))
		return;

	InitIFFasDOS(iff);

	if ((iff->iff_Stream = (IPTR)Open(name, MODE_NEWFILE)) != 0)
	{
		if (!(error = OpenIFF(iff, IFFF_WRITE)))
		{
			if (!error && !(error = PushChunk(iff, ID_TABL, ID_FORM, IFFSIZE_UNKNOWN)))
			{
				ULONG handle;
				if ((handle = GetCellIterator(page, (struct tablePos *)&page->pg_MarkCol, FALSE)) != 0) {
					error = SaveCells(iff, page, handle, IO_IGNORE_PROTECTED_CELLS | IO_SAVE_FULL_NAMES);
					FreeCellIterator(handle);
				}
				if (!error)
					error = PopChunk(iff);
			}
			if (error)
				ErrorRequest(GetString(&gLocaleInfo, MSG_SAVE_BLOCK_ERR), IFFErrorText(error));
			CloseIFF(iff);
		}
		Close((BPTR)iff->iff_Stream);
	}
	FreeIFF(iff);
}


// BLOCK POS,NAME,LOAD/S,SAVE/S,REQ/S,TEXTONLY/S
//	   		0   1	2	  3	  	4	 	5

ULONG
rxBlock(long *opts)
{
	char name[400];

	if (!opts[1] && !opts[4])
		return(RC_WARN);

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	if (opts[1])
		strcpy(name,(STRPTR)opts[1]);
	else
		name[0] = 0;

	if ((opts[1] && !opts[4]) || AslRequestTags(fileReq,
#ifdef __amigaos4__
														   	ASLFR_Window,		 rxpage->pg_Window,
#else
														   	ASLFR_Window,		 scr->FirstWindow,
#endif
															ASLFR_TitleText,	 opts[2] ? GetString(&gLocaleInfo, MSG_LOAD_BLOCK_TITLE) : GetString(&gLocaleInfo, MSG_SAVE_BLOCK_TITLE),
															ASLFR_InitialDrawer, "",
															ASLFR_InitialFile,   name,
															ASLFR_InitialPattern,"#?",
#ifdef __amigaos4__
															ASLFR_AcceptPattern, "#?.igb",
#endif
															ASLFR_DoSaveMode,	opts[3],
															ASLFR_DoPatterns,	FALSE,
															ASLFR_DrawersOnly,   FALSE,
															TAG_END))
	{
		if (opts[4])
		{
			strcpy(name,fileReq->fr_Drawer);
			AddPart(name,fileReq->fr_File,400);
		}
		if (opts[2])
			LoadBlock(rxpage,name,0 | (opts[5] ? PC_TEXTONLY : 0));
		else
			SaveBlock(rxpage,name);
	}

	EndRxPos((STRPTR)opts[0]);
	return RC_OK;
}

// CUT POS,TEXTONLY/S,UNIT/N/K
//	 0   1		  2

ULONG
rxCut(long *opts)
{
	ULONG unit = clipunit;

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	if (opts[2])
		clipunit = *(LONG *)opts[2];

	CutCopyClip(rxpage,CCC_CUT | CCC_CURRENT | (opts[1] ? CCC_TEXTONLY : 0));
	clipunit = unit;

	EndRxPos((STRPTR)opts[0]);

	return RC_OK;
}


// COPY POS,UNIT/N/K
//	  0   1

ULONG
rxCopy(long *opts)
{
	ULONG unit = clipunit;

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	if (opts[1])
		clipunit = *(LONG *)opts[1];
	CutCopyClip(rxpage,CCC_COPY | CCC_CURRENT);
	clipunit = unit;

	EndRxPos((STRPTR)opts[0]);
	return RC_OK;
}

// DELETE POS,TEXTONLY/S
//		0   1

ULONG
rxDelete(long *opts)
{
	BeginRxPos(RXPOS_BLOCK, (STRPTR)opts[0]);
	CutCopyClip(rxpage, CCC_DELETE | CCC_CURRENT | (opts[1] ? CCC_TEXTONLY : 0));
	EndRxPos((STRPTR)opts[0]);

	return RC_OK;
}

// PASTE POS,TEXTONLY/S,UNIT/N/K,INTERN/S
//	   0   1		  2		3

ULONG
rxPaste(long *opts)
{
	ULONG unit = clipunit;

	BeginRxPos(RXPOS_CELL, (STRPTR)opts[0]);
	if (opts[2])
		clipunit = *(LONG *)opts[2];
	PasteClip(rxpage, (struct PasteNode *) (opts[3] ? (APTR)~0L : NULL), (opts[1] ? PC_TEXTONLY : 0));
	clipunit = unit;

	EndRxPos((STRPTR)opts[0]);
	return RC_OK;
}

// CLIP REQ/S,UNIT/N/K
//	  0	 1

ULONG rxClip(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_CLIP,TAG_END);
	if (opts[1])
		clipunit = *(long *)opts[1];
	return(RC_OK);
}

// BORDER POS,LEFT/K,RIGHT/K,BELOW/K,ABOVE/K,BLOCK/S,REQ/S
//		0   1	  2	   3	   4	   5	   6

ULONG rxBorder(long *opts)
{
	ULONG  color[4] = {~0L,~0L,~0L,~0L};
	long   point[4],i,j;
	struct colorPen *cp;
	double pt;
	STRPTR t;

	if (opts[6])
		OpenAppWindow(WDT_BORDER,TAG_END);
	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);
	if (opts[1] || opts[2] || opts[3] || opts[4])
	{
		for(i = 0;i < 4;i++)
		{
			if ((t = (STRPTR)opts[i + 1]) != 0)
			{
				for(j = 0;*(t+j) && *(t+j) != '/';j++);
				if (*(t+j) && *(t+j+1))
				{
					for(cp = (APTR)colors.mlh_Head;j && cp->cp_Node.ln_Succ && strnicmp(t,cp->cp_Node.ln_Name,j);cp = (APTR)cp->cp_Node.ln_Succ);
					if (j && cp->cp_Node.ln_Succ)
						color[i] = cp->cp_ID;
					else if (j && isdigit(*t) && (cp = GetColorPen(atol(t))))
						color[i] = cp->cp_ID;
					else
						color[i] = FindColorPen(0,0,0);
					t += j+1;
					pt = ConvertNumber(t,CNT_POINT);
					point[i] = ((long)pt) << 5;
				}
				else
				{
					color[i] = 0;
					point[i] = 0;
				}
			}
		}
		SetBorder(rxpage,opts[5],color[0],point[0],color[1],point[1],color[2],point[2],color[3],point[3]);
	}
	EndRxPos((STRPTR)opts[0]);
	return(RC_OK);
}

// FORMAT POS,SET/K,TYPE/N,FRACTION/N,ALIGN/K,PRI/N,NEGPEN/K,NEGPARENTHESES/S,REQ/S,GLOBAL/S
//		0   1	 2	  3		  4	   5	 6		7				8	 9

ULONG rxFormat(long *opts)
{
	struct tableField *tf = NULL;
	struct colorPen *cp;
	long   col,maxcol = 0;
	UBYTE  align;

	if (opts[8])
	{
		OpenAppWindow(WDT_CELL,WA_Page,0,TAG_END);
		return 0;
	}
	cp = GetRxPen(opts[6]);

	if (opts[4])
	{
		if (*(STRPTR)opts[4] == 'l' || !stricmp((STRPTR)opts[4],"left"))
			align = TFA_LEFT;
		else if (*(STRPTR)opts[4] == 'r' || !stricmp((STRPTR)opts[4],"right"))
			align = TFA_RIGHT;
		else if (*(STRPTR)opts[4] == 'c' || !stricmp((STRPTR)opts[4],"center"))
			align = TFA_HCENTER;
		else
		{
			ErrorRequest(GetString(&gLocaleInfo, MSG_INVALID_ALIGNMENT_ERR));
			align = TFA_LEFT;
		}
	}

	if (opts[1] && opts[2])
		AddFormat(opts[9] ? &prefs.pr_Formats : &rxpage->pg_Mappe->mp_Prefs.pr_Formats,(STRPTR)opts[1],opts[5] ? *(long *)opts[5] : 0,opts[3] ? *(long *)opts[3] : -1,opts[4] ? align : -1,cp ? cp->cp_ID : 0,ITA_NONE,opts[2]);

	if (opts[1] && opts[2] || opts[3] || opts[4] || opts[6] || opts[7])
	{
		BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);
		BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_SET_FORMAT_UNDO));

		while ((tf = GetMarkedFields(rxpage, tf, TRUE)) != 0)
		{
			if ((col = changeFormat(tf,opts,cp ? cp->cp_ID : 0,align)) > maxcol)
				maxcol = col;
		}
		EndUndo(rxpage);
		RefreshMarkedTable(rxpage,maxcol,TRUE);
		EndRxPos((STRPTR)opts[0]);
	}
	return 0;
}

// FRACTION POS,NUM/N,INC/S,DEC/S
//		  0   1   2	 3

ULONG rxFraction(long *opts)
{
	struct tableField *tf = NULL;
	long   col,maxcol = 0;

	if (opts[1] || opts[2] || opts[3])
	{
		BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);
		BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_DECIMAL_PLACES_UNDO));

		while ((tf = GetMarkedFields(rxpage, tf, TRUE)) != 0)
		{
			setKomma(tf,opts[2],opts[3],opts[1]);
			col = tf->tf_Col+tf->tf_Width;
			if (tf != rxpage->pg_Gad.tf || rxpage->pg_Gad.DispPos <= PGS_FRAME)
			{
				SetTFText(rxpage,tf,(STRPTR)~0L);
				if (col < tf->tf_Col+tf->tf_Width)
					col = tf->tf_Col+tf->tf_Width;
			}
			if (maxcol < col)
				maxcol = col;
		}
		EndUndo(rxpage);
		RefreshMarkedTable(rxpage,maxcol,TRUE);
		EndRxPos((STRPTR)opts[0]);
	}
	return 0;
}

// ZOOM PERCENT/N,PAGE/S,REQ/S
//	  0		 1	  2

ULONG rxZoom(long *opts)
{
	long zoom;

	if (opts[2])
		OpenAppWindow(WDT_ZOOM,TAG_END);
	if (opts[0] || opts[1])
	{
		if (opts[1])
			zoom = -1;
		else
			zoom = (*(long *)opts[0] << 10)/100;

		SetZoom(rxpage,zoom,FALSE,TRUE);
	}
	return(RC_OK);
}

// CALC EXP=EXPRESSION,VAR/K
//	  0			  1

ULONG rxCalc(long *opts)
{
	struct Term *k;
	struct Result r;
	STRPTR s;

	if ((k = CreateTree(rxpage, (STRPTR)opts[0])) != 0)
	{
		tf_col = 0;  tf_row = 0;  tf_format = 0;
		memset(&r,0,sizeof(struct Result));

#ifdef __amigaos4__
		if (!CalcTree(&r,k))
#else
		if (CalcTree(&r,k))
#endif
		{
			if (r.r_Type == RT_TEXT)
				s = r.r_Text;
			else
				s = AllocString(FitValueInFormat(r.r_Value,NULL,tf_format,-1,ITA_NONE));
		}
		else
			s = "#err-type";
		SetRexxResult((STRPTR)opts[1],s);
		DeleteTree(k);
	}
	return(RC_OK);
}

// RECALC

ULONG rxRecalc(long *opts)
{
	RecalcTableFields(rxpage);
	return(RC_OK);
}

// QUIET TOGGLE/S,RESUME/S,DISCARD/S
//	   0		1		2

ULONG rxQuiet(long *opts)
{
	if (!rxquiet && opts[0] || !opts[1] || !opts[2])
		rxquiet = TRUE;
	else if (rxquiet && (opts[0] || opts[1]))
	{
		struct Node *ln;

		rxquiet = FALSE;
		if (!opts[2])
		{
			foreach(&errors,ln)		  // vorerst
				ErrorRequest(ln->ln_Name);
		}
		while((ln = MyRemHead(&errors)) != 0)
		{
			FreeString(ln->ln_Name);
			FreePooled(pool,ln,sizeof(struct Node));
		}
	}
	return(RC_OK);
}

// LASTERROR VAR
//		   0

ULONG rxLastError(long *opts)
{
	STRPTR err = lasterror ? lasterror : (STRPTR)"ok";

	SetRexxResult((STRPTR)opts[0],err);
	return(RC_OK);
}

// QUIT

ULONG rxQuit(long *opts)
{
	BYTE changed = FALSE;
	struct Mappe *mp;

	foreach(&gProjects,mp)
	{
		if (mp->mp_Modified)
			changed++;
	}
	if (changed > 1)
	{
		long rc;

		rc = DoRequest(GetString(&gLocaleInfo, MSG_UNSAVED_DOCUMENTS_REQ),GetString(&gLocaleInfo, MSG_QUIT_SAVE_CANCEL_REQ));
		if (rc == 2)
		{
			while(!IsListEmpty((struct List *)&gProjects))
			{
				if (!DisposeProject((struct Mappe *)gProjects.mlh_Head))
				{
					changed = FALSE;
					break;
				}
			}
			ende = changed;
		}
		else if (rc)
		{
			foreach(&gProjects,mp)  // schnell und leise beenden
				mp->mp_Modified = 0;
			ende = TRUE;
		}
	}
	else
		ende = TRUE;
	return(RC_OK);
}

// ABOUT

ULONG rxAbout(long *opts)
{
	OpenAppWindow(WDT_INFO,WA_Data,NULL,TAG_END);
	return 0;
}

// NEW

ULONG rxNew(long *opts)
{
	OpenProjWindow(NULL,TAG_END);
	return 0;
}

// NEWPAGE NAME/K,QUIET/S
//		 0	  1

ULONG rxNewPage(long *opts)
{
	struct Page *page = rxpage;

	if (NewPage(page->pg_Mappe))
	{
		if (opts[0])
		{
			FreeString(rxpage->pg_Node.ln_Name);
			rxpage->pg_Node.ln_Name = AllocString((STRPTR)opts[0]);
		}
		if (!opts[1])
			UpdateProjPage(page->pg_Window,rxpage);
	}
	return 0;
}

// CURRENT MAP=WINDOW/K,PAGE,NEXT/S,PREV/S,SHOW/S,NUM/N/K
//		      0			 1 	  2	     3	    4	  5

ULONG rxCurrent(long *opts)
{
	struct Mappe *mp = NULL;
	struct Page *page = NULL;

	if (opts[0])	   // MAP=WINDOW
	{
		mp = (struct Mappe *)FindTag(&gProjects,(STRPTR)opts[0]);
	}
	else if (opts[5])
		mp = (struct Mappe *)FindListNumber(&gProjects,*(LONG *)opts[5]);
	else if (rxpage)
		mp = rxpage->pg_Mappe;
	if (!mp)
		return(RC_WARN);

	if (opts[1])	   // PAGE
	{
		if (!(page = (struct Page *)FindTag(&mp->mp_Pages,(STRPTR)opts[1])) && !(page = (struct Page *)FindListNumber(&mp->mp_Pages,atol((STRPTR)opts[1]))))
			return(RC_WARN);
	}
	else if (opts[2])  // NEXT
	{
		if (rxpage->pg_Node.ln_Succ->ln_Succ)
			page = (struct Page *)rxpage->pg_Node.ln_Succ;
	}
	else if (opts[3])  // PREV
	{
		if (rxpage->pg_Node.ln_Pred->ln_Pred)
			page = (struct Page *)rxpage->pg_Node.ln_Pred;
	}
	else if (!rxpage || rxpage->pg_Mappe != mp)
		page = mp->mp_actPage;

	SetMainPage(page);

	if (opts[4])	   // SHOW
	{
		if (mp->mp_Window)
		{
			UpdateProjPage(mp->mp_Window,rxpage);
			ActivateWindow(mp->mp_Window);
			WindowToFront(mp->mp_Window);
		}
		else
			OpenProjWindow(rxpage,TAG_END);
	}
	return(RC_OK);
}

// PAGE NAME/K,UP/S,DOWN/S,FIRST/S,LAST/S,REQ/S
//	  0	  1	2	  3	   4	  5

ULONG rxPage(long *opts)
{
	struct Page *page = rxpage;
	struct Node *ln = NULL;

	if (opts[0])
		page = (struct Page *)FindTag(&rxpage->pg_Mappe->mp_Pages,(STRPTR)opts[0]);
	if (page)
	{
		if (opts[1])
			ln = page->pg_Node.ln_Pred->ln_Pred;
		else if (opts[2] && page->pg_Node.ln_Succ->ln_Succ)
			ln = page->pg_Node.ln_Succ;
		else if (opts[3])
			ln = (struct Node *)&page->pg_Mappe->mp_Pages;
		else if (opts[4])
			ln = (struct Node *)page->pg_Mappe->mp_Pages.mlh_TailPred;
		if (ln)
		{
			MyRemove(page);
			Insert((struct List *)&page->pg_Mappe->mp_Pages,(struct Node *)page,ln);
		}
		if (opts[5])
			OpenAppWindow(WDT_PAGE,WA_Data,page,TAG_END);
	}
	return(RC_OK);
}

// DELETEPAGE NAME/K
//			0

ULONG rxDeletePage(long *opts)
{
	struct Mappe *mp;
	struct Page *page = rxpage;

	mp = page->pg_Mappe;
	if (opts[0] && !(page = (struct Page *)FindTag(&mp->mp_Pages,(STRPTR)opts[0])))
		page = (struct Page *)FindListNumber(&mp->mp_Pages,atol((STRPTR)opts[0]));
	if (page)
	{
		MyRemove(page);
		if (IsListEmpty((struct List *)&mp->mp_Pages))
			rxpage = NewPage(mp);
		else if (rxpage == page)
			rxpage = (struct Page *)mp->mp_Pages.mlh_Head;
		MyAddTail(&mp->mp_Pages, page);
		if (page->pg_Window)
			UpdateProjPage(page->pg_Window,rxpage);
		DisposePage(page);
#ifdef __amigaos4__
		RecalcTableFields(rxpage); //recalc all links to deleted table
#endif
	}
	return RC_OK;
}

// HIDE

ULONG
rxHide(long *opts)
{
	struct Mappe *mp;
	struct Window *win;

	if ((win = (mp = rxpage->pg_Mappe)->mp_Window) != 0)
	{
		((struct Page *)((struct winData *)win->UserData)->wd_Data)->pg_Window = NULL;
		mp->mp_Window = NULL;
		CloseAppWindow(win,TRUE);

		if (!GetAppWindow(WDT_ANY))
			OpenAppWindow(WDT_INFO,TAG_END);
	}
	return RC_OK;
}

// INTERACTIVE SET/S,TOGGLE/S,NONE/S
//			 0	 1		2

ULONG
rxInteractive(long *opts)
{
	struct Mappe *mp;

	mp = rxpage->pg_Mappe;
	if (opts[0] || opts[1] && !(mp->mp_Flags & MPF_SCRIPTS))
		mp->mp_Flags |= MPF_SCRIPTS;
	else if (opts[2] || opts[1])
		mp->mp_Flags &= ~MPF_SCRIPTS;

	UpdateInteractive(mp, TRUE);
	return RC_OK;
}

#ifdef DEBUG

extern void DumpReference(struct Reference *r);
extern void VerboseDumpReference(struct Reference *r);

extern struct ArrayList gUnresolvedRefs,gTimedRefs;

// DUMPREF UNRESOLVED/S TIMED/S
//		 0			1

ULONG rxDumpRef(long *opts)
{
	struct Reference **refs;
	int index;

	if (opts[0])	// unresolved references
	{
		bug("*** begin dumping of unresolved references ***\n");
		refs = (struct Reference **)gUnresolvedRefs.al_List;
		index = gUnresolvedRefs.al_Last;
	}
	else if (opts[1])   // timed references
	{
		bug("*** begin dumping of timed references ***\n");
		refs = (struct Reference **)gTimedRefs.al_List;
		index = gTimedRefs.al_Last;
	}

	if (opts[0] || opts[1])
	{
		while (index-- > 0)
		{
			bug("%ld. ",index);
			VerboseDumpReference(refs[index]);
		}
		bug("/// end of reference dump ///\n");
	}
	else
	{
		struct tableField *tf;

		bug("### dump current cell reference ###\n");
		if (rxpage && (tf = rxpage->pg_Gad.tf))
			VerboseDumpReference(tf->tf_Reference);
		else
			bug("\tno page or cell\n");
		bug("||| end of reference dump |||\n");
	}
	return RC_OK;
}

#endif

// DOCUMENT REQ/S
//		  0

ULONG
rxDocument(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_DOCUMENT,TAG_END);

	return RC_OK;
}

// DOCINFO REQ/S
//		 0

ULONG
rxDocInfo(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_DOCINFO, TAG_END);

	return RC_OK;
}

// PAGESETUP REQ/S
//		  0

ULONG
rxPageSetup(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_PAGESETUP, TAG_END);

	return RC_OK;
}

// DIAGRAM NAME/K,REQ/S
//		 0	  1

ULONG
rxDiagram(long *opts)
{
	if (opts[1])
		OpenAppWindow(WDT_DIAGRAM, TAG_END);

	return RC_OK;
}

// TITLE POS,SET/K,DELETE/S,HORIZ/S,VERT/S,REQ/S
//	   0   1	 2		3	   4	  5

ULONG
rxTitle(long *opts)
{
	long col,row;

	if (opts[5])
		OpenAppWindow(WDT_SETTITLE,WA_Data,opts[4],TAG_END);

	BeginRxPos(RXPOS_CELL,(STRPTR)opts[0]);

	if (!GetRxPos(&col,&row))
	{
		EndRxPos((STRPTR)opts[0]);
		return(RC_WARN);
	}

	if (opts[1])   // Titel setzen
	{
		struct tableSize *ts;

		if (opts[4]) // vertikal
			col = 0;
		else		 // horizontal
			row = 0;

		AllocTableSize(rxpage,col,row);
		if (opts[4])
			ts = rxpage->pg_tfHeight+row-1;
		else
			ts = rxpage->pg_tfWidth+col-1;

		if (ts)
		{
			FreeString(ts->ts_Title);
			ts->ts_Title = AllocString((STRPTR)opts[1]);
			DrawTableTitles(rxpage,!opts[4]);
			RecalcMap(rxpage->pg_Mappe);
		}
	}
	else		   // Titel löschen
	{
		struct tableSize *ts = NULL;

		if (opts[4] && rxpage->pg_Rows >= row)  // vertikal
			ts = rxpage->pg_tfHeight+row-1;
		else if (rxpage->pg_Cols >= col)		// horizontal
			ts = rxpage->pg_tfWidth+col-1;

		if (ts)
		{
			FreeString(ts->ts_Title);
			ts->ts_Title = NULL;
			DrawTableTitles(rxpage,!opts[4]);
			RecalcMap(rxpage->pg_Mappe);
		}
	}
	EndRxPos((STRPTR)opts[0]);
	return(RC_OK);
}

// CELLSIZE POS,WIDTH/K,HEIGHT/K,OPTWIDTH/S,OPTHEIGHT/S,MINWIDTH/S,MINHEIGHT/S,REQ/S
//		  0   1	   2		3		  4		   5		  6		   7

ULONG rxCellSize(long *opts)
{
	UWORD mode = CCS_STANDARD;

	if (opts[7])
		OpenAppWindow(WDT_CELLSIZE,TAG_END);
	else
	{
		if (opts[3] || opts[4])
			mode |= (opts[3] ? CCS_OPTWIDTH : 0) | (opts[4] ? CCS_OPTHEIGHT : 0);
		else if (opts[5] || opts[6])
			mode |= (opts[5] ? CCS_MINWIDTH : 0) | (opts[6] ? CCS_MINHEIGHT : 0);
		BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);
		if (opts[1] || opts[2] || (mode & (CCS_OPTWIDTH | CCS_OPTHEIGHT)))
			ChangeCellSize(rxpage,(STRPTR)opts[1],(STRPTR)opts[2],mode,NULL);
		EndRxPos((STRPTR)opts[0]);
	}
	return RC_OK;
}

// CELLCOLUMNS POS,SET/S,TOGGLE/S,RESET/S
//			 0   1	 2		3

ULONG
rxCellColumns(long *opts)
{
	struct tableField *tf;
	struct tablePos tp;
	long   maxcol = 0;

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);
	BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_CELL_WIDTH_UNDO));

	if (!GetRxTablePos(&tp))
		return RC_WARN;
	if (tp.tp_Width == -1)
		tp.tp_Width = rxpage->pg_Cols-tp.tp_Col;
	if (tp.tp_Height == -1)
		tp.tp_Height = rxpage->pg_Rows-tp.tp_Row;

	for (;tp.tp_Height >= 0;tp.tp_Height--)
	{
		if ((tf = GetTableField(rxpage, tp.tp_Col, tp.tp_Row + tp.tp_Height)) != 0)
		{
			if (opts[1] || opts[2] && tf->tf_WidthSet == 0xffff)
				tf->tf_WidthSet = tp.tp_Width;
			else if (opts[3] || opts[2] && tf->tf_WidthSet != 0xffff)
				tf->tf_WidthSet = 0xffff;
			if (tf->tf_Width+tp.tp_Col > maxcol)
				maxcol = tf->tf_Width+tp.tp_Col;
			SetTFWidth(rxpage,tf);
			if (tf->tf_Width+tp.tp_Col > maxcol)
				maxcol = tf->tf_Width+tp.tp_Col;
		}
	}
	EndUndo(rxpage);
	RefreshMarkedTable(rxpage,maxcol,TRUE);
	EndRxPos((STRPTR)opts[0]);

	return RC_OK;
}

// CELL POS,INSERTROW/S,INSERTCOL/S,SHIFTDOWN/S,SHIFTRIGHT/S,REMOVEROW/S,REMOVECOL/S,SHIFTUP/S,SHIFTLEFT/S
//	  0       1		       2		   3		   4			5		   6		   7		 8

ULONG
rxCell(long *opts)
{
	struct UndoNode *un;
	struct tableField *tf,*ctf;
	struct tablePos tp;
	long   diff,mode = 0;
	char   s[32];

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	if (!GetRxTablePos(&tp))
		return RC_WARN;

	if (tp.tp_Width == -1 && tp.tp_Height == -1)
	{
		EndRxPos((STRPTR)opts[0]);
		return RC_WARN;
	}
	if (opts[1] || opts[3])
		mode = UNT_INSERT_VERT_CELLS;
	if (opts[2] || opts[4])
		mode = UNT_INSERT_HORIZ_CELLS;
	if (opts[5] || opts[7])
		mode = UNT_REMOVE_VERT_CELLS;
	if (opts[6] || opts[8])
		mode = UNT_REMOVE_HORIZ_CELLS;
	if (opts[1] || opts[5])  // Reihe einfügen/entfernen
	{
		strcpy(s,GetString(&gLocaleInfo, MSG_ROW_UNDO));
		tp.tp_Col = 1;  tp.tp_Width = -1;
	}
	if (opts[2] || opts[6])  // Spalte einfügen/entfernen
	{
		strcpy(s,GetString(&gLocaleInfo, MSG_COLUMN_UNDO));
		tp.tp_Row = 1;  tp.tp_Height = -1;
	}
	if (opts[3] || opts[4] || opts[7] || opts[8])
		strcpy(s,GetString(&gLocaleInfo, MSG_CELLS_UNDO));

	if (mode)
	{
		if (mode == UNT_INSERT_VERT_CELLS || mode == UNT_INSERT_HORIZ_CELLS)
			strcat(s,GetString(&gLocaleInfo, MSG_INSERT_CELLS_UNDO));
		else
			strcat(s,GetString(&gLocaleInfo, MSG_REMOVE_CELLS_UNDO));

		if ((un = CreateUndo(rxpage, UNDO_PRIVATE, s)) != 0)
		{
			un->un_Type = mode;
			un->un_TablePos.tp_Col = tp.tp_Col;   un->un_TablePos.tp_Row = tp.tp_Row;
			un->un_TablePos.tp_Width = tp.tp_Width; un->un_TablePos.tp_Height = tp.tp_Height;
			if (mode == UNT_REMOVE_HORIZ_CELLS || mode == UNT_REMOVE_VERT_CELLS)
			{
				LONG *mms, i;

				foreach (&rxpage->pg_Table, tf)
				{
					if (tf->tf_Col >= tp.tp_Col
						&& (tp.tp_Width == -1 || tf->tf_Col <= tp.tp_Col+tp.tp_Width)
						&& tf->tf_Row >= tp.tp_Row
						&& (tp.tp_Height == -1 || tf->tf_Row <= tp.tp_Row+tp.tp_Height)
						&& (ctf = CopyCell(rxpage, tf)))
					{
						MyAddTail(&un->un_UndoList, ctf);
					}
				}
				if (mode == UNT_REMOVE_VERT_CELLS)
				{
					diff = tp.tp_Height + 1;
					if ((mms = AllocPooled(pool, diff * sizeof(LONG))) != 0)
					{
						for(i = 0; i < diff; i++)
						{
							if (tp.tp_Row+i > rxpage->pg_Rows)
								*(mms+i) = rxpage->pg_mmStdHeight;
							else
								*(mms+i) = (rxpage->pg_tfHeight+tp.tp_Row+i-1)->ts_mm;
						}
					}
					/* MERKER: ErrorRequest() */
				}
				else
				{
					diff = tp.tp_Width + 1;
					if ((mms = AllocPooled(pool, diff * sizeof(LONG))) != 0)
					{
						for(i = 0; i < diff; i++)
						{
							if (tp.tp_Col+i > rxpage->pg_Cols)
								*(mms+i) = rxpage->pg_mmStdWidth;
							else
								*(mms+i) = (rxpage->pg_tfWidth+tp.tp_Col+i-1)->ts_mm;
						}
					}
				}
				un->un_mmUndo = (ULONG)mms;
			}
		}
		InReCells(rxpage, mode, tp.tp_Col, tp.tp_Row, tp.tp_Width, tp.tp_Height, NULL);
	}
	EndRxPos((STRPTR)opts[0]);

	return 0;
}

// TEXT POS,SET/K,DELETE/S
//	  0   1	 2

ULONG
rxText(long *opts)
{
	struct tableField *tf = NULL;
	struct UndoNode *un;
	long   maxcol = 0;

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	if (rxpage->pg_Gad.DispPos != PGS_NONE)
		SetTabGadget(rxpage,(STRPTR)~0L,PGS_FRAME);

	un = BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_CELL_TEXT_UNDO));

	while ((tf = GetMarkedFields(rxpage, tf, opts[2] ? FALSE : TRUE)) != 0)
	{
		FreeString(tf->tf_Original);
		tf->tf_Original = NULL;
		if (opts[1])
			tf->tf_Text = AllocString((STRPTR)opts[1]);
		if (opts[2])
			tf->tf_Text = NULL;
		if (tf->tf_Col+tf->tf_Width > maxcol)
			maxcol = tf->tf_Col+tf->tf_Width;
		UpdateCellText(rxpage,tf);

		if (rxpage->pg_Mappe->mp_Flags & MPF_SCRIPTS)
			UpdateMaskCell(rxpage->pg_Mappe,rxpage,tf,un);
		if (tf->tf_Col+tf->tf_Width > maxcol)
			maxcol = tf->tf_Col+tf->tf_Width;
	}
	EndUndo(rxpage);
	RefreshMarkedTable(rxpage,maxcol,TRUE);
	EndRxPos((STRPTR)opts[0]);

	return RC_OK;
}

// ALIGN POS,LEFT/S,CENTER/S,RIGHT/S,DELETE/S,TOP/S,MIDDLE/S,BOTTOM/S,REQ/S
//	   0   1	  2		3	   4		5	 6		7		8

ULONG
rxAlign(long *opts)
{
	BYTE alignH,alignV;

	if (opts[8])
		OpenAppWindow(WDT_CELL, WA_Page,1,TAG_END);
	else
	{
		BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);
		alignH = (opts[1] ? TFA_LEFT : 0) | (opts[2] ? TFA_HCENTER : 0) | (opts[3] ? TFA_RIGHT : 0) | (opts[4] ? TFA_VIRGIN : 0);
		alignV = (opts[5] ? TFA_TOP : 0) | (opts[6] ? TFA_VCENTER : 0) | (opts[7] ? TFA_BOTTOM : 0);
		SetAlignment(rxpage,alignH,alignV);
		if (opts[4])
			RecalcTableFields(rxpage);
		EndRxPos((STRPTR)opts[0]);
	}
	return(RC_OK);
}

// COLOR POS,APEN/K,BPEN/K,REQ/S
//	   0   1	  2	  3

ULONG rxColor(long *opts)
{
	struct colorPen *apen,*bpen;

	if (opts[3])
		OpenAppWindow(WDT_CELL,WA_Page,1,TAG_END);

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	apen = GetRxPen(opts[1]);
	bpen = GetRxPen(opts[2]);

	if (apen || bpen)
		SetPageColor(rxpage,apen,bpen);

	EndRxPos((STRPTR)opts[0]);

	return(RC_OK);
}

// NEGPARENTHESES POS,SET/S,TOGGLE/S,NONE/S
//				0   1	 2		3

ULONG rxNegParentheses(long *opts)
{
	struct tableField *tf = NULL;
	long   maxcol = 0;

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_NEG_VALUES_UNDO));
	while((tf = GetMarkedFields(rxpage,tf,FALSE)))
	{
		if (opts[1] || opts[2] && !(tf->tf_Flags & TFF_NEGPARENTHESES))
			tf->tf_Flags |= TFF_NEGPARENTHESES;
		else if (opts[3] || opts[2] && tf->tf_Flags & TFF_NEGPARENTHESES)
			tf->tf_Flags &= ~TFF_NEGPARENTHESES;

		if (maxcol < tf->tf_Col+tf->tf_Width)
			maxcol = tf->tf_Col+tf->tf_Width;
		if (!(tf->tf_Type & TFT_FORMULA))
			SetTFText(rxpage,tf,(STRPTR)~0L);
	}
	EndUndo(rxpage);
	RefreshMarkedTable(rxpage,maxcol,TRUE);

	EndRxPos((STRPTR)opts[0]);

	return(RC_OK);
}

// SEPARATOR POS,SET/S,TOGGLE/S,NONE/S
//		   0   1	 2		3

ULONG rxSeparator(long *opts)
{
	struct tableField *tf = NULL;
	long   maxcol = 0;

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_NUM_SEPARATOR_UNDO));
	while ((tf = GetMarkedFields(rxpage, tf, FALSE)) != 0)
	{
		if (opts[1] || opts[2] && !(tf->tf_Flags & TFF_SEPARATE))
			tf->tf_Flags |= TFF_SEPARATE;
		else if (opts[3] || opts[2] && tf->tf_Flags & TFF_SEPARATE)
			tf->tf_Flags &= ~TFF_SEPARATE;

		if (maxcol < tf->tf_Col+tf->tf_Width)
			maxcol = tf->tf_Col+tf->tf_Width;
		if (!(tf->tf_Type & TFT_FORMULA))
			SetTFText(rxpage,tf,(STRPTR)~0L);
	}
	EndUndo(rxpage);
	RefreshMarkedTable(rxpage,maxcol,TRUE);

	EndRxPos((STRPTR)opts[0]);

	return(RC_OK);
}

// INSERTFORMULA POS,NAME,TYPE/N/K,REQ/S
//			   0   1	2		3

ULONG rxInsertFormula(long *opts)
{
	if (opts[2] && !GetAppWindow(WDT_FORMEL))
	{
		fewftype = *(LONG *)opts[2];
		MakeFewFuncs();
	}
	if (opts[3])
		OpenAppWindow(WDT_FORMEL,TAG_END);
	else if (opts[1])
	{
		BeginRxPos(RXPOS_CELL,(STRPTR)opts[0]);
#ifdef __amigaos4__
		CreateTabGadget(rxpage, rxpage->pg_MarkCol, rxpage->pg_MarkRow, TRUE);
#endif
		insertFormel(rxpage,(STRPTR)opts[1]);
		EndRxPos((STRPTR)opts[0]);
	}
	return(RC_OK);
}

// IGNOREEVENT
//

ULONG
rxIgnoreEvent(long *opts)
{
	ignoreEvent = TRUE;
	return RC_OK;
}

// PRINT REQ/S
//	   0

ULONG
rxPrint(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_PRINTER, TAG_END);

	return RC_OK;
}

// DBNEW NAME,PARENT,NOREFS/S
//	   0	1	  2

ULONG
rxDbNew(long *opts)
{
	struct Database *db = NULL,*sdb;
	struct tableField *tf;
	struct coordPkt cp;
	struct tablePos tp;
	struct Field *fi;
	struct Mask *ma;
	struct Page *opg;
	ULONG  rc = RC_OK;
	char   t[142],*s;
	long   col,row,hei,i;

	if (opts[0])
		db = (APTR)FindTag(&rxpage->pg_Mappe->mp_Databases,(STRPTR)opts[0]);
	else
	{
		if (!GetRxTablePos(&tp))
			return RC_WARN;

		foreach (&rxpage->pg_Mappe->mp_Databases, sdb)
		{
			if (rxpage == sdb->db_Page && InTablePos(&tp,&sdb->db_TablePos))
				db = sdb;
		}
		if (!db)
		{
			if ((ma = GuessMask(rxpage)) != 0)
				db = (APTR)MyFindName(&rxpage->pg_Mappe->mp_Databases,ma->ma_Node.ln_Name);
		}
	}

	if (db)
	{
		col = i = db->db_TablePos.tp_Col;  hei = db->db_TablePos.tp_Height;  row = db->db_TablePos.tp_Row+hei;
		opg = rxpage;  rxpage = db->db_Page;
		tf = NULL;  s = NULL;
		while(!tf && i <= col+db->db_TablePos.tp_Width)	 /* MERKER: Datenbank leer?? */
			tf = GetTableField(rxpage,i++,row);
		if (tf && tf->tf_Row == row)
		{
			i = col+db->db_TablePos.tp_Width;
			for(;tf->tf_Node.mln_Succ && tf->tf_Col <= i && tf->tf_Row == row;tf = (APTR)tf->tf_Node.mln_Succ)
			{
				if (tf->tf_Text)
				{
					s = (STRPTR)~0L;
					break;
				}
			}
		}
		ma = NULL;
		if (rxpage->pg_Mappe->mp_Flags & MPF_SCRIPTS)
			ma = (APTR)MyFindName(&rxpage->pg_Mappe->mp_Masks, db->db_Node.ln_Name);
		if (!(ma && ma->ma_Page == db->db_Page))
		{
			setCoordPkt(rxpage,&cp,col,row+(s ? 1 : 0));
			CreateTabGadget(rxpage,cp.cp_Col,cp.cp_Row,FALSE);
		}
		if (s)	/* Datenbank enthält bereits einen Satz */
		{
			struct PasteNode *pn;

			SetMark(rxpage,col,row,db->db_TablePos.tp_Width,0);
			pn = CutCopyClip(rxpage,CCC_COPY | CCC_CELLS);
			if (!(rc = processIntCmd("CELL SHIFTDOWN")))
				PasteClip(rxpage,pn,0);
			SetMark(rxpage,-1,0,0,0);

			db->db_Current = hei+1;
			for(i = 0,fi = (APTR)db->db_Fields.mlh_Head;fi->fi_Node.ln_Succ;fi = (APTR)fi->fi_Node.ln_Succ,i++)
			{
				if ((tf = GetTableField(db->db_Page, col+i, row + 1)) != 0)
				{
					if (fi->fi_Node.ln_Type == FIT_COUNTER)
					{
						sprintf(t,"%lu",hei+2);
						SetTFText(db->db_Page,tf,t);
					}
					else
					{
						SetTFText(db->db_Page,tf,NULL);
						if (fi->fi_Node.ln_Type == FIT_REFERENCE && !opts[2])
						{
							sprintf(t,"DBNEW %s %s",fi->fi_Special,db->db_Node.ln_Name);
							processIntCmd(t);
						}
					}
				}
			}
			DrawTablePos(db->db_Page,col,row+1,i-1,0);
			if (cp.cp_Col != rxpage->pg_Gad.cp.cp_Col || cp.cp_Row != rxpage->pg_Gad.cp.cp_Row)
				CreateTabGadget(rxpage,cp.cp_Col,cp.cp_Row,TRUE);
		}
		rxpage = opg;

		if (opts[1] && (sdb = (APTR)FindTag(&rxpage->pg_Mappe->mp_Databases,(STRPTR)opts[1])))
		{
			for(i = 0,fi = (APTR)sdb->db_Fields.mlh_Head;fi->fi_Node.ln_Succ;fi = (APTR)fi->fi_Node.ln_Succ,i++)
			{
				if (fi->fi_Node.ln_Type == FIT_REFERENCE && !strcmp(fi->fi_Special,db->db_Node.ln_Name))
				{
					if ((tf = GetTableField(sdb->db_Page, sdb->db_TablePos.tp_Col + i, sdb->db_TablePos.tp_Row + sdb->db_Current)) != 0)
					{
						sprintf(t,"%lu",hei+2);
						if ((s = AllocPooled(pool, (tf->tf_Text ? strlen(tf->tf_Text) + 2 : 0) + strlen(t) + 2)) != 0)
						{
							strcpy(s,"#");
							if (tf->tf_Text)
							{
								strcat(s,tf->tf_Text);
								strcat(s,", ");
							}
							strcat(s,t);
							SetTFText(sdb->db_Page,tf,s);
							DrawTableField(sdb->db_Page,tf);
						}
					}
				}
			}
		}
		RecalcTableFields(rxpage);

		if (ma && !IsListEmpty((struct List *)&ma->ma_Fields))
			CreateTabGadget(ma->ma_Page,((struct MaskField *)ma->ma_Fields.mlh_Head)->mf_Col,((struct MaskField *)ma->ma_Fields.mlh_Head)->mf_Row,FALSE);
	}
	else
		rc = RC_WARN;
	return(rc);
}

// DBCURRENT NAME/A,NEXT/S,PREV/S,FIRST/S,LAST/S,PARENT,NUM/N/K
//		   0	  1	  2	  3	   4	  5	  6

ULONG rxDbCurrent(long *opts)
{
	struct Database *db,*pdb;
	long   *refs,i,j,cur;

	if ((db = (APTR)FindTag(&rxpage->pg_Mappe->mp_Databases, (STRPTR)opts[0])) != 0)
	{
		if (opts[6])
			j = *(long *)opts[6];
		cur = db->db_Current;
		if (opts[5] && (pdb = (APTR)FindTag(&rxpage->pg_Mappe->mp_Databases,(STRPTR)opts[5])))
		{
			if ((refs = GetDBReferences(db, pdb)) != 0)
			{
				for(i = 1;i <= *refs;i++)		   // aktuellen Eintrag aus Referenzen heraussuchen
					if (*(refs+i) == cur)
						break;
				if (opts[1])
					opts[6] ? i += j : i++;
				else if (opts[2])
					opts[6] ? i -= j : i--;
				else if (opts[3])
					i = 1;
				else if (opts[4])
					i = *refs;
				else if (opts[6])
					i = j;
				if (i < 1)
					i = 1;
				else if (i > *refs)
					i = *refs;
				i = *(refs+i);
				FreePooled(pool,refs,(*refs+1)*sizeof(long));
				if (i >= 0 && i <= db->db_TablePos.tp_Height)
					db->db_Current = i;
			}
		}
		else
		{
			UBYTE mode;

			if (opts[1] || opts[2])
				mode = DBC_REL;
			else
				mode = DBC_ABS;
			if (opts[1])
				i = opts[6] ? j : 1;
			else if (opts[2])
				i = opts[6] ? -j : -1;
			else if (opts[3])
				i = 0;
			else if (opts[4])
				i = db->db_TablePos.tp_Height;
			else if (opts[6])
				i = j-1;

			SetDBCurrent(db,mode,i);
		}
		if (cur != db->db_Current)
			RecalcTableFields(db->db_Page);
	}
	return(RC_OK);
}

#ifdef __amigaos4__
// DBSETFILTER DBNAME,FILTER/K
//		  0	 	1		2	   
ULONG
rxDbSetFilter(long *opts)
{
    struct Database *db;
	struct Node *fnode = NULL;
	struct Filter *ofi;

	if (opts[0] && !(db = (struct Database *)FindTag(&rxpage->pg_Mappe->mp_Databases,(STRPTR)opts[0])))
		return RC_WARN;
	if (opts[1] && IsMinListEmpty(&db->db_Filters))
		return RC_WARN;
	if (db->db_Filter && db->db_Filter->fi_Type == FIT_SEARCH) //No Filterset if search is active
		return RC_WARN;
    if(fnode = FindTag(&db->db_Filters, (STRPTR)opts[1]))
    {
		if (!fnode->ln_Type) 
		{
			foreach (&db->db_Filters, ofi)  /* remove old active */
				if(ofi->fi_Node.ln_Type)
				{
					ofi->fi_Node.ln_Type = 0;
					db->db_Filter = NULL;
					UpdateDBCurrent(db, db->db_Current);
					break;
				}
			fnode->ln_Type = 1;
			MakeFilter(db, (struct Filter *)fnode);
			db->db_Filter = (struct Filter *)fnode;
		}
		else
		{
			fnode->ln_Type = 0;
			db->db_Filter = NULL;
		}
		UpdateDBCurrent(db, db->db_Current);
		RecalcTableFields(rxpage);
	}
	return RC_OK;
}
#endif

// DBSEARCH NAME,FILTER/K,INPUT/S,START/S,STOP/S,TOGGLE/S
//		  0	1		2	   3	   4	  5

ULONG
rxDbSearch(long *opts)
{
	struct Database *db;
	struct Filter *fi;
	struct Mask *ma = NULL;

	if (opts[0] && !(db = (struct Database *)FindTag(&rxpage->pg_Mappe->mp_Databases,(STRPTR)opts[0])))
		return RC_WARN;
	if (!opts[0] && !(ma = GuessMask(rxpage)))
		return RC_WARN;

	if (ma)
		db = (struct Database *)MyFindName(&rxpage->pg_Mappe->mp_Databases, ma->ma_Node.ln_Name);

	if (opts[1])
	{
		if (!(fi = (struct Filter *)FindTag(&db->db_Filters,(STRPTR)opts[1])))
		{
			if ((fi = AllocPooled(pool, sizeof(struct Filter))) != 0)
			{
				fi->fi_Filter = AllocString((STRPTR)opts[1]);
				fi->fi_Type = FIT_SEARCH;
				if (!(fi->fi_Root = CreateTree(db->db_Page,fi->fi_Filter)))
				{
					FreePooled(pool, fi, sizeof(struct Filter));
					fi = NULL;
				}
			}
		}
		if (fi)
			SetFilter(db, fi);
		else
			ErrorRequest(GetString(&gLocaleInfo, MSG_SET_FILTER_ERR));
	}
	else if (ma || (ma = (struct Mask *)FindTag(&rxpage->pg_Mappe->mp_Masks,(STRPTR)opts[0])))
	{
		if (opts[2] || opts[5] && !ma->ma_Node.ln_Type && (!db->db_Filter || db->db_Filter->fi_Type != FIT_SEARCH))
		{
			struct MaskField *mf;
			struct tableField *tf;

			if (db->db_Filter && db->db_Filter->fi_Type == FIT_SEARCH)
			{
				FreeFilter(db->db_Filter);   /* alte Suche abbrechen */
				db->db_Filter = NULL;
			}
			ma->ma_Node.ln_Type = 1;	   /* Maske in Suchmodus setzen */
			foreach(&ma->ma_Fields,mf)	 /* Felder löschen */
			{
				if ((tf = AllocTableField(ma->ma_Page, mf->mf_Col, mf->mf_Row)) != 0)
				{
					if (tf->tf_Text)
					{
						SetTFText(ma->ma_Page,tf,NULL);
						DrawTableField(ma->ma_Page,tf);
					}
				}
				if (ma->ma_Fields.mlh_Head == (APTR)mf)
					CreateTabGadget(ma->ma_Page,mf->mf_Col,mf->mf_Row,TRUE);
			}
		}
		else if ((!db->db_Filter || db->db_Filter->fi_Type != FIT_SEARCH) && ma->ma_Node.ln_Type && (opts[3] || opts[5]))
		{
			MakeSearchFilter(db,ma);
			RecalcTableFields(rxpage);
		}
		else if (db->db_Filter && db->db_Filter->fi_Type == FIT_SEARCH && opts[4] || opts[5])
		{
			struct Field *fi;

			SetFilter(db,NULL);
			foreach(&db->db_Fields,fi)
			{
				if (fi->fi_Node.ln_Type == FIT_REFERENCE && (db = (APTR)MyFindName(&rxpage->pg_Mappe->mp_Databases, fi->fi_Special)))
				{
					if (db->db_Filter && db->db_Filter->fi_Type == FIT_SEARCH)
						SetFilter(db,NULL);
				}
			}
			RecalcTableFields(rxpage);
		}
	}
	return RC_OK;
}

// DATABASE REQ/S
//		  0

ULONG
rxDatabase(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_DATABASE, TAG_END);

	return RC_OK;
}

// MASK REQ/S
//	  0

ULONG
rxMask(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_MASK, TAG_END);

	return RC_OK;
}

// INDEX REQ/S
//	   0

ULONG
rxIndex(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_INDEX, TAG_END);

	return RC_OK;
}

// FILTER REQ/S
//		0

ULONG
rxFilter(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_FILTER,TAG_END);

	return RC_OK;
}

// NAME NAME,SET/K,TYPE,DELETE/S,REQ/S,GLOBAL/S
//	      0	  1	     2	   3	  4	      5

ULONG
rxName(long *opts)
{
	struct MinList *list;
	struct Mappe *mp;
	struct Name *nm;

	if (opts[4])
	{
		OpenAppWindow(WDT_PREFNAMES,WA_Data,rxpage && !opts[5] ? rxpage->pg_Mappe : NULL,TAG_END);
		return RC_OK;
	}

	if (!opts[0])
	{
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_NAME_ERR));
		return RC_WARN;
	}

	mp = opts[5] ? NULL : rxpage->pg_Mappe;
	list = &GetLocalPrefs(mp)->pr_Names;
	nm = (struct Name *)FindTag(list,(STRPTR)opts[0]);

	if (opts[1] || opts[2] && nm)   // SET or TYPE
	{
		UBYTE type = NMT_NONE;

		if (opts[2])
		{
			if (!stricmp((STRPTR)opts[2],"cell") || !stricmp((STRPTR)opts[2],"c"))
				type = NMT_CELL;
			else if (!stricmp((STRPTR)opts[2],"search") || !stricmp((STRPTR)opts[2],"s"))
				type = NMT_SEARCH;
			else if (stricmp((STRPTR)opts[2],"none") && stricmp((STRPTR)opts[2],"n"))
				ErrorRequest(GetString(&gLocaleInfo, MSG_NAME_TYPE_MISSING_ERR));
		}
		if (nm)
		{
			if (opts[2])
				nm->nm_Node.ln_Type = type;
			if (opts[1])
				SetNameContent(nm, AllocString((STRPTR)opts[1]));

			RefreshName(nm);
		}
		else
		{
			if (!IsValidName(NULL, (STRPTR)opts[0]))
				return RC_WARN;

			if (!opts[5])
				AddPrefsModuleToLocalPrefs(mp,WDT_PREFNAMES);

			AddName(list,(STRPTR)opts[0],(STRPTR)opts[1],type,rxpage);
		}
		RefreshPrefsModule(GetLocalPrefs(mp),NULL,WDT_PREFNAMES);
	}
	else if (opts[3] && nm)
	{
		MyRemove(nm);
		FreeName(nm);
		RefreshPrefsModule(GetLocalPrefs(mp),NULL,WDT_PREFNAMES);
	}
	else
	{
		ErrorRequest(GetString(&gLocaleInfo, MSG_BAD_ARGUMENTS_ERR));
		return RC_WARN;
	}

	return RC_OK;
}

// CELLNAME POS,SET/K,DELETE/S,REQ/S
//		      0   1	    2		3

ULONG rxCellName(long *opts)
{
	if (opts[3])
		OpenAppWindow(WDT_SETNAME,TAG_END);

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	if (opts[1])
	{
		char t[16];

		if (rxpage->pg_MarkCol != -1)
			TablePos2String(rxpage,(struct tablePos *)&rxpage->pg_MarkCol,t);
		else
			strcpy(t,Coord2String(rxpage->pg_Gad.cp.cp_Col,rxpage->pg_Gad.cp.cp_Row));
#ifdef __amigaos4__
		opts[0] = opts[1];
		opts[1] = (long)t;
		opts[2] = (long)"c";
		opts[3] = opts[4] = opts[5] = FALSE;
		rxName(opts);
#else
		RexxCall(rxName,opts[1],t,"c",FALSE,FALSE,FALSE);
#endif
	}
	else
	{
		struct Name *nm;

		foreach(&rxpage->pg_Mappe->mp_Prefs.pr_Names,nm)
		{
			if (nm->nm_Node.ln_Type == NMT_CELL)
			{
				if (rxpage->pg_MarkCol != -1 && !memcmp(&rxpage->pg_MarkCol,&nm->nm_TablePos,sizeof(struct tablePos)) || rxpage->pg_MarkCol == -1 && !nm->nm_TablePos.tp_Width && !nm->nm_TablePos.tp_Height && nm->nm_TablePos.tp_Col == rxpage->pg_Gad.cp.cp_Col && nm->nm_TablePos.tp_Row == rxpage->pg_Gad.cp.cp_Row)
#ifdef __amigaos4__
				{
					opts[0] = (long)nm->nm_Node.ln_Name;
					opts[1] = (long)NULL;
					opts[2] = (long)"c";
					opts[3] = opts[4] = (long)FALSE;
					rxName(opts);
				}
#else
					RexxCall(rxName,nm->nm_Node.ln_Name,NULL,"c",FALSE,FALSE);
#endif
			}
		}
	}
	EndRxPos((STRPTR)opts[0]);
	DisplayTablePos(rxpage);

	return(RC_OK);
}

// NOTES REQ/S
//	   0

ULONG rxNotes(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_NOTES,TAG_END);
	return(RC_OK);
}

// SEARCH REQ/S,NEXT/S
//		0	 1

ULONG rxSearch(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_FIND,TAG_END);
	else if (opts[1])
		SearchReplace(rxpage,searchMode & ~SM_REPLACE);
	return(RC_OK);
}

// REPLACE REQ/S,NEXT/S
//		 0	 1

ULONG rxReplace(long *opts)
{
	if (opts[0])
		OpenAppWindow(WDT_REPLACE,TAG_END);
	else if (opts[1])
		SearchReplace(rxpage,searchMode | SM_REPLACE);
	return(RC_OK);
}

// NEXT

ULONG rxNext(long *opts)
{
	struct SearchNode *sn;
	long   found = 0;

	for(sn = (APTR)search.mlh_Head;sn->sn_Node.ln_Succ;sn = (APTR)sn->sn_Node.ln_Succ)
		if (sn->sn_Node.ln_Type) found++;
	if (found == 1 && (sn = (APTR)search.mlh_Head)->sn_Node.ln_Type && !sn->sn_Text)
	{
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_SEARCH_PATTERN_YET_ERR));
		return(RC_WARN);
	}
	SearchReplace(rxpage,searchMode | SM_CELL);
	return(RC_OK);
}

#define REQUEST_STRING 0
#define REQUEST_NUMBER 1

STRPTR RexxRequest(STRPTR prompt,STRPTR deftext,UBYTE mode)
{
	struct IntuiMessage *msg;
	struct Gadget *gadlist,*gad,*sgad;
	struct Window *win;
	struct MinList list;
	STRPTR shortcuts = "eoa",result = NULL;
	long   width,height,cols,id;
	BYTE   done = FALSE;

	width = scr->Width >> 1;
	MyNewList(&list);
	cols = WordWrapText((struct List *)&list,prompt,width-lborder-rborder);
	height = barheight+bborder+fontheight*(2+cols)+20;

	if (!(gad = CreateContext(&gadlist)))
		return(NULL);

	ngad.ng_TextAttr = scr->Font;
	ngad.ng_VisualInfo = vi;
	ngad.ng_TopEdge = barheight+3+(fontheight+1)*cols+(cols ? 2 : 0);
	ngad.ng_GadgetText = "_Eingabe:";
	ngad.ng_LeftEdge = lborder+TLn("Eingabe:")+8;
	ngad.ng_Width = width-rborder-ngad.ng_LeftEdge;
	ngad.ng_Height = fontheight+4;
	ngad.ng_GadgetID = 1;				   /* 1 */
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_UserData = NULL;
	sgad = CreateGadget(mode == REQUEST_NUMBER ? NUMBER_KIND : STRING_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge += fontheight+7;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+20;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OK_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = shortcuts[1];
	gad = CreateGadget(BUTTON_KIND,sgad,&ngad,GT_Underscore,'_',TAG_END);

	ngad.ng_LeftEdge = width-rborder-ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID = shortcuts[2];
	/*gad =*/ CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);

	if (!(win = OpenWindowTags(NULL,WA_Left,   (scr->Width-width) >> 1,
																	WA_Top,	(scr->Height-height) >> 1,
																	WA_Width,  width,
																	WA_Height, height,
																	WA_Title,  GetString(&gLocaleInfo, MSG_IGNITION_REQUEST_TITLE),
																	WA_Flags,  WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET | WFLG_ACTIVATE,
																	WA_IDCMP,  IDCMP_RAWKEY | IDCMP_VANILLAKEY | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW,
																	WA_Gadgets,gadlist,
																	WA_PubScreen,scr,
#ifdef __amigaos4__
														            WA_ScreenTitle,	IgnTitle,
#endif
																	TAG_END)))
		return(NULL);

	{
		struct Node *ln;

		cols = win->BorderTop+3;

		foreach(&list,ln)
		{
			itext.IText = ln->ln_Name;
			PrintIText(win->RPort,&itext,(win->Width-IntuiTextLength(&itext)) >> 1,cols);
			cols += fontheight;
		}
	}
	while(!done)
	{
		WaitPort(win->UserPort);

		if (!(msg = GT_GetIMsg(win->UserPort)))
			continue;

		switch(msg->Class)
		{
			case IDCMP_GADGETUP:
				id = ((struct Gadget *)msg->IAddress)->GadgetID;
			case IDCMP_VANILLAKEY:
				if (msg->Class == IDCMP_VANILLAKEY)
					id = msg->Code;

				if (id == shortcuts[0])
					ActivateGadget(sgad,win,NULL);
				else if (id == shortcuts[1] || id == shortcuts[2])
				{
					if (id == shortcuts[1] && GT_GetGadgetAttrs(sgad,win,NULL,GTST_String,&result,TAG_END))
						result = AllocString(result);
					done = TRUE;
				}
				break;
			case IDCMP_CLOSEWINDOW:
				done = TRUE;
				break;
		}
		GT_ReplyIMsg(msg);
	}
	CloseWindow(win);
	FreeGadgets(gadlist);

	return(result);
}

// REQUESTNUMBER PROMPT,DEFAULT/K/N,VAR
//			   0	  1		   2

ULONG rxRequestNumber(long *opts)
{
	STRPTR t;

	SetBusy(TRUE,BT_APPLICATION);

	t = RexxRequest((STRPTR)opts[0],(APTR)opts[1],REQUEST_NUMBER);
	SetRexxResult((STRPTR)opts[2],t);
	FreeString(t);

	SetBusy(FALSE,BT_APPLICATION);

	return(RC_OK);
}

// REQUESTSTRING PROMPT,DEFAULT/K,VAR
//			   0	  1		 2

ULONG
rxRequestString(long *opts)
{
	STRPTR t;

	SetBusy(TRUE,BT_APPLICATION);

	t = RexxRequest((STRPTR)opts[0],(STRPTR)opts[1],REQUEST_STRING);
	SetRexxResult((STRPTR)opts[2],t);
	FreeString(t);

	SetBusy(FALSE,BT_APPLICATION);

	return(RC_OK);
}

// LOADPICTURE NAME/K,REQ/S
//			 0	  1

ULONG
rxLoadPicture(long *opts)
{
	char name[256];

#ifdef __amigaos4__
	return RC_OK;
#endif
	name[0] = 0;
	if (opts[0])
		strcpy(name,(STRPTR)opts[0]);
	if (opts[1])
	{
		if (AslRequestTags(fileReq,
#ifdef __amigaos4__
								   ASLFR_Window,		 rxpage->pg_Window,
#else
								   ASLFR_Window,		 scr->FirstWindow,
#endif
								   ASLFR_TitleText,	 GetString(&gLocaleInfo, MSG_LOAD_PICTURE_TITLE),
								   ASLFR_InitialDrawer, graphicpath,
								   ASLFR_InitialFile,   "",
								   ASLFR_DoSaveMode,	FALSE,
								   ASLFR_InitialPattern,"#?",
								   ASLFR_DoPatterns,	FALSE,
								   ASLFR_DrawersOnly,   FALSE,
								   TAG_END))
		{
			strcpy(name,fileReq->fr_Drawer);
			AddPart(name,fileReq->fr_File,255);
		}
	}
	if (*name)
	{
		/*CreatePicture(rxpage,name);*/
		DrawTable(rxpage->pg_Window);
	}
	return RC_OK;
}

// OBJECTINFO NAME
//			0

ULONG
rxObjectInfo(long *opts)
{
	struct gObject *go;

	if (opts[0])
	{
		if ((go = (struct gObject *)FindCommand((struct MinList *)&rxpage->pg_gObjects, (STRPTR)opts[0])) != 0)
			OpenGObjectWindow(go);
		else
			return RC_WARN;
	}
	else
	{
		for(go = (APTR)rxpage->pg_gObjects.mlh_Head;go->go_Node.ln_Succ;go = (APTR)go->go_Node.ln_Succ)
		{
			if (go->go_Flags & GOF_SELECTED)
				OpenGObjectWindow(go);
		}
	}
	return RC_OK;
}

// DUBOBJECT NAME
//		   0

ULONG
rxDupObject(long *opts)
{
	struct UndoNode *un;

	if (opts[0])
	{
		struct gObject *go;

		foreach (&rxpage->pg_gObjects, go)
		{
			if (!stricmp((STRPTR)opts[0], go->go_Node.ln_Name ? go->go_Node.ln_Name : ""))
			{
				if (!(un = CreateUndo(rxpage, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_DUPLICATE_OBJECTS_UNDO))))
					return RC_WARN;

				un->un_Type = UNT_ADD_OBJECTS;
				DuplicateGGroup(rxpage, OBJECTGROUP(go), un);
				break;
			}
		}
	}
	else
	{
		struct gGroup *gg;

		foreach (&rxpage->pg_gGroups, gg)
		{
			if (gg->gg_Flags & GOF_SELECTED)
			{
				if (!(un = CreateUndo(rxpage, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_DUPLICATE_OBJECTS_UNDO))))
					return RC_WARN;

				un->un_Type = UNT_ADD_OBJECTS;
				DuplicateGGroup(rxpage, gg, un);
			}
		}
	}

	return RC_OK;
}

// INSERTOBJECT TYPE,REQ/S
//			  0	1

ULONG
rxInsertObject(long *opts)
{
	if (opts[1])
		OpenAppWindow(WDT_GCLASSES, TAG_END);

	return RC_OK;
}


void
ObjectError(STRPTR named)
{
	if (named)
		ErrorRequest(GetString(&gLocaleInfo, MSG_NAMED_OBJECT_NOT_FOUND_ERR),named);
	else
		ErrorRequest(GetString(&gLocaleInfo, MSG_NO_OBJECT_SELECTED_ERR));
}

// REFOBJECT NAME,REQ/S
//		   0	1

ULONG
rxRefObject(long *opts)
{
	STRPTR name = (STRPTR)opts[0];
	struct gObject *go;
	struct gClass *gc;
	ULONG  found = 0;

	if (!(gc = FindGClass("embedded")))
	{
		ErrorRequest(GetString(&gLocaleInfo, MSG_REFERENCE_CLASS_NOT_FOUND_ERR));
		return RC_WARN;
	}
	foreach(&rxpage->pg_gObjects, go)
	{
		if (name && !zstricmp(go->go_Node.ln_Name, name) || !name && go->go_Flags & GOF_SELECTED)
		{
			gMakeRefObject(rxpage, gc, go, NULL);
			found++;
		}
	}
	if (!found)
	{
		ObjectError(name);
		return RC_WARN;
	}
	return RC_OK;
}

// ObjectOrder - support function for OBJECTTOFRONT and OBJECTTOBACK

ULONG
ObjectOrder(struct Page *page,STRPTR name,UBYTE type)
{
	struct gObject *go;
	ULONG  found = 0;

	foreach (&page->pg_gObjects, go)
	{
		if ((name && !zstricmp(go->go_Node.ln_Name,name)) || (!name && go->go_Flags & GOF_SELECTED))
		{
			ChangeGObjectOrder(page, go, type);
			found++;
		}
	}
	if (!found)
	{
		ObjectError(name);
		return RC_WARN;
	}
	return RC_OK;
}


// OBJECTTOFRONT NAME,MOST/S
//			   0	1

ULONG
rxObjectToFront(long *opts)
{
	return ObjectOrder(rxpage,(STRPTR)opts[0],opts[1] ? GO_FRONTMOST : GO_TOFRONT);
}

// OBJECTTOBACK NAME,MOST/S
//			  0	1

ULONG
rxObjectToBack(long *opts)
{
	return ObjectOrder(rxpage,(STRPTR)opts[0],opts[1] ? GO_BACKMOST : GO_TOBACK);
}

// GROUP NAME
//	   0

ULONG
rxGroup(long *opts)
{
	/*if (GroupGFrames(rxpage,(STRPTR)opts[0]))*/
		return(RC_OK);
	/*return(RC_WARN);*/
}

// UNGROUP NAME
//		 0

ULONG
rxUngroup(long *opts)
{
	/*if (UngroupGFrames(rxpage,(STRPTR)opts[0]))*/
		return RC_OK;
	/*return(RC_WARN);*/
}

// CLOSE NAME,QUIET
//	   0	1

ULONG
rxClose(long *opts)
{
	struct Mappe *mp;
	struct Window *win;

	if (opts[0])
	{
		if (!(mp = (struct Mappe *)FindTag(&gProjects,(STRPTR)opts[0])))
			return RC_WARN;
	}
	else if (rxpage)
		mp = rxpage->pg_Mappe;
	else
		return RC_OK;

	if (opts[1])
		mp->mp_Modified = 0;

	win = mp->mp_Window;
	if (DisposeProject(mp))
		CloseAppWindow(win, TRUE);

	return RC_OK;
}

// LOAD NAME,REQ/S,NEW/S,TYPE/K,TYPEREQ/S,OLDPATH/S
//	     0	  1	   2	 3	  		4		 5     

ULONG rxLoad(long *opts)
{
	struct Window *filewin = NULL;
	struct IOType *type = NULL;
	struct Page *page;
	struct Mappe *mp;
	char   dest[256];
	ULONG  rc;
	char   suffixes[257];

	/*D(bug("LOAD name: %s, req: %ld, new: %ld\n",opts[0],opts[1],opts[2]));*/
	page = rxpage;
	if ((mp = NewProject()) != 0) {
		if (opts[5] && page) {
			FreeString(mp->mp_Path);
			mp->mp_Path = AllocString(page->pg_Mappe->mp_Path);
		}
		if (opts[0]) {
#ifdef __amigaos4__
			if(!rxpage->pg_Window)				//Wenn es nicht die Tabelle 0 ist
			{									//dann den page-Zeiger auf Tabelle 0 bestimmen
				if (!(page = (struct Page *)FindTag(&mp->mp_Pages, "0")) && !(page = (struct Page *)FindListNumber(&mp->mp_Pages,atol("0"))))
					return(RC_WARN);
 				else
					SetMainPage(page);			//Hauptseite auf Tabelle 0 setzem
			}
#endif
			strcpy(dest, (STRPTR)opts[0]);
			SetMapName(mp, (STRPTR)FilePart(dest));
			dest[(ULONG)((char *)PathPart(dest) - dest)] = 0;
			if (!opts[5] || *dest)
				FreeString(mp->mp_Path);
			if (*dest)
				mp->mp_Path = AllocString(dest);
		}

		if (opts[3]) {
			if ((type = (APTR)FindTag(&iotypes, (STRPTR)opts[3])) == NULL) {
				struct IOType *io;

				foreach(&iotypes, io) {
					if (io->io_Suffix && !stricmp(io->io_Suffix, (STRPTR)opts[3])) {
						type = io;
						break;
					}
				}

				if (type == NULL && isdigit(*(STRPTR)opts[3]))
					type = (APTR)FindListNumber(&iotypes, atol((STRPTR)opts[3]));
#ifdef __amigaos4__
				if(!type)  //if io-modul is not found
				{
				    ErrorRequest(GetString(&gLocaleInfo, MSG_ADD_ON_NOT_FOUND_ERR),opts[3], "Load");
					return RC_WARN;
				}
#endif
			}
		}

		if (opts[1]) {
			if (opts[4])
				filewin = OpenAppWindow(WDT_FILETYPE, WA_ExtData, TRUE, TAG_END);

#ifdef __amigaos4__
			struct IOType *io;

			Strlcpy(suffixes, "#?.(", 256);
			if(opts[3])
			{
				    Strlcat(suffixes, type->io_Suffix, 256);
				    Strlcat(suffixes, "|",256);
			}
			else
			{
				foreach(&iotypes, io) {
				    Strlcat(suffixes, io->io_Suffix, 256);
				    Strlcat(suffixes, "|",256);
					}
			}
			suffixes[Strlen(suffixes) - 1] = ')';
#endif
			rc = AslRequestTags(fileReq,
#ifdef __amigaos4__
					ASLFR_Window,		 page->pg_Window,
#else
					ASLFR_Window,		 scr->FirstWindow,
#endif
					ASLFR_TitleText,	 GetString(&gLocaleInfo, MSG_LOAD_DOCUMENT_TITLE),
					ASLFR_InitialDrawer, mp->mp_Path ? mp->mp_Path : projpath,
					ASLFR_InitialFile,   mp->mp_Node.ln_Name,
#ifdef __amigaos4__
					ASLFR_InitialPattern,"#?",
					ASLFR_AcceptPattern, suffixes,
#else
					ASLFR_InitialPattern,"#?",
					ASLFR_AcceptPattern, "#?.(igs|tcd|csv|txt)",
#endif
					ASLFR_DoSaveMode,	FALSE,
					ASLFR_DoPatterns,	FALSE,
					ASLFR_DrawersOnly,   FALSE,
					ASLFR_IntuiMsgFunc,  &fileHook,
					TAG_END);
			if (filewin) {
				type = ((struct winData *)filewin->UserData)->wd_ExtData[0];
				CloseAppWindow(filewin, TRUE);
			}

			if (rc) {
				FreeString(mp->mp_Path);
				mp->mp_Path = AllocString(fileReq->fr_Drawer);
				SetMapName(mp, fileReq->fr_File);
			} else {
				DisposeProject(mp);
				SetMainPage(page);
				return 0;
			}
		}

		if (LoadProject(mp, type)) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_PROJECT_ERR), mp->mp_Node.ln_Name);
			return RC_WARN;
		}

		if (!opts[2] && page) {
			struct Window *win;

			if ((win = page->pg_Window) != 0) {
#ifdef __amigaos4__
				if(DisposeProject(page->pg_Mappe) == false)
					return RC_OK;
#endif
				if (rxpage->pg_Mappe->mp_Flags & MPF_SAVEWINPOS) {
					struct IBox box;

					CopyMem(&mp->mp_WindowBox, &box, sizeof(struct IBox));
					NormalizeWindowBox(&box);

					/***** WDT_PROJECT bekommt einen IDCMP_NEWSIZE *****/
					{
						struct winData *wd = (struct winData *)win->UserData;

						RemoveGadgets(&win->FirstGadget, FALSE);
						FreeWinIconObjs(win, wd);
						RemoveGList(win, wd->wd_Gadgets, CountGadToolGads(win));
						FreeGadgets(wd->wd_Gadgets);
						wd->wd_Gadgets = NULL;

						page->pg_Window = NULL;
						rxpage->pg_Window = rxpage->pg_Mappe->mp_Window = win;
						wd->wd_Data = rxpage;
					}
					ChangeWindowBox(win, box.Left, box.Top, box.Width, box.Height);
					SetWindowTitles(win, GetPageTitle(rxpage), (UBYTE *)~0L);
				} else
					UpdateProjPage(win, rxpage);

				// update all existing prefs modules
				RefreshPrefsModules(&mp->mp_Prefs, ~(PRF_NAMES | PRF_FORMAT | PRF_CMDS));
			}
#ifdef __amigaos4__
			else
				DisposeProject(page->pg_Mappe);
			RefreshProjWindows(TRUE); //Beseitigt Fehler in Tabellenfenster beim Laden zB nach laden von "Rechnen mit Typen.igs"
#else
			DisposeProject(page->pg_Mappe);
#endif
		} else
			OpenProjWindow(rxpage, TAG_END);
		handleEvent(rxpage, EVT_START, 0, 0);
		return RC_OK;
	}
	return RC_WARN;
}

// SAVE NAME,REQ/S,TYPEREQ/S TYPE/K TYPEPREFS/K
//	     0	  1	 	 2			3       4

uint32
rxSave(long *opts)
{
	struct Window *filewin = NULL;
#ifdef __amigaos4__
	struct Window *tmpwin = NULL; //Used, when an ending the program no window is open
#endif
	struct Mappe *mp = rxpage->pg_Mappe;
	char   dest[256];
	bool   confirmOverwrite = false;
	struct IOType *type = mp->mp_FileType;
	long   rc;
	char   suffixes[256];
		
	// Change name if the argument 1 is present

	if (opts[0]) {
		FreeString(mp->mp_Path);
		strcpy(dest, (STRPTR)opts[0]);
		SetMapName(mp, (STRPTR)FilePart(dest));
		if (PathPart(dest) != (STRPTR)dest) {
			dest[(ULONG)((char *)PathPart(dest)-dest)] = 0;
			mp->mp_Path = AllocString(dest);
		} else
			mp->mp_Path = AllocString(projpath);
	}

	if (opts[3]) {
		if ((type = (APTR)FindTag(&iotypes, (STRPTR)opts[3])) == NULL) {
			struct IOType *io;

			foreach(&iotypes, io) {
				if (io->io_Suffix && !stricmp(io->io_Suffix, (STRPTR)opts[3])) {
					type = io;
					break;
				}
			}

			if (type == NULL && isdigit(*(STRPTR)opts[3]))
				type = (APTR)FindListNumber(&iotypes, atol((STRPTR)opts[3]));
#ifdef __amigaos4__
			if(!type)  //if io-modul is not found
				{
				    ErrorRequest(GetString(&gLocaleInfo, MSG_ADD_ON_NOT_FOUND_ERR),opts[3], "Save");
					return RC_WARN;
				}
#endif
		}
	}

	if (opts[1] || (mp->mp_Flags & MPF_UNNAMED) != 0 || (type != NULL && (type->io_Flags & IOF_WRITEABLE) == 0)) {
		if (opts[2]) 
		{
			filewin = OpenAppWindow(WDT_FILETYPE, TAG_END);
			mp->mp_Flags &= ~MPF_WARNED_IOTYPE;
		}
#ifdef __amigaos4__
		if (opts[4]) 
		{
            InitIOType(type);
            if(type->io_OpenPrefsGUI)
            {
	        	type->io_OpenPrefsGUI(scr);
		        type->io_ClosePrefsGUI();
	        }
		}
#endif
		confirmOverwrite = TRUE;

#ifdef __amigaos4__
		struct IOType *io;

		Strlcpy(suffixes, "#?.(", 256);
		if(opts[3])
		{
			    Strlcat(suffixes, type->io_Suffix, 256);
			    Strlcat(suffixes, "|",256);
		}
		else
		{
			foreach(&iotypes, io) {
			    Strlcat(suffixes, io->io_Suffix, 256);
			    Strlcat(suffixes, "|",256);
				}
		}
		suffixes[Strlen(suffixes) - 1] = ')';

		if (!CountAppWindows())			//no more windows open, create tmpwindow
			rxpage->pg_Window = tmpwin = OpenWindowTags(NULL,WA_Top,0,WA_Width,1,WA_Height,1,WA_Flags,WFLG_BACKDROP | WFLG_BORDERLESS,WA_PubScreen,scr,TAG_END);
#endif
		rc = AslRequestTags(fileReq,
#ifdef __amigaos4__
				ASLFR_Window,		 	rxpage->pg_Window,
#else
				ASLFR_Window,		 	scr->FirstWindow,
#endif
				ASLFR_TitleText,		GetString(&gLocaleInfo, MSG_SAVE_DOCUMENT_TITLE),
				ASLFR_InitialDrawer,	mp->mp_Path ? mp->mp_Path : projpath,
				ASLFR_InitialFile,		mp->mp_Node.ln_Name,
#ifdef __amigaos4__
				ASLFR_AcceptPattern, suffixes,
#else
				ASLFR_InitialPattern,"#?",
#endif
				ASLFR_DoSaveMode,		TRUE,
				ASLFR_DoPatterns,		FALSE,
				ASLFR_DrawersOnly,		FALSE,
				ASLFR_IntuiMsgFunc,		&fileHook,
				TAG_END);

		if (filewin) {
			type = ((struct winData *)filewin->UserData)->wd_ExtData[0];
			CloseAppWindow(filewin, TRUE);
		}

#ifdef __amigaos4__
		if (tmpwin)
			CloseWindow(tmpwin);
#endif
		if (rc) {
			FreeString(mp->mp_Path);
			mp->mp_Path = AllocString(fileReq->fr_Drawer);
			SetMapName(mp, fileReq->fr_File);
		} else
			return 0;
	}

	if (!opts[0] && !opts[1] && !opts[2]) {
		// just save - if the current file type is no default type
		// warn about missing features
		
		if (type != NULL && (type->io_Flags & IOF_NODEFAULT) != 0
			&& (mp->mp_Flags & MPF_WARNED_IOTYPE) == 0
			&& (prefs.pr_File->pf_Flags & PFF_WARN_IOTYPE) != 0) {
			int rc = DoRequest(GetString(&gLocaleInfo, MSG_IOTYPE_NO_DEFAULT_REQ),
						GetString(&gLocaleInfo, MSG_YES_NO_REQ), type->io_Node.ln_Name);
			if (rc == 0)
				return 0;
			else
				mp->mp_Flags |= MPF_WARNED_IOTYPE;
		}
	}

	if ((rc = SaveProject(mp, type, confirmOverwrite)) != 0) {
		if (!confirmOverwrite || rc > 0)
			ErrorRequest(GetString(&gLocaleInfo, MSG_SAVE_PROJECT_ERR));
	} else {
		struct Page *page;

		mp->mp_Flags &= ~MPF_UNNAMED;
		UpdateMapTitle(mp);

		foreach(&mp->mp_Pages, page)
			page->pg_Modified = 0;
		mp->mp_Modified = FALSE;
	}

#ifdef __amigaos4__
	if (mp->mp_Window && CountAppWindows())
#else
	if (mp->mp_Window)
#endif
		SetWindowTitles(mp->mp_Window,mp->mp_Title+(mp->mp_Window->Flags & WFLG_WINDOWACTIVE ? 0 : 3),(STRPTR)~0L);

	return 0;
}

// PREFS DISPLAY/S,FILE/S,SCREEN/S,KEYS/S,FORMAT/S,PRINTER/S,COLORS/S,TABLE/S,ICON/S,CMDS/S,MENU/S,SYSTEM/S,NAMES/S,CONTEXTMENU/S,REQ/S,LOAD/S,SAVE/S,NAME/K,ADD/S,KEEPOLD/S,GLOBAL/S
//	   0		 1	  2		3	  4		5		 6		7	   8	  9	  10	 11	   12	  13			14	15	 16	 17	 18	19		20

#define PREFS_REQ 14
#define PREFS_LOAD 15
#define PREFS_SAVE 16
#define PREFS_NAME 17
#define PREFS_ADD 18
#define PREFS_KEEP 19
#define PREFS_GLOBAL 20

ULONG
rxPrefs(long *opts)
{
	long flags = 0,i;

	for(i = 0;i < PREFS_REQ;i++)
		flags |= (opts[i] & 1) << i;

	if (opts[PREFS_LOAD])
		flags |= (opts[PREFS_ADD] ? PRF_ADDCONTENTS : 0) | (opts[PREFS_KEEP] ? PRF_KEEPOLDCONTENTS : 0);

	if (opts[PREFS_REQ]) {
		struct Mappe *mp = NULL;
		long type;

		if (!opts[PREFS_GLOBAL] && rxpage)
			mp = rxpage->pg_Mappe;

		for (i = 0x10000; i; i >>= 1) {
			if ((flags & i) == 0 || (type = GetPrefsModuleType(i)) == 0)
				continue;

			OpenAppWindow(type, WA_Data, mp, TAG_END);
		}

		if (!(flags & 0xfff) && !opts[PREFS_LOAD] && !opts[PREFS_SAVE])
			OpenAppWindow(WDT_PREFS, TAG_END);
	}
	if (!(flags & PRF_ALL))
		flags |= PRF_ALL;

	if (opts[PREFS_LOAD] || opts[PREFS_SAVE]) {
		char dest[256], doit = FALSE;

		if (!opts[PREFS_REQ]) {
			if (!opts[PREFS_NAME])
				strcpy(dest, CONFIG_PATH "/ignition.prefs");
			else
				strcpy(dest, (STRPTR)opts[PREFS_NAME]);

			doit = TRUE;
		} else if (AslRequestTags(fileReq,
#ifdef __amigaos4__
			   	ASLFR_Window,		 rxpage->pg_Window,
#else
			   	ASLFR_Window,		 scr->FirstWindow,
#endif
				ASLFR_TitleText,		opts[PREFS_LOAD]
					? GetString(&gLocaleInfo, MSG_LOAD_PREFERENCES_TITLE)
					: GetString(&gLocaleInfo, MSG_SAVE_PREFERENCES_TITLE),
				ASLFR_InitialDrawer,	CONFIG_PATH,
#ifdef __amigaos4__
				ASLFR_AcceptPattern, 	"#?.prefs", 
				ASLFR_DoPatterns, 	 	TRUE,
#endif
				ASLFR_InitialFile,  	"ignition.prefs",
				ASLFR_InitialPattern,	"#?.prefs",
				ASLFR_DoSaveMode,		opts[PREFS_SAVE],
				ASLFR_DoPatterns,		TRUE,
				ASLFR_DrawersOnly,   	FALSE,
				TAG_END)) {
			strcpy(dest, fileReq->fr_Drawer);
			AddPart(dest, fileReq->fr_File, 255);

			doit = TRUE;
		}

		if (doit) {
			if (opts[PREFS_LOAD]) {
				LoadPrefs(&prefs,dest,(BPTR)NULL,flags);
//		RefreshPrefsModules(&prefs,flags);
			} else
				SavePrefs(&prefs,dest,flags);
		}
	}
	return 0;
}

// REDO

ULONG
rxRedo(long *opts)
{
	if (ApplyRedo(rxpage))
		return RC_OK;

	return RC_WARN;
}

// UNDO

ULONG
rxUndo(long *opts)
{
	if (ApplyUndo(rxpage))
		return RC_OK;

	return RC_WARN;
}

// SELECT POS,LEFT/S,RIGHT/S,ABOVE/S,BELOW/S,BLOCK/S
//		0   1	  2	   3	   4	   5

ULONG
rxSelect(long *opts)
{
	struct tablePos tp;
	long   col,row;

	if (opts[0])
	{
		BeginRxPos(RXPOS_CELL, (STRPTR)opts[0]);
		CreateTabGadget(rxpage, rxpage->pg_MarkCol, rxpage->pg_MarkRow, TRUE);
		EndRxPos((STRPTR)opts[0]);

		return RC_OK;
	}

	if (rxpage->pg_Gad.DispPos != PGS_NONE)
	{
		// aktuelle Zelle bewegen
		col = rxpage->pg_Gad.cp.cp_Col;
		row = rxpage->pg_Gad.cp.cp_Row;
		if (opts[1])
			col--;
		if (opts[2])
			col++;
		if (opts[3])
			row--;
		if (opts[4])
			row++;
		if (col < 1 || row < 1)
		{
			DisplayBeep(NULL);
			return RC_OK;
		}
	}
	else
	{
		// es gibt noch keine aktuelle Zelle
		col = 1;
		row = 1;
	}

	if (opts[5])	// BLOCK
	{
		if (!GetRxTablePos(&tp))
		{
			tp.tp_Col = tp.tp_Row = 1;
			tp.tp_Width = tp.tp_Height = 0;
		}
		if (col != rxpage->pg_Gad.cp.cp_Col)
		{
			if (col > rxpage->pg_Gad.cp.cp_Col && col > tp.tp_Col+tp.tp_Width || col < rxpage->pg_Gad.cp.cp_Col && col >= tp.tp_Col)
				tp.tp_Width = col-tp.tp_Col;
			else
			{
				tp.tp_Width = tp.tp_Col+tp.tp_Width-col;
				tp.tp_Col = col;
			}
		}
		if (row != rxpage->pg_Gad.cp.cp_Row)
		{
			if (row > rxpage->pg_Gad.cp.cp_Row && row > tp.tp_Row+tp.tp_Height || row < rxpage->pg_Gad.cp.cp_Row && row >= tp.tp_Row)
				tp.tp_Height = row-tp.tp_Row;
			else
			{
				tp.tp_Height = tp.tp_Row+tp.tp_Height-row;
				tp.tp_Row = row;
			}
		}
		SetMark(rxpage, tp.tp_Col, tp.tp_Row, tp.tp_Width, tp.tp_Height);
	}
	else
		SetMark(rxpage, -1, 0, 0, 0);

	CreateTabGadget(rxpage, col, row, TRUE);

	return RC_OK;
}

// HELP GUIDE,ACTIVEWINDOW/S
//	  0	 1

ULONG
rxHelp(long *opts)
{
	struct Window *swin,*helpwin = NULL;
	struct Gadget *sgad;
	struct winData *wd;
	struct AppCmd *ac;
	APTR   layer;
	BOOL   noprefix = FALSE;
	char   t[256];
	long   x,y,i;

	t[0] = 0;
	if (opts[0])
	{
		strcpy(t, (STRPTR)opts[0]);
	}
	else if (imsg.Class == IDCMP_MENUHELP)
	{
		struct IgnAppMenuEntry *ame;
		struct MenuItem *item;

		strcpy(t, "unknown");
		if ((item = ItemAddress(prefs.pr_Menu,imsg.Code)) && (ame = GTMENUITEM_USERDATA(item)) && (ac = FindAppCmd(rxpage,ame->am_AppCmd)) && ac->ac_Guide)
			strcpy(t, ac->ac_Guide),  noprefix = TRUE;
	}
	else
	{
		strcpy(t, "main");
		if (!opts[1] && (layer = WhichLayer(&scr->LayerInfo, x = win->LeftEdge + imsg.MouseX, y = win->TopEdge + imsg.MouseY)))
		{
			for (swin = scr->FirstWindow; swin; swin = swin->NextWindow)
			{
#ifdef __amigaos4__
				if(swin->UserPort != iport)	//Not a ignition then next
					continue;
#endif
				if (layer == swin->WLayer && swin->UserPort == iport)
				{
					x -= swin->LeftEdge;
					y -= swin->TopEdge;
					helpwin = swin;
					wd = (struct winData *)helpwin->UserData;
					break;
				}
			}
		}

		if (opts[1] || !helpwin)
			helpwin = win,  wd = (struct winData *)helpwin->UserData,  x = imsg.MouseX,  y = imsg.MouseY;

		if (wd->wd_Type == WDT_PROJECT)
		{
			struct PrefDisp *pd = ((struct Page *)wd->wd_Data)->pg_Mappe->mp_Prefs.pr_Disp;

			strcpy(t, "project");
			if (x > wd->wd_TabX && x < wd->wd_TabX+wd->wd_TabW && y > wd->wd_TabY && y < wd->wd_TabY+wd->wd_TabH)
				strcpy(t, "proj_table");
			else if (x > wd->wd_TabX-pd->pd_AntiWidth && x < wd->wd_TabX+wd->wd_TabW && y > wd->wd_TabY-pd->pd_AntiHeight && y < wd->wd_TabY+wd->wd_TabH)
				strcpy(t, "proj_anti");
			else if ((i = IsOverProjSpecial(helpwin, x, y)))
			{
				switch (i)
				{
					case GID_APEN:
						strcpy(t,"proj_apen");
						break;
					case GID_BPEN:
						strcpy(t,"proj_bpen");
						break;
					case GID_POSITION:
						strcpy(t,"form_pos");
						break;
					case GID_STATUS:
						strcpy(t,"helpbar");
						break;
				}
			}
			else for (sgad = helpwin->FirstGadget; sgad; sgad = sgad->NextGadget)
			{
				if (PointInGadget(sgad, x, y))
				{
					if (sgad->GadgetID == GID_ICONOBJ && (ac = FindAppCmd(rxpage, (STRPTR)sgad->UserData)) && ac->ac_Guide)
						strcpy(t,ac->ac_Guide),  noprefix = TRUE;
					else
						sprintf(t, "proj_gadget%03ld", sgad->GadgetID);
					break;
				}
			}
		}
		else if (wd->wd_Type != WDT_INFO)
		{
			STRPTR helpName = gCreateWinData[wd->wd_Type - 1].cwd_HelpName;

			sprintf(t, "window_%s", helpName);

			if (wd->wd_PageHandlingGadget && PointInGadget(wd->wd_PageHandlingGadget, x, y))
			{
				GetAttr(PAGEGA_Active, (Object *)wd->wd_PageHandlingGadget, (IPTR *)&i);
				sprintf(t + strlen(t), "_page%ld", i);
			}
		}
	}
	if (t[0] && gAmigaGuide)
	{
//printf("rxHelp: <%s>\n", t);
		if (!opts[0] && !noprefix)
			strins(t, "ignition.guide/");
		strins(t, "link ");
		SendAmigaGuideCmdA(gAmigaGuide, t, NULL);
	}

	return RC_OK;
}

// COMMAND NAME,REQ/S
//		 0	1

ULONG
rxCommand(long *opts)
{
	ProcessAppCmd(rxpage, (STRPTR)opts[0]);
	if (opts[1])
		OpenAppWindow(WDT_COMMAND, TAG_END);

	return RC_OK;
}

// SCRIPT NAME/A,EXTERN/S,NEW/S,EDIT/S,DELETE/S,REQ/S
//		0	  1		2	 3	  4		5

ULONG
rxScript(long *opts)
{
	struct Mappe *mp;

	mp = rxpage->pg_Mappe;

	if (opts[5])
		OpenAppWindow(WDT_SCRIPTS,TAG_END);

	if (!opts[0])
	{
		if (opts[5])
			return(RC_OK);

		ErrorRequest(GetString(&gLocaleInfo, MSG_BAD_ARGUMENTS_ERR));
		return(RC_WARN);
	}
	if (opts[1] || !opts[2] && !opts[3] && !opts[4])
		return(RunRexxScript(opts[1] ? RXS_EXTERN : RXS_INTERN,(STRPTR)opts[0]));

	if (opts[2])
		NewRexxScript(mp,(STRPTR)opts[0],TRUE);

	if (opts[3] || opts[4])
	{
		struct RexxScript *rxs;

		if (!(rxs = (APTR)FindTag(&mp->mp_RexxScripts,(STRPTR)opts[0])))
			return(RC_WARN);

		if (opts[3])
			EditRexxScript(mp,rxs);
		else if (opts[4])
			DeleteRexxScript(mp,rxs);
	}
	return(RC_OK);
}

// POS2COORD POS,STEM/K,COL/K,ROW/K
//		   	  0   1	     2	 3

ULONG rxPos2Coord(long *opts)
{
	char t[64],s[16];
	long col,row;

	if (!opts[1] && (!opts[2] || !opts[3]))
		return(RC_WARN);
	if (opts[0])
		String2Coord((STRPTR)opts[0],&col,&row);
	else if (rxpage)
		col = rxpage->pg_Gad.cp.cp_Col,  row = rxpage->pg_Gad.cp.cp_Row;
	else
		return(RC_WARN);
	if (opts[1])
		sprintf(t, "%s.COL", (char *)opts[1]);
	else
		strcpy(t,(STRPTR)opts[2]);
	sprintf(s,"%lu",col);
	StringToUpper(t);
	SetRexxVar(rxmsg,t,s,strlen(s));
	if (opts[1])
		sprintf(t, "%s.ROW", (char *)opts[1]);
	else
		strcpy(t,(STRPTR)opts[3]);
	sprintf(s,"%lu",row);
	StringToUpper(t);
	SetRexxVar(rxmsg,t,s,strlen(s));
	return(RC_OK);
}

// COORD2POS COORD,COL/N,ROW/N,VAR/K/A
//		       0	 1	 2	     3

ULONG rxCoord2Pos(long *opts)
{
	char t[1024], s[1024], *s2;
	long col = 1,row = 1;
	static char sc[16];

	if (!opts[3] || !opts[0] && (!opts[1] || !opts[2]))
		return(RC_WARN);
	if (opts[0])
	{
		sprintf(t, "%s.COL", (char *)opts[0]);
		StringToUpper(t);
#ifdef __amigaos4__
		if (!GetRexxVar(rxmsg,t,s))
#else
		if (!GetRexxVar(rxmsg,t,&s))
#endif
			col = atol(s);
		sprintf(t, "%s.ROW", (char *)opts[0]);
		StringToUpper(t);
#ifdef __amigaos4__
		if (!GetRexxVar(rxmsg,t,s))
#else
		if (!GetRexxVar(rxmsg,t,&s))
#endif
			row = atol(s);
	}
	else
	{
		col = *(ULONG *)opts[1];
		row = *(ULONG *)opts[2];
	}
#ifdef __amigaos4__
	if (col < 1 || row < 1)
	{ 
		strcpy(t,(STRPTR)opts[3]);
		//strcpy(sc, "ERROR");
		StringToUpper(t);
		SetRexxVar(rxmsg,t,"ERROR",strlen(sc));
		return RC_FATAL;
	}
#else
	if (col < 1)
		col = 1;
	if (row < 1)
		row = 1;
#endif
	strcpy(t,(STRPTR)opts[3]);
	s2 = Coord2String(col,row);
	StringToUpper(t);
	SetRexxVar(rxmsg,t,s2,strlen(s2));
	return(RC_OK);
}

// SORT POS,REVERSE/S,HORIZ/S,CMP=COMPARE/K
//	  0   1		 2	   3

BOOL reverse;

long
SortCompare(struct sortNode *sna,struct sortNode *snb)
{
	long rc;
	if (sna->sn_Text || snb->sn_Text)
	{
		if (sna->sn_Text && !snb->sn_Text)
			rc = 1;
		else if (!sna->sn_Text && snb->sn_Text)
			rc = -1;
		else
			rc = stricmp(sna->sn_Text,snb->sn_Text);
	}
	else
		rc = (long)(sna->sn_Value-snb->sn_Value);

	if (reverse)
		return -rc;

	return rc;
}


ULONG
rxSort(long *opts)
{
	struct sortNode *sn;
	struct tableField *tf = NULL;
	struct tablePos tp;
	struct Term *t;
	struct MinList list;
	long   i,col,row,cmp,slen,wid,hei;
	BOOL   text = FALSE;

	if (IsListEmpty((struct List *)&rxpage->pg_Table))
		return RC_WARN;

	if (opts[1])		// Reverse sort
		reverse = TRUE;
	else
		reverse = FALSE;

	BeginRxPos(RXPOS_BLOCK,(STRPTR)opts[0]);

	if (rxpage->pg_MarkCol != -1)
	{
		wid = rxpage->pg_MarkWidth;
		hei = rxpage->pg_MarkHeight;
		if (wid == -1)
			wid = rxpage->pg_Cols;
		if (hei == -1)
			hei = rxpage->pg_Rows;

		if (opts[3] && FillTablePos(&tp,t = CreateTree(rxpage,(STRPTR)opts[3])))  // find compare
			DeleteTree(t);
		else
		{
			tp.tp_Row = rxpage->pg_MarkRow;
			tp.tp_Col = rxpage->pg_MarkCol;
		}
		if (opts[2])	  // horizontal sort
		{
			slen = wid+1;
			cmp = tp.tp_Row;
		}
		else			  // vertical sort
		{
			slen = hei+1;
			cmp = tp.tp_Col;
		}
		if ((sn = AllocPooled(pool, sizeof(struct sortNode) * slen)) != 0)
		{
			col = opts[2] ? rxpage->pg_MarkCol : cmp;
			row = !opts[2] ? rxpage->pg_MarkRow : cmp;

			while(!tf && col >= rxpage->pg_MarkCol && col <= rxpage->pg_MarkCol+wid && row >= rxpage->pg_MarkRow && row <= rxpage->pg_MarkRow+hei)
			{
				tf = GetTableField(rxpage,col,row);
				if (opts[2])
					col++;
				else
					row++;
			}
			if (tf)
			{
				if (tf->tf_Type & TFT_TEXT)
					text = TRUE;
				col = opts[2] ? rxpage->pg_MarkCol : cmp;
				row = !opts[2] ? rxpage->pg_MarkRow : cmp;
				for(i = 0,tf = (APTR)rxpage->pg_Table.mlh_Head;col >= rxpage->pg_MarkCol && col <= rxpage->pg_MarkCol+wid && row >= rxpage->pg_MarkRow && row <= rxpage->pg_MarkRow+hei;i++)
				{
					while(tf->tf_Node.mln_Succ && (opts[2] && (tf->tf_Row != row || tf->tf_Col < col) || !opts[2] && (tf->tf_Col != col || tf->tf_Row < row)))
						tf = (APTR)tf->tf_Node.mln_Succ;
					if (tf->tf_Col == col && tf->tf_Row == row)
					{
						(sn+i)->sn_Number = opts[2] ? col : row;
						if (text)
							(sn+i)->sn_Text = tf->tf_Text;
						else
							(sn+i)->sn_Value = tf->tf_Value;
					}
					if (opts[2])
						col++;
					else
						row++;
				}
				qsort(sn,slen,sizeof(struct sortNode),(APTR)/*(int *)(void const *,void const *)*/SortCompare);
				MyNewList(&list);  tf = NULL;  wid = 0;
				BeginUndo(rxpage,UNDO_BLOCK,GetString(&gLocaleInfo, MSG_SORT_UNDO));

				while ((tf = GetMarkedFields(rxpage, tf, FALSE)) != 0)
				{
					RemoveCell(rxpage, tf, false);
						// we don't need to recalculate here because all cells will be inserted again

					if (tf->tf_Col+tf->tf_Width > wid)
						wid = tf->tf_Col+tf->tf_Width;

					MyAddTail(&list, tf);
				}
				for(tf = (APTR)list.mlh_Head;tf->tf_Node.mln_Succ;tf = (APTR)tf->tf_Node.mln_Succ)
				{
					for(i = 0;i < slen;i++)
					{
						if (!opts[2] && (sn+i)->sn_Number == tf->tf_Row)
						{
							tf->tf_Row = i+rxpage->pg_MarkRow;
							break;
						}
						else if(opts[2] && (sn+i)->sn_Number == tf->tf_Col)
						{
							tf->tf_Col = i+rxpage->pg_MarkCol;
							break;
						}
					}
				}
				while ((tf = (struct tableField *)MyRemHead(&list)) != 0)
					InsertCell(rxpage, tf, false);

				EndUndo(rxpage);
				RefreshMarkedTable(rxpage, wid, TRUE);
			}
			else
				ErrorRequest(GetString(&gLocaleInfo, MSG_EMPTY_COMPARE_ROW_COL_ERR),opts[2] ? GetString(&gLocaleInfo, MSG_COMPARE_ROW_ERR) : GetString(&gLocaleInfo, MSG_COMPARE_COLUMN_ERR));
			FreePooled(pool,sn,slen);
		}
	}
	EndRxPos((STRPTR)opts[0]);
	return(RC_OK);
}

// GETBORDER POS,LEFT/K,RIGHT/K,BELOW/K,ABOVE/K,STEM/K
//		   0   1	  2	   3	   4	   5

void GetBorderData(struct tableField *tf,long side,STRPTR s)
{
	struct colorPen *cp;

	if (!tf || !tf->tf_Border[side])
	{
		strcpy(s,"/");
		return;
	}
	*s = 0;
	if ((cp = GetColorPen(tf->tf_BorderColor[side])) != 0)
		strcpy(s,cp->cp_Node.ln_Name);
	strcat(s,"/");
	strcat(s,ita((double)tf->tf_Border[side]/32,-3,ITA_NONE));
	strcat(s,"pt");
}


ULONG rxGetBorder(long *opts)
{
	struct tableField *tf;
	char   t[64],*s;

	if (!opts[5] && !opts[1] && !opts[2] && !opts[3] && !opts[4])
		return(RC_WARN);
	if (!(s = AllocPooled(pool,512)))
		return(RC_FATAL);
	BeginRxPos(RXPOS_CELL,(STRPTR)opts[0]);
	tf = GetTableField(rxpage,rxpage->pg_MarkCol,rxpage->pg_MarkRow);
	if (opts[5] || opts[1])			   // LEFT
	{
		if (opts[5])
			sprintf(t, "%s.LEFT", (char *)opts[5]);
		else
			strcpy(t,(STRPTR)opts[1]);
		StringToUpper(t);
		GetBorderData(tf,0,s);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (opts[5] || opts[2])			   // RIGHT
	{
		if (opts[5])
			sprintf(t, "%s.RIGHT", (char *)opts[5]);
		else
			strcpy(t,(STRPTR)opts[2]);
		StringToUpper(t);
		GetBorderData(tf,1,s);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (opts[5] || opts[3])			   // BELOW
	{
		if (opts[5])
			sprintf(t, "%s.BELOW", (char *)opts[5]);
		else
			strcpy(t,(STRPTR)opts[3]);
		StringToUpper(t);
		GetBorderData(tf,2,s);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (opts[5] || opts[4])			   // ABOVE
	{
		if (opts[5])
			sprintf(t,"%s.ABOVE", (char *)opts[5]);
		else
			strcpy(t,(STRPTR)opts[4]);
		StringToUpper(t);
		GetBorderData(tf,3,s);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	EndRxPos((STRPTR)opts[0]);
	FreePooled(pool,s,512);
	return(RC_OK);
}

// GETCELL POS,APEN/S,BPEN/S,FONT/S,FONTSIZE/S,STYLE/S,TEXT/S,FORMAT/S,VISIBLE/S,ALIGNMENT/S,NOTE/S,VAR/K,STEM/K
//		    0    1	   2	   3	   4		  5	     6	     7		  8		    9		   10	 11	    12

ULONG rxGetCell(long *opts)
{
	struct tableField *tf;
	ULONG  flags = ~0L;
	char   t[64],*s;
	long   i,j,k;
	BOOL   stem = FALSE;

	if (opts[12])
		stem = TRUE;
	for(i = 1,j = 0,k = 0;i < 11;i++)
	{
		if (opts[i])
		{
			j |= 1 << i;
			k++;
		}
	}
	if (k)
		flags = j;
	if (!opts[11] && !opts[12] || opts[11] && k != 1)
		return(RC_WARN);
	if (!(s = AllocPooled(pool,1024)))
		return(RC_FATAL);
	if (opts[11])
		strcpy(t,(STRPTR)opts[11]);
	BeginRxPos(RXPOS_CELL,(STRPTR)opts[0]);
	tf = GetTableField(rxpage,rxpage->pg_MarkCol,rxpage->pg_MarkRow);
	if (flags & (1 << 1))  // APEN
	{
		sprintf(s,"%lu",tf ? tf->tf_APen : rxpage->pg_APen);
		if (stem)
			sprintf(t, "%s.APEN", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 2))  // BPEN
	{
		sprintf(s,"%lu",tf ? tf->tf_BPen : rxpage->pg_BPen);
		if (stem)
			sprintf(t, "%s.BPEN", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 3))  // FONT
	{
		sprintf(s,"%s",tf ? tf->tf_FontInfo->fi_Family->ln_Name : rxpage->pg_Family->ln_Name);
		if (stem)
			sprintf(t, "%s.FONT", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 4))  // FONTSIZE
	{
		sprintf(s,"%s",ita((tf ? tf->tf_FontInfo->fi_FontSize->fs_PointHeight : rxpage->pg_PointHeight)/65536.0,-1,FALSE));
		if (stem)
			sprintf(t, "%s.FONTSIZE", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 5))  // STYLE
	{
		sprintf(s,"%lu",tf ? tf->tf_FontInfo->fi_Style : FS_PLAIN);
		if (stem)
			sprintf(t, "%s.STYLE", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 6))  // TEXT
	{
		sprintf(s,"%s",tf && tf->tf_Original ? tf->tf_Original : (STRPTR)"");
		if (stem)
			sprintf(t, "%s.TEXT", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 7))  // FORMAT
	{
		sprintf(s,"%s",tf && tf->tf_Format ? tf->tf_Format : (STRPTR)"");
		if (stem)
			sprintf(t, "%s.FORMAT", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 8))  // VISIBLE
	{
		sprintf(s,"%s",tf && tf->tf_Text ? tf->tf_Text : (STRPTR)"");
		if (stem)
			sprintf(t, "%s.VISIBLE", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 9))  // ALIGNMENT
	{
		sprintf(s,"%ld",tf ? tf->tf_Alignment : TFA_BOTTOM | TFA_LEFT);
		if (stem)
			sprintf(t, "%s.ALIGNMENT", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (flags & (1 << 10))  // NOTE
	{
		sprintf(s,"%s",tf && tf->tf_Note ? tf->tf_Note : (STRPTR)"");
		if (stem)
			sprintf(t, "%s.NOTE", (char *)opts[12]);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	EndRxPos((STRPTR)opts[0]);
	FreePooled(pool,s,1024);
	return(RC_OK);
}

// GETPOS COL/K,ROW/K,STEM/K
//		0	 1	 2

ULONG rxGetPos(long *opts)
{
	char t[64],s[16];

	if (opts[0])			   /* Column */
	{
		strcpy(t,(STRPTR)opts[0]);
		sprintf(s,"%lu",rxpage->pg_Gad.cp.cp_Col);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (opts[1])			   /* Row */
	{
		strcpy(t,(STRPTR)opts[1]);
		sprintf(s,"%lu",rxpage->pg_Gad.cp.cp_Row);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	if (opts[2])			   /* Both */
	{
		sprintf(t, "%s.COL", (char *)opts[2]);
		sprintf(s,"%lu",rxpage->pg_Gad.cp.cp_Col);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
		sprintf(t, "%s.ROW", (char *)opts[2]);
		sprintf(s,"%lu",rxpage->pg_Gad.cp.cp_Row);
		StringToUpper(t);
		SetRexxVar(rxmsg,t,s,strlen(s));
	}
	return(RC_OK);
}

// LOCK

ULONG rxLock(long *opts)
{
	SetBusy(TRUE,BT_PROJECT);
	rxpage->pg_Locked++;

	return(RC_OK);
}

// UNLOCK

ULONG rxUnLock(long *opts)
{
	if (rxpage->pg_Locked)
	{
		rxpage->pg_Locked--;
		DrawTable(rxpage->pg_Window);
		SetBusy(FALSE,BT_PROJECT);
		return RC_OK;
	}
	return RC_WARN;
}

// GetCheckboxState Checkbox-name 
//					0

ULONG rxGetCbState(long *opts)
{
    struct gInterface *gi,*sgi = NULL;
    struct gObject *go;
    STRPTR obj,t;
	
	if(opts[0] == 0)
		return RC_FAIL;
    if (!(go = MyFindName(&calcpage->pg_gObjects, (STRPTR)opts[0])))
    	return RC_FAIL;

    for(gi = go->go_Class->gc_Interface;gi->gi_Tag;gi++) // bei gInterface suchen
    {
        if (!zstricmp(gi->gi_Name,(STRPTR)"Checked"))
        {
            sgi = gi;
            break;
        }
    }
	if(!sgi)
		return RC_FAIL;
    GetGObjectAttr(go,sgi->gi_Tag,(ULONG *)&obj);
	return (ULONG)obj;
}

// GetCheckboxState Checkbox-name State
//					0				1

ULONG rxSetCbState(long *opts)
{
    struct gInterface *gi,*sgi = NULL;
    struct gObject *go;
	
	if(opts[0] == 0)
		return RC_FAIL;
    if (!(go = MyFindName(&rxpage->pg_gObjects, (STRPTR)opts[0])))
    	return RC_FAIL;
    for(gi = go->go_Class->gc_Interface;gi->gi_Tag;gi++) // bei gInterface suchen
    {
        if (!zstricmp(gi->gi_Name,(STRPTR)"Checked"))
        {
            sgi = gi;
            break;
        }
    }
	if(!sgi)
		return RC_FAIL;
	SetGObjectAttrs(rxpage, go, sgi->gi_Tag, (((char **)opts)[1][0] == '0' ? 0 : 1), TAG_DONE);
	return RC_OK;
}

// TEST

ULONG
rxTest(long *opts)
{
	// struct gObj *go;

	// OpenAppWindow(WDT_DOCUMENT,TAG_END);
	/*if (go = CreateButton(rxpage,"Test",NULL))
		SetGFramePositionMode(rxpage,go->go_Frame);*/
	return RC_OK;
}


void
AddIntCmd(ULONG (*func)(long *),UBYTE flags,STRPTR template)
{
	struct IntCmd *ic;

	if ((ic = AllocPooled(pool, sizeof(struct IntCmd))) != 0)
	{
		ic->ic_Node.ln_Name = template;
		ic->ic_Node.ln_Type = flags;
		ic->ic_Function = func;
		MyAddTail(&intcmds, ic);
	}
}


void
CloseRexx(void)
{
	if (notifyport)
	{
		EmptyMsgPort(notifyport);
#ifdef __amigaos4__
		FreeSysObject(ASOT_PORT, notifyport);
#else
		DeleteMsgPort(notifyport);
#endif
	}
	if (rxsigbit != -1)
		FreeSignal(rxsigbit);
}


void
initRexx(void)
{
	MyNewList(&rexxports);

#ifdef __amigaos4__
	if ((notifyport = AllocSysObjectTags(ASOT_PORT, TAG_END)) != 0)
#else
	if ((notifyport = CreateMsgPort()) != 0)
#endif
	{
		notifysig = 1L << notifyport->mp_SigBit;
		//rxmask = notifysig;
	}
	if ((rxsigbit = AllocSignal(-1L)) != -1)
		rxmask = 1L << rxsigbit;

	/******** Function ******* Flags **** Template *******************************************************************************************/

	AddIntCmd(rxAbout,		 ICF_NONE,  "ABOUT");
	AddIntCmd(rxAlign,		 ICF_PAGE,  "ALIGN POS,LEFT/S,CENTER/S,RIGHT/S,DELETE/S,TOP/S,MIDDLE/S,BOTTOM/S,REQ/S");
	AddIntCmd(rxBlock,		 ICF_PAGE,  "BLOCK POS,NAME,LOAD/S,SAVE/S,REQ/S");
	AddIntCmd(rxBorder,		ICF_PAGE,  "BORDER POS,LEFT/K,RIGHT/K,BELOW/K,ABOVE/K,BLOCK/S,REQ/S");
	AddIntCmd(rxCalc,		  ICF_PAGE | ICF_MSG, "CALC EXP=EXPRESSION,VAR/K");
	AddIntCmd(rxCell,		  ICF_PAGE,  "CELL POS,INSERTROW/S,INSERTCOL/S,SHIFTDOWN/S,SHIFTRIGHT/S,REMOVEROW/S,REMOVECOL/S,SHIFTUP/S,SHIFTLEFT/S");
	AddIntCmd(rxCellColumns,   ICF_PAGE,  "CELLCOLUMNS POS,SET/S,TOGGLE/S,RESET/S");
	AddIntCmd(rxCellName,	  ICF_PAGE,  "CELLNAME POS,SET/K,REMOVE/S,REQ/S");
	AddIntCmd(rxCellSize,	  ICF_PAGE,  "CELLSIZE POS,WIDTH/K,HEIGHT/K,OPTWIDTH/S,OPTHEIGHT/S,MINWIDTH/S,MINHEIGHT/S,REQ/S");
	AddIntCmd(rxClip,		  ICF_NONE,  "CLIP REQ/S,UNIT/N/K");
	AddIntCmd(rxClose,		 ICF_NONE,  "CLOSE NAME,QUIET/S");
	AddIntCmd(rxColor,		 ICF_PAGE,  "COLOR POS,APEN/K,BPEN/K,REQ/S");
	AddIntCmd(rxCommand,	   ICF_NONE,  "COMMAND NAME,REQ/S");
	AddIntCmd(rxCoord2Pos,	 ICF_MSG,   "COORD2POS COORD,COL/N,ROW/N,VAR/K/A");
	AddIntCmd(rxCopy,		  ICF_PAGE,  "COPY POS,UNIT/N/K");
	AddIntCmd(rxCurrent,	   ICF_NONE,  "CURRENT MAP=WINDOW/K,PAGE,NEXT/S,PREV/S,SHOW/S,NUM/N/K");
	AddIntCmd(rxCut,		   ICF_PAGE,  "CUT POS,TEXTONLY/S,UNIT/N/K");
	AddIntCmd(rxDatabase,	  ICF_NONE,  "DATABASE REQ/S");
	AddIntCmd(rxDbCurrent,	 ICF_PAGE,  "DBCURRENT NAME/A,NEXT/S,PREV/S,FIRST/S,LAST/S,PARENT,NUM/N/K");
	AddIntCmd(rxDbNew,		 ICF_PAGE,  "DBNEW NAME,PARENT,NOREFS/S");
	AddIntCmd(rxDbSearch,	  ICF_PAGE,  "DBSEARCH NAME,FILTER/K,INPUT/S,START/S,STOP/S,TOGGLE/S");
#ifdef __amigaos4__
	AddIntCmd(rxDbSetFilter,	 ICF_PAGE,  "DBSETFILTER DBNAME, FILTERNAME");
#endif
	AddIntCmd(rxDelete,		ICF_PAGE,  "DELETE POS,TEXTONLY/S");
	AddIntCmd(rxDeletePage,	ICF_PAGE,  "DELETEPAGE NAME");
	AddIntCmd(rxDiagram,	   ICF_NONE,  "DIAGRAM NAME,REQ/S");
	AddIntCmd(rxDocInfo,	   ICF_NONE,  "DOCINFO REQ/S");
	AddIntCmd(rxDocument,	  ICF_NONE,  "DOCUMENT REQ/S");
#ifdef DEBUG
	AddIntCmd(rxDumpRef,	   ICF_NONE,  "DUMPREF UNRESOLVED/S,TIMED/S");
#endif
	AddIntCmd(rxDupObject,	 ICF_PAGE,  "DUPOBJECT NAME");
	AddIntCmd(rxFilter,		ICF_NONE,  "FILTER REQ/S");
	AddIntCmd(rxFormat,		ICF_PAGE,  "FORMAT POS,SET/K,TYPE/N,FRACTION/N,ALIGN/K,PRI/N,NEGPEN/K,NEGPARENTHESES/S,REQ/S,GLOBAL/S");
#ifdef __amigaos4__
	AddIntCmd(rxFraction,	  ICF_PAGE,  "FRACTION POS,NUM/N,INC/S,DEC/S");
#else
	AddIntCmd(rxFraction,	  ICF_PAGE,  "FRACTION POS,NUM,INC/S,DEC/S");
#endif
	AddIntCmd(rxGetBorder,	 ICF_MSG | ICF_PAGE, "GETBORDER POS,LEFT/K,RIGHT/K,BELOW/K,ABOVE/K,STEM/K");
#ifdef __amigaos4__
	AddIntCmd(rxGetCbState,	 ICF_PAGE,  "GETCBSTATE CBNAME");
#endif
	AddIntCmd(rxGetCell,	   ICF_MSG | ICF_PAGE, "GETCELL POS,APEN/S,BPEN/S,FONT/S,FONTSIZE/S,STYLE/S,TEXT/S,FORMAT/S,VISIBLE/S,ALIGNMENT/S,NOTE/S,VAR/K,STEM/K");
	AddIntCmd(rxGetPos,		ICF_MSG | ICF_PAGE, "GETPOS COL/K,ROW/K,STEM/K");
	AddIntCmd(rxGroup,		 ICF_PAGE,  "GROUP NAME");
	AddIntCmd(rxHelp,		  ICF_NONE,  "HELP GUIDE,ACTIVEWINDOW/S");
	AddIntCmd(rxHide,		  ICF_PAGE,  "HIDE");
	AddIntCmd(rxIgnoreEvent,   ICF_NONE,  "IGNOREEVENT");
	AddIntCmd(rxIndex,		 ICF_NONE,  "INDEX REQ/S");
	AddIntCmd(rxInsertFormula, ICF_PAGE,  "INSERTFORMULA POS,NAME,TYPE/N/K,REQ/S");
	AddIntCmd(rxInsertObject,  ICF_NONE,  "INSERTOBJECT TYPE,REQ/S");
	AddIntCmd(rxInteractive,   ICF_PAGE,  "INTERACTIVE SET/S,TOGGLE/S,NONE/S");
	AddIntCmd(rxLastError,	 ICF_MSG,   "LASTERROR VAR");
	AddIntCmd(rxLoad,		  ICF_NONE,  "LOAD NAME,REQ/S,NEW/S,TYPE/K,TYPEREQ/S,OLDPATH/S");
	AddIntCmd(rxLoadPicture,   ICF_PAGE,  "LOADPICTURE NAME,REQ/S");
	AddIntCmd(rxLock,		  ICF_PAGE,  "LOCK");
	AddIntCmd(rxMask,		  ICF_NONE,  "MASK REQ/S");
	AddIntCmd(rxName,		  ICF_NONE,  "NAME NAME,SET/K,TYPE,DELETE/S,REQ/S,GLOBAL/S");
	AddIntCmd(rxNegParentheses,ICF_PAGE,  "NEGPARENTHESES POS,SET/S,TOGGLE/S,NONE/S");
	AddIntCmd(rxNew,		   ICF_NONE,  "NEW");
	AddIntCmd(rxNewPage,	   ICF_PAGE,  "NEWPAGE NAME,QUIET/S");
	AddIntCmd(rxNext,		  ICF_PAGE,  "NEXT");
	AddIntCmd(rxNop,		   ICF_NONE,  "NOP");
	AddIntCmd(rxNotes,		 ICF_NONE,  "NOTES REQ/S");
	AddIntCmd(rxObjectInfo,	ICF_PAGE,  "OBJECTINFO NAME");
	AddIntCmd(rxObjectToBack,  ICF_PAGE,  "OBJECTTOBACK NAME,MOST/S");
	AddIntCmd(rxObjectToFront, ICF_PAGE,  "OBJECTTOFRONT NAME,MOST/S");
	AddIntCmd(rxPage,		  ICF_PAGE,  "PAGE NAME,UP/S,DOWN/S,FIRST/S,LAST/S,REQ/S");
	AddIntCmd(rxPageSetup,	 ICF_NONE,  "PAGESETUP REQ/S");
	AddIntCmd(rxPaste,		 ICF_PAGE,  "PASTE POS,TEXTONLY/S,UNIT/N/K,INTERN/S");
	AddIntCmd(rxPos2Coord,	 ICF_MSG,   "POS2COORD POS,STEM/K,COL/K,ROW/K");
	AddIntCmd(rxPrefs,		 ICF_NONE,  "PREFS DISPLAY/S,FILE/S,SCREEN/S,KEYS/S,FORMAT/S,PRINTER/S,COLORS/S,TABLE/S,ICON/S,CMDS/S,MENU/S,SYSTEM/S,NAMES/S,CONTEXTMENU/S,REQ/S,LOAD/S,SAVE/S,NAME/K,ADD/S,KEEPOLD/S,GLOBAL/S");
	AddIntCmd(rxPrint,		 ICF_NONE,  "PRINT REQ/S");
#ifndef __amigaos4__
	AddIntCmd(rxNop,		   ICF_NONE,  "PROJECTLIST");
#endif
	AddIntCmd(rxQuiet,		 ICF_NONE,  "QUIET TOGGLE/S,RESUME/S,DISCARD/S");
	AddIntCmd(rxQuit,		  ICF_NONE,  "QUIT");
	AddIntCmd(rxRecalc,		ICF_PAGE,  "RECALC");
	AddIntCmd(rxRedo,		  ICF_PAGE,  "REDO");
	AddIntCmd(rxRefObject,	 ICF_PAGE,  "REFOBJECT NAME,REQ/S");
	AddIntCmd(rxReplace,	   ICF_NONE,  "REPLACE REQ/S,NEXT/S");
	AddIntCmd(rxRequestString, ICF_MSG,   "REQUESTSTRING PROMPT,DEFAULT/K,VAR");
	AddIntCmd(rxRequestNumber, ICF_MSG,   "REQUESTNUMBER PROMPT,DEFAULT/K/N,VAR");
#ifdef __amigaos4__
	AddIntCmd(rxSave,		  ICF_PAGE,  "SAVE NAME,REQ/S,TYPEREQ/S,TYPE/K,TYPEPREFS/S");
#else
	AddIntCmd(rxSave,		  ICF_PAGE,  "SAVE NAME,REQ/S,TYPEREQ/S");
#endif
	AddIntCmd(rxScript,		ICF_PAGE,  "SCRIPT NAME,EXTERN/S,NEW/S,EDIT/S,DELETE/S,REQ/S");
	AddIntCmd(rxSearch,		ICF_NONE,  "SEARCH REQ/S,NEXT/S");
	AddIntCmd(rxSelect,		ICF_PAGE,  "SELECT POS,LEFT/S,RIGHT/S,ABOVE/S,BELOW/S,BLOCK/S");
	AddIntCmd(rxSeparator,	 ICF_PAGE,  "SEPARATOR POS,SET/S,TOGGLE/S,NONE/S");
#ifdef __amigaos4__
	AddIntCmd(rxSetCbState,	 ICF_PAGE,  "SETCBSTATE CBNAME, STATE");
#endif
	AddIntCmd(rxSort,		  ICF_PAGE,  "SORT POS,REVERSE/S,HORIZ/S,CMP=COMPARE/K");
	AddIntCmd(rxTest,		  ICF_NONE,  "TEST");
	AddIntCmd(rxText,		  ICF_PAGE,  "TEXT POS,SET/K,DELETE/S");
	AddIntCmd(rxTitle,		 ICF_PAGE,  "TITLE POS,SET/K,DELETE/S,HORIZ/S,VERT/S,REQ/S");
	AddIntCmd(rxUndo,		  ICF_PAGE,  "UNDO");
	AddIntCmd(rxUngroup,	   ICF_PAGE,  "UNGROUP NAME");
	AddIntCmd(rxUnLock,		ICF_PAGE,  "UNLOCK");
	AddIntCmd(rxZoom,		  ICF_PAGE,  "ZOOM PERCENT/N,PAGE/S,REQ/S");

	intCmdArray = BuildCommandArray(&intcmds,&intCmdArraySize);
}


#ifndef __amigaos4__
ULONG
RexxCall(ULONG (*func)(long *opts),...)
{
	/* rxmsg = NULL; */
	if (func)
		return func((long *)&func + 1);

	return RC_FAIL;
}
#endif

void
RemoveRexxPort(struct RexxPort *rxp)
{
	if (!rxp)
		return;

	MyRemove(rxp);
	RemPort(&rxp->rxp_Port);
	FreeString(rxp->rxp_Node.ln_Name);
	if (rxp->rxp_Message)
	{
		ClearRexxMsg(rxp->rxp_Message,1);
		DeleteRexxMsg(rxp->rxp_Message);
	}
	FreePooled(pool, rxp, sizeof(struct RexxPort));
}


struct RexxPort *
MakeRexxPort(void)
{
	struct RexxPort *rxp;
	long   i,len;
	STRPTR t;

	if (rxsigbit == -1)
		return(NULL);

	if ((t = AllocPooled(pool, (len = strlen(pubname)) + 2)) != 0)
	{
		strcpy(t,pubname);
		strcpy(t+len,"A");
	}
	else
		return(NULL);

	for(i = 0;i < 26 && FindPort(t);i++)
		*(t+len) = 'A'+i;

	if (i != 26)
	{
		if ((rxp = AllocPooled(pool, sizeof(struct RexxPort))) != 0)
		{
			if ((rxp->rxp_Message = CreateRexxMsg(&rxp->rxp_Port, "ign", t)) != 0)
			{
				rxp->rxp_Message->rm_Stdout = rxout;
				rxp->rxp_Message->rm_Stdin = rxout;
				MyAddTail(&rexxports, rxp);

				MyNewList(&rxp->rxp_Port.mp_MsgList);
				rxp->rxp_Port.mp_Node.ln_Name = rxp->rxp_Node.ln_Name = t;
				rxp->rxp_Port.mp_SigBit = rxsigbit;
				rxp->rxp_Port.mp_Flags = PA_SIGNAL;
				rxp->rxp_Port.mp_SigTask = FindTask(NULL);
				AddPort(&rxp->rxp_Port);

				rxp->rxp_Page = rxpage;   // set current context
				return(rxp);
			}
			FreePooled(pool,rxp,sizeof(struct RexxPort));
		}
	}
	FreeString(t);

	return NULL;
}


void
FreeRexxScript(struct RexxScript *rxs)
{
	if (!rxs)
		return;

	FreeString(rxs->rxs_Node.ln_Name);
	FreeString(rxs->rxs_Data);
	FreePooled(pool, rxs, sizeof(struct RexxScript));
}


ULONG
RunRexxScript(UBYTE type, STRPTR name)
{
	struct MsgPort *hostPort;
	struct RexxScript *rxs = NULL;
	struct RexxPort *rxp;
	struct RexxMsg *sendmsg;

	if (!name)
		return(RC_WARN);

	if (type == RXS_INTERN) {
		if (!(rxs = (APTR)FindTag(&rxpage->pg_Mappe->mp_RexxScripts,name))) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_INTERNAL_SCRIPT_NOT_FOUND_ERR),name);
			return(RC_WARN);
		}
	}
	if (!(rxp = MakeRexxPort())) {
		ErrorRequest(GetString(&gLocaleInfo, MSG_CREATE_SCRIPT_ENVIRONMENT_ERR));
		return(RC_FAIL);
	}
	sendmsg = rxp->rxp_Message;

	if (rxs) {
		sendmsg->rm_Args[0] = CreateArgstring(rxs->rxs_Data,strlen(rxs->rxs_Data));
		sendmsg->rm_Action = RXCOMM | RXFF_STRING;
	} else {
		sendmsg->rm_Args[0] = CreateArgstring(name,strlen(name));
		sendmsg->rm_Action = RXCOMM;
	}
	Forbid();
	if (!(hostPort = FindPort("REXX")) && !FindTask("RexxMaster")) {
#ifdef __amigaos4__
		SystemTags("SYS:System/RexxMast >Nil:", SYS_Input, NULL, SYS_Output, NULL, TAG_DONE);
#else
		Execute("SYS:System/RexxMast >Nil:", (BPTR)NULL, (BPTR)NULL);
#endif
		hostPort = FindPort("REXX");
	}
	if (hostPort)
		PutMsg(hostPort, (struct Message *)sendmsg);
	Permit();

	if (!hostPort) {
		ErrorRequest(GetString(&gLocaleInfo, MSG_REXX_PORT_NOT_FOUND_ERR));
		return RC_FAIL;
	}

	return RC_OK;
}


long
handleEvent(struct Page *page,BYTE type,long col,long row)
{
	struct Mappe *mp;
	char /*name[256],*/button[10];

	ignoreEvent = FALSE;
	if (!page || !((mp = page->pg_Mappe)->mp_Flags & MPF_SCRIPTS) || !mp->mp_Events[type].ev_Command || !(mp->mp_Events[type].ev_Flags & EVF_ACTIVE))
		return 0;

	if (type == EVT_LBUTTON || type == EVT_RBUTTON) {
		if (imsg.Code & IECODE_UP_PREFIX)
			strcpy(button,"up ");
		else
			strcpy(button,"down ");
	} else
		strcpy(button,"");

	ProcessAppCmd(page,mp->mp_Events[type].ev_Command);
							
	return (long)ignoreEvent;
}

		
void
handleRexx(struct RexxMsg *rxmsg)
{
	D(bug("handleRexx() - %s\n  action: %lx - port: %s - ext: %s\n",rxmsg->rm_Args[0],rxmsg->rm_Action,rxmsg->rm_CommAddr,rxmsg->rm_FileExt));
	D(bug("  task: %lx - replyport: %lx\n", rxmsg->rm_TaskBlock, rxmsg->rm_Node.mn_ReplyPort));

	rxmsg->rm_Result1 = processIntCmd(rxmsg->rm_Args[0]);

	if (rxmsg->rm_Action & RXFF_RESULT && !rxmsg->rm_Result1 && !rxmsg->rm_Result2)
		rxmsg->rm_Result2 = (long)CreateArgstring("ok", 2);
}


