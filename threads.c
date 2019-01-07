#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "epicsStdioRedirect.h"
#include "epicsString.h"
#include "epicsThread.h"
#include "iocsh.h"
#include "errlog.h"
#include "epicsExport.h"

#ifndef CPU_SETSIZE
typedef unsigned int cpu_set_t;
#define CPU_SETSIZE (8*sizeof(cpu_set_t))
#define CPU_ZERO(set)       (*(set)=0)
#define CPU_SET(cpu, set)   (*(set)|=(1<<cpu))
#define CPU_CLR(cpu, set)   (*(set)&=~(1<<cpu))
#define CPU_ISSET(cpu, set) (!!(*(set)&(1<<cpu)))
#endif

#ifndef __GLIBC_PREREQ
#define __GLIBC_PREREQ(a,b) 0
#endif

#if !__GLIBC_PREREQ(2,4)
#define pthread_getaffinity_np(id, size, set) ENOSYS
#define pthread_setaffinity_np(id, size, set) ENOSYS
#endif

/* Original epicsThreadPriorityMax 99 is wrong */
#undef epicsThreadPriorityMax
#define epicsThreadPriorityMax (100)

static const iocshArg epicsThreadSetPriorityArg0 = { "thread", iocshArgString };
static const iocshArg epicsThreadSetPriorityArg1 = { "priority", iocshArgString };
static const iocshArg epicsThreadSetPriorityArg2 = { "[+/-diff]", iocshArgInt };
static const iocshArg * const epicsThreadSetPriorityArgs[3] = { &epicsThreadSetPriorityArg0, &epicsThreadSetPriorityArg1, &epicsThreadSetPriorityArg2};
static const iocshFuncDef epicsThreadSetPriorityDef = { "epicsThreadSetPriority", 3, epicsThreadSetPriorityArgs };

static epicsThreadId epicsThreadGetIdFromNameOrNumber(const char* threadname)
{
    epicsThreadId id;
    char* end;

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
            return NULL;
        }
    }
    return id;
}

static int epicsThreadStrToPrio(const char* priostr, int diff)
{
    int priority;
    char* end;

    priority = strtol(priostr, &end, 0);
    if (!*end)
    {

        if (priority < epicsThreadPriorityMin ||
            priority > epicsThreadPriorityMax)
        {
            fprintf(stderr, "Priority %s out of range %d-%d.\n",
                priostr, epicsThreadPriorityMin, epicsThreadPriorityMax);
            return -1;
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
                return -1;
            }
        }
        while (diff++ < 0 && priority > epicsThreadPriorityMin)
            epicsThreadHighestPriorityLevelBelow(priority, (unsigned int *)&priority);
        while (diff-- > 0 && priority < 100)
            epicsThreadLowestPriorityLevelAbove(priority, (unsigned int *)&priority);
    }
    return priority;
}

static void epicsThreadSetPriorityFunc(const iocshArgBuf *args)
{
    const char* threadname = args[0].sval;
    const char* priostr = args[1].sval;
    int diff = args[2].ival;
    epicsThreadId id;
    int priority;
    int old_errVerbose = errVerbose;

    if (!threadname || !*threadname || !priostr || !*priostr)
    {
        fprintf(stderr, "usage: epicsThreadSetPriority thread, priority\n");
        return;
    }
    id = epicsThreadGetIdFromNameOrNumber(threadname);
    if (!id) return;
    priority = epicsThreadStrToPrio(priostr, diff);
    if (priority < 0) return;
    errVerbose = 1;
    epicsThreadSetPriority(id, priority);
    errVerbose = old_errVerbose;
}

void epicsThreadPrintAffinityList(cpu_set_t* cpuset)
{
    int cpu, first = -2;
    
    for (cpu = 0; cpu <= CPU_SETSIZE; cpu++)
    {
        if (cpu < CPU_SETSIZE && CPU_ISSET(cpu, cpuset))
        {
            if (first < 0)
            {
                if (first == -1) printf(",");
                printf("%u", cpu);
                first = cpu;
            }
        }
        else
        {
            if (first >= 0)
            {
                if (first < cpu-1) printf("-%u", cpu-1);
                first = -1;
            }
        }
    }
    printf("\n");
}

static int epicsThreadParseAffinityList(const char* cpulist, cpu_set_t* cpuset)
{
    int n = 0;
    unsigned int first, last, cpu;
    char sign = 0, c;

    if (sscanf(cpulist, " %[+-]%n", &sign, &n)) cpulist+=n;
    if (sign == 0) CPU_ZERO(cpuset);

    do {
        if (n = 0, sscanf(cpulist, " %[+-]%n", &sign, &n)) cpulist+=n;
        if (n = 0, sscanf(cpulist, "%u %n", &first, &n))
        {
            cpulist+=n;
            last = first;
            if (n = 0, sscanf(cpulist, "-%u %n", &last, &n)) cpulist+=n;
            for (cpu = first; cpu <= last; cpu++)
            {
                if (cpu >= CPU_SETSIZE) break;
                if (sign == '-') CPU_CLR(cpu, cpuset);
                else CPU_SET(cpu, cpuset);
            }
        }
        c = 0;
        if (n = 0, sscanf(cpulist, "%[,;] %n", &c, &n)) cpulist+=n;
    } while (c);
    if (cpulist[0])
        fprintf(stderr, "rubbish at end of list: \"%s\"\n", cpulist);
    return 0;
}

int epicsThreadGetAffinity(epicsThreadId id, cpu_set_t* cpuset)
{
    int status = ESRCH;
    if (id) status = pthread_getaffinity_np(epicsThreadGetPosixThreadId(id), sizeof(cpu_set_t), cpuset);
    switch (status)
    {
        case 0:
            break;
        case EINVAL:
            fprintf(stderr, "epicsThreadGetAffinity: invalid mask\n");
            break;
        case ESRCH:
            fprintf(stderr, "epicsThreadGetAffinity: invalid thread\n");
            break;
        case ENOSYS:
            fprintf(stderr, "epicsThreadGetAffinity: function not implemented\n");
            break;
        default:
            fprintf(stderr, "epicsThreadGetAffinity: unknown error\n");
    }
    return status;
}

static const iocshArg epicsThreadGetAffinityArg0 = { "thread", iocshArgString };
static const iocshArg * const epicsThreadGetAffinityArgs[1] = { &epicsThreadGetAffinityArg0};
static const iocshFuncDef epicsThreadGetAffinityDef = { "epicsThreadGetAffinity", 1, epicsThreadGetAffinityArgs };

static void epicsThreadGetAffinityFunc(const iocshArgBuf *args)
{
    const char* threadname = args[0].sval;
    cpu_set_t cpuset;
    epicsThreadId id;

    if (!threadname || !*threadname)
    {
        fprintf(stderr, "usage: epicsThreadGetAffinity thread\n");
        return;
    }
    id = epicsThreadGetIdFromNameOrNumber(threadname);
    epicsThreadGetAffinity(id, &cpuset);
    epicsThreadPrintAffinityList(&cpuset);
}

int epicsThreadSetAffinity(epicsThreadId id, cpu_set_t* cpuset)
{
    int status = ESRCH;
    if (id) status = pthread_setaffinity_np(epicsThreadGetPosixThreadId(id), sizeof(cpuset), cpuset);
    switch (status)
    {
        case 0:
            break;
        case EINVAL:
            fprintf(stderr, "epicsThreadSetAffinity: invalid mask\n");
            break;
        case ESRCH:
            fprintf(stderr, "epicsThreadSetAffinity: invalid thread\n");
            break;
        case ENOSYS:
            fprintf(stderr, "epicsThreadSetAffinity: function not implemented\n");
            break;
        default:
            fprintf(stderr, "epicsThreadSetAffinity: unknown error\n");
    }
    return status;
}

static const iocshArg epicsThreadSetAffinityArg0 = { "thread", iocshArgString };
static const iocshArg epicsThreadSetAffinityArg1 = { "cpulist", iocshArgString };
static const iocshArg * const epicsThreadSetAffinityArgs[2] = { &epicsThreadSetAffinityArg0, &epicsThreadSetAffinityArg1};
static const iocshFuncDef epicsThreadSetAffinityDef = { "epicsThreadSetAffinity", 2, epicsThreadSetAffinityArgs };

static void epicsThreadSetAffinityFunc(const iocshArgBuf *args)
{
    const char* threadname = args[0].sval;
    const char* cpulist = args[1].sval;
    cpu_set_t cpuset;
    epicsThreadId id;

    if (!threadname || !*threadname || !cpulist || !*cpulist)
    {
        fprintf(stderr, "usage: epicsThreadSetAffinity thread, affinitymask\n");
        return;
    }
    id = epicsThreadGetIdFromNameOrNumber(threadname);
    epicsThreadGetAffinity(id, &cpuset);
    epicsThreadParseAffinityList(cpulist, &cpuset);
    epicsThreadSetAffinity(id, &cpuset);
    epicsThreadGetAffinity(id, &cpuset);
    epicsThreadPrintAffinityList(&cpuset);
}


static void
threadsRegister(void)
{
    iocshRegister(&epicsThreadSetPriorityDef, epicsThreadSetPriorityFunc);
    iocshRegister(&epicsThreadGetAffinityDef, epicsThreadGetAffinityFunc);
    iocshRegister(&epicsThreadSetAffinityDef, epicsThreadSetAffinityFunc);
}

epicsExportRegistrar(threadsRegister);
