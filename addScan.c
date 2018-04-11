/* addScan.c
*
*  add a new scan rate to the ioc
*
*  $Author: zimoch $
*
*  $Source: /cvs/G/DRV/misc/addScan.c,v $
*
*/

#include <string.h>
#include <stdlib.h>
#ifdef vxWorks
#include <sysLib.h>
#endif
#include "dbScan.h"
#include "dbStaticLib.h"
#include "dbAccess.h"
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
extern DBBASE *pdbbase;
#else
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"
#endif


int addScan (char* ratestr)
{
    dbMenu  *menuScan;
    int     l, i, j, nChoice;   
    char    **papChoiceName;      
    char    **papChoiceValue;
    double  rate, r;
    char    dummy;
    char    *name;
    
    if (interruptAccept)
    {
        fprintf(stderr, "addScan: Can add a scan period only before iocInit!\n");
        return -1;
    }
    if (!ratestr || sscanf (ratestr, "%lf%c", &rate, &dummy) != 1 || rate <= 0)
    {
        fprintf(stderr, "addScan: Argument '%s' must be a number > 0\n", ratestr);
        return -1;
    }    
    menuScan = dbFindMenu(pdbbase,"menuScan");
    nChoice = menuScan->nChoice;
    for (i=SCAN_1ST_PERIODIC; i < nChoice; i++)
    {
        r = strtod(menuScan->papChoiceValue[i], NULL);
        if (r == rate)
        {
            fprintf(stderr, "addScan: rate %s already available\n", menuScan->papChoiceValue[i]);
            return 0;
        }
        if (r < rate) break;
    }
    papChoiceName=dbCalloc(nChoice+1,sizeof(char*));
    papChoiceValue=dbCalloc(nChoice+1,sizeof(char*));
    for (j=0; j < i; j++)
    {
        papChoiceName[j] = menuScan->papChoiceName[j];
        papChoiceValue[j] = menuScan->papChoiceValue[j];
    }
    name = ratestr;
    while (name[0]=='0') name++;
    l = strlen(name);
    papChoiceValue[i] = dbCalloc(l+16,1);
    strcpy(papChoiceValue[i], name);
    strcpy(papChoiceValue[i]+l, " second");
    for (j=i; j < nChoice; j++)
    {
        papChoiceName[j+1] = menuScan->papChoiceName[j];
        papChoiceValue[j+1] = menuScan->papChoiceValue[j];
    }
    free(menuScan->papChoiceName);
    free(menuScan->papChoiceValue);
    menuScan->papChoiceName=papChoiceName;
    menuScan->papChoiceValue=papChoiceValue;
    menuScan->nChoice = nChoice+1;
#ifdef vxWorks
#ifndef SYS_CLK_RATE_MAX
#define SYS_CLK_RATE_MAX 5000
#endif
    i = (int)4/rate;
    if (sysClkRateGet() < i)
    {
        if (i > SYS_CLK_RATE_MAX)
        {
            i = SYS_CLK_RATE_MAX;
            fprintf(stderr, "addScan: sysClkRate is limited to %i Hz! Scan rate may not work as expected.\n", i);
        }
        fprintf(stderr, "addScan: increasing sysClkRate from %d to %d Hz\n",
            sysClkRateGet(), i);
        if (sysClkRateSet(i) != 0)
        {
            fprintf(stderr, "addScan: increasing sysClkRate failed! Scan rate may not work as expected.\n");
        }
    }
#endif
    return 0;    
}

#ifndef EPICS_3_13
static const iocshArg addScanArg0 = { "rate", iocshArgString };
static const iocshArg * const addScanArgs[1] = { &addScanArg0 };
static const iocshFuncDef addScanDef = { "addScan", 1, addScanArgs };
static void addScanFunc (const iocshArgBuf *args)
{
    addScan(args[0].sval);
}
static void addScanRegister(void)
{
    iocshRegister (&addScanDef, addScanFunc);
}
epicsExportRegistrar(addScanRegister);
#endif

