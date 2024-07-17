/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	clipboard support.


#include "TextEdit_includes.h"
#include "dos/dostags.h"


void PUBLIC internal_Text2Clipboard(REG(d0, UBYTE clipunit),REG(a0, STRPTR t),REG(d1, long len),REG(a6, struct ClassBase *cb))
{
  struct IFFHandle *iff;
  long   error;

  if ((iff = AllocIFF()) != 0)
  {
    if ((iff->iff_Stream = (ULONG)OpenClipboard((long)clipunit)) != 0)
    {
      InitIFFasClip(iff);
      if (!(error = OpenIFF(iff,IFFF_WRITE)))
      {
        if (!(error = PushChunk(iff,ID_FTXT,ID_FORM,IFFSIZE_UNKNOWN)))
        {
          if (!(error = PushChunk(iff,0,ID_CHRS,IFFSIZE_UNKNOWN)))
          {
            if (WriteChunkBytes(iff,t,len) != len)
              error = IFFERR_WRITE;
            if (!error)
              error = PopChunk(iff);
          }
          if (!error)
            error = PopChunk(iff);
        }
        if (error)           // rudimentäre Fehlerbehandlung
          DisplayBeep(NULL);

        CloseIFF(iff);
      }
      CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
  }
}


STRPTR PUBLIC internal_TextFromClipboard(REG(d0, UBYTE clipunit),REG(a0, APTR pool),REG(a6, struct ClassBase *cb))
{
  struct IFFHandle *iff;
  long   error,size = 1;
  STRPTR t = NULL,temp;

  if ((iff = AllocIFF()) != 0)
  {
    if ((iff->iff_Stream = (ULONG)OpenClipboard(clipunit)) != 0)
    {
      InitIFFasClip(iff);

      if (!(error = OpenIFF(iff,IFFF_READ)))
      {
        struct ContextNode *cn;

        StopChunk(iff,ID_FTXT,ID_CHRS);

        while(TRUE)
        {
          if ((error = ParseIFF(iff,IFFPARSE_SCAN)) == IFFERR_EOC)
            continue;
          else if (error)
            break;

          if ((cn = CurrentChunk(iff)) && cn->cn_ID == ID_CHRS && cn->cn_Type == ID_FTXT)
          {
            if ((temp = AllocPooled(pool,size+cn->cn_Size+(t ? 1 : 0))) != 0)
            {
              if (t)
              {
                CopyMem(t,temp,size);
                FreePooled(pool,t,size);
                *(temp+size-1) = '\n';
                size++;
              }
              ReadChunkBytes(iff,temp+size-1,cn->cn_Size);

              size += cn->cn_Size;
              t = temp;
            }
          }
        }
        CloseIFF(iff);
      }
      CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
  }
  return(t);
}

struct Task *g_owner;
struct ClassBase *g_cb;
long   g_sigbit,g_len;
STRPTR g_t;
UBYTE  g_clipunit;
APTR *global_vars;

void PUBLIC process_Text2Clipboard(void)
{
  internal_Text2Clipboard(g_clipunit,g_t,g_len,g_cb);
  Signal(g_owner,1L << g_sigbit);
}


void PUBLIC process_TextFromClipboard(void)
{
  g_t = internal_TextFromClipboard(g_clipunit,g_t,g_cb);
  Signal(g_owner,1L << g_sigbit);
}


LIBFUNC void Text2Clipboard(
  REG(d0, UBYTE clipunit),
  REG(a0, STRPTR t),
  REG(d1, long len))
{
  struct Task *this;

  if (!t || !len || !cb->cb_IFFParseBase)
    return;

  if ((this = FindTask(NULL)) && this->tc_Node.ln_Type == NT_TASK)
  {
    ObtainSemaphore(&cb->cb_LockSemaphore);

    if ((g_sigbit = AllocSignal(-1L)) != -1)
    {
      g_clipunit = clipunit;
      g_t = t;
      g_len = len;
      g_cb = cb;
      g_owner = this;

      if (CreateNewProcTags(NP_Entry,process_Text2Clipboard,TAG_END))
        Wait(1L << g_sigbit);

      FreeSignal(g_sigbit);
    }
    ReleaseSemaphore(&cb->cb_LockSemaphore);
  }
  else
    internal_Text2Clipboard(clipunit,t,len,cb);
}


LIBFUNC STRPTR TextFromClipboard(
  REG(d0, UBYTE clipunit),
  REG(a0, APTR pool))
{
  struct Task *this;

  if (!pool || !cb->cb_IFFParseBase)
    return(NULL);

  if ((this = FindTask(NULL)) && this->tc_Node.ln_Type == NT_TASK)
  {
    STRPTR t = NULL;

    ObtainSemaphore(&cb->cb_LockSemaphore);

    if ((g_sigbit = AllocSignal(-1L)) != -1)
    {
      g_clipunit = clipunit;
      g_t = pool;
      g_cb = cb;
      g_owner = this;

      if (CreateNewProcTags(NP_Entry,process_TextFromClipboard,TAG_END))
      {
        Wait(1L << g_sigbit);
        t = g_t;
      }

      FreeSignal(g_sigbit);
    }
    ReleaseSemaphore(&cb->cb_LockSemaphore);

    return(t);
  }
  else
    return(internal_TextFromClipboard(clipunit,pool,cb));
}
