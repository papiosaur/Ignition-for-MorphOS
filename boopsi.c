/* BOOPSI Classes
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#ifdef __MORPHOS__
#include <cybergraphx/cybergraphics.h>
#else
#include "cybergraphics.h"
#endif
#include <graphics/rpattr.h>
#include <intuition/gadgetclass.h>
//#include <datatypes/pictureclass.h>
#if defined  __amigaos4__ || defined __MORPHOS__
	#include <proto/gtdrag.h>
	#include <proto/cybergraphics.h>
#endif

#if !defined(PDTM_WRITEPIXELARRAY)
#	define PBPAFMT_RGB  0	/* 3 bytes per pixel (red, green, blue) */
#	define PBPAFMT_RGBA 1	/* 4 bytes per pixel (red, green, blue, alpha channel) */
#	define PBPAFMT_ARGB 2	/* 4 bytes per pixel (alpha channel, red, green, blue) */

#	define PDTA_SourceMode	(DTA_Dummy + 250)
#	define PDTA_DestMode	(DTA_Dummy + 251)
#	define PMODE_V43 (1)	/* Extended mode */
#	define PDTM_WRITEPIXELARRAY (DTM_Dummy + 0x60)

// picture.datatype v44
#	define LOW_DITHER_QUALITY
#endif

#define TRACE_BOOPSI 0
#if TRACE_BOOPSI
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif
 
#define IM(o) ((struct Image *)o)


extern ULONG PUBLIC RenderHook(REG(a0, struct Hook *h),REG(a2, struct ImageNode *in),REG(a1, struct LVDrawMsg *msg));
extern ULONG PUBLIC PopUpHook(REG(a0, struct Hook *h),REG(a2, struct Node *n),REG(a1, struct LVDrawMsg *msg));
extern ULONG PUBLIC FormelHook(REG(a0, struct Hook *h),REG(a2, struct Node *n),REG(a1, struct LVDrawMsg *msg));
extern ULONG PUBLIC gLinkHook(REG(a0, struct Hook *h),REG(a2, struct gLink *gl),REG(a1, struct LVDrawMsg *msg));
extern ULONG PUBLIC LinkHook(REG(a0, struct Hook *h),REG(a2, struct Link *l),REG(a1, struct LVDrawMsg *msg));
extern ULONG PUBLIC FormatHook(REG(a0, struct Hook *h),REG(a2, struct Node *n),REG(a1, struct LVDrawMsg *msg));
extern ULONG PUBLIC SelectHook(REG(a0, struct Hook *h),REG(a2, struct Node *n),REG(a1, struct LVDrawMsg *msg));
extern ULONG PUBLIC ColorHook(REG(a0, struct Hook *h),REG(a2, struct colorPen *cp),REG(a1, struct LVDrawMsg *msg));
extern void PUBLIC fillHookFunc(REG(a0, struct Hook *h), REG(a2, struct RastPort *rastp), REG(a1, struct Rectangle *rect));


/********************************** Page-Gadget **********************************/

void
AddPageGadgets(struct RastPort *rp,struct Gadget *gad,struct PageGData *pd)
{
	int active = pd->pd_Active;

	if (!pd || !pd->pd_Window || !pd->pd_Pages[active])
		return;

	pd->pd_Gadgets = pd->pd_Pages[active];
	AddGList(pd->pd_Window,pd->pd_Gadgets,-1,-1,NULL);
	RefreshGList(pd->pd_Gadgets,pd->pd_Window,NULL,pd->pd_GadgetCount[active]);
	GT_RefreshWindow(pd->pd_Window,NULL);

	if (pd->pd_Refresh)
		pd->pd_Refresh(pd->pd_Window,active);
}


void
RemovePageGadgets(struct RastPort *rp, struct Gadget *gad, struct PageGData *pd)
{
	if (rp) {
		long x = gad->LeftEdge;
		long y = gad->TopEdge;

		EraseRect(rp, x, y, x + gad->Width, y + gad->Height);
	}

	if (pd && pd->pd_Gadgets)
		RemoveGList(pd->pd_Window, pd->pd_Gadgets, pd->pd_GadgetCount[pd->pd_Active]);
}


IPTR PUBLIC
DispatchPageGadget(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	struct PageGData *pd = INST_DATA(cl, o);
	IPTR retval = 0;
	long i;

	switch (msg->MethodID) {
		case OM_NEW:
			if ((retval = DoSuperMethodA(cl, o, msg)) != 0) {
				struct Gadget *gad;

				pd = INST_DATA(cl, retval);
				pd->pd_Active = GetTagData(PAGEGA_Active, 0L, ((struct opSet *)msg)->ops_AttrList);
				pd->pd_Pages = (struct Gadget **)GetTagData(PAGEGA_Pages, 0, ((struct opSet *)msg)->ops_AttrList);
				pd->pd_Refresh = (APTR)GetTagData(PAGEGA_RefreshFunc, 0, ((struct opSet *)msg)->ops_AttrList);
				
				// Count pages and gadgets in the pages
				for (i = 0; pd->pd_Pages[i]; i++);
#ifdef __amigaos4__
				pd->pd_GadgetCount = AllocVecTags(i * sizeof(ULONG), AVT_Type, MEMF_PRIVATE, AVT_ClearWithValue, 0, TAG_DONE );
#else
				pd->pd_GadgetCount = AllocVec(i * sizeof(ULONG), MEMF_CLEAR | MEMF_PUBLIC);
#endif
				pd->pd_Count = i;
				for (i = 0; i < pd->pd_Count; i++) {
					for (gad = pd->pd_Pages[i]; gad; gad = gad->NextGadget)
						pd->pd_GadgetCount[i]++;
				}

				D(bug("DispatchPageGadget: pd_Pages = 0x%lx, pd_Pages[0] = %lx, count = %ld, gagetCount[0] = %ld\n",
					pd->pd_Pages, pd->pd_Pages[0], pd->pd_Count, pd->pd_GadgetCount[0]));
			}
			break;
		case OM_DISPOSE:
			RemovePageGadgets(NULL, (struct Gadget *)o, pd);
			for (i = 0; i < pd->pd_Count; i++) {
				RemoveGadgets(&pd->pd_Pages[i],FALSE);  // alle nicht GadTools-Gadgets entfernen
				FreeGadgets(pd->pd_Pages[i]);			// GadTools-Gadgets freigeben
			}
			FreeVec(pd->pd_GadgetCount);
			retval = DoSuperMethodA(cl, o, msg);
			break;
		case OM_SET:
		case OM_UPDATE:
			DoSuperMethodA(cl, o, msg);
			{
				ULONG  tt[5] = {PAGEGA_Active,0,GA_ID,0,TAG_END};
				struct TagItem *tstate,*ti;
				struct RastPort *rp;

				tstate = ((struct opSet *)msg)->ops_AttrList;
				while ((ti = NextTagItem(&tstate)) != 0) {
					switch (ti->ti_Tag) {
						case PAGEGA_Active:
							i = ti->ti_Data;
							if (i > pd->pd_Count-1)
								i = pd->pd_Count-1;
							if (i < 0)
								i = 0;

							if (i != pd->pd_Active && (rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo))) {
								RemovePageGadgets(rp,(struct Gadget *)o,pd);
								pd->pd_Active = i;
								AddPageGadgets(rp,(struct Gadget *)o,pd);
								ReleaseGIRPort(rp);
							}
							tt[1] = pd->pd_Active;
							tt[3] = ((struct Gadget *)o)->GadgetID;
							DoSuperMethod(cl,o,OM_NOTIFY,tt,((struct opSet *)msg)->ops_GInfo,msg->MethodID == OM_UPDATE ? ((struct opUpdate *)msg)->opu_Flags : 0);
							break;
						case PAGEGA_BeginUpdatePage:
							//bug("page: begin update\n");
							i = ti->ti_Data;

							if (i == pd->pd_Active && (rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo))) {
								RemovePageGadgets(rp,(struct Gadget *)o,pd);
								ReleaseGIRPort(rp);
							}

							pd->pd_GadgetCount[i] = 0;
							break;
						case PAGEGA_EndUpdatePage:
							i = ti->ti_Data;

							pd->pd_GadgetCount[i] = 0;
							for(gad = pd->pd_Pages[i];gad;gad = gad->NextGadget)
								pd->pd_GadgetCount[i]++;

							if (i == pd->pd_Active && (rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo))) {
								AddPageGadgets(rp,(struct Gadget *)o,pd);
								ReleaseGIRPort(rp);
							}
							//bug("page: end update\n");
							break;
					}
				}
			}
			break;
		case OM_GET:
			if (((struct opGet *)msg)->opg_AttrID == PAGEGA_Active) {
				*((struct opGet *)msg)->opg_Storage = pd->pd_Active;
				retval = TRUE;
			} else
				retval = DoSuperMethodA(cl,o,msg);
			break;
		case GM_RENDER:
			if (!pd->pd_Gadgets) {
				struct gpRender *gpr = (struct gpRender *)msg;

				pd->pd_Window = gpr->gpr_GInfo->gi_Window;
				AddPageGadgets(gpr->gpr_RPort,(struct Gadget *)o,pd);
			} else if (pd->pd_Refresh)
				pd->pd_Refresh(pd->pd_Window,pd->pd_Active);
			break;
		case GM_HITTEST:
			retval = 0;
			break;
		default:
			retval = DoSuperMethodA(cl,o,msg);
	}
	return retval;
}


/********************************** Index-Gadget **********************************/
			
 
void
DrawIndexGadget(struct RastPort *rp, struct Gadget *gad, struct GadgetInfo *gi, struct IndexGData *id, BOOL all)
{
	long x, y, w, i, h, d, current;
	struct TextExtent te;
	UWORD *pens;
	UBYTE full;

	pens = id->id_dri->dri_Pens;
	h = rp->Font->tf_YSize+4;
	if (all) {  // draw the whole gadget
		APTR obj;

		if ((obj = NewObject(NULL, "frameiclass", IA_Width, gad->Width, IA_Height, gad->Height - h, IA_EdgesOnly, TRUE, TAG_END)) != 0) {
			DrawImage(rp, obj, gad->LeftEdge, gad->TopEdge + h);
			DisposeObject(obj);
		}
		for (w = 4, i = 0; i < id->id_Count; i++) {
			id->id_Pos[i] = w;
			id->id_Length[i] = strlen(id->id_Labels[i]);
			id->id_Width[i] = TextLength(rp,id->id_Labels[i],id->id_Length[i]) + 18;
			if (id->id_Width[i] < id->id_MinWidth)			 // label too small
				id->id_Width[i] = id->id_MinWidth;
			if (id->id_Width[i]+id->id_Pos[i] > gad->Width-4)  // labels wider than gadget
				id->id_Pos[i] = gad->Width - 4 - id->id_Width[i];

			w += id->id_Width[i] + id->id_Space;
		}
		if (w > gad->Width - 4) {
			for (i = id->id_Count - 2; i > 0; i--) {
				if (id->id_Pos[i]+id->id_Width[i] > id->id_Pos[i+1]+id->id_Width[i+1]-id->id_MinWidth)
					id->id_Pos[i] = id->id_Pos[i+1]+id->id_Width[i+1]-id->id_MinWidth-id->id_Width[i];

				if (id->id_Pos[i] > id->id_Pos[i+1]-id->id_MinWidth)
					id->id_Pos[i] = id->id_Pos[i+1]-id->id_MinWidth;
			}
		}
	}
	y = gad->TopEdge;
	current = id->id_Current = id->id_NewCurrent;
		// no special handling until now

	for (i = 0, x = 4 + gad->LeftEdge; i < id->id_Count; i++) {
		if (i != current)
			y++, h--;
		while (x > id->id_Pos[i] + id->id_Width[i])
			i++;

		SetAPen(rp, pens[SHINEPEN]);
		if (i < id->id_Count-1 && i < current) {
			// Width relative to the next
			w = id->id_Pos[i + 1] - id->id_Pos[i] - 1 - id->id_Space;
		} else {
			// Width relative to the current position
			w = id->id_Width[i] - (x - id->id_Pos[i] - gad->LeftEdge) - 1;
		}
		full = w + 1 == id->id_Width[i];
		if (full || i <= current) {
			// draw left edge
			if (i == current) {
				Move(rp, x - 1, y + h - 1);
				Draw(rp, x, y + h - 1);
			} else
				Move(rp, x, y + h - 1);
			Draw(rp, x, y + 1);
			Draw(rp, x + 1, y + 1);
			EraseRect(rp, x, y, x, y);
		}

		// draw top edge
		Move(rp, x + 1, y);
		Draw(rp, x + w - 2, y);
		if (i != current)
			EraseRect(rp, x, y - 1, x + w, y - 1);

		if (full || i >= current) {
			// draw right edge
			SetAPen(rp, pens[SHADOWPEN]);
			WritePixel(rp, x + w - 1, y + 1);
			Move(rp, x + w, y + 2);
			if (i == current) {
				Draw(rp, x + w, y + h - 2);
				WritePixel(rp, x + w + 1, y + h - 1);
			} else
				Draw(rp, x + w, y + h - 1);
			EraseRect(rp, x + w - 1, y, x + w, y);
			EraseRect(rp, x + w, y + 1, x + w, y + 1);
		}

		SetABPenDrMd(rp, pens[TEXTPEN], pens[BACKGROUNDPEN], JAM2);
		if (i <= id->id_Current || full) {
			// draw label
			d = TextFit(rp, id->id_Labels[i], id->id_Length[i], &te, NULL, 1, w-8, h);
			Move(rp, x + 9, y + 2 + rp->Font->tf_Baseline);
			Text(rp, id->id_Labels[i], d);
			SetAPen(rp, pens[BACKGROUNDPEN]);
			RectFill(rp,x+1,y+2,x+8,y+h-2);
			RectFill(rp,x+8+te.te_Width + 1,y+2,x+w-(full ? 1 : -1),y+h-2);
				// TODO: is te.te_Width smaller in Amithlon??
			// clear gaps
			RectFill(rp,x + 2, y + 1, x + w - (full ? 2 : 1), y + 1);
			RectFill(rp,x+1,y+h-(i == current ? 2 : 1),x+w-(full ? 1 : 0),y+h-1);
		} else if (i > current) {
			d = TextFit(rp,id->id_Labels[i]+id->id_Length[i]-1,id->id_Length[i],&te,NULL,-1,w-7,h);
			Move(rp,x+w-te.te_Width-6,y+2+rp->Font->tf_Baseline);
			Text(rp,id->id_Labels[i]+id->id_Length[i]-d,d);
			SetAPen(rp,pens[BACKGROUNDPEN]);
			RectFill(rp,x,y+2,x+w-te.te_Width-7,y+h-2);
			RectFill(rp,x+w-7,y+2,x+w-1,y+h-2);
			// clear gaps
			RectFill(rp,x,y+1,x+w-2,y+1);
			RectFill(rp,x,y+h-1,x+w-1,y+h-1);
		}

		if (i == current) {
			// draw or erase bottom edge
			RectFill(rp, x + 1, y + h - 1, x + w, y + h - 1);
			RectFill(rp, x, y + h, x + w + 1, y + h);
		} else {
			SetAPen(rp, pens[SHINEPEN]);
			Move(rp, x, y + h);
			Draw(rp, x + w + id->id_Space, y + h);
		}
		if (id->id_Space)
			EraseRect(rp,x+w+1,y-1,x+w+id->id_Space,y+h-(i == current ? 2 : 1));
		x += w+1+id->id_Space;
		if (i != current)
			y--, h++;
	}
}


void
NotifyIndexSuperClass(APTR cl, APTR o, Msg msg, struct GadgetInfo *gi, struct IndexGData *id)
{
	ULONG tt[5] = {IGA_Active,0,GA_ID,0,TAG_END};

	if (id)
		tt[1] = id->id_Active;
	tt[3] = ((struct Gadget *)o)->GadgetID;
	DoSuperMethod(cl,o,OM_NOTIFY,tt,gi,msg->MethodID == OM_UPDATE ? ((struct opUpdate *)msg)->opu_Flags : 0);
}


IPTR PUBLIC
DispatchIndexGadget(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	IPTR retval = 0;
	struct IndexGData *id;
	long i;

	id = INST_DATA(cl,o);

	switch(msg->MethodID) {
		case OM_NEW:
			if (!GetTagData(GA_DrawInfo, 0, ((struct opSet *)msg)->ops_AttrList))
				break;
			if ((retval = DoSuperMethodA(cl, o, msg)) != 0) {
				id = INST_DATA(cl,retval);
				id->id_Active = id->id_Current = id->id_NewCurrent = GetTagData(IGA_Active,0L,((struct opSet *)msg)->ops_AttrList);
				id->id_Labels = (STRPTR *)GetTagData(IGA_Labels,0L,((struct opSet *)msg)->ops_AttrList);
				id->id_dri = (struct DrawInfo *)GetTagData(GA_DrawInfo,0L,((struct opSet *)msg)->ops_AttrList);

				for (i = 0;*(id->id_Labels+i);i++);  // Zähle Labels
				id->id_Count = i;
#ifdef __amigaos4__
				id->id_Pos = AllocVecTags(i*sizeof(LONG)*3, AVT_ClearWithValue, 0, TAG_DONE);
#else
				id->id_Pos = AllocVec(i*sizeof(LONG)*3,MEMF_CLEAR);
#endif
				id->id_Width = id->id_Pos+i;
				id->id_Length = id->id_Width+i;
				id->id_MinWidth = 16;
				id->id_Space = 3;
				id->id_TextSpace = 6;
			}
			break;
		case OM_DISPOSE:
			FreeVec(id->id_Pos);
			retval = DoSuperMethodA(cl,o,msg);
			break;
		case OM_SET:
		case OM_UPDATE:
		case OM_NOTIFY:
			DoSuperMethodA(cl,o,msg);
			{
				struct TagItem *tstate,*ti;

				tstate = ((struct opSet *)msg)->ops_AttrList;
				while ((ti = NextTagItem(&tstate)) != 0) {
					switch (ti->ti_Tag) {
						case IGA_Active:
							id->id_Active = id->id_NewCurrent = ti->ti_Data;
							if (msg->MethodID != OM_NOTIFY)
								NotifyIndexSuperClass(cl,o,msg,((struct opSet *)msg)->ops_GInfo,id);
							if (id->id_NewCurrent != id->id_Current) {
								struct RastPort *rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo);

								DrawIndexGadget(rp,(struct Gadget *)o,((struct opSet *)msg)->ops_GInfo,id,FALSE);
								ReleaseGIRPort(rp);
							}
							break;
					}
				}
			}
			break;
		case OM_GET:
			if (((struct opGet *)msg)->opg_AttrID == IGA_Active) {
				*((struct opGet *)msg)->opg_Storage = id->id_Active;
				retval = TRUE;
			}
			else
				retval = DoSuperMethodA(cl,o,msg);
			break;
		case GM_RENDER:
			{
				struct gpRender *gpr = (struct gpRender *)msg;

				DrawIndexGadget(gpr->gpr_RPort,(struct Gadget *)o,gpr->gpr_GInfo,id,TRUE);
			}
			break;
		case GM_HITTEST:
			{
				struct gpHitTest *gpht = (struct gpHitTest *)msg;

				if (gpht->gpht_Mouse.X > 4 && gpht->gpht_Mouse.X < ((struct Gadget *)o)->Width-4 && gpht->gpht_Mouse.Y < id->id_dri->dri_Font->tf_YSize+4)
					retval = GMR_GADGETHIT;
			}
			break;
		case GM_GOACTIVE:
		case GM_HANDLEINPUT:
			{
				struct gpInput *gpi = (struct gpInput *)msg;
				struct InputEvent *ie = gpi->gpi_IEvent;

				if (ie && ie->ie_Class == IECLASS_RAWMOUSE) {
					if (ie->ie_Code == IECODE_RBUTTON) {
						if (id->id_Current != id->id_Active) {
							struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);

							id->id_NewCurrent = id->id_Active;
							DrawIndexGadget(rp,(struct Gadget *)o,gpi->gpi_GInfo,id,FALSE);
							ReleaseGIRPort(rp);
						}
						*gpi->gpi_Termination = 0xffff;
						retval = GMR_REUSE | GMR_VERIFY;
					} else if (ie->ie_Code == (IECODE_LBUTTON | IECODE_UP_PREFIX)) {
						id->id_Active = id->id_Current;
						*gpi->gpi_Termination = id->id_Active & 0xffff;
						NotifyIndexSuperClass(cl,o,msg,gpi->gpi_GInfo,id);
						retval = GMR_REUSE | GMR_VERIFY;
					} else {
						long x = gpi->gpi_Mouse.X,xp;

						id->id_NewCurrent = id->id_Current;
						for (i = 0;i < id->id_Current && id->id_Current == id->id_NewCurrent;i++) {
							if (x >= id->id_Pos[i] && x < id->id_Pos[i+1])
								id->id_NewCurrent = i;
						}
						for (i = id->id_Current,xp = id->id_Pos[i];i < id->id_Count && id->id_Current == id->id_NewCurrent;i++) {
							if (x >= xp && x <= id->id_Pos[i]+id->id_Width[i])
								id->id_NewCurrent = i;
							xp = id->id_Pos[i]+id->id_Width[i]+id->id_Space;
						}

						if (id->id_NewCurrent != id->id_Current) {
							struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);

							DrawIndexGadget(rp,(struct Gadget *)o,gpi->gpi_GInfo,id,FALSE);
							ReleaseGIRPort(rp);
						}
						retval = GMR_MEACTIVE;
					}
				}
			}
			break;
		default:
			retval = DoSuperMethodA(cl,o,msg);
	}
	return retval;
}

/********************************** Frames-Gadget **********************************/


/* Frame-Definition:
**
** Bits: 15-12  11-8	7-6	5-4	3-2	 1-0
**		----	color  top  bottom  right  left
**
** On the edges: 00 - keine, 01 - 1pt, 10 - 2pt, 11 - 3pt
**
** Set bits for "color" mean white (top, bottom, right, left)
*/

const UWORD kPredefinedFrames[] = {
	0x0000, 0x0010, 0x0040, 0x0001, 0x0004, 0x0015, 0x0055,
	0x0050, 0x0020, 0x0080, 0x0002, 0x0008, 0x002a, 0x00aa,
	0x0005, 0x0014, 0x0041, 0x0044, 0x0011, 0x0021, 0x0081,
	0x0018, 0x0069, 0x0096, 0x0955, 0x0655, 0x09aa, 0x06aa
};
const UWORD stdframe_count = 28;
const UWORD stdframe_cols = 7,stdframe_rows = 4;
UWORD stdframe_width = 25;
UWORD stdframe_height = 25;
ULONG stdframe_active = ~0L,stdframe_new = ~0L;


void
DrawFatRect(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2)
{
	RectFill(rp,x1,y1,x1+1,y2);
	RectFill(rp,x1+2,y1,x2,y1+1);
	RectFill(rp,x2-1,y1+2,x2,y2);
	RectFill(rp,x1+2,y2-1,x2-2,y2);
}


void
EraseFatRect(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2)
{
	EraseRect(rp,x1,y1,x1+1,y2);
	EraseRect(rp,x1+2,y1,x2,y1+1);
	EraseRect(rp,x2-1,y1+2,x2,y2);
	EraseRect(rp,x1+2,y2-1,x2-2,y2);
}


void
DrawFGFrame(struct RastPort *rp, long type, long x, long y)
{
	long a;

	rp->linpatcnt = 15;  rp->Flags |= FRST_DOT;
	rp->LinePtrn = 0xaaaa;
	SetAPen(rp,7);	/* grau */
	Move(rp,x+2,y+5);  Draw(rp,x+stdframe_width-3,y+5);
	Move(rp,x+2,y+stdframe_height-6);  Draw(rp,x+stdframe_width-3,y+stdframe_height-6);
	Move(rp,x+5,y+2);  Draw(rp,x+5,y+stdframe_height-3);
	Move(rp,x+stdframe_width-6,y+2);  Draw(rp,x+stdframe_width-6,y+stdframe_height-3);
	rp->linpatcnt = 0;  rp->LinePtrn = 0xffff;

	if (type & 0x000c) {
		/* right */
		SetAPen(rp,type & 0x0200 ? 2 : 1);  /* white/black */
		/*for(a = 0;a < ((type & 0xc) >> 2);a++)
			Move(rp,x+stdframe_width-6-a,y+5+a), Draw(rp,x+stdframe_width-6-a,y+stdframe_height-6-a);*/
		RectFill(rp,x+stdframe_width-5-((type & 0xc) >> 2),y+5,x+stdframe_width-6,y+stdframe_height-6);
	}
	if (type & 0x0030) {
		/* bottom */
		SetAPen(rp,type & 0x0400 ? 2 : 1);  /* white/black */
		RectFill(rp,x+5,y+stdframe_height-5-((type & 0x30) >> 4),x+stdframe_width-6,y+stdframe_height-6);
	}
	if (type & 0x0003) {
		/* left */
		SetAPen(rp,type & 0x0100 ? 2 : 1);  /* white/black */
		for(a = 0;a < (type & 0x3);a++)
			Move(rp,x+5+a,y+5+a), Draw(rp,x+5+a,y+stdframe_height-6-a);
		/*RectFill(rp,x+5,y+5,x+4+((type & 0x3) >> 0),y+stdframe_height-6);*/
	}
	if (type & 0x00c0) {
		/* top */
		SetAPen(rp,type & 0x0800 ? 2 : 1);  /* white/black */
		for(a = 0;a < ((type & 0xc0) >> 6);a++)
			Move(rp,x+5+a,y+5+a), Draw(rp,x+stdframe_width-6-a,y+5+a);
		/*RectFill(rp,x+5,y+5,x+stdframe_width-6,y+4+((type & 0xc0) >> 6));*/
	}
}


void
DrawFramesGadget(struct RastPort *rp, struct Gadget *gad, struct GadgetInfo *gi, BOOL all)
{
	struct Image *img;
	long x,y,a,b;

	if (all) {
		// draw the whole gadget
		if ((img = (struct Image *)NewObject(NULL,"frameiclass",
				IA_Width,  stdframe_width,
				IA_Height, stdframe_height,
				TAG_END)) != NULL) {
			for (x = 0, y = 0; y < stdframe_rows; x++) {
				if (x >= stdframe_cols)
					x = 0,y++;
				if (y < stdframe_rows) {
					DrawImage(rp,img,a = gad->LeftEdge+x*(stdframe_width+3),b = gad->TopEdge+y*(stdframe_height+3));
					DrawFGFrame(rp,stdframe_count > x+y*stdframe_cols ? kPredefinedFrames[x+y*stdframe_cols] : 0L,a,b);
				}
			}
			DisposeObject((Object *)img);
		}
	}
	if (stdframe_new != stdframe_active) {
		if (stdframe_active != ~0L) {
			x = gad->LeftEdge + (stdframe_width + 3) * (stdframe_active % stdframe_cols) - 2;
			y = gad->TopEdge + (stdframe_height + 3) * (stdframe_active / stdframe_cols) - 2;
			EraseFatRect(rp, x, y, x + stdframe_width + 3, y + stdframe_height + 3);
		}
		SetAPen(rp,3);  stdframe_active = stdframe_new;
		if (stdframe_active != ~0L)	{
			x = gad->LeftEdge + (stdframe_width + 3) * (stdframe_active % stdframe_cols) - 2;
			y = gad->TopEdge + (stdframe_height + 3) * (stdframe_active / stdframe_cols) - 2;
			DrawFatRect(rp ,x ,y ,x + stdframe_width + 3, y + stdframe_height + 3);
		}
	}
}


IPTR PUBLIC
DispatchFramesGadget(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	IPTR retval = 0;
	long x, y;

	switch (msg->MethodID) {
		case OM_NEW:
			if ((retval = DoSuperMethodA(cl,o,msg)) != 0)
				SetAttrs((Object *)retval,GA_RelVerify,TRUE,TAG_END);
			break;
		case GM_DOMAIN:
			if (o == (APTR)cl) {
				struct gpDomain *gpd = (struct gpDomain *)msg;

				gpd->gpd_Domain.Width = stdframe_cols*(stdframe_width+3)-3;
				gpd->gpd_Domain.Height = stdframe_rows*(stdframe_height+3)-3;
				retval = TRUE;
			} else
				retval = DoSuperMethodA(cl,o,msg);
			break;
		case GM_RENDER:
			{
				struct gpRender *gpr = (struct gpRender *)msg;

				DrawFramesGadget(gpr->gpr_RPort,(struct Gadget *)o,gpr->gpr_GInfo,TRUE);
			}
			break;
		case GM_HITTEST:
			retval = GMR_GADGETHIT;
			break;
		case GM_GOINACTIVE:
			{
				struct gpInput *gpi = (struct gpInput *)msg;
				struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);

				stdframe_new = ~0L;
				DrawFramesGadget(rp,(struct Gadget *)o,gpi->gpi_GInfo,FALSE);
				ReleaseGIRPort(rp);
			}
			retval = 0;
			break;
		case GM_GOACTIVE:
		case GM_HANDLEINPUT:
			{
				struct gpInput *gpi = (struct gpInput *)msg;
				struct InputEvent *ie = gpi->gpi_IEvent;

				if (ie && ie->ie_Class == IECLASS_RAWMOUSE) {
					if (ie->ie_Code == IECODE_RBUTTON) {
						*gpi->gpi_Termination = 0xffff;
						retval = GMR_REUSE | GMR_VERIFY;
					} else if (ie->ie_Code == (IECODE_LBUTTON | IECODE_UP_PREFIX)) {
						*gpi->gpi_Termination = stdframe_active & 0xffff;
						retval = GMR_REUSE | GMR_VERIFY;
					} else {
						struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);

						x = gpi->gpi_Mouse.X;  y = gpi->gpi_Mouse.Y;
						if (x < 0 || x > ((struct Gadget *)o)->Width || y < 0 || y > ((struct Gadget *)o)->Height)
							stdframe_new = ~0L;
						else if ((x % (stdframe_width+3)) <= stdframe_width && (y % (stdframe_height+3)) <= stdframe_height)
							stdframe_new = stdframe_cols*(y / (stdframe_height+3))+x / (stdframe_width+3);
						DrawFramesGadget(rp,(struct Gadget *)o,gpi->gpi_GInfo,FALSE);
						ReleaseGIRPort(rp);
						retval = GMR_MEACTIVE;
					}
				}
			}
			break;
		default:
			retval = DoSuperMethodA(cl,o,msg);
	}
	return retval;
}


/********************************** PopUp-Image **********************************/


IPTR PUBLIC
DispatchPopUpImage(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	if (msg->MethodID == IM_DRAW || msg->MethodID == IM_DRAWFRAME) {
		/*struct impDraw *imp = (struct impDraw *)msg;
		long	x,y;

		{
			struct Image *im = popUpImg;

			if (imp->imp_State == IDS_SELECTED)
				im = popDownImg;

			DrawImage(imp->imp_RPort,im,x = imp->imp_Offset.X,y = imp->imp_Offset.Y);
		}
		if (imp->imp_State == IDS_DISABLED || imp->imp_State == IDS_SELECTEDDISABLED)
			GhostRect(imp->imp_RPort,1,x,y,x+IM(o)->Width-1,y+IM(o)->Height-1);
		*/
		return 0;
	} else if (msg->MethodID == OM_GET) {
		if (((struct opGet *)msg)->opg_AttrID == IA_SupportsDisable) {
			*((struct opGet *)msg)->opg_Storage = TRUE;
			return TRUE;
		}
	}
	return DoSuperMethodA(cl, o, msg);
}


/********************************** Bitmap-Image **********************************/


IPTR PUBLIC
DispatchBitmapImage(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	struct BitmapIData *bd;

	bd = INST_DATA(cl,o);

	switch (msg->MethodID) {
		case OM_NEW:
		{
			Object *this;

			if ((this = (Object *)DoSuperMethodA(cl, o, msg))) {
				bd = INST_DATA(cl,this);
				bd->bd_Bitmap = (struct BitMap *)GetTagData(BIA_Bitmap, 0, ((struct opSet *)msg)->ops_AttrList);
				bd->bd_SelectedBitmap = (struct BitMap *)GetTagData(BIA_SelectedBitmap, 0, ((struct opSet *)msg)->ops_AttrList);
			}
			return (ULONG)this;
		}

		case OM_GET:
			if (((struct opGet *)msg)->opg_AttrID == IA_SupportsDisable) {
				*((struct opGet *)msg)->opg_Storage = TRUE;
				return TRUE;
			}
			return DoSuperMethodA(cl, o, msg);

		case IM_DRAW:
		case IM_DRAWFRAME:
		{
			struct impDraw *imp = (struct impDraw *)msg;
			struct BitMap *bm = bd->bd_Bitmap;
			long x,y;

			if (imp->imp_State == IDS_SELECTED && bd->bd_SelectedBitmap)
				bm = bd->bd_SelectedBitmap;

			BltBitMapRastPort(bm,0,0,imp->imp_RPort,x = imp->imp_Offset.X,y = imp->imp_Offset.Y,IM(o)->Width,IM(o)->Height,0xc0);
			//DrawBevelBox(imp->imp_RPort,x,y,IM(o)->Width,IM(o)->Height,GT_VisualInfo,vi,GTBB_FrameType,BBFT_BUTTON,TAG_END);

			if (imp->imp_State == IDS_DISABLED || imp->imp_State == IDS_SELECTEDDISABLED)
				GhostRect(imp->imp_RPort,1,x,y,x+IM(o)->Width-1,y+IM(o)->Height-1);

			return 0;
		}

		default:
			return DoSuperMethodA(cl, o, msg);
	}
}


/********************************** Picture-Image **********************************/


void
SetBitMapHeader(struct PictureIData *pd, struct Image *im, Object *pic)
{
	struct BitMapHeader *bmh;

	if (GetDTAttrs(pic, PDTA_BitMapHeader, &bmh, TAG_END)) {
		bmh->bmh_Width = im->Width;
		bmh->bmh_Height = im->Height;
		bmh->bmh_Depth = pd->pd_Depth;
	}
}


ULONG
GetBestPictureModeID(ULONG width, ULONG height, ULONG depth)
{
	ULONG mode_id = 0;

	mode_id = BestModeID(BIDTAG_NominalWidth,	width,
						 BIDTAG_NominalHeight,	height,
						 BIDTAG_DesiredWidth,	width,
						 BIDTAG_DesiredHeight,	height,
						 BIDTAG_Depth,		 	depth,
						 TAG_END);
	if (mode_id == INVALID_ID) {
		/* Uses OverScan values for checking. */
		/* Assumes an ECS-System.			 */

		if ((width > 724) && (depth < 3))
			mode_id = SUPER_KEY;
		else if((width > 362) && (depth < 5))
			mode_id = HIRES_KEY;
		else
			mode_id = LORES_KEY;

		if (!ModeNotAvailable(mode_id | PAL_MONITOR_ID)) {
			/* for PAL  Systems */
			if (height > 283)
				mode_id |= LACE;

			 mode_id |= PAL_MONITOR_ID;
		} else {
			if (!ModeNotAvailable(mode_id | NTSC_MONITOR_ID)) {
				/* for NTSC Systems */
				if (height > 241)
					mode_id |= LACE;

				mode_id |= NTSC_MONITOR_ID;
			}
		}
	}
	return mode_id;
}


void
SetPictureScreen(struct PictureIData *pd, struct Image *im, struct Screen *scr)
{
	struct BitMap *bm = NULL;
	Object *pic;

	if (scr != pd->pd_Screen || !pd->pd_BitMap) {
		DisposeObject(pd->pd_Remapped);	  /* free old remapped */
		pd->pd_Remapped = NULL;
		pd->pd_BitMap = pd->pd_SourceBitMap;
		pd->pd_Screen = scr;

		if (!scr)
			return;


		if (CyberGfxBase && GetCyberMapAttr(bm = pd->pd_BitMap, CYBRMATTR_ISCYBERGFX) == -1 && pd->pd_Depth >= 24) {
			if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) >= -15)
				return;

			TRACE(("loading true color image...\n"));

			if ((pic = NewObject(NULL, "picture.datatype",
					DTA_ObjName,	pd->pd_Name ? pd->pd_Name : (STRPTR)"image", PDTA_Screen,  scr,
					PDTA_Remap,		TRUE,
					DTA_Name,		"picture",
					TAG_END)) != 0)
			{
				ULONG tags[] = {LBMI_WIDTH, 0, LBMI_HEIGHT, 0, LBMI_PIXFMT, 0, LBMI_BYTESPERROW, 0, LBMI_BASEADDRESS, 0, TAG_END};
				ULONG mod, format, w, h, data;
				APTR handle;

				SetBitMapHeader(pd, im, pic);

				SetDTAttrs(pic, NULL, NULL, /*DTA_ObjName,	  pd->pd_Name ? pd->pd_Name : (STRPTR)"image",*/
#ifndef __amigaos4__
											PDTA_SourceMode,  PMODE_V43,
#endif
											DTA_NominalHoriz, im->Width,
											DTA_NominalVert,  im->Height,
											PDTA_ModeID,	  GetBestPictureModeID(im->Width, im->Height, 8),
											TAG_END);

				tags[1] = (ULONG)&w;  tags[3] = (ULONG)&h;
				tags[5] = (ULONG)&format;  tags[7] = (ULONG)&mod;  tags[9] = (ULONG)&data;

				if ((handle = LockBitMapTagList(bm, (struct TagItem *)tags)) != 0) {
					if (format == PIXFMT_RGB24)
						format = PBPAFMT_RGB;
					else if (format == PIXFMT_RGBA32)
						format = PBPAFMT_RGBA;
					else if (format == PIXFMT_ARGB32)
						format = PBPAFMT_ARGB;

					DoMethod(pic, PDTM_WRITEPIXELARRAY, data, format, mod, 0, 0, im->Width, im->Height);
					UnLockBitMap(handle);
				}
			}
			bm = NULL;
		} else {
			/* clut/standard bitmap */
			if ((bm = AllocBitMap(im->Width, im->Height, pd->pd_Depth, BMF_MINPLANES, pd->pd_BitMap)) != 0)
				BltBitMap(pd->pd_BitMap, 0, 0, bm, 0, 0, im->Width, im->Height, 0xc0, 0xff, NULL);

			TRACE(("setting CLUT8 picture data...\n"));

			pic = NewObject(NULL, "picture.datatype", PDTA_Screen,			scr,
													  PDTA_Remap,			TRUE,
													  PDTA_FreeSourceBitMap, TRUE,
													  PDTA_NumColors,		pd->pd_NumColors,
													  PDTA_BitMap,			bm,
#ifdef LOW_DITHER_QUALITY
													/*PDTA_DitherQuality*/DTA_Dummy+222,	0,
#endif
													  DTA_Name,			  "picture",
													  TAG_END);
			if (pic != NULL) {
				struct ColorRegister *cmap = NULL;
				LONG *cregs = NULL;

				SetBitMapHeader(pd, im, pic);

				if (GetDTAttrs(pic, PDTA_ColorRegisters, &cmap,
									PDTA_CRegs, &cregs,
									TAG_END) > 1) {
					if (cmap != NULL)
						CopyMem(pd->pd_ColorMap, cmap, sizeof(struct ColorRegister) * pd->pd_NumColors);
					if (cregs != NULL)
						CopyMem(pd->pd_ColorRegs, cregs, sizeof(LONG) * 3 * pd->pd_NumColors);
				}
			}
		}

		if (pic != NULL) {
			if (DoMethod(pic, DTM_PROCLAYOUT, NULL, 1)) {
				if (GetDTAttrs(pic, PDTA_DestBitMap, &bm, TAG_END)) {
					pd->pd_BitMap = bm;
					pd->pd_Remapped = pic;
					return;
				}
			}
			bm = NULL;
			DisposeObject(pic);
		}
		if (bm)
			FreeBitMap(bm);
	}
}


ULONG
CopyFromPictureImage(struct PictureIData *pd, struct Image *fim, LONG *cregs, struct Image *im)
{
	if (fim && cregs) {
		struct BitMap bm;
		UBYTE *planedata = (APTR)fim->ImageData;
		long width = fim->Width;
		long height = fim->Height;
		long depth = fim->Depth;
		long planesize, cols, i;

		planesize = (((width + 15) >> 4) << 1)*height;
		InitBitMap(&bm, depth, width, height);

		for (i = 0;i < depth;i++)
			bm.Planes[i] = planedata + i*planesize;

		cols = 1 << depth;

#ifdef __amigaos4__
		if ((pd->pd_ColorMap = AllocVecTags(sizeof(struct ColorRegister) * cols, AVT_Type, MEMF_PRIVATE, TAG_DONE )) != 0) {
#else
		if ((pd->pd_ColorMap = AllocVec(sizeof(struct ColorRegister) * cols, MEMF_PUBLIC)) != 0) {
#endif
			for (i = 0;i < cols;i++) {
				pd->pd_ColorMap[i].red =	cregs[i * 3 + 0];
				pd->pd_ColorMap[i].green = cregs[i * 3 + 1];
				pd->pd_ColorMap[i].blue =  cregs[i * 3 + 2];
			}

#ifdef __amigaos4__
			if ((pd->pd_ColorRegs = AllocVecTags(sizeof(LONG) * 3 * cols, AVT_Type, MEMF_PRIVATE, TAG_DONE )) != 0) {
#else
			if ((pd->pd_ColorRegs = AllocVec(sizeof(LONG) * 3 * cols, MEMF_PUBLIC)) != 0) {
#endif
				CopyMem(cregs, pd->pd_ColorRegs, sizeof(LONG) * 3 * cols);
				pd->pd_NumColors = cols;
				pd->pd_Depth = depth;

				im->Width = width;
				im->Height = height;

				if ((pd->pd_SourceBitMap = AllocBitMap(width, height, depth, 0, NULL))) {
					BltBitMap(&bm, 0, 0, pd->pd_SourceBitMap, 0, 0, width, height, 0xc0, 0xff, NULL);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


ULONG
CopyFromPictureObject(struct PictureIData *pd, Object *obj, struct Image *im)
{
	struct ColorRegister *cmap;
	struct BitMapHeader *bmh;
	LONG *cregs, cols;
	struct BitMap *bm;

	if (GetDTAttrs(obj, PDTA_ColorRegisters,	&cmap,
						PDTA_BitMap,			&bm,
						PDTA_CRegs,				&cregs,
						PDTA_NumColors,			&cols,
						PDTA_BitMapHeader,		&bmh,
						TAG_DONE) > 4) {
		if (bm && cregs && cmap && bmh) {
#ifdef __amigaos4__
			if ((pd->pd_ColorMap = AllocVecTags(sizeof(struct ColorRegister) * cols, AVT_Type, MEMF_PRIVATE, TAG_DONE )) != 0) {
//			if ((pd->pd_ColorMap = AllocVecTags(sizeof(struct ColorRegister) * cols, AVT_Type, MEMF_SHARED, TAG_DONE )) != 0) {
#else
			if ((pd->pd_ColorMap = AllocVec(sizeof(struct ColorRegister) * cols, MEMF_PUBLIC)) != 0) {
#endif
				CopyMem(cmap, pd->pd_ColorMap, sizeof(struct ColorRegister) * cols);
#ifdef __amigaos4__
				if ((pd->pd_ColorRegs = AllocVecTags(sizeof(LONG) * 3 * cols, AVT_Type, MEMF_PRIVATE, TAG_DONE )) != 0) {
//				if ((pd->pd_ColorRegs = AllocVecTags(sizeof(LONG) * 3 * cols, AVT_Type, MEMF_SHARED, TAG_DONE )) != 0) {
#else
				if ((pd->pd_ColorRegs = AllocVec(sizeof(LONG) * 3 * cols, MEMF_PUBLIC)) != 0) {
#endif
					CopyMem(cregs, pd->pd_ColorRegs, sizeof(LONG) * 3 * cols);

					pd->pd_NumColors = cols;
					pd->pd_Depth = bmh->bmh_Depth;

					im->Width = bmh->bmh_Width;
					im->Height = bmh->bmh_Height;

					if ((pd->pd_SourceBitMap = AllocBitMap(bmh->bmh_Width, bmh->bmh_Height, bmh->bmh_Depth, BMF_MINPLANES, bm)) != 0) {
						BltBitMap(bm, 0, 0, pd->pd_SourceBitMap, 0, 0, im->Width, im->Height, 0xc0, 0xff, NULL);
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}


BOOL
LoadPictureObject(struct PictureIData *pd, struct Image *im)
{
	ULONG  tags[] = {
		DTA_GroupID,		GID_PICTURE,
#ifndef __amigaos4__						//Under AOS4 this paramaeter produces problems with most picturetype (Grim-Reaper)
		PDTA_SourceMode,	PMODE_V43,
#endif
		PDTA_Remap,			FALSE,
		TAG_END
	};
	Object *obj;

	if ((obj = NewDTObjectA(pd->pd_Name, (struct TagItem *)tags)) != 0) {
		if (DoMethod(obj, DTM_PROCLAYOUT, NULL, 1) && CopyFromPictureObject(pd, obj, im)) {
			DisposeDTObject(obj);
			SetPictureScreen(pd, im, pd->pd_Screen);
			return TRUE;
		}
		DisposeDTObject(obj);
	}
	return FALSE;
}


IPTR PUBLIC
DispatchPictureImage(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	struct PictureIData *pd;
	IPTR   retval = 0;

	pd = INST_DATA(cl, o);

	switch (msg->MethodID) {
		case OM_NEW:
			if ((retval = DoSuperMethodA(cl, o, msg)) != 0) {
				pd = INST_DATA(cl, retval);
				pd->pd_Screen = (struct Screen *)GetTagData(PDTA_Screen, 0, ((struct opSet *)msg)->ops_AttrList);

				IM(retval)->Height = 5;
				IM(retval)->Width = 5;

				if ((pd->pd_Name = AllocString((STRPTR)GetTagData(DTA_Name, 0, ((struct opSet *)msg)->ops_AttrList))) != 0) {
					if (!GetTagData(PIA_DelayLoad, FALSE, ((struct opSet *)msg)->ops_AttrList)) {
						if (LoadPictureObject(pd, (struct Image *)retval))
							break;
					} else
						break;
				} else {
					struct Image *im;
					LONG *cregs;

					if ((im = (APTR)GetTagData(PIA_FromImage, 0, ((struct opSet *)msg)->ops_AttrList)) != 0) {
						if ((cregs = (APTR)GetTagData(PIA_WithColors, 0, ((struct opSet *)msg)->ops_AttrList)) != 0) {
							if (CopyFromPictureImage(pd, im, cregs, (struct Image *)retval)) {
								SetPictureScreen(pd, (struct Image *)retval, pd->pd_Screen);
								break;
							}
						}
					}
				}
				DoSuperMethod(cl, (Object *)retval, OM_DISPOSE);
				retval = 0;
			}
			break;
		case OM_DISPOSE:
			DisposeObject(pd->pd_Remapped);
			if (pd->pd_ColorMap)
				FreeVec(pd->pd_ColorMap);
			if (pd->pd_ColorRegs)
				FreeVec(pd->pd_ColorRegs);
			if (pd->pd_SourceBitMap)
				FreeBitMap(pd->pd_SourceBitMap);
			FreeString(pd->pd_Name);

			retval = DoSuperMethodA(cl, o, msg);
			break;
		case OM_SET:
		{
			struct TagItem *ti;

			retval = DoSuperMethodA(cl, o, msg);
			if ((ti = FindTagItem(PDTA_Screen, ((struct opSet *)msg)->ops_AttrList)) != 0) {
				if (pd->pd_SourceBitMap)
					SetPictureScreen(pd, (struct Image *)o, (struct Screen *)ti->ti_Data);
				else if (pd->pd_Name) {
					pd->pd_Screen = (struct Screen *)ti->ti_Data;
#ifdef __amigaos4__								//Avoid Guru/Grim when an unknown or to big data is loaded
					if(!LoadPictureObject(pd, (struct Image *)o))
					{
						FreeString(pd->pd_Name);
					    pd->pd_Name = AllocString("icons/prefs_sys.icon");
					    LoadPictureObject(pd, (struct Image *)o);
					}
#else
					LoadPictureObject(pd, (struct Image *)o);
#endif
				}
			}
			break;
		}
		case OM_GET:
			if (((struct opGet *)msg)->opg_AttrID == PIA_BitMap) {
				*((struct opGet *)msg)->opg_Storage = (ULONG)pd->pd_BitMap;
				retval = TRUE;
			} else
				retval = DoSuperMethodA(cl, o, msg);
			break;
		case PIM_LOAD:
			if (pd->pd_Name && !pd->pd_SourceBitMap)
				LoadPictureObject(pd, (struct Image *)o);
			break;
		case IM_DRAW:
		case IM_DRAWFRAME:
			{
				struct impDraw *imp = (struct impDraw *)msg;
				long x,y;

				x = IM(o)->LeftEdge + imp->imp_Offset.X;
				y = IM(o)->TopEdge + imp->imp_Offset.Y;

				if (pd->pd_BitMap)
					BltBitMapRastPort(pd->pd_BitMap, 0, 0, imp->imp_RPort, x, y, IM(o)->Width, IM(o)->Height, 0xc0);
			}
			break;
		default:
			retval = DoSuperMethodA(cl, o, msg);
	}
	return retval;
}


/********************************** Button-Class **********************************/


IPTR PUBLIC
DispatchButton(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	IPTR  rc;

	switch(msg->MethodID) {
		case GM_GOACTIVE:
		case GM_HANDLEINPUT:
		{
			struct gpInput *gpi = (struct gpInput *)msg;
			struct InputEvent *ie = gpi->gpi_IEvent;

			rc = DoSuperMethodA(cl,o,msg);
			if (ie->ie_Class == IECLASS_RAWMOUSE && ie->ie_Code == SELECTUP && !(gpi->gpi_Mouse.X >= 0 && gpi->gpi_Mouse.Y >= 0 && gpi->gpi_Mouse.X < ((struct Gadget *)o)->Width && gpi->gpi_Mouse.Y < ((struct Gadget *)o)->Height))
				rc = (rc & ~GMR_NOREUSE) | GMR_REUSE;
			break;
		}
		default:
			return DoSuperMethodA(cl,o,msg);
	}
	return rc;
}


/********************************** ColorButton-Class **********************************/


void
DrawColorButton(struct RastPort *rp, struct Gadget *gad, struct ColorGData *cd, int mode)
{
	int x = gad->LeftEdge, y = gad->TopEdge, w = gad->Width, h = gad->Height;

	if (mode < 3) {
		if (mode == 1) {
			/* erase free frame */

#if 1 /*def P96_WORKAROUND*/
			// Picasso96 doesn't respect the JAM2 mode when drawing
			// lines with patterns - so we have to erase the dither
			// frame first
			EraseRect(rp, x + 2, y + 1, x + 3, y + h - 2);			// left
			EraseRect(rp, x + w - 4, y + 1, x + w - 3, y + h - 2);	// right
			EraseRect(rp, x + 3, y + 1, x + w - 4, y + 2);			// top
			EraseRect(rp, x + 3, y + h - 3, x + w - 4, y + h - 2);	// bottom
#else
			EraseRect(rp, x + 2, y + 1, x + 2, y + h - 2);			// left
			EraseRect(rp, x + w - 3, y + 1, x + w - 3, y + h - 2);	// right
			EraseRect(rp, x + 3, y + 1, x + w - 4, y + 1);			// top
			EraseRect(rp, x + 3, y + h - 2, x + w - 4, y + h - 2);	// bottom
#endif

			/* draw dither-frame around the color field */

			SetDrPt(rp, 0x5555);

			SetABPenDrMd(rp, 1, 0, JAM2);
			DrawRect(rp, x + 3, y + 2, w - 7, h - 5);

			rp->LinePtrn = 0xffff;
		}

		DrawBevelBox(rp, x, y, w, h, GT_VisualInfo, vi, gad->Flags & GFLG_SELECTED ? GTBB_Recessed : TAG_IGNORE, TRUE, TAG_END);
	}

	if (mode != 2) {
		/* draw color field */

		if (cd->cd_Pen == -1)
			SetHighColor(rp, cd->cd_Color);
		else
			SetAPen(rp, cd->cd_Pen);

		RectFill(rp, x + 4, y + 3, x + w - 5, y + h - 4);
	}
}


ULONG
SetColorButton(struct Gadget *gad, struct GadgetInfo *gi, struct ColorGData *cd, struct TagItem *tstate)
{
	struct TagItem *ti;
	ULONG pen = cd->cd_Pen;

	while ((ti = NextTagItem(&tstate))) {
		switch (ti->ti_Tag) {
			case CGA_Color:
				cd->cd_Color = ti->ti_Data;
				cd->cd_Pen = GetColorPen(cd->cd_Color)->cp_Pen;
				break;
			case CGA_Pen:
				cd->cd_Pen = ti->ti_Data;
				break;
		}
	}

	if (gi && pen != cd->cd_Pen) {
		struct RastPort *rp = ObtainGIRPort(gi);

		DrawColorButton(rp, gad, cd, 42);
		ReleaseGIRPort(rp);
	}
	return 0;
}


IPTR PUBLIC
DispatchColorButton(REG(a0, Class *cl), REG(a2, struct Gadget *gad), REG(a1, Msg msg))
{
	struct ColorGData *cd;
	IPTR rc = 0L;

	cd = INST_DATA(cl,gad);

	switch (msg->MethodID) {
		case OM_NEW:
			if ((rc = DoSuperMethodA(cl, (Object *)gad, msg)) != 0) {
				cd = INST_DATA(cl, rc);
				SetAttrs((APTR)rc, GA_RelVerify, TRUE, TAG_END);
				SetColorButton((struct Gadget *)rc, NULL, cd, ((struct opSet *)msg)->ops_AttrList);
			}
			break;
		case GM_RENDER:
		{
			struct gpRender *gpr = (struct gpRender *)msg;

			DrawColorButton(gpr->gpr_RPort, gad, cd, gpr->gpr_Redraw);
			break;
		}
		case OM_SET:
			return DoSuperMethodA(cl, (Object *)gad, msg)
				| SetColorButton(gad, ((struct opSet *)msg)->ops_GInfo, cd, ((struct opSet *)msg)->ops_AttrList);

		case OM_GET:
			switch (((struct opGet *)msg)->opg_AttrID) {
				case CGA_Color:
					*((struct opGet *)msg)->opg_Storage = (ULONG)cd->cd_Color;
					return TRUE;
				case CGA_Pen:
					*((struct opGet *)msg)->opg_Storage = (ULONG)cd->cd_Pen;
					return TRUE;
				default:
					return DoSuperMethodA(cl, (Object *)gad, msg);
			}
			break;
		/*case GM_GOACTIVE:
		case GM_HANDLEINPUT:
		{
			struct gpInput *gpi = (struct gpInput *)msg;
			struct InputEvent *ie = gpi->gpi_IEvent;

			rc = DoSuperMethodA(cl,(Object *)gad,msg);
			if (ie->ie_Class == IECLASS_RAWMOUSE && ie->ie_Code == SELECTUP && !(gpi->gpi_Mouse.X >= 0 && gpi->gpi_Mouse.Y >= 0 && gpi->gpi_Mouse.X < gad->Width && gpi->gpi_Mouse.Y < gad->Height))
				rc = (rc & ~GMR_NOREUSE) | GMR_REUSE;
			break;
		}*/
		default:
			return DoSuperMethodA(cl, (Object *)gad, msg);
	}
	return rc;
}


/********************************** Icon-Obj **********************************/


IPTR PUBLIC
DispatchIconObj(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
	switch (msg->MethodID) {
		case GM_GOACTIVE:
		case GM_HANDLEINPUT:
		{
			ULONG  ti[5] = {GA_UserData,0,GA_ID,0,TAG_END};
			struct gpInput *gpi = (struct gpInput *)msg;
			struct InputEvent *ie = gpi->gpi_IEvent;

			if (ie->ie_Class == IECLASS_RAWMOUSE && ie->ie_Code == SELECTUP && gpi->gpi_Mouse.X >= 0 && gpi->gpi_Mouse.Y >= 0 && gpi->gpi_Mouse.X < ((struct Gadget *)o)->Width && gpi->gpi_Mouse.Y < ((struct Gadget *)o)->Height) {
				ti[1] = (ULONG)((struct Gadget *)o)->UserData;
				ti[3] = ((struct Gadget *)o)->GadgetID;
				DoSuperMethod(cl,o,OM_NOTIFY,ti,gpi->gpi_GInfo,0L);
			}
		}
		default:
			return(DoSuperMethodA(cl,o,msg));
			break;
	}
}


/********************************** Init/CleanUp **********************************/


void
FreeAppClasses(void)
{
	FreeClass(buttonclass);
	FreeClass(iconobjclass);

	FreeClass(pagegclass);
	FreeClass(indexgclass);

	FreeClass(framesgclass);

	if (pictureiclass) {
		CloseLibrary((struct Library *)pictureiclass->cl_UserData);  /* picture-datatype */
		FreeClass(pictureiclass);
	}
	FreeClass(bitmapiclass);
	FreeClass(popupiclass);
}


void
InitAppClasses(void)
{
    	
	SETHOOK(fillHook, fillHookFunc);
	SETHOOK(renderHook, RenderHook);
	SETHOOK(formelHook, FormelHook);
	SETHOOK(formatHook, FormatHook);
	SETHOOK(glinkHook, gLinkHook);
	SETHOOK(linkHook, LinkHook);
	SETHOOK(colorHook, ColorHook);
	SETHOOK(selectHook, SelectHook);
	SETHOOK(popUpHook, PopUpHook);
	SETHOOK(fileHook, HandleFileTypeIDCMP);

	CopyMem(GTD_GetHook(GTDH_TREE),&treeHook,sizeof(struct Hook));

	if ((buttonclass = MakeClass(NULL, "buttongclass", NULL, 0, 0)) != 0)
		SETDISPATCHER(buttonclass, DispatchButton);

	if ((colorgclass = MakeClass(NULL, "buttongclass", NULL, sizeof(struct ColorGData), 0)) != 0)
		SETDISPATCHER(colorgclass, DispatchColorButton);

	if ((iconobjclass = MakeClass(NULL, "frbuttonclass", NULL, 0, 0)) != 0)
		SETDISPATCHER(iconobjclass, DispatchIconObj);

	if ((pagegclass = MakeClass(NULL, "gadgetclass", NULL, sizeof(struct PageGData), 0)) != 0)
		SETDISPATCHER(pagegclass, DispatchPageGadget);

	if ((indexgclass = MakeClass(NULL, "gadgetclass", NULL, sizeof(struct IndexGData), 0)) != 0)
		SETDISPATCHER(indexgclass, DispatchIndexGadget);

	if ((framesgclass = MakeClass(NULL, "gadgetclass", NULL, 0, 0)) != 0)
		SETDISPATCHER(framesgclass, DispatchFramesGadget);

	if ((pictureiclass = MakeClass(NULL, "imageclass", NULL, sizeof(struct PictureIData), 0)) != 0) {
		SETDISPATCHER(pictureiclass, DispatchPictureImage);
		pictureiclass->cl_UserData = (IPTR)OpenLibrary("datatypes/picture.datatype",39);
	}
	if ((bitmapiclass = MakeClass(NULL, "imageclass", NULL, sizeof(struct BitmapIData), 0)) != 0)
		SETDISPATCHER(bitmapiclass, DispatchBitmapImage);

	if ((popupiclass = MakeClass(NULL, "imageclass", NULL, 0, 0)) != 0)
		SETDISPATCHER(popupiclass, DispatchPopUpImage);

}
