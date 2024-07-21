;/*
gcc -Wall -gstabs _AboutProgress.c -o _AboutProgress -D__USE_INLINE__
quit
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/fuelgauge.h>
#include <images/label.h>

#include "_about_images.h"

#define OBJ(x)  objects[x]
#define GAD(x)  (struct Gadget *)objects[x]


enum
{
	OID_TEMPORAL = 0, // "temporal" gadgets to add to winobj[]

	OID_ABOUT_OBJ,
	OID_ABOUT_FOOTER,
	OID_OK,

	LAST_NUM
};


struct Library *DOSBase;
struct Library *IntuitionBase = NULL;
struct Library *GfxBase = NULL;
struct Library *DiskfontBase = NULL;

struct DOSIFace *IDOS;
struct IntuitionIFace *IIntuition = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct DiskfontIFace *IDiskfont = NULL;

// The class library base
struct ClassLibrary *ButtonBase = NULL, *LabelBase = NULL, *LayoutBase = NULL, *BitMapBase = NULL,
                    *WindowBase = NULL, *FuelGaugeBase = NULL;

// The class pointer
Class *ButtonClass, *LabelClass, *LayoutClass, *BitMapClass,
      *WindowClass, *FuelGaugeClass;

struct Window *win;
Object *objects[LAST_NUM], *winobj;
ULONG sigwait;
uint32 wsigmask;


/* Get screen at front, used by DoMessage()/ObtainColors()/ReleasePens() */
struct Screen *FrontMostScr(void)
{
	ULONG intuition_lock;
	struct Screen *front_screen_address, *public_screen_address = NULL;
	struct List *public_screen_list;
	struct PubScreenNode *public_screen_node;

	intuition_lock = LockIBase(0L);

	front_screen_address = ((struct IntuitionBase *)IntuitionBase)->FirstScreen;
	if( (front_screen_address->Flags & PUBLICSCREEN) || (front_screen_address->Flags & WBENCHSCREEN) ) {
		 UnlockIBase(intuition_lock);

		public_screen_list = LockPubScreenList();
		public_screen_node = (struct PubScreenNode *)public_screen_list->lh_Head;
		while(public_screen_node) {
			if(public_screen_node->psn_Screen == front_screen_address) {
				public_screen_address = public_screen_node->psn_Screen;
				break;
			}

			public_screen_node = (struct PubScreenNode *)public_screen_node->psn_Node.ln_Succ;
		}

		UnlockPubScreenList();
	}
	else
		UnlockIBase(intuition_lock);

	if(!public_screen_address) {
	public_screen_address = LockPubScreen(NULL);
	UnlockPubScreen(NULL, public_screen_address);
	}

//IDOS->Printf("%lx\n", (int)public_screen_address);
	return(public_screen_address);
}


void createAboutWindow(struct TextAttr *txtattr, STRPTR txt)
{
	if(txt == NULL)
	{
		OBJ(OID_ABOUT_FOOTER) = NewObject(ButtonClass, NULL, //"button.gadget",
			GA_ID,        OID_OK,//9
			GA_RelVerify, TRUE,
			GA_Text,      "_OK",
		TAG_DONE);
	}
	else
	{
		OBJ(OID_ABOUT_FOOTER) = NewObject(FuelGaugeClass, NULL, //"fuelgauge.gadget",
			GA_ID,   0,
			//GA_RelVerify, TRUE,
			GA_Text, txt,
			FUELGAUGE_Min,   0,
			FUELGAUGE_Max,   100,
			FUELGAUGE_Level, 25,
			FUELGAUGE_Ticks, 0,
			FUELGAUGE_Percent, FALSE,
		TAG_DONE);
	}

	winobj = NewObject(WindowClass, NULL, //"window.class",
					WA_Title,"AboutProgress",
					WA_DragBar,      TRUE,
					WA_CloseGadget,  txt? FALSE : TRUE,
					WA_SizeGadget,   FALSE,
					WA_DepthGadget,  TRUE,
					WA_Activate,     TRUE,
					//WA_PubScreen,    scr,
					//WA_IDCMP,IDCMP_MENUPICK, // makes main menu options work
					//(prefs.pr_Flags&PRF_SIMPLEWINDOWS)? WA_SimpleRefresh : TAG_IGNORE, TRUE,
					//WINDOW_SharedPort, iport,
					WINDOW_Position,   WPOS_CENTERSCREEN,
					WINDOW_Layout, NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						LAYOUT_SpaceOuter,  TRUE,
						LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
						//LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
						//LAYOUT_BevelStyle,  BVS_GROUP,
// LOGO & ABOUT TEXT
						LAYOUT_AddChild, /*OBJ(OID_P_GROUP_BTN) =*/ NewObject(LayoutClass, NULL, //"layout.gadget",
						LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
						LAYOUT_HorizAlignment, LALIGN_CENTER,
						//LAYOUT_VertAlignment, LALIGN_TOP,
						//LAYOUT_SpaceOuter,  TRUE,
						LAYOUT_AddChild, NewObject(ButtonClass, NULL,
							GA_ReadOnly, TRUE,
							BUTTON_RenderImage, &logoOriginalImage,
							BUTTON_BevelStyle,  BVS_NONE,
							BUTTON_Transparent, TRUE,
						TAG_DONE),
						CHILD_WeightedHeight, 0,
						LAYOUT_AddImage, NewObject(LabelClass, NULL, //"label.image",
							IA_Font, txtattr,//screen->Font,
							LABEL_Underscore,    0,
							LABEL_Justification, LJ_RIGHT,
							//LABEL_VerticalSpacing, 5,
							LABEL_SoftStyle,     FSF_BOLD,
							LABEL_Text,  "Version 1.00 beta7\n19.11.2019",
						TAG_DONE),
						CHILD_WeightedWidth, 100,
					LAYOUT_AddImage, NewObject(LabelClass, NULL, //"label.image",
						IA_Font, txtattr,//screen->Font,
						LABEL_Underscore,    0,
						LABEL_Text, "Copyright © 1996-20189 pinc Software.\n",
						LABEL_Text, "All rights reserved.\n\n",
						LABEL_Text, " Axel Dörfler\n",
						LABEL_Text, " www.pinc-software.de\n info@pinc-software.de\n\n",
						LABEL_Text, " Open Source (AOS4-ReAction version)\n",
						LABEL_Text, " Achim Pankalla\n www.os4welt.de",
					TAG_DONE),
				TAG_DONE), // END LOGO & ABOUT TEXT
				CHILD_WeightedHeight, 0,
// PINC IMAGE
				LAYOUT_AddChild, NewObject(LayoutClass, NULL, //"layout.gadget",
					LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
					//LAYOUT_SpaceOuter,  TRUE,
					LAYOUT_VertAlignment, LALIGN_BOTTOM,
					LAYOUT_AddChild, NewObject(ButtonClass, NULL,
						GA_ReadOnly, TRUE,
						BUTTON_RenderImage, &pincOriginalImage,
						//BUTTON_BitMap,,
						BUTTON_BevelStyle, BVS_NONE,
						BUTTON_Transparent, TRUE,
					TAG_DONE),
					CHILD_WeightedHeight, 0,
				TAG_DONE),  // END PINC IMAGE
			//CHILD_WeightedHeight, 0,
			TAG_DONE),
// [BUTTONS]
			LAYOUT_AddChild, OBJ(OID_ABOUT_OBJ) = NewObject(LayoutClass, NULL, //"layout.gadget",
				LAYOUT_Orientation,    LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,     BVS_SBAR_VERT,
				LAYOUT_SpaceOuter,     TRUE,
				LAYOUT_HorizAlignment, LALIGN_CENTER,
				LAYOUT_AddChild, OBJ(OID_ABOUT_FOOTER),
			TAG_DONE),
			CHILD_WeightedHeight, 0,

					TAG_DONE),
	TAG_END);
}


//UpdateProgressBar(struct ProgressBar *pb, CONST_STRPTR t, float p)
void UpdateFuelGauge(CONST_STRPTR t, uint8 val)
{
	RefreshSetGadgetAttrs(GAD(OID_ABOUT_FOOTER), win, NULL, GA_Text,t, FUELGAUGE_Level,val, TAG_DONE);
}



int main(void)
{
 struct Screen *screen = NULL;
 struct TextFont *tf;
 struct TextAttr ta = {"Times.font", 13, 0, 0};

 DOSBase = OpenLibrary("dos.library", 52);
 IntuitionBase = OpenLibrary("intuition.library", 52);
 IIntuition = (struct IntuitionIFace *)GetInterface(IntuitionBase, "main", 1, NULL);
 GfxBase = OpenLibrary("graphics.library", 52);
 IGraphics = (struct GraphicsIFace *)GetInterface(GfxBase, "main", 1, NULL);
 DiskfontBase = OpenLibrary("diskfont.library", 52);
 IDiskfont = (struct DiskfontIFace *)GetInterface(DiskfontBase, "main", 1, NULL);

 screen = FrontMostScr();
 if( (tf=OpenDiskFont(&ta)) == NULL )
 {
Printf("Using default font\n");
  ta = *(screen->Font);
 }

 ButtonBase = OpenClass("gadgets/button.gadget", 52, &ButtonClass);
 LabelBase = OpenClass("images/label.image", 52, &LabelClass);
 FuelGaugeBase = OpenClass("gadgets/fuelgauge.gadget", 52, &FuelGaugeClass);
 LayoutBase = OpenClass("gadgets/layout.gadget", 52, &LayoutClass);
 WindowBase = OpenClass("window.class", 52, &WindowClass);


/*
	pincImage = (struct Image *)NewObject(pictureiclass, NULL,
			PIA_FromImage,	&pincOriginalImage,
			PIA_WithColors,	standardPalette + 1,
			PDTA_Screen,	iscr,
			TAG_END)) != NULL)
AddImageObj(NULL,pincImage);

	logoImage = (struct Image *)NewObject(pictureiclass, NULL,
	      PIA_FromImage,  &logoOriginalImage,
       PIA_WithColors, standardPalette+1,
       PDTA_Screen,    screen,
      TAG_END);
*/


	createAboutWindow(&ta, "loading started."); // UpdateProgress ABOUT
	if( (win=(struct Window *)IDoMethod(winobj, WM_OPEN, NULL)) )
	{
		Delay(200);
		UpdateFuelGauge("loading 123...", 40);
		Delay(200);
		UpdateFuelGauge("loading abc...", 85);
		Delay(200);
		UpdateFuelGauge("loading finished.", 100);
		Delay(200);
		IDoMethod(winobj, WM_CLOSE, NULL);
		win = NULL;
	}


	createAboutWindow(&ta, NULL); // normal ABOUT
	if( (win=(struct Window *)IDoMethod(winobj, WM_OPEN, NULL)) )
	{
		BOOL done = TRUE;
		uint16 code = 0;
		uint32 siggot = 0, wsigmask = 0, result = WMHI_LASTMSG;

		while(done)
		{
			GetAttr(WINDOW_SigMask, winobj, &wsigmask);
			siggot = Wait(wsigmask|SIGBREAKF_CTRL_C);

			if(siggot & SIGBREAKF_CTRL_C) { done = FALSE; break; }

			while( (result=IDoMethod(winobj, WM_HANDLEINPUT, &code)) != WMHI_LASTMSG )
			{
				switch(result & WMHI_CLASSMASK)
				{
					case WMHI_CLOSEWINDOW:
						done = FALSE;
					break;
					case WMHI_GADGETUP:
						switch(result & WMHI_GADGETMASK)
						{
							case OID_OK:
								done = FALSE;
							break;
						}
					break;
				}
			}
		} // END while(done)
		IDoMethod(winobj, WM_CLOSE, NULL);
	}


 if(tf) CloseFont(tf);

 CloseClass(ButtonBase);
 CloseClass(LabelBase);
 CloseClass(FuelGaugeBase);
 CloseClass(LayoutBase);
 CloseClass(WindowBase);

 DropInterface( (struct Interface *)IDiskfont );
 CloseLibrary(DiskfontBase);
 DropInterface( (struct Interface *)IGraphics );
 CloseLibrary(GfxBase);
 DropInterface( (struct Interface *)IIntuition );
 CloseLibrary(IntuitionBase);
 CloseLibrary(DOSBase);
}