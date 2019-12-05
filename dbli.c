/* dbli.c
*
*  list info fields
*
* Copyright (C) 2017 Dirk Zimoch
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

#include <string.h>
#include <stdlib.h>
#include "epicsStdio.h"
#include "epicsStdioRedirect.h"
#include "dbStaticLib.h"
#include "dbAccess.h"
#include "epicsString.h"
#include "iocsh.h"
#include "epicsExport.h"

/* it is just crazy how much we would have to include to get this definition */

long dbNextMatchingInfo(DBENTRY *pdbentry, const char* patternlist[])
{
    long status = 0;
    const char** pattern;

    if (!pdbentry->precordType)
    {
        status = dbFirstRecordType(pdbentry);
        if (status) return status;
    }
    while(1) {
        status = dbNextInfo(pdbentry);
        while (status) {
            status = dbNextRecord(pdbentry);
            while (status) {
                status = dbNextRecordType(pdbentry);
                if (status) return status;
                status = dbFirstRecord(pdbentry);
            }
            status = dbFirstInfo(pdbentry);
        }
        if (!patternlist || !*patternlist) return 0;
        for (pattern = patternlist; *pattern; pattern++)
            if (epicsStrGlobMatch(dbGetInfoName(pdbentry), *pattern)) return 0;
    }
}

static void dblilist(const char** patternlist)
{
    DBENTRY dbentry;
    void* p;

    dbInitEntry(pdbbase, &dbentry);
    while (dbNextMatchingInfo(&dbentry, patternlist) == 0)
    {
        printf("%s.%s \"%s\"", dbGetRecordName(&dbentry), dbGetInfoName(&dbentry), dbGetInfoString(&dbentry));
        if ((p = dbGetInfoPointer(&dbentry)) != NULL) printf(" %p", p);
        printf("\n");
    }
}

/* for vxWorks shell: up to 10 args */
void dbli(const char* p0, const char* p1, const char* p2, const char* p3, const char* p4, const char* p5, const char* p6, const char* p7, const char* p8, const char* p9)
{
    const char* patternlist[11];
    patternlist[0] = p0;
    patternlist[1] = p1;
    patternlist[2] = p2;
    patternlist[3] = p3;
    patternlist[4] = p4;
    patternlist[5] = p5;
    patternlist[6] = p6;
    patternlist[7] = p7;
    patternlist[8] = p8;
    patternlist[9] = p9;
    patternlist[10] = NULL;
    dblilist(patternlist);
}

static const iocshFuncDef dbliDef =
    { "dbli", 1, (const iocshArg *[]) {
    &(iocshArg) { "pattern...", iocshArgArgv },
}};

void dbliFunc(const iocshArgBuf *args)
{
    dblilist((const char**)args[0].aval.av+1);
}

static void dbliRegistrar(void)
{
    iocshRegister(&dbliDef, dbliFunc);
}

epicsExportRegistrar(dbliRegistrar);
