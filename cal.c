/* cal.c
*
*  list channel access connections by record or field
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "caProto.h"
#include "epicsString.h"
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
int epicsStrGlobMatch(const char *str, const char *pattern);
#define epicsMutexMustLock(lock) FASTLOCK(&lock)
#define epicsMutexUnlock(lock)   FASTUNLOCK(&lock)
#define CA_MAJOR_PROTOCOL_REVISION CA_PROTOCOL_VERSION
#include <inetLib.h>
#define ipAddrToA(addr, buffer, size) inet_ntoa_b((addr)->sin_addr, buffer)
struct dbFldDes{
    char *prompt;
    char *name;
};
#include "server.h"
#else
#include "osiSock.h"
#include "dbBase.h"
#include "dbCommon.h"
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "server.h"
#include "epicsExport.h"
#endif

#if EPICS_VERSION*10000+EPICS_REVISION*100+EPICS_MODIFICATION < 31412
#define chanListLock addrqLock
#define chanList     addrq
#endif

#if EPICS_VERSION*10000+EPICS_REVISION*100+EPICS_MODIFICATION > 31500
#define getAddr(pciu) pciu->dbch->addr
#else
#define getAddr(pciu) pciu->addr
#endif

int calDebug = 0;
#ifndef EPICS_3_13
epicsExportAddress(int, calDebug);
#endif

long cal(const char* match, int level)
{
#ifndef _WIN32
    struct client *client;
    int matchfield;
#define MAX_FIELD_NAME_LENGTH 10
    char fullname[PVNAME_STRINGSZ+MAX_FIELD_NAME_LENGTH+1];
    char clientref[100];

    if (match && !*match) match= NULL;
    matchfield = match && strchr(match, '.');

    LOCK_CLIENTQ
    for (client = (struct client *)ellNext(&clientQ.node); client; client = (struct client *)ellNext(&client->node))
    {
        struct channel_in_use *pciu;
        int n = 0;
        if (calDebug) fprintf(stderr, "host: %s\nuser: %s\n", client->pHostName, client->pUserName);
        if (level >= 2) {
            n = sprintf(clientref, "V%u.%u %s:",
                CA_MAJOR_PROTOCOL_REVISION,
                client->minor_version_number,
                client->proto == IPPROTO_UDP ? "UDP" :
                client->proto == IPPROTO_TCP ? "TCP" : "UKN");
        }
        n += sprintf(clientref + n, "%.36s@", client->pUserName ? client->pUserName : "?");
        if (client->pHostName)
        {
            sprintf(clientref + n, "%.*s:%i",
                (int)strcspn(client->pHostName, "."),
                client->pHostName, ntohs(client->addr.sin_port));
        }
        else
        {
            ipAddrToA(&client->addr, clientref + n, sizeof(clientref) - n);
            if (clientref[n] > '9')
                sprintf(clientref + n + strcspn(clientref + n, "."), ":%i",
                    ntohs(client->addr.sin_port));
        }
        if (calDebug) fprintf(stderr, "clientref: %s\n", clientref);
        epicsMutexMustLock(client->chanListLock);
        for (pciu = (struct channel_in_use *) ellFirst(&client->chanList); pciu;
                                pciu = (struct channel_in_use *)ellNext(&pciu->node))
        {
            const char* recname = getAddr(pciu).precord->name;
            if (calDebug) fprintf(stderr, "channel: %s\n", recname);
            if (!recname) continue;
            sprintf(fullname, "%.*s.%.*s",
                PVNAME_STRINGSZ, recname,
                MAX_FIELD_NAME_LENGTH, ((struct dbFldDes*)getAddr(pciu).pfldDes)->name);
            if (calDebug) fprintf(stderr, "fullname: %s\n", fullname);
            if (!match || epicsStrGlobMatch(matchfield ? fullname : recname, match)
                || (client->pUserName && epicsStrGlobMatch(client->pUserName, match))
                || (client->pHostName && epicsStrGlobMatch(client->pHostName, match))
                || epicsStrGlobMatch(clientref, match))
            {
                printf("%s%s %s==> %s\n",
#ifndef EPICS_3_13
                    level < 3 ? "" :
                        pciu->state == rsrvCS_invalid ? "[invalid]" :
                        pciu->state == rsrvCS_pendConnectResp ? "[connect]" :
                        pciu->state == rsrvCS_inService ? "[active]" :
                        pciu->state == rsrvCS_pendConnectRespUpdatePendAR ? "[connectAR]" :
                        pciu->state == rsrvCS_inServiceUpdatePendAR ? "[activeAR]" :
                        pciu->state == rsrvCS_shutdown ? "[shutdown]" : "[unknown]",
#else
                    "",
#endif
                    clientref,
                    level < 1 ? "" :
                        asCheckPut(pciu->asClientPVT) ?
                            pciu->asClientPVT->trapMask ? "w" : "w" :
                        asCheckGet(pciu->asClientPVT) ? "r" : "n",
                    fullname
                );
            }
        }
        epicsMutexUnlock(client->chanListLock);
    }
    UNLOCK_CLIENTQ
#endif
    return 0;
}

#ifndef EPICS_3_13
static const iocshFuncDef calDef =
    { "cal", 2, (const iocshArg *[]) {
    &(iocshArg) { "record name pattern", iocshArgString },
    &(iocshArg) { "level", iocshArgInt },
}};

void calFunc(const iocshArgBuf *args)
{
    cal(args[0].sval, args[1].ival);
}

static void calRegistrar(void)
{
    iocshRegister(&calDef, calFunc);
}

epicsExportRegistrar(calRegistrar);
#endif
