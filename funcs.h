/* Public function prototypes
 *
 * Copyright 1996-2009 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */
#ifndef IGN_FUNCS_H
#define IGN_FUNCS_H

#if defined(__MORPHOS__)
//#include <stdbool.h>
//#include "SDI_compiler.h"
typedef BYTE int8;
typedef UBYTE uint8;
typedef WORD int16;
typedef UWORD uint16;
typedef LONG int32;
typedef ULONG uint32;
#endif

// boopsi.c
extern void EraseFatRect(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2);
extern void DrawFatRect(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2);
extern void FreeAppClasses(void);
extern void InitAppClasses(void);

// hooks.c
extern void GhostRect(struct RastPort *rp, UWORD pen, UWORD x0, UWORD y0, UWORD x1, UWORD y1);
extern void FreeLinks(struct MinList *links);
extern struct Link *AddLinkNode(struct MinList *links, struct MinNode *ln, APTR func);
extern void AddLinkList(struct MinList *links, struct MinList *add, APTR func);

// images.c
extern void FreeImageObj(struct ImageObj *io);
extern struct ImageObj *AddImageObj(STRPTR t, struct Image *im);
extern void RefreshImageObjs(struct Screen *scr);
extern void FreeImage(struct Image *img);
extern struct Image *LoadImage(STRPTR t);
extern void FreeImages(void);
extern void AllocImages(struct Screen *scr);
	
// serial.c
extern void InitSerialNumber(void);
extern int32 CheckSerialNumber(bool init);
 
// handlewindows.c
extern void UpdateGInterface(struct Window *giwin, struct MinList *list, struct gObject *go, UBYTE page);
extern void HandleGGadget(struct Page *page, struct gObject *go);
extern void UpdateObjectGadgets(struct Window *win);
extern void PUBLIC HandleFileTypeIDCMP(REG(a0, struct Hook *), REG(a2, struct FileRequester *fr), REG(a1, struct IntuiMessage *msg));

// pointer.c
extern void SetMousePointer(struct Window *win, ULONG which);
extern void InitPointers(void);
extern void FreePointers(void);

// clip.c
extern void SetNewTFOrigin(struct MinList *list);
extern void PasteCells(struct Page *page, struct MinList *mlh, UBYTE mode);
extern struct PasteNode *CutCopyClip(struct Page *page, UBYTE mode);
extern struct PasteNode *PasteClip(struct Page *page, struct PasteNode *pn, UBYTE mode);
extern struct gGroup *CopyGGroup(struct gGroup *, bool objects);
extern struct gGroup *DuplicateGGroup(struct Page *page, struct gGroup *cgg, struct UndoNode *un);

// undo.c
extern void AddUndoLink(struct MinList *list, APTR obj);
extern struct UndoNode *CreateUndo(struct Page *page, UBYTE type, CONST_STRPTR t);
extern struct UndoNode *BeginUndo(struct Page *page, UBYTE type, CONST_STRPTR t);
extern void EndUndo(struct Page *);
extern void FreeUndo(struct Page *page, struct UndoNode *un);
extern void MakeUndoRedoList(struct Page *page, struct UndoNode *un, struct MinList *list);
extern void MakeCellSizeUndo(struct UndoNode *un, UBYTE flags, ULONG mm, long pixel, long pos);
extern void ApplyObjectSizeUndoRedo(struct Page *page, struct UndoNode *un, char type);
extern void ApplyObjectsMoveUndoRedo(struct Page *page, struct UndoNode *un, char type);
extern bool ApplyUndo(struct Page *page);
extern bool ApplyRedo(struct Page *page);

// edit.c
extern bool DocumentSecurity(struct Page *page, struct tableField *tf);
extern void SetTabGadget(struct Page *page, STRPTR t, long pos);
extern void FreeTabGadget(struct Page *page);
extern void HandleTabGadget(struct Page *page);
extern void CreateTabGadget(struct Page *page, long col, long row, bool makeVisible);
extern bool QueryPassword(CONST_STRPTR t, STRPTR password);
extern ULONG PUBLIC PasswordEditHook(REG(a0, struct Hook *hook), REG(a2, struct SGWork *sgw), REG(a1, ULONG *msg));

// database.c
extern bool IsDBEmpty(struct Database *db);
extern struct tableField *GetFields(struct Database *db, long pos);
extern STRPTR AllocFieldText(struct Database *db, struct tableField *tf, long pos);
extern struct Term *GetField(struct Database *db, STRPTR t);
extern void CreateFields(struct Database *db);
extern void UpdateMaskCell(struct Mappe *mp, struct Page *page, struct tableField *tf, struct UndoNode *un);
extern struct Mask *IsOverMask(struct Page *page);
extern struct Mask *GuessMask(struct Page *page);
extern long *GetDBReferences(struct Database *db, struct Database *pdb);
extern void RefreshMaskFields(struct Mappe *mp, bool refresh);
extern void SetDBCurrent(struct Database *db, UBYTE mode, long pos);
extern void UpdateDBCurrent(struct Database *db, ULONG current);
extern void UpdateIndices(struct Database *db);
extern bool MakeIndex(struct Database *db, struct Index *in);
extern bool MakeFilter(struct Database *db, struct Filter *fi);
extern void PrepareFilter(struct Database *db, struct Term *t);
extern void SetFilter(struct Database *db, struct Filter *fi);
extern void FreeFilter(struct Filter *fi);
extern void MakeSearchFilter(struct Database *db, struct Mask *ma);

// printer.c
extern void FreeWDTPrinter(struct List *list);
extern void PrintProject(struct List *list, struct wdtPrinter *wp, WORD unit, ULONG flags);
extern bool ReadUnitName(char *filename, char *name, int unit);
extern void UpdatePrinterGadgets(struct Window *win, struct winData *wd);
extern void InitPrinter(void);

// lock.c
extern struct LockNode *FindLockNode(struct MinList *list, ULONG length, ...) VARARGS68K;
extern void RemLockNode(struct LockNode *ln);
extern void RemLockNodeData(struct MinList *list, ULONG length,...) VARARGS68K;
extern struct LockNode *AddLockNode(struct MinList *list, BYTE pri, APTR func, ULONG length, ...) VARARGS68K;
extern struct LockNode *AddFreeLock(APTR list, struct Window *win);
extern struct LockNode *AddListViewLock(struct MinList *list, struct Window *win, struct Gadget *gad);
extern struct LockNode *AddTreeLock(struct MinList *list, struct Window *win, struct Gadget *gad);
extern struct LockNode *AddTextLock(struct MinList *list, struct Window *win, struct Gadget *gad);
extern void UnlockListNode(struct MinList *list, struct MinNode *node, UBYTE flags);
extern bool LockListNode(struct MinList *list, struct MinNode *node, UBYTE flags);
extern void UnlockList(struct MinList *list, UBYTE flags);
extern bool LockList(struct MinList *list, UBYTE flags);
extern void RefreshLockList(struct MinList *list);
extern void FreeLockList(APTR list);
extern void AddLockedTail(struct MinList *list, struct MinNode *ln);
extern void AddLockedHead(struct MinList *list, struct MinNode *ln);
extern void RemoveFromLockedList(struct MinList *list, struct MinNode *ln);
extern void RemoveLocked(struct MinNode *ln);
extern struct MinNode *RemLockedHead(struct MinList *list);
extern void PUBLIC FreeLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags));
extern void PUBLIC ListViewLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags));
extern void PUBLIC TreeLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags));
extern void PUBLIC TextLock(REG(a0, struct LockNode *ln), REG(a1, struct MinNode *node), REG(d0, UBYTE flags));

// search.c
extern void SearchReplace(struct Page *page, UWORD mode);
extern void InitSearch(void);

// cmd.c
extern void FreeCommandArray(struct Node **array, ULONG arraySize);
extern struct Node **BuildCommandArray(struct MinList *list, ULONG *arraySize);
extern ULONG processIntCmd(STRPTR t);
extern long ProcessAppCmd(struct Page *page, STRPTR t);
extern struct AppCmd *NewAppCmd(struct AppCmd *ac);
extern void FreeAppCmd(struct AppCmd *ac);
extern struct AppCmd *FindAppCmd(struct Page *page, STRPTR t);
extern void CopyAppKeys(struct MinList *from, struct MinList *to);
extern void FreeAppKeys(struct MinList *list);
extern void SetAppKeyName(struct AppKey *ak);
extern long handleKey(struct Page *page, struct AppKey *ak);

// rexx.c
extern void EditRexxScript(struct Mappe *mp, struct RexxScript *rxs);
extern void FreeRexxScriptData(struct RexxScript *rxs);
extern void DeleteRexxScript(struct Mappe *mp, struct RexxScript *rxs);
extern void NotifyRexxScript(struct RexxScript *rxs);
extern struct RexxScript *NewRexxScript(struct Mappe *mp, STRPTR name, BYTE header);
extern void handleRexx(struct RexxMsg *);
extern long handleEvent(struct Page *page, BYTE type, long col, long row);
extern void RemoveRexxPort(struct RexxPort *rxp);
extern void FreeRexxScript(struct RexxScript *rxs);
extern ULONG RunRexxScript(UBYTE type, STRPTR name);
#ifdef __amigaos4__
extern ULONG RexxCall(ULONG (*func)(long *opts),long p1, ...) VARARGS68K;
#else
extern ULONG RexxCall(ULONG (*func)(long *opts), ...) VARARGS68K;
#endif
extern void CloseRexx(void);
extern void initRexx(void);

// menu.c
extern void FreeAppMenus(struct MinList *);
extern void CopyAppMenus(struct MinList *from, struct MinList *to);
extern void FreeAppMenu(struct Prefs *pr);
extern void RefreshMenu(struct Prefs *pr);
extern void HandleMenu(void);
extern void UpdateMenuSpecials(struct Prefs *pr);
extern void CalculateContextMenu(struct MinList *list);
extern void FreeContextMenu(struct MinList *list);
extern struct ContextMenu *AddContextMenu(struct MinList *list, STRPTR title, STRPTR cmd);
extern void CopyContextMenu(struct MinList *from, struct MinList *to);
extern void RefreshContextMenus(struct Prefs *pr);
extern void InitContextMenus(struct Prefs *pr);
extern void HandleContext(struct Page *page, LONG type, ULONG col, ULONG row);

// support.c
extern void Rect32ToRect(struct Rect32 *rect32, struct Rectangle *rect);
extern int cmdcmp(STRPTR *c1, STRPTR *c2);
extern int32 cmdlen(STRPTR t);
extern STRPTR PUBLIC AllocString(REG(a0, CONST_STRPTR string));
extern STRPTR PUBLIC AllocStringLength(REG(a0, STRPTR string), REG(d0, long len));
extern void PUBLIC FreeString(REG(a0, STRPTR string));
extern void zstrcpy(STRPTR to, STRPTR from);
extern int zstrcmp(STRPTR a, STRPTR b);
extern int zstricmp(STRPTR a, STRPTR b);
extern int zstrnicmp(STRPTR a, STRPTR b, LONG len);
extern void strdel(STRPTR t, long len);
#ifdef __amigaos4__
extern void ReplaceTextString(char *sstr, char *rstr, char *nstr, int8 pos);
#endif
extern void StringToUpper(STRPTR t);
extern struct Node *FindCommand(struct MinList *, STRPTR);
extern struct Node *FindTag(struct MinList *, STRPTR);
extern STRPTR GetUniqueName(struct MinList *list, STRPTR base);
extern void MakeUniqueName(struct MinList *list, STRPTR *name);
extern int32 CountNodes(struct MinList *);
extern long GetListWidth(struct MinList *);
extern void moveList(struct MinList *, struct MinList *);
extern void swapLists(struct MinList *, struct MinList *);
extern long compareType(struct Node **lna, struct Node **lnb);
extern long LinkNameSort(struct Link **la, struct Link **lb);
extern void freeSort(APTR sortBuffer, ULONG len);
extern APTR allocSort(struct MinList *l, ULONG *len);
extern void sortList(struct MinList *);
extern void SortTypeList(struct MinList *);
extern void SortListWith(struct MinList *l, APTR func);
extern void InsertAt(struct MinList *l, struct Node *n, long pos);
extern void MoveTo(struct Node *n, struct MinList *l1, long pos1, struct MinList *l2, long pos2);
extern struct NumberLink *FindLink(struct MinList *list, APTR link);
extern struct MinList *FindList(struct MinNode *ln);
extern struct Node *FindListNumber(struct MinList *l, long num);
extern long FindListEntry(struct MinList *l, struct MinNode *n);
extern void ProcentToString(ULONG p, STRPTR t);
extern double ConvertDegreeProcent(STRPTR s);
extern int32 ConvertTime(STRPTR s);
extern double ConvertNumber(STRPTR s, UBYTE targettype);
extern void WriteChunkString(APTR iff, STRPTR t);
extern void MakeLocaleStrings(struct MinList *list, LONG id, ...) VARARGS68K;
extern struct List *MakeLocaleStringList(LONG id, ...) VARARGS68K;
extern void FreeStringList(struct List *list);
extern void MakeStrings(struct MinList *list, const STRPTR string, ...)	VARARGS68K;
extern struct List *MakeStringList(const STRPTR string, ...) VARARGS68K;
extern ULONG DoClassMethodA(Class *cl, Msg msg);
extern bool IsDoubleClick(WORD entry);
extern struct Node *HandleLVRawKeys(struct Gadget *lvgad, struct Window *win, struct MinList *list, long itemheight);
extern ULONG GetCheckBoxFlag(struct Gadget *gad, struct Window *win, ULONG flag);
extern LONG WordWrapText(struct List *lh, STRPTR t, long width);
extern void FreeListItems(struct MinList *list, APTR free);
extern void CopyListItems(struct MinList *from, struct MinList *to, APTR copy);
extern struct MinNode *FindLinkCommand(struct MinList *list, STRPTR command);
extern struct Link *FindLinkWithName(struct MinList *list, STRPTR name);
extern struct MinNode *FindLinkName(struct MinList *list, STRPTR name);
extern void AddLink(struct MinList *list, struct MinNode *node, APTR func);
extern LONG GetListNumberOfName(struct MinList *mlh, STRPTR name, bool cmd,	bool link);
extern bool AddToNameList(STRPTR buffer, STRPTR t, int *plen, int maxlength);
extern struct Library *IgnOpenClass(STRPTR secondary, STRPTR name, LONG version);
extern void RemoveFromArrayList(struct ArrayList *al, void *entry);
extern int32 AddToArrayList(struct ArrayList *al, void *entry);
extern bool AddListToArrayList(struct ArrayList *source, struct ArrayList *dest);
extern bool CopyArrayList(struct ArrayList *source, struct ArrayList *dest);
extern void MakeEmptyArrayList(struct ArrayList *al);
#define CountArrayListItems(al) (al)->al_Last
#define GetArrayListArray(al) (al)->al_List
extern void SetLocalizedName(STRPTR names[10], STRPTR *_name, STRPTR line);
extern void GetLocalizedName(STRPTR names[10], STRPTR *_name, int *_language);

#ifdef DEBUG
extern void dump(STRPTR pre, UBYTE *a, long len);
#else
#  define dump(a, b, c) ;
#endif

// project.c
extern struct Page * PUBLIC NewPage(REG(a0, struct Mappe *mp));
extern struct Mappe *NewProject(void);
extern void DisposePage(struct Page *page);
extern bool DisposeProject(struct Mappe *mp);
extern void SetMediumSize(struct Mappe *mp);
extern void SetMapName(struct Mappe *mp, STRPTR name);
extern void UpdateModified(struct Mappe *mp);
extern void SetMainPage(struct Page *page);
extern void UpdateMapTitle(struct Mappe *mp);
extern STRPTR GetPageTitle(struct Page *page);
extern void SetProjectMouseReport(struct Page *page, bool set);
#ifdef __amigaos4__
extern void UpdatePageFont(struct Page *page,  ...) VARARGS68K;
#else
extern void UpdatePageFont(struct Page *page, ULONG tag, ...) VARARGS68K;
#endif
extern void UpdateProjPage(struct Window *win, struct Page *page);
extern void ProjectToGObjects(struct Page *page, struct winData *wd);
extern void ASM handleProjIDCMP(REG(a0, struct TagItem *tags));
extern void drawSelect(struct RastPort *rp, long x1, long y1, long x2, long y2);
extern void SetCellSecurity(struct Page *page, LONG security);
extern void SetCellPattern(struct Page *page, long col, UBYTE pattern);
extern void SetAlignment(struct Page *page, BYTE alignH, BYTE alignV);
extern void SetMark(struct Page *, long, long, long, long);
extern void SetPageColor(struct Page *, struct colorPen *, struct colorPen *);
extern void SetPageProps(struct Page *);
extern void insertFormel(struct Page *page, STRPTR t);

// ignition.c
extern void NormalizeWindowBox(struct IBox *box);
#ifdef __amigaos4__
extern struct Window *OpenProjWindow(struct Page *,  ...) VARARGS68K;
#else
extern struct Window *OpenProjWindow(struct Page *, ULONG, ...) VARARGS68K;
#endif
extern struct Gadget *MakeProjectGadgets(struct winData *, long w, long h);
extern void MakeMarkText(struct Page *page, STRPTR t);
extern void DisplayTablePos(struct Page *page);
extern void DrawStatusFlags(struct Mappe *mp, struct Window *win);
extern void DrawStatusText(struct Page *page, CONST_STRPTR t);
extern void DrawHelpText(struct Window *win, struct Gadget *gad, CONST_STRPTR t);
extern void DrawBars(struct Window *win);
extern void RefreshToolBar(struct Page *);
extern void FreeSession(struct Session *s);
extern void RefreshSession(void);
extern struct Session *FindInSession(struct Mappe *mp);
extern void AddToSession(struct Mappe *mp);
extern void UpdateInteractive(struct Mappe *mp, bool refresh);
extern ULONG IsOverProjSpecial(struct Window *win, LONG x, LONG y);
extern void MakeFewFuncs(void);
extern void FreeWinIconObjs(struct Window *, struct winData *);
extern void RefreshProjectWindow(struct Window *win, bool all);
extern void RefreshProjWindows(bool all);
extern void EmptyMsgPort(struct MsgPort *port);
extern struct MinList *GetIconObjsList(struct Prefs *pr);
extern void FreeIconObjs(struct MinList *list);
extern struct IconObj *CopyIconObj(struct IconObj *io);
extern void AddIconObj(struct MinList *list, struct AppCmd *ac, long);
extern void FreeAppIcon(void);
extern void InitAppIcon(void);
extern void InitHelp(struct Screen *scr);

/************ misc inline functions ************/

static __inline long
pt2pixel(struct Page *page, long pt, bool width)
{
	return (long)(pt*(width ? page->pg_DPI >> 16 : page->pg_DPI & 0xffff)
		/ (72*256));
}

#define zstrlen(a) (a ? strlen(a) : 0)

#ifdef __amigaos4__
extern STRPTR GetString(struct LocaleInfo *li, LONG stringNum);
#endif

#endif	/* IGN_FUNCS_H */
		  
