/* AppCmd & AppKeys implementation
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include "funcs.h"

#ifdef __amigaos4__
	#include <devices/conunit.h>
	struct ConsoleIFace *IConsole;
#else
	#define ConsoleDevice conDevice
#endif

struct Library *conDevice = NULL;
struct Node **intCmdArray;
ULONG intCmdArraySize;


struct AppCmd *
NewAppCmd(struct AppCmd *ac)
{
    struct AppCmd *sac;
    struct Command *cmd,*scmd;

	if ((sac = AllocPooled(pool, sizeof(struct AppCmd))) != 0) {
		if (ac) {
			CopyMem(ac, sac, sizeof(struct AppCmd));
            sac->ac_Node.in_Name = AllocString(ac->ac_Node.in_Name);
            sac->ac_HelpText = AllocString(ac->ac_HelpText);
            if ((sac->ac_ImageName = AllocString(ac->ac_ImageName)) != 0)
                sac->ac_Node.in_Image = LoadImage(sac->ac_ImageName);
            MyNewList(&sac->ac_Cmds);
			foreach (&ac->ac_Cmds,cmd) {
				if ((scmd = AllocPooled(pool, sizeof(struct Command))) != 0) {
                    scmd->cmd_Name = AllocString(cmd->cmd_Name);
                    scmd->cmd_Pri = cmd->cmd_Pri;
                    scmd->cmd_Type = cmd->cmd_Type;
                    MyAddTail(&sac->ac_Cmds, scmd);
                }
            }
		} else
            MyNewList(&sac->ac_Cmds);
    }
    return sac;
}


void
FreeAppCmd(struct AppCmd *ac)
{
    struct Command *cmd;

    if (!ac)
        return;

	while ((cmd = (APTR)MyRemHead(&ac->ac_Cmds)) != 0) {
        FreeString(cmd->cmd_Name);
        FreePooled(pool,cmd,sizeof(struct Command));
    }
    FreeImage(ac->ac_Node.in_Image);
    FreeString(ac->ac_Node.in_Name);
    FreeString(ac->ac_ImageName);
    FreeString(ac->ac_HelpText);
    FreePooled(pool, ac, sizeof(struct AppCmd));
}


struct AppCmd *
FindAppCmd(struct Page *page, STRPTR t)
{
    if (page)
        return (struct AppCmd *)FindLinkName(&page->pg_Mappe->mp_AppCmds,t);

    return (struct AppCmd *)MyFindName(&prefs.pr_AppCmds, t);
}


void
CopyAppKeys(struct MinList *from,struct MinList *to)
{
    struct AppKey *ak,*sak;

    foreach(from, sak)
    {
        if ((ak = AllocPooled(pool, sizeof(struct AppKey))) != 0)
        {
            *ak = *sak;
            ak->ak_Node.ln_Name = AllocString(sak->ak_Node.ln_Name);
            ak->ak_AppCmd = AllocString(sak->ak_AppCmd);
            MyAddTail(to, ak);
        }
    }
}


void
FreeAppKeys(struct MinList *list)
{
    struct AppKey *ak;

    while((ak = (struct AppKey *)MyRemHead(list)) != 0)
    {
        FreeString(ak->ak_Node.ln_Name);
        FreeString(ak->ak_AppCmd);
        FreePooled(pool,ak,sizeof(struct AppKey));
    }
}


long
GetASCII(struct InputEvent *ievent,STRPTR buffer,long buflen,UWORD code)
{
    ievent->ie_Class = IECLASS_RAWKEY;
    ievent->ie_Code = code;
    ievent->ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
    ievent->ie_position.ie_addr = imsg.IAddress ? *((APTR *)imsg.IAddress) : NULL;

    return RawKeyConvert(ievent,buffer,buflen,NULL);
}


BOOL
FindVanillaName(STRPTR t, UWORD code)
{
	switch (code)
    {
        case 8:
            strcat(t,"backspace");  break;
        case 9:
            strcat(t,"tab");  break;
        case 13:
            strcat(t,"return");  break;
        case 27:
            strcat(t,"esc");  break;
        case 32:
            strcat(t,"space");  break;
        case 127:
            strcat(t,"del");  break;
        default:
            if (code > 32 && code < 127)
                sprintf(&t[strlen(t)],"%c",tolower(code));
            else
                return FALSE;
            break;
    }
    return TRUE;
}

void
SetAppKeyName(struct AppKey *ak)
{
    struct InputEvent *ievent;
    struct IOStdReq conReq;
    ULONG  class,len,i;
    UWORD  code,qual;
    UBYTE  t[64],buffer[8];

#ifdef __amigaos4__
	if (!ak || OpenDevice("console.device", CONU_LIBRARY, (struct IORequest *)&conReq, 0))
    	return;
    	
    struct ConsoleDevice *ConsoleDevice = (struct ConsoleDevice *)conReq.io_Device;
    IConsole = (struct ConsoleIFace*)GetInterface((struct Library*)ConsoleDevice, "main", 1, NULL);
#else
    if (!ak || OpenDevice("console.device",-1,(struct IORequest *)&conReq,0))
        return;
#endif
    if ((ievent = AllocPooled(pool, sizeof(struct InputEvent))) != 0)
    {
        conDevice = (struct Library *)conReq.io_Device;
        FreeString(ak->ak_Node.ln_Name);
        class = ak->ak_Class;  code = ak->ak_Code;  qual = ak->ak_Qualifier;  strcpy(t,"");
        if (qual & IEQUALIFIER_NUMERICPAD)
            strcat(t,"<num> ");
        if (qual & IEQUALIFIER_CONTROL)
            strcat(t,"ctrl ");
        if (qual & IEQUALIFIER_COMMAND)
        {
            if ((qual & IEQUALIFIER_COMMAND) == IEQUALIFIER_RCOMMAND)
                strcat(t,"r");
            else if ((qual & IEQUALIFIER_COMMAND) == IEQUALIFIER_LCOMMAND)
                strcat(t,"l");
            strcat(t,"amiga ");
        }
        if (qual & IEQUALIFIER_SHIFT && !(class == IDCMP_VANILLAKEY && (code > 32 && code < 65 || code > 90 && code < 127)))
        {
            /*if ((qual & IEQUALIFIER_SHIFT) == IEQUALIFIER_RSHIFT)
                strcat(t,"r");
            else if ((qual & IEQUALIFIER_SHIFT) == IEQUALIFIER_LSHIFT)
                strcat(t,"l");*/
            strcat(t,"shift ");
        }
        if (qual & IEQUALIFIER_ALT)
        {
            /*if ((qual & IEQUALIFIER_ALT) == IEQUALIFIER_RALT)
                strcat(t,"r");
            else if ((qual & IEQUALIFIER_ALT) == IEQUALIFIER_LALT)
                strcat(t,"l");*/
            strcat(t,"alt ");
        }
        if (class == IDCMP_VANILLAKEY)
        {
            if (!FindVanillaName(t,code))
            {
                ievent->ie_Class = IECLASS_RAWKEY;
                ievent->ie_Qualifier = qual;
                ievent->ie_position.ie_addr = NULL/**((APTR *)imsg.IAddress)*/;
                *buffer = 255;
                for(i = 0;code != *buffer && i < 128;i++)
                {
                    ievent->ie_Code = i;
                    /*len =*/ RawKeyConvert(ievent,buffer,8,NULL);
                }
                ievent->ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
                len = RawKeyConvert(ievent,buffer,8,NULL);
                if (len == -1 || !FindVanillaName(t,(UWORD)*buffer))
                    sprintf(&t[strlen(t)],"0x%lx",code);
            }
        }
        else
        {
            switch(code)
            {
                case CURSORUP:
                    strcat(t,"c.up");  break;
                case CURSORDOWN:
                    strcat(t,"c.down");  break;
                case CURSORLEFT:
                    strcat(t,"c.left");  break;
                case CURSORRIGHT:
                    strcat(t,"c.right");  break;
                case 66:
                    strcat(t,"tab");  break;
                case 95:
                    strcat(t,"help");  break;
                default:
                    if (code >= 80 && code < 90)
                        sprintf(&t[strlen(t)],"f%ld",code-79);
                    else
                    {
                        len = GetASCII(ievent,buffer,8,code);
                        if (len != -1)
                            strncat(t,buffer,len);
                        else
                            strcat(t,"##err");
                    }
                    break;
            }
        }
        ak->ak_Node.ln_Name = AllocString(t);
    }
#ifdef __amigaos4__
    DropInterface((struct Interface *)IConsole);
    CloseDevice((struct IORequest *)&conReq);
#else
    ConsoleDevice = NULL;
    CloseDevice((struct IORequest *)&conReq);
#endif
}


struct AppKey *
SearchAppKey(struct Page *page,struct MinList *list)
{
    struct AppKey *ak;
    UWORD  qual;

	foreach (list, ak)
    {
        if (ak->ak_Class == imsg.Class && ak->ak_Code == imsg.Code)
        {
            if (ak->ak_Node.ln_Type == AKT_ALWAYS || page->pg_Gad.DispPos > PGS_FRAME && ak->ak_Node.ln_Type == AKT_EDITONLY || page->pg_Gad.DispPos <= PGS_FRAME && ak->ak_Node.ln_Type == AKT_NOEDIT)
            {
                if (((qual = ak->ak_Qualifier) & IEQUALIFIER_CONTROL) == (imsg.Qualifier & IEQUALIFIER_CONTROL))
                {
                    if (((qual & IEQUALIFIER_SHIFT) > 0) == ((imsg.Qualifier & IEQUALIFIER_SHIFT) > 0))
                    {
                        if (((qual & IEQUALIFIER_ALT) > 0) == ((imsg.Qualifier & IEQUALIFIER_ALT) > 0))
                        {
                            if ((qual & IEQUALIFIER_COMMAND) == (imsg.Qualifier & IEQUALIFIER_COMMAND))
                                return ak;
                        }
                    }
                }
            }
        }
    }
    return NULL;
}


long
handleKey(struct Page *page,struct AppKey *ak)
{
    if (!ak && (imsg.Class == IDCMP_RAWKEY || imsg.Class == IDCMP_VANILLAKEY))
    {
        /* für richtige Benutzung von add/remove (ist vielleicht zu lahm und überflüssig) */
        /*struct Prefs *pr = &page->pg_Mappe->mp_Prefs;
        struct PrefsModule *pm = GetPrefsModule(pr,WDT_PREFKEYS);

        if (!(ak = SearchAppKey(page,&pr->pr_AppKeys)) && (!pm || pm && pm->pm_Flags & PMF_ADD))*/

        if (!(ak = SearchAppKey(page,&page->pg_Mappe->mp_Prefs.pr_AppKeys)))
            ak = SearchAppKey(page,&prefs.pr_AppKeys);
    }
    if (ak)
        ProcessAppCmd(page,ak->ak_AppCmd);

    return (long)ak;
}


STRPTR cmdbuffer;
long   cmdbuflen;


STRPTR
EnlargeCommandBuffer(void)
{
    STRPTR temp;

    if ((temp = AllocPooled(pool, cmdbuflen + 256)) != 0)
    {
        if (cmdbuffer)
        {
            CopyMem(cmdbuffer,temp,cmdbuflen);
            FreePooled(pool,cmdbuffer,cmdbuflen);
        }
        cmdbuffer = temp;
        cmdbuflen += 256;
    }
    return cmdbuffer;
}


STRPTR
BuildCommand(STRPTR t)
{
    STRPTR s = cmdbuffer;
    int    i = 0,j;

    t += cmdlen(t);
    if (*t)
    {
        t++;

        for(i = 0;*t;i++)
        {
            if (i >= cmdbuflen-3)
                s = EnlargeCommandBuffer();

            if (*t == '\\' && *(t+1) == '%')
            {
                t += 2;
                *s++ = '%';
                continue;
            }
            if (*t == '%')
            {
                if (!strnicmp(t+1,"screen",6))
                {
                    t += 7;
                    strcpy(s,pubname);
                    j = strlen(pubname)-1;
                }
                else if (!strnicmp(t+1,"map",3))
                {
                    STRPTR a;

                    t += 4;

                    if (rxpage)
                        a = rxpage->pg_Mappe->mp_Node.ln_Name;
                    else
                        a = "-";

                    strcpy(s,a);
                    j = strlen(a)-1;
                }
                else if (!strnicmp(t+1,"page",4))
                {
                    STRPTR a;

                    t += 5;

                    if (rxpage)
                        a = rxpage->pg_Node.ln_Name;
                    else
                        a = "-";

                    strcpy(s,a);
                    j = strlen(a)-1;
                }
                else if (!strnicmp(t+1,"pos",3))
                {
                    t += 4;

                    if (rxpage && rxpage->pg_Gad.DispPos != PGS_NONE)
                    {
                        STRPTR a = Coord2String(rxpage->pg_Gad.cp.cp_Col,rxpage->pg_Gad.cp.cp_Row);

                        strcpy(s,a);
                        j = strlen(a)-1;
                    }
                }
                else if (!strnicmp(t+1,"col",3))
                {
                    t += 4;

                    if (rxpage && rxpage->pg_Gad.DispPos != PGS_NONE)
                    {
                        char a[16];

                        sprintf(a,"%lu",rxpage->pg_Gad.cp.cp_Col);
                        strcpy(s,a);
                        j = strlen(a)-1;
                    }
                }
                else if (!strnicmp(t+1,"row",3))
                {
                    t += 4;

                    if (rxpage && rxpage->pg_Gad.DispPos != PGS_NONE)
                    {
                        char a[16];

                        sprintf(a,"%lu",rxpage->pg_Gad.cp.cp_Row);
                        strcpy(s,a);
                        j = strlen(a)-1;
                    }
                }
                else
                    j = 0, *s++ = *t++;

                if (j)
                    i += j;  s += j+1;
            }
            else
                *s++ = *t++;
        }
    }
    if (i >= cmdbuflen-3)
        s = EnlargeCommandBuffer();
    *s++ = '\n';  *s = 0;

    return cmdbuffer;
}


int
compareCommands(struct Node **lna,struct Node **lnb)
{
    STRPTR ta = (*lna)->ln_Name,tb = (*lnb)->ln_Name;
    int ca,cb;

    ca = cmdlen(ta);  cb = cmdlen(tb);

    if ((cb - ca) != 0)
        return stricmp(ta,tb);

    return strnicmp(ta,tb,ca);
}


struct Node *
SearchCommandInArray(STRPTR t, struct Node **array, ULONG arraySize)
{
    struct Node searchln, *ln;

    searchln.ln_Name = t;  ln = &searchln;

    ln = bsearch(&ln, array, arraySize, sizeof(struct Node *), (APTR)compareCommands);
    if (ln != NULL)
        return(*(struct Node **)ln);

    return NULL;
}


void
FreeCommandArray(struct Node **array, ULONG arraySize)
{
    freeSort(array, arraySize);
}


struct Node **
BuildCommandArray(struct MinList *list, ULONG *arraySize)
{
    return allocSort(list, arraySize);
}


ULONG
processIntCmd(STRPTR t)
{
    struct IntCmd *ic;
    struct CSource cs;
    struct RDArgs *rda;
    STRPTR cmd;
    ULONG  rc;
    long   opts[MAX_OPTS];

    if (!t)
        return 0;

    if (!(ic = (struct IntCmd *)SearchCommandInArray(t, intCmdArray, intCmdArraySize)))
        return 0;

    if (!rxpage && (ic->ic_Node.ln_Type & ICF_PAGE) != 0)
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_NO_PROJECT_CONTEXT_ERR),t);
        return RC_WARN;
    }
    if (!rxmsg && (ic->ic_Node.ln_Type & ICF_MSG) != 0)
    {
        ErrorRequest(GetString(&gLocaleInfo, MSG_COMMAND_IN_SCRIPT_ONLY_ERR), t);
        return 0;
    }

    rc = RC_WARN;
    SetBusy(TRUE, BT_APPLICATION);

    if ((cmd = BuildCommand(t)) != 0)
    {
        cs.CS_Buffer = cmd; /*t2+cmdlen(ic->ic_Node.ln_Name)+1;*/
        cs.CS_Length = strlen(cs.CS_Buffer);
        cs.CS_CurChr = 0;
        memset((char *)opts, 0, sizeof(opts));

        if ((rda = AllocDosObject(DOS_RDARGS, TAG_END)) != 0)
        {
            rda->RDA_Source = cs;
            rda->RDA_Flags |= RDAF_NOPROMPT;

            if (ReadArgs(ic->ic_Node.ln_Name + cmdlen(ic->ic_Node.ln_Name) + 1, opts, rda))
            {
                rc = ic->ic_Function(opts);
                FreeArgs(rda);
            }
            else
            {
                char buff[128];

                if (!Fault(IoErr(), NULL, buff, 128))
                    strcpy(buff,GetString(&gLocaleInfo, MSG_UNKNOWN_ERR));
				ErrorRequest(GetString(&gLocaleInfo, MSG_CANNOT_EXECUTE_COMMAND_ERR), t, buff);
            }
            FreeDosObject(DOS_RDARGS, rda);
        }
    }
    SetBusy(FALSE, BT_APPLICATION);

    return rc;
}


long
ProcessAppCmd(struct Page *page, STRPTR t)
{
    struct AppCmd *ac;
    struct Command *cmd,*scmd;
    struct List l;
    BPTR   out = (BPTR)NULL;

    if (!t)
        return -1;

    if (!(ac = FindAppCmd(page,t)))
    {
        return (long)processIntCmd(t);
    }

    MyNewList(&l);
    for (cmd = (APTR)ac->ac_Cmds.mlh_Head;cmd->cmd_Succ;cmd = cmd->cmd_Succ)
    {
        if ((scmd = AllocPooled(pool, sizeof(struct Command))) != 0)
        {
            scmd->cmd_Type = cmd->cmd_Type;
            scmd->cmd_Name = AllocString(cmd->cmd_Name);
            MyAddTail(&l, scmd);
        }
    }
    if (ac->ac_Output)
    {
        out = rxout;
        rxout = Open(ac->ac_Output,MODE_NEWFILE);
    }
    for (cmd = (APTR)l.lh_Head;cmd->cmd_Succ;cmd = cmd->cmd_Succ)
    {
        switch (cmd->cmd_Type)
        {
            case CMDT_INTERN:
                processIntCmd(cmd->cmd_Name);
                break;
            case CMDT_AREXX:
                RunRexxScript(RXS_EXTERN,cmd->cmd_Name);
                break;
            case CMDT_DOS:
#ifdef __amigaos4__
				SystemTags((CONST_STRPTR)cmd->cmd_Name, SYS_Input, NULL, SYS_Output, rxout, TAG_DONE);
#else
                Execute((CONST_STRPTR)cmd->cmd_Name,(BPTR)NULL,rxout);
#endif
                break;
        }
    }
    if (out)
    {
        Close(rxout);
        rxout = out;
    }
    while ((cmd = (struct Command *)MyRemHead(&l)) != 0)
    {
        FreeString(cmd->cmd_Name);
        FreePooled(pool,cmd,sizeof(struct Command));
    }

    return 0;
}



