/* :ts=4
 *  $VER: init.c $Revision$ (16-Sep-2013)
 *
 *  This file is part of gtdrag.
 *
 *  Copyright (c) 2013 Hyperion Entertainment CVBA.
 *  All Rights Reserved.
 *
 * $Id$
 *
 * $Log$
 *
 *
 */


#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <libraries/gtdrag.h>
#include <libraries/iffparse.h>
#include <libraries/gadtools.h>
#include <interfaces/gtdrag.h>
#include <proto/gtdrag.h>
#include <SDI_hook.h>
//#include <stdarg.h>

#include "gtdrag_loc.h"
#include "gtdrag_private.h"

/* Version Tag */
#include "gtdrag.library_rev.h"
STATIC CONST UBYTE USED verstag[] = VERSTAG;

extern ULONG IFFStreamHook(REG(a0, struct Hook *h), REG(a2, struct IFFHandle *iff), REG(a1, struct IFFStreamCmd *sc));
extern ULONG RenderHook(REG(a0, struct Hook * h), REG(a2, struct ImageNode *in), REG(a1, struct LVDrawMsg *msg));
extern ULONG TreeHook(REG(a0, struct Hook *h), REG(a2, struct TreeNode *tn), REG(a1, struct LVDrawMsg *msg));

/*hilfszeiger auf libs für Hool-Funktionen*/
struct ExecIFace *IExecB;
struct GraphicsIFace *IGfx;
struct IntuitionIFace *IIntui;
struct UtilityIFace *IUtil;

//struct Interface *IUtility __attribute__((force_no_baserel)) = NULL;

/*
 * The system (and compiler) rely on a symbol named _start which marks
 * the beginning of execution of an ELF file. To prevent others from 
 * executing this library, and to keep the compiler/linker happy, we
 * define an empty _start symbol here.
 *
 * On the classic system (pre-AmigaOS 4.x) this was usually done by
 * moveq #0,d0
 * rts
 *
 */
int32 _start(void)
{
    /* If you feel like it, open DOS and print something to the user */
	return RETURN_FAIL;
}


/* Expunge the library, delete from the library-list */
static APTR libExpunge(struct LibraryManagerInterface *Self)
{
    /* If your library cannot be expunged, return 0 */
    //struct ExecIFace *IExec = (struct ExecIFace *)(*(struct ExecBase **)4)->MainInterface;
    APTR result = (APTR)0;
	struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
    struct ExecIFace *IExec = (struct ExecIFace *)libBase->IExec;

  
    if (libBase->libNode.lib_OpenCnt == 0)
    {
    	struct DragApp *da;
    	struct Node *n;
    
     	result = (APTR)libBase->segList;

        /* Undo what the init code did */
    	while((n = IExec->RemHead((struct List *)&gadlist)))
      		IExec->FreeVec(n);
    	while((n = IExec->RemHead((struct List *)&winlist)))
      		IExec->FreeVec(n);
    	while((da = (struct DragApp *)IExec->RemHead((struct List *)&applist)))
      		IExec->FreeVec(da);
      
        IExec->Remove((struct Node *)libBase);
        IExec->DeleteLibrary((struct Library *)libBase);
   	}
    else
    {
        result = (APTR)0;
        libBase->libNode.lib_Flags |= LIBF_DELEXP;
    }
    return result;
}



/* Open the library */
static struct Library *libOpen(struct LibraryManagerInterface *Self, ULONG version)
{
    struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase; 
    struct ExecIFace *IExec = (struct ExecIFace *)libBase->IExec;

    /* Add up the open count */
   	libBase->libNode.lib_Flags &= ~LIBF_DELEXP;
   	libBase->libNode.lib_OpenCnt++;

  	IExec->ObtainSemaphore(&libBase->LockSemaphore);

  	if(libBase->libNode.lib_OpenCnt == 1)
  	{

    	libBase->DOSBase = IExec->OpenLibrary("dos.library", 52);
 		libBase->IDOS = (struct DOSIFace *)IExec->GetInterface(libBase->DOSBase, "main", 1, NULL);
 		libBase->IntuitionBase = IExec->OpenLibrary("intuition.library", 52);
      	libBase->IIntuition = (struct IntuitionIFace *)IExec->GetInterface(libBase->IntuitionBase, "main", 1, NULL);
 		libBase->UtilityBase = IExec->OpenLibrary("utility.library", 53);
 		libBase->IUtility = (struct UtilityIFace *)IExec->GetInterface(libBase->UtilityBase, "main", 1, NULL);
      	libBase->GfxBase = IExec->OpenLibrary("graphics.library", 52);
      	libBase->IGraphics = (struct GraphicsIFace *)IExec->GetInterface(libBase->GfxBase, "main", 1, NULL);
      	libBase->CyberGfxBase = IExec->OpenLibrary("cybergraphics.library", 43);
      	libBase->ICyberGfx = (struct CyberGfxIFace *)IExec->GetInterface(libBase->CyberGfxBase, "main", 1, NULL);
      	libBase->LayersBase = IExec->OpenLibrary("layers.library", 53);
      	libBase->ILayers = (struct LayersIFace *)IExec->GetInterface(libBase->LayersBase, "main", 1, NULL);
		libBase->GadToolsBase = IExec->OpenLibrary("gadtools.library", 52);
 		libBase->IGadTools = (struct GadToolsIFace *)IExec->GetInterface(libBase->GadToolsBase, "main", 1, NULL);

  		IExecB = IExec;
  		IGfx = libBase->IGraphics;
  		IIntui = libBase->IIntuition;
  		IUtil = libBase->IUtility;


    	if (!(dmport = IExec->AllocSysObjectTags(ASOT_PORT, TAG_END)) || !libBase->IntuitionBase ||
          	  !libBase->UtilityBase || !libBase->LayersBase || !libBase->GadToolsBase || !libBase->GfxBase)
    	{
    	struct IntuiMessage *msg;

    		while((msg = (APTR)IExec->GetMsg(dmport)))
    	  		FreeDropMessage((struct GtdragIFace *)Self, msg);
	    	IExec->FreeSysObject(ASOT_PORT, dmport);

      		IExec->DropInterface((struct Interface *)libBase->ILayers);
      		IExec->CloseLibrary(libBase->LayersBase);
       		IExec->DropInterface((struct Interface *)libBase->ICyberGfx);
       		IExec->CloseLibrary(libBase->CyberGfxBase);
       		IExec->DropInterface((struct Interface *)libBase->IGraphics);
       		IExec->CloseLibrary(libBase->GfxBase);
       		IExec->DropInterface((struct Interface *)libBase->IGadTools);
       		IExec->CloseLibrary(libBase->GadToolsBase);
       		IExec->DropInterface((struct Interface *)libBase->IUtility);
       		IExec->CloseLibrary(libBase->UtilityBase);
       		IExec->DropInterface((struct Interface *)libBase->IIntuition);
       	 	IExec->CloseLibrary(libBase->IntuitionBase);
   	    	IExec->DropInterface((struct Interface *)libBase->IDOS);
	        IExec->CloseLibrary(libBase->DOSBase);
 
      		IExec->ReleaseSemaphore(&libBase->LockSemaphore);
      		libExpunge(Self);

      		libBase->libNode.lib_OpenCnt--;
      		return(NULL);
    	}
  	}
  	IExec->ReleaseSemaphore(&libBase->LockSemaphore);
    return (struct Library *)(struct GTDragBase *)libBase;
}


/* Close the library */
static APTR libClose(struct LibraryManagerInterface *Self)
{
    struct GTDragBase *libBase = (struct GTDragBase *)Self->Data.LibBase;
    struct ExecIFace *IExec = (struct ExecIFace *)libBase->IExec;
    /* Make sure to undo what open did */

	if(libBase->libNode.lib_OpenCnt)
	    ((struct Library *)libBase)->lib_OpenCnt--;

	if(!(libBase->libNode.lib_OpenCnt))
	{
	  	IExec->ObtainSemaphore(&libBase->LockSemaphore);

   		if (dmport)
  		{
    		struct IntuiMessage *msg;

    		while((msg = (APTR)IExec->GetMsg(dmport)))
      		FreeDropMessage((struct GtdragIFace *)Self, msg);
    		IExec->FreeSysObject(ASOT_PORT, dmport);
  		}
        IExec->DropInterface((struct Interface *)libBase->ILayers);
        IExec->CloseLibrary(libBase->LayersBase);
        IExec->DropInterface((struct Interface *)libBase->ICyberGfx);
        IExec->CloseLibrary(libBase->CyberGfxBase);
        IExec->DropInterface((struct Interface *)libBase->IGraphics);
        IExec->CloseLibrary(libBase->GfxBase);
        IExec->DropInterface((struct Interface *)libBase->IGadTools);
        IExec->CloseLibrary(libBase->GadToolsBase);
        IExec->DropInterface((struct Interface *)libBase->IUtility);
        IExec->CloseLibrary(libBase->UtilityBase);
        IExec->DropInterface((struct Interface *)libBase->IIntuition);
       	IExec->CloseLibrary(libBase->IntuitionBase);
    	IExec->DropInterface((struct Interface *)libBase->IDOS);
	    IExec->CloseLibrary(libBase->DOSBase);
  		IExec->ReleaseSemaphore(&libBase->LockSemaphore);
	
		if(libBase->libNode.lib_Flags & LIBF_DELEXP)
			return(libExpunge(Self));
	}
    return 0;
}


/* The ROMTAG Init Function */
static struct Library *libInit(struct GTDragBase *libBase, APTR seglist, struct ExecIFace *IExec)
{
    libBase->libNode.lib_Node.ln_Type = NT_LIBRARY;
    libBase->libNode.lib_Node.ln_Pri  = 0;
    libBase->libNode.lib_Node.ln_Name = "gtdrag.library";
    libBase->libNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
    libBase->libNode.lib_Version      = VERSION;
    libBase->libNode.lib_Revision     = REVISION;
    libBase->libNode.lib_IdString     = VSTRING;
    libBase->IExec                    = IExec;

    libBase->segList = (BPTR)seglist;

    /* Add additional init code here if you need it. For example, to open additional Libraries:*/
      
  		/** init global data **/

	IExec->NewList((struct List *)&applist);
	IExec->NewList((struct List *)&gadlist);
	IExec->NewList((struct List *)&winlist);

	mx = 0;  my = 0;
	gdo = NULL;  dg = NULL;
	fakemsg = FALSE;  noreport = FALSE;

	/** init hooks and helpers**/
	treeHook.h_Entry = (HOOKFUNC)TreeHook;
	renderHook.h_Entry = (HOOKFUNC)RenderHook;
	iffstreamHook.h_Entry = (HOOKFUNC)IFFStreamHook;

	/** init semaphores **/
	IExec->InitSemaphore(&ListSemaphore);
	IExec->InitSemaphore(&libBase->LockSemaphore);
  
    return (struct Library *)libBase;
}

/* ------------------- Manager Interface ------------------------ */
/* These are generic. Replace if you need more fancy stuff */
static uint32 _generic_Obtain(struct Interface *Self) {
	return ++Self->Data.RefCount;
}

static uint32 _generic_Release(struct Interface *Self) {
	return --Self->Data.RefCount;
}

/* Manager interface vectors */
static CONST APTR lib_manager_vectors[] = {
	(APTR)_generic_Obtain,
	(APTR)_generic_Release,
	NULL,
	NULL,
	(APTR)libOpen,
	(APTR)libClose,
	(APTR)libExpunge,
	NULL,
	(APTR)-1
};

/* "__library" interface tag list */
STATIC CONST struct TagItem lib_managerTags[] =
{
	{ MIT_Name,			(Tag)"__library"		},
	{ MIT_VectorTable,	(Tag)lib_manager_vectors},
	{ MIT_Version,		1						},
	{ TAG_DONE,			0						}
};

/* ------------------- Library Interface(s) ------------------------ */

#include "gtdrag_vectors.c"

/* Uncomment this line (and see below) if your library has a 68k jump table */
/* extern APTR VecTable68K[]; */

STATIC CONST struct TagItem mainTags[] =
{
	{ MIT_Name,			(Tag)"main"			},
	{ MIT_VectorTable,	(Tag)main_vectors	},
	{ MIT_Version,		1					},
	{ TAG_DONE,			0					}
};

STATIC CONST CONST_APTR libInterfaces[] =
{
	lib_managerTags,
	mainTags,
	NULL
};

STATIC CONST struct TagItem libCreateTags[] =
{
	{ CLT_DataSize,		sizeof(struct GTDragBase)	},
	{ CLT_InitFunc,		(Tag)libInit			},
	{ CLT_Interfaces,	(Tag)libInterfaces		},
	/* Uncomment the following line if you have a 68k jump table */
	/* { CLT_Vector68K, (Tag)VecTable68K }, */
	{TAG_DONE,		 0 }
};


/* ------------------- ROM Tag ------------------------ */
STATIC CONST struct Resident lib_res USED =
{
	RTC_MATCHWORD,
	(struct Resident *)&lib_res,
	(APTR)(&lib_res + 1),
	RTF_NATIVE|RTF_AUTOINIT, /* Add RTF_COLDSTART if you want to be resident */
	VERSION,
	NT_LIBRARY, /* Make this NT_DEVICE if needed */
	0, /* PRI, usually not needed unless you're resident */
	"gtdrag.library",
	VSTRING,
	(APTR)libCreateTags
};

