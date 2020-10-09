#include <string.h>
#include <stdlib.h>

#include "envDefs.h"
#include "epicsEnvUnset.h"

#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
#include <stdio.h>
#include <envLib.h>
#include <shellLib.h>
#else
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"
#endif

#define NOT      1
#define GREATER  2
#define LESS     4
#define EQUAL    8
#define MATCH   16

static int match(const char *str, const char *pattern)
{
    const char *cp = NULL, *mp = NULL;

    while ((*str) && (*pattern != '*')) {
        if ((*pattern != *str) && (*pattern != '?'))
            return 0;
        pattern++;
        str++;
    }
    while (*str) {
        if (*pattern == '*') {
            if (!*++pattern)
                return 1;
            mp = pattern;
            cp = str+1;
        }
        else if ((*pattern == *str) || (*pattern == '?')) {
            pattern++;
            str++;
        }
        else {
            pattern = mp;
            str = cp++;
        }
    }
    while (*pattern == '*')
        pattern++;
    return !*pattern;
}

#ifdef EPICS_3_13
static void epicsEnvSet(const char* variable, const char* value)
{
    char* e;
    e = malloc(strlen(variable)+strlen(value)+2);
    if (!e) {
        fprintf(stderr, "ifEnvSet: out of memory\n");
        shellScriptAbort();
        return;
    }
    sprintf(e, "%s=%s", variable, value);
    putenv(e);
    free(e);
}
#endif

int ifEnvSet(const char* arg, const char* condition, const char *variable, const char* value, const char* elsevalue)
{
    int op = 0;
    int result = 0;
    long num_condition, num_arg;
    char* e;
    if (!arg || !condition) {
        printf ("usage: ifEnvSet \"argument\", \"condition\", \"variable\", \"value\", \"elsevalue\"\n");
        printf ("    If \"argument\" matches \"condition\", then set \"variable\" to \"value\"\n");
        printf ("        (if no \"value\" is given, then unset \"variable\")\n");
        printf ("    else set \"variable\" to \"elsevalue\"\n");
        printf ("        (if no \"elsevalue\" is given, then do nothing).\n");
        printf ("The condition consists of operators and argument with:\n");
        printf ("Operator ! negates the result\n");
        printf ("Operators < = > (or combinations) compare with numerical argument\n");
        printf ("Operator ~ matches against the argument glob pattern\n");
        printf ("No operator means to check for string equality with argument\n");
#ifdef vxWorks
        printf ("Returns 1 (true) or 0 (false) even if no variable is given\n");
#endif
        printf ("Examples:\n");
#ifdef vxWorks
        printf ("ifEnvSet getenv(\"IP_ADDR\"), \"~129.129.*\", \"EPICS_CA_ADDR_LIST\", \"129.129.130.255\"\n");
#else 
        printf ("ifEnvSet \"$(IP_ADDR)\", \"~129.129.*\", \"EPICS_CA_ADDR_LIST\", \"129.129.130.255\"\n");
#endif
        return 0;
    }
    while (*condition == '!') { op ^= NOT; condition++; }
    if (*condition == '~') { op |= MATCH; condition++; }
    else do {
        switch (*condition) {
            case '>': op |= GREATER; break;
            case '<': op |= LESS; break;
            case '=': op |= EQUAL; break;
            default: goto cont;
        }
        condition++;
    } while (1);
cont:
    if (op & MATCH) {
        result |= match(arg, condition);
    }
    num_condition = strtol(condition, &e, 0);
    if (*condition || *e == 0) {
        num_arg = strtol(arg, NULL, 0);
        /* argument is a number */
        if (op & EQUAL) {
            result |= (num_arg == num_condition);
        }
        if (op & LESS) {
            result |= (num_arg < num_condition);
        }
        if (op & GREATER) {
            result |= (num_arg > num_condition);
        }
    } else {
        /* argument is not a number */
        if (op & (GREATER | LESS)) {
            fprintf(stderr, "ifEnvSet: < and > require numeric argument\n");
            return 0;
        }
        /* must be string equal */
        result |= (strcmp(arg, condition) == 0);
    }
    if (op & NOT) {
        result = !result;
    }
    if (!result) value = elsevalue;
    if (variable && value) {
        epicsEnvSet(variable, value);
        printf("%s=%s\n", variable, value);
    } else if (result && variable && !value) {
        epicsEnvUnset(variable);
        printf("epicsEnvUnset %s\n", variable);
    }
    return result;
}

#ifndef EPICS_3_13

static const iocshArg iesArg0 = { "argument", iocshArgString };
static const iocshArg iesArg1 = { "condition", iocshArgString };
static const iocshArg iesArg2 = { "variable", iocshArgString };
static const iocshArg iesArg3 = { "value", iocshArgString };
static const iocshArg iesArg4 = { "[else_value]", iocshArgString };
static const iocshArg * const iesArgs[] = { &iesArg0, &iesArg1, &iesArg2, &iesArg3, &iesArg4};
static const iocshFuncDef iesDef = { "ifEnvSet", sizeof(iesArgs)/sizeof(iesArgs[0]), iesArgs };

void iesFunc (const iocshArgBuf *args)
{
    ifEnvSet(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].sval);
}

static void
ifEnvSetRegister(void)
{
    iocshRegister (&iesDef, iesFunc);
}

epicsExportRegistrar(ifEnvSetRegister);

#endif
