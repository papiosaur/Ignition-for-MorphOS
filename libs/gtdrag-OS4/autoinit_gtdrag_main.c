/*
**	$Id$
**	Generated by IDLTool 53.1
**	Do not edit
**
**	AutoInit stub for gtdrag
**
**	Copyright (c) 2010 Hyperion Entertainment CVBA.
**	All Rights Reserved.
*/

#include <exec/types.h>
#include <intuition/gadgetclass.h>
#include <libraries/gtdrag.h>

#include <interfaces/gtdrag.h>
#include <proto/exec.h>
#include <assert.h>

struct GtdragIFace * IGtdrag = NULL;
static struct Library * __GTDragBase;
static struct GtdragIFace * __IGtdrag;

/****************************************************************************/

extern struct Library * GTDragBase;

/****************************************************************************/

void gtdrag_main_constructor(void)
{
    if (GTDragBase == NULL) /* Library base is NULL, we need to open it */
    {
        /* We were called before the base constructor.
         * This means we will be called _after_ the base destructor.
         * So we cant drop the interface _after_ closing the last library base,
         * we just open the library here which ensures that.
         */
        __GTDragBase = GTDragBase = (struct Library *)IExec->OpenLibrary("gtdrag.library", 0L);
        assert(GTDragBase != NULL);
    }

    __IGtdrag = IGtdrag = (struct GtdragIFace *)IExec->GetInterface((struct Library *)GTDragBase, "main", 1, NULL);
    assert(IGtdrag != NULL);
}
__attribute__((section(".ctors.zzzy"))) static void
(*gtdrag_main_constructor_ptr)(void) USED = gtdrag_main_constructor;

/****************************************************************************/

void gtdrag_main_destructor(void)
{
    if (__IGtdrag)
    {
        IExec->DropInterface ((struct Interface *)__IGtdrag);
    }
    if (__GTDragBase)
    {
        IExec->CloseLibrary((struct Library *)__GTDragBase);
    }
}
__attribute__((section(".dtors.zzzy"))) static void
(*gtdrag_main_destructor_ptr)(void) USED = gtdrag_main_destructor;

/****************************************************************************/

