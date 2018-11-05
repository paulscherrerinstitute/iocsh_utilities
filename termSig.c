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
void termSigHandler(int sig, siginfo_t* info , void* ctx)
{
    /* try to clean up before exit */
    if (termSigDebug)
        errlogPrintf("\nSignal %s (%d)\n", strsignal(sig), sig);
    signal(sig, SIG_DFL);
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

    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
#endif
    epicsAtExit(termSigExitFunc, NULL);
}

epicsExportRegistrar(termSigRegistrar);
