/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	support functions.


#include "TextEdit_includes.h"


int PRIVATE CountNodes(struct List *l)
{
  struct Node *n;
  int i = 0;

  if (!l)
    return(0);
  for(n = l->lh_Head;n->ln_Succ;n = n->ln_Succ,i++);
  return(i);
}


void PRIVATE strdel(STRPTR t,long len)
{
  STRPTR s;

  for(s = t+len;*s;t++,s++)
    *t = *s;
  *t = 0;
}


LIBFUNC VOID FreeEditList(REG(a0, struct EditGData * ed))
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


void PRIVATE
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


LIBFUNC BOOL PrepareEditText(
  REG(a0, struct EditGData * ed),
  REG(a1, struct RastPort * rp),
  REG(a2, STRPTR t))
{
  LIBFUNC_INIT

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
  
  LIBFUNC_EXIT
}


long PRIVATE GetEditCursorPos(struct ClassBase *cb,struct RastPort *rp,struct EditGData *ed,long x,long y)
{
  struct MinNode *mln;
  struct EditLine *el;
  long   i;

  if (IsListEmpty((struct List *)&ed->ed_List))
    return(0);

  i = (y-ed->ed_BorderV)/ed->ed_LineHeight;
  for(mln = ed->ed_Top;i-- && mln->mln_Succ->mln_Succ;mln = mln->mln_Succ);

  el = EDITLINE(mln);
  for(x -= ed->ed_BorderH,i = LINEOFFSET(mln);el->el_Word;el++)
  {
    if (x < i+el->el_Width)
      break;
    i += el->el_Width;
  }
  if (!el->el_Word) { /* last character */
    --el;
    return --el->el_Length - 1 + el->el_Word-ed->ed_Text;
  } else {             /* character in word */
    struct TextExtent extent;
    ULONG  fit;

    x -= 1+i;
    if (el->el_Type == ELT_WORD)
      fit = TextFit(rp,el->el_Word,el->el_Length,&extent,NULL,1,x,ed->ed_LineHeight+1);
    else if (el->el_Type == ELT_SPACE || el->el_Type == ELT_TAB)
      fit = x/(el->el_Width/el->el_Length);
    else
      fit = 0;
    return(fit+el->el_Word-ed->ed_Text);
  }
}


struct MinNode * PRIVATE GetEditCursorLine(struct EditGData *ed,ULONG pos,long *line)
{
  struct MinNode *mln;

  *line = ed->ed_TextLines-1;
  pos += (ULONG)ed->ed_Text;

  for(mln = ed->ed_List.mlh_TailPred;mln->mln_Pred && (ULONG)EDITLINE(mln)->el_Word > pos;mln = mln->mln_Pred,(*line)--);
  if (mln->mln_Pred)
    return(mln);
  return(NULL);
}


void PRIVATE GetEditCursorCoord(struct ClassBase *cb,struct RastPort *rp,struct EditGData *ed,ULONG pos,long *x,long *y,long *width)
{
  struct MinNode *mln;
  long   line = ed->ed_TextLines-1,top;

  if (!*ed->ed_Text)
  {
    *x = 0;  *y = 0;
    if (*width)
      *width = ed->ed_CharWidth;
    return;
  }
  pos += (ULONG)ed->ed_Text;
  for(top = 0,mln = ed->ed_List.mlh_Head;mln->mln_Succ && ed->ed_Top != mln;mln = mln->mln_Succ,top++);
  for(mln = ed->ed_List.mlh_TailPred;mln->mln_Pred && (ULONG)EDITLINE(mln)->el_Word > pos;mln = mln->mln_Pred,line--);
  *y = (line-top)*ed->ed_LineHeight;

  if (*width)
    *width = ed->ed_CharWidth;
  if (mln->mln_Pred)
  {
    struct EditLine *el = EDITLINE(mln);
    long   w;

    for(w = LINEOFFSET(mln);el->el_Word && (ULONG)el->el_Word+el->el_Length <= pos;w += (el++)->el_Width);
    if (el->el_Word)
    {
      if (el->el_Type == ELT_WORD)
      {
        if (width && *(STRPTR)pos != 0 && *(STRPTR)pos != '\n')
          *width = TextLength(rp,(STRPTR)pos,1);
        *x = w+TextLength(rp,el->el_Word,pos-(ULONG)el->el_Word);
      }
      else if (el->el_Type == ELT_SPACE || el->el_Type == ELT_TAB)
      {
        if (width)
          *width = ed->ed_MinSpace;
        w += (el->el_Width/el->el_Length)*(pos-(ULONG)el->el_Word);
        if (w > ed->ed_Width)
          w = ed->ed_Width;
        *x = w;
      }
      else
      {
        *x = w;
        if (width)
          *width = el->el_Width;
      }
    }
    else
      *x = w;
  }
  else
    *x = 0;
}


BOOL PRIVATE MakeEditCursorVisible(struct ClassBase *cb,struct RastPort *rp,struct Gadget *gad,struct GadgetInfo *gi,struct EditGData *ed)
{
  struct MinNode *cursor,*mln;
  long   line,top;

  if ((cursor = GetEditCursorLine(ed,ed->ed_Pos,&line)) != 0)
  {
    for(top = 0,mln = ed->ed_List.mlh_Head;mln->mln_Succ && mln != ed->ed_Top;mln = mln->mln_Succ,top++);
    if (line < top || line >= top+ed->ed_Lines)
    {
      if (line < top)
        ed->ed_Top = cursor;
      else if (line >= top+ed->ed_Lines)
      {
        for(mln = ed->ed_Top;mln->mln_Succ->mln_Succ && top+ed->ed_Lines <= line;mln = mln->mln_Succ,top++);
        ed->ed_Top = mln;
      }
      DrawEditGadget(cb,rp,gad,gi,ed,FALSE);
      return(TRUE);
    }
  }
  return(FALSE);
}


BOOL PRIVATE MakeEditScroller(struct ClassBase *cb,struct EditGData *ed,struct Gadget *gad,struct Gadget *previous)
{
  if (!previous)
    return(FALSE);

  if (!cb->cb_ScrollerBase)
  {
    if (!(cb->cb_ScrollerBase = OpenLibrary("gadgets/pScroller.gadget",0)))
      cb->cb_ScrollerBase = OpenLibrary("pScroller.gadget",0);
  }
  if (!cb->cb_ScrollerBase)
    return(FALSE);

  if (!(ed->ed_Scroller = NewObject(NULL,"pinc-scrollergadget",GA_ID,         113,
                                                               GA_Left,       gad->LeftEdge+gad->Width-16,
                                                               GA_Top,        gad->TopEdge,
                                                               GA_Width,      16,
                                                               GA_Height,     gad->Height,
                                                               GA_Previous,   previous,
                                                               PGA_Borderless,TRUE,
                                                               ICA_TARGET,    gad,
                                                               TAG_END)))
    return(FALSE);
  ed->ed_Scroller->NextGadget = gad;
  return(TRUE);
}


void PRIVATE SetEditBuffer(struct ClassBase *cb,struct EditGData *ed,long newsize)
{
  STRPTR temp;

  if ((temp = AllocPooled(ed->ed_Pool,newsize)) != 0)
  {
    CopyMem(ed->ed_Text,temp,min(ed->ed_Size,newsize));
    if (ed->ed_Text)
      FreePooled(ed->ed_Pool,ed->ed_Text,ed->ed_Size);
    ed->ed_Size = newsize;
    ed->ed_Text = temp;
  }
}
