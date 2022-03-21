#ifndef _XKBPRINT_H_
#define _XKBPRINT_H_ 1

/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#define LABEL_AUTO      -1
#define LABEL_NONE      0
#define LABEL_KEYNAME   1
#define LABEL_KEYCODE   2
#define LABEL_SYMBOLS   3

#define NO_SYMBOLS      0
#define COMMON_SYMBOLS  1
#define ALL_SYMBOLS     2

#define FORMAT_BASIC	0
#define FORMAT_MINIMAL	1
#define FORMAT_ALTGR	2
#define FORMAT_ISO		3
#define FORMAT_EXTRA	4

#define PROFILE_DEFAULT	0
#define PROFILE_SHAPE	1
#define PROFILE_COLOR	2
#define PROFILE_SA		3
#define PROFILE_DSA		4
#define PROFILE_DCS		5
#define PROFILE_OEM		6
#define PROFILE_CHICKLET 7
#define PROFILE_FLAT	8


typedef struct _XKBPrintArgs {
    int         grid;
    int         label;
    int         baseLabelGroup;
    int         nLabelGroups;
    int			nLabelLayers;
    int         nTotalGroups;
    int         labelLevel;
    int         wantSymbols;
    int			labelFormat;
    int			profile;
    Bool        wantKeycodes;
    Bool        wantDiffs;
    Bool        scaleToFit;
    Bool        wantColor;
    Bool        level1;
    Bool		UnicodeAlpha; // false to disable alpha for unicode
    Bool		group2; // enable group on right of keycap
    Bool		group2Color; // color 2nd group blue
    Bool		altNames;
} XKBPrintArgs;

extern Bool
DumpInternalFont(
    FILE *       /* out */ ,
    const char * /* fontName */
);

extern Bool
GeometryToJSON(
    FILE *              /* out */ ,
    XkbFileInfo *       /* result */ ,
    XKBPrintArgs *      /* args */
);

#endif                          /* _XKBPRINT_H_ */
