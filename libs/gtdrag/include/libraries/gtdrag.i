        IFND LIBRARIES_GTDRAG_I
LIBRARIES_GTDRAG_I    SET     1
**
**  $VER: gtdrag.i 3.9 (1.7.99)
**  Includes Release 3.4
**
**  Drag&Drop with GadTools
**
**  Copyright ©1999 pinc Software.
**  All rights Reserved.
**

*------------------------------------------------------------------------*

        IFND EXEC_LISTS_I
        INCLUDE 'exec/lists.i'
        ENDC

        IFND UTILITY_HOOKS_I
        INCLUDE 'utility/hooks.i'
        ENDC

        IFND INTUITION_CLASSUSR_I
        INCLUDE 'intuition/classusr.i'
        ENDC

*------------------------------------------------------------------------*

********************** ObjectDescription structure **********************

* the ObjectDescription structure describes the contents of the object
* being dragged.

    STRUCTURE ObjectDescription,0

      APTR   od_Object            ; the pointer to the object
      ULONG  od_GroupID           ; DataTypes-GroupID (see datatypes.h)
      ULONG  od_Type              ; rough format of the object
      ULONG  od_InternalType      ; application related type
      APTR   od_Owner             ; owner name or NULL if you are the owner (STRPTR)
      APTR   od_UserData

      LABEL  od_SIZE


* Values for od_Type

ODT_UNKNOWN   EQU 0    ; should only be used rarely
ODT_STRING    EQU 1    ; simple c-string (STRPTR)
ODT_NODE      EQU 2    ; (struct Node *)
ODT_IMAGENODE EQU 4    ; (struct ImageNode *)
ODT_TREENODE  EQU 8    ; (struct TreeNode *)
ODT_IMAGE     EQU 16   ; (struct Image *)
ODT_ICON      EQU 32   ; (struct DiskObject *)
ODT_IFF       EQU 64   ; pointer to an IFF-data stream
ODT_LOCK      EQU 128  ; a file lock
ODT_DATATYPE  EQU 256  ; a datatype object
ODT_BITMAP    EQU 512  ; (struct BitMap *)


********************** DropMessage structure **********************

* You receive the DropMessage structure if someone has dropped an object
* on a gadget or in a window of your application. The IAddress field of
* the IntuiMessage points to this structure.
* Remember: all fields are read-only!

    STRUCTURE DropMessage,0

      STRUCT dm_Node,MLN_SIZE
      STRUCT dm_Object,od_SIZE   ; dragged object
      APTR   dm_Gadget           ; source gadget
      APTR   dm_Window           ; source gadget's window
      APTR   dm_Target;          ; pointer to the target gadget
      LONG   dm_SourceEntry      ; the list position of the entry
      LONG   dm_TargetEntry      ; dto. - may be higher than the number of entries
      ULONG  dm_Flags

      LABEL  dm_SIZE

* Values for dm_Flags

DMF_DROPOVER  EQU 1              ; for treeviews


*** The flags for the IDCMP-MsgPort of your Window ***

DRAGIDCMP        EQU  LISTVIEWIDCMP!IDCMP_MOUSEBUTTONS
IDCMP_OBJECTDROP EQU  IDCMP_DISKINSERTED!IDCMP_DISKREMOVED

* If you get a message of the IDCMP_OBJECTDROP class, someone dropped
* an object on a gadget or in a window that supports it.
* The IAddress-field of the IntuiMessage points to the DragMsg in this
* case.

*** a drag key qualifier ***

IEQUALIFIER_DRAGKEY  EQU  IEQUALIFIER_LALT!IEQUALIFIER_RALT


*** Tags to pass to GTD_AddGadget() (a few also for GTD_AddWindow()) ***

GTDA_TagBase    EQU  TAG_USER+$90000

** describe the object

GTDA_Object            EQU  GTDA_TagBase+1    ; drag node from a non-listview
GTDA_GroupID           EQU  GTDA_TagBase+2    ; Datatypes GroupID (see datatypes/datatypes.h)
GTDA_Type              EQU  GTDA_TagBase+3    ; rough format of object
GTDA_InternalType      EQU  GTDA_TagBase+4    ; internal type flag
GTDA_Mask              EQU  GTDA_InternalType ; for compatibility
GTDA_Image             EQU  GTDA_TagBase+5    ; image for dragging
GTDA_RenderHook        EQU  GTDA_TagBase+6    ; render hook for listview/object
GTDA_ObjectDescription EQU  GTDA_TagBase+7    ; pointer to struct ObjectDescription (contents will be copied)

** general use

GTDA_Width         EQU  GTDA_TagBase+8     ; width of icon (only for GTDA_RenderHook & GTDA_Images)
GTDA_Height        EQU  GTDA_TagBase+9     ; height of a icon ("")
GTDA_NoDrag        EQU  GTDA_TagBase+10    ; do not drag from this gadget
GTDA_AcceptTypes   EQU  GTDA_TagBase+11    ; accept mask value for internal drag&drop
GTDA_AcceptMask    EQU  GTDA_AcceptTypes   ; for compatibility
GTDA_AcceptFunc    EQU  GTDA_TagBase+12    ; function which checks for acceptance of the drag
GTDA_ObjectFunc    EQU  GTDA_TagBase+13    ; callback function before a drag starts
GTDA_SourceEntry   EQU  GTDA_TagBase+21    ; specifies the dm_SourceEntry field

** listview specials

GTDA_ItemHeight    EQU  GTDA_TagBase+14    ; height of a listview entry
GTDA_NoPosition    EQU  GTDA_TagBase+15    ; no position highlighting and scrolling
GTDA_Same          EQU  GTDA_TagBase+16    ; can move items inside its own list
GTDA_Images        EQU  GTDA_TagBase+17    ; drags only images (listview MUST contain ImageNodes/TreeNodes)
GTDA_NoScrolling   EQU  GTDA_TagBase+18    ; disables scrolling
GTDA_DropOverItems EQU  GTDA_TagBase+19    ; objects are dropped over other items
GTDA_TreeView      EQU  GTDA_TagBase+20    ; activate treeview specials
GTDA_DropBetweenItems EQU GTDA_TagBase+22  ; works only in conjungtion with GTDA_DropOverItems


*** Tags to pass to GTD_AddApp() ***

GTDA_InternalOnly  EQU  GTDA_TagBase+42    ; only internal drag&drop
GTDA_NewStyle      EQU  GTDA_TagBase+43    ; v3 is supported


*** Constants for passing to GTD_GetHook() ***

GTDH_IMAGE      EQU  1
GTDH_TREE       EQU  2
GTDH_IFFSTREAM  EQU  3

*** data for GTDH_IFFSTREAM, set Hook's h_data to this structure ***

    STRUCTURE IFFStreamHookData,0

      APTR   is_Pool           ; a memory pool, buffer will be AllocMem()ed if NULL
      APTR   is_Buffer         ; pointer to the buffer
      ULONG  is_Size;          ; size of the buffer
      ULONG  is_Position;      ; current stream position

      LABEL  ifshd_SIZE


********************** BOOPSI gadgets **********************

BOOPSI_KIND    EQU  1000                   ; GTD_AddGadget() - Type

GM_OBJECTDRAG  EQU  GTDA_TagBase+1000      ; see structures below
GM_OBJECTDROP  EQU  GTDA_TagBase+1001
GM_RENDERDRAG  EQU  GTDA_TagBase+1002


    STRUCTURE gpObjectDrag,4

      ; ULONG  MethodID
      APTR   gpod_Object       ; pointer to an ObjectDescription
      APTR   gpod_Source       ; pointer to the source gadget
      WORD   gpod_X
      WORD   gpod_Y            ; mouse coordinates

      LABEL  gpodrag_SIZE

* if GM_OBJECTDRAG is invoked you should return one of these values
* to report gtdrag that you could make use of it (to speed up things)

GMR_REJECTOBJECT  EQU  1
GMR_ACCEPTOBJECT  EQU  2
GMR_UPDATE        EQU  4       ; please update me (GM_RENDERDRAG will be invoked)
GMR_FINAL         EQU  8       ; store the result and do not ask again


    STRUCTURE gpObjectDrop,4

      ; ULONG  MethodID
      APTR   gpod_Message      ; pointer to a DropMessage
      ULONG  gpod_Qualifier

      LABEL  gpodrop_SIZE


    STRUCTURE gpRenderDrag,4

      ; ULONG  MethodID
      APTR   gprd_GInfo        ; gadget context
      APTR   gprd_RPort        ; already for use
      ULONG  gprd_Mode         ; one of the GRENDER_xxx
      WORD   gprd_MouseX
      WORD   gprd_MouseY       ; mouse coordinates

      LABEL  gprd_SIZE

GRENDER_HIGHLIGHT    EQU 0     ; highlight yourself
GRENDER_DELETE       EQU 1     ; delete all highlights
GRENDER_INTERIM      EQU 2     ; refreshing between two GRENDER_HIGHLIGHT

* You have to return TRUE if you processed the rendering yourself,
* otherwise gtdrag renders the standard highlighting.
* Returning FALSE will also let gtdrag stop sending you GM_OBJECTDRAG
* messages for the current drag.



                     ***************************
********************** additional structures ***********************
**                  **************************                    **
**                                                                **
** The additional structures provide special rendering options in **
** listviews which are supported by gtdrag. The CallBack-Hooks    **
** necessary to display these are reachable via the API.          **
** See GTD_GetHook().                                             **
**                                                                **
********************************************************************


********************** ImageNode **********************

* The ImageNode structure is used to have both text and images in a listview.
* A render hook for this type is provided. It is not a must!


    STRUCTURE ImageNode,0

      APTR  in_Succ        ; same as ListNode
      APTR  in_Pred
      UBYTE in_Type
      BYTE  in_Pri
      APTR  in_Name
      APTR  in_Image       ; plus pointer to an Image

      LABEL in_SIZE


********************** Tree Structures **********************

* The TreeNode structure provides the possibility of displaying trees
* and images within a listview. A special tree hook is included.


    STRUCTURE TreeList,0

      STRUCT tl_View,MLH_SIZE
      STRUCT tl_Tree,MLH_SIZE

      LABEL tl_SIZE


    STRUCTURE gTreeNode,0

      STRUCT gtn_Node,in_SIZE       ; avoid complications with datatypes/datatypes.i
      STRUCT gtn_ViewNode,MLN_SIZE
      STRUCT gtn_Nodes,MLH_SIZE
      ULONG  gtn_DepthLines
      UBYTE  gtn_Depth
      UWORD  gtn_Flags              ; ""
      WORD   gtn_X
      WORD   gtn_Y
      APTR   gtn_Special

      LABEL  gtn_SIZE


TNF_NONE        EQU 0
TNF_CONTAINER   EQU 1
TNF_OPEN        EQU 2
TNF_ADD         EQU 4
TNF_REPLACE     EQU 8
TNF_LAST        EQU 16     ; last node in group
TNF_STATIC      EQU 32     ; can't move node
TNF_NOSUBDIRS   EQU 64     ; can't create sub-directory (this flag is for you only)
TNF_SORT        EQU 256    ; sort by name
TNF_HIGHLIGHTED EQU 512    ; highlight name


****** the following macros are C macros ******

** from the ViewNode to the TreeNode **
* #define TREENODE(ln) ((struct TreeNode *)((UBYTE *)ln - sizeof(struct ImageNode)))

** size of the knobs **
TREEKNOBSIZE  EQU  6

** is the mouse pointer over the tree-knob? **
* #define MouseOverTreeKnob(tn,h,msg) ((tn)->tn_X != -1 && (msg)->MouseX >= (tn)->tn_X && (msg)->MouseY >= (tn)->tn_Y+h && (msg)->MouseX <= (tn)->tn_X+TREEKNOBSIZE && (msg)->MouseY <= (tn)->tn_Y+TREEKNOBSIZE+h)


ENDC  ; LIBRARIES_GTDRAG_I
