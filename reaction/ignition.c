/* Main application
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#define CATCOMP_CODE
#define CATCOMP_BLOCK

#include "types.h"
#include "funcs.h"
#include "version.h"
#ifdef __amigaos4__ 
	#include <stdarg.h>

	#include <proto/gtdrag.h>
	#include <proto/timer.h>
	#include "reaction/reactionGUI.h" // RA
	#include <proto/texteditor.h>
	#include <gadgets/texteditor.h>
	#include <proto/scroller.h>
	#include <gadgets/scroller.h>
	#include <proto/colorwheel.h>
	#include <gadgets/colorwheel.h>
	#include <gadgets/gradientslider.h>
	#include <proto/elf.h>

	extern struct CreateWinData gCreateWinData[]; // RA
	extern struct Window *GetActiveAppWindow(void); // RA

	uint32 *unicode_map;	//Pointer for Unicodes
	char IgnTitle[100];		//ScreenTitle for ignition, must be unique for PubScreen handling
#endif

 
#define GHELP_OBJECT 7   /* delay until the help text is shown */
#define GHELP_GADGET 2   /* (in IntuiTicks) */

#define MAX_SESSIONS 20     /* Maximal number of saved sessions */
#define MAX_RECENTFUNCS 20  /* ... of recently used functions */
#define MAX_FONTSIZES 20    /* ... of font sizes */


void wbstart(struct List *list, struct WBArg *wbarg, LONG first, LONG numargs);
int32 LoadFiles(struct List *list);

extern void InitGadgetLabels(void);

extern struct MinList flangs;
extern struct SignalSemaphore fontSemaphore, gWindowSemaphore;
extern struct Task *asp_printtask;
extern struct ArrayList gTimedRefs;
extern struct CreateWinData gCreateWinData[]; // RA
extern struct Window *GetActiveAppWindow(void); // RA

struct Screen *scr;
struct Window *win,*popwin;
struct winData *wd;
struct Prefs prefs,recycledprefs;
struct TreeList prefstree;
struct MsgPort *iport, *rxport, *wbport, *notifyport;
void   *vi;
APTR   pool;
struct IntuiMessage imsg;
struct RexxMsg *rxmsg;
static struct timerequest *sTimeRequest;
uint32 sTimeInSeconds;
struct WBStartup *sm;
BPTR   shelldir;
APTR   gAmigaGuide, appicon;
struct DiskObject *appdo;
struct IntuiText itext = {1, 0, 1, 0, 0, NULL, NULL, NULL};
WORD   fontheight, barheight, boxwidth, itemheight, itemwidth;
WORD   bborder, lborder, rborder, linelen;
struct Gadget *gad;
struct NewGadget ngad;
struct Image *rightImg, *leftImg, *upImg, *downImg, *pincImage, *logoImage;
struct DrawInfo *dri;
struct ScreenModeRequester *scrReq;
struct FileRequester *fileReq;
struct FontRequester *fontReq;
STRPTR iconpath,projpath,graphicpath,prefname;
struct MinList gProjects, events, search, replace;
struct MinList intcmds, outputs, history;
struct MinList toolobjs, iotypes, refs, usedfuncs;
struct MinList fewfuncs, funcs, sizes, zooms, scrcolors;
struct MinList images, clips, files, locks,sessions;
struct Page *rxpage;
struct RastPort scrRp;
struct Interrupt *ghelpintr;
Class  *iconobjclass,*framesgclass,*indexgclass,*pagegclass,*scrollergclass;
Class  *arrowiclass,*pictureiclass,*bitmapiclass,*popupiclass,*buttonclass,*colorgclass;
struct Hook passwordEditHook;
struct IOStdReq *ghelpio;
ULONG  ghelpcnt,lastsecs,wd_StatusWidth,wd_PosWidth,dhelpcnt = 0;
ULONG  sigwait,agsig,notifysig;
WORD   lventry,fewftype;
LONG   dithPtrn = 0x5555aaaa, gDPI;
struct Node *stdfamily;
ULONG  stdpointheight;
struct Locale *loc;
bool   ende = false, gScreenHasChanged;
STRPTR pubname = "IGNITION.\0\0\0", gEditor = "ged";
struct RDArgs *ra;
struct Library *GTDragBase, *CyberGfxBase, *TextEditBase, *ScrollerBase;
#ifdef __amigaos4__
	//struct Library *ColorWheelBase, *GradientSliderBase;
	struct CyberGfxIFace *ICyberGfx;
	struct EGlyphEngine EEngine;
	//struct ColorWheelIFace      *IColorWheel;
	//struct GradientSliderIFace  *IGradientSlider;
	char portname[13];
	static struct NewAmigaGuide nag;
#else
	struct Library *ColorWheelBase, *GradientSliderBase, *TimerBase;
#endif
struct Library *BulletBase, *AmigaGuideBase;
bool   noabout = false, gIsBeginner = false;
struct LocaleInfo gLocaleInfo;
BOOL debug = FALSE;

struct InputEvent PUBLIC *
ghelpFunc(REG(a0, struct InputEvent *ie))
{
    if (ie->ie_Class == IECLASS_RAWMOUSE)
        ghelpcnt = 0;

	return ie;
}


void
EraseGadgetBox(struct RastPort *rp,struct Gadget *gad)
{
    long x,y,w;

    if (!gad)
        return;

    x = gad->LeftEdge - ((gad->GadgetType & GTYP_STRGADGET) ? 2 : 0);
    y = gad->TopEdge;
    w = gad->Width + ((gad->GadgetType & GTYP_STRGADGET) ? 4 : 0);
    EraseRect(rp,x,y,x+w,y+gad->Height-1);
}


void
RemoveToolObj(struct ToolObj *to)
{
    MyRemove(to);
    FreePooled(pool, to, sizeof(struct ToolObj));
}


void
AddToolObj(int32 type, STRPTR name, STRPTR help)
{
    struct ToolObj *to;

    if ((to = AllocPooled(pool, sizeof(struct ToolObj))) != 0) {
        to->to_Type = type;
        /* strcpy(to->to_Name,name); */
        MyAddTail(&toolobjs, to);
    }
}


struct MinList *
GetIconObjsList(struct Prefs *pr)
{
    if (HasPrefsModule(pr, WDT_PREFICON))
        return &pr->pr_IconObjs;

    return &prefs.pr_IconObjs;
}


void
FreeIconObjs(struct MinList *l)
{
    struct IconObj *io;

    while ((io = (APTR)MyRemHead(l)) != 0) {
        FreeString(io->io_AppCmd);
        FreePooled(pool, io, sizeof(struct IconObj));
    }
}


struct IconObj *
CopyIconObj(struct IconObj *io)
{
    struct IconObj *sio;

    if ((sio = AllocPooled(pool, sizeof(struct IconObj))) != 0) {
        sio->io_AppCmd = AllocString(io->io_AppCmd);
        sio->io_Node.in_Name = sio->io_AppCmd;
        sio->io_Node.in_Image = io->io_Node.in_Image;
    }
    return sio;
}


void
AddIconObj(struct MinList *list, struct AppCmd *ac, long pos)
{
    struct IconObj *io;

    if ((io = AllocPooled(pool,sizeof(struct IconObj))) == NULL)
        return;

    if (ac) {
        io->io_AppCmd = AllocString(ac->ac_Node.in_Name);
        io->io_Node.in_Name = io->io_AppCmd;
    /*  if (!strcmp(io->io_AppCmd,"Leerfeld"))
            io->io_AppCmd = NULL; */
        io->io_Node.in_Image = ac->ac_Node.in_Image;
    }
    InsertAt(list,(struct Node *)io,pos);
}


void
MakeMarkText(struct Page *page, STRPTR t)
{
    struct Term *k;
    char s[64];

	// TODO: this is language specific!
    if ((prefs.pr_Table->pt_Flags & PTF_MARKSUM) != 0)
        strcpy(s, "summe(");
    else
        strcpy(s, "mittelwert(");
    strcat(s, t);  strcat(s, ")");

    if ((k = CreateTree(page, s)) != 0) {
        strcat(t, " (");
        if ((prefs.pr_Table->pt_Flags & PTF_MARKAVERAGE) != 0)
            strcat(t, "ø ");
        strcat(t, ita(TreeValue(k), -3, ITA_NONE));
        strcat(t, ")");
        DeleteTree(k);
    }
}


void
DisplayTablePos(struct Page *page)
{
    struct winData *wd;
    struct tableField *tf;
    long w = wd_PosWidth - 4;
    char t[64];

    if (!page->pg_Mappe->mp_Prefs.pr_Disp->pd_FormBar || !page->pg_Window)
        return;

    wd = (struct winData *)page->pg_Window->UserData;
    itext.FrontPen = 1;
    itext.ITextFont = scr->Font;
    EraseRect(page->pg_Window->RPort, 10, wd->wd_FormY + 5, 9 + w, wd->wd_FormY + fontheight + 6);

    if (page->pg_MarkCol == -1) {
        struct Name *nm,*snm = NULL;

        if (page->pg_Gad.DispPos == PGS_NONE)
            return;
        foreach (&page->pg_Mappe->mp_Prefs.pr_Names, nm) {
            if (nm->nm_Node.ln_Type == NMT_CELL
				&& nm->nm_Page == page
				&& PosInTablePos(&nm->nm_TablePos, page->pg_Gad.cp.cp_Col, page->pg_Gad.cp.cp_Row)) {
                snm = nm;
                if (!nm->nm_TablePos.tp_Width && nm->nm_TablePos.tp_Height)
                    break;
            }
        }
        if (snm)
            itext.IText = snm->nm_Node.ln_Name;
        else
            itext.IText = Coord2String(page->pg_Gad.cp.cp_Col, page->pg_Gad.cp.cp_Row);
    } else {
        strcpy(t, Coord2String(page->pg_MarkCol, page->pg_MarkRow));
        strcat(t, ":");
        strcat(t, Coord2String(page->pg_MarkWidth != -1 ? page->pg_MarkCol + page->pg_MarkWidth : page->pg_Rows, page->pg_MarkHeight != -1 ? page->pg_MarkRow + page->pg_MarkHeight : page->pg_Rows));
        if (prefs.pr_Table->pt_Flags & (PTF_MARKSUM | PTF_MARKAVERAGE))
            MakeMarkText(page, t);
        itext.IText = t;
    }
    makeClip(page->pg_Window, 10, wd->wd_FormY + 5, w - 1, fontheight + 1);
    PrintIText(page->pg_Window->RPort, &itext, 10 + ((w - IntuiTextLength(&itext)) >> 1), wd->wd_FormY + 6);
    freeClip(page->pg_Window);

    {
        STRPTR s = NULL;
        BOOL disabled = FALSE;

        if ((tf = GetTableField(page, page->pg_Gad.cp.cp_Col, page->pg_Gad.cp.cp_Row)) != 0) {
            if ((tf->tf_Flags & TFF_SECURITY) > TFF_STATIC)
                disabled = TRUE;
            else
                s = tf->tf_Original;
        }
        GT_SetGadgetAttrs(GadgetAddress(page->pg_Window, GID_FORM), page->pg_Window, NULL, GTST_String, s, GA_Disabled, disabled, TAG_END);
    }
}


void
DrawIconBar(struct Prefs *pr, struct Window *win, struct winData *wd)
{
    if (pr->pr_Disp->pd_IconBar == PDIB_TOP) {
        SetAPen(win->RPort, 2);
        DrawDithRect(win->RPort, win->BorderLeft, wd->wd_IconY, win->Width - 1 - win->BorderRight, wd->wd_IconY - 2 + wd->wd_IconH);
        if (pr->pr_Disp->pd_ToolBar) {
            Move(win->RPort, win->BorderLeft, wd->wd_IconY + wd->wd_IconH);
            Draw(win->RPort, win->Width - 1 - win->BorderRight, wd->wd_IconY + wd->wd_IconH);
        }
        SetAPen(win->RPort, 1);
        Move(win->RPort, win->BorderLeft, wd->wd_IconY + wd->wd_IconH - 1);
        Draw(win->RPort, win->Width - 1 - win->BorderRight, wd->wd_IconY + wd->wd_IconH - 1);
    } else {
        SetAPen(win->RPort, 2);
        DrawDithRect(win->RPort, win->BorderLeft, wd->wd_IconY + 1, win->BorderLeft + wd->wd_IconW - 2, wd->wd_IconY + wd->wd_IconH);
        Move(win->RPort, win->BorderLeft, wd->wd_IconY);
        Draw(win->RPort, win->BorderLeft + wd->wd_IconW - 2, wd->wd_IconY);
        SetAPen(win->RPort, 1);
        Move(win->RPort, win->BorderLeft + wd->wd_IconW - 1, wd->wd_IconY);
        Draw(win->RPort, win->BorderLeft + wd->wd_IconW - 1, wd->wd_IconY + wd->wd_IconH);
    }
}


static void
frameToolGadgets(struct Window *win,long idA,long idB)
{
    if ((gad = GadgetAddress(win, idA)) != 0) {
        Move(win->RPort,gad->LeftEdge-1,gad->TopEdge+gad->Height-1);
        Draw(win->RPort,gad->LeftEdge-1,gad->TopEdge-1);
        for(;(idA <= idB) && GadgetAddress(win,idA);gad = GadgetAddress(win,idA++));
        Draw(win->RPort,gad->LeftEdge-1+gad->Width,gad->TopEdge-1);
    }
}


void
DrawToolBar(struct Prefs *pr,struct Window *win,struct winData *wd)
{
	int32 i;

	SetAPen(win->RPort, 2);
	DrawDithRect(win->RPort, win->BorderLeft, wd->wd_ToolY, win->Width - 1 - win->BorderRight, wd->wd_ToolY - 2 + wd->wd_ToolH);

	for (i = GID_FONT; i <= GID_ZOOM; i++)
		EraseGadgetBox(win->RPort, GadgetAddress(win, i));

	SetAPen(win->RPort, 1);
	Move(win->RPort, win->BorderLeft, wd->wd_ToolY + wd->wd_ToolH - 1);
	Draw(win->RPort, win->Width - 1 - win->BorderRight, wd->wd_ToolY + wd->wd_ToolH - 1);

	frameToolGadgets(win, GID_PLAIN, GID_UNDERLINED);
	frameToolGadgets(win, GID_JLEFT, GID_JRIGHT);
	frameToolGadgets(win, GID_JTOP, GID_JBOTTOM);
}


void
DrawFormBar(struct Prefs *pr,struct Window *win,struct winData *wd)
{
#ifdef __amigaos4__
	DrawBevelBox(win->RPort, win->BorderLeft + 4, wd->wd_FormY + 4, wd_PosWidth, fontheight + 4, GT_VisualInfo, vi, GTBB_Recessed, TRUE, GTBB_FrameType, BBFT_BUTTON, TAG_END);
#else
	DrawBevelBox(win->RPort, win->BorderLeft + 4, wd->wd_FormY + 4, wd_PosWidth, fontheight + 4, GT_VisualInfo, vi, GTBB_Recessed, TRUE, GTBB_FrameType, BBFT_BUTTON);
#endif

    if (pr->pr_Disp->pd_FormBar == PDFB_TOP)
    {
		SetAPen(win->RPort, 1);
        Move(win->RPort,win->BorderLeft,wd->wd_FormY+wd->wd_FormH);
        Draw(win->RPort,win->Width-1-win->BorderRight,wd->wd_FormY+wd->wd_FormH);
    }
    else
    {
		SetAPen(win->RPort, 2);
        Move(win->RPort,win->BorderLeft,wd->wd_FormY);
        Draw(win->RPort,win->Width-1-win->BorderRight,wd->wd_FormY);
    }
}


void
DrawStatusFlags(struct Mappe *mp,struct Window *win)
{
    struct winData *wd = (struct winData *)win->UserData;

    if (!mp->mp_Prefs.pr_Disp->pd_HelpBar || !wd->wd_Data)
        return;

    EraseRect(win->RPort,win->Width-win->BorderRight-wd_StatusWidth,wd->wd_HelpY+3,win->Width-win->BorderRight-4,wd->wd_HelpY+wd->wd_HelpH-2);
    makeClip(win,win->Width-win->BorderRight-wd_StatusWidth,wd->wd_HelpY,wd_StatusWidth-4,wd->wd_HelpH);

#ifdef __amigaos4__
    if (mp->mp_Flags & MPF_SCRIPTS)
		itext.IText = GetString(&gLocaleInfo, MSG_INTERACTIVE_STATUS);
    else
		itext.IText = GetString(&gLocaleInfo, MSG_EDIT_STATUS);
#else
    if (mp->mp_Flags & MPF_SCRIPTS)
		itext.IText = "inter";
    else
		itext.IText = "edit";
#endif

    itext.FrontPen = 1;
    itext.BackPen = 0;
    itext.ITextFont = scr->Font;
	PrintIText(win->RPort, &itext, win->Width - win->BorderRight - 4 - IntuiTextLength(&itext), wd->wd_HelpY + 3);
    freeClip(win);
}


void
DrawStatusText(struct Page *page, CONST_STRPTR t)
{
    if (page && page->pg_Window && page->pg_Mappe->mp_Prefs.pr_Disp->pd_HelpBar && t)
        DrawHelpText(page->pg_Window,NULL,t);
}


void
DrawHelpText(struct Window *win, struct Gadget *gad, CONST_STRPTR t)
{
    struct winData *wd;
    struct Page *page;
    struct AppCmd *ac;
    char   s[256];

    wd = (struct winData *)win->UserData;
    page = wd->wd_Data;

    if (gad)
    {
        switch(gad->GadgetID)
        {
            case GID_PAGE:
            case GID_PAGEPOPPER:
                t = GetString(&gLocaleInfo, MSG_CHOOSE_PAGE_HELP);
                break;
            case GID_FORM:
                t = GetString(&gLocaleInfo, MSG_INPUT_FORMULA_HELP);
                break;
            case GID_POPPER:
                t = GetString(&gLocaleInfo, MSG_FAST_FUNCTION_CHOOSER_HELP);
                break;
            case GID_CHOOSE:
                t = GetString(&gLocaleInfo, MSG_FUNCTION_CHOOSER_HELP);
                break;
            case GID_FONT:
            case GID_POPFONT:
                t = GetString(&gLocaleInfo, MSG_CHOOSE_FONT_HELP);
                break;
            case GID_SIZE:
                t = GetString(&gLocaleInfo, MSG_SET_FONT_SIZE_HELP);
                break;
            case GID_POPSIZE:
                t = GetString(&gLocaleInfo, MSG_CHOOSE_FONT_SIZE_HELP);
                break;
            case GID_ZOOM:
                t = GetString(&gLocaleInfo, MSG_SET_ZOOM_HELP);
                break;
            case GID_POPZOOM:
                t = GetString(&gLocaleInfo, MSG_CHOOSE_ZOOM_HELP);
                break;
            case GID_APEN:
                t = GetString(&gLocaleInfo, MSG_SET_TEXTPEN_HELP);
                break;
            case GID_BPEN:
                t = GetString(&gLocaleInfo, MSG_SET_BACKPEN_HELP);
                break;
            case GID_PLAIN:
                t = GetString(&gLocaleInfo, MSG_PLAIN_FONT_HELP);
                break;
            case GID_BOLD:
                t = GetString(&gLocaleInfo, MSG_BOLD_FONT_HELP);
                break;
            case GID_ITALIC:
                t = GetString(&gLocaleInfo, MSG_ITALICS_FONT_HELP);
                break;
            case GID_UNDERLINED:
                t = GetString(&gLocaleInfo, MSG_UNDERLINED_FONT_HELP);
                break;
            case GID_JLEFT:
                t = GetString(&gLocaleInfo, MSG_ALIGN_LEFT_HELP);
                break;
            case GID_JCENTER:
                t = GetString(&gLocaleInfo, MSG_ALIGN_CENTER_HELP);
                break;
            case GID_JRIGHT:
                t = GetString(&gLocaleInfo, MSG_ALIGN_RIGHT_HELP);
                break;
            case GID_JTOP:
                t = GetString(&gLocaleInfo, MSG_VALIGN_TOP_HELP);
                break;
            case GID_JVCENTER:
                t = GetString(&gLocaleInfo, MSG_VALIGN_MIDDLE_HELP);
                break;
            case GID_JBOTTOM:
                t = GetString(&gLocaleInfo, MSG_VALIGN_BOTTOM_HELP);
                break;
            case GID_ICONOBJ:
                if ((ac = FindAppCmd(page,(STRPTR)gad->UserData)) && ac->ac_HelpText)
                {
                    t = ac->ac_HelpText;  s[0] = 0;
                    if (!stricmp("Undo",(STRPTR)gad->UserData))
                        sprintf(s,t,page->pg_CurrentUndo && page->pg_CurrentUndo->un_Node.ln_Succ ? page->pg_CurrentUndo->un_Node.ln_Name : "-");
                    else if (!stricmp("Redo",(STRPTR)gad->UserData))
                        sprintf(s,t,page->pg_CurrentUndo && page->pg_CurrentUndo->un_Node.ln_Pred->ln_Pred ? page->pg_CurrentUndo->un_Node.ln_Pred->ln_Name : "-");
                    if (*s)
                        t = s;
                }
                break;
            case GID_TOOLOBJ:
                break;
        }
    }
    if (!page->pg_Mappe->mp_Prefs.pr_Disp->pd_HelpBar)
    {
		ShowPopUpText((STRPTR)t, -1, -1);
        return;
    }
    if (t || dhelpcnt++ > 3)
        EraseRect(win->RPort,lborder,wd->wd_HelpY+3,win->Width-win->BorderRight-4-wd_StatusWidth,wd->wd_HelpY+wd->wd_HelpH-2);
    if (t)
    {
        makeClip(win,lborder,wd->wd_HelpY,win->Width-win->BorderRight-4-lborder-wd_StatusWidth,wd->wd_HelpH);
        itext.IText = t;
        itext.FrontPen = 1;
        itext.BackPen = 0;
        itext.ITextFont = scr->Font;
        PrintIText(win->RPort,&itext,lborder,wd->wd_HelpY+3);
        freeClip(win);
        dhelpcnt = 0;
    }
}


void
DrawHelpBar(struct Prefs *pr, struct Window *win, struct winData *wd)
{
	SetAPen(win->RPort, 1);
	Move(win->RPort, win->BorderLeft,wd->wd_HelpY);
	Draw(win->RPort, win->Width-1-win->BorderRight,wd->wd_HelpY);
	Move(win->RPort, win->Width-win->BorderRight-wd_StatusWidth-2,wd->wd_HelpY+2);
	Draw(win->RPort, win->Width-win->BorderRight-wd_StatusWidth-2,win->Height-1-win->BorderBottom);
	SetAPen(win->RPort, 2);
	Move(win->RPort, win->BorderLeft,wd->wd_HelpY+1);
	Draw(win->RPort, win->Width-1-win->BorderRight,wd->wd_HelpY+1);
	Move(win->RPort, win->Width-win->BorderRight-wd_StatusWidth-1,wd->wd_HelpY+2);
	Draw(win->RPort, win->Width-win->BorderRight-wd_StatusWidth-1,win->Height-1-win->BorderBottom);
    DrawStatusFlags(((struct Page *)wd->wd_Data)->pg_Mappe,win);
}


void
DrawBars(struct Window *win)
{
    struct winData *wd;
    struct PrefDisp *pd;
    struct Prefs *pr;

    wd = (APTR)win->UserData;
    pr = &((struct Page *)wd->wd_Data)->pg_Mappe->mp_Prefs;
    pd = pr->pr_Disp;

    SetBPen(win->RPort,0);
    if (pd->pd_IconBar)
        DrawIconBar(pr,win,wd);
    if (pd->pd_ToolBar)
        DrawToolBar(pr,win,wd);
    if (pd->pd_FormBar)
        DrawFormBar(pr,win,wd);
    if (pd->pd_HelpBar)
        DrawHelpBar(pr,win,wd);
    DrawTableFrame(win,wd);
}


uint32
IsOverProjSpecial(struct Window *win, int32 x, int32 y)
{
    struct winData *wd;
    struct Prefs *pr;

    if (!win)
        return 0L;

    wd = (struct winData *)win->UserData;
    pr = &((struct Page *)wd->wd_Data)->pg_Mappe->mp_Prefs;

    if (pr->pr_Disp->pd_HelpBar && x >= win->Width-win->BorderRight-wd_StatusWidth && x < win->Width-win->BorderRight && y > wd->wd_HelpY && y < wd->wd_HelpY+wd->wd_HelpH)
        return GID_STATUS;

    if (pr->pr_Disp->pd_FormBar && x >= win->BorderLeft+4 && x <= win->BorderLeft+4+wd_PosWidth && y > wd->wd_FormY && y < wd->wd_FormY+wd->wd_FormH)
        return GID_POSITION;

    return 0L;
}


void
UpdateInteractive(struct Mappe *mp, bool refresh)
{
	struct Node *ln;
	
	if ((mp->mp_Flags & MPF_SCRIPTS) != 0) {
		foreach(&mp->mp_Pages, ln) {
			SetMark((struct Page *)ln, -1, 0, 0, 0);
			DeselectGObjects((struct Page *)ln);
		}
		RefreshMaskFields(mp, refresh);
	}

	if (mp->mp_Window) {
		if (mp->mp_Flags & MPF_SCRIPTS)
			mp->mp_Window->Flags |= WFLG_RMBTRAP;
		else
			mp->mp_Window->Flags &= ~WFLG_RMBTRAP;
		DrawStatusFlags(mp, mp->mp_Window);
	}
}


void
RefreshFlagGadget(struct Window *win,struct Gadget *gad,long style,long type,long comp)
{
    bool flagged = ((style & comp) == type);

    if (gad && flagged != (bool)(gad->Flags & GFLG_SELECTED))
    {
        if ((gad->GadgetType & GTYP_GTYPEMASK) != GTYP_CUSTOMGADGET)
        {
            if (flagged)
                gad->Flags |= GFLG_SELECTED;
            else
                gad->Flags &= ~GFLG_SELECTED;
            RefreshGList(gad,win,NULL,1);
        }
        /*else
            SetGadgetAttrs(gad,win,NULL,GA_Selected,flagged,TAG_END);*/
    }
}


void
RefreshToolBar(struct Page *page)
{
    struct Window *win;
    struct tableField *tf;
    struct Gadget *gad;
    struct FontFamily *ff;
    long   height,style;
    ULONG  align;

    if (!page || !(win = page->pg_Window))
        return;

    ff = (struct FontFamily *)page->pg_Family;
    height = page->pg_PointHeight;
    style = 0;
    align = TFA_BOTTOM | TFA_LEFT;

    if (page->pg_Gad.DispPos != PGS_NONE && (tf = page->pg_Gad.tf))
    {
        if ((tf->tf_Text || tf->tf_Original || tf->tf_Flags & TFF_FONTSET) && tf->tf_FontInfo)
        {
            ff = (struct FontFamily *)tf->tf_FontInfo->fi_Family;
            height = tf->tf_FontInfo->fi_FontSize->fs_PointHeight;
            style = tf->tf_FontInfo->fi_Style;
        }
        align = tf->tf_Alignment;
    }
    if ((gad = GadgetAddress(win,GID_FONT)) != 0) {
        if (gad->UserData != ff)
        {
            GT_SetGadgetAttrs(gad,win,NULL,GTTX_Text,ff->ff_Node.ln_Name,TAG_END);
            gad->UserData = ff;
        }
    }
    if ((gad = GadgetAddress(win,GID_SIZE)) && (long)gad->UserData != height)
    {
        char txt[32];

        strcpy(txt,ita(height/65536.0,-3,ITA_NONE));
        strcat(txt," pt");

        GT_SetGadgetAttrs(gad,win,NULL,GTST_String,txt,TAG_END);
        gad->UserData = (APTR)height;
    }
    RefreshFlagGadget(win,GadgetAddress(win,GID_PLAIN),style,FS_PLAIN,~FS_PLAIN);
    RefreshFlagGadget(win,GadgetAddress(win,GID_BOLD),style,FS_BOLD,FS_BOLD);
    RefreshFlagGadget(win,GadgetAddress(win,GID_ITALIC),style,FS_ITALIC,FS_ITALIC);
    RefreshFlagGadget(win,GadgetAddress(win,GID_UNDERLINED),style,FS_UNDERLINED,FS_UNDERLINED);

    RefreshFlagGadget(win,GadgetAddress(win,GID_JLEFT),align,TFA_LEFT,TFA_HCENTER);
    RefreshFlagGadget(win,GadgetAddress(win,GID_JCENTER),align,TFA_HCENTER,TFA_HCENTER);
    RefreshFlagGadget(win,GadgetAddress(win,GID_JRIGHT),align,TFA_RIGHT,TFA_HCENTER);

    RefreshFlagGadget(win,GadgetAddress(win,GID_JTOP),align,TFA_TOP,TFA_VCENTER);
    RefreshFlagGadget(win,GadgetAddress(win,GID_JVCENTER),align,TFA_VCENTER,TFA_VCENTER);
    RefreshFlagGadget(win,GadgetAddress(win,GID_JBOTTOM),align,TFA_BOTTOM,TFA_VCENTER);

    if ((gad = GadgetAddress(win,GID_ZOOM)) && (long)gad->UserData != page->pg_Zoom)
    {
        char txt[32];

        ProcentToString(page->pg_Zoom,txt);
        GT_SetGadgetAttrs(gad,win,NULL,GTST_String,txt,TAG_END);
        gad->UserData = (APTR)page->pg_Zoom;
        if (rxpage == page && (win = GetAppWindow(WDT_ZOOM)) && (gad = GadgetAddress(win,1)))
            GT_SetGadgetAttrs(gad,win,NULL,GTST_String,txt,TAG_END);
    }
}


struct Gadget *
MakeToolGadgets(struct winData *wd,struct Gadget *gad,long w)
{
    int    colorwidth = 2*boxwidth;
    struct Gadget *pgad;
    struct Page *page;
    char   t[16];
	
    page = wd->wd_Data;
    w -= downImg->Width;

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = wd->wd_ToolY+3;
    ngad.ng_Width = TLn("CG Triumvirate Light")+42;
    if (ngad.ng_LeftEdge+ngad.ng_Width > w)
        return gad;
    ngad.ng_Height = fontheight+4;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID = GID_FONT;
    gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GTTX_Text,page->pg_Family ? page->pg_Family->ln_Name : "-",TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge+boxwidth > w)
        return gad;
    ngad.ng_Flags = PLACETEXT_IN;
    ngad.ng_GadgetID = GID_POPFONT;
    gad = CreatePopGadget(wd,gad,FALSE);

    ngad.ng_LeftEdge += ngad.ng_Width+8;
    ngad.ng_Width = TLn("142 pt")+16;
    if (ngad.ng_LeftEdge + ngad.ng_Width > w)
        return gad;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID = GID_SIZE;
    sprintf(t,"%ld pt",page->pg_PointHeight >> 16);
    gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,t,TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_POPSIZE;
    gad = CreatePopGadget(wd,gad,FALSE);

    ngad.ng_TopEdge++;
    ngad.ng_Height--;
    ngad.ng_LeftEdge += ngad.ng_Width+8;
    ngad.ng_Width = boxwidth;
    if (ngad.ng_LeftEdge+ngad.ng_Width > w)
        return gad;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FONT_PLAIN_CHAR);
    ngad.ng_GadgetID = GID_PLAIN;
    gad = CreateGadget(BUTTON_KIND,gad,&ngad,GA_Immediate,TRUE,TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge+boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_BOLD;
    gad = CreateToolGadget(wd,IMG_BOLD,gad);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge+boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_ITALIC;
    gad = CreateToolGadget(wd,IMG_ITALIC,gad);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge+boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_UNDERLINED;
    gad = CreateToolGadget(wd,IMG_UNDERLINED,gad);

    ngad.ng_TopEdge--;
    ngad.ng_LeftEdge += 8+boxwidth;
    if (ngad.ng_LeftEdge+colorwidth > w)
        return gad;

	pgad = NewObj(wd, WOT_GADGET, colorgclass, NULL,
			GA_Top,       ngad.ng_TopEdge,
            GA_Left,      ngad.ng_LeftEdge,
            GA_Width,     colorwidth,
            GA_Height,    fontheight+4,
            GA_Previous,  gad,
            CGA_Color,    page->pg_APen,
            GA_ID,        GID_APEN,
            TAG_END);

    ngad.ng_LeftEdge += colorwidth;
    if (ngad.ng_LeftEdge+boxwidth > w)
        return pgad;
    ngad.ng_GadgetID = GID_APEN;
    if ((gad = CreatePopGadget(wd,pgad,FALSE)) != 0)
        gad->UserData = pgad;

    ngad.ng_LeftEdge += 8+boxwidth;
    if (ngad.ng_LeftEdge+colorwidth > w)
        return gad;
	pgad = NewObj(wd, WOT_GADGET, colorgclass, NULL,
			GA_Top,       ngad.ng_TopEdge,
            GA_Left,      ngad.ng_LeftEdge,
            GA_Width,     colorwidth,
            GA_Height,    fontheight+4,
            GA_Previous,  gad,
            CGA_Color,    page->pg_BPen,
            GA_ID,        GID_BPEN,
            TAG_END);

    ngad.ng_LeftEdge += colorwidth;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return pgad;
    ngad.ng_GadgetID = GID_BPEN;
    if ((gad = CreatePopGadget(wd,pgad,FALSE)) != 0)
        gad->UserData = pgad;

    ngad.ng_TopEdge++;
	ngad.ng_LeftEdge += ngad.ng_Width + 8;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_JLEFT;
    gad = CreateToolGadget(wd,IMG_JLEFT,gad);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_JCENTER;
    gad = CreateToolGadget(wd,IMG_JCENTER,gad);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_JRIGHT;
    gad = CreateToolGadget(wd,IMG_JRIGHT,gad);

    ngad.ng_LeftEdge += ngad.ng_Width+6;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_JTOP;
    gad = CreateToolGadget(wd,IMG_JTOP,gad);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_JVCENTER;
    gad = CreateToolGadget(wd,IMG_JVCENTER,gad);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge + boxwidth > w)
        return gad;
    ngad.ng_GadgetID = GID_JBOTTOM;
    gad = CreateToolGadget(wd,IMG_JBOTTOM,gad);

    ngad.ng_TopEdge--;
    ngad.ng_Height++;
    ngad.ng_LeftEdge += ngad.ng_Width+8;
    ngad.ng_Width = TLn("1000%")+16;
    if (ngad.ng_LeftEdge + ngad.ng_Width > w)
        return gad;
    ngad.ng_GadgetText = NULL;
    ngad.ng_GadgetID = GID_ZOOM;
    ProcentToString(page->pg_Zoom,t);
    gad = CreateGadget(STRING_KIND,gad,&ngad,GTST_String,t,TAG_END);

    ngad.ng_LeftEdge += ngad.ng_Width;
    if (ngad.ng_LeftEdge+boxwidth > w)
        return gad;

    ngad.ng_GadgetID = GID_POPZOOM;
    gad = CreatePopGadget(wd,gad,FALSE);

    return gad;
}


int32
MakeIconGads(struct Prefs *pr,struct winData *wd,struct Gadget *gad,long w,long h)
{
    struct IconObj *io; 
    struct Image *img;
    int32  count = 0;

    ngad.ng_LeftEdge = lborder;
    ngad.ng_TopEdge = wd->wd_IconY+3;
    ngad.ng_GadgetID = GID_ICONOBJ;

    foreach(GetIconObjsList(pr),io)
    {
        ngad.ng_Width = pr->pr_Icon->pi_Width;
        ngad.ng_Height = pr->pr_Icon->pi_Height;
        if (io->io_Node.in_Image)
        {
            img = NewObj(wd,WOT_ICONIMG,NULL,"frameiclass",IA_FrameType,FRAME_DEFAULT,TAG_END);
            gad = NewObj(wd,WOT_GADGET,iconobjclass,NULL,gad ? GA_Previous : TAG_IGNORE,   gad,
                                                                                                     GA_LabelImage, io->io_Node.in_Image,
                                                                                                     GA_Image,      img,
                                                                                                     GA_Left,       ngad.ng_LeftEdge,
                                                                                                     GA_Top,        ngad.ng_TopEdge,
                                                                                                     GA_UserData,   AllocString(io->io_AppCmd),
                                                                                                     GA_ID,         GID_ICONOBJ,
                                                                                                     ICA_TARGET,    ICTARGET_IDCMP,
                                                                                                     TAG_END);
            SetAttrs(gad,GA_Width,ngad.ng_Width,GA_Height,ngad.ng_Height,TAG_END);  // um Frame-Image zu umgehen
            count++;
        }
        else
            ngad.ng_Width = ngad.ng_Height = pr->pr_Icon->pi_Spacing;
        if (pr->pr_Disp->pd_IconBar == PDIB_LEFT)
        {
            ngad.ng_TopEdge += ngad.ng_Height;
            if (ngad.ng_TopEdge + pr->pr_Icon->pi_Height > wd->wd_IconH + wd->wd_IconY)
                break;
        }
        else
        {
            ngad.ng_LeftEdge += ngad.ng_Width;
            if (ngad.ng_LeftEdge + pr->pr_Icon->pi_Width > wd->wd_IconW)
                break;
        }
    }
    return count;
}


void
FreeWinIconObjs(struct Window *win,struct winData *wd)
{
	struct winObj *wo, *swo;

	for (wo = (APTR)wd->wd_Objs.mlh_Head; wo->wo_Node.mln_Succ;) {
		swo = (struct winObj *)wo->wo_Node.mln_Succ;
		if (wo->wo_Type == WOT_GADGET || wo->wo_Type == WOT_ICONIMG) {
			MyRemove(wo);
			DisposeObject(wo->wo_Obj);
			FreePooled(pool, wo, sizeof(struct winObj));
		}
		wo = swo;
	}
	wd->wd_ExtData[0] = NULL;
}


void
RefreshProjectWindow(struct Window *win,BOOL all)
{
    struct winData *wd;
    struct Page *page;

    if (!win)
        return;

    wd = (struct winData *)win->UserData;

    if (all)
    {
        page = wd->wd_Data;
        RemoveGadgets(&win->FirstGadget,FALSE);
        FreeWinIconObjs(win,wd);
        RemoveGList(win,wd->wd_Gadgets,CountGadToolGads(win));
        FreeGadgets(wd->wd_Gadgets);
        wd->wd_Gadgets = NULL;
        EraseRect(win->RPort,win->BorderLeft,win->BorderTop,win->Width-1-win->BorderRight,win->Height-1-win->BorderBottom);

        AddGList(win,MakeProjectGadgets(wd,win->Width,win->Height),-1,-1,NULL);
        DrawBars(win);
        RefreshGadgets(win->FirstGadget,win,NULL);
        GT_RefreshWindow(win,NULL);
#ifdef __amigaos4__
        RefreshWindowFrame(win); //Manchmal ist der Rahmen noch "beschädigt"
#endif
        if (page->pg_MarkCol != -1)
            setTableCoord(page,(struct Rect32 *)&page->pg_MarkX1,page->pg_MarkCol,page->pg_MarkRow,page->pg_MarkWidth,page->pg_MarkHeight);
        setCoordPkt(page,&page->pg_Gad.cp,page->pg_Gad.cp.cp_Col,page->pg_Gad.cp.cp_Row);
        page->pg_Gad.cp.cp_X += page->pg_wTabX;
        page->pg_Gad.cp.cp_Y += page->pg_wTabY;
        SetPageProps(page);
    }
    DrawTable(win);
}


void
RefreshProjWindows(bool all)
{
    struct Window *savewin;
    struct winData *savewd;

    if (!scr)
        return;

    SetBusy(true,BT_APPLICATION);

    savewin = win;
    savewd = wd;

    for (win = scr->FirstWindow;win;win = win->NextWindow)
    {
        wd = (struct winData *)win->UserData;
        if (win->UserPort == iport && wd->wd_Type == WDT_PROJECT)
            RefreshProjectWindow(win,all);
    }
    win = savewin;
    wd = savewd;

    SetBusy(false,BT_APPLICATION);
}


struct Gadget *
MakeProjectGadgets(struct winData *wd,long w,long h)
{
    struct PrefDisp *pd;
    struct Prefs *pr;
    struct Page *page;
    long   y;

    if (!wd || !(page = wd->wd_Data))
        return(NULL);

    pr = &page->pg_Mappe->mp_Prefs;
    pd = pr->pr_Disp;

    y = barheight-1;

    if (pd->pd_ToolBar || pd->pd_FormBar == PDFB_TOP || pd->pd_IconBar == PDIB_TOP)
        y++;

    wd->wd_IconH = pr->pr_Icon->pi_Height+7;
    wd->wd_ToolH = fontheight+11;
    wd->wd_FormH = fontheight+11;
    wd->wd_HelpH = fontheight+4;

    if (pd->pd_IconBar == PDIB_TOP)
    {
        wd->wd_IconY = y;
        wd->wd_IconW = w-downImg->Width-4;
        y += wd->wd_IconH;
        if (!pd->pd_ToolBar && pd->pd_FormBar != PDFB_TOP)
            y--;
    }
    if (pd->pd_ToolBar)
    {
        if (pd->pd_IconBar == PDIB_TOP)
            y++;
        wd->wd_ToolY = y;
        y += wd->wd_ToolH;
        if (pd->pd_FormBar != PDFB_TOP)
            y--;
    }
    if (pd->pd_FormBar == PDFB_TOP)
    {
        y--;
        wd->wd_FormY = y;
        y += wd->wd_FormH;
    }
    wd->wd_TabX = lborder;
    if (pd->pd_ShowAntis)
        wd->wd_TabX += pd->pd_AntiWidth;
    wd->wd_TabY = y;
    wd->wd_TabH = h-wd->wd_TabY-1-leftImg->Height;
    if (pd->pd_HelpBar)
    {
        wd->wd_TabH -= wd->wd_HelpH;
        wd->wd_HelpY = wd->wd_TabY+wd->wd_TabH+1;
    }
    if (pd->pd_FormBar == PDFB_BOTTOM)
    {
        wd->wd_TabH -= wd->wd_FormH;
        wd->wd_FormY = wd->wd_TabY+wd->wd_TabH+1;
    }
    if (pd->pd_IconBar == PDIB_LEFT)
    {
        wd->wd_IconY = y+1;
        wd->wd_IconW = pr->pr_Icon->pi_Width+9;
        wd->wd_IconH = wd->wd_TabH-1;
        wd->wd_TabX += wd->wd_IconW;
    }
    wd->wd_TabY += 4;
    if (pd->pd_ShowAntis)
    {
        wd->wd_TabY += pd->pd_AntiHeight;
        wd->wd_TabH -= pd->pd_AntiHeight;
    }
    wd->wd_TabH -= 7;
    wd->wd_TabW = w-downImg->Width-wd->wd_TabX-6;

    if (!pd->pd_ShowAntis && !pd->pd_HelpBar && !pd->pd_ToolBar && pd->pd_IconBar == PDIB_NONE && pd->pd_FormBar == PDFB_NONE)
    {
        wd->wd_TabY -= 3;  wd->wd_TabX -= 4;
        wd->wd_TabW += 9;  wd->wd_TabH += 6;
    }

    page->pg_wTabX = wd->wd_TabX;  page->pg_wTabY = wd->wd_TabY;
    page->pg_wTabW = wd->wd_TabW;  page->pg_wTabH = wd->wd_TabH;

    if (!(/*wd->wd_Gadgets =*/ gad = CreateContext(&wd->wd_Gadgets)))
        return(NULL);

    ngad.ng_VisualInfo = vi;
    ngad.ng_TextAttr = scr->Font;
    ngad.ng_UserData = NULL;
    ngad.ng_TopEdge = scr->BarHeight+4;
    ngad.ng_Height = fontheight+4;

    if (pd->pd_FormBar)
    {
        ngad.ng_LeftEdge = TLn(GetString(&gLocaleInfo, MSG_FORMULA_GAD))+lborder+(y = wd_PosWidth+13);
        ngad.ng_TopEdge = wd->wd_FormY+4;
#ifdef __amigaos4__
        ngad.ng_Width = w-ngad.ng_LeftEdge-upImg->Width-82-2*boxwidth-TLn(GetString(&gLocaleInfo, MSG_CHOOSE_GAD))-TLn("Seite 999") - 51;
#else
        ngad.ng_Width = w-ngad.ng_LeftEdge-upImg->Width-82-2*boxwidth-TLn(GetString(&gLocaleInfo, MSG_CHOOSE_GAD))-TLn("Seite 999");
#endif
        ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FORMULA_GAD);
        ngad.ng_Flags = PLACETEXT_LEFT;
        ngad.ng_GadgetID = GID_FORM;
        if (ngad.ng_Width > 24)
            gad = CreateGadget(STRING_KIND,gad,&ngad,GA_Immediate,TRUE,GTST_MaxChars,512,TAG_END);

        ngad.ng_LeftEdge += ngad.ng_Width;
        ngad.ng_Flags = PLACETEXT_IN;
        ngad.ng_GadgetID = GID_POPPER;
        if (ngad.ng_LeftEdge > y)
            gad = CreatePopGadget(wd,gad,FALSE);
        else
            ngad.ng_Width = boxwidth;

        ngad.ng_LeftEdge += ngad.ng_Width;
        ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CHOOSE_GAD))+20;
        ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CHOOSE_GAD);
        ngad.ng_GadgetID = GID_CHOOSE;
        if (ngad.ng_LeftEdge > y)
            gad = CreateGadget(BUTTON_KIND,gad,&ngad,TAG_END);

        ngad.ng_LeftEdge += ngad.ng_Width+6;
#ifdef __amigaos4__
        ngad.ng_Width = TLn("Seite 999")+102;
#else
        ngad.ng_Width = TLn("Seite 999")+51;
#endif
        ngad.ng_GadgetText = NULL;
        ngad.ng_GadgetID = GID_PAGE;
        gad = CreateGadget(TEXT_KIND,gad,&ngad,GTTX_Border,TRUE,GTTX_Text,page->pg_Node.ln_Name,TAG_END);

        ngad.ng_LeftEdge += ngad.ng_Width;
        ngad.ng_GadgetID = GID_PAGEPOPPER;
        gad = CreatePopGadget(wd,gad,FALSE);
    }
    if (pd->pd_ToolBar)
        gad = (APTR)MakeToolGadgets(wd,gad,w);
    if (pd->pd_IconBar)
        wd->wd_ExtData[0] = (APTR)MakeIconGads(pr,wd,gad,w,h);

    return(wd->wd_Gadgets);
}


void
NormalizeWindowBox(struct IBox *box)
{
    if (box->Left == -1 || box->Left > scr->Width-boxwidth)
        box->Left = 0;
    if (box->Top == -1 || box->Top > scr->Height-fontheight)
        box->Top = scr->BarHeight+1;
    if (box->Width == -1 || (box->Width+box->Left) > scr->Width)
        box->Width = scr->Width-box->Left;
    if (box->Height == -1 || (box->Height+box->Top) > scr->Height)
        box->Height = scr->Height-box->Top;
}


#ifdef __amigaos4__
//struct Window VARARGS68K *OpenProjWindowA(struct Page *page, struct TagItem *tags);
struct Window *OpenProjWindowA(struct Page *page, struct TagItem *tags)
{
    struct Window *win;
    struct Mappe *mp;
    struct IBox box;
    BYTE   neu = FALSE;

    if (!page)
    {
        mp = NewProject();
        page = NewPage(mp);
        neu = TRUE;
    }
    else
        mp = page->pg_Mappe;
    if (!page)
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_CREATE_PROJECT_ERR));
		return NULL;
    }
    box.Left = GetTagData(WA_Left,mp->mp_WindowBox.Left,(struct TagItem *)tags);
    box.Top = GetTagData(WA_Top,mp->mp_WindowBox.Top,(struct TagItem *)tags);
    box.Width = GetTagData(WA_Width,mp->mp_WindowBox.Width,(struct TagItem *)tags);
    box.Height = GetTagData(WA_Height,mp->mp_WindowBox.Height,(struct TagItem *)tags);

    NormalizeWindowBox(&box);

	if ((wd = AllocPooled(pool, sizeof(struct winData))) != 0) {
        wd->wd_Type = WDT_PROJECT;
        wd->wd_Data = page;
        MyNewList(&wd->wd_Objs);
        wd->wd_BorGads = MakeBorderScroller(wd);
        wd->wd_Server = handleProjIDCMP;

		if (MakeProjectGadgets(wd, box.Width, box.Height))
        {
			if ((win = OpenWindowTags(NULL, WA_Flags,        WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE |WFLG_SIZEGADGET |WFLG_SIZEBBOTTOM | WFLG_SIZEBRIGHT | WFLG_REPORTMOUSE,
                                           WA_Title,        GetPageTitle(page),
                                           WA_Left,         box.Left,
                                           WA_Top,          box.Top,
                                           WA_Width,        box.Width,
                                           WA_Height,       box.Height,
                                           ((prefs.pr_Flags & PRF_SIMPLEPROJS) ? WA_SimpleRefresh : WA_NoCareRefresh) ,TRUE,
                                           WA_NewLookMenus, TRUE,
                                           WA_MenuHelp,     TRUE,
                                           WA_PubScreen,    scr,
                                           WA_Gadgets,      wd->wd_Gadgets,
                                           WA_MinWidth,     wd_PosWidth+70+boxwidth+TLn("Seite 999")+downImg->Width,
                                           WA_MinHeight,    scr->WBorTop+4*fontheight+prefs.pr_Icon->pi_Height+34+leftImg->Height,
                                           WA_MaxWidth,     -1,
                                           WA_MaxHeight,    -1,
                                           WA_ScreenTitle, 	IgnTitle,
                                           TAG_END)) != 0)
            {
                win->UserPort = iport;  win->UserData = (APTR)wd;
#ifdef __amigaos4__
                ModifyIDCMP(win, APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN |	IDCMP_NEWSIZE | IDCMP_IDCMPUPDATE |	IDCMP_SIZEVERIFY | IDCMP_ACTIVEWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_MENUVERIFY | IDCMP_EXTENDEDMOUSE | (prefs.pr_Flags & PRF_SIMPLEPROJS ? IDCMP_REFRESHWINDOW : 0));
#else
                ModifyIDCMP(win, APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_NEWSIZE | IDCMP_IDCMPUPDATE | IDCMP_SIZEVERIFY | IDCMP_ACTIVEWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_MENUVERIFY | (prefs.pr_Flags & PRF_SIMPLEPROJS ? IDCMP_REFRESHWINDOW : 0));
#endif
                AddGList(win,wd->wd_BorGads,-1,-1,NULL);
                SetMenuStrip(win,mp->mp_Prefs.pr_Menu);
                DrawBars(win);
                RefreshGadgets(win->FirstGadget,win,NULL);
                RefreshGList(wd->wd_Gadgets,win,NULL,CountGadToolGads(win));
                GT_RefreshWindow(win,NULL);
                page->pg_Window = mp->mp_Window = win;

                if (!neu)
                {
                    SetMediumSize(mp);
					SetZoom(page, page->pg_Zoom, TRUE, FALSE);
                }
                DrawTable(win);
                SetPageProps(page);
				return win;
            }
            FreeGadgets(wd->wd_Gadgets);
        }
		FreePooled(pool, wd, sizeof(struct winData));
    }
    return NULL;
}

struct Window *OpenProjWindow(struct Page *page, ...)
{
	va_list ap;
	struct TagItem *tags;
	struct Window *rwin;

	va_startlinear(ap, page);
	tags = va_getlinearva(ap, struct TagItem *);
	rwin =OpenProjWindowA(page, tags);
	va_end(ap);
	
	return rwin;
}
#else
struct Window *
OpenProjWindow(struct Page *page,ULONG tag1,...)
{
    struct Window *win;
    struct Mappe *mp;
    struct IBox box;
    BYTE   new = FALSE;

    if (!page)
    {
        mp = NewProject();
        page = NewPage(mp);
        new = TRUE;
    }
    else
        mp = page->pg_Mappe;
    if (!page)
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_CREATE_PROJECT_ERR));
		return NULL;
    }
    box.Left = GetTagData(WA_Left,mp->mp_WindowBox.Left,(struct TagItem *)&tag1);
    box.Top = GetTagData(WA_Top,mp->mp_WindowBox.Top,(struct TagItem *)&tag1);
    box.Width = GetTagData(WA_Width,mp->mp_WindowBox.Width,(struct TagItem *)&tag1);
    box.Height = GetTagData(WA_Height,mp->mp_WindowBox.Height,(struct TagItem *)&tag1);

    NormalizeWindowBox(&box);

	if ((wd = AllocPooled(pool, sizeof(struct winData))) != 0) {
        wd->wd_Type = WDT_PROJECT;
        wd->wd_Data = page;
        MyNewList(&wd->wd_Objs);
        wd->wd_BorGads = MakeBorderScroller(wd);
        wd->wd_Server = handleProjIDCMP;

		if (MakeProjectGadgets(wd, box.Width, box.Height))
        {
			if ((win = OpenWindowTags(NULL, WA_Flags,        WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE |WFLG_SIZEGADGET |WFLG_SIZEBBOTTOM | WFLG_SIZEBRIGHT | WFLG_REPORTMOUSE,
                                           WA_Title,        GetPageTitle(page),
                                           WA_Left,         box.Left,
                                           WA_Top,          box.Top,
                                           WA_Width,        box.Width,
                                           WA_Height,       box.Height,
                                           prefs.pr_Flags & PRF_SIMPLEPROJS ? WA_SimpleRefresh : WA_NoCareRefresh,TRUE,
                                           WA_NewLookMenus, TRUE,
                                           WA_MenuHelp,     TRUE,
                                           WA_PubScreen,    scr,
                                           WA_Gadgets,      wd->wd_Gadgets,
                                           WA_MinWidth,     wd_PosWidth+70+boxwidth+TLn("Seite 999")+downImg->Width,
                                           WA_MinHeight,    scr->WBorTop+4*fontheight+prefs.pr_Icon->pi_Height+34+leftImg->Height,
                                           WA_MaxWidth,     -1,
                                           WA_MaxHeight,    -1,
                                           TAG_END)) != 0)
            {
                win->UserPort = iport;  win->UserData = (APTR)wd;
                ModifyIDCMP(win, APPIDCMP | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_NEWSIZE | IDCMP_IDCMPUPDATE | IDCMP_SIZEVERIFY | IDCMP_ACTIVEWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_MENUVERIFY | (prefs.pr_Flags & PRF_SIMPLEPROJS ? IDCMP_REFRESHWINDOW : 0));
                AddGList(win,wd->wd_BorGads,-1,-1,NULL);
                SetMenuStrip(win,mp->mp_Prefs.pr_Menu);
                DrawBars(win);
                RefreshGadgets(win->FirstGadget,win,NULL);
                RefreshGList(wd->wd_Gadgets,win,NULL,CountGadToolGads(win));
                GT_RefreshWindow(win,NULL);
                page->pg_Window = mp->mp_Window = win;

                if (!new)
                {
                    SetMediumSize(mp);
					SetZoom(page, page->pg_Zoom, TRUE, FALSE);
                }
                DrawTable(win);
                SetPageProps(page);

				return win;
            }
            FreeGadgets(wd->wd_Gadgets);
        }
		FreePooled(pool, wd, sizeof(struct winData));
    }
    return NULL;
}
#endif

void
MakeFewFuncs(void)
{
    struct Node *ln,*f;

	while ((ln = MyRemHead(&fewfuncs)) != 0)
		FreePooled(pool, ln, sizeof(struct Node));

    if (fewftype == FT_RECENT)
    {
        foreach(&usedfuncs,f)
        {
            if ((ln = AllocPooled(pool, sizeof(struct Node))) != 0) {
                ln->ln_Name = ((struct Node *)f->ln_Name)->ln_Name;
                MyAddTail(&fewfuncs, ln);
            }
        }
    }
    else
    {
        BOOL in;

        foreach(&funcs,f)
        {
            in = FALSE;
            if (fewftype == FT_ALL || fewftype == f->ln_Type)
                in = TRUE;
            if (fewftype == FT_MATH && f->ln_Type >= FTM_ALGEBRA && f->ln_Type <= FTM_TRIGO)
                in = TRUE;
            if (in && (ln = AllocPooled(pool,sizeof(struct Node))))
            {
                ln->ln_Name = f->ln_Name;
                MyAddTail(&fewfuncs, ln);
            }
        }
    }
    sortList(&fewfuncs);
}


void
RemInputHandler(void)
{
    if (ghelpio)
    {
        ghelpio->io_Data = ghelpintr;
        ghelpio->io_Command = IND_REMHANDLER;
        DoIO((struct IORequest *)ghelpio);

        CloseDevice((struct IORequest *)ghelpio);
#ifdef __amigaos4__
		FreeSysObject(ASOT_IOREQUEST, (struct IORequest *)ghelpio);
#else
        DeleteExtIO((struct IORequest *)ghelpio);
#endif
    }
    FreePooled(pool,ghelpintr,sizeof(struct Interrupt));
}


void
AddInputHandler(void)
{
    if ((ghelpintr = AllocPooled(pool, sizeof(struct Interrupt))) != 0) {
        ghelpintr->is_Code = (void(*)())ghelpFunc;
        ghelpintr->is_Node.ln_Name = "ignition-GadgetHelp";
        ghelpintr->is_Node.ln_Pri = 100;
#ifdef __amigaos4__
		if ((ghelpio = (struct IOStdReq *)AllocSysObjectTags(ASOT_IOREQUEST, ASOIOR_ReplyPort, iport, ASOIOR_Size, sizeof(struct IOStdReq), TAG_END)) != 0)
#else
        if ((ghelpio = (struct IOStdReq *)CreateExtIO(iport, sizeof(struct IOStdReq))) != 0)
#endif
		{
            if (!OpenDevice("input.device", 0, (struct IORequest *)ghelpio, 0)) {
                ghelpio->io_Data = ghelpintr;
                ghelpio->io_Command = IND_ADDHANDLER;
                DoIO((struct IORequest *)ghelpio);
            }
        }
    }
}


void
RefreshSession(void)
{
    struct Mappe *mp;

	if (IsListEmpty((struct List *)&gProjects))
        return;

	SetBusy(TRUE, BT_APPLICATION);
    UpdateMenuSpecials(&prefs);

	foreach (&gProjects, mp)
    {
        if (mp->mp_Prefs.pr_Menu != prefs.pr_Menu)
            UpdateMenuSpecials(&mp->mp_Prefs);
    }
	SetBusy(FALSE, BT_APPLICATION);
}


void
FreeSession(struct Session *s)
{
    FreeString(s->s_Node.ln_Name);
    FreeString(s->s_Path);

    FreePooled(pool,s,sizeof(struct Session));
}


/** Sucht eine Mappe in der Sitzungs-Datei.
 *
 *  @param mp die zu suchende Mappe
 *  @return s die Sitzung, oder NULL, wenn die Mappe nicht gefunden wurde
 */

struct Session *
FindInSession(struct Mappe *mp)
{
    STRPTR t = mp->mp_Node.ln_Name,p = mp->mp_Path;
    struct Session *s;

    for(s = (struct Session *)sessions.mlh_Head;s->s_Node.ln_Succ && (stricmp(s->s_Node.ln_Name,t) || zstricmp(s->s_Path,p));s = (APTR)s->s_Node.ln_Succ);
    if (s->s_Node.ln_Succ)
        return(s);

    return(NULL);
}


/** Sucht einen Dateinamen in der Sitzungs-Datei. Der Name
 *  der Datei beinhaltet den Pfadnamen.
 *
 *  @param filename der Name der zu suchenden Datei
 *  @return s die Sitzung, oder NULL, wenn die Datei nicht gefunden wurde
 */

struct Session *
FindSession(STRPTR filename)
{
    struct Session *s;

    for(s = (struct Session *)sessions.mlh_Head;s->s_Node.ln_Succ && stricmp(s->s_Filename,filename);s = (APTR)s->s_Node.ln_Succ);
    if (s->s_Node.ln_Succ)
        return s;

    return NULL;
}


bool gSessionChanged;

void
AddToSession(struct Mappe *mp)
{
    struct Session *s;
    char   filename[512];

    if (!mp)
        return;

	filename[0] = '\0';
	if (mp->mp_Path)
		strcpy(filename,mp->mp_Path);

    AddPart(filename,mp->mp_Node.ln_Name,512);

    if ((s = FindSession(filename)) != 0) {
        if ((struct Session *)sessions.mlh_Head == s)
            return;

        MyRemove(s);
    }
    else
    {
        if (!(s = AllocPooled(pool,sizeof(struct Session))))
            return;

        s->s_Node.ln_Name = AllocString(mp->mp_Node.ln_Name);
        s->s_Path = AllocString(mp->mp_Path);
        s->s_Filename = AllocString(filename);
    }
    gSessionChanged = true;
    MyAddHead(&sessions, s);

    RefreshSession();
}


void
SaveSession(STRPTR name)
{
    BPTR file;

    /*if (IsListEmpty((struct List *)&sessions))
    {
        DeleteFile(name);
        return;
    }*/
	if ((file = Open(name, MODE_NEWFILE)) != 0) {
        struct Session *s;
        struct Node *ln;
        long count = 0;

        /** write recent files **/

        FPuts(file,"[recent files]\n");

        foreach(&sessions,s)
        {
            FPuts(file,s->s_Filename);
            FPutC(file,'\n');

            if (++count > MAX_SESSIONS)
                break;
        }

        /** write recent functions **/

        FPutC(file,'\n');
        FPuts(file,"[recent functions]\n");
        count = 0;

		foreach(&usedfuncs, ln) {
			if (ln->ln_Name != (APTR)-1L) {
				FPuts(file, (char *)&((struct Function *)ln->ln_Name)->f_ID);
				FPutC(file, '\n');

				if (++count > MAX_RECENTFUNCS)
					break;
			}
		}

        /** write font sizes **/

        FPutC(file,'\n');
        FPuts(file,"[font sizes]\n");
        count = 0;

        foreach(&sizes,ln)
        {
            char t[16];

            sprintf(t,"%ld\n",(long)(atof(ln->ln_Name)*100+0.5));
            FPuts(file,t);

            if (++count > MAX_FONTSIZES)
                break;
        }

        /** write window positions **/

        //if (prefs.pr_Flags & PRF_SAVEWINPOS)
        {
            /* MERKER: to be implemented */
        }
        Close(file);
    }
}


void
LoadSession(STRPTR name)
{
    struct Session *s;
    BPTR   file;

    while ((s = (APTR)MyRemHead(&sessions)) != 0)
        FreeSession(s);

	if ((file = Open(name, MODE_OLDFILE)) != 0) {
		char t[512], mode = 0;
        BPTR lock;
        int  i;

		while(FGets(file, t, 512))
        {
            if (t[0] == '\n')
                continue;

            if (t[0] == '[')
            {
                if (!stricmp(t,"[recent files]\n"))
                    mode = 1;
                else if (!stricmp(t,"[recent functions]\n"))
                    mode = 2;
                else if (!stricmp(t,"[window positions]\n"))
                    mode = 3;
                else if (!stricmp(t,"[font sizes]\n"))
                    mode = 4;
                else
                    mode = 0;
                continue;
            }
            if (mode == 0)
                continue;

            if (t[i = strlen(t)-1] == '\n')
                t[i] = 0;

            switch (mode)
            {
                case 1:  // recent files
                    if (FindSession(t))                 // Datei gibt es doppelt (Fehler im Session-File)
                        break;

#ifdef __amigaos4__
					SetProcWindow((APTR)-1);												//Requester für Devicemounten aus.
#endif
                    if (!(lock = Lock(t,ACCESS_READ)))  // Datei existiert nicht
                    {
                        gSessionChanged = true;
#ifdef __amigaos4__
 						SetProcWindow(NULL);												//Requester für Devicemounten ein
#endif
                       break;
                    }
                    UnLock(lock);
#ifdef __amigaos4__
					SetProcWindow(NULL);													//Requester für Devicemounten ein
#endif

					if ((s = AllocPooled(pool, sizeof(struct Session))) != 0) {
                        STRPTR u;

                        s->s_Filename = AllocString(t);
                        s->s_Node.ln_Name = AllocString(FilePart(t));
                        if ((u = PathPart(t)) != 0) {
                            *u = 0;
                            s->s_Path = AllocString(t);
                        }
                        MyAddTail(&sessions, s);
                    }
                    break;
                case 2:  // recent functions
                {
                    struct Function *f;
                    struct Node *ln;

                    if ((f = FindFunctionWithLanguage((APTR)flangs.mlh_TailPred,t)) && (ln = AllocPooled(pool,sizeof(struct Node))))
                    {
                        ln->ln_Name = (STRPTR)f;
                        MyAddTail(&usedfuncs, ln);
                    }
                    break;
                }
                case 3:  // window positions
                    break;
                case 4:  // font sizes
                {
                    double size = atol(t)/100.0;
                    struct Node *ln;

                    if ((ln = AllocPooled(pool,sizeof(struct Node))) != 0) {
                        char t[20];

                        strcpy(t,ita(size,-3,ITA_NONE));
                        strcat(t," pt");

                        ln->ln_Name = AllocString(t);
                        MyAddTail(&sizes, ln);
                    }
                    break;
                }
            }
        }
        Close(file);
    }
    RefreshSession();
}


void
FreeAppIcon(void)
{
    if (wbport)
    {
        struct Message *msg;

        if (appdo)
        {
            if (appicon)
            {
                RemoveAppIcon(appicon);
                appicon = NULL;
            }
            FreeDiskObject(appdo);
            appdo = NULL;
        }

        while ((msg = GetMsg(wbport)) != 0)
            ReplyMsg(msg);
#ifdef __amigaos4__
		FreeSysObject(ASOT_PORT, wbport);
#else
        DeleteMsgPort(wbport);
#endif
        sigwait = (1L << SIGBREAKB_CTRL_C) | (1L << iport->mp_SigBit) | (1L << rxport->mp_SigBit);
        wbport = NULL;
    }
}


void
InitAppIcon(void)
{
	BPTR olddir, dir;

    if (!(prefs.pr_Flags & PRF_APPICON) || wbport)
        return;

	if ((dir = Lock(iconpath, SHARED_LOCK)) == (BPTR)NULL)
		return;

#ifdef __amigaos4__
	olddir = SetCurrentDir(dir);
	if ((wbport = AllocSysObjectTags(ASOT_PORT, TAG_END)) != 0) 
#else
	olddir = CurrentDir(dir);
	if ((wbport = CreateMsgPort()) != 0) 
#endif
	{
		sigwait = (1L << SIGBREAKB_CTRL_C) | (1L << iport->mp_SigBit) | (1L << rxport->mp_SigBit) | (1L << wbport->mp_SigBit);
		if ((appdo = GetDiskObject("def_app")) != 0) {
			appdo->do_Type = 0;
			appicon = AddAppIconA(0, 0, "ignition", wbport, (BPTR)NULL, appdo, (const struct TagItem *)NULL);
		}
	}
#ifdef __amigaos4__
	SetCurrentDir(olddir);
#else
	CurrentDir(olddir);
#endif
	UnLock(dir);
}


void
CloseCheck(void)
{
    struct PrefsModule *pm;
    char   modified[420];
	long   rc, i = 0;
    BOOL   nowindow = FALSE;

    if (!GetAppWindow(WDT_ANY))
        nowindow = TRUE;

	modified[0] = '\0';

	foreach (&prefs.pr_Modules, pm) {
		if (pm->pm_Flags & PMF_MODIFIED) {
			sprintf(&modified[i], "\n   · %s", pm->pm_Node.in_Name);
			i += strlen(modified + i);
		}
    }
	if (modified[0]) {
		rc = DoRequest(GetString(&gLocaleInfo, MSG_UNSAVED_PREFERENCES_REQ),
				nowindow ? GetString(&gLocaleInfo, MSG_QUIT_SAVE_AND_QUIT_REQ)
				: GetString(&gLocaleInfo, MSG_QUIT_SAVE_AND_QUIT_CANCEL_REQ), modified);

        if (nowindow && !rc || !nowindow && rc == 2)
            processIntCmd("PREFS SAVE");

        if (!nowindow && !rc)
            ende = FALSE;
    }

	if (ende && asp_printtask != NULL) {
        ErrorRequest(GetString(&gLocaleInfo, MSG_PRINT_IN_PROGRESS_REQ));
        ende = FALSE;
    }
}


void
EmptyMsgPort(struct MsgPort *port)
{
    struct Message *msg;

    while ((msg = GetMsg(port)) != 0)
        ReplyMsg(msg);
}


void
CloseApp(void)
{
    struct Node *n;

    gLockStop = TRUE;   // do not apply any lock-functions

    if (rxout)
        Close(rxout);

	if (gAmigaGuide)
    {
		CloseAmigaGuide(gAmigaGuide);
		gAmigaGuide = NULL;
    }

    FreeAppIcon();
    if ((prefs.pr_Flags & PRF_USESESSION) && gSessionChanged)
		SaveSession(CONFIG_PATH "/ignition.session");

    while ((win = GetAppWindow(WDT_ANY)) != 0)
        CloseAppWindow(win, TRUE);

	while (!IsListEmpty((struct List *)&gProjects))
		DisposeProject((struct Mappe *)gProjects.mlh_Head);

    while ((n = MyRemHead(&prefs.pr_AppCmds)) != 0)
        FreeAppCmd((struct AppCmd *)n);

    while ((n = MyRemHead(&images)) != 0)
        FreeImageObj((struct ImageObj *)n);

    FreePointers();
    FreeAppMenu(&prefs);
    FreeAslRequest(fileReq);
   	FreeAslRequest(scrReq);
    FreeAslRequest(fontReq);

    if (scr)
    {
        CloseAppScreen(scr);
        scr = NULL;
    }
    if (prefs.pr_Disp && prefs.pr_Disp->pd_AntiFont)
        CloseFont(prefs.pr_Disp->pd_AntiFont);

    if (iport)
    {
        EmptyMsgPort(iport);
#ifdef __amigaos4__
		FreeSysObject(ASOT_PORT, iport);
#else
        DeleteMsgPort(iport);
#endif
    }
    FreeFonts();
    CloseRexx();
    if (rxport)
    {
        EmptyMsgPort(rxport);
#ifdef __amigaos4__
		FreeSysObject(ASOT_PORT, rxport);
#else
        DeletePort(rxport);
#endif
    }
    FreeGraphics();
    closeIO();

	CloseCatalog(gLocaleInfo.li_Catalog);
    CloseLocale(loc);


#ifdef __amigaos4__
	DropInterface((struct Interface *)ICyberGfx);
	CloseLibrary((struct Library *)CyberGfxBase);
	//DropInterface( (struct Interface *)IGradientSlider );
	//CloseLibrary(GradientSliderBase);

	closeRAresources(); // RA
#else
    CloseLibrary(ScrollerBase);
    CloseLibrary(TextEditBase);
    CloseLibrary(ColorWheelBase);
    CloseLibrary(GradientSliderBase);
    CloseLibrary(CyberGfxBase);
    CloseLibrary(AmigaGuideBase);
#endif

    while ((n = MyRemHead(&sizes)) != 0) {
        FreeString(n->ln_Name);
        FreePooled(pool, n, sizeof(struct Node));
    }
}


void
InitAsl(void)
{
	fileReq = AllocAslRequestTags(ASL_FileRequest, 		ASLFR_InitialHeight,   scr->Height-5*barheight,
                                                   		ASLFR_InitialLeftEdge, 50,
                                                   		ASLFR_InitialTopEdge,  5*barheight/2,
                                                   		ASLFR_InitialWidth,    scr->Width/2-25,
                                                   		ASLFR_RejectIcons,    TRUE,
                                                   		TAG_END);
	scrReq = AllocAslRequestTags(ASL_ScreenModeRequest, ASLSM_InitialDisplayID,     prefs.pr_Screen->ps_ModeID,
                                                        ASLSM_InitialDisplayDepth,  prefs.pr_Screen->ps_Depth,
                                                        ASLSM_InitialDisplayWidth,  prefs.pr_Screen->ps_Width,
                                                        ASLSM_InitialDisplayHeight, prefs.pr_Screen->ps_Height,
                                                        ASLSM_DoDepth,              TRUE,
                                                        ASLSM_DoHeight,             TRUE,
                                                        ASLSM_DoWidth,              TRUE,
                                                        ASLSM_DoOverscanType,       TRUE,
                                                        ASLSM_MinDepth,             3,
                                                        ASLSM_InitialHeight,        scr->Height-5*barheight,
                                                        ASLSM_InitialLeftEdge, 50,
                                                        ASLSM_InitialTopEdge,  5*barheight/2,
                                                        ASLSM_InitialWidth,    scr->Width/2-25,
                                                        TAG_END);
	fontReq = AllocAslRequestTags(ASL_FontRequest, 		ASLFO_InitialHeight, scr->Height-5*barheight,
                                                   		ASLFO_InitialLeftEdge, 50,
                                                   		ASLFO_InitialTopEdge,  5*barheight/2,
                                                   		ASLFO_InitialWidth,    scr->Width/2-25,
                                                   		TAG_END);
}


const ULONG sizepts[] = {8, 9, 10, 12, 14, 16, 18, 24, 36, 72, 0};


void
InitApp(void)
{
    struct Screen *iscr = NULL;
    struct Window *win = NULL;
    struct ProgressBar *pb = NULL;
    struct DiskObject *dio;
    BPTR   olddir;
    long   i;

    iconpath = AllocString("icons/");  projpath = AllocString("sheets/");  graphicpath = AllocString("graphic/");

	MyNewList(&gProjects);  MyNewList(&locks);
    MyNewList(&toolobjs);  MyNewList(&usedfuncs);
    MyNewList(&fonts);  MyNewList(&errors);
    MyNewList(&intcmds);  MyNewList(&fontpaths);
    MyNewList(&outputs);  MyNewList(&sessions);
    MyNewList(&families);  MyNewList(&colors);  MyNewList(&infos);
    MyNewList(&clips);  MyNewList(&images);  MyNewList(&sizes);

	if (sm && sm->sm_NumArgs) {
		i = sm->sm_NumArgs - 1;
#ifdef __amigaos4__
        olddir = SetCurrentDir(sm->sm_ArgList[i].wa_Lock);
#else
        olddir = CurrentDir(sm->sm_ArgList[i].wa_Lock);
#endif
		if ((dio = GetDiskObject(sm->sm_ArgList[i].wa_Name)) != 0) {
			STRPTR value;

			if (dio->do_ToolTypes) {
				// add fonts
				for (i = 0; dio->do_ToolTypes[i]; i++)
					if (!strnicmp(dio->do_ToolTypes[i], "FONTS=", 6))
                        AddFontPath(dio->do_ToolTypes[i]+6);
            }

			if ((prefname = FindToolType(dio->do_ToolTypes, "WITH")) != 0)
                prefname = AllocString(prefname);
			if (FindToolType(dio->do_ToolTypes, "NOABOUT"))
                noabout = TRUE;
			if (FindToolType(dio->do_ToolTypes, "BEGINNER"))
				gIsBeginner = TRUE;
			if ((value = FindToolType(dio->do_ToolTypes, "EDITOR")) != 0)
				gEditor = AllocString(value);
			if ((value = FindToolType(dio->do_ToolTypes, "Debug")) != 0)
				debug = TRUE;

            FreeDiskObject(dio);
        }
#ifdef __amigaos4__
        SetCurrentDir(olddir);
#else
        CurrentDir(olddir);
#endif
    }
    InitSemaphore(&gWindowSemaphore);
    InitSemaphore(&fontSemaphore);
	
	if (gEditor == NULL) {
		// TODO: find good editor default values!
	}

#ifdef __amigaos4__
//	gLocaleInfo.li_LocaleBase = (struct Library *)ILocale;
	//gLocaleInfo.li_ILocale = (struct Library *)ILocale;
	gLocaleInfo.li_ILocale = ILocale;
#else
	gLocaleInfo.li_LocaleBase = LocaleBase;
#endif
    loc = OpenLocale(NULL);
	gLocaleInfo.li_Catalog = OpenCatalog(loc, "ignition.catalog", OC_BuiltInLanguage, "english", TAG_END);

    if (!noabout && (iscr = scr = LockPubScreen(NULL)))
        InitAppScreen(iscr);

    /** create logo-images from PictureImage-Class **/
	if ((pincImage = (struct Image *)NewObject(pictureiclass, NULL,
	                                           PIA_FromImage,	&pincOriginalImage,
	                                           PIA_WithColors,	standardPalette + 1,
	                                           PDTA_Screen,	iscr,
	                                          TAG_END)) != NULL)
	{
		AddImageObj(NULL,pincImage);
	}

	if ((logoImage = (struct Image *)NewObject(pictureiclass,NULL,PIA_FromImage, &logoOriginalImage,
	                                           PIA_WithColors,standardPalette+1,
	                                           PDTA_Screen,   iscr,
	                                          TAG_END)) != NULL)
	{
		AddImageObj(NULL,logoImage);
	}

    if (iscr)
    {
		prefs.pr_WinPos[WDT_INFO].Left = -1;  // Fenster soll zentriert werden
		prefs.pr_WinPos[WDT_INFO].Top = -1;
		prefs.pr_WinPos[WDT_INFO].Width = -1;
		prefs.pr_WinPos[WDT_INFO].Height = -1;
//openRAresources(); // RA
		win = OpenAppWindow(WDT_INFO,WA_Data,TRUE,TAG_END);
		pb = NewProgressBar(win, 8, gHeight - fontheight - 9, gWidth - 16, fontheight + 4);
//CreateRAInfo(iscr->Font, GetString(&gLocaleInfo,MSG_OPEN_CLASSES_PROGRESS)); // UpdateProgress ABOUT
//win=(struct Window *)IDoMethod(winobj[WDT_INFO], WM_OPEN, NULL);
		UpdateProgressBar(pb,GetString(&gLocaleInfo, MSG_OPEN_CLASSES_PROGRESS),(float)0.04);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_OPEN_CLASSES_PROGRESS), 30);
//Delay(200);
    }

    SETHOOK(passwordEditHook, PasswordEditHook);
    fewftype = 1;
    CurrentTime(&lastsecs,(ULONG *)&i);

#ifdef __amigaos4__
	unicode_map = (uint32 *)ObtainCharsetInfo(DFCS_NUMBER, loc->loc_CodeSet, DFCS_MAPTABLE); 
	CyberGfxBase = (struct Library *)OpenLibrary("cybergraphics.library",43);
	ICyberGfx = (struct CyberGfxIFace *)GetInterface((struct Library *)CyberGfxBase,"main",1,NULL);
    UpdateProgressBar(pb,(STRPTR)~0L,(float)0.08);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_OPEN_CLASSES_PROGRESS), 25);
    openRAresources(); // RA
    UpdateProgressBar(pb,(STRPTR)~0L,(float)0.10);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_OPEN_CLASSES_PROGRESS), 40);
//Delay(200);
#else
    CyberGfxBase = OpenLibrary("cybergraphics.library",41);

    if (!(ColorWheelBase = OpenLibrary("gadgets/colorwheel.gadget",39)))
        ErrorOpenLibrary("ColorWheel.gadget",NULL);

    UpdateProgressBar(pb,(STRPTR)~0L,(float)0.08);

    if (!(ScrollerBase = IgnOpenClass("gadgets","pScroller.gadget",0)))
        ErrorOpenLibrary("pScroller.gadget",NULL);

    UpdateProgressBar(pb,(STRPTR)~0L,(float)0.10);

    if (!(TextEditBase = IgnOpenClass("gadgets","pTextEdit.gadget",0)))
        ErrorOpenLibrary("pTextEdit.gadget",NULL);
#endif


	InitCalc();
	InitSearch();
	InitGraphics();
	initFuncs();
	initIO();
	InitPrinter();
	InitGadgetLabels();
	InitPrefsGadgetsLabels();
	MakeFewFuncs();

	MakeStrings(&zooms, "50%", "100%", "125%", "150%", "200%", "400%", "800%",
		"1000%", "-", GetString(&gLocaleInfo, MSG_ZOOM_OVERVIEW_GAD), NULL);
	MakeLocaleStrings(&events, MSG_OPEN_DOCUMENT_EVENT, MSG_CLOSE_DOCUMENT_EVENT, MSG_CELL_SELECTION_EVENT, MSG_INPUT_DONE_EVENT,
		MSG_TIME_EVENT, MSG_RECALC_EVENT, MSG_LEFT_MOUSE_BUTTON_EVENT, MSG_RIGHT_MOUSE_BUTTON_EVENT, TAG_END);

#ifdef __amigaos4__
	//Geändert, da unteres Konstrukt Reaper-Meldung generiert
	Strlcpy(portname, "IGNITION.X", 13);
	Forbid();
	for (i = 1; i < 9; i++) {
		((char *)portname)[9]  = i + 48;
        if (!FindPort(portname))
            break;
    }
    Permit();
    pubname = portname;
    Strlcpy(IgnTitle, SCREEN_TITLE, 100);
    Strlcat(IgnTitle, "   Rexx-Port: ", 100);
    Strlcat(IgnTitle, portname, 100);
#else
	for (i = 1; i < 9; i++) {
		*(pubname+9) = i + 48;
        if (!FindPort(pubname))
            break;
    }
#endif

	UpdateProgressBar(pb, GetString(&gLocaleInfo, MSG_SEARCH_FONTS_PROGRESS), (float)0.15);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_SEARCH_FONTS_PROGRESS), 50);
//Delay(200);
    SearchFonts();
    if (!(stdfamily = MyFindName(&families, "CG Triumvirate")) && !IsListEmpty((struct List *)&families))
        stdfamily = (struct Node *)families.mlh_Head;
#ifdef __amigaos4__
    stdpointheight = 10 << 16;
#else
    stdpointheight = 18 << 16;
#endif

	UpdateProgressBar(pb, GetString(&gLocaleInfo, MSG_OPEN_MOUSE_POINTER_PROGRESS), (float)0.17);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_OPEN_MOUSE_POINTER_PROGRESS), 60);
//Delay(200);
    InitPointers();

	UpdateProgressBar(pb, GetString(&gLocaleInfo, MSG_LOAD_PREFERENCES_PROGRESS), (float)0.18);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_LOAD_PREFERENCES_PROGRESS), 65);
//Delay(200);
    scr = NULL;
	InitAppPrefs(prefname ? prefname : (STRPTR) CONFIG_PATH "/ignition.prefs");
    scr = iscr;
    FreeString(prefname);  prefname = NULL;

    if (prefs.pr_Flags & PRF_USEDBUFFER)
        InitDoubleBuffer();

	UpdateProgressBar(pb, GetString(&gLocaleInfo, MSG_LOAD_SESSION_PROGRESS), (float)0.22);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_LOAD_SESSION_PROGRESS), 70);
//Delay(200);
	LoadSession(CONFIG_PATH "/ignition.session");
    initRexx();
	rxout = Open(((struct Node *)outputs.mlh_Head)->ln_Name, MODE_NEWFILE); /* outputs cannot be empty */

    if (IsListEmpty((struct List *)&sizes)) // saved in the session file
    {
        int  i;

        for(i = 0;sizepts[i];i++)
        {
            struct Node *ln;

            if ((ln = AllocPooled(pool,sizeof(struct Node))) != 0) {
                char t[20];

                sprintf(t,"%ld pt",sizepts[i]);
                ln->ln_Name = AllocString(t);

                MyAddTail(&sizes, ln);
            }
        }
    }

    if (IsListEmpty((struct List *)&families))
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_NO_OUTLINE_FONTS_ERR));
        ende = TRUE;
    }

    if (win)
    {
        struct ImageObj *io;
        long   num = CountNodes(&images)-3, count = 0;   // Logos sind bereits geladen & gerendert

        foreach(&images,io)
        {
            if (io->io_Image->Depth == CUSTOMIMAGEDEPTH)
                DoMethod((APTR)io->io_Image,PIM_LOAD);
            if (count < num)
                count++;
            UpdateProgressBar(pb,GetString(&gLocaleInfo, MSG_LOAD_ICONS_PROGRESS),(float)(0.78*count/num+0.22));
//Printf("count=%ld num=%ld\n",count,num);
//UpdateFuelGauge(win, GetString(&gLocaleInfo,MSG_LOAD_ICONS_PROGRESS), 70+count);
        }
        DisposeProgressBar(pb);

		CloseAppWindow(win, TRUE);
//Delay(200);
//IDoMethod(winobj[WDT_INFO], WM_CLOSE, NULL);
        FreeVisualInfo(vi);
		UnlockPubScreen(NULL, iscr);
    }
}


void
handleTimer(void)
{
    struct Mappe *mp;
    struct Page *page;
    ULONG  secs,mics;

    if (ende)
        return;

	CurrentTime(&secs, &mics);

	if (prefs.pr_File->pf_AutoSave && lastsecs + prefs.pr_File->pf_AutoSaveIntervall < secs)
    {
		foreach (&gProjects, mp)
        {
            if (mp->mp_Modified)
            {
				if (prefs.pr_File->pf_AutoSave == PFAS_ON
					|| (prefs.pr_File->pf_AutoSave == PFAS_REMEMBER
						&& DoRequest(GetString(&gLocaleInfo, MSG_AUTOSAVE_REQ),GetString(&gLocaleInfo, MSG_SAVE_BACK_REQ),mp->mp_Node.ln_Name,prefs.pr_File->pf_AutoSaveIntervall / 60)))
                {
                    page = rxpage;  rxpage = (struct Page *)mp->mp_Pages.mlh_Head;
                    processIntCmd("SAVE");
                    rxpage = page;
                }
            }
        }
        lastsecs = secs;
    }
 
    /* update time references */
	RecalcReferencesList(&gTimedRefs);

    /* call scripts */
	foreach (&gProjects, mp)
    {
		// ToDo: is that really needed anymore??
		/*if (mp->mp_Flags & MPF_CUTIME)
        {
            if (!mp->mp_Window || mp->mp_actPage->pg_Action == PGA_NONE)
                RecalcTableFields((struct Page *)mp->mp_Pages.mlh_Head);
		}*/
        if (mp->mp_Flags & MPF_SCRIPTS)
        {
            if ((mp->mp_Events[EVT_TIME].ev_Flags & EVF_ACTIVE) && mp->mp_Events[EVT_TIME].ev_Intervall && !(secs % mp->mp_Events[EVT_TIME].ev_Intervall))
				handleEvent((struct Page *)mp->mp_Pages.mlh_Head, EVT_TIME, 0, 0);
        }
    }
}


void
handleGadgetHelp(void)
{
    struct Window *swin;
    struct winData *wd;
    APTR   layer;
    struct Gadget *gad = NULL,*sgad;
    struct tableField *tf;
    long   x,y,i;

    if ((layer = WhichLayer(&scr->LayerInfo, x = win->LeftEdge+imsg.MouseX, y = win->TopEdge+imsg.MouseY)) != 0) {
		for (swin = scr->FirstWindow;swin;swin = swin->NextWindow)
        {
            if (layer == swin->WLayer && swin->UserPort == iport)
            {
                x -= swin->LeftEdge;
                y -= swin->TopEdge;

                if ((wd = (struct winData *)swin->UserData)->wd_Type == WDT_PROJECT) /*&& GetLocalPrefs(wd->wd_Data)->pr_Disp->pd_HelpBar)*/
                {
					struct Page *page = wd->wd_Data;
					char buffer[64];
					STRPTR helpText = NULL;

					if (ghelpcnt == GHELP_OBJECT
						&& x > wd->wd_TabX && x < wd->wd_TabX+wd->wd_TabW && y > wd->wd_TabY && y < wd->wd_TabY+wd->wd_TabH)
                    {
                        struct gObject *go;

                        foreach(&page->pg_gObjects,go)
                        {
                            if (CheckGObject(page, go, x-wd->wd_TabX+page->pg_TabX, y-wd->wd_TabY+page->pg_TabY))
                            {
								helpText = go->go_Help;
                                break;
                            }
                        }
						if (!helpText && page->pg_Mappe->mp_Flags & MPF_NOTES
							&& (tf = GetTableFieldCoord(wd->wd_Data, x - wd->wd_TabX, y - wd->wd_TabY)) != NULL)
							helpText = tf->tf_Note;

						ShowPopUpText(helpText, -1, -1);
                        return;
                    }
                    if (ghelpcnt != GHELP_GADGET)
                        return;

					if ((i = IsOverProjSpecial(swin, x, y)) != 0) {
						switch (i)
                        {
                            case GID_APEN:
								helpText = GetString(&gLocaleInfo, MSG_CELL_TEXT_COLOR_HELP);
                                break;
                            case GID_BPEN:
								helpText = GetString(&gLocaleInfo, MSG_CELL_BACK_COLOR_HELP);
                                break;
                            case GID_STATUS:
								helpText = buffer;
								sprintf(helpText, GetString(&gLocaleInfo, MSG_STATUS_MODE_HELP), page->pg_Mappe->mp_Flags & MPF_SCRIPTS ? GetString(&gLocaleInfo, MSG_INTERACTIVE_MODE_HELP) : GetString(&gLocaleInfo, MSG_EDIT_MODE_HELP));
								//helpText = "Statuszeile: (i)nteraktiver oder (n)ormaler Modus";
                                break;
                            case GID_POSITION:
								helpText = GetString(&gLocaleInfo, MSG_BLOCK_POSITION_HELP);
                                break;
                        }
                    }
					else for (sgad = swin->FirstGadget; sgad && !gad; sgad = sgad->NextGadget)
                    {
						if (x >= sgad->LeftEdge && x <= sgad->LeftEdge + sgad->Width && y >= sgad->TopEdge && y <= sgad->TopEdge+sgad->Height)
                            gad = sgad;
                    }
					DrawHelpText(swin, gad, helpText);
                }
            }
        }
    }
}


bool
IsOverTabGadget(struct Page *page)
{
    if (page->pg_Gad.DispPos > PGS_FRAME)
    {
        /*if (page->pg_Gad.cp.cp_X < imsg.MouseX && imsg.MouseX < page->pg_Gad.cp.cp_X+page->pg_Gad.cp.cp_W && page->pg_Gad.cp.cp_Y < imsg.MouseY && imsg.MouseY < page->pg_Gad.cp.cp_Y+page->pg_Gad.cp.cp_H)*/
            return true;
    }
    return false;
}


void
HandleApp(void)
{
    static struct IntuiMessage *msg;
    static struct TagItem *tags;
    static ULONG  sigrcvd;

    sigwait = (1L << SIGBREAKB_CTRL_C) | (1L << iport->mp_SigBit) | (1L << rxport->mp_SigBit) | (wbport ? 1L << wbport->mp_SigBit : 0) | rxmask | agsig | notifysig;
    while (!ende)
    {
		if (sTimeRequest != NULL && CheckIO((struct IORequest *)sTimeRequest))
        {
			WaitIO((struct IORequest *)sTimeRequest);                  /* the message may still be in the queue */
			sTimeRequest->tr_time.tv_secs = ++sTimeInSeconds;

			SendIO((struct IORequest *)sTimeRequest);
		}
        sigrcvd = Wait(sigwait);

		// RA
		//* Handle window.class */
		if(sigrcvd & wsigmask)
		{
			//win = imsg.IDCMPWindow;
			if( (win=GetActiveAppWindow()) )
			{
				wd = (struct winData *)win->UserData;
DBUG("wd->wd_Type=%ld   cwd_IDCMP=0x%08lx\n", wd->wd_Type,gCreateWinData[wd->wd_Type-1].cwd_IDCMP);
				if(wd->wd_Server && (wd->wd_Type>0 && wd->wd_Type<=WDT_SETNAME? !gCreateWinData[wd->wd_Type-1].cwd_IDCMP : 0))
				{
					wd->wd_Server(tags);
				}
			}
		}
		/* Handle window.class */
		// RA

		while ((msg = (struct IntuiMessage *)GTD_GetIMsg(iport)) != 0) {
            imsg = *msg;
            win = imsg.IDCMPWindow;  tags = NULL;
            wd = (struct winData *)win->UserData;

			switch (imsg.Class)
            {
                case IDCMP_IDCMPUPDATE:
                    tags = CloneTagItems(imsg.IAddress);
                    break;
                case IDCMP_OBJECTDROP:
                    if ((imsg.IAddress = AllocPooled(pool,sizeof(struct DropMessage))) != 0)
                        CopyMem(msg->IAddress,imsg.IAddress,sizeof(struct DropMessage));
                    break;
                case IDCMP_MENUVERIFY:
                    if (wd->wd_Type == WDT_PROJECT && imsg.Code == MENUHOT)
                    {
                        struct PrefDisp *pd = ((struct Page *)wd->wd_Data)->pg_Mappe->mp_Prefs.pr_Disp;

                        /** Kontext-Menü **/

                        if (imsg.Qualifier & IEQUALIFIER_RBUTTON && prefs.pr_Flags & PRF_CONTEXTMENU && imsg.MouseX >= wd->wd_TabX-pd->pd_AntiWidth && imsg.MouseX <= wd->wd_TabX+wd->wd_TabW && imsg.MouseY >= wd->wd_TabY-pd->pd_AntiHeight && imsg.MouseY <= wd->wd_TabY+wd->wd_TabH)
                            imsg.Code = msg->Code = MENUCANCEL;

                        /*if (((struct Page *)wd->wd_Data)->pg_Mappe->mp_Flags & MPF_SCRIPTS && ((struct Page *)wd->wd_Data)->pg_Mappe->mp_Events[EVT_RBUTTON].ev_Type)
                        {
                            if (imsg.MouseX > wd->wd_TabX && imsg.MouseX < wd->wd_TabX+wd->wd_TabW && imsg.MouseY > wd->wd_TabY && imsg.MouseY < wd->wd_TabY+wd->wd_TabH)
                                msg->Code = MENUCANCEL;
                        }*/
                        if (IsOverTabGadget(wd->wd_Data))
                        {
                            UWORD oldflags = calcflags;

                            calcflags &= ~CF_REQUESTER;
                            SetTabGadget(wd->wd_Data,(STRPTR)~0L,PGS_FRAME);
                            calcflags = oldflags;
                        }
                    }
                    break;
                case IDCMP_SIZEVERIFY:
                    if (wd->wd_Type == WDT_PROJECT)
                    {
                        RemoveGadgets(&win->FirstGadget,FALSE);
                        FreeWinIconObjs(win,wd);
                        RemoveGList(win,wd->wd_Gadgets,CountGadToolGads(win)); //+(long)wd->wd_ExtData[0]);
                    }
                    else
                        RemoveGList(win,wd->wd_Gadgets,-1 /*CountGadToolGads(win)*/);
                    FreeGadgets(wd->wd_Gadgets);
                    wd->wd_Gadgets = NULL;
                    break;
            }

            GTD_ReplyIMsg(msg);

            if (imsg.Class == IDCMP_INTUITICKS)
            {
                ghelpcnt++;
                if (ghelpcnt == GHELP_GADGET || ghelpcnt == GHELP_OBJECT)
                    handleGadgetHelp();
            }
            else if (popwin)
                ClosePopUpText();
            if (imsg.Class == IDCMP_MENUPICK && imsg.Code != MENUNULL)
                HandleMenu();
            else if (imsg.Class == IDCMP_MENUHELP || (imsg.Class == IDCMP_RAWKEY && imsg.Code == RAWKEY_HELP))
            {
				ProcessAppCmd(rxpage, "HELP");
			}
            else if (imsg.Class == IDCMP_REFRESHWINDOW)
            {
                if (wd->wd_Type == WDT_PROJECT)  // TODO: that's not really a good solution
                {
                    GT_BeginRefresh(win);
					GT_EndRefresh(win, TRUE);

                    DrawBars(win);
					RefreshGadgets(win->FirstGadget, win, NULL);
					RefreshGList(wd->wd_Gadgets, win, NULL, CountGadToolGads(win));
					GT_RefreshWindow(win, NULL);
                    DrawTable(win);
                }
                else
                {
                    GT_BeginRefresh(win);
					RefreshAppWindow(win, wd);
					GT_EndRefresh(win, TRUE);
                }
            }
            else
            {
                /*if (wd->wd_Type == WDT_PROJECT && imsg.Class != IDCMP_MENUVERIFY && rxpage != wd->wd_Data)
                    SetMainPage(wd->wd_Data);*/
                if (wd->wd_Server)
                    wd->wd_Server(tags);
                else if (imsg.Class != IDCMP_INTUITICKS)
                {
					if (CountAppWindows() > 1
						|| DoRequest(GetString(&gLocaleInfo, MSG_CLOSE_WINDOW_QUIT_REQ), GetString(&gLocaleInfo, MSG_QUIT_BACK_REQ)))
						CloseAppWindow(win, TRUE);
                }
            }
            FreeTagItems(tags);

            if (imsg.Class == IDCMP_OBJECTDROP)
				FreePooled(pool, imsg.IAddress, sizeof(struct DropMessage));
        }

		/* Handle ARexx messages */
		while ((rxmsg = (struct RexxMsg *)GetMsg(rxport)) != 0) {
			if (rxmsg == (struct RexxMsg *)sTimeRequest)
                handleTimer();
            else
            {
                if (IsRexxMsg(rxmsg))
                    handleRexx(rxmsg);
                ReplyMsg((struct Message *)rxmsg);
            }
        }

		/* Internal ARexx Scripts */
        if (sigrcvd & rxmask)
        {
            struct RexxPort *nrxp;

			for (rxp = (APTR)rexxports.mlh_Head; rxp->rxp_Node.ln_Succ; rxp = nrxp)
            {
                nrxp = (struct RexxPort *)rxp->rxp_Node.ln_Succ;

                if ((rxmsg = (struct RexxMsg *)GetMsg(&rxp->rxp_Port)) != 0) {
                    if (rxmsg != rxp->rxp_Message)
                    {
                        if (IsRexxMsg(rxmsg))
                        {
							swmem((UBYTE *)&rxpage, (UBYTE *)&rxp->rxp_Page, sizeof(APTR));   // Skript-Kontext setzen
                            handleRexx(rxmsg);
							swmem((UBYTE *)&rxpage, (UBYTE *)&rxp->rxp_Page, sizeof(APTR));   // vorherigen Kontext wiederherstellen
                        }
                        ReplyMsg((struct Message *)rxmsg);
                    }
                    else
                        RemoveRexxPort(rxp);
                }
            }
        }

		/* Handle DOS notifications */
        if (sigrcvd & notifysig)
        {
            struct NotifyMessage *nm;
            struct RexxScript *rxs;

            while ((nm = (struct NotifyMessage *)GetMsg(notifyport)) != 0) {
                rxs = (struct RexxScript *)nm->nm_NReq->nr_UserData;
                ReplyMsg((struct Message *)nm);
                NotifyRexxScript(rxs);
            }
        }

		/* Handle Workbench/AppIcon messages */
		if (wbport && sigrcvd & (1L << wbport->mp_SigBit))
        {
            struct AppMessage *am;

			while ((am = (struct AppMessage *)GetMsg(wbport)) != 0) {
                if (am->am_NumArgs > 0)  /* icon drop */
					wbstart((struct List *)&files, am->am_ArgList, 0, am->am_NumArgs);
                ScreenToFront(scr);
                ReplyMsg((struct Message *)am);
            }
            if (!IsListEmpty((struct List *)&files))
            {
				SetBusy(true, BT_APPLICATION);
                LoadFiles((struct List *)&files);
				SetBusy(false, BT_APPLICATION);
            }
        }

		/* Handle messages from AmigaGuide */
		if ((sigrcvd & agsig) != 0 && AmigaGuideBase)
        {
            struct AmigaGuideMsg *agm;

			while ((agm = GetAmigaGuideMsg(gAmigaGuide)) != 0) {
                /* error handling to be implemented */
                ReplyAmigaGuideMsg(agm);
            }
        }

		/* Terminate programme? */
        if ((sigrcvd & (1L << SIGBREAKB_CTRL_C)) || !GetAppWindow(WDT_ANY))
        {
			// The PubScreen sends us a CTRL-C signal when it is closed - but that
			// also happens when the screen resolution changes, so ChangeAppScreen()
			// sets this flag so that we can ignore the signal
			if (gScreenHasChanged)
				gScreenHasChanged = false;
			else
				ende = true;
        }

        if (ende)
            CloseCheck();
    }
}


void
InitHelp(struct Screen *scr)
{
#ifndef __amigaos4__
	static struct NewAmigaGuide nag;
#endif
		// must stay valid until the guide is created...

#ifndef __amigaos4__
	AmigaGuideBase = OpenLibrary("amigaguide.library", 39L);
#endif
	if (AmigaGuideBase == NULL)
		return;

	memset(&nag, 0, sizeof(struct NewAmigaGuide));
    nag.nag_Screen = scr;
    // nag.nag_PubScreen = pubname;
#ifdef __amigaos4__
    nag.nag_Name = "help/ignition.guide";
#else
    nag.nag_Name = "ignition.guide";
#endif
    nag.nag_BaseName = "ignition";
    nag.nag_ClientPort = "IGNITION_HELP";
    nag.nag_Flags = HTF_NOACTIVATE;
	gAmigaGuide = OpenAmigaGuideAsyncA(&nag, NULL);
	agsig = AmigaGuideSignal(gAmigaGuide);
}


int32
LoadFiles(struct List *list)
{
    struct FileArg *fa;
    struct Mappe *mp;
    char   t[256];

	while ((fa = (struct FileArg *)MyRemHead(list)) != 0) {
		if (NameFromLock(fa->fa_Lock, t, 256)) {
			if ((mp = NewProject()) != 0) {
                FreeString(mp->mp_Path);
                mp->mp_Path = AllocString(t);
				SetMapName(mp, fa->fa_Node.ln_Name);
				if (LoadProject(mp, NULL)) {
					ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_PROJECT_ERR), mp->mp_Node.ln_Name);
                    DisposeProject(mp);
                    SetMainPage(NULL);
				} else
					OpenProjWindow(rxpage, TAG_END);
            }
        }

        if (fa->fa_Node.ln_Type == FAT_FROMDOS)
            UnLock(fa->fa_Lock);
        FreeString(fa->fa_Node.ln_Name);
        FreePooled(pool,fa,sizeof(struct FileArg));
    }

	return RETURN_OK;
}


long
dosstart(struct List *list)
{
#ifdef __amigaos4__
	struct AnchorPath *ap;
#else
    struct AnchorPath ALIGNED ap;
#endif
    struct FileArg *fa;
    long   rc = RETURN_OK,temprc;
    long   opts[NUM_OPTS];
    char   **args,*arg;

    memset((char *)opts,0,sizeof(opts));
    if ((ra = ReadArgs(TEMPLATE,opts,NULL)) != 0) {
        if (opts[OPT_WITH])
            prefname = AllocString((STRPTR)opts[OPT_WITH]);
        if (opts[OPT_FILE])
        {
            args = (char **)opts[OPT_FILE];
            while(!rc && (arg = *args++))
            {
#ifdef __amigaos4__
				ap = AllocDosObjectTags(DOS_ANCHORPATH, ADO_Mask, SIGBREAKF_CTRL_C, ADO_Strlen, 1024L, TAG_END ); 
                MatchFirst(arg, ap);
#else
                memset(&ap,0,sizeof(struct AnchorPath));
                MatchFirst(arg,&ap);
#endif
                if ((temprc = IoErr()) != 0)
					PrintFault(temprc, NULL);

                while(!temprc)
                {
#ifdef __amigaos4__
                    if (ap->ap_Info.fib_DirEntryType <= 0) // file
                    {
                        if ((fa = AllocPooled(pool, sizeof(struct FileArg))) != 0) 
                            fa->fa_Lock = DupLock(ap->ap_Current->an_Lock);
                            fa->fa_Node.ln_Name = AllocString(ap->ap_Info.fib_FileName);
						{
                            fa->fa_Node.ln_Type = FAT_FROMDOS;
                            MyAddTail(list, fa);
                        }
                    }
                    temprc = MatchNext(ap);
#else
                    if (ap.ap_Info.fib_DirEntryType <= 0) // file
                    {
                        if ((fa = AllocPooled(pool, sizeof(struct FileArg))) != 0) 
                            fa->fa_Lock = DupLock(ap.ap_Current->an_Lock);
                            fa->fa_Node.ln_Name = AllocString(ap.ap_Info.fib_FileName);
						{
                            fa->fa_Node.ln_Type = FAT_FROMDOS;
                            MyAddTail(list, fa);
                        }
                    }
                    temprc = MatchNext(&ap);
#endif
                }
#ifdef __amigaos4__
				MatchEnd(ap);
				FreeDosObject(DOS_ANCHORPATH,ap);
#else
                MatchEnd(&ap);
#endif
                if (temprc == ERROR_NO_MORE_ENTRIES)
                    rc = RETURN_OK;
                else if (temprc == ERROR_BREAK)
                    rc = RETURN_WARN;
                else
                    rc = RETURN_FAIL;
            }
        }
        FreeArgs(ra);
        return rc;
    }
	PrintFault(IoErr(), NULL);
    return RETURN_FAIL;
}


void
wbstart(struct List *list, struct WBArg *wbarg, LONG first, LONG numargs)
{
    struct FileArg *fa;
    long   i;

    if (!wbarg || !numargs)
        return;

    for (i = first;i < numargs;i++)
    {
        if ((fa = AllocPooled(pool,sizeof(struct FileArg))) != 0) {
            if (first) /* wb-start */
            {
                fa->fa_Lock = wbarg[i].wa_Lock;
                fa->fa_Node.ln_Type = FAT_FROMWB;
            }
            else       /* app-icon */
            {
                fa->fa_Lock = DupLock(wbarg[i].wa_Lock);
                fa->fa_Node.ln_Type = FAT_FROMDOS;
            }
            fa->fa_Node.ln_Name = AllocString(wbarg[i].wa_Name);
            MyAddTail(list, fa);
        }
    }
}


int
main(int argc, char **argv)
{
    static long rc;

#ifdef __amigaos4__
    if (!(pool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_CLEAR | MEMF_ANY, ASOPOOL_Puddle, 16384, ASOPOOL_Threshold, 16384, TAG_END)))
#else
    if (!(pool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 16384, 16384)))
#endif
        return RETURN_FAIL;

#ifdef __amigaos4__
	NewMinList(&files);
#else
    MyNewList(&files);
#endif
	if (argc) {
		if ((rc = dosstart((struct List *)&files)) != 0) {
#ifdef __amigaos4__
			FreeSysObject(ASOT_MEMPOOL, pool);
#else
            DeletePool(pool);
#endif
            return rc;
        }
    }
    else if ((sm = (struct WBStartup *)argv) != 0)
        wbstart((struct List *)&files, sm->sm_ArgList, 1, sm->sm_NumArgs);

#ifndef __amigaos4__
    shelldir = CurrentDir(GetProgramDir());
	if ((GTDragBase = OpenLibrary("gtdrag.library", 3)) != 0) {
#else
    shelldir = SetCurrentDir(GetProgramDir());
	{
#endif
        InitAppClasses();

		if (GTD_AddApp("ignition", GTDA_NewStyle, TRUE, TAG_END)) {
#ifdef __amigaos4__
			if ((iport = AllocSysObjectTags(ASOT_PORT, TAG_END)) != 0) {
#else
			if ((iport = CreateMsgPort()) != 0) {
#endif
                InitApp();

#ifdef __amigaos4__
				if ((rxport = AllocSysObjectTags(ASOT_PORT, ASOPORT_Name, pubname, ASOPORT_Pri, 0, TAG_END)) != 0) {
#else
				if ((rxport = CreatePort(pubname, 0)) != 0) {
#endif
#ifdef __amigaos4__
					if ((sTimeRequest = (struct timerequest *)AllocSysObjectTags(ASOT_IOREQUEST, ASOIOR_ReplyPort, rxport, ASOIOR_Size, sizeof(struct timerequest), TAG_END)) != 0) {
#else
					if ((sTimeRequest = (struct timerequest *)CreateExtIO(rxport, sizeof(struct timerequest))) != 0) {
#endif
						sTimeRequest->tr_node.io_Command = TR_ADDREQUEST;
						if (!OpenDevice(TIMERNAME, UNIT_WAITUNTIL, (struct IORequest *)sTimeRequest, 0L)) {
							TimerBase = (struct Device *)sTimeRequest->tr_node.io_Device;
								// make the timer device library functions available

							GetSysTime((struct TimeVal *)&sTimeRequest->tr_time);
							sTimeInSeconds = sTimeRequest->tr_time.tv_secs + 1;
							sTimeRequest->tr_time.tv_secs = sTimeInSeconds;

							SendIO((struct IORequest *)sTimeRequest);
                                // WaitIO() will produce Enforcer-Hits if that's missing
						} else {
							/* opening the device failed - we have no timer... */
#ifdef __amigaos4__
							FreeSysObject(ASOT_IOREQUEST, (struct IORequest *)sTimeRequest);
#else
							DeleteExtIO((struct IORequest *)sTimeRequest);
#endif
							sTimeRequest = NULL;
                        }
                    }
                    AddInputHandler();
                    InitAppIcon();
					if (!ende && (scr = OpenAppScreen())) {
                        InitAsl();
                        rc = RETURN_FAIL;
						if (IsListEmpty((struct List *)&files)) {
                            if (OpenProjWindow(NULL, TAG_END))	
                                rc = RETURN_OK;
						} else
                            rc = LoadFiles((struct List *)&files);
							
                        if (rc == RETURN_OK)
                            HandleApp();
                    }
                    RemInputHandler();
                }

				if (sTimeRequest != NULL) {
					AbortIO((struct IORequest *)sTimeRequest);
					WaitIO((struct IORequest *)sTimeRequest);
					CloseDevice((struct IORequest *)sTimeRequest);
#ifdef __amigaos4__
					FreeSysObject(ASOT_IOREQUEST, (struct IORequest *)sTimeRequest);
#else
					DeleteExtIO((struct IORequest *)sTimeRequest);
#endif
                }
            }
            CloseApp();
            GTD_RemoveApp();
        }
        FreeAppClasses();
#ifndef __amigaos4__
        CloseLibrary(GTDragBase);       
	} else
        ErrorOpenLibrary("gtdrag.library", NULL);
    CurrentDir(shelldir);
#else
	}
	SetCurrentDir(shelldir);
#endif
#ifdef __amigaos4__
	FreeSysObject(ASOT_MEMPOOL, pool);
#else
    DeletePool(pool);
#endif

    return RETURN_OK;
}
