/* Color management functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"


static struct ColorContext sDefaultContext;
static LONG sTruePen, sLowTruePen, sOutlineTruePen;	   
 

static __inline struct ColorContext *
GetColorContext(struct RastPort *rp)
{
	if (rp == NULL || rp->RP_User == NULL)
		return &sDefaultContext;

	return (struct ColorContext *)rp->RP_User;
}


/************************ Standard-Amiga Context ************************/


static void
Standard_SetHighColor(struct RastPort *rp, ULONG color)
{
	struct colorPen *cp = GetColorPen(color);

	SetAPen(rp, cp->cp_Pen);
}


static void
Standard_SetLowColor(struct RastPort *rp, ULONG color)
{
	struct colorPen *cp = GetColorPen(color);

	SetBPen(rp, cp->cp_Pen);
}


static void
Standard_SetOutlineColor(struct RastPort *rp, ULONG color)
{
	struct colorPen *cp = GetColorPen(color);

	SetOutlinePen(rp, cp->cp_Pen);
}


static void
Standard_Release(struct Screen *screen, struct ColorContext *context)
{
}
 

static void
Standard_Init(struct Screen *screen, struct ColorContext *context, bool all)
{
	sDefaultContext.cc_TrueColor = false;

	sDefaultContext.cc_Release = Standard_Release;
	sDefaultContext.cc_SetHighColor = Standard_SetHighColor;
	sDefaultContext.cc_SetLowColor = Standard_SetLowColor;
	sDefaultContext.cc_SetOutlineColor = Standard_SetOutlineColor;
}


/************************ TrueColor-Amiga Context ************************/


static void
True_SetHighColor(struct RastPort *rp, ULONG id)
{
    ULONG red = (id >> 16) & 0xff;
    ULONG green = (id >> 8) & 0xff;
    ULONG blue = id & 0xff;

    SetRGB32(&scr->ViewPort, sTruePen, RGB32(red), RGB32(green), RGB32(blue));
	SetAPen(rp, sTruePen);
}


static void
True_SetLowColor(struct RastPort *rp, ULONG id)
{
    ULONG red = (id >> 16) & 0xff;
    ULONG green = (id >> 8) & 0xff;
    ULONG blue = id & 0xff;

    SetRGB32(&scr->ViewPort, sLowTruePen, RGB32(red), RGB32(green), RGB32(blue));
	SetBPen(rp, sLowTruePen);
}


static void
True_SetOutlineColor(struct RastPort *rp, ULONG id)
{
    ULONG red = (id >> 16) & 0xff;
    ULONG green = (id >> 8) & 0xff;
    ULONG blue = id & 0xff;

    SetRGB32(&scr->ViewPort, sOutlineTruePen, RGB32(red), RGB32(green), RGB32(blue));
	SetOutlinePen(rp, sOutlineTruePen);
}


static void
True_Release(struct Screen *screen, struct ColorContext *context)
{
	ReleasePen(screen->ViewPort.ColorMap, sTruePen);
	ReleasePen(screen->ViewPort.ColorMap, sLowTruePen);
	ReleasePen(screen->ViewPort.ColorMap, sOutlineTruePen);
}


static void
True_Init(struct Screen *screen, struct ColorContext *context)
{
	sDefaultContext.cc_TrueColor = true;

	sDefaultContext.cc_Release = True_Release;
	sDefaultContext.cc_SetHighColor = True_SetHighColor;
	sDefaultContext.cc_SetLowColor = True_SetLowColor;
	sDefaultContext.cc_SetOutlineColor = True_SetOutlineColor;

	sTruePen = ObtainPen(screen->ViewPort.ColorMap, -1, 0L, 0L, 0L, PEN_EXCLUSIVE | PEN_NO_SETCOLOR);
	sLowTruePen = ObtainPen(screen->ViewPort.ColorMap, -1, 0L, 0L, 0L, PEN_EXCLUSIVE | PEN_NO_SETCOLOR);
	sOutlineTruePen = ObtainPen(screen->ViewPort.ColorMap, -1, 0L, 0L, 0L, PEN_EXCLUSIVE | PEN_NO_SETCOLOR);
}


/************************ Other and public color functions ***************************/
 

void
ReleaseAppColors(struct Screen *screen)
{
	struct ColorContext *context = GetColorContext(NULL);
    struct colorPen *cp;

	if (scr == NULL)
		return;

	if (context->cc_Release != NULL)
		context->cc_Release(screen, context);

	// ToDo: move this to Standard_Release() - but that might break some other code...
	foreach (&colors, cp)
		ReleasePen(scr->ViewPort.ColorMap, cp->cp_Pen);
}


void
ObtainAppColors(struct Screen *screen, BOOL all)
{
	struct ColorContext *context = GetColorContext(NULL);
    struct colorPen *cp;

	if (!screen)
        return;

	// ToDo: MorphOS TrueColor Unterstützung einbauen

	if (GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) > 8)
		True_Init(screen, context);
	else
		Standard_Init(screen, context, all);

	// ToDo: move this to Standard_Init() - but that might break some other code...
	foreach (&colors, cp) {
		if (all || cp->cp_Pen == -1) {
			ULONG r, g, b;
			r = cp->cp_Red;
			g = cp->cp_Green;
			b = cp->cp_Blue;
            r = r | (r << 8) | (r << 16) | (r << 24);
            g = g | (g << 8) | (g << 16) | (g << 24);
            b = b | (b << 8) | (b << 16) | (b << 24);

			cp->cp_Pen = ObtainBestPen(screen->ViewPort.ColorMap, r, g, b, TAG_END);
        }
    }
}


void
FreeAppColors(void)
{
    struct colorPen *cp;

	ReleaseAppColors(scr);

	while ((cp = (struct colorPen *)MyRemHead(&colors)) != 0) {
        FreeString(cp->cp_Node.ln_Name);
		FreePooled(pool, cp, sizeof(struct colorPen));
    }
}


void
GetColorName(STRPTR t, UBYTE red, UBYTE green, UBYTE blue)
{
	static const LONG farbton[] = {
		MSG_RED_COLOR, MSG_ORANGE_COLOR, MSG_YELLOW_COLOR, MSG_GREEN_COLOR,
		MSG_TURQUOISE_COLOR, MSG_BLUE_COLOR, MSG_PURPLE_COLOR
	};
	static const WORD tons = 7, tonzahl[] = {-20, 15, 33, 50, 115, 125, 200, 236};
	static const LONG ograus[] = {MSG_WHITE_COLOR, MSG_BRIGHT_GRAY_COLOR, MSG_GRAY_COLOR, MSG_DARK_GRAY_COLOR, MSG_BLACK_COLOR};
	static const WORD grauzahl[] = {230, 180, 110, 50};
    struct ColorWheelHSB hsb;
    struct ColorWheelRGB rgb;
	WORD   hue, sat, brt, i;

    rgb.cw_Red = RGB32(red);
    rgb.cw_Green = RGB32(green);
    rgb.cw_Blue = RGB32(blue);
	ConvertRGBToHSB(&rgb, &hsb);
    hue = hsb.cw_Hue >> 24;
    sat = hsb.cw_Saturation >> 24;
    brt = hsb.cw_Brightness >> 24;

    if (red == 241 && green == 159 && blue == 159)
		strcpy(t, GetString(&gLocaleInfo, MSG_PIGGY_PINK_COLOR));
    else if (red == 132 && green == 91 && blue == 22)
		strcpy(t, GetString(&gLocaleInfo, MSG_BROWN_COLOR));
    else if (sat > 25)
    {
        if (hue > tonzahl[0]+256)
            hue -= 256;
        for(i = 0;i < tons;i++)
        {
            if (hue > tonzahl[i] && hue <= tonzahl[i+1])
				strcpy(t, GetString(&gLocaleInfo, farbton[i]));
        }
        if (brt < 120)
			strins(t, GetString(&gLocaleInfo, MSG_DARK_COLOR_TINT_NAME));
        t[0] = toupper(t[0]);
    }
    else
    {
		for (i = 0; i < 4 && brt < grauzahl[i]; i++);
		strcpy(t, GetString(&gLocaleInfo, ograus[i]));
    }
}


void
FindColorName(struct colorPen *cp)
{
	char t[32];

	GetColorName(t, cp->cp_Red, cp->cp_Green, cp->cp_Blue);
    FreeString(cp->cp_Node.ln_Name);
    cp->cp_Node.ln_Name = AllocString(t);
}


struct colorPen *
GetBestPen(UBYTE red, UBYTE green, UBYTE blue)
{
	struct colorPen *cp, *bcp;
	ULONG dist = ~0L, d;
	long r, g, b;

	foreach (&colors, cp)
    {
		r = red - cp->cp_Red;      /* kürzester Vektor im Farbraum */
		g = green - cp->cp_Green;
		b = blue - cp->cp_Blue;

		d = r*r + g*g + b*b;
        if (d < dist)
        {
            dist = d;
            bcp = cp;
        }
    }
	return bcp;
}


ULONG PUBLIC
FindColorPen(REG(d0, UBYTE red), REG(d1, UBYTE green), REG(d2, UBYTE blue))
{
    struct colorPen *cp;

	if ((cp = GetBestPen(red, green, blue)) != 0)
        return cp->cp_ID;

    return 0;
}


ULONG PUBLIC
TintColor(REG(d0, ULONG id), REG(d1, float tint))
{
    ULONG red = ((id >> 16) & 0xff) * tint;
    ULONG green = ((id >> 8) & 0xff) * tint;
    ULONG blue = (id & 0xff) * tint;

    if (red > 255)
        red = 255;
    if (green > 255)
        green = 255;
    if (blue > 255)
        blue = 255;

    return (ULONG)((red << 16L) | (green << 8L) | blue);
}


struct colorPen *
GetColorPen(ULONG id)
{
    struct colorPen *cp;

	foreach (&colors, cp)
    {
        if (cp->cp_ID == id)
            return cp;
    }

    return GetBestPen((id >> 16) & 0xff,(id >> 8) & 0xff,id & 0xff);
}


void PUBLIC
SetOutlineColor(REG(a0, struct RastPort *rp), REG(d0, ULONG color))
{
	struct ColorContext *context = GetColorContext(rp);

	context->cc_SetOutlineColor(rp, color);
}


void PUBLIC
SetHighColor(REG(a0, struct RastPort *rp), REG(d0, ULONG color))
{
	struct ColorContext *context = GetColorContext(rp);

	context->cc_SetHighColor(rp, color);
}


void PUBLIC
SetLowColor(REG(a0, struct RastPort *rp), REG(d0, ULONG color))
{
	struct ColorContext *context = GetColorContext(rp);

	context->cc_SetLowColor(rp, color);
}


void PUBLIC
SetColors(REG(a0, struct RastPort *rp), REG(d0, ULONG apenID), REG(d1, ULONG bpenID))
{
    if (apenID != ~0L)
		SetHighColor(rp, apenID);

    if (bpenID != ~0L)
		SetLowColor(rp, bpenID);
}


void PUBLIC	/* not yet public */
SetColorPen(REG(a0, struct RastPort *rp), REG(a1, struct colorPen *pen))
{
	struct ColorContext *context = GetColorContext(rp);

	if (context->cc_TrueColor)
		context->cc_SetHighColor(rp, pen->cp_ID);
	else
		SetAPen(rp, pen->cp_Pen);
}

 
/*
long compareColors(const struct colorPen **cpa,const struct colorPen **cpb)
{
    return((long)(*cpa)->cp_ID-(long)(*cpb)->cp_ID);
}
*/

void
UniqueColors(struct MinList *cols)
{
	struct colorPen *cp, *ncp, *scp;

    /*SortListWith(cols,compareColors);
    foreach(cols,cp)
    {
        while((ncp = (APTR)cp->cp_Node.ln_Succ)->cp_Node.ln_Succ && cp->cp_ID == ncp->cp_ID)
        {
            if (!cp->cp_Node.ln_Name && ncp->cp_Node.ln_Name)
                cp->cp_Node.ln_Name = ncp->cp_Node.ln_Name;
            else
                FreeString(ncp->cp_Node.ln_Name);
            MyRemove(ncp);
            FreePooled(pool,ncp,sizeof(struct colorPen));
        }
    }*/
	foreach (cols, cp)
    {
        scp = cp;

		while ((ncp = (APTR)scp->cp_Node.ln_Succ)->cp_Node.ln_Succ)
        {
            if (cp->cp_ID == ncp->cp_ID)
            {
                if (!cp->cp_Node.ln_Name && ncp->cp_Node.ln_Name)
                    cp->cp_Node.ln_Name = ncp->cp_Node.ln_Name;
                else
                    FreeString(ncp->cp_Node.ln_Name);
                MyRemove(ncp);
				FreePooled(pool, ncp, sizeof(struct colorPen));
            }
            else
                scp = ncp;
        }
    }
}


struct colorPen *
AddColor(struct MinList *list, STRPTR name, UBYTE red, UBYTE green, UBYTE blue)
{
    struct colorPen *cp;

	if ((cp = AllocPooled(pool, sizeof(struct colorPen))) != 0)
    {
        cp->cp_Red = red;
        cp->cp_Green = green;
        cp->cp_Blue = blue;
        cp->cp_Pen = -1;
        cp->cp_ID = (red << 16) | (green << 8) | blue;
        if (name)
            cp->cp_Node.ln_Name = AllocString(name);
        else
        {
            cp->cp_Node.ln_Type = 1;
            FindColorName(cp);
        }
		MyAddTail(list, cp);
    }
    return cp;
}
