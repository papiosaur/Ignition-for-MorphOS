#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>


extern void ErrorOpenLibrary(STRPTR, STRPTR);


// The class library base
struct ClassLibrary *CheckBoxBase = NULL, *ButtonBase = NULL, *ClickTabBase = NULL, *LabelBase = NULL,
                    *LayoutBase = NULL, *GetFontBase = NULL, *GetFileBase = NULL, *GetScreenModeBase = NULL,
                    *IntegerBase = NULL, *RadiobuttonBase = NULL, *PaletteBase = NULL, *StringBase = NULL,
                    *GradientSliderBase = NULL, *ColorWheelBase = NULL, *GetColorBase = NULL,
                    *SliderBase = NULL, *SpaceBase = NULL, *BevelBase = NULL, *TexteditorBase =NULL,
                    *ScrollerBase = NULL, *WindowBase = NULL, *RequesterBase = NULL, *FuelGaugeBase = NULL;
struct Library *ChooserBase = NULL;
struct Library *ListBrowserBase = NULL;
//struct Library *GradientSliderBase = NULL;
//struct Library *ColorWheelBase = NULL;
// The class pointer
Class *CheckBoxClass, *ButtonClass, *ClickTabClass, *LabelClass, *LayoutClass, *GetFontClass,
      *GetFileClass, *GetScreenModeClass, *IntegerClass, *RadiobuttonClass, *PaletteClass,
      *StringClass, *WindowClass, *RequesterClass, *ChooserClass, *ListBrowserClass,
      *GradientSliderClass, *ColorWheelClass, *GetColorClass, *SliderClass, *SpaceClass,
      *BevelClass, *TexteditorClass, *ScrollerClass, *FuelGaugeClass;
// Some interfaces needed
struct ChooserIFace *IChooser = NULL;
struct ListBrowserIFace *IListBrowser = NULL;
//struct GradientSliderIFace *IGradientSlider = NULL;
//struct ColorWheelIFace *IColorWheel = NULL;


/* Open required ReAction gadgets/classes */
BOOL openRAresources(void)
{
  CheckBoxBase = OpenClass("gadgets/checkbox.gadget", 52, &CheckBoxClass);
  ButtonBase = OpenClass("gadgets/button.gadget", 52, &ButtonClass);
  ClickTabBase = OpenClass("gadgets/clicktab.gadget", 52, &ClickTabClass);
  LabelBase = OpenClass("images/label.image", 52, &LabelClass);
  GetFontBase = OpenClass("gadgets/getfont.gadget", 52, &GetFontClass);
  GetFileBase = OpenClass("gadgets/getfile.gadget", 52, &GetFileClass);
  GetScreenModeBase = OpenClass("gadgets/getscreenmode.gadget", 52, &GetScreenModeClass);
  IntegerBase = OpenClass("gadgets/integer.gadget", 52, &IntegerClass);
  RadiobuttonBase = OpenClass("gadgets/radiobutton.gadget", 52, &RadiobuttonClass);
  PaletteBase = OpenClass("gadgets/palette.gadget", 52, &PaletteClass);
  StringBase = OpenClass("gadgets/string.gadget", 52, &StringClass);
  GradientSliderBase = OpenClass("gadgets/gradientslider.gadget", 52, &GradientSliderClass);
  ColorWheelBase = OpenClass("gadgets/colorwheel.gadget", 52, &ColorWheelClass);
  GetColorBase = OpenClass("gadgets/getcolor.gadget", 52, &GetColorClass);
  SliderBase = OpenClass("gadgets/slider.gadget", 52, &SliderClass);
  SpaceBase = OpenClass("gadgets/space.gadget", 52, &SpaceClass);
  BevelBase = OpenClass("images/bevel.image", 52, &BevelClass);
  TexteditorBase = OpenClass("gadgets/texteditor.gadget", 52, &TexteditorClass);
  ScrollerBase = OpenClass("gadgets/scroller.gadget", 52, &ScrollerClass);
  FuelGaugeBase = OpenClass("gadgets/fuelgauge.gadget", 52, &FuelGaugeClass);
  //BitMapBase = OpenClass("images/bitmap.image", 52, &BitMapClass);
  LayoutBase = OpenClass("gadgets/layout.gadget", 52, &LayoutClass);
  WindowBase = OpenClass("window.class", 52, &WindowClass);
  RequesterBase = OpenClass("requester.class", 52, &RequesterClass);
  if(!CheckBoxBase||!ButtonBase||!ClickTabBase||!LabelBase||!GetFontBase||!GetFileBase
     ||!GetScreenModeBase||!IntegerBase||!RadiobuttonBase||!PaletteBase||!StringBase
     ||!GradientSliderBase||!ColorWheelBase||!GetColorBase||!SliderBase||!SpaceBase||!BevelBase
     ||!TexteditorBase||!ScrollerBase||!FuelGaugeBase
     ||!LayoutBase||!WindowBase||!RequesterBase)
  {
    ErrorOpenLibrary("ReAction classes/gadgets", NULL);
    return FALSE;
  }

  ListBrowserBase = (struct Library *)OpenClass("gadgets/listbrowser.gadget", 52, &ListBrowserClass);
  if(!ListBrowserBase)
  {
    ErrorOpenLibrary("listbrowser.gadget", "52");
    return FALSE;
  }
  IListBrowser = (struct ListBrowserIFace *)GetInterface( (struct Library*)ListBrowserBase, "main", 1, NULL );
  if(!IListBrowser)
  {
    ErrorOpenLibrary("ListBrowserIFace", NULL);
    return FALSE;
  }

  ChooserBase = (struct Library *)OpenClass("gadgets/chooser.gadget", 52, &ChooserClass);
  if(!ChooserBase)
  {
    ErrorOpenLibrary("chooser.gadget", "52");
    return FALSE;
  }
  IChooser = (struct ChooserIFace *)GetInterface( (struct Library *)ChooserBase, "main", 1, NULL);
  if(!IChooser)
  {
    ErrorOpenLibrary("ChooserIFace", NULL);
    return FALSE;
  }

  /*ColorWheelBase = (struct Library *)OpenClass("gadgets/colorwheel.gadget", 52, &ColorWheelClass);
  if(!ColorWheelBase)
  {
    ErrorOpenLibrary("colorwheel.gadget", "52");
    return FALSE;
  }
  IColorWheel = (struct ColorWheelIFace *)GetInterface( (struct Library *)ColorWheelBase, "main", 1, NULL);
  if(!IColorWheel)
  {
    ErrorOpenLibrary("ColorWheelIFace", NULL);
    return FALSE;
  }*/

  /*GradientSliderBase = (struct Library *)OpenClass("gadgets/gradientslider.gadget", 52, &GradientSliderClass);
  if(!GradientSliderBase)
  {
    ErrorOpenLibrary("gradientslider.gadget", "52");
    return FALSE;
  }
  IGradientSlider = (struct GradientSliderIFace *)GetInterface( (struct Library *)GradientSliderBase, "main", 1, NULL);
  if(!IGradientSlider)
  {
    ErrorOpenLibrary("GradientSliderIFace", NULL);
    return FALSE;
  }*/

 return TRUE;
}


void closeRAresources(void)
{
  CloseClass(CheckBoxBase);
  CloseClass(ButtonBase);
  CloseClass(ClickTabBase);
  CloseClass(LabelBase);
  CloseClass(GetFontBase);
  CloseClass(GetFileBase);
  CloseClass(GetScreenModeBase);
  CloseClass(IntegerBase);
  CloseClass(RadiobuttonBase);
  CloseClass(PaletteBase);
  CloseClass(StringBase);
  CloseClass(GradientSliderBase);
  CloseClass(ColorWheelBase);
  CloseClass(GetColorBase);
  CloseClass(SliderBase);
  CloseClass(SpaceBase);
  CloseClass(BevelBase);
  CloseClass(TexteditorBase);
  CloseClass(ScrollerBase);
  CloseClass(FuelGaugeBase);
  //CloseClass(BitMapBase);
  CloseClass(LayoutBase);
  CloseClass(RequesterBase);
  CloseClass(WindowBase);

  //DropInterface( (struct Interface *)IGradientSlider );
  //CloseClass( (struct ClassLibrary *)GradientSliderBase );

  //DropInterface( (struct Interface *)IColorWheel );
  //CloseClass( (struct ClassLibrary *)ColorWheelBase );

  DropInterface( (struct Interface *)IChooser );
  CloseClass( (struct ClassLibrary *)ChooserBase );

  DropInterface( (struct Interface *)IListBrowser );
  CloseClass( (struct ClassLibrary *)ListBrowserBase );
}
