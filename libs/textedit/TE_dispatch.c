/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	class dispatcher


#include "TextEdit_includes.h"
#include "compatibility.h"

ULONG PRIVATE DispatchEditHandleRawKey(struct ClassBase *cb,struct EditGData *ed,struct gpInput *gpi,struct Gadget *gad,struct InputEvent *ie)
{
  struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);
  struct MinNode *mln = NULL;
  long   line,pos = 0;
  ULONG  retval;

  retval = GMR_MEACTIVE;
  switch(ie->ie_Code)
  {
    case CURSORUP:
    case CURSORDOWN:
      if ((mln = GetEditCursorLine(ed,ed->ed_Pos,&line)) != 0)
        pos = ed->ed_Text+ed->ed_Pos-EDITLINE(mln)->el_Word;
    case CURSORLEFT:
    case CURSORRIGHT:
      if (!*ed->ed_Text)
        break;
      DrawEditCursor(cb,rp,gad,ed);
      if (ie->ie_Qualifier & (IEQUALIFIER_RALT | IEQUALIFIER_LALT))
      {
        if (mln)
        {
          if (ie->ie_Code == CURSORUP)
            mln = ed->ed_List.mlh_Head;
          else
            mln = ed->ed_List.mlh_TailPred;
        }
        else if ((mln = GetEditCursorLine(ed,ed->ed_Pos,&line)) != 0)
        {
          struct EditLine *el,*fel = EDITLINE(mln);

          pos = (ULONG)ed->ed_Text+ed->ed_Pos;

          if (ie->ie_Code == CURSORLEFT)
          {
            for(el = fel;el->el_Word && pos > (ULONG)(el+1)->el_Word;el++);
            while(fel != el && el->el_Type != ELT_WORD)
              el--;
            if (fel == el && ed->ed_Pos == el->el_Word-ed->ed_Text) /* first word in line */
            {
              if (mln->mln_Pred->mln_Pred)  /* previous line */
              {
                mln = mln->mln_Pred;
                for(el = EDITLINE(mln);(el+1)->el_Word;el++);
                ed->ed_Pos = el->el_Word-ed->ed_Text;
              }
              else                          /* already on the top */
                ed->ed_Pos = 0;
            }
            else if (el->el_Word)
              ed->ed_Pos = el->el_Word-ed->ed_Text;
            else if (fel != el)
              ed->ed_Pos = (el-1)->el_Word-ed->ed_Text;
            else
              ed->ed_Pos = 0;
          }
          else if (ie->ie_Code == CURSORRIGHT)
          {
            for(el = fel;el->el_Word && pos >= (ULONG)(el+1)->el_Word;el++);
            if (el->el_Word)
              el++;
            while(el->el_Word && el->el_Type != ELT_WORD)
              el++;
            if (!el->el_Word)
            {
              if (mln->mln_Succ->mln_Succ)  /* next line */
              {
                mln = mln->mln_Succ;
                ed->ed_Pos = EDITLINE(mln)->el_Word-ed->ed_Text;
              }
              else                          /* already in the last line */
                ed->ed_Pos = strlen(ed->ed_Text);
            }
            else
              ed->ed_Pos = el->el_Word-ed->ed_Text;
          }
        }
      }
      else if (ie->ie_Qualifier & (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT))
      {
        if (mln)
        {
          line = 0;
          if (ie->ie_Code == CURSORUP)
            for(;mln->mln_Pred->mln_Pred && line < ed->ed_Lines-1;mln = mln->mln_Pred,line++);
          else
            for(;mln->mln_Succ->mln_Succ && line < ed->ed_Lines-1;mln = mln->mln_Succ,line++);
        }
        else if ((mln = GetEditCursorLine(ed,ed->ed_Pos,&line)) != 0)
        {
          if (ie->ie_Code == CURSORLEFT)
            ed->ed_Pos = EDITLINE(mln)->el_Word-ed->ed_Text;
          else if (ie->ie_Code == CURSORRIGHT)
          {
            if (mln->mln_Succ->mln_Succ)
              ed->ed_Pos = EDITLINE(mln->mln_Succ)->el_Word-ed->ed_Text-1;
            else
              ed->ed_Pos = strlen(ed->ed_Text);
          }
        }
      }
      else if (mln)
      {
        if (ie->ie_Code == CURSORUP)
          mln = mln->mln_Pred;
        else
          mln = mln->mln_Succ;
      }
      else if (ie->ie_Code == CURSORLEFT)
      {
        ed->ed_Pos--;
        if (ed->ed_Pos < 0)
          ed->ed_Pos = 0;
      }
      else if (ie->ie_Code == CURSORRIGHT)
      {
        ed->ed_Pos++;
        if (ed->ed_Pos > strlen(ed->ed_Text))
          ed->ed_Pos = strlen(ed->ed_Text);
      }
      if ((ie->ie_Code == CURSORUP || ie->ie_Code == CURSORDOWN) && mln->mln_Succ && mln->mln_Pred)
      {
        struct EditLine *el = EDITLINE(mln);

        pos += (ULONG)EDITLINE(mln)->el_Word;
        for(;(el+1)->el_Word;el++);
        if ((ULONG)el->el_Word+el->el_Length < pos)
          pos = (ULONG)el->el_Word+el->el_Length-1;
        ed->ed_Pos = pos-(ULONG)ed->ed_Text;
      }
      if (!(ie->ie_Qualifier & IEQUALIFIER_CONTROL))
        ed->ed_MarkPos = ed->ed_Pos;

      DrawEditCursor(cb,rp,gad,ed);
      MakeEditCursorVisible(cb,rp,gad,gpi->gpi_GInfo,ed);
      break;
    default:
      if (!(ie->ie_Code & IECODE_UP_PREFIX))
      {
        char c[5];
        long top;

        if (RawKeyConvert(ie,c,5,NULL) > 0)
        {
          struct MinNode *mln;

          for(top = 0,mln = ed->ed_List.mlh_Head;mln->mln_Succ && mln != ed->ed_Top;top++,mln = mln->mln_Succ);

          if (ie->ie_Qualifier & IEQUALIFIER_RCOMMAND)  /* do specials */
          {
            long len = abs(ed->ed_MarkPos-ed->ed_Pos)+1;
            long pos = min(ed->ed_MarkPos,ed->ed_Pos);

            switch(c[0])
            {
              case 'c':
                DrawEditCursor(cb,rp,gad,ed);
                Text2Clipboard(ed->ed_ClipUnit,ed->ed_Text+pos,len);
                ed->ed_Pos = ed->ed_MarkPos;
                DrawEditCursor(cb,rp,gad,ed);
                break;
              case 'v':
              {
                STRPTR t;
                long   len;

                if ((t = TextFromClipboard(ed->ed_ClipUnit,ed->ed_Pool)) != 0)
                {
                  FreeEditList(ed);
                  if ((len = strlen(t)+1) > ed->ed_Size-strlen(ed->ed_Text))
                    SetEditBuffer(cb,ed,ed->ed_Size+len);
                  strins(ed->ed_Text+ed->ed_Pos,t);
                  ed->ed_Pos += len-1;
                  FreePooled(ed->ed_Pool,t,len);
                }
                else
                  DisplayBeep(NULL);
                break;
              }
              case 'x':
                FreeEditList(ed);
                if (len < 2)
                {
                  Text2Clipboard(ed->ed_ClipUnit,ed->ed_Text,strlen(ed->ed_Text));
                  *ed->ed_Text = 0;
                  ed->ed_Pos = 0;
                }
                else
                {
                  Text2Clipboard(ed->ed_ClipUnit,ed->ed_Text+pos,len);
                  strdel(ed->ed_Text+pos,len);
                  ed->ed_Pos = pos;
                }
                break;
            }
          }
          else if (!(ie->ie_Qualifier & IEQUALIFIER_LCOMMAND))
          {
            if (ed->ed_Size < strlen(ed->ed_Text)+2)
              SetEditBuffer(cb,ed,ed->ed_Size+256);
            switch(c[0])
            {
              case 13:   /* Return */
                FreeEditList(ed);
                if (ed->ed_Flags & EDF_AUTOINDENT)
                {
                  int  spaces = 0,pos,i;
                  char ch;

                  for(pos = ed->ed_Pos;pos && *(ed->ed_Text+pos) != '\n';pos--);

                  if (!pos)
                    strins(ed->ed_Text+ed->ed_Pos++,"\n");
                  else
                    spaces++;

                  for(;(ch = *(ed->ed_Text+pos+spaces)) != '\n' && isspace(ch) && spaces+pos < ed->ed_Pos;spaces++);
                      // Spaces zählen

                  while(ed->ed_Size < strlen(ed->ed_Text)+spaces)         // Buffer erweitern
                    SetEditBuffer(cb,ed,ed->ed_Size+256);

                  for(i = strlen(ed->ed_Text);i >= ed->ed_Pos;i--)        // Text nach hinten verschieben
                    ed->ed_Text[i+spaces] = ed->ed_Text[i];

                  strncpy(ed->ed_Text+ed->ed_Pos,ed->ed_Text+pos,spaces); // Abstand kopieren
                  ed->ed_Pos += spaces;
                }
                else
                  strins(ed->ed_Text+ed->ed_Pos++,"\n");
                break;
              case 27:   /* Escape */
                retval = GMR_NOREUSE /*| GMR_VERFIY*/;
                break;
              case 8:    /* Backspace */
                if (!ed->ed_Pos)
                  break;
                if (ie->ie_Qualifier & (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT | IEQUALIFIER_LALT | IEQUALIFIER_RALT))
                {
                  if ((mln = GetEditCursorLine(ed,ed->ed_Pos,&line)) != 0)
                  {
                    struct EditLine *el = EDITLINE(mln);

                    if (ie->ie_Qualifier & (IEQUALIFIER_RALT | IEQUALIFIER_LALT))
                    {
                      for(;el->el_Word && el->el_Word+el->el_Length < ed->ed_Pos+ed->ed_Text;el++);
                    }
                    if (el->el_Word)
                    {
                      if (el->el_Word < ed->ed_Text+ed->ed_Pos)
                      {
                        strdel(el->el_Word,ed->ed_Text+ed->ed_Pos-el->el_Word);
                        ed->ed_Pos = el->el_Word-ed->ed_Text;
                      }
                      else
                      {
                        ed->ed_Pos--;
                        strdel(ed->ed_Text+ed->ed_Pos,1);
                      }
                    }
                    FreeEditList(ed);
                    break;
                  }
                }
                else
                {
                  FreeEditList(ed);
                  if (ed->ed_Pos > ed->ed_MarkPos)
                  {
                    strdel(ed->ed_Text+ed->ed_MarkPos,ed->ed_Pos-ed->ed_MarkPos+1);
                    ed->ed_Pos = ed->ed_MarkPos;
                  }
                  else
                  {
                    ed->ed_Pos--;
                    strdel(ed->ed_Text+ed->ed_Pos,1);
                  }
                }
                break;
              case 127:  /* Delete */
                if (!(*(ed->ed_Text+ed->ed_Pos)))
                  break;
                if (ie->ie_Qualifier & (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT | IEQUALIFIER_LALT | IEQUALIFIER_RALT))
                {
                  if ((mln = GetEditCursorLine(ed,ed->ed_Pos,&line)) != 0)
                  {
                    struct EditLine *el = EDITLINE(mln);
                    STRPTR to = NULL,from = ed->ed_Text+ed->ed_Pos;

                    if (ie->ie_Qualifier & (IEQUALIFIER_RALT | IEQUALIFIER_LALT))
                    {
                      for(;el->el_Word && el->el_Word <= ed->ed_Pos+ed->ed_Text;el++);
                      if (el->el_Word)
                        to = el->el_Word;
                      else
                        to = ed->ed_Text+strlen(ed->ed_Text);
                    }
                    else if (ie->ie_Qualifier & (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT))
                    {
                      if (mln->mln_Succ->mln_Succ)
                      {
                        mln = mln->mln_Succ;
                        to = EDITLINE(mln)->el_Word;
                      }
                      else
                        to = ed->ed_Text+strlen(ed->ed_Text);
                    }
                    if (to)
                      strdel(from,to-from);
                    FreeEditList(ed);
                  }
                }
                else
                {
                  FreeEditList(ed);
                  if (ed->ed_Pos != ed->ed_MarkPos)
                  {
                    strdel(ed->ed_Text+min(ed->ed_Pos,ed->ed_MarkPos),abs(ed->ed_Pos-ed->ed_MarkPos)+1);
                    ed->ed_Pos = min(ed->ed_Pos,ed->ed_MarkPos);
                  }
                  else
                    strdel(ed->ed_Text+ed->ed_Pos,1);
                }
                break;
              default:
                FreeEditList(ed);
                c[1] = 0;
                strins(ed->ed_Text+ed->ed_Pos++,c);
            }
          }
          ed->ed_MarkPos = ed->ed_Pos;
          if (IsListEmpty((struct List *)&ed->ed_List))
          {
            PrepareEditText(ed,rp,ed->ed_Text);
            for(mln = ed->ed_List.mlh_Head;mln->mln_Succ && top;top--,mln = mln->mln_Succ);
            ed->ed_Top = mln;

            if (!MakeEditCursorVisible(cb,rp,gad,gpi->gpi_GInfo,ed))
              DrawEditGadget(cb,rp,gad,gpi->gpi_GInfo,ed,FALSE);
          }
        }
      }
  }
  ReleaseGIRPort(rp);
  return(retval);
}


BOOL PRIVATE DispatchEditNew(struct ClassBase *cb,struct opSet *ops,Object *o,struct EditGData *ed)
{
  struct Gadget *previous;
  struct DrawInfo *dri;
  STRPTR t;

  if ((dri = (struct DrawInfo *)GetTagData(GA_DrawInfo,0,ops->ops_AttrList)) != 0)
  {
    if ((ed->ed_Pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,8192,8192)) != 0)
    {
      ed->ed_GadWidth = ((struct Gadget *)o)->Width;
      ed->ed_LineHeight = dri->dri_Font->tf_YSize+2;

      previous = (APTR)GetTagData(GA_Previous,0,ops->ops_AttrList);
      if (GetTagData(EGA_Scroller,FALSE,ops->ops_AttrList) && MakeEditScroller(cb,ed,(struct Gadget *)o,previous))
        ed->ed_GadWidth -= 16;

      ed->ed_APen = dri->dri_Pens[TEXTPEN];
      ed->ed_BPen = dri->dri_Pens[BACKGROUNDPEN];
      ed->ed_Spacing = GetTagData(EGA_Spacing,0,ops->ops_AttrList);
      ed->ed_TabSpaces = GetTagData(EGA_TabSpaces,8,ops->ops_AttrList);
      ed->ed_FrameType = GetTagData(EGA_FrameType,FRAME_RIDGE,ops->ops_AttrList);
      ed->ed_MaxSpaces = GetTagData(EGA_MaxSpaces,3,ops->ops_AttrList);
      ed->ed_BorderH = 6;
      ed->ed_BorderV = 2;
      ed->ed_Flags = (GetTagData(EGA_Justification,EDJ_LEFT,ops->ops_AttrList) & EDF_JUSTIFICATION) |
                     (GetTagData(EGA_AutoIndent,FALSE,ops->ops_AttrList) ? EDF_AUTOINDENT : 0);

      if (GetTagData(EGA_ShowControls,0,ops->ops_AttrList))
        ed->ed_Flags |= EDF_SPECIAL;
      NEWLIST((struct List *)&ed->ed_List);
      ed->ed_Top = ed->ed_List.mlh_Head;

      t = (STRPTR)GetTagData(EGA_Text,0,ops->ops_AttrList);
      if ((ed->ed_Text = AllocPooled(ed->ed_Pool,ed->ed_Size = (t ? strlen(t) : 0)+4096)) != 0)
      {
        if (t)
          CopyMem(t,ed->ed_Text,strlen(t));
        return(TRUE);
      }
      DeletePool(ed->ed_Pool);
    }
  }
  return(FALSE);
}


IPTR DispatchEditGadget(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
  struct EditGData *ed;
  struct ClassBase *cb;
  IPTR   retval = 0;

  ed = INST_DATA(cl,o);
  cb = (APTR)cl->cl_UserData;

  switch(msg->MethodID)
  {
    case OM_NEW:
      if ((retval = DoSuperMethodA(cl,o,msg)) != 0)
      {
        SetAttrs((Object *)retval,GA_TabCycle,TRUE,TAG_END);
        ed = INST_DATA(cl,retval);

        if (!DispatchEditNew(cb,(struct opSet *)msg,(Object *)retval,ed))
        {
          DoSuperMethod(cl,(Object *)retval,OM_DISPOSE);
          retval = 0;
        }
      break;
    }
    case OM_DISPOSE:
      DisposeObject(ed->ed_Scroller);
      DeletePool(ed->ed_Pool);

      retval = DoSuperMethodA(cl,o,msg);
      break;
    case OM_SET:
    case OM_UPDATE:
    case OM_NOTIFY:
      DoSuperMethodA(cl,o,msg);
      {
        struct TagItem *tstate,*ti;

        tstate = ((struct opSet *)msg)->ops_AttrList;
        while((ti = NextTagItem(&tstate)) != 0)
        {
          switch(ti->ti_Tag)
          {
            case EGA_Text:
            {
              struct RastPort *rp = ObtainGIRPort(((struct opUpdate *)msg)->opu_GInfo);
              long   len;

              FreeEditList(ed);
              ed->ed_MarkPos = ed->ed_Pos = 0;
              if (ti->ti_Data)
              {
                if (ed->ed_Size < (len = strlen((STRPTR)ti->ti_Data)))
                  SetEditBuffer(cb,ed,len+256);
                CopyMem((APTR)ti->ti_Data,ed->ed_Text,len+1);
              }
              else
                *ed->ed_Text = 0;
              PrepareEditText(ed,rp,ed->ed_Text);
              DrawEditGadget(cb,rp,(struct Gadget *)o,((struct opSet *)msg)->ops_GInfo,ed,FALSE);
              ReleaseGIRPort(rp);
              break;
            }
            case PGA_Top:
            {
              struct RastPort *rp = ObtainGIRPort(((struct opUpdate *)msg)->opu_GInfo);
              struct MinNode *mln;
              long   top = ti->ti_Data;

              for(mln = ed->ed_List.mlh_Head;mln->mln_Succ && top;top--,mln = mln->mln_Succ);
              ed->ed_Top = mln;
              DrawEditGadget(cb,rp,(struct Gadget *)o,NULL,ed,FALSE);
              ReleaseGIRPort(rp);
              break;
            }
            case GA_Disabled:
            {
              struct RastPort *rp = ObtainGIRPort(((struct opUpdate *)msg)->opu_GInfo);

              DrawEditGadget(cb,rp,(struct Gadget *)o,NULL,ed,FALSE);
              ReleaseGIRPort(rp);
              break;
            }
          }
        }
      }
      break;
    case OM_GET:
      if (((struct opGet *)msg)->opg_AttrID == EGA_Text)
      {
        *((struct opGet *)msg)->opg_Storage = (ULONG)ed->ed_Text;
        retval = TRUE;
      }
      else
        retval = DoSuperMethodA(cl,o,msg);
      break;
    case GM_RENDER:
      {
        struct gpRender *gpr = (struct gpRender *)msg;

        ed->ed_CharWidth = gpr->gpr_RPort->Font->tf_XSize >> 1;
        ed->ed_Width = ed->ed_GadWidth-2*ed->ed_BorderH-ed->ed_CharWidth;
        ed->ed_LineHeight = gpr->gpr_RPort->Font->tf_YSize+ed->ed_Spacing;
        ed->ed_Lines = (((struct Gadget *)o)->Height-2*ed->ed_BorderV)/ed->ed_LineHeight;
        ed->ed_MinSpace = TextLength(gpr->gpr_RPort," ",1);
        ed->ed_MaxSpace = 42*ed->ed_MinSpace;
        PrepareEditText(ed,gpr->gpr_RPort,ed->ed_Text);

        SetAttrs(ed->ed_Scroller,PGA_Visible, ed->ed_Lines,TAG_END);
        DrawEditGadget(cb,gpr->gpr_RPort,(struct Gadget *)o,gpr->gpr_GInfo,ed,TRUE);
      }
      break;
    case GM_HITTEST:
      {
        struct gpHitTest *gpht = (struct gpHitTest *)msg;

        if (gpht->gpht_Mouse.X > ed->ed_BorderH && gpht->gpht_Mouse.Y > ed->ed_BorderV && gpht->gpht_Mouse.Y < ((struct Gadget *)o)->Height-ed->ed_BorderV && gpht->gpht_Mouse.X < ed->ed_GadWidth-ed->ed_BorderH)
          retval = GMR_GADGETHIT;
      }
      break;
    case GM_GOACTIVE:
    case GM_HANDLEINPUT:
      {
        struct gpInput *gpi = (struct gpInput *)msg;
        struct InputEvent *ie = gpi->gpi_IEvent;
        long x = gpi->gpi_Mouse.X, y = gpi->gpi_Mouse.Y;

        if (!ie)
        {
          struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);

          ed->ed_Flags |= EDF_ACTIVE;
          DrawEditCursor(cb,rp,(struct Gadget *)o,ed);
          ReleaseGIRPort(rp);
          retval = GMR_MEACTIVE;
          break;
        }
        if (ie->ie_Class == IECLASS_RAWMOUSE)
        {
          if (ie->ie_Code == IECODE_RBUTTON)
            retval = GMR_REUSE;
          else if (ie->ie_Code == (IECODE_LBUTTON | IECODE_UP_PREFIX))
          {
            ed->ed_Flags |= EDF_ACTIVE;
            ed->ed_Flags &= ~EDF_MARK;
            retval = GMR_MEACTIVE;
          }
          else if (ie->ie_Code == IECODE_LBUTTON)
          {
            if (x > 0 && x < ed->ed_GadWidth && y > 0 && y < ((struct Gadget *)o)->Height)
            {                         /* Cursor setzen */
              struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);
              long   pos;

              pos = GetEditCursorPos(cb,rp,ed,x,y);
              retval = GMR_MEACTIVE;
              if (ed->ed_Flags & EDF_ACTIVE)
              {
                if (pos == ed->ed_Pos && ed->ed_Pos == ed->ed_MarkPos)
                {
                  ReleaseGIRPort(rp);
                  break;
                }
                DrawEditCursor(cb,rp,(struct Gadget *)o,ed);
              }
              ed->ed_Flags |= EDF_MARK;
              ed->ed_MarkPos = ed->ed_Pos = pos;
              DrawEditCursor(cb,rp,(struct Gadget *)o,ed);
              ReleaseGIRPort(rp);
            }
            else                      /* Eingabe beenden */
              retval = GMR_REUSE;
          }
          else if (ed->ed_Flags & EDF_MARK)      /* Block markieren */
          {
            struct RastPort *rp = ObtainGIRPort(gpi->gpi_GInfo);
            long   pos;

            pos = GetEditCursorPos(cb,rp,ed,x,y);

            if (pos != ed->ed_Pos)
            {
              DrawEditCursor(cb,rp,(struct Gadget *)o,ed);
              ed->ed_Pos = pos;
              DrawEditCursor(cb,rp,(struct Gadget *)o,ed);
            }
            ReleaseGIRPort(rp);
            retval = GMR_MEACTIVE;
          }
        }
        else if (ie->ie_Class == IECLASS_RAWKEY)
          retval = DispatchEditHandleRawKey(cb,ed,gpi,(struct Gadget *)o,ie);
        ed->ed_Flags |= EDF_ACTIVE;
      }
      break;
    case GM_GOINACTIVE:
      if (ed->ed_Flags & EDF_ACTIVE)
      {
        struct gpGoInactive *gpgi = (APTR)msg;
        struct RastPort *rp = ObtainGIRPort(gpgi->gpgi_GInfo);

        ed->ed_Flags &= ~EDF_ACTIVE;
        DrawEditCursor(cb,rp,(struct Gadget *)o,ed);
        ReleaseGIRPort(rp);
      }
    default:
      retval = DoSuperMethodA(cl,o,msg);
  }
  return(retval);
}
