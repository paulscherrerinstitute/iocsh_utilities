/* echo.c
*
*  echo a string on the ioc shell
*
* Copyright (C) 2018 Dirk Zimoch
*
* This is a back port for older EPICS base versions that do not provide this function.
* The original echo iocsh function is published under the EPICS Open License
* which can be found here: https://epics.anl.gov/license/open.php
* The original contained the following copyright notice:
*/
/*************************************************************************\
* Copyright (c) 2007 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The University of Saskatchewan
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
#else
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"
#endif

#include "epicsString.h"

#ifndef vxWorks
static
#endif
int echo(const char* str)
{
    if (str)
    {
        char* s = malloc(strlen(str)+1);
        dbTranslateEscape(s, str);
        fputs(s, stdout);
        free(s);
    }
    putchar('\n');
    return 0;
}

#ifndef EPICS_3_13
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
