/*
* epicsEnvUnset - delete environment variable
*
* DISCLAIMER: Use at your own risc and so on. No warranty, no refund.
*/

#include "iocsh.h"
#include "epicsExport.h"
#include "epicsStdioRedirect.h"

#ifdef vxWorks
/* Do the next best thing: invalidate the variable name */
#include <envLib.h>
#include <string.h>
epicsShareFunc void epicsShareAPI epicsEnvUnset (const char *name)
{
    char* var;

    if (!name) return;
    var = getenv(name);
    if (!var) return;
    var -= strlen(name);
    var --;
    *var = 0;
}
#else
#include <stdlib.h>
epicsShareFunc void epicsShareAPI epicsEnvUnset (const char *name)
{
    if (!name) return;
    unsetenv(name);
}
#endif

static const iocshArg epicsEnvUnsetArg0 = { "name", iocshArgString};
static const iocshArg * const epicsEnvUnsetArgs[1] = { &epicsEnvUnsetArg0 };
static const iocshFuncDef epicsEnvUnsetDef = { "epicsEnvUnset", 1, epicsEnvUnsetArgs };

static void epicsEnvUnsetFunc(const iocshArgBuf *args)
{
    epicsEnvUnset(args[0].sval);
}

static void epicsEnvUnsetRegister(void)
{
    iocshRegister (&epicsEnvUnsetDef, epicsEnvUnsetFunc);
}

epicsExportRegistrar(epicsEnvUnsetRegister);
