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
    char *matchfield = NULL;
    char *alt_match = NULL;
    const char* symbol;
    const char* target;

    if (match)
    {
        matchfield = strchr(match, '.');
        if (!matchfield)
        {
            alt_match = malloc(strlen(match)+3);
            sprintf(alt_match, "%s.*", match);
        }
        else if (strcmp(matchfield, ".VAL") == 0 || strcmp(matchfield, ".*") == 0)
        {
            alt_match = malloc(matchfield-match+1);
            sprintf(alt_match, "%.*s", (int)(matchfield-match), match);
        }
    }

    dbInitEntry(pdbbase, &dbEntry);
    for (status = dbFirstRecordType(&dbEntry); !status; status = dbNextRecordType(&dbEntry))
    for (status = dbFirstRecord(&dbEntry); !status; status = dbNextRecord(&dbEntry))
    {
        #ifdef DBRN_FLAGS_ISALIAS
        if (dbIsAlias(&dbEntry)) continue;
        #endif
        for (status = dbFirstField(&dbEntry, 0); !status; status = dbNextField(&dbEntry, 0))
        {
            switch (dbEntry.pflddes->field_type)
            {
                default:
                    continue;
                case DBF_INLINK:
                    symbol = "<--";
                    break;
                case DBF_OUTLINK:
                    symbol = "-->";
                    break;
                case DBF_FWDLINK:
                    symbol = "...";
                    break;
            }
            switch (((DBLINK *)dbEntry.pfield)->type)
            {
                default:
                    continue;
                #ifdef PN_LINK
                case PN_LINK:
                #endif
                case PV_LINK:
                case DB_LINK:
                case CA_LINK:
                    break;
            }
            target = ((DBLINK *)dbEntry.pfield)->value.pv_link.pvname;
            if (!target) continue;
            if (!match || (!strchr(target, '.') == !matchfield ? epicsStrGlobMatch(target, match)
                : alt_match ? epicsStrGlobMatch(target, alt_match) : 0))
            {
                printf("%s.%s %s %s\n", dbGetRecordName(&dbEntry), dbGetFieldName(&dbEntry),
                    symbol, dbGetString(&dbEntry));
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
