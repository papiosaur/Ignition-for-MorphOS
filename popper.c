/* popup windows and stuff routines
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __amigaos4__
	#include <proto/gtdrag.h>
	#include "libs/textedit/TextEdit_private.h"
	#include <stdarg.h>

	#define EDF_JUSTIFICATION 3 /* the mask        */
	#define EDF_SPECIAL 64      /* draw return/tabs */

	#define ELT_WORD 0
	#define ELT_SPACE 1
	#define ELT_TAB 2
	#define ELT_NEWLINE 3
	#define ELT_END 4
#endif
#ifndef __amigaos4__
	#include "libs/textedit/include/gadgets/TextEdit.h"
	#include "libs/textedit/TextEdit_private.h"
#endif

extern struct Window *popwin;

#ifdef __amigaos4__
long PopUpListA(struct Window *win, struct Gadget *refgad, struct MinList *l, struct TagItem *tags)
{
	struct Window *popWindow;
	struct Node *n;
	struct Hook *callback;
	short  rx,ry,rw;
	short  mx,my,x,y,sx,sy,w,sh,h,itemheight;
	long   items,selected,old,count,top,oldtop,stop = 0;
	ULONG  oldidcmp;
	BOOL   scroller,ende = FALSE,inlist = FALSE,scrollmode = FALSE,noreport = FALSE;

	if (!refgad)
		return ~0L;

	rx = refgad->LeftEdge;  ry = refgad->TopEdge;  rw = refgad->Width;

	oldidcmp = win->IDCMPFlags;
	ModifyIDCMP(win, IDCMP_INTUITICKS | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_GADGETUP);

	if (!(win->Flags & WFLG_REPORTMOUSE)) {
		ReportMouse(TRUE, win);
		noreport = TRUE;
	}

	selected = GetTagData(POPA_Selected, 0, (struct TagItem *)tags);
	if (selected == ~0L)
		selected = 0;

	itemheight = GetTagData(POPA_ItemHeight,fontheight,(struct TagItem *)tags);
	items = GetTagData(POPA_MaxItems,20,(struct TagItem *)tags);
	callback = (struct Hook *)GetTagData(POPA_CallBack, (ULONG)&popUpHook, (struct TagItem *)tags);
	count = CountNodes(l);
	items = min(items, count);
	old = selected;

	if ((refgad->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET) {
		rx -= 6;
		ry -= 2;
		rw += 12;
	}

	if (FindTagItem(POPA_Width, (struct TagItem *)tags)) {
		rw = GetTagData(POPA_Width, 0, (struct TagItem *)tags);
		if (win->LeftEdge + rx + rw > scr->Width || GetTagData(POPA_Left, FALSE, (struct TagItem *)tags))
			rx -= rw;
		if (rx < 0) {
			rw += rx;
			rx = 0;
		}
	}

	x = rx;
	sx = win->LeftEdge + x;
	sy = win->TopEdge + ry + fontheight + 4;

	w = rw + boxwidth;
	sh = items * itemheight + 4;
	for (; items > 2 && sh + sy > scr->Height; sh = --items * itemheight + 4);

	scroller = items != count;

	if (sh + sy > scr->Height || items < 3 && scroller) {
		items = GetTagData(POPA_MaxItems, 20, (struct TagItem *)tags);
		items = min(items, count);

		for (sy -= fontheight + 8 + itemheight * items; sy < 0; sy += itemheight);

		sh = win->TopEdge + ry - sy;
		scroller = items != count;
	}
	y = sy - win->TopEdge + 2;
	h = sh - 4;
	top = oldtop = (count-selected < items) ? count-items : selected;
	
	if ((popWindow = OpenWindowTags(NULL,
			WA_Left,		sx,
			WA_Top,			sy,
			WA_Width,		w,
			WA_Height,		sh,
			WA_BlockPen,	0,
			WA_NoCareRefresh, TRUE,
			WA_PubScreen,	scr,
			WA_Borderless,	TRUE,
			TAG_END)) != 0) {
		struct RastPort *rp = popWindow->RPort;
		struct IntuiMessage *msg;
		struct LVDrawMsg lvdm;
		struct TextFont *font;
		
		font = OpenBitmapFont(scr->Font);
		SetFont(rp, font);

		lvdm.lvdm_MethodID = LV_DRAW;
		lvdm.lvdm_RastPort = rp;
		lvdm.lvdm_DrawInfo = dri;
		lvdm.lvdm_State = LVR_NORMAL;

		//EraseRect(rp, sx, sy, sx + w - 1, sy + sh - 1);
		SetMaxPen(rp, GetTagData(POPA_MaxPen, 7, (struct TagItem *)tags));
		if (scroller) {
			w -= 16;
			SetAPen(rp, 1);
			RectFill(rp, w + 4, 2 + (top * h) / count, w + 11, 2 + ((top + items) * h) / count - 1);
			DrawBevelBox(rp, w, 0, 16, sh, GT_VisualInfo, vi, GTBB_FrameType, BBFT_BUTTON, TAG_END);
		}

		DrawBevelBox(rp, 0, 0, w, sh, GT_VisualInfo, vi, TAG_END);
		lvdm.lvdm_Bounds.MinX = 2;
		lvdm.lvdm_Bounds.MaxX = w - 3;
		lvdm.lvdm_Bounds.MinY = 2;
		lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;

		n = FindListNumber(l, top);
		for (my = top; my < top + items; my++) {
			CallHookPkt(callback, n, &lvdm);
			lvdm.lvdm_Bounds.MinY += itemheight;
			lvdm.lvdm_Bounds.MaxY += itemheight;
			n = n->ln_Succ;
		}

		while (!ende) {
			WaitPort(iport);

			while ((msg = GTD_GetIMsg(iport)) != 0) {
				switch (msg->Class) {
					case IDCMP_GADGETUP:
					case IDCMP_MOUSEBUTTONS:
						if (!inlist)
							selected = ~0L;
						ende = TRUE;
						break;
					case IDCMP_INTUITICKS:
						if (msg->Qualifier & IEQUALIFIER_RBUTTON) {
							selected = ~0L;
							ende = TRUE;
							break;
						}
						mx = msg->MouseX;
						my = msg->MouseY;
						oldtop = top;
						if (scroller && ++stop == 2 && mx > x+w && mx < x+w+16 && my >= y && my <= y+h) {
							scrollmode = TRUE;
							SetAPen(rp, 2);
							RectFill(rp, w + 4, 2 + (top * h) / count, w + 11, 2 + ((top + items) * h) / count - 1);
						}
						if (inlist && my < y && top > 0)
							top--;
						if (inlist && my > y+h && top < count-items)
							top++;

						goto drawPopList;
						break;

					case IDCMP_MOUSEMOVE:
						mx = msg->MouseX;
						my = msg->MouseY;
						stop = 0;  oldtop = top;  old = selected;  selected = -1;
						if (scrollmode && mx < x + w) {
							scrollmode = FALSE;
							SetAPen(rp,1);
							RectFill(rp, w + 4, 2 + (top * h) / count, w + 11, 2 + ((top + items) * h) / count - 1);
						}
						if (mx >= x && mx <= x + w && my >= y && my <= y + h)
							inlist = TRUE;
						if (inlist && (mx < x || mx > x+w))
							inlist = FALSE;
						if (inlist && my >= y && my <= y+h)
							selected = (my - y - 1) / itemheight + top;

					drawPopList:
						if (scrollmode) {
							top = ((my - y - 1) * count) / h - (items >> 1);
							if (top < 0)
								top = 0;
							if (top > count - items)
								top = count - items;
						}
						if (selected != old && old >= oldtop && old <= oldtop + items - 1) {
							lvdm.lvdm_Bounds.MinY = 2 + itemheight * (old - oldtop);
							lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
							CallHookPkt(callback, FindListNumber(l, old), &lvdm);
						}
						if (top != oldtop) {
							if (abs(top-oldtop) < items - 1) {
								if (top < oldtop) {
									// Scrolling nach oben
									ClipBlit(rp, 0, 2, rp, 0, 2 + itemheight * (oldtop - top), w, itemheight * (items - oldtop + top), 0xc0);
									n = FindListNumber(l, oldtop);
									lvdm.lvdm_Bounds.MinY = 2 + itemheight * (oldtop - top);
									lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
									for (; oldtop >= top; oldtop--) {
										CallHookPkt(callback, n, &lvdm);
										lvdm.lvdm_Bounds.MinY -= itemheight;
										lvdm.lvdm_Bounds.MaxY -= itemheight;
										n = n->ln_Pred;
									}
								} else {
									// unten
									ClipBlit(rp, 0, 2 + itemheight * (top - oldtop), rp, 0, 2, w, itemheight * (items - top + oldtop), 0xc0);
									n = FindListNumber(l, --oldtop + items - 1);
									lvdm.lvdm_Bounds.MinY = 2 + itemheight * (items + oldtop - top - 1);
									lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
									for (; oldtop <= top; oldtop++) {
										CallHookPkt(callback, n, &lvdm);
										lvdm.lvdm_Bounds.MinY += itemheight;
										lvdm.lvdm_Bounds.MaxY += itemheight;
										n = n->ln_Succ;
									}
								}
							} else {
								lvdm.lvdm_Bounds.MinY = 2;
								lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
								n = FindListNumber(l, top);
								for (my = top; my < top + items; my++) {
									CallHookPkt(callback, n, &lvdm);
									lvdm.lvdm_Bounds.MinY += itemheight;
									lvdm.lvdm_Bounds.MaxY += itemheight;
									n = n->ln_Succ;
								}
							}
							if (scroller) {
								SetABPenDrMd(rp, scrollmode ? 2 : 1, 0, JAM2);
								mx = 2 + (top * h) / count;
								my = 2 + ((top + items) * h) / count;
								if (top > 0 && 2 <= mx - 1)
									EraseRect(rp, w + 4, 2, w + 11, mx - 1);

								RectFill(rp, w + 4, mx, w + 11, my - 1);

								if (my < h + 1)
									EraseRect(rp, w + 4, my, w + 11, h + 1);
							}
							old = selected;
						}

						if (selected != old && selected >= top && selected <= top+items-1) {
							lvdm.lvdm_Bounds.MinY = 2 + itemheight * (selected - top);
							lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
							lvdm.lvdm_State = LVR_SELECTED;
							CallHookPkt(callback, FindListNumber(l, selected), &lvdm);
							lvdm.lvdm_State = LVR_NORMAL;
						}
						break;
				}

				GTD_ReplyIMsg(msg);
			}
		}
		
		CloseFont(font);
		CloseWindow(popWindow);
	}

	if (noreport)
		ReportMouse(FALSE, win);

	ModifyIDCMP(win, oldidcmp);

	return selected;
}

long VARARGS68K PopUpList(struct Window *,struct Gadget *,struct MinList *l, ...);
long PopUpList(struct Window *win, struct Gadget *refgad, struct MinList *l, ...)
{
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, l);
	tags = va_getlinearva(ap, struct TagItem *);
    return PopUpListA(win, refgad, l, tags);
}

#else
long
PopUpList(struct Window *win, struct Gadget *refgad, struct MinList *l, ULONG tag1,...)
{
	struct Window *popWindow;
	struct Node *n;
	struct Hook *callback;
	short  rx,ry,rw;
	short  mx,my,x,y,sx,sy,w,sh,h,itemheight;
	long   items,selected,old,count,top,oldtop,stop = 0;
	ULONG  oldidcmp;
	BOOL   scroller,ende = FALSE,inlist = FALSE,scrollmode = FALSE,noreport = FALSE;

	if (!refgad)
		return ~0L;

	rx = refgad->LeftEdge;  ry = refgad->TopEdge;  rw = refgad->Width;

	oldidcmp = win->IDCMPFlags;
	ModifyIDCMP(win, IDCMP_INTUITICKS | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_GADGETUP);

	if (!(win->Flags & WFLG_REPORTMOUSE)) {
		ReportMouse(TRUE, win);
		noreport = TRUE;
	}

	selected = GetTagData(POPA_Selected, 0, (struct TagItem *)&tag1);
	if (selected == ~0L)
		selected = 0;

	itemheight = GetTagData(POPA_ItemHeight,fontheight,(struct TagItem *)&tag1);
	items = GetTagData(POPA_MaxItems,20,(struct TagItem *)&tag1);
	callback = (struct Hook *)GetTagData(POPA_CallBack, (ULONG)&popUpHook, (struct TagItem *)&tag1);
	count = CountNodes(l);
	items = min(items, count);
	old = selected;

	if ((refgad->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET) {
		rx -= 6;
		ry -= 2;
		rw += 12;
	}

	if (FindTagItem(POPA_Width, (struct TagItem *)&tag1)) {
		rw = GetTagData(POPA_Width, 0, (struct TagItem *)&tag1);
		if (win->LeftEdge + rx + rw > scr->Width || GetTagData(POPA_Left, FALSE, (struct TagItem *)&tag1))
			rx -= rw;
		if (rx < 0) {
			rw += rx;
			rx = 0;
		}
	}

	x = rx;
	sx = win->LeftEdge + x;
	sy = win->TopEdge + ry + fontheight + 4;

	w = rw + boxwidth;
	sh = items * itemheight + 4;
	for (; items > 2 && sh + sy > scr->Height; sh = --items * itemheight + 4);

	scroller = items != count;

	if (sh + sy > scr->Height || items < 3 && scroller) {
		items = GetTagData(POPA_MaxItems, 20, (struct TagItem *)&tag1);
		items = min(items, count);

		for (sy -= fontheight + 8 + itemheight * items; sy < 0; sy += itemheight);

		sh = win->TopEdge + ry - sy;
		scroller = items != count;
	}
	y = sy - win->TopEdge + 2;
	h = sh - 4;
	top = oldtop = (count-selected < items) ? count-items : selected;
	
	if ((popWindow = OpenWindowTags(NULL,
			WA_Left,		sx,
			WA_Top,			sy,
			WA_Width,		w,
			WA_Height,		sh,
			WA_BlockPen,	0,
			WA_NoCareRefresh, TRUE,
			WA_PubScreen,	scr,
			WA_Borderless,	TRUE,
			TAG_END)) != 0) {
		struct RastPort *rp = popWindow->RPort;
		struct IntuiMessage *msg;
		struct LVDrawMsg lvdm;
		struct TextFont *font;
		
		font = OpenBitmapFont(scr->Font);
		SetFont(rp, font);

		lvdm.lvdm_MethodID = LV_DRAW;
		lvdm.lvdm_RastPort = rp;
		lvdm.lvdm_DrawInfo = dri;
		lvdm.lvdm_State = LVR_NORMAL;

		//EraseRect(rp, sx, sy, sx + w - 1, sy + sh - 1);
		SetMaxPen(rp, GetTagData(POPA_MaxPen, 7, (struct TagItem *)&tag1));
		if (scroller) {
			w -= 16;
			SetAPen(rp, 1);
			RectFill(rp, w + 4, 2 + (top * h) / count, w + 11, 2 + ((top + items) * h) / count - 1);
			DrawBevelBox(rp, w, 0, 16, sh, GT_VisualInfo, vi, GTBB_FrameType, BBFT_BUTTON, TAG_END);
		}

		DrawBevelBox(rp, 0, 0, w, sh, GT_VisualInfo, vi, TAG_END);
		lvdm.lvdm_Bounds.MinX = 2;
		lvdm.lvdm_Bounds.MaxX = w - 3;
		lvdm.lvdm_Bounds.MinY = 2;
		lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;

		n = FindListNumber(l, top);
		for (my = top; my < top + items; my++) {
			CallHookPkt(callback, n, &lvdm);
			lvdm.lvdm_Bounds.MinY += itemheight;
			lvdm.lvdm_Bounds.MaxY += itemheight;
			n = n->ln_Succ;
		}

		while (!ende) {
			WaitPort(iport);

			while ((msg = GTD_GetIMsg(iport)) != 0) {
				switch (msg->Class) {
					case IDCMP_GADGETUP:
					case IDCMP_MOUSEBUTTONS:
						if (!inlist)
							selected = ~0L;
						ende = TRUE;
						break;
					case IDCMP_INTUITICKS:
						if (msg->Qualifier & IEQUALIFIER_RBUTTON) {
							selected = ~0L;
							ende = TRUE;
							break;
						}
						mx = msg->MouseX;
						my = msg->MouseY;
						oldtop = top;
						if (scroller && ++stop == 2 && mx > x+w && mx < x+w+16 && my >= y && my <= y+h) {
							scrollmode = TRUE;
							SetAPen(rp, 2);
							RectFill(rp, w + 4, 2 + (top * h) / count, w + 11, 2 + ((top + items) * h) / count - 1);
						}
						if (inlist && my < y && top > 0)
							top--;
						if (inlist && my > y+h && top < count-items)
							top++;

						goto drawPopList;
						break;

					case IDCMP_MOUSEMOVE:
						mx = msg->MouseX;
						my = msg->MouseY;
						stop = 0;  oldtop = top;  old = selected;  selected = -1;
						if (scrollmode && mx < x + w) {
							scrollmode = FALSE;
							SetAPen(rp,1);
							RectFill(rp, w + 4, 2 + (top * h) / count, w + 11, 2 + ((top + items) * h) / count - 1);
						}
						if (mx >= x && mx <= x + w && my >= y && my <= y + h)
							inlist = TRUE;
						if (inlist && (mx < x || mx > x+w))
							inlist = FALSE;
						if (inlist && my >= y && my <= y+h)
							selected = (my - y - 1) / itemheight + top;

					drawPopList:
						if (scrollmode) {
							top = ((my - y - 1) * count) / h - (items >> 1);
							if (top < 0)
								top = 0;
							if (top > count - items)
								top = count - items;
						}
						if (selected != old && old >= oldtop && old <= oldtop + items - 1) {
							lvdm.lvdm_Bounds.MinY = 2 + itemheight * (old - oldtop);
							lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
							CallHookPkt(callback, FindListNumber(l, old), &lvdm);
						}
						if (top != oldtop) {
							if (abs(top-oldtop) < items - 1) {
								if (top < oldtop) {
									// Scrolling nach oben
									ClipBlit(rp, 0, 2, rp, 0, 2 + itemheight * (oldtop - top), w, itemheight * (items - oldtop + top), 0xc0);
									n = FindListNumber(l, oldtop);
									lvdm.lvdm_Bounds.MinY = 2 + itemheight * (oldtop - top);
									lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
									for (; oldtop >= top; oldtop--) {
										CallHookPkt(callback, n, &lvdm);
										lvdm.lvdm_Bounds.MinY -= itemheight;
										lvdm.lvdm_Bounds.MaxY -= itemheight;
										n = n->ln_Pred;
									}
								} else {
									// unten
									ClipBlit(rp, 0, 2 + itemheight * (top - oldtop), rp, 0, 2, w, itemheight * (items - top + oldtop), 0xc0);
									n = FindListNumber(l, --oldtop + items - 1);
									lvdm.lvdm_Bounds.MinY = 2 + itemheight * (items + oldtop - top - 1);
									lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
									for (; oldtop <= top; oldtop++) {
										CallHookPkt(callback, n, &lvdm);
										lvdm.lvdm_Bounds.MinY += itemheight;
										lvdm.lvdm_Bounds.MaxY += itemheight;
										n = n->ln_Succ;
									}
								}
							} else {
								lvdm.lvdm_Bounds.MinY = 2;
								lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
								n = FindListNumber(l, top);
								for (my = top; my < top + items; my++) {
									CallHookPkt(callback, n, &lvdm);
									lvdm.lvdm_Bounds.MinY += itemheight;
									lvdm.lvdm_Bounds.MaxY += itemheight;
									n = n->ln_Succ;
								}
							}
							if (scroller) {
								SetABPenDrMd(rp, scrollmode ? 2 : 1, 0, JAM2);
								mx = 2 + (top * h) / count;
								my = 2 + ((top + items) * h) / count;
								if (top > 0 && 2 <= mx - 1)
									EraseRect(rp, w + 4, 2, w + 11, mx - 1);

								RectFill(rp, w + 4, mx, w + 11, my - 1);

								if (my < h + 1)
									EraseRect(rp, w + 4, my, w + 11, h + 1);
							}
							old = selected;
						}

						if (selected != old && selected >= top && selected <= top+items-1) {
							lvdm.lvdm_Bounds.MinY = 2 + itemheight * (selected - top);
							lvdm.lvdm_Bounds.MaxY = lvdm.lvdm_Bounds.MinY + itemheight - 1;
							lvdm.lvdm_State = LVR_SELECTED;
							CallHookPkt(callback, FindListNumber(l, selected), &lvdm);
							lvdm.lvdm_State = LVR_NORMAL;
						}
						break;
				}

				GTD_ReplyIMsg(msg);
			}
		}
		
		CloseFont(font);
		CloseWindow(popWindow);
	}

	if (noreport)
		ReportMouse(FALSE, win);

	ModifyIDCMP(win, oldidcmp);

	return selected;
}
#endif

#define PWIDTH (boxwidth+2)
#define PHEIGHT (fontheight+6)


#ifdef __amigaos4__
long PopUpTableA(struct Window *win, struct Gadget *refgad, UWORD cols, UWORD rows, APTR func, struct TagItem *tags)
{
	struct RastPort *rp = &scr->RastPort;
	struct IntuiMessage *msg;
	struct BitMap *bm;
	short  px,py,pw,ph,rx,ry;
	short  mx,my,x,y,sx,sy,w,h;
	long   selected = -1,old;
	ULONG  oldidcmp;
	BOOL   ende = FALSE,in = FALSE,noreport = FALSE;

	if (!refgad)
		return ~0L;

	px = refgad->LeftEdge+win->LeftEdge;  py = refgad->TopEdge+refgad->Height+win->TopEdge;
	pw = cols*PWIDTH+6;  ph = PHEIGHT*cols+4;

	oldidcmp = win->IDCMPFlags;  ModifyIDCMP(win,oldidcmp | IDCMP_INTUITICKS | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE);
	if (!(win->Flags & WFLG_REPORTMOUSE)) {
		ReportMouse(TRUE,win);
		noreport = TRUE;
	}

	if (px+pw > scr->Width || GetTagData(POPA_Left, FALSE, (struct TagItem *)tags))
		px = px+refgad->Width-pw;
	if (px < 0)
		px = 0;

	if ((bm = AllocBitMap(pw, ph, GetBitMapAttr(rp->BitMap, BMA_DEPTH), BMF_MINPLANES, rp->BitMap)) != 0) {
		LockLayers(&scr->LayerInfo);
		UnlockLayer(win->RPort->Layer);
		BltBitMap(rp->BitMap,px,py,bm,0,0,pw,ph,0xc0,0xff,NULL);
		EraseRect(rp,px,py,px+pw-1,py+ph-1);

		DrawBevelBox(rp,px,py,pw,ph,GT_VisualInfo,vi,TAG_END);
		((ASM void (*)(REG (a0, struct RastPort *), REG(d0, UWORD), REG(d1, UWORD), REG(d2, UWORD), REG(d3, UWORD)))func)(rp, rx = px+4, ry = py+3, cols, rows);
		x = rx-win->LeftEdge;  y = ry-win->TopEdge;
		w = pw-8;  h = ph-6;

		while (!ende) {
			WaitPort(iport);
			while ((msg = (struct IntuiMessage *)GetMsg(iport)) != 0) {
				switch (msg->Class) {
					case IDCMP_GADGETUP:
						selected = ~0L;
						ende = TRUE;
						break;
					case IDCMP_MOUSEBUTTONS:
						if (!in)
							selected = ~0L;
						ende = TRUE;
						break;
					case IDCMP_INTUITICKS:
						if (msg->Qualifier & IEQUALIFIER_RBUTTON) {
							selected = ~0L;
							ende = TRUE;
							break;
						}
						break;
					case IDCMP_MOUSEMOVE:
						mx = msg->MouseX;
						my = msg->MouseY;
						old = selected;  selected = -1;

						if (mx >= x && mx < x+w && my >= y && my < y+h) {
							in = TRUE;
							sx = (mx-x)/PWIDTH;  sy = (my-y)/PHEIGHT;
							selected = sy*cols+sx;
						} else
							in = FALSE;

						if (old != -1 && (!in || in && old != selected)) {
							mx = rx + (old % cols) * PWIDTH - 2;
							my = ry + (old / cols) * PHEIGHT - 2;
							EraseFatRect(rp, mx, my, mx + PWIDTH + 1, my + PHEIGHT + 1);
						}
						
						if (in && old != selected) {
							SetAPen(rp,3);
							mx = rx + sx * PWIDTH - 2;
							my = ry + sy * PHEIGHT - 2;
							DrawFatRect(rp, mx, my, mx + PWIDTH + 1, my + PHEIGHT + 1);
						}
						break;
				}
				ReplyMsg((struct Message *)msg);
			}
		}
		BltBitMapRastPort(bm,0,0,rp,px,py,pw,ph,0xc0);
		FreeBitMap(bm);
		LockLayer(0, win->RPort->Layer);
		UnlockLayers(&scr->LayerInfo);
	}

	if (noreport)
		ReportMouse(FALSE, win);

	ModifyIDCMP(win,oldidcmp);
	return selected ;
}

long VARARGS68K PopUpTable(struct Window *win,struct Gadget *refgad,UWORD cols,UWORD rows,APTR func,...);
long PopUpTable(struct Window *win, struct Gadget *refgad, UWORD cols, UWORD rows, APTR func,  ...)
{
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, func);
	tags = va_getlinearva(ap, struct TagItem *);
    return PopUpTableA(win, refgad, cols, rows, func, tags);
}

#else
long
PopUpTable(struct Window *win, struct Gadget *refgad, UWORD cols, UWORD rows, APTR func, ULONG tag1, ...)
{
	struct RastPort *rp = &scr->RastPort;
	struct IntuiMessage *msg;
	struct BitMap *bm;
	short  px,py,pw,ph,rx,ry;
	short  mx,my,x,y,sx,sy,w,h;
	long   selected = -1,old;
	ULONG  oldidcmp;
	BOOL   ende = FALSE,in = FALSE,noreport = FALSE;

	if (!refgad)
		return ~0L;

	px = refgad->LeftEdge+win->LeftEdge;  py = refgad->TopEdge+refgad->Height+win->TopEdge;
	pw = cols*PWIDTH+6;  ph = PHEIGHT*cols+4;

	oldidcmp = win->IDCMPFlags;  ModifyIDCMP(win,oldidcmp | IDCMP_INTUITICKS | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE);
	if (!(win->Flags & WFLG_REPORTMOUSE)) {
		ReportMouse(TRUE,win);
		noreport = TRUE;
	}

	if (px+pw > scr->Width || GetTagData(POPA_Left, FALSE, (struct TagItem *)&tag1))
		px = px+refgad->Width-pw;
	if (px < 0)
		px = 0;

	if ((bm = AllocBitMap(pw, ph, GetBitMapAttr(rp->BitMap, BMA_DEPTH), BMF_MINPLANES, rp->BitMap)) != 0) {
#ifndef __AROS__ /* quick fix to avoid deadlock */
		LockLayers(&scr->LayerInfo);
		UnlockLayer(win->RPort->Layer);
#endif
		BltBitMap(rp->BitMap,px,py,bm,0,0,pw,ph,0xc0,0xff,NULL);
		EraseRect(rp,px,py,px+pw-1,py+ph-1);

		DrawBevelBox(rp,px,py,pw,ph,GT_VisualInfo,vi,TAG_END);
		((ASM void (*)(REG (a0, struct RastPort *), REG(d0, UWORD), REG(d1, UWORD), REG(d2, UWORD), REG(d3, UWORD)))func)(rp, rx = px+4, ry = py+3, cols, rows);
		x = rx-win->LeftEdge;  y = ry-win->TopEdge;
		w = pw-8;  h = ph-6;

		while (!ende) {
			WaitPort(iport);
			while ((msg = (struct IntuiMessage *)GetMsg(iport)) != 0) {
				switch (msg->Class) {
					case IDCMP_GADGETUP:
						selected = ~0L;
						ende = TRUE;
						break;
					case IDCMP_MOUSEBUTTONS:
						if (!in)
							selected = ~0L;
						ende = TRUE;
						break;
					case IDCMP_INTUITICKS:
						if (msg->Qualifier & IEQUALIFIER_RBUTTON) {
							selected = ~0L;
							ende = TRUE;
							break;
						}
						break;
					case IDCMP_MOUSEMOVE:
						mx = msg->MouseX;
						my = msg->MouseY;
						old = selected;  selected = -1;

						if (mx >= x && mx < x+w && my >= y && my < y+h) {
							in = TRUE;
							sx = (mx-x)/PWIDTH;  sy = (my-y)/PHEIGHT;
							selected = sy*cols+sx;
						} else
							in = FALSE;

						if (old != -1 && (!in || in && old != selected)) {
							mx = rx + (old % cols) * PWIDTH - 2;
							my = ry + (old / cols) * PHEIGHT - 2;
							EraseFatRect(rp, mx, my, mx + PWIDTH + 1, my + PHEIGHT + 1);
						}
						
						if (in && old != selected) {
							SetAPen(rp,3);
							mx = rx + sx * PWIDTH - 2;
							my = ry + sy * PHEIGHT - 2;
							DrawFatRect(rp, mx, my, mx + PWIDTH + 1, my + PHEIGHT + 1);
						}
						break;
				}
				ReplyMsg((struct Message *)msg);
			}
		}
		BltBitMapRastPort(bm,0,0,rp,px,py,pw,ph,0xc0);
		FreeBitMap(bm);
#ifndef __AROS__ /* quick fix to avoid deadlock */
		LockLayer(0, win->RPort->Layer);
		UnlockLayers(&scr->LayerInfo);
#endif
	}

	if (noreport)
		ReportMouse(FALSE, win);

	ModifyIDCMP(win,oldidcmp);
	return selected ;
}
#endif

UWORD ptrn[16][4] = {
	{0x0000,0x0000,0x0000,0x0000},
	{0x0000,0x0000,0x0000,0x1111},
	{0x0000,0x0000,0x3333,0x3333},
//  {0xffff,0xffff,0xcccc,0xcccc},
	{0x4444,0x0000,0x1111,0x0000},
	{0x5555,0x5555,0x5555,0x5555},
	{0x0000,0xffff,0x0000,0xffff},
	{0x3333,0x3333,0x3333,0x3333},
	{0x0000,0x0000,0xffff,0xffff},
	{0x5555,0xaaaa,0x5555,0xaaaa},
	{0xcccc,0xcccc,0x3333,0x3333},
	{0x8888,0x4444,0x2222,0x1111},
	{0xcccc,0x9999,0x3333,0x6666},
	{0x7777,0xaaaa,0xdddd,0xaaaa},
	{0x8888,0x5555,0x2222,0x5555},
	{0x1111,0x2222,0x4444,0x8888},
	{0x6666,0x3333,0x9999,0xcccc}
};


UWORD
shift(UWORD value,UBYTE s)
{
	if (!s)
		return(value);

	{
		long hi = value << s;

		return((UWORD)(hi | (hi >> 16)));
	}
}


void
SetPattern(struct RastPort *rp, UBYTE pattern, UBYTE x, UBYTE y)
{
	static UWORD p[4];
	short  i;

	for (i = 0; i < 4; i++, y++) {
		if (y > 3)
			y = 0;
		p[i] = shift(ptrn[pattern][y],x);
	}
	SetAfPt(rp, p, 2);
}


void PUBLIC
PatternPopper(REG(a0, struct RastPort *rp), REG(d0, UWORD x), REG(d1, UWORD y), REG(d2, UWORD cols), REG(d3, UWORD rows))
{
	long c = 0,a,b;

	SetBPen(rp, 0);
	for (a = 0; a < rows; a++) {
		for (b = 0; b < cols; b++, c++) {
			SetAPen(rp, 1);  SetPattern(rp, c, 0, 0);
			RectFill(rp, x + PWIDTH * b, y, x + PWIDTH * b + PWIDTH - 3, y + PHEIGHT - 3);
		}
		y += PHEIGHT;
	}
	SetAfPt(rp, NULL, 0);
}


void PUBLIC
ColorPopper(REG(a0, struct RastPort *rp), REG(d0, UWORD x), REG(d1, UWORD y), REG(d2, UWORD cols), REG(d3, UWORD rows))
{
	struct colorPen *cp = (APTR)colors.mlh_Head;
	long a, b;

	for (a = 0; a < rows; a++) {
		for (b = 0; b < cols && cp->cp_Node.ln_Succ; b++, cp = (APTR)cp->cp_Node.ln_Succ) {
			SetColorPen(rp, cp);
			RectFill(rp, x + PWIDTH * b, y, x + PWIDTH * b + PWIDTH - 3, y + PHEIGHT - 3);
		}
		y += PHEIGHT;
	}
}


long
PopColors(struct Window *win, struct Gadget *gad)
{
	long selected;

	if ((selected = PopUpList(win, gad, &colors,
			POPA_CallBack, &colorHook,
			POPA_MaxPen, ~0L,
			POPA_ItemHeight, fontheight + 4,
			POPA_Width, GetListWidth(&colors) + boxwidth + 5,
			TAG_END)) != -1) {
		struct colorPen *cp;

		for (cp = (APTR)colors.mlh_Head;selected--;cp = (APTR)cp->cp_Node.ln_Succ);
		selected = cp->cp_ID;
	}

	return selected;
}


#ifdef __amigaos4__
VOID FreeEditList(REG(a0, struct EditGData * ed))
{
  struct MinNode *mln;
  struct EditLine *el;
  long   count;

  while((mln = (APTR)RemHead((struct List *)&ed->ed_List)) != 0)
  {
    for(count = 0,el = EDITLINE(mln);el->el_Word;el++,count++);
    FreePooled(ed->ed_Pool,mln,sizeof(struct MinNode)+sizeof(STRPTR)+sizeof(ULONG)+sizeof(struct EditLine)*count);
  }
  ed->ed_Top = ed->ed_List.mlh_Head;
  ed->ed_TextLines = 0;
}


void 
JustifyEditLine(struct EditGData *ed, struct EditLine *fel, long width, BOOL lastLine)
{
  long   count = 0,add = 0,type = 0,i;
  struct EditLine *el;

  for(el = fel;el->el_Word && (el+1)->el_Word;el++) /* Are there any tabs? */
  {
    if (el->el_Type == ELT_TAB)
      count++;
  }
  if (el == fel || (--el)->el_Type == ELT_NEWLINE || !*(el->el_Word+el->el_Length) || lastLine)
    return;                      /* newline or end of text */
  if (count)                     /* There are tabs in this line */
  {
    add = (ed->ed_Width-width)/count;
    type = ELT_TAB;
  }
  else                           /* no tabs */
  {
    for(el = fel;el->el_Word && (el+1)->el_Word;el++) /* Are there any spaces? */
    {
      if (el->el_Type == ELT_SPACE)
        count++;
    }
    if (count)                     /* There are spaces in this line */
    {
      add = (ed->ed_Width-width)/count;
      if (add > ed->ed_MaxSpace)
        add = ed->ed_MaxSpace;
      type = ELT_SPACE;
    }
  }
  if (count)
  {
    for(el = fel;el->el_Word && (el+1)->el_Word;el++)
    {
      if (el->el_Type == type)
        el->el_Width += add;
    }
    i = ed->ed_Width-width-add*count;
    while(i > 0 && (type == ELT_TAB || add < ed->ed_MaxSpace))
    {
      for(el = fel;el->el_Word && (el+1)->el_Word && i > 0;el++)
      {
        if (el->el_Type == type)
          el->el_Width++,  i--,  add++;
      }
    }
  }
}


BOOL PrepareEditText(struct EditGData * ed, struct RastPort * rp, STRPTR t)
{
  struct EditLine *stack;
  long   size = 256;
  STRPTR s = t;

  if (!ed)
    return 0;
  FreeEditList(ed);

  if (t && (stack = AllocPooled(ed->ed_Pool,sizeof(struct EditLine)*size)))
  {
    struct MinNode *mln;
    long   l,x = 0,count = 0,w = 0;

    while(*s)
    {
      stack[count].el_Word = s;
      if (*s == ' ')                       /* it is a space */
      {
        for(l = 0;*s && *s == ' ';s++,l++); /* count spaces */
        stack[count].el_Length = l;
        stack[count].el_Width = ed->ed_MinSpace*l;
        stack[count].el_Type = ELT_SPACE;
      }
      else if (*s == '\t')                 /* it is a tab */
      {
        for(l = 0;*s && *s == '\t';s++,l++); /* count tabs */
        stack[count].el_Length = l;
        stack[count].el_Width = ed->ed_MinSpace*ed->ed_TabSpaces*l;
        stack[count].el_Type = ELT_TAB;
      }
      else if (*s == '\n')                 /* it is a newline */
      {
        s++;
        stack[count].el_Length = 1;
        if (ed->ed_Flags & EDF_SPECIAL)
          stack[count].el_Width = TextLength(rp,"¶",1);
        else
          stack[count].el_Width = ed->ed_CharWidth;
        stack[count].el_Type = ELT_NEWLINE;
      }
      else                                 /* it is a word */
      {
        for(l = 0;*s && *s != ' ' && *s != '\t' && *s != '\n';s++,l++); /* count word characters */
        stack[count].el_Length = l;
        stack[count].el_Width = TextLength(rp,stack[count].el_Word,l);
        stack[count].el_Type = ELT_WORD;
      }
      x += stack[count].el_Width;

      if (stack[count].el_Type == ELT_WORD && x > ed->ed_Width || (!*s || stack[count].el_Type == ELT_NEWLINE) && ++count)   /* neue Zeile */
      {
        if (x > ed->ed_Width)
        {
          if (count)                /* move last word to the next line */
          {
            if (stack[count].el_Word)
              s = stack[count].el_Word;
          }
          else                      /* one word is too long */
            count++;
        }
        if ((mln = AllocPooled(ed->ed_Pool,sizeof(struct MinNode)+sizeof(STRPTR)+sizeof(ULONG)+sizeof(struct EditLine)*count)) != 0)
        {
          AddTail((struct List *)&ed->ed_List,(struct Node *)mln);
          if (!*s)
            w += stack[count-1].el_Width;
          if ((ed->ed_Flags & EDF_JUSTIFICATION) == EDJ_RIGHT || (ed->ed_Flags & EDF_JUSTIFICATION) == EDJ_CENTERED)
          {
            if ((ed->ed_Flags & EDF_JUSTIFICATION) == EDJ_RIGHT)
              w = ed->ed_Width-1-w;
            else
              w = (ed->ed_Width-w-1) >> 1;
            if (w < 0)
              w = 0;
            LINEOFFSET(mln) = w;
          }
          ed->ed_TextLines++;
          CopyMem(stack,EDITLINE(mln),count*sizeof(struct EditLine));
          if ((ed->ed_Flags & EDF_JUSTIFICATION) == EDJ_JUSTIFY)
			JustifyEditLine(ed, EDITLINE(mln), w, !s[0]);
          x = 0;  count = 0;  w = 0;
        }
      }
      else
        w += stack[count++].el_Width;

      if (count > size)     /* stack overflow: enlarge the stack */
      {
        struct EditLine *temp;

        if ((temp = AllocPooled(ed->ed_Pool,sizeof(struct EditLine)*(size+256))) != 0)
        {
          CopyMem(stack,temp,(count-1)*sizeof(struct EditLine));
          FreePooled(ed->ed_Pool,stack,sizeof(struct EditLine)*size);
          stack = temp;  size += 256;
        }
        else
          DisplayBeep(NULL);
      }
    }
    FreePooled(ed->ed_Pool,stack,sizeof(struct EditLine)*size);
    if (s > t && *(s-1) == '\n')
    {
      struct MinNode *mln;

      if ((mln = AllocPooled(ed->ed_Pool,sizeof(struct MinNode)+sizeof(STRPTR)+sizeof(ULONG)+sizeof(struct EditLine))) != 0)
      {
        AddTail((struct List *)&ed->ed_List,(struct Node *)mln);
        stack = EDITLINE(mln);
        stack->el_Word = s;
        stack->el_Type = ELT_END;
        ed->ed_TextLines++;
      }
    }
  }
  ed->ed_Top = ed->ed_List.mlh_Head;
  return(TRUE);
}
#endif


void
ClosePopUpText(void)
{
	struct EditGData *ed;

	if (!popwin)
		return;

	ed = (APTR)popwin->UserData;
	ModifyIDCMP((struct Window *)ed->ed_Scroller, ed->ed_FrameType);
	CloseWindow(popwin);
	popwin = NULL;
	PrepareEditText(ed, NULL, NULL);     /* free lines */
}


void
ShowPopUpText(STRPTR t, long wx, long wy)
{
	static struct EditGData ed;     /* use a function of editgclass */

	struct EditLine *el;
	struct MinNode *mln;
	long height, tw, x, y, bpen;

	if (!t)
		return;
	if (popwin)
		ClosePopUpText();
	ed.ed_Pool = pool;
	ed.ed_TabSpaces = 8;
	ed.ed_Width = scr->Width/4;
	ed.ed_GadWidth = ed.ed_Width+8;
	ed.ed_Flags = EDJ_LEFT /*EDJ_JUSTIFY*/;
	ed.ed_CharWidth = scr->RastPort.Font->tf_XSize >> 1;
	ed.ed_LineHeight = fontheight+1;
	ed.ed_Lines = 999;
	ed.ed_MinSpace = TextLength(&scr->RastPort," ",1);
	ed.ed_MaxSpace = 4*ed.ed_MinSpace;
	ed.ed_Text = t;
	MyNewList(&ed.ed_List);
	PrepareEditText(&ed,&scr->RastPort,t);  /* create lines */

	height = scr->Height >> 1;
	height = min(ed.ed_TextLines*ed.ed_LineHeight,height-(height % ed.ed_LineHeight))+4;

	// find maximum text width
	for (mln = ed.ed_Top, tw = 0; mln->mln_Succ; mln = mln->mln_Succ) {
		for(x = 0,el = EDITLINE(mln);el->el_Word;el++)
			x += el->el_Width;
		if (tw < x)
			tw = x;
	}
	ed.ed_GadWidth = tw+8;

	x = 12+imsg.MouseX+win->LeftEdge;
	if (x > scr->Width-ed.ed_GadWidth) {
		if (imsg.MouseY+win->TopEdge+height >= scr->Height) {
			if (imsg.MouseY+win->TopEdge-height < 0)
				x = imsg.MouseX+win->LeftEdge-ed.ed_GadWidth-12;
			else
				y = imsg.MouseY+win->TopEdge-height-5;
		} else
			y = imsg.MouseY+win->TopEdge+5;
	} else
		y = imsg.MouseY+win->TopEdge-height/2;

	if ((popwin = OpenWindowTags(NULL,
			WA_Left,       wx == -1 ? x : wx,
			WA_Top,        wy == -1 ? y : wy,
			WA_Width,      ed.ed_GadWidth,
			WA_Height,     height,
			WA_NoCareRefresh,TRUE,
			/*WA_AutoAdjust, TRUE,*/
			WA_PubScreen,  scr,
			WA_Borderless, TRUE,
			TAG_END)) != 0) {
		struct RastPort *rp = popwin->RPort;
		long   base;

		popwin->UserData = (APTR)&ed;
		ed.ed_Scroller = (APTR)win;
		//ReportMouse(TRUE,win);
		ed.ed_FrameType = win->IDCMPFlags;
		ModifyIDCMP(win,ed.ed_FrameType | IDCMP_MOUSEMOVE);

		SetFont(rp,scr->RastPort.Font);
		ed.ed_GadWidth = popwin->Width;
		height = popwin->Height;
		//DrawBevelBox(rp,0,0,ed.ed_GadWidth,height,GT_VisualInfo,vi,TAG_END);
		SetAPen(rp,dri->dri_Pens[SHADOWPEN]);
		DrawRect(rp,0,0,ed.ed_GadWidth-1,height-1);
		SetAPen(rp,bpen = FindColor(scr->ViewPort.ColorMap,0xeeeeeeee,0xeeeeeeee,0xe2e2e2e2,-1));
		RectFill(rp,1,1,ed.ed_GadWidth-2,height-2);
		y = 2+(base = rp->Font->tf_Baseline);
		SetABPenDrMd(rp,dri->dri_Pens[TEXTPEN],bpen,JAM2);

		for (mln = ed.ed_Top;mln->mln_Succ && y < height;mln = mln->mln_Succ) {
			x = 4+LINEOFFSET(mln);
			el = EDITLINE(mln);
			for (;el->el_Word;el++) {
				if (!el->el_Type) {
					Move(rp,x,y);
					Text(rp,el->el_Word,el->el_Length);
				}
				x += el->el_Width;
			}
			y += ed.ed_LineHeight;
		}
	} else
		PrepareEditText(&ed, NULL, NULL);     /* free lines */
//#endif
}

