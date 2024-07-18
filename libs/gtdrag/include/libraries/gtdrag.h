#ifndef LIBRARIES_GTDRAG_H
#define LIBRARIES_GTDRAG_H 1
/*
**  $VER: gtdrag.h 3.11 (12.8.2003)
**  Includes Release 3.6
**
**  Drag&Drop with GadTools
**
**  Copyright ©1996-2003 pinc Software.
**  All rights Reserved.
*/

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif


/********************* ObjectDescription structure *********************/

/* the ObjectDescription structure describes the contents of the object
 * being dragged.
 */

struct ObjectDescription
{
  APTR   od_Object;           /* the pointer to the object */
  ULONG  od_GroupID;          /* DataTypes-GroupID (see datatypes.h) */
  ULONG  od_Type;             /* rough format of the object */
  ULONG  od_InternalType;     /* application related type */
  STRPTR od_Owner;            /* owner name or NULL if you are the owner */
  APTR   od_UserData;
};

/** Values for od_Type **/

#define ODT_UNKNOWN 0    /* should only be used rarely    */
#define ODT_STRING 1     /* simple c-string (STRPTR)      */
#define ODT_NODE 2       /* (struct Node *)               */
#define ODT_IMAGENODE 4  /* (struct ImageNode *)          */
#define ODT_TREENODE 8   /* (struct TreeNode *)           */
#define ODT_IMAGE 16     /* (struct Image *)              */
#define ODT_ICON 32      /* (struct DiskObject *)         */
#define ODT_IFF 64       /* pointer to an IFF-data stream */
#define ODT_LOCK 128     /* a file lock                   */
#define ODT_DATATYPE 256 /* a datatype object             */
#define ODT_BITMAP 512   /* (struct BitMap *)             */


/********************* DropMessage structure *********************/

/* You receive the DropMessage structure if someone has dropped an object
 * on a gadget or in a window of your application. The IAddress field of
 * the IntuiMessage points to this structure.
 * Remember: all fields are read-only!
 */

struct DropMessage
{
  struct MinNode dm_Node;
  struct ObjectDescription dm_Object;  /* dragged object */
  struct Gadget *dm_Gadget;     /* source gadget */
  struct Window *dm_Window;     /* source gadget's window */
  struct Gadget *dm_Target;     /* pointer to the target gadget */
  LONG   dm_SourceEntry;        /* the list position of the entry */
  LONG   dm_TargetEntry;        /* dto. - may be higher than the number of entries */
  ULONG  dm_Flags;
};

#define DMF_DROPOVER 1          /* for treeviews */


/** The flags for the IDCMP-MsgPort of your Window **/

#define DRAGIDCMP (LISTVIEWIDCMP | IDCMP_MOUSEBUTTONS)
#define IDCMP_OBJECTDROP (IDCMP_DISKINSERTED | IDCMP_DISKREMOVED)

/* If you get a message of the IDCMP_OBJECTDROP class, someone dropped
 * an object on a gadget or in a window that supports it.
 * The IAddress-field of the IntuiMessage points to the DragMsg in this
 * case.
 */

/** a drag key qualifier **/

#define IEQUALIFIER_DRAGKEY (IEQUALIFIER_LALT | IEQUALIFIER_RALT)


/** Tags to pass to GTD_AddGadget() (a few also for GTD_AddWindow()) **/

#define GTDA_TagBase    (TAG_USER + 0x90000)

/* describe the object */

#define GTDA_Object            GTDA_TagBase + 1   /* drag node from a non-listview */
#define GTDA_GroupID           GTDA_TagBase + 2   /* Datatypes GroupID (see datatypes/datatypes.h) */
#define GTDA_Type              GTDA_TagBase + 3   /* rough format of object */
#define GTDA_InternalType      GTDA_TagBase + 4   /* internal type flag */
#define GTDA_Mask              GTDA_InternalType  /* for compatibility */
#define GTDA_Image             GTDA_TagBase + 5   /* image for dragging */
#define GTDA_RenderHook        GTDA_TagBase + 6   /* render hook for listview/object */
#define GTDA_ObjectDescription GTDA_TagBase + 7   /* pointer to struct ObjectDescription (contents will be copied) */

/* general use */

#define GTDA_Width         GTDA_TagBase + 8     /* width of icon (only for GTDA_RenderHook & GTDA_Images) */
#define GTDA_Height        GTDA_TagBase + 9     /* height of a icon ("") */
#define GTDA_NoDrag        GTDA_TagBase + 10    /* do not drag from this gadget */
#define GTDA_AcceptTypes   GTDA_TagBase + 11    /* accept mask value for internal drag&drop */
#define GTDA_AcceptMask    GTDA_AcceptTypes     /* for compatibility */
#define GTDA_AcceptFunc    GTDA_TagBase + 12    /* function which checks for acceptance of the drag */
#define GTDA_ObjectFunc    GTDA_TagBase + 13    /* callback function before a drag starts */
#define GTDA_SourceEntry   GTDA_TagBase + 21    /* specifies the dm_SourceEntry field */

/* listview specials */

#define GTDA_ItemHeight    GTDA_TagBase + 14    /* height of a listview entry */
#define GTDA_NoPosition    GTDA_TagBase + 15    /* no position highlighting and scrolling */
#define GTDA_Same          GTDA_TagBase + 16    /* can move items inside its own list */
#define GTDA_Images        GTDA_TagBase + 17    /* drags only images (listview MUST contain ImageNodes/TreeNodes) */
#define GTDA_NoScrolling   GTDA_TagBase + 18    /* disables scrolling */
#define GTDA_DropOverItems GTDA_TagBase + 19    /* objects are dropped over other items */
#define GTDA_TreeView      GTDA_TagBase + 20    /* activate treeview specials */
#define GTDA_DropBetweenItems GTDA_TagBase + 22  /* works only in conjungtion with GTDA_DropOverItems */


/** Tags to pass to GTD_AddApp() **/

#define GTDA_InternalOnly  GTDA_TagBase + 42   /* only internal drag&drop */
#define GTDA_NewStyle      GTDA_TagBase + 43   /* v3 is supported */


/** Constants for passing to GTD_GetHook() **/

#define GTDH_IMAGE 1
#define GTDH_TREE 2
#define GTDH_IFFSTREAM 3

/** data for GTDH_IFFSTREAM, set Hook's h_data to this structure **/

struct IFFStreamHookData
{
  APTR  is_Pool;        /* a memory pool, buffer will be AllocMem()ed if NULL */
  UBYTE *is_Buffer;     /* pointer to the buffer */
  ULONG is_Size;        /* size of the buffer */
  ULONG is_Position;    /* current stream position */
};


/********************* BOOPSI gadgets *********************/

#define BOOPSI_KIND 1000            /* GTD_AddGadget() - Type */

#define GMR_HANDLEYOURSELF (~0L)    /* GTD_HandleInput() return value */

#define GM_OBJECTDRAG   (GTDA_TagBase + 1000)    /* see structures below */
#define GM_OBJECTDROP   (GTDA_TagBase + 1001)
#define GM_RENDERDRAG   (GTDA_TagBase + 1002)

struct gpObjectDrag
{
  ULONG  MethodID;
  struct ObjectDescription *gpod_Object;
  struct Gadget *gpod_Source;      /* pointer to the source gadget */
  struct
  {
    WORD X,Y;                      /* mouse coordinates */
  }      gpod_Mouse;
};

/* if GM_OBJECTDRAG is invoked you should return one of these values
 * to report gtdrag that you could make use of it (to speed up things)
 */

#define GMR_REJECTOBJECT 1
#define GMR_ACCEPTOBJECT 2
#define GMR_UPDATE 4               /* please update me (GM_RENDERDRAG will be invoked) */
#define GMR_FINAL 8                /* store the result and do not ask again */


struct gpObjectDrop
{
  ULONG  MethodID;
  struct DropMessage *gpod_Message;
  ULONG  gpod_Qualifier;
};


struct gpRenderDrag
{
  ULONG  MethodID;
  struct GadgetInfo *gprd_GInfo;   /* gadget context */
  struct RastPort *gprd_RPort;     /* already for use */
  ULONG  gprd_Mode;                /* one of the GRENDER_xxx */
  struct
  {
    WORD X,Y;                      /* mouse coordinates */
  }      gprd_Mouse;
};

#define GRENDER_HIGHLIGHT 0        /* highlight yourself */
#define GRENDER_DELETE 1           /* delete all highlights */
#define GRENDER_INTERIM 2          /* refreshing between two GRENDER_HIGHLIGHT */

/* You have to return TRUE if you processed the rendering yourself,
 * otherwise gtdrag renders the standard highlighting.
 * Returning FALSE will also let gtdrag stop sending you GM_OBJECTDRAG
 * messages for the current drag.
 */



                     /*************************/
/********************* additional structures **********************\
**                  **************************                    **
**                                                                **
** The additional structures provide special rendering options in **
** listviews which are supported by gtdrag. The CallBack-Hooks    **
** necessary to display these are reachable via the API.          **
** See GTD_GetHook().                                             **
**                                                                **
\******************************************************************/


/********************* ImageNode *********************/

/* The ImageNode structure is used to have both text and images in a listview.
 * A render hook for this type is provided. It is not a must!
 */

struct ImageNode
{
  struct ImageNode *in_Succ;
  struct ImageNode *in_Pred;
#if defined(__AROS__)
  #warning FIXME when V1 ABI is out
  STRPTR in_Name;
  UBYTE  in_Type;
  BYTE   in_Pri;
#else
  UBYTE  in_Type;
  BYTE   in_Pri;
  STRPTR in_Name;
#endif
  struct Image *in_Image;
};


/********************* Tree Structures *********************/

/* The TreeNode structure provides the possibility of displaying trees
 * and images within a listview. A special tree hook is included.
 */

struct TreeList
{
  struct MinList tl_View;
  struct MinList tl_Tree;
};

struct TreeNode
{
  struct ImageNode tn_Node;
  struct MinNode tn_ViewNode;
  struct MinList tn_Nodes;
  ULONG  tn_DepthLines;
  UBYTE  tn_Depth;
  UWORD  tn_Flags;
  WORD   tn_X,tn_Y;
  APTR   tn_Special;
};

#define TNF_NONE 0
#define TNF_CONTAINER 1
#define TNF_OPEN 2
#define TNF_ADD 4
#define TNF_REPLACE 8
#define TNF_LAST 16          /* last node in group */
#define TNF_STATIC 32        /* can't move node */
#define TNF_NOSUBDIRS 64     /* can't create sub-directory (this flag is for you only) */
#define TNF_SORT 256         /* sort by name (AddTreeNode() only) */
#define TNF_HIGHLIGHTED 512  /* highlight name */


/* from the ViewNode to the TreeNode */
#define TREENODE(ln) ((struct TreeNode *)((UBYTE *)ln - sizeof(struct ImageNode)))

/* size of the knobs */
#define TREEKNOBSIZE_X 12	// tree open/close
#define TREEKNOBSIZE_Y 8
#define TREEKNOBSIZE 6		// tree add/replace

/* is the mouse pointer over the tree-knob? */
#define MouseOverTreeKnob(tn, h, msg) \
	((tn)->tn_X != -1 \
	&& (msg)->MouseX >= (tn)->tn_X \
	&& (msg)->MouseY >= (tn)->tn_Y + h \
	&& (msg)->MouseX <= (tn)->tn_X + TREEKNOBSIZE_X \
	&& (msg)->MouseY <= (tn)->tn_Y + TREEKNOBSIZE_Y + h)


#endif  /* LIBRARIES_GTDRAG_H */
