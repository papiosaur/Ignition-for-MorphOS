/* screen management
 *
 * Copyright 1996-©2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "version.h"
#include "funcs.h"

#if defined  __amigaos4__ || defined __MORPHOS__
	extern char IgnTitle[100];		//ScreenTitle for ignition, must be unique for PubScreen handling
#endif

void
FreeArrows(void)
{
    DisposeObject((Object *)upImg);
    DisposeObject((Object *)downImg);
    DisposeObject((Object *)rightImg);
    DisposeObject((Object *)leftImg);
}


bool
AllocArrows(void)
{
	if ((rightImg = (struct Image *)NewObject(NULL, "sysiclass", SYSIA_Which, RIGHTIMAGE, SYSIA_DrawInfo, dri, TAG_END)) != 0) {
		if ((leftImg = (struct Image *)NewObject(NULL, "sysiclass", SYSIA_Which, LEFTIMAGE, SYSIA_DrawInfo, dri, TAG_END)) != 0) {
			if ((upImg = (struct Image *)NewObject(NULL, "sysiclass", SYSIA_Which, UPIMAGE, SYSIA_DrawInfo, dri, TAG_END)) != 0) {
				if ((downImg = (struct Image *)NewObject(NULL, "sysiclass", SYSIA_Which, DOWNIMAGE, SYSIA_DrawInfo, dri, TAG_END)) != 0)
                    return true;

                DisposeObject((Object *)upImg);
            }
            DisposeObject((Object *)leftImg);
        }
        DisposeObject((Object *)rightImg);
    }
    return false;
}


void
ChangeAppScreen(bool closescreen)
{
    struct MinList list;
    struct saveWin *sw;

	// Save all additional window data

    MyNewList(&list);
	while ((win = GetAppWindow(WDT_ANY)) != 0) {
		if ((sw = AllocPooled(pool, sizeof(struct saveWin))) != 0) {
            sw->sw_Type = ((struct winData *)win->UserData)->wd_Type;
            sw->sw_Left = win->LeftEdge;
            sw->sw_Top = win->TopEdge;
            sw->sw_Width = win->Width;
            sw->sw_Height = win->Height;

            if (sw->sw_Type == WDT_PROJECT)
                sw->sw_WinData = ((struct winData *)win->UserData)->wd_Data;
			else {
                sw->sw_Title = win->Title;
                if ((sw->sw_WinData = AllocPooled(pool,sizeof(struct winData))) != 0)
                    CopyMem(win->UserData,sw->sw_WinData,sizeof(struct winData));

                MyNewList(&sw->sw_WinData->wd_Objs);
            }
            MyAddTail(&list, sw);
        }
		CloseAppWindow(win, FALSE);
    }

	if (closescreen) {
        CloseAppScreen(scr);
        scr = NULL;
    }

	if (!closescreen || (scr = OpenAppScreen())) {
		while ((sw = (struct saveWin *)MyRemTail(&list)) != 0) {
			if (sw->sw_Type == WDT_PROJECT) {
                OpenProjWindow((struct Page *)sw->sw_WinData,WA_Left,  sw->sw_Left,
                                                             WA_Top,   sw->sw_Top,
                                                             WA_Width, sw->sw_Width,
                                                             WA_Height,sw->sw_Height,
                                                             TAG_END);
			} else {
                OpenAppWindow(sw->sw_Type,WA_Left,   sw->sw_Left,
                                          WA_Top,    sw->sw_Top,
                                          WA_Width,  sw->sw_Width,
                                          WA_Height, sw->sw_Height,
                                          WA_WinData,sw->sw_WinData,
                                          TAG_END);
            }
			FreePooled(pool, sw, sizeof(struct saveWin));
        }
    }
	
	gScreenHasChanged = true;
}


void
RefreshItemSize(void)
{
    struct PrefIcon pi;
    struct Mappe *mp;

    pi.pi_Width = pi.pi_Height = 0;

	RefreshIconSizeFromList(&pi, &prefs.pr_AppCmds, false);
	foreach (&gProjects, mp) {
		RefreshIconSizeFromList(&pi, &mp->mp_Prefs.pr_AppCmds, false);
	}

	itemheight = max(pi.pi_Height + 2, fontheight + 2);
	itemwidth = pi.pi_Width - 2;
}


void
CloseAppScreen(struct Screen *scr)
{
    if (!scr)
        return;

	gRemoveObjectsFromScreen(NULL, scr);
    FreeDoubleBufferBitMap();

    FreeArrows();
    FreeImages();
    RefreshImageObjs(NULL);
    FreeVisualInfo(vi);
	FreeScreenDrawInfo(scr, dri);

	if (gAmigaGuide) {
		CloseAmigaGuide(gAmigaGuide);
		gAmigaGuide = NULL;
    }
	ReleasePen(scr->ViewPort.ColorMap, (LONG)treeHook.h_Data);

	if (scr->UserData == (APTR)iport) {
		// this is our own screen

		/*// wait until the screen can be closed
		while (!CloseScreen(scr)) {
			Wait(1L << SIGBREAKB_CTRL_C);
		}
		*/
		// wait for half a second if the screen couldn't be closed, temporaly reactivate, because problems with SA_PubSig
		while (!CloseScreen(scr))
			Delay(25);
	} else {
		ReleaseAppColors(scr);
		UnlockPubScreen(NULL, scr);
    }
}


void
InitAppScreen(struct Screen *scr)
{
    fontheight = barheight = scr->Font->ta_YSize;
	barheight += scr->WBorTop + 1;
    linelen = TLn("_");
	lborder = scr->WBorLeft + 4;
    bborder = scr->WBorBottom;
	rborder = scr->WBorRight + 4;

	vi = GetVisualInfo(scr, TAG_END);
}


struct Screen *
OpenAppScreen(void)
{
    struct Screen *scr = NULL;
    long   depth;
    struct Rectangle dims;
     
#ifdef __amigaos4__
			ULONG dispid;
#endif

	switch (prefs.pr_Screen->ps_Type) {
        case PST_OWN:
            if ((scr = OpenScreenTags(NULL,SA_LikeWorkbench, TRUE,
                                          SA_PubName,       pubname,
#ifndef __amigaos4__
										  SA_Title,			IgnTitle,
	                                      SA_Interleaved,   TRUE,
	                                      SA_Colors32,      standardPalette,
#else
										  SA_Title,         SCREEN_TITLE,
#endif
                                          SA_Font,          &prefs.pr_Screen->ps_TextAttr,
                                          SA_DisplayID,     prefs.pr_Screen->ps_ModeID,
                                          SA_Width,         prefs.pr_Screen->ps_Width,
                                          SA_Height,        prefs.pr_Screen->ps_Height,
                                          SA_Depth,         prefs.pr_Screen->ps_Depth,
#ifndef __amigaos4__
                                          SA_BackFill,      prefs.pr_Screen->ps_BackFill ? &fillHook : NULL,
#endif 
                                          //SA_PubTask,       FindTask(NULL),
										  //SA_PubSig,        SIGBREAKB_CTRL_C,
                                          TAG_END)) != 0)
                scr->UserData = (APTR)iport;
            break;
        case PST_LIKEWB:            
			if ((scr = LockPubScreen("Workbench")) != 0) {
				// if the depth of the Workbench screen is lower than 3 (fewer than 8 colours),
				// we will set our amount of colours to a minimum of 8 colours.
				if ((depth = scr->RastPort.BitMap->Depth) < 3)
                    depth = 3;
#ifdef __amigaos4__
				dispid = GetVPModeID(&scr->ViewPort);
#endif
                    
				UnlockPubScreen(NULL, scr);
            }
            if ((scr = OpenScreenTags(NULL,SA_LikeWorkbench, TRUE,
#ifdef __amigaos4__
                                          SA_DisplayID,     dispid,
										  SA_Title,			IgnTitle,
#endif
										  SA_PubName,       pubname,
#ifndef __amigaos4__
										  SA_Title,         SCREEN_TITLE,
	                                      SA_Interleaved,   TRUE,
	                                      SA_Colors32,      standardPalette,
#endif
	                                      SA_Depth,         depth,
#ifndef __amigaos4__
	                                      SA_BackFill,      prefs.pr_Screen->ps_BackFill ? &fillHook : NULL,
#endif
                                          //SA_PubTask,       FindTask(NULL),
										  //SA_PubSig,        SIGBREAKB_CTRL_C,
	                                      TAG_END)) != 0)
            	scr->UserData = (APTR)iport;
            break;
        case PST_PUBLIC:
            scr = LockPubScreen(prefs.pr_Screen->ps_PubName);
            break;
    }

	if (!scr) {
		ErrorRequest(GetString(&gLocaleInfo, MSG_OPEN_SCREEN_ERR));
        scr = LockPubScreen(NULL);
	}

    ScreenToFront(scr);

	if (scr) {
		ULONG xdpi, ydpi;

		if (scr->UserData == (APTR)iport) {
			// reserve the first 8 screen colors
			PubScreenStatus(scr, 0);
			for (depth = 0; depth < 8; depth++) {
				ObtainPen(scr->ViewPort.ColorMap, depth, 0, 0, 0, /*PEN_EXCLUSIVE |*/ PEN_NO_SETCOLOR);
			//for (depth = 0; depth < 8; depth++) {
				//ObtainPen(scr->ViewPort.ColorMap, depth, standardPalette[depth * 3 + 1], standardPalette[depth * 3 + 2], standardPalette[depth * 3 + 3], PEN_EXCLUSIVE | PEN_NO_SETCOLOR);
			}
        }
        ObtainAppColors(scr,TRUE);
		MakeTmpRas(scr->Width, scr->Height);
        RefreshImageObjs(scr);
		treeHook.h_Data = (APTR)ObtainBestPen(scr->ViewPort.ColorMap, 0x44444444, 0x44444444, 0x44444444,
			OBP_Precision, PRECISION_GUI, TAG_END);

        InitAppScreen(scr);

        RefreshItemSize();
        itext.ITextFont = scr->Font;
        {
            struct Mappe *mp;

            RefreshContextMenus(&prefs);
			foreach (&gProjects, mp) {
                RefreshContextMenus(&mp->mp_Prefs);
			}
        }
        dri = GetScreenDrawInfo(scr);
		boxwidth = max((fontheight + 5) * dri->dri_Resolution.Y / dri->dri_Resolution.X, 9);
        if (!(boxwidth % 2))
            boxwidth++;
#ifdef __amigaos4__
        wd_StatusWidth = max(TLn(GetString(&gLocaleInfo, MSG_EDIT_STATUS)), TLn(GetString(&gLocaleInfo, MSG_INTERACTIVE_STATUS))) + 8;
#else
        wd_StatusWidth = TLn("Inter") + 8;
#endif
        wd_PosWidth = TLn("AAA999:AAA999")+42;

        prefs.pr_Screen->ps_dimWidth = scr->Width;
        prefs.pr_Screen->ps_dimHeight = scr->Height;
		if (QueryOverscan(GetVPModeID(&scr->ViewPort), &dims, OSCAN_TEXT)) {
			prefs.pr_Screen->ps_dimWidth = dims.MaxX + 1 - dims.MinX;
			prefs.pr_Screen->ps_dimHeight = dims.MaxY + 1 - dims.MinY;
        }

		xdpi = ((LONG)prefs.pr_Screen->ps_dimWidth * (LONG)(25.4*1024)/prefs.pr_Screen->ps_mmWidth);
		ydpi = ((LONG)prefs.pr_Screen->ps_dimHeight * (LONG)(25.4*1024)/prefs.pr_Screen->ps_mmHeight);
		gDPI = (xdpi << 16) | ydpi;

		if (prefs.pr_Screen->ps_BackFill) {
            scrRp = scr->RastPort;
            scrRp.Layer = NULL;  SetAPen(&scrRp, prefs.pr_Screen->ps_BFColor);
            scrRp.AreaPtrn = (short *)&dithPtrn;  scrRp.AreaPtSz = 1;
            if (scr->UserData == (APTR)iport)
				RectFill(&scrRp, 0, scr->BarHeight + 1, scr->Width - 1, scr->Height - 1);
        }
		LayoutMenus(prefs.pr_Menu, vi, GTMN_NewLookMenus, TRUE, TAG_END);

        AllocImages(scr);
        AllocArrows();
        InitHelp(scr);

        AllocDoubleBufferBitMap(scr);
		gAddObjectsToScreen(NULL, scr);
    }
    return scr;
}
