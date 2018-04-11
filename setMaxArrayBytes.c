#include <stdlib.h>

#define DBFLDTYPES_GBLSOURCE
#include "dbAccess.h"
#include "dbStaticLib.h"
#include "dbFldTypes.h"
#include "special.h"
#include "recSup.h"
#include "envDefs.h"
#include "initHooks.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"

extern DBBASE *pdbbase;
int setMaxArrayBytesDebug;
epicsExportAddress(int, setMaxArrayBytesDebug);
extern long caFieldSize(int type, long nelem);

static void setMaxArrayBytes(initHookState initState)
{
    DBENTRY dbEntry;
    DBADDR addr;
    struct rset *prset;
    dbRecordNode *precnode;
    long status, size, maxNeeded = 0, maxBytes = 0;

    if (initState != initHookAfterFinishDevSup) return;
    
    if (setMaxArrayBytesDebug)
        fprintf(stderr, "reading array sizes\n");

    dbInitEntry(pdbbase,&dbEntry);

    /* foreach record type */
    for (status = dbFirstRecordType(&dbEntry); status == 0; status = dbNextRecordType(&dbEntry))
    {
        /* Any records of this type? */
        if (dbGetNRecords(&dbEntry) == 0) continue;

        /* foreach field */
        for (status = dbFirstField(&dbEntry, 0); status == 0; status = dbNextField(&dbEntry, 0))
        {
            /* all array fields are DBF_NOACCESS and SPC_DBADDR */
            if (dbEntry.pflddes->field_type != DBF_NOACCESS) continue;
            if (dbEntry.pflddes->special != SPC_DBADDR) continue;
        
            if (setMaxArrayBytesDebug)
               fprintf(stderr, "%s.%s\n", dbGetRecordTypeName(&dbEntry), dbGetFieldName(&dbEntry));

            /* foreach record */
            for (precnode = (dbRecordNode *)ellFirst(&dbEntry.precordType->recList); precnode; precnode = (dbRecordNode *)ellNext(&precnode->node))
            {
                addr.pfldDes = dbEntry.pflddes;
                addr.pfield = dbEntry.pfield;
                addr.precord = precnode->precord;
                addr.no_elements = 1;

                /* get array type and size */
                prset = dbGetRset(&addr);
                if (!prset)
                {
                    fprintf(stderr, "%s no prset\n", addr.precord->name);
                    continue;
                }
                prset->cvt_dbaddr(&addr);
                size = caFieldSize(addr.field_type, addr.no_elements);
                if (size > maxNeeded) maxNeeded = size;
            
                if (setMaxArrayBytesDebug)
                    fprintf(stderr, "%s.%s %s[%ld] %ld bytes\n", addr.precord->name, dbGetFieldName(&dbEntry),
                        pamapdbfType[addr.field_type].strvalue+4, addr.no_elements, size);
            }
        }
    }
    dbFinishEntry(&dbEntry);
    envGetLongConfigParam(&EPICS_CA_MAX_ARRAY_BYTES, &maxBytes);
    if (maxNeeded > maxBytes)
    {
        char buffer[20];
        sprintf(buffer, "%ld", maxNeeded);
        epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", buffer);
        printf("EPICS_CA_MAX_ARRAY_BYTES = %s\n", getenv("EPICS_CA_MAX_ARRAY_BYTES"));
    }
    else
    {
        printf("EPICS_CA_MAX_ARRAY_BYTES = %ld is sufficient (%ld needed)\n", maxBytes, maxNeeded);
    }
}

static void setMaxArrayBytesRegister(void)
{
    initHookRegister(setMaxArrayBytes);
}
epicsExportRegistrar(setMaxArrayBytesRegister);
