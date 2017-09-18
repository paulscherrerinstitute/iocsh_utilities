#include <string.h>
#include <stdlib.h>
#include "dbStaticLib.h"
#include "epicsString.h"
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
int epicsStrGlobMatch(const char *str, const char *pattern);
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
    size_t l = 0;
    char *d = NULL;
    char *alt_match = NULL;

    if (match)
    {
        l = strlen(match);
        d = strchr(match, '.');
        
        if (!d)
        {
            alt_match = malloc(l+3);
            sprintf(alt_match, "%s.*", match);
        }
        else if (strcmp(d, ".VAL") == 0 || strcmp(d, ".*") == 0)
        {
            alt_match = malloc(d-match+1);
            sprintf(alt_match, "%.*s", (int)(d-match), match);
        }
    }

    dbInitEntry(pdbbase, &dbEntry);
    for (status = dbFirstRecordType(&dbEntry); !status; status = dbNextRecordType(&dbEntry))
    for (status = dbFirstRecord(&dbEntry); !status; status = dbNextRecord(&dbEntry))
    {
#if EPICS_VERSION*10000+EPICS_REVISION*100+EPICS_MODIFICATION >= 31411
        if (dbIsAlias(&dbEntry)) continue;
#endif
        for (status = dbFirstField(&dbEntry, 0); !status; status = dbNextField(&dbEntry, 0))
        {
            if (dbGetLinkType(&dbEntry) == DCT_LINK_PV)
            {
                const char* target = ((DBLINK *)dbEntry.pfield)->value.pv_link.pvname;
                if (!match || (
                        (strchr(target, '.') ? 1 : 0) == (d ? 1 : 0) ?
                            epicsStrGlobMatch(target, match) : 
                            alt_match ? epicsStrGlobMatch(target, alt_match) : 0))
                {
                    printf("%s.%s ==> %s\n", dbGetRecordName(&dbEntry), dbGetFieldName(&dbEntry),
                        dbGetString(&dbEntry));
                }
            }
        }
    }
    dbFinishEntry(&dbEntry);   
    free(alt_match);
    return 0;
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
