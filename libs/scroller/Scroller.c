/*
 * Copyright ©1999-2010 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	dispatcher functions.


#include "Scroller_includes.h"


/********************************** Arrow-Images **********************************/

#define IM(o) ((struct Image *)o)

IPTR PUBLIC DispatchArrowImage(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
  struct ArrowIData *ad;
  struct ClassBase *cb;
  IPTR   retval = 0;

  ad = INST_DATA(cl,o);
  cb = (APTR)cl->cl_UserData;

  switch(msg->MethodID)
  {
    case OM_NEW:
      if ((retval = DoSuperMethodA(cl,o,msg)) != 0)
      {
        IM(retval)->Width = GetTagData(IA_Width,16,((struct opSet *)msg)->ops_AttrList);
        IM(retval)->Height = GetTagData(IA_Height,13,((struct opSet *)msg)->ops_AttrList);
        if (IM(retval)->Width < 10)
          IM(retval)->Width = 10;
        if (IM(retval)->Height < 8)
          IM(retval)->Height = 8;

        ad = INST_DATA(cl,retval);
        ad->ad_Type = GetTagData(SYSIA_Which,0,((struct opSet *)msg)->ops_AttrList);
        ad->ad_Frame = NewObject(NULL,"frameiclass",IA_FrameType, FRAME_BUTTON,
                                                    IA_Width,     IM(retval)->Width,
                                                    IA_Height,    IM(retval)->Height,
                                                    TAG_END);
      }
      break;
    case OM_DISPOSE:
      DisposeObject(ad->ad_Frame);
      retval = DoSuperMethodA(cl,o,msg);
      break;
    case IM_DRAW:
    case IM_DRAWFRAME:
      {
        struct impDraw *imp = (struct impDraw *)msg;
        long   x,y,w,h;

        x = IM(o)->LeftEdge+imp->imp_Offset.X;
        y = IM(o)->TopEdge+imp->imp_Offset.Y;
        w = IM(o)->Width-9;
        h = IM(o)->Height-5;
        if (!(w % 2))
          w++;
        DrawImageState(imp->imp_RPort,ad->ad_Frame,x,y,imp->imp_State,imp->imp_DrInfo);

        x += 4;
        y += 2;
        if (imp->imp_DrInfo)
          SetAPen(imp->imp_RPort,imp->imp_DrInfo->dri_Pens[TEXTPEN]);
        else
          SetAPen(imp->imp_RPort,1);

        if (ad->ad_Type == ADT_UP)
        {
          Move(imp->imp_RPort,x,y+h);     /* up arrow */
          Draw(imp->imp_RPort,x+w/2,y);
          Draw(imp->imp_RPort,x+w-1,y+h);
          Move(imp->imp_RPort,x+1,y+h);
          Draw(imp->imp_RPort,x+w/2+1,y);
          Draw(imp->imp_RPort,x+w,y+h);
        }
        else
        {
          Move(imp->imp_RPort,x,y);       /* down arrow */
          Draw(imp->imp_RPort,x+w/2,y+h);
          Draw(imp->imp_RPort,x+w-1,y);
          Move(imp->imp_RPort,x+1,y);
          Draw(imp->imp_RPort,x+w/2+1,y+h);
          Draw(imp->imp_RPort,x+w,y);
        }
      }
      break;
    default:
      retval = DoSuperMethodA(cl,o,msg);
  }
  return(retval);
}

/********************************** Scroller-Gadget **********************************/

BOOL PRIVATE DispatchScrollerNew(struct ClassBase *cb,struct ScrollerGData *sd,struct Gadget *gad,struct opSet *ops)
{
  static struct TagItem upmap[] = {{GA_ID,SGA_Up},{TAG_END}};
  static struct TagItem downmap[] = {{GA_ID,SGA_Down},{TAG_END}};
  struct Gadget *previous;
  long   x,y,w,h;

  if (!(previous = (struct Gadget *)GetTagData(GA_Previous,0,ops->ops_AttrList)))
    return(FALSE);
  sd->sd_ItemHeight = GetTagData(SGA_ItemHeight,13,ops->ops_AttrList);

  if (!(sd->sd_DownImage = NewObject(cb->cb_ArrowClass,NULL,IA_Width,    gad->Width,
                                                            IA_Height,   sd->sd_ItemHeight,
                                                            SYSIA_Which, ADT_DOWN,
                                                            TAG_END)))
    return(FALSE);
  if (!(sd->sd_UpImage = NewObject(cb->cb_ArrowClass,NULL,IA_Width,    gad->Width,
                                                          IA_Height,   sd->sd_ItemHeight,
                                                          SYSIA_Which, ADT_UP,
                                                          TAG_END)))
    return(FALSE);
  if (!(sd->sd_Frame = NewObject(NULL,"frameiclass",IA_Left,      gad->LeftEdge,
                                                    IA_Top,       gad->TopEdge,
                                                    IA_Width,     gad->Width,
                                                    IA_Height,    gad->Height-(h = sd->sd_UpImage->Height)*2,
                                                    IA_FrameType, FRAME_BUTTON,
                                                    TAG_END)))
    return(FALSE);
  if (!(sd->sd_Up = NewObject(NULL,"buttongclass",GA_ID,        1,
                                                  GA_Left,      x = gad->LeftEdge,
                                                  GA_Top,       y = gad->TopEdge+gad->Height-h*2,
                                                  GA_Width,     w = sd->sd_UpImage->Width,
                                                  GA_Height,    h,
                                                  GA_Image,     sd->sd_UpImage,
                                                  GA_Previous,  previous,
                                                  ICA_TARGET,   gad,
                                                  ICA_MAP,      upmap,
                                                  TAG_END)))
    return(FALSE);
  if (!(sd->sd_Down = NewObject(NULL,"buttongclass",GA_ID,        1,
                                                    GA_Left,      x,
                                                    GA_Top,       y+h,
                                                    GA_Width,     w,
                                                    GA_Height,    h,
                                                    GA_Image,     sd->sd_DownImage,
                                                    GA_Previous,  sd->sd_Up,
                                                    ICA_TARGET,   gad,
                                                    ICA_MAP,      downmap,
                                                    TAG_END)))
    return(FALSE);
  sd->sd_Down->NextGadget = gad;
  return(TRUE);
}


IPTR PUBLIC DispatchScrollerGadget(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
{
  struct ScrollerGData *sd;
  struct ClassBase *cb;
  IPTR   retval = 0;

  sd = INST_DATA(cl,o);
  cb = (APTR)cl->cl_UserData;

  switch(msg->MethodID)
  {
    case OM_NEW:
      if ((retval = DoSuperMethodA(cl,o,msg)) != 0)
      {
        struct Gadget *gad = (struct Gadget *)retval;

        sd = INST_DATA(cl,retval);

        if (DispatchScrollerNew(cb,sd,gad,(struct opSet *)msg))
        {
          //gad->NextGadget = sd->sd_Down;
          SetAttrs((APTR)retval,GA_Left,      gad->LeftEdge+4,
                                GA_Top,       gad->TopEdge+2,
                                GA_Width,     gad->Width-8,
                                GA_Height,    gad->Height-2*sd->sd_UpImage->Height-4,
                                PGA_Total,    GetTagData(PGA_Total,0,((struct opSet *)msg)->ops_AttrList),
                                GA_RelVerify, TRUE,
                                TAG_END);
        }
        else
        {
          DoMethod(o,OM_DISPOSE);
          retval = 0;
        }
      }
      break;
    case OM_UPDATE:
    {
      struct opUpdate *opu = (APTR)msg;
      long   move = 0;

      retval = DoSuperMethodA(cl,o,msg);

      if ((move = -GetTagData(SGA_Up,0,opu->opu_AttrList)) != 0)
        if (!(sd->sd_Up->Flags & GFLG_SELECTED))
          move = 0;
      if (!move && (move = GetTagData(SGA_Down,0,opu->opu_AttrList)))
        if (!(sd->sd_Down->Flags & GFLG_SELECTED))
          move = 0;
      if (move)
      {
        long top;

        if (GetAttr(PGA_Top,o,(IPTR *)&top))
        {
          top += move;
          if (top >= 0)
          {
            struct RastPort *rp = ObtainGIRPort(opu->opu_GInfo);
            struct TagItem  tags[] = {{PGA_Top,0},{TAG_END}};

            SetAttrs(o,PGA_Top,top,TAG_END);
            GetAttr(PGA_Top,o,&tags[0].ti_Data);
            DoSuperMethod(cl,o,OM_NOTIFY,tags,opu->opu_GInfo,0);
            DoSuperMethod(cl,o,GM_RENDER,opu->opu_GInfo,rp,GREDRAW_UPDATE);
            ReleaseGIRPort(rp);
          }
        }
      }
      break;
    }
    case OM_DISPOSE:
      DisposeObject(sd->sd_UpImage);
      DisposeObject(sd->sd_DownImage);
      DisposeObject(sd->sd_Up);
      DisposeObject(sd->sd_Down);
      DisposeObject(sd->sd_Frame);
      retval = DoSuperMethodA(cl,o,msg);
      break;
    case GM_RENDER:
    {
      struct gpRender *gpr = (struct gpRender *)msg;

      DrawImage(gpr->gpr_RPort,sd->sd_Frame,0,0);
      retval = DoSuperMethodA(cl,o,msg);
      break;
    }
    default:
      retval = DoSuperMethodA(cl,o,msg);
  }
  return(retval);
}
