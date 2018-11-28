#include <string.h>
#include <stdlib.h>
#include "epicsStdioRedirect.h"
#include "epicsString.h"
#include "epicsThread.h"
#include "iocsh.h"
#include "epicsExport.h"

static const iocshArg epicsThreadSetPriorityArg0 = { "thread", iocshArgString };
static const iocshArg epicsThreadSetPriorityArg1 = { "priority", iocshArgString };
static const iocshArg epicsThreadSetPriorityArg2 = { "[+/-diff]", iocshArgInt };
static const iocshArg * const epicsThreadSetPriorityArgs[3] = { &epicsThreadSetPriorityArg0, &epicsThreadSetPriorityArg1, &epicsThreadSetPriorityArg2};
static const iocshFuncDef epicsThreadSetPriorityDef = { "epicsThreadSetPriority", 3, epicsThreadSetPriorityArgs };

static void epicsThreadSetPriorityFunc(const iocshArgBuf *args)
{
    const char* threadname = args[0].sval;
    const char* priostr = args[1].sval;
    int diff = args[2].ival;
    char* end;
    epicsThreadId id;
    unsigned int priority;

    if (!threadname || !*threadname || !priostr || !*priostr) {
        fprintf(stderr, "usage: epicsThreadSetPriorityFunc thread, priority");
        return;
    }
#if defined(__LP64__) || defined(_WIN64)
    id = (epicsThreadId)(size_t) strtoull(threadname, &end, 0);
#else
    id = (epicsThreadId)(size_t) strtoul(threadname, &end, 0);
#endif
    if (*end || !id)
    {
        id = epicsThreadGetId(threadname);
        if (!id)
        {
            fprintf(stderr, "Thread %s not found.\n", threadname);
            return;
        }
    }

    priority = strtol(priostr, &end, 0);
    if (!*end)
    {
        if ((int)priority < epicsThreadPriorityMin ||
            priority > epicsThreadPriorityMax)
        {
            fprintf(stderr, "Priority %s out of range %d-%d.\n",
                priostr, epicsThreadPriorityMin, epicsThreadPriorityMax);
            return;
        }
    }
    else
    {
        if (epicsStrCaseCmp(priostr, "Min") == 0)
            priority = epicsThreadPriorityMin;
        else if (epicsStrCaseCmp(priostr, "Max") == 0)
            priority = epicsThreadPriorityMax;
        else if (epicsStrCaseCmp(priostr, "Low") == 0)
            priority = epicsThreadPriorityLow;
        else if (epicsStrCaseCmp(priostr, "Medium") == 0)
            priority = epicsThreadPriorityMedium;
        else if (epicsStrCaseCmp(priostr, "High") == 0)
            priority = epicsThreadPriorityHigh;
        else if (epicsStrCaseCmp(priostr, "CAServerLow") == 0)
            priority = epicsThreadPriorityCAServerLow;
        else if (epicsStrCaseCmp(priostr, "CAServerHigh") == 0)
            priority = epicsThreadPriorityCAServerHigh;
        else if (epicsStrCaseCmp(priostr, "ScanLow") == 0)
            priority = epicsThreadPriorityScanLow;
        else if (epicsStrCaseCmp(priostr, "ScanHigh") == 0)
            priority = epicsThreadPriorityScanHigh;
        else if (epicsStrCaseCmp(priostr, "Iocsh") == 0)
            priority = epicsThreadPriorityIocsh;
        else if (epicsStrCaseCmp(priostr, "BaseMax") == 0)
            priority = epicsThreadPriorityBaseMax;
        else {
            epicsThreadId otherthread = epicsThreadGetId(priostr);
            if (otherthread)
                priority = epicsThreadGetPriority(otherthread);
            else
            {
                fprintf(stderr, "Unknown priority %s.\n", priostr);
                return;
            }
        }
        while (diff++ < 0 && priority > epicsThreadPriorityMin)
            epicsThreadHighestPriorityLevelBelow(priority, &priority);
        while (diff-- > 0 && priority < epicsThreadPriorityMax)
            epicsThreadLowestPriorityLevelAbove(priority, &priority);
    }
    epicsThreadSetPriority(id, priority);
}

static void
threadPrioRegister(void)
{
    iocshRegister(&epicsThreadSetPriorityDef, epicsThreadSetPriorityFunc);
}

epicsExportRegistrar(threadPrioRegister);
