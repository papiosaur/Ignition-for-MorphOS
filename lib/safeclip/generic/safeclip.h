/* Clipping for Area... functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */

#ifndef SAFECLIP_H
#define SAFECLIP_H

#include <exec/types.h>
#include <graphics/rastport.h>

ULONG SafeInit (ULONG nvertmax);
VOID SafeClose (VOID);
VOID SafeSetLimits (LONG x1, LONG y1, LONG x2, LONG y2);
VOID SafeAreaDraw (LONG x, LONG y);
VOID SafeAreaEnd (struct RastPort *rp);

#endif // SAFECLIP_H
