/* ulimit.c
*
*  set resource limits from ioc shell
*
* Copyright (C) 2011 Dirk Zimoch
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define _FILE_OFFSET_BITS 64

#include <string.h>
#include <stdlib.h>
#ifdef UNIX
#include <sys/resource.h>
#endif

#include "dbDefs.h"
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"

#ifdef UNIX

static const iocshArg ulimitArg0 = { "resource", iocshArgString};
static const iocshArg ulimitArg1 = { "limit", iocshArgString};
static const iocshArg * const ulimitArgs[2] = { &ulimitArg0, &ulimitArg1 };
static const iocshFuncDef ulimitDef = { "ulimit", 2, ulimitArgs };

#if __WORDSIZE == 64
#define rlim_t_fmt "lu"
#else
#define rlim_t_fmt "llu"
#endif

static void printLimit(rlim_t limit, unsigned int shift)
{
    if (limit == RLIM_INFINITY)
        printf ("unlimited\n");
    else
        printf ("%" rlim_t_fmt "\n", limit>>shift);
}

struct { char option; int res; const char* txt; const char* units; unsigned int shift; }
resources[] = {
 {'c', RLIMIT_CORE,        "core file size",       "blocks",  9},
 {'d', RLIMIT_DATA,        "data seg size",        "kbytes", 10},
#ifdef RLIMIT_NICE
 {'e', RLIMIT_NICE,        "scheduling priority",  "",        0},
#endif
 {'f', RLIMIT_FSIZE,       "file size",            "blocks",  9},
#ifdef RLIMIT_SIGPENDING
 {'i', RLIMIT_SIGPENDING,  "pending signals",      "",        0},
#endif
 {'l', RLIMIT_MEMLOCK,     "max memory size",      "kbytes", 10},
 {'m', RLIMIT_RSS,         "max memory size",      "kbytes", 10},
 {'n', RLIMIT_NOFILE,      "open files",           "",        0},
#ifdef RLIMIT_MSGQUEUE
 {'q', RLIMIT_MSGQUEUE,    "POSIX message queues", "bytes",   0},
#endif
#ifdef RLIMIT_RTPRIO
 {'r', RLIMIT_RTPRIO,      "real-time priority",   "",        0},
#endif
 {'s', RLIMIT_STACK,       "stack size",           "kbytes", 10},
 {'t', RLIMIT_CPU,         "cpu time",             "seconds", 0},
 {'u', RLIMIT_NPROC,       "max user processes",   "",        0},
 {'v', RLIMIT_AS,          "virtual memory",       "kbytes", 10},
#ifdef RLIMIT_LOCKS
 {'x', RLIMIT_LOCKS,       "file locks",           "",        0},
#endif
};

static void ulimitFunc(const iocshArgBuf *args)
{
    struct rlimit rlimit;
    char* limit = args[1].sval;
    char option = 'f';
    unsigned int i;

    if (args[0].sval && args[0].sval[0] == '-')
        option = args[0].sval[1];
    else
        limit = args[0].sval;

    if (option == 'a')
    {
        for (i = 0; i < NELEMENTS(resources); i++)
        {
            getrlimit(resources[i].res, &rlimit);
            printf("%-*s(%s%s-%c) ",
                30-(resources[i].units[0]?(int)strlen(resources[i].units):-2),
                resources[i].txt,
                resources[i].units,
                resources[i].units[0]?", ":"",
                resources[i].option);
            printLimit(rlimit.rlim_cur, resources[i].shift);
        }
        return;
    }
    for (i = 0; i < NELEMENTS(resources); i++)
    {
        if (option == resources[i].option) break;
    }
    if (i == NELEMENTS(resources))
    {
        printf("unknown option -%c\n", option);
        return;
    }
    if (getrlimit(resources[i].res, &rlimit) != 0)
    {
        printf("getrlimit %s failed: %m\n", resources[i].txt);
        return;
    }
    if (!limit)
    {
        /* report current limit */
        printLimit(rlimit.rlim_cur, resources[i].shift);
    }
    else
    {
        /* set new limit */
        if (strcmp(limit, "unlimited") == 0)
            rlimit.rlim_cur = RLIM_INFINITY;
        else
        {
            char* p;
            rlimit.rlim_cur = strtoul(limit, &p, 0)<<resources[i].shift;
            if (*p != 0)
            {
                printf("argument %s must be unsigned integer or \"unlimited\"\n", limit);
                return;
            }
        }
        if (setrlimit(resources[i].res, &rlimit) != 0)
        {
            printf("setrlimit %s failed: %m\n", resources[i].txt);
            return;
        }
    }
}
#endif

static void ulimitRegister(void)
{
#ifdef UNIX
    iocshRegister(&ulimitDef, ulimitFunc);
#endif
}

epicsExportRegistrar(ulimitRegister);
