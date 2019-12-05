/* epicsEnvUnset.c
*
*  unset (delete) an environment variable
*
* Copyright (C) 2018 Dirk Zimoch
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
#else
#include "iocsh.h"
#include "epicsExport.h"
#include "epicsStdioRedirect.h"
#endif

#ifdef vxWorks
/* Do the next best thing: invalidate the variable name */
#include <envLib.h>
#include <string.h>
void epicsEnvUnset (const char *name)
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
void epicsEnvUnset (const char *name)
{
    if (!name) return;
#ifdef _WIN32
    /* simulate unset by setting a empty value */
    char *cp = malloc(strlen(name) + 2);
    strcpy(cp, name);
    strcat(cp, "=");
    if (putenv(cp) < 0)
        fprintf(stderr, "Failed to unset environment parameter \"%s\"", name);
    free(cp);
#else
    unsetenv(name);
#endif
}
#endif

#ifndef EPICS_3_13
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
#endif
