/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	library base functions


#include "TextEdit_includes.h"


#define VERSION	        1
#define REVISION        5
#define DATE            "26.12.2010"
#define VERS            "pTextEdit.gadget"
#define VSTRING         "pTextEdit.gadget 1.5 (26.12.2010)\r\n"
#define VERSTAG         "\0$VER: pTextEdit.gadget 1.5 (26.10.2010)"

static const char UserLibName[] = VERS;
static const char UserLibID[]   = VERSTAG;

LIBFUNC static struct ClassBase * LibInit(REG(a0, BPTR Segment), REG(d0, struct ClassBase *cb), REG(a6, struct ExecBase *sb));
LIBFUNC static BPTR               LibExpunge (REG(a6, struct ClassBase *cb));
LIBFUNC static struct ClassBase * LibOpen    (REG(a6, struct ClassBase *cb));
LIBFUNC static BPTR               LibClose   (REG(a6, struct ClassBase *cb));
LIBFUNC static LONG               LibNull    (void);


int Main(void)
{
  return RETURN_FAIL;
}

LIBFUNC static LONG LibNull(VOID)
{
  return 0;
}


LIBFUNC Class * GetClass(void)
{
  // return(((struct ClassBase *)cb)->cb_Class);
  return 0; // FIXME
}


void PRIVATE TE_Exit(struct ClassBase *cb)
{
  if (FreeClass(cb->cb_Class))
    cb->cb_Class = NULL;

  CloseLibrary(cb->cb_ScrollerBase);
  cb->cb_ScrollerBase = NULL;
  CloseLibrary(cb->cb_IFFParseBase);
  CloseLibrary(cb->cb_DOSBase);
  CloseLibrary(cb->cb_IntuitionBase);
  CloseLibrary((struct Library *)cb->cb_GfxBase);
  CloseLibrary(cb->cb_UtilityBase);
  CloseDevice(&cb->cb_Console);
}


int PRIVATE TE_Init(struct ClassBase *cb)
{
  if ((cb->cb_IntuitionBase = OpenLibrary("intuition.library",37)) != NULL)
  {
    if ((cb->cb_GfxBase = (APTR)OpenLibrary("graphics.library",37)) != NULL)
    {
      if ((cb->cb_UtilityBase = OpenLibrary("utility.library",37)) != NULL)
      {
        cb->cb_Console.io_Message.mn_Length = sizeof(struct IOStdReq);
        if (!OpenDevice("console.device",-1,(struct IORequest *)&cb->cb_Console,0))
        {
          Class *cl;

          if ((cl = MakeClass("pinc-editgadget",GADGETCLASS,NULL,sizeof(struct EditGData),0)) != NULL)
          {
            cl->cl_Dispatcher.h_Entry = (APTR)DispatchEditGadget;
            cl->cl_Dispatcher.h_Data = cb;
            cl->cl_UserData = (ULONG)cb;
            AddClass(cl);

            if ((cb->cb_IFFParseBase = OpenLibrary("iffparse.library",37)) != NULL)
              cb->cb_DOSBase = OpenLibrary("dos.library",37);
            cb->cb_Class = cl;

            return(TRUE);
          }
          CloseDevice(&cb->cb_Console);
        }
        CloseLibrary(cb->cb_UtilityBase);
      }
      CloseLibrary(cb->cb_GfxBase);
    }
    CloseLibrary(cb->cb_IntuitionBase);
  }
  return(FALSE);
}

#define libvector LFUNC_FAS(GetClass) \
  LFUNC_FA_(Text2Clipboard) \
  LFUNC_FA_(TextFromClipboard) \
  LFUNC_FA_(FreeEditList) \
  LFUNC_FA_(PrepareEditText)


STATIC CONST_APTR LibVectors[] =
{
  LibOpen,
  LibClose,
  LibExpunge,
  LibNull,
  libvector,
  (APTR)-1
};


STATIC CONST IPTR LibInitTab[] =
{
  sizeof(struct ClassBase),
  (IPTR)LibVectors,
  (IPTR)NULL,
  (IPTR)LibInit
};


static const USED_VAR struct Resident ROMTag =
{
  RTC_MATCHWORD,
  (struct Resident *)&ROMTag,
  (struct Resident *)(&ROMTag + 1),
  RTF_AUTOINIT,
  VERSION,
  NT_LIBRARY,
  0,
  (char *)UserLibName,
  (char *)UserLibID + 6,
  (APTR)LibInitTab
};


struct ClassBase * LibInit(REG(a0, BPTR segment),REG(d0, struct ClassBase *cb),REG(a6, struct ExecBase *ExecBase))
{
  if(ExecBase->LibNode.lib_Version < 37)
    return(NULL);

  cb->cb_LibNode.lib_Node.ln_Type = NT_LIBRARY;
  cb->cb_LibNode.lib_Node.ln_Pri  = 0;
  cb->cb_LibNode.lib_Node.ln_Name = UserLibName;
  cb->cb_LibNode.lib_Flags        = LIBF_CHANGED | LIBF_SUMUSED;
  cb->cb_LibNode.lib_Version      = VERSION;
  cb->cb_LibNode.lib_Revision     = REVISION;
  cb->cb_LibNode.lib_IdString     = (APTR)UserLibID;
  cb->cb_LibSegment = segment;
  cb->cb_SysBase = ExecBase;

  InitSemaphore(&cb->cb_LockSemaphore);

  return cb;
}


LIBFUNC struct ClassBase * LibOpen(REG(a6, struct ClassBase *cb))
{
  cb->cb_LibNode.lib_Flags &= ~LIBF_DELEXP;
  cb->cb_LibNode.lib_OpenCnt++;

  ObtainSemaphore(&cb->cb_LockSemaphore);

  if(cb->cb_LibNode.lib_OpenCnt == 1)
  {
    if (!TE_Init(cb))
    {
      TE_Exit(cb);

      cb->cb_LibNode.lib_OpenCnt--;
      ReleaseSemaphore(&cb->cb_LockSemaphore);
      return(NULL);
    }
  }
  ReleaseSemaphore(&cb->cb_LockSemaphore);
  return (cb);
}


LIBFUNC BPTR LibExpunge(REG(a6, struct ClassBase *cb))
{
  if (!cb->cb_LibNode.lib_OpenCnt && cb->cb_LibSegment)
  {
    BPTR TempSegment = cb->cb_LibSegment;

    Remove((struct Node *)cb);
    FreeMem((BYTE *)cb - cb->cb_LibNode.lib_NegSize,cb->cb_LibNode.lib_NegSize + cb->cb_LibNode.lib_PosSize);
    return(TempSegment);
  }
  else
  {
    cb->cb_LibNode.lib_Flags |= LIBF_DELEXP;
    return(NULL);
  }
}


LIBFUNC BPTR LibClose(REG(a6, struct ClassBase *cb))
{
  if (cb->cb_LibNode.lib_OpenCnt)
    cb->cb_LibNode.lib_OpenCnt--;

  if (!cb->cb_LibNode.lib_OpenCnt)
  {
    ObtainSemaphore(&cb->cb_LockSemaphore);
    TE_Exit(cb);
    ReleaseSemaphore(&cb->cb_LockSemaphore);

    if (cb->cb_LibNode.lib_Flags & LIBF_DELEXP)
      return(LibExpunge(cb));
  }
  return(NULL);
}
