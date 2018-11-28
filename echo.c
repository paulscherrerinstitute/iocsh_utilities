#include <stdio.h>
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
#endif

#include "epicsString.h"

int echo(char* str)
{
    if (str)
    {
        dbTranslateEscape(str, str);
        fputs(str, stdout);
    }
    putchar('\n');
    return 0;
}

#ifndef EPICS_3_13
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"

static const iocshArg echoArg0 = { "string", iocshArgString };
static const iocshArg * const echoArgs[1] = { &echoArg0 };
static const iocshFuncDef echoDef = { "echo", 1, echoArgs };

static void echoFunc(const iocshArgBuf *args)
{
    echo (args[0].sval);
}

static void echoRegister(void)
{
    iocshRegister (&echoDef, echoFunc);
}

epicsExportRegistrar(echoRegister);
#endif
