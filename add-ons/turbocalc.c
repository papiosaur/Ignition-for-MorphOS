/* ignition TurboCalc-I/O-Module
 *
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#ifdef __amigaos4__
	#include <proto/exec.h>
	#include <proto/utility.h>
	#include <proto/locale.h>
	#include <ctype.h>
#else
	#include <stdio.h>
#endif

#include "iotype.h"
#include "turbocalc.h"


extern APTR ioBase;

#ifdef __amigaos4__
	#undef  strlen
	#define strlen  Strlen
#endif

const STRPTR __version = "$VER: turbocalc.io 0.5 (13.02.2022)";

#ifdef __amigaos4__
	#define bug DebugPrintF
	#define D(x); //x;
	//#define DEBUG_OS4 
#else
	/*extern sprintf(STRPTR,STRPTR,...);*/
	extern kprintf(STRPTR,...);

	#define bug kprintf
	#define D(x) ;
#endif

STRPTR
MapTCFuncs(UBYTE n)
{
	D(bug("MapTCFuncs: %02X\n", n));
	switch (n) {
		case 0x01: return("EXP");
		case 0x02: return("LN");
		case 0x03: return("L10");
		case 0x07: return("QRT");
		case 0x08: return("FAK");
		case 0x09: return("Pi");
		case 0x0a: return("SNH");
		case 0x0b: return("CSH");
		case 0x0c: return("TNH");
		case 0x0d: return("SIN");
		case 0x0e: return("COS");
		case 0x0f: return("TAN");
		case 0x10: return("ASN");
		case 0x11: return("ACS");
		case 0x12: return("ATN");
		case 0x13: return("DEG");
		case 0x14: return("RAD");
		case 0x15: return("IF");
		case 0x16: return("SEL");
		case 0x18: return("INT");
		case 0x19: return("INT");
		case 0x1a: return("ABS");
		case 0x1b: return("SGN");
		case 0x1c: return("ROU");
		case 0x1e: return("RAN");
		case 0x1d: return("NOT");
		case 0x22: return("LEF");
		case 0x23: return("RIG");
		case 0x25: return("AVG");
		case 0x26: return("CHR");
		case 0x27: return("LEN");
		case 0x28: return("LWC");
		case 0x29: return("UP2");
		case 0x2a: return("UPC");
		case 0x2c: return("REP");
		case 0x2d: return("TRI");
		case 0x2e: return("CLN");
		case 0x2f: return("VAL");
		case 0x37: return("TNM");
		case 0x3a: return("TDY");
		case 0x3b: return("NOW");
		case 0x3d: return("YER");
		case 0x3e: return("DAY");
		case 0x3f: return("MON");
		case 0x40: return("WKD");
		case 0x4a: return("DNM");
		case 0x43: return("DMI");
		case 0x44: return("DMA");
		case 0x46: return("DSM");
		case 0x47: return("DPT");
		case 0x48: return("DAV");
		case 0x4b: return("MIN");
		case 0x4c: return("MAX");
		case 0x4e: return("SUM");
		case 0x50: return("MID");
		case 0x51: return("AND");
		case 0x52: return("OR");
		case 0x53: return("XOR");
		case 0x55: return("INM");
		case 0x56: return("ITX");
		case 0x57: return("IDT");
		case 0x58: return("ITI");
		case 0x59: return("IEC");
		case 0x5e: return("CAI");
		case 0x5f: return("CAP");
		case 0x60: return("TRM");
		case 0x61: return("CZV");
		case 0x62: return("CRE");
		case 0x63: return("CRI");
		case 0x64: return("CRR");
		case 0x73: return("MIR");
		case 0x74: return("SHL");
		case 0x75: return("SHR");
		case 0x7b: return("VAR");
		case 0x84: return("GCD");
		case 0xa0: return("THR");
		case 0xa1: return("TMN");
		case 0xa2: return("TSC");
		case 0xb8: return("PAT");
		default:
			DebugPrintF("------->Code: 0x%02X\n",n);
			return("NOP");
	}
	return NULL;
}


STRPTR
MapTCCellFormat(UBYTE n)
{
	D(bug("MapTCCellFormat: 0x%02X\n", n));
	switch (n) {
		case 0x1A	: return(AllocString("0 ¤"));
		case 0x1E	: return(AllocString("#D.#M.#Y"));
		case 0x1F	: return(AllocString("#D.#M.#y"));
		case 0x21	: return(AllocString("#d. %m, #y"));
		case 0x25	: return(AllocString("#H:#M:#S"));
		case 0x26	: return(AllocString("#H:#M"));
		default		: return(NULL);
	}
}

#ifdef __amigaos4__
/*
Check the Formulastring for the @ sign and count them.
*/
int CountPageLink(char *formula)
{
	int i, c = 0;

	for(i = 0; formula[i]; i++)
		if(formula[i] == '@')
			c++;
	return c;
}

/*
Check the Formulastring for the a double @ and then return TRUE else FALSE.
*/
int CheckDoubleAT(char *formula)
{
	int i = 0;

	for(i = 0; formula[i]; i++)
		if(formula[i] == '@' && formula[i + 1] == '@')
			return 1;
	return 0;
}

/*
Converts the Turbocalc-Format or a page link to the ignition
one.
*/
STRPTR ConvertPageLinks(char *dest, int plc)
{
	int i1, i2;
	STRPTR old = AllocString(dest);
	struct Locale *loc;													//Localepointer

	loc = OpenLocale(NULL);						//Map to locale behaviour
//DebugPrintF("len=%ld\n", Strlen(dest));
//DebugPrintF("AltText=<%s>\n", dest);
	i1 = Strlen(old) - 1;
	i2 = Strlen(dest) - 1 + plc;
//DebugPrintF("len1=%ld len2=%ld\n", i1, i2);
	while(i1 >= 0)									//go throw the string
	{
//DebugPrintF("c=%c\n", old[i1]);
		if(old[i1] == '@')
		{
			dest[i2--] = old[i1--];
			while(i1 && (IsAlNum(loc, old[i1]) || old[i1] == ' '))
				dest[i2--] = old[i1--];
			dest[i2--] = '@';
		}
		else
			dest[i2--] = old[i1--];
	}
//DebugPrintF("schleife:<%s> plc =%ld i =%ld\n", dest, plc, i);
	CloseLocale(loc);							//Free pointer
	FreeString(old);
//DebugPrintF("NeuText=<%s>\n", dest);
	return AllocString(dest);
}
#endif

void
ConvertTCFormula(struct Cell *c,STRPTR t,long len)
{
	long i,col,row;
	BYTE a,b;
	STRPTR dest;
#ifdef __amigaos4__
	BYTE fct = 0;
	int plc;
#endif

	D(bug("ConvertTCFormula: <%s> len=%d\n", t, len));
	if ((dest = AllocPooled(pool, len * 3)) != 0) {
#ifdef __amigaos4__
		ClearMem(dest, len * 3);
#endif
		for(i = 0;i < len;i++) {
			if (*(t+i) >= 0x08 && *(t+i) <= 0x0b) {
				/* relative & absolute references */
				col = *(WORD *)(t+i+1);	row = *(WORD *)(t+i+3);
				a = *(t+i) == 0x0a || *(t+i) == 0x08;
				b = *(t+i) == 0x09 || *(t+i) == 0x08;
				D(bug("col: %d row: %d\n", col, row));
				D(bug("Coord2String-Para: %d %d %d %d\n",a,(a ? 0 : c->c_Col)+col,b,(b ? 0 : c->c_Row)+row ));
#ifdef __amigaos4__
				Strlcat(dest,Coord2String(a,(a ? 1 : c->c_Col)+col,b,(b ? 1 : c->c_Row)+row), len * 2);
#else
				strcat(dest,Coord2String(a,(a ? 1 : c->c_Col)+col,b,(b ? 1 : c->c_Row)+row));
#endif
				i += 4;
#ifdef __amigaos4__
			} else if (*(t+i) == 0x04 || *(t+i) == 0x05) {
#else
			} else if (*(t+i) == 0x04) {
#endif
				/* Function */
				if (MapTCFuncs(*(t+ ++i)))
#ifdef __amigaos4__
					{
					Strlcat(dest,MapTCFuncs(*(t+i)), len * 2);
					if(*(t+i) != 0x09) //some fct in tc work without () ign needs them. but pi not!
						fct = 1;
					}
#else
					strcat(dest,MapTCFuncs(*(t+i)));
#endif
			} else if (*(t+i) < ' ') {
				D(bug("unknown: %ld\n",*(t+i)));
				break;
			} else {
				/* standard text */
				dest[strlen(dest)] = *(t+i);
				D(bug("Text: <%s>\n", dest));
			}
		}
		if (c->c_Text) {
			// bug("ctcf: old text: '%s'\n",c->c_Text);
			FreeString(c->c_Text);
		}
#ifdef __amigaos4__
		if(fct && dest[strlen(dest) - 1] != ')' &&  (dest[strlen(dest) - 1] < 0x30 || dest[strlen(dest) - 1] > 0x39))
		{
			Strlcat(dest, "()", len * 2);
		//	fct = 0;
		}
		if(plc = CountPageLink(dest))
		{
			c->c_Text = ConvertPageLinks(dest, plc);
			if(CheckDoubleAT(c->c_Text))
			{
				c->c_Text[0] = '#'; //wrong ext link format
			}
		}
		else
#endif
			c->c_Text = AllocString(dest);
			FreePooled(pool,dest,len*3);
	}
}


long PUBLIC
load(REG(d0, BPTR dat), REG(a0, struct Mappe *mp))
{
	struct Cell *c;
	struct Page *page;
	struct THeader th;
	struct TCell *tc;
	APTR buf = NULL;
	ULONG len = 0,size,l,col,row,*cols;
	char t[64];
	double d;
	UWORD w;
	UWORD cols_size = 0;
	UBYTE b,*cell,ieeemode,r,g;
	uint16 spos;//Stringstartposition der Strings in Dateiinfo
	char tabver[3] = {"XX\0"};
	struct ClockData cd;
#ifdef __amigaos4__
	uint8 dateflag;    //TC date is under 1.1.1978
	uint8 colorflag;  

	STRPTR sp;
	long i;
#endif

	page = NewPage(mp);
	while (FRead(dat,&th,sizeof(struct THeader),1)) {
#ifdef __amigaos4__
		size = (th.th_Size[0] << 8) + th.th_Size[1];
#else
		size = (th.th_Size[0] << 16) + th.th_Size[1];
#endif
#ifdef DEBUG_OS4
		DebugPrintF("------->Type: 0x%02X Size: %05d (0x%02X 0x%02X)\n", th.th_Type, size, th.th_Size[0],th.th_Size[1]);
#endif
		if (len < size) {
			if (buf && len)
				FreePooled(pool,buf,len);
			buf = AllocPooled(pool,len = size);
		}
		if (buf) {
			// bug("Type: %ld - Chunklänge: %ld\n",th.th_Type,size);
			FRead(dat,buf,size,1);
			switch (th.th_Type) {
				case FILE_START:
					D(bug("	Filestart\n"));
					ieeemode = *((UBYTE *)buf+11);
					break;
				case FILE_END:
					D(bug("	Fileend.\n"));
#ifdef __amigaos4__
					if(cols)
					{
						foreach(&page->pg_Table,c) {
							if (c->c_APen)
								c->c_APen = c->c_ReservedPen = cols[c->c_APen];
							else
								c->c_APen = c->c_ReservedPen = page->pg_APen;
							if (c->c_BPen)
								c->c_BPen = cols[c->c_BPen];
							else
								c->c_BPen = page->pg_BPen;
							UpdateCellText(page,c);
						}
					}
					if(cols_size)
					{
						FreePooled(pool,cols,cols_size);
						cols_size = 0;
					}
					cols = NULL;
					if(size >0) //If FILE_END has a length, then only the actual page is ready.
					{
						page = NewPage(mp);
					}
#endif
					break;
				case FILE_VERSION:
					D(bug("	Fileversion.\n"));
					//if (((struct TVersion *)buf)->tv_Version > TC_VERSION || ((struct TVersion *)buf)->tv_Version == TC_VERSION&& ((struct TVersion *)buf)->tv_Revision > TC4_REVISION)
					//	ReportError("Warnung!\nDas Dateiformat ist neuer als das bekannte. (%d / %d)", ((uint8 *)buf)[0], ((uint8 *)buf)[1]);
					break;
				case FILE_PASSWORD:
					D(bug("	Password.\n"));
					break;
				case FILE_OPT0:
					D(bug("	Optionen-0.\n"));
					break;
				case FILE_OPT1:
					D(bug("	Optionen-1.\n"));
					break;
				case FILE_WIDTH:
					D(bug("	File Width\n"));
					l = *(ULONG *)(cell = buf);	w = *(UWORD *)(cell+4);
					SetTableSize(page,l+1,0);
					(page->pg_tfWidth+l)->ts_Pixel = w;
					(page->pg_tfWidth+l)->ts_mm = mm(page,w,TRUE);
					break;
				case FILE_HEIGHT:
					D(bug("	File height\n"));
					l = *(ULONG *)(cell = buf);	w = *(UWORD *)(cell+4);
					SetTableSize(page,0,l+1);
					(page->pg_tfHeight+l)->ts_Pixel = w;
					(page->pg_tfHeight+l)->ts_mm = mm(page,w,FALSE);
					break;
				case FILE_CELL:
				case FILE_LCELL:
					D(bug("	Cell/LCell-3.\n"));
#ifdef DEBUG_OS4
					DebugPrintF("Type: %02d Size: %05d Data: ", th.th_Type, size);
					for(i = 0; i < size; i++)
						DebugPrintF("%02X ", ((uint8 *)buf)[i]);
					DebugPrintF("\n");
#endif
					cell = buf;
					if (th.th_Type == FILE_CELL)
						col = *(UWORD *)cell, row = *(UWORD *)(cell+2), cell += 4;
					else
						col = *(ULONG *)cell, row = *(ULONG *)(cell+4), cell += 8;
#ifdef DEBUG_OS4
					DebugPrintF("Cell: %c%02d\n", col + 1 + 64, row + 1);
#endif
					if ((c = NewCell(page, col + 1,row + 1)) != 0)
					{
						b = *cell++;
						if (b == TCT_TEXT)
						{
							D(bug("TCT_TEXT\n"));
							w = *(UWORD *)cell;	cell += 2;
							c->c_Text = AllocStringLength(cell,w);
							cell += w;
							c->c_Type = CT_TEXT;
						}
						else if (b != TCT_EMPTY)
						{
							D(bug(" not TCT_EMPTY\n"));
							if (/*ieeemode &&*/ b == TCT_FLOAT)
								cell += 4;
							d = *(double *)cell;
							l = *(long *)cell;
							cell += 8;
						}
						switch(b)
						{
							case TCT_TIME:
								D(bug("TCT_TIME\n"));
#ifdef __amigaos4__
								sp = ASPrintf("%ld:%02ld:%02ld",(l/3600),((l/60) % 60),(l % 60));
								c->c_Text = AllocString(sp);
								FreeVec(sp);
#else
								sprintf(t,"%ld:%02ld:%02ld",l/3600,(l/60) % 60,l % 60);
								c->c_Text = AllocString(t);
#endif
								break;
							case TCT_FLOAT:
								D(bug("TCT_FLOAT: %lf\n", d));
								c->c_Text = AllocString(ita(d,-1,ITA_NONE));
								c->c_Type = CT_VALUE;
								break;
							case TCT_INT:
								D(bug("TCT_INT\n"));
#ifdef __amigaos4__
								sp = ASPrintf("%ld",l);
								c->c_Text = AllocString(sp);
								FreeVec(sp);
#else
								sprintf(t,"%ld",l);
								c->c_Text = AllocString(t);
#endif
								break;
							case TCT_DATE:
								D(bug("date: %ld\n",l));

								if(l >= DATECORR)
								{
									l -= DATECORR;
									dateflag = 0;
								}
								else
									dateflag = 1;
								Amiga2Date(l * 24 * 60 * 60, &cd);
								D(bug("Amiga2Date: %02d.%02d.%04d\n", cd.mday, cd.month, cd.year));
#ifdef __amigaos4__
								sp = ASPrintf("%02ld.%02ld.%02ld", (long)cd.mday, (long)cd.month, (long)(dateflag ? cd.year - 78 : cd.year));
								c->c_Text = AllocString(sp);
								FreeVec(sp);
#else
								sprintf(t,"%02d.%02d.%04d", cd.mday, cd.month, (dateflag ? cd.year -78 : cd.year));
								c->c_Text = AllocString(t);
#endif
								c->c_Type = CT_TEXT;
								break;
						}
						D(bug("Celltext: %s\n",c->c_Text));
						if (b != TCT_EMPTY)
						{
							w = *(UWORD *)cell;	cell += 2;
							D(bug("Pos: %d\n",(long)cell - (long)buf));
							ConvertTCFormula(c,cell,w);
							cell += w;
						}
						tc = (struct TCell *)cell;
						//Farben setzen
						col = FindColorPen(0,0,0);
						for(row = 0;row < 4;row++)
							c->c_BorderColor[row] = col;
						c->c_APen = c->c_ReservedPen = tc->tc_Color1 & TC_COLOR_MASK;
						c->c_BPen = tc->tc_Color0 & TC_COLOR_MASK;
						//Rahmen bearbeiten
						if (tc->tc_Border & TC_BORDER_LEFT)
							c->c_Border[0] = (tc->tc_Border & TC_BORDER_LEFT) << 3;
						if (tc->tc_Border & TC_BORDER_RIGHT)
							c->c_Border[1] = (tc->tc_Border & TC_BORDER_RIGHT);
						if (tc->tc_Border & TC_BORDER_DOWN)
							c->c_Border[2] = (tc->tc_Border & TC_BORDER_DOWN) >> 6;
						if (tc->tc_Border & TC_BORDER_UP)
							c->c_Border[3] = (tc->tc_Border & TC_BORDER_UP) >> 3;
						//Schriftstile bearbeiten
						if (tc->tc_Font & TC_STYLE_MASK)
						{
							struct TagItem tags[2] = {{FA_Style, ((tc->tc_Font & (1 << TC_STYLE_ITALIC)) ? FS_ITALIC : 0) |
														((tc->tc_Font & (1 << TC_STYLE_BOLD)) ? FS_BOLD : 0) |
														((tc->tc_Font & (1 << TC_STYLE_UNDERLINED)) ? FS_UNDERLINED : 0)},
												{TAG_END, 0}};
							c->c_FontInfo = ChangeFontInfoA(NULL, page->pg_DPI, tags, FALSE);
						}
						else
						{
							struct TagItem tags[2] = {{FA_Style, 0}, {TAG_END, 0}};
							c->c_FontInfo = ChangeFontInfoA(NULL, page->pg_DPI, tags, FALSE);
						}
						//Ausrichtung in der Zelle bearbeiten
						if(tc->tc_Flags)
						{
							uint8 va[4] = {0, CA_TOP, CA_VCENTER, CA_BOTTOM};

							c->c_Alignment = (tc->tc_Flags & TCF_ALIGNMENT);
							c->c_Alignment += va[(tc->tc_Flags & TCF_VALIGNMENT)>>3];
						}
#ifdef __amigaos4__
						if(c->c_Format = MapTCCellFormat(tc->tc_TextFormat))
							c->c_Flags |= CF_FORMATSET;
#endif
#ifdef DEBUG_OS4
						DebugPrintF("Format: <%s> (0x%02x) Type:0x%02X\n", c->c_Format, tc->tc_TextFormat, c->c_Type);
#endif
					}//END if(c=NewCell
					D(bug("End FILE_(L)CELL\n\n"));
					break;
				case FILE_NAME:
					/*This is not a filename. it is a name in the table*/
					/*the first part is: 0A 00OA then the real length next two bytes*/
					/*then a new 0A with the value of the name*/
					D(bug("Namen.\n"));
#ifdef __amigaos4__
					if(cell[0] == 0)	//is it a datalen (first part of a name)
					{
						char name[100], value[100];		//buffer for name-name and name-value
						
						ClearMem(name, 100);			//Init buffers
						ClearMem(value, 100);

						w = *(UWORD *)(buf);
#ifdef DEBUG_OS4
						DebugPrintF("Datasize: %05d (Kennung:%d / Typesize:%d)\n", w, cell[0], size);
#endif
						ChangeFilePosition(dat, -((int64)(size - 2)), OFFSET_CURRENT);
						FRead(dat, name, (w < 100 ? w : 99), 1);
						if(w > 99)
							ChangeFilePosition(dat, w - 99, OFFSET_CURRENT);
						FRead(dat,&th,sizeof(struct THeader),1);
						size = (th.th_Size[0] << 8) + th.th_Size[1];
#ifdef DEBUG_OS4
						DebugPrintF("Interne Daten Name: %s\n", name);
						DebugPrintF("Type: 0x%02X Typsize: %05d (0x%02X 0x%02X)\n", th.th_Type, size, th.th_Size[0],th.th_Size[1]);
#endif
						if(th.th_Type != 0x07)			//in the moment only "normal"-names (0x07)will be read (0x0d = Makro 0x0c=Kriteria)
						{
							ChangeFilePosition(dat, size, OFFSET_CURRENT);
							break;
						}
						FRead(dat, value, (size < 100 ? size : 99), 1);
						if(size > 99)
							ChangeFilePosition(dat, size - 99, OFFSET_CURRENT);
#ifdef DEBUG_OS4
						DebugPrintF("Interne Daten Value: %s\n", value);
#endif
						AddName(&mp->mp_Prefs.pr_Names, name, value, NMT_NONE, page);
					}
#else
					w = *(UWORD *)(cell = buf);	cell += 2;
#endif
					//D(bug("	Name: %s\n",cell));
					D(bug("Name End\n\n"));
					break;
				case FILE_OPT2:
					D(bug("	Optionen-2.\n"));
					break;
				case FILE_WINDOW:
					D(bug("	Window.\n"));
					break;
				case FILE_FONTS:
					D(bug("	Fonts.\n"));
					break;
				case FILE_SCREEN:
					D(bug("	Screen.\n"));
					break;
				case FILE_COLOR:
					D(bug("	Color.\n"));
					w = *(UWORD *)(cell = buf);
#ifdef DEBUG_OS4
					DebugPrintF("Type: %02d Size: %05d Data: ", th.th_Type, w);
					for(i = 0; i < w; i++)
						DebugPrintF("%02X ", ((uint8 *)buf)[i]);
					DebugPrintF("\n");
#endif
					if ((cols = AllocPooled(pool, w * sizeof(ULONG) + 4)) != 0)
					{
						struct colorPen *cp;

						cols_size = w * sizeof(ULONG) + 4;
						for(cell += 2,col = 1;w;w--,cell += 2,col++)
						{
							row = *(UWORD *)cell;
							r = ((row & 0x0f00) >> 8) | ((row & 0x0f00) >> 4);
							g = ((row & 0x00f0) >> 4) | (row & 0x00f0);
							b = ((row & 0x000f) << 4) | (row & 0x000f);
#ifdef __amigaos4__
							//Farbe hinzufügen
							//Dann ID bestimmen, dann gehts, sonst nicht, AddPen falsche ID liefert. Muß korregiert werden
							//Fehler nur in AOS4 Version, also Systembedingt.
							cp = AddPen(NULL, r, g, b);		
							cols[col] = FindColorPen(r,g,b);	 
#else
							if ((cp = AddPen(NULL, r, g, b)) != 0)
								cols[col] = cp->cp_ID;
							else
								cols[col] = 0;
#endif
						}
					}
					break;
				case FILE_DIAGRAM:
					D(bug("	Diagram.\n"));
					break;
				case FILE_STDFONTS:
					D(bug("	Std-Fonts.\n"));
					break;
				case FILE_PATTERNS:
					D(bug("	Patterns.\n"));
					break;
				case FILE_COLUMNFLAGS:
					D(bug("	Column-Flags.\n"));
					break;
				case FILE_ROWFLAGS:
					D(bug("	Row-Flags.\n"));
					break;
				case FILE_SHEETSIZE:
					D(bug("	Sheet-Size.\n"));
					break;
				case FILE_SYSTEMFONTS:
					D(bug("	System-Fonts.\n"));
					break;
				case FILE_FROZEN:
					D(bug("	Frozen.\n"));
					break;
				case FILE_SAVEOPT:
					D(bug("	Save-Options.\n"));
					break;
				case FILE_CRYPT:
					D(bug("	Crypt.\n"));
					break;
				case FILE_DIAGRAM2:
					D(bug("	Diagram-2.\n"));
					break;
				case FILE_GLOBALFLAGS:
					D(bug("	Global-Flags.\n"));
					break;
				case FILE_OBJECT:
					D(bug("	Object.\n"));
					break;
				case FILE_STDCHART:
					D(bug("	Std-Chart.\n"));
					break;
				case FILE_OPT3:
					D(bug("	Optionen-3.\n"));
					break;
				case FILE_LASTFILES:
					D(bug("	Lastfiles.\n"));
					break;
				case FILE_CURSOR:
					D(bug("	Cursor.\n"));
					break;
				case FILE_STARTUPOPTIONS:
					D(bug("	Startup-Options.\n"));
					break;
				case FILE_TURBOCALCOWNER:
					D(bug("	TC-Owner.\n"));
					break;
				case FILE_FILEINFO:
					D(bug("	File-Info.\n"));
#ifdef DEBUG_OS4
					DebugPrintF("Type: %02d Size: %05d Data: ", th.th_Type, size);
					for(i = 0; i < size; i++)
						DebugPrintF("%02X ", ((uint8 *)buf)[i]);
					DebugPrintF("\n");
#endif
					spos = 22;
					mp->mp_Author = AllocString(&((uint8 *)buf)[spos]);
					spos += ((uint8 *)buf)[spos - 1] + 2;
					mp->mp_CatchWords = AllocString(&((uint8 *)buf)[spos]);
					spos += ((uint8 *)buf)[spos - 1] + 2;
					mp->mp_Note = AllocStringLength(&((uint8 *)buf)[spos], (uint8)((uint8 *)buf)[spos - 1]);
					tabver[0] = (uint8)((uint8 *)buf)[15] / 10 + 0x30;
					tabver[1] = (uint8)((uint8 *)buf)[15] % 10 + 0x30;
					mp->mp_Version = AllocString(tabver);
					break;

				case FILE_TABPAGE:
					D(bug("	File-TabPage.\n"));
#ifdef __amigaos4__
					Strlcpy(t, &(((char *)buf)[6]), (((uint8 *)buf)[5] < 63 ? ((uint8 *)buf)[5] + 1: 63));
                    if (!page->pg_Node.ln_Name)
                    {
                        FreeString(page->pg_Node.ln_Name);
                        page->pg_Node.ln_Name = AllocString(t);
					}
					else
                        page->pg_Node.ln_Name = AllocString(t);
#endif
					break;
				default:
					D(bug("	UNKNOWN (0x%02X).\n",th.th_Type));
#ifdef DEBUG_OS4
					DebugPrintF("Type: 0x%02X Size: %05d Data: ", th.th_Type, size);
					for(i = 0; i < size; i++)
						DebugPrintF("%02X ", ((uint8 *)buf)[i]);
					DebugPrintF("\n");
#endif
					break;
			}
		}
	}
	if (buf && len)
		FreePooled(pool,buf,len);
#ifndef __amigaos4__
	foreach (&mp->mp_Pages,page) {
		foreach(&page->pg_Table,c) {
			if (c->c_APen)
				c->c_APen = c->c_ReservedPen = cols[c->c_APen];
			else
				c->c_APen = c->c_ReservedPen = page->pg_APen;
			if (c->c_BPen)
				c->c_BPen = cols[c->c_BPen];
			else
				c->c_BPen = page->pg_BPen;
			UpdateCellText(page,c);
		}
	}
	if(cols_size)
		FreePooled(pool,cols,cols_size);
	cols = NULL;
#endif
	return RETURN_OK;
}


long PUBLIC
save(REG(d0, BPTR dat), REG(a0, struct Mappe *mp))
{
	return RETURN_WARN;
}


void PUBLIC
closePrefsGUI(void)
{
}


void PUBLIC
openPrefsGUI(REG(a0, struct Screen *scr))
{
}


STRPTR PUBLIC
getPrefs(void)
{
	return NULL;
}


long PUBLIC
setPrefs(REG(a0, STRPTR t))
{
	return TRUE;
}


#if defined(__SASC)
void STDARGS
_XCEXIT(void)
{
}
#endif
