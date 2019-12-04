/* bootNotify.c
*
*  call a script on the boot PC and give it a lot of boot infos
*
* Copyright (C) 2004 Dirk Zimoch
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
#include <version.h>
#include "bootInfo.h"
#include "rsh.h"
#include "epicsVersion.h"

int bootNotify (char* script, char* script2)
{
    char command[256];
    char epicsver[15];
    char* vxver = vxWorksVersion;

    if (script == NULL)
    {
        printErr("usage: bootNotify [\"<path>\",] \"<script>\"\n");
        return ERROR;
    }
    if (script2)
    {
        sprintf(command, "%s/%s", script, script2);
    }
    else
    {
        sprintf(command, "%s", script);
    }
    /* strip off any leading non-numbers */
    while (vxver && (*vxver < '0' || *vxver > '9')) vxver++;

    sprintf(epicsver, "%d.%d.%d",
        EPICS_VERSION, EPICS_REVISION, EPICS_MODIFICATION);
    return rsh(bootHost(), command, bootInfo("%T %e %n %d %F %s"),
        vxver, epicsver, etherAddr(ifName()), 0);
}
