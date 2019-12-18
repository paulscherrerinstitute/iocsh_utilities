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

#if __WORDSIZE == 64
#define rlim_t_fmt "lu"
#else
#define rlim_t_fmt "llu"
#endif

#if !defined(RLIMIT_NOFILE) && defined(RLIMIT_OFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif

#if !defined(RLIMIT_LOCKS) && defined(RLIMIT_POSIXLOCKS)
#define RLIMIT_LOCKS RLIMIT_POSIXLOCKS
#endif

#if !defined(RLIMIT_PTHREAD) && defined(RLIMIT_NTHR)
#define RLIMIT_PTHREAD RLIMIT_NTHR
#endif

struct { int option; int res; const char* txt; const char* units; unsigned int shift; }
resources[] = {
#ifdef RLIMIT_NPTS
 {'P', RLIMIT_NPTS,        "number of pseudoterminals", "",           0},
#endif
#ifdef RLIMIT_PTHREAD
 {'T', RLIMIT_PTHREAD,     "number of threads",         "",           0},
#endif
#ifdef RLIMIT_SBSIZE
 {'b', RLIMIT_SBSIZE,      "socket buffer size",        "bytes",      0},
#endif
#ifdef RLIMIT_CORE
 {'c', RLIMIT_CORE,        "core file size",            "blocks",     9},
#endif
#ifdef RLIMIT_DATA
 {'d', RLIMIT_DATA,        "data seg size",             "kbytes",    10},
#endif
#ifdef RLIMIT_NICE
 {'e', RLIMIT_NICE,        "scheduling priority",       "",           0},
#endif
#ifdef RLIMIT_FSIZE
 {'f', RLIMIT_FSIZE,       "file size",                 "blocks",     9},
#endif
#ifdef RLIMIT_SIGPENDING
 {'i', RLIMIT_SIGPENDING,  "pending signals",           "",           0},
#endif
#ifdef RLIMIT_KQUEUES
 {'k', RLIMIT_KQUEUES,     "max kqueues",               "",           0},
#endif
#ifdef RLIMIT_MEMLOCK
 {'l', RLIMIT_MEMLOCK,     "max locked memory",         "kbytes",    10},
#endif
#ifdef RLIMIT_RSS
 {'m', RLIMIT_RSS,         "max memory size",           "kbytes",    10},
#endif
#ifdef RLIMIT_NOFILE
 {'n', RLIMIT_NOFILE,      "open files",                "",           0},
#endif
#ifdef RLIMIT_PIPESIZE
 {'p', RLIMIT_PIPESIZE,    "pipe size",                 "512 bytes",  0},
#endif
#ifdef RLIMIT_MSGQUEUE
 {'q', RLIMIT_MSGQUEUE,    "POSIX message queues",      "bytes",      0},
#endif
#ifdef RLIMIT_RTPRIO
 {'r', RLIMIT_RTPRIO,      "real-time priority",        "",           0},
#endif
#ifdef RLIMIT_STACK
 {'s', RLIMIT_STACK,       "stack size",                "kbytes",    10},
#endif
#ifdef RLIMIT_CPU
 {'t', RLIMIT_CPU,         "cpu time",                  "seconds",    0},
#endif
#ifdef RLIMIT_NPROC
 {'u', RLIMIT_NPROC,       "max user processes",        "",           0},
#endif
#ifdef RLIMIT_AS
 {'v', RLIMIT_AS,          "virtual memory",            "kbytes",    10},
#endif
#ifdef RLIMIT_SWAP
 {'w', RLIMIT_SWAP,        "swap size",                 "kbytes",    10},
#endif
#ifdef RLIMIT_LOCKS
 {'x', RLIMIT_LOCKS,       "file locks",                "",           0},
#endif
};

static const iocshArg ulimitArg0 = { "[-SH"
#ifdef RLIMIT_NPTS
    "P"
#endif
#ifdef RLIMIT_PTHREAD
    "T"
#endif
#ifdef RLIMIT_SBSIZE
    "b"
#endif
#ifdef RLIMIT_CORE
    "c"
#endif
#ifdef RLIMIT_DATA
    "d"
#endif
#ifdef RLIMIT_NICE
    "e"
#endif
#ifdef RLIMIT_FSIZE
    "f"
#endif
#ifdef RLIMIT_SIGPENDI
    "i"
#endif
#ifdef RLIMIT_KQUEUES
    "k"
#endif
#ifdef RLIMIT_MEMLOCK
    "l"
#endif
#ifdef RLIMIT_RSS
    "m"
#endif
#ifdef RLIMIT_NOFILE
    "n"
#endif
#ifdef RLIMIT_PIPESIZE
    "p"
#endif
#ifdef RLIMIT_MSGQUEUE
    "q"
#endif
#ifdef RLIMIT_RTPRIO
    "r"
#endif
#ifdef RLIMIT_STACK
    "s"
#endif
#ifdef RLIMIT_CPU
    "t"
#endif
#ifdef RLIMIT_NPROC
    "u"
#endif
#ifdef RLIMIT_AS
    "v"
#endif
#ifdef RLIMIT_SWAP
    "w"
#endif
#ifdef RLIMIT_LOCKS
    "x"
#endif
    "]", iocshArgString};
static const iocshArg ulimitArg1 = { "[limit]", iocshArgString};
static const iocshArg * const ulimitArgs[2] = { &ulimitArg0, &ulimitArg1 };
static const iocshFuncDef ulimitDef = { "ulimit", 2, ulimitArgs };

#define SOFT 1
#define HARD 2
#define LONG 4

static void printValue(rlim_t limit, unsigned int shift)
{
    if (limit == RLIM_INFINITY)
        printf ("unlimited");
    else
        printf ("%-9" rlim_t_fmt, limit>>shift);
}

static void printLine(int which, int i, struct rlimit *rlimit)
{
    if (which & LONG)
        printf("%-*s(%s%s-%c) ",
            30-(resources[i].units[0]?(int)strlen(resources[i].units):-2),
            resources[i].txt,
            resources[i].units,
            resources[i].units[0]?", ":"",
            resources[i].option);
    if (which & SOFT)
        printValue(rlimit->rlim_cur, resources[i].shift);
    if ((which & (HARD|SOFT)) == (HARD|SOFT)) printf(" ");
    if (which & HARD)
        printValue(rlimit->rlim_max, resources[i].shift);
    printf("\n");
}

static void ulimitFunc(const iocshArgBuf *args)
{
    struct rlimit rlimit;
    char* limit = args[1].sval;
    char* options = "";
    unsigned int i, j, o;
    int which = 0;

    if (args[0].sval && args[0].sval[0] == '-')
        options = args[0].sval+1;
    else
        limit = args[0].sval;

    if (strchr(options,'S')) which |= SOFT;
    if (strchr(options,'H')) which |= HARD;
    i = strlen(options) - !!(which&SOFT) - !!(which&HARD);
    if (i > 1) which |= LONG;
    if (i == 0) options = "f";
    if (!(which&(SOFT|HARD))) which |= SOFT;

    if (strchr(options, 'a'))
    {
        which |= LONG;
        for (i = 0; i < NELEMENTS(resources); i++)
        {
            if (getrlimit(resources[i].res, &rlimit) != 0) continue;
            printLine(which, i, &rlimit);
        }
        return;
    }
    for (j = 0; (o=options[j]); j++)
    {
        if (o == 'S' || o == 'H') continue;
        for (i = 0; i < NELEMENTS(resources); i++)
            if (resources[i].option == o) break;
        if (i == NELEMENTS(resources))
        {
            fprintf(stderr, "unknown option -%c\n", o);
            continue;
        }
        if (getrlimit(resources[i].res, &rlimit) != 0)
        {
            fprintf(stderr, "getrlimit \"%s\" failed: %m\n", resources[i].txt);
            continue;
        }
        if (!limit)
            printLine(which, i, &rlimit);
        else
        {
            /* set new limit */
            rlim_t newlimit;

            if (strcasecmp(limit, "unlimited") == 0)
                newlimit = RLIM_INFINITY;
            else
            {
                char* p;
                if (sizeof(rlim_t) > sizeof(unsigned long))
                    newlimit = strtoull(limit, &p, 0);
                else
                    newlimit = strtoul(limit, &p, 0);
                newlimit <<= resources[i].shift;
                if (*p != 0)
                {
                    fprintf(stderr, "argument %s must be unsigned integer or \"unlimited\"\n", limit);
                    return;
                }
            }
            if (which & SOFT)
                rlimit.rlim_cur = newlimit;
            if (which & HARD)
                rlimit.rlim_max = newlimit;
            if (setrlimit(resources[i].res, &rlimit) != 0)
            {
                fprintf(stderr, "setrlimit \"%s\" failed: %m\n", resources[i].txt);
                return;
            }
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
