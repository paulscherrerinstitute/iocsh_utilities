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
    int type;
    int when;
    union {
        const char* a[12];
        char cmd[256];
    } x;
} *cmdlist, **cmdlast=&cmdlist;

void afterInitHook(initHookState state)
{
    struct cmditem *item;

    for (item = cmdlist; item != NULL; item = item->next)
    {
        if (item->when != state) continue;
#ifndef EPICS_3_13
        if (item->type == 1)
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

static int first_time = 1;

static struct cmditem *newItem(const char* cmd, int type, int when)
{
    struct cmditem *item;
    if (!cmd)
    {
        fprintf(stderr, "usage: afterInit command, args...\n");
        return NULL;
    }
    if (interruptAccept)
    {
        fprintf(stderr, "afterInit can only be used before iocInit\n");
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
    item->type = type;
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

#ifdef vxWorks
int afterInit(const char* cmd, const char* a1, const char* a2, const char* a3, const char* a4, const char* a5, const char* a6, const char* a7, const char* a8, const char* a9, const char* a10, const char* a11)
{
    struct cmditem *item = newItem(cmd, 0, initHookAfterIocRunning);
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

int atInit(const char* cmd, const char* a1, const char* a2, const char* a3, const char* a4, const char* a5, const char* a6, const char* a7, const char* a8, const char* a9, const char* a10, const char* a11)
{
    struct cmditem *item = newItem(cmd, 0, initHookAtBeginning);
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
#endif

#ifndef EPICS_3_13
static void __atInitStage(int when, int wordcount, char* cmdword[])
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
    __atInitStage(initHookAfterIocRunning, args[0].aval.ac, args[0].aval.av);
}

static const iocshFuncDef atInitDef = {
    "atInit", 1, (const iocshArg *[]) {
        &(iocshArg) { "commandline", iocshArgArgv },
}};

static void atInitFunc(const iocshArgBuf *args)
{
    __atInitStage(initHookAtBeginning, args[0].aval.ac, args[0].aval.av);
}

static const iocshFuncDef atInitStageDef = {
    "atInitStage", 2, (const iocshArg *[]) {
        &(iocshArg) { "hook", iocshArgString },
        &(iocshArg) { "commandline", iocshArgArgv },
}};

static void atInitStageFunc(const iocshArgBuf *args)
{
    static const struct { char* name; int hook; } hookMap[] = {
        { "IocBuild",           initHookAtIocBuild },
        { "Beginning",          initHookAtBeginning },
        { "CallbackInit",       initHookAfterCallbackInit },
        { "CaLinkInit",         initHookAfterCaLinkInit },
        { "InitDrvSup",         initHookAfterInitDrvSup },
        { "InitRecSup",         initHookAfterInitRecSup },
        { "InitDevSup",         initHookAfterInitDevSup },
        { "InitDatabase",       initHookAfterInitDatabase },
        { "FinishDevSup",       initHookAfterFinishDevSup },
        { "ScanInit",           initHookAfterScanInit },
        { "InitialProcess",     initHookAfterInitialProcess },
        { "CaServerInit",       initHookAfterCaServerInit },
        { "IocBuilt",           initHookAfterIocBuilt },
        { "IocRun",             initHookAtIocRun },
        { "DatabaseRunning",    initHookAfterDatabaseRunning },
        { "CaServerRunning",    initHookAfterCaServerRunning },
        { "IocRunning",         initHookAfterIocRunning },
        { "IocPause",           initHookAtIocPause },
        { "CaServerPaused",     initHookAfterCaServerPaused },
        { "DatabasePaused",     initHookAfterDatabasePaused },
        { "IocPaused",          initHookAfterIocPaused },
#if defined(EPICS_VERSION_INT)
#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 3, 1)
        { "Shutdown",           initHookAtShutdown },
        { "CloseLinks",         initHookAfterCloseLinks },
        { "StopScan",           initHookAfterStopScan },
        { "StopCallback",       initHookAfterStopCallback },
        { "StopLinks",          initHookAfterStopLinks },
        { "BeforeFree",         initHookBeforeFree },
        { "Shutdown",           initHookAfterShutdown },
#endif
#endif
        { "InterruptAccept",    initHookAfterInterruptAccept },
        { "End",                initHookAtEnd },
        { NULL,                 -1  }
    };

    int i;
    const char* hook = args[0].sval;
    if (!hook) {
        fprintf(stderr, "usage: atInitStage hook, command, args...\n");
        return;
    }

    if (epicsStrnCaseCmp("initHook", hook, 8) == 0) hook+=8;
    if (epicsStrnCaseCmp("At", hook, 2) == 0) hook+=2;
    if (epicsStrnCaseCmp("After", hook, 5) == 0) hook+=5;
    for (i = 0; hookMap[i].name; i++) {
        if (epicsStrCaseCmp(hook, hookMap[i].name) == 0) {
            __atInitStage(hookMap[i].hook, args[1].aval.ac, args[1].aval.av);
            return;
        }
    }
    fprintf(stderr, "atInitStage error: Unknown hook name '%s'\n", hook);
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
