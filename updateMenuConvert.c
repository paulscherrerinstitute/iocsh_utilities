/* updateMenuConvert.c
*
*  add all breakpoint tables found on this ioc
*  to the menu convert (used by LINR field)
*
* DISCLAIMER: Use at your own risc and so on. No warranty, no refund.
*/

#include <string.h>
#include <stdlib.h>

#include "ellLib.h"
#include "dbScan.h"
#include "dbStaticLib.h"
#include "dbAccess.h"
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
extern DBBASE *pdbbase;
#else
#include "epicsStdioRedirect.h"
#include "iocsh.h"
#include "epicsExport.h"
#endif

typedef struct node {
    ELLNODE node;
    char    *name;
    char    *value;
} node;

int updateMenuConvert ()
{
    brkTable *pbrkTable;
    dbMenu   *menuConvert;
    ELLLIST  missing;              
    node     *pbtable;               
    int      l, i, found, nChoice;   
    char     **papChoiceName;      
    char     **papChoiceValue;     
    
    if (interruptAccept)
    {
        fprintf(stderr, "updateMenuConvert: Can update menuConvert only before iocInit!\n");
        return -1;
    }
    menuConvert = dbFindMenu(pdbbase,"menuConvert");
    ellInit(&missing);
    for(pbrkTable = (brkTable *)ellFirst(&pdbbase->bptList);
        pbrkTable;
        pbrkTable = (brkTable *)ellNext(&pbrkTable->node))
    {
        found=0;
        for(i=0; i<menuConvert->nChoice; i++)
        {
            if (strcmp(menuConvert->papChoiceValue[i],pbrkTable->name)==0)
            {
                found=1;
                break;
            }
        }
        if (!found)
        {
            pbtable = dbCalloc(1,sizeof(struct node));
            l=strlen(pbrkTable->name);
            pbtable->name = dbCalloc(l+12,1);
            pbtable->value = dbCalloc(l+1,1);
            strcpy(pbtable->name, "menuConvert");
            strcpy(pbtable->name+11, pbrkTable->name);
            strcpy(pbtable->value, pbrkTable->name);
            ellAdd(&missing, &pbtable->node);
        }
    }
    if (ellCount(&missing))
    {
        nChoice = menuConvert->nChoice + ellCount(&missing);
        
        papChoiceName=dbCalloc(nChoice,sizeof(char*));
        papChoiceValue=dbCalloc(nChoice,sizeof(char*));
        for (i=0; i<menuConvert->nChoice; i++)
        {
            papChoiceName[i] = menuConvert->papChoiceName[i];
            papChoiceValue[i] = menuConvert->papChoiceValue[i];
        }
        for (; i<nChoice; i++)
        {
            pbtable = (node*)ellFirst(&missing);
            printf("Adding breakpoint table \"%s\" to menuConvert.\n", pbtable->value);
            papChoiceName[i] = pbtable->name;
            papChoiceValue[i] = pbtable->value;
            ellDelete(&missing, &pbtable->node);
        }
        free(menuConvert->papChoiceName);
        free(menuConvert->papChoiceValue);
        menuConvert->papChoiceName=papChoiceName;
        menuConvert->papChoiceValue=papChoiceValue;
        menuConvert->nChoice = nChoice;
    }
    else
    {
        printf("Nothing to add to menuConvert.\n");
    }
    return 0;
}

#ifndef EPICS_3_13
static const iocshFuncDef updateMenuConvertDef = { "updateMenuConvert", 0, NULL };
static void updateMenuConvertFunc (const iocshArgBuf *args)
{
    updateMenuConvert();
}
static void updateMenuConvertRegister(void)
{
    iocshRegister (&updateMenuConvertDef, updateMenuConvertFunc);
}
epicsExportRegistrar(updateMenuConvertRegister);
#endif

