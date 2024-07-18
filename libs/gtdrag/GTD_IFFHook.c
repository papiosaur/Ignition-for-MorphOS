/*
 * Copyright ©1999-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the MIT License.
 */

//!	IFF Streaming-Hook


#include "gtdrag_includes.h"


#define BUFFERSIZE 2048


ULONG PUBLIC IFFStreamHook(REG(a0, struct Hook *h), REG(a2, struct IFFHandle *iff), REG(a1, struct IFFStreamCmd *sc))
{
  struct IFFStreamHookData *is = h->h_Data;

  switch(sc->sc_Command)
  {
    case IFFCMD_INIT:
      if (!is)
        return(-1L);

      is->is_Position = 0;
      break;
    case IFFCMD_CLEANUP:
      break;
    case IFFCMD_READ:
      CopyMem(is->is_Buffer+is->is_Position,sc->sc_Buf,sc->sc_NBytes);
      is->is_Position += sc->sc_NBytes;
      break;
    case IFFCMD_WRITE:
      if (is->is_Position+sc->sc_NBytes > is->is_Size)
      {
        APTR  temp;
        ULONG size = is->is_Size;

        for(;sc->sc_NBytes+is->is_Position > size;size += BUFFERSIZE);

        if (is->is_Pool)
          temp = AllocPooled(is->is_Pool,size);
        else
          temp = AllocMem(size,0L);

        if (!temp)
          return(-1);

        if (is->is_Buffer)
        {
          if (is->is_Position)
            CopyMem(is->is_Buffer,temp,is->is_Position);

          if (is->is_Pool)
            FreePooled(is->is_Pool,is->is_Buffer,is->is_Size);
          else
            FreeMem(is->is_Buffer,is->is_Size);

          is->is_Buffer = temp;
          is->is_Size = size;
        }
      }
      CopyMem(sc->sc_Buf,is->is_Buffer+is->is_Position,sc->sc_NBytes);
      is->is_Position += sc->sc_NBytes;
      break;
    case IFFCMD_SEEK:
      is->is_Position += sc->sc_NBytes;
      break;
  }
  if (is->is_Position < 0 || is->is_Position > is->is_Size)
    return(-1L);

  return(0L);
}
