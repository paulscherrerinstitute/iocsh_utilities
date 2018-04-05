/* bootNotify.c
*
*  call a script on the boot PC and give it a lot of boot infos
*
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
