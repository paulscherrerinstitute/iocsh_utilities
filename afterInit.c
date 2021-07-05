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
#endif

struct cmditem
{
    struct cmditem* next;
    int type;
    union {
        char* a[12];
        char cmd[256];
    } x;
} *cmdlist, **cmdlast=&cmdlist;

void afterInitHook(initHookState state)
{
    struct cmditem *item;

    if (state !=
#ifdef INCinitHooksh
        /* old: without iocPause etc */
        initHookAfterInterruptAccept
#else
        /* new: with iocPause etc */
        initHookAfterIocRunning
#endif
        ) return;
    for (item = cmdlist; item != NULL; item = item->next)
    {
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

static struct cmditem *newItem(char* cmd, int type)
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
    item->next = NULL;
    *cmdlast = item;
    cmdlast = &item->next;
    return item;
}

#ifdef vxWorks
int afterInit(char* cmd, char* a1, char* a2, char* a3, char* a4, char* a5, char* a6, char* a7, char* a8, char* a9, char* a10, char* a11)
{
    struct cmditem *item = newItem(cmd, 0);
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
static const iocshFuncDef afterInitDef = {
    "afterInit", 1, (const iocshArg *[]) {
        &(iocshArg) { "commandline", iocshArgArgv },
}};

static void afterInitFunc(const iocshArgBuf *args)
{
    int i, n;
    struct cmditem *item = newItem(args[0].aval.av[1], 1);
    if (!item) return;

    n = sprintf(item->x.cmd, "%.255s", args[0].aval.av[1]);
    for (i = 2; i < args[0].aval.ac; i++)
    {
        if (strpbrk(args[0].aval.av[i], " ,\"\\"))
            n += sprintf(item->x.cmd+n, " '%.*s'", 255-3-n, args[0].aval.av[i]);
        else
            n += sprintf(item->x.cmd+n, " %.*s", 255-1-n, args[0].aval.av[i]);
    }
}

static void afterInitRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister (&afterInitDef, afterInitFunc);
    }
}
epicsExportRegistrar(afterInitRegister);
#endif
