/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * pio.c -- Packaged I/O routines.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: pio.c,v $
 * Revision 1.2  2005/06/22 03:09:52  arthchan2003
 * 1, Fixed doxygen documentation, 2, Added  keyword.
 *
 * Revision 1.3  2005/06/16 00:14:08  archan
 * Added const keyword to file argument for file_open
 *
 * Revision 1.2  2005/06/15 06:23:21  archan
 * change headers from io.h to pio.h
 *
 * Revision 1.1  2005/06/15 06:11:03  archan
 * sphinx3 to s3.generic: change io.[ch] to pio.[ch]
 *
 * Revision 1.4  2005/04/20 03:49:32  archan
 * Add const to string argument of myfopen.
 *
 * Revision 1.3  2005/03/30 01:22:48  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 08-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added stat_mtime().
 * 
 * 11-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added _myfopen().
 * 
 * 05-Sep-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !(defined(WIN32) || defined(_WIN32_WCE))
#include <unistd.h>
#endif
#include <assert.h>

#include "pio.h"
#include "err.h"


FILE *
fopen_comp(const char *file, char *mode, int32 * ispipe)
{
    char command[16384];
    FILE *fp;
    int32 k, isgz;

    k = strlen(file);

#if (WIN32)
    *ispipe = (k > 3) && ((strcmp(file + k - 3, ".gz") == 0)
                          || (strcmp(file + k - 3, ".GZ") == 0));
    isgz = *ispipe;
#else
    *ispipe = 0;
    isgz = 0;
    if ((k > 2)
        && ((strcmp(file + k - 2, ".Z") == 0)
            || (strcmp(file + k - 2, ".z") == 0))) {
        *ispipe = 1;
    }
    else {
        if ((k > 3) && ((strcmp(file + k - 3, ".gz") == 0)
                        || (strcmp(file + k - 3, ".GZ") == 0))) {
            *ispipe = 1;
            isgz = 1;
        }
    }
#endif

    if (*ispipe) {
#if (WIN32)
        if (strcmp(mode, "r") == 0) {
            sprintf(command, "gzip.exe -d -c %s", file);
            if ((fp = _popen(command, mode)) == NULL) {
                E_ERROR_SYSTEM("_popen (%s,%s) failed\n", command, mode);
                return NULL;
            }
        }
        else if (strcmp(mode, "w") == 0) {
            sprintf(command, "gzip.exe > %s", file);

            if ((fp = _popen(command, mode)) == NULL) {
                E_ERROR_SYSTEM("_popen (%s,%s) failed\n", command, mode);
                return NULL;
            }
        }
        else {
            E_ERROR("fopen_comp not implemented for mode = %s\n", mode);
            return NULL;
        }
#else
        if (strcmp(mode, "r") == 0) {
            if (isgz)
                sprintf(command, "gunzip -c %s", file);
            else
                sprintf(command, "zcat %s", file);

            if ((fp = popen(command, mode)) == NULL) {
                E_ERROR_SYSTEM("popen (%s,%s) failed\n", command, mode);
                return NULL;
            }
        }
        else if (strcmp(mode, "w") == 0) {
            if (isgz)
                sprintf(command, "gzip > %s", file);
            else
                sprintf(command, "compress -c > %s", file);

            if ((fp = popen(command, mode)) == NULL) {
                E_ERROR_SYSTEM("popen (%s,%s) failed\n", command, mode);
                return NULL;
            }
        }
        else {
            E_ERROR("fopen_comp not implemented for mode = %s\n", mode);
            return NULL;
        }
#endif
    }
    else {
        fp = fopen(file, mode);
    }

    return (fp);
}


void
fclose_comp(FILE * fp, int32 ispipe)
{
    if (ispipe) {
#if (WIN32)
        _pclose(fp);
#else
        pclose(fp);
#endif
    }
    else
        fclose(fp);
}


FILE *
fopen_compchk(char *file, int32 * ispipe)
{
    char tmpfile[16384];
    int32 k, isgz;
    struct stat statbuf;

    k = strlen(file);

#if (WIN32)
    *ispipe = (k > 3) && ((strcmp(file + k - 3, ".gz") == 0)
                          || (strcmp(file + k - 3, ".GZ") == 0));
    isgz = *ispipe;
#else
    *ispipe = 0;
    isgz = 0;
    if ((k > 2)
        && ((strcmp(file + k - 2, ".Z") == 0)
            || (strcmp(file + k - 2, ".z") == 0))) {
        *ispipe = 1;
    }
    else {
        if ((k > 3) && ((strcmp(file + k - 3, ".gz") == 0)
                        || (strcmp(file + k - 3, ".GZ") == 0))) {
            *ispipe = 1;
            isgz = 1;
        }
    }
#endif

    strcpy(tmpfile, file);
    if (stat(tmpfile, &statbuf) != 0) {
        /* File doesn't exist; try other compressed/uncompressed form, as appropriate */
        E_ERROR_SYSTEM("stat(%s) failed\n", tmpfile);

        if (*ispipe) {
            if (isgz)
                tmpfile[k - 3] = '\0';
            else
                tmpfile[k - 2] = '\0';

            if (stat(tmpfile, &statbuf) != 0)
                return NULL;
        }
        else {
            strcpy(tmpfile + k, ".gz");
            if (stat(tmpfile, &statbuf) != 0) {
#if (! WIN32)
                strcpy(tmpfile + k, ".Z");
                if (stat(tmpfile, &statbuf) != 0)
                    return NULL;
#else
                return NULL;
#endif
            }
        }

        E_WARN("Using %s instead of %s\n", tmpfile, file);
    }

    return (fopen_comp(tmpfile, "r", ispipe));
}


#define FREAD_RETRY_COUNT	60

int32
fread_retry(void *pointer, int32 size, int32 num_items, FILE * stream)
{
    char *data;
    uint32 n_items_read;
    uint32 n_items_rem;
    uint32 n_retry_rem;
    int32 loc;

    n_retry_rem = FREAD_RETRY_COUNT;

    data = pointer;
    loc = 0;
    n_items_rem = num_items;

    do {
        n_items_read = fread(&data[loc], size, n_items_rem, stream);

        n_items_rem -= n_items_read;

        if (n_items_rem > 0) {
            /* an incomplete read occurred */

            if (n_retry_rem == 0)
                return -1;

            if (n_retry_rem == FREAD_RETRY_COUNT) {
                E_ERROR_SYSTEM("fread() failed; retrying...\n");
            }

            --n_retry_rem;

            loc += n_items_read * size;
#if (! WIN32)
            sleep(1);
#endif
        }
    } while (n_items_rem > 0);

    return num_items;
}


#define STAT_RETRY_COUNT	10

int32
stat_retry(char *file, struct stat * statbuf)
{
    int32 i;

    for (i = 0; i < STAT_RETRY_COUNT; i++) {
        if (stat(file, statbuf) == 0)
            return 0;

        if (i == 0) {
            E_ERROR_SYSTEM("stat(%s) failed; retrying...\n", file);
        }
#if (! WIN32)
        sleep(1);
#endif
    }

    return -1;
}


int32
stat_mtime(char *file)
{
    struct stat statbuf;

    if (stat(file, &statbuf) != 0)
        return -1;

    return ((int32) statbuf.st_mtime);
}


FILE *
_myfopen(const char *file, char *mode, char *pgm, int32 line)
{
    FILE *fp;

    if ((fp = fopen(file, mode)) == NULL) {
        fflush(stdout);
        fprintf(stderr,
                "FATAL_ERROR: \"%s\", line %d: fopen(%s,%s) failed; ",
                pgm, line, file, mode);
        perror("");
        fflush(stderr);

        exit(errno);
    }

    return fp;
}
