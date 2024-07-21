/* windows related routines
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "windefs.h"
#include "images.h"
#include "classes.h"
#include "version.h"
#include <stdarg.h>
#ifdef __amigaos4__
	#include "reactionGUI.h" // RA
	#include <proto/gtdrag.h>
	#include <gadgets/button.h>

	// RA
	Object *winobj[NUM_WDT];
	uint32 wsigmask;
	struct ColumnInfo *ra_popup_ci = NULL; // ReAction ListBrowser's ColumnInfo PopUp

	extern void refreshColorRAWin(struct Window *ra_win); // handleprefs.c
	// RA

	extern char IgnTitle[100];		//ScreenTitle for ignition, must be unique for PubScreen handling
#endif

// for the version command
const STRPTR kVersion = VERSTRING;

int32 gWidth, gHeight;
struct SignalSemaphore gWindowSemaphore;

extern struct Image *toolImage[9];
extern struct RastPort *doublerp;
extern struct Layer_Info *gli;
extern STRPTR lasterror;
extern BOOL debug;

// RA
int32
DoRequestA(CONST_STRPTR t, CONST_STRPTR gads, APTR args)
{
	long rc;

	SetBusy(true, BT_APPLICATION);

	Object *requester = NewObject(RequesterClass, NULL, //"requester.class",
	                              REQ_Image,      REQIMAGE_DEFAULT,
	                              REQ_TitleText,  GetString(&gLocaleInfo,MSG_IGNITION_REQUEST_TITLE),
	                              REQ_BodyText,   t,
	                              REQ_VarArgs,    args,
	                              REQ_GadgetText, gads,
	                             TAG_DONE);

	rc = IDoMethod(requester, RM_OPENREQ, NULL, NULL, scr);
	DisposeObject(requester);
	SetBusy(false, BT_APPLICATION);

	return rc;
}
// RA
/*int32
DoRequestA(CONST_STRPTR t, CONST_STRPTR gads, APTR args)
{
	struct EasyStruct es = {sizeof(struct EasyStruct), 0, NULL, NULL, NULL};
	struct Window *win = NULL;
	bool   noscreen = false;
	long   rc = 0;

	SetBusy(true, BT_APPLICATION);
	if (!scr) {
		scr = LockPubScreen(NULL);
		noscreen = true;
	}
	if (!scr->FirstWindow)
		win = OpenWindowTags(NULL,WA_Top,0,WA_Width,1,WA_Height,1,WA_Flags,WFLG_BACKDROP | WFLG_BORDERLESS,WA_PubScreen,scr,TAG_END);

	es.es_Title = GetString(&gLocaleInfo, MSG_IGNITION_REQUEST_TITLE);
	es.es_TextFormat = t;
	es.es_GadgetFormat = gads;
	if(scr->FirstWindow)
		rc = EasyRequestArgs(scr->FirstWindow,&es,0,args);
	if (win)
		CloseWindow(win);
	if (noscreen) {
		UnlockPubScreen(NULL,scr);
		scr = NULL;
	}
	SetBusy(false, BT_APPLICATION);

	return rc;
}*/

#ifdef __amigaos4__
int32
DoRequest(CONST_STRPTR t, CONST_STRPTR gads, ...)
{
	va_list ap;
	STRPTR s;
	int32 rvalue;

	va_startlinear(ap, gads);
	s = va_getlinearva(ap, STRPTR);
	rvalue =  DoRequestA(t, gads, s);
	va_end(ap);
	
	return rvalue;
}
#else
long
DoRequest(CONST_STRPTR t, CONST_STRPTR gads, ...)
{
	return DoRequestA(t, gads, &gads + 1);
}
#endif

#ifdef __amigaos4__
void
ErrorRequest(CONST_STRPTR t, ...)
{
	STRPTR s;
	long len;

	if ((s = AllocPooled(pool, len = strlen(t) + 256)) != 0) {
		va_list args;

		va_start(args, t);
		vsprintf(s, t, args);
		va_end(args);

		FreeString(lasterror);
		lasterror = AllocString(s);

		FreePooled(pool, s, len);
	}
	if (rxquiet) {
		struct Node *ln;

		if (lasterror && (ln = AllocPooled(pool, sizeof(struct Node)))) {
			ln->ln_Name = AllocString(lasterror);
			MyAddTail(&errors, ln);
		}
	} else
	{
		va_list ap;
		STRPTR s;

		va_startlinear(ap, t);
		s = va_getlinearva(ap, STRPTR);
		DoRequestA(t,GetString(&gLocaleInfo, MSG_OK_GAD), s);
		va_end(ap);
	}
}
#else
void
ErrorRequest(CONST_STRPTR t, ...)
{
	STRPTR s;
	long len;

	if ((s = AllocPooled(pool, len = strlen(t) + 256)) != 0) {
		va_list args;

		va_start(args, t);
		vsprintf(s, t, args);
		va_end(args);

		FreeString(lasterror);
		lasterror = AllocString(s);

		FreePooled(pool, s, len);
	}
	if (rxquiet) {
		struct Node *ln;

		if (lasterror && (ln = AllocPooled(pool, sizeof(struct Node)))) {
			ln->ln_Name = AllocString(lasterror);
			MyAddTail(&errors, ln);
		}
	} else
		DoRequestA(t,GetString(&gLocaleInfo, MSG_OK_GAD),&t+1);
}
#endif

void PUBLIC
ErrorRequestA(REG(a0, CONST_STRPTR t), REG(a1, APTR args))
{
	STRPTR s;
	long   len;

	if ((s = AllocPooled(pool, len = strlen(t) + 256)) != 0)
	{
		/* MERKER: funktioniert nur auf dem 68k! */
		vsprintf(s, t, args);

		FreeString(lasterror);
		lasterror = AllocString(s);

		FreePooled(pool, s, len);
	}
	if (rxquiet)
	{
		struct Node *ln;

		if (lasterror && (ln = AllocPooled(pool, sizeof(struct Node))))
		{
			ln->ln_Name = AllocString(lasterror);
			MyAddTail(&errors, ln);
		}
	}
	else
		DoRequestA(t,GetString(&gLocaleInfo, MSG_OK_GAD),args);
}


void
ErrorOpenLibrary(STRPTR lib, STRPTR paket)
{
	if (!lib && !paket)
		return;

	if (lib)
	{
		if (paket)
			ErrorRequest(GetString(&gLocaleInfo, MSG_OPEN_LIBRARY_PACKAGE_ERR), lib, paket);
		else
			ErrorRequest(GetString(&gLocaleInfo, MSG_OPEN_LIBRARY_ERR), lib);
	}
	else if (paket)
		ErrorRequest(GetString(&gLocaleInfo, MSG_OPEN_FROM_PACKAGE_ERR), paket);
}


void
MakeLocaleLabels(const char *labels[], ULONG id, ...)
{
	int i = 0;

	va_list args;
	va_start(args, id);

	while (id != TAG_END) {
		labels[i++] = GetString(&gLocaleInfo, id);

		id = va_arg(args, ULONG);
	}

	va_end(args);
}


void
MakeShortCutString(STRPTR shortCuts, ULONG id, ...)
{
#ifdef DEBUG
	STRPTR start = shortCuts;
#endif
	va_list args;
	va_start(args, id);

	while (id != TAG_END) {
		CONST_STRPTR string = GetString(&gLocaleInfo, id);
		
		// find short cut
		while (string[0] && string[0] != '_')
			string++;

		if (string[0]) {
#ifdef DEBUG
			// check if the short cut already exists
			shortCuts[0] = '\0';
			if (strchr(start, ToLower(string[1])))
				ErrorRequest("Translation short cuts are the same:\n\"%s\" (id = %ld)", GetString(&gLocaleInfo, id), id);
#endif
			shortCuts[0] = ToLower(string[1]);
		} else {
			ErrorRequest("Catalog string misses a short cut:\n\"%s\" (id = %ld)", GetString(&gLocaleInfo, id), id);
			shortCuts[0] = '\0';
		}
		shortCuts++;

		id = va_arg(args, ULONG);
	}

	// properly NULL terminate the short cut string
	shortCuts[0] = '\0';

	va_end(args);
}


/** Wechselt zwischen Wartemauszeiger und normalem für das aktuelle Projekt (rxpage)
 *  oder für das ganze Programm. Im letzteren Fall sorgt es außerdem dafür, daß
 *  alle Fenster vor Mausklick geschützt sind (mittels Requester).
 *
 *  @param set TRUE für den Wartezustand, FALSE für normal
 *  @param type BT_APPLICATION für das ganze Programm, BT_PROJECT für das aktuelle
 *	Projekt.
 */

void
SetBusy(BOOL set, BYTE type)
{
	static int32 locked = 0;
	struct Window *bwin;
	struct winData *bwd;

	if (!scr)
		return;

	if (type == BT_APPLICATION)
	{
		if (set)
			locked++;
		else
			locked--;
		if (locked && !(locked == 1 && set))
			return;

		for (bwin = scr->FirstWindow; bwin; bwin = bwin->NextWindow)
		{
			if (bwin->UserPort == iport && (bwd = (struct winData *)bwin->UserData))
			{
				if (set)
				{
					InitRequester(&bwd->wd_Requester);
					Request(&bwd->wd_Requester,bwin);
					SetWindowPointer(bwin,WA_BusyPointer,TRUE,WA_PointerDelay,TRUE,TAG_END);
				}
				else
				{
					if (bwin->FirstRequest)
						EndRequest(&bwd->wd_Requester,bwin);
					SetWindowPointer(bwin,TAG_END);
				}
			}
		}
	}
	else if (rxpage && !locked)
	{
		if (set)
		{
			SetWindowPointer(rxpage->pg_Window,WA_BusyPointer,TRUE,WA_PointerDelay,TRUE,TAG_END);
		}
		else
		{
			SetWindowPointer(rxpage->pg_Window,TAG_END);
		}
	}
}


void
StandardNewSize(void (*create)(struct winData *,long,long))
{
	EraseRect(win->RPort,win->BorderLeft,win->BorderTop,win->Width-1-win->BorderRight,win->Height-1-win->BorderBottom);
	if ((gad = CreateContext(&wd->wd_Gadgets)) != 0)
		create(wd,win->Width,win->Height);
	AddGList(win,wd->wd_Gadgets,-1,-1,NULL);
	RefreshGadgets(wd->wd_Gadgets,win,NULL);
	GT_RefreshWindow(win,NULL);
}


void PUBLIC
DrawRect(REG(a0, struct RastPort *rp), REG(d0, long x), REG(d1, long y), REG(d2, long w), REG(d3, long h))
{
	Move(rp,x,y);
	Draw(rp,x+w,y);
	Draw(rp,x+w,y+h);
	Draw(rp,x,y+h);
	Draw(rp,x,y+1);
}


struct Gadget *
PageGadget(struct Gadget *gad, long num)
{
	if (!gad)
		return NULL;

	for (;gad && (gad->GadgetID != num);gad = gad->NextGadget);
	while (gad && gad->NextGadget && gad->NextGadget->GadgetID == num)
		gad = gad->NextGadget;

	return gad;
}


struct Gadget *
GadgetAddress(struct Window *win, long num)
{
	struct Gadget *gad;

	if (!win)
		return NULL;

	for (gad = win->FirstGadget;gad && (gad->GadgetID != num);gad = gad->NextGadget);
	while (gad && gad->NextGadget && gad->NextGadget->GadgetID == num)
		gad = gad->NextGadget;

   return gad;
}


void
DisableGadget(struct Gadget *gad, struct Window *win, bool disable)
{
	LONG tags[] = {GA_Disabled, false, TAG_END};

	if (!gad)
		return;

	tags[1] = disable;

	if (gad->GadgetType & 0x100)
		GT_SetGadgetAttrsA(gad, win, NULL, (struct TagItem *)tags);
	else
		SetGadgetAttrsA(gad, win, NULL, (struct TagItem *)tags);
}


long
CountGadToolGads(struct Window *win)
{
	struct Gadget *gad;
	long   i;

	for (i = 0,gad = win->FirstGadget;gad;gad = gad->NextGadget)
	{
		if (gad->GadgetType & 0x100)
			i++;
	}
	return i;
}


struct Gadget *
CreateImgGadget(struct Image *up,struct Image *down,long h,struct Gadget *gad,struct NewGadget *ng,BOOL disabled)
{
	long flags;

	ng->ng_Width = boxwidth;
	ng->ng_Height = fontheight+h;
	ng->ng_GadgetText = NULL;
	flags = ng->ng_Flags;
	ng->ng_Flags = 0;
	if ((gad = CreateGadgetA(GENERIC_KIND, gad, ng, NULL)) != 0)
	{
		gad->GadgetType |= GTYP_BOOLGADGET;
		gad->Flags |= GFLG_GADGIMAGE | GFLG_GADGHIMAGE | (disabled ? GFLG_DISABLED : 0);
		gad->Activation |= GACT_IMMEDIATE | (h == 3 ? GACT_TOGGLESELECT : GACT_RELVERIFY);
		gad->GadgetRender = up;
		gad->SelectRender = down;
	}
	ng->ng_Flags = flags;
	return(gad);
}


struct Gadget *
CreateToolGadget(struct winData *wd, long id, struct Gadget *gad)
{
	return CreateImgGadget(toolImage[id], toolImage[id], 3, gad, &ngad, false);
}


struct Gadget *
CreatePopGadget(struct winData *wd, struct Gadget *gad, bool disabled)
{
	ngad.ng_Width = boxwidth;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetText = NULL;

	return NewObj(wd,WOT_GADGET,buttonclass,NULL,		GA_Left, 		ngad.ng_LeftEdge,
												 		GA_Top,			ngad.ng_TopEdge,
												 		GA_Height,  	fontheight + 4,
												 		GA_Image,		popImage,
												 		GA_Disabled,	disabled,
												 		GA_Previous,	gad,
												 		GA_Immediate,	TRUE,
												 		GA_RelVerify,	TRUE,
												 		GA_ID,			ngad.ng_GadgetID,
												 		TAG_END);
	//return(CreateImgGadget(popImage,popImage,4,gad,&ngad,disabled));
}


void
DrawPointSliderValue(struct RastPort *rp,struct Gadget *gad,WORD komma)
{
	char t[16];
	int  w = TLn(GetString(&gLocaleInfo, MSG_AUTO_VALUE_GAD)),tw;
	int  y = gad->TopEdge+1;

	if (komma >= 0)
		sprintf(t,"%ld",komma);
	else
		strcpy(t,GetString(&gLocaleInfo, MSG_AUTO_VALUE_GAD));
	itext.IText = t;
	itext.DrawMode = JAM2;
	tw = IntuiTextLength(&itext);
	PrintIText(rp,&itext,gad->LeftEdge-8-tw,y);
	if (komma >= 0)
		EraseRect(rp,gad->LeftEdge-8-w,y,gad->LeftEdge-9-tw,y+fontheight+2);
}


void
DrawField(struct RastPort *rp, WORD x, WORD y)
{
	struct Image *obj;

	if ((obj = (struct Image *)NewObject(NULL,"frameiclass",IA_Width,	 boxwidth << 1,
										   IA_Height,	fontheight + 4,
										   IA_Recessed,  TRUE,
										   IA_FrameType, FRAME_BUTTON,
										   TAG_END)) != 0)
	{
		DrawImage(rp, obj, x, y);
		DisposeObject((Object *)obj);
	}
}


void
DrawColorField(struct RastPort *rp, struct Gadget *gad, ULONG col, BOOL appColors)
{
	short  x,y;

	if (gad == NULL)
		return;

	DrawField(rp, x = gad->LeftEdge - 2 * boxwidth, y = gad->TopEdge);

	if (appColors)
		SetHighColor(rp, col);
	else
		SetAPen(rp, col);

	RectFill(rp, x + 4, y + 2, x + (boxwidth << 1) - 5, y + fontheight + 1);
}


void
DrawPatternField(struct RastPort *rp, struct Gadget *gad, ULONG col, BYTE pattern)
{
	short  x,y;

	if (gad == NULL)
		return;

	DrawField(rp, x = gad->LeftEdge - 2 * boxwidth, y = gad->TopEdge);
	SetColors(rp, col, 0xffffff);

	SetPattern(rp, pattern, 0, 0);
	RectFill(rp, x + 3, y + 2, x + (boxwidth << 1) - 4, y + fontheight + 1);

	SetAfPt(rp, NULL, 0);
	SetBPen(rp, 0);
}


void
DrawGroupBorder(struct RastPort *rp, CONST_STRPTR t,long x, long y, long w, long h)
{
	struct Image *obj;

	if ((obj = (struct Image *)NewObject(NULL,"frameiclass",IA_Top,	   t ? (fontheight >> 1)-1 : 0,
										   IA_Width,	 w,
										   IA_Height,	h-(t ? (fontheight >> 1)-1 : 0),
										   IA_Recessed,  1,
										   IA_EdgesOnly, 1,
										   TAG_END)) != 0)
	{
		DrawImage(rp, obj, x, y);
		DisposeObject((Object *)obj);
	}
	if ((obj = (struct Image *)NewObject(NULL,"frameiclass",IA_Top,	   t ? (fontheight >> 1) : 1,
										   IA_Left,	  1,
										   IA_Width,	 w-2,
										   IA_Height,	h-(t ? (fontheight >> 1)+1 : 2),
										   IA_EdgesOnly, 1,
										   TAG_END)) != 0)
	{
		DrawImage(rp, obj, x, y);
		DisposeObject((Object *)obj);
	}
	if (t)
	{
		itext.FrontPen = 1;
		itext.ITextFont = scr->Font;
		itext.IText = t;  itext.DrawMode = JAM2;
		PrintIText(rp,&itext,x+8,y);
	}
}


void
DrawDithRect(struct RastPort *rp,long x1,long y1,long x2,long y2)
{
	rp->AreaPtrn = (unsigned short *)&dithPtrn;
	rp->AreaPtSz = 1;
	RectFill32(rp,x1,y1,x2,y2);
	rp->AreaPtrn = NULL;
	rp->AreaPtSz = 0;
}


#ifdef __amigaos4__
APTR VARARGS68K NewObj(struct winData *wd,short type,APTR cl,STRPTR name,...);// ULONG tag1,...);
APTR NewObj(struct winData *wd,short type,APTR cl,STRPTR name,...)// ULONG tag1,...)
#else
APTR NewObj(struct winData *wd,short type,APTR cl,STRPTR name,ULONG tag1,...)
#endif
{
	struct winObj *wo;
	APTR   *obj;

#ifdef __amigaos4__
	va_list ap;
	const struct TagItem *tags;

	va_startlinear(ap, name);
	tags = va_getlinearva(ap, struct TagItem *);

	if ((obj = (APTR)NewObjectA(cl, name, tags)) != 0)
	{
		if ((wo = AllocPooled(pool, sizeof(struct winObj))) != 0)
		{
			wo->wo_Obj = obj;
			wo->wo_Type = type;
			MyAddTail(&wd->wd_Objs, wo);
		}
	}
#else
	if ((obj = (APTR)NewObjectA(cl, name, (struct TagItem *)&tag1)) != 0)
	{
		if ((wo = AllocPooled(pool, sizeof(struct winObj))) != 0)
		{
			wo->wo_Obj = obj;
			wo->wo_Type = type;
			MyAddTail(&wd->wd_Objs, wo);
		}
	}
#endif
	return obj;
}


uint32
CountAppWindows(void)
{
	struct Window *win;
	uint32 count;

	if (!scr || !iport)
		return 0L;

	for (win = scr->FirstWindow,count = 0L;win;win = win->NextWindow)
	{
		if (win->UserPort == iport)
			count++;
	}
	return count;
}


struct Window *
GetAppWindow(long type)
{
	struct Window *win;
	if (!scr || !iport)
		return NULL;

	if (type != WDT_ANY)
	{
		for (win = scr->FirstWindow;win;win = win->NextWindow)
		{
			if (win->UserPort == iport && ((struct winData *)win->UserData)->wd_Type == type)
				return win;
		}
	}
	else
	{
		for (win = scr->FirstWindow;win;win = win->NextWindow)
		{
			if (win->UserPort == iport)
				return win;
		}
	}
	return NULL;
}


// RA
struct Window *GetActiveAppWindow(void)
{
	struct Window *win;
	int32 act;

	if(!scr || !iport)
		return NULL;

	for(win=scr->FirstWindow; win; win=win->NextWindow)
	{
		if(win->UserPort == iport)
		{
			GetWindowAttr( win, WA_Activate,&act, sizeof(act) );
//DBUG("GetActiveAppWindow() win=0x%08lx   act=%ld\n", win,act);
			if(act)
				return win;
		}
	}
	return NULL;
}
// RA


struct Window *
GetAppWinTitle(STRPTR t)
{
	struct Window *win;

	for (win = scr->FirstWindow;win;win = win->NextWindow)
	{
		if ((win->UserPort == iport) && (!strcmp(win->Title,t)))
			return win;
	}
	return NULL; /* suppress compiler warning */
}


APTR
makeLayerClip(struct Layer *l,long x,long y,long w,long h)
{
	struct Rectangle rect;
	struct Region *region;

	rect.MinX = x;  rect.MinY = y;  rect.MaxX = x+w;  rect.MaxY = y+h;
	region = NewRegion();
	OrRectRegion(region,(CONST struct Rectangle *)&rect);

	return InstallClipRegion(l, (CONST struct Region *)region);
}


void
freeLayerClip(struct Layer *l)
{
	struct Region *region;

	if ((region = InstallClipRegion(l, NULL)) != 0)
		DisposeRegion(region);
}


APTR
makeClip(struct Window *win,long x,long y,long w,long h)
{
	return makeLayerClip(win->WLayer,x,y,w,h);
}


void
freeClip(struct Window *win)
{
	freeLayerClip(win->WLayer);
}


struct Gadget *
MakeBorderScroller(struct winData *wd)
{
	struct Gadget *gad[6];

	if ((gad[0] = (struct Gadget *)NewObj(wd,WOT_SCROLL,NULL,"buttongclass",
												GA_ID,           PROPGAD_RIGHT_ID,
												GA_Image,        rightImg,
												GA_RelBottom,    1-leftImg->Height,
												GA_RelRight,     1-leftImg->Width-upImg->Width,
												GA_BottomBorder, TRUE,
												GA_UserData,     42,
												ICA_TARGET,      ICTARGET_IDCMP,
											TAG_END)) != 0)
	{
		if ((gad[1] = (struct Gadget *)NewObj(wd,WOT_SCROLL,NULL,"buttongclass",GA_ID,		PROPGAD_LEFT_ID,
																			   GA_Image,	 leftImg,
																			   GA_RelBottom, 1-leftImg->Height,
																			   GA_RelRight,  1-2*leftImg->Width-upImg->Width,
																			   GA_Previous,  gad[0],
																			   GA_BottomBorder,TRUE,
																			   GA_UserData,  42,
																			   ICA_TARGET,   ICTARGET_IDCMP,
																			   TAG_END)) != 0)
		{
			if ((gad[2] = (struct Gadget *)NewObj(wd,WOT_SCROLL,NULL,"buttongclass",GA_ID,		PROPGAD_DOWN_ID,
																				   GA_Image,	 downImg,
																				   GA_RelBottom, 1 - leftImg->Height - downImg->Height,
																				   GA_RelRight,  1-downImg->Width,
																				   GA_Previous,  gad[1],
																				   GA_RightBorder,TRUE,
																				   GA_UserData,  42,
																				   ICA_TARGET,   ICTARGET_IDCMP,
																				   TAG_END)) != 0)
			{
				if ((gad[3] = (struct Gadget *)NewObj(wd,WOT_SCROLL,NULL,"buttongclass",GA_ID,		PROPGAD_UP_ID,
																					   GA_Image,	 upImg,
																					   GA_RelBottom, 1 - leftImg->Height - downImg->Height - upImg->Height,
																					   GA_RelRight,  1-upImg->Width,
																					   GA_Previous,  gad[2],
																					   GA_UserData,  42,
																					   GA_RightBorder,TRUE,
																					   ICA_TARGET,   ICTARGET_IDCMP,
																					   TAG_END)) != 0)
				{
					if ((gad[4] = (struct Gadget *)NewObj(wd,WOT_SCROLL,NULL,"propgclass",GA_ID,		 PROPGAD_VERT_ID,
																						 GA_Top,		scr->BarHeight+2,
																						 GA_Width,	  upImg->Width-8,
																						 GA_RelRight,   5-upImg->Width,
																						 GA_RelHeight,  -scr->BarHeight-2-upImg->Height*3,
																						 GA_RightBorder,TRUE,
																						 GA_Previous,   gad[3],
																						 GA_RelVerify,  TRUE,
																						 PGA_Borderless,TRUE,
																						 PGA_NewLook,   TRUE,
																						 PGA_Total,	 0,
																						 GA_UserData,   42,
																						 ICA_TARGET,	ICTARGET_IDCMP,
																						 TAG_END)) != 0)
					{
						if ((gad[5] = (struct Gadget *)NewObj(wd,WOT_SCROLL,NULL,"propgclass",GA_ID,		 PROPGAD_HORIZ_ID,
																							 GA_Top,		scr->BarHeight+2,
																							 GA_RelBottom,  3-leftImg->Height,
																							 GA_RelWidth,   -4-scr->WBorLeft-leftImg->Width*2-upImg->Width,
																							 GA_Height,	 leftImg->Height-4,
																							 GA_Left,	   scr->WBorLeft-1,
																							 GA_BottomBorder,TRUE,
																							 GA_Previous,   gad[4],
																							 GA_RelVerify,  TRUE,
																							 PGA_Freedom,   FREEHORIZ,
																							 PGA_Borderless,TRUE,
																							 PGA_NewLook,   TRUE,
																							 PGA_Total,	 0,
																							 GA_UserData,   42,
																							 ICA_TARGET,	ICTARGET_IDCMP,
																							 TAG_END)) != 0)
								return gad[0];
					}
				}
			}
		}
	}
	return NULL;
}


bool
IsPrefsWindow(UBYTE type)
{
	return (bool)(type >= WDT_PREFS && type <= WDT_ENDPREFS);
}


/** Entfernt alle Gadgets einer Gadgetliste, die nicht vom
 *  System oder GadTools stammen.
 *
 *  @param gad Zeiger auf den Listenanfang
 */

void
RemoveGadgets(struct Gadget **firstgad,BOOL border)
{
	struct Gadget *sgad,*lastgad = NULL,*gad = *firstgad;

	while(gad)
	{
		sgad = gad->NextGadget;
		if (!(gad->GadgetType & (0x100 | GTYP_SYSGADGET)) && (border || !(gad->Activation & (GACT_LEFTBORDER | GACT_RIGHTBORDER | GACT_TOPBORDER | GACT_BOTTOMBORDER))))
		{
			if (lastgad)
				lastgad->NextGadget = sgad;
			else
				*firstgad = sgad;
		}
		else
			lastgad = gad;
		gad = sgad;
	}
}


#ifndef __amigaos4__
void
StripIntuiMessages(struct MsgPort *mp, struct Window *win)
{
	struct IntuiMessage *msg;
	struct Node *succ;

	msg = (struct IntuiMessage *)mp->mp_MsgList.lh_Head;

	while ((succ = msg->ExecMessage.mn_Node.ln_Succ) != 0)
	{
		if (msg->IDCMPWindow == win)
		{
			MyRemove(msg);
			ReplyMsg((struct Message *)msg);
		}
		msg = (struct IntuiMessage *)succ;
	}
}
#endif

// RA
BOOL isRAwinobjEmpty(void) // are ReAction windows(.class) still open?
{
	BOOL empty_wo = TRUE;
	int16 i = NUM_WDT;

	while(i)
	{
		i--;
		if(winobj[i])
		{
			empty_wo = FALSE;
			break;
		}
	}
DBUG("isRAwinobjEmpty()=%ld (%ld)\n", empty_wo,i);
	return(empty_wo);
}
// RA

void
CloseAppWindow(struct Window *win,BOOL cleanUp)
{
	struct Gadget **pages;
	struct winData *wd;
	struct winObj *wo;
	STRPTR title;
	long   type;

	if (!win)
		return;

	ObtainSemaphore(&gWindowSemaphore);

	wd = (struct winData *)win->UserData;
	if (win->FirstRequest)
		EndRequest(&wd->wd_Requester, win);

	if (wd->wd_CleanUp)
		wd->wd_CleanUp(win, cleanUp);

	RemLockNode(wd->wd_Lock);
	Forbid();
	StripIntuiMessages(win->UserPort, win);
	win->UserPort = NULL;

	ModifyIDCMP(win, 0L);
	Permit();

	gad = wd->wd_Gadgets;
	RemoveGadgets(&win->FirstGadget, TRUE);

	while ((wo = (struct winObj *)MyRemTail(&wd->wd_Objs)) != 0)
	{
		DisposeObject(wo->wo_Obj);
		FreePooled(pool, wo, sizeof(struct winObj));
	}

	// save window position & bounds
	prefs.pr_WinPos[type = wd->wd_Type].Left = win->LeftEdge;
	prefs.pr_WinPos[type].Top = win->TopEdge;
	
	if (gCreateWinData[type - 1].cwd_ResizableGadgets) {
		prefs.pr_WinPos[type].Width = win->Width;
		prefs.pr_WinPos[type].Height = win->Height;
	}

	//Crash test freeClip(win);
	title = win->Title;
	pages = wd->wd_Pages;
	FreePooled(pool, wd, sizeof(struct winData));

	ClearMenuStrip(win);

	// RA
		if( (type>0 && type<=WDT_SETNAME? !gCreateWinData[type-1].cwd_IDCMP : 0) )
	{
DBUG("CloseAppWindow():WM_CLOSE RA (0x%08lx)\n", type);
		IDoMethod(winobj[type], WM_CLOSE, NULL);
		winobj[type] = NULL;
		if(isRAwinobjEmpty() == TRUE) wsigmask = 0;
	}
	else
	{
DBUG("CloseAppWindow():CloseWindow nonRA\n");
		CloseWindow(win);
	}
	// RA
	//CloseWindow(win);

	if (IsPrefsWindow(type))
		FreeString(title);

	FreeGadgets(gad);
	if (pages)
		FreeVec(pages);

	ReleaseSemaphore(&gWindowSemaphore);
}


void
DrawRequesterBorder(struct Window *win)
{
	struct Image *obj;

	SetAPen(win->RPort,2);
	DrawDithRect(win->RPort,win->BorderLeft,win->BorderTop,win->Width-1-win->BorderRight,win->Height-1-win->BorderBottom);
	RefreshGadgets(win->FirstGadget,win,NULL);

	if ((obj = (struct Image *)NewObject(NULL, "frameiclass",
				IA_Width,		win->Width - rborder - lborder,
				IA_Height,		win->Height - win->BorderTop - win->BorderBottom - 14 - fontheight,
				IA_Recessed,	TRUE,
				TAG_END)) != 0)
	{
		DrawImage(win->RPort, obj, lborder, win->BorderTop + 4);
		DisposeObject((Object *)obj);
	}
}


void
DisposeProgressBar(struct ProgressBar *pb)
{
	if (!pb)
		return;

	FreePooled(pool, pb, sizeof(struct ProgressBar));
}


struct ProgressBar *
NewProgressBar(struct Window *win, long x, long y, long w, long h)
{
	struct ProgressBar *pb;

	if ((pb = AllocPooled(pool, sizeof(struct ProgressBar))) != 0)
	{
		pb->pb_RPort = win->RPort;
		pb->pb_X = x + 2;
		pb->pb_Y = y + 1;
		pb->pb_Width = w - 5;
		pb->pb_Height = h - 3;
		DrawBevelBox(win->RPort, x, y, w, h, GT_VisualInfo, vi, GTBB_Recessed, TRUE, TAG_END);
		EraseRect(win->RPort, x + 2, y + 1, x + w - 3, y + h - 2);
	}
	return pb;
}

/*void UpdateFuelGauge(struct Window *w, CONST_STRPTR t, uint8 lvl)
{
Printf("[0x%08lx]'%s': %ld\n",w,t,lvl);
	if(lvl > 100) lvl = 100;
	RefreshSetGadgetAttrs(GAD(OID_ABOUT_FOOTER), w, NULL, GA_Text,t, FUELGAUGE_Level,lvl, TAG_DONE);
}*/
void
UpdateProgressBar(struct ProgressBar *pb, CONST_STRPTR t, float p)
{
	struct RastPort *rp;
	long   x,y,w,h;
	int    diff;

	if (!pb)
		return;

	rp = pb->pb_RPort;

	if (t == (APTR)~0L)
		t = pb->pb_Text;

	if (p > 1.0)
		p = 1.0;
	else if (p < 0.0)
		p = 0.0;

	w = (long)(pb->pb_Width*p+0.5);
	diff = zstrcmp((STRPTR)t,pb->pb_Text);

	if (w > pb->pb_Width)
		w = pb->pb_Width;

	if (w == pb->pb_BarWidth && !diff)
		return;

	SetAPen(rp,3);
	x = pb->pb_X;  y = pb->pb_Y;  h = y+pb->pb_Height;

	if (diff)
		RectFill(rp,x,y,x+pb->pb_BarWidth,h);
	if (pb->pb_BarWidth < w)
		RectFill(rp,x+pb->pb_BarWidth+1,y,x+w,h);
	if (diff || pb->pb_BarWidth > w)
		EraseRect(rp,x+w+1,y,x+pb->pb_Width,h);

#ifdef __amigaos4__
	Strlcpy(pb->pb_Text,t,PROGRESSBAR_TEXTLEN);
#else
	stccpy(pb->pb_Text,t,PROGRESSBAR_TEXTLEN);
#endif
	pb->pb_BarWidth = w;

	if (!t)
		return;

	itext.IText = t;
	itext.ITextFont = scr->Font;
	itext.FrontPen = 2;
	itext.DrawMode = JAM1;
	PrintIText(rp, &itext, x + 3, y + 1);
}


struct TextFont *
OpenBitmapFont(struct TextAttr *textAttr)
{
	static const STRPTR fallback[] = {"Helvetica.font", "XHelvetica.font", "Courier.font", "XCourier.font", NULL};
	struct TextFont *font;
	int loop = -1;

	do {
		font = OpenDiskFont(textAttr);
		if (font == NULL) {
			// try fall back fonts
			textAttr->ta_Name = fallback[++loop];
		}
	} while (font == NULL && fallback[loop] != NULL);
	
	return font;
}


/*void
CreateRAInfo(struct TextAttr *txtattr, CONST_STRPTR txt)
{
	if(txt == NULL)
	{
		OBJ(OID_ABOUT_FOOTER) = NewObject(ButtonClass, NULL, //"button.gadget",
			GA_ID,        OID_OK,//9
			GA_RelVerify, TRUE,
			GA_Text,      "_OK",
		TAG_DONE);
	}
	else
	{
		OBJ(OID_ABOUT_FOOTER) = NewObject(FuelGaugeClass, NULL, //"fuelgauge.gadget",
			GA_ID,   0,
			//GA_RelVerify, TRUE,
			GA_Text, txt,
			FUELGAUGE_Min,   0,
			FUELGAUGE_Max,   100,
			FUELGAUGE_Level, 0,
			FUELGAUGE_Ticks, 0,
			FUELGAUGE_Percent, FALSE,
		TAG_DONE);
	}

	winobj[WDT_INFO] = NewObject(WindowClass, NULL, //"window.class",
					WA_Title,"AboutProgress",
					WA_DragBar,      TRUE,
					WA_CloseGadget,  txt? FALSE : TRUE,
					WA_SizeGadget,   FALSE,
					WA_DepthGadget,  TRUE,
					WA_Activate,     TRUE,
					//WA_PubScreen,    scr,
					//WA_IDCMP,IDCMP_MENUPICK, // makes main menu options work
					//(prefs.pr_Flags&PRF_SIMPLEWINDOWS)? WA_SimpleRefresh : TAG_IGNORE, TRUE,
					//WINDOW_SharedPort, iport,
					WINDOW_Position,   WPOS_CENTERSCREEN,
					WINDOW_Layout, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						LAYOUT_SpaceOuter,  TRUE,
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						//LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
						//LAYOUT_BevelStyle,  BVS_GROUP,
// LOGO & ABOUT TEXT
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						LAYOUT_HorizAlignment, LALIGN_CENTER,
						//LAYOUT_VertAlignment, LALIGN_TOP,
						//LAYOUT_SpaceOuter,  TRUE,
// 						LAYOUT_AddChild, NewObject(ButtonClass, NULL, // "button.gadget",
//							GA_ReadOnly, TRUE,
//							BUTTON_RenderImage, &logoOriginalImage,
//							BUTTON_BevelStyle,  BVS_NONE,
//							BUTTON_Transparent, TRUE,
//						TAG_DONE),
						LAYOUT_AddChild, NewObject(LabelClass, NULL, //"label.image",
							//IA_Font, txtattr,
							LABEL_DisposeImage, TRUE,
							LABEL_Image,        logoImage,
						TAG_DONE),
						CHILD_WeightedHeight, 0,
						LAYOUT_AddImage, NewObject(LabelClass, NULL, //"label.image",
							IA_Font, txtattr,
							LABEL_Underscore,    0,
							LABEL_Justification, LJ_RIGHT,
							//LABEL_VerticalSpacing, 5,
							LABEL_SoftStyle,     FSF_BOLD,
							LABEL_Text, "Version 1.00 beta7\n19.11.2019",
						TAG_DONE),
						CHILD_WeightedWidth, 100,
					LAYOUT_AddImage, NewObject(LabelClass, NULL, //"label.image",
						IA_Font, txtattr,
						LABEL_Underscore, 0,
						LABEL_Text, "Copyright © 1996-20189 pinc Software.\n",
						LABEL_Text, "All rights reserved.\n\n",
						LABEL_Text, " Axel Dörfler\n",
						LABEL_Text, " www.pinc-software.de\n info@pinc-software.de\n\n",
						LABEL_Text, " Open Source (AOS4-ReAction version)\n",
						LABEL_Text, " Achim Pankalla\n www.os4welt.de",
					TAG_DONE),
				TAG_DONE), // END LOGO & ABOUT TEXT
				CHILD_WeightedHeight, 0,
// PINC IMAGE
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_VertAlignment, LALIGN_BOTTOM,
					LAYOUT_AddChild, NewObject(ButtonClass, NULL, // "button.gadget",
						GA_ReadOnly, TRUE,
						BUTTON_RenderImage, &pincOriginalImage,
						//BUTTON_BitMap,,
						BUTTON_BevelStyle,  BVS_NONE,
						BUTTON_Transparent, TRUE,
					TAG_DONE),
					CHILD_WeightedHeight, 0,
				TAG_DONE),  // END PINC IMAGE
			//CHILD_WeightedHeight, 0,
			TAG_DONE),
// [BUTTONS]
			LAYOUT_AddChild, OBJ(OID_ABOUT_OBJ) = NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation,    LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,     BVS_SBAR_VERT,
				LAYOUT_SpaceOuter,     TRUE,
				LAYOUT_HorizAlignment, LALIGN_CENTER,
				LAYOUT_AddChild, OBJ(OID_ABOUT_FOOTER),
			TAG_DONE),
			CHILD_WeightedHeight, 0,

					TAG_DONE),
	TAG_END);
}*/

void
CreateInfo(struct Window *win)
{
	struct TextFont *tf;
	struct TextAttr ta = {"Times.font", 11, 0, 0};
	uint32 border = 193;
	char buffer[128];

	DrawRequesterBorder(win);
	SetAPen(win->RPort, 2);	// white right border
	RectFill(win->RPort, border + lborder, barheight + 6, win->Width - 3 - rborder, win->Height - 13 - fontheight - bborder);
	SetAPen(win->RPort, 1); // black left area
	RectFill(win->RPort, 2 + lborder, barheight + 5, border + lborder, win->Height - 13 - fontheight - bborder);
	DrawImage(win->RPort, logoImage, 8 + lborder, barheight + 10);
	DrawImage(win->RPort, pincImage, border + 5 + lborder, win->Height - 87 - fontheight - bborder);

	if ((tf = OpenBitmapFont(&ta)) != 0)
		itext.ITextFont = &ta;

	itext.FrontPen = 2;  itext.BackPen = 1;
	sprintf(buffer, GetString(&gLocaleInfo, MSG_VERSION_INFO), VERSION);
	itext.IText = buffer;  PrintIText(win->RPort, &itext, border - 5 - lborder - IntuiTextLength(&itext), barheight + 55);
	itext.IText = INFODATE;  PrintIText(win->RPort, &itext, border - 5 - lborder - IntuiTextLength(&itext), barheight + 66);
	itext.FrontPen = 0;
	itext.IText = IGNITION_COPYRIGHT;  PrintIText(win->RPort,&itext,6+lborder,barheight+87);
	itext.IText = GetString(&gLocaleInfo, MSG_ALL_RIGHTS_RESERVED_INFO);  PrintIText(win->RPort,&itext,6+lborder,barheight+100);
	itext.IText = "Axel Dörfler";  PrintIText(win->RPort,&itext,16+lborder,barheight+117);
	itext.IText = "http://www.pinc-software.de";  PrintIText(win->RPort,&itext,16+lborder,barheight+129);
	itext.IText = "eMail: info@pinc-software.de";  PrintIText(win->RPort,&itext,16+lborder,barheight+141);
#ifdef __amigaos4__
	itext.IText = "Open Source (AOS4-ReAction version)"; PrintIText(win->RPort, &itext, 16 + lborder, barheight + 170); // RA
	//itext.IText = "Open Source AmigaOS4 Version"; PrintIText(win->RPort, &itext, 16 + lborder, barheight + 170);
	itext.IText = "Achim Pankalla"; PrintIText(win->RPort, &itext, 16 + lborder, barheight + 182);
	itext.IText = "www.os4welt.de"; PrintIText(win->RPort, &itext, 16 + lborder, barheight + 194);
#else
	itext.IText = "Open Source Version"; PrintIText(win->RPort, &itext, 6 + lborder, barheight + 190);
#endif
	CloseFont(tf);
	itext.FrontPen = 1;  itext.BackPen = 0;
	itext.ITextFont = scr->Font;
}


void
SetNamePosition(long *x, long *y)
{
	long hei = barheight + fontheight + 12;

	if (wd->wd_Type == WDT_PROJECT && imsg.Class != IDCMP_MENUPICK) {
		*x = imsg.MouseX + win->LeftEdge - (scr->Width >> 2);
		*y = imsg.MouseY + win->TopEdge - (hei >> 1);
	} else {
		*x = scr->Width >> 2;
		*y = (scr->Height - hei) >> 1;
	}
}


void
RefreshAppWindow(struct Window *win, struct winData *wd)
{
	struct RastPort *rp = win->RPort;
	long   x,y;

	switch (wd->wd_Type) {
		case WDT_PREFSCREEN:
			break;
		case WDT_PREFDISP:
			break;
		case WDT_PREFFORMAT:
			// RA
			RefreshSetGadgetAttrs(GAD(OID_F_COPY), win, NULL, GA_Disabled,TRUE, TAG_DONE);
			RefreshSetGadgetAttrs(GAD(OID_F_DEL), win, NULL, GA_Disabled,TRUE, TAG_DONE);
			RefreshSetGadgetAttrs(GAD(OID_F_GROUP_PROP), win, NULL, GA_Disabled,TRUE, TAG_DONE);
			RefreshSetGadgetAttrs(GAD(OID_F_GROUP_OPTS), win, NULL, GA_Disabled,TRUE, TAG_DONE);
			// RA
			break;
		case WDT_PREFFILE:
			break;
		case WDT_PREFTABLE:
			break;
		case WDT_PREFCOLORS:
			break;
		case WDT_DEFINECMD:
			break;
		case WDT_OBJECT:
		{
			struct gGadget *gg;
			long lines = max((long)wd->wd_ExtData[2], 4);

			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_POSITION_BORDER),
				lborder, y = barheight + fontheight + 10,
				(x = (long)wd->wd_ExtData[3]) - 4 - lborder,
				fontheight + 4 + (fontheight + 7) * lines);
			if (wd->wd_ExtData[2]) {
				DrawGroupBorder(rp, GetString(&gLocaleInfo, MSG_MISC_BORDER),
					x, y, win->Width - x - rborder,
					fontheight + 4 + (fontheight + 7) * lines);
			}

			for (gg = (APTR)((struct List *)wd->wd_ExtData[4])->lh_Head;
					gg->gg_Node.mln_Succ; gg = (APTR)gg->gg_Node.mln_Succ) {
				if (gg->gg_Type == GIT_PEN) {
					struct gInterface *gi;

					for (gi = ((struct gObject *)wd->wd_Data)->go_Class
							->gc_Interface; gi && gi->gi_Tag; gi++) {
						if (gg->gg_Tag == gi->gi_Tag) {
							itext.IText = GetGLabel(gi);
							PrintIText(rp, &itext, gg->gg_Gadget->LeftEdge - 8
									- 2 * boxwidth - IntuiTextLength(&itext),
								gg->gg_Gadget->TopEdge + 3);
						}
					}
				}
			}
			break;
		}
		case WDT_PREVIEW:
		{
			struct gObject *go = wd->wd_ExtData[0];
			struct gClass *gc;

			SetAPen(rp, FindColor(scr->ViewPort.ColorMap,
				0xffffffff, 0xffffffff, 0xffffffff, -1));
			RectFill(rp, win->BorderLeft, win->BorderTop,
				x = win->Width - 2 - win->BorderRight,
				y = win->Height - 2 - win->BorderBottom);
			EraseRect(rp, x + 1, win->BorderTop, x + 1, y);
			EraseRect(rp, win->BorderLeft, y + 1, x, y + 1);

			if (go && (gc = go->go_Class)->gc_Draw) {
				struct gBounds gb;

				SetGRastPort(rp);

				gb.gb_Left = win->BorderLeft;
				gb.gb_Right = x;
				gb.gb_Top = win->BorderTop;
				gb.gb_Bottom = y;
				makeClip(win, gb.gb_Left, gb.gb_Top, gb.gb_Right, gb.gb_Bottom);
				gc->gc_Draw(NULL, gDPI, grp, gc, go, &gb);
				freeClip(win);
			}
			break;
		}
		case WDT_BORDER:
			if ((gad = GadgetAddress(win, 1)) != 0) {
				DrawGroupBorder(rp, GetString(&gLocaleInfo, MSG_PRESETS_BORDER), lborder, win->BorderTop + 3, win->Width - lborder - rborder, gad->TopEdge - win->BorderTop - 6);
				DrawBevelBox(rp, lborder, gad->TopEdge + 4 + fontheight, win->Width - lborder - rborder, fontheight*2 + 19, GT_VisualInfo, vi, GTBB_Recessed, TRUE, TAG_END);
			}
			DrawColorField(rp,GadgetAddress(win,3),FindColorPen(0,0,0),TRUE);
			break;
		case WDT_NOTES:
//			itext.IText = GetString(&gLocaleInfo, MSG_INSERT_EXISTING_NOTE_GAD);
//			if ((gad = GadgetAddress(win, 2)) != 0)
//				PrintIText(rp, &itext, win->Width - 16 - boxwidth - IntuiTextLength(&itext), gad->TopEdge + 2);
			break;
		case WDT_PAGESETUP:
			if ((gad = GadgetAddress(win, 1)) != 0)
				DrawGroupBorder(rp, GetString(&gLocaleInfo, MSG_PAGE_FORMAT_BORDER), lborder, gad->TopEdge - fontheight - 2, win->Width - rborder - lborder, 5*fontheight + 32);
			if ((gad = GadgetAddress(win,5)) != 0)
				DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_PAGE_MARGINS_BORDER),lborder,gad->TopEdge-fontheight-4,win->Width-rborder-lborder,5*fontheight+32);
			break;
		case WDT_DOCINFO:
//			itext.IText = GetString(&gLocaleInfo, MSG_COMMENTS_LABEL);
//			// ToDo: short cut is not shown here!
//			gad = wd->wd_ExtData[1];
//			PrintIText(rp,&itext,gad->LeftEdge - 8 - IntuiTextLength(&itext),gad->TopEdge+2);
			break;
		case WDT_DOCUMENT:
			if ((gad = GadgetAddress(win, 1)) != 0)
				DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_PROPERTIES_BORDER),lborder,gad->TopEdge-fontheight-2,win->Width-lborder-rborder,4*fontheight+25);
			if ((gad = GadgetAddress(win, 8)) != 0)
				DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_SECURITY_BORDER),lborder,gad->TopEdge-fontheight-4,win->Width-lborder-rborder,fontheight*5+33);
			if ((gad = GadgetAddress(win, 12)) != 0)
				DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_EVENT_DEFINITION_BORDER),lborder,gad->TopEdge-2-fontheight,win->Width-lborder-rborder,fontheight*7+11);
			break;
		case WDT_PRINTER:
			DrawGroupBorder(rp, GetString(&gLocaleInfo, MSG_RANGE_BORDER), lborder, win->BorderTop + 11 + fontheight,
				win->Width - lborder - rborder, fontheight * 16 + 15);

			if ((gad = GadgetAddress(win, 13)) != 0) {
				DrawGroupBorder(rp, GetString(&gLocaleInfo, MSG_OPTIONS_BORDER), lborder, gad->TopEdge - fontheight - 2,
					win->Width - lborder - rborder, fontheight * 3 + 18);
			}
			break;
		case WDT_PRINTSTATUS:
			itext.IText = GetString(&gLocaleInfo, MSG_CURRENT_SHEET_PROGRESS);
			PrintIText(rp,&itext,(win->Width-IntuiTextLength(&itext)) >> 1,barheight+3); //2+6*fontheight);
			itext.IText = GetString(&gLocaleInfo, MSG_CURRENT_PAGE_PROGRESS);
			PrintIText(rp,&itext,(win->Width-IntuiTextLength(&itext)) >> 1,barheight+2*fontheight+14); //2+6*fontheight);
			break;
		case WDT_PAGE:
			itext.IText = GetString(&gLocaleInfo, MSG_FORE_BACKGROUND_COLOR_GAD);
			PrintIText(rp,&itext,lborder,barheight+2*fontheight+19);
			DrawColorField(rp,GadgetAddress(win,3),(ULONG)wd->wd_ExtData[0],TRUE);
			DrawColorField(rp,GadgetAddress(win,4),(ULONG)wd->wd_ExtData[1],TRUE);
			break;
		case WDT_PRESSKEY:
			DrawRequesterBorder(win);
			itext.IText = GetString(&gLocaleInfo, MSG_PRESS_A_KEY_REQ);
			PrintIText(rp,&itext,(win->Width-IntuiTextLength(&itext)) >> 1,barheight+4+(fontheight >> 1));
			break;
		case WDT_FIND:
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_SEARCH_FOR_BORDER),lborder,win->BorderTop+3,win->Width-lborder-rborder,fontheight*9+18);
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_OPTIONS_BORDER),lborder,win->BorderTop+24+9*fontheight,win->Width-lborder-rborder,fontheight*5+32);
			break;
		case WDT_REPLACE:
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_SEARCH_FOR_BORDER),lborder,win->BorderTop+3,(gad = wd->wd_ExtData[1])->Width+16,fontheight*9+18);
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_REPLACE_WITH_BORDER),gad->LeftEdge-8,win->BorderTop+3,gad->Width+16,fontheight*9+18);
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_OPTIONS_BORDER),lborder,win->BorderTop+24+9*fontheight,win->Width-lborder-rborder,fontheight*4+25);
			break;
		case WDT_MASK:
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_MASK_BORDER),lborder,win->BorderTop+fontheight+9,win->Width-lborder-rborder,fontheight*3+18);
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_FIELDS_BORDER),lborder,win->BorderTop+29+4*fontheight,win->Width-lborder-rborder,fontheight*8+15);
			break;
		case WDT_INDEX:
			if (wd->wd_ExtData[6])
				y = 3;
			else
				y = fontheight+9;
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_FIELDS_BORDER),lborder,win->BorderTop+y,win->Width-lborder-rborder,fontheight*11+22);
			break;
		case WDT_FILTER:
			if (wd->wd_ExtData[6])
				y = 3;
			else
				y = fontheight+9;
			DrawGroupBorder(rp,GetString(&gLocaleInfo, MSG_FIELDS_BORDER),lborder,win->BorderTop+y,win->Width-lborder-rborder,fontheight*12+29);
			break;
		case WDT_CLIP:
			break;
		case WDT_INFO:
			CreateInfo(win);
			//CreateRAInfo(NULL, NULL);
			break;
	}
}


bool
WindowIsProjectDependent(long type)
{
	switch (type) {
		case WDT_PAGE:
		case WDT_DOCUMENT:
		case WDT_DIAGRAM:
		case WDT_PREVIEW:
		case WDT_DATABASE:
		case WDT_FILTER:
		case WDT_SCRIPTS:
		case WDT_INDEX:
		case WDT_MASK:
			return true;
	}
	return false;
}


bool
TestOpenAppWindow(struct Window **rwin, long type, struct TagItem *ti)
{
	struct Window *win = NULL;
	APTR   data = (APTR)GetTagData(WA_Data, 0, ti);

	*rwin = NULL;
	switch (type) {
		case WDT_PROJECT:
		case WDT_DEFINECMD:
			break;
		case WDT_OBJECT:
			for (win = scr->FirstWindow;win;win = win->NextWindow) {
				if (win->UserPort == iport
					&& ((struct winData *)win->UserData)->wd_Type == type
					&& ((struct winData *)win->UserData)->wd_Data == data)
					break;
			}
			break;
		case WDT_DIAGRAM:
		{
			struct gDiagram *current = (APTR)GetTagData(WA_CurrentDiagram, 0, ti);

			for (win = scr->FirstWindow; win; win = win->NextWindow) {
				struct winData *wd = (APTR)win->UserData;

				if (win->UserPort == iport && wd->wd_Type == type
					&& wd->u.diagram.wd_CurrentDiagram == current)
					break;
			}
			break;
		}
		default:
			if (type != WDT_PREFS && IsPrefsWindow(type)) {
				for (win = scr->FirstWindow; win != NULL;
						win = win->NextWindow) {
					if (win->UserPort == iport
						&& ((struct winData*)win->UserData)->wd_Type == type
#ifdef __amigaos4__
						)
#else
						&& ((struct winData *)win->UserData)->wd_Data == data)
#endif
						break;
				}
			} else
				win = GetAppWindow(type);
			break;
	}

	if (win) {
		long active;

		wd = (struct winData *)win->UserData;
		active = GetTagData(WA_Page, ~0L, ti);
		if (active != ~0L && wd->wd_PageHandlingGadget) {
			SetGadgetAttrs(wd->wd_PageHandlingGadget, win, NULL,
				PAGEGA_Active, active, TAG_END);
		}

		WindowToFront(win);
		ActivateWindow(win);

		*rwin = win;
		return FALSE;
	}
	if (WindowIsProjectDependent(type)) {
		if (!rxpage) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_NO_PROJECT_ERR));
			return FALSE ;
		}

		switch (type) {
			case WDT_MASK:
			case WDT_FILTER:
			case WDT_INDEX:
				if (IsListEmpty((struct List *)&rxpage->pg_Mappe
						->mp_Databases)) {
					ErrorRequest(GetString(&gLocaleInfo, MSG_NO_DATABASE_ERR));
					return FALSE;
				}
				break;
		}
	}

	if (type == WDT_FIND)
		CloseAppWindow(GetAppWindow(WDT_REPLACE), TRUE);
	if (type == WDT_REPLACE)
		CloseAppWindow(GetAppWindow(WDT_FIND), TRUE);

	return TRUE;
}

#ifdef __amigaos4__
struct Window *OpenAppWindowA(long type, struct TagItem *tags)
{
	struct Window *win;
	struct winData *wd;
	bool error = false, reopened = false;
	int32 idcmp, flags, x, y, minWidth = -1L, minHeight = -1L;
	CONST_STRPTR title;

	ObtainSemaphore(&gWindowSemaphore);

	if (!TestOpenAppWindow(&win, type, (struct TagItem *)tags)) {
		ReleaseSemaphore(&gWindowSemaphore);
		return win;
	}
	title = (STRPTR)GetTagData(WA_Title, (ULONG)GetString(&gLocaleInfo,
	                           MSG_IGNITION_REQUEST_TITLE), (struct TagItem *)tags);

	if ((wd = (struct winData *)GetTagData(WA_WinData, 0, (struct TagItem *)tags)) != 0)
		reopened = true;
	x = GetTagData(WA_Left, prefs.pr_WinPos[type].Left, (struct TagItem *)tags);
	y = GetTagData(WA_Top, prefs.pr_WinPos[type].Top, (struct TagItem *)tags);

	ngad.ng_VisualInfo = vi;
	ngad.ng_TextAttr = scr->Font;
	ngad.ng_UserData = NULL;
	ngad.ng_TopEdge = barheight + 3;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetID = 1;

	if (wd || (wd = AllocPooled(pool, sizeof(struct winData))))
	{
		struct TagItem *ti;

		wd->wd_Type = type;

		if (!reopened && type != WDT_DEFINECMD && type != WDT_OBJECT)
			wd->wd_Data = rxpage;
		if ((ti = FindTagItem(WA_Data, (struct TagItem *)tags)) != 0)
			wd->wd_Data = (APTR)ti->ti_Data;

		if (!reopened)
		{
			wd->wd_CurrentPage = GetTagData(WA_Page, 0, (struct TagItem *)tags);
			wd->wd_ExtData[0] = (APTR)GetTagData(WA_ExtData, 0, (struct TagItem *)tags);
			if (type == WDT_DEFINECMD)
				wd->u.definecmd.wd_Map = (APTR)GetTagData(WA_Map, 0, (struct TagItem *)tags);
		}
		MyNewList(&wd->wd_Objs);
		if ((gad = CreateContext(&wd->wd_Gadgets)) != 0)
		{
			wd->wd_Server = gCreateWinData[type-1].cwd_Server;
			wd->wd_CleanUp = gCreateWinData[type-1].cwd_CleanUp;

			flags = WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE;
			idcmp = gCreateWinData[type-1].cwd_IDCMP | (prefs.pr_Flags & PRF_SIMPLEWINDOWS ? IDCMP_REFRESHWINDOW : 0);

			if (IsPrefsWindow(type))
				title = MakePrefsTitle(wd->wd_Data, type, GetString(&gLocaleInfo, gCreateWinData[type-1].cwd_Title));
			else if (gCreateWinData[type-1].cwd_Title)
				title = GetString(&gLocaleInfo, gCreateWinData[type-1].cwd_Title);

			gWidth = GetTagData(WA_Width, -1, (struct TagItem *)tags);
			if (gWidth == -1)
				gWidth = prefs.pr_WinPos[type].Width;
			gHeight = GetTagData(WA_Height, -1, (struct TagItem *)tags);
			if (gHeight == -1)
				gHeight = prefs.pr_WinPos[type].Height;

			switch (type)
			{
				case WDT_PREFS:
					minWidth = TLn(GetString(&gLocaleInfo, MSG_ADD_GAD)) + TLn(GetString(&gLocaleInfo, MSG_REMOVE_GAD)) + 100;
					minHeight = barheight + fontheight + itemheight*5 + 17 + leftImg->Height;

					if (gWidth == -1) {
						gHeight = barheight + itemheight*9 + 2*fontheight + 21 + leftImg->Height;
						gWidth = TLn(GetString(&gLocaleInfo, MSG_FORMATS_PREFS)) + 120;
						gWidth = max(gWidth, minWidth);
					}

					InitTreeList(&prefstree);
					break;
				case WDT_DEFINECMD:
				{
					// RA
DBUG("OpenAppWindowA():WDT_DEFINECMD\n");
					struct Node *ln = NULL, *n = NULL;

					wd->wd_ExtData[5] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE); // PopUp list(browser) "internal"
					if(ra_popup_ci == NULL)
					{// ColumnInfo struct, but not used simultaneously
						ra_popup_ci = AllocLBColumnInfo(2,
						                                LBCIA_Column,0, //LBCIA_Weight,20,
						                                LBCIA_Column,1, //LBCIA_Weight,80,
						                               TAG_DONE);
					}
					// PopUp list(browser) "internal"
					foreach(&intcmds, ln)
					{
DBUG("\t[5]'%s' (0x%08lx)\n",ln->ln_Name,ln);
						n = AllocListBrowserNode(2,
						                         LBNA_Column,0, LBNCA_Image,NULL,
						                         LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,ln->ln_Name,
						                         LBNA_UserData, ln,
						                        TAG_DONE);
						if(n)
						{
							AddTail( (struct List *)wd->wd_ExtData[5], n );
//DBUG("\t[5] node=0x%08lx\n",n);
						}
					}
					// PopUp end

					wd->wd_ExtData[6] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE); // commands list(browser)

					wd->wd_ExtData[7] = (struct List *)AllocSysObjectTags(ASOT_LIST, TAG_DONE); // outputs chooserlist
					foreach(&outputs, ln)
					{
DBUG("\t'%s' (0x%08lx)\n",ln->ln_Name,ln);
						n = AllocChooserNode(CNA_CopyText,TRUE, CNA_Text,ln->ln_Name, TAG_DONE);
						if(n)
						{
							AddTail( (struct List *)wd->wd_ExtData[7], n );
						}
					}
					// RA

					if (!wd->u.definecmd.wd_AppCmd)
						wd->u.definecmd.wd_AppCmd = NewAppCmd(wd->wd_Data);
					}
					break;
				case WDT_PREFKEYS:
				{
					// RA
DBUG("OpenAppWindowA():WDT_PREFKEYS\n");
					// PopUp
					struct Mappe *mp = GetPrefsMap( GetLocalPrefs(wd->wd_Data) );
					struct MinList *list;
					struct Image *im;
					struct Node *ln = NULL, *n = NULL;
DBUG("\tmp=0x%08lx\n",mp);
					wd->wd_ExtData[4] = (struct List *)AllocSysObject(ASOT_LIST, NULL);
					if(ra_popup_ci == NULL)
					{// ColumnInfo struct, but not used simultaneously
						ra_popup_ci = AllocLBColumnInfo(2,
						                                LBCIA_Column,0, //LBCIA_Weight,20,
						                                LBCIA_Column,1, //LBCIA_Weight,80,
						                               TAG_DONE);
					}
					// PopUp list(browser) "normal"
					list = mp? &mp->mp_AppCmds : &prefs.pr_AppCmds;
					foreach(list, ln)
					{
						im = ((struct AppCmd *)ln)->ac_Node.in_Image;
DBUG("\t'%s' (0x%08lx)\n",ln->ln_Name,im);
						n = AllocListBrowserNode(2,
						                         LBNA_Column,0, LBNCA_Image,im,
						                         LBNA_Column,1, LBNCA_CopyText,TRUE, LBNCA_Text,ln->ln_Name,
						                         LBNA_UserData, ln,
						                        TAG_DONE);
						if(n)
						{
							AddTail( (struct List *)wd->wd_ExtData[4], n );
						}
					}
					// PopUp end
				}
				// RA
					/*minWidth = TLn("ctrl amiga w") + TLn(GetString(&gLocaleInfo, MSG_RECORD_GAD)) + boxwidth + 70 + lborder + rborder;
					minHeight = barheight + 11 * fontheight + 43 + leftImg->Height;

					if (gWidth == -1) {
						gWidth = minWidth;
						gHeight = minHeight + 8 * fontheight;
					}*/
					break;
				case WDT_FILETYPE:
					flags = WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_RMBTRAP;
					x = fileReq->fr_LeftEdge + fileReq->fr_Width + 16;
					break;
				case WDT_SETTITLE:
				case WDT_SETNAME:
					SetNamePosition(&x, &y);
					break;
				case WDT_CLIP:
					minWidth = scr->Width >> 2;  minHeight = barheight + fontheight*5 + 14 + leftImg->Height;

					if (gWidth == -1) {
						gWidth = minWidth;
						gHeight = barheight + fontheight*11 + 14 + leftImg->Height;
					}
					break;
				case WDT_GCLASSES:
				{
					long w = TLn(GetString(&gLocaleInfo, MSG_INSERT_OBJECT_GAD));

					minWidth = TLn(GetString(&gLocaleInfo, MSG_KEEP_WINDOWS_OPEN_GAD));
					minWidth = max(w,minWidth)+boxwidth+itemwidth+lborder+rborder;

					minHeight = itemheight * 2 + barheight + fontheight + 17 + leftImg->Height;

					if (gWidth == -1) {
						gWidth = minWidth + 42;
						gHeight = minHeight + 3*itemheight;
					}
					break;
				}
				case WDT_NOTES:
				{
//					minWidth = TLn(GetString(&gLocaleInfo, MSG_NOTE_GAD))+3*TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+boxwidth+76;
//					minHeight = fontheight*6+barheight+21 + leftImg->Height;

					if (!reopened)
					{
						initNotes(wd);

						wd->wd_Data = rxpage ? rxpage->pg_Mappe : NULL;
					}
//					if (gWidth == -1) {
//						gWidth = minWidth;
//						gHeight = minHeight + 2 * fontheight;
//					}
					break;
				}
				case WDT_SCRIPTS:
				{
					minWidth = 2*TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + TLn(GetString(&gLocaleInfo, MSG_DESCRIPTION_GAD)) + 120;
					minHeight = fontheight*11 + barheight + 21 + leftImg->Height;
 
					if (gWidth == -1) {
						gWidth = minWidth + 80;
						gHeight = minHeight + 2*fontheight;
					}
					if (!reopened)
						wd->wd_Data = rxpage ? rxpage->pg_Mappe : NULL;
					break;
				}
				case WDT_PAGESETUP:
				case WDT_DOCINFO:
					if (!reopened)
						wd->wd_Data = rxpage ? rxpage->pg_Mappe : NULL;
					break;
				case WDT_DIAGRAM:
				{
					struct gDiagram *sgd;

					if (!reopened && (sgd = wd->u.diagram.wd_CurrentDiagram)) // altes Diagramm bearbeiten
						wd->u.diagram.wd_OldDiagram = sgd;

					if (wd->u.diagram.wd_OldDiagram)
						title = GetString(&gLocaleInfo, MSG_CHANGE_DIAGRAM_TITLE);
					break;
				}
				case WDT_PREVIEW:
					minWidth = scr->Width/5;  minHeight = scr->Height/5;

					if (!reopened)
						gWidth = scr->Width/3,  gHeight = scr->Height/3;
					flags |= WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM;
					break;
			}

			if (!reopened && gCreateWinData[type-1].cwd_Init)
				gCreateWinData[type-1].cwd_Init(wd);

			if (gCreateWinData[type-1].cwd_Gadgets)
				gCreateWinData[type-1].cwd_Gadgets(wd);
			else if (gCreateWinData[type-1].cwd_ResizableGadgets) {
				gWidth = max(gWidth, minWidth);
				gHeight = max(gHeight, minHeight);

				gCreateWinData[type-1].cwd_ResizableGadgets(wd, gWidth, gHeight);
				flags |= WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM;
			}

			if (!error)
			{
				if((wd->wd_Type>0 && wd->wd_Type<=WDT_SETNAME? !gCreateWinData[wd->wd_Type-1].cwd_IDCMP : 0))
				{
DBUG("ReAction win (type=%ld) x=%ld y=%ld\n", type,x,y);
					winobj[type] = NewObject(WindowClass, NULL, //"window.class",
					        WA_Title,        title,
					        WA_DragBar,      TRUE,
					        WA_CloseGadget,  TRUE,
					        WA_SizeGadget,   TRUE,
					        WA_DepthGadget,  TRUE,
					        //WA_Activate,     TRUE, // using ActivateWindow(win) few lines below
					        x==-1? TAG_IGNORE:WA_Left, x,
					        y==-1? TAG_IGNORE:WA_Top,  y,
					        //WA_NewLookMenus, TRUE,
					        //WA_MenuHelp,     TRUE,
					        WA_PubScreen,    scr,
					        //WA_Flags,        flags,
					        WA_IDCMP,        IDCMP_MENUPICK, // makes main menu options work
					        //(prefs.pr_Flags&PRF_SIMPLEWINDOWS)? WA_SimpleRefresh : TAG_IGNORE, TRUE,
					        WINDOW_SharedPort, iport,
					        //WINDOW_Position,   WPOS_CENTERSCREEN,
					        WINDOW_Layout, NewObject(LayoutClass, NULL, //"layout.gadget",
					         LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					         LAYOUT_SpaceOuter,  TRUE,
					         LAYOUT_AddChild, OBJ(OID_TEMPORAL),
					        TAG_DONE),
					TAG_END);
					if(x==-1 && y==-1)
					{
						SetAttrs(winobj[type], WINDOW_Position,WPOS_CENTERSCREEN, TAG_DONE);
					}
					win = (struct Window *)IDoMethod(winobj[type], WM_OPEN, NULL);
				}
				else
				{
					if (x == -1)
						x = (scr->Width - gWidth) >> 1;
					if (y == -1)
						y = (scr->Height - gHeight) >> 1;

					win = OpenWindowTags(NULL,
											   WA_Flags,        flags,
											   WA_Left,         x,
											   WA_Top,          y,
											   WA_Title,        title,
											   WA_Width,        gWidth,
											   WA_Height,       gHeight,
											   WA_NewLookMenus, TRUE,
											   WA_MenuHelp,     TRUE,
											   WA_PubScreen,    scr,
											   WA_Gadgets,      wd->wd_Gadgets,
//											   WA_IDCMP, LISTVIEWIDCMP,
											   prefs.pr_Flags & PRF_SIMPLEWINDOWS ? WA_SimpleRefresh : TAG_IGNORE, TRUE,
											   WA_ScreenTitle,  IgnTitle,
											   TAG_END);
				}
				if(win)
				{
DBUG("\twin=0x%08lx\n",win);
					if( (wd->wd_Type>0 && wd->wd_Type<=WDT_SETNAME? !gCreateWinData[wd->wd_Type-1].cwd_IDCMP : 0) )
					{
						uint32 mysigmask = 0;

						GetAttr(WINDOW_SigMask, winobj[type], &mysigmask);
//DBUG("sigwait=0x%08lx\n", sigwait);
						wsigmask |= mysigmask;
						sigwait |= mysigmask;
						//winptr->UserData = (APTR)wd;
						win->UserData = (APTR)wd;
						//wd->wd_Mother = (struct Window *)GetTagData(WA_Mother,(ULONG)winptr->Parent,(struct TagItem *)tags);
						wd->wd_Mother = (struct Window *)GetTagData(WA_Mother, (ULONG)win->Parent, (struct TagItem *)tags);
						ScreenToFront(scr);
						ActivateWindow(win); // or it won't react on first click/event ¿:-/
					}
					else
					{
						win->UserPort = iport;
						win->UserData = (APTR)wd;
						wd->wd_Mother = (struct Window *)GetTagData(WA_Mother,(ULONG)win->Parent,(struct TagItem *)tags);
						ModifyIDCMP(win, idcmp);
						if (minWidth != -1)
							WindowLimits(win, minWidth, minHeight, -1, -1);
						ScreenToFront(scr);
					}

					/* Post-Init */
					switch (type)
					{
						case WDT_PREFTABLE:
						{
							struct Node *n = NULL;
							STRPTR res_string;

							GetAttr(CHOOSER_SelectedNode, OBJ(OID_T_QUALFUNC), (uint32 *)&n);
							GetChooserNodeAttrs(n, CNA_Text,&res_string, TAG_DONE);
DBUG("OpenAppWindowA():WDT_PREFTABLE '%s'\n", res_string);
							//RefreshSetGadgetAttrs(GAD(OID_T_QUALFUNC), win, NULL, CHOOSER_Title,res_string, TAG_DONE);
							SetAttrs(OBJ(OID_T_QUALFUNC), CHOOSER_Title,res_string, TAG_DONE);
							IDoMethod(winobj[WDT_PREFTABLE], WM_RETHINK);
						}
							break;
						case WDT_PREFSCREEN:
							if (!reopened)
								wd->wd_Data = (APTR)prefs.pr_Screen->ps_BFColor;
							break;
						case WDT_PREFDISP:
							if (!reopened)
							{
								struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

								if (pr != &prefs && pr->pr_Disp == prefs.pr_Disp)
								{
									AddPrefsModuleToLocalPrefs(GetPrefsMap(pr),WDT_PREFDISP);

									wd->wd_ExtData[2] = (APTR)TRUE;
								}
								else
									wd->wd_ExtData[2] = (APTR)FALSE;

								if ((wd->wd_ExtData[1] = AllocPooled(pool, sizeof(struct PrefDisp))) != 0)
									CopyMem(pr->pr_Disp,wd->wd_ExtData[1],sizeof(struct PrefDisp));
							}
							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);
							break;
						case WDT_PREFMENU:
							GTD_AddGadget(LISTVIEW_KIND,wd->wd_ExtData[0],win,GTDA_InternalType, DRAGT_MENU,
																			GTDA_Same,		 TRUE,
																			GTDA_AcceptTypes,  DRAGT_MENU,
																			TAG_END);
							GTD_AddGadget(LISTVIEW_KIND,wd->wd_ExtData[1],win,GTDA_InternalType, DRAGT_SUBMENU,
																			GTDA_Same,		 TRUE,
																			GTDA_AcceptTypes,  DRAGT_SUBMENU,
																			TAG_END);
							GTD_AddGadget(LISTVIEW_KIND,wd->wd_ExtData[2],win,GTDA_InternalType, DRAGT_SUBMENU,
																			GTDA_Same,		 TRUE,
																			GTDA_AcceptTypes,  DRAGT_SUBMENU,
																			TAG_END);

							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);
							break;
						case WDT_PREFICON:
						{
							struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[0], win,
									GTDA_NoPosition,   TRUE,
									GTDA_InternalType, DRAGT_APPCMD,
									GTDA_AcceptTypes,  DRAGT_ICONOBJ,
									GTDA_ItemHeight,   itemheight,
									TAG_END);
							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[1], win,
									GTDA_InternalType, DRAGT_ICONOBJ,
									GTDA_AcceptTypes,  DRAGT_APPCMD | DRAGT_ICONOBJ | DRAGT_ICONSEPARATOR,
									GTDA_Same,		 TRUE,
									GTDA_ItemHeight,   itemheight,
									TAG_END);
							GTD_AddGadget(BUTTON_KIND, GadgetAddress(win,3), win,
									GTDA_InternalType, DRAGT_ICONSEPARATOR,
									GTDA_AcceptTypes,  0,
									GTDA_RenderHook,   &renderHook,
									GTDA_Object,	   wd->wd_ExtData[3],
									GTDA_Width,		200,
									GTDA_Height,	   itemheight,
									TAG_END);
							GTD_AddGadget(BUTTON_KIND,GadgetAddress(win,4),win,GTDA_AcceptTypes,DRAGT_ICONOBJ | DRAGT_ICONSEPARATOR,TAG_END);

							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);

							//wd->wd_ExtData[5] = AddLockNode(&pr->pr_AppCmds,0,PrefIconAppCmdLock,3*sizeof(APTR),win,wd->wd_ExtData[0],wd);
							//wd->wd_ExtData[7] = AddListViewLock(&pr->pr_IconObjs,win,wd->wd_ExtData[1]);
							break;
						}
						case WDT_PRINTER:
							UpdatePrinterGadgets(win, wd);
							break;
						case WDT_PRINTSTATUS:
						{
							struct wdtPrintStatus *wps = wd->wd_ExtData[0];

							if (wps)
							{
								wps->wps_Window = win;

								wps->wps_ProjectBar = NewProgressBar(win,lborder,barheight+6+fontheight,gWidth-lborder-rborder,fontheight+4);
								wps->wps_PageBar = NewProgressBar(win,lborder,barheight+17+3*fontheight,gWidth-lborder-rborder,fontheight+4);
								wps->wps_SinglePageBar = NewProgressBar(win, lborder, barheight + 24 + 4*fontheight,
									gWidth - lborder - rborder, fontheight + 4);
							}
							break;
						}
						case WDT_SETTITLE:
						case WDT_SETNAME:
							wd->wd_Lock = AddLockNode(RXMAP,0,RxMapLock,sizeof(APTR),win);
						case WDT_COMMAND:
							ActivateGadget(GadgetAddress(win,1),win,NULL);
							break;
						case WDT_DEFINECMD:
							if (!reopened && wd->wd_Data)
								((struct AppCmd *)wd->wd_Data)->ac_Locked++;
							ActivateGadget(GadgetAddress(win,1),win,NULL);
							break;
						case WDT_OBJECT:
							if (wd->wd_Data) // "verbindet" das Fenster mit dem dazugehörigen Objekt
								((struct gObject *)wd->wd_Data)->go_Window = win;

							UpdateObjectGadgets(win);
							break;
						case WDT_DIAGRAM:
							if (wd->u.diagram.wd_CurrentDiagram)								//open existing diagram
								wd->u.diagram.wd_CurrentDiagram->gd_Object.go_Window = win;
							else																//create new diagram
								{																//set first diagramtyp as default to avoid grim when use some gadgets (example horiz/vertical
								struct MinNode *mln;

								mln = gdiagrams.mlh_Head;
								SetDiagramType(win, wd, (struct gClass *)mln);
								}

							UpdateObjectGadgets(win);
							break;
						case WDT_PAGE:
							if (!reopened)
							{
								wd->wd_ExtData[0] = (APTR)((struct Page *)wd->wd_Data)->pg_APen;
								wd->wd_ExtData[1] = (APTR)((struct Page *)wd->wd_Data)->pg_BPen;
							}
							break;
						case WDT_CELL:
							if (!reopened)
								wd->wd_Data = NULL;
							wd->wd_ExtData[5] = AddLockNode(&rxpage->pg_Mappe->mp_Formats,0,CellWindowLock,sizeof(APTR),win);
							wd->wd_Lock = AddLockNode(RXMAP,0,CellWindowLock,sizeof(APTR),win);
#ifdef __amigaos4__
							if(wd->wd_CurrentPage == 1) //page 1 must be refreshed, because wd is not set at window-creation
								RefreshCellPageGadgets(win, wd->wd_CurrentPage);
#endif
							break;
						case WDT_ZOOM:
							wd->wd_Lock = AddLockNode(RXMAP,0,ZoomWindowLock,2*sizeof(APTR),win,GadgetAddress(win,1));
							break;
						case WDT_GCLASSES:
							wd->wd_ExtData[0] = AddListViewLock(&gclasses,win,GadgetAddress(win,1));
						case WDT_BORDER:
						case WDT_NOTES:
						case WDT_CELLSIZE:
						case WDT_FIND:
						case WDT_REPLACE:
						case WDT_FORMEL:
							wd->wd_Lock = AddLockNode(RXMAP,0,RxMapLock,sizeof(APTR),win);
							break;
						case WDT_PREFS:
						{
							struct Gadget *gad = GadgetAddress(win, 1);

							GTD_AddGadget(LISTVIEW_KIND, gad, win,
									GTDA_Same,		TRUE,
									GTDA_AcceptTypes, DRAGT_PREFS,
									GTDA_InternalType,DRAGT_PREFS,
									GTDA_ItemHeight,  itemheight,
									GTDA_TreeView,	TRUE,
									GTDA_InternalOnly,TRUE,
									TAG_END);
							wd->wd_Lock = AddTreeLock((struct MinList *)&prefstree, win, gad);
							break;
						}
						case WDT_PREFCONTEXT:
						{
							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[6], win,
									GTDA_InternalType, DRAGT_SUBMENU,
									GTDA_Same,		 TRUE,
									GTDA_AcceptTypes,  DRAGT_SUBMENU,
									TAG_END);
							goto openappwindow_addfreelock;
						}
						case WDT_PREFCMDS:
						{
							struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[0], win,
									GTDA_InternalType, DRAGT_APPCMD,
									GTDA_AcceptTypes, 0,
									GTDA_ItemHeight,  itemheight,
									TAG_END);
							wd->wd_ExtData[5] = AddLockNode(&pr->pr_AppCmds,0,AppCmdLock,3*sizeof(APTR),win,GadgetAddress(win,1),wd->wd_Data);
							goto openappwindow_addfreelock;
						}

						case WDT_PREFKEYS:
							GTD_AddWindow(win, GTDA_AcceptTypes, DRAGT_APPCMD, TAG_END);

							goto openappwindow_addfreelock;

						case WDT_SCRIPTS:
							wd->wd_ExtData[5] = AddLockNode(&((struct Mappe *)wd->wd_Data)->mp_RexxScripts,0,ScriptsWindowLock,sizeof(APTR)*2,win,wd->wd_ExtData[0]);

							goto openappwindow_addfreelock;

						case WDT_PREFFORMAT:
						case WDT_PREFNAMES:
						case WDT_PREFCHOICE:
openappwindow_addfreelock:  /***************************************************************/
							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);
							break;

						case WDT_PREVIEW:
							RefreshPreviewSize(win);
							break;
					}
					RefreshAppWindow(win,wd);
					GT_RefreshWindow(win,NULL);
					SetMenuStrip(win,rxpage ? rxpage->pg_Mappe->mp_Prefs.pr_Menu : prefs.pr_Menu);

					ReleaseSemaphore(&gWindowSemaphore);
					return win;
				}
			}
			FreeGadgets(wd->wd_Gadgets);
		}
#ifdef __amigaos4__
		FreeVec(wd);
#else
		FreeMem(wd,sizeof(struct winData));
#endif
	}
	ReleaseSemaphore(&gWindowSemaphore);

	return NULL;
}

struct Window *OpenAppWindow(long type, ...)
{
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, type);
	tags = va_getlinearva(ap, struct TagItem *);
    return OpenAppWindowA(type, tags);
}

#else
struct Window *
OpenAppWindow(long type, ULONG tag1, ...)
{
	struct Window *win;
	struct winData *wd;
	bool error = false, reopened = false;
	int32 idcmp, flags, x, y, minWidth = -1L, minHeight = -1L;
	CONST_STRPTR title;

	ObtainSemaphore(&gWindowSemaphore);

	if (!TestOpenAppWindow(&win, type, (struct TagItem *)&tag1)) {
		ReleaseSemaphore(&gWindowSemaphore);
		return win;
	}
	title = (STRPTR)GetTagData(WA_Title, (ULONG)GetString(&gLocaleInfo,
		MSG_IGNITION_REQUEST_TITLE), (struct TagItem *)&tag1);

	if ((wd = (struct winData *)GetTagData(WA_WinData, 0, (struct TagItem *)&tag1)) != 0)
		reopened = true;

	x = GetTagData(WA_Left, prefs.pr_WinPos[type].Left, (struct TagItem *)&tag1);
	y = GetTagData(WA_Top, prefs.pr_WinPos[type].Top, (struct TagItem *)&tag1);

	ngad.ng_VisualInfo = vi;
	ngad.ng_TextAttr = scr->Font;
	ngad.ng_UserData = NULL;
	ngad.ng_TopEdge = barheight + 3;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetID = 1;

	if (wd || (wd = AllocPooled(pool, sizeof(struct winData))))
	{
		struct TagItem *ti;

		wd->wd_Type = type;

		if (!reopened && type != WDT_DEFINECMD && type != WDT_OBJECT)
			wd->wd_Data = rxpage;
		if ((ti = FindTagItem(WA_Data, (struct TagItem *)&tag1)) != 0)
			wd->wd_Data = (APTR)ti->ti_Data;

		if (!reopened)
		{
			wd->wd_CurrentPage = GetTagData(WA_Page, 0, (struct TagItem *)&tag1);
			wd->wd_ExtData[0] = (APTR)GetTagData(WA_ExtData, 0, (struct TagItem *)&tag1);
	
			if (type == WDT_DEFINECMD)
				wd->u.definecmd.wd_Map = (APTR)GetTagData(WA_Map, 0, (struct TagItem *)&tag1);
		}
		MyNewList(&wd->wd_Objs);

		if ((gad = CreateContext(&wd->wd_Gadgets)) != 0)
		{
			wd->wd_Server = gCreateWinData[type - 1].cwd_Server;
			wd->wd_CleanUp = gCreateWinData[type - 1].cwd_CleanUp;

			flags = WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE;
			idcmp = gCreateWinData[type - 1].cwd_IDCMP | (prefs.pr_Flags & PRF_SIMPLEWINDOWS ? IDCMP_REFRESHWINDOW : 0);

			if (IsPrefsWindow(type))
				title = MakePrefsTitle(wd->wd_Data, type, GetString(&gLocaleInfo, gCreateWinData[type - 1].cwd_Title));
			else if (gCreateWinData[type - 1].cwd_Title)
				title = GetString(&gLocaleInfo, gCreateWinData[type - 1].cwd_Title);

			gWidth = GetTagData(WA_Width, -1, (struct TagItem *)&tag1);
			if (gWidth == -1)
				gWidth = prefs.pr_WinPos[type].Width;
			gHeight = GetTagData(WA_Height, -1, (struct TagItem *)&tag1);
			if (gHeight == -1)
				gHeight = prefs.pr_WinPos[type].Height;

			switch (type)
			{
				case WDT_PREFS:
					minWidth = TLn(GetString(&gLocaleInfo, MSG_ADD_GAD)) + TLn(GetString(&gLocaleInfo, MSG_REMOVE_GAD)) + 100;
					minHeight = barheight + fontheight + itemheight*5 + 17 + leftImg->Height;

					if (gWidth == -1) {
						gHeight = barheight + itemheight*9 + 2*fontheight + 21 + leftImg->Height;
						gWidth = TLn(GetString(&gLocaleInfo, MSG_FORMATS_PREFS)) + 120;
						gWidth = max(gWidth, minWidth);
					}

					InitTreeList(&prefstree);
					break;
				case WDT_DEFINECMD:
					if (!wd->u.definecmd.wd_AppCmd)
						wd->u.definecmd.wd_AppCmd = NewAppCmd(wd->wd_Data);
					break;
				case WDT_PREFKEYS:
					minWidth = TLn("ctrl amiga w") + TLn(GetString(&gLocaleInfo, MSG_RECORD_GAD)) + boxwidth + 70 + lborder + rborder;
					minHeight = barheight + 11 * fontheight + 43 + leftImg->Height;

					if (gWidth == -1) {
						gWidth = minWidth;
						gHeight = minHeight + 8 * fontheight;
					}
					break;
				case WDT_FILETYPE:
					flags = WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_RMBTRAP;
					x = fileReq->fr_LeftEdge + fileReq->fr_Width + 16;
					break;
				case WDT_SETTITLE:
				case WDT_SETNAME:
					SetNamePosition(&x, &y);
					break;
				case WDT_CLIP:
					minWidth = scr->Width >> 2;  minHeight = barheight + fontheight*5 + 14 + leftImg->Height;

					if (gWidth == -1) {
						gWidth = minWidth;
						gHeight = barheight + fontheight*11 + 14 + leftImg->Height;
					}
					break;
				case WDT_GCLASSES:
				{
					long w = TLn(GetString(&gLocaleInfo, MSG_INSERT_OBJECT_GAD));

					minWidth = TLn(GetString(&gLocaleInfo, MSG_KEEP_WINDOWS_OPEN_GAD));
					minWidth = max(w,minWidth)+boxwidth+itemwidth+lborder+rborder;

					minHeight = itemheight * 2 + barheight + fontheight + 17 + leftImg->Height;

					if (gWidth == -1) {
						gWidth = minWidth + 42;
						gHeight = minHeight + 3*itemheight;
					}
					break;
				}
				case WDT_NOTES:
				{
					minWidth = TLn(GetString(&gLocaleInfo, MSG_NOTE_GAD))+3*TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+boxwidth+76;
					minHeight = fontheight*6+barheight+21 + leftImg->Height;

					if (!reopened) {
						initNotes(wd);

						wd->wd_Data = rxpage ? rxpage->pg_Mappe : NULL;
					}
					if (gWidth == -1) {
						gWidth = minWidth;
						gHeight = minHeight + 2 * fontheight;
					}
					break;
				}
				case WDT_SCRIPTS:
				{
					minWidth = 2*TLn(GetString(&gLocaleInfo, MSG_DELETE_GAD)) + TLn(GetString(&gLocaleInfo, MSG_DESCRIPTION_GAD)) + 120;
					minHeight = fontheight*11 + barheight + 21 + leftImg->Height;
 
					if (gWidth == -1) {
						gWidth = minWidth + 80;
						gHeight = minHeight + 2*fontheight;
					}
					if (!reopened)
						wd->wd_Data = rxpage ? rxpage->pg_Mappe : NULL;
					break;
				}
				case WDT_PAGESETUP:
				case WDT_DOCINFO:
					if (!reopened)
						wd->wd_Data = rxpage ? rxpage->pg_Mappe : NULL;
					break;
				case WDT_DIAGRAM:
				{
					struct gDiagram *sgd;

					if (!reopened && (sgd = wd->u.diagram.wd_CurrentDiagram)) // altes Diagramm bearbeiten
						wd->u.diagram.wd_OldDiagram = sgd;

					if (wd->u.diagram.wd_OldDiagram)
						title = GetString(&gLocaleInfo, MSG_CHANGE_DIAGRAM_TITLE);
					break;
				}
				case WDT_PREVIEW:
					minWidth = scr->Width/5;  minHeight = scr->Height/5;

					if (!reopened)
						gWidth = scr->Width/3,  gHeight = scr->Height/3;
					flags |= WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM;
					break;
			}

			if (!reopened && gCreateWinData[type-1].cwd_Init)
				gCreateWinData[type - 1].cwd_Init(wd);

			if (gCreateWinData[type - 1].cwd_Gadgets)
				gCreateWinData[type - 1].cwd_Gadgets(wd);
			else if (gCreateWinData[type - 1].cwd_ResizableGadgets) {
				gWidth = max(gWidth, minWidth);
				gHeight = max(gHeight, minHeight);

				gCreateWinData[type - 1].cwd_ResizableGadgets(wd, gWidth, gHeight);
				flags |= WFLG_SIZEGADGET | WFLG_SIZEBBOTTOM;
			}

			if (!error)
			{
				if (x == -1)
					x = (scr->Width - gWidth) >> 1;
				if (y == -1)
					y = (scr->Height - gHeight) >> 1;

				if ((win = OpenWindowTags(NULL, WA_Flags,		flags,
											   WA_Left,		 x,
											   WA_Top,		  y,
											   WA_Title,		title,
											   WA_Width,		gWidth,
											   WA_Height,	   gHeight,
											   WA_NewLookMenus, TRUE,
											   WA_MenuHelp,	 TRUE,
											   WA_PubScreen,	scr,
											   WA_Gadgets,	  wd->wd_Gadgets,
											   prefs.pr_Flags & PRF_SIMPLEWINDOWS ? WA_SimpleRefresh : TAG_IGNORE,TRUE,
											   TAG_END)) != 0)
				{
					win->UserPort = iport;  win->UserData = (APTR)wd;
					wd->wd_Mother = (struct Window *)GetTagData(WA_Mother,(ULONG)win->Parent,(struct TagItem *)&tag1);
					ModifyIDCMP(win, idcmp);
					if (minWidth != -1)
						WindowLimits(win, minWidth, minHeight, -1, -1);
					ScreenToFront(scr);

					/* Post-Init */

					switch (type)
					{
						case WDT_PREFSCREEN:
							if (!reopened)
								wd->wd_Data = (APTR)prefs.pr_Screen->ps_BFColor;
							break;
						case WDT_PREFDISP:
							if (!reopened)
							{
								struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

								if (pr != &prefs && pr->pr_Disp == prefs.pr_Disp)
								{
									AddPrefsModuleToLocalPrefs(GetPrefsMap(pr),WDT_PREFDISP);

									wd->wd_ExtData[2] = (APTR)TRUE;
								}
								else
									wd->wd_ExtData[2] = (APTR)FALSE;

								if ((wd->wd_ExtData[1] = AllocPooled(pool, sizeof(struct PrefDisp))) != 0)
									CopyMem(pr->pr_Disp,wd->wd_ExtData[1],sizeof(struct PrefDisp));
							}
							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);
							break;
						case WDT_PREFMENU:
							GTD_AddGadget(LISTVIEW_KIND,wd->wd_ExtData[0],win,GTDA_InternalType, DRAGT_MENU,
																			GTDA_Same,		 TRUE,
																			GTDA_AcceptTypes,  DRAGT_MENU,
																			TAG_END);
							GTD_AddGadget(LISTVIEW_KIND,wd->wd_ExtData[1],win,GTDA_InternalType, DRAGT_SUBMENU,
																			GTDA_Same,		 TRUE,
																			GTDA_AcceptTypes,  DRAGT_SUBMENU,
																			TAG_END);
							GTD_AddGadget(LISTVIEW_KIND,wd->wd_ExtData[2],win,GTDA_InternalType, DRAGT_SUBMENU,
																			GTDA_Same,		 TRUE,
																			GTDA_AcceptTypes,  DRAGT_SUBMENU,
																			TAG_END);

							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);
							break;
						case WDT_PREFICON:
						{
							struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[0], win,
									GTDA_NoPosition,   TRUE,
									GTDA_InternalType, DRAGT_APPCMD,
									GTDA_AcceptTypes,  DRAGT_ICONOBJ,
									GTDA_ItemHeight,   itemheight,
									TAG_END);
							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[1], win,
									GTDA_InternalType, DRAGT_ICONOBJ,
									GTDA_AcceptTypes,  DRAGT_APPCMD | DRAGT_ICONOBJ | DRAGT_ICONSEPARATOR,
									GTDA_Same,		 TRUE,
									GTDA_ItemHeight,   itemheight,
									TAG_END);
							GTD_AddGadget(BUTTON_KIND, GadgetAddress(win,3), win,
									GTDA_InternalType, DRAGT_ICONSEPARATOR,
									GTDA_AcceptTypes,  0,
									GTDA_RenderHook,   &renderHook,
									GTDA_Object,	   wd->wd_ExtData[3],
									GTDA_Width,		200,
									GTDA_Height,	   itemheight,
									TAG_END);
							GTD_AddGadget(BUTTON_KIND,GadgetAddress(win,4),win,GTDA_AcceptTypes,DRAGT_ICONOBJ | DRAGT_ICONSEPARATOR,TAG_END);

							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);

							wd->wd_ExtData[5] = AddLockNode(&pr->pr_AppCmds,0,PrefIconAppCmdLock,3*sizeof(APTR),win,wd->wd_ExtData[0],wd);
							wd->wd_ExtData[7] = AddListViewLock(&pr->pr_IconObjs,win,wd->wd_ExtData[1]);
							break;
						}
						case WDT_PRINTER:
							UpdatePrinterGadgets(win, wd);
							break;
						case WDT_PRINTSTATUS:
						{
							struct wdtPrintStatus *wps = wd->wd_ExtData[0];

							if (wps)
							{
								wps->wps_Window = win;

								wps->wps_ProjectBar = NewProgressBar(win,lborder,barheight+6+fontheight,gWidth-lborder-rborder,fontheight+4);
								wps->wps_PageBar = NewProgressBar(win,lborder,barheight+17+3*fontheight,gWidth-lborder-rborder,fontheight+4);
								wps->wps_SinglePageBar = NewProgressBar(win, lborder, barheight + 24 + 4*fontheight,
									gWidth - lborder - rborder, fontheight + 4);
							}
							break;
						}
						case WDT_SETTITLE:
						case WDT_SETNAME:
							wd->wd_Lock = AddLockNode(RXMAP,0,RxMapLock,sizeof(APTR),win);
						case WDT_COMMAND:
							ActivateGadget(GadgetAddress(win,1),win,NULL);
							break;
						case WDT_DEFINECMD:
							if (!reopened && wd->wd_Data)
								((struct AppCmd *)wd->wd_Data)->ac_Locked++;
							ActivateGadget(GadgetAddress(win,1),win,NULL);
							break;
						case WDT_OBJECT:
							if (wd->wd_Data) // "verbindet" das Fenster mit dem dazugehörigen Objekt
								((struct gObject *)wd->wd_Data)->go_Window = win;

							UpdateObjectGadgets(win);
							break;
						case WDT_DIAGRAM:
							if (wd->u.diagram.wd_CurrentDiagram)
								wd->u.diagram.wd_CurrentDiagram->gd_Object.go_Window = win;

							UpdateObjectGadgets(win);
							break;
						case WDT_PAGE:
							if (!reopened)
							{
								wd->wd_ExtData[0] = (APTR)((struct Page *)wd->wd_Data)->pg_APen;
								wd->wd_ExtData[1] = (APTR)((struct Page *)wd->wd_Data)->pg_BPen;
							}
							break;
						case WDT_CELL:
							if (!reopened)
								wd->wd_Data = NULL;
							wd->wd_ExtData[5] = AddLockNode(&rxpage->pg_Mappe->mp_Formats,0,CellWindowLock,sizeof(APTR),win);
							wd->wd_Lock = AddLockNode(RXMAP,0,CellWindowLock,sizeof(APTR),win);
							break;
						case WDT_ZOOM:
							wd->wd_Lock = AddLockNode(RXMAP,0,ZoomWindowLock,2*sizeof(APTR),win,GadgetAddress(win,1));
							break;
						case WDT_GCLASSES:
							wd->wd_ExtData[0] = AddListViewLock(&gclasses,win,GadgetAddress(win,1));
						case WDT_BORDER:
						case WDT_NOTES:
						case WDT_CELLSIZE:
						case WDT_FIND:
						case WDT_REPLACE:
						case WDT_FORMEL:
							wd->wd_Lock = AddLockNode(RXMAP,0,RxMapLock,sizeof(APTR),win);
							break;
						case WDT_PREFS:
						{
							struct Gadget *gad = GadgetAddress(win, 1);

							GTD_AddGadget(LISTVIEW_KIND, gad, win,
									GTDA_Same,		TRUE,
									GTDA_AcceptTypes, DRAGT_PREFS,
									GTDA_InternalType,DRAGT_PREFS,
									GTDA_ItemHeight,  itemheight,
									GTDA_TreeView,	TRUE,
									GTDA_InternalOnly,TRUE,
									TAG_END);
							wd->wd_Lock = AddTreeLock((struct MinList *)&prefstree, win, gad);
							break;
						}
						case WDT_PREFCONTEXT:
						{
							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[6], win,
									GTDA_InternalType, DRAGT_SUBMENU,
									GTDA_Same,		 TRUE,
									GTDA_AcceptTypes,  DRAGT_SUBMENU,
									TAG_END);
							goto openappwindow_addfreelock;
						}
						case WDT_PREFCMDS:
						{
							struct Prefs *pr = GetLocalPrefs(wd->wd_Data);

							GTD_AddGadget(LISTVIEW_KIND, wd->wd_ExtData[0], win,
									GTDA_InternalType, DRAGT_APPCMD,
									GTDA_AcceptTypes, 0,
									GTDA_ItemHeight,  itemheight,
									TAG_END);
							wd->wd_ExtData[5] = AddLockNode(&pr->pr_AppCmds,0,AppCmdLock,3*sizeof(APTR),win,GadgetAddress(win,1),wd->wd_Data);
							goto openappwindow_addfreelock;
						}

						case WDT_PREFKEYS:
							GTD_AddWindow(win, GTDA_AcceptTypes, DRAGT_APPCMD, TAG_END);

							goto openappwindow_addfreelock;

						case WDT_SCRIPTS:
							wd->wd_ExtData[5] = AddLockNode(&((struct Mappe *)wd->wd_Data)->mp_RexxScripts,0,ScriptsWindowLock,sizeof(APTR)*2,win,wd->wd_ExtData[0]);

							goto openappwindow_addfreelock;

						case WDT_PREFFORMAT:
						case WDT_PREFNAMES:
						case WDT_PREFCHOICE:
						openappwindow_addfreelock:  /***************************************************************/
							if (wd->wd_Data)   // if it is map-related
								wd->wd_Lock = AddFreeLock(wd->wd_Data,win);
							break;

						case WDT_PREVIEW:
							RefreshPreviewSize(win);
							break;
					}
					RefreshAppWindow(win,wd);
					GT_RefreshWindow(win,NULL);
					SetMenuStrip(win,rxpage ? rxpage->pg_Mappe->mp_Prefs.pr_Menu : prefs.pr_Menu);

					ReleaseSemaphore(&gWindowSemaphore);
					return win;
				}
			}
			FreeGadgets(wd->wd_Gadgets);
		}
		FreeMem(wd,sizeof(struct winData));
	}
	ReleaseSemaphore(&gWindowSemaphore);

	return NULL;
}
#endif
