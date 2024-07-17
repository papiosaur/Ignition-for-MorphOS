/* Print support implementation
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"

#include <exec/execbase.h> 
#include <dos/dostags.h>
#include <prefs/prefhdr.h>
#include <prefs/printertxt.h>
#ifdef __amigaos4__
	#include <prefs/printergfx.h>
	
	BOOL pdf = FALSE;
	APTR oldfunc1 = NULL, oldfunc2 = NULL, aslreq =  NULL;
	extern void gInitArea(long xmin, long ymin, long xmax, long ymax);
#endif

#ifndef ID_PDEV
#	define ID_PDEV MAKE_ID('p', 'd', 'e', 'v')
#	define UNITNAMESIZE 40
struct PrinterDeviceUnitPrefs { char pd_UnitName[40]; };
#endif

extern void DrawRastaCellWidth(struct RastPort *rp, long x, long y, long w, long h);
extern void DrawTFText(struct RastPort *rp, struct Page *page, struct Rectangle *bounds, struct tableField *tf, long x, long y);
#ifdef __amigaos4__
	#include <prefs/printerps.h>
	
	#include "classes.h"
	
	#define STRIPE 100
	
	extern BOOL PrintGObject(struct RastPort *rp, struct Page *page, struct gObject *go, long x, long y, struct Rect32 *rect);
#endif

// async printer task
static struct Task *asp_maintask;
struct Task *asp_printtask;
struct wdtPrintStatus *asp_wps;
static struct List asp_queue;
static BOOL asp_adding;
static long asp_current,asp_count;
static struct Mappe *asp_mp;
static STRPTR asp_title;
static const char *sPrintLabels[6];

extern struct SignalSemaphore gWindowSemaphore;


struct PrintScreenContext {
	struct ColorMap *ps_ColorMap;
	struct ViewPort *ps_ViewPort;
	struct Screen *ps_Screen;
};
 
struct PrintColorContext {
	struct ColorContext pc;
	struct ColorMap *pc_ColorMap;
	struct ViewPort *pc_ViewPort;
	LONG   pc_TruePen;
	LONG   pc_LowTruePen;
	LONG   pc_OutlineTruePen;
};

#ifdef __amigaos4__
	extern void ClosePrintScreen(struct PrintScreenContext *context);
	extern BOOL OpenPrintScreen(struct PrintScreenContext *context, struct Mappe *map, struct RastPort *rp, struct PrintColorContext *colorContext);
#endif
 
static const LONG kIFFPrefChunks[] = {
	ID_PREF, ID_PRHD,
	ID_PREF, ID_PDEV,
};
#define kIFFPrefChunkCount 2

#ifdef __amigaos4__
#define INIT		0
#define POSTSCRIPT	1
#define FILE		2
#define NORMAL		3
uint8 prefsbuffer[204800];

struct PrinterPSPrefs 			PSPrefs;
struct PrefHeader				HDR;
struct PrinterTxtPrefs 			PTXT;
struct PrinterUnitPrefs 		PUNT;
struct PrinterDeviceUnitPrefs 	PDEV;
struct PrinterGfxPrefs  		PGFX;
struct PData 	{
    			uint8	Unit;
    			uint8	Type;
    			uint16	XDPI;
    			uint16	YDPI;
    			uint16  XOffset;
    			uint16	YOffset;
    			} PDatas;
    			
uint16 dpiX[] 	= {0, 68, 68, 75, 100, 150, 300, 600};
uint16 dpiY[]	= {0, 35, 70, 75, 100, 150, 300, 600};
uint16 PSdpiX[]	= {0, 100, 150, 300, 600, 1200, 2400, 0};
uint16 PSdpiY[]	= {0, 100, 150, 300, 600, 1200, 2400, 0};

uint8 GetPageFormatID(uint32 height, BOOL ps, int32 *pw, int32 *ph)
{
    uint8 PSid[] 	= {7, 2, 8, 0, 1};		//ID for PS-Prefs
    uint8 nonPSid[]	= {8, 9, 10, 0, 1};		//ID for normal printer 
    uint8 i;
    
	for (i = 0; i < pageSizes; i++)			//Search for Papersize
    {
        if (pageHeight[i] == height)
            break;
    }
    if(ps)
    {
        *pw = (int32)((((double)pageWidth[i] / 1024.0) / 25.4) * 72000.0);
        *ph = (int32)((((double)pageHeight[i] / 1024.0) / 25.4) * 72000.0);
    }
    return(ps ? PSid[i] : nonPSid[i]);
}

BOOL GetPrinterPrefs(struct Mappe *mp, uint8 typ)
{
	struct IFFHandle *iff;
	BPTR   fp;
	struct StoredProperty *sp;
	struct PrefHeader *hdr;
	struct PrinterPSPrefs *prefsps;
	struct PrinterTxtPrefs *prefstxt;
	struct PrinterUnitPrefs *prefspunt;
	struct PrinterDeviceUnitPrefs *prefspdev;
	struct PrinterGfxPrefs *prefgfx;
	char prefsfile[] = {"env:sys/printer_.prefs"};
	
	prefsfile[15] = PDatas.Unit + 0x30;	//Set prefsfilenumber
	if(typ == POSTSCRIPT)
	{
		if (!(fp = Open("env:sys/printerps.prefs", MODE_OLDFILE)))
			return FALSE;
	}
	else
	{
		if (!(fp = Open((PDatas.Unit == 0 ? "env:sys/printer.prefs" : prefsfile), MODE_OLDFILE)))
			return FALSE;
	}
	if ((iff = AllocIFF()) != 0)
	{
		iff->iff_Stream = (IPTR)fp;
		InitIFFasDOS(iff);

		if (!OpenIFF(iff, IFFF_READ))
		{
			switch(typ)
			{
			    case INIT:
					PropChunk(iff, ID_PREF, ID_PRHD);
					PropChunk(iff, ID_PREF, ID_PTXT);
					PropChunk(iff, ID_PREF, ID_PUNT);
					PropChunk(iff, ID_PREF, ID_PDEV);
					PropChunk(iff, ID_PREF, ID_PGFX);
					StopOnExit(iff,ID_PREF, ID_FORM);
					ParseIFF(iff, IFFPARSE_SCAN);
				    sp = FindProp(iff, ID_PREF, ID_PRHD);
				    hdr = sp->sp_Data;
				    sp = FindProp(iff, ID_PREF, ID_PTXT);
				    prefstxt = sp->sp_Data;
				    sp = FindProp(iff, ID_PREF, ID_PUNT);
				    prefspunt = sp->sp_Data;
				    sp = FindProp(iff, ID_PREF, ID_PDEV);
				    prefspdev = sp->sp_Data;
				    sp = FindProp(iff, ID_PREF, ID_PGFX);
				    prefgfx = sp->sp_Data;
				    if(Stricmp("PostScript", prefstxt->pt_Driver) == 0)
				    {
				    	PDatas.Type = POSTSCRIPT;
				    	if(prefgfx->pg_PrintDensity == 7)
				    		PDatas.XDPI	= PDatas.YDPI = 0;
				    	else
						{
							PDatas.XDPI		= PSdpiX[prefgfx->pg_PrintDensity];
				    		PDatas.YDPI		= PSdpiY[prefgfx->pg_PrintDensity];
					    }			    	
				    }
				    else
				    {
				    	PDatas.Type = FILE;
				    	PDatas.XDPI	= dpiX[prefgfx->pg_PrintDensity];
				    	PDatas.YDPI	= dpiY[prefgfx->pg_PrintDensity];
			    		PDatas.XOffset	= (uint16)(((mp->mp_BorderLeft / 10240.0) / 2.54) * (double)PDatas.XDPI);
			    		PDatas.YOffset	= (uint16)(((mp->mp_BorderTop  / 10240.0) / 2.54) * (double)PDatas.YDPI);
				    }
				    CopyMem(hdr, &HDR, sizeof(struct PrefHeader));
				    CopyMem(prefstxt, &PTXT, sizeof(struct PrinterTxtPrefs));
				    CopyMem(prefspunt, &PUNT, sizeof(struct PrinterUnitPrefs));
				    CopyMem(prefspdev, &PDEV, sizeof(struct PrinterDeviceUnitPrefs));
			    	CopyMem(prefgfx, &PGFX, sizeof(struct PrinterGfxPrefs));
					break;			        
			    case POSTSCRIPT:
					PropChunk(iff, ID_PREF, ID_PRHD);
					PropChunk(iff, ID_PREF, ID_PSPD);
					StopOnExit(iff,ID_PREF, ID_FORM);
					ParseIFF(iff, IFFPARSE_SCAN);
				    sp = FindProp(iff, ID_PREF, ID_PRHD);
				    hdr = sp->sp_Data;
			    	sp = FindProp(iff, ID_PREF, ID_PSPD);
				    prefsps = sp->sp_Data;
				    if(PDatas.XDPI == 0)
				    {
						PDatas.XDPI	= prefsps->ps_HorizontalDPI;
			    		PDatas.YDPI	= prefsps->ps_VerticalDPI;
				    }
				    CopyMem(prefsps, &PSPrefs, sizeof(struct PrinterPSPrefs));
			    	CopyMem(hdr, &HDR, sizeof(struct PrefHeader));
			    	break;
		    }
		}
		CloseIFF(iff);
	}
	FreeIFF(iff);
	Close(fp);
	return TRUE;
}

BOOL SetPrinterPrefs(struct Mappe *mp, uint32 type)
{
	struct IFFHandle *iff;
	BPTR   fp;
//	struct PrefHeader hdr;
	struct PrinterPSPrefs psprefs;
	struct PrinterTxtPrefs prefstxt;
	struct PrinterGfxPrefs prefgfx;
	char prefsfile[] = {"env:sys/printer_.prefs"};
	
	//Set new Margins
	if(type == POSTSCRIPT)
	{
	    CopyMem(&PSPrefs, &psprefs, sizeof(struct PrinterPSPrefs)); //create copy from user-prefs
		psprefs.ps_LeftEdge 	= ((mp->mp_BorderLeft / 1024.0) / 25.4) * 72000;
		psprefs.ps_TopEdge		= ((mp->mp_BorderTop / 1024.0) / 25.4) *72000;
		psprefs.ps_PaperFormat	= GetPageFormatID(mp->mp_mmHeight, TRUE, &psprefs.ps_PaperWidth, &psprefs.ps_PaperHeight);
		psprefs.ps_Width		= psprefs.ps_PaperWidth - psprefs.ps_LeftEdge - ((mp->mp_BorderRight/1024.0) / 25.4) * 72000;
		psprefs.ps_Height		= psprefs.ps_PaperHeight - psprefs.ps_TopEdge - ((mp->mp_BorderBottom/1024.0) / 25.4) * 72000;
		psprefs.ps_Aspect		= ASP_HORIZ;
		psprefs.ps_ScalingType	= ST_ASPECT_ASIS;
		psprefs.ps_Centering 	= CENT_NONE;
		if (!(fp = Open("env:sys/printerps.prefs", MODE_READWRITE)))
			return FALSE;
	}
	else
	{
		prefsfile[15] = PDatas.Unit + 0x30;	//Set prefsfilenumber
	    CopyMem(&PGFX, &prefgfx, sizeof(struct PrinterGfxPrefs)); //create copy from user-prefs
	    CopyMem(&PTXT, &prefstxt, sizeof(struct PrinterTxtPrefs)); //create copy from user-prefs
	    prefgfx.pg_PrintXOffset	= 0; // (UBYTE)((mp->mp_BorderLeft / 1024.0) / 2.5);
	    prefgfx.pg_PrintYOffset	= 0; //(UBYTE)((mp->mp_BorderTop / 1024.0) / 2.5);
	    prefgfx.pg_Aspect 		= PA_HORIZONTAL;
	    prefgfx.pg_Dimensions	= PD_IGNORE;
	    prefgfx.pg_Dithering	= PD_ORDERED;
	    prefgfx.pg_GraphicFlags	= 0;
		prefstxt.pt_PaperSize	= GetPageFormatID(mp->mp_mmHeight, FALSE, 0, 0);
		if (!(fp = Open((PDatas.Unit == 0 ? "env:sys/printer.prefs" : prefsfile), MODE_OLDFILE)))
			return FALSE;
	}
		
	//Write Prefs-File
	if ((iff = AllocIFF()) != 0)
	{
		iff->iff_Stream = (IPTR)fp;
		InitIFFasDOS(iff);

		if (!OpenIFF(iff, IFFF_WRITE))
		{
		    PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN);
		    PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct PrefHeader));
		    WriteChunkBytes(iff, &HDR, sizeof(struct PrefHeader));
		    PopChunk(iff);
			if(type == POSTSCRIPT)
			{
			    PushChunk(iff, ID_PREF, ID_PSPD, sizeof(struct PrinterPSPrefs));
			    WriteChunkBytes(iff, &psprefs, sizeof(struct PrinterPSPrefs));
			    PopChunk(iff);
		    }
		    else
		    {
			    PushChunk(iff, ID_PREF, ID_PTXT, sizeof(struct PrinterTxtPrefs));
			    WriteChunkBytes(iff, &prefstxt, sizeof(struct PrinterTxtPrefs));
			    PopChunk(iff);
			    PushChunk(iff, ID_PREF, ID_PUNT, sizeof(struct PrinterUnitPrefs));
			    WriteChunkBytes(iff, &PUNT, sizeof(struct PrinterUnitPrefs));
			    PopChunk(iff);
			    PushChunk(iff, ID_PREF, ID_PDEV, sizeof(struct PrinterDeviceUnitPrefs));
			    WriteChunkBytes(iff, &PDEV, sizeof(struct PrinterDeviceUnitPrefs));
			    PopChunk(iff);
			    PushChunk(iff, ID_PREF, ID_PGFX, sizeof(struct PrinterGfxPrefs));
			    WriteChunkBytes(iff, &prefgfx, sizeof(struct PrinterGfxPrefs));
			    PopChunk(iff);
		    }
		    PopChunk(iff);
		}
		CloseIFF(iff);
	}
	FreeIFF(iff);
	Close(fp);
	return TRUE;
}

BOOL RestorePrinterPrefs(uint32 type)
{
	struct IFFHandle *iff;
	BPTR   fp;
	char prefsfile[] = {"env:sys/printer_.prefs"};

	if(type == POSTSCRIPT)
	{
		if (!(fp = Open("env:sys/printerps.prefs", MODE_OLDFILE)))
			return FALSE;
	}
	else
	{
		prefsfile[15] = PDatas.Unit + 0x30;	//Set prefsfilenumber
		if (!(fp = Open((PDatas.Unit == 0 ? "env:sys/printer.prefs" : prefsfile), MODE_OLDFILE)))
			return FALSE;
	}
	if ((iff = AllocIFF()) != 0)
	{
		iff->iff_Stream = (IPTR)fp;
		InitIFFasDOS(iff);
		if (!OpenIFF(iff, IFFF_WRITE))
		{
		    PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN);
		    PushChunk(iff, ID_PREF, ID_PRHD, sizeof(struct PrefHeader));
		    WriteChunkBytes(iff, &HDR, sizeof(struct PrefHeader));
	    	PopChunk(iff);
			if(type == POSTSCRIPT)
			{
			    PushChunk(iff, ID_PREF, ID_PSPD, sizeof(struct PrinterPSPrefs));
			    WriteChunkBytes(iff, &PSPrefs, sizeof(struct PrinterPSPrefs));
			    PopChunk(iff);
			}
			else
			{
			    PushChunk(iff, ID_PREF, ID_PTXT, sizeof(struct PrinterTxtPrefs));
			    WriteChunkBytes(iff, &PTXT, sizeof(struct PrinterTxtPrefs));
			    PopChunk(iff);
			    PushChunk(iff, ID_PREF, ID_PUNT, sizeof(struct PrinterUnitPrefs));
			    WriteChunkBytes(iff, &PUNT, sizeof(struct PrinterUnitPrefs));
			    PopChunk(iff);
			    PushChunk(iff, ID_PREF, ID_PDEV, sizeof(struct PrinterDeviceUnitPrefs));
			    WriteChunkBytes(iff, &PDEV, sizeof(struct PrinterDeviceUnitPrefs));
			    PopChunk(iff);
			    PushChunk(iff, ID_PREF, ID_PGFX, sizeof(struct PrinterGfxPrefs));
			    WriteChunkBytes(iff, &PGFX, sizeof(struct PrinterGfxPrefs));
			    PopChunk(iff);
			}
	    	PopChunk(iff);
			CloseIFF(iff);
		}
	}
	FreeIFF(iff);
	Close(fp);
	return TRUE;
}

void CreatePDFFile(void)
{
    SystemTags("GHOSTSCRIPT:gs -q -dNOPAUSE -sDEVICE=pdfwrite -sOutputFile=ram:Print.pdf -dBATCH ram:Print.ps", SYS_Input,NULL,SYS_Output,NULL,SYS_Error,NULL,SYS_Asynch,FALSE,TAG_DONE);
    Delete("ram:Print.ps");
}

STATIC BOOL APICALL WriteFileData(struct FileRequester *requester, ...)
{
    if(requester != aslreq) //the new request is not active
		requester = aslreq;
	return TRUE;
}

STATIC APTR APICALL AslReqAlloc(uint32 type, ...)
{
    return aslreq;
}
#endif

BOOL
ReadUnitName(char *filename, char *name, int unit)
{
	struct ContextNode *cn;
	struct IFFHandle *iff;
	BOOL   ok = TRUE;
	BPTR   file;

	if (!(file = Open(filename, MODE_OLDFILE)))
		return FALSE;

	sprintf(name, GetString(&gLocaleInfo, MSG_DEFAULT_PRINTER_UNIT_NAME), unit);

	if ((iff = AllocIFF()) != 0)
	{
		iff->iff_Stream = (IPTR)file;
		InitIFFasDOS(iff);

		if (!OpenIFF(iff, IFFF_READ))
		{
			long error;

			if (!ParseIFF(iff, IFFPARSE_STEP))
			{
				cn = CurrentChunk(iff);
				if (cn->cn_ID == ID_FORM && cn->cn_Type == ID_PREF)
				{
					if (!StopChunks(iff, (LONG *)kIFFPrefChunks, kIFFPrefChunkCount))
					{
						while (ok)
						{
							if ((error = ParseIFF(iff, IFFPARSE_SCAN)) != 0) {
//printf("parse error = %ld\n", error);
								break;
							}
 
							cn = CurrentChunk(iff);
							if (cn->cn_Type != ID_PREF)
								continue;

							switch (cn->cn_ID) {
								case ID_PRHD:
								{
									struct PrefHeader phead;
									if (ReadChunkBytes(iff, &phead, sizeof(struct PrefHeader)) != sizeof(struct PrefHeader)
										|| phead.ph_Version != 0)
										ok = FALSE;
									break;
								}

								case ID_PDEV:
								{
									struct PrinterDeviceUnitPrefs pdev;
//puts("hier bin isch ooch");
									if (ReadChunkBytes(iff, &pdev, sizeof(pdev)) == sizeof(pdev))
									{
										if (pdev.pd_UnitName[0])
											strcpy(name, pdev.pd_UnitName);
									}
									break;
								}
							}
						}
					}
				}
			}
			CloseIFF(iff);
		}
		FreeIFF(iff);
	}
	Close(file);

	return TRUE;
}


void
ClosePrintWindow(struct wdtPrintStatus *wps)
{
	if (!wps)
		return;

	wps->wps_Locked--;
	CloseAppWindow(wps->wps_Window,TRUE);
}


struct wdtPrintStatus *
OpenPrintWindow(void)
{
	struct Window *win = OpenAppWindow(WDT_PRINTSTATUS,TAG_END);

	if (win)
	{
		struct winData *wd = (struct winData *)win->UserData;
		struct wdtPrintStatus *wps = (struct wdtPrintStatus *)wd->wd_ExtData[0];

		if (wps)
			wps->wps_Locked++;
		return wps;
	}
	return NULL;
}


void
FreePrinterProject(struct List *list)
{
	struct tableField *tf;
	struct Page *page;
	struct Mappe *mp = NULL;

	{
		struct wdtPrinter *wp;

		foreach(list,wp)
		{
			if (wp->wp_Page)
			{
				mp = wp->wp_Page->pg_Mappe;
				break;
			}
		}
	}
	if (!mp || mp->mp_Node.ln_Type != LNT_PRINTERMAP)
		return;

	while ((page = (APTR)MyRemHead(&mp->mp_Pages)) != 0)
	{
		/*** free cells ***/

		while ((tf = (APTR)MyRemHead(&page->pg_Table)) != 0)
		{
			FreeString(tf->tf_Text);
			FreeFontInfo(tf->tf_FontInfo);

			FreePooled(pool,tf,sizeof(struct tableField));
		}

		/*** free objects ***/
		{
			int i,num = page->pg_NumObjects;
			struct gObject **gos = page->pg_ObjectOrder;

			for(i = 0;i < num;i++)
				FreeGObject(gos[i]);

			if (gos)
				FreePooled(pool,gos,sizeof(APTR)*num);
		}

		/*** free misc ***/
		FreeString(page->pg_Node.ln_Name);

		FreePooled(pool,page,sizeof(struct Page));
	}
	FreeString(mp->mp_Node.ln_Name);
	FreeString(mp->mp_Path);

	FreePooled(pool,mp,sizeof(struct Mappe));
}


struct tableSize *
CopyTableSize(struct Page *page, BOOL horiz)
{
	struct tableSize *ts;
	long size;

	if ((ts = AllocPooled(pool, sizeof(struct tableSize) * (size = horiz ? page->pg_Cols : page->pg_Rows))) != 0)
	{
		long i;

		CopyMem(horiz ? page->pg_tfWidth : page->pg_tfHeight, ts, sizeof(struct tableSize) * size);

		for (i = 0; i < size; i++)
			ts[i].ts_Title = AllocString(ts[i].ts_Title);
	}
	return ts;
}


struct Mappe *
GetPrintListMap(struct List *list)
{
	struct wdtPrinter *wp;

	if (!list || IsListEmpty(list) || !(wp = (struct wdtPrinter *)list->lh_Head)->wp_Page)
		return NULL;

	return wp->wp_Page->pg_Mappe;
}


struct Mappe *
CopyPrinterProject(struct List *list)
{
	struct Mappe *mp = GetPrintListMap(list);
	struct Page *cpage,*page;
	struct wdtPrinter *wp;
	struct Mappe *cmp;

	if (!mp || mp->mp_Node.ln_Type != LNT_MAP)
		return NULL;

	if ((cmp = AllocPooled(pool,sizeof(struct Mappe))) == NULL)
		return NULL;

	CopyMem(mp, cmp, sizeof(struct Mappe));
	MyNewList(&cmp->mp_Pages);

	cmp->mp_Node.ln_Name = AllocString(mp->mp_Node.ln_Name);
	cmp->mp_Node.ln_Type = LNT_PRINTERMAP;
	cmp->mp_Path = AllocString(mp->mp_Path);
	cmp->mp_actPage = NULL;
	cmp->mp_Window = NULL;

	foreach (list, wp)
	{
		if (!wp->wp_Node.ln_Type)
		{
			wp->wp_Page = NULL;
			continue;
		}
		page = wp->wp_Page;

		if ((cpage = AllocPooled(pool, sizeof(struct Page))) != 0)
		{
			struct tableField *tf, *ctf;

			CopyMem(page, cpage, sizeof(struct Page));
			MyNewList(&cpage->pg_Table);
			MyNewList(&cpage->pg_gObjects);
			MyNewList(&cpage->pg_gGroups);

			cpage->pg_Node.ln_Name = AllocString(page->pg_Node.ln_Name);
			cpage->pg_Node.ln_Type = LNT_PRINTERPAGE;
			cpage->pg_Window = NULL;
			cpage->pg_Mappe = cmp;

			cpage->pg_tfWidth = CopyTableSize(page, TRUE);
			cpage->pg_tfHeight = CopyTableSize(page, FALSE);

			wp->wp_Page = cpage;

			/*** Copy cells, update tableSize's cell references ***/

			foreach (&page->pg_Table, tf)
			{
				if ((ctf = AllocPooled(pool, sizeof(struct tableField))) != 0)  // simple copy, must not be freed via FreeTableField()!
				{
					CopyMem(tf, ctf, sizeof(struct tableField));
					ctf->tf_Text = AllocString(tf->tf_Text);
					ctf->tf_FontInfo = CopyFontInfo(tf->tf_FontInfo);

					MyAddTail(&cpage->pg_Table, ctf);
				}
			}
			LinkCellsToTableSize(cpage);

			/*** Copy gObjects ***/
			{
				int i, num = page->pg_NumObjects;
				struct gObject **cgos = NULL;

				if (num && (cgos = AllocPooled(pool, sizeof(APTR)*num)))
				{
					struct gObject **gos = page->pg_ObjectOrder;

					for (i = 0; i < num; i++)
					{
						struct gObject *go;
						
						// ToDo: embedded_diagram copies the diagram it references
						//	to the source page, not ours...

						if ((go = (struct gObject *)gDoMethod(gos[i], GCM_COPY)) != 0)
						{
							cgos[i] = go;
							MyAddTail(&cpage->pg_gGroups, OBJECTGROUP(go));
							MyAddTail(&cpage->pg_gObjects, go);
						}
					}
				}
				cpage->pg_ObjectOrder = cgos;
			}

			MyAddTail(&cmp->mp_Pages, cpage);
		}
	}
	return cmp;
}


void
PrintPattern(struct RastPort *rp, UBYTE pattern, WORD x1, WORD y1, WORD x2, WORD y2, UBYTE a, UBYTE b)
{
	SetPattern(rp, pattern, a, b);  SetDrMd(rp, JAM2);
	RectFill(rp, x1, y1, x2, y2);
	SetAfPt(rp, NULL, 0);
}


void
#ifdef __amigaos4__
PrintTableRegion(union printerIO *pio, struct RastPort *rp, struct Page *page, struct Rect32 *frame)
#else
PrintTableRegion(struct RastPort *rp, struct Page *page, struct Rect32 *frame)
#endif
{
	long   tabLeft,tabTop,tabXOff,tabYOff;
	long   x,y,i,border,w,h,xmin,ymin;
	struct Rectangle rect;
	struct tableField *tf;

	if (!rp || !page)
		return;

	rect.MinX = rect.MinY = 0;  xmin = frame->MinX;  ymin = frame->MinY;
	rect.MaxX = frame->MaxX - xmin;  rect.MaxY = frame->MaxY - ymin;

	/* Erstes Feld im Bereich in X-Richtung */
	border = xmin;
	for(x = 0,i = 0,tabLeft = 1;x <= border;x += i)
	{
		i = GetTFWidth(page,tabLeft);
		if (x+i <= border)
			tabLeft++;
	}
	tabXOff = (x-i)-border;
	if (!(i+tabXOff))
		tabXOff = 0;

	/* Erstes Feld im Bereich in Y-Richtung */
	border = ymin;
	for(y = 0,i = 0,tabTop = 1;y <= border;y += i)
	{
		i = GetTFHeight(page,tabTop);
		if (y+i <= border)
			tabTop++;
	}
	tabYOff = (y-i)-border;
	if (!(i+tabYOff))
		tabYOff = 0;

	SetHighColor(rp, page->pg_BPen);
	SetBPen(rp, 0);  SetDrMd(rp, JAM2);
#ifdef __amigaos4__
	RectFill(rp, rect.MinX, rect.MinY, pio->iodrp.io_DestCols, pio->iodrp.io_SrcHeight + 2);
#else
	RectFill(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
#endif

#ifndef __amigaos4__
	makeLayerClip(rp->Layer,rect.MinX,rect.MinY,rect.MaxX-rect.MinX,rect.MaxY-rect.MinY);
#endif

	for(y = rect.MinY+tabYOff;y <= rect.MaxY;y += h,tabTop++)
	{
		long nextleft;

		h = GetTFHeight(page,tabTop);  tf = NULL;
		makeLayerClip(rp->Layer,rect.MinX,rect.MinY,rect.MaxX-rect.MinX,rect.MaxY-rect.MinY);

		for(x = rect.MinX+tabXOff,i = nextleft = tabLeft;(x <= rect.MaxX);x += w,i++)
		{
			w = GetTFWidth(page,i);
			if (!tf)
				tf = GetTableField(page,i,tabTop);
			else while(tf && (tf->tf_Col < i && tf->tf_Row == tabTop || tf->tf_Row < tabTop))
				tf = NextTableField(tf);

			if (tf && tf->tf_Col == i && tf->tf_Row == tabTop)
			{
				nextleft += tf->tf_Width+2;

				if (tf->tf_BPen != page->pg_BPen || tf->tf_Pattern && tf->tf_PatternColor != tf->tf_BPen)
				{
					if (tf->tf_Pattern)
					{
						ULONG bpen = GetBPen(rp);

						SetColors(rp,tf->tf_PatternColor,tf->tf_BPen);
						PrintPattern(rp,tf->tf_Pattern,x,y,x+w-1,y+h-1,xmin % 16,ymin % 4);
						SetHighColor(rp,tf->tf_BPen);
						SetBPen(rp,bpen);
					}
					else
					{
						SetHighColor(rp,tf->tf_BPen);
						RectFill(rp,x,y,x+w-1,y+h-1);
					}
				}
				/*else if (nobackpen)
				{
					SetHighColor(rp,tf->tf_BPen);
					if ((pd->pd_Rasta & PDR_CYCLES) && !(pd->pd_Rasta & PDR_CELLWIDTH))
						RectFill(rp,x,y,x+w-2,y+h-2);
					else
						RectFill(rp,x,y,x+w-1,y+h-1);
				}*/

				if (tf->tf_Border[0])
				{
					SetHighColor(rp,tf->tf_BorderColor[0]);
					/*border = ((tf->tf_Border[0]*(page->pg_DPI >> 16)/72)+1)/32;
					RectFill(rp,x,y,x+border,y+h-1);*/
					DrawVertBlock(rp,page->pg_DPI,x,y,y+h-1,tf->tf_Border[0]*8,0);
				}
				if (tf->tf_Border[1])
				{
					SetHighColor(rp,tf->tf_BorderColor[1]);
					DrawVertBlock(rp,page->pg_DPI,x+w-1,y+h-1,y,tf->tf_Border[1]*8,0);
				}
				if (tf->tf_Border[2])
				{
					SetHighColor(rp,tf->tf_BorderColor[2]);
					DrawHorizBlock(rp,page->pg_DPI,x+w-1,y+h-1,x,tf->tf_Border[2]*8,0);
				}
				if (tf->tf_Border[3])
				{
					SetHighColor(rp,tf->tf_BorderColor[3]);
					DrawHorizBlock(rp,page->pg_DPI,x,y,x+w-1,tf->tf_Border[3]*8,0);
				}
				/*if (pd->pd_Rasta & PDR_CELLWIDTH)
					DrawRastaCellWidth(irp,x,y,!tf->tf_Width ? w : GetTotalWidth(page,tf),h);*/

				tf = NextTableField(tf);
			}
		}
		freeLayerClip(rp->Layer);

		tf = GetRealTableField(page, tabLeft, tabTop);
		for(x = rect.MinX+tabXOff,i = tabLeft;(i <= page->pg_Cols) && (x <= rect.MaxX);x += GetTFWidth(page,i++))
		{
			if (!tf)
				tf = GetTableField(page, i, tabTop);
			else while(tf && (tf->tf_Col + tf->tf_Width < i && tf->tf_Row == tabTop || tf->tf_Row < tabTop))
				tf = NextTableField(tf);

			while (tf && tf->tf_Col <= i && tf->tf_Col+tf->tf_Width >= i && tf->tf_Row == tabTop)
			{
				if (tf->tf_Col == i)
					DrawTFText(rp, page, &rect, tf, x, y);
				else
				{
					for(w = x,border = i;border > tf->tf_Col;w -= GetTFWidth(page,--border));
					DrawTFText(rp, page, &rect, tf, w, y);
				}
				tf = NextTableField(tf);
			}
		}
	}

	makeLayerClip(rp->Layer, rect.MinX, rect.MinY, rect.MaxX - rect.MinX, rect.MaxY - rect.MinY);
	SetGRastPort(rp);

	for (i = 0; i < page->pg_NumObjects; i++)
	{
		struct gObject *go;

		if (!(go = page->pg_ObjectOrder[i]))
			continue;

		if ((x = go->go_Left) <= rect.MaxX + xmin && x + go->go_Right - go->go_Left >= rect.MinX + xmin &&
				(y = go->go_Top) <= rect.MaxY + ymin && y + go->go_Bottom - go->go_Top >= rect.MinY + ymin) {			
			// object is visible
#ifdef __amigaos4__
			if(!PrintGObject(grp, page, go, x - xmin, y - ymin, frame))
			{
				freeLayerClip(rp->Layer);
				return;
			}
#else
			DrawGObject(grp, page, go, x- xmin, y - ymin);
#endif
		}
	}
	freeLayerClip(rp->Layer);
}

BOOL PrintGObject(struct RastPort *rp, struct Page *page, struct gObject *go, long x, long y, struct Rect32 *rect)
{
    struct gClass *gc = go->go_Class;
	struct gBounds gb;
	struct BitMap *bm;
	struct BitScaleArgs bsa;
	struct gPicture *gp = GINST_DATA(gc,go);
	long wid = (go->go_Right - go->go_Left  - GOKNOB_SIZE) - GOKNOB_SIZE;
	long hei = (go->go_Bottom - go->go_Top  - GOKNOB_SIZE) - GOKNOB_SIZE;
	struct RastPort *tmp_rp;
	struct BitMap *tmp_bm;
	struct Layer_Info *tmp_li;

	if(gp->gp_Image &&  !Strnicmp(gc->gc_ClassName, "picture", 7))
	{
		if (GetAttr(PIA_BitMap, (Object *)gp->gp_Image, (IPTR *)&bm))
		{
		    ClearMem(&bsa, sizeof(struct BitScaleArgs));
			bsa.bsa_SrcX 		= (rect->MinX - go->go_Left < 0 ? 0 : (long)((rect->MinX - go->go_Left) * ((double)gp->gp_Width / (double)(go->go_Right - go->go_Left)))); 
			bsa.bsa_SrcY 		= (rect->MinY - go->go_Top < 0 ? 0 : (long)((rect->MinY - go->go_Top) * ((double)gp->gp_Height / (double)(go->go_Bottom - go->go_Top))));
			bsa.bsa_SrcWidth 	= ((wid + go->go_Left) > rect->MaxX ? (rect->MaxX - go->go_Left) * ((double)gp->gp_Width / (go->go_Right - go->go_Left)) : gp->gp_Width - bsa.bsa_SrcX);
			bsa.bsa_SrcHeight 	= (long)(STRIPE * ((double)gp->gp_Height / (go->go_Bottom - go->go_Top)));

			bsa.bsa_DestX 		= (x > 0 ? x : 0);
			bsa.bsa_DestY 		= (y > 0 ? y : 0);
			bsa.bsa_XSrcFactor 	= gp->gp_Width;  
			bsa.bsa_YSrcFactor 	= (long)(STRIPE * ((double)gp->gp_Height / (go->go_Bottom - go->go_Top))); 
			bsa.bsa_XDestFactor = wid;
			bsa.bsa_YDestFactor = STRIPE;
			bsa.bsa_SrcBitMap = bm;
			bsa.bsa_DestBitMap 	= rp->BitMap;
			BitMapScale(&bsa);
			if(((bsa.bsa_DestX & 0xf) + bsa.bsa_DestWidth) >= 1008) //Forbidden values for BitMapScale
				return FALSE;
		}
	}
	else
	{  
		if (gc->gc_Draw == NULL)
			return FALSE;
		DrawGObject(grp, page, go, x, y);
	}
	return TRUE;
}

void
FreeWDTPrinter(struct List *list)
{
	struct wdtPrinter *wp;

	if (!list)
		return;

	while ((wp = (struct wdtPrinter *)MyRemHead(list)) != 0)
	{
		FreeString(wp->wp_Node.ln_Name);
		FreeString(wp->wp_Range);

		FreePooled(pool, wp, sizeof(struct wdtPrinter));
	}
	FreePooled(pool, list, sizeof(struct List));
}

#define PM_HORIZONTAL 1
#define PM_VERTICAL 2

struct PrintInfo
{
	UBYTE pi_PageMode;
	UBYTE pi_Mode;
	LONG  pi_BeginX,pi_BeginY;
	LONG  pi_Width,pi_Height;
	LONG  pi_Stripe;
	LONG  pi_Right,pi_Bottom;
};


BOOL
NextPage(struct PrintInfo *pi, struct Rect32 *rect)
{
	UBYTE pm = pi->pi_Mode;

	if (pi->pi_PageMode == PRTPM_FIRST)
		return FALSE;

	if (pm & PM_HORIZONTAL)
	{
		if (pm == PM_HORIZONTAL && rect->MaxX >= pi->pi_Right)
			return(FALSE);

		rect->MinX = rect->MaxX+1;  rect->MaxX += pi->pi_Width-1;
		rect->MinY = pi->pi_BeginY;  rect->MaxY = pi->pi_BeginY+pi->pi_Stripe-1;
	}
	if (pm & PM_VERTICAL && !(pm & PM_HORIZONTAL && rect->MinX < pi->pi_Right))
	{
		rect->MinX = pi->pi_BeginX;  rect->MaxX = pi->pi_BeginX+pi->pi_Width-1;
		rect->MinY = pi->pi_BeginY+pi->pi_Height-1;  rect->MaxY = rect->MinY+pi->pi_Stripe-1;
	}

	if (rect->MinY >= pi->pi_Bottom || rect->MinX >= pi->pi_Right)
		return FALSE;

	return TRUE;
}


BYTE
PrintPage(union printerIO *pio,struct PrinterExtendedData *ped,struct wdtPrintStatus *wps,struct RastPort *rp,long wid,long hei,long stripe,struct Page *page,struct wdtPrinter *wp)
{
	long   tabx,taby,wtabx,wtaby;
	long   oldzoom,index = 1,count;
	struct PrintInfo pi;
	struct Rect32 rect;
	BYTE   error = 0,go;
	char   t[32];
#ifdef __amigaos4__
	PLANEPTR atemp;
	long i;
#endif
	/*** Initialize PrintInfo structure ***/
	pi.pi_PageMode = wp->wp_PageMode;

	if (pi.pi_PageMode == PRTPM_ALL || pi.pi_PageMode == PRTPM_RANGE)
		pi.pi_Mode = PM_HORIZONTAL | PM_VERTICAL;
	else if (pi.pi_PageMode == PRTPM_HORIZONTAL)
		pi.pi_Mode = PM_HORIZONTAL;
	else if (pi.pi_PageMode == PRTPM_VERTICAL)
		pi.pi_Mode = PM_VERTICAL;
	else
		pi.pi_Mode = 0;

	tabx = page->pg_TabX;  taby = page->pg_TabY;			// alte Werte sichern
	wtabx = page->pg_wTabX;  wtaby = page->pg_wTabY;

	page->pg_TabX = rect.MinX;  page->pg_TabY = rect.MinY;  // neu initialisieren
	page->pg_wTabX = page->pg_wTabY = 0;

	/* MERKER: nur X dpi wird angepaßt, unterschiedliche X/Y dpi funktionieren nicht... */
	oldzoom = page->pg_Zoom;			  
#ifdef __amigaos4__
	SetZoom(page, (long)(wp->wp_Zoom * PDatas.XDPI / (gDPI >> 16)), FALSE, FALSE);
#else
	SetZoom(page, (long)(wp->wp_Zoom * ped->ped_XDotsInch / (gDPI >> 16)), FALSE, FALSE);
#endif

	if (pi.pi_PageMode == PRTPM_RANGE)
	{
		setTableCoord(page, &rect, wp->wp_TablePos.tp_Col, wp->wp_TablePos.tp_Row, wp->wp_TablePos.tp_Width, wp->wp_TablePos.tp_Height);
		pi.pi_Right = rect.MaxX;  pi.pi_Bottom = rect.MaxY;
	}
	else
	{
		rect.MinX = rect.MinY = 0;
		pi.pi_Right = page->pg_TabW - 1;  pi.pi_Bottom = page->pg_TabH - 1;
	}

	pi.pi_Width = wid; pi.pi_Height = hei;  pi.pi_Stripe = stripe;
	rect.MaxX = rect.MinX + wid - 1;  rect.MaxY = rect.MinY + stripe - 1;

	pi.pi_BeginX = rect.MinX;

	/*** Calculate number of pages to print ***/

	{
		long horiz = (pi.pi_Right - rect.MinX + pi.pi_Width-1) / pi.pi_Width;
		long vert = (pi.pi_Bottom - rect.MinY + pi.pi_Height-1) / pi.pi_Height;

		switch (pi.pi_PageMode)
		{
			case PRTPM_FIRST:
				count = 1;
				break;
			case PRTPM_HORIZONTAL:
				count = horiz;
				break;
			case PRTPM_VERTICAL:
				count = vert;
				break;
			default:
				count = horiz * vert;
				break;
		}
	}
	for(;;index++)
	{
		if (rect.MaxX >= pi.pi_Right)
			rect.MaxX = pi.pi_Right;
		if (rect.MaxY >= pi.pi_Bottom)
			rect.MaxY = pi.pi_Bottom;

		pi.pi_BeginY = rect.MinY;
		go = TRUE;

		if (wps && wps->wps_PageBar)
		{
			sprintf(t, GetString(&gLocaleInfo, MSG_PAGE_X_OF_Y_PROGRESS),index,count);
			UpdateProgressBar(wps->wps_PageBar,t,(float)(1.0*index/count));
		}
#ifdef __amigaos4__
		pio->iodrp.io_Special |= SPECIAL_NOFORMFEED;
#else
		pio->iodrp.io_Special = SPECIAL_NOFORMFEED;
#endif

#ifdef __amigaos4__
		//Create top margin for non postscript-driver
		SetHighColor(rp, page->pg_BPen);
		SetBPen(rp, 0);  SetDrMd(rp, JAM2);
		RectFill(rp, 0, 0, ped->ped_MaxXDots, stripe);
		for(i = 0; i < PDatas.YOffset / stripe; i++)
			if ((error = DoIO((struct IORequest *)pio)) != 0)
				break;
		if(PDatas.YOffset % stripe)
		{
		    pio->iodrp.io_SrcHeight = PDatas.YOffset % stripe;
		    pio->iodrp.io_DestRows = PDatas.YOffset % stripe;
		    error = DoIO((struct IORequest *)pio);
		    pio->iodrp.io_SrcHeight = stripe - 2;
		    pio->iodrp.io_DestRows = stripe - 2;
		}
#endif
		for(;go && rect.MinY < pi.pi_BeginY + hei; rect.MinY = rect.MaxY - 1, rect.MaxY += stripe - 2)
		{
			char s[16];

			if (rect.MinY && pio->iodrp.io_SrcY == 0)
				pio->iodrp.io_SrcY = 1;

			if (rect.MaxY > pi.pi_BeginY+hei || rect.MaxY > pi.pi_Bottom)
			{
				rect.MaxY = min(pi.pi_BeginY+hei,pi.pi_Bottom);
				pio->iodrp.io_SrcHeight = pio->iodrp.io_DestRows = rect.MaxY - rect.MinY;
				go = FALSE;
			}
#ifndef __amigaos4__
			if (rect.MaxX > pi.pi_BeginX+wid)
			{
				pio->iodrp.io_SrcWidth = pio->iodrp.io_DestCols = rect.MaxX - rect.MinX;
			}
#endif
			page->pg_TabX = rect.MinX;  page->pg_TabY = rect.MinY;

			if (wps && wps->wps_SinglePageBar)
			{
				float percent = rect.MaxY - pi.pi_BeginY;

				if (pi.pi_BeginY+hei < pi.pi_Bottom)
					percent /= hei;
				else
					percent /= pi.pi_Bottom - pi.pi_BeginY;

				sprintf(s,"%ld",(int)(percent*100.0));
				UpdateProgressBar(wps->wps_SinglePageBar,s,percent);
#ifdef __amigaos4__
				if(GT_GetIMsg(wps->wps_Window->UserPort))
				    return 1;
#endif
			}

			D(bug("  printing lines: %ld-%ld\n",rect.MinY+pio->iodrp.io_SrcY,rect.MaxY+pio->iodrp.io_SrcY));
#ifdef __amigaos4__
			PrintTableRegion(pio, rp, page, &rect);
#else
			PrintTableRegion(rp, page, &rect);
#endif
			if (!go || rect.MaxY > pi.pi_BeginY+hei)  // page should be ejected
			{
				pio->iodrp.io_Special &= ~SPECIAL_NOFORMFEED;
			}

#ifdef __amigaos4__
			//Create left margin for non postscript-driver
			BltBitMap(rp->BitMap, 0, 0, rp->BitMap, PDatas.XOffset, 0, wid + PDatas.XOffset, stripe, 0x0C0, 0xff, atemp);
			SetHighColor(rp, page->pg_BPen);
			SetBPen(rp, 0);  SetDrMd(rp, JAM2);
			RectFill(rp, 0, 0, PDatas.XOffset, stripe);
#endif

			if ((error = DoIO((struct IORequest *)pio)) != 0)
				break;

			if (!rect.MinY)
				rect.MaxY--;
		}

		pio->iodrp.io_SrcY = 0;
#ifdef __amigaos4__
		if(PDatas.Type == POSTSCRIPT)
		{
#endif
			pio->iodrp.io_SrcWidth = pio->iodrp.io_DestCols = wid;
			pio->iodrp.io_SrcHeight = pio->iodrp.io_DestRows = stripe-2;
#ifdef __amigaos4__
		}
#endif
		if (!NextPage(&pi,&rect))
			break;
	}

	if (page->pg_Node.ln_Type != LNT_PRINTERPAGE)
	{
		SetZoom(page,oldzoom,FALSE,FALSE);
		page->pg_TabX = tabx;   page->pg_TabY = taby;
		page->pg_wTabX = wtabx; page->pg_wTabY = wtaby;
	}
	return error;
}


#ifdef __amigaos4__
void
ResumeTask(struct Task *task)
{
	RestartTask(task, 0);
}
#else
void
ResumeTask(struct Task *task)
{
	Disable();
	MyRemove(task);
	task->tc_State = TS_READY;
	MyAddHead(&SysBase->TaskReady,task);
	Enable();
}
#endif

#ifndef __amigaos4__
void
SuspendTask(struct Task *task)
{
	Disable();
	MyRemove(task);
	task->tc_State = 8;
	MyAddHead(&SysBase->IntrList,task);
	Enable();
}
#endif

static void
Print_SetHighTrueColor(struct RastPort *rp, ULONG color)
{
	struct PrintColorContext *context = (struct PrintColorContext *)rp->RP_User;
	ULONG red = (color >> 16) & 0xff;
	ULONG green = (color >> 8) & 0xff;
	ULONG blue = color & 0xff;

	SetRGB32(context->pc_ViewPort, context->pc_TruePen, RGB32(red), RGB32(green), RGB32(blue));
	SetAPen(rp, context->pc_TruePen);
}


static void
Print_SetLowTrueColor(struct RastPort *rp, ULONG color)
{
	struct PrintColorContext *context = (struct PrintColorContext *)rp->RP_User;
	ULONG red = (color >> 16) & 0xff;
	ULONG green = (color >> 8) & 0xff;
	ULONG blue = color & 0xff;

	SetRGB32(context->pc_ViewPort, context->pc_LowTruePen, RGB32(red), RGB32(green), RGB32(blue));
	SetBPen(rp, context->pc_LowTruePen);
}


static void
Print_SetOutlineTrueColor(struct RastPort *rp, ULONG color)
{
	struct PrintColorContext *context = (struct PrintColorContext *)rp->RP_User;
	ULONG red = (color >> 16) & 0xff;
	ULONG green = (color >> 8) & 0xff;
	ULONG blue = color & 0xff;

	SetRGB32(context->pc_ViewPort, context->pc_OutlineTruePen, RGB32(red), RGB32(green), RGB32(blue));
	SetOutlinePen(rp, context->pc_OutlineTruePen);
}


static void
Print_SetHighColor(struct RastPort *rp, ULONG color)
{
	struct PrintColorContext *context = (struct PrintColorContext *)rp->RP_User;
	ULONG red = (color >> 16) & 0xff;
	ULONG green = (color >> 8) & 0xff;
	ULONG blue = color & 0xff;
	
	//bug("high red = %lx, green = %lx, blue = %lx (color = %lx)\n", RGB32(red), RGB32(green), RGB32(blue), color);
	SetAPen(rp, /*blue = */FindColor(context->pc_ColorMap, RGB32(red), RGB32(green), RGB32(blue), -1));
	//bug("high pen = %ld\n", blue);
}


static void
Print_SetLowColor(struct RastPort *rp, ULONG color)
{
	struct PrintColorContext *context = (struct PrintColorContext *)rp->RP_User;
	ULONG red = (color >> 16) & 0xff;
	ULONG green = (color >> 8) & 0xff;
	ULONG blue = color & 0xff;

	SetBPen(rp, FindColor(context->pc_ColorMap, RGB32(red), RGB32(green), RGB32(blue), -1));
}


static void
Print_SetOutlineColor(struct RastPort *rp, ULONG color)
{
	struct PrintColorContext *context = (struct PrintColorContext *)rp->RP_User;
	ULONG red = (color >> 16) & 0xff;
	ULONG green = (color >> 8) & 0xff;
	ULONG blue = color & 0xff;

	SetOutlinePen(rp, FindColor(context->pc_ColorMap, RGB32(red), RGB32(green), RGB32(blue), -1));
}


static void
Print_InitColor(struct PrintColorContext *context, struct PrintScreenContext *screenContext)
{
	struct colorPen *cp;

	context->pc.cc_TrueColor = true;

	context->pc_ColorMap = screenContext->ps_ColorMap;
	context->pc_ViewPort = screenContext->ps_ViewPort;

	if (context->pc.cc_TrueColor) {
		context->pc.cc_SetHighColor = Print_SetHighTrueColor;
		context->pc.cc_SetLowColor = Print_SetLowTrueColor;
		context->pc.cc_SetOutlineColor = Print_SetOutlineTrueColor;

		context->pc_TruePen = ObtainPen(context->pc_ColorMap, -1, 0L, 0L, 0L, PEN_EXCLUSIVE | PEN_NO_SETCOLOR);
		context->pc_LowTruePen = ObtainPen(context->pc_ColorMap, -1, 0L, 0L, 0L, PEN_EXCLUSIVE | PEN_NO_SETCOLOR);
		context->pc_OutlineTruePen = ObtainPen(context->pc_ColorMap, -1, 0L, 0L, 0L, PEN_EXCLUSIVE | PEN_NO_SETCOLOR);
	} else {
		context->pc.cc_SetHighColor = Print_SetHighColor;
		context->pc.cc_SetLowColor = Print_SetLowColor;
		context->pc.cc_SetOutlineColor = Print_SetOutlineColor;
	}

	foreach (&colors, cp) {
		ULONG r, g, b;
		r = cp->cp_Red;
		g = cp->cp_Green;
		b = cp->cp_Blue;
		r = r | (r << 8) | (r << 16) | (r << 24);
		g = g | (g << 8) | (g << 16) | (g << 24);
		b = b | (b << 8) | (b << 16) | (b << 24);
			
		// there is no need to save the allocated pen (because the context will be freed soon)
		/*cp->cp_Pen =*/ ObtainBestPen(context->pc_ColorMap, r, g, b, TAG_END);
	}
	
	// ToDo: allocate nice colors (all over the rainbow)
}


bool
OpenPrintScreen(struct PrintScreenContext *context, struct Mappe *map, struct RastPort *rp, struct PrintColorContext *colorContext)
{
	bool trueColor, needScreen;
	int32 depth;

	// Print in true color mode?
	trueColor = CyberGfxBase != NULL;
	needScreen = false;
	memset(context, 0, sizeof(struct PrintScreenContext));
	InitRastPort(rp);

	depth =	GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH);
	if (trueColor && depth != 24 || !trueColor && depth != 8) {
		struct Page *page;
		struct gObject *go;

		// we may need our own screen, if there are any pictures in this map
		// as you may have noticed, this is a hack!
		
		foreach (&map->mp_Pages, page) {
			foreach (&page->pg_gObjects, go) {
				if (gIsSubclassFrom(go->go_Class, FindGClass("picture"))) {
					needScreen = true;
					break;
				}
			}
		}
	}

//bug("printing needs own screen? %s\n", needScreen ? "yes" : "no");
needScreen = true;

	if (needScreen) {
#ifdef __amigaos4__
		ULONG dispid;

		dispid = GetVPModeID(&scr->ViewPort);
#endif
		context->ps_Screen = OpenScreenTags(NULL,
					SA_Title,			"ignition.print",
					SA_LikeWorkbench,	TRUE,
#ifdef __amigaos4__
                    SA_DisplayID,     dispid,
#endif
#ifndef __amigaos4__
					SA_Interleaved,		TRUE,
					SA_Colors32,		standardPalette,
#endif
#ifdef __amigaos4__
					SA_Depth,			trueColor ? 32 : 8,
#else
					SA_Depth,			trueColor ? 24 : 8,
#endif
					SA_Draggable,		FALSE,
					SA_Parent,			scr,
#ifdef __amigaos4__
					SA_Height,			STRIPE,
#else
					SA_Height,			42,
#endif
					SA_ShowTitle,		FALSE,
					SA_Behind,			TRUE,
					SA_Quiet,			TRUE,
					SA_Type,			CUSTOMSCREEN,
					//SA_Exclusive,		  TRUE,
					TAG_END);
		if (context->ps_Screen == NULL)
			return false;
			
		context->ps_ViewPort = &context->ps_Screen->ViewPort;
		context->ps_ColorMap = context->ps_Screen->ViewPort.ColorMap;

		rp->BitMap = context->ps_Screen->RastPort.BitMap;
	} else {
		struct ViewPort *viewPort;
		
		viewPort = AllocPooled(pool, sizeof(struct ViewPort));
		if (viewPort == NULL)
			return false;

		context->ps_ColorMap = GetColorMap(256);
		if (context->ps_ColorMap == NULL) {
			FreePooled(pool, viewPort, sizeof(struct ViewPort));
			return false;
		}
		
		// initialize ViewPort
		context->ps_ViewPort = &scr->ViewPort;
		viewPort->DWidth = scr->ViewPort.DWidth;
		viewPort->DHeight = scr->ViewPort.DHeight;
		viewPort->DxOffset = scr->ViewPort.DxOffset;
		viewPort->DyOffset = scr->ViewPort.DyOffset;
		viewPort->Modes = scr->ViewPort.Modes;
		viewPort->SpritePriorities = scr->ViewPort.SpritePriorities;
		viewPort->ExtendedModes = scr->ViewPort.ExtendedModes;
   
		viewPort->ColorMap = context->ps_ColorMap;

		AttachPalExtra(context->ps_ColorMap, viewPort);
		
		rp->BitMap = scr->RastPort.BitMap;
			// ToDo: this is a hack
	}

	Print_InitColor(colorContext, context);
	rp->RP_User = (void *)colorContext;
	
	return true;
}

 
void
ClosePrintScreen(struct PrintScreenContext *context)
{
    
	if (context->ps_Screen) {
		// we had a screen
		CloseScreen(context->ps_Screen);
	} else {
		// we didn't have a screen

		if (context->ps_ColorMap)
			FreeColorMap(context->ps_ColorMap);
	}
}

 
void
Print(struct List *list, struct wdtPrinter *wp, WORD unit, struct wdtPrintStatus *wps)
{
	struct Mappe *mp = GetPrintListMap(list);
	struct PrintScreenContext screenContext;
	struct PrintColorContext colorContext;
	struct PrinterExtendedData *ped;
#ifdef __amigaos4__
	struct Preferences *prefs;	//Printer Prefs
	BOOL stop = FALSE;
#endif
	long   width, height, stripe;
	struct PrinterData *pd;
	union  printerIO *pio;
	struct Layer_Info *li;
	struct MsgPort *port;
	struct RastPort *rp;
	struct BitMap *bm;

#ifdef __amigaos4__
	oldfunc1 = (APTR)1; //Set marker of starting printing
	if (!(port = AllocSysObjectTags(ASOT_PORT, TAG_END)))
#else
	if (!(port = CreateMsgPort()))
#endif
		return;
 
	rp = AllocPooled(pool, sizeof(struct RastPort));
	if (rp == NULL) {
#ifdef __amigaos4__
		FreeSysObject(ASOT_PORT, port);
#else
		DeleteMsgPort(port);
#endif
		return;
	}
#ifdef __amigaos4__
	GetPrinterPrefs(mp, INIT);
	GetPrinterPrefs(mp, POSTSCRIPT);
	SetPrinterPrefs(mp, PDatas.Type);
	Delay(50); //Wait for writing new prefs
#endif
	if (!OpenPrintScreen(&screenContext, mp, rp, &colorContext)) {
#ifdef __amigaos4__
		FreeSysObject(ASOT_PORT, port);
#else
		DeleteMsgPort(port);
#endif
		FreePooled(pool, rp, sizeof(struct RastPort));
		return;
	}
#ifdef __amigaos4__
	if ((pio = (union printerIO *)AllocSysObjectTags(ASOT_IOREQUEST, ASOIOR_ReplyPort, port, ASOIOR_Size, sizeof(union printerIO), TAG_END)) != 0)
#else
	if ((pio = (union printerIO *)CreateExtIO(port, sizeof(union printerIO))) != 0)
#endif
	{ 
		if (!OpenDevice("printer.device", unit, (struct IORequest *)pio, 0))
		{
			pd = (struct PrinterData *)pio->iodrp.io_Device;
			ped = (struct PrinterExtendedData *)&pd->pd_SegmentData->ps_PED;

			/* MERKER to be implemented: printer-prefs übernehmen */
			pio->iodrp.io_Command = PRD_DUMPRPORT;
			pio->iodrp.io_RastPort = rp;
			pio->iodrp.io_ColorMap = screenContext.ps_ColorMap;
			pio->iodrp.io_Modes = GetVPModeID(screenContext.ps_ViewPort);
			pio->iodrp.io_SrcX = 0;  pio->iodrp.io_SrcY = 0;
			pio->iodrp.io_SrcWidth = 10;  pio->iodrp.io_SrcHeight = 10;
			pio->iodrp.io_DestCols = 0;  pio->iodrp.io_DestRows = 0;
			pio->iodrp.io_Special = SPECIAL_NOPRINT;
			DoIO((struct IORequest *)pio);
	
			D(bug("destcols: %ld\n", pio->iodrp.io_DestCols));
			D(bug("destrows: %ld\n", pio->iodrp.io_DestRows));
			D(bug("numrows: %ld\n", ped->ped_NumRows));
			D(bug("maxX = %ld, maxY = %ld\n", ped->ped_MaxXDots, ped->ped_MaxYDots));
#ifdef __amigaos4__
			if(PDatas.Type != POSTSCRIPT)
			{
				PDatas.XDPI = (uint16)(ped->ped_MaxXDots / (mp->mp_mmWidth / 1024.0 / 25.4)) / 10 * 10; //Correction, beacause some driver lie and deliver wrong values in XDotsInch
				PDatas.YDPI = ped->ped_MaxYDots / (mp->mp_mmHeight / 1024.0 / 25.4);
			    PDatas.XOffset	= (uint16)(((mp->mp_BorderLeft / 10240.0) / 2.54) * (double)PDatas.XDPI);
			    PDatas.YOffset	= (uint16)(((mp->mp_BorderTop  / 10240.0) / 2.54) * (double)PDatas.YDPI);
			}
			width 	= ped->ped_MaxXDots - (int32)(((double)(mp->mp_BorderRight +  mp->mp_BorderLeft) / (1024.0 * 25.4)) * (double)PDatas.XDPI);
			height 	= ped->ped_MaxYDots - (int32)((double)(((mp->mp_BorderTop +  mp->mp_BorderBottom) *  PDatas.YDPI)) / (1024.0 * 25.4));
			stripe = STRIPE;
			pio->iodrp.io_SrcWidth = ped->ped_MaxXDots;  
			pio->iodrp.io_SrcHeight = stripe - 2;
			pio->iodrp.io_DestCols = ped->ped_MaxXDots;
			pio->iodrp.io_DestRows = stripe;
			pio->iodrp.io_Special = SPECIAL_NOFORMFEED;
#else
			width = /*ped->ped_MaxXDots;*/ pio->iodrp.io_DestCols;
			height = ped->ped_MaxYDots;//pio->iodrp.io_DestRows;  /* MERKER: vorerst, der Wert ist nicht korrekt... */
			stripe = 42;
			pio->iodrp.io_SrcWidth = width;  pio->iodrp.io_SrcHeight = stripe - 2;
			pio->iodrp.io_DestCols = width;/*stripe;*/
			pio->iodrp.io_DestRows = pio->iodrp.io_SrcHeight; /*stripe;*/
			pio->iodrp.io_Special = SPECIAL_NOFORMFEED|SPECIAL_ASPECT;
#endif

#ifdef __amigaos4__
			if (!pio->iodrp.io_Error && (bm = AllocBitMap(ped->ped_MaxXDots, stripe, GetBitMapAttr(rp->BitMap, BMA_DEPTH), BMF_CLEAR | BMF_MINPLANES, rp->BitMap)))
#else
			if (!pio->iodrp.io_Error && (bm = AllocBitMap(width, stripe, GetBitMapAttr(rp->BitMap, BMA_DEPTH), BMF_CLEAR | BMF_MINPLANES, rp->BitMap)))
#endif
			{
				rp->BitMap = bm;
				if ((li = NewLayerInfo()) != 0)
				{
#ifdef __amigaos4__
					if ((rp->Layer = CreateBehindLayer(li, rp->BitMap, 0, 0, ped->ped_MaxXDots - 1, stripe - 1, LAYERSIMPLE, NULL)) != 0)
#else
					if ((rp->Layer = CreateBehindLayer(li, rp->BitMap, 0, 0, width - 1, stripe - 1, LAYERSIMPLE, NULL)) != 0)
#endif
					{
						struct wdtPrinter *swp;
						LONG   index = 0,count = 0;
						UBYTE  error = 0;

#ifdef __amigaos4__
						while(GT_GetIMsg(wps->wps_Window->UserPort));	//Remove all messages
						gInitArea(-width, -height, width, height);
#endif

						/* Zu druckende Tabellen zählen */
						foreach (list, swp)
						{
							if (!swp->wp_Node.ln_Type)
								continue;

							count++;
						}
#ifdef __amigaos4__
						count *= wp->wp_Copies;
						if(pdf)	//if pdf-printing, then a "page" more for showing pdf-generation
							count++;
#endif
						// ToDo: do it right!
						if (screenContext.ps_Screen != NULL) {
							gRemoveObjectsFromScreen(mp, scr);
							gAddObjectsToScreen(mp, screenContext.ps_Screen);
						}

						/* Tabellen drucken */
#ifdef __amigaos4__
					while(wp->wp_Copies--)
					{
#endif
						foreach (list, swp)
						{
							struct wdtPrinter *realwp;
							char t[64];

							if (!swp->wp_Node.ln_Type)
								continue;

							if (wps && wps->wps_ProjectBar)
							{
#ifdef __amigaos4__
								Strlcpy(t, swp->wp_Page->pg_Node.ln_Name, 50);
#else
								stccpy(t, swp->wp_Page->pg_Node.ln_Name, 50);
#endif
								sprintf(t + strlen(t), " (%ld/%ld)", index + 1, count);
								UpdateProgressBar(wps->wps_ProjectBar, t, (float)(1.0 * (++index) / count));
#ifdef __amigaos4__
								if(GT_GetIMsg(wps->wps_Window->UserPort))
								{
								    pdf = FALSE;
								    error = 1;
								    break;
								}
#endif
							}

							realwp = wp ? wp : swp;

							if (realwp->wp_PageMode == PRTPM_RANGE
								&& !realwp->wp_TablePos.tp_Col && !realwp->wp_TablePos.tp_Row
								&& !realwp->wp_TablePos.tp_Width && !realwp->wp_TablePos.tp_Height)
							{
								ErrorRequest(GetString(&gLocaleInfo, MSG_NO_VALID_RANGE_ERR));
								continue;
							}
#ifdef __amigaos4__
    						if(pdf && oldfunc1 == (APTR)1) //pdf-print and first page!
    						{
    							aslreq = AllocAslRequestTags(ASL_FileRequest, ASLFR_InitialDrawer, "ram:", ASLFR_InitialFile, "Print.ps", TAG_DONE); //set request with spezial parameter
        						Disable();
        						oldfunc1 =  SetMethod((struct Interface *)IAsl, offsetof(struct AslIFace, AllocAslRequest), AslReqAlloc); //redirect asl functions
        						oldfunc2 =  SetMethod((struct Interface *)IAsl, offsetof(struct AslIFace, AslRequest), WriteFileData);
        						Enable();
							}
							if(error != 1)
							{
#endif
								if ((error = PrintPage(pio, ped, wps, rp, width, height, stripe, swp->wp_Page, realwp)) != 0)
								{
									break;
								}
#ifdef __amigaos4__
							}
     						if(pdf && oldfunc2 != NULL) //only for one page, to redirect functions as short as possible
    						{
        						Disable(); //redirect to original functions 
								SetMethod((struct Interface *)IAsl, offsetof(struct AslIFace, AllocAslRequest), oldfunc1);
								SetMethod((struct Interface *)IAsl, offsetof(struct AslIFace, AslRequest), oldfunc2);
    							Enable();
    							oldfunc1 = oldfunc2 = NULL; //reset function-poiter
							}
#endif
							Delay(100);
						}
#ifdef __amigaos4__
					}
						if(error == 1 && oldfunc2 != NULL)	//print canceled in page
   						{				//then do not forget to repair OS ;-)
       						Disable(); //redirect to original functions 
							SetMethod((struct Interface *)IAsl, offsetof(struct AslIFace, AllocAslRequest), oldfunc1);
							SetMethod((struct Interface *)IAsl, offsetof(struct AslIFace, AslRequest), oldfunc2);
   							Enable();
   							oldfunc1 = oldfunc2 = NULL; //reset function-poiter
						}
						
						if(pdf) //shown pdf gen.
						{
							UpdateProgressBar(wps->wps_ProjectBar, "PDF", (float)(1.0 * (++index) / count));
						}
#endif
						DeleteLayer(0, rp->Layer);
											
						// ToDo: do it right!
						if (screenContext.ps_Screen != NULL) {
							gRemoveObjectsFromScreen(mp, screenContext.ps_Screen);
							gAddObjectsToScreen(mp, scr);
						}

#ifdef __amigaos4__
						if (error > 1)
#else
						if (error)
#endif
							ErrorRequest(GetString(&gLocaleInfo, MSG_PRINTING_FAILED_ERR), (long)error);
					}
					DisposeLayerInfo(li);
				}
				FreeBitMap(bm);
			}
			CloseDevice((struct IORequest *)pio);
		}
		else
			ErrorRequest(GetString(&gLocaleInfo, MSG_OPEN_PRINTER_UNIT_ERR), (long)unit);

#ifdef __amigaos4__
		RestorePrinterPrefs(PDatas.Type);
		FreeSysObject(ASOT_IOREQUEST, pio);
#else
		DeleteExtIO((struct IORequest *)pio);
#endif
	}

	ClosePrintScreen(&screenContext);
	FreePooled(pool, rp, sizeof(struct RastPort));
#ifdef __amigaos4__
	FreeSysObject(ASOT_PORT, port);
    if(pdf) //create pdf
    {
		CreatePDFFile();
    }
    else
	    Delete("ram:Print.ps");
#else
	DeleteMsgPort(port);
#endif

	{
		BOOL suspended = FALSE;

		if (asp_maintask && FindTask(NULL) != asp_maintask)  // Memory-Pools sind nicht multithreading-fähig
		{
#ifdef __amigaos4__
			SuspendTask(asp_maintask, 0);
#else
			SuspendTask(asp_maintask);
#endif
			suspended = TRUE;
		}

		FreePrinterProject(list);
		FreeWDTPrinter(list);

		if (suspended)
			ResumeTask(asp_maintask);
	}
}


struct AsyncPrint {
	struct MinNode ap_Node;
	struct List *ap_PrintList;
	struct wdtPrinter *ap_PrintOptions;
	WORD   ap_Unit;
};


void
SetAsyncPrintStatusTitle(void)
{
	ObtainSemaphore(&gWindowSemaphore);

	if (asp_mp)
	{
		sprintf(asp_title,GetString(&gLocaleInfo, MSG_PRINT_PROGRESS_TITLE), asp_mp->mp_Node.ln_Name, asp_current, asp_count);
		if (asp_wps)
			SetWindowTitles(asp_wps->wps_Window, asp_title, (STRPTR)~0L);
	}

	ReleaseSemaphore(&gWindowSemaphore);
}


void SAVEDS
AsyncPrint(void)
{
	struct AsyncPrint *ap;
	struct List list;
	char   title[128];

	asp_printtask = FindTask(NULL);
	asp_wps = OpenPrintWindow();
	asp_current = 1;
	asp_title = title;
	MyNewList(&list);

	while ((ap = (struct AsyncPrint *)MyRemHead(&asp_queue)) || asp_adding)
	{
		if (ap)
		{
			struct wdtPrinter *wp;

			foreach(ap->ap_PrintList,wp)
			{
				if (wp->wp_Page)
				{
					asp_mp = wp->wp_Page->pg_Mappe;
					break;
				}
			}
			SetAsyncPrintStatusTitle();

			Print(ap->ap_PrintList, ap->ap_PrintOptions, ap->ap_Unit, asp_wps);
			asp_current++;

			MyAddTail(&list, ap);
		}
		else
			Delay(10);
	}
	ClosePrintWindow(asp_wps);

#ifdef __amigaos4__
	SuspendTask(asp_maintask, 0);
#else
	SuspendTask(asp_maintask);
#endif

	while((ap = (struct AsyncPrint *)MyRemHead(&list)))
		FreePooled(pool, ap, sizeof(struct AsyncPrint));

	ResumeTask(asp_maintask);

	asp_printtask = NULL;
}


void
PrintProject(struct List *list, struct wdtPrinter *wp, WORD unit, ULONG flags)
{
	STRPTR error = NULL;

	{
		struct Mappe *mp = GetPrintListMap(list);

		if (mp == NULL) {
			DisplayBeep(NULL);
			return;
		}
		if (mp->mp_mmWidth < 1024 || mp->mp_mmHeight < 1024) {
			ErrorRequest(GetString(&gLocaleInfo, MSG_INVALID_PAPER_SIZE_ERR));
			return;
		}
	}

	if (flags & MPPRTF_ASYNCHRON) {
		struct AsyncPrint *ap;

		asp_adding = TRUE;

		if ((ap = AllocPooled(pool, sizeof(struct AsyncPrint))) != 0) {
			if (CopyPrinterProject(list)) {
				ap->ap_PrintList = list;  // make printer queue entry
				ap->ap_PrintOptions = wp;
				ap->ap_Unit = unit;

				if (asp_printtask == NULL) {
					// es gibt noch keinen Print-Thread, also starten wir einen

					asp_maintask = FindTask(NULL);
					asp_adding = FALSE;

					MyNewList(&asp_queue);
					MyAddTail(&asp_queue, ap);
					asp_count = 1;

					if (CreateNewProcTags(NP_Entry, AsyncPrint, NP_StackSize, 6192, NP_Name, "ignition Printer Process", TAG_END))
						return;
					else
						error = GetString(&gLocaleInfo, MSG_CREATE_TASK_ERR);
				} else {
					// hängen wir den neuen Print-Job in die Queue

					MyAddTail(&asp_queue, ap);
					asp_count++;
					SetAsyncPrintStatusTitle();
					if (asp_wps)
						WindowToFront(asp_wps->wps_Window);

					asp_adding = FALSE;
					return;
				}
				FreePrinterProject(list);
			}
			else
				error = GetString(&gLocaleInfo, MSG_COPY_PROJECT_ERR);
		}
		else
			error = GetString(&gLocaleInfo, MSG_NO_MEMORY_IFFERR);

		asp_adding = FALSE;
		flags &= ~MPPRTF_ASYNCHRON;
		ErrorRequest(GetString(&gLocaleInfo, MSG_ASYNC_PRINT_FAILED_ERR), error);
	}

	SetBusy(TRUE, BT_APPLICATION);
	{
		struct wdtPrintStatus *wps = OpenPrintWindow();

		Print(list, wp, unit, wps);
		ClosePrintWindow(wps);
	}
	SetBusy(FALSE, BT_APPLICATION);
}


/************************************* Fenster-Erstellung *************************************/


void ASM
CreatePrinterGadgets(REG(a0, struct winData *wd))
{
	struct Mappe *mp = ((struct Page *)wd->wd_Data)->pg_Mappe;
	long   w,mxwidth;

#ifdef __amigaos4__
	PDatas.Unit = 0; //Dialog always starts with printer-unit 0
#endif
#ifdef __amigaos4__
	MakeShortCutString(wd->wd_ShortCuts, MSG_PRINT_UGAD, MSG_CANCEL_UGAD, TAG_END);
	wd->wd_ShortCutsID[0] = 16; //Set Gadgetnumber 
	wd->wd_ShortCutsID[1] = 18; //Set Gadgetnumber 
#endif
	gWidth = TLn(GetString(&gLocaleInfo, MSG_INVERT_GAD)) + TLn(GetString(&gLocaleInfo, MSG_ALL_GAD))
		+ TLn(GetString(&gLocaleInfo, MSG_PREFS_FOR_ALL_SHEETS_GAD)) + 142;
	gHeight = barheight + 20 * fontheight + 69 + bborder;

	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PRINT_TO_LABEL);
	ngad.ng_LeftEdge = lborder + 8 + TLn(ngad.ng_GadgetText);
	ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge - boxwidth - TLn(GetString(&gLocaleInfo, MSG_OPTIONS_UGAD)) - 16 - linelen;
	ngad.ng_Flags = PLACETEXT_LEFT;		 // 1
	gad = CreateGadget(TEXT_KIND, gad, &ngad, GTTX_Border, TRUE, GTTX_Text, wd->wd_ExtData[5], TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;					 // 2
	gad = CreatePopGadget(wd, gad, FALSE);

	ngad.ng_LeftEdge += boxwidth;
	ngad.ng_Width = gWidth - rborder - ngad.ng_LeftEdge;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_OPTIONS_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					 // 3
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GA_Disabled, TRUE, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = lborder + 8;
	ngad.ng_TopEdge += 2 * fontheight + 10;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_ALL_GAD)) + TLn(GetString(&gLocaleInfo, MSG_INVERT_GAD)) + 42;
	ngad.ng_Height = fontheight * 14 + 4;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;					 // 4
	gad = wd->wd_ExtData[0] = CreateGadget(LISTVIEW_KIND, gad, &ngad, GTLV_Labels, wd->wd_ExtData[1],
		GTLV_ShowSelected, NULL, GTLV_CallBack, &selectHook, TAG_END);

	ngad.ng_TopEdge += ngad.ng_Height;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_ALL_GAD)) + 21;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ALL_GAD);
	ngad.ng_GadgetID++;					 // 5
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_INVERT_GAD)) + 21;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_INVERT_GAD);
	ngad.ng_GadgetID++;					 // 6
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	w = ngad.ng_LeftEdge + ngad.ng_Width + 6;
	mxwidth = (boxwidth * fontheight) / (fontheight + 4);

	ngad.ng_LeftEdge = w;
	ngad.ng_TopEdge = barheight + 2 * fontheight + 13;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PREFS_FOR_ALL_SHEETS_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;					 // 7
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GTCB_Scaled, TRUE, GTCB_Checked, TRUE, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge += TLn(GetString(&gLocaleInfo, MSG_RANGE_LABEL)) + 22 + mxwidth;
	ngad.ng_TopEdge += 5 * fontheight + 30;
	ngad.ng_Width = gWidth - ngad.ng_LeftEdge - rborder - 8;
	ngad.ng_GadgetText = NULL;
	ngad.ng_GadgetID++;					 // 8
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, NULL, TAG_END);

	ngad.ng_LeftEdge = w + 4;
	ngad.ng_TopEdge -= 4 * fontheight + 13;
	ngad.ng_Width = mxwidth;
	ngad.ng_Height = fontheight;
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;					 // 9
	gad = CreateGadget(MX_KIND, gad, &ngad, GTMX_Labels, &sPrintLabels, GTMX_Active, 0,
		GTMX_Scaled, TRUE, GTMX_Spacing, 4, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = w + TLn(GetString(&gLocaleInfo, MSG_NUMBER_OF_COPIES_LABEL)) + 8;
	ngad.ng_TopEdge += 5 * fontheight + 33;
	ngad.ng_Width = gWidth - 8 - rborder - ngad.ng_LeftEdge;
	ngad.ng_Height = fontheight + 4;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_NUMBER_OF_COPIES_LABEL);
	ngad.ng_Flags = PLACETEXT_LEFT;
	ngad.ng_GadgetID++;					 // 10
	gad = CreateGadget(INTEGER_KIND, gad, &ngad, GTIN_Number, 1, STRINGA_Justification, GTJ_RIGHT, TAG_END);

	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_Width -= boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_ZOOM_LABEL);
	ngad.ng_GadgetID++;					 // 11
	gad = CreateGadget(STRING_KIND, gad, &ngad, GTST_String, "100%", TAG_END);

	ngad.ng_LeftEdge += ngad.ng_Width;
	ngad.ng_GadgetID++;					 // 12
	gad = CreatePopGadget(wd, gad, FALSE);

	ngad.ng_LeftEdge = lborder + 8;
	ngad.ng_TopEdge = gHeight - 3 * fontheight - 26 - bborder;
	ngad.ng_Width = boxwidth;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PAGE_NUMBERS_GAD);
	ngad.ng_Flags = PLACETEXT_RIGHT;
	ngad.ng_GadgetID++;					 // 13
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GA_Disabled, TRUE, GTCB_Scaled, TRUE,
		GTCB_Checked, mp->mp_PrinterFlags & MPPRTF_PAGENUMBERS, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = gWidth / 2;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_FILE_NAME_GAD);
	ngad.ng_GadgetID++;					 // 14
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad, GA_Disabled, TRUE, GTCB_Scaled, TRUE,
		GTCB_Checked, mp->mp_PrinterFlags & MPPRTF_NAME, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = lborder + 8;
	ngad.ng_TopEdge += fontheight + 7;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PRINT_ASYNC_GAD);
	ngad.ng_GadgetID++;					 // 15
	gad = CreateGadget(CHECKBOX_KIND, gad, &ngad,
			GTCB_Scaled,	TRUE,
// ToDo: enable it again!
			GTCB_Checked,	false, //mp->mp_PrinterFlags & MPPRTF_ASYNCHRON,
			GT_Underscore,	'_',
			GA_Disabled,	true,
			TAG_END);

	ngad.ng_LeftEdge = lborder;
	ngad.ng_TopEdge = gHeight - fontheight - 7 - bborder;
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_PREFERENCES_GAD)) + 24;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PRINT_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID++;					 // 16
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = (gWidth - ngad.ng_Width) / 2;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_PREFERENCES_GAD);
	ngad.ng_GadgetID++;					 // 17
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, /*GA_Disabled, TRUE,*/ GT_Underscore, '_', TAG_END);

	ngad.ng_LeftEdge = gWidth - rborder - ngad.ng_Width;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_GadgetID++;					 // 18
	gad = CreateGadget(BUTTON_KIND, gad, &ngad, GT_Underscore, '_', TAG_END);
}


void ASM
CreatePrintStatusGadgets(REG(a0, struct winData *wd))
{
	gWidth = scr->Width / 3;
	if (gWidth < 300)
		gWidth = 300;

	gHeight = barheight + 6*fontheight + 40 + bborder;

	//wd->wd_ShortCuts = "a";
	MakeShortCutString(wd->wd_ShortCuts, MSG_CANCEL_UGAD, TAG_END);
	ngad.ng_Width = TLn(GetString(&gLocaleInfo, MSG_CANCEL_GAD))+24;
	ngad.ng_LeftEdge = (gWidth - ngad.ng_Width) / 2;
	ngad.ng_TopEdge = gHeight - fontheight - 7 - bborder;
	ngad.ng_GadgetText = GetString(&gLocaleInfo, MSG_CANCEL_UGAD);
	ngad.ng_Flags = PLACETEXT_IN;
	ngad.ng_GadgetID = 1;				   // 1
	gad = CreateGadget(BUTTON_KIND,gad,&ngad,GT_Underscore,'_',TAG_END);
}


/************************************* Fenster-Handling *************************************/


void
MakePrinterList(struct List *list)
{
	char filename[30];
	char name[UNITNAMESIZE];
	int i;

	MyNewList(list);
	strcpy(filename, "env:sys/printer.prefs");

	for (i = 0; i < 10; i++)
	{
		if (i == 1)
			strins(filename + 15, "1");
		else if (i > 1)
			filename[15] = i + '0';

		if (ReadUnitName(filename, name, i)) {
			struct Node *ln;

			if ((ln = AllocPooled(pool, sizeof(struct Node))) != 0) {
				ln->ln_Name = AllocString(name);
				MyAddTail(list, ln);
			}
		}
	}
}


void
UpdatePrinterGadgets(struct Window *win, struct winData *wd)
{
	struct wdtPrinter *wp;

	if ((wp = (APTR)wd->wd_ExtData[2]) != 0)
	{
		GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Selected, FindListEntry(wd->wd_ExtData[1], (struct MinNode *)wp), TAG_END);

		GT_SetGadgetAttrs(GadgetAddress(win, 8), win, NULL, GTST_String, wp->wp_Range,
			GA_Disabled, wp->wp_PageMode == PRTPM_RANGE ? FALSE : TRUE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 9), win, NULL, GTMX_Active, wp->wp_PageMode, GA_Disabled, FALSE, TAG_END);
		GT_SetGadgetAttrs(GadgetAddress(win, 10), win, NULL, GTIN_Number, wp->wp_Copies, GA_Disabled, FALSE, TAG_END);
		{
			char t[32];

			ProcentToString(wp->wp_Zoom, t);
			GT_SetGadgetAttrs(GadgetAddress(win, 11), win, NULL, GTST_String, t, GA_Disabled, FALSE, TAG_END);
		}
		SetGadgetAttrs(GadgetAddress(win, 12), win, NULL, GA_Disabled, FALSE, TAG_END);
	}
	else
	{
		int i;

		for (i = 8; i < 13; i++)
			DisableGadget(GadgetAddress(win, i), win, TRUE);
	}
}


void ASM
ClosePrinterWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;

	if (clean)
	{
		FreeWDTPrinter(wd->wd_ExtData[1]);
		FreeString((STRPTR)wd->wd_ExtData[5]);
	}
}


void ASM
HandlePrinterIDCMP(REG(a0, struct TagItem *tag))
{
#ifdef __amigaos4__
	struct wdtPrinter *wp = (struct wdtPrinter *)wd->wd_ExtData[2];
#else
	struct wdtPrinter *wp;//workaround to prevent crash with window close gadegt
	
    if(imsg.Class != 512)
    	wp = (struct wdtPrinter *)wd->wd_ExtData[2];
#endif
	
if(imsg.Class != 4194304 && imsg.Class != 512) printf("Class: %ld ID: %ld %ld\n", imsg.Class, (gad = imsg.IAddress)->GadgetID, wd->wd_ExtData);
	switch (imsg.Class)
	{
		case IDCMP_GADGETDOWN:
			switch ((gad = imsg.IAddress)->GadgetID)
			{
				case 2:   // Drucker-Liste
				{
					struct List list;
					struct Node *ln;
					long   i;

					/*** Druckerliste aufbauen ***/

					FreeString((STRPTR)wd->wd_ExtData[5]);  wd->wd_ExtData[5] = NULL;
					MakePrinterList(&list);

					/*** Liste anzeigen und auswählen ***/

					i = PopUpList(win, gad = GadgetAddress(win, 1), (struct MinList *)&list, TAG_END);
					if (i != ~0L)
					{
						ln = FindListNumber((struct MinList *)&list,i);
						wd->wd_ExtData[5] = ln->ln_Name;  ln->ln_Name = NULL;
						wd->wd_ExtData[4] = (APTR)i;
#ifdef __amigaos4__
						PDatas.Unit = i; //Save printer-unit 
						if(Strnicmp("PDF", wd->wd_ExtData[5], 3))
							pdf = FALSE;
						else
							pdf = TRUE;
#endif
						GT_SetGadgetAttrs(gad, win, NULL, GTTX_Text, wd->wd_ExtData[5], TAG_END);
					}
					/*** Liste freigeben ***/

					while ((ln = MyRemHead(&list)) != 0) {
						FreeString(ln->ln_Name);
						FreePooled(pool, ln, sizeof(struct Node));
					}
					break;
				}
				case 9:   // Seitenmodus
					if (wp)
					{
						wp->wp_PageMode = imsg.Code;
						GT_SetGadgetAttrs(GadgetAddress(win, 8), win, NULL, GA_Disabled, imsg.Code == PRTPM_RANGE ? FALSE : TRUE, TAG_END);
					}
					break;
				case 12:  // Vergrößerungs PopUp
					if (wp)
					{
						struct MinNode *tail = zooms.mlh_TailPred, *tailPred = zooms.mlh_TailPred->mln_Pred;
						long   i;

						// remove "-" & "Overview" from zooms list for display
						MyRemove(tailPred);
						MyRemove(tail);

						i = PopUpList(win,gad = GadgetAddress(win, 11), &zooms, TAG_END);
						if (i != ~0L)
						{
							struct Node *ln = FindListNumber(&zooms, i);
							char   t[32];

							ProcentToString(wp->wp_Zoom = (atol(ln->ln_Name) << 10) / 100, t);
							GT_SetGadgetAttrs(gad, win, NULL, GTST_String, t, TAG_END);
						}

						MyAddTail(&zooms, tailPred);
						MyAddTail(&zooms, tail);
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
				case 4:
					wp = (struct wdtPrinter *)((struct Gadget *)wd->wd_ExtData[0])->UserData;

					if (wp && imsg.Code == FindListEntry(wd->wd_ExtData[1], (struct MinNode *)wp))
					{
						GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, ~0L, TAG_END);
						wp->wp_Node.ln_Type = (!wp->wp_Node.ln_Type) & 1;
						GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, wd->wd_ExtData[1], TAG_END);
					}
					else
					{
						long i;

						for(i = 0,wp = (APTR)((struct List *)wd->wd_ExtData[1])->lh_Head;i < imsg.Code && wp->wp_Node.ln_Succ;i++,wp = (APTR)wp->wp_Node.ln_Succ);
						((struct Gadget *)wd->wd_ExtData[0])->UserData = wp;  // aktuellen in der Liste setzen

						if (wd->wd_ExtData[3] == NULL)  // falls unterschiedliche Einstellungen pro Tabelle
						{
							wd->wd_ExtData[2] = wp;
							UpdatePrinterGadgets(win, wd);
						}
					}
					break;
				case 5:  // Alle
				case 6:  // Umkehren
					GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, ~0L, TAG_END);
					for (wp = (APTR)((struct List *)wd->wd_ExtData[1])->lh_Head; wp->wp_Node.ln_Succ; wp = (APTR)wp->wp_Node.ln_Succ)
					{
						if (gad->GadgetID == 5)
							wp->wp_Node.ln_Type = 1;
						else
							wp->wp_Node.ln_Type = (!wp->wp_Node.ln_Type) & 1;
					}
					GT_SetGadgetAttrs(wd->wd_ExtData[0], win, NULL, GTLV_Labels, wd->wd_ExtData[1], TAG_END);
					break;
				case 7:   // Einstellungen gelten für alle Tabellen
					wd->wd_ExtData[3] = (APTR)((long)imsg.Code);
					if (!imsg.Code)
						UpdatePrinterGadgets(win, wd);
					break;
				case 8:   // Bereich
					if (wp)
					{
						STRPTR t;

						if (GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END))
						{
							struct Term *term;

							if ((term = CreateTree(wp->wp_Page, t)) && FillTablePos(&wp->wp_TablePos, term))
							{
								FreeString(wp->wp_Range);
								wp->wp_Range = AllocString(t);
							}
							else
							{
								ErrorRequest(GetString(&gLocaleInfo, MSG_INVALID_CELL_RANGE_ERR));
								GT_SetGadgetAttrs(gad, win, NULL, GTST_String, wp->wp_Range, TAG_END);
							}
						}
					}
					break;
				case 10:   // Kopienanzahl
					if (wp)
					{
						LONG num;

						if (GT_GetGadgetAttrs(gad, win, NULL, GTIN_Number, &num, TAG_END))
							wp->wp_Copies = num;
					}
					break;
#ifdef __amigaos4__
				case 11:  // Vergrößerung
#else
				case 12:  // Vergrößerung
#endif
					if (wp)
					{
						STRPTR t;
						
						if (GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END))
#ifdef __amigaos4__
						{
						    char gt[15];
						    
						    Strlcpy(gt, t, 15);
						    if(!strchr(gt, '%'))
						    {
						    	Strlcat(gt, "%", 10);
						    	GT_SetGadgetAttrs(gad, win, NULL, GTST_String, &gt, TAG_END);
						    }
							wp->wp_Zoom = (ULONG)(ConvertDegreeProcent(gt) * 1024 + 0.5);
						}
#else
							wp->wp_Zoom = (ULONG)(ConvertDegreeProcent(t) * 1024 + 0.5);
#endif
					}
					break;
				case 16:  // Ok
				{
					struct Mappe *mp = ((struct Page *)wd->wd_Data)->pg_Mappe;
					BOOL   one = (BOOL)((long)wd->wd_ExtData[3]);
					WORD   unit = (WORD)((long)wd->wd_ExtData[4]);
					struct List *list;
					LONG   flags;

					list = wd->wd_ExtData[1];
					wd->wd_ExtData[1] = NULL;

					flags = GetCheckBoxFlag(GadgetAddress(win, 13), win, MPPRTF_PAGENUMBERS)
// ToDo: enable it again!
//							  | GetCheckBoxFlag(GadgetAddress(win, 15), win, MPPRTF_ASYNCHRON)
							| GetCheckBoxFlag(GadgetAddress(win, 14), win, MPPRTF_NAME);

					mp->mp_PrinterFlags = flags;

					CloseAppWindow(win, TRUE);

					PrintProject(list, one ? wp : NULL, unit, flags);
					break;
				}

				case 17:  // Einstellungen...
				{
					char command[64];
					
#ifdef __amigaos4__
					sprintf(command, "SYS:Prefs/Printer PUBSCREEN \"%s\"", (prefs.pr_Screen->ps_Type == PST_PUBLIC ? prefs.pr_Screen->ps_PubName : pubname));
#else
					sprintf(command, "SYS:Prefs/Printer PUBSCREEN \"%s\"", pubname);
#endif
					SystemTags(command,
						SYS_Asynch,		TRUE,
						SYS_Input,		NULL,
						SYS_Output,		NULL,
						SYS_UserShell,	TRUE,
						TAG_END);
					break;
				}

				case 18:  // Abbrechen
					CloseAppWindow(win, TRUE);
					break;
			}
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
			break;
	}
}


void ASM
ClosePrintStatusWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	struct wdtPrintStatus *wps = wd->wd_ExtData[0];
	struct ProgressBar *projpb = wps->wps_ProjectBar;
	struct ProgressBar *pagepb = wps->wps_PageBar;
	struct ProgressBar *spagepb = wps->wps_SinglePageBar;

	asp_wps = NULL;  wps->wps_Window = NULL;
	wps->wps_ProjectBar = NULL;  wps->wps_PageBar = NULL;  wps->wps_SinglePageBar = NULL;

	DisposeProgressBar(projpb);
	DisposeProgressBar(pagepb);
	DisposeProgressBar(spagepb);

	if (clean && !wps->wps_Locked)
		FreePooled(pool, wps, sizeof(struct wdtPrintStatus));
}


void ASM
HandlePrintStatusIDCMP(REG(a0, struct TagItem *tag))
{
    printf("Hallo\n");
}


void
InitPrinter(void)
{
	sPrintLabels[0] = GetString(&gLocaleInfo, MSG_PRINT_ALL_PAGES_GAD);
	sPrintLabels[1] = GetString(&gLocaleInfo, MSG_PRINT_VERTICALLY_THROUGH_GAD);
	sPrintLabels[2] = GetString(&gLocaleInfo, MSG_PRINT_HORIZONTALLY_THROUGH_GAD);
	sPrintLabels[3] = GetString(&gLocaleInfo, MSG_PRINT_FIRST_PAGE_GAD);
	sPrintLabels[4] = GetString(&gLocaleInfo, MSG_PRINT_RANGE_LABEL);
	sPrintLabels[5] = NULL;
}
 
