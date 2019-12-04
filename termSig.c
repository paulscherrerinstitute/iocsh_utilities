/* termSig.c
*
*  call epicsExitCallAtExits() even when ioc terminates with a signal (e.g. CTRL-C)
*
* Copyright (C) 2018 Dirk Zimoch
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

#define _GNU_SOURCE 1
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include "errlog.h"
#include "epicsExit.h"
#include "epicsExport.h"

int termSigDebug = 0;
epicsExportAddress(int, termSigDebug);

#ifndef vxWorks
struct sigaction old_handler_SIGHUP;
struct sigaction old_handler_SIGINT;
struct sigaction old_handler_SIGPIPE;
struct sigaction old_handler_SIGALRM;
struct sigaction old_handler_SIGTERM;
struct sigaction old_handler_SIGUSR1;
struct sigaction old_handler_SIGUSR2;

void termSigHandler(int sig, siginfo_t* info , void* ctx)
{
    /* try to clean up before exit */
    if (termSigDebug)
        errlogPrintf("\nSignal %s (%d)\n", strsignal(sig), sig);

    /* restore original handlers */
    sigaction(SIGHUP,  &old_handler_SIGHUP,  NULL);
    sigaction(SIGINT,  &old_handler_SIGINT,  NULL);
    sigaction(SIGPIPE, &old_handler_SIGPIPE, NULL);
    sigaction(SIGALRM, &old_handler_SIGALRM, NULL);
    sigaction(SIGTERM, &old_handler_SIGTERM, NULL);
    sigaction(SIGUSR1, &old_handler_SIGUSR1, NULL);
    sigaction(SIGUSR2, &old_handler_SIGUSR2, NULL);

    epicsExitCallAtExits(); /* will only start executing handlers once */
    /* send the same signal again */
    raise(sig);
}
#endif

static void termSigExitFunc(void* arg)
{
    if (termSigDebug)
        errlogPrintf("Exit handlers executing\n");
}

static void termSigRegistrar (void)
{
#ifndef vxWorks
    struct sigaction sa = {{0}};

    /* make sure that exit handlers are called even at signal termination (e,g CTRL-C) */
    sa.sa_sigaction = termSigHandler;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGHUP,  &sa, &old_handler_SIGHUP);
    sigaction(SIGINT,  &sa, &old_handler_SIGINT);
    sigaction(SIGPIPE, &sa, &old_handler_SIGPIPE);
    sigaction(SIGALRM, &sa, &old_handler_SIGALRM);
    sigaction(SIGTERM, &sa, &old_handler_SIGTERM);
    sigaction(SIGUSR1, &sa, &old_handler_SIGUSR1);
    sigaction(SIGUSR2, &sa, &old_handler_SIGUSR2);
#endif
    epicsAtExit(termSigExitFunc, NULL);
}

epicsExportRegistrar(termSigRegistrar);
