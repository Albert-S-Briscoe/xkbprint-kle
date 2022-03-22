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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <X11/Xlocale.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKM.h>
#include <X11/extensions/XKBfile.h>
#include <X11/keysym.h>

#include <xkbcommon/xkbcommon.h>

#if defined(sgi)
#include <malloc.h>
#endif

#include <stdlib.h>

#include "utils.h"
#include "xkbprint-kle.h"

/***====================================================================***/


#define	WANT_DEFAULT	0
#define	WANT_PS_FILE	1
#define	WANT_X_SERVER	2


static unsigned         outputFormat = WANT_DEFAULT;
static const char *     wantLocale = "C";
static char *           rootDir;
static char *           inputFile;
static char *           outputFile;
static char *           inDpyName;
static char *           outDpyName;
static Display *        inDpy;
static Display *        outDpy;
static XKBPrintArgs     args;
static unsigned         warningLevel = 5;
static Bool             synch;

/***====================================================================***/

static void
Usage(int argc, char *argv[])
{
    fprintf(stderr, "Usage: %s [options] {<input-file>|<display>} [<output-file>] \n%s", argv[0],
            "\n"
            "Useful options:\n"
            "-h, --help         Print this message\n"
            "-c                 Color second printed group blue\n"
            "-f <what>          Change what number of characters are printed on each key. <what> can be:\n"
            "                       \"basic\": base and shift (default),\n"
            "                       \"minimal\": base level only,\n"
            "                       \"none\": no labels (doesn't affect keycodes),\n"
            "                       \"altgr\": level 3 and 4 on right,\n"
            "                       \"iso\": 2 groups, 3 levels, 2nd group on the right,\n"
            "                       \"extended\": iso with 4th level on keycap front.\n"
            "-k, --keycodes     Also print keycodes, if possible\n"
            "--keysymnames      Print full keysym names for keys that don't type printable characters (Shift_L instead of Shift)\n"
            "--no-unicode-alpha Disable merging alphabetical unicode characters into uppercase only\n"
            "-o <file>          Specifies output file name\n"
            "-p <profile>       Specify the KLE keycap profile. Valid options for <profile> are:\n"
            "                       \"SA\", \"DSA\", \"DCS\", \"OEM\", \"CHICKLET\", \"FLAT\"\n"
            "-u                 Replace certain labels (only arrows currently) with Unicode characters\n"
            "\n"
			"Less useful options:\n"
            "-lg <num>          Use keyboard group <num> to print labels (?)\n"
            "-ll <num>          Use shift level <num> to print labels (?)\n" // base level in the future
#ifdef DEBUG
            "-d [flags]    Report debugging information\n"
            "-I[<dir>]     Specifies a top level directory\n"
            "              for include directives.  You can\n"
            "              specify multiple directories.\n"
#endif
            "-lc <locale>  Use <locale> for fonts and symbols (doesn't affect json output)\n"
            "--mono        Disable colors from xkb. Might not disable everything.\n"
            "--no-simplify Disable simplifying weird keysyms (like dead keys) to more normal ones\n"
            "-R[<DIR>]     Specifies the root directory for relative path names\n"
            "-synch        Force synchronization\n"
            "-version      Print program version\n"
            "-w <lvl>      Set warning level (0=none, 10=all) (doesn't do much)\n"
            "\n"
            "Example: ./xkbprint-kle -k -u --no-unicode-alpha -f extended $DISPLAY output.json\n"
        );
}

/***====================================================================***/

static Bool
parseArgs(int argc, char *argv[])
{
    register int i;

    args.scaleToFit = True;
    args.wantColor = True;
    args.wantKeycodes = False;
    args.UnicodeAlpha = True;
    args.group2Color = False;
    args.altNames = True;
    args.UnicodeLabels = False;
    args.simplifyKeysyms = True;
    args.labelFormat = FORMAT_BASIC;
    args.baseLabelGroup = 0;
    args.labelLevel = 0;
    args.profile = PROFILE_DEFAULT;
    for (i = 1; i < argc; i++) {
        if ((argv[i][0] != '-') || (uStringEqual(argv[i], "-"))) {
            if (inputFile == NULL) {
                inputFile = argv[i];
            }
            else if (outputFile == NULL) {
                outputFile = argv[i];
            }
            else {
                uWarning("Too many file names on command line\n");
                uAction("Compiling %s, writing to %s, ignoring %s\n",
                        inputFile, outputFile, argv[i]);
            }
        }
        else if ((strcmp(argv[i], "-h") == 0) ||
                 (strcmp(argv[i], "--help") == 0)) {
            Usage(argc, argv);
            exit(0);
        }
        else if (strcmp(argv[i], "-c") == 0) {
            args.group2Color = True;
        }
#ifdef DEBUG
        else if (strcmp(argv[i], "-d") == 0) {
            if ((i >= (argc - 1)) || (!isdigit(argv[i + 1][0]))) {
                debugFlags = 1;
            }
            else {
                sscanf(argv[++i], "%i", &debugFlags);
            }
            uInformation("Setting debug flags to %d\n", debugFlags);
        }
#endif
        else if (strcmp(argv[i], "-f") == 0) {
            if (++i >= argc) {
                uWarning("Label format not specified\n");
                uAction("Trailing \"-f\" option ignored\n");
            }
            else if (uStrCaseEqual(argv[i], "basic"))
                args.labelFormat = FORMAT_BASIC;
            else if (uStrCaseEqual(argv[i], "minimal"))
                args.labelFormat = FORMAT_MINIMAL;
            else if (uStrCaseEqual(argv[i], "altgr"))
                args.labelFormat = FORMAT_ALTGR;
            else if (uStrCaseEqual(argv[i], "iso"))
                args.labelFormat = FORMAT_ISO;
            else if (uStrCaseEqual(argv[i], "extended"))
                args.labelFormat = FORMAT_EXTRA;
            else if (uStrCaseEqual(argv[i], "none"))
                args.labelFormat = FORMAT_NONE;
            else {
                uWarning("Unknown label format \"%s\" specified\n", argv[i]);
                uAction("Ignored\n");
            }
        }
        else if ((strcmp(argv[i], "-k") == 0) || (strcmp(argv[i], "--keycodes") == 0)) {
            args.wantKeycodes = True;
        }
        else if (strcmp(argv[i], "--keysymnames") == 0) {
            args.altNames = False;
        }
        else if (strcmp(argv[i], "-lc") == 0) {
            if (++i >= argc) {
                uWarning("Locale not specified\n");
                uAction("Trailing \"-lc\" option ignored\n");
            }
            else
                wantLocale = argv[i];
        }
        else if (strcmp(argv[i], "-lg") == 0) {
            int tmp;

            if (++i >= argc) {
                uWarning("Label group not specified\n");
                uAction("Trailing \"-lg\" option ignored\n");
            }
            else if ((sscanf(argv[i], "%i", &tmp) != 1) || (tmp < 1) ||
                     (tmp > 4)) {
                uWarning("Label group must be an integer in the range 1..2\n");
                uAction("Illegal group %d ignored\n", tmp);
            }
            else
                args.baseLabelGroup = tmp - 1;
        }
        else if (strcmp(argv[i], "-ll") == 0) {
            int tmp;

            if (++i >= argc) {
                uWarning("Label level not specified\n");
                uAction("Trailing \"-ll\" option ignored\n");
            }
            else if ((sscanf(argv[i], "%i", &tmp) != 1) || (tmp < 1) ||
                     (tmp > 255)) {
                uWarning("Label level must be in the range 1..4\n");
                uAction("Illegal level %d ignored\n", tmp);
            }
            else
                args.labelLevel = tmp - 1;
        }
        else if (strcmp(argv[i], "--mono") == 0) {
            args.wantColor = False;
        }
        else if (strcmp(argv[i], "--no-simplify") == 0) {
            args.simplifyKeysyms = False;
        }
        else if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                uWarning("No output file specified\n");
                uAction("Trailing \"-o\" option ignored\n");
            }
            else if (outputFile != NULL) {
                uWarning("Multiple output files specified\n");
                uAction("Compiling %s, ignoring %s\n", outputFile, argv[i]);
            }
            else
                outputFile = argv[i];
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (++i >= argc) {
                uWarning("Key profile not specified\n");
                uAction("Trailing \"-p\" option ignored\n");
            }
            else if (uStrCaseEqual(argv[i], "SA"))
                args.profile = PROFILE_SA;
            else if (uStrCaseEqual(argv[i], "DSA"))
                args.profile = PROFILE_DSA;
            else if (uStrCaseEqual(argv[i], "DCS"))
                args.profile = PROFILE_DCS;
            else if (uStrCaseEqual(argv[i], "OEM"))
                args.profile = PROFILE_OEM;
            else if (uStrCaseEqual(argv[i], "CHICKLET"))
                args.profile = PROFILE_CHICKLET;
            else if (uStrCaseEqual(argv[i], "FLAT"))
                args.profile = PROFILE_FLAT;
            else {
                uWarning("Unknown key profile \"%s\" specified\n", argv[i]);
                uAction("Ignored\n");
            }
        }
        else if (strncmp(argv[i], "-R", 2) == 0) {
            if (argv[i][2] == '\0') {
                uWarning("No root directory specified\n");
                uAction("Ignoring -R option\n");
            }
            else if (rootDir != NULL) {
                uWarning("Multiple root directories specified\n");
                uAction("Using %s, ignoring %s\n", rootDir, argv[i]);
            }
            else
                rootDir = &argv[i][2];
        }
        else if ((strcmp(argv[i], "-synch") == 0) ||
                 (strcmp(argv[i], "-s") == 0)) {
            synch = True;
        }
        else if (strcmp(argv[i], "-u") == 0) {
            args.UnicodeLabels = True;
        }
        else if (strcmp(argv[i], "--no-unicode-alpha") == 0) {
            args.UnicodeAlpha = False;
        }
        else if (strcmp(argv[i], "-version") == 0) {
            puts(PACKAGE_STRING);
            exit(0);
        }
        else if (strcmp(argv[i], "-w") == 0) {
            if ((i >= (argc - 1)) || (!isdigit(argv[i + 1][0]))) {
                warningLevel = 0;
            }
            else {
                int itmp;

                if (sscanf(argv[++i], "%i", &itmp))
                    warningLevel = itmp;
            }
        }
        else {
            uError("Unknown flag \"%s\" on command line\n", argv[i]);
            Usage(argc, argv);
            return False;
        }
    }
    if (rootDir) {
        if (warningLevel > 8) {
            uWarning("Changing root directory to \"%s\"\n", rootDir);
        }
        if ((chdir(rootDir) < 0) && (warningLevel > 0)) {
            uWarning("Couldn't change root directory to \"%s\"\n", rootDir);
            uAction("Root directory (-R) option ignored\n");
        }
    }
	if (inputFile == NULL) {
        uError("No input file specified\n");
        Usage(argc, argv);
        return False;
    }
    else if (uStringEqual(inputFile, "-")) {
        /* Nothing */
    }
    else if (strchr(inputFile, ':') == NULL) {
        size_t len = strlen(inputFile);

        if ((len > 4) && (strcmp(&inputFile[len - 4], ".xkm") == 0)) {
            /* Nothing */
        }
        else {
            FILE *file;

            file = fopen(inputFile, "r");
            if (file) {
                fclose(file);
            }
            else {
                fprintf(stderr, "Cannot open \"%s\" for reading\n", inputFile);
                return False;
            }
        }
    }
    else {
        inDpyName = inputFile;
        inputFile = NULL;
    }

    if (outputFormat == WANT_DEFAULT)
        outputFormat = WANT_PS_FILE;
    if ((outputFile == NULL) && (inputFile != NULL) &&
        uStringEqual(inputFile, "-")) {
        size_t len;

        len = strlen("stdin.eps") + 2;
        outputFile = calloc(len, sizeof(char));

        if (outputFile == NULL) {
            uInternalError("Cannot allocate space for output file name\n");
            uAction("Exiting\n");
            exit(1);
        }
        snprintf(outputFile, len, "stdin.ps");
    }
    else if ((outputFile == NULL) && (inputFile != NULL)) {
        size_t len;
        char *base, *ext;

        base = strrchr(inputFile, '/');
        if (base == NULL)
            base = inputFile;
        else
            base++;

        len = strlen(base) + strlen("eps") + 2;
        outputFile = calloc(len, sizeof(char));

        if (outputFile == NULL) {
            uInternalError("Cannot allocate space for output file name\n");
            uAction("Exiting\n");
            exit(1);
        }
        ext = strrchr(base, '.');
        if (ext == NULL) {
            snprintf(outputFile, len, "%s.json", base);
        }
        else {
            strcpy(outputFile, base);
            strcpy(&outputFile[ext - base + 1], "json");
        }
    }
    else if (outputFile == NULL) {
        size_t len;
        char *ch, *name, buf[128];

        if (inDpyName[0] == ':')
            snprintf(name = buf, sizeof(buf), "server%s", inDpyName);
        else
            name = inDpyName;

        len = strlen(name) + strlen("eps") + 2;
        outputFile = calloc(len, sizeof(char));

        if (outputFile == NULL) {
            uInternalError("Cannot allocate space for output file name\n");
            uAction("Exiting\n");
            exit(1);
        }
        strcpy(outputFile, name);
        for (ch = outputFile; (*ch) != '\0'; ch++) {
            if (*ch == ':')
                *ch = '-';
            else if (*ch == '.')
                *ch = '_';
        }
        *ch++ = '.';
        strcpy(ch, "json");
    }
    else if (strchr(outputFile, ':') != NULL) {
        outDpyName = outputFile;
        outputFile = NULL;
        outputFormat = WANT_X_SERVER;
        uInternalError("Output to an X server not implemented yet\n"); // why would it ever be? PS to x? I'm leaving this here just because I think it's funny.
        return False;
    }
    return True;
}

static Display *
GetDisplay(char *program, char *dpyName)
{
    int mjr, mnr, error;
    Display *dpy;

    mjr = XkbMajorVersion;
    mnr = XkbMinorVersion;
    dpy = XkbOpenDisplay(dpyName, NULL, NULL, &mjr, &mnr, &error);
    if (dpy == NULL) {
        switch (error) {
        case XkbOD_BadLibraryVersion:
            uInformation("%s was compiled with XKB version %d.%02d\n",
                         program, XkbMajorVersion, XkbMinorVersion);
            uError("X library supports incompatible version %d.%02d\n",
                   mjr, mnr);
            break;
        case XkbOD_ConnectionRefused:
            uError("Cannot open display \"%s\"\n", dpyName);
            break;
        case XkbOD_NonXkbServer:
            uError("XKB extension not present on %s\n", dpyName);
            break;
        case XkbOD_BadServerVersion:
            uInformation("%s was compiled with XKB version %d.%02d\n",
                         program, XkbMajorVersion, XkbMinorVersion);
            uError("Server %s uses incompatible version %d.%02d\n",
                   dpyName, mjr, mnr);
            break;
        default:
            uInternalError("Unknown error %d from XkbOpenDisplay\n", error);
        }
    }
    else if (synch)
        XSynchronize(dpy, True);
    return dpy;
}

/***====================================================================***/


int
main(int argc, char *argv[])
{
    FILE *file;
    int ok;
    XkbFileInfo result;

    uSetErrorFile(NullString);
    if (!parseArgs(argc, argv))
        exit(1);
#ifdef DEBUG
#ifdef sgi
    if (debugFlags & 0x4)
        mallopt(M_DEBUG, 1);
#endif
#endif
    file = NULL;
    XkbInitAtoms(NULL);
/*     XkbInitIncludePath(); */
    if (inputFile != NULL) {
        if (uStringEqual(inputFile, "-")) {
            static char in[] = "stdin";

            file = stdin;
            inputFile = in;
        }
        else {
            file = fopen(inputFile, "r");
        }
    }
    else if (inDpyName != NULL) {
        inDpy = GetDisplay(argv[0], inDpyName);
        if (!inDpy) {
            uAction("Exiting\n");
            exit(1);
        }
    }
    if (outDpyName != NULL) {
        uInternalError("Output to an X server not implemented yet\n");
        outDpy = GetDisplay(argv[0], outDpyName);
        if (!outDpy) {
            uAction("Exiting\n");
            exit(1);
        }
    }
    if ((inDpy == NULL) && (outDpy == NULL)) {
        int mjr, mnr;

        mjr = XkbMajorVersion;
        mnr = XkbMinorVersion;
        if (!XkbLibraryVersion(&mjr, &mnr)) {
            uInformation("%s was compiled with XKB version %d.%02d\n",
                         argv[0], XkbMajorVersion, XkbMinorVersion);
            uError("X library supports incompatible version %d.%02d\n",
                   mjr, mnr);
            uAction("Exiting\n");
            exit(1);
        }
    }
    ok = True;
    if (file) {
        unsigned tmp;

        bzero((char *) &result, sizeof(result));
        if ((result.xkb = XkbAllocKeyboard()) == NULL) {
            uFatalError("Cannot allocate keyboard description\n");
            /* NOTREACHED */
        }
        tmp = XkmReadFile(file, XkmGeometryMask, XkmKeymapLegal, &result);
        if ((tmp & XkmGeometryMask) != 0) {
            uError("Couldn't read geometry from XKM file \"%s\"\n", inputFile);
            uAction("Exiting\n");
            ok = False;
        }
        if ((tmp & XkmKeyNamesMask) != 0)
            args.wantKeycodes = False;
/*        if (args.label == LABEL_AUTO) {
            if (result.defined & XkmSymbolsMask)
                args.label = LABEL_SYMBOLS;
            else if (result.defined & XkmKeyNamesMask)
                args.label = LABEL_KEYCODE;
            else
                args.label = LABEL_KEYNAME;
        }
        else if ((args.label == LABEL_KEYCODE) &&
                 ((tmp & XkmKeyNamesMask) != 0)) {
            uError("XKM file \"%s\" doesn't have keycodes\n", inputFile);
            uAction("Cannot label keys as requested. Exiting\n");
            ok = False;
        }
        else if ((args.label == LABEL_SYMBOLS) && ((tmp & XkmSymbolsMask) != 0)) {
            uError("XKM file \"%s\" doesn't have symbols\n", inputFile);
            uAction("Cannot label keys as requested. Exiting\n");
            ok = False;
        }*/
    }
    else if (inDpy != NULL) {
        bzero((char *) &result, sizeof(result));
        result.type = XkmKeymapFile;
        result.xkb = XkbGetMap(inDpy, XkbAllMapComponentsMask, XkbUseCoreKbd);
        if (result.xkb == NULL)
            uWarning("Cannot load keyboard description\n");
        if (XkbGetNames(inDpy, XkbAllNamesMask, result.xkb) != Success)
            uWarning("Cannot load names\n");
        if (XkbGetGeometry(inDpy, result.xkb) != Success) {
            uFatalError("Cannot load geometry for %s\n", inDpyName);
        }
    }
    else {
        fprintf(stderr, "Cannot open \"%s\" to read geometry\n", inputFile);
        ok = 0;
    }
    if (ok) {
        FILE *out = NULL;

        if (setlocale(LC_ALL, (wantLocale)) == NULL) {
            if (wantLocale != NULL) {
                uWarning("Couldn't change to locale %s\n", wantLocale);
                uAction("Using \"C\" locale, instead\n");
            }
        }
        /* need C numerics so decimal point doesn't get screwed up */
        setlocale(LC_NUMERIC, "C");
        if ((inDpy != outDpy) &&
            (XkbChangeKbdDisplay(outDpy, &result) != Success)) {
            uInternalError("Error converting keyboard display from %s to %s\n",
                           inDpyName, outDpyName);
            exit(1);
        }
        if (outputFile != NULL) {
            if (uStringEqual(outputFile, "-")) {
                static char of[] = "stdout";

                out = stdout;
                outputFile = of;
            }
            else {
                out = fopen(outputFile, "w");
                if (out == NULL) {
                    uError("Cannot open \"%s\" to write keyboard description\n",
                           outputFile);
                    uAction("Exiting\n");
                    exit(1);
                }
            }
        }
        switch (outputFormat) {
        case WANT_PS_FILE:
            ok = GeometryToJSON(out, &result, &args);
            break;
        case WANT_X_SERVER:
            uInternalError("Output to X server not implemented yet\n");
            break;
        default:
            uInternalError("Unknown output format %d\n", outputFormat);
            uAction("No output file created\n");
            ok = False;
            break;
        }
        if (!ok) {
            uError("Error creating output file\n");
        }
    }
    if (inDpy)
        XCloseDisplay(inDpy);
    inDpy = NULL;
    if (outDpy)
        XCloseDisplay(outDpy);
    return (ok == 0);
}
