/* font-engine, -management, and character routines
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include <stdarg.h>
	extern struct EGlyphEngine EEngine;
	extern uint32 *unicode_map;
#endif

#define TRACE_FONT 0
#if TRACE_FONT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

struct MinList infos, fonts, families, fontpaths;
struct SignalSemaphore fontSemaphore;

static struct Font *
OpenFontEngine(struct FontFamily *ff, long style)
{
	struct Font *fo, *fallback = NULL;
	char   engine[64], *name;
#ifdef __amigaos4__
	BOOL found = FALSE;
#endif
	if (!ff)
		return NULL;

	for (fo = (APTR)fonts.mlh_Head; fo->fo_Node.ln_Succ; fo = (APTR)fo->fo_Node.ln_Succ) {
		name = (STRPTR)GetTagData(OT_Family, 0, fo->fo_Tags);
		if (name != NULL && !strcmp(name, ff->ff_Node.ln_Name)) {
			// Found font of the requested family - now check for its properties.
			// If the font matches 100%, take it
			if ((fo->fo_Style & FS_STYLE_MASK) == style)
			{
			    found = TRUE;
				break;
			}
			// If the italic style is requested (or not), prefer those fonts
			// which has this property set (or not), no matter if they are
			// bold
			if ((style & FS_ITALIC) == (fo->fo_Style & FS_ITALIC))
				fallback = fo;

			if (fallback == NULL)
				fallback = fo;
		}
	}
#ifdef __amigaos4__
	if(!found) 
	{
	    char *styles[7] = {
    	    				"(Regular)", "", "Bold", "", "Italic / Oblique", "", "Italic / Oblique & Bold"
        				  };
	    char mess[256];
    
    	sprintf(mess, "%s Style %s %s", ff->ff_Node.ln_Name, styles[style & 0xf], (0x20 & style ? "Underline" : ""));
		ErrorRequest(GetString(&gLocaleInfo, MSG_FONT_NOT_FOUND_ERR), mess);
	}
#endif	
	if (!fo->fo_Node.ln_Succ) {
		if (fallback == NULL)
			return NULL;

		// let's use our fallback font for this family, and go from there...
		fo = fallback;
//printf("Font <%s> ausgewählt!\n", fo->fo_Node.ln_Name);
	}

	if (fo && !fo->fo_Engine && (name = (STRPTR)GetTagData(OT_Engine, 0, fo->fo_Tags)) != NULL) {
		// open font engine with first usage

		fo->fo_Space = GetTagData(OT_SpaceWidth, 2540, fo->fo_Tags);

#ifdef __amigaos4__
		Strlcpy(engine, name, 64);  Strlcat(engine, ".library", 64);
		if ((EEngine.ege_BulletBase = OpenLibrary(engine, 0)) != 0) {
#else
		strcpy(engine, name);  strcat(engine, ".library");
		if ((BulletBase = OpenLibrary(engine, 0L)) != 0) {
#endif
#ifdef __amigaos4__
            if (!EEngine.ege_BulletBase)
                return NULL;
            EEngine.ege_IBullet = (struct BulletIFace *)GetInterface(EEngine.ege_BulletBase, "main", 1, NULL);

			if (EOpenEngine(&EEngine)) {
			    if(!(fo->fo_Engine = AllocVecTags(sizeof(struct EGlyphEngine), AVT_Type, MEMF_PRIVATE, TAG_DONE)))
			    {
                	DropInterface((struct Interface *)EEngine.ege_IBullet);
					CloseLibrary(EEngine.ege_BulletBase);
			        return NULL;
			    }
			    CopyMem(&EEngine, fo->fo_Engine, sizeof(struct EGlyphEngine));
				if (!ESetInfo(fo->fo_Engine, OT_OTagPath, fo->fo_Node.ln_Name, OT_OTagList, fo->fo_Tags, TAG_END))
					return fo;

				ECloseEngine(&EEngine);
				fo->fo_Engine = NULL;
			}
            if (EEngine.ege_IBullet)
                DropInterface((struct Interface *)EEngine.ege_IBullet);
			CloseLibrary(EEngine.ege_BulletBase);
		}
#else
			if ((fo->fo_Engine = OpenEngine()) != 0) {
				if (!SetInfo(fo->fo_Engine, OT_OTagPath, fo->fo_Node.ln_Name, OT_OTagList, fo->fo_Tags, TAG_END))
					return fo;

				CloseEngine(fo->fo_Engine);
				fo->fo_Engine = NULL;
			}
			CloseLibrary(BulletBase);
		}
#endif		
		// we only get here in case of an error above
		return NULL;
	}
	return fo;
}


struct FontChar *
FindFontChar(struct FontSize *fs, UWORD code)
{
	struct FontChar *fc;

	if (code < 256)
		return fs->fs_CharsArray[code];

	for (fc = (APTR)fs->fs_Chars.mlh_Head; fc->fc_Node.mln_Succ; fc = (APTR)fc->fc_Node.mln_Succ) {
		if (fc->fc_Code == code)
			return fc;
	}

	return NULL;
}


struct FontChar *
CreateChar(struct FontSize *fs, UWORD code)
{
	struct FontChar *fc;
#ifdef __amigaos4__
	struct EGlyphEngine *gle;
#else
	struct GlyphEngine *gle;
#endif
	struct GlyphMap *glm;

	if (!fs)
		return NULL;

	if ((fc = FindFontChar(fs, code)) != 0)
		return fc;
	// we really have to recreate the character
	gle = fs->fs_Font->fo_Engine;
#ifdef __amigaos4__
	if (!ESetInfo(gle, OT_DeviceDPI,  fs->fs_DPI,
#else
	if (!SetInfo(gle, OT_DeviceDPI,  fs->fs_DPI,
#endif
									 OT_PointHeight, fs->fs_PointHeight,
									 OT_RotateSin,   fs->fs_RotateSin,
									 OT_RotateCos,   fs->fs_RotateCos,
									 OT_ShearSin,    fs->fs_ShearSin,
									 OT_ShearCos,    fs->fs_ShearCos,
									 OT_EmboldenX,   fs->fs_EmboldenX,
#ifdef __amigaos4__
									 OT_GlyphCode,   (unicode_map ? unicode_map[code] : code),
#else
									 OT_GlyphCode,   code,
#endif
									 TAG_END))
	{
#ifdef __amigaos4__
		if (!EObtainInfo(gle, OT_GlyphMap, &glm, TAG_END))
#else
		if (!ObtainInfo(gle, OT_GlyphMap, &glm, TAG_END))
#endif
		{
			if ((fc = AllocPooled(pool, sizeof(struct FontChar))) != 0)
			{
				fc->fc_Code = code;
				fc->fc_Glyph = *glm;
				if ((fc->fc_Glyph.glm_BitMap = AllocPooled(fs->fs_Pool, glm->glm_BMModulo * glm->glm_BMRows)) != 0)
				{
					CopyMem(glm->glm_BitMap, fc->fc_Glyph.glm_BitMap, glm->glm_BMModulo * glm->glm_BMRows);
					if (code < 256)
						fs->fs_CharsArray[code] = fc;
					else
						MyAddTail(&fs->fs_Chars, fc);
				}
				else
					FreePooled(pool, fc, sizeof(struct FontChar));
			}
#ifdef __amigaos4__
			EReleaseInfo(gle, OT_GlyphMap, glm, TAG_END);
#else
			ReleaseInfo(gle, OT_GlyphMap, glm, TAG_END);
#endif
		}
	}
	return fc;
}


void
GetFontChars(struct FontSize *fs, STRPTR t)
{
	if (!fs)
		return;

	for (; *t; t++) {
		CreateChar(fs, (UWORD)*t);
	}
}


uint32 PUBLIC
OutlineLength(REG(a0, struct FontInfo *fi), REG(a1, STRPTR t), REG(d0, long len))
{
	struct FontSize *fs;
	struct FontChar *fc;
	ULONG  length = 0,last = 0,kern = 0,space;

	if (!t || !len || !fi)
		return 0;	
#ifdef __amigaos4__
	fs  = fi->fi_FontSize;
	space = (fi->fi_CharSpace * (long)(fs->fs_DPI >> 16) + 36) / 72;
#else
	space = (fi->fi_CharSpace*(long)((fs = fi->fi_FontSize)->fs_DPI >> 16)+36)/72;
#endif

	for (; *t && len; t++, len--) {
		if (*t == ' ')
			length += fs->fs_Space;
		else if ((fc = CreateChar(fs, (UWORD)*t)) != 0) {
			if (fi->fi_Kerning) {
				if (*(t+1)) {
#ifdef __amigaos4__
					if (ESetInfo(fs->fs_Font->fo_Engine,OT_GlyphCode,*t,OT_GlyphCode2,*(t+1),TAG_END) || EObtainInfo(fs->fs_Font->fo_Engine,fi->fi_Kerning == FK_TEXT ? OT_TextKernPair : OT_DesignKernPair,&kern,TAG_END))
#else
					if (SetInfo(fs->fs_Font->fo_Engine,OT_GlyphCode,*t,OT_GlyphCode2,*(t+1),TAG_END) || ObtainInfo(fs->fs_Font->fo_Engine,fi->fi_Kerning == FK_TEXT ? OT_TextKernPair : OT_DesignKernPair,&kern,TAG_END))
#endif
						kern = 0;
				} else
					kern = 0;
			}
			length += ((fc->fc_Glyph.glm_Width-kern)*fs->fs_EMWidth+space) >> 16;
			last += ((fc->fc_Glyph.glm_Width-kern)*fs->fs_EMWidth+space) & 65535;
			if (last > 65535) {
				length++;
				last -= 65536;
			}
		}
	}
	return length;
}


uint32 PUBLIC
OutlineHeight(REG(a0, struct FontInfo *fi),REG(a1, STRPTR t),REG(d0, long len))
{
	if (fi)
		return fi->fi_FontSize->fs_EMHeight;    /* TODO: was Pointheight */

	return 0L;                                /* there is something missing... */
	// if (
}


void PUBLIC
DrawText(REG(a0, struct RastPort *rp), REG(a1, struct FontInfo *fi), REG(a2, STRPTR t), REG(d0, long x), REG(d1, long y))
{
	struct FontSize *fs;
	struct FontChar *fc;
	struct GlyphMap glm;
	ULONG  emwidth,last = 0,ox = x,kern = 0,space;
	UWORD  newwidth;

	if (!t || !fi || !rp)
		return;

	SetDrMd(rp, JAM1);

	space = (fi->fi_CharSpace*(long)((fs = fi->fi_FontSize)->fs_DPI >> 16)+36)/72;
	emwidth = fs->fs_EMWidth;
	y += fs->fs_EMHeight - fs->fs_EMTop;

	for (;*t; t++)
	{
		if (*t == ' ')
			x += fs->fs_Space;
		else if ((fc = CreateChar(fs, (UWORD)*t)) != 0)
		{
			glm = fc->fc_Glyph;
			if (fi->fi_Kerning)
			{
				if (t[1])
				{
#ifdef __amigaos4__
					if (ESetInfo(fs->fs_Font->fo_Engine, OT_GlyphCode, t[0], OT_GlyphCode2, t[1], TAG_END) 
						|| EObtainInfo(fs->fs_Font->fo_Engine, fi->fi_Kerning == FK_TEXT ? OT_TextKernPair : OT_DesignKernPair, &kern, TAG_END))
#else
					if (SetInfo(fs->fs_Font->fo_Engine, OT_GlyphCode, t[0], OT_GlyphCode2, t[1], TAG_END)
						|| ObtainInfo(fs->fs_Font->fo_Engine, fi->fi_Kerning == FK_TEXT ? OT_TextKernPair : OT_DesignKernPair, &kern, TAG_END))
#endif
						kern = 0;
				}
				else
					kern = 0;
			}
			{
				long a = (glm.glm_Width - kern) * emwidth + space;

				newwidth = a >> 16;
				last += a & 65535;
			}

			if (last > 65535)
			{
				newwidth++;
				last -= 65536;
			}

			BltTemplate((PLANEPTR)((ULONG)glm.glm_BitMap + glm.glm_BMModulo * glm.glm_BlackTop + ((glm.glm_BlackLeft >> 4) << 1)),
				glm.glm_BlackLeft & 0xf, glm.glm_BMModulo, rp, x - glm.glm_X0 + glm.glm_BlackLeft, y - glm.glm_Y0 + glm.glm_BlackTop,
				glm.glm_BlackWidth, glm.glm_BlackHeight);

			x += newwidth;
		}
	}
	/** (double) underlined, strike-through **/
	{
		long style = fi->fi_Style;

		if (style & (FS_UNDERLINED | FS_DOUBLE_UNDERLINED)) {
			DrawHorizBlock(rp, fs->fs_DPI, ox, y += ((fs->fs_PointHeight >> 16)*(fs->fs_DPI & 0xffff))/(72*9),x,0x100,0);
			if (style & FS_DOUBLE_UNDERLINED)
				DrawHorizBlock(rp, fs->fs_DPI, ox, y+((256+128)*(fs->fs_DPI & 0xffff))/(72*128),x,0x100,0);
		} else if (style & FS_STRIKE_THROUGH)
			DrawHorizBlock(rp, fs->fs_DPI, ox, y - (fs->fs_EMHeight >> 1) + fs->fs_EMTop, x, 0x100, 0);
	}
}

#if 0
void PUBLIC
DrawTextWithWidth(REG(a0, struct RastPort *rp), REG(a1, struct FontInfo *fi), REG(a2, STRPTR t), REG(d0, long x), REG(d1, long y), REG(d2, long width))
{
	struct FontSize *fs;
	struct FontChar *fc;
	struct GlyphMap glm;
	ULONG  emwidth,last = 0,ox = x,kern = 0,space;
	ULONG  newwidth = 0,shortwidth;

	if (!t || !fi || !rp)
		return;

	space = (fi->fi_CharSpace*(long)((fs = fi->fi_FontSize)->fs_DPI >> 16)+36)/72;
	emwidth = fs->fs_EMWidth;  width += x;
	y += fs->fs_EMHeight-fs->fs_EMTop;

	for(;*t && width > x;t++)
	{
		if (*t == ' ')
		{
			x += fs->fs_Space;
			newwidth += fs->fs_Space << 16;
		}
		else if (fc = CreateChar(fs,(UWORD)*t))
		{
			glm = fc->fc_Glyph;
			if (fi->fi_Kerning)
			{
				if (*(t+1))
				{
					if (SetInfo(fs->fs_Font->fo_Engine,OT_GlyphCode,*t,OT_GlyphCode2,*(t+1),TAG_END) || ObtainInfo(fs->fs_Font->fo_Engine,fi->fi_Kerning == FK_TEXT ? OT_TextKernPair : OT_DesignKernPair,&kern,TAG_END))
						kern = 0;
				}
				else
					kern = 0;
			}
			newwidth += ((glm.glm_Width-kern)*emwidth+space);
			shortwidth = (newwidth+32768) >> 16;
			if (ox+shortwidth < width)
			{
				BltTemplate((PLANEPTR)((ULONG)glm.glm_BitMap+glm.glm_BMModulo*glm.glm_BlackTop+((glm.glm_BlackLeft >> 4) << 1)),
										glm.glm_BlackLeft & 0xf,glm.glm_BMModulo,rp,x-glm.glm_X0+glm.glm_BlackLeft,y-glm.glm_Y0+glm.glm_BlackTop,
										glm.glm_BlackWidth,glm.glm_BlackHeight);
				x = ox+shortwidth;
			}
			else
			{
				long ax;

				BltTemplate((PLANEPTR)((ULONG)glm.glm_BitMap+glm.glm_BMModulo*glm.glm_BlackTop+((glm.glm_BlackLeft >> 4) << 1)),
										glm.glm_BlackLeft & 0xf,glm.glm_BMModulo,rp,ax = x-glm.glm_X0+glm.glm_BlackLeft,y-glm.glm_Y0+glm.glm_BlackTop,
										width-ax,glm.glm_BlackHeight);
				x = width;
			}
		}
	}
	if (fi->fi_Style & FS_UNDERLINED)
		DrawHorizBlock(rp,fs->fs_DPI,ox,y+((fs->fs_PointHeight >> 16)*(fs->fs_DPI & 0xffff))/(72*9),x,0x100,0);
}
#endif

void
FreeFontSize(struct FontSize *fs)
{
	struct FontChar *fc;

	if (fs && --fs->fs_Locked <= 0)
	{
		int32 i;
		for (i = 0; i < 256; i++)
		{
			if (fs->fs_CharsArray[i])
				FreePooled(pool, fs->fs_CharsArray[i], sizeof(struct FontChar));
		}
		while ((fc = (struct FontChar *)MyRemHead(&fs->fs_Chars)) != 0)
			FreePooled(pool, fc, sizeof(struct FontChar));

#ifdef __amigaos4__
		FreeSysObject(ASOT_MEMPOOL, fs->fs_Pool);
#else
		DeletePool(fs->fs_Pool);
#endif
		FreePooled(pool, fs, sizeof(struct FontSize));
	}
}


struct FontSize *
GetFontSize(struct FontInfo *fi, ULONG dpi, long pointheight)
{
	struct Font *fo;
	struct FontSize *fs;
	struct FontChar *fc;
	long   xdpi = (dpi >> 16), ydpi = dpi & 0xffff;

	if (!(fo = OpenFontEngine((APTR)fi->fi_Family, fi->fi_Style)))
		return NULL;

	if ((fs = AllocPooled(pool, sizeof(struct FontSize))) != 0)
	{
		MyNewList(&fs->fs_Chars);
		fs->fs_DPI = dpi;
		fs->fs_PointHeight = pointheight;
		fs->fs_EMWidth = ((pointheight * xdpi)/72) >> 16;
		fs->fs_EMHeight = ((pointheight * ydpi)/72) >> 16;
		fs->fs_Space = (fo->fo_Space*(pointheight >> 16)*xdpi) / (2540*250);
		fs->fs_RotateCos = fs->fs_ShearCos = 0x10000;
		fs->fs_Font = fo;
		fs->fs_Locked = 1;

#ifdef __amigaos4__
		fs->fs_Pool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_CLEAR | MEMF_CHIP, ASOPOOL_Puddle, FS_POOLSIZE, ASOPOOL_Threshold, FS_POOLSIZE, TAG_END);
#else
		fs->fs_Pool = CreatePool(MEMF_CLEAR | MEMF_CHIP, FS_POOLSIZE, FS_POOLSIZE);    // ToDo: CyberGraphX-Support fehlt!!
#endif
		if (!fs->fs_Pool)
		{
			FreePooled(pool, fs, sizeof(struct FontSize));
			return NULL;
		}
		if ((fc = CreateChar(fs, '{')) != 0)
			fs->fs_EMTop = fs->fs_EMHeight-fc->fc_Glyph.glm_Y0 + fc->fc_Glyph.glm_BlackTop;
	}
//printf("Height=%ld DPI=(%ld/%ld) Height=(%ld/%ld)\n", fs->fs_PointHeight, xdpi, ydpi, fs->fs_EMHeight, pointheight);
	return fs;
}


void PUBLIC 
FreeFontInfo(REG(a0, struct FontInfo *fi))
{
	//D(bug("freefont: %lx, locked = %ld\n",fi,fi ? fi->fi_Locked : 0));
	if (!fi || --fi->fi_Locked > 0)
		return;

	MyRemove(fi);
	FreeFontSize(fi->fi_FontSize);
	FreePooled(pool,fi,sizeof(struct FontInfo));
}

struct FontInfo * PUBLIC ChangeFontInfoA(REG(a0, struct FontInfo *ofi), REG(d0, ULONG thisdpi), REG(a1, struct TagItem *ti), REG(d1, UBYTE freeref))
{
	struct FontInfo *fi = NULL, *sfi = NULL;
	struct TagItem *tstate;
	struct Node *family;
	struct FontSize *fs;
	ULONG  height;
	ULONG  style;
	LONG   space, embolden;
	WORD   rotate, shear;
	UBYTE  kerning;
	uint8 i;

//printf("FC dpi=%ld (0x%08X / 0x%08X)\n", thisdpi, thisdpi, ~0L);
	ObtainSemaphore(&fontSemaphore);

	/* Setzen der Standardwerte (anhand der vorgegebenen Struktur oder Defaults) */

	//D(bug("changefont: %lx, size = %lx, freeref = %s\n",ofi,ofi ? ofi->fi_FontSize : NULL,freeref ? "true" : "false"));
																								
	if (ofi)
	{
		/* Set pre-defined attributes */

		height = (fs = ofi->fi_FontSize)->fs_PointHeight;
		embolden = fs->fs_BasicEmboldenX;
		shear = fs->fs_BasicShear;
		rotate = fs->fs_Rotate;
		family = ofi->fi_Family;
		style = ofi->fi_Style;
		space = ofi->fi_CharSpace;
		kerning = ofi->fi_Kerning;

		if (thisdpi == ~0L)
			thisdpi = fs->fs_DPI;
	}
	else
	{
		height = stdpointheight;
		embolden = 0;
		shear = 0;
		rotate = 0;
		family = stdfamily;
		style = FS_PLAIN;
		space = 0;
		kerning = FK_NONE;

		if (thisdpi == ~0L)
			thisdpi = gDPI;
	}

	tstate = ti;
	while ((ti = NextTagItem(&tstate)) != 0)   /* Change attributes */
	{
		if (ti->ti_Data == ~0L)
			continue;

		switch (ti->ti_Tag)
		{
			case FA_Family:
				family = (struct Node *)ti->ti_Data;
				break;
			case FA_PointHeight:
				height = ti->ti_Data;
				break;
			case FA_Style:
				style = ti->ti_Data;
				break;
			case FA_Space:
//        space = ((long)ti->ti_Data*(long)(thisdpi >> 16)+36)/72;
				space = ti->ti_Data;
				break;
			case FA_Rotate:
				rotate = ti->ti_Data;
				break;
			case FA_Shear:
				shear = ti->ti_Data;
				break;
			case FA_Kerning:
				kerning = ti->ti_Data;
				break;
/*      case FA_FreeReference:
				freeref = ti->ti_Data ? TRUE : FALSE;
				break;*/
			default:
			    break;
		}
	}

	/* unter allen bestehenden FontInfos passende heraussuchen */
// 	for(i = 0, fi = (struct FontInfo *)infos.mlh_Head; ((struct MinNode *)fi)->mln_Succ && i < count; fi = (struct FontInfo *)((struct MinNode *)fi)->mln_Succ, i++)
 	foreach (&infos, fi)
	{
		fs = fi->fi_FontSize;

		if (fs->fs_DPI == thisdpi
			&& fi->fi_Family == family
			&& (fi->fi_Style & FS_STYLE_MASK) == (style & FS_STYLE_MASK)
			&& fs->fs_PointHeight == height
			&& fs->fs_Rotate == rotate
			&& fs->fs_BasicShear == shear
			&& fs->fs_BasicEmboldenX == embolden)
		{
			sfi = fi;
			if (style == fi->fi_Style && space == fi->fi_CharSpace && kerning == fi->fi_Kerning)
				break;
		}
	}

	TRACE(("search for style: %ld (0x%08lx)\n", style, sfi));

	if (fi->fi_Node.mln_Succ)     /* this one fits */
	{
		TRACE(("found font that fits: style = %ld\n", fi->fi_Style));
		fi->fi_Locked++;

		if (freeref && ofi)
		{
			FreeFontInfo(ofi);
		}

		ReleaseSemaphore(&fontSemaphore);
		return fi;
	}

	if (freeref && ofi && ofi->fi_Locked == 1)  /* the previous one has only been used once, and can be changed */
	{
		FreeFontSize(ofi->fi_FontSize);
		fi = ofi;  ofi = NULL;
	}
	else if ((fi = AllocPooled(pool, sizeof(struct FontInfo))) != 0)
	{
		fi->fi_Locked = 1;
		MyAddTail(&infos, fi);
	}

	if (ofi && freeref)
	{
			FreeFontInfo(ofi);
	}

	if (!fi)
	{
		ReleaseSemaphore(&fontSemaphore);
		return NULL;
	}

	fi->fi_Family = family;
	fi->fi_Style = style;
	fi->fi_CharSpace = space;
	fi->fi_Kerning = kerning;

	if (sfi)
	{
		TRACE(("old font matches with style = %ld\n", sfi->fi_FontSize->fs_Font->fo_Style));
		fi->fi_FontSize = sfi->fi_FontSize;
		fi->fi_FontSize->fs_Locked++;
	}
	else if ((fi->fi_FontSize = fs = GetFontSize(fi, thisdpi, height)))
	{
		TRACE(("got font size with style = %ld\n", fs->fs_Font->fo_Style));

		fs->fs_BasicShear = fs->fs_Shear = shear;
		fs->fs_BasicEmboldenX = fs->fs_EmboldenX = embolden;
		fs->fs_Rotate = rotate;

		if (rotate)
		{
			fs->fs_RotateSin = (LONG)(sin(rotate * PI / 180.0) * 65536.0);
			fs->fs_RotateCos = (LONG)(cos(rotate * PI / 180.0) * 65536.0);
		}
		else
			fs->fs_RotateCos = 0x10000;

		// if the current font is not italic, compute it manually by shearing it
		if ((style & FS_ITALIC) != 0 && (fs->fs_Font->fo_Style & FS_ITALIC) == 0)
		{
			TRACE(("must make italic!\n"));
			shear += FS_ITALIC_ANGLE;
			fs->fs_Shear = shear;
		}

		if (shear)
		{
			fs->fs_ShearSin = (LONG)(sin(shear * PI / 180.0) * 65536.0);
			fs->fs_ShearCos = (LONG)(cos(shear * PI / 180.0) * 65536.0);
		}
		else
			fs->fs_ShearCos = 0x10000;
		
		// if the current font is not bold, embold it manually
		if ((style & FS_BOLD) != 0 && (fs->fs_Font->fo_Style & FS_BOLD) == 0) {
			TRACE(("must make bold\n"));
			fs->fs_EmboldenX += FS_BOLD_FACTOR;
		}
	}

	if (fi->fi_FontSize)
	{
		ReleaseSemaphore(&fontSemaphore);
		return fi;
	}

	FreeFontInfo(fi);
	ReleaseSemaphore(&fontSemaphore);

	return NULL;
}


struct FontInfo * PUBLIC
NewFontInfoA(REG(a0, struct FontInfo *fi), REG(d0, ULONG dpi), REG(a1, struct TagItem *ti))
{
	return ChangeFontInfoA(fi, dpi, ti, FALSE);
}

#ifdef __amigaos4__
struct FontInfo * VARARGS68K NewFontInfo(struct FontInfo *fi, ULONG dpi, ...);
struct FontInfo *NewFontInfo(struct FontInfo *fi, ULONG dpi, ...)
#else
struct FontInfo *NewFontInfo(struct FontInfo *fi, ULONG dpi, ULONG tag, ...)
#endif
{
#ifdef __amigaos4__
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, dpi);
	tags = va_getlinearva(ap, struct TagItem *);
//	return NewFontInfoA(fi, dpi, tags);
	return ChangeFontInfoA(fi, dpi, tags, FALSE);
#else
	return ChangeFontInfoA(fi, dpi, (struct TagItem *)&tag, FALSE);
#endif
}


struct FontInfo * PUBLIC
SetFontInfoA(REG(a0, struct FontInfo *ofi), REG(d0, ULONG thisdpi), REG(a1, struct TagItem *ti))
{
	return ChangeFontInfoA(ofi, thisdpi, ti, TRUE);
}


/** @short creates/changes a FontInfo structure
 *  @varargs SetFontInfo
 *
 *  With this function you can create or change an existing FontInfo structure.\n
 *
 *  If you specify a reference FontInfo, this will be used as a base for all
 *  changes. If you want this reference to be changed directly or be freed after
 *  the changes have been applied (you are already the owner of this FontInfo structure),
 *  you can achieve this by setting FA_FreeReference to TRUE.
 *
 *  @tag FA_Family (struct Family *) - pointer to the font Family
 *  @tag FA_PointHeight (FIXED) - height of the font
 *  @tag FA_Style (ULONG) - the FS_xxx style flags
 *  @tag FA_FreeReference (BOOL) - if set to TRUE, the reference FontInfo will be freed or
 *         changed directly.
 *
 *  @param ofi the reference FontInfo structure
 *  @param thisdpi gewünschte Auflösung in DotsPerInch (16 Hi-Bit: X, 16 Lo-Bit: Y)
 *  @param tag Tag-Liste
 *
 *  @return fi die neue FontInfo oder NULL
 *
 *  @see FreeFontInfo()
 */

#ifdef __amigaos4__
struct FontInfo * VARARGS68K SetFontInfo(struct FontInfo *ofi, ULONG dpi, ...);
struct FontInfo *SetFontInfo(struct FontInfo *ofi, ULONG dpi, ...)
#else
struct FontInfo *SetFontInfo(struct FontInfo *ofi, ULONG dpi, ULONG tag, ...)
#endif
{
#ifdef __amigaos4__
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, dpi);
	tags = va_getlinearva(ap, struct TagItem *);
	return ChangeFontInfoA(ofi, dpi, tags, TRUE);
#else
	return ChangeFontInfoA(ofi, dpi, (struct TagItem *)&tag, TRUE);
#endif
}


struct FontInfo * PUBLIC
CopyFontInfo(REG(a0, struct FontInfo *fi))
{
	if (!fi)
		return NULL;

	fi->fi_Locked++;

	return fi;
}


void
FreeFamily(struct FontFamily *ff)
{
	FreeString(ff->ff_Node.ln_Name);
	FreePooled(pool,ff,sizeof(struct FontFamily));
}


void
FreeFonts(void)
{
	struct FontFamily *ff;
	struct Font *fo;

	while ((ff = (struct FontFamily *)MyRemHead(&families)) != 0)
		FreeFamily(ff);
	while (!IsListEmpty((struct List *)&infos))
		FreeFontInfo((struct FontInfo *)infos.mlh_Head);

	while ((fo = (struct Font *)MyRemHead(&fonts)) != 0)
	{
		FreePooled(pool, fo->fo_Tags, GetTagData(OT_FileIdent, 0, fo->fo_Tags));
		if (fo->fo_Engine)
		{
#ifdef __amigaos4__
            ECloseEngine(fo->fo_Engine);
            if ((fo->fo_Engine)->ege_IBullet)
                DropInterface((struct Interface *)fo->fo_Engine->ege_IBullet);
			CloseLibrary(fo->fo_Engine->ege_BulletBase);
			FreeVec(fo->fo_Engine);
#else
			BulletBase = fo->fo_Engine->gle_Library;
			CloseEngine(fo->fo_Engine);
			CloseLibrary(BulletBase);
#endif
		}
		FreePooled(pool, fo, sizeof(struct Font));
	}
}


void
GetFonts(struct MinList *list, STRPTR dir, BOOL addfont)
{
#ifdef __amigaos4__
	struct AnchorPath *ap;
#else
	struct AnchorPath ALIGNED ap;
#endif
	struct TagItem *otag;
	struct FontFamily *ff;
	struct Font *fo;
	BPTR   current,font;
	long   rc,i;
	char   *t,file[256];

#ifdef __amigaos4__
	ap = AllocDosObjectTags(DOS_ANCHORPATH, ADO_Mask, SIGBREAKF_CTRL_C,	ADO_Strlen, 1024L, TAG_END ); 
#else
	memset((char *)&ap, 0, sizeof(struct AnchorPath));
#endif
	if (!(current = Lock(dir, ACCESS_READ)))
		return;


#ifdef __amigaos4__
	current = SetCurrentDir(current);

	for (rc = MatchFirst("#?.otag", ap); !rc; rc = MatchNext(ap))
	{
		if ((font = Open(ap->ap_Info.fib_FileName, MODE_OLDFILE)) != 0)
		{
			if ((otag = AllocPooled(pool, ap->ap_Info.fib_Size)) != 0)
			{
				Read(font, otag, ap->ap_Info.fib_Size);
#else
	current = CurrentDir(current);

	for (rc = MatchFirst("#?.otag", &ap); !rc; rc = MatchNext(&ap))
	{
		if ((font = Open(ap.ap_Info.fib_FileName, MODE_OLDFILE)) != 0)
		{
			if ((otag = AllocPooled(pool, ap.ap_Info.fib_Size)) != 0)
			{
				Read(font, otag, ap.ap_Info.fib_Size);
#endif
				for (i = 0; (otag + i)->ti_Tag != TAG_END; i++)
				{
					// OTAG fonts are in big endian format
					(otag+i)->ti_Tag = LONG2BE((otag+i)->ti_Tag);
					(otag+i)->ti_Data = LONG2BE((otag+i)->ti_Data);

					if ((otag + i)->ti_Tag & OT_Indirect)
						(otag + i)->ti_Data += (ULONG)otag;
				}
				if (addfont && (fo = AllocPooled(pool, sizeof(struct Font))))
				{
					int32 style = GetTagData(OT_SlantStyle, 1, otag);
					int32 weight = GetTagData(OT_StemWeight, OTS_Medium, otag);
#ifdef __amigaos4__
					int32 conden = GetTagData(OT_HorizStyle, OTH_Condensed, otag);	//Auch verdichtete Schrift beachten, damit sie nicht benutzt wird

					Strlcpy(file,dir, 256);
					AddPart(file, ap->ap_Info.fib_FileName, 256);
#else
					strcpy(file,dir);
					AddPart(file, ap.ap_Info.fib_FileName, 256);
#endif
					fo->fo_Node.ln_Name = AllocString(file);
					// set this font's properties
					if (style == OTS_Italic)
						fo->fo_Style |= FS_ITALIC;
					if (weight >= OTS_DemiBold)
						fo->fo_Style |= FS_BOLD;
#ifdef __amigaos4__
					if (conden <= OTH_Condensed)
						fo->fo_Style |= FS_CONDENSED;
#endif
					//fo->fo_Style = ~0L;
					fo->fo_Tags = otag;
//printf("add font: %s / %s , style = %ld\n", file, ap->ap_Info.fib_FileName, fo->fo_Style);

					TRACE(("add font: %s, style = %ld\n", file, fo->fo_Style));
					MyAddTail(&fonts, fo);
				}

				if (!(t = (STRPTR)GetTagData(OT_Family, 0, otag)))
				{
#ifdef __amigaos4__
					Strlcpy(t = file, ap->ap_Info.fib_FileName, 256);
#else
					strcpy(t = file, ap.ap_Info.fib_FileName);
#endif
					if ((i = strlen(file)) > 5)
						file[i - 5] = 0;
				}

				if (!(ff = (APTR)MyFindName(list, t)))
				{
					TRACE(("family = %s\n", t));

					if (t && (ff = AllocPooled(pool, sizeof(struct FontFamily))))
					{
						ff->ff_Node.ln_Name = AllocString(t);
						MyAddTail(list, ff);
					}
				}
			}
			Close(font);
		}
	}
#ifdef __amigaos4__
	MatchEnd(ap);
	FreeDosObject(DOS_ANCHORPATH,ap);
	current = SetCurrentDir(current);
#else
	MatchEnd(&ap);
	current = CurrentDir(current);
#endif

	UnLock(current);

	sortList(list);
}


void
AddFontPath(STRPTR path)
{
	struct Node *ln;

	if (!path)
		return;

	if ((ln = AllocPooled(pool,sizeof(struct Node))) != 0)
	{
		ln->ln_Name = AllocString(path);
		MyAddTail(&fontpaths, ln);
	}
}


void 
SearchFonts(void)
{
	struct Node *ln;

	MyNewList(&families);

	if (IsListEmpty((struct List *)&fontpaths)) {
		struct DosList *list = LockDosList(LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);

		AddFontPath("FONTS:");
		
		if (list) {
			if (FindDosEntry(list, "MOSSYS", LDF_ASSIGNS | LDF_VOLUMES) != NULL)
				AddFontPath("MOSSYS:Fonts");
			
			UnLockDosList(LDF_ASSIGNS | LDF_VOLUMES | LDF_READ);
		}
	}

	foreach (&fontpaths, ln)
		GetFonts(&families, ln->ln_Name, TRUE);
}
