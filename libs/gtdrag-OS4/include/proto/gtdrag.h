#ifndef PROTO_GTDRAG_H
#define PROTO_GTDRAG_H

/*
**	$Id$
**	Includes Release 50.1
**
**	Prototype/inline/pragma header file combo
**
**	Copyright (c) 2010 Hyperion Entertainment CVBA.
**	All Rights Reserved.
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef LIBRARIES_GTDRAG_H
#include <libraries/gtdrag.h>
#endif

/****************************************************************************/

#ifndef __NOLIBBASE__
 #ifndef __USE_BASETYPE__
  extern struct Library * GTDragBase;
 #else
  extern struct Library * GTDragBase;
 #endif /* __USE_BASETYPE__ */
#endif /* __NOLIBBASE__ */

/****************************************************************************/

#ifdef __amigaos4__
 #include <interfaces/gtdrag.h>
 #ifdef __USE_INLINE__
  #include <inline4/gtdrag.h>
 #endif /* __USE_INLINE__ */
 #ifndef CLIB_GTDRAG_PROTOS_H
  #define CLIB_GTDRAG_PROTOS_H 1
 #endif /* CLIB_GTDRAG_PROTOS_H */
 #ifndef __NOGLOBALIFACE__
  extern struct GtdragIFace *IGtdrag;
 #endif /* __NOGLOBALIFACE__ */
#else /* __amigaos4__ */
 #ifndef CLIB_GTDRAG_PROTOS_H
  #include <clib/gtdrag_protos.h>
 #endif /* CLIB_GTDRAG_PROTOS_H */
 #if defined(__GNUC__)
  #ifndef __PPC__
   #include <inline/gtdrag.h>
  #else
   #include <ppcinline/gtdrag.h>
  #endif /* __PPC__ */
 #elif defined(__VBCC__)
  #ifndef __PPC__
   #include <inline/gtdrag_protos.h>
  #endif /* __PPC__ */
 #else
  #include <pragmas/gtdrag_pragmas.h>
 #endif /* __GNUC__ */
#endif /* __amigaos4__ */

/****************************************************************************/

#endif /* PROTO_GTDRAG_H */
