#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbScan.h>
#include <dbAccess.h>
#include <initHooks.h>
#include <epicsVersion.h>

#ifdef BASE_VERSION
#define EPICS_3_13
#else
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>
#endif

#ifdef INCinitHooksh
    /* old: without iocPause etc */
    #define initHookAfterIocRunning initHookAfterInterruptAccept
#endif

struct cmditem
{
    struct cmditem* next;
    int when;
    int useIocshCmd;
    union {
        const char* a[12];
        char cmd[256];
    } x;
} *cmdlist, **cmdlast=&cmdlist;

static void afterInitHook(initHookState state)
{
    struct cmditem *item;

    for (item = cmdlist; item != NULL; item = item->next)
    {
        if (item->when != state) continue;
#ifndef EPICS_3_13
        if (item->useIocshCmd)
        {
            printf("%s\n", item->x.cmd);
            iocshCmd(item->x.cmd);
        }
        else
#endif
        ((void (*)())item->x.a[0])(item->x.a[1], item->x.a[2], item->x.a[3], item->x.a[4], item->x.a[5],
            item->x.a[6], item->x.a[7], item->x.a[8], item->x.a[9], item->x.a[10], item->x.a[11]);
    }
}

static struct cmditem *newItem(const char* cmd, int useIocshCmd, int when)
{
    static int first_time = 1;
    struct cmditem *item;
    if (interruptAccept)
    {
        fprintf(stderr, "This function can only be used before iocInit\n");
        return NULL;
    }
    if (first_time)
    {
        first_time = 0;
        initHookRegister(afterInitHook);
    }
    item = malloc(sizeof(struct cmditem));
    if (item == NULL)
    {
        perror("afterInit");
        return NULL;
    }
    item->useIocshCmd = useIocshCmd;
    item->when = when;
    if (when >= initHookAtIocPause && when < initHookAfterInterruptAccept)
    {
        /* Reverse order for "shutdown hooks" */
        if (!cmdlist) cmdlast = &item->next;
        item->next = cmdlist;
        cmdlist = item;
    }
    else
    {
        item->next = NULL;
        *cmdlast = item;
        cmdlast = &item->next;
    }
    return item;
}

static int hookNameToNumber(const char* hook)
{
    static const struct { char* name; int hook; } hookMap[] = {
        { "IocBuild",               initHookAtIocBuild },
        { "Beginning",              initHookAtBeginning },
        { "AfterCallbackInit",      initHookAfterCallbackInit },
        { "AfterCaLinkInit",        initHookAfterCaLinkInit },
        { "AfterInitDrvSup",        initHookAfterInitDrvSup },
        { "AfterInitRecSup",        initHookAfterInitRecSup },
        { "AfterInitDevSup",        initHookAfterInitDevSup },
        { "AfterInitDatabase",      initHookAfterInitDatabase },
        { "AfterFinishDevSup",      initHookAfterFinishDevSup },
        { "AfterScanInit",          initHookAfterScanInit },
        { "AfterInitialProcess",    initHookAfterInitialProcess },
        { "AfterCaServerInit",      initHookAfterCaServerInit },
        { "AfterIocBuilt",          initHookAfterIocBuilt },
        { "IocRun",                 initHookAtIocRun },
        { "AfterDatabaseRunning",   initHookAfterDatabaseRunning },
        { "AfterCaServerRunning",   initHookAfterCaServerRunning },
        { "AfterIocRunning",        initHookAfterIocRunning },
        { "IocPause",               initHookAtIocPause },
        { "AfterCaServerPaused",    initHookAfterCaServerPaused },
        { "AfterDatabasePaused",    initHookAfterDatabasePaused },
        { "AfterIocPaused",         initHookAfterIocPaused },
#if defined(EPICS_VERSION_INT)
#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 3, 1)
        { "Shutdown",               initHookAtShutdown },
        { "AfterCloseLinks",        initHookAfterCloseLinks },
        { "AfterStopScan",          initHookAfterStopScan },
        { "AfterStopCallback",      initHookAfterStopCallback },
        { "AfterStopLinks",         initHookAfterStopLinks },
        { "BeforeFree",             initHookBeforeFree },
        { "AfterShutdown",          initHookAfterShutdown },
#endif
#endif
        { "InterruptAccept",    initHookAfterInterruptAccept },
        { "End",                initHookAtEnd },
        { NULL,                 -1  }
    };

    int i;
    if (!hook)
    {
        fprintf(stderr, "usage: atInitStage hook, command, args...\n");
        return -1;
    }

    if (epicsStrnCaseCmp("initHook", hook, 8) == 0) hook+=8;
    if (epicsStrnCaseCmp("At", hook, 2) == 0) hook+=2;
    for (i = 0; hookMap[i].name; i++) {
        if (epicsStrCaseCmp(hook, hookMap[i].name) == 0
            || (hookMap[i].name[0] == 'A' && epicsStrCaseCmp(hook, hookMap[i].name+5) == 0))
        {
            return hookMap[i].hook;
        }
    }
    fprintf(stderr, "atInitStage error: Unknown hook name '%s'\n", hook);
    return -1;
}

#ifdef vxWorks
int atInitStageVx(int when, const char* cmd, const char* a1, const char* a2, const char* a3, const char* a4, const char* a5, const char* a6, const char* a7, const char* a8, const char* a9, const char* a10, const char* a11)
{
    struct cmditem *item = newItem(cmd, 0, when);
    if (!item) return -1;

    item->x.a[0] = cmd;
    item->x.a[1] = a1;
    item->x.a[2] = a2;
    item->x.a[3] = a3;
    item->x.a[4] = a4;
    item->x.a[5] = a5;
    item->x.a[6] = a6;
    item->x.a[7] = a7;
    item->x.a[8] = a8;
    item->x.a[9] = a9;
    item->x.a[10] = a10;
    item->x.a[11] = a11;

    return 0;
}

int atInitStage(const char* hook, const char* cmd, const char* a1, const char* a2, const char* a3, const char* a4, const char* a5, const char* a6, const char* a7, const char* a8, const char* a9, const char* a10)
{
    int when = hookNameToNumber(hook);
    if (when < 0) return -1;
    if (!cmd)
    {
        fprintf(stderr, "usage: atInitStage hook, command, args...\n");
        return -1;
    }
    return atInitStageVx(when, cmd, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, NULL);
}

int afterInit(const char* cmd, const char* a1, const char* a2, const char* a3, const char* a4, const char* a5, const char* a6, const char* a7, const char* a8, const char* a9, const char* a10, const char* a11)
{
    if (!cmd)
    {
        fprintf(stderr, "usage: afterInit command, args...\n");
        return -1;
    }
    return atInitStageVx(initHookAfterIocRunning, cmd, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

int atInit(const char* cmd, const char* a1, const char* a2, const char* a3, const char* a4, const char* a5, const char* a6, const char* a7, const char* a8, const char* a9, const char* a10, const char* a11)
{
    if (!cmd)
    {
        fprintf(stderr, "usage: atInit command, args...\n");
        return -1;
    }
    return atInitStageVx(initHookAtBeginning, cmd, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}
#endif

#ifndef EPICS_3_13
static void atInitStageIocsh(int when, int wordcount, char* cmdword[])
{
    int i, n;
    struct cmditem *item = newItem(cmdword[1], 1, when);
    if (!item) return;

    n = sprintf(item->x.cmd, "%.*s", (int)sizeof(item->x.cmd)-1, cmdword[1]);
    for (i = 2; i < wordcount; i++)
    {
        if (strpbrk(cmdword[i], " ,\"\\"))
            n += sprintf(item->x.cmd+n, " '%.*s'",(int)sizeof(item->x.cmd)-4-n, cmdword[i]);
        else
            n += sprintf(item->x.cmd+n, " %.*s", (int)sizeof(item->x.cmd)-2-n, cmdword[i]);
    }
}

static const iocshFuncDef afterInitDef = {
    "afterInit", 1, (const iocshArg *[]) {
        &(iocshArg) { "commandline", iocshArgArgv },
}};

static void afterInitFunc(const iocshArgBuf *args)
{
    if (!args[0].aval.av[0])
    {
        fprintf(stderr, "usage: afterInit command, args...\n");
        return;
    }
    atInitStageIocsh(initHookAfterIocRunning, args[0].aval.ac, args[0].aval.av);
}

static const iocshFuncDef atInitDef = {
    "atInit", 1, (const iocshArg *[]) {
        &(iocshArg) { "commandline", iocshArgArgv },
}};

static void atInitFunc(const iocshArgBuf *args)
{
    if (!args[0].aval.av[0])
    {
        fprintf(stderr, "usage: atInit command, args...\n");
        return;
    }
    atInitStageIocsh(initHookAtBeginning, args[0].aval.ac, args[0].aval.av);
}

static const iocshFuncDef atInitStageDef = {
    "atInitStage", 2, (const iocshArg *[]) {
        &(iocshArg) { "hook", iocshArgString },
        &(iocshArg) { "commandline", iocshArgArgv },
}};

static void atInitStageFunc(const iocshArgBuf *args)
{
    int when = hookNameToNumber(args[0].sval);
    if (when < 0) return;
    if (!args[1].aval.av[0])
    {
        fprintf(stderr, "usage: atInitStage hook, command, args...\n");
        return;
    }
    atInitStageIocsh(when, args[1].aval.ac, args[1].aval.av);
}

static void afterInitRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister (&afterInitDef, afterInitFunc);
        iocshRegister (&atInitDef, atInitFunc);
        iocshRegister (&atInitStageDef, atInitStageFunc);
    }
}
epicsExportRegistrar(afterInitRegister);
#endif
