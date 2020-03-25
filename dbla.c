/* dbla.c
*
*  overwrite the dbla command from EPICS base to allow reverse lookup
*
* Copyright (C) 2017 Dirk Zimoch
*
* The original dbla iocsh function is published under the EPICS Open License
* which can be found here: https://epics.anl.gov/license/open.php
* The original contained the following copyright notice:
*/
/*************************************************************************\
* Copyright (c) 2009 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include "dbStaticLib.h"
#include "epicsString.h"
#include "epicsStdioRedirect.h"
#include "iocsh.h"
#include "epicsVersion.h"
#include "dbAccess.h"
#include "epicsExport.h"

#ifndef vxWorks
#define dbla __dbla
#endif

long epicsShareAPI dbla(const char* match)
{
#if EPICS_VERSION*10000+EPICS_REVISION*100+EPICS_MODIFICATION >= 31411
    DBENTRY dbEntry;
    long status;
    const char* alias;
    const char* realname;

    dbInitEntry(pdbbase, &dbEntry);
    for (status = dbFirstRecordType(&dbEntry); !status; status = dbNextRecordType(&dbEntry))
    for (status = dbFirstRecord(&dbEntry); !status; status = dbNextRecord(&dbEntry))
    {
        if (!dbIsAlias(&dbEntry)) continue;

        dbFindField(&dbEntry, "NAME");
        realname = dbGetString(&dbEntry);
        alias = dbGetRecordName(&dbEntry);

        if (!match || epicsStrGlobMatch(realname, match) || epicsStrGlobMatch(alias, match))
        {
            printf("%s -> %s\n", alias, realname);
        }
    }
    dbFinishEntry(&dbEntry);
#endif
    return 0;
}

static const iocshFuncDef dblaDef =
    { "dbla", 1, (const iocshArg *[]) {
    &(iocshArg) { "record_name_pattern", iocshArgString },
}};

void dblaFunc(const iocshArgBuf *args)
{
    dbla(args[0].sval);
}

static void dblaRegistrar(void)
{
    iocshRegister(&dblaDef, dblaFunc);
}

epicsExportRegistrar(dblaRegistrar);
