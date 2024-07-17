/* ignition TurboCalc-IO-Modul
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef TURBOCALC_H
#define TURBOCALC_H

//Turbocalc starts with 1.1.1900 the utility-date-functions with 1.1.1978 this is the correction in day
#define DATECORR       28489

#define FA_PointHeight  TAG_USER+100  /* Tags for SetFontInfo() */
#define FA_Space        TAG_USER+101
#define FA_Style        TAG_USER+102
#define FA_Rotate       TAG_USER+103
#define FA_Kerning      TAG_USER+104
#define FA_Family       TAG_USER+105
#define FA_Shear        TAG_USER+106
#define FA_Embolden     TAG_USER+107  /* not yet implemented */
#define FA_Width        TAG_USER+108  /* not yet implemented */

struct THeader {
	UBYTE th_Type;
	UBYTE th_Size[2];
};

enum {
	FILE_END,
	FILE_START,			/* 12:TURBOCALC\8, floatmode, 0.b */
	FILE_VERSION,		/* 2:(version, revision): see above! */
	FILE_PASSWORD,		/* flags.b (<>0: ask_password), flags2.b 2+3*txt
						 * pwopenread_txt
						 * pwwriteread_txt
						 * pwchange_txt */
	FILE_OPT0,			/* 6:stdwidth.w, stdheight.w, flags1.b 0.b */
	FILE_OPT1,			/* 8+2_txt: decimalpoint.b, 1000sep.b, datumsep.b, waehrung.b
						 * sheet_waehrungprefix_txt, sheet_waehrungsuffix_txt
						 * dateorder.b, localeflags.b, 0.b, 0.b */
	FILE_WIDTH,			/* x.l, width.w only if differs from std! */
	FILE_HEIGHT,		/* y.l, height.w */
	FILE_CELL,			/* x.w,y.w,
						 * type.b,
						 * ieee.l (only for type==FLOAT!) see note at the end
						 * (data1.l, data2.l) - for type==TEXT: text_txt, (skipped for type==EMPTY)
						 * formel_txt (not for type==EMPTY)
						 * format.Format_Size */
	FILE_LCELL,			/* dito, but x.l, y.l */
	FILE_NAME,			/* name_txt
						 * typ.b
						 * TYPE_TEXT: text_txt
						 * TYPE_FORMEL_x: formel_txt
						 * else: data1.l, data2.l */
	FILE_OPT2,			/* print options: 42+3*txt!
						 * printflags.b, printflags2.b
						 * printLM.w, RM.w, UM.w, BM.w
						 * printwidth.w, printheight.w
						 * printfile_txt, printheader_txt, printfooter_txt
						 * sheet_titlex0.l, sheet_titlex1.l, sheet_titley0.l, sheet_titley1.l
						 * printflags3.b, 0.b, 0.b, 0.b
						 * scalex.w, scaley.w, 0.w, 0.w */
	FILE_WINDOW,		/* add this at the end of the file!!!
						 * 14.w: x1.w, y1.w, x2.w, y2.w
						 * flags.b, pad.b, pad.l */
	FILE_FONTS,			/* 31 times: (font_txt, size.w, pad.w | or 0.w for unused) */
	FILE_SCREEN,		/* flag.b (0=off, 1=wb-clone, 2=on)
						 * width.w,height.w, depth.w, id.l, smartrefresh, pad.b */
	FILE_COLOR,			/* num.w
						 * r.b,g.b,b.b,pad.b */
	FILE_DIAGRAM,		/* (72+2*DIAGRAM_MAXY+(4+2*DIAGRAM_MAXY)_txt+6_fnt)
						 * x1.w, <1.w, x2.w, y2.w - window-dim
						 * winflags.b (bit0: hidden), pad.b
						 * name_txt
						 * x1.l, y1.l, x2.l, y2.l - data-range
						 * type.b, realtype.b, flags2.b, flags.b
						 * min.l, min2.l
						 * printflags.b, printpad.b, printwidth.w, printheight.w
						 * max.l, max2.l (sorry, min & max should be together, but...)
						 **
						 * titleflags.b, pad.b
						 * title_txt, title_font style.b pad.b *3
						 **
						 * legendeflags.b, style.b legende_fnt
						 * DIAGRAM_MAXY*legende_txt
						 **
						 * patternflags.b, pad.b
						 * colorpattern.w * DIAGRAM_MAXY
						 **
						 * achseflags.b, style.b achse_fnt
						 * DIAGRAM_MAXY*achse_txt
						 **
						 * yachseflags.b, yachsepad.b, yachse_fnt
						 * yachseticknum.w, yachsesubticknum.w */
	FILE_STDFONTS,		/* font_txt, size.w, pad.w | or 0.w for unused */
	FILE_PATTERNS,		/* 4*PATTERNMAXSIZE +_txt
						 * std_pfad_txt
						 * pattern0.4*PATTERNMAXSIZE */
	FILE_COLUMNFLAGS,
	FILE_ROWFLAGS,		/* 12,row/column.l, Format.Format_Size */
	FILE_SHEETSIZE,		/* 12:sheet_limitx/y.l, pad.l */
	FILE_SYSTEMFONTS,	/* font_txt, size.w, pad.w	 or 0.w for unused for text
						 * font_txt, size.w, pad.w	 or 0.w for unused for menu */
	FILE_FROZEN,		/* V3! - should be immedialty after FILE_WINDOW!!!
						 * 24,freezex,freezey,freezepix,freezepiy,0,0	 - all .l
						 * this is only written, if there is freezing!!! */
	FILE_SAVEOPT,		/* 20+txt,saveflags.b,0.b, 0.w autosavetime.w, 0.w, 0.l
						 * sheet_autosavepath_txt, 0.l, 0.l */
	FILE_CRYPT,			/* checkkey.w
						 * flags0,1.b	- reserved
						 * long0,long1 for padding (filled with scratch!) */
	FILE_DIAGRAM2,		/* dia_legendex1..y2 (16)
						 * dia_achsex1..y2	 (16)
						 * symbolmode.b, pad.b, width.w height.w, pad.w (8)
						 * kuchenexplode.b (MAXEXPLODE)
						 * iffdepth/width/height.w	(6)
						 * refreshflags0/1.b (2)
						 * 2*pad.l (8)
						 ** 3.10: (FEATURE_CHARTXY)
						 * xachseflags, xachsestyle, xachseticknum, xachsesubticknum (6)
						 * xmin.l, xmin2.l, xmax.l, xmax2.l	(16) */
	FILE_GLOBALFLAGS,	/* GlobalFlags.l (0..3), GlobalFlags_autoopendir_txt, 0.w
						 * Preview_flags.b 0.b, Preview_Width/Height/Depth.w, Preview_screenmode */
	FILE_OBJECT,		/* (36+3*txt)
						 * x1,y1,x2,y2.l (16)
						 * objecttype.l (-1=extern) (4)
						 * objectclass_txt, name_txt, macro_txt (3*txt)
						 * backcolor, pattern, frame, pad.b (as long! 4)
						 * flags0,1,2,3.b (as long! 4)
						 * pad1.l, pad2.l (8) */
	FILE_STDCHART,		/* (2+Stdchart_save_firstfont+3*fnt)
						 * reserved.w (2)
						 * StdChart (StdChart_save_firstfont)
						 * fonts (3*fnt) */
	FILE_OPT3,			/* (26)
						 * sheet_printiffdept/with/height.w, undo_depth.w, printiff_scale (10)
						 * 0.l, 0.l, 0.l, 0.l (16) */
	FILE_LASTFILES,		/* only used for 'lastfiles'
						 * flags0/1/2/3.b (4)
						 * filename_txt */
	FILE_CURSOR,		/* (28) immediatly after FILE_WINDOW (or _FROZEN)
						 * cellx/y.l
						 * blockx1/y1/x2/y2.l (if blockx1=-1.l, then no block selected!)
						 * flags0,1,2,3.b (reserved) */
	FILE_STARTUPOPTIONS,	/* these options *must* follow immediatly
							 * after FILE_START & will be checked
							 * by the File_CheckType-routine!
							 * flags0-7	; (8) reserved up to now! */
	FILE_TURBOCALCOWNER,	/* (12+txt)
							 * reserved.l (0)				;
							 * tc-serial-id_txt[6]	 ; depends on TC-version!!!
							 * username_txt					;
							 * reserved.w (0)		; reserved for company-name! */
	FILE_FILEINFO,		/* (20 + 4*txt)
						 * reserved.l (0)
						 * creation_date.l
						 * creation_time.l
						 * version.l
						 * workingtime.l
						 * author_txt
						 * title_txt
						 * subject_txt */
	FILE_TABPAGE
} TCTypes;


/*
** ..._txt: num.w txt.num
** ..._font: fontname_txt [size.w pad.w] (only if fontname-size<>0!)
*/

/************************** FILE_CELL **************************/

/*struct TCellHeader
{
	UWORD tc_Col,tc_Row;
	UBYTE tc_Type;
	union
	{
		struct {
			UBYTE tc_TextLength[2];
			char	tc_Text[0]
		};
	};
	UBYTE tc_FormelLength[2];
	char	tc_Formel[0];
};

struct TLCellHeader
{
	ULONG tc_Col,tc_Row;
	UBYTE tc_Type;
};
*/
struct TCell
{
	UBYTE tc_Color0;		 /* 0 = std., values 1-63 */
	UBYTE tc_Color1;		 /* "										 */
	UBYTE tc_Border;
	UBYTE tc_Font;			 /* 0 = std., bits 0-2: style, bits 3-7: font number, see FILE_FONTS */
	BYTE tc_TextFormat; /* 0 = std., -1 (only for row & column_format): invalid format! */
	UBYTE tc_Flags;
	UBYTE tc_Label[6];	 /* ???? */
};

#define TC_COLOR_MASK 0x3f
#define TC_PATTERN_MASK 0xc0
#define TC_PATTERN_LSHIFT 6
#define TC_BORDER_LEFT 0x03		 /* 00=off, 01= thin, 10 = medium, 11= bold */
#define TC_BORDER_RIGHT 0x0c
#define TC_BORDER_UP 0x30
#define TC_BORDER_DOWN 0xc0
#define TC_FONT_LSHIFT 3
#define TC_STYLE_ITALIC 2
#define TC_STYLE_BOLD 1
#define TC_STYLE_UNDERLINED 0
#define TC_STYLE_MASK 0x07
#define TCF_HIDDEN = 7
#define TCF_PROTECTED = 6
#define TCF_HIDEFORMEL = 5
#define TCF_VALIGNMENT 0x18	/* 00=std, 01=upper, 10=mid, 11=lower */
#define TCF_ALIGNMENT 0x07	 /* 000=std (nums/date/time right, text left, others: centered)
														 ** 001=left, 010=right, 011=centered
														 ** 100=repeated(ni), 101/110/111=multiline_alignment!!! */

#define TCT_EMPTY 0
#define TCT_NO 1		 /* empty cell */
#define TCT_FLOAT 2
#define TCT_NUM 3
#define TCT_INT TCT_NUM
#define TCT_DATE 4
#define TCT_TIME 5
#define TCT_BOOL 6
#define TCT_TEXT 7	 /* d0=*text (or 0, then is empty text!!!) */
#define TCT_ERROR 9

/******************************************************************
**
** Note on ieee of FILE_CELL:
**
** TurboCalc can use all of the Amiga's float-formats.
** (Depending on floatmode in FILE_START)
** Thus TC stores floats in two ways:
** 1) as single ieee (32bit) [which can be converted by all float-formats]
** 2) and in the currently used float-format (double ieee, if available)
**		This is always stored as 64 bit (data1/2)
**
** floatmode==0 -> double ieee.
** floatmode<>0 -> use single ieee (as other format is less precise)
**
*/

/************************** FILE_VERSION **************************/

struct TVersion
{
	UBYTE tv_Version;
	UBYTE tv_Revision;
};

#define TC_VERSION 1
#define TC4_REVISION 9
#define TC5_REVISION 10

#endif	/* TURBOCALC_H */

