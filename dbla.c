#include "dbStaticLib.h"
#include "epicsString.h"
#include "epicsStdioRedirect.h"
#include "iocsh.h"
#include "epicsExport.h"

/* it is just crazy how much we would have to include to get this definition */
extern DBBASE *pdbbase;

long epicsShareAPI dbla(const char* match)
{
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
            printf("%s --> %s\n", alias, realname);
        }
    }
    dbFinishEntry(&dbEntry);   
    return 0;
}

/* Needed to overwrite existing dbla */
#ifdef __GNUC__
long __dbla(const char* match) __attribute__ ((alias ("dbla")));
#define dbla __dbla
#endif

static const iocshFuncDef dblaDef =
    { "dbla", 1, (const iocshArg *[]) {
    &(iocshArg) { "record name pattern", iocshArgString },
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
