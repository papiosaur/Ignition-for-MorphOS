/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	draw routines.


#include "TextEdit_includes.h"
#include "compatibility.h"

void PRIVATE DrawEditCursor(struct ClassBase *cb,struct RastPort *rp,struct Gadget *gad,struct EditGData *ed)
{
  long cx,cy,cw,mx,my,i,j,by,ox,oy;

  GetEditCursorCoord(cb,rp,ed,ed->ed_Pos,&cx,&cy,&cw);
  ox = gad->LeftEdge+ed->ed_BorderH;
  oy = gad->TopEdge+ed->ed_BorderV;
  by = ed->ed_Lines*ed->ed_LineHeight;

  if (ed->ed_Pos == ed->ed_MarkPos)
  {
    if (cy >= 0 && cy < by)
    {
      SetDrMd(rp,COMPLEMENT);
      RectFill(rp,cx+ox,cy+oy,cx+ox+cw-1,cy+oy+ed->ed_LineHeight-1);
      SetDrMd(rp,JAM2);
    }
  }
  else
  {
    GetEditCursorCoord(cb,rp,ed,ed->ed_MarkPos,&mx,&my,&i);
    if (cy > my || cy == my && cx > mx)
    {
      swmem((UBYTE *)&cy,(UBYTE *)&my,sizeof(long));
      swmem((UBYTE *)&cx,(UBYTE *)&mx,sizeof(long));
    }
    else
      swmem((UBYTE *)&cw,(UBYTE *)&i,sizeof(long));

    SetDrMd(rp,COMPLEMENT);
    if (cy == my && cy >= 0 && cy < by)
      RectFill(rp,cx+ox,cy+oy,mx+cw+ox-1,my+oy+ed->ed_LineHeight-1);
    else
    {
      if (cy >= 0 && cy < by)
        RectFill(rp,cx+ox,cy+oy,ed->ed_Width+ed->ed_CharWidth-1+ox,cy+oy+ed->ed_LineHeight-1);
      for(i = j = cy+ed->ed_LineHeight;j < my;j += ed->ed_LineHeight);
      if (i != j && i < by && j > 0)
      {
        i = max(i,0);
        j = min(j,by);
        RectFill(rp,ox,i+oy,ox-1+ed->ed_Width+ed->ed_CharWidth,j+oy-1);
      }
      if (my >= 0 && my < by)
         RectFill(rp,ox,my+oy,mx-1+cw+ox,my+oy+ed->ed_LineHeight-1);
    }
    SetDrMd(rp,JAM2);
  }
}


void PRIVATE DrawEditGadget(struct ClassBase *cb,struct RastPort *rp,struct Gadget *gad,struct GadgetInfo *gi,struct EditGData *ed,BOOL all)
{
  struct MinNode *mln;
  struct EditLine *el;
  long   x,y,i,base;

  if (all)
  {
    struct Image *im;

    if ((im = NewObject(NULL,"frameiclass",IA_Width,     ed->ed_GadWidth,
                                          IA_Height,    gad->Height,
                                          IA_FrameType, ed->ed_FrameType,
                                          IA_EdgesOnly, TRUE,
                                          TAG_END)) != 0)
    {
      DrawImage(rp,im,gad->LeftEdge,gad->TopEdge);
      DisposeObject(im);
    }
  }
  y = gad->TopEdge+ed->ed_BorderV+(base = rp->Font->tf_Baseline);
  SetABPenDrMd(rp,ed->ed_APen,ed->ed_BPen,JAM2);
  for(i = 0,mln = ed->ed_Top;mln->mln_Succ && i < ed->ed_Lines;mln = mln->mln_Succ,i++)
  {
    x = gad->LeftEdge+ed->ed_BorderH;
    EraseRect(rp,x-1,y-base,x+LINEOFFSET(mln)-1,y-base+ed->ed_LineHeight-1);
    x += LINEOFFSET(mln);

    el = EDITLINE(mln);
    for(;el->el_Word;el++)
    {
      if (el->el_Type == ELT_WORD)
      {
        Move(rp,x,y);
        Text(rp,el->el_Word,el->el_Length);
      }
      else
      {
        EraseRect(rp,x,y-base,x+el->el_Width-1,y-base+ed->ed_LineHeight-1);
        if (ed->ed_Flags & EDF_SPECIAL)
        {
          switch(el->el_Type)
          {
            case ELT_SPACE:
            {
              long j,p,w = el->el_Width/el->el_Length;
              for(j = 0,p = x;j < el->el_Length;j++,p += w)
                WritePixel(rp,p+w/2,y-base/2+1);
              break;
            }
            case ELT_TAB:
              Move(rp,x+1,y-base/2-1);
              Draw(rp,x+1,y-base/2+3);
              Move(rp,x+1,y-base/2+1);
              Draw(rp,x+el->el_Width-2,y-base/2+1);
              Move(rp,x+el->el_Width-2,y-base/2-1);
              Draw(rp,x+el->el_Width-2,y-base/2+3);
              break;
            case ELT_NEWLINE:
              Move(rp,x,y);
              Text(rp,"¶",1);
              break;
          }
        }
      }
      x += el->el_Width;
    }
    EraseRect(rp,x,y-base,gad->LeftEdge+ed->ed_GadWidth-ed->ed_BorderH+1,y-base+ed->ed_LineHeight-1);
    y += ed->ed_LineHeight;
  }
  EraseRect(rp,gad->LeftEdge+ed->ed_BorderH-1,y-base,gad->LeftEdge+ed->ed_GadWidth-ed->ed_BorderH+1,gad->TopEdge+gad->Height-ed->ed_BorderV-1);
  if (ed->ed_Flags & EDF_ACTIVE)
    DrawEditCursor(cb,rp,gad,ed);

  if (gad->Flags & GFLG_DISABLED)
  {
    ULONG ghostPtrn = 0x44441111;

    SetABPenDrMd(rp,ed->ed_APen,ed->ed_BPen,JAM1);
    rp->AreaPtrn = (unsigned short *)&ghostPtrn;
    rp->AreaPtSz = 1;
    x = gad->LeftEdge+4;
    y = gad->TopEdge+2;
    RectFill(rp,x,y,x+ed->ed_GadWidth-10,y+gad->Height-5);
    rp->AreaPtrn = NULL;
    rp->AreaPtSz = 0;
  }
  if (ed->ed_Scroller && gi)
  {
    long   top = 0,i;
    static long total;

    for(mln = ed->ed_List.mlh_Head;mln->mln_Succ && mln != ed->ed_Top;top++,mln = mln->mln_Succ);
    if (GetAttr(PGA_Top,(Object *)ed->ed_Scroller,(IPTR *)&i) && i == top)
    {
      if (total == ed->ed_TextLines)
        return;
    }
    total = ed->ed_TextLines;
    SetAttrs(ed->ed_Scroller,PGA_Total,   total,
                             PGA_Top,     top,
                             TAG_END);
    DoMethod((APTR)ed->ed_Scroller,GM_RENDER,gi,rp,GREDRAW_REDRAW);
  }
}
