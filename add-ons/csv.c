/* ignition CSV-I/O-Module
 *
 * Copyright 1996-2016 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#ifdef __amigaos4__
	#include <exec/exec.h>
	#include <proto/exec.h>
	#include <proto/dos.h>
	#include <proto/utility.h>
	#include <proto/diskfont.h>
	#include <proto/gadtools.h>
	#include <proto/intuition.h>

	#include <string.h>

	#include <intuition/gadgetclass.h>
	#include <libraries/gadtools.h>
	#include <diskfont/diskfonttag.h>

	#define CATCOMP_NUMBERS
	#include "../ignition_strings.h"
#else
	#include <stdlib.h>
#endif

#include "iotype.h"
#include <proto/locale.h>

const STRPTR __version = "$VER: csv.io 1.04 (28.12.2022)";

//Alternatives for using of prefs
#define PREFS_GUESS 		1											//Automatic-mode for separator detecting
#define PREFS_ASK 			2											//Ask for the separator

//Global variables for prefs and separator
char *separators = {";,\t"};											//Possible spearators
char loadprefs    = PREFS_GUESS;										//default load-separator / Option
char saveprefs    = ';';												//default save-separator / Option
long sethyphen	  = 1;													//Flag for inclosing data with "
long setsep		  = 0;

//Global variables for GUI
#ifdef __amigaos4__
	struct Window *guiwin = NULL;										//GUI-Window for preferences
	struct Gadget *guigad = NULL;										//Startpointer for all gadgets
	APTR vi 			  = NULL;										//VisualInfo Pointer
	char *cbtext 	= "\"...\"";										//Text fpr checkbox-gadget.
	char *cytext[] 	= {"  ;  ", "  ,  ", "<TAB>", NULL};				//Texts for cycle-gadget.
	static struct Catalog *sCatalog;
	#define CBGAD	1													//ID for checkbox gadget
	#define CYGAD	2													//ID for radio gadget
	#define BUTGAD  3													//ID for OK gadget
#endif

#ifdef __amigaos4__														//Some needed things to compile under AOS4
	STRPTR Strchr(STRPTR t, char c)										//AOS4 Strchr
	{
		int i;															//Index variable
	
		for(i = 0; t[i] != '\0'; i++)									//run through the string
			if(t[i] == c)												//if give character found
				return &t[i];											//Return a pointer to his position
		return NULL;													//Nothing found
	}
	#undef  strlen														//undef standard functions
	#undef  stricmp
	#undef  strnicmp
	#undef  strchr
	#define strlen   Strlen												//use the utility-Lib functions
	#define stricmp  Stricmp
	#define strnicmp Strnicmp
	#define strchr   Strchr
#else																	//Some needed things to compile other OS then OS4
	/*
	This functions determines the size of the hole csv-file. It rebuilds a new api-function from AOS4
	*/
	long GetFileSize(BPTR file)
	{
		struct FileInfoBlock *fib;										//Create FileInfoBlock pointer
		long size = 0;													//size of the csv-file

		if(!(fib = AllocDosObject(DOS_FIB,TAG_END)))					//Alloc a FileInfoBlockPointer
			return(0);
		if(ExamineFH(file,fib))											//Read FileInfoBlock data
			size = fib->fib_Size;   									//set size variable
		FreeDosObject(DOS_FIB,fib);										//Free DosObject
		return(size);													//return size value
	}
#endif


struct Locale *loc;														//Localepointer

#ifdef __amigaos4__
//UCS-translate things
uint32 ucsmap[256];														//Pointer for Unicodes-map
struct DiskfontIFace *IDiskfont;										//Library variable

BOOL InitUTF(void)
{
	struct Locale *loc;																				//Pointer for locale data
	struct DiskfontIFace *IDiskfont;																//Pointer for library
	struct Library *DiskfontBase;																	//library pointer
	struct Library *Base;																			//library pointer
	uint32 *tmp;																					//tmp pointer
	char lang[64];																					//buffer for language filename
	uint32 i;																						//index
	char filename[100];																				//filename for the functionnames

  	if((Base = (struct Library *)OpenLibrary("utility.library", 54)) == NULL)						//Test Utility-Libversion
  	{
  		return FALSE;
  	}
 	CloseLibrary((struct Library *)Base);
  	if((DiskfontBase = (struct Library *)OpenLibrary("diskfont.library", 50)) == NULL)				//Open diskfont-library
  		return FALSE;
	IDiskfont = (struct DiskfontIFace *)GetInterface((struct Library *)DiskfontBase,"main",1,NULL);
	
    loc = OpenLocale(NULL);																			//Open locale enviroment
	tmp = (uint32*)ObtainCharsetInfo(DFCS_NUMBER, loc->loc_CodeSet, DFCS_MAPTABLE);					//get ucs-map array
	if(tmp != NULL)																					//get a result
		CopyMem(tmp, ucsmap, sizeof(ucsmap));														//save information
  	DropInterface((struct Interface *)IDiskfont);													//close all
	CloseLibrary((struct Library *)DiskfontBase);
    CloseLocale(loc);
    return (tmp != NULL ? TRUE : FALSE);															//return success
}

void ConvertText(char *str)
{
    uint32 ic, im;																					//indices for the tables
    uint32 count;																					//number of ucs-chracters in string
    int32 data[2048];																				//buffer for the ucs-char
    char cstr[1023];																				//Buffer für Konvertierung
    
    ClearMem(data, 2048);
   	count = UTF8toUCS4(str, data, 4096, UTF_INVALID_SUBST_FFFD);									//transform utf data to ucs
  	for(ic = 0; ic < count; ic++)																	//prozess all ucs data
  	{
  		for(im = 0; im < 256; im++)																	//compare all characters
  		{
  			if(ucsmap[im] == data[ic] && ic < 2048)													//is ucs identical to char in table
				cstr[ic] = im;																		//character code found insert it
		}
	}
	cstr[ic] = '\0';																				//Ende setzen
	Strlcpy(str, cstr, 1023);																		//kopieren
	return;
}
#endif

/*
This function do all operations to reset the PrefsGui.
*/
void PUBLIC closePrefsGUI(void)
{
#ifdef __amigaos4__
	guiwin 	= NULL;
	guigad 	= NULL;
	vi 		= NULL;
	CloseCatalog(sCatalog);
#endif
}

BOOL HandlePrefsSelection(struct Window *win)
{
	BOOL rvalue = TRUE;													//Return vlaue is true, until CLOSEWINDOW
	struct IntuiMessage *imsg;											//Messagepointer
	ULONG Class;														//Messagetype
	struct Gadget *gad;													//Pointer to actual gadget

	Wait (1 << win->UserPort->mp_SigBit);								//Wait for signal on windowport
	while(imsg = GT_GetIMsg(win->UserPort))								//Porcess all signals
	{
		Class = imsg->Class;											//Store class information
		gad = (struct Gadget *)imsg->IAddress;							//Store gadgetpointer
		GT_ReplyIMsg(imsg);												//Free message
		switch (Class)													//Separate classes
		{
			case IDCMP_GADGETUP:										//Process gadget events
				switch(gad->GadgetID)
				{
					case CBGAD:
						GT_GetGadgetAttrs(gad, guiwin, NULL, GTCB_Checked, &sethyphen, TAG_DONE);
						break;
					case CYGAD:
						GT_GetGadgetAttrs(gad, guiwin, NULL, GTCY_Active, &setsep, TAG_DONE);
						saveprefs = separators[setsep];
						break;
#ifdef __amigaos4__														//Some needed things to compile under AOS4
					case BUTGAD:
						rvalue = FALSE;
						break;
#endif
				}														//END switch code
				break;

			case IDCMP_CLOSEWINDOW:										//Process windows close event
				rvalue = FALSE;											//Set return-value to false
				break;

			case IDCMP_REFRESHWINDOW:									//Process refresh event
				GT_BeginRefresh(win);
				GT_EndRefresh(win, TRUE);
				break;
		}																//END switch Class
	}																	//END while loop
	return(rvalue);
}

/*
This function do all operations to open the PrefsGui.
*/
void PUBLIC openPrefsGUI(REG(a0, struct Screen *scr))
{
#ifdef __amigaos4__
	struct NewGadget ng;												//NewGadget Struktur for creating gadgets
	struct Gadget *gad;													//Gadgetpointer for gadgetchain

	if(guiwin == NULL)													//Window not open, then make it
	{																	//Attention, ignition call openPrefsGUI more the once
		sCatalog = OpenCatalog(NULL, "ignition.catalog", OC_BuiltInLanguage, "english", TAG_END);
		if((vi = GetVisualInfo(scr, TAG_END)) != NULL)					//Get VisualInfo
		{																//Successful
			gad = CreateContext(&guigad);								//Create Gadget Context
			ng.ng_GadgetText = cbtext;									//First gadget checkbox
			ng.ng_LeftEdge	 = 120;
			ng.ng_TopEdge    = 35;
			ng.ng_Width      = 30;
			ng.ng_Height     = 40;
			ng.ng_VisualInfo = vi;
			ng.ng_Flags 	 = PLACETEXT_RIGHT;
			ng.ng_GadgetID 	 = CBGAD;
			ng.ng_TextAttr   = scr->Font;
			ng.ng_UserData 	 = NULL;
			gad = CreateGadget(CHECKBOX_KIND, gad, &ng, GTCB_Checked, sethyphen, TAG_END);
			ng.ng_LeftEdge	 = 20;
			ng.ng_TopEdge    = 35;
			ng.ng_Width      = 90;
			ng.ng_Height 	 = 20;
			ng.ng_GadgetText = "";
			ng.ng_GadgetID = CYGAD;
			ng.ng_Flags = NG_HIGHLABEL;
			gad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, cytext, GTCY_Active, setsep, TAG_DONE);
			ng.ng_LeftEdge = 20;
			ng.ng_TopEdge  = 58;
			ng.ng_Width    = 160;
			ng.ng_GadgetText = (STRPTR)GetCatalogStr(sCatalog, MSG_OK_GAD, "OK");
			ng.ng_GadgetID = BUTGAD;
			gad = CreateGadget(BUTTON_KIND,gad,&ng,GT_Underscore,'_',TAG_END);

			if ((guiwin = OpenWindowTags(NULL,
								 WA_Flags,		0,
								 WA_Left,	 	(scr->Width - 200) / 2,	//Set in the middle of the screen
								 WA_Top,	  	(scr->Height - 85) / 2,
								 WA_Title, 		"ignition CSV",
								 WA_Width,		200,
								 WA_Height,	   	85,
								 WA_PubScreen,	scr,
								 WA_DragBar, TRUE,
								 WA_DepthGadget, TRUE,
								 WA_CloseGadget, TRUE,
								 WA_Activate, TRUE,
								 WA_IDCMP, 		IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | CHECKBOXIDCMP,
								 TAG_END)) != (struct Window *)NULL)
			{															//Window successfully opened
				AddGList(guiwin, guigad, -1, -1, NULL);					//Gadgets aktivieren
				RefreshGList(guigad, guiwin, NULL, ((UWORD) -1));
				GT_RefreshWindow(guiwin, NULL);
				while(HandlePrefsSelection(guiwin));					//Process the window events until closewindow
				CloseWindow(guiwin);
			}															//END Window successfully opened		
			FreeGadgets(guigad);										//Free system variables
			FreeVisualInfo(vi);
		}																//END Successfully get VisualInfo
	}																	//END Window not open
#endif
}

/*
This function decides with the prefs-value the contents of the PrefsString.
*/
void setPrefsString(short prefs, STRPTR t)
{
	if (prefs == PREFS_GUESS)											//For PREFS_GUESS
		strcpy(t,"guess");												//Set PrefsString to "guess"
	else if (prefs == PREFS_ASK)										//If PREFS_ASK
		strcpy(t,"ask");												//Then set it to "ask"
	else																//When not guess or ask
	{																	//write into PrefsString
		strcpy(t,"sep=");												//sep=
		t[4] = prefs;													//and the sign which stored in prefs
		t[5] = 0;														//set new string end
	}
}

/*
This functions gets the actual setting for load and save csv-files.
The settings of the global prefs variables are stored in a single string.
first the loadprefs and the saveprefs serarted with a &.
*/
STRPTR PUBLIC getPrefs(void)
{
	static char t[32];													//Create permanent variable t

	setPrefsString(loadprefs,t);										//Get loadprefs
#ifdef __amigaos4__														//Append an & as separator
	Strlcat(t,"&", 32);
#else
	strcat(t,"&");
#endif
	setPrefsString(saveprefs,t+strlen(t));								//Append the saveprefs to t

	return t;															//Return the pointer to t.
}

/*
This function sets the global variables loadprefs and saveprefs. It gets the setting
from the static variable t and writes the information in loadprefs and saveprefs.
*/
long PUBLIC setPrefs(REG(a0, STRPTR t))
{
	UBYTE  ok = TRUE;													//flag for data
	STRPTR s;															//pointer to separator character

	if(!t)																//if data availble
		return(TRUE);													//end with true

	if((s = strchr(t,'&')) != 0)										//is a separator sign in the string
	{																	//then
		*s = 0;															//remove & sign
																		//Analyse the data right from & for saveprefs
		if(!stricmp("ask", s+1))										//if ask in the string
			saveprefs = PREFS_ASK;										//saveprefs is ASK
		else if (!strnicmp("sep=",s+1,4))								//if sep= is in the string
			saveprefs = *(s+5);											//then get the five character as separator
		else															//if not target is found
			ok = FALSE;													//no vaild config is found.
	}
	if(ok)																//if a valid saveprefs are found
	{																	//then get the loadprefs config
		if(!stricmp("guess", t))
			loadprefs = PREFS_GUESS;									//loadprefs is GUESS
		else if (!stricmp("ask",t))
			loadprefs = PREFS_ASK;										//loadprefs is ASK
		else if (!strnicmp("sep=",t,4))
			loadprefs = *(t+4);											//Separator is in the config
		else
			ok = FALSE;													//no vaild config is found.
	}
	if(s)  																//if s is set
		*s = '&';														//recover the string
	
	if(!ok)																//error state?
		ReportError("No valid configuration");							//visualazation of an error-message

	return(TRUE);
}

/*
This function determines the separator-character.
*/
char GetSeparator(char prefsmode, char *buffer)
{
	char sep = '\0';													//to determine separator-char
	char *s;															//Actual position in buffer

	if(prefsmode == PREFS_GUESS)										//Guess-Mode
	{
		int  i;															//Index-variable

		for(s = buffer; *s; s++)										//Run through the buffer
		{
			for(i = 0; separators[i] != *s && separators[i]; i++);		//Compare buffer char with every separator
			if (i <= strlen(separators) - 1)								//if a separator found
				return separators[i];									//use it and end
		}
	}
	else if (prefsmode == PREFS_ASK)									//Ask user for separator
	{
		ReportError("Not yet implemented.");							//no comment ;-)
	}
	else																//Use separator from csv.iodescr
		sep = prefsmode;
	return sep;
}

/*
This function removes all not ASCII-characters from the string, to avoid problems
on loading these data.
*/
void RemoveNonASCII(char *string)
{
	long i;																//Index

	for(i = 0; string[i]; i++)											//run over string
		if(!IsPrint(loc, string[i]) && (string[i] < 32 || string[i] > 126) && string[i] != 10 && string[i] != 9 && string[i])
			string[i] = ' ';											//Replace a not printable characters with space
}

/*
This function extracted the data for one cell of the table from the given buffer b with
the separator sep.
The position parameter marks the beginning point of analyse and will be changed by
analyze the buffer.
If the data are isolated, there will be copied to a new allocated buffer and the
pointer to this buffer is returned. The position parameter is now set on the end of
the isolated data in the buffer.
If buffer is empty, NULL is returned.
*/
char *GetCellData(char sep, char *b, long *pos)
{
	char *start = &b[*pos];												//Marker for startposition
	long offset = 0;													//is 0 when no " are used else 1
	char *c;															//Pointer to new string

	if(b[*pos] == '\0')													//No data
		return(NULL);
	for(;; (*pos)++)													//Run through buffer
	{
		if(b[*pos] == '\r')												//End of line is CR LF
#ifdef __amigaos4__	
			Strlcpy(&b[*pos], &b[(*pos) + 1], strlen(&b[*pos]));		//then remove CR
#else
			strcpy(&b[*pos], &b[(*pos) + 1]);
#endif
		if(b[*pos] == sep  || 											//separator reached or
		   b[*pos] == '\n' ||					 						//End of line reached or
		   b[*pos] == '\0')												//End of buffer reached
		{
			if(*start == '"' && b[(*pos) - 1] != '"')					//broken line with data embedded with "?
				continue; 												//Yes, then search for the real end of data
			if(*start == '"')											//Data embedded in "
			{
				start++;												//Ignore "
				offset = 1;												//Set offset for "
			}															//End if start = '"'
			else
				offset = 0;												//No " no offset
			if(&b[*pos] - start - offset > 0)
				c = AllocStringLength(start, &b[*pos] - start - offset);//Generate data-space
			else
				c = AllocStringLength(" ", 1);							//no data
			ConvertText(c);
			return c;													//Return Pointer to the data
		}																//End if end of cell-data reached
	}																	//End of for-loop
}

/*
This functions controls the reading of the data from a csv-file.
It put the separated data into a new table of ignition.
Every field of a line in a separate column-field in a row.
Every line in a separated row of the table.
*/
long PUBLIC load(REG(d0, BPTR file), REG(a0, struct Mappe *mp))
{
	char *buffer = NULL;												//Buffer for the whole csv-data
	char *cdata;														//pointer to the buffer for cell-data
	long pos = 0;														//Position in the buffer
	char sep;															//Actual separator char
	struct Page *page;													//Pointer to a new table
	struct Cell *c;														//Pointer to a new cell in the table
	long size = 0;														//Size of the csv-file

	InitUTF();															//Init UTF conversion
	loc = OpenLocale(NULL);												//Map to locale behaviour
	size = GetFileSize(file);											//Get size of the csv-file
	if(!(buffer = AllocPooled(pool, 1 + size)))							//Reserve memory for file and end string char
		return(RETURN_FAIL);											//not enough memory
	if(FRead(file, buffer, 1, size) != size)							//Read the whole file
	{																	//when a error occurs
		ReportError("Error at reading file.");							//Show messages
		FreePooled(pool,buffer,size+1);									//Free memory
		return(RETURN_FAIL);											//End function
	}
	sep = GetSeparator(loadprefs, buffer);								//Get the actual separator
	if((page = NewPage(mp)) != 0)										//Create new table in ignition
	{																	//Successful
		long col = 1, row = 1;											//Actual column and row number

		while(cdata = GetCellData(sep, buffer, &pos))					//Loop if data available
		{
			if((c = NewCell(page, col, row)) != 0)						//if successfully create cell
			{
				RemoveNonASCII(cdata);									//Remove non ascii's
				c->c_Text = cdata;										//set text to data-buffer
				UpdateCellText(page,c);									//Update Cell
				col = (buffer[pos] == '\n' ? 1 : col + 1);				//Update column counter
				row = (buffer[pos] == '\n' ? row + 1 : row);			//Update row counter
				pos += (buffer[pos] ? 1 : 0);							//Jump over the reached separator if not \0
			}
		}																//while-Loop for get cell-data
	}																	//End if NewPage
	FreePooled(pool,buffer,size + 1);									//Release buffer memory
	CloseLocale(loc);													//Free pointer
	return(RETURN_OK);													//End function
}

/*
This function saves the hole table in a ascii-file in the csv-format. The separator
is selected with the methode which is descripted in the file csv.iodescr.
*/
long PUBLIC save(REG(d0, BPTR dat), REG(a0, struct Mappe *mp))
{
	struct Page *page;													//Pointer to the actual page/table
	struct Cell *c;														//Pointer to the actual cell
	ULONG  lastrow = 1,lastcol = 1;										//Conuter for row and column
	UBYTE  sep = ';';													//separator character default ;
	char   *buffer;														//Tmp String Buffer

	loc = OpenLocale(NULL);												//Map to locale behaviour
	if(saveprefs > 0)													//if saveprefs is set
		sep = saveprefs;												//Get separator from saveprefs

	foreach(&mp->mp_Pages,page)											//run through all pages
	{
		foreach(&page->pg_Table,c)										//run through all cell of a table
			if(c->c_Text)												//Cell has contents
			{
				while(lastrow < c->c_Row)								//Generate empty rows before contents
				{
					FPutC(dat,'\n');
					lastrow++;  lastcol = 1;
				}
				while(lastcol < c->c_Col)								//Generate empty columns before contents
				{
					FPutC(dat,sep);
					lastcol++;
				}
				if(sethyphen)
					FPutC(dat,'"');										//Write contents embedded in "-characters
				buffer = AllocString(c->c_Text);						//Copy text
				RemoveNonASCII(buffer);									//Remove non ascii
				FPuts(dat,buffer);										//write data
				FreeString(buffer);										//Free memory
				if(sethyphen)
					FPutC(dat,'"');
			}															//END Cell has contents
		lastrow = 1; lastcol = 1;										//Next table with a separator line
		FPutC(dat,'\n');
		FPutC(dat,'\n');
	}																	//END of Table
	CloseLocale(loc);													//Free pointer
	return(RETURN_OK);
}

#if defined(__SASC)
void STDARGS _XCEXIT(long a)
{
}
#endif
