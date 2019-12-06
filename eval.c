/* eval.c
*
*  evaluate (i.e. execute) the output of a command as a new command
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

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsString.h"
#include "epicsExport.h"

int evalDebug = 1;
epicsExportAddress(int, evalDebug);

static void evalFunc (const iocshArgBuf *args)
{
    char commandline[1024];
    char* p = commandline;
    unsigned int i;
    FILE* orig_stdout;
    FILE* bufferfile;
    char* buffer;
    size_t buffersize;
    size_t len;
    char *arg;

    for (i = 0; i < args[1].aval.ac; i++) {
        arg = args[1].aval.av[i];
        len = strlen(arg);

        if (p - commandline + len + 4 >= sizeof(commandline))
        {
            fprintf(stderr, "command line too long\n");
            return;
        }
        *p++ = '\'';
        p += epicsStrSnPrintEscaped(p, sizeof(commandline)-(p-commandline)-3, arg, len);
        *p++ = '\'';
        *p++ = ' ';
        //p += sprintf(p, " \"%.*s\"", (int)len, arg);
    }
    *p = 0;
    bufferfile = open_memstream(&buffer, &buffersize);
    orig_stdout = epicsGetThreadStdout();
    epicsSetThreadStdout(bufferfile);
    if (evalDebug) fprintf(stderr, "iocshCmd:(%s)\n", commandline);
    iocshCmd(commandline);
    fclose(bufferfile);
    while (isspace(buffer[buffersize-1])) buffer[--buffersize] = 0;
    if (evalDebug) { fprintf(stderr, "result:"); epicsStrPrintEscaped(stderr, buffer, buffersize); fprintf(stderr, "\n");}
    epicsSetThreadStdout(orig_stdout);
    iocshCmd(buffer);
    free(buffer);
}

static const iocshArg evalArg0 = { "command", iocshArgString };
static const iocshArg evalArg1 = { "arguments", iocshArgArgv };
static const iocshArg * const evalArgs[] = { &evalArg0, &evalArg1 };
static const iocshFuncDef evalDef = { "eval", 2, evalArgs };

static void evalRegister(void)
{
    if (evalDebug) fprintf(stderr, "registering 'eval' command\n");
    iocshRegister (&evalDef, evalFunc);
}
epicsExportRegistrar(evalRegister);
#endif
