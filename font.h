/*
 * Copyright ©1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_FONT_H
#define IGN_FONT_H

struct FontFamily {
	struct Node ff_Node;
};

struct Font {
	struct Node fo_Node;
#ifdef __amigaos4__
	struct EGlyphEngine *fo_Engine;
#else
	struct GlyphEngine *fo_Engine;
#endif
	long   fo_Style;
	long   fo_Space;
	struct TagItem *fo_Tags;
};

struct FontInfo {
	struct MinNode fi_Node;
	struct FontSize *fi_FontSize;
	struct Node *fi_Family;
	LONG   fi_CharSpace;
	long   fi_Style;
	UBYTE  fi_Kerning;
	long   fi_Locked;
};

#ifdef __amigaos4__
	#define FS_STYLE_MASK 15
	#define FS_PLAIN 0         /* Font-Style */
	#define FS_UNDERLINED 32
	#define FS_BOLD 2
	#define FS_ITALIC 4
	#define FS_CONDENSED 8
#else
	#define FS_STYLE_MASK 3
	#define FS_PLAIN 0         /* Font-Style */
	#define FS_BOLD 1
	#define FS_ITALIC 2
	#define FS_UNDERLINED 4
#endif
#define FS_DOUBLE_UNDERLINED 8
#define FS_STRIKE_THROUGH 16
#define FS_ALLBITS 32768
#define FS_UNSET 65536

#define FS_ITALIC_ANGLE 20
#define FS_BOLD_FACTOR 0x1000

#define FK_NONE 0          /* Kerning */
#define FK_TEXT 1
#define FK_DESIGN 2

#define FA_PointHeight  TAG_USER+100  /* Tags for SetFontInfo() */
#define FA_Space        TAG_USER+101
#define FA_Style        TAG_USER+102
#define FA_Rotate       TAG_USER+103
#define FA_Kerning      TAG_USER+104
#define FA_Family       TAG_USER+105
#define FA_Shear        TAG_USER+106
#define FA_Embolden     TAG_USER+107  /* not yet implemented */
#define FA_Width        TAG_USER+108  /* not yet implemented */

#define FA_FreeReference TAG_USER+120


struct FontSize {
	struct FontChar *fs_CharsArray[256];	// all character codes from 0 to 256
	struct MinList fs_Chars;				// all codes > 256
	struct Font *fs_Font;
	ULONG  fs_DPI;
	ULONG  fs_PointHeight;
	ULONG  fs_EMWidth;
	ULONG  fs_EMHeight;
	WORD   fs_EMTop;
	WORD   fs_Space;
	WORD   fs_Rotate;
	WORD   fs_Shear, fs_BasicShear;
	ULONG  fs_RotateSin,fs_RotateCos;
	ULONG  fs_ShearSin,fs_ShearCos;
	ULONG  fs_EmboldenX, fs_BasicEmboldenX;
	APTR   fs_Pool;
	long   fs_Locked;
};

#define FS_POOLSIZE 8192

struct FontChar {
	struct MinNode fc_Node;
	UWORD  fc_Code;
	struct GlyphMap fc_Glyph;
};

/*************************** Prototypes ***************************/

extern void SearchFonts(void);
extern void GetFonts(struct MinList *list,STRPTR dir,BOOL addfont);
extern void AddFontPath(STRPTR path);
extern void FreeFonts(void);
extern void GetFontChars(struct FontSize *,STRPTR);
extern void PUBLIC DrawTextWithWidth(REG(a0, struct RastPort *rp),REG(a1, struct FontInfo *fi),REG(a2, STRPTR t),REG(d0, long x),REG(d1, long y),REG(d2, long width));
extern void PUBLIC DrawText(REG(a0, struct RastPort *rp),REG(a1, struct FontInfo *fi),REG(a2, STRPTR t),REG(d0, long x),REG(d1, long y));
extern ULONG PUBLIC OutlineLength(REG(a0, struct FontInfo *fi),REG(a1, STRPTR text),REG(d0, long len));
extern ULONG PUBLIC OutlineHeight(REG(a0, struct FontInfo *fi),REG(a1, STRPTR text),REG(d0, long len));
extern void PUBLIC FreeFontInfo(REG(a0, struct FontInfo *fi));
extern struct FontInfo * PUBLIC NewFontInfoA(REG(a0, struct FontInfo *fi),REG(d0, ULONG dpi),REG(a1, struct TagItem *ti));
#ifdef __amigaos4__
	extern struct FontInfo *NewFontInfo(struct FontInfo *fi,ULONG dpi,...) VARARGS68K;
#else
	extern struct FontInfo *NewFontInfo(struct FontInfo *fi,ULONG dpi,ULONG tag,...) VARARGS68K;
#endif
	extern struct FontInfo * PUBLIC SetFontInfoA(REG(a0, struct FontInfo *fi),REG(d0, ULONG dpi),REG(a1, struct TagItem *ti));
#ifdef __amigaos4__
	extern struct FontInfo *SetFontInfo(struct FontInfo *fi,ULONG dpi,...) VARARGS68K;
#else
	extern struct FontInfo *SetFontInfo(struct FontInfo *fi,ULONG dpi,ULONG tag,...) VARARGS68K;
#endif
extern struct FontInfo * PUBLIC CopyFontInfo(REG(a0, struct FontInfo *fi));

#endif   /* IGN_FONT_H */
