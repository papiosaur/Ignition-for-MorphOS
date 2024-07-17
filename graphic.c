/* General and 2D graphics routines
 *
 * Copyright 1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "classes.h"
#ifdef __amigaos4__
	#include "lib/safeclip/generic/safeclip.h"
#else
	#include "lib/safeclip/sasc-68k/safeclip.h"
#endif


#define AREABUFFERS 300

struct MinList colors;
struct RastPort *grp, *doublerp, *irp;
static struct Layer_Info *gli;
static struct TmpRas sTempRaster;
static struct AreaInfo sAreaInfo;
static PLANEPTR sTempBitmap = NULL;
static APTR *sAreaBuffer = NULL;

#ifdef __amigaos4__

#define PI 3.14159265
#define DEGREE_STEPS PI/128
double
gcalcllength(double vx, double vy)
{
    return sqrt(vx*vx + vy*vy);
}

void
gAreaArcMove(struct RastPort *rp, long x, long y, long xradius, long yradius, double degree)
{
    gAreaDraw(rp, (long)(x + sin(degree) * xradius + 0.5), (long)(y + cos(degree) * yradius + 0.5));
}


void
gAreaArc(struct RastPort *rp, long x, long y, long xradius, long yradius, double degree)
{
    gAreaDraw(rp, (long)(x + sin(degree) * xradius + 0.5), (long)(y + cos(degree) * yradius + 0.5));
}


void
DrawArc(struct RastPort *rp, long x, long y, long xradius, long yradius, double start, double end)
{
    double degree;

    start = start * PI / 180.0;
    end = end * PI / 180.0;

    gAreaMove(rp, x, y);

	for (degree = start; degree < end; degree += DEGREE_STEPS) {
        gAreaArc(rp, x, y, xradius, yradius, degree);
    }

    if (degree > end)
        gAreaArc(rp, x, y, xradius, yradius, end);

    gAreaEnd(rp);
}


void
drawSide(struct RastPort *rp, long x, long y, long xradius, long yradius, long height, double start, double end)
{
    double degree;

    start = start * PI / 180.0;
    end = end * PI / 180.0;

    gAreaArcMove(rp, x, y + height, xradius, yradius, start);
    gAreaArc(rp, x, y, xradius, yradius, start);

    for (degree = start; degree < end; degree += DEGREE_STEPS)
        gAreaArc(rp, x, y, xradius, yradius, degree);

    if (degree > end)
        gAreaArc(rp, x, y, xradius, yradius, end);

    gAreaArc(rp, x, y + height, xradius, yradius, end);

    for (degree = end; degree > start; degree -= DEGREE_STEPS)
        gAreaArc(rp, x, y + height, xradius, yradius, degree);

    gAreaEnd(rp);
}
#endif
 
void
gInitArea(long xmin, long ymin, long xmax, long ymax)
{
	SafeSetLimits(xmin, ymin, xmax, ymax);
}


void PUBLIC
libAreaMove(REG(a0, struct RastPort *rp), REG(d0, LONG x), REG(d1, LONG y))
{
	gAreaMove(rp, x, y);
}


void PUBLIC
libAreaDraw(REG(a0, struct RastPort *rp), REG(d0, LONG x), REG(d1, LONG y))
{
	gAreaDraw(rp, x, y);
}


void PUBLIC
libAreaEnd(REG(a0, struct RastPort *rp))
{
	gAreaEnd(rp);
}


ULONG PUBLIC
GetDPI(REG(a0, struct Page *page))
{
	return page->pg_DPI;
}


LONG PUBLIC
GetOffset(REG(a0, struct Page *page), REG(d0, BOOL horiz))
{
	return horiz ? page->pg_wTabX - page->pg_TabX : page->pg_wTabY - page->pg_TabY;
}


void
RectFill32(struct RastPort *rp, long x1, long y1, long x2, long y2)
{
	x1 = max(x1, 0);  y1 = max(y1, 0);
	x2 = min(x2, 32567);  y2 = min(y2, 32567);
	RectFill(rp, x1, y1, x2, y2);
}


void
DrawHorizBlock(struct RastPort *rp, ULONG dpi, long x1, long y, long x2, ULONG points, UWORD flags)
{
	long width;

	width = (points * (dpi & 0xffff)) / (72*256);

	if (x2 < x1)
		RectFill(rp, x2, y - width, x1, y);
	else
		RectFill(rp, x1, y, x2, y + width);
}


void
DrawVertBlock(struct RastPort *rp, ULONG dpi, long x, long y1, long y2, ULONG points, UWORD flags)
{
	long width;

	width = (points * (dpi >> 16)) / (72*256);

	if (y2 < y1)
		RectFill(rp, x - width, y2, x, y1);
	else
		RectFill(rp, x, y1, x + width, y2);
}


void PUBLIC
DrawLine(REG(a0, struct RastPort *rp), REG(d0, ULONG dpi), REG(d1, long x1), REG(d2, long y1), REG(d3, long x2), REG(d4, long y2), REG(d5, ULONG points), REG(d6, UWORD flags))
{
	double f,a,b,c,border;
	long pen,xa,yb;

	grp->Layer = rp->Layer;   /* MERKER: das Zeugs könnte und sollte raus */
	grp->BitMap = rp->BitMap;
	pen = GetAPen(rp);
	SetAPen(grp, pen);
	SetOutlinePen(grp, pen);   /* MERKER: Darf die das?? */

	f = ((double)(dpi >> 16))/((double)(dpi & 0xffff));
	a = (double)(y2 - y1)*f;
	b = (double)(x2 - x1);
	c = sqrt(a*a + b*b);
	border = ((double)points*(dpi >> 16))/9437184.0;   // (72.0*65536.0*2)
	xa = (long)(border * a/c+0.5);  yb = -(long)((border*b/c)+0.5)/f;

	if (xa || yb) {
		gAreaMove(grp, x1 + xa, y1 + yb);
		gAreaDraw(grp, x2 + xa, y2 + yb);
		gAreaDraw(grp, x2 - xa, y2 - yb);
		gAreaDraw(grp, x1 - xa, y1 - yb);
		gAreaEnd(grp);
	} else {
		Move(rp, x1, y1);
		Draw(rp, x2, y2);
	}
	grp->Flags &= ~AREAOUTLINE;
}


void
FreeTmpRas(void)
{
	if (sTempBitmap)
		FreeVec(sTempBitmap);
}


void
MakeTmpRas(long width, long height)
{
	long bytes = ((width + 15) >> 3)*height;

	FreeTmpRas();
#ifdef __amigaos4__
	if ((sTempBitmap = AllocVecTags(bytes, AVT_Type, MEMF_PRIVATE, AVT_ClearWithValue, 0, TAG_DONE)) != 0)
#else
	if ((sTempBitmap = AllocVec(bytes, MEMF_CHIP | MEMF_CLEAR)) != 0)
#endif
	{
		InitTmpRas(&sTempRaster, sTempBitmap, bytes);
		grp->TmpRas = &sTempRaster;
	}

	gInitArea(0, 0, width - 1, height - 1);
}


void
FreeDoubleBufferBitMap(void)
{
	if (doublerp) {
		DeleteLayer(0, doublerp->Layer);
		FreeBitMap(doublerp->BitMap);
		doublerp->BitMap = NULL;
	}
}


void
AllocDoubleBufferBitMap(struct Screen *scr)
{
	if (doublerp) {
		if ((doublerp->BitMap = AllocBitMap(scr->Width, scr->Height, GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH), BMF_MINPLANES, scr->RastPort.BitMap)) != 0)
			doublerp->Layer = CreateUpfrontLayer(gli,doublerp->BitMap,0,0,scr->Width-1,scr->Height-1,LAYERSIMPLE,NULL);
		else
			FreeDoubleBuffer();
	}
}


void
FreeDoubleBuffer(void)
{
	if (!doublerp)
		return;

	if (doublerp->BitMap)
		FreeDoubleBufferBitMap();

	DisposeLayerInfo(gli);
	FreePooled(pool, doublerp, sizeof(struct RastPort));
	doublerp = NULL;
}


void
InitDoubleBuffer(void)
{
	if ((doublerp = AllocPooled(pool, sizeof(struct RastPort))) != 0)
	{
		gli = NewLayerInfo();
		InitRastPort(doublerp);
	}
}


void
FreeGraphics(void)
{
	FreeGClasses();

	if (sAreaBuffer)
		FreePooled(pool, sAreaBuffer, AREABUFFERS * 5);

	SafeClose();  /* safe area-functions by Peter Knight */
	FreeTmpRas();
	if (grp)
		FreePooled(pool, grp, sizeof(struct RastPort));

	FreeDoubleBuffer();
	if (irp)
		FreePooled(pool, irp, sizeof(struct RastPort));
}


void
InitGraphics(void)
{
	struct colorPen *cp;
	long   i;
	char   t[16];

	InitGClasses();

	if (IsListEmpty((struct List *)&colors)) {
		AddColor(&colors, NULL, 170, 170, 170);   // grey
		AddColor(&colors, NULL,   0,   0,   0);   // black
		AddColor(&colors, NULL, 255, 255, 255);   // white
		AddColor(&colors, NULL, 101, 138, 186);   // blue
		AddColor(&colors, NULL,  85, 223,  85);   // green
		AddColor(&colors, NULL, 239,  69,  69);   // red
		AddColor(&colors, NULL, 255, 239,  69);   // yellow
		AddColor(&colors, NULL,  97,  97,  97);   // dark grey
	}

	//Standardfarben für ignition GUI setzen
#ifdef __amigaos4__
	struct Screen *tscr;
	
	if ((tscr = LockPubScreen("Workbench")) != 0) {
		GetRGB32(tscr->ViewPort.ColorMap,0,8,(ULONG *)&standardPalette[1]);
		UnlockPubScreen(NULL, tscr);
        }
#endif
	
	MyNewList(&scrcolors);
	for (i = 0; i < 8; i++) {
		if ((cp = AllocPooled(pool, sizeof(struct colorPen))) != 0) {
			sprintf(t, GetString(&gLocaleInfo, MSG_COLOR), i + 1);
			cp->cp_Node.ln_Name = AllocString(t);
			cp->cp_Pen = i;
			cp->cp_Red = standardPalette[i * 3 + 1] >> 24;
			cp->cp_Green = standardPalette[i * 3 + 2] >> 24;
			cp->cp_Blue = standardPalette[i * 3 + 3] >> 24;
			MyAddTail(&scrcolors, cp);
		}
	}

	if ((grp = AllocPooled(pool, sizeof(struct RastPort))) != 0) {
		InitRastPort(grp);
		if ((sAreaBuffer = AllocPooled(pool, AREABUFFERS * 5)))
			InitArea(grp->AreaInfo = &sAreaInfo, sAreaBuffer, AREABUFFERS);
		SafeInit(AREABUFFERS * 5);
		MakeTmpRas(800, 600);
	}
	if ((irp = AllocPooled(pool, sizeof(struct RastPort))) != 0)
		InitRastPort(irp);
}


#define JSR 0x4ef9

const APTR gClassFuncTable[] = {
	/*  0 */ SetOutlineColor,
	/*  1 */ SetLowColor,
	/*  2 */ gGetLink,
	/*  3 */ gSuperDraw,
	/*  4 */ TintColor,
	/*  5 */ gInsertRemoveCellsTerm,
	/*  6 */ gInsertRemoveCellsTablePos,
	/*  7 */ CalcTerm,
	/*  8 */ CopyTerm,
	/*  9 */ DeleteTerm,
	/* 10 */ CreateTerm,
	/* 11 */ mm,
	/* 12 */ pixel,
	/* 13 */ OutlineHeight,
	/* 14 */ OutlineLength,
	/* 15 */ DrawText,
	/* 16 */ NewFontInfoA,
	/* 17 */ CopyFontInfo,
	/* 18 */ SetFontInfoA,
	/* 19 */ FreeFontInfo,
	/* 20 */ GetOffset,
	/* 21 */ GetDPI,
	/* 22 */ libAreaEnd,
	/* 23 */ libAreaDraw,
	/* 24 */ libAreaMove,
	/* 25 */ DrawLine,
	/* 26 */ DrawRect,
	/* 27 */ FindColorPen,
	/* 28 */ SetColors,
	/* 29 */ SetHighColor,
	/* 30 */ gDoSuperMethodA,
	/* 31 */ gDoMethodA,
	/* 32 */ FreeString,
	/* 33 */ AllocString,
	/* 34 */ AllocStringLength,
#ifdef __amigaos4__
	/* 35 */ gAreaArcMove,
	/* 36 */ gAreaArc,
	/* 37 */ DrawArc,
	/* 38 */ drawSide,
	/* 39 */ gcalcllength,
#endif
	NULL
};

int32 gClassFuncTableSize = sizeof(gClassFuncTable);

