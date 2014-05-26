#ifndef UTILS_H
#define	UTILS_H 1

  /*\
   * $Xorg: utils.h,v 1.3 2000/08/17 19:54:50 cpqbld Exp $
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
/* $XFree86: xc/programs/xkbprint/utils.h,v 1.5 2001/07/25 15:05:25 dawes Exp $ */

/***====================================================================***/

#include 	<stdio.h>
#include	<X11/Xos.h>
#include	<X11/Xfuncproto.h>
#include	<X11/Xfuncs.h>
#include 	<stdarg.h>
#include	<stddef.h>

_XFUNCPROTOBEGIN

#ifndef NUL
#define	NUL	'\0'
#endif

/***====================================================================***/

#ifndef OPAQUE_DEFINED
typedef void *Opaque;
#endif
#ifndef NullOpaque
#define	NullOpaque	((Opaque)NULL)
#endif

#ifndef BOOLEAN_DEFINED
typedef char Boolean;
#endif

#ifndef True
#define	True	((Boolean)1)
#define	False	((Boolean)0)
#endif /* ndef True */
#define	booleanText(b)	((b)?"True":"False")

#ifndef COMPARISON_DEFINED
typedef int Comparison;

#define	Greater		((Comparison)1)
#define	Equal		((Comparison)0)
#define	Less		((Comparison)-1)
#define	CannotCompare	((Comparison)-37)
#define	comparisonText(c)	((c)?((c)<0?"Less":"Greater"):"Equal")
#endif


/***====================================================================***/

extern Opaque uAlloc(unsigned    /* size */);
extern Opaque uCalloc(unsigned   /* n */ ,
                      unsigned   /* size */);
extern Opaque uRealloc(Opaque    /* old */ ,
                       unsigned  /* newSize */);
extern Opaque uRecalloc(Opaque   /* old */ ,
                        unsigned /* nOld */ ,
                        unsigned /* nNew */ ,
                        unsigned /* newSize */);
extern void uFree(Opaque         /* ptr */);

#define	uTypedAlloc(t)		((t *)uAlloc((unsigned)sizeof(t)))
#define	uTypedCalloc(n,t)	((t *)uCalloc((unsigned)n,(unsigned)sizeof(t)))
#define	uTypedRealloc(pO,n,t)	((t *)uRealloc((Opaque)pO,((unsigned)n)*sizeof(t)))
#define	uTypedRecalloc(pO,o,n,t) ((t *)uRecalloc((Opaque)pO,((unsigned)o),((unsigned)n),sizeof(t)))
#if (defined mdHasAlloca) && (mdHasAlloca)
#define	uTmpAlloc(n)	((Opaque)alloca((unsigned)n))
#define	uTmpFree(p)
#else
#define	uTmpAlloc(n)	uAlloc(n)
#define	uTmpFree(p)	uFree(p)
#endif

/***====================================================================***/

extern Boolean uSetErrorFile(const char *name);
extern void uInformation(const char *s, ...);
extern void uAction(const char *s, ...);
extern void uWarning(const char *s, ...);
extern void uError(const char *s, ...);
extern void uFatalError(const char *s, ...);
extern void uInternalError(const char *s, ...);

/***====================================================================***/

#define	NullString	((char *)NULL)

#define	uStringText(s)		((s)==NullString?"<NullString>":(s))
#define	uStringEqual(s1,s2)	(uStringCompare(s1,s2)==Equal)
#define	uStringPrefix(p,s)	(strncmp(p,s,strlen(p))==0)
#define	uStringCompare(s1,s2)	(strcmp(s1,s2))
#define	uStrCaseEqual(s1,s2)	(uStrCaseCmp(s1,s2)==0)
#ifdef HAVE_STRCASECMP
#define	uStrCaseCmp(s1,s2)	(strcasecmp(s1,s2))
#define	uStrCasePrefix(p,s)	(strncasecmp(p,s,strlen(p))==0)
#else
extern int uStrCaseCmp(const char * /* s1 */ ,
                       const char * /* s2 */);

extern int uStrCasePrefix(const char * /* p */ ,
                          const char * /* str */);
#endif
#ifdef HAVE_STRDUP
#define	uStringDup(s1)		(strdup(s1))
#else
extern char *uStringDup(const char *  /* s1 */);
#endif

/***====================================================================***/

_XFUNCPROTOEND

#endif                          /* UTILS_H */
