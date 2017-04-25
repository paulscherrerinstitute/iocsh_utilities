#include <stdio.h>
#include <stdlib.h>

#define DBFLDTYPES_GBLSOURCE
#include "dbAccess.h"
#include "dbStaticLib.h"
#include "dbFldTypes.h"
#include "special.h"
#include "recSup.h"
#include "envDefs.h"
#include "initHooks.h"
#include "epicsExport.h"

extern DBBASE *pdbbase;
int setMaxArrayBytesDebug;
epicsExportAddress(int, setMaxArrayBytesDebug);

static void setMaxArrayBytes(initHookState initState)
{
    DBENTRY dbEntry;
    DBADDR addr;
    struct rset *prset;
    dbRecordNode *precnode;
    long status, size, maxNeeded = 0, maxBytes = 0;

    if (initState != initHookAfterFinishDevSup) return;

    dbInitEntry(pdbbase,&dbEntry);

    /* foreach record type */
    for (status = dbFirstRecordType(&dbEntry); status == 0; status = dbNextRecordType(&dbEntry))
    {
        /* Any records of this type? */
        if (dbGetNRecords(&dbEntry) == 0) continue;

        /* foreach field */
        for (status = dbFirstField(&dbEntry, 0); status == 0; status = dbNextField(&dbEntry, 0))
        {
            /* all array fields are SPC_DBADDR */
            if (dbEntry.pflddes->special != SPC_DBADDR) continue;
        
            if (setMaxArrayBytesDebug)
                printf("%s.%s\n", dbGetRecordTypeName(&dbEntry), dbGetFieldName(&dbEntry));

            /* foreach record */
            for (precnode = (dbRecordNode *)ellFirst(&dbEntry.precordType->recList); precnode; precnode = (dbRecordNode *)ellNext(&precnode->node))
            {
                addr.pfldDes = dbEntry.pflddes;
                addr.precord = precnode->precord;

                /* get array type and size */
                prset = dbGetRset(&addr);
                if (!prset)
                {
                    printf("%s no prset\n", addr.precord->name);
                    continue;
                }
                prset->cvt_dbaddr(&addr);
                size = dbBufferSize(addr.field_type, 0, addr.no_elements);
                if (size > maxNeeded) maxNeeded = size;
            
                if (setMaxArrayBytesDebug)
                    printf("%s.%s %s[%ld] %ld bytes\n", addr.precord->name, dbGetFieldName(&dbEntry),
                        pamapdbfType[addr.field_type].strvalue+4, addr.no_elements, size);
            }
        }
    }
    dbFinishEntry(&dbEntry);
    maxNeeded += 450; /* space for overhead (sizeof(dbr_ctrl_enum) == 422) */
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
    static int firstTime = 1;
    if (firstTime) {
        initHookRegister(setMaxArrayBytes);
        firstTime = 0;
    }
}
epicsExportRegistrar(setMaxArrayBytesRegister);
