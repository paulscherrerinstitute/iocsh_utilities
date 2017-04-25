#include <string.h>
#include <stdlib.h>
#include "epicsStdio.h"
#include "epicsStdioRedirect.h"
#include "dbStaticLib.h"
#include "epicsString.h"
#include "iocsh.h"
#include "epicsExport.h"

/* it is just crazy how much we would have to include to get this definition */
extern DBBASE *pdbbase;

long forEachInfo(const char* infopatterns, long func(DBENTRY *pdbentry, void* usr), void* usr)
{
    DBENTRY dbentry;
    char *infocopy = NULL;
    char **matchlist = NULL;
    long status = 0;

    if (infopatterns) {
        char *p;
        int n, i;

        infocopy = epicsStrDup(infopatterns);
        n = 1;
        p = infocopy;
        while (*p && (p = strchr(p,' '))) {
            n++;
            while (*p == ' ') p++;
        }
        matchlist = dbCalloc(n+1,sizeof(char *));
        p = infocopy;
        for (i = 0; i < n; i++) {
            matchlist[i] = p;
            if (i < n - 1) {
                p = strchr(p, ' ');
                *p++ = 0;
                while (*p == ' ') p++;
            }
        }
        matchlist[n] = NULL;
    }
    
    dbInitEntry(pdbbase, &dbentry);
    for (status = dbFirstRecordType(&dbentry); !status; status = dbNextRecordType(&dbentry))
    for (status = dbFirstRecord(&dbentry); !status; status = dbNextRecord(&dbentry))
    for (status = dbFirstInfo(&dbentry); !status; status = dbNextInfo(&dbentry))
    {
        if (matchlist)
        {
            char **match;
            const char *name = dbGetInfoName(&dbentry);

            for (match = matchlist; *match; match++)
            {
                if (epicsStrGlobMatch(name, *match))
                {
                    status = func(&dbentry, usr);
                    break; /* match each entry only once */
                }
            }
        }
        else
            status = func(&dbentry, usr);
        if (status) goto end;
    }
    status = 0;
end:
    dbFinishEntry(&dbentry);
    free(matchlist);
    free(infocopy);
    return status;
}

static long dbPrintInfo(DBENTRY *pdbentry, void* usr)
{
    void* p;
    
    printf("%s %s \"%s\"", dbGetRecordName(pdbentry), dbGetInfoName(pdbentry), dbGetInfoString(pdbentry));
    if ((p = dbGetInfoPointer(pdbentry)) != NULL)
        printf(" %p", p);
    printf("\n");
    return 0;
}

long dbli(const char* infopatterns)
{
    return forEachInfo(infopatterns, dbPrintInfo, NULL);
}

static const iocshFuncDef dbliDef =
    { "dbli", 1, (const iocshArg *[]) {
    &(iocshArg) { "info pattern ...", iocshArgString },
}};

void dbliFunc(const iocshArgBuf *args)
{
    dbli(args[0].sval);
}

static void dbliRegistrar(void)
{
    iocshRegister(&dbliDef, dbliFunc);
}

epicsExportRegistrar(dbliRegistrar);
