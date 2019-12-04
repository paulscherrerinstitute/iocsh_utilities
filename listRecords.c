/* listRecords.c
*
*  listRecords is a wrapper function for dbl 
*  it hides the changed syntax of dbl between R3.13 and R3.14.
*
* Copyright (C) 2007 Dirk Zimoch
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

#include <stddef.h>
#include <errno.h>
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
#include <stdio.h>
long dbl(char *precordTypename, char *filename, char *fields);
#else
#define EPICS_3_14
#include <string.h>
#include "dbTest.h"
#include "epicsStdioRedirect.h"
#include "iocsh.h"
#include "epicsExport.h"
#endif

int listRecords(char* filename, char* fields)
{
#ifdef EPICS_3_13
    return dbl(NULL, filename, fields);
#else
    {
        FILE* oldStdout = NULL;
        FILE* newStdout = NULL;

        if (filename && *filename)
        {
            newStdout = fopen(filename, "w");
            if (!newStdout)
            {
                fprintf(stderr, "Can't open %s for writing: %s\n",
                    filename, strerror(errno));
                return errno;
            }
            oldStdout = epicsGetThreadStdout();
            epicsSetThreadStdout(newStdout);
        }
        dbl(0L, fields);
        if (newStdout)
        {
            fclose(newStdout);
            epicsSetThreadStdout(oldStdout);
        }
        return 0;
    }
#endif
}

#ifdef EPICS_3_14
static const iocshArg listRecordsArg0 = { "filename", iocshArgString };
static const iocshArg listRecordsArg1 = { "fields", iocshArgString };
static const iocshArg * const listRecordsArgs[2] = { &listRecordsArg0, &listRecordsArg1 };
static const iocshFuncDef listRecordsDef = { "listRecords", 2, listRecordsArgs };
static void listRecordsFunc (const iocshArgBuf *args)
{
    listRecords(args[0].sval, args[1].sval);
}
static void listRecordsRegister(void)
{
    iocshRegister (&listRecordsDef, listRecordsFunc);
}
epicsExportRegistrar(listRecordsRegister);
#endif


