/* disctools.c
*
*  provide several file system functions in the ioc shell
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

#define _FILE_OFFSET_BITS 64

#ifdef UNIX
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <glob.h>
#include <errno.h>
#endif

#include "iocsh.h"
#include "epicsStdioRedirect.h"
#include "epicsExport.h"

#ifdef UNIX

/* dir, ll, ls */
static const iocshArg * const dirArgs[1] = {
    &(iocshArg){ "files/direcories", iocshArgArgv }
};
static const iocshFuncDef dirDef = { "dir", 1, dirArgs };
static const iocshFuncDef llDef = { "ll", 1, dirArgs };
static const iocshFuncDef lsDef = { "ls", 1, dirArgs };

static int nohidden(const struct dirent *entry)
{
    return entry->d_name[0] != '.';
}


void llOut(const char* dirname, const char* filename)
{
    struct stat filestat;
    struct tm time;
    struct group* group;
    struct passwd* user;
    char target[256];
    char timestr[20];
    char fullname[256];
    char type;

    if (dirname)
        snprintf(fullname, sizeof(fullname), "%s/%s", dirname, filename);
    else
        snprintf(fullname, sizeof(fullname), "%s", filename);
    if (lstat(fullname, &filestat) == 0)
    {
        if (S_ISREG(filestat.st_mode)) type='-';
        else if (S_ISDIR(filestat.st_mode)) type='d';
        else if (S_ISCHR(filestat.st_mode)) type='c';
        else if (S_ISBLK(filestat.st_mode)) type='b';
        else if (S_ISFIFO(filestat.st_mode)) type='p';
        else if (S_ISLNK(filestat.st_mode)) type='l';
        else if (S_ISSOCK(filestat.st_mode)) type='s';
        else type='?';
        printf("%c%c%c%c%c%c%c%c%c%c %4llu",
            type,
            filestat.st_mode & S_IRUSR ? 'r' : '-',
            filestat.st_mode & S_IWUSR ? 'w' : '-',
            filestat.st_mode & S_ISUID ? 's' :
            filestat.st_mode & S_IXUSR ? 'x' : '-',
            filestat.st_mode & S_IRGRP ? 'r' : '-',
            filestat.st_mode & S_IWGRP ? 'w' : '-',
            filestat.st_mode & S_ISGID ? 's' :
            filestat.st_mode & S_IXGRP ? 'x' : '-',
            filestat.st_mode & S_IROTH ? 'r' : '-',
            filestat.st_mode & S_IWOTH ? 'w' : '-',
            filestat.st_mode & S_ISVTX ? 't' :
            filestat.st_mode & S_IXOTH ? 'x' : '-',
            (unsigned long long) filestat.st_nlink);
        user=getpwuid(filestat.st_uid);
        if (user) printf(" %-8s", user->pw_name);
        else printf(" %-8d", filestat.st_uid);
        group=getgrgid(filestat.st_gid);
        if (group) printf(" %-8s", group->gr_name);
        else printf(" %-8d", filestat.st_gid);
        localtime_r(&filestat.st_mtime, &time);
        strftime(timestr, 20, "%b %e %Y %H:%M", &time);
        if (S_ISCHR(filestat.st_mode) || S_ISBLK(filestat.st_mode))
            printf (" %3d, %3d", major(filestat.st_rdev), minor(filestat.st_rdev));
        else
            printf (" %8lld", (unsigned long long) filestat.st_size);
        printf (" %s %s", timestr, filename);
        if (S_ISLNK(filestat.st_mode))
        {
            ssize_t len;
            len = readlink(fullname, target, 255);
            if (len == -1) perror(filename);
            else
            {
                target[len] = 0;
                printf(" -> %s\n", target);
            }
        }
        else
        {
            printf("\n");
        }
    }
    else
    {
        if (errno == EACCES)
        {
            printf("??????????    ? ?        ?               ?           ? %s\n", filename);
        }
        else
        {
            perror(filename);
        }
    }
}

#ifndef GLOB_BRACE
#define GLOB_BRACE 0
#endif
#ifndef GLOB_TILDE_CHECK
#define GLOB_TILDE_CHECK 0
#endif

static void dirFunc(const iocshArgBuf *args)
{
    struct stat filestat;
    glob_t globinfo;
    const char* filename;
    size_t i, numfiles=0, offs=0;
    int len, maxlen=0;
    int longformat = 1;
    int rows, cols, r, c;
    int width = 80;

    if (strcmp(args[0].aval.av[0], "ls") == 0)
    {
        struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);
        if (w.ws_col) width = w.ws_col;
        longformat = 0;
    }

    if (args[0].aval.ac == 1)
    {
        if (glob("./", 0, NULL, &globinfo) != 0)
        {
            perror(".");
            return;
        }
    }
    else for (i = 1; i < (size_t)args[0].aval.ac; i++)
    {
        const char* arg;
        int status;
        arg = args[0].aval.av[i];
        errno = 0;

        /* a few problems with glob:
            1. broken links matching wildcards are missing
            2. trailing / does not only match directories
        */
        int dironly = arg[strlen(arg)-1] == '/';
        status = glob(arg, (i>1 ? GLOB_APPEND : 0)
            | (dironly ? GLOB_ONLYDIR : 0)
            | GLOB_NOCHECK | GLOB_BRACE | GLOB_TILDE_CHECK,
            NULL, &globinfo);
        if (status == 0 && dironly)
        {
            /* if we are looking for dirs, remove all non-dirs */
            int found=0;
            while (offs < globinfo.gl_pathc)
            {
                len = strlen(globinfo.gl_pathv[offs])-1;
                if (globinfo.gl_pathv[offs][len] != '/')
                    globinfo.gl_pathv[offs][0] = 0;
                else found=1;
                offs++;
            }
            if (!found) status = GLOB_NOMATCH;
        }
        if (status != 0)
        {
            if (status == GLOB_NOMATCH)
                errno = ENOENT;
            if (errno) perror(arg);
        }
    }
    for (i = 0; i < globinfo.gl_pathc; i++)
    {
        filename = globinfo.gl_pathv[i];
        if (filename[0] == 0) continue;

        /* do directories later */
        if (lstat(filename, &filestat) || S_ISDIR(filestat.st_mode))
            continue;

        numfiles++;
        if (longformat)
        {
            llOut(NULL, filename);
            globinfo.gl_pathv[i][0] = 0; /* mark as done */
            continue;
        }
        len =  strlen(filename);
        if (len > maxlen) maxlen = len;
    }
    if (!longformat && maxlen)
    {
        cols = width/(maxlen+=2);
        if (cols == 0)
        {
            cols = 1;
            maxlen = 0;
        }
        rows = (numfiles-1)/cols+1;
        for (r = 0; r < rows; r++)
        {
            for (c = 0; c < cols; c++)
            {
                i = r + c*rows;
                if (i >= numfiles) continue;
                printf("%-*s", maxlen, globinfo.gl_pathv[i]);
                globinfo.gl_pathv[i][0] = 0; /* mark as done */
            }
            printf("\n");
        }
    }
    for (i = 0; i < globinfo.gl_pathc; i++)
    {
        struct dirent** namelist;
        int direntries;
        int j;
        maxlen = 0;

        filename = globinfo.gl_pathv[i];
        if (filename[0] == 0) /* skip files marked before */
            continue;
        len =  strlen(filename);

        if (globinfo.gl_pathc > 1)
        {
            if (numfiles > 0) printf("\n");
            printf("%s:\n", filename);
        }
        direntries = scandir(filename, &namelist, nohidden,
    #ifdef _GNU_SOURCE
            versionsort
    #else
            alphasort
    #endif
        );
        if (direntries < 0)
        {
            perror(filename);
            continue;
        }
        for (j = 0; j < direntries; j++)
        {
            if (longformat)
            {
                llOut(filename, namelist[j]->d_name);
                free(namelist[j]);
                continue;
            }
            len = strlen(namelist[j]->d_name);
            if (len > maxlen) maxlen = len;
        }
        if (!longformat && maxlen)
        {
            cols = width/(maxlen+=2);
            if (cols == 0)
            {
                cols = 1;
                maxlen = 0;
            }
            rows = (direntries-1)/cols+1;
            for (r = 0; r < rows; r++)
            {
                for (c = 0; c < cols; c++)
                {
                    j = r + c*rows;
                    if (j >= direntries) continue;
                    printf("%-*s",
                        maxlen, namelist[j]->d_name);
                    free(namelist[j]);
                }
                printf("\n");
            }
        }
        free(namelist);
    }
    globfree(&globinfo);
}

/* mkdir */
static const iocshArg mkdirArg0 = { "directrory", iocshArgString };
static const iocshArg * const mkdirArgs[1] = { &mkdirArg0 };
static const iocshFuncDef mkdirDef = { "mkdir", 1, mkdirArgs };

static void mkdirFunc(const iocshArgBuf *args)
{
    char* dirname;

    dirname = args[0].sval;
    if (!dirname)
    {
        fprintf(stderr, "missing directory name\n");
        return;
    }
    if (mkdir(dirname, 0777))
    {
        perror(dirname);
    }
}

/* rmdir */
static const iocshArg rmdirArg0 = { "directrory", iocshArgString };
static const iocshArg * const rmdirArgs[1] = { &rmdirArg0 };
static const iocshFuncDef rmdirDef = { "rmdir", 1, rmdirArgs };

static void rmdirFunc(const iocshArgBuf *args)
{
    char* dirname;

    dirname = args[0].sval;
    if (!dirname)
    {
        fprintf(stderr, "missing directory name\n");
        return;
    }
    if (rmdir(dirname))
    {
        perror(dirname);
    }
}

/* rm */
static const iocshArg rmArg0 = { "[-rf] files", iocshArgArgv};
static const iocshArg * const rmArgs[1] = { &rmArg0 };
static const iocshFuncDef rmDef = { "rm", 1, rmArgs };

static int globerr (const char *epath, int eerrno)
{
    fprintf(stderr, "rm: %s %s\n", epath, strerror(eerrno));
    return 0;
}

struct rmflags_t {
    unsigned int r : 1;
    unsigned int f : 1;
    unsigned int endflags : 1;
};

static void rmPriv(char* pattern, struct rmflags_t flags)
{
    char* filename;
    size_t j;
    glob_t globresult;

    glob(pattern, GLOB_NOSORT|GLOB_NOMAGIC
#ifdef  GLOB_BRACE
            |GLOB_BRACE|GLOB_TILDE
#endif
            ,globerr, &globresult);
    for (j = 0; j < globresult.gl_pathc; j++)
    {
        filename = globresult.gl_pathv[j];
        if (unlink(filename))
        {
            if (errno == ENOENT && flags.f) continue;
            if (errno == EISDIR && flags.r)
            {
                char* p=malloc(strlen(filename)+3);
                if (p)
                {
                    sprintf(p, "%s/*", filename);
                    rmPriv(p, flags);
                    free(p);
                    if (rmdir(filename) == 0) continue;
                }
            }
            perror(filename);
        }
    }
    globfree(&globresult);
}

static void rmFunc(const iocshArgBuf *args)
{
    char *arg;
    int i,j;
    struct rmflags_t flags = {0};

    for (i = 1; i < args[0].aval.ac; i++)
    {
        arg = args[0].aval.av[i];
        if (arg[0] != '-') continue;
        if (arg[1] == '-' && arg[2] == 0) break;
        for (j = 1; arg[j]; j++)
        {
            switch (arg[j])
            {
                case 'r': flags.r = 1; break;
                case 'f': flags.f = 1; break;
                default:
                    fprintf(stderr, "rm: ignoring unknown option -%c\n", arg[j]);
            }
        }
    }
    for (i = 1; i < args[0].aval.ac; i++)
    {
        arg = args[0].aval.av[i];
        if (!flags.endflags && arg[0] == '-')
        {
            if (arg[1] == '-' && arg[2] == 0)
                flags.endflags = 1;
            continue;
        }
        rmPriv(arg, flags);
    }
}

/* mv */
static const iocshArg mvArg0 = { "oldname", iocshArgString };
static const iocshArg mvArg1 = { "newname", iocshArgString };
static const iocshArg * const mvArgs[2] = { &mvArg0, &mvArg1 };
static const iocshFuncDef mvDef = { "mv", 2, mvArgs };

static void mvFunc(const iocshArgBuf *args)
{
    char* oldname;
    char* newname;
    struct stat filestat;
    char filename[256];

    oldname = args[0].sval;
    newname = args[1].sval;
    if (!oldname || !newname)
    {
        fprintf(stderr, "need 2 file names\n");
        return;
    }
    if (!stat(newname, &filestat) && S_ISDIR(filestat.st_mode))
    {
        snprintf(filename, sizeof(filename), "%s/%s", newname, oldname);
        newname = filename;
    }
    if (rename(oldname, newname))
    {
        perror("mv");
    }
}

/* cp, copy */
static const iocshArg cpArg0 = { "source", iocshArgString };
static const iocshArg cpArg1 = { "target", iocshArgString };
static const iocshArg * const cpArgs[2] = { &cpArg0, &cpArg1 };
static const iocshFuncDef cpDef = { "cp", 2, cpArgs };
static const iocshFuncDef copyDef = { "copy", 2, cpArgs };

static void cpFunc(const iocshArgBuf *args)
{
    char buffer [256];
    char* sourcename;
    char* targetname;
    FILE* sourcefile;
    FILE* targetfile;
    size_t len;

    sourcename = args[0].sval;
    targetname = args[1].sval;
    if (sourcename == NULL || sourcename[0] == '\0')
        sourcefile = stdin;
    else if (!(sourcefile = fopen(sourcename,"r")))
    {
        perror(sourcename);
        return;
    }
    if (targetname == NULL || targetname[0] == '\0')
        targetfile = stdout;
    else if (!(targetfile = fopen(targetname,"w")))
    {
        perror(targetname);
        return;
    }
    while (!feof(sourcefile))
    {
        len = fread(buffer, 1, 256, sourcefile);
        if (ferror(sourcefile))
        {
            perror(sourcename);
            break;
        }
        fwrite(buffer, 1, len, targetfile);
        if (ferror(targetfile))
        {
            perror(targetname);
            break;
        }
    }
    if (sourcefile != stdin)
        fclose(sourcefile);
    if (targetfile != stdout)
        fclose(targetfile);
    else
        fflush(stdout);
}

/* umask */
static const iocshArg umaskArg0 = { "mask", iocshArgString };
static const iocshArg * const umaskArgs[1] = { &umaskArg0 };
static const iocshFuncDef umaskDef = { "umask", 1, umaskArgs };

static void umaskFunc(const iocshArgBuf *args)
{
    mode_t mask;
    if (args[0].sval == NULL)
    {
        mask = umask(0);
        umask(mask);
        printf("%03o\n", (int)mask);
        return;
    }
    if (sscanf(args[0].sval, "%o", &mask) == 1)
    {
        umask(mask);
        return;
    }
    fprintf(stderr, "mode %s not recognized\n", args[0].sval);
}

/* chmod */
static const iocshArg chmodArg0 = { "mode", iocshArgInt };
static const iocshArg chmodArg1 = { "file", iocshArgString };
static const iocshArg * const chmodArgs[2] = { &chmodArg0, &chmodArg1 };
static const iocshFuncDef chmodDef = { "chmod", 2, chmodArgs };

static void chmodFunc(const iocshArgBuf *args)
{
    mode_t mode = args[0].ival;
    char* path = args[1].sval;
    if (chmod(path, mode) != 0)
    {
        perror(path);
    }
}
#endif

static void
disctoolsRegister(void)
{
#ifdef UNIX
    iocshRegister(&dirDef, dirFunc);
    iocshRegister(&llDef, dirFunc);
    iocshRegister(&lsDef, dirFunc);
    iocshRegister(&mkdirDef, mkdirFunc);
    iocshRegister(&rmdirDef, rmdirFunc);
    iocshRegister(&rmDef, rmFunc);
    iocshRegister(&mvDef, mvFunc);
    iocshRegister(&cpDef, cpFunc);
    iocshRegister(&copyDef, cpFunc);
    iocshRegister(&umaskDef, umaskFunc);
    iocshRegister(&chmodDef, chmodFunc);
#endif
}

epicsExportRegistrar(disctoolsRegister);
