/* gObject/gGroup related functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"
#include "classes.h"

#ifdef __amigaos4__
	#include <stdarg.h>
#endif


#ifdef DEBUG
//#  define G2(x) bug("  "); x
//#  define G3(x) bug("    "); x
#  define G2(x) ;
#  define G3(x) ;
#else
#  define G2(x) ;
#  define G3(x) ;
#endif

extern struct RastPort *grp;
ULONG  msfptrn;
WORD   fx, fy, fw, fh, ofx, ofy;
static bool gCommandExecuted;


/*************************************** OO support functions ***************************************/


ULONG
gDoClassMethodA(struct gClass *gc, APTR go, Msg msg)
{
	G2(bug("doClassMethodA: %lx - object: %lx - method: %ld\n", gc, go, *(ULONG *)msg));
    if (gc)
    {
			return gc->gc_Dispatch(gc, go, msg);
	}
	
    return 0L;
}


#ifdef __amigaos4__
ULONG gDoClassMethod(struct gClass *gc, APTR go,  ...) VARARGS68K;
ULONG gDoClassMethod(struct gClass *gc, APTR go,  ...)
{
	va_list ap;
	Msg m;


    if (gc)
    {
        ULONG rvalue;
		
		va_startlinear(ap, go);
		m = va_getlinearva(ap, Msg);

		rvalue = gDoClassMethodA(gc, go, m);
		va_end(ap);
		return rvalue;
 	}
    return 0L;
}
#else
ULONG
gDoClassMethod(struct gClass *gc, APTR go, ULONG id, ...)
{
	G2(bug("doClassMethod: %lx - object: %lx - method: %ld\n", gc, go, id));
    if (gc)
		return gc->gc_Dispatch(gc, go, (Msg)&id);

    return 0L;
}
#endif

ULONG PUBLIC
gDoMethodA(REG(a0, APTR go), REG(a1, Msg msg))
{
    
	G2(bug("doMethodA: object: %lx - method: %ld\n", go, *(ULONG *)msg));

    if (go)
		return ((struct gObject *)go)->go_Class->gc_Dispatch(((struct gObject *)go)->go_Class, go, msg);
    return 0L;
}


#ifdef __amigaos4__
ULONG gDoMethod(APTR go,  ...) VARARGS68K;
ULONG gDoMethod(APTR go,  ...)
{
	va_list ap;
	Msg m;


    if (go)
    {
        ULONG rvalue;

		va_startlinear(ap, go);
		m = va_getlinearva(ap, Msg);
		
		rvalue = gDoMethodA(go, m);
		va_end(ap);
		return rvalue;
	}
    return 0L;
}
#else
ULONG
gDoMethod(APTR go, ULONG id, ...)
{
	G2(bug("doMethod: object: %lx - method: %ld\n", go, id));
    if (go)
		return ((struct gObject *)go)->go_Class->gc_Dispatch(((struct gObject *)go)->go_Class, go, (Msg)&id);

    return 0L;
}
#endif


ULONG PUBLIC
gDoSuperMethodA(REG(a0, struct gClass *gc), REG(a1, APTR go), REG(a2, Msg msg))
{
	G3(bug("doSuperMethodA: %lx - object: %lx - method: %ld\n", gc, go, *(ULONG *)msg));
    if ((gc = gc->gc_Super) != 0)
		return gc->gc_Dispatch(gc, go, msg);

    return 0L;
}


BOOL PUBLIC
gIsSubclassFrom(REG(a0, struct gClass *gc), REG(a1, struct gClass *supergc))
{
    if (!supergc)
        return false;

	while (gc)
    {
        if (gc == supergc)
			return TRUE;
        gc = gc->gc_Super;
    }
	return FALSE;
}


void PUBLIC
gSuperDraw(REG(d0, struct Page *page), REG(d1, ULONG dpi), REG(a0, struct RastPort *rp), REG(a1, struct gClass *gc),
	REG(a2, struct gObject *go), REG(a3, struct gBounds *gb))
{
    if (gc->gc_Super->gc_Draw)
		gc->gc_Super->gc_Draw(page, dpi, rp, gc->gc_Super, go, gb);
}


void
SetGRastPort(struct RastPort *rp)
{
	G3(bug("setGRastPort: %lx\n", rp));

    grp->BitMap = rp->BitMap;
    grp->Layer = rp->Layer;
    grp->Flags &= ~AREAOUTLINE;
	grp->RP_User = rp->RP_User;
}


/*************************************** all other functions ***************************************/


void PUBLIC
gInsertRemoveCellsTerm(REG(a0, struct gcpInReCells *gcpc), REG(a1, struct Page *page), REG(a2, STRPTR *term), REG(a3, struct Term *t))
{
    if (page != gcpc->gcpc_Page)
        return;

    tf_col = 0;  tf_row = 0;
	InReCeKn(t, gcpc->gcpc_Offset, gcpc->gcpc_Diff, gcpc->gcpc_Comp, gcpc->gcpc_First, gcpc->gcpc_Last);

    if (term != NULL)  // Update term string
    {
        FreeString(*term);
		*term = TreeTerm(t, FALSE);
    }
}


void PUBLIC
gInsertRemoveCellsTablePos(REG(a0, struct gcpInReCells *gcpc), REG(a1, struct Page *page), REG(a2, STRPTR *term), REG(a3, struct tablePos *tp))
{
    if (page != gcpc->gcpc_Page)
        return;

    /* MERKER: to be implemented */
}


void
DrawGObject(struct RastPort *rp, struct Page *page, struct gObject *go, long x, long y)
{
    struct gClass *gc = go->go_Class;
	struct gBounds gb;

	G2(bug("drawGObject: %lx - page: %lx - x: %ld - y: %ld\n", go, page, x, y));
	if (gc->gc_Draw == NULL)
		return;

	gb.gb_Left = x + GOKNOB_SIZE;
	gb.gb_Top = y + GOKNOB_SIZE;
	gb.gb_Right = go->go_Right - go->go_Left + x - GOKNOB_SIZE;
	gb.gb_Bottom = go->go_Bottom - go->go_Top + y - GOKNOB_SIZE;
	gc->gc_Draw(page, page->pg_DPI, rp, gc, go, &gb);
}


void
DrawSingleGObject(struct Page *page, struct gObject *go)
{
    struct Window *win;

    G2(bug("drawSingleGObject\n"));
    if (!page || !(win = page->pg_Window))
        return;

	DrawTableCoord(page, go->go_Left - page->pg_TabX + page->pg_wTabX, go->go_Top - page->pg_TabY + page->pg_wTabY,
		go->go_Right - page->pg_TabX + page->pg_wTabX, go->go_Bottom - page->pg_TabY + page->pg_wTabY);
}


void
RefreshGObjectReferences(struct gObject *go)
{
    struct Link *l;

    G2(bug("refreshGObjectReferences\n"));
	if (go == NULL)
        return;

	foreach (&go->go_ReferencedBy, l)
	{
		struct gObject *go = l->l_Link;
		struct Page *page = go->go_Page;

		if (page->pg_Window == NULL)
			continue;

		DrawTableCoord(page, go->go_Left - page->pg_TabX + page->pg_wTabX, go->go_Top - page->pg_TabY + page->pg_wTabY,
			go->go_Right - page->pg_TabX + page->pg_wTabX, go->go_Bottom - page->pg_TabY + page->pg_wTabY);
	}
}


void
DrawGObjectKnobs(struct Page *page,struct gObject *go)
{
    long   i,x,y,pen;
    BOOL   reset;

    G2(bug("drawGObjectKnobs: page: %lx - go: %lx\n",page,go));

    if (!page || !go || !page->pg_Window)
        return;

    makeClip(page->pg_Window,page->pg_wTabX,page->pg_wTabY,page->pg_wTabW,page->pg_wTabH);
    SetHighColor(grp,0x000000);  pen = GetAPen(grp);  reset = FALSE;

    for(i = 0;i < go->go_NumKnobs;i++)
    {
        x = page->pg_wTabX-page->pg_TabX+pixel(page,go->go_Knobs[i].x,TRUE);
        y = page->pg_wTabY-page->pg_TabY+pixel(page,go->go_Knobs[i].y,FALSE);

        if (ReadPixel(grp,x,y) == pen)
            SetAPen(grp,0), reset = TRUE;

        RectFill(grp,x-GOKNOB_SIZE,y-GOKNOB_SIZE,x+GOKNOB_SIZE,y+GOKNOB_SIZE);

        if (reset)
            SetAPen(grp,pen), reset = FALSE;
    }
    if (go->go_Flags & GOF_FRAMED)
        DrawRect(grp,page->pg_wTabX-page->pg_TabX+go->go_Left+GOKNOB_SIZE,page->pg_wTabY-page->pg_TabY+go->go_Top+GOKNOB_SIZE,go->go_Right-go->go_Left-2*GOKNOB_SIZE,go->go_Bottom-go->go_Top-2*GOKNOB_SIZE);
    freeClip(page->pg_Window);
}


LONG
CheckGObjectKnobs(struct Page *page,struct gObject *go,long x,long y)
{
    long i,kx,ky;

    for (i = 0; i < go->go_NumKnobs; i++)
    {
        kx = x-pixel(page,go->go_Knobs[i].x,TRUE);
        ky = y-pixel(page,go->go_Knobs[i].y,FALSE);
        if (abs(kx) <= GOKNOB_SIZE && abs(ky) <= GOKNOB_SIZE)
            return i + 1;
    }
    return 0;
}


BYTE
CheckGObject(struct Page *page,struct gObject *go,long x,long y)
{
    if (x > go->go_Left && x < go->go_Right && y > go->go_Top && y < go->go_Bottom)
        return (BYTE)gDoMethod(go, GCM_HITTEST, mm(page,x,TRUE), mm(page,y,FALSE));

    return false;
}


BYTE
CheckGGroup(struct Page *page,struct gGroup *gg,long x,long y)
{
    if (gg->gg_Type == GOT_OBJECT)
    {
        struct gObject *go = GROUPOBJECT(gg);
        long   i;

        if ((i = CheckGObjectKnobs(page,go,x,y)) != 0)
        {
            G2(bug("checkGGroup: change knob %d\n",i));
            page->pg_ChangeObject = go;
            page->pg_NumPoints = go->go_NumKnobs;
            page->pg_CurrentPoint = i-1;
            if ((page->pg_Points = AllocPooled(pool, sizeof(struct coord)*go->go_NumKnobs)) != 0)
            {
                for (i = 0; i < go->go_NumKnobs; i++)
                {
                    page->pg_Points[i].x = pixel(page,go->go_Knobs[i].x,TRUE);
                    page->pg_Points[i].y = pixel(page,go->go_Knobs[i].y,FALSE);
                }
            }
            return PGA_CHANGE | PGA_KNOBS;
        }
        return PGA_NONE;
    }
    else
    {
        struct gGroup *realgg = gg;

#if 0
	if (prefs.pr_Disp->pd_Flags & PDF_SHOWGROUPS)  /* Rahmen um Gruppen */
        {
        }
#endif
        for(gg = (APTR)realgg->gg_Objects.mlh_TailPred;gg->gg_Node.mln_Pred;gg = (APTR)gg->gg_Node.mln_Pred)
        {
            if (CheckGObjectKnobs(page,GROUPOBJECT(gg),x,y))
                return PGA_CHANGE | PGA_KNOBS;
        }
#if 0
        for(gg = (APTR)realgg->gg_Objects.mlh_TailPred;gg->gg_Node.mln_Pred;gg = (APTR)gg->gg_Node.mln_Pred)
        {
            if (CheckGObject(page,GROUPOBJECT(gg),x,y))
                return(PGA_CHANGE);
        }
#endif
    }
	return 0; /* suppress compiler warning */
}


void
DeselectGObject(struct Page *page,struct gObject *go)
{
    if (!go || !(go->go_Flags & GOF_SELECTED))  /*** || (go->go_Flags & GOF_PROTECTED)) ***/
        return;

    go->go_Flags &= ~GOF_SELECTED;

    DrawTableCoord(page,go->go_Left-page->pg_TabX+page->pg_wTabX,go->go_Top-page->pg_TabY+page->pg_wTabY,go->go_Right-page->pg_TabX+page->pg_wTabX,go->go_Bottom-page->pg_TabY+page->pg_wTabY);
}


void
DeselectGObjects(struct Page *page)
{
    struct gObject *go;

    for(go = (struct gObject *)page->pg_gObjects.mlh_Head;go->go_Node.ln_Succ;go = (APTR)go->go_Node.ln_Succ)
        DeselectGObject(page,go);
}


void
UpdateGObject(struct Page *page,struct gObject *go)
{
    G(bug("updateGObject: %lx - page: %lx\n",go,page));

    go->go_Left = pixel(page,go->go_mmLeft,TRUE)-GOKNOB_SIZE;
    go->go_Top = pixel(page,go->go_mmTop,FALSE)-GOKNOB_SIZE;
    go->go_Right = pixel(page,go->go_mmRight,TRUE)+GOKNOB_SIZE;
    go->go_Bottom = pixel(page,go->go_mmBottom,FALSE)+GOKNOB_SIZE;

    gDoMethod(go,GCM_UPDATE,page->pg_DPI);
}


void
UpdateGGroup(struct Page *page,struct gGroup *gg)
{
    G(bug("updateGGroup: %lx - page: %lx\n",gg,page));

    if (gg->gg_Type == GOT_OBJECT)
        UpdateGObject(page,GROUPOBJECT(gg));
    else
    {
        struct gGroup *igg;

        foreach(&gg->gg_Objects,igg)
            UpdateGGroup(page,igg);

        gg->gg_Left = pixel(page,gg->gg_mmLeft,TRUE);
        gg->gg_Top = pixel(page,gg->gg_mmTop,FALSE);
        gg->gg_Right = pixel(page,gg->gg_mmRight,TRUE);
        gg->gg_Bottom = pixel(page,gg->gg_mmBottom,FALSE);
    }
}


void
UpdateGGroups(struct Page *page)
{
    struct gGroup *gg;

    foreach(&page->pg_gGroups,gg)
        UpdateGGroup(page,gg);
}


void
DeselectGGroup(struct Page *page,struct gGroup *gg,BYTE draw)
{
    G(bug("deselectGGroup: %lx - page: %lx - draw: %d\n",gg,page,draw));

    if (gg->gg_Type == GOT_OBJECT)
        DeselectGObject(page,GROUPOBJECT(gg));
    else
    {
        struct gGroup *igg;

        foreach(&gg->gg_Objects,igg)
            DeselectGGroup(page,igg,FALSE);
        gg->gg_Flags &= ~GOF_SELECTED;

        if (draw /*&& prefs.pr_Disp->pd_Flags & PDF_SHOWGROUPS*/)  // <-- wäre ganz nett
            DrawTableCoord(page,gg->gg_Left-page->pg_TabX+page->pg_wTabX,gg->gg_Top-page->pg_TabY+page->pg_wTabY,gg->gg_Right-page->pg_TabX+page->pg_wTabX,gg->gg_Bottom-page->pg_TabY+page->pg_wTabY);
    }
}


void
DeselectGGroups(struct Page *page)
{
    struct gGroup *gg;

    if (!page)
        return;

    foreach(&page->pg_gGroups,gg)
    {
        if ((gg->gg_Flags & GOF_PROTECTED | GOF_SELECTED) == GOF_SELECTED)
            DeselectGGroup(page,gg,TRUE);
    }
}


void
SelectGGroup(struct Page *page,struct gGroup *gg,BYTE mode)
{
    G(bug("selectGGroup: %lx - page: %lx - draw: %d\n",gg,page,mode));

    if (!page || !gg)
        return;

    gg->gg_Flags |= GOF_PROTECTED;
    if (mode == ACTGO_EXCLUSIVE)
        DeselectGGroups(page);
    gg->gg_Flags &= ~GOF_PROTECTED;

    if (page->pg_Window)
        SetGRastPort(page->pg_Window->RPort);

    if (!(gg->gg_Flags & GOF_SELECTED))
    {
        if (gg->gg_Type == GOT_GROUP)
        {
            struct gGroup *igg;

            for(igg = (APTR)gg->gg_Objects.mlh_Head;igg->gg_Node.mln_Succ;igg = (APTR)igg->gg_Node.mln_Succ)
            {
                if (gg->gg_Type == GOT_OBJECT)
                    DrawGObjectKnobs(page,GROUPOBJECT(igg));
            }
        }
        else
            DrawGObjectKnobs(page,GROUPOBJECT(gg));
    }
    gg->gg_Flags |= GOF_SELECTED;

    /*if (gg->gg_Type == GOT_GROUP && prefs.pr_Disp->pd_Flags & PDF_SHOWGROUPS)  // <-- wäre ganz nett
        DrawGGroup(page,gg);*/
}


/*
BYTE CheckGObj(struct Page *page,struct gObj *go,long x,long y)
{
    long x1,y1,x2,y2;

    if (go->go_Frame->gf_Flags & GFF_HIDDEN)
        return(FALSE);
    if (go->go_Left <= x && (go->go_Left+go->go_Width) >= x && go->go_Top <= y && (go->go_Top+go->go_Height) >= y)
    {
        switch(go->go_Type)
        {
            case GOT_DIAGRAM:
            case GOT_BUTTON:
            case GOT_PICTURE:
                return(TRUE);
            case GOT_VECTOR:
                if (go->go_SubType == GOSTV_LINE)    // MERKER: muß auch nochmal überarbeitet werden
                {
                    x1 = go->go_Left;  y1 = go->go_Top;  x2 = x1+go->go_Width;  y2 = y1+go->go_Height;
                    if (go->go_Flip & GOFLIP_X)
                        swmem(&x1,&x2,sizeof(long));
                    if (go->go_Flip & GOFLIP_Y)
                        swmem(&y1,&y2,sizeof(long));
                    // bug("diff: %ld\n",abs((x2-x1)*(y-y1)-(y2-y1)*(x-x1)));
                    if (abs((x2-x1)*(y-y1)-(y2-y1)*(x-x1)) < 500)
                        return(TRUE);
                }
                else if (go->go_SubType == GOSTV_CIRCLE)
                {
                }
                else
                    return(TRUE);
                break;
        }
    }
    return(FALSE);
}
*/

/*
BOOL UngroupGFrames(struct Page *page,STRPTR name)
{
    struct gFrame *gf,*ngf,*cgf;
    struct UndoNode *un = NULL;
    struct gObj *go;
    long   count = 0,x,y,i = 0;

    if (!page)
        return(FALSE);
    for(gf = (APTR)page->pg_gFrames.mlh_Head;gf->gf_Node.ln_Succ;i++,gf = ngf)
    {
        ngf = (APTR)gf->gf_Node.ln_Succ;
        if (name && !stricmp(name,gf->gf_Node.ln_Name) || !name && (gf->gf_Flags & GFF_SELECTED) && CountNodes((struct List *)&gf->gf_Objects) > 1)
        {
            gf->gf_Pos = i;
            if (!un)
            {
                if (un = CreateUndo(page,UNDO_PRIVATE,"Objektgruppierung auflösen"))
                    un->un_Type = UNT_OBJECTS;
            }
            if (un && (cgf = CopyGFrame(gf,TRUE)))
                MyAddTail(&un->un_UndoList,cgf);
            while(go = (APTR)MyRemHead((struct List *)&gf->gf_Objects))
            {
                go->go_Frame = go->go_OriginalFrame;
                MyAddTail((struct List *)&go->go_Frame->gf_Objects,(struct Node *)go);
                Insert((struct List *)&page->pg_gFrames,go->go_Frame,gf);
                if (un && (cgf = CopyGFrame(gf,TRUE)))
                {
                    go->go_Frame->gf_Undo = un;
                    MyAddTail(&un->un_UndoList,cgf);
                }
                go->go_Frame->gf_Pos = i++;
                UpdateGFrameSize(page,go->go_Frame);
            }
            MyRemove(gf);  count++;  i--;
            DrawTableCoord(page,x = gf->gf_Left-page->pg_TabX+page->pg_wTabX,y = gf->gf_Top-page->pg_TabY+page->pg_wTabY,x+gf->gf_Width,y+gf->gf_Height);
            FreeGFrame(gf);
        }
    }
    if (!count)
        DisplayBeep(NULL);
}
*/

//#ifdef __amigaos4__
//struct gGroups
	//{
	    //struct Node gf_Node;
	    //uint16 gf_Flags;
	    //struct List gf_Objects;
        //struct UndoNode *gf_Undo;
        //uint32 gf_Left, gf_Top, gf_Width, gf_Height;
	//};
//
////struct gFrame *GroupGFrames(struct Page *page,STRPTR name)
//struct gFrame *GroupGFrames(struct Page *page,STRPTR name)
//{
////    struct gFrame *gf,*sgf,*ngf;
    //struct gGroups *gf,*sgf,*ngf;
    //struct UndoNode *un;
    //struct gObj *go;
    //long   count = 0,x,y;
//
    //if (!page)
        //return(NULL);
////    for(gf = (APTR)page->pg_gFrames.mlh_Head;gf->gf_Node.ln_Succ;gf = (APTR)gf->gf_Node.ln_Succ)
    //for(gf = (APTR)page->pg_gGroups.mlh_Head;gf->gf_Node.ln_Succ;gf = (APTR)gf->gf_Node.ln_Succ)
    //{
        ////if (gf->gf_Flags & GFF_SELECTED)
       	//if (gf->gf_Flags & GOF_SELECTED)
            //count++;
    //}
    //if (count < 2)
    //{
        //ErrorRequest("Es müssen mindestens\n2 Objekte markiert sein.");
        //return(NULL);
    //}
    //un = BeginObjectsUndo(page,"Objekte gruppieren");
    //if (gf = CreateGFrame(page,NULL,FALSE))
    //{
        //if (name)
        //{
            //FreeString(gf->gf_Node.ln_Name);
            //gf->gf_Node.ln_Name = AllocString(name);
        //}
////        for(sgf = (APTR)page->pg_gFrames.mlh_Head;sgf->gf_Node.ln_Succ;)
        //for(sgf = (APTR)page->pg_gGroups.mlh_Head;sgf->gf_Node.ln_Succ;)
        //{
            //ngf = (APTR)sgf->gf_Node.ln_Succ;
////           if (sgf->gf_Flags & GFF_SELECTED)
            //if (sgf->gf_Flags & GOF_SELECTED)
            //{
                //MyRemove(sgf);
////                sgf->gf_Flags &= ~GFF_SELECTED;
                //sgf->gf_Flags &= ~GOF_SELECTED;
                //foreach(&sgf->gf_Objects,go)
                //{
////                    go->go_Frame = gf;
                    //MyRemove((struct Node *)go);
                    //MyAddTail((struct List *)&gf->gf_Objects,(struct Node *)go);
                //}
            //}
            //sgf = ngf;
        //}
////        gf->gf_Flags |= GFF_SELECTED;
        //gf->gf_Flags |= GOF_SELECTED;
        //UpdateGFrameSize(page,gf);
        //if (gf->gf_Undo = un)
        //{
            //struct gFrame *cgf;
//
            //if (cgf = CopyGFrame(gf,TRUE))
                //MyAddTail(&un->un_RedoList,cgf);
        //}
        //DrawTableCoord(page,x = gf->gf_Left-page->pg_TabX+page->pg_wTabX,y = gf->gf_Top-page->pg_TabY+page->pg_wTabY,x+gf->gf_Width,y+gf->gf_Height);
    //}
    //return(gf);
//}
//#endif

void
SizeGObject(struct Page *page,struct gObject *go,LONG w,LONG h)
{
    struct UndoNode *un;
    struct point2d *p;

	G(bug("sizeGObject: %lx - page: %lx - w: %ld - h: %ld\n", go, page, w, h));

    if (!page || !go)
        return;
    if (!w || !h)
    {
        UpdateObjectGadgets(go->go_Window);
        ErrorRequest(GetString(&gLocaleInfo, MSG_INVALID_RESIZING_ERR));
        return;
    }

	if ((p = AllocPooled(pool, sizeof(struct point2d) * go->go_NumKnobs)) != 0)
    {
		if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_RESIZE_OBJECT_UNDO))) != 0)
        {
			double xf = (double)w / abs(go->go_mmRight - go->go_mmLeft);
			double yf = (double)h / abs(go->go_mmBottom - go->go_mmTop);
            long   i;

			un->un_Type = UNT_OBJECT_SIZE;
			un->un_Object = go;
			un->un_UndoKnobs = go->go_Knobs;
			un->un_RedoKnobs = p;

			for (i = 0; i < go->go_NumKnobs; i++)
            {
				p[i].x = (go->go_Knobs[i].x - go->go_mmLeft) * xf + go->go_mmLeft;
				p[i].y = (go->go_Knobs[i].y - go->go_mmTop) * yf + go->go_mmTop;
            }

			ApplyObjectSizeUndoRedo(page, un, TYPE_REDO);
        }
    }
}


struct gObject *
gMakeRefObject(struct Page *page, struct gClass *gc, struct gObject *go, const STRPTR undoText)
{
	LONG tags[3] = {GEA_References, 0, TAG_END};
    struct UndoNode *un;
    struct gObject *rgo;

    if (!gc && !(gc = FindGClass("embedded")))
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_REFERENCE_CLASS_NOT_FOUND_ERR));
		return NULL;
    }

    if (page->pg_Window)
        ProjectToGObjects(page,(struct winData *)page->pg_Window->UserData);

    tags[1] = (ULONG)go;

	if ((rgo = (struct gObject *)gDoClassMethod(gc, gc, GCM_NEW, tags, page)) != 0)
    {
        if (!rgo->go_Node.ln_Name)
        {
            struct gObject *refgo = NULL;
            STRPTR s,t;

			GetGObjectAttr(rgo, GEA_References, (ULONG *)&refgo);
			G3(bug("ref: refgo = %lx\n, go = %lx\n", refgo, go));

            //refgo = ((struct gEmbedded *)GINST_DATA(gc,rgo))->ge_References;
			if (refgo && (t = rgo->go_Node.ln_Name = AllocPooled(pool, strlen(s = refgo->go_Node.ln_Name) + 10)))
            {
				strcpy(t, s);
				strcat(t, GetString(&gLocaleInfo, MSG_REFERENCE_SUFFIX_NAME));
            }
            else
                rgo->go_Node.ln_Name = AllocString(GetString(&gLocaleInfo, MSG_REFERENCE_OBJECT_NAME));
        }
		MakeUniqueName(&page->pg_gObjects, &rgo->go_Node.ln_Name);

		RefreshGObjectBounds(page, rgo);
		AddGObject(page, NULL, rgo, ADDREM_DRAW);

		if ((un = CreateUndo(page, UNDO_PRIVATE, undoText != NULL ? undoText : GetString(&gLocaleInfo, MSG_CREATE_REFERENCE_OBJECT_UNDO))) != 0)
        {
			un->un_Type = UNT_ADD_OBJECTS;
			un->un_Object = rgo;
        }
    }
	return 0; /* suppress compiler warning */
}


struct gInterface *
GetGInterfaceTag(struct gClass *gc, ULONG tag)
{
    struct gInterface *gi;

	G3(bug("GetGInterfaceTag(gc = 0x%lx (%s), tag = 0x%lx)\n", gc, gc ? gc->gc_ClassName : (STRPTR)"-", tag));
	if (!gc)
		return NULL;

	if ((gi = gc->gc_Interface) != 0)
    {
		while (gi->gi_Tag && gi->gi_Tag != tag)
            gi++;

        if (gi->gi_Tag == tag)
            return gi;
    }
	
	return GetGInterfaceTag(gc->gc_Super, tag);
}


void
CopyGTagValue(ULONG type, struct TagItem *ti)
{
	switch (type)
    {
        case GIT_FONT:
            ti->ti_Data = (ULONG)CopyFontInfo((struct FontInfo *)ti->ti_Data);
            break;
        case GIT_TEXT:
        case GIT_FILENAME:
        case GIT_FORMULA:
            ti->ti_Data = (ULONG)AllocString((STRPTR)ti->ti_Data);
            break;
        case GIT_PEN:
        case GIT_CHECKBOX:
        case GIT_CYCLE:
        case GIT_WEIGHT:
		case GIT_DATA_PAGE:
            break;
        case GIT_BUTTON:
            ti->ti_Tag = TAG_IGNORE;
            break;
        default:
			ErrorRequest(GetString(&gLocaleInfo, MSG_UNKNOWN_TAG_TYPE_ERR), ti->ti_Tag, type);
            break;
    }
}


ULONG
GetGObjectAttr(struct gObject *go, ULONG tag, ULONG *data)
{
    ULONG rvalue;
    
	G2(bug("getGObjectAttr: %lx - tag: %lx - data: %lx\n", go, tag, data));
	return gDoMethod(go, GCM_GET, tag, data);
}


/** Setzt ein oder mehrere Objektattribute und sorgt dafür, daß
 *  das die Änderungen korrekt dargestellt werden.
 *  Diese Funktion erstellt keine Undo/Redo-Möglichkeit.
 *
 *  @param page die Seite des Objektes
 *  @param go das Objekt
 *  @param tags eine Tag-Liste deren Tags vom Objekt definiert werden
 */

void
gSetObjectAttrsA(struct Page *page, struct gObject *go, struct TagItem *tags)
{
    long rc;

	if ((rc = gDoMethod(go, GCM_SET, tags)) != 0)
    {
        if (rc == GCPR_REDRAW)
        {
			if (!gObjectIsDiagram(go))
			{
				DrawTableCoord(page, go->go_Left - page->pg_TabX + page->pg_wTabX, go->go_Top - page->pg_TabY + page->pg_wTabY,
					go->go_Right - page->pg_TabX + page->pg_wTabX, go->go_Bottom - page->pg_TabY + page->pg_wTabY);
			}
			else
				RefreshGObjectBounds(page, go);

			RefreshGObjectReferences(go);
        }
        else if (rc & GCPR_UPDATESIZE)
			RefreshGObjectDrawing(page, go);
    }
}


#ifdef __amigaos4__
void gSetObjectAttrs(struct Page *page, struct gObject *go,...) VARARGS68K;
void gSetObjectAttrs(struct Page *page, struct gObject *go,...)
#else
void gSetObjectAttrs(struct Page *page, struct gObject *go, ULONG tag1, ...)
#endif
{
#ifdef __amigaos4__
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, go);
	tags = va_getlinearva(ap, struct TagItem *);
	gSetObjectAttrsA(page, go, tags);
#else
	gSetObjectAttrsA(page, go, (struct TagItem *)&tag1);
#endif
}


/** Setzt ein oder mehrere Objektattribute und sorgt dafür, daß
 *  die Änderungen korrekt dargestellt werden, und daß der
 *  vorherige Zustand wieder hergestellt werden kann.<br>
 *  Benutzt gSetObjectAttrs().
 *
 *  @param page die Seite des Objektes
 *  @param go das Objekt
 *  @param tags eine Tag-Liste deren Tags vom Objekt definiert werden
 */

void
SetGObjectAttrsA(struct Page *page, struct gObject *go, struct TagItem *tags)
{
    struct UndoNode *un;

	G(bug("setGObjectAttr: %lx - page: %lx - tags: %lx\n", go, page, tags));

    if (!page || !go)
        return;

	if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_OBJECT_ATTRIBUTES_UNDO))) != 0)
    {
		struct TagItem *ti, *fromTags, *toTags, *tstate = tags;
        long   i;

		un->un_Type = UNT_OBJECT_ATTRS;

        fromTags = CloneTagItems(tags);
        toTags = CloneTagItems(tags);

        if (fromTags && toTags)
        {
            struct gInterface *gi;

			for (i = 0; (ti = NextTagItem(&tstate)) != 0; i++)
            {
				G3(bug("tag = 0x%08lx\n", ti->ti_Tag));
				if ((gi = GetGInterfaceTag(go->go_Class, ti->ti_Tag)) != 0)
                {
					G3(bug("    -> interface found, type = %ld\n", gi->gi_Type));
					if (gDoMethod(go, GCM_GET, fromTags[i].ti_Tag, &fromTags[i].ti_Data))
						CopyGTagValue(gi->gi_Type, fromTags + i);
					CopyGTagValue(gi->gi_Type, toTags + i);

					G3(bug("%2ld.  to: tag = 0x%08lx, value = 0x%08lx (%ld) (%s)\n", i, (toTags + i)->ti_Tag, (toTags + i)->ti_Data, (toTags + i)->ti_Data, gi->gi_Type == GIT_TEXT || gi->gi_Type == GIT_FORMULA ? (toTags + i)->ti_Data : (ULONG)"-"));
					G3(bug("   from: tag = 0x%08lx, value = 0x%08lx (%ld) (%s)\n", fromTags[i].ti_Tag, fromTags[i].ti_Data, (fromTags + i)->ti_Data, gi->gi_Type == GIT_TEXT || gi->gi_Type == GIT_FORMULA ? (fromTags + i)->ti_Data : (ULONG)"-"));
                }
				else
				{
					G(bug("NO gInterface!!!\n"));
					fromTags[i].ti_Tag = TAG_IGNORE;
					toTags[i].ti_Tag = TAG_IGNORE;
				}
            }
			un->un_UndoTags = fromTags;
			un->un_RedoTags = toTags;
			un->un_Object = go;
        }
        else
        {
            FreeTagItems(fromTags);
            FreeTagItems(toTags);
        }
    }
	gSetObjectAttrsA(page, go, tags);
    UpdateObjectGadgets(go->go_Window);
}


#ifdef __amigaos4__
void SetGObjectAttrs(struct Page *page, struct gObject *go, ...) VARARGS68K;
void SetGObjectAttrs(struct Page *page, struct gObject *go, ...)
#else
void SetGObjectAttrs(struct Page *page, struct gObject *go, ULONG tag1, ...)
#endif
{
#ifdef __amigaos4__
	va_list ap;
	struct TagItem *tags;

	va_startlinear(ap, go);
	tags = va_getlinearva(ap, struct TagItem *);
	SetGObjectAttrsA(page, go, tags);
#else
	SetGObjectAttrsA(page, go, (struct TagItem *)&tag1);
#endif
}


void
FreeObjectChange(struct Page *page)
{
	G(bug("freeObjectChange: page: %lx\n", page));

    if (page->pg_Action & PGA_KNOBS)
    {
		FreePooled(pool, page->pg_Points, page->pg_NumPoints * sizeof(struct coord));
        page->pg_Points = NULL;
        page->pg_ChangeObject = NULL;
        page->pg_CurrentPoint = 0;
        page->pg_NumPoints = 0;
    }
    page->pg_Action = PGA_NONE;
}


void
DrawGObjectMove(struct Page *page)
{
    if (!fx && !fy)
        return;

	makeClip(win, wd->wd_TabX, wd->wd_TabY, wd->wd_TabW, wd->wd_TabH);
    SetGRastPort(win->RPort);
	SetDrMd(grp, COMPLEMENT);

    if (page->pg_Action & PGA_KNOBS)   /* change knobs */
    {
        page->pg_Points[page->pg_CurrentPoint].x = fx;
        page->pg_Points[page->pg_CurrentPoint].y = fy;

        gDoMethod(page->pg_ChangeObject,GCM_CHANGEPOINT,grp,page->pg_Points,
                            (ofx << 16) | (ofy & 0xffff),page->pg_NumPoints,GCPAM_UPDATE,page->pg_CurrentPoint);
    }
    else                               /* move object(s) */
    {
        struct gObject *go;

        for(go = (APTR)page->pg_gObjects.mlh_Head;go->go_Node.ln_Succ;go = (APTR)go->go_Node.ln_Succ)
        {
            if (go->go_Flags & GOF_SELECTED)
            {
                long x = wd->wd_TabX+go->go_Left-page->pg_TabX+fx;
                long y = wd->wd_TabY+go->go_Top-page->pg_TabY+fy;
                long w = go->go_Right-go->go_Left+fw,h = go->go_Bottom-go->go_Top+fh;

                DrawRect(grp,x,y,w,h);
            }
        }
    }
    freeClip(win);
    SetDrMd(grp,JAM2);
}


void
DrawMultiSelectFrame(struct Page *page,UBYTE mode)
{
    struct RastPort *rp = win->RPort;
    long x1,y1,x2,y2;

    if (mode == DMSF_BEGIN || mode == DMSF_ALL)
    {
        makeClip(win,wd->wd_TabX,wd->wd_TabY,wd->wd_TabW,wd->wd_TabH);
        rp->linpatcnt = 15;
        SetDrMd(rp,COMPLEMENT);
    }
    rp->LinePtrn = msfptrn;
    x1 = fx-page->pg_TabX+page->pg_wTabX;  x2 = x1+fw-fx;
    y1 = fy-page->pg_TabY+page->pg_wTabY;  y2 = y1+fh-fy;
    Move(rp,x1+1,y1);  Draw(rp,x2,y1);
    Move(rp,x2,y1+1);  Draw(rp,x2,y2);
    Move(rp,x2-1,y2);  Draw(rp,x1,y2);
    Move(rp,x1,y2-1);  Draw(rp,x1,y1);
    if (mode == DMSF_END || mode == DMSF_ALL)
    {
        SetDrMd(rp,JAM2);
        rp->linpatcnt = 0;  rp->LinePtrn = 0xffff;
        freeClip(win);
    }
}


void
DoGObjectAction(struct Page *page, struct gObject *go, long type)
{
	G(bug("doGObjectAction: %lx - page: %lx - type: %ld\n", go, page, type));

    if (!page)
        return;

    if (!go)
    {
		foreach(&page->pg_gObjects, go)     // Selektiertes Objekt finden, wenn
        {                                   // keines übergeben wurde
            if (go->go_Flags & GOF_PRESSED)
            {
                go->go_Flags &= ~GOF_PRESSED;   // deselektieren
                break;
            }
        }
        if (!go->go_Node.ln_Succ)           // keines gefunden
            return;
    }
    else if (type == SELECTDOWN)
    {
        go->go_Flags |= GOF_PRESSED;
		gCommandExecuted = FALSE;
    }
    else
        go->go_Flags &= ~GOF_PRESSED;

	if (type != SELECTDOWN && !gCommandExecuted
		&& CheckGObject(page, go, page->pg_TabX + imsg.MouseX - page->pg_wTabX, page->pg_TabY + imsg.MouseY - page->pg_wTabY))
			gDoMethod(go, GCM_COMMAND);

	DrawSingleGObject(page, go);
}


void
FreeCreateObject(struct Page *page, struct winData *wd, BOOL abort)
{
    struct coord *p = page->pg_Points;
    long   cur = page->pg_CurrentPoint;

	G(bug("freeCreateObject: page: %lx\n", page));

    if (page)
    {
        if (abort)
        {
            struct Window *win = page->pg_Window;

            page->pg_HotSpot = PGHS_CELL;
            SetMousePointer(win,STANDARD_POINTER);

            if (p) // Objekt-Rahmen wieder entfernen
            {
                long ox = page->pg_wTabX-page->pg_TabX, oy = page->pg_wTabY-page->pg_TabY;

                SetGRastPort(win->RPort);
                makeClip(win,wd->wd_TabX,wd->wd_TabY,wd->wd_TabW,wd->wd_TabH);
                SetDrMd(grp,COMPLEMENT);

				gDoClassMethod(page->pg_CreateFromClass,NULL,GCM_ADDPOINT,grp, p,(ox << 16) | (oy & 0xffff),cur,GCPAM_REDRAW);
                freeClip(win);
                SetDrMd(grp,JAM2);
            }
        }
        if (p)
            FreePooled(pool,p,cur*sizeof(struct coord));

        page->pg_Action = PGA_NONE;
        page->pg_Points = NULL;
        page->pg_CreateFromClass = NULL;
        page->pg_CurrentPoint = 0;
        page->pg_NumPoints = 0;
    }
    if (wd)
        wd->wd_ExtData[6] = NULL;

	SetProjectMouseReport(page, FALSE);
}


void
RefreshGObjectBounds(struct Page *page, struct gObject *go)
{
    G(bug("refreshGObjectBounds: %lx - page: %lx\n",go,page));

    if (!go || !page)
        return;

	gDoMethod(go, GCM_BOX);
	UpdateGObject(page, go);
}


/** Entfernt ein Objekt von einer Seite. Das Objekt wird nicht
 *  freigegeben.
 *
 *  @param page die Seite des Objektes (darf nicht NULL sein!)
 *  @param go das zu entfernende Objekt
 *  @param flags es können die Flags ADDREM_xxx gesetzt und kombiniert werden
 *
 *  @flag ADDREM_NONE keine besondere Aktion
 *  @flag ADDREM_DRAW die Seite wird neu gezeichnet
 *  @flag ADDREM_NOGROUP der Gruppeneintrag wird nicht entfernt
 *  @flag ADDREM_NOREFS Objekte, die das aktuelle Objekt referenzieren
 *    werden nicht benachrichtigt.
 *	@flag ADDREM_CLOSE_WINDOW ein evtl. vorhandenes Fenster wird geschlossen
 */

void
RemoveGObject(struct Page *page, struct gObject *go, BYTE flags)
{
    long   num = page->pg_NumObjects;

    G({bug("RemoveGObject: 0x%lx - page: 0x%lx - (",go,page);
         if (flags & ADDREM_DRAW) bug("draw ");
         if (flags & ADDREM_NOGROUP) bug("nogroup");
         if (flags & ADDREM_NOREFS) bug(" norefs");
         bug(")\n");});

    if (!num || !go || !page->pg_ObjectOrder)
        return;

    if (page->pg_NumObjects > 1)
    {
        long i,pos = go->go_Pos,oldsize = page->pg_OrderSize;
        struct gObject **gos = NULL;

		if (num < oldsize - 10 && (gos = AllocPooled(pool, sizeof(APTR) * (num-1))))  // shrink buffer
        {
            if (pos)
				CopyMem(page->pg_ObjectOrder, gos, sizeof(APTR)*pos);

			CopyMem(page->pg_ObjectOrder + pos + 1, gos + pos, sizeof(APTR) * (num - 1 - pos));
			FreePooled(pool,page->pg_ObjectOrder, sizeof(APTR)*num);
            page->pg_ObjectOrder = gos;
			page->pg_OrderSize = num - 1;

			for (i = pos; i < num-1; i++)
                gos[i]->go_Pos = i;
        }
        else
        {
            gos = page->pg_ObjectOrder;

            for(i = pos;i < num-1;i++)
            {
                gos[i] = gos[i+1];
                gos[i]->go_Pos = i;
            }
        }
    }
    page->pg_NumObjects--;

    MyRemove(go);
#ifndef __amigaos4__
    go->go_Page = NULL;		//calls the grim in CopyGGroup
#endif

    if (!(flags & ADDREM_NOGROUP))
        MyRemove(OBJECTGROUP(go));

    if (flags & ADDREM_DRAW)
        DrawTableCoord(page,go->go_Left-page->pg_TabX+page->pg_wTabX,go->go_Top-page->pg_TabY+page->pg_wTabY,go->go_Right-page->pg_TabX+page->pg_wTabX,go->go_Bottom-page->pg_TabY+page->pg_wTabY);

	if (flags & ADDREM_CLOSE_WINDOW)
		gDoMethod(go, GCM_CLOSEWINDOW);

    if (!(flags & ADDREM_NOREFS))
    {
        struct Link *l,*cl,*nl;
        struct gObject *rgo;
        struct MinList list;

        MyNewList(&list);

        for(l = (APTR)go->go_ReferencedBy.mlh_Head;l->l_Node.mln_Succ;l = nl)
        {
            nl = (struct Link *)l->l_Node.mln_Succ;

			if ((cl = AllocPooled(pool, sizeof(struct Link))) != 0)
            {
                cl->l_Link = l->l_Link;
                MyAddTail(&list, cl);
            }
            rgo = l->l_Link;
			gSetObjectAttrs(rgo->go_Page, rgo, GEA_References, NULL, TAG_END);
        }
        moveList(&list,&go->go_ReferencedBy);
        go->go_Flags |= GOF_INVALIDREFS;
    }
}


void
AddGObject(struct Page *page,struct gGroup *gg,struct gObject *go,BYTE flags)
{
    struct gObject **gos;
    LONG   pos = go->go_Pos,num = page->pg_NumObjects,oldsize;

    G({bug("AddGObject: 0x%lx - page: 0x%lx - (",go,page);
         if (flags & ADDREM_DRAW) bug("draw");
         if (flags & ADDREM_NOGROUP) bug("nogroup");
         if (flags & ADDREM_NOREFS) bug("norefs");
         bug(")\n");
        });

    if (pos < 0 || pos > num)   // Objektposition am Ende
        pos = num;

    if (num+1 > (oldsize = page->pg_OrderSize))  /* neues ObjectOrder-Array erstellen */
    {
        if ((gos = AllocPooled(pool, sizeof(APTR) * (num + 10))) != 0)
            page->pg_OrderSize = num+10;
    }
    else
        gos = page->pg_ObjectOrder;

    if (!gos)
    {
        D(bug("AddGObject(): Allocating new ObjectOrder failed!"));
        return;
    }

    if (page->pg_ObjectOrder)
    {
        long i;

        if (gos != page->pg_ObjectOrder)
        {
            CopyMem(page->pg_ObjectOrder,gos,sizeof(APTR)*pos);
            if (pos != num)
                CopyMem(page->pg_ObjectOrder+pos,gos+pos+1,sizeof(APTR)*(num-pos));

            FreePooled(pool,page->pg_ObjectOrder,sizeof(APTR)*oldsize);

            for(i = pos+1;i < num+1;i++)
                gos[i]->go_Pos = i;
        }
        else
        {
            for(i = num;i > pos;i--)
            {
                gos[i] = gos[i-1];
                gos[i]->go_Pos = i;
            }
        }
    }
    page->pg_ObjectOrder = gos;
    gos[go->go_Pos = pos] = go;
    page->pg_NumObjects++;

    MyAddTail(&page->pg_gObjects, go);
    go->go_Page = page;   /* MERKER: ist das so okay?? (oder muß "go" das wissen?) */

    if (!(flags & ADDREM_NOGROUP))
    {
        if (gg)
            MyAddTail(&gg->gg_Objects, OBJECTGROUP(go));
        else
            MyAddTail(&page->pg_gGroups, OBJECTGROUP(go));
    }
    if (flags & ADDREM_DRAW)
    {
        UpdateGObject(page,go);
        DrawTableCoord(page,go->go_Left-page->pg_TabX+page->pg_wTabX,go->go_Top-page->pg_TabY+page->pg_wTabY,go->go_Right-page->pg_TabX+page->pg_wTabX,go->go_Bottom-page->pg_TabY+page->pg_wTabY);
    }

    /** aktualisiere Referenzen **/

    if (go->go_Flags & GOF_INVALIDREFS && !(flags & ADDREM_NOREFS))
    {
        struct MinList list;
        struct gObject *rgo;
        struct Link *l;

        moveList(&go->go_ReferencedBy,&list);

        while((l = (struct Link *)MyRemHead(&list)))
        {
            rgo = l->l_Link;
            gSetObjectAttrs(rgo->go_Page, rgo, GEA_References, go, TAG_END);
            FreePooled(pool,l,sizeof(struct Link));
        }
        go->go_Flags &= ~GOF_INVALIDREFS;
    }
}


BOOL
GObjectsOverlap(struct gObject *ago,struct gObject *bgo)
{
    if (ago->go_mmLeft <= bgo->go_mmRight && ago->go_mmRight >= bgo->go_mmLeft && ago->go_mmTop <= bgo->go_mmBottom && ago->go_mmBottom >= bgo->go_mmTop)
    {
        /* MERKER: da fehlt noch was */
        return true;
    }
    return false;
}


BOOL
ChangeGObjectOrder(struct Page *page, struct gObject *go, UBYTE type)
{
    struct UndoNode *un;
    long   i;

    G(bug("changeGObjectOrder: page: %lx - go: %lx - type: %ld\n",page,go,type));

    if (!page || !go)
        return false;

//bug("go_Pos = %ld, pg_NumObjects = %ld, type = %d\n", go->go_Pos, page->pg_NumObjects, type);
    if (go->go_Pos == page->pg_NumObjects - 1 && (type == GO_FRONTMOST || type == GO_TOFRONT))
        return true;
    if (go->go_Pos == 0 && (type == GO_BACKMOST || type == GO_TOBACK))
        return true;

	if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_OBJECT_DEPTH_UNDO))) != 0)
    {
		un->un_Type = UNT_OBJECT_DEPTH;
		un->un_Object = go;
		un->un_UndoPosition = go->go_Pos;
    }

    RemoveGObject(page, go, ADDREM_NOREFS);

    switch (type)
    {
        case GO_FRONTMOST:
            go->go_Pos = page->pg_NumObjects;
            break;
        case GO_BACKMOST:
            go->go_Pos = 0;
            break;
        case GO_TOFRONT:
            for (i = go->go_Pos;i < page->pg_NumObjects;i++)
            {
                if (GObjectsOverlap(go, page->pg_ObjectOrder[i]))
                {
					go->go_Pos = i + 1;
                    break;
                }
            }
            break;
        case GO_TOBACK:
			for (i = go->go_Pos - 1; i >= 0; i--)
            {
                if (GObjectsOverlap(go, page->pg_ObjectOrder[i]))
                {
                    go->go_Pos = i;
                    break;
                }
            }
            break;
    }
    AddGObject(page, NULL, go, ADDREM_DRAW | ADDREM_NOREFS);

    if (un)
		un->un_RedoPosition = go->go_Pos;

    return true;
}


void
RemoveGGroup(struct Page *page, struct gGroup *gg, BYTE draw, LONG level)
{
	G(bug("removeGGroup: %lx - page: %lx - draw: %d - level: %l\n", gg, page, draw, level));

    if (gg->gg_Type == GOT_OBJECT)
		RemoveGObject(page, GROUPOBJECT(gg), (level ? ADDREM_NOGROUP : 0) | (draw ? ADDREM_DRAW : 0) | ADDREM_CLOSE_WINDOW);
    else
    {
        struct gGroup *igg;

		foreach (&gg->gg_Objects, igg)
			RemoveGGroup(page, igg, FALSE, level + 1);

        MyRemove(gg);

        if (draw)
			DrawTableCoord(page, gg->gg_Left - page->pg_TabX + page->pg_wTabX, gg->gg_Top - page->pg_TabY + page->pg_wTabY,
				gg->gg_Right - page->pg_TabX + page->pg_wTabX, gg->gg_Bottom - page->pg_TabY + page->pg_wTabY);
    }
}


void
AddGGroup(struct Page *page, struct gGroup *gg, BYTE draw, LONG level)
{
	G(bug("addGGroup: %lx - page: %lx - draw: %d - level: %l\n", gg, page, draw, level));

    if (gg->gg_Type == GOT_OBJECT)
		AddGObject(page, NULL, GROUPOBJECT(gg), (level ? ADDREM_NOGROUP : 0) | (draw ? ADDREM_DRAW : 0));
    else
    {
        struct gGroup *igg;

		foreach (&gg->gg_Objects, igg)
			AddGGroup(page, igg, FALSE, level + 1);

		MyAddTail(&page->pg_gGroups, gg);

		UpdateGGroup(page, gg);
        if (draw)
			DrawTableCoord(page, gg->gg_Left - page->pg_TabX + page->pg_wTabX, gg->gg_Top - page->pg_TabY + page->pg_wTabY,
				gg->gg_Right - page->pg_TabX + page->pg_wTabX, gg->gg_Bottom - page->pg_TabY + page->pg_wTabY);
    }
}


void
FreeGGroup(struct gGroup *gg)
{
	G(bug("freeGGroup: %lx\n", gg));

    if (gg->gg_Type == GOT_OBJECT)
    {
        struct gObject *go;

        MyRemove(go = GROUPOBJECT(gg));
        FreeGObject(go);
    }
    else
    {
        struct gGroup *igg;

		while ((igg = (APTR)MyRemHead(&gg->gg_Objects)) != 0)
            FreeGGroup(igg);
		FreePooled(pool, gg, sizeof(struct gGroup));
    }
}


void
FreeGObject(struct gObject *go)
{
    G(bug("freeGObject: %lx\n",go));
    gDoMethod(go,GCM_DISPOSE);
}


struct gObject *
NewGObjectA(struct Page *page, struct gClass *gc, struct point2d *points, ULONG numPoints, struct TagItem *tags)
{
	struct UndoNode *un;
	struct gObject *go;

	G(bug("newGObjectA: page: %lx - class: %lx - points: %lx - num: %ld - tags: %lx\n", page, gc, points, numPoints, tags));
	
	if (!gc || !points || (go = (struct gObject *)gDoClassMethod(gc, gc, GCM_NEW, tags, page)) == NULL)
		return NULL;

    if (!go->go_Node.ln_Name)
        go->go_Node.ln_Name = AllocString(gc->gc_Node.in_Name);
	MakeUniqueName(&page->pg_gObjects, &go->go_Node.ln_Name);

	go->go_Knobs = points;
	go->go_NumKnobs = numPoints;

	RefreshGObjectBounds(page, go);
	AddGObject(page, NULL, go, ADDREM_NONE);

	if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_CREATE_OBJECT_UNDO))) != 0)
    {
		un->un_Type = UNT_ADD_OBJECTS;
		un->un_Object = go;
    }

	return go;
}

/*
struct gObject *
NewGObject(struct Page *page, struct gClass *gc, struct point2d *p, ULONG num, ULONG tag,...)
{
	return NewGObjectA(page, gc, p, num, (struct TagItem *)&tag);
}
*/

/** Bereitet die angegebene Seite vor, ein Objekt mit der Maus
 *  einzufügen.
 *
 *  @param page die Seite
 *  @param gc die Klasse des einzufügenden Objektes
 *  @param more
 */

void
PrepareCreateObject(struct Page *page, struct gClass *gc, BOOL more)
{
	G(bug("insertGObject: page: %lx - class: %lx - more: %ld\n", page, gc, more));

    if (!page || !page->pg_Window || !gc)
        return;

    if (!(gc->gc_Node.in_Type & GCT_INTERNAL) && !gc->gc_Segment && !LoadGClass(gc))
        return;

    if (page->pg_Mappe->mp_Flags & MPF_SCRIPTS)
    {
        page->pg_Mappe->mp_Flags &= ~MPF_SCRIPTS;
		UpdateInteractive(page->pg_Mappe, TRUE);
        DrawStatusText(page,GetString(&gLocaleInfo, MSG_SWITCH_TO_NORMAL_MODE_STATUS));
    }
	if ((page->pg_NumPoints = gDoClassMethod(gc, NULL, GCM_BEGINPOINTS, more ? GCPBM_MORE : GCPBM_ONE)) != 0)
    {
        struct winData *wd = (struct winData *)page->pg_Window->UserData;

        page->pg_CreateFromClass = gc;
        page->pg_Points = NULL;
        page->pg_CurrentPoint = 0;
        page->pg_Action = PGA_CREATE;
        page->pg_HotSpot = PGHS_OBJECT;

		ProjectToGObjects(page, wd);
		SetProjectMouseReport(page, TRUE);

        DeselectGObjects(page);
    }
    else
    {
		ErrorRequest(GetString(&gLocaleInfo, MSG_INIT_OBJECT_ERR));
        page->pg_CreateFromClass = NULL;
    }
}


struct gObject *
DoCreateObject(struct Page *page)
{
    struct gObject *go = NULL;
    struct point2d *p,*sp = NULL;
    struct Window *cwin;
    long   i,num;

    G(bug("doCreateObject: page: %lx\n",page));

    if ((cwin = GetAppWindow(WDT_GCLASSES)) != 0)   // WDT_CLASSES zurücksetzen
    {
        GT_SetGadgetAttrs(GadgetAddress(cwin,1),cwin,NULL,GTLV_Selected,~0L,TAG_END);
        SetWindowPointer(cwin,TAG_END);
    }

    if ((p = AllocPooled(pool, page->pg_CurrentPoint * sizeof(struct point2d))) != 0)
    {
        struct winData *wd = NULL;

        for(i = 0;i < page->pg_CurrentPoint;i++)  // Pixel in Millimeter umrechnen
        {
            p[i].x = mm(page,page->pg_Points[i].x,TRUE);
            p[i].y = mm(page,page->pg_Points[i].y,FALSE);
        }
        gDoClassMethod(page->pg_CreateFromClass,NULL,GCM_ENDPOINTS,p,i,&sp,&num);

        if (sp && num)
        {
            if (sp != p)
                FreePooled(pool,p,i*sizeof(struct point2d));
            if ((go = NewGObjectA(page, page->pg_CreateFromClass, sp, num, NULL)) != 0)
                DrawTableCoord(page,go->go_Left-page->pg_TabX+page->pg_wTabX,go->go_Top-page->pg_TabY+page->pg_wTabY,go->go_Right-page->pg_TabX+page->pg_wTabX,go->go_Bottom-page->pg_TabY+page->pg_wTabY);
        }

        if (page->pg_Window)
            wd = (struct winData *)page->pg_Window->UserData;

		FreeCreateObject(page, wd, FALSE);
    }
    return go;
}


void
RefreshGObjectDrawing(struct Page *page, struct gObject *go)
{
    long r,l,t,b,gr,gl,gt,gb;
    long xoff = page->pg_wTabX-page->pg_TabX;
    long yoff = page->pg_wTabY-page->pg_TabY;

    G(bug("refreshGObjectDrawing: %lx - page: %lx\n",go,page));

    l = max(page->pg_wTabX,go->go_Left+xoff);  r = min(page->pg_wTabW+page->pg_wTabX,go->go_Right+xoff);
    t = max(page->pg_wTabY,go->go_Top+yoff);  b = min(page->pg_wTabH+page->pg_wTabY,go->go_Bottom+yoff);

	RefreshGObjectBounds(page, go);

    gl = max(page->pg_wTabX,go->go_Left+xoff);  gr = min(page->pg_wTabW+page->pg_wTabX,go->go_Right+xoff);
    gt = max(page->pg_wTabY,go->go_Top+yoff);  gb = min(page->pg_wTabH+page->pg_wTabY,go->go_Bottom+yoff);

    if (l >= gl && r <= gr && t >= gt && b <= gb)      // altes in neuem
        DrawTableCoord(page,gl,gt,gr,gb);
    else if (l <= gl && r >= gr && t <= gt && b >= gb) // neues in altem
        DrawTableCoord(page,l,t,r,b);
    else if (b < gt || gb < t || r < gl || gr < l)     // unabhängig
    {
        DrawTableCoord(page,l,t,r,b);
        DrawTableCoord(page,gl,gt,gr,gb);
    }
    else                                               // überlappend
    {
        if (t < gt)
            DrawTableCoord(page,l,t,r,gt-1);
        else if (t != gt)
            DrawTableCoord(page,gl,gt,gr,t-1);

		DrawTableCoord(page, min(gl, l), max(t, gt), max(gr, r), min(b, gb));

        if (b < gb)
			DrawTableCoord(page, gl, b + 1, gr, gb);
        else if (b != gb)
			DrawTableCoord(page, l, gb + 1, r, b);
    }
	RefreshGObjectReferences(go);
}


void
AddCreateObjectPoint(struct Page *page,LONG x,LONG y,LONG ox,LONG oy)
{
    struct coord *newp,*oldp = page->pg_Points;
    long   i = page->pg_CurrentPoint;

    if (oldp && i > 1 && abs(oldp[i-2].x-x) < 2 && abs(oldp[i-2].y-y) < 2) // no difference
        return;

    SetGRastPort(page->pg_Window->RPort);
    makeClip(win,wd->wd_TabX,wd->wd_TabY,wd->wd_TabW,wd->wd_TabH);
    SetDrMd(grp,COMPLEMENT);
    if (page->pg_CurrentPoint == page->pg_NumPoints)
    {
        gDoClassMethod(page->pg_CreateFromClass,NULL,GCM_ADDPOINT,grp,
                                     oldp,(ox << 16) | (oy & 0xffff),i,GCPAM_UPDATE);
        freeClip(win);
        SetDrMd(grp,JAM2);

        DoCreateObject(page);
    }
    else if ((newp = AllocPooled(pool, (i + 1) * sizeof(struct coord))) != 0)      // new point
    {
        if (oldp)
        {
            CopyMem(oldp,newp,i*sizeof(struct coord));

            /*gDoClassMethod(page->pg_CreateFromClass,NULL,GCM_ADDPOINT,grp,  // darf gar nicht sein
                                         oldp,(ox << 16) | (oy & 0xffff),i,GCPAM_UPDATE);*/

            FreePooled(pool,oldp,i*sizeof(struct coord));
        }
        newp[i].x = x;
        newp[i].y = y;
        page->pg_Points = newp;
        page->pg_CurrentPoint++;

        gDoClassMethod(page->pg_CreateFromClass,NULL,GCM_ADDPOINT,grp,
                                     newp,(ox << 16) | (oy & 0xffff),page->pg_CurrentPoint,GCPAM_ONEMORE);
        freeClip(win);
        SetDrMd(grp,JAM2);
    }
}


struct Window *
OpenGObjectWindow(struct gObject *go)
{
    return (struct Window *)gDoMethod(go,GCM_OPENWINDOW);
}


struct gGroup *
GetParentGGroup(struct Page *page,struct gGroup *gg)
{
    struct MinList *list = FindList((struct MinNode *)gg);

    if (list == &page->pg_gGroups)
        return gg;

    /* group is part of a parent gGroup */

    G2({
        if (gg->gg_Type != GOT_OBJECT)
        {
            bug("getParentGroup: help!\n");
            return gg;
        }
    });
    return (struct gGroup *)((UBYTE *)list-(UBYTE *)&((struct gGroup *)0L)->gg_Objects);
}


struct gGroup *
MouseGGroup(struct Page *page,LONG *pos)
{
    long   i;

    for(i = page->pg_NumObjects-1;i >= 0;i--)
    {
        if (CheckGObject(page,page->pg_ObjectOrder[i],page->pg_TabX+imsg.MouseX-page->pg_wTabX,page->pg_TabY+imsg.MouseY-page->pg_wTabY))
        {
            if (pos)
                *pos = i;
            return(GetParentGGroup(page,OBJECTGROUP(page->pg_ObjectOrder[i])));
        }
    }
    return NULL;
    /*
    for(i = 0,gg = (APTR)page->pg_gGroups.mlh_TailPred;gg->gg_Node.mln_Pred;gg = (APTR)gg->gg_Node.mln_Pred)
    {
        if (gg->gg_Type == GOT_OBJECT)
            selected = CheckGObject(page,GROUPOBJECT(gg),page->pg_TabX+imsg.MouseX-page->pg_wTabX,page->pg_TabY+imsg.MouseY-page->pg_wTabY), i++;
        else
        {
            struct gGroup *igg;

            for(igg = (APTR)gg->gg_Objects.mlh_TailPred;igg->gg_Node.mln_Pred;igg = (APTR)igg->gg_Node.mln_Pred,i++)  // scanning in reverse order -> object depth
            {
                if (selected = CheckGObject(page,GROUPOBJECT(igg),page->pg_TabX+imsg.MouseX-page->pg_wTabX,page->pg_TabY+imsg.MouseY-page->pg_wTabY))
                {
                    gg = igg;
                    break;
                }
            }
        }
        if (selected)
        {
            if (pos)
                *pos = i;
            return(gg);
        }
    }
    return(NULL);
    */
}


BOOL
HandleGObjects(struct Page *page)
{
    if (page->pg_Action & PGA_CREATE)
    {
        switch(imsg.Class)
        {
            case IDCMP_MOUSEBUTTONS:
            {
                long ox = page->pg_wTabX-page->pg_TabX, oy = page->pg_wTabY-page->pg_TabY;

                if (imsg.Code == SELECTDOWN || imsg.Code == SELECTUP)   // add point or finish
                {
                    if (!(imsg.MouseX > page->pg_wTabX && imsg.MouseX < page->pg_wTabX+page->pg_wTabW && imsg.MouseY > page->pg_wTabY && imsg.MouseY < page->pg_wTabY+page->pg_wTabH))
                    {
                        FreeCreateObject(page,wd,FALSE);
                        return false;
                    }
                    if (imsg.Code == SELECTDOWN && !page->pg_CurrentPoint)
                        AddCreateObjectPoint(page,imsg.MouseX-ox,imsg.MouseY-oy,ox,oy);

                    AddCreateObjectPoint(page,imsg.MouseX-ox,imsg.MouseY-oy,ox,oy);
                    return true;
                }
                else if (imsg.Code == MENUDOWN)        // finish or abort
                {
                    if (page->pg_NumPoints == ~0L && page->pg_CurrentPoint > 2)
                        DoCreateObject(page);

                    FreeCreateObject(page,wd,TRUE);

                    imsg.Code = MENUUP;
                    return true;  // prevent cells to be selected
                }
                break;
            }
            case IDCMP_MOUSEMOVE:
            {
                struct coord *c;

                if ((c = page->pg_Points) && page->pg_CurrentPoint)
                {
                    long ox = page->pg_wTabX-page->pg_TabX, oy = page->pg_wTabY-page->pg_TabY;
                    long mode = GCPAM_UPDATE;

                    SetGRastPort(page->pg_Window->RPort);
                    makeClip(page->pg_Window,page->pg_wTabX,page->pg_wTabY,page->pg_wTabW,page->pg_wTabH);
                    SetDrMd(grp,COMPLEMENT);

                    if (page->pg_Action & PGA_REFRESH)
                    {
                        page->pg_Action &= ~PGA_REFRESH;
                        mode = GCPAM_REDRAW;
                    }
                    else
                        gDoClassMethod(page->pg_CreateFromClass,NULL,GCM_ADDPOINT,grp,
                                                     c,(ox << 16) | (oy & 0xffff),page->pg_CurrentPoint,GCPAM_UPDATE);

                    c[page->pg_CurrentPoint-1].x = imsg.MouseX-ox;
                    c[page->pg_CurrentPoint-1].y = imsg.MouseY-oy;

                    gDoClassMethod(page->pg_CreateFromClass,NULL,GCM_ADDPOINT,grp,
                                                 c,(ox << 16) | (oy & 0xffff),page->pg_CurrentPoint,mode);
                    SetDrMd(grp,JAM2);
                    freeClip(page->pg_Window);
                }
                break;
            }
        }
    }
    else if (page->pg_Action & PGA_MULTISELECT)
    {
        switch (imsg.Class)
        {
            case IDCMP_MOUSEBUTTONS:
                if (imsg.Code & IECODE_UP_PREFIX)
                    wd->wd_ExtData[6] = NULL;

                if (imsg.Code == MENUDOWN)
                {
                    DrawMultiSelectFrame(page,DMSF_ALL);
                    imsg.Code = MENUUP;
                }
                else if (imsg.Code == SELECTUP)
                {
                    if (!(page->pg_Action & PGA_REFRESH))
                        DrawMultiSelectFrame(page,DMSF_ALL);
                    if (abs(fx-fw) < 2)
                        page->pg_HotSpot = PGHS_CELL;
                    else
                    {
                        struct gGroup *gg;
                        BYTE   changed = FALSE;

                        if (fw < fx)
                            swmem((UBYTE *)&fx, (UBYTE *)&fw, sizeof(fw));
                        if (fh < fy)
                            swmem((UBYTE *)&fy, (UBYTE *)&fh, sizeof(fh));
                        for(gg = (APTR)page->pg_gGroups.mlh_Head;gg->gg_Node.mln_Succ;gg = (APTR)gg->gg_Node.mln_Succ)
                        {
                            if (fx <= gg->gg_Right && fy <= gg->gg_Bottom && fw >= gg->gg_Left && fh >= gg->gg_Top)
                                gg->gg_Flags |= GOF_PROTECTED, changed = TRUE;
                        }
                        DeselectGGroups(page);
                        if (changed)
                        {
                            foreach(&page->pg_gGroups,gg)
                            {
                                if (gg->gg_Flags & GOF_PROTECTED)
                                    SelectGGroup(page,gg,0);
                            }
                        }
                    }
                }
                break;
            case IDCMP_MOUSEMOVE:
            {
                BOOL first = FALSE;

                if (page->pg_Action & PGA_REFRESH)
                {
                    page->pg_Action &= ~PGA_REFRESH;
                    msfptrn = 0x00ff;  first = TRUE;
                }
                else
                    DrawMultiSelectFrame(page,DMSF_BEGIN);

                fw = imsg.MouseX-page->pg_wTabX+page->pg_TabX;
                fh = imsg.MouseY-page->pg_wTabY+page->pg_TabY;
                DrawMultiSelectFrame(page,first ? DMSF_ALL : DMSF_END);
                break;
            }
            case IDCMP_INTUITICKS:
                if (!(page->pg_Action & PGA_REFRESH))
                {
                    WaitTOF();
                    DrawMultiSelectFrame(page,DMSF_BEGIN);
                    msfptrn = (msfptrn >> 2) | (msfptrn & 1 ? (3 << 14) : 0);
                    DrawMultiSelectFrame(page,DMSF_END);
                }
                break;
        }
    }
    else if (page->pg_Action & PGA_CHANGE)
    {
        switch (imsg.Class)
        {
            case IDCMP_MOUSEBUTTONS:
                if (page->pg_Action & (PGA_MOVE | PGA_KNOBS))
                    DrawGObjectMove(page);

                if (imsg.Code == SELECTUP && (fx || fy))    // process frame changes
                {
                    struct UndoNode *un;
                    struct gObject *go;
					long i, x, y;

					x = mm(page, fx, TRUE);
					y = mm(page, fy, FALSE);

                    if (page->pg_Action & PGA_MOVE)
                    {
						if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_MOVE_OBJECTS_UNDO))) != 0)
                        {
							un->un_Type = UNT_OBJECTS_MOVE;
							un->un_MoveDeltaX = x;
							un->un_MoveDeltaY = y;

							foreach (&page->pg_gObjects, go)
                            {
                                if (go->go_Flags & GOF_SELECTED)
									AddUndoLink(&un->un_UndoList, go);
                            }
							ApplyObjectsMoveUndoRedo(page, un, TYPE_REDO);
                        }
                        else
                            ErrorRequest(GetString(&gLocaleInfo, MSG_PERFORM_ACTION_UNDO_FAILED_ERR));
                    }
                    else if (page->pg_Action & PGA_KNOBS)
                    {
                        go = page->pg_ChangeObject;
                        i = page->pg_CurrentPoint;

                        if (x != go->go_Knobs[i].x || y != go->go_Knobs[i].y)
                        {
							if ((un = CreateUndo(page, UNDO_PRIVATE, GetString(&gLocaleInfo, MSG_OBJECT_SIZE_UNDO))) != 0)
                            {
								un->un_Type = UNT_OBJECT_KNOB;
								un->un_Object = go;
								un->un_PointNumber = i;
								un->un_UndoPoint = go->go_Knobs[i];
								un->un_RedoPoint.x = x;
								un->un_RedoPoint.y = y;
                            }
							gDoMethod(go, GCM_UPDATEPOINT, x, y, i);
							RefreshGObjectDrawing(page, go);
                            if (go->go_Window)
                                UpdateObjectGadgets(go->go_Window);
                        }
                    }
                }
                else if (imsg.Code == MENUDOWN)              // abort object move/size
                    imsg.Code = MENUUP;

                FreeObjectChange(page);
                break;
            case IDCMP_MOUSEMOVE:                  // resize/move frame(s)
                if (page->pg_Action & (PGA_MOVE | PGA_KNOBS) && !(page->pg_Action & PGA_REFRESH))
                    DrawGObjectMove(page);
                else if (page->pg_Action == PGA_CHANGE)
                    page->pg_Action |= PGA_MOVE;

                page->pg_Action &= ~PGA_REFRESH;

                if (page->pg_Action & PGA_MOVE)
                {
					fx = imsg.MouseX + page->pg_TabX - page->pg_wTabX - ofx;
					fy = imsg.MouseY + page->pg_TabY - page->pg_wTabY - ofy;
                }
                else if (page->pg_Action & PGA_KNOBS)
                {
					ofx = page->pg_wTabX - page->pg_TabX;
					ofy = page->pg_wTabY - page->pg_TabY;
					fx = imsg.MouseX - ofx;
					fy = imsg.MouseY - ofy;
                }
                DrawGObjectMove(page);
                break;
        }
    }
    else if (imsg.Class == IDCMP_MOUSEBUTTONS)
    {
        switch (imsg.Code)
        {
            case SELECTDOWN:
            {
                struct gGroup *gg;

				if (!(imsg.MouseX > page->pg_wTabX && imsg.MouseX < page->pg_wTabX + page->pg_wTabW
					&& imsg.MouseY > page->pg_wTabY && imsg.MouseY < page->pg_wTabY + page->pg_wTabH))
					return false;

				foreach (&page->pg_gGroups,gg)
                {
					if (gg->gg_Flags & GOF_SELECTED
						&& (page->pg_Action = CheckGGroup(page, gg, imsg.MouseX - page->pg_wTabX + page->pg_TabX,
								imsg.MouseY - page->pg_wTabY + page->pg_TabY)))
                    {
						SelectGGroup(page, gg, !(imsg.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) ? ACTGO_EXCLUSIVE : 0);
                        break;
                    }
                }
                if (!gg->gg_Node.mln_Succ)
                    gg = NULL;

                if (!gg)
                {
                    long   pos;

                    page->pg_Action = PGA_NONE;
					if ((gg = MouseGGroup(page, &pos)) != 0)
                    {
                        struct gObject *go = GROUPOBJECT(gg);

                        if (page->pg_Mappe->mp_Flags & MPF_SCRIPTS)  /* interaktiver Modus */
                        {
							SetProjectMouseReport(page, FALSE);
                            page->pg_ChangeObject = go;
							DoGObjectAction(page, go, SELECTDOWN);
							return true;
                        }
                        else
                        {
                            SelectGGroup(page,gg,!(imsg.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) ? ACTGO_EXCLUSIVE : 0);

                            page->pg_HotSpot = PGHS_OBJECT;
                            if (IsDoubleClick(pos))
                            {
                                SetProjectMouseReport(page,FALSE);
                                wd->wd_ExtData[6] = NULL;
                                OpenGObjectWindow(go);
                                return true;
                            }
                            else
                                page->pg_Action = PGA_CHANGE;
                        }
                    }
                }
                if (gg && page->pg_Action & PGA_CHANGE)          // create UndoNode
                {
                    if (page->pg_Action & PGA_KNOBS)
                        page->pg_Action |= PGA_REFRESH;

                    ofx = imsg.MouseX+page->pg_TabX-page->pg_wTabX;
                    ofy = imsg.MouseY+page->pg_TabY-page->pg_wTabY;
                    fx = 0;  fy = 0;  fw = 0;  fh = 0;

                    return true;
                }
                if (page->pg_HotSpot == PGHS_OBJECT && !(page->pg_Mappe->mp_Flags & MPF_SCRIPTS))
                {
                    page->pg_Action = PGA_MULTISELECT | PGA_REFRESH;
					fx = fw = imsg.MouseX - page->pg_wTabX + page->pg_TabX;
					fy = fh = imsg.MouseY - page->pg_wTabY + page->pg_TabY;

					SetMousePointer(win, POINTER_OBJECT);
                    return true;
                }
                break;
            }
            case MENUDOWN:
                imsg.Code = MENUUP;
                break;
            case SELECTUP:
                if (page->pg_Mappe->mp_Flags & MPF_SCRIPTS)   // Interaktiver Modus
                {
					DoGObjectAction(page, page->pg_ChangeObject, SELECTUP);
                    page->pg_ChangeObject = NULL;
                    return true;
                }
                break;
        }
    }
    else if (page->pg_Mappe->mp_Flags & MPF_SCRIPTS)
    {
        struct gObject *go = page->pg_ChangeObject;

        if (!go)
            return false;

        if (!(imsg.Qualifier & IEQUALIFIER_LEFTBUTTON))
        {
            if (go->go_Flags & GOF_PRESSED)
				DoGObjectAction(page, go, SELECTUP);

            page->pg_ChangeObject = NULL;
            page->pg_HotSpot = PGHS_OBJECT;
			return false;
        }
        if (imsg.Class == IDCMP_INTUITICKS)
        {
            static int ticks;

            if (++ticks % 3)
                return true;

            if ((go->go_Flags & (GOF_CONTINUALCMD | GOF_PRESSED)) == (GOF_CONTINUALCMD | GOF_PRESSED))
            {
				gCommandExecuted = TRUE;
				gDoMethod(go, GCM_COMMAND);
            }
        }
        else if (imsg.Class == IDCMP_MOUSEMOVE)
        {
            ULONG pressed = 0;

			if (CheckGObject(page, go, page->pg_TabX + imsg.MouseX - page->pg_wTabX, page->pg_TabY + imsg.MouseY - page->pg_wTabY))
                pressed = GOF_PRESSED;

            if (pressed != (go->go_Flags & GOF_PRESSED))
            {
                go->go_Flags = (go->go_Flags & ~GOF_PRESSED) | pressed;
				DrawSingleGObject(page, go);
            }
        }
    }

    return false;
}


static void
gObjectsScreenFor(struct Mappe *mp, ULONG methodID, struct Screen *screen)
{
    struct gObject *go;
    struct Page *pg;

	foreach (&mp->mp_Pages, pg)
	{
		foreach (&pg->pg_gObjects, go)
			gDoMethod(go, methodID, scr);
	}
}

 
static void
gObjectsScreen(struct Mappe *mp, ULONG methodID, struct Screen *screen)
{
	if (mp == NULL) {
		foreach (&gProjects, mp) {
			gObjectsScreenFor(mp, methodID, screen);
		}
	} else
		gObjectsScreenFor(mp, methodID, screen);
}


void
gRemoveObjectsFromScreen(struct Mappe *mp, struct Screen *scr)
{
	gObjectsScreen(mp, GCM_REMOVE_FROM_SCREEN, scr);
}


void
gAddObjectsToScreen(struct Mappe *mp, struct Screen *scr)
{
	gObjectsScreen(mp, GCM_ADD_TO_SCREEN, scr);
}


