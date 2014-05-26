
  /*\
   * $Xorg: utils.c,v 1.5 2000/08/17 19:54:50 cpqbld Exp $
   *
   *                          COPYRIGHT 1990
   *                    DIGITAL EQUIPMENT CORPORATION
   *                       MAYNARD, MASSACHUSETTS
   *                        ALL RIGHTS RESERVED.
   *
   * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
   * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
   * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE
   * FOR ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED
   * WARRANTY.
   *
   * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
   * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
   * ADDITION TO THAT SET FORTH ABOVE.
   *
   * Permission to use, copy, modify, and distribute this software and its
   * documentation for any purpose and without fee is hereby granted, provided
   * that the above copyright notice appear in all copies and that both that
   * copyright notice and this permission notice appear in supporting
   * documentation, and that the name of Digital Equipment Corporation not be
   * used in advertising or publicity pertaining to distribution of the
   * software without specific, written prior permission.
  \*/
/* $XFree86: xc/programs/xkbprint/utils.c,v 3.4 2001/01/17 23:46:11 dawes Exp $ */

#include 	"utils.h"
#include	<ctype.h>
#include	<stdlib.h>

unsigned int debugFlags;


/***====================================================================***/

Opaque
uAlloc(unsigned size)
{
    return ((Opaque) malloc(size));
}

/***====================================================================***/

Opaque
uCalloc(unsigned n, unsigned size)
{
    return ((Opaque) calloc(n, size));
}

/***====================================================================***/

Opaque
uRealloc(Opaque old, unsigned newSize)
{
    if (old == NULL)
        return ((Opaque) malloc(newSize));
    else
        return ((Opaque) realloc((char *) old, newSize));
}

/***====================================================================***/

Opaque
uRecalloc(Opaque old, unsigned nOld, unsigned nNew, unsigned itemSize)
{
    char *rtrn;

    if (old == NULL)
        rtrn = (char *) calloc(nNew, itemSize);
    else {
        rtrn = (char *) realloc((char *) old, nNew * itemSize);
        if ((rtrn) && (nNew > nOld)) {
            bzero(&rtrn[nOld * itemSize], (nNew - nOld) * itemSize);
        }
    }
    return (Opaque) rtrn;
}

/***====================================================================***/

void
uFree(Opaque ptr)
{
    if (ptr != (Opaque) NULL)
        free((char *) ptr);
    return;
}


/***====================================================================***/

static FILE *errorFile = NULL;

Boolean
uSetErrorFile(const char *name)
{
    if ((errorFile != NULL) && (errorFile != stderr)) {
        fprintf(errorFile, "switching to %s\n", name ? name : "stderr");
        fclose(errorFile);
    }
    if (name != NullString)
        errorFile = fopen(name, "w");
    else
        errorFile = stderr;
    if (errorFile == NULL) {
        errorFile = stderr;
        return (False);
    }
    return (True);
}

void
uInformation(const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    vfprintf(errorFile, s, ap);
    fflush(errorFile);
    va_end(ap);
    return;
}

/***====================================================================***/

void
uAction(const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile, "                  ");
    vfprintf(errorFile, s, ap);
    fflush(errorFile);
    va_end(ap);
    return;
}

/***====================================================================***/

void
uWarning(const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile, "Warning:          ");
    vfprintf(errorFile, s, ap);
    fflush(errorFile);
    va_end(ap);
    return;
}

/***====================================================================***/

void
uError(const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile, "Error:            ");
    vfprintf(errorFile, s, ap);
    fflush(errorFile);
    va_end(ap);
    return;
}

/***====================================================================***/

void
uFatalError(const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile, "Fatal Error:      ");
    vfprintf(errorFile, s, ap);
    fprintf(errorFile, "                  Exiting\n");
    fflush(errorFile);
    va_end(ap);
    exit(1);
    /* NOTREACHED */
}

/***====================================================================***/

void
uInternalError(const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    fprintf(errorFile, "Internal error:   ");
    vfprintf(errorFile, s, ap);
    fflush(errorFile);
    va_end(ap);
    return;
}

/***====================================================================***/

#ifndef HAVE_STRDUP
char *
uStringDup(const char *str)
{
    char *rtrn;

    if (str == NULL)
        return NULL;
    rtrn = (char *) uAlloc(strlen(str) + 1);
    strcpy(rtrn, str);
    return rtrn;
}
#endif

#ifndef HAVE_STRCASECMP
int
uStrCaseCmp(const char *str1, const char *str2)
{
    char buf1[512], buf2[512];
    char c, *s;

    register int n;

    for (n = 0, s = buf1; (c = *str1++); n++) {
        if (isupper(c))
            c = tolower(c);
        if (n > 510)
            break;
        *s++ = c;
    }
    *s = '\0';
    for (n = 0, s = buf2; (c = *str2++); n++) {
        if (isupper(c))
            c = tolower(c);
        if (n > 510)
            break;
        *s++ = c;
    }
    *s = '\0';
    return (strcmp(buf1, buf2));
}
#endif
