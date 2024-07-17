/* gClass for ignition
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "gclass.h"
#include "gclass_protos.h"

#if defined(__SASC)
#	include "gclass_pragmas.h"
#endif

#define CATCOMP_NUMBERS
#ifdef __amigaos4__
	#include <proto/exec.h>
	#include <proto/utility.h>
	#include "../ignition_strings.h"
#else
	#include "ignition_strings.h"
#endif

#include <string.h>

#ifdef __amigaos4__
	#undef  strcmp
	#define strcmp  Stricmp
#endif

const char *version = "$VER: text.gc 0.2 (6.8.2003)";

#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack(2)
		#endif
	#elif defined(__VBCC__)
		#pragma amiga-align
	#endif
#endif
struct gText {
	STRPTR gt_Text, gt_ShowText;
	struct Term *gt_Term;
	struct FontInfo *gt_FontInfo;
	ULONG gt_Color;
};
#ifdef __amigaos4__
	#ifdef __GNUC__
		#ifdef __PPC__
			#pragma pack()
		#endif
	#elif defined(__VBCC__)
		#pragma default-align
	#endif
#endif

struct gInterface interface[] = {
	{GOA_Pen,      NULL/*"Textfarbe:"*/, GIT_PEN, NULL, NULL},
	{GOA_FontInfo, NULL, GIT_FONT, NULL, NULL},
	{GOA_Text,     NULL, GIT_FORMULA, NULL, NULL},
	{0}
};

const STRPTR superClass = "root";
ULONG instanceSize = sizeof(struct gText);

static struct Catalog *sCatalog;
 

void PUBLIC
draw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *grp), REG(a1, struct gClass *gc),
	REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{	
	struct gText *gt = GINST_DATA(gc, go);
	long w, h;
	long width = gb->gb_Right - gb->gb_Left;
	long height = gb->gb_Bottom - gb->gb_Top;

	if ((gt->gt_FontInfo = SetFontInfoA(gt->gt_FontInfo, dpi, NULL))
		&& (h = OutlineHeight(gt->gt_FontInfo, gt->gt_ShowText, -1)) < height)
	{
		w = OutlineLength(gt->gt_FontInfo, gt->gt_ShowText, -1);
		if (w < width) {
			SetHighColor(grp, gt->gt_Color);
			DrawText(grp, gt->gt_FontInfo, gt->gt_ShowText, gb->gb_Left + (width - w) / 2, gb->gb_Top + (height - h) / 2);
				// zentriert
		}
	}
}


ULONG
set(struct Page *page, struct gObject *go, struct gText *gt, struct TagItem *tstate)
{
	struct TagItem *ti;
	ULONG rc = GCPR_NONE;

	if (tstate == NULL)
		return GCPR_NONE;


	while ((ti = NextTagItem(&tstate)) != 0) {
		switch (ti->ti_Tag) {
			case GOA_Text:
				// any changes to the text?
				if (!strcmp(gt->gt_Text ? gt->gt_Text : (STRPTR)"", ti->ti_Data ? (char *)ti->ti_Data : ""))
					break;

				FreeString(gt->gt_Text);
				FreeString(gt->gt_ShowText);
				DeleteTerm(gt->gt_Term);

				gt->gt_Text = AllocString((STRPTR)ti->ti_Data);
				gt->gt_Term = CreateTerm(page, gt->gt_Text);
				gt->gt_ShowText = CalcTerm(page, gt->gt_Text, gt->gt_Term, NULL);

				rc = GCPR_REDRAW;
				break;

			case GOA_FontInfo:
				FreeFontInfo(gt->gt_FontInfo);
				gt->gt_FontInfo = (struct FontInfo *)ti->ti_Data;
				rc = GCPR_REDRAW;
				break;

			case GOA_Pen:
				if (gt->gt_Color != ti->ti_Data) {
					gt->gt_Color = ti->ti_Data;
					rc = GCPR_REDRAW;
				}
				break;
		}
	}

	return rc;
}


ULONG PUBLIC
dispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
	struct gText *gt = GINST_DATA(gc, go);
	ULONG rc = 0L;

	switch (msg->MethodID) {
		case GCM_NEW:
			if ((rc = gDoSuperMethodA(gc, go, msg)) != 0) {
				go = (struct gObject *)rc;  gt = GINST_DATA(gc, go);

				go->go_Flags |= GOF_FRAMED;
				gt->gt_Color = 0x000000;
				gt->gt_FontInfo = NewFontInfoA(NULL, STANDARD_DPI, NULL);

				set(go->go_Page, go, gt, ((struct gcpSet *)msg)->gcps_AttrList);
			}
			break;

		case GCM_DISPOSE:
			FreeString(gt->gt_Text);
			FreeString(gt->gt_ShowText);
			FreeFontInfo(gt->gt_FontInfo);
			DeleteTerm(gt->gt_Term);

			rc = gDoSuperMethodA(gc, go, msg);
			break;

		case GCM_COPY:
		{
			struct gObject *cgo;

			if ((cgo = (struct gObject *)(rc = gDoSuperMethodA(gc, go, msg))) != 0) {
				struct gText *cgt = GINST_DATA(gc, cgo);

				cgt->gt_Text = AllocString(gt->gt_Text);
				cgt->gt_Term = CopyTerm(gt->gt_Term);
				cgt->gt_ShowText = AllocString(gt->gt_ShowText);
				cgt->gt_FontInfo = CopyFontInfo(gt->gt_FontInfo);
			}
			break;
		}
		case GCM_SET:
			rc = gDoSuperMethodA(gc, go, msg) | set(go->go_Page, go, gt, ((struct gcpSet *)msg)->gcps_AttrList);
			break;

		case GCM_GET:
			rc = TRUE;

			switch (((struct gcpGet *)msg)->gcpg_Tag) {
				case GOA_Text:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gt->gt_Text;
					break;
				case GOA_FontInfo:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gt->gt_FontInfo;
					break;
				case GOA_Pen:
					*((struct gcpGet *)msg)->gcpg_Storage = gt->gt_Color;
					break;

				default:
					rc = gDoSuperMethodA(gc, go, msg);
			}
			break;

		case GCM_RECALC:
		{
			STRPTR t = gt->gt_ShowText;

			gt->gt_ShowText = CalcTerm(go->go_Page, gt->gt_Text, gt->gt_Term, NULL);

			if (strcmp(gt->gt_ShowText ? gt->gt_ShowText : (STRPTR)"", t ? t : (STRPTR)""))
				rc = GCPR_REDRAW;

			FreeString(t);
			break;
		}
		default:
			return gDoSuperMethodA(gc, go, msg);
  }

  return rc;
}


ULONG PUBLIC
freeClass(REG(a0, struct gClass *gc))
{
	CloseCatalog(sCatalog);
	return TRUE;
}


ULONG PUBLIC
initClass(REG(a0, struct gClass *gc))
{
	sCatalog = OpenCatalog(NULL, "ignition.catalog", OC_BuiltInLanguage, "deutsch", TAG_END);
	interface[0].gi_Label = (STRPTR)GetCatalogStr(sCatalog, MSG_TEXT_COLOR_LABEL, "Textfarbe:");

	return TRUE;
}

