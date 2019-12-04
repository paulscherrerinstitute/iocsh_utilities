/* glob.c
*
* This is an unmodified copy of the epicsStrGlobMatch EPICS function to make it
* available for older EPICS base versions that did not provide it.
* The original epicsStrGlobMatch function is published under the EPICS Open License
* which can be found here: https://epics.anl.gov/license/open.php
* The original contained the following copyright and authorship notices:
*/
/*************************************************************************\
* Copyright (c) 2009 Helmholtz-Zentrum Berlin fuer Materialien und Energie.
* Copyright (c) 2009 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

/* Authors: Jun-ichi Odagiri, Marty Kraimer, Eric Norum,
 *          Mark Rivers, Andrew Johnson, Ralph Lange
 */

int epicsStrGlobMatch(
    const char *str, const char *pattern)
{
    const char *cp=NULL, *mp=NULL;

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
