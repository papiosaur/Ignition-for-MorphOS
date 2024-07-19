/****h* Autodoc/safeclip.h [1.0] *
*  NAME
*    safeclip.h
*  FUNCTION
*    Interface to system rendering routines, but with clipping.
*  AUTHOR
*    Peter Knight: All programming.
*    <pak@star.sr.bham.ac.uk>
*  CREATION DATE
*    24-Mar-96
*  COPYRIGHT
*    No restriction (Public Domain).
*    Use these routines as you wish in your programs (a 
*    mention in the credits would be nice, but it's up to
*    you).
**********
*/

#include <exec/types.h>
#include <proto/graphics.h>

/* Prototypes for functions from safeclip.c */
ULONG SafeInit (ULONG nvertmax);
VOID SafeClose (VOID);
VOID SafeAreaEnd (struct RastPort *rp);
VOID SafeRectFill (struct RastPort *rp, LONG x1, LONG y1, LONG x2, LONG y2);
VOID SafeDraw (struct RastPort *rp, LONG x, LONG y);

/****** safeclip.h/SafeSetLimits
* NAME
*   SafeSetLimits
* SYNOPSIS
*        SafeSetLimits (x1, y1, x2, y2)
*   VOID SafeSetLimits (LONG, LONG, LONG, LONG)
* FUNCTION
*   Set limits of clipping rectangle
* INPUTS
*   (x1,y1) - Coordinates of upper left of clipping rectangle
*   (x2,y2) - Coordinates of lower right of clipping rectangle
* RESULTS
* SEE ALSO
*/
static __inline VOID
SafeSetLimits (LONG x1, LONG y1, LONG x2, LONG y2)
{
  extern LONG CLP_xmin, CLP_ymin, CLP_xmax, CLP_ymax;
  CLP_xmin = x1;
  CLP_ymin = y1;
  CLP_xmax = x2;
  CLP_ymax = y2;
}
/*********/

/****** safeclip.h/SafeAreaDraw
* NAME
*   SafeAreaDraw
* SYNOPSIS
*        SafeAreaDraw (x, y)
*   VOID SafeAreaDraw (LONG, LONG)
* FUNCTION
*   Add a vertex to polygon vertex list.
* INPUTS
*   (x,y) - Coordinates of new vertex
* RESULTS
* SEE ALSO
*   safeclip.c/SafeAreaEnd 
*/
static __inline VOID
SafeAreaDraw (LONG x, LONG y)
{
  extern LONG CLP_nvert, CLP_nvertmax, (*CLP_vert)[2];
  if (CLP_nvert < CLP_nvertmax)
    {
      CLP_vert[CLP_nvert][0] = x;
      CLP_vert[CLP_nvert][1] = y;
      CLP_nvert++;
    }
}
/*********/

/****** safeclip.h/SafeMove
* NAME
*   SafeMove
* SYNOPSIS
*        SafeMove (x, y)
*   VOID SafeMove (LONG, LONG)
* FUNCTION
*   Move drawing pen to new position.
* INPUTS
*   (x,y) - Coordinates of new point.
* RESULTS
* SEE ALSO
*   safeclip.c/SafeDraw
*/
static __inline VOID
SafeMove (LONG x, LONG y)
{
  extern LONG CLP_lastx, CLP_lasty;
  CLP_lastx = x;
  CLP_lasty = y;
}
/*********/

/****** safeclip.h/SafeSetRast
* NAME
*   SafeSetRast
* SYNOPSIS
*        SafeSetRast (rp, pen)
*   VOID SafeSetRast (struct RastPort *, UBYTE)
* FUNCTION
*   Set the entire clipping region to a certain colour.
* INPUTS
*   rp - Pointer to RastPort
*   pen - Pen number to fill region with.
* RESULTS
* SEE ALSO
*/
static __inline VOID
SafeSetRast (struct RastPort *rp, UBYTE pen)
{
  extern LONG CLP_xmin, CLP_ymin, CLP_xmax, CLP_ymax;
  UBYTE oldPen = rp->FgPen;   /* should use GetAPen() if v39+ */
  SetAPen (rp, pen);
  RectFill (rp, CLP_xmin, CLP_ymin, CLP_xmax, CLP_ymax);
  SetAPen (rp, oldPen);
}
/*********/

/****** safeclip.h/SafeWritePixel
* NAME
*   SafeWritePixel
* SYNOPSIS
*        SafeWritePixel (rp, x, y)
*   VOID SafeWritePixel (struct RastPort *, LONG, LONG)
* FUNCTION
*   Set the colour of an individual pixel.
* INPUTS
*   rp - Pointer to RastPort
*   (x,y) - Coordinates of pixel
* RESULTS
* SEE ALSO
*/
static __inline VOID
SafeWritePixel (struct RastPort *rp, LONG x, LONG y)
{
  extern LONG CLP_xmin, CLP_xmax, CLP_ymin, CLP_ymax;
  if (x >= CLP_xmin && x <= CLP_xmax && y >= CLP_ymin && y <= CLP_ymax)
    WritePixel (rp, x, y);
}
/*********/

/****** safeclip.h/SafeBltBitMapRastPort
* NAME
*   SafeBltBitMapRastPort
* SYNOPSIS
*        SafeBltBitMapRastPort (srcbm, srcx, srcy,
*                               destrp, destx, desty.
*                               sizex, sizey, minterm)
*   VOID SafeBltBitMapRastPort (struct BitMap *, LONG, LONG,
*                               struct RastPort *, LONG, LONG,
*                               LONG, LONG, UBYTE)
* FUNCTION
*   Blit a rectangle region from a BitMap to a RastPort.
* INPUTS
*   srcbm - Pointer to source BitMap
*   (srcx,srcy) - Coordinates of upper left of source rectangle.
*   destrp - Pointer to destination RastPort
*   (destx,desty) - Coordinates of upper left of destination rectangle.
*   sizex, sizey - Size of source rectangle
*   minterm - Minterm for blitter to use during copy
* RESULTS
* SEE ALSO
*   SafeBltMaskBitMapRastPort
*/
static __inline VOID
SafeBltBitMapRastPort (struct BitMap *srcbm, LONG srcx, LONG srcy,
		       struct RastPort *destrp, LONG destx, LONG desty,
		       LONG sizex, LONG sizey, UBYTE minterm)
{
  extern LONG CLP_xmin, CLP_xmax, CLP_ymin, CLP_ymax;
  LONG xlo = destx, xhi = destx + sizex - 1;
  LONG ylo = desty, yhi = desty + sizey - 1;
  if (xlo < CLP_xmin) xlo = CLP_xmin;
  if (ylo < CLP_ymin) ylo = CLP_ymin;
  if (xhi > CLP_xmax) xhi = CLP_xmax;
  if (yhi > CLP_ymax) yhi = CLP_ymax;
  if (xlo <= xhi && ylo <= yhi && 
      xlo >= CLP_xmin && xhi <= CLP_xmax && 
      ylo >= CLP_ymin && yhi <= CLP_ymax)
    {
      BltBitMapRastPort (srcbm, srcx + (xlo - destx), srcy + (ylo - desty),
			 destrp, xlo, ylo,
			 (xhi - xlo) + 1, (yhi - ylo) + 1,
			 minterm);
    }
}
/*********/

/****** safeclip.h/SafeBltMaskBitMapRastPort
* NAME
*   SafeBltMaskBitMapRastPort
* SYNOPSIS
*        SafeBltMaskBitMapRastPort (srcbm, srcx, srcy,
*                               destrp, destx, desty.
*                               sizex, sizey, minterm, bltmask)
*   VOID SafeBltMaskBitMapRastPort (struct BitMap *, LONG, LONG,
*                               struct RastPort *, LONG, LONG,
*                               LONG, LONG, UBYTE, PLANEPTR)
* FUNCTION
*   Blit a rectangle region from a BitMap to a RastPort through
*   a single plane mask.
* INPUTS
*   srcbm - Pointer to source BitMap
*   (srcx,srcy) - Coordinates of upper left of source rectangle.
*   destrp - Pointer to destination RastPort
*   (destx,desty) - Coordinates of upper left of destination rectangle.
*   sizex, sizey - Size of source rectangle
*   minterm - Minterm for blitter to use during copy
*   bltmask - Pointer to single plane mask
* RESULTS
* SEE ALSO
*   SafeBltBitMapRastPort
*/
static __inline VOID
SafeBltMaskBitMapRastPort (struct BitMap *srcbm, LONG srcx, LONG srcy,
			   struct RastPort *destrp, LONG destx, LONG desty,
			   LONG sizex, LONG sizey, UBYTE minterm, PLANEPTR bltmask)
{
  extern LONG CLP_xmin, CLP_xmax, CLP_ymin, CLP_ymax;
  LONG xlo = destx, xhi = destx + sizex - 1, ylo = desty, yhi = desty + sizey - 1;
  if (xlo < CLP_xmin) xlo = CLP_xmin;
  if (ylo < CLP_ymin) ylo = CLP_ymin;
  if (xhi > CLP_xmax) xhi = CLP_xmax;
  if (yhi > CLP_ymax) yhi = CLP_ymax;
  if (xlo <= xhi && ylo <= yhi && 
      xlo >= CLP_xmin && xhi <= CLP_xmax && 
      ylo >= CLP_ymin && yhi <= CLP_ymax)
    {
      BltMaskBitMapRastPort (srcbm, srcx + (xlo - destx), srcy + (ylo - desty),
			     destrp, xlo, ylo,
			     (xhi - xlo) + 1, (yhi - ylo) + 1,
			     minterm, bltmask);
    }
}
/*********/

