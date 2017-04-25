#include <string.h>
#include <stdlib.h>
#include "dbStaticLib.h"
#include "epicsString.h"
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13

static int epicsStrGlobMatch(
    const char *str, const char *pattern)
{
    const char *cp=NULL, *mp=NULL;
    
    while ((*str) && (*pattern != '*')) {
        if ((*pattern != *str) && (*pattern != '?'))
            return 0;
        pattern++;
        str++;
    }
    while (*str) {
        if (*pattern == '*') {
            if (!*++pattern)
                return 1;
            mp = pattern;
            cp = str+1;
        }
        else if ((*pattern == *str) || (*pattern == '?')) {
            pattern++;
            str++;
        }
        else {
            pattern = mp;
            str = cp++;
        }
    }
    while (*pattern == '*')
        pattern++;
    return !*pattern;
}

#else
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"
#endif

/* it is just crazy how much we would have to include to get this definition */
extern DBBASE *pdbbase;

long dbll(const char* match)
{
    DBENTRY dbEntry;
    long status = 0;
    char *matchanyfield = NULL;

    if (match && strchr(match, '.') == 0)
    {
        matchanyfield = malloc(strlen(match)+3);
        sprintf(matchanyfield, "%s.*", match);
    }

    dbInitEntry(pdbbase, &dbEntry);
    for (status = dbFirstRecordType(&dbEntry); !status; status = dbNextRecordType(&dbEntry))
    for (status = dbFirstRecord(&dbEntry); !status; status = dbNextRecord(&dbEntry))
    for (status = dbFirstField(&dbEntry, 0); !status; status = dbNextField(&dbEntry, 0))
    {
        if (dbGetLinkType(&dbEntry) == DCT_LINK_PV)
        {
            const char* target = ((DBLINK *)dbEntry.pfield)->value.pv_link.pvname;
            if (!match || epicsStrGlobMatch(target, match) ||
                (matchanyfield && epicsStrGlobMatch(target, matchanyfield)))
            {
                printf("%s.%s --> %s\n", dbGetRecordName(&dbEntry), dbGetFieldName(&dbEntry),
                    dbGetString(&dbEntry));
            }
        }
    }
    dbFinishEntry(&dbEntry);   
    free(matchanyfield);
    return status;
}

#ifndef EPICS_3_13
static const iocshFuncDef dbllDef =
    { "dbll", 1, (const iocshArg *[]) {
    &(iocshArg) { "record name pattern", iocshArgString },
}};

void dbllFunc(const iocshArgBuf *args)
{
    dbll(args[0].sval);
}

static void dbllRegistrar(void)
{
    iocshRegister(&dbllDef, dbllFunc);
}

epicsExportRegistrar(dbllRegistrar);
#endif
