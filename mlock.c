#include "iocsh.h"
#include "epicsExport.h"
#include "epicsStdioRedirect.h"

#ifdef UNIX
#include <sys/mman.h>
#endif

#ifdef UNIX
static const iocshFuncDef mlockDef = { "mlock", 0, NULL };

static void mlockFunc(const iocshArgBuf *args)
{
    int status;
    
    status = mlockall(MCL_CURRENT|MCL_FUTURE);
    
    if (status != 0) {
        perror("mlock failed");
    }
}

static const iocshFuncDef munlockDef = { "munlock", 0, NULL };

static void munlockFunc(const iocshArgBuf *args)
{
    int status;
    
    status = munlockall();
    
    if (status != 0) {
        perror("mlock failed");
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
