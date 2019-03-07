/*
* exec - execute shell commands.
*
* DISCLAIMER: Use at your own risc and so on. No warranty, no refund.
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef UNIX
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#endif

#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"

int execDebug;
epicsExportAddress(int,execDebug);

#ifdef UNIX
static const iocshArg execArg0 = { "command", iocshArgString };
static const iocshArg execArg1 = { "arguments", iocshArgArgv };
static const iocshArg * const execArgs[2] = { &execArg0, &execArg1 };
static const iocshFuncDef execDef = { "exec", 2, execArgs };
static const iocshFuncDef exclDef = { "!", 2, execArgs }; /* alias */

static void execFunc (const iocshArgBuf *args)
{
    char commandline [256];
    int i;
    int status;
    size_t len;
    char *p = commandline;
    char *arg;
    char special;

    if (args[0].sval == NULL)
    {
        p = getenv("SHELL");
        if (!p) p = "/bin/sh";
        strncpy(commandline, p, sizeof(commandline)-1);
        status = system(commandline);
    }
    else
    {
        for (i = 0; i < args[1].aval.ac; i++)
        {
            arg = args[1].aval.av[i];
            len = strlen(arg);
            special = 0;
            if (len) {
                special = arg[len-1];
                if (special == '|' || special == ';' || special == '&') len--;
                else special = 0;
            }

            if (p - commandline + len + 4 >= sizeof(commandline))
            {
                fprintf(stderr, "command line too long\n");
                return;
            }

            /* quote words to protect any special chars */
            /* There will be a probem with quotes in arguments */
            /* use '\"' to pass quotes */
            p += sprintf(p, " \"%.*s\"", (int)len, arg);

            /* add unquoted special chars | ; & */
            if (special) p += sprintf(p, "%c", special);
        }
        if (execDebug) fprintf(stderr, "system(%s)\n", commandline);
        status = system(commandline);
    }
    if (status == -1)
    {
        fprintf(stderr, "system(%s) failed: %s\n", commandline, strerror(errno));
        return;
    }    
    if (WIFSIGNALED(status))
    {
#ifdef __USE_GNU
        fprintf(stderr, "%s killed by signal %d: %s\n",
            args[0].sval, WTERMSIG(status), strsignal(WTERMSIG(status)));
#else
        fprintf(stderr, "%s killed by signal %d\n",
            args[0].sval, WTERMSIG(status));
#endif
    }
    if (WEXITSTATUS(status))
    {
        fprintf(stderr, "exit status is %d\n", WEXITSTATUS(status));
    }
}

/* sleep */
static const iocshArg sleepArg0 = { "seconds", iocshArgDouble };
static const iocshArg * const sleepArgs[1] = { &sleepArg0 };
static const iocshFuncDef sleepDef = { "sleep", 1, sleepArgs };

static void sleepFunc (const iocshArgBuf *args)
{
    struct timespec sleeptime;

    sleeptime.tv_sec = (long) args[0].dval;
    sleeptime.tv_nsec = (long) ((args[0].dval - (long) args[0].dval) * 1000000000);
    while (nanosleep (&sleeptime, &sleeptime) == -1)
    {
        if (errno != EINTR)
        {
            perror("sleep");
            break;
        }
    }
}
#endif

static void
execRegister(void)
{
#ifdef UNIX
    iocshRegister (&execDef, execFunc);
    iocshRegister (&exclDef, execFunc);
    iocshRegister (&sleepDef, sleepFunc);
#endif
}

epicsExportRegistrar(execRegister);

#ifdef vxWorks
#include <vxWorks.h>
#ifdef _WRS_VXWORKS_MAJOR
/* vxWorks 6+ */
#include <private/shellLibP.h>
#else
/* vxWorks 5 */
#include <shellLib.h>
#include "strdup.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "epicsVersion.h"
#define EPICSVER EPICS_VERSION*10000+EPICS_REVISION*100+EPICS_MODIFICATION

#include "macLib.h"
#include "epicsString.h"

char* expand(const char* src)
{
    MAC_HANDLE *mac = NULL;
    size_t len, linesize;
    static char *expanded;

    if (!src || !*src) return (char*)src;
    
    if (!strchr(src, '$'))
    {
        return (char*)src;
    }
    
    if (macCreateHandle(&mac,(
    #if (EPICSVER>=31501)
        const
        #endif
        char*[]){ "", "environ", NULL, NULL }) != 0) return NULL;
    macSuppressWarning(mac, 1);
    #if (EPICSVER<31403)
    {
        extern char** ppGlobalEnviron;
        char** pairs;
        /* Have no environment macro substitution, thus load envionment explicitly */
        /* Actually, environmant macro substitution was introduced in 3.14.3 */
        for (pairs = ppGlobalEnviron; *pairs; pairs++)
        {
            char* var, *eq;

            /* take a copy to replace '=' with null byte */
            if ((var = strdup(*pairs)) == NULL) break;
            eq = strchr(var, '=');
            if (eq)
            {
                *eq = 0;
                macPutValue(mac, var, eq+1);
            }
            free(var);            
        }
    }
    #endif

    linesize = 128;
    expanded = malloc(linesize);
#if (EPICSVER<31400)
    while (expanded && (len = labs(macExpandString(mac, (char*)src, expanded, linesize/2))) >= linesize/2)
#else       
    while (expanded && (len = labs(macExpandString(mac, src, expanded, linesize-1))) >= linesize-2)
#endif
    {
        free(expanded);
        if ((expanded = malloc(linesize *= 2)) == NULL) break;
    }
    macDeleteHandle(mac);
    return expanded;
}

int exec(const char* cmd)
{
    char *expanded = expand(cmd);
    if (!expanded || !*expanded) return 0;
    printf("%s\n", expanded);
#ifdef _WRS_VXWORKS_MAJOR
/* vxWorks 6 */
    {
        SHELL_EVAL_VALUE result;
        shellInterpEvaluate(expanded, "C", &result);
        return result.value.intVal;
    }
#else
/* vxWorks 5 */
    return execute(expanded);
#endif
}
#endif

