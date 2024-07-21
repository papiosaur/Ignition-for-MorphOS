/* 3D and diagram functions
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "classes.h"

#ifdef __amigaos4__
	#include <stdarg.h>
	#include <clib/macros.h>
#endif


#ifdef ENABLE_DIAGRAM_3D
#	include "pragmas/agl_pragmas.h"
#	include "pragmas/aglu_pragmas.h"
#	include "pragmas/aglut_pragmas.h"

#	include "gl/gl.h"
#	include "gl/glsm.h"
#	include "gl/glusm.h"
#	include "gl/glutsm.h"
#	include "gl/amigamesa.h"

	struct glreg gl_reg;
	struct glureg glu_reg;
	struct glutreg glut_reg;

	struct Library* glBase;
	struct Library* gluBase;
	struct Library* glutBase;
	ULONG  gcglcount;
#endif

double psx, psy;


#define GD_BORDER		4096	// 4 mm
#define GA_DEPTH_WIDTH	4096	// 4 mm


#ifdef ENABLE_DIAGRAM_3D

void
setv(struct point3d *a, struct point3d *b)
{
	CopyMem(b, a, sizeof(struct point3d));
}


void
addv(struct point3d *a, struct point3d *b)
{
	a->x += b->x;
	a->y += b->y;
	a->z += b->z;
}


void
subv(struct point3d *a, struct point3d *b)
{
	a->x -= b->x;
	a->y -= b->y;
	a->z -= b->z;
}


void
turnv(struct point3d *p)
{
	p->x *= -1.0;
	p->y *= -1.0;
	p->z *= -1.0;
}


double
lengthv(struct point3d *p)
{
	return sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
}


void
normv(struct point3d *p)
{
	double len = lengthv(p);

	p->x /= len;
	p->y /= len;
	p->z /= len;
}


struct text3d *
gText3d(STRPTR s, struct point3d *p, UBYTE size, ULONG pen, UBYTE flags)
{
	struct text3d *t = NULL;

	D(bug("gText3d()\n"));
/*  if (!g_sc)
		return(NULL);
	if (t = AllocPooled(pool,sizeof(struct text3d)))
	{
		if (flags & TF_COPIED)
			t->t_Label = AllocString(s);
		else
			t->t_Label = s;
		t->t_NumPoint = AddPoint(p->x,p->y,p->z);
		if (size == 100)
			t->t_FontInfo = CopyFontInfo(g_sc->sc_FontInfo);
		else
			t->t_FontInfo = SetFontInfo(g_sc->sc_FontInfo,dpi,FA_PointHeight,g_sc->sc_FontInfo->fi_FontSize->fs_PointHeight*size/100,TAG_END);
		t->t_Pen = pen;

		MyAddTail(&g_sc->sc_Texts,t);
	}*/
	return t;
}


double
zvalue(struct field3d *fi)
{
//  struct point3d *p;
	double d = 0.0;
//  WORD   *n,i;

	D(bug("zvalue()\n"));
 /* p = g_sc->sc_ProjPoints;*/

	/* Mittelwert */
/* for(n = fi->fi_Points,i = 0;i < fi->fi_NumPoints;i++,n++)
		d += (p+*n)->z;
	return(d/i); */

	/* Minimum */
/*  n = fi->fi_Points;
	d = (p+*(n++))->z;
	for(i = 1;i < fi->fi_NumPoints;i++,n++)
	{
		if (d > (p+*n)->z)
			d = (p+*n)->z;
	}*/
	return d;
}


int
zordercmp(struct field3d **f1, struct field3d **f2)
{
	double d1,d2;

	d1 = zvalue(*f1);
	d2 = zvalue(*f2);
	if (d1 > d2)
		return 1L;
	else if (d1 == d2)
		return 0L;

	return -1L;
}

#if 0
void
gSetColor(struct RastPort *rp, ULONG id, double intensity)
{
	SetHighColor(rp, TintColor(id, intensity));

	/* vielleicht noch Muster setzen? */
}
#endif

void
SetObjectColor(struct object3d *ob, LONG apen, LONG open)
{
	short i;

	if (ob)
	{
		for (i = 0; i < ob->ob_NumFields; i++)
		{
			if (apen != -1)
				(ob->ob_Fields+i)->fi_APen = apen;
			if (open != -1)
				(ob->ob_Fields+i)->fi_OPen = open;
		}
	}
}
#endif
 

long gtfcol, gtfrow;

/** Gibt die Zellen einer Tabelle in der für Diagramme richtigen
 *  Reihenfolge zurück.
 *
 *  @param page Seite der Tabelle
 *  @param tf letzte Zelle (NULL beim initialen Aufruf)
 *  @param tp der Bereich in der Tabelle
 *  @param horiz TRUE, wenn die Daten horizontal ausgelesen werden, sonst FALSE
 *  @param datarow legt fest, welche Reihe ausgelesen werden soll
 *
 *  @return tf ~0 für leere Zellen, NULL für die letzte Zelle, sonst ein Zeiger auf die Zelle
 */

struct tableField * PUBLIC
GetTableFields(REG(a0, struct Page *page), REG(a1, struct tableField *tf), REG(a2, struct tablePos *tp),
	REG(d0, BOOL horiz), REG(d1, long datarow))
{
	long col,row;

	if (!page || !tp)
		return NULL;

	if (tf)
	{
		if (tf == (struct tableField *)~0L)
		{
			col = gtfcol;
			row = gtfrow;
		}
		else
		{
			col = tf->tf_Col;
			row = tf->tf_Row;
		}
	}
	else
	{
		col = tp->tp_Col + (horiz ? 0 : datarow);
		row = tp->tp_Row + (horiz ? datarow : 0);
	}
	if (tf)
	{
		if (horiz)
		{
			col++;
			if (col > tp->tp_Col+tp->tp_Width)
				return NULL;
		}
		else
		{
			row++;
			if (row > tp->tp_Row+tp->tp_Height)
				return NULL;
		}
	}
	gtfcol = col;  gtfrow = row;
	if (!(tf = GetTableField(page, col, row)))
		tf = (struct tableField *)~0L;

	return tf;
}


struct gLink * PUBLIC
gGetLink(REG(a0, struct gDiagram *gd), REG(d0, long col), REG(d1, long row))
{
    struct gLink *link;
    
	if (!gd || !gd->gd_Links)
		return NULL;

#ifndef __amigaos4__
	if (gd->gd_ReadData == GDRD_VERT)
		link = gd->gd_Links+(col * gd->gd_Rows + row);
#endif
	link = gd->gd_Links+(col + row * gd->gd_Cols);
	//printf("Range Diagram: %s Wert in gLink=%lf\n", gd->gd_Range, link->gl_Value);
	return link;
}



void
#ifdef __amigaos4__
gInitLinks(struct gDiagram *gd)
#else
gInitLinks(struct gDiagram *gd, BOOL all)
#endif
{
	static const ULONG gil_rowcolors[] = {0xdd0000, 0x00dd00, 0x0000dd, 0xdddd00, 0x880000, 0x006600, 0xff9922, 0x880088, 0x000077, 0x333333};
	static const UBYTE gil_colors = 10;
	struct gLink *gl;
	long   i,j;

	MyNewList(&gd->gd_Values);

	for (j = 0; j < gd->gd_Rows; j++)
	{
		for (i = 0; i < gd->gd_Cols; i++)
		{
			if ((gl = gGetLink(gd, i, j)) != 0)
			{
				gl->gl_Row = j;
#ifndef __amigaos4__
				if (all && gl->gl_Color == ~0L)
#endif
				{
					if (gd->gd_Rows > 1)
						gl->gl_Color = gil_rowcolors[j % gil_colors];
					else
						gl->gl_Color = gil_rowcolors[i % gil_colors];

					gl->gl_Flags = 0;
				}
				gl->gl_Flags = (gl->gl_Flags & ~(GLF_FIRST_OF_ROW | GLF_LAST_OF_ROW)) | (i == 0 ? GLF_FIRST_OF_ROW : 0);

				MyAddTail(&gd->gd_Values, gl);	// generate a list (for list-view)
			}
		}
		if (gl)
			gl->gl_Flags |= GLF_LAST_OF_ROW;
	}
}


BOOL
gFillCells(struct gDiagram *gd)
{
	struct tableField *tf = NULL;
	long   i,col,row,num,wid;
	struct gLink *gl;
	ULONG  handle;
	UBYTE  changed = FALSE;

	if (!gd || !(gl = gd->gd_Links) || !gd->gd_DataPage)
		return FALSE;

#ifdef __amigaos4__
	if (gd->gd_ReadData == GDRD_VERT)
	{
		wid = gd->gd_TablePos.tp_Height + 1;
		num = wid * (gd->gd_TablePos.tp_Width + 1);
	}
	else
#endif
	{
		wid = gd->gd_TablePos.tp_Width + 1;
		num = wid * (gd->gd_TablePos.tp_Height + 1);
	}
	
	for (i = 0;i < num;i++)
		gl[i].gl_Cell = NULL;

	col = gd->gd_TablePos.tp_Col;  row = gd->gd_TablePos.tp_Row;
	if ((handle = GetCellIterator(gd->gd_DataPage,&gd->gd_TablePos,FALSE)) != 0)
	{
		while ((tf = NextCell(handle)) != 0)
#ifdef __amigaos4__
			if (gd->gd_ReadData == GDRD_VERT)
				gl[(tf->tf_Col - col) * wid + tf->tf_Row - row].gl_Cell = tf;
			else
#endif
				gl[tf->tf_Col - col + (tf->tf_Row - row) * wid].gl_Cell = tf;
		FreeCellIterator(handle);
	}

	for (i = 0;i < num;i++)
	{
		if ((tf = gl[i].gl_Cell) != 0)
		{
			if (gl[i].gl_Value != tf->tf_Value)
			{
				gl[i].gl_Value = tf->tf_Value;
				changed = TRUE;
			}
		}
		else if (gl[i].gl_Value != 0.0)
		{
			gl[i].gl_Value = 0.0;
			changed = TRUE;
		}
	}
	return changed;
}


void
RefreshPreviewSize(struct Window *win)
{
	struct winData *wd = (APTR)win->UserData;
	struct gObject *go = wd->wd_ExtData[0];

	G(bug("RefreshPreviewSize()\n"));

	if (!go)
		return;

	psx = psy = 1024;
	if (go->go_Right - go->go_Left)
		psx = 1024.0 * (win->Width - 2 - win->BorderLeft - win->BorderRight) / (go->go_Right - go->go_Left);
	if (go->go_Bottom - go->go_Top)
		psy = 1024.0 * (win->Height - 2 - win->BorderTop - win->BorderBottom) / (go->go_Bottom - go->go_Top);
}


/***************************** Diagram-Root *****************************/

struct gInterface gDiagramInterface[] =
{
	{GDA_ReadData,	NULL, GIT_CYCLE, NULL, "readdata"},
	{GDA_DataPage,	NULL, GIT_DATA_PAGE, NULL, "datapage"},
	{GDA_Range,		NULL, GIT_FORMULA, NULL, "range"},
	{0}
};

#define GDS_REMAKE 1
#define GDS_TRANSPLANDIAS 2


void
UpdateRangeData(struct gDiagram *gd,struct tablePos *tp)
{
	if (tp->tp_Col != gd->gd_TablePos.tp_Col || tp->tp_Row != gd->gd_TablePos.tp_Row || tp->tp_Width != gd->gd_TablePos.tp_Width || tp->tp_Height != gd->gd_TablePos.tp_Height)
	{
		long oldnum = (gd->gd_TablePos.tp_Width+1)*(gd->gd_TablePos.tp_Height+1);
		long num = (tp->tp_Width+1)*(tp->tp_Height+1);
		struct gLink *gl = gd->gd_Links;

		if (gd->gd_ReadData == GDRD_VERT)
		{
			gd->gd_Rows = tp->tp_Width+1;
			gd->gd_Cols = tp->tp_Height+1;
		}
		else
		{
			gd->gd_Rows = tp->tp_Height+1;
			gd->gd_Cols = tp->tp_Width+1;
		}
		CopyMem(tp,&gd->gd_TablePos,sizeof(struct tablePos));
		MyNewList(&gd->gd_Values);

		if ((gd->gd_Links = AllocPooled(pool, sizeof(struct gLink)*num)) != 0)
		{
			if (gl)
				CopyMem(gl,gd->gd_Links,sizeof(struct gLink)*min(num,oldnum));
			else
				oldnum = 0;

			if (oldnum < num)  // neue Links markieren
			{
				int i;

				for(i = oldnum;i < num;i++)
					gd->gd_Links[i].gl_Color = ~0L;
			}
#ifdef __amigaos4__
			gInitLinks(gd);
#else
			gInitLinks(gd, FALSE);
#endif
			gFillCells(gd);
		}
		if (gl)
			FreePooled(pool,gl,sizeof(struct gLink)*oldnum);
	}
}


static bool
UpdateRange(struct gDiagram *gd, STRPTR range, bool force)
{
	struct tablePos tp;
	struct Term *term;
	bool success = TRUE;

	if (!force && !zstrcmp(gd->gd_Range,range))
		return FALSE;

	FreeString(gd->gd_Range);
	if (!(gd->gd_Range = AllocString(range)))
		return FALSE;

	if ((term = CreateTree(gd->gd_DataPage, gd->gd_Range)) && FillTablePos(&tp, term))
	{
		if (gd->gd_Reference != NULL)
			UpdateReferences(gd->gd_Reference, term);
		else
			gd->gd_Reference = MakeReference(gd->gd_Object.go_Page, RTYPE_OBJECT, gd, term);

		if (gd->gd_Reference != NULL)
			UpdateRangeData(gd,&tp);
		else
			success = FALSE;

		DeleteTree(term);
	}
	return success;
}


static ULONG
gDiagramSet(struct gDiagram *gd,struct TagItem *tstate)
{
	struct TagItem *ti;
	ULONG  rc = GCPR_NONE;
	//BYTE   set = 0;

	if (tstate)
	{
		while ((ti = NextTagItem(&tstate)) != 0)
		{
			switch (ti->ti_Tag)
			{
				case GDA_Range:
					if (UpdateRange(gd, (STRPTR)ti->ti_Data, FALSE))
					{
						//set = GDS_REMAKE;
						rc |= GCPR_REDRAW;
					}
					break;
				case GDA_DataPage:
					if ((APTR)ti->ti_Data != gd->gd_DataPage)
					{
						gd->gd_DataPage = (struct Page *)ti->ti_Data;
#ifdef __amigaos4__
						gInitLinks(gd);
#else
						gInitLinks(gd, FALSE);
#endif
						gFillCells(gd);
						//set = GDS_REMAKE;
						rc |= GCPR_REDRAW;
					}
					break;
				case GDA_ReadData:
					gd->gd_ReadData = (BYTE)ti->ti_Data;
					if (gd->gd_ReadData == GDRD_VERT)
					{
						gd->gd_Rows = gd->gd_TablePos.tp_Width + 1;
						gd->gd_Cols = gd->gd_TablePos.tp_Height + 1;
					}
					else
					{
						gd->gd_Rows = gd->gd_TablePos.tp_Height + 1;
						gd->gd_Cols = gd->gd_TablePos.tp_Width + 1;
					}
#ifdef __amigaos4__
					gInitLinks(gd);
#else
					gInitLinks(gd, FALSE);
#endif
					gFillCells(gd);
					//set = GDS_REMAKE;
					rc |= GCPR_REDRAW;
					break;
			}
		}
	}
	return rc;
}


static uint32
gDiagramGet(struct gClass *gc, struct gDiagram *gd, struct gcpGet *gcpg)
{
	switch (gcpg->gcpg_Tag)
	{
		case GDA_ReadData:
			*gcpg->gcpg_Storage = gd->gd_ReadData;
			break;
		case GDA_DataPage:
			*gcpg->gcpg_Storage = (ULONG)gd->gd_DataPage;
			break;
		case GDA_Range:
			*gcpg->gcpg_Storage = (ULONG)gd->gd_Range;
			break;
		default:
			return gDoSuperMethodA(gc, gd, (Msg)gcpg);
	}
	return true;
}


ULONG PUBLIC
gDiagramDispatch(REG(a0, struct gClass *gc), REG(a2, struct gDiagram *gd), REG(a1, Msg msg))
{
	ULONG rc = 0L;

	switch (msg->MethodID)
	{
		case GCM_NEW:
			if ( ((APTR)  ((struct gClass *)gd)->gc_Dispatch  )  == gDiagramDispatch)  // keine Instanzen dieser Klasse
				return 0;

			if ((rc = gDoSuperMethodA(gc, gd, msg)) != 0)
			{
				struct point2d *p;

				gd = (struct gDiagram *)rc;

				MyNewList(&gd->gd_Values);
				gd->gd_Object.go_Flags |= GOF_FRAMED;

				gDiagramSet(gd,((struct gcpSet *)msg)->gcps_AttrList);

				if ((p = AllocPooled(pool, sizeof(struct point2d) * 2)) != 0)
				{
					p[0].x = 10240;  p[0].y = 10240;				// 1 cm Abstand von der linken, oberen Ecke
					p[1].x = 10240 + 102400;  p[1].y = 10240 + 102400;  // 10 cm breit und hoch

					gDoClassMethod(gc, gc, GCM_ENDPOINTS, p, 2, &gd->gd_Object.go_Knobs, &gd->gd_Object.go_NumKnobs);
					if (gd->gd_Object.go_Knobs != p)
						FreePooled(pool, p, sizeof(struct point2d) * 2);
				}
			}
			break;
		case GCM_DISPOSE:
			FreeString(gd->gd_Range);
			FreeReference(gd->gd_Reference, true);
			if (gd->gd_Links)
				FreePooled(pool, gd->gd_Links, sizeof(struct gLink) * (gd->gd_TablePos.tp_Width + 1) * (gd->gd_TablePos.tp_Height + 1));

			rc = gDoSuperMethodA(gc, gd, msg);
			break;
		case GCM_COPY:
		{
			struct gDiagram *cgd;

			if ((cgd = (struct gDiagram *)(rc = gDoSuperMethodA(gc, gd, msg))))
			{
				struct Term *term;

				cgd->gd_Range = AllocString(gd->gd_Range);
				if ((term = CreateTree(gd->gd_DataPage, gd->gd_Range)) != NULL)
				{
					cgd->gd_Reference = MakeReference(gd->gd_Object.go_Page, RTYPE_OBJECT, cgd, term);
					DeleteTree(term);
				}
				else
					cgd->gd_Reference = NULL;

				if (gd->gd_Links && (cgd->gd_Links = AllocPooled(pool, sizeof(struct gLink) * gd->gd_Rows * gd->gd_Cols)))
				{
					CopyMem(gd->gd_Links, cgd->gd_Links, sizeof(struct gLink) * gd->gd_Rows * gd->gd_Cols);
#ifdef __amigaos4__
					gInitLinks(cgd);
#else
					gInitLinks(cgd, FALSE);
#endif
				}
			}
			break;
		}
		case GCM_SET:
			rc = gDoSuperMethodA(gc, gd, msg) | gDiagramSet(gd, ((struct gcpSet *)msg)->gcps_AttrList);
			break;
		case GCM_GET:
			rc = gDiagramGet(gc, gd, (struct gcpGet *)msg);
			break;
		/*case GCM_HITTEST:
			break;*/
		case GCM_OPENWINDOW:
			return (ULONG)OpenAppWindow(WDT_DIAGRAM, WA_CurrentDiagram, gd, WA_Page, 2, TAG_END);
			break;
		case GCM_RECALC:
			if (gFillCells(gd))
				return GCPR_REDRAW;
			break;
		case GCM_INSERTREMOVECELLS:
		{
			struct gcpInReCells *gcpc = (APTR)msg;
			struct Term *t;

			if ((t = CreateTree(gd->gd_DataPage,gd->gd_Range)) != 0)
			{
				struct tablePos tp;

				tf_col = 0;  tf_row = 0;
				InReCeKn(t, gcpc->gcpc_Offset, gcpc->gcpc_Diff, gcpc->gcpc_Comp, gcpc->gcpc_First, gcpc->gcpc_Last);

				FreeString(gd->gd_Range);
				gd->gd_Range = TreeTerm(t, FALSE);

				FillTablePos(&tp, t);
				UpdateRangeData(gd, &tp);

				DeleteTree(t);
			}
			break;
		}
		case GCM_ENDPOINTS:
		{
			struct gcpEndPoints *gcpe = (APTR)msg;
			struct point2d *p = NULL;

D(bug("GCM_ENDPOINTS: %ld points: %ld:%ld, %ld:%ld\n",gcpe->gcpe_NumPoints,gcpe->gcpe_Points[0].x,gcpe->gcpe_Points[0].y,gcpe->gcpe_Points[1].x,gcpe->gcpe_Points[1].y));
			if (!gcpe->gcpe_Points && (p = gcpe->gcpe_Points = AllocPooled(pool,sizeof(struct point2d)*2)))
			{
				p[0].x = 10240;  p[0].y = 10240;				// 1 cm Abstand
				p[1].x = 10240+102400;  p[1].y = 10240+102400;  // 10 cm Breit
				gcpe->gcpe_NumPoints = 2;
			}
			rc = gDoSuperMethodA(gc,gd,msg);
D(bug("-> %ld points\n",*gcpe->gcpe_StorageNumPoints));
			if (p && *gcpe->gcpe_StoragePoints != p)
				FreePooled(pool,p,sizeof(struct point2d)*2);
			break;
		}
/*	case GCM_BOX:
		{
			LONG   xmin,xmax,ymin,ymax,i,num;
			struct scene3d *sc;
			struct point3d *p;

			rc = gDoSuperMethodA(gc,gd,msg);

			if ((sc = gd->gd_Scene) && (p = gd->gd_Scene->sc_ProjPoints))
			{
				num = sc->sc_NumPoints;

				xmin = xmax = p[0].x*10240;
				ymin = ymax = p[0].y*10240;

				for(i = 1;i < num;i++)
				{
					if (p[i].x*10240 < xmin)
						xmin = p[i].x*10240;
					else if (p[i].x*10240 > xmax)
						xmax = p[i].x*10240;
					if (p[i].y*10240 < ymin)
						ymin = p[i].y*10240;
					else if (p[i].y*10240 > ymax)
						ymax = p[i].y*10240;
				}
				sc->sc_Position.x = -xmin+GD_BORDER;
				sc->sc_Position.y = ymax+GD_BORDER;

				sc->sc_Size.x = sc->sc_Position.x+xmax+GD_BORDER;
				sc->sc_Size.y = sc->sc_Position.y-ymin+GD_BORDER;

				sc->sc_SizeFactorX = 1.0*(gd->gd_Object.go_mmRight-gd->gd_Object.go_mmLeft)/sc->sc_Size.x;
				sc->sc_SizeFactorY = 1.0*(gd->gd_Object.go_mmBottom-gd->gd_Object.go_mmTop)/sc->sc_Size.y;
			}
			break;
		}*/
		case GCM_INITAFTERLOAD:
			rc = gDoSuperMethodA(gc,gd,msg);
			gd->gd_DataPage = (struct Page *)FindListNumber(&gd->gd_Object.go_Page->pg_Mappe->mp_Pages,gd->gd_PageNumber);
			gFillCells(gd);
			break;
		case GCM_LOAD:
		{
			struct IFFHandle *iff = ((struct gcpIO *)msg)->gcpio_IFFHandle;
			char   range[128];
			struct gLink *gl;
			int	i,num;

			if ((rc = gDoSuperMethodA(gc,gd,msg)) < 0)
				break;

			ReadChunkBytes(iff,&gd->gd_Flags,1);
			ReadChunkBytes(iff,&gd->gd_ReadData,1);
			ReadChunkString(iff,range,128);
			ReadChunkBytes(iff,&gd->gd_PageNumber,4);

			UpdateRange(gd,range,FALSE);
			gl = gd->gd_Links;

			num = gd->gd_Cols*gd->gd_Rows;
			for(i = 0;i < num;i++)
			{
				ULONG color = 0;
				UBYTE value;

				ReadChunkBytes(iff,&value,1);
				ReadChunkBytes(iff,(UBYTE *)&color+1,3);

				if (gl)
				{
					gl[i].gl_Flags = value;
					gl[i].gl_Color = color;
				}
			}
			ReadChunkBytes(iff,&i,1);
			break;
		}
		case GCM_SAVE:
		{
			struct IFFHandle *iff = ((struct gcpIO *)msg)->gcpio_IFFHandle;
			struct gLink *gl = gd->gd_Links;
			int	i,num;

			if ((rc = gDoSuperMethodA(gc,gd,msg)) < 0)
				break;

			WriteChunkBytes(iff,&gd->gd_Flags,1);
			WriteChunkBytes(iff,&gd->gd_ReadData,1);
			WriteChunkString(iff,gd->gd_Range);
			if (gd->gd_DataPage)
				i = FindListEntry((struct MinList *)&gd->gd_DataPage->pg_Mappe->mp_Pages, (struct MinNode *)gd->gd_DataPage);
			else
				i = ~0L;
			WriteChunkBytes(iff, &i, 4);

			num = gd->gd_Cols*gd->gd_Rows;
			for (i = 0; i < num; i++)
			{
				WriteChunkBytes(iff, &gl[i].gl_Flags, 1);
				WriteChunkBytes(iff, (UBYTE *)&gl[i].gl_Color + 1, 3);
			}
			i = 0;
			WriteChunkBytes(iff,&i,1);
			break;
		}
		case GCDM_SETLINKATTR:
		{
			struct gcpSetLinkAttr *gcps = (APTR)msg;
			struct gLink *gl = gcps->gcps_Link;
			ULONG  color = gcps->gcps_Color;
			UBYTE  marked = gcps->gcps_Marked;

			if (!gl || (color == ~0L && marked == (UBYTE)~0) || (gl->gl_Color == color && ((gl->gl_Flags & GLF_MARKED) > 0) == (marked > 0)))
				return 0L;

			if (color != ~0L)
				gl->gl_Color = color;
			if (marked != (UBYTE)~0)
				gl->gl_Flags = (gl->gl_Flags & ~GLF_MARKED) | (marked ? GLF_MARKED : 0);
			return 1L;
		}
		default:
			return gDoSuperMethodA(gc, gd, msg);
	}
	return rc;
}


/***************************** 3D-Diagramm *****************************/

#ifdef ENABLE_DIAGRAM_3D

void
glColor(ULONG color)
{
	glColor3b((color >> 16) & 0xff,(color >> 8) & 0xff,color & 0xff);
}


void
CloseMesaLibraries(void)
{
	CloseLibrary(glutBase);
	CloseLibrary(gluBase);
	CloseLibrary(glBase);
}


BOOL
OpenMesaLibraries(struct gDiagram3d *gd,ULONG which)
{
	if (!which)
		return(TRUE);

	if (which && (glBase || (glBase = OpenLibrary("agl.library",0))))
	{
		if (!(which & GDOPENGL_GLU) || gluBase || (gluBase = OpenLibrary("aglu.library",0)))
		{
			if (!(which & GDOPENGL_GLUT) || glutBase || (glutBase = OpenLibrary("aglut.library",0)))
			{
				CacheClearU();

				gl_reg.size = (int)sizeof(struct glreg);
				gl_reg.func_exit = exit;
				registerGL(&gl_reg);

				if (gluBase)
				{
					glu_reg.size = (int)sizeof(struct glureg);
					glu_reg.glbase = glBase;
					registerGLU(&glu_reg);
				}
				if (glutBase)
				{
					glut_reg.size = (int)sizeof(struct glutreg);
					glut_reg.func_exit = exit;
					glut_reg.glbase = glBase;
					glut_reg.glubase = gluBase;
					registerGLUT(&glut_reg);
				}
				return(TRUE);
			}
			else
				ErrorOpenLibrary("aglut.library","StormMesa");
		}
		else
			ErrorOpenLibrary("aglu.library","StormMesa");
	}
	else
		ErrorOpenLibrary("agl.library","StormMesa");

	CloseLibrary(gluBase);
	CloseLibrary(glBase);

	return FALSE;
}


void PUBLIC
gDiagram3dDraw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *rp), REG(a1, struct gClass *gc),
	REG(a2, struct gDiagram3d *gd), REG(a3, struct gBounds *gb))
{
	LONG x = gb->gb_Left,y = gb->gb_Top;
	LONG w = gb->gb_Right-gb->gb_Left;
	LONG h = gb->gb_Bottom-gb->gb_Top;
	//struct gObject *go = &gd->gd_Diagram.gd_Object;

//bug("coords: (%ld:%ld - %ld:%ld)\n",x,y,x+go->go_Right-go->go_Left-1,y+go->go_Bottom-go->go_Top-1);
//bug("size: (%ld:%ld,%ld:%ld) pixel - (%ld:%ld,%ld:%ld) mm\n",go->go_Left,go->go_Top,go->go_Right,go->go_Bottom,go->go_mmLeft,go->go_mmTop,go->go_mmRight,go->go_mmBottom);
	SetAPen(rp,1);
	RectFill(rp,x,y,x+w-1,y+h-1);

	if (!gd->gd_Context)
		return;

	glMakeCurrent(gd->gd_Context,((struct glContext *)gd->gd_Context)->buffer);

	glViewport(x,y,x+w,y+h);

	/*if (gd->gd_Diagram.gd_CreateDiagram)
		gd->gd_Diagram.gd_CreateDiagram(gd);*/

	glFlush();
}


ULONG
gDiagram3dSet(struct gDiagram3d *gd,struct TagItem *tstate)
{
	struct TagItem *ti;
	ULONG  rc = GCPR_NONE;
	//BYTE   set = 0;

	if (tstate)
	{
		while(ti = NextTagItem(&tstate))
		{
			switch(ti->ti_Tag)
			{
				case GDA_OpenGL:
					OpenMesaLibraries(gd,ti->ti_Data);
					break;
				case GDA_RotX:
					gd->gd_RotX = ti->ti_Data;
					rc |= GCPR_UPDATESIZE;
					//set |= GDS_TRANSPLANDIAS;
					break;
				case GDA_RotY:
					gd->gd_RotY = ti->ti_Data;
					rc |= GCPR_UPDATESIZE;
					//set |= GDS_TRANSPLANDIAS;
					break;
				case GDA_RotZ:
					gd->gd_RotZ = ti->ti_Data;
					rc |= GCPR_UPDATESIZE;
					//set |= GDS_TRANSPLANDIAS;
					break;
			}
		}
	}

	return rc;
}


void
gFreeGLContext(struct gDiagram3d *gd)
{
	G(bug("freeGLContext: diagram: %lx\n",gd));
	if (!gd->gd_Context)
		return;

	glDestroyContext(gd->gd_Context);
	gd->gd_Context = NULL;

	if (!--gcglcount)
		CloseMesaLibraries();
}


APTR
gCreateGLContext(void)
{
	APTR c;

	G(bug("createGLContext\n"));
	if (c = glCreateContext(AMA_Screen,scr,AMA_RastPort,grp,
				 AMA_RGBMode, GL_TRUE,
				 TAG_END))
		gcglcount++;
	else
		ErrorRequest(GetString(&gLocaleInfo,MSG_GL_CONTEXT_ERR));

	return c;
}


APTR
gMakeGLContext(struct gDiagram3d *gd)
{
	G(bug("makeGLContext: diagram: %lx\n",gd));
	if (!gd || gd->gd_Context)
		return NULL;

	if (!OpenMesaLibraries(gd,GDOPENGL_GL))
		return NULL;

	return gd->gd_Context = gCreateGLContext();
}


ULONG PUBLIC
gDiagram3dDispatch(REG(a0, struct gClass *gc), REG(a2, struct gDiagram3d *gd), REG(a1, Msg msg))
{
	ULONG rc = 0L;

	switch (msg->MethodID)
	{
		case GCM_NEW:
			if (((struct gClass *)gd)->gc_Dispatch == gDiagram3dDispatch)  // keine Instanzen dieser Klasse
				return(NULL);
			if (rc = gDoSuperMethodA(gc,gd,msg))
			{
				gd = (struct gDiagram3d *)rc;

				gd->gd_Observer.z = 150.0;
				gd->gd_RotX = -15;
				gd->gd_RotY = 12;

				gd->gd_Context = gMakeGLContext(gd);

				gDiagram3dSet(gd,((struct gcpSet *)msg)->gcps_AttrList);
			}
			break;
		case GCM_DISPOSE:
			gFreeGLContext(gd);

			rc = gDoSuperMethodA(gc,gd,msg);
			break;
		case GCM_COPY:
		{
			struct gDiagram3d *cgd;

			if (cgd = (struct gDiagram3d *)gDoSuperMethodA(gc,gd,msg))
			{
				cgd->gd_Context = NULL;
				cgd->gd_Context = gMakeGLContext(cgd);
			}
			break;
		}
		case GCM_SET:
			rc = gDoSuperMethodA(gc,gd,msg) | gDiagram3dSet(gd,((struct gcpSet *)msg)->gcps_AttrList);
			break;
		case GCM_ADD_TO_SCREEN:
			gMakeGLContext(gd);
			break;
		case GCM_REMOVE_FROM_SCREEN:
			gFreeGLContext(gd);
			break;
		default:
			rc = gDoSuperMethodA(gc,gd,msg);
	}

	return rc;
}


/***************************** 3D-Balken-Diagramm *****************************/


void PUBLIC
gBalken3dDraw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *rp), REG(a1, struct gClass *gc),
	REG(a2, struct gDiagram3d *gd), REG(a3, struct gBounds *gb))
{
	//struct point3d rectps[] = {{0.0,0.0,0.0},{6.0,0.0,0.0},{6.0,0.0,2.0}};
	struct point3d boxps[] = {{0.3,3.0,0.5},{0.3,0.0,0.5},{1.3,0.0,0.5},{0.3,0.0,1.5}};
	struct gLink *gl;
	struct tableField *tf;
	double max,min,move;
	long   i,j,k;

//  return;

	glBegin(GL_QUADS);

	min = max = move = 0.0;

	for(j = 0;j < gd->gd_Diagram.gd_Rows;j++)
	{
		for(k = 0;k < 4;k++)
			boxps[k].x = k == 2 ? 1.3 : 0.3;

		for(i = 0;i < gd->gd_Diagram.gd_Cols;i++)
		{
			gl = gGetLink(gd,i,j);

			if (tf = gl->gl_Cell)
			{
				boxps[0].y = fabs(tf->tf_Value);
				if (tf->tf_Value < 0.0)
				{
					move = tf->tf_Value;

					for(k = 0;k < 4;k++)
						boxps[k].y += move;
				}
			}
			else
				boxps[0].y = 0.0;

			if (boxps[0].y < min)
				min = boxps[0].y;
			else if (boxps[0].y > max)
				max = boxps[0].y;

			glColor(gl->gl_Color);

			glVertex3dv((GLdouble const *)&boxps[0]);
			glVertex3dv((GLdouble const *)&boxps[1]);
			glVertex3dv((GLdouble const *)&boxps[2]);
			glVertex3dv((GLdouble const *)&boxps[3]);

			for(k = 0;k < 4;k++)
				boxps[k].x += 1.5;
			if (move)
			{
				for(k = 0;k < 4;k++)
					boxps[k].y -= move;
				move = 0.0;
			}
		}
		if (j < gd->gd_Diagram.gd_Rows-1)
		{
			for(k = 0;k < 4;k++)
				boxps[k].z += 1.5;
		}
	}
	glEnd();
 /*   if (o = gRect3dA(rectps))
			SetObjectColor(o,0xff0000,-1);
		rectps[1].x = 0.0;
		rectps[1].y = 3.5;
		rectps[2].x = 0.0;
		rectps[2].y = 3.5;
		if (o = gRect3dA(rectps))
			SetObjectColor(o,0x00ff00,-1);

		for(i = 0;i < 4;i++)
			boxps[i].x += 1.5;
		boxps[0].y -= 0.5;
		if (o = gBox3dA(boxps))
			SetObjectColor(o,0xffffff,-1);
		for(i = 0;i < 4;i++)
			boxps[i].x += 1.5;
		boxps[0].y -= 1;
		if (o = gBox3dA(boxps))
			SetObjectColor(o,0x0000ff,-1);
		for(i = 0;i < 4;i++)
			boxps[i].x += 1.5;
		boxps[0].y -= 0.3;
		if (o = gBox3dA(boxps))
			SetObjectColor(o,0xffff00,-1);
	}*/
}


ULONG PUBLIC
gBalken3dDispatch(REG(a0, struct gClass *gc), REG(a2, struct gDiagram3d *gd), REG(a1, Msg msg))
{
	ULONG rc;

	switch (msg->MethodID)
	{
		case GCM_NEW:
			if (rc = gDoSuperMethodA(gc,gd,msg))
			{
				//gd = (struct gDiagram3d *)rc;
				//gd->gd_Diagram.gd_CreateDiagram = gBalken3dCreateDiagram;
			}
			break;
		/*case GCM_DISPOSE:
			break;
		case GCM_SET:
			break;
		case GCM_GET:
			break;
		case GCM_HITTEST:
			break;*/
		default:
			rc = gDoSuperMethodA(gc,gd,msg);
	}
	return rc;
}
#endif // ENABLE_DIAGRAM_3D

/***************************** 2D-Achsen für Diagramme *****************************/

struct gInterface gAxesInterface[] =
{
	{GAA_FontInfo,		NULL, GIT_FONT,		NULL, "axes_font"},
	{GAA_NumberPen,		NULL, GIT_PEN,		NULL, "axes_numberpen"},
	{GAA_Raster,		NULL, GIT_CYCLE,	NULL, "axes_raster"},
	{GAA_ShowNumbers,	NULL, GIT_CHECKBOX,	NULL, "axes_shownumbers"},
	{GAA_TransparentBackground, NULL, GIT_CHECKBOX, NULL, "axes_background"},
	{GAA_BPen,			NULL, GIT_PEN,		NULL, "axes_backgroundpen"},
	{0}
};


/** Berechnet die Koordinate eines Y-Wertes in der Darstellung
 */

long REGARGS
GetAxisY(struct gAxes *ga, double y, int scale)
{
	/* MERKER: scale ist noch nicht implementiert! */
	return (long)((ga->ga_Max - y) * (ga->ga_Bounds.gb_Bottom - ga->ga_Bounds.gb_Top) / (ga->ga_Max - ga->ga_Min) + 0.5) + ga->ga_Bounds.gb_Top;
}
		
 
/** Berechnet den für eine Achse benutzten 10er Logarithmus
 */

double REGARGS
GetAxisLg(double x)
{
	double lg;

	lg = log10(fabs(x));
	if (lg < 0)
		lg--;
	else if (lg > 2)
		lg--;

	return lg;
}


double
GetAxisValue(double x, double b, BOOL lower)
{
	if (lower)
		return (int)((x - fmod(x, b)) / b) * b;

	return (int)((x + b - fmod(x + b, b)) / b) * b;
}


void
CalcAxes(struct gDiagram *gd, struct gAxes *ga)
{
	struct gLink *gl;
	double mini,maxi,a;
	int	i,j;

	if (ga->ga_Flags & GAF_STATICAXIS)
	{
		ga->ga_Divisor = GetAxisLg(ga->ga_Max);
		return;
	}
	// Minima und Maxima finden
	for(j = 0;j < gd->gd_Rows;j++)
	{
		for(i = 0;i < gd->gd_Cols;i++)
		{
			if (!(gl = gGetLink(gd,i,j)))
				continue;

			a = gl->gl_Value;
			if (i == 0 && j == 0)
				mini = maxi = a;
			else if (a > maxi)
				maxi = a;
			else if (a < mini)
				mini = a;
		}
	}
	if (mini == maxi)
		maxi += 1;

	{
		double lga = GetAxisLg(maxi);
		double lgb = GetAxisLg(mini);

		a = max(lga,lgb);
	}
	a = pow(10.0,1.0*(int)a);

	if (mini < 0.0)
		ga->ga_Min = GetAxisValue(mini,a,TRUE);
	else
		ga->ga_Min = 0.0;

	ga->ga_Max = GetAxisValue(maxi,a,FALSE);
	ga->ga_Divisor = a;

//bug("-> max = %ld, min = %ld, divisor = %ld\n",(long)ga->ga_Max,(long)ga->ga_Min,(long)ga->ga_Divisor);
}


void
ResizeAxes(struct Page *page, struct gDiagram *gd, struct gAxes *ga, struct gBounds *gb)
{
	char  t[20],s[20],*a;
	long  height = ga->ga_FontInfo->fi_FontSize->fs_EMHeight >> 1;
	long  offset;

	/* MERKER: natürlich nur zum Testen... */
	CalcAxes(gd, ga);

	strcpy(s, ita(ga->ga_Min, -1, ITA_NONE));
	strcpy(t, ita(ga->ga_Max, -1, ITA_NONE));

	if (strlen(s) >= strlen(t))
		a = s;
	else
		a = t;

	if (ga->ga_Flags & GAF_PSEUDO3D)
	{
		gDoMethod(gd, GCM_GET, GAA_Pseudo3DDepth, &ga->ga_Depth);
		if (page != NULL)
			offset = ga->ga_Depth * pixel(page, GA_DEPTH_WIDTH, TRUE);
		else
			offset = ga->ga_Depth * mm_to_pixel_dpi(gDPI, GA_DEPTH_WIDTH, TRUE);
	}
	else
		offset = 0;

	ga->ga_Bounds.gb_Left = gb->gb_Left + (ga->ga_Flags & GAF_SHOWNUMBERS ? OutlineLength(ga->ga_FontInfo, a, -1) : 0) + 15;
	ga->ga_Bounds.gb_Top = gb->gb_Top + height + 2 + offset;
	ga->ga_Bounds.gb_Right = gb->gb_Right - 5 - offset;
	ga->ga_Bounds.gb_Bottom = gb->gb_Bottom - height - 2;
}


void PUBLIC
gAxesDraw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *rp),
	REG(a1, struct gClass *gc), REG(a2, struct gDiagram *gd), REG(a3, struct gBounds *gb))
{
	struct gAxes *ga = GINST_DATA(gc, gd);
	long   offset = 0;
	long   c;

	ga->ga_FontInfo = SetFontInfoA(ga->ga_FontInfo, dpi, NULL);

	if (gb->gb_Left != ga->ga_Frame.gb_Left || gb->gb_Top != ga->ga_Frame.gb_Top || gb->gb_Right != ga->ga_Frame.gb_Right || gb->gb_Bottom != ga->ga_Frame.gb_Bottom)
		ResizeAxes(page, gd, ga, gb);
					
	if ((ga->ga_Flags & GAF_PSEUDO3D) != 0)
	{
		if (page != NULL)
			offset = ga->ga_Depth * pixel(page, GA_DEPTH_WIDTH, TRUE);
		else
			offset = ga->ga_Depth * mm_to_pixel_dpi(dpi, GA_DEPTH_WIDTH, TRUE);
	}

	if ((ga->ga_Flags & GAF_TRANSPARENT_BACKGROUND) == 0)
	{
		SetHighColor(rp, ga->ga_BPen);
		RectFill(rp, ga->ga_Bounds.gb_Left + offset,ga->ga_Bounds.gb_Top - offset,ga->ga_Bounds.gb_Right + offset,
			ga->ga_Bounds.gb_Bottom - offset);
	}
	if (offset != 0)
	{
		// linker Rand
		SetHighColor(rp,TintColor(ga->ga_BPen, 0.9f));
		gAreaMove(rp, ga->ga_Bounds.gb_Left, ga->ga_Bounds.gb_Top);
		gAreaDraw(rp, ga->ga_Bounds.gb_Left + offset,ga->ga_Bounds.gb_Top - offset);
		gAreaDraw(rp, ga->ga_Bounds.gb_Left + offset,ga->ga_Bounds.gb_Bottom - offset);
		gAreaDraw(rp, ga->ga_Bounds.gb_Left, ga->ga_Bounds.gb_Bottom);
		gAreaEnd(rp);

		// unterer Rand
		SetHighColor(rp, TintColor(ga->ga_BPen, 0.7f));
		if (ga->ga_Min > 0 || ga->ga_Max < 0)
			c = ga->ga_Bounds.gb_Bottom;
		else
			c = GetAxisY(ga, 0.0, 1);
		gAreaMove(rp, ga->ga_Bounds.gb_Left, c);
		gAreaDraw(rp, ga->ga_Bounds.gb_Left + offset, c - offset);
		gAreaDraw(rp, ga->ga_Bounds.gb_Right + offset, c - offset);
		gAreaDraw(rp, ga->ga_Bounds.gb_Right, c);
		gAreaEnd(rp);
	}
	SetHighColor(rp, ga->ga_APen);

	Move(rp, ga->ga_Bounds.gb_Left - 1, ga->ga_Bounds.gb_Top);	 // left Y-axis
	Draw(rp, ga->ga_Bounds.gb_Left - 1, ga->ga_Bounds.gb_Bottom);

	Move(rp, ga->ga_Bounds.gb_Left - 5, c = GetAxisY(ga, 0.0, 1));   // zero (X-axis)
	Draw(rp, ga->ga_Bounds.gb_Right, c);

	if (ga->ga_FontInfo)
	{
#if defined __amigaos4__ || defined __MORPHOS__
		double y = (ga->ga_Min > 0.0 ? 0.0 : ga->ga_Min), div = ga->ga_Divisor;  //So nun auch eine Y Beschriftung bei Werten unter 0
		long w, i, h = (ga->ga_Max - ga->ga_Min) /div, old, lastfont;
#else
		double y = *max(ga->ga_Min,0.0), div = ga->ga_Divisor;
		long w, i, h = ga->ga_Max / div, old, lastfont;
#endif
		char t[20];

		old = lastfont = 0x7fffffff;
		if (h > 9)
		{
			div *= 2.5;
			h = ga->ga_Max/div;
		}

		/*** MERKER: die Auswahl der Label und Striche sollte noch ein wenig überarbeitet werden... ***/

		if (ga->ga_Raster == GAR_POINTS)
			rp->linpatcnt = 15;

		for (i = 0; i <= h; i++, old = c, y += div)
		{
			c = GetAxisY(ga, y, 1);
			if (c >= old - 1)
				continue;

			if (ga->ga_NumberPen != ga->ga_APen)
				SetHighColor(rp,ga->ga_APen);
			Move(rp, ga->ga_Bounds.gb_Left - 1, c);

			if (c < lastfont - OutlineHeight(ga->ga_FontInfo, t, -1) - 2)
			{
				strcpy(t, ita(y, -1, ITA_NONE));

				Draw(rp, ga->ga_Bounds.gb_Left - 5, c);

				if (ga->ga_Raster)
				{
					if (ga->ga_Raster == GAR_POINTS && y != 0)
						rp->LinePtrn = 0xaaaa;

					Move(rp, ga->ga_Bounds.gb_Left, c);
					Draw(rp, ga->ga_Bounds.gb_Left + offset, c - offset);
					Draw(rp, ga->ga_Bounds.gb_Right + offset, c - offset);

					if (offset != 0 && y == 0)
						Draw(rp, ga->ga_Bounds.gb_Right, c);

					if (ga->ga_Raster == GAR_POINTS && y != 0)
						rp->LinePtrn = 0xffff;
				}
				
				if (ga->ga_Flags & GAF_SHOWNUMBERS)
				{
					if (ga->ga_NumberPen != ga->ga_APen)
						SetHighColor(rp,ga->ga_NumberPen);

					w = OutlineLength(ga->ga_FontInfo, t, -1) + 7;
					DrawText(rp, ga->ga_FontInfo, t, ga->ga_Bounds.gb_Left - w, c - OutlineHeight(ga->ga_FontInfo, t, -1) / 2);
				}
				lastfont = c;
			}
			else
				Draw(rp, ga->ga_Bounds.gb_Left - 3, c);
		}
	}
}


ULONG
gAxesSet(struct gDiagram *gd, struct gAxes *ga, struct TagItem *tstate)
{
	struct TagItem *ti;
	ULONG  rc = GCPR_NONE;

	if (tstate)
	{
		while ((ti = NextTagItem(&tstate)) != 0)
		{
			switch (ti->ti_Tag)
			{
				case GAA_Raster:
					if (ga->ga_Raster != ti->ti_Data)
					{
						ga->ga_Raster = ti->ti_Data;
						rc |= GCPR_REDRAW;
					}
					break;
				case GAA_ShowNumbers:
					if (((ga->ga_Flags & GAF_SHOWNUMBERS) ? TRUE : FALSE) != (ti->ti_Data ? TRUE : FALSE))
					{
						ga->ga_Flags = (ga->ga_Flags & ~GAF_SHOWNUMBERS) | (ti->ti_Data ? GAF_SHOWNUMBERS : 0);
						rc |= GCPR_REDRAW;
					}
					break;
				case GAA_FontInfo:
					FreeFontInfo(ga->ga_FontInfo);
					ga->ga_FontInfo = (struct FontInfo *)ti->ti_Data;
					rc |= GCPR_REDRAW;
					break;
				case GAA_NumberPen:
					if (ga->ga_NumberPen != ti->ti_Data)
					{
						ga->ga_NumberPen = ti->ti_Data;
						rc |= GCPR_REDRAW;
					}
					break;
				case GAA_BPen:
					if (ga->ga_BPen != ti->ti_Data)
					{
						ga->ga_BPen = ti->ti_Data;
						rc |= GCPR_REDRAW;
					}
					break;
				case GAA_Pseudo3D:
					if (((ga->ga_Flags & GAF_PSEUDO3D) ? TRUE : FALSE) != (ti->ti_Data ? TRUE : FALSE))
					{
						ga->ga_Flags = (ga->ga_Flags & ~GAF_PSEUDO3D) | (ti->ti_Data ? GAF_PSEUDO3D : 0);
						rc |= GCPR_REDRAW;
					}
					break;
				case GAA_TransparentBackground:
					if (((ga->ga_Flags & GAF_TRANSPARENT_BACKGROUND) ? TRUE : FALSE) != (ti->ti_Data ? TRUE : FALSE))
					{
						ga->ga_Flags = (ga->ga_Flags & ~GAF_TRANSPARENT_BACKGROUND) | (ti->ti_Data ? GAF_TRANSPARENT_BACKGROUND : 0);
						rc |= GCPR_REDRAW;
					}
					break;
			}
		}
	}
	return rc;
}


ULONG PUBLIC
gAxesDispatch(REG(a0, struct gClass *gc), REG(a2, struct gDiagram *gd), REG(a1, Msg msg))
{
	struct gAxes *ga = GINST_DATA(gc, gd);
	ULONG  rc = 0L;

	switch (msg->MethodID)
	{
		case GCM_NEW:
			if ((rc = gDoSuperMethodA(gc, gd, msg)) != 0)
			{
				gd = (struct gDiagram *)rc;
				ga = GINST_DATA(gc, gd);

				ga->ga_Flags |= GAF_SHOWNUMBERS;
				ga->ga_APen = 0x000000;
				ga->ga_BPen = 0xffffff;
				ga->ga_Depth = 1;   // MERKER: sollte nach der Tiefe des gDiagrams gehen

				ga->ga_FontInfo = NewFontInfoA(NULL, ~0L, NULL);

				gAxesSet(gd, ga, ((struct gcpNew *)msg)->gcpn_AttrList);
				CalcAxes(gd, ga);
			}
			break;
		case GCM_DISPOSE:
			FreeFontInfo(ga->ga_FontInfo);
			rc = gDoSuperMethodA(gc, gd, msg);
			break;
		case GCM_COPY:
		{
			struct gObject *cgo;

			if ((cgo = (struct gObject *)(rc = gDoSuperMethodA(gc, gd, msg))) != 0)
			{
				struct gAxes *cga = GINST_DATA(gc, cgo);

				cga->ga_FontInfo = CopyFontInfo(ga->ga_FontInfo);
			}
			break;
		}
		case GCM_SET:
			rc = gDoSuperMethodA(gc,gd,msg) | gAxesSet(gd,ga,((struct gcpSet *)msg)->gcps_AttrList);
			break;
		case GCM_GET:
			rc = TRUE;

			switch (((struct gcpGet *)msg)->gcpg_Tag)
			{
/*		case GOA_Text:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gb->gb_Text;
					break;*/
				case GAA_FontInfo:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)ga->ga_FontInfo;
					break;
				case GAA_NumberPen:
					*((struct gcpGet *)msg)->gcpg_Storage = ga->ga_NumberPen;
					break;
/*		case GAA_APen:
					*((struct gcpGet *)msg)->gcpg_Storage = ga->ga_APen;
					break;*/
				case GAA_BPen:
					*((struct gcpGet *)msg)->gcpg_Storage = ga->ga_BPen;
					break;
				case GAA_ShowNumbers:
					*((struct gcpGet *)msg)->gcpg_Storage = ga->ga_Flags & GAF_SHOWNUMBERS ? TRUE : FALSE;
					break;
				case GAA_Pseudo3D:
					*((struct gcpGet *)msg)->gcpg_Storage = ga->ga_Flags & GAF_PSEUDO3D ? TRUE : FALSE;
					break;
				case GAA_TransparentBackground:
					*((struct gcpGet *)msg)->gcpg_Storage = ga->ga_Flags & GAF_TRANSPARENT_BACKGROUND ? TRUE : FALSE;
					break;
				case GAA_DepthWidth:
					*((struct gcpGet *)msg)->gcpg_Storage = GA_DEPTH_WIDTH;
					break;
				case GAA_Bounds:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)&ga->ga_Bounds;
					break;
				case GAA_Raster:
					*((struct gcpGet *)msg)->gcpg_Storage = ga->ga_Raster;
					break;
				default:
					rc = gDoSuperMethodA(gc,gd,msg);
			}
			break;
		/*case GCM_HITTEST:
			break;*/
		/******************************** additional methods ********************************/
		case GCAM_GETBORDERS:
		{
			struct gcapGetBorders *gcap = (APTR)msg;
			long   offset = (ga->ga_Flags & GAF_PSEUDO3D) ? pixel(gd->gd_Object.go_Page, GA_DEPTH_WIDTH, TRUE) * gcap->gcap_Row : 0;

			if (gcap->gcap_StorageLeft)
				*gcap->gcap_StorageLeft = (long)ga->ga_Bounds.gb_Left + offset;
			if (gcap->gcap_StorageRight)
				*gcap->gcap_StorageRight = (long)ga->ga_Bounds.gb_Right + offset;
			if (gcap->gcap_StorageYOrigin)
				*gcap->gcap_StorageYOrigin = GetAxisY(ga, 0.0, 1) - offset;
			break;
		}
		case GCAM_GETCOORD:
		{
			struct gcapGetCoord *gcap = (APTR)msg;
			long   offset = (ga->ga_Flags & GAF_PSEUDO3D) ? pixel(gd->gd_Object.go_Page, GA_DEPTH_WIDTH, TRUE) * gcap->gcap_Row + 2 : 0;
			long   y;

			y = GetAxisY(ga,gcap->gcap_Y,1) - offset;
#ifndef __amigaos4__
			if (y < 0)
				return 0;
#endif
			return (ULONG)y;
		}
		case GCM_SAVE:
			if ((rc = gDoSuperMethodA(gc,gd,msg)) < 0)
				break;

			SaveGInterface((struct gcpIO *)msg,gc->gc_Interface,(struct gObject *)gd);
			break;
		case GCM_LOAD:
		{
			struct TagItem *tags;

			if ((rc = gDoSuperMethodA(gc, gd, msg)) < 0)
				break;

			tags = LoadGObjectTags((struct gcpIO *)msg);
			gDoMethod(gd, GCM_SET, tags);
			FreeLoadedGObjectTags(tags);
			break;
		}
		default:
			rc = gDoSuperMethodA(gc, gd, msg);
	}
	return rc;
}


/***************************** Diagramm-Referenz-Klasse *****************************/


struct gInterface gEmbedDiagramInterface[] =
{
	{GEA_References, NULL, GIT_BUTTON, NULL},
	{0}
};


void PUBLIC
gEmbedDiagramDraw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *rp), REG(a1, struct gClass *gc),
	REG(a2, struct gDiagram *gd), REG(a3, struct gBounds *gb))
{
	gSuperDraw(page, dpi, rp, gc, (struct gObject *)gd, gb);
}


ULONG PUBLIC
gEmbedDiagramDispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
	ULONG  rc;

	switch (msg->MethodID)
	{
		case GCM_NEW:
			if ((rc = gDoSuperMethodA(gc, go, msg)))
			{
				struct gEmbedded *ge = GINST_DATA(gc->gc_Super, (struct gObject *)rc);

				ge->ge_Type = GET_DIAGRAM;
			}
			break;
		case GCM_COPY:
		{
			struct gObject *cgo;

			// don't call the super method's GCM_COPY but the one of the super class's
			// super class and copy the referenced object instead of only linking it
			// to this object
			if ((cgo = (struct gObject *)(rc = gDoSuperMethodA(gc->gc_Super, go, msg))))
			{
				struct gEmbedded *cge = GINST_DATA(gc->gc_Super, cgo);

				cge->ge_Link.l_Link = cgo;
				if (cge->ge_References != NULL)
				{
					struct gDiagram *diagram = (struct gDiagram *)gDoMethod(cge->ge_References, GCM_COPY);
 
					cge->ge_References = (struct gObject *)diagram;
					if (diagram != NULL) {
						diagram->gd_Object.go_Page = go->go_Page;
						MyAddTail(&diagram->gd_Object.go_ReferencedBy, &cge->ge_Link);
						MyAddTail(&go->go_Page->pg_gDiagrams, diagram);
					}
				}
			}
			break;
		}

		case GCM_CLOSEWINDOW:
		{
			struct gEmbedded *ge = GINST_DATA(gc->gc_Super, go);

			rc = gDoSuperMethodA(gc, go, msg);
			
			if (ge->ge_References)
				gDoMethod(ge->ge_References, GCM_CLOSEWINDOW);
			break;
		}
		case GCM_INITAFTERLOAD:
		{
			struct gEmbedded *ge = GINST_DATA(gc->gc_Super, go);
			struct gObject *rgo;
			struct Page *page;

			rc = gDoSuperMethodA(gc, go, msg);

			if (ge->ge_Type != GET_DIAGRAM || !go->go_Page || !(page = (struct Page *)FindListNumber(&go->go_Page->pg_Mappe->mp_Pages,ge->ge_PageNumber)))
				break;

			if ((rgo = (struct gObject *)FindListNumber(&page->pg_gDiagrams, ge->ge_Pos)) != 0)
			{
				ULONG tags[3] = {GEA_References, 0, TAG_END};

				tags[1] = (ULONG)rgo;
				gDoMethod(go, GCM_SET, tags);
			}
			break;
		}
		default:
			return gDoSuperMethodA(gc, go, msg);
	}
	return rc;
}


/***************************** Fenster-Handling für Diagramme *****************************/


void ASM
handlePreviewIDCMP(REG(a0, struct TagItem *tag))
{
	//struct gObject *go = wd->wd_ExtData[0];

	switch (imsg.Class)
	{
		case IDCMP_NEWSIZE:
			RefreshPreviewSize(win);
			RefreshAppWindow(win, wd);
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
			break;
	}
}


void
UpdateDiagramGadgets(struct Window *win)
{
	struct winData *wd = (APTR)win->UserData;
	struct gDiagram *gd = wd->u.diagram.wd_CurrentDiagram;
	struct Gadget *gad;
	long   activePage = -1, i;

	if (!gd)
		return;

	if ((gad = wd->wd_PageHandlingGadget) != NULL)
		GetAttr(IGA_Active, (Object *)gad, (IPTR *)&activePage);

	/** Achsen-Gadgets (de-)aktivieren **/
	{
		BOOL disable;

		disable = !gIsSubclassFrom(gd->gd_Object.go_Class, FindGClass("axes"));

		if ((gad = PageGadget(wd->wd_Pages[4], 15)) && disable != (gad->Flags & GFLG_DISABLED ? TRUE : FALSE))
		{
			for (i = 14; i <= 22; i++)
				DisableGadget(PageGadget(wd->wd_Pages[4], i), activePage == 4 ? win : NULL, disable);
		}
	}
	
	/** Werte-Seite **/
	if ((gad = PageGadget(wd->wd_Pages[3], 10)) != 0)
	{
		GT_SetGadgetAttrs(gad, activePage == 3 ? win : NULL, NULL, GTLV_Labels, &gd->gd_Values, GA_Disabled, FALSE, TAG_END);
		for (i = 11; i < 14; i++)
			DisableGadget(PageGadget(wd->wd_Pages[3], i), activePage == 3 ? win : NULL, FALSE);
	}
	
	/** Diagrammtyp-Seite **/
	if ((gad = PageGadget(wd->wd_Pages[1], 8)) != 0)
	{
		GT_SetGadgetAttrs(gad, activePage == 1 ? win : NULL, NULL, GTLV_Selected,
			FindListEntry((struct MinList *)&gdiagrams, (struct MinNode *)gd->gd_Object.go_Class), TAG_END);
	}
		
	/** Darstellung-Seite **/
	UpdateGInterface(win, wd->u.diagram.wd_Gadgets, (struct gObject *)gd, activePage);
}


void
RefreshDiagram(struct gDiagram *gd)
{
	struct Window *pwin = GetAppWindow(WDT_PREVIEW);

	if (pwin)
	{
		RefreshPreviewSize(pwin);
		RefreshAppWindow(pwin, (struct winData *)pwin->UserData);
	}
	else
	{
		RefreshGObjectReferences((struct gObject *)gd);
/*		  struct gObject *go;
		struct Link *l;

		foreach (&gd->gd_Object.go_ReferencedBy, l)
			DrawSingleGObject((go = l->l_Link)->go_Page, go);*/
	}
}


/*!	Sets the attributes of a diagram, and makes the changes of the diagram
	visible on either the preview window or directly the page.
	Additionally, all gadgets in the diagram window are updated to the current
	values.

	@param win the diagram window (WDT_DIAGRAM)
	@param gd the diagram
	@param ti the attributes to change
*/
void
SetDiagramAttrsA(struct Window *win, struct gDiagram *gd, struct TagItem *ti)
{
	struct winData *wd = (struct winData *)win->UserData;
	ULONG rc;

	if (!win || !gd)
		return;

	if (wd->u.diagram.wd_OldDiagram != NULL) {
		// If we are only altering an existing diagram, we direct the changes
		// to the standard call which will generate an UndoNode for us
		SetGObjectAttrsA(gd->gd_Object.go_Page, (struct gObject *)gd, ti);
		return;
	}

	if ((rc = gDoMethod(gd, GCM_SET, ti)) && rc & GCPR_REDRAW) {
		RefreshGObjectBounds(wd->wd_Data, (struct gObject *)gd);
		RefreshDiagram(gd);
	}
	UpdateDiagramGadgets(win);
}


#ifdef __amigaos4__
void SetDiagramAttrs(struct Window *win, struct gDiagram *gd,...) VARARGS68K;
void SetDiagramAttrs(struct Window *win, struct gDiagram *gd,...)
#else
void SetDiagramAttrs(struct Window *win, struct gDiagram *gd, ULONG tag1,...)
#endif
{
#ifdef __amigaos4__
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, gd);
	tags = va_getlinearva(ap, struct TagItem *);
	return SetDiagramAttrsA(win, gd, tags);
#else
	SetDiagramAttrsA(win, gd, (struct TagItem *)&tag1);
#endif
}


bool
FillDiagramTransPoint(struct Window *win, struct point3d *p)
{
	struct Gadget *pageGadget = ((struct winData *)win->UserData)->wd_Pages[3];
	struct Gadget *gad;
	STRPTR t;

	if ((gad = PageGadget(pageGadget, 13)) && GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END))
	{
		p->x = ConvertNumber(t, CNT_CM);
		if ((gad = PageGadget(pageGadget, 14)) && GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END))
		{
			p->y = ConvertNumber(t, CNT_CM);
			if ((gad = PageGadget(pageGadget, 15)) && GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END))
			{
				p->z = ConvertNumber(t, CNT_CM);
				return true;
			}
		}
	}
	return false;
}


void
UpdateObjectReferences(struct gObject *oldgo, struct gObject *newgo)
{
	struct gObject *rgo;
	struct Link *l, *nl;

	for (l = (APTR)oldgo->go_ReferencedBy.mlh_Head; (nl = (APTR)l->l_Node.mln_Succ) != 0; l = nl)
	{
		rgo = l->l_Link;
		gSetObjectAttrs(rgo->go_Page, rgo, GEA_References, newgo, TAG_END);
	}
}


void
UpdateDiagramTypePage(struct Window *win, struct winData *wd, struct gDiagram *gd)
{
	struct Gadget *pagegad = wd->u.diagram.wd_PageGadget;

	SetGadgetAttrs(pagegad, win, NULL, PAGEGA_BeginUpdatePage, 2, TAG_END);

	RemoveDiagramGadgets(wd);
	if (wd->wd_Pages[2] == NULL)
		gad = CreateContext(&wd->wd_Pages[2]);
	AddDiagramGadgets(gd, wd->wd_Pages[2], wd->u.diagram.wd_Gadgets, win->Width, win->Height);

	SetGadgetAttrs(pagegad, win, NULL, PAGEGA_EndUpdatePage, 2, TAG_END);
}

 
void
SetDiagramType(struct Window *win, struct winData *wd, struct gClass *gc)
{
	struct gDiagram *gd, *lastgd, *oldgd;
	struct UndoNode *un = NULL;
	struct Page *page;
	struct Gadget *gad;
	ULONG  storage;

	if (!win || !wd || !gc || !(gc->gc_Node.in_Type & GCT_INTERNAL) && !gc->gc_Segment && !LoadGClass(gc))
		return;

	if ((lastgd = wd->u.diagram.wd_CurrentDiagram) != 0)
	{
		if (gc == lastgd->gd_Object.go_Class)
			return;
	}

	oldgd = wd->u.diagram.wd_OldDiagram;

	if (!((gad = PageGadget(wd->wd_Pages[0], 5))
		&& GT_GetGadgetAttrs(gad, win, NULL, GTTX_Text, &storage, TAG_END)
		&& (page = (struct Page *)MyFindName(&((struct Page *)wd->wd_Data)->pg_Mappe->mp_Pages, (STRPTR)storage))))
		page = (struct Page *)wd->wd_Data;

	if ((gd = (struct gDiagram *)gDoClassMethod(gc, gc, GCM_NEW, NULL, page)) != 0)
	{
		ULONG  tags[] = {GOA_Name,	0,
						 GDA_DataPage,0,
						 GDA_Range,   0,
						 GDA_ReadData,0,
/*					 	 GDA_RotX,	0,
						 GDA_RotY,	0,
						 GDA_RotZ,	0,
						 GDA_Trans,   NULL,*/
						 TAG_END};

		wd->u.diagram.wd_CurrentDiagram = gd;
		
		/* update object's window pointer */
		gd->gd_Object.go_Window = win;
		if (lastgd)
			lastgd->gd_Object.go_Window = NULL;

		if ((gad = PageGadget(wd->wd_Pages[0], 1)) && GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &storage, TAG_END))
			tags[1] = storage;	// GOA_Name
		if ((gad = PageGadget(wd->wd_Pages[0], 5)) != 0)
			tags[3] = (ULONG)gad->UserData;
		if ((gad = PageGadget(wd->wd_Pages[0], 2)) && GT_GetGadgetAttrs(gad,win,NULL,GTST_String,&storage,TAG_END))
			tags[5] = storage;	// GDA_Range
		if ((gad = PageGadget(wd->wd_Pages[0], 4)) && GT_GetGadgetAttrs(gad,win,NULL,GTCY_Active,&storage,TAG_END))
			tags[7] = storage;	// GDA_ReadData
/*	if ((gad = PageGadget(diagramPages[2],18)) && GT_GetGadgetAttrs(gad,win,NULL,GTSL_Level,&storage,TAG_END))
			tags[9] = storage;
		if ((gad = PageGadget(diagramPages[2],19)) && GT_GetGadgetAttrs(gad,win,NULL,GTSL_Level,&storage,TAG_END))
			tags[11] = storage;
		if ((gad = PageGadget(diagramPages[2],20)) && GT_GetGadgetAttrs(gad,win,NULL,GTSL_Level,&storage,TAG_END))
			tags[13] = storage;*/
		/*if (FillDiagramTransPoint(win,&trans))
			tags[15] = (ULONG)&trans;*/

		RefreshGObjectBounds(page, (struct gObject *)gd);
		{
			struct Window *pwin;

			if ((pwin = GetAppWindow(WDT_PREVIEW)) != 0)
			{
				((struct winData *)pwin->UserData)->wd_ExtData[0] = gd;
				RefreshPreviewSize(pwin);
			}
		}
		/* add the class interface to the correct page */
		UpdateDiagramTypePage(win, wd, gd);

		/* we can't simply use SetDiagramAttrsA() here, because that may create an undo node;
		 * wd_OldDiagram has to be NULL to prevent this
		 */
		wd->u.diagram.wd_OldDiagram = NULL;
		SetDiagramAttrsA(win, gd, (struct TagItem *)tags);
		wd->u.diagram.wd_OldDiagram = oldgd;

		if (oldgd)
		{
			/* we only need an undo node if the diagram is not newly created  */
			if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo,MSG_DIAGRAM_TYPE_UNDO))) != NULL)
			{
				un->un_UndoDiagram = lastgd;
				un->un_RedoDiagram = gd;
				un->un_Type = UNT_DIAGRAM_TYPE;
			}

			MyAddTail(&gd->gd_Object.go_Page->pg_gDiagrams, gd);
		}
	}

	if (lastgd)
	{
		UpdateObjectReferences((struct gObject *)lastgd, (struct gObject *)gd);

		if (oldgd)
			MyRemove(lastgd);

		if (!un)
			FreeGObject((struct gObject *)lastgd);
	}
}


void ASM
CloseDiagramWindow(REG(a0, struct Window *win), REG(d0, BOOL clean))
{
	struct winData *wd = (struct winData *)win->UserData;
	struct gDiagram *gd = wd->u.diagram.wd_CurrentDiagram;
	struct gGadget *gg;

	if (gd != NULL)
		gd->gd_Object.go_Window = NULL;

	while ((gg = (APTR)MyRemHead(wd->u.diagram.wd_Gadgets)) != 0)
		FreePooled(pool, gg, sizeof(struct gGadget));

	if (clean)
	{
		struct gDiagram *gd;

		FreePooled(pool, wd->u.diagram.wd_Gadgets, sizeof(struct MinList));

		if (!ende)  // damit da nichts durcheinandergeht
			CloseAppWindow(GetAppWindow(WDT_PREVIEW), TRUE);

		if ((gd = wd->u.diagram.wd_CurrentDiagram) != 0)
		{
			// "Ok" will set wd_CurrentDiagram to NULL, so we can safely free
			// it, when it's still there here (if you are editing a diagram,
			// wd_CurrentDiagram will always point to the latest diagram)
			if (wd->u.diagram.wd_OldDiagram == NULL)
				FreeGObject((struct gObject *)gd);
		}
	}
}


void ASM
HandleDiagramIDCMP(REG(a0, struct TagItem *tag))
{
	struct gDiagram *gd = wd->u.diagram.wd_CurrentDiagram;
	struct Page *page = wd->wd_Data;
	long   i;

	/* die Scheißtheorie: danach fröstelts einen! (A.B., 10.4.97) */

	switch (imsg.Class)
	{
		case IDCMP_GADGETDOWN:
			switch ((gad = imsg.IAddress)->GadgetID)
			{
			/****** Inhalt ******/
				case 6:
				{
					struct Mappe *mp = page->pg_Mappe;

					i = PopUpList(win, gad = GadgetAddress(win, 5), &mp->mp_Pages, TAG_END);
					if (i != ~0L)
					{
						for (page = (struct Page *)mp->mp_Pages.mlh_Head; i && page->pg_Node.ln_Succ; page = (APTR)page->pg_Node.ln_Succ, i--);
						SetDiagramAttrs(win, gd, GDA_DataPage, page, TAG_END);
						GT_SetGadgetAttrs(gad, win, NULL, GTTX_Text, page->pg_Node.ln_Name, TAG_END);
						gad->UserData = page;
					}
					break;
				}
			/****** Werte ******/
				case 11:
				{
					struct gLink *gl;
					struct Gadget *lvgad = GadgetAddress(win, 10);

					if (!gd || !lvgad || !(gl = lvgad->UserData))
						break;

					if ((i = PopColors(win,gad)) != ~0L)
					{
						gad->UserData = (APTR)i;
						GT_SetGadgetAttrs(lvgad, win, NULL, GTLV_Labels, ~0L, TAG_END);

						if (gDoMethod(gd, GCDM_SETLINKATTR, gl, i, ~0L))
							RefreshDiagram(gd);

						DrawColorField(win->RPort, gad, gl->gl_Color, TRUE);
						GT_SetGadgetAttrs(lvgad, win, NULL, GTLV_Labels, &gd->gd_Values, TAG_END);
					}
					break;
				}
			/****** Achsen & Objekt-Attribute ******/
				default:
					HandleGGadget(page, (struct gObject *)gd);
			}
			break;
#ifdef __amigaos4__
		case IDCMP_VANILLAKEY:
			if(!(gad = GadgetAddressfromSC(win, wd, imsg.Code)))
				break;
        case IDCMP_GADGETUP:
            if(imsg.Class == IDCMP_GADGETUP)
				gad = imsg.IAddress;
			switch (gad->GadgetID)
#else
        case IDCMP_GADGETUP:
			switch ((gad = imsg.IAddress)->GadgetID)
#endif
			{
			/****** Inhalt ******/
/*				  case 1:
				{
					STRPTR t;

					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
					SetDiagramAttrs(win, gd, GOA_Name, t, TAG_END);
					break;
				}
				case 2:
				{
					STRPTR t;

					GT_GetGadgetAttrs(gad, win, NULL, GTST_String, &t, TAG_END);
					SetDiagramAttrs(win, gd, GDA_Range, t, TAG_END);
					break;
				}*/
				case 3:
					if (rxpage->pg_MarkCol != -1)
					{
						char   t[32];

						TablePos2String(rxpage, (struct tablePos *)&rxpage->pg_MarkCol, t);
						GT_SetGadgetAttrs(PageGadget(wd->wd_Pages[0], 2), win, NULL, GTST_String, t, TAG_END);
						GT_SetGadgetAttrs(PageGadget(wd->wd_Pages[0], 5), win, NULL, GTTX_Text, rxpage->pg_Node.ln_Name, TAG_END);

						SetDiagramAttrs(win, gd, GDA_DataPage, rxpage, GDA_Range, t, TAG_END);
					}
					else
						ErrorRequest(GetString(&gLocaleInfo,MSG_NO_BLOCK_ERR));
					break;
/*				  case 4:
					SetDiagramAttrs(win, gd, GDA_ReadData, imsg.Code, TAG_END);
					break;*/
			/****** Diagrammtyp ******/
				case 8:
				{
					struct MinNode *mln;

					for (mln = gdiagrams.mlh_Head, i = 0; i < imsg.Code; mln = mln->mln_Succ, i++);
					SetDiagramType(win, wd, (struct gClass *)mln);
					break;
				}
			/****** Darstellung ******/
			/****** Werte ******/
				case 10:
					if (gd)
					{
						struct gLink *gl;
						long   marking = 0;

						//GT_SetGadgetAttrs(gad,win,NULL,GTLV_Labels,&gd->gd_Values,TAG_END);
						for(gl = (APTR)gd->gd_Values.mlh_Head,i = 0;i < imsg.Code;gl = (APTR)gl->gl_Node.mln_Succ,i++);
						gad->UserData = gl;
						GT_SetGadgetAttrs(GadgetAddress(win,12),win,NULL,GTTX_Text,gl->gl_Cell ? gl->gl_Cell->tf_Text : NULL,TAG_END);

						if (gDoMethod(gd,GCM_GET,GDA_SupportsMarking,&marking) && marking)
							GT_SetGadgetAttrs(GadgetAddress(win,13),win,NULL,GTCB_Checked,gl->gl_Flags & GLF_MARKED,GA_Disabled,FALSE,TAG_END);
						else
							DisableGadget(GadgetAddress(win,13),win,TRUE);

						DrawColorField(win->RPort,GadgetAddress(win,11),gl->gl_Color,TRUE);
					}
					break;
				case 13:
				{
					struct gLink *gl;

					if ((gad = GadgetAddress(win, 10)) && (gl = gad->UserData))
					{
						GT_SetGadgetAttrs(gad, win, NULL, GTLV_Labels, ~0L,TAG_END);
						gl->gl_Flags = (gl->gl_Flags & ~GLF_MARKED) | (imsg.Code ? GLF_MARKED : 0);
						GT_SetGadgetAttrs(gad, win, NULL, GTLV_Labels, &gd->gd_Values, TAG_END);

						RefreshDiagram(gd);
					}
					break;
				}
			/****** Achsen ******/
				// nada
			/****** Ok, Vorschau & Abbrechen (nur für neue Diagramme) ******/
				case 33:
				{
					// don't let CloseDiagramWindow() free this object
					if (gd)
						gd->gd_Object.go_Window = NULL;
					wd->u.diagram.wd_CurrentDiagram = NULL;
					CloseAppWindow(win, TRUE);

					if (gd)
					{
						struct gClass *gc = FindGClass("embed_diagram");

						gMakeRefObject(page, gc, (struct gObject *)gd, GetString(&gLocaleInfo, MSG_CREATE_DIAGRAM_OBJ));
						MyAddTail(&page->pg_gDiagrams, gd);
					}
					break;
				}
				case 34:
					OpenAppWindow(WDT_PREVIEW, WA_ExtData, gd, TAG_END);
					break;
				case 35:
					CloseAppWindow(win, TRUE);
					break;
			/****** Inhalt, Achsen & Objekt-Attribute ******/
				default:
					HandleGGadget(page, (struct gObject *)gd);
			}
			break;
		case IDCMP_CLOSEWINDOW:
			CloseAppWindow(win, TRUE);
			break;
	}
}
