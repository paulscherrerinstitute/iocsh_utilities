/*
* mlock - Lock ioc in memory.
*
* DISCLAIMER: Use at your own risc and so on. No warranty, no refund.
*/

#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"

#ifdef UNIX
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>
#endif

#ifdef UNIX
static const iocshFuncDef mlockDef = { "mlock", 0, NULL };

static void mlockFunc(const iocshArgBuf *args)
{
    int status;
    
    status = mlockall(MCL_CURRENT|MCL_FUTURE);
    
    if (status != 0) {
        if (errno == ENOMEM)
        {
            struct rlimit rlimit;
            getrlimit(RLIMIT_MEMLOCK, &rlimit);
            fprintf(stderr,
                "mlock failed: limit of %ld kBytes exceeded\n",
                rlimit.rlim_cur>>10);
        }
        else
            perror("mlock failed");
    }
}

static const iocshFuncDef munlockDef = { "munlock", 0, NULL };

static void munlockFunc(const iocshArgBuf *args)
{
    int status;
    
    status = munlockall();
    
    if (status != 0) {
        perror("munlock failed");
    }
}
#endif

static void mlockRegister(void)
{
#ifdef UNIX
    iocshRegister(&mlockDef, mlockFunc);
    iocshRegister(&munlockDef, munlockFunc);
#endif
}
epicsExportRegistrar(mlockRegister);
