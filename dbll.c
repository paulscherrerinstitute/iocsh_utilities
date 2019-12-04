/* dbll.c
*
*  list links pointing to a record
*
* Copyright (C) 2017 Dirk Zimoch
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

#include <string.h>
#include <stdlib.h>
#include "dbStaticLib.h"
#include "epicsString.h"
#include "epicsVersion.h"
#ifdef BASE_VERSION
#define EPICS_3_13
extern int epicsStrGlobMatch(const char *str, const char *pattern);
extern struct dbBase *pdbbase;
#else
#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "dbAddr.h"
#include "dbAccessDefs.h"
#include "epicsExport.h"
#endif

long dbll(const char* match, const char* types)
{
    DBENTRY dbEntry;
    long status = 0;
    char *matchfield = NULL;
    char *alt_match = NULL;
    const char* symbol;
    const int imask = 1;
    const int omask = 2;
    const int fmask = 4;
    const int pmask = 8;
    const int cmask = 16;
    const int dmask = 32;
    unsigned int typemask;

    if (match)
    {
        if (!*match || strcmp(match, "*") == 0)
        {
            match = NULL;
        }
        else
        {
            matchfield = strchr(match, '.');
            if (!matchfield)
            {
                alt_match = malloc(strlen(match)+3);
                sprintf(alt_match, "%s.*", match);
            }
            else if (strcmp(matchfield, ".VAL") == 0 || strcmp(matchfield, ".*") == 0)
            {
                alt_match = malloc(matchfield-match+1);
                sprintf(alt_match, "%.*s", (int)(matchfield-match), match);
            }
        }
    }

    if (types && *types)
    {
        typemask = 0;
        while (*types)
        {
            switch (*types++)
            {
                case 'o':
                case 'O':
                    typemask |= omask;
                    break;
                case 'i':
                case 'I':
                    typemask |= imask;
                    break;
                case 'f':
                case 'F':
                    typemask |= fmask;
                    break;
                case 'p':
                case 'P':
                    typemask |= pmask;
                    break;
                case 'c':
                case 'C':
                    typemask |= cmask;
                    break;
                case 'd':
                case 'D':
                    typemask |= dmask;
                    break;
           }
        }
        if (!(typemask & (omask|imask|fmask))) typemask |= (omask|imask|fmask);
        if (!(typemask & (cmask|dmask))) typemask |= (cmask|dmask);
    }
    else
    {
        typemask = ~pmask;
    }

    dbInitEntry(pdbbase, &dbEntry);
    for (status = dbFirstRecordType(&dbEntry); !status; status = dbNextRecordType(&dbEntry))
    for (status = dbFirstRecord(&dbEntry); !status; status = dbNextRecord(&dbEntry))
    {
        int ilink;

        #ifdef DBRN_FLAGS_ISALIAS
        if (dbIsAlias(&dbEntry)) continue;
        #endif

        for (ilink = 0; dbGetLinkField(&dbEntry, ilink) == 0; ilink++)
        {
            DBLINK *link = (DBLINK *)dbEntry.pfield;
            const char* target = link->value.pv_link.pvname;
            if (!target) continue;
            switch (link->type)
            {
                default:
                    continue;
                #ifdef PN_LINK
                case PN_LINK:
                #endif
                case PV_LINK:
                case DB_LINK:
                    if (!(typemask & dmask)) continue;
                    break;
                case CA_LINK:
                    if (!(typemask & cmask)) continue;
                    break;
            }
            switch (dbEntry.pflddes->field_type)
            {
                default:
                    continue;
                case DBF_INLINK:
                    if (!(typemask & imask)) continue;
                    if (link->value.pv_link.pvlMask & pvlOptPP
                        #ifdef PN_LINK
                        || link->type == PN_LINK
                        #endif
                        )
                        symbol = "<=p";
                    else
                    {
                        if (typemask & pmask) continue;
                        symbol = "<==";
                    }
                    break;
                case DBF_OUTLINK:
                    if (!(typemask & omask)) continue;
                    if (link->value.pv_link.pvlMask & pvlOptPP)
                    symbol = "p=>";
                    else
                    {
                        if (typemask & pmask) continue;
                        symbol = "==>";
                    }
                    break;
                case DBF_FWDLINK:
                    if (!(typemask & fmask)) continue;
                    symbol = "p->";
                    break;
            }
            if (!match || (!strchr(target, '.') == !matchfield ? epicsStrGlobMatch(target, match)
                : alt_match ? epicsStrGlobMatch(target, alt_match) : 0))
            {
                printf("%s.%s %s %s\n", dbGetRecordName(&dbEntry), dbGetFieldName(&dbEntry),
                    symbol, dbGetString(&dbEntry));
            }
        }
    }
    dbFinishEntry(&dbEntry);   
    free(alt_match);
    return 0;
}

#ifndef EPICS_3_13
static const iocshFuncDef dbllDef =
    { "dbll", 2, (const iocshArg *[]) {
    &(iocshArg) { "record name pattern", iocshArgString },
    &(iocshArg) { "link type filter [iofcdp]", iocshArgString },
}};

/*
    dbll: List links (that point to given records)

    Record name pattern:
        Glob pattern of target records or fields, e.g.
            recordname : show all links to record
            recordname.VAL : show only links to VAL field
            *.VAL : show links to VAL fiels of all records
            *.*ST : show links to fields that end with ST

    Link type filters:
        i : show only input links
        o : show only output links
        f : show only forward links
        c : show only channel access links
        d : show only database links
        p : show only links that make the target process      

    Example: Show all input or output PP links:
        dbll * iop

    Link symbols:
        Processing links: p
        Output links:  ==>
        Input links:   <==
        Forward links: P->
*/

void dbllFunc(const iocshArgBuf *args)
{
    dbll(args[0].sval, args[1].sval);
}

static void dbllRegistrar(void)
{
    iocshRegister(&dbllDef, dbllFunc);
}

epicsExportRegistrar(dbllRegistrar);
#endif
