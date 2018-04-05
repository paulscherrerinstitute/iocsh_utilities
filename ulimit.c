#include <string.h>
#include <stdlib.h>
#ifdef UNIX
#include <sys/resource.h>
#endif
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"

#ifdef UNIX

static const iocshArg ulimitArg0 = { "resource", iocshArgString};
static const iocshArg ulimitArg1 = { "limit", iocshArgString};
static const iocshArg * const ulimitArgs[2] = { &ulimitArg0, &ulimitArg1 };
static const iocshFuncDef ulimitDef = { "ulimit", 2, ulimitArgs };

static void printLimit(rlim_t limit, unsigned int shift)
{
    if (limit == RLIM_INFINITY)
        printf ("unlimited\n");
    else printf ("%lu\n", limit>>shift);
}

static void ulimitFunc(const iocshArgBuf *args)
{
    int resource;
    struct rlimit rlimit;
    char* limit = args[1].sval;
    char option = 'f';
    unsigned int shift = 0;

    if (args[0].sval && args[0].sval[0] == '-')
        option = args[0].sval[1];
    else
        limit = args[0].sval;
    switch (option)
    {
        case 'a':
        /* report all */
            getrlimit(RLIMIT_CORE, &rlimit);
            printf("core file size          (blocks, -c) ");
            printLimit(rlimit.rlim_cur, 9);
            getrlimit(RLIMIT_DATA, &rlimit);
            printf("data seg size           (kbytes, -d) ");
            printLimit(rlimit.rlim_cur, 10);
        #ifdef RLIMIT_NICE
            getrlimit(RLIMIT_NICE, &rlimit);
            printf("scheduling priority             (-e) ");
            printLimit(rlimit.rlim_cur, 0);
        #endif
            getrlimit(RLIMIT_FSIZE, &rlimit);
            printf("file size               (blocks, -f) ");
            printLimit(rlimit.rlim_cur, 9);
            getrlimit(RLIMIT_SIGPENDING, &rlimit);
            printf("pending signals                 (-i) ");
            printLimit(rlimit.rlim_cur, 0);
            getrlimit(RLIMIT_MEMLOCK, &rlimit);
            printf("max locked memory       (kbytes, -l) ");
            printLimit(rlimit.rlim_cur, 10);
            getrlimit(RLIMIT_RSS, &rlimit);
            printf("max memory size         (kbytes, -m) ");
            printLimit(rlimit.rlim_cur, 10);
            getrlimit(RLIMIT_NOFILE, &rlimit);
            printf("open files                      (-n) ");
            printLimit(rlimit.rlim_cur, 0);
            getrlimit(RLIMIT_MSGQUEUE, &rlimit);
            printf("POSIX message queues     (bytes, -q) ");
            printLimit(rlimit.rlim_cur, 0);
        #ifdef RLIMIT_RTPRIO
            getrlimit(RLIMIT_RTPRIO, &rlimit);
            printf("real-time priority              (-r) ");
            printLimit(rlimit.rlim_cur, 0);
        #endif
            getrlimit(RLIMIT_STACK, &rlimit);
            printf("stack size              (kbytes, -s) ");
            printLimit(rlimit.rlim_cur, 10);
            getrlimit(RLIMIT_CPU, &rlimit);
            printf("cpu time               (seconds, -t) ");
            printLimit(rlimit.rlim_cur, 0);
            getrlimit(RLIMIT_NPROC, &rlimit);
            printf("max user processes              (-u) ");
            printLimit(rlimit.rlim_cur, 0);
            getrlimit(RLIMIT_AS, &rlimit);
            printf("virtual memory          (kbytes, -v) ");
            printLimit(rlimit.rlim_cur, 10);
            getrlimit(RLIMIT_LOCKS, &rlimit);
            printf("file locks                      (-x) ");
            printLimit(rlimit.rlim_cur, 0);
            return;
        case 'c': resource = RLIMIT_CORE; shift = 9; break;
        case 'd': resource = RLIMIT_DATA; shift = 10; break;
        #ifdef RLIMIT_NICE
        case 'e': resource = RLIMIT_NICE; break;
        #endif
        case 'f': resource = RLIMIT_FSIZE; shift = 9; break;
        case 'i': resource = RLIMIT_SIGPENDING; break;
        case 'l': resource = RLIMIT_MEMLOCK; shift = 10; break;
        case 'm': resource = RLIMIT_RSS; shift = 10; break;
        case 'n': resource = RLIMIT_NOFILE; break;
        case 'q': resource = RLIMIT_MSGQUEUE; break;
        #ifdef RLIMIT_RTPRIO
        case 'r': resource = RLIMIT_RTPRIO; break;
        #endif
        case 's': resource = RLIMIT_STACK; shift = 10; break;
        case 't': resource = RLIMIT_CPU; break;
        case 'u': resource = RLIMIT_NPROC; break;
        case 'v': resource = RLIMIT_AS; shift = 10; break;
        case 'x': resource = RLIMIT_LOCKS; break;
        default:
            printf("unknown option -%c\n", option);
            return;
    }
    if (getrlimit(resource, &rlimit) != 0)
    {
        printf("getrlimit %d %m", resource);
        return;
    }
    if (!limit)
    {
        /* report current limit */
        printLimit(rlimit.rlim_cur, shift);
    }
    else
    {
        /* set new limit */
        if (strcmp(limit, "unlimited") == 0)
            rlimit.rlim_cur = RLIM_INFINITY;
        else
        {
            char* p;
            rlimit.rlim_cur = strtoul(limit, &p, 0)<<shift;
            if (*p != 0)
            {
                printf("argument %s must be unsigned integer or unlimited\n", limit);
                return;
            }
        }
        if (setrlimit(resource, &rlimit) != 0)
        {
            printf("setrlimit %d %m", resource);
            return;
        }
    }
}
#endif

static void ulimitRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
#ifdef UNIX
        iocshRegister(&ulimitDef, ulimitFunc);
#endif
        firstTime = 0;
    }
}

epicsExportRegistrar(ulimitRegister);
