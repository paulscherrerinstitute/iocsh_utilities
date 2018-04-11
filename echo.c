#include "iocsh.h"
#include "epicsExport.h"
#include "epicsStdioRedirect.h"

static const iocshArg echoArg0 = { "arguments", iocshArgArgv };
static const iocshArg * const echoArgs[1] = { &echoArg0 };
static const iocshFuncDef echoDef = { "echo", 2, echoArgs };

static void echoFunc(const iocshArgBuf *args)
{
    int i;
    for (i = 1; i < args[0].aval.ac; i++)
    {
        if (i>1) putchar(' ');
        fputs(args[0].aval.av[i], stdout);
    }
    putchar('\n');
}

static void echoRegister(void)
{
    iocshRegister (&echoDef, echoFunc);
}

epicsExportRegistrar(echoRegister);
