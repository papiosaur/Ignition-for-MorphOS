/* Several graphics base classes
 *
 * Copyright 1996-2007 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "classes.h"

#include <graphics/scale.h>

#ifdef __amigaos4__
	#include <proto/elf.h>
#endif

#define G_POINTS 8
#define GRLOAD_BUFFERSIZE 2048

#ifdef DEBUG
#	define GLOAD(x) ;
#else
#	define GLOAD(x) ;
#endif


struct MinList gclasses, gdiagrams, intclasses;

extern struct RastPort *doublerp;

extern struct { UWORD jsr; APTR func; } gClassFuncTable[]; 	
extern int32 gClassFuncTableSize;
	// these should be "const", but SAS/C seems to have problems with this (at compile time)...


/*************************************** class definitions ***************************************/

const STRPTR gcNames[] = {"embedded","embed_diagram","axes",   "picture",NULL};
const STRPTR gcSuper[] = {"root",	"embedded",	 "diagram","root"};
const UBYTE  gcTypes[] = {0,		 0,			  0,		GCT_OBJECT};
const STRPTR gcImages[] = {NULL,	 NULL,		   NULL,	 "icons/obj_pic.icon"};
const APTR   gcDispatch[] = {gEmbeddedDispatch, gEmbedDiagramDispatch, gAxesDispatch, gPictureDispatch};
const APTR   gcDraw[] = {gEmbeddedDraw, gEmbedDiagramDraw, gAxesDraw, gPictureDraw};
struct gInterface *gcInterface[] = {gEmbeddedInterface, gEmbedDiagramInterface, gAxesInterface, gPictureInterface};
const ULONG  gcObjSize[] = {sizeof(struct gEmbedded), 0, sizeof(struct gAxes), sizeof(struct gPicture)};
STRPTR gcLabels[4];


/*************************************** rootclass ***************************************/


struct gInterface gRootInterface[] = {
	{GOA_Name, NULL, GIT_TEXT, NULL, "name"},
	{0}
};


ULONG
gRootSet(struct gObject *go,struct TagItem *tstate)
{
	struct TagItem *ti;

	if (tstate) {
		while ((ti = NextTagItem(&tstate)) != 0) {
			switch (ti->ti_Tag) {
				case GOA_Name:
					FreeString(go->go_Node.ln_Name);
					go->go_Node.ln_Name = AllocString((STRPTR)ti->ti_Data);
					break;
				case GOA_Help:
					FreeString(go->go_Help);
					go->go_Help = AllocString((STRPTR)ti->ti_Data);
					break;
				case GOA_Command:
					FreeString(go->go_Command);
					go->go_Command = AllocString((STRPTR)ti->ti_Data);
					break;
				case GOA_Page:
					go->go_Page = (struct Page *)ti->ti_Data;
					break;
				case GOA_ContinualCommand:
					if (ti->ti_Data)
						go->go_Flags |= GOF_CONTINUALCMD;
					else
						go->go_Flags &= ~GOF_CONTINUALCMD;
					break;
			}
		}
	}
	return GCPR_NONE;
}

#if (G_POINTS == 8)
void
gRootSetEdgeKnobs(struct point2d *p)
{
	p[4].x = (p[3].x+p[0].x)/2;  p[4].y = p[0].y;
	p[5].x = p[4].x;  p[5].y = p[3].y;
	p[6].x = p[0].x;  p[6].y = (p[3].y+p[0].y)/2;
	p[7].x = p[3].x;  p[7].y = p[6].y;
}
#endif

void
WriteTag(struct IFFHandle *iff,UBYTE cmd,ULONG tag)
{
	WriteChunkBytes(iff,&cmd,1);
	WriteChunkBytes(iff,&tag,4);
}


UBYTE *gio_types;
LONG gio_numtags;


static void
AddGObjectTag(struct TagItem **tags, long *tagpos, ULONG tag, UBYTE type, APTR data)
{
	if (*tagpos + 1 > gio_numtags) {
		struct TagItem *newtags;
		UBYTE  *newtypes;

		if ((newtags = AllocPooled(pool, (gio_numtags + 14) * sizeof(struct TagItem))) != 0) {
			if ((newtypes = AllocPooled(pool, (gio_numtags + 14))) != 0) {
				if (gio_types) {
					CopyMem(gio_types,newtypes,gio_numtags);
					FreePooled(pool,gio_types,gio_numtags);
				}
				gio_types = newtypes;
			}
			if (*tags) {
				CopyMem(*tags,newtags,gio_numtags * sizeof(struct TagItem));
				FreePooled(pool,*tags,gio_numtags * sizeof(struct TagItem));
			}
			*tags = newtags;
			gio_numtags += 14;
		} else
			return;
	}
	if (!*tags)
		return;

	(*tags)[*tagpos].ti_Tag = tag;
	(*tags)[*tagpos].ti_Data = (ULONG)data;
	gio_types[*tagpos] = type;
	(*tagpos)++;
}


void
FreeLoadedGObjectTags(struct TagItem *tags)
{
	if (!tags)
		return;

	if (gio_types) {
		int i;

		for (i = 0;i < gio_numtags;i++) {
			switch (gio_types[i]) {
				case GIT_FILENAME:
				case GIT_TEXT:
				case GIT_FORMULA:
					FreeString((STRPTR)tags[i].ti_Data);
					break;
			}
		}
		FreePooled(pool,gio_types,gio_numtags);
	}
	FreePooled(pool,tags,gio_numtags*sizeof(struct TagItem));
}


struct TagItem *
LoadGObjectTags(struct gcpIO *gcpio)
{
	struct IFFHandle *iff = gcpio->gcpio_IFFHandle;
	struct TagItem *tags = NULL;
	long i,tag,tagpos = 0;
	char *buf,type;
	STRPTR s;

	if (!(buf = AllocPooled(pool,GRLOAD_BUFFERSIZE)))
		return NULL;

	gio_numtags = 0;  gio_types = NULL;

	do {
		ReadChunkBytes(iff,&type,1);
		if (!type)
			break;
		if (ReadChunkBytes(iff,&tag,4) != 4)
			break;

		switch (type) {
			case GIT_FONT:
			{
				LONG  size,style;
				UWORD number;

				if (ReadChunkBytes(iff, &number, 2) == 2
					&& ReadChunkBytes(iff, &size, 4) == 4
					&& ReadChunkBytes(iff, &style, 4)) {
					struct Node *ln;

					if ((ln = FindListNumber(gcpio->gcpio_Fonts, number)) != 0) {
						struct FontInfo *fi;

						if ((fi = NewFontInfo(NULL, gDPI, FA_Family,	  ln->ln_Name,
														 FA_PointHeight, size,
														 FA_Style,	   style,
														 TAG_END)) != 0)
							AddGObjectTag(&tags, &tagpos, tag, type, fi);
						GLOAD(bug("  font: %s - size: %ld - style: %ld\n",((struct Node *)ln->ln_Name)->ln_Name,size >> 16,style));
					} else {
						GLOAD(bug("  font not found.\n"));
					}
				}
				break;
			}
			case GIT_FILENAME:
			case GIT_TEXT:
				if ((s = AllocString(ReadChunkString(iff, buf, GRLOAD_BUFFERSIZE))) != 0)
					AddGObjectTag(&tags,&tagpos,tag,type,s);
				GLOAD(bug("  text: '%s'\n",s));
				break;
			case GIT_FORMULA:
			{
				if ((s = ReadChunkString(iff, buf, GRLOAD_BUFFERSIZE)) != 0) {
					long   oldflags = calcflags;
					struct Term *term = NULL;

					if (*s == '=')
					{
						term = CreateTree(gcpio->gcpio_Page, s);
					}
					else if (*s == '#')
						s = AllocString(s + 1);
					else
						s = AllocString(s);

					calcflags = calcflags & ~(CF_REQUESTER | CF_SHORTFUNCS);

					AddGObjectTag(&tags,&tagpos,tag,type,term ? TreeTerm(term,TRUE) : s);
					DeleteTree(term);
					calcflags = oldflags;
				}
				GLOAD(bug("  formula: '%s'\n",s));
				break;
			}
			case GIT_CYCLE:
			case GIT_CHECKBOX:
			case GIT_WEIGHT:
			case GIT_BUTTON:
				if (ReadChunkBytes(iff,&i,4) == 4)
					AddGObjectTag(&tags,&tagpos,tag,type,(APTR)i);
				GLOAD(bug("  value: %ld\n",i));
				break;
			case GIT_PEN:
				i = 0;
				if (ReadChunkBytes(iff,&i,3) == 3) {
					i = (i >> 8) & 0xffffff;
					AddGObjectTag(&tags,&tagpos,tag,type,(APTR)i);
				}
				GLOAD(bug("  pen: %lx\n",i));
				break;
		}
	} while (tag);

	FreePooled(pool, buf, GRLOAD_BUFFERSIZE);

	if (tags)
		AddGObjectTag(&tags, &tagpos, TAG_END, 0, NULL);

	return tags;
}


struct gObject *
gRootLoad(struct gObject *go,struct gcpIO *gcpio)
{
	struct IFFHandle *iff = gcpio->gcpio_IFFHandle;
	struct TagItem *tags;
	struct gClass *gc;
	struct point2d *p;
	long   i,num,pos;
	STRPTR s,name;

	if (go)		 /* called as a member function */
		return NULL;

	GLOAD(bug("gRootLoad() - class function\n"));

	{
		char ibuf[128];

		if (!(s = ReadChunkString(iff,ibuf,128)))
			return NULL;

		if (!(gc = FindGClass(s)))
			return NULL;

		if (!(gc->gc_Node.in_Type & GCT_INTERNAL) && !gc->gc_Segment && !LoadGClass(gc))
			return NULL;

		if (!(name = AllocString(ReadChunkString(iff,ibuf,128))))
			return NULL;
	}
	ReadChunkBytes(iff,&pos,4);

	if (ReadChunkBytes(iff,&num,4) != 4 || !num || !(p = AllocPooled(pool,sizeof(struct point2d)*num)))
	{
		FreeString(name);
		return NULL;
	}
	ReadChunkBytes(iff,p,sizeof(struct point2d)*num);

	tags = LoadGObjectTags(gcpio);

	{
		struct point2d *sp;

		gDoClassMethod(gc,NULL,GCM_ENDPOINTS,p,i = num,&sp,&num);
		if (sp != p)
			FreePooled(pool,p,i*sizeof(struct point2d));
		if (sp && num && (go = (struct gObject *)gDoClassMethod(gc,gc,GCM_NEW,tags,gcpio->gcpio_Page)))
		{
			go->go_Node.ln_Name = name;
			go->go_Pos = pos;

			go->go_Knobs = sp;
			go->go_NumKnobs = num;

			RefreshGObjectBounds(gcpio->gcpio_Page,go);
		}
		FreeLoadedGObjectTags(tags);

		if (go)
		{
			if (gDoMethodA(go,(Msg)gcpio) < 0)
				ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_OBJECT_ERR),go->go_Node.ln_Name,gc->gc_Node.in_Name);
		}
		else if (sp)
			FreePooled(pool,sp,num*sizeof(struct point2d));
	}

	return go;
}


void
SaveGInterface(struct gcpIO *gcpio, struct gInterface *gi, struct gObject *go)
{
	struct IFFHandle *iff = gcpio->gcpio_IFFHandle;
	struct MinList *fonts = gcpio->gcpio_Fonts;
	long   tag,i;

	if (!gi)
		return;

	for (; (tag = gi->gi_Tag); gi++)
	{
		UBYTE c;

		switch (c = (UBYTE)gi->gi_Type)
		{
			case GIT_TEXT:
			case GIT_FILENAME:
			{
				STRPTR s;

				if (GetGObjectAttr(go,tag,(ULONG *)&s) && s)
				{
					WriteTag(iff,c,tag);
					WriteChunkString(iff,s);
				}
				break;
			}
			case GIT_FORMULA:
			{
				STRPTR s;

				if (GetGObjectAttr(go,tag,(ULONG *)&s) && s)
				{
					long   oldflags = calcflags;
					struct Term *term = NULL;

					WriteTag(iff, c, tag);

					if (*s == '=')
					{
						calcflags &= ~CF_SHORTFUNCS;
#ifdef __amigaos4__
						term = CreateTree(go->go_Page,s);		//graphic.h line 440 is gcpio_Page only valid for Load
#else
						term = CreateTree(gcpio->gcpio_Page,s);
#endif
						calcflags |= CF_SHORTFUNCS;
					}
					WriteChunkString(iff,term ? StaticTreeTerm(term,FALSE) : s);
					DeleteTree(term);

					calcflags = oldflags;
				}
				break;
			}
			case GIT_FONT:
			{
				struct NumberLink *nl = NULL;
				struct FontInfo *fi;

				if (GetGObjectAttr(go,tag,(ULONG *)&fi) && (nl = FindLink(fonts,fi->fi_Family)))
				{
					WriteTag(iff,c,tag);
					WriteChunkBytes(iff,&nl->nl_Number,2);
					WriteChunkBytes(iff,&fi->fi_FontSize->fs_PointHeight,4);
					WriteChunkBytes(iff,&fi->fi_Style,4);
				}
				break;
			}
			case GIT_PEN:
			{
				ULONG pen;

				if (GetGObjectAttr(go, tag, &pen))
				{
					WriteTag(iff, c, tag);
					WriteChunkBytes(iff, ((UBYTE *)&pen) + 1, 3);
				}
				break;
			}
			case GIT_CYCLE:
			case GIT_CHECKBOX:
			case GIT_WEIGHT:
				if (GetGObjectAttr(go,tag,(ULONG *)&i))
				{
					WriteTag(iff,c,tag);
					WriteChunkBytes(iff,&i,4);
				}
				break;
			case GIT_BUTTON:	/* werden nicht gespeichert */
			case GIT_NONE:
				break;
#ifdef DEBUG
			default:
				bug("%lx: kann Attribut nicht speichern: %lx (Typ: %ld)\n", go, gi->gi_Tag, c);
				break;
#endif
		}
	}
	i = 0;
	WriteChunkBytes(iff, &i, 1);
}


ULONG
gRootSave(struct gObject *go,struct gcpIO *gcpio)
{
	struct IFFHandle *iff = gcpio->gcpio_IFFHandle;

	WriteChunkString(iff, go->go_Class->gc_ClassName);
	WriteChunkString(iff, go->go_Node.ln_Name);
	WriteChunkBytes(iff, &go->go_Pos, 4);

	if (go->go_Knobs)
	{
		struct point2d *p;
		long   num;

		if (gDoMethod(go, GCM_REALPOINTS, &p, &num))
		{
			WriteChunkBytes(iff, &num,4);
			WriteChunkBytes(iff, p, num*sizeof(struct point2d));

			if (p != go->go_Knobs)
				FreePooled(pool,p,sizeof(struct point2d)*num);
		}
	}
	else
	{
		long i = 0;

		WriteChunkBytes(iff, &i, 4);
	}
	SaveGInterface(gcpio, go->go_Class->gc_Interface, go);

	return 0L;
}


ULONG PUBLIC
gRootDispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
	ULONG rc = 0L;

	switch (msg->MethodID)
	{
		case GCM_NEW:
		{
			struct gObject *ngo;

			if (((struct gClass *)go)->gc_Dispatch == (APTR)gRootDispatch)  // keine Instanzen dieser Klasse
				return 0;
			if (go && (ngo = AllocPooled(pool,((struct gClass *)go)->gc_InstSize)))
			{
				ngo->go_Class = (struct gClass *)go;
				ngo->go_Type = GOT_OBJECT;
				ngo->go_Page = ((struct gcpNew *)msg)->gcpn_Page;
				ngo->go_Pos = -1;
				MyNewList(&ngo->go_ReferencedBy);
				gRootSet(ngo,((struct gcpNew *)msg)->gcpn_AttrList);

				rc = (ULONG)ngo;
			}
			break;
		}
		case GCM_DISPOSE:
		{
			struct gObject *rgo;
			struct Link *l;

			CloseAppWindow(go->go_Window, TRUE);

			while ((l = (struct Link *)MyRemHead(&go->go_ReferencedBy)) != 0)
			{
				rgo = l->l_Link;
				SetGObjectAttrs(rgo->go_Page, rgo, GEA_References, NULL, TAG_END);
			}

			FreeString(go->go_Node.ln_Name);
			FreeString(go->go_Help);
			FreeString(go->go_Command);
			FreePooled(pool,go,go->go_Class->gc_InstSize);
			break;
		}
		case GCM_COPY:
		{
			struct gObject *cgo;

			if (go && (cgo = AllocPooled(pool,go->go_Class->gc_InstSize)))
			{
				CopyMem(go,cgo,go->go_Class->gc_InstSize);
				cgo->go_Node.ln_Name = AllocString(go->go_Node.ln_Name);
				cgo->go_Help = AllocString(go->go_Help);
				cgo->go_Command = AllocString(go->go_Command);
				MyNewList(&cgo->go_ReferencedBy);

				if ((cgo->go_Knobs = AllocPooled(pool, go->go_NumKnobs * sizeof(struct point2d))) != 0)
					CopyMem(go->go_Knobs,cgo->go_Knobs,sizeof(struct point2d)*go->go_NumKnobs);
				cgo->go_Pos = -1;
				cgo->go_Page = NULL;
				cgo->go_Window = NULL;

				rc = (ULONG)cgo;
			}
			break;
		}
		case GCM_SET:
			rc = gRootSet(go,((struct gcpSet *)msg)->gcps_AttrList);
			break;
		case GCM_GET:
			rc = TRUE;
			switch(((struct gcpGet *)msg)->gcpg_Tag)
			{
				case GOA_Name:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)go->go_Node.ln_Name;
					break;
				case GOA_Command:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)go->go_Command;
					break;
				case GOA_Help:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)go->go_Help;
					break;
				case GOA_ContinualCommand:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)(go->go_Flags & GOF_CONTINUALCMD);
					break;
				default:
					rc = FALSE;
			}
			break;
		case GCM_OPENWINDOW:
			return (ULONG)OpenAppWindow(WDT_OBJECT, WA_Data, go, TAG_END);
			break;
		case GCM_CLOSEWINDOW:
			CloseAppWindow(go->go_Window, TRUE);
			break;
		case GCM_REALPOINTS:
		{
			struct point2d *p;
			UWORD  num = 2;

			if ((p = AllocPooled(pool, sizeof(struct point2d) * num)) != 0)
			{
				p[0] = go->go_Knobs[0];
				p[1] = go->go_Knobs[3];

				*((struct gcpRealPoints *)msg)->gcpr_StoragePoints = p;
				*((struct gcpRealPoints *)msg)->gcpr_StorageNumPoints = num;

				rc = TRUE;
			}
			break;
		}
		case GCM_BEGINPOINTS:
			rc = 2;
			break;
		case GCM_ENDPOINTS:
		{
			struct point2d *s = ((struct gcpEndPoints *)msg)->gcpe_Points,*t;
			UWORD  num = ((struct gcpEndPoints *)msg)->gcpe_NumPoints;

#if (G_POINTS == 4)
			if (t = AllocPooled(pool,sizeof(struct point2d)*4))
			{
				t[0] = s[0];
				t[1].x = s[1].x;  t[1].y = s[0].y;
				t[2].x = s[0].x;  t[2].y = s[1].y;
				t[3] = s[1];
				num = 4;
			}
#elif (G_POINTS == 8)
			if ((t = AllocPooled(pool, sizeof(struct point2d) * 8)) != 0)
			{
				t[0] = s[0];
				t[1].x = s[1].x;  t[1].y = s[0].y;
				t[2].x = s[0].x;  t[2].y = s[1].y;
				t[3] = s[1];
				gRootSetEdgeKnobs(t);
				num = 8;
			}
#endif
			*((struct gcpEndPoints *)msg)->gcpe_StoragePoints = t;
			*((struct gcpEndPoints *)msg)->gcpe_StorageNumPoints = num;
			break;
		}
		case GCM_UPDATEPOINT:
		{
			struct point2d *p = go->go_Knobs;
			LONG   num = ((struct gcpUpdatePoint *)msg)->gcpu_NumPoint;

			p[num].x = ((struct gcpUpdatePoint *)msg)->gcpu_Point.x;
			p[num].y = ((struct gcpUpdatePoint *)msg)->gcpu_Point.y;

			switch (num)
			{
				case 0:
					p[1].y = p[0].y;
					p[2].x = p[0].x;
					break;
				case 1:
					p[0].y = p[1].y;
					p[3].x = p[1].x;
					break;
				case 2:
					p[3].y = p[2].y;
					p[0].x = p[2].x;
					break;
				case 3:
					p[2].y = p[3].y;
					p[1].x = p[3].x;
					break;
#if (G_POINTS == 8)
				case 4:
					p[0].y = p[4].y;
					p[1].y = p[4].y;
					break;
				case 5:
					p[2].y = p[5].y;
					p[3].y = p[5].y;
					break;
				case 6:
					p[0].x = p[6].x;
					p[2].x = p[6].x;
					break;
				case 7:
					p[1].x = p[7].x;
					p[3].x = p[7].x;
					break;
#endif
			}
			if (p[0].x > p[1].x)
			{
				swmem((UBYTE *)&p[0].x, (UBYTE *)&p[1].x, sizeof(long));
				swmem((UBYTE *)&p[2].x, (UBYTE *)&p[3].x, sizeof(long));
			}
			if (p[0].y > p[2].y)
			{
				swmem((UBYTE *)&p[0].y, (UBYTE *)&p[2].y, sizeof(long));
				swmem((UBYTE *)&p[1].y, (UBYTE *)&p[3].y, sizeof(long));
			}
#if (G_POINTS == 8)
			gRootSetEdgeKnobs(p);
#endif
			break;
		}
		case GCM_ADDPOINT:
			/* we refresh the whole drawing, so we don't care about GCPAM_UPDATE/REDRAW */

			if (((struct gcpAddPoint *)msg)->gcpa_NumPoints == 2)
			{
				struct coord *p = ((struct gcpAddPoint *)msg)->gcpa_Points;
				WORD   ox = ((struct gcpAddPoint *)msg)->gcpa_Offset.x, oy = ((struct gcpAddPoint *)msg)->gcpa_Offset.y;

				DrawRect(((struct gcpAddPoint *)msg)->gcpa_RastPort,p[0].x+ox,p[0].y+oy,p[1].x-p[0].x,p[1].y-p[0].y);
			}
			break;
		case GCM_CHANGEPOINT:
		{
			struct coord *p = ((struct gcpAddPoint *)msg)->gcpa_Points;
			WORD   ox = ((struct gcpAddPoint *)msg)->gcpa_Offset.x,oy = ((struct gcpAddPoint *)msg)->gcpa_Offset.y;

			switch(((struct gcpAddPoint *)msg)->gcpa_Point)
			{
				case 1:
					p[0].y = p[1].y;
					p[3].x = p[1].x;
					break;
				case 2:
					p[3].y = p[2].y;
					p[0].x = p[2].x;
					break;
#if (G_POINTS == 8)
				case 4:
					p[0].y = p[4].y;
					break;
				case 5:
					p[3].y = p[5].y;
					break;
				case 6:
					p[0].x = p[6].x;
					break;
				case 7:
					p[3].x = p[7].x;
					break;
#endif
			}
			DrawRect(((struct gcpAddPoint *)msg)->gcpa_RastPort,p[0].x+ox,p[0].y+oy,p[3].x-p[0].x,p[3].y-p[0].y);
			break;
		}
		case GCM_BOX: 
		{
			struct point2d *p = go->go_Knobs;
			LONG   xmin,ymin,xmax,ymax,i;

			if (!p)
				break;

			xmin = xmax = p[0].x;
			ymin = ymax = p[0].y;
			for(i = 1;i < go->go_NumKnobs;i++)
			{
				if (p[i].x < xmin)
					xmin = p[i].x;
				else if (p[i].x > xmax)
					xmax = p[i].x;
				if (p[i].y < ymin)
					ymin = p[i].y;
				else if (p[i].y > ymax)
					ymax = p[i].y;
			}
			go->go_mmLeft = xmin;  go->go_mmTop = ymin;
			go->go_mmRight = xmax;  go->go_mmBottom = ymax;

			//gDoMethod(go,GCM_UPDATEPOINT,xmin,ymin,0);
			//gDoMethod(go,GCM_UPDATEPOINT,xmax,ymax,3);
			break;
		}
		case GCM_HITTEST:
			rc = TRUE;
			break;
		case GCM_COMMAND:
			/* Execute command */
			ProcessAppCmd(go->go_Page,go->go_Command);
			break;
		case GCM_SAVE:
			rc = gRootSave(go,(struct gcpIO *)msg);
			break;
		case GCM_LOAD:
			rc = (ULONG)gRootLoad(go,(struct gcpIO *)msg);
			break;
		case GCM_UPDATE_UI:
			if (go->go_Window)
				UpdateObjectGadgets(go->go_Window);
			break;
	}
	return rc;
}


/******************************* embedded object *****************************/


struct gInterface gEmbeddedInterface[] = {
	{GEA_References, NULL, GIT_BUTTON, NULL, NULL},
	{GEA_AdoptSize, NULL, GIT_BUTTON, NULL, NULL},
	{0}
};


void PUBLIC
gEmbeddedDraw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *rp), REG(a1, struct gClass *gc),
	REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{
	struct gEmbedded *ge = GINST_DATA(gc, go);
	struct gObject *rgo;
	long   x = gb->gb_Left,y = gb->gb_Top;
	long   width = gb->gb_Right-gb->gb_Left;
	long   height = gb->gb_Bottom-gb->gb_Top;

	/*SetHighColor(grp,page->pg_BPen);
	RectFill(grp,x,y,x+width,y+height);*/

	if ((rgo = ge->ge_References) != 0)
		rgo->go_Class->gc_Draw(page,dpi,grp,rgo->go_Class,rgo,gb);
	else
	{
		long w10 = width/10,h10 = height/10;
		long w4 = width/4,h4 = height/4;
		long w2 = width/2,h2 = height/2;

		SetAPen(grp,1);
		DrawRect(grp,x,y,width,height);

		SetHighColor(grp,0xff0000);

		gAreaMove(grp,x+w2-w4,y+h2+h4-h10);
		gAreaDraw(grp,x+w2+w4-w10,y+h2-h4);
		gAreaDraw(grp,x+w2+w4,y+h2-h4+h10);
		gAreaDraw(grp,x+w2-w4+w10,y+h2+h4);
		gAreaEnd(grp);

		gAreaMove(grp,x+w2-w4+w10,y+h2-h4);
		gAreaDraw(grp,x+w2+w4,y+h2+h4-h10);
		gAreaDraw(grp,x+w2+w4-w10,y+h2+h4);
		gAreaDraw(grp,x+w2-w4,y+h2-h4+h10);
		gAreaEnd(grp);

		//RectFill(grp,x+width/3,y+height/3,x+width/2,y+height/2);
	}
}


ULONG
gEmbeddedSet(struct gObject *go, struct gEmbedded *ge, struct TagItem *tstate)
{
	ULONG  rc = GCPR_NONE;
	struct TagItem *ti;

	if (tstate)
	{
		while ((ti = NextTagItem(&tstate)) != 0)
		{
			switch (ti->ti_Tag)
			{
				case GEA_References:
				{
					struct gObject *rgo,*posgo;
					struct point2d *p;

					if (ti->ti_Data == ~0L) // Button pressed
					{
						if (ge->ge_References)
							gDoMethod(ge->ge_References, GCM_OPENWINDOW);
						break;
					}
					posgo = rgo = (struct gObject *)ti->ti_Data;

					/* if data is an embedded object, data's referenced is used */

					if (rgo && rgo->go_Class == go->go_Class)
						rgo = ((struct gEmbedded *)GINST_DATA(go->go_Class, rgo))->ge_References;

					if (!go->go_Page || ge->ge_References == rgo)
						break;
					if (rgo && rgo->go_Type != GOT_OBJECT)
					{
						ErrorRequest(GetString(&gLocaleInfo, MSG_REFERENCE_TO_GROUP_ERR));
						break;
					}

					if (ge->ge_References)
						MyRemove(&ge->ge_Link);
					if (rgo)
						MyAddTail(&rgo->go_ReferencedBy, &ge->ge_Link);

					ge->ge_References = rgo;

					if (rgo && go->go_mmLeft == go->go_mmRight && (p = AllocPooled(pool,2*sizeof(struct point2d))))
					{
						struct point2d *sp;
						long   num;

						p[0].x = posgo->go_mmLeft+5*1024;
						p[0].y = posgo->go_mmTop+5*1024;
						p[1].x = posgo->go_mmRight+5*1024;
						p[1].y = posgo->go_mmBottom+5*1024;

						gDoClassMethod(go->go_Class,NULL,GCM_ENDPOINTS,p,2,&sp,&num);

						if (sp != p)
							FreePooled(pool,p,2*sizeof(struct point2d));

						if (go->go_Knobs)			/* alte Knobs entfernen */
							FreePooled(pool,go->go_Knobs,go->go_NumKnobs*sizeof(struct point2d));
						go->go_Knobs = sp;		   /* neue Knobs setzen */
						go->go_NumKnobs = num;
					}
					rc |= GCPR_REDRAW;
					break;
				}
				case GEA_AdoptSize:
				{
					struct gObject *rgo = ge->ge_References;

					if (!rgo)
						break;
					if (ti->ti_Data == ~0L)
						SizeGObject(go->go_Page,go,rgo->go_mmRight - rgo->go_mmLeft,rgo->go_mmBottom - rgo->go_mmTop);
					rc |= GCPR_UPDATESIZE;
					break;
				}
			}
		}
	}
	return rc;
}


ULONG PUBLIC
gEmbeddedDispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
	struct gEmbedded *ge = GINST_DATA(gc, go);
	ULONG  rc;
	switch (msg->MethodID)
	{
		case GCM_NEW:
			if ((rc = gDoSuperMethodA(gc, go, msg)) != 0)
			{
				go = (struct gObject *)rc;  ge = GINST_DATA(gc, go);
				go->go_Flags |= GOF_FRAMED;
				ge->ge_Link.l_Link = go;
				ge->ge_Type = GET_LINK;

				gEmbeddedSet(go, ge, ((struct gcpSet *)msg)->gcps_AttrList);
			}
			break;
		case GCM_COPY:
		{
			struct gObject *cgo;

			if ((cgo = (struct gObject *)(rc = gDoSuperMethodA(gc, go, msg))) != 0)
			{
				struct gEmbedded *cge = GINST_DATA(gc, cgo);

				cge->ge_Link.l_Link = cgo;
				if (cge->ge_References)
					MyAddTail(&cge->ge_References->go_ReferencedBy, &cge->ge_Link);
			}
			break;
		}
		case GCM_SET:
			if (!(rc = gDoSuperMethodA(gc, go, msg)))
				return gEmbeddedSet(go, ge, ((struct gcpSet *)msg)->gcps_AttrList);
			break;
		case GCM_GET:
			switch(((struct gcpGet *)msg)->gcpg_Tag)
			{
				case GEA_References:
					D(bug("get GEA_REFERENCES **************************************************************************************\n"));
					//*((struct gcpGet *)msg)->gcpg_Storage = ge->ge_References ? ge->ge_References->go_Pos : -1;
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)ge->ge_References;
					return TRUE;
				case GEA_AdoptSize:
					*((struct gcpGet *)msg)->gcpg_Storage = 0L;
					return TRUE;
				default:
					return gDoSuperMethodA(gc, go, msg);
			}
			break;
		case GCM_INITAFTERLOAD:
		{
			struct gObject *rgo;
			struct Page *page;

			rc = gDoSuperMethodA(gc,go,msg);

			if (ge->ge_Type == GET_DIAGRAM || !go->go_Page || !(page = (struct Page *)FindListNumber(&go->go_Page->pg_Mappe->mp_Pages,ge->ge_PageNumber)))
				break;

			foreach(&page->pg_gObjects,rgo)
			{
				if (rgo->go_Pos == ge->ge_Pos)
				{
					ULONG tags[3] = {GEA_References, 0, TAG_END};

					tags[1] = (ULONG)rgo;
					gDoMethod(go, GCM_SET, tags);
					break;
				}
			}
			break;
		}
		case GCM_LOAD:
			if ((rc = gDoSuperMethodA(gc,go,msg)) < 0)
				break;

			ReadChunkBytes(((struct gcpIO *)msg)->gcpio_IFFHandle, &ge->ge_PageNumber, 4);
			ReadChunkBytes(((struct gcpIO *)msg)->gcpio_IFFHandle, &ge->ge_Pos, 4);
			break;
		case GCM_SAVE:
		{
			struct IFFHandle *iff = ((struct gcpIO *)msg)->gcpio_IFFHandle;
			struct gObject *rgo;

			if ((rc = gDoSuperMethodA(gc, go, msg)) < 0)
				break;

			if ((rgo = ge->ge_References) && rgo->go_Page)
			{
				ULONG p = FindListEntry(&rgo->go_Page->pg_Mappe->mp_Pages, (struct MinNode *)rgo->go_Page);

				WriteChunkBytes(iff,&p,4);
				if (ge->ge_Type == GET_DIAGRAM)
					p = FindListEntry(&rgo->go_Page->pg_gDiagrams, (struct MinNode *)rgo);
				else
					p = ge->ge_References->go_Pos;

					WriteChunkBytes(iff, &p, 4);
			}
			else
			{
				ULONG n = ~0L;

				WriteChunkBytes(iff, &n, 4);
				WriteChunkBytes(iff, &n, 4);
			}
			break;
		}
		default:
			return gDoSuperMethodA(gc, go, msg);
	}
	return rc;
}


/*************************************** picture-class ***************************************/
		
 
struct gInterface gPictureInterface[] = {
	{GPA_FileName, NULL, GIT_FILENAME, NULL, "filename"},
	{GPA_RealSize, NULL, GIT_BUTTON, NULL, NULL},
	{GPA_KeepAspectRatio, NULL, GIT_CHECKBOX, NULL, "keepaspectratio"},
	{GPA_Center,   NULL, GIT_CHECKBOX, NULL, "center"},
	{0}
};


void
gPictureNewImage(struct gPicture *gp)
{
	FreeImage(gp->gp_Image);

	//if (gp->gp_Image = NewObject(pictureiclass,NULL,DTA_Name,gp->gp_FileName/*,PIA_DelayLoad,TRUE*/,TAG_END))
	if ((gp->gp_Image = LoadImage(gp->gp_FileName)) != 0)
	{
		//AddImageObj(gp->gp_FileName,gp->gp_Image);
		SetAttrs(gp->gp_Image, PDTA_Screen, scr, TAG_END);

		gp->gp_Width = gp->gp_Image->Width;
		gp->gp_Height = gp->gp_Image->Height;
	}
}


ULONG
gPictureSet(struct gObject *go,struct gPicture *gp,struct TagItem *tstate)
{
	struct Page *page = go->go_Page;
	struct TagItem *ti;
	ULONG  rc = GCPR_NONE;

	if (tstate)
	{
		while ((ti = NextTagItem(&tstate)) != 0)
		{
			switch(ti->ti_Tag)
			{
				case GPA_FileName:
					if (!strcmp(gp->gp_Name ? gp->gp_Name : (STRPTR)"",ti->ti_Data ? (char *)ti->ti_Data : ""))
						break;

					FreeString(gp->gp_Name);
					FreeString(gp->gp_FileName);
					DeleteTerm(gp->gp_Term);

					gp->gp_Name = AllocString((STRPTR)ti->ti_Data);
					gp->gp_Term = CreateTerm(page,gp->gp_Name);
					gp->gp_FileName = CalcTerm(page,gp->gp_Name,gp->gp_Term,NULL);

					gPictureNewImage(gp);
					rc = GCPR_UPDATESIZE;
					break;
				case GPA_RealSize:
				{
					if (!gp->gp_Image)
						break;
					if (ti->ti_Data == ~0L)
						SizeGObject(page,go,mm(page,gp->gp_Width,TRUE),mm(page,gp->gp_Height,FALSE));

					rc |= GCPR_UPDATESIZE;
					break;
				}
				case GPA_KeepAspectRatio:
				{
					if ((bool)ti->ti_Data == gp->gp_KeepAspectRatio)
						break;

					gp->gp_KeepAspectRatio = (bool)ti->ti_Data;
					rc |= GCPR_REDRAW;
					break;
				}
				case GPA_Center:
				{
					if ((bool)ti->ti_Data == gp->gp_Center)
						break;

					gp->gp_Center = (bool)ti->ti_Data;
					rc |= GCPR_REDRAW;
					break;
				}
			}
		}
	}
	return rc;
}


void PUBLIC
gPictureDraw(REG(d0, struct Page *page),REG(d1, ULONG dpi),REG(a0, struct RastPort *rp),REG(a1, struct gClass *gc),REG(a2,struct gObject *go),REG(a3, struct gBounds *gb))
{
	struct gPicture *gp = GINST_DATA(gc,go);
	long x = gb->gb_Left,y = gb->gb_Top;
	long wid = gb->gb_Right - gb->gb_Left;
	long hei = gb->gb_Bottom - gb->gb_Top;

	if (gp->gp_Image)
	{
		struct BitMap *bm;

		if (GetAttr(PIA_BitMap, (Object *)gp->gp_Image, (IPTR *)&bm))
		{
			struct BitScaleArgs bsa;
			double factorX, factorY;
			long   a, xmin, ymin, oldx, oldy;

			memset(&bsa, 0, sizeof(struct BitScaleArgs));

			if (doublerp)
			{
				xmin = page->pg_wTabX;
				ymin = page->pg_wTabY;
			}
			else
			{
				xmin = page->pg_wTabX + (a = page->pg_Window->LeftEdge);
				x += a;
				ymin = page->pg_wTabY + (a = page->pg_Window->TopEdge);
				y += a;
			}
			oldx = x;  oldy = y;
			x = max(xmin, x);  y = max(ymin, y);
												
			bsa.bsa_XSrcFactor = gp->gp_Width;  bsa.bsa_YSrcFactor = gp->gp_Height;
			bsa.bsa_XDestFactor = wid;  bsa.bsa_YDestFactor = hei;
			bsa.bsa_DestX = x;  bsa.bsa_DestY = y;

			factorX = (1.0 * bsa.bsa_XSrcFactor / bsa.bsa_XDestFactor);
			factorY = (1.0 * bsa.bsa_YSrcFactor / bsa.bsa_YDestFactor);

			if (gp->gp_KeepAspectRatio)
			{
				if (factorX < factorY)
				{
					bsa.bsa_XDestFactor = bsa.bsa_XSrcFactor / factorY;
					if (gp->gp_Center)
						bsa.bsa_DestX += (wid - bsa.bsa_XDestFactor) >> 1;
				}
				else if (factorY < factorX)
				{
					bsa.bsa_YDestFactor = bsa.bsa_YSrcFactor / factorX;
					if (gp->gp_Center)
						bsa.bsa_DestY += (hei - bsa.bsa_YDestFactor) >> 1;
				}
			}

			bsa.bsa_SrcX = (long)((x - oldx) * factorX + 0.5);
			bsa.bsa_SrcY = (long)((y - oldy) * factorY + 0.5);
			a = xmin + page->pg_wTabW + 1 - x;  wid -= x - oldx;
			bsa.bsa_SrcWidth =  (long)(min(a, wid) * factorX + 0.5);
			a = ymin + page->pg_wTabH + 1 - y;  hei -= y - oldy;
			bsa.bsa_SrcHeight = (long)(min(a, hei) * factorY + 0.5);

			bsa.bsa_SrcBitMap = bm;
			bsa.bsa_DestBitMap = rp->BitMap;
			
			// ToDo: this only works with double buffering!
			BitMapScale(&bsa);
		}
	}
	else
	{
		SetHighColor(rp, 0x00ffffff);
		RectFill(rp, x, y, gb->gb_Right, gb->gb_Bottom);
	}
}


ULONG PUBLIC
gPictureDispatch(REG(a0, struct gClass *gc), REG(a2, struct gObject *go), REG(a1, Msg msg))
{
	struct gPicture *gp = GINST_DATA(gc, go);
	ULONG  rc;

	switch (msg->MethodID)
	{
		case GCM_NEW:
			if ((rc = gDoSuperMethodA(gc, go, msg)) != 0)
			{
				go = (struct gObject *)rc;  gp = GINST_DATA(gc, go);

				gPictureSet(go, gp, ((struct gcpSet *)msg)->gcps_AttrList);
			}
			break;
		case GCM_COPY:
		{
			struct gObject *cgo;

			if ((cgo = (struct gObject *)(rc = gDoSuperMethodA(gc, go, msg))) != 0)
			{
				struct gPicture *cgp = GINST_DATA(gc, cgo);

				cgp->gp_Name = AllocString(gp->gp_Name);
				cgp->gp_FileName = AllocString(gp->gp_FileName);
				cgp->gp_Term = CopyTerm(gp->gp_Term);
				cgp->gp_Image = LoadImage(gp->gp_FileName);
			}
			break;
		}
		case GCM_SET:
			return gDoSuperMethodA(gc, go, msg) | gPictureSet(go, gp, ((struct gcpSet *)msg)->gcps_AttrList);

		case GCM_GET:
			rc = TRUE;

			switch(((struct gcpGet *)msg)->gcpg_Tag)
			{
				case GPA_FileName:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gp->gp_Name;
					break;
				case GPA_KeepAspectRatio:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gp->gp_KeepAspectRatio;
					break;
				case GPA_Center:
					*((struct gcpGet *)msg)->gcpg_Storage = (ULONG)gp->gp_Center;
					break;
				default:
					rc = gDoSuperMethodA(gc, go, msg);
			}
			break;
		case GCM_RECALC:
		{
			STRPTR t = gp->gp_FileName;

			gp->gp_FileName = CalcTerm(go->go_Page, gp->gp_Name, gp->gp_Term, NULL);

			if (zstrcmp(gp->gp_FileName, t))
			{
				gPictureNewImage(gp);
				rc = GCPR_REDRAW;
			}
			FreeString(t);
			break;
		}

		case GCM_ADD_TO_SCREEN:
		{
			struct Screen *screen = ((struct gcpScreen *)msg)->gcps_Screen;
				
			if (gp->gp_Image != NULL)
				SetAttrs(gp->gp_Image, PDTA_Screen, screen, TAG_END);
		}
		case GCM_REMOVE_FROM_SCREEN:
			//bug("screen changed\n");
			break;

		default:
			return gDoSuperMethodA(gc, go, msg);
	}
	return rc;
}


/*************************************** general class functions ***************************************/


struct gClass *
FindGClass(STRPTR name)
{
	struct gClass *gc;

	foreach(&gclasses, gc)
	{
		if (!stricmp(name, gc->gc_ClassName))
			return gc;
	}
	foreach(&gdiagrams, gc)
	{
		if (!stricmp(name, gc->gc_ClassName))
			return gc;
	}
	foreach(&intclasses, gc)
	{
		if (!stricmp(name, gc->gc_ClassName))
			return gc;
	}
	return NULL;
}


void
FreeGClass(struct gClass *gc)
{
	FreeImage(gc->gc_Node.in_Image);

	if (gc->gc_FreeClass)
		gc->gc_FreeClass(gc);

	if (gc->gc_Segment)
		UnLoadSeg(gc->gc_Segment);

	FreePooled(pool, gc, sizeof(struct gClass));
}


struct gClass *
MakeGClass(STRPTR name, UBYTE type, struct gClass *sgc, STRPTR label,
	STRPTR image, APTR dispatch, APTR draw, struct gInterface *gi,
	ULONG objsize)
{
	struct gClass *gc;

	if (!(type & GCT_ROOT) && !sgc)
		return NULL;

	if ((gc = AllocPooled(pool, sizeof(struct gClass))) != NULL) {
		gc->gc_Node.in_Name = label;
		gc->gc_Node.in_Type = type;
		gc->gc_Node.in_Image = LoadImage(image);
		gc->gc_Super = sgc;
		gc->gc_Dispatch = dispatch;
		gc->gc_Draw = draw;		/* set special 3d-drawing routine */
		gc->gc_Interface = gi;
		gc->gc_InstOffset = sgc ? sgc->gc_InstSize : 0;
		if (objsize % 2) // mögliches Padding-Byte einschieben
			objsize++;
		gc->gc_InstSize = objsize+gc->gc_InstOffset;
		gc->gc_ClassName = name;
		if (sgc)
		{
			if (!dispatch)
				gc->gc_Dispatch = sgc->gc_Dispatch;
			if (!draw)
				gc->gc_Draw = sgc->gc_Draw;
			if (!gi)
				gc->gc_Interface = sgc->gc_Interface;
		}
		MyAddTail((type & GCT_OBJECT ? &gclasses : (type & GCT_DIAGRAM ? &gdiagrams : &intclasses)),gc);
		D(bug("add class: (%lx) %s [%s]\n",gc,name,(type & GCT_OBJECT ? "gclasses" : (type & GCT_DIAGRAM ? "gdiagrams" : "intclasses"))));
	}
	return gc;
}


#ifdef __amigaos4__
BOOL
LoadGClass(struct gClass *gc)
{
	BOOL ASM (*initGCSegment)(REG(a0, APTR), REG(a1, APTR *), REG(a2, APTR), REG(a3, APTR), REG(a6, APTR), 	REG(d0, APTR), REG(d1, APTR),REG(d2, APTR), REG(d3, APTR), REG(d4, long));
	BPTR dir, olddir; //, segment;
	Elf32_Handle elfhandle;
	Elf32_Handle filehandle;
	struct Elf32_SymbolQuery query;

	if (gc->gc_Segment || !gc->gc_ClassName)
		return TRUE;

	if ((dir = Lock(CLASSES_PATH, ACCESS_READ)) != 0) {
		olddir = SetCurrentDir(dir);
		if ((gc->gc_Segment = LoadSeg(gc->gc_ClassName)) != 0) {
			//Get ELF handler
			GetSegListInfoTags(gc->gc_Segment,GSLI_ElfHandle,&elfhandle,TAG_DONE);
	
			if (elfhandle != NULL) {//Find the ELF handler?
				//Reopen the ELF file
				if((filehandle = OpenElfTags(OET_ElfHandle,elfhandle,TAG_DONE))) {
					//Check version first
					query.Flags = ELF32_SQ_BYNAME;
					query.Name = "Version";
					if (SymbolQuery(filehandle, 1, &query) != 0) {
						if ((*(LONG*)query.Value) == 2) {//If the version is 2 then this plugin is a compatible GadgetClass
							//Let's query for the symbol name "Operator"
							query.Name = "InitGClass";
							if (SymbolQuery(filehandle, 1, &query) != 0) {
								//Close the opened ELF file
								CloseElfTags(filehandle, CET_CloseInput, TRUE, TAG_DONE);
								//Store the address of the Function
								initGCSegment = (void*)query.Value;
								if (!initGCSegment(gc, (APTR *)gClassFuncTable, pool, IGraphics, (APTR)IExec, NULL, NULL, (APTR)IUtility, (APTR)ILocale, MAKE_ID('I','G','N',0)))
								{
									ErrorRequest(GetString(&gLocaleInfo, MSG_INIT_CLASS_ERR), gc->gc_ClassName);
									UnLoadSeg(gc->gc_Segment);
								}
							}
							else
								UnLoadSeg(gc->gc_Segment);
						}
						else
							UnLoadSeg(gc->gc_Segment);
					}
					else
						UnLoadSeg(gc->gc_Segment);
				}
				else
					UnLoadSeg(gc->gc_Segment);
			}
		} else {
			ErrorRequest(GetString(&gLocaleInfo, MSG_CLASS_NOT_FOUND_ERR),gc->gc_ClassName);
		}

		SetCurrentDir(olddir);
		UnLock(dir);
	}
	if (!gc->gc_Segment) {
		// remove class
		RemoveLocked((struct MinNode *)gc);
		FreeGClass(gc);

		return FALSE;
	}
	return TRUE;
}
#else
BOOL
LoadGClass(struct gClass *gc)
{
	BOOL ASM (*initGCSegment)(REG(a0, APTR), REG(a1, APTR *), REG(a2, APTR),
		REG(a3, APTR), REG(a6, APTR), REG(d0, APTR), REG(d1, APTR),
		REG(d2, APTR), REG(d3, APTR), REG(d4, long));
	BPTR dir, olddir, segment;

	if (gc->gc_Segment || !gc->gc_ClassName)
		return TRUE;

	if ((dir = Lock(CLASSES_PATH, ACCESS_READ)) != 0) {
		olddir = CurrentDir(dir);
		if ((segment = LoadSeg(gc->gc_ClassName)) != 0) {
			initGCSegment = MKBADDR(segment) + sizeof(APTR);
			if (initGCSegment(gc, gClassFuncTable, pool, GfxBase, SysBase,
					MathIeeeDoubBasBase, MathIeeeDoubTransBase, UtilityBase,
					LocaleBase, MAKE_ID('I','G','N',0)))
				gc->gc_Segment = segment;
			else {
				ErrorRequest(GetString(&gLocaleInfo, MSG_INIT_CLASS_ERR),
					gc->gc_ClassName);
				UnLoadSeg(segment);
			}
		} else {
			ErrorRequest(GetString(&gLocaleInfo, MSG_CLASS_NOT_FOUND_ERR),
				gc->gc_ClassName);
		}

		CurrentDir(olddir);
		UnLock(dir);
	}
	if (!gc->gc_Segment) {
		// remove class
		RemoveLocked((struct MinNode *)gc);
		FreeGClass(gc);

		return FALSE;
	}
	return TRUE;
}
#endif


void
FreeGClasses(void)
{
	struct gClass *gc;

	while ((gc = (struct gClass *)MyRemHead(&gclasses)) != 0)
		FreeGClass(gc);
	while ((gc = (struct gClass *)MyRemHead(&gdiagrams)) != 0)
		FreeGClass(gc);
	while ((gc = (struct gClass *)MyRemHead(&intclasses)) != 0)
		FreeGClass(gc);
}


void
InitGClasses(void)
{
	Printf("Enter init gclasses...\n");
	
	struct gClass *gc, *dgc;
#if defined __amigaos4__ 
	struct AnchorPath *ap;
#else
	struct AnchorPath ALIGNED ap;
#endif
	BPTR dir, olddir, dat;
	char t[512];
	long rc, i;

	MyNewList(&gclasses);
	MyNewList(&gdiagrams);
	MyNewList(&intclasses);

	Printf("NEW LISTS...\n");	
	
	/* initialize internal classes */

		Aqui falla
	
	if ((gc = MakeGClass("root", GCT_ROOT | GCT_INTERNAL, NULL, NULL, NULL, gRootDispatch, NULL, gRootInterface, sizeof(struct gObject))) != 0) {
		if ((dgc = MakeGClass("diagram", GCT_ROOT | GCT_INTERNAL, gc, NULL, NULL, gDiagramDispatch, NULL, gDiagramInterface, sizeof(struct gDiagram) - sizeof(struct gObject))) != 0) {
#ifdef ENABLE_DIAGRAM_3D
			MakeGClass("diagram3d", GCT_ROOT | GCT_INTERNAL, dgc, NULL, NULL, gDiagram3dDispatch, gDiagram3dDraw, NULL, sizeof(struct gDiagram3d) - sizeof(struct gDiagram));
#endif
		}

		gcLabels[3] = GetString(&gLocaleInfo, MSG_PICTURE_CLASS_NAME);

		for (i = 0; gcNames[i]; i++)
			MakeGClass(gcNames[i], gcTypes[i] | GCT_INTERNAL, FindGClass(gcSuper[i]), gcLabels[i], gcImages[i], gcDispatch[i], gcDraw[i], gcInterface[i], gcObjSize[i]);
	}

	Printf("Make Gclass...\n");	

	/* load external class descriptions */

	if ((dir = Lock(CLASSES_PATH, ACCESS_READ)) != 0) {
#if defined __amigaos4__
		olddir = SetCurrentDir(dir);
		ap = AllocDosObjectTags(	DOS_ANCHORPATH, 
	                         	  	ADO_Mask, SIGBREAKF_CTRL_C,
	                         		ADO_Strlen, 1024L,
	                         		TAG_END ); 
#else
		olddir = CurrentDir(dir);
		//memset(&ap, 0, sizeof(struct AnchorPath));
#endif
		
		Printf("Lock...\n");
		
		
#if defined __amigaos4__
		for (rc = MatchFirst("#?.gcdescr", ap); !rc; rc = MatchNext(ap)) {
			if ((dat = Open(ap->ap_Info.fib_FileName, MODE_OLDFILE)) != 0) {
				STRPTR name = NULL, super = NULL, icon = NULL, filename;
				STRPTR localizedNames[10];
					// Note: this is not save against changes to the loc_PrefLanguages table
					//	while parsing the class description file
				long type = 0;

				if ((filename = AllocPooled(pool, i = strlen(ap->ap_Info.fib_FileName) - 4)) != 0)
					CopyMem(ap->ap_Info.fib_FileName, filename, i - 1);
				
				
	Printf("Match...\n");			
				
#else
		for (rc = MatchFirst("#?.gcdescr", &ap); !rc; rc = MatchNext(&ap)) {
			if ((dat = Open(ap.ap_Info.fib_FileName, MODE_OLDFILE)) != 0) {
				STRPTR name = NULL, super = NULL, icon = NULL, filename;
				STRPTR localizedNames[10];
					// Note: this is not save against changes to the loc_PrefLanguages table
					//	while parsing the class description file
				long type = 0;

				if ((filename = AllocPooled(pool, i = strlen(ap.ap_Info.fib_FileName) - 4)) != 0)
					CopyMem(ap.ap_Info.fib_FileName, filename, i - 1);
#endif				
				
				Printf("Match...\n");	
				
				for (i = 0; i < 10; i++)
					localizedNames[i] = NULL;

				while (FGets(dat, t, 512)) {
					t[strlen(t) - 1] = '\0';
						// kill Linefeed

					if (!strnicmp(t, "NAME", 4) && (t[4] == '=' || t[4] == ':'))
						SetLocalizedName(localizedNames, &name, t + 4);
					else if (!strnicmp(t, "SUPER=", 6))
						super = AllocString(t + 6);
					else if (!strnicmp(t, "ICON=", 5))
						icon = AllocString(t + 5);
					else if (!stricmp(t, "OBJECT"))
						type = GCT_OBJECT;
					else if (!stricmp(t, "DIAGRAM"))
						type = GCT_DIAGRAM;
				}
#ifdef __amigaos4__
				SetCurrentDir(olddir);
#else
				CurrentDir(olddir);
#endif

				// search correct localized name for current locale

				GetLocalizedName(localizedNames, &name, NULL);
					
				// create class

				if (!MakeGClass(filename, type, FindGClass(super), name, icon, NULL, NULL, NULL, 0))
					ErrorRequest(GetString(&gLocaleInfo, MSG_INIT_CLASS_ERR), name ? name : filename);

				FreeString(icon);

#ifdef __amigaos4__
				SetCurrentDir(dir);
#else
				CurrentDir(dir);
#endif
				Close(dat);
			}
		}
#if defined __amigaos4__
		MatchEnd(ap);
		FreeDosObject(DOS_ANCHORPATH,ap);
		SetCurrentDir(olddir);
#else
		MatchEnd(&ap);
		CurrentDir(olddir);
#endif
		UnLock(dir);
	
		Printf("Free...\n");
	}
	sortList(&gclasses);
	sortList(&gdiagrams);

	// initialize localized gInterface strings

	gRootInterface[0].gi_Label = GetString(&gLocaleInfo, MSG_NAME_LABEL);

	gEmbeddedInterface[0].gi_Label = GetString(&gLocaleInfo, MSG_OBJECT_PROPERTIES_GAD);
	gEmbeddedInterface[1].gi_Label = GetString(&gLocaleInfo, MSG_ADOPT_SIZE_OF_ORIGINAL_OBJECT_GAD);

	gPictureInterface[0].gi_Label = GetString(&gLocaleInfo, MSG_FILE_NAME_LABEL);
	gPictureInterface[1].gi_Label = GetString(&gLocaleInfo, MSG_ADOPT_SIZE_IN_PIXEL_GAD);
	gPictureInterface[2].gi_Label = GetString(&gLocaleInfo, MSG_KEEP_ASPECT_RATIO_GAD);
	gPictureInterface[3].gi_Label = GetString(&gLocaleInfo, MSG_CENTER_GAD);

	// diagram classes

	gEmbedDiagramInterface[0].gi_Label = GetString(&gLocaleInfo, MSG_DIAGRAM_PREFS_GAD);
}
