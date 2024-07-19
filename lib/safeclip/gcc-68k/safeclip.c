/****h* Autodoc/safeclip.c [1.0] *
*  NAME
*    safeclip.c
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
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include "safeclip.h"

/* Global variables. */
LONG CLP_xmin, CLP_ymin, CLP_xmax, CLP_ymax;
LONG (*CLP_vert)[2] = NULL, (*CLP_wrk)[2] = NULL;
LONG CLP_nvert = 0, CLP_nvertmax;
LONG CLP_lastx, CLP_lasty;

/****i* safeclip.c/Clip2d
* NAME
*   Clip2d
* SYNOPSIS
*   res = Clip2d (n)
*   LONG Clip2d (LONG)
* FUNCTION
*   C interface to ASM routine for clipping filled polygons.
* INPUTS
*   n - Number of vertices in polygon
* RESULTS
*   res - Number of vertices in clipped polygon
* SEE ALSO
*/
static __inline LONG
Clip2d (LONG n)
{
  register LONG _res __asm("d0");
  register LONG *a0 __asm("a0") = &CLP_vert[0][0];
  register LONG *a1 __asm("a1") = CLP_wrk[0];
  register LONG d0 __asm("d0") = n;
  __asm ("jsr _clip2d"
	 : "=r" (_res)
	 : "r" (a0), "r" (a1), "r" (d0)
	 : "d0", "memory");
  return _res;
}
/*********/

/****i* safeclip.c/ClipLine
* NAME
*   ClipLine
* SYNOPSIS
*   res = ClipLine ()
*   LONG ClipLine (VOID)
* FUNCTION
*   C interface to ASM routine for clipping line segments.
* INPUTS
* RESULTS
*   res - TRUE if any part of line was drawn, otherwise FALSE.
* SEE ALSO
*/
static __inline LONG
ClipLine (VOID)
{
  register LONG _res __asm("d0");
  register LONG *a0 __asm("a0") = &CLP_vert[0][0];
  __asm ("jsr _lineclip"
	 : "=r" (_res)
	 : "r" (a0)
	 : "d0", "memory");
  return _res;
}
/*********/

/****** safeclip.c/SafeAreaEnd
* NAME
*   SafeAreaEnd
* SYNOPSIS
*        SafeAreaEnd (rp)
*   VOID SafeAreaEnd (struct RastPort *)
* FUNCTION
*   Process buffer AreaDraw() instructions
* INPUTS
*   rp - Pointer to RastPort on which to draw.
* RESULTS
* SEE ALSO
*   SafeAreaDraw
*/
VOID
SafeAreaEnd (struct RastPort *rp)
{
  if (CLP_nvert)
    {
      WORD i;
      SafeAreaDraw (CLP_vert[0][0], CLP_vert[0][1]);  /* ensure shape is closed */
      CLP_nvert = Clip2d (CLP_nvert);
      AreaMove (rp, CLP_vert[0][0], CLP_vert[0][1]);
      for (i = 1; i < CLP_nvert; i++)
	AreaDraw (rp, CLP_vert[i][0], CLP_vert[i][1]);
      AreaEnd (rp);
      CLP_nvert = 0;
    }
}
/*********/

/****** safeclip.c/SafeRectFill
* NAME
*   SafeRectFill
* SYNOPSIS
*        SafeRectFill (rp, x1, y1, x2, y2)
*   VOID SafeRectFill (struct RastPort *, LONG, LONG, LONG, LONG)
* FUNCTION
*   Draw a filled rectangle
* INPUTS
*   rp - Pointer to RastPort on which to draw.
*   (x1,y1) - Coordinates of upper left corner
*   (x2,y2) - Coordinates of lower right corner
* RESULTS
* SEE ALSO
*/
VOID
SafeRectFill (struct RastPort *rp, LONG x1, LONG y1, LONG x2, LONG y2)
{
  SafeAreaDraw (x1, y1);
  SafeAreaDraw (x2, y1);
  SafeAreaDraw (x2, y2);
  SafeAreaDraw (x1, y2);
  SafeAreaEnd (rp);
}
/*********/

/****** safeclip.c/SafeDraw
* NAME
*   SafeDraw
* SYNOPSIS
*        SafeDraw (rp, x, y)
*   VOID SafeDraw (struct RastPort *, LONG, LONG)
* FUNCTION
*   Move from previous point to new point and draw line.
* INPUTS
*   rp - Pointer to RastPort on which to draw.
*   (x,y) - Coordinates of point to draw to
* RESULTS
* SEE ALSO
*   SafeMove
*/
VOID
SafeDraw (struct RastPort *rp, LONG x, LONG y)
{
  CLP_vert[0][0] = CLP_lastx;
  CLP_vert[0][1] = CLP_lasty;
  CLP_vert[1][0] = x;
  CLP_vert[1][1] = y;
  if (ClipLine ())
    {
      Move (rp, CLP_vert[0][0], CLP_vert[0][1]);
      Draw (rp, CLP_vert[1][0], CLP_vert[1][1]);
    }
  CLP_lastx = x;
  CLP_lasty = y;
}
/*********/

/****** safeclip.c/SafeInit
* NAME
*   SafeInit
* SYNOPSIS
*   res = SafeInit (nvertmax)
*   ULONG SafeInit (ULONG)
* FUNCTION
*   Initialise clipping routines
* INPUTS
*   nvertmax - Maximum number of vertices per polygon that you will use
* RESULTS
*   res - FALSE if everything was OK, otherwise TRUE
* SEE ALSO
*   SafeClose
*/
ULONG
SafeInit (ULONG nvertmax)
{
  CLP_nvertmax = nvertmax;
  if (!(CLP_vert = (LONG (*)[2]) AllocVec (8 * CLP_nvertmax, MEMF_PUBLIC)))
    return 1;
  if (!(CLP_wrk = (LONG (*)[2]) AllocVec (8 * CLP_nvertmax, MEMF_PUBLIC)))
    {
      FreeVec (CLP_vert);
      CLP_vert = 0;
      return 2;
    }
  return 0;
}
/*********/

/****** safeclip.c/SafeClose
* NAME
*   SafeClose
* SYNOPSIS
*        SafeClose ()
*   VOID SafeClose (VOID)
* FUNCTION
*   Free memory allocated by SafeInit()
* INPUTS
* RESULTS
* SEE ALSO
*   SafeInit
*/
VOID
SafeClose (VOID)
{
  if (CLP_wrk)
    {
      FreeVec (CLP_wrk);
      CLP_wrk = 0;
    }
  if (CLP_vert)
    {
      FreeVec (CLP_vert);
      CLP_vert = 0;
    }
}
/*********/

