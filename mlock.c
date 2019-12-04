/* mlock.c
*
*  lock virtual address space of ioc in memory
*
* Copyright (C) 2012 Dirk Zimoch
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
