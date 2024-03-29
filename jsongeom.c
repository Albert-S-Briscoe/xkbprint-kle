#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define XK_TECHNICAL
#define XK_PUBLISHING
#define XK_KATAKANA
#include <stdio.h>
#include <ctype.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKM.h>
#include <X11/extensions/XKBfile.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <xkbcommon/xkbcommon.h>
#include <json-c/json.h>

#if defined(sgi)
#include <malloc.h>
#endif

#include <stdlib.h>

#include "utils.h"
#include "xkbprint-kle.h"
#include "isokeys.h"

typedef struct {
	float				x;
	float				y;
	int					profile;
	Bool				ghosted;
	float				height;
	char				baseColor[8];
	char				textColor[8];
} JSONState;

typedef struct {
    Display *           dpy;
    XkbDescPtr          xkb;
    XkbGeometryPtr      geom;
    int                 black;
    int                 white;
    int                 color;
    int                 font;
    int                 fontSize;
    int                 nPages;
    int                 x1, y1;
    int                 x2, y2;
    XKBPrintArgs *      args;
    JSONState			lastkey;
} PSState;

#define NLABELS     8
#define LABEL_LEN   30

// maximum that can reasonably fit on a keycap
#define LABEL_LAYERS	4
#define LABEL_GROUPS	2

typedef struct {
    unsigned    present;
    Bool        alpha[2];
    char        label[NLABELS][LABEL_LEN];
    int         font[NLABELS];
    int         size[NLABELS];
} KeyTop;

static char *getcolor(int Color, PSState *state) {
	XkbGeometryPtr geom = state->geom;
	static char output[20];
    int tmp;
    register int i;

    i = Color;
    if (Color < geom->num_colors) {
        XkbColorPtr color = &geom->colors[i];
        if (uStrCaseEqual(color->spec, "black"))
        	strcpy(output, "#000000");
        else if (uStrCaseEqual(color->spec, "white"))
        	strcpy(output, "#ffffff");
        else if ((sscanf(color->spec, "grey%d", &tmp) == 1) ||
                 (sscanf(color->spec, "gray%d", &tmp) == 1) ||
                 (sscanf(color->spec, "Grey%d", &tmp) == 1) ||
                 (sscanf(color->spec, "Gray%d", &tmp) == 1)) {
        	sprintf(output, "#%1$.2x%1$.2x%1$.2x", (int)(((float) tmp) * -2.55 + 255));
        }
        else if ((tmp = (uStrCaseEqual(color->spec, "red") * 100)) ||
             (sscanf(color->spec, "red%d", &tmp) == 1)) {
        	sprintf(output, "#%1$.2x0000", (int)(((float) tmp) * 2.55));
		}
        else if ((tmp = (uStrCaseEqual(color->spec, "green") * 100)) ||
             (sscanf(color->spec, "green%d", &tmp) == 1)) {
        	sprintf(output, "#00%1$.2x00", (int)(((float) tmp) * 2.55));
		}
        else if ((tmp = (uStrCaseEqual(color->spec, "blue") * 100)) ||
             (sscanf(color->spec, "blue%d", &tmp) == 1)) {
        	sprintf(output, "#0000%1$.2x", (int)(((float) tmp) * 2.55));
        }
        else
        	strcpy(output, "#eeeeee"); // ≈ .9
    }
    return output;
}

// PSShapDef()

// PSProlog() contains some useful calculations on overall size maybe


static json_object
*PSDoodad(FILE *out, PSState *state, XkbDoodadPtr doodad)
{
    XkbDescPtr xkb;
    const char *name = "", *dname;
	float yoffset = 0;
    int sz, leading;
	char *keycolor;
    float x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0;
    json_object *output;
	json_object *properties = json_object_new_object();

    xkb = state->xkb;
    if (doodad->any.name != None) {
        dname = XkbAtomGetString(xkb->dpy, doodad->any.name);
	}
    else {
        dname = "NoName";
    }

//	fprintf(stderr, "Doodad: dname: \"%s\"\n", dname);

    switch (doodad->any.type) {
    case XkbOutlineDoodad:
    case XkbSolidDoodad:
    	output = json_object_new_array();

        name = XkbAtomGetString(xkb->dpy,
                                XkbShapeDoodadShape(xkb->geom,
                                                    &doodad->shape)->name);
//		fprintf(stderr, " Outline/Solid Doodad \"%s\"\n", name);


        if (state->args->wantColor) {
//	        PSSetColor(out, state, doodad->shape.color_ndx);
            if (doodad->any.type != XkbOutlineDoodad) {
//	            fprintf(out, "true %d %d %d %s %% Doodad %s\n",
//	                    doodad->shape.angle,
//	                    doodad->shape.left, doodad->shape.top, name, dname);
//	            PSSetColor(out, state, state->black);
            }
//	        fprintf(out, "false %d %d %d %s %% Doodad %s\n",
//	                doodad->shape.angle,
//	                doodad->shape.left, doodad->shape.top, name, dname);
			keycolor = getcolor(doodad->shape.color_ndx, state);
        	if (strcmp(keycolor, state->lastkey.baseColor) != 0) {
        		json_object_object_add(properties, "c", json_object_new_string(keycolor));
        		strcpy(state->lastkey.baseColor, keycolor);
			}

        }
        else {
//	        fprintf(out, "false %d %d %d %s %% Doodad %s\n",
//	                doodad->shape.angle,
//	                doodad->shape.left, doodad->shape.top, name, dname);
        }

		for (int i = 0; i < state->geom->num_shapes; i++) {
//			PSShapeDef(out, state, &state->geom->shapes[i]);
			//        file  state     shape
//			fprintf(stderr, "   Shape name: \"%s\"\n", XkbAtomGetString(state->dpy, state->geom->shapes[i].name));
			if (strcmp(name, XkbAtomGetString(state->dpy, state->geom->shapes[i].name)) == 0) {
//				fprintf(stderr, "    Got it\n");
				x1 = ((double) state->geom->shapes[i].bounds.x1) / 190.0;
				y1 = ((double) state->geom->shapes[i].bounds.y1) / 190.0;
				x2 = ((double) state->geom->shapes[i].bounds.x2) / 190.0;
				y2 = ((double) state->geom->shapes[i].bounds.y2) / 190.0;
				break;
			}
		}


		yoffset = (double) (doodad->shape.top) / 190.0 + y1;
		yoffset -= state->lastkey.y;
		yoffset -= 1.0;
		json_object_object_add(properties, "x", json_object_new_double((double) doodad->shape.left / 190.0 + x1));
		json_object_object_add(properties, "y", json_object_new_double((double) yoffset));
		json_object_object_add(properties, "w", json_object_new_double(x2 - x1));
		json_object_object_add(properties, "h", json_object_new_double(y2 - y1));
//		if (!state->lastkey.ghosted) {
//			json_object_object_add(properties, "g", json_object_new_boolean(True));
//			state->lastkey.ghosted = True;
//		}
		if (state->lastkey.profile != PROFILE_FLAT) {
			json_object_object_add(properties, "p", json_object_new_string("FLAT"));
			state->lastkey.profile = PROFILE_FLAT;
		}

		json_object_array_add(output, properties);
		state->lastkey.y = ((double) (doodad->shape.top) / 190.0) + y1;

		json_object_array_add(output, json_object_new_string(""));
		return output;
        break;
    case XkbTextDoodad:
//		fprintf(stderr, " Text Doodad\n");
//	    fprintf(out, "%% Doodad %s\n", dname);
//	    PSSetColor(out, state, doodad->text.color_ndx);
//	    PSGSave(out, state);
//	    fprintf(out, "%d %d translate\n", doodad->text.left, doodad->text.top);
        if (doodad->text.angle != 0)
//	        fprintf(out, "%s rotate\n",
//	                XkbGeomFPText(doodad->text.angle, XkbMessage));
        sz = 14;
        if (doodad->text.font) {
//	        FontStuff stuff;

//	        if (CrackXLFDName(doodad->text.font, &stuff)) {
//	            if (stuff.ptSize > 0)
//	                sz = stuff.ptSize / 10;
//	            ClearFontStuff(&stuff);
//	        }
        }
//	    PSSetFont(out, state, FONT_LATIN1, sz, True);
        leading = (sz * 12) / 10;
        if (strchr(doodad->text.text, '\n') == NULL) {
//	        fprintf(out, "0 %d pts moveto 1 -1 scale\n", (leading * 8) / 10);
//	        fprintf(out, "(%s) show\n", doodad->text.text);
        }
        else {
            char *tmp, *buf, *end;
            int offset = (leading * 8 / 10);

            tmp = buf = strdup(doodad->text.text);
            while (tmp != NULL) {
                end = strchr(tmp, '\n');
                if (end != NULL)
                    *end++ = '\0';
//	            fprintf(out, "0 %d pts moveto 1 -1 scale\n", offset);
//	            fprintf(out, "(%s) show 1 -1 scale\n", tmp);
                offset += leading;
                tmp = end;
            }
            free(buf);
        }
//	    PSGRestore(out, state);
        break;
    case XkbIndicatorDoodad:
        name = XkbAtomGetString(xkb->dpy,
                                XkbIndicatorDoodadShape(xkb->geom,
                                                        &doodad->indicator)->
                                name);
//		fprintf(stderr, " Indicator Doodad \"%s\"\n", name);
		if (state->args->wantColor) {
//	        PSSetColor(out, state, doodad->indicator.off_color_ndx);
//	        fprintf(out, "true 0 %d %d %s %% Doodad %s\n",
//	                doodad->indicator.left, doodad->indicator.top, name, dname);
//	        PSSetColor(out, state, state->black);
        }
//	    fprintf(out, "false 0 %d %d %s %% Doodad %s\n",
//	            doodad->indicator.left, doodad->indicator.top, name, dname);
        break;
    case XkbLogoDoodad:
        name = XkbAtomGetString(xkb->dpy,
                                XkbLogoDoodadShape(xkb->geom,
                                                   &doodad->logo)->name);
//		fprintf(stderr, " Logo Doodad \"%s\"\n", name);
        if (state->args->wantColor)
//	        PSSetColor(out, state, doodad->shape.color_ndx);
//	    fprintf(out, "false %d %d %d %s %% Doodad %s\n",
//	            doodad->shape.angle,
//	            doodad->shape.left, doodad->shape.top, name, dname);
        break;
    }
//	json_object_array_add(output, json_object_new_string(""));
	output = json_object_new_object(); // lazy way to fix alignment issues because of not implementing every doodad
    return output;
}

// ignore PSKeycapsSymbol()

// PSNonLatin1Symbol() might be mildly interesting

static void
labelSymbol(char *label) {
	if (strcmp(label, "Up") == 0) {
		strcpy(label, "↑");
	}
	if (strcmp(label, "Left") == 0) {
		strcpy(label, "←");
	}
	if (strcmp(label, "Down") == 0) {
		strcpy(label, "↓");
	}
	if (strcmp(label, "Right") == 0) {
		strcpy(label, "→");
	}
	return;
}

static void
simplifyLabel(char *label) { // suggestions are welcome
	if ((strcmp(label, "Shift_L") == 0) ||
	    (strcmp(label, "Shift_R") == 0)) {
		strcpy(label, "Shift");
	}
	if (strcmp(label, "ISO_Left_Tab") == 0) {
		strcpy(label, "Tab");
	}
	if (strcmp(label, "BackSpace") == 0) {
		strcpy(label, "Backspace");
	}
	if (strcmp(label, "Multi_key") == 0) {
		strcpy(label, "Compose");
	}
	if (strcmp(label, "ISO_Level3_Shift") == 0) {
		strcpy(label, "Level3"); // AltGr?
	}
	if (strcmp(label, "ISO_Level5_Shift") == 0) {
		strcpy(label, "Level5");
	}
	if ((strcmp(label, "Alt_L") == 0) ||
		(strcmp(label, "Alt_R") == 0) ||
		(strcmp(label, "Meta_L") == 0) ||
		(strcmp(label, "Meta_R") == 0)) {
		strcpy(label, "Alt");
	}
	if ((strcmp(label, "Super_L") == 0) ||
	    (strcmp(label, "Super_R") == 0)) {
		strcpy(label, "Win");
	}
	if (strcmp(label, "Prior") == 0) {
		strcpy(label, "PgUp");
	}
	if (strcmp(label, "Next") == 0) {
		strcpy(label, "PgDown");
	}
	if (strcmp(label, "Escape") == 0) {
		strcpy(label, "Esc");
	}
	if ((strcmp(label, "Control_L") == 0) ||
		(strcmp(label, "Control_R") == 0)) {
		strcpy(label, "Ctrl");
	}
	if (strcmp(label, " ") == 0) {
		strcpy(label, "NNBSP");
	}
	if (strcmp(label, "‌") == 0) {
		strcpy(label, "ZWNJ");
	}
	if (strcmp(label, " ") == 0) {
		strcpy(label, "NBSP");
	}
	if (strncmp(label, "ISO_", 4) == 0) { // remove ISO_ prefix
		strcpy(label, label + 4);
	}
	for (int i = 1; label[i]; i++) { // underscores look better as spaces. Starts at 1 so that the underscore label doesn't match.
		if (label[i] == '_')
			label[i] = ' ';
	}
	return;
}

static KeySym
CheckSymbolAlias(KeySym sym)
{
    if (XkbKSIsKeypad(sym)) {
        if ((sym >= XK_KP_0) && (sym <= XK_KP_9))
            sym = (sym - XK_KP_0) + XK_0;
        else
            switch (sym) {
            case XK_KP_Space:
                return XK_space;
            case XK_KP_Tab:
                return XK_Tab;
            case XK_KP_Enter:
                return XK_Return;
            case XK_KP_F1:
                return XK_F1;
            case XK_KP_F2:
                return XK_F2;
            case XK_KP_F3:
                return XK_F3;
            case XK_KP_F4:
                return XK_F4;
            case XK_KP_Home:
                return XK_Home;
            case XK_KP_Left:
                return XK_Left;
            case XK_KP_Up:
                return XK_Up;
            case XK_KP_Right:
                return XK_Right;
            case XK_KP_Down:
                return XK_Down;
            case XK_KP_Page_Up:
                return XK_Page_Up;
            case XK_KP_Page_Down:
                return XK_Page_Down;
            case XK_KP_End:
                return XK_End;
            case XK_KP_Begin:
                return XK_Begin;
            case XK_KP_Insert:
                return XK_Insert;
            case XK_KP_Delete:
                return XK_Delete;
            case XK_KP_Equal:
                return XK_equal;
            case XK_KP_Multiply:
                return XK_asterisk;
            case XK_KP_Add:
                return XK_plus;
            case XK_KP_Subtract:
                return XK_minus;
            case XK_KP_Divide:
                return XK_slash;
            }
    }
    else if (XkbKSIsDeadKey(sym)) {
        switch (sym) {
        case XK_dead_grave:
            sym = XK_grave;
            break;
        case XK_dead_acute:
            sym = XK_acute;
            break;
        case XK_dead_circumflex:
            sym = XK_asciicircum;
            break;
        case XK_dead_tilde:
            sym = XK_asciitilde;
            break;
        case XK_dead_macron:
            sym = XK_macron;
            break;
        case XK_dead_breve:
            sym = XK_breve;
            break;
        case XK_dead_abovedot:
            sym = XK_abovedot;
            break;
        case XK_dead_diaeresis:
            sym = XK_diaeresis;
            break;
        case XK_dead_abovering:
            sym = XK_degree;
            break;
        case XK_dead_doubleacute:
            sym = XK_doubleacute;
            break;
        case XK_dead_caron:
            sym = XK_caron;
            break;
        case XK_dead_cedilla:
            sym = XK_cedilla;
            break;
        case XK_dead_ogonek:
            sym = XK_ogonek;
            break;
        case XK_dead_iota:
            sym = XK_Greek_iota;
            break;
        case XK_dead_voiced_sound:
            sym = XK_voicedsound;
            break;
        case XK_dead_semivoiced_sound:
            sym = XK_semivoicedsound;
            break;
        }
    }
    return sym;
}


// complicated and probably hard to read label arrangement code
static void
arrangeLabels(char *output, char keycaps[LABEL_GROUPS][LABEL_LAYERS][LABEL_LEN], int kc, PSState *state) {
	const char *labels[12] = { "", "", "", "", "", "", "", "", "", "", "", "" };
    char keycode[4] = ""; // keycode string

/*
where labels[x] is actually placed on the keycap.
+---------+
|0   8   2|
|6   9   7|
|1   10  3|
+---------+
|4   11  5|
*/
	switch (state->args->labelFormat) {
	case FORMAT_MINIMAL:
		if (keycaps[0][0][0] == '\0') // show alphabetical characters correctly
			labels[0] = keycaps[0][1];
		else
			labels[0] = keycaps[0][0];
		break;
	case FORMAT_ALTGR:
		labels[0] = keycaps[0][1];
		labels[1] = keycaps[0][0];
		labels[2] = keycaps[0][3];
		labels[3] = keycaps[0][2];
		break;
	case FORMAT_ISO:
		labels[0] = keycaps[0][1];
		labels[1] = keycaps[0][2];
		labels[2] = keycaps[1][1];
		labels[3] = keycaps[1][2];
		// 4
		// 5
		labels[6] = keycaps[0][0];
		labels[7] = keycaps[1][0];
		break;
	case FORMAT_EXTRA:
		labels[0] = keycaps[0][1];
		labels[1] = keycaps[0][2];
		labels[2] = keycaps[1][1];
		labels[3] = keycaps[1][2];
		labels[4] = keycaps[0][3];
		labels[5] = keycaps[1][3];
		labels[6] = keycaps[0][0];
		labels[7] = keycaps[1][0];
		break;
	case FORMAT_NONE:
		break;
	case FORMAT_BASIC: // format_basic (default)
	default:
		labels[0] = keycaps[0][1];
		labels[1] = keycaps[0][0];
		break;
	}

	// move labels to the top on keys with only one label in a group (like backspace or shift).
	// 0, 6, 1, 4
	if (labels[0][0] == '\0' && labels[4][0] == '\0') {
		if (labels[1][0] == '\0' && labels[6][0] != '\0') {
			labels[0] = labels[6];
			labels[6] = "";
		}
		if (labels[1][0] != '\0' && labels[6][0] == '\0') {
			labels[0] = labels[1];
			labels[1] = "";
		}
	}
	// 2, 7, 3, 5
	if (labels[2][0] == '\0' && labels[5][0] == '\0') {
		if (labels[3][0] == '\0' && labels[7][0] != '\0') {
			labels[2] = labels[7];
			labels[7] = "";
		}
		if (labels[3][0] != '\0' && labels[7][0] == '\0') {
			labels[2] = labels[3];
			labels[3] = "";
		}
	}

	for (int i = 0; i < 11; i++) {
		strcat(output, labels[i]);
		strcat(output, "\n");
	}

    if (state->args->wantKeycodes) {
		sprintf(keycode, "%d", kc);
		strcat(output, keycode);
    }

	return;
}


static json_object *FindKeysymsByName(XkbDescPtr xkb, char *name, PSState *state, KeyTop *top) {
    static char buf[LABEL_LEN];
    int kc;
    KeySym sym, *syms, topSyms[NLABELS];
    int level, group;
    int eG, nG, gI, l, g;
    json_object *output;
    char keycaps[LABEL_GROUPS][LABEL_LAYERS][LABEL_LEN]; // array of strings to store keycap text in. 4 groups, 64 levels is the max in xkb
    char formattedstr[256] = ""; // used to arrange text on the keycaps
    int groups = 0; // keep track of number of groups

    bzero(top, sizeof(KeyTop));
    kc = XkbFindKeycodeByName(xkb, name, True);
    for (int i = 0; i < LABEL_GROUPS; i++) {
    	for (int a = 0; a < LABEL_LAYERS; a++) {
			keycaps[i][a][0] = '\0';
    	}
    }

    if (state->args != NULL) {
        level = state->args->labelLevel;
        group = state->args->baseLabelGroup;
    }
    else
        level = group = 0;
    syms = XkbKeySymsPtr(xkb, kc);
    eG = group;
    nG = XkbKeyNumGroups(xkb, kc);
    gI = XkbKeyGroupInfo(xkb, kc);
//	if ((state->args->wantDiffs) && (eG >= XkbKeyNumGroups(xkb, kc)))
//	    return False;           /* XXX was a return with no value */
    if (nG == 0) {
        return False;
    }
    else if (nG == 1) {
        eG = 0;
    }
    else if (eG >= XkbKeyNumGroups(xkb, kc)) {
        switch (XkbOutOfRangeGroupAction(gI)) {
        default:
            eG %= nG;
            break;
        case XkbClampIntoRange:
            eG = nG - 1;
            break;
        case XkbRedirectIntoRange:
            eG = XkbOutOfRangeGroupNumber(gI);
            if (eG >= nG)
                eG = 0;
            break;
        }
    }

//	fprintf(stderr, "Current keycode: %d\n", kc);

    for (g = 0; g < LABEL_GROUPS; g++) {
        if ((eG + g) >= nG)
            continue;
        for (l = 0; l < LABEL_LAYERS; l++) {
			char utf8[LABEL_LEN];
//            int font, sz;

            if (level + l >= XkbKeyGroupWidth(xkb, kc, (eG + g)))
                continue;
            sym = syms[((eG + g) * XkbKeyGroupsWidth(xkb, kc)) + (level + l)];

            if (state->args->simplifyKeysyms)
                sym = CheckSymbolAlias(sym);
            if ((g * 2) + l < NLABELS)
	            topSyms[(g * 2) + l] = sym;

            utf8[0] = '\0';
            xkb_keysym_to_utf8(sym, utf8, LABEL_LEN);

            if ((utf8[0] & '\xE0') && (utf8[0] != '\x7f')) { // exclude ascii control codes and empty strings
                strcpy(buf, utf8);
           	}
           	else {
                char *tmp;

                tmp = XKeysymToString(sym); // full keysym name (eg "Aacute" or "ISO_Level3_Shift" or "Backspace")
                if (tmp != NULL)
                    strcpy(buf, tmp);
                else
                    snprintf(buf, sizeof(buf), "(%ld)", sym); // last resort: print keysym as a number
           	}
//			fprintf(stderr, "        G%dL%d text: \"%s\"\n", g, l, buf);
			if (state->args->UnicodeLabels) {
				labelSymbol(buf);
			}
			if (state->args->altNames) {
				simplifyLabel(buf);
		    }
			strcpy(keycaps[g][l], buf);
        }

        // not sure where to put this, here should be ok ish
        // remove duplicate labels
		if (strcmp(keycaps[g][0], keycaps[g][1]) == 0) {
        	keycaps[g][1][0] = '\0';
		}
		if (strcmp(keycaps[g][0], keycaps[g][2]) == 0) {
			keycaps[g][2][0] = '\0';
        }
        if (strcmp(keycaps[g][0], keycaps[g][3]) == 0) {
        	keycaps[g][3][0] = '\0';
        }

		if ((g == 0 || g == 1) && (keycaps[g][0][0] != '\0') && (keycaps[g][1][0] != '\0') && (state->args->UnicodeAlpha || !(keycaps[g][0][0] & '\x80'))) {
            KeySym lower, upper;

            XConvertCase(topSyms[(g * 2)], &lower, &upper);
            if ((topSyms[(g * 2)] == lower) && (topSyms[(g * 2) + 1] == upper)) {
//				fprintf(stderr, "        alpha\n");
            	keycaps[g][0][0] = '\0';
	            top->alpha[g] = True;
            }
	    }
        groups += 1;
	}

	arrangeLabels(formattedstr, keycaps, kc, state);

	output = json_object_new_string(formattedstr);
	return output;
}


// extremely messy function
static json_object *PSSection(FILE *out, PSState *state, XkbSectionPtr section) {
	json_object *sectionjson = json_object_new_array();
    int r, offset;
    XkbRowPtr row;
//	Display *dpy;
    XkbDescPtr xkb;

    xkb = state->xkb;
//	dpy = xkb->dpy;
//	fprintf(out, "%% Begin Section '%s'\n", (section->name != None ?
//	        XkbAtomGetString(dpy, section-> name) : "NoName"));
//	fprintf(out, "%d %d translate\n", section->left, section->top);
	if (section->angle != 0)
       	fprintf(stderr, "Rotated section found. Rotated sections are currently unsupported. The rotation will be ignored, possibly breaking the rest of the layout.\n"
						"If you have a layout with a rotated section, please open an issue that includes the xkb geometry.\n");
//	    fprintf(out, "%s rotate\n", XkbGeomFPText(section->angle, XkbMessage));
    if (section->doodads) {
        XkbDrawablePtr first, draw;

        first = draw = XkbGetOrderedDrawables(NULL, section);
        while (draw) {
            if (draw->type == XkbDW_Section)
                PSSection(out, state, draw->u.section);
//	        else {
//	            PSDoodad(out, state, draw->u.doodad);
//	        }
            draw = draw->next;
        }
        XkbFreeOrderedDrawables(first);
    }
//	fprintf(stderr, "Rows: %d\n", section->num_rows);
    for (r = 0, row = section->rows; r < section->num_rows; r++, row++) {
		json_object *rowjson = json_object_new_array();
	    int k;
        XkbKeyPtr key;
        XkbShapePtr shape;
        XkbBoundsRec bounds;

//	    fprintf(stderr, "  Row: %d\n", r);

        if (row->vertical) {
        	fprintf(stderr, "Vertical row found. Vertical rows are currently unsupported and will be ignored. This may break other sections.\n"
							"If you have a layout with a vertical row, please open an issue that includes the xkb geometry.\n");
//		    fprintf(stderr, "    vertical row\n");
            offset = row->top;
        }
        else {
//		    fprintf(stderr, "    regular row\n");
            offset = row->left;
        }
//        fprintf(out, "%% Begin %s %d\n", row->vertical ? "column" : "row",
//                r + 1);

//	    PSSetColor(out, state, xkb->geom->label_color->pixel);
//	    PSSetFont(out, state, FONT_LATIN1, 12, True);

//	    fprintf(stderr, "    keys: %d\n", row->num_keys);

        for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
//	        char *name, *name2, buf[LABEL_LEN], buf2[LABEL_LEN];
//	        int x, y;
            KeyTop top;
            char *keycolor;
            char *textcolor;
			int flag = 0;

			json_object *properties = json_object_new_object();
            shape = XkbKeyShape(xkb->geom, key);
            XkbComputeShapeTop(shape, &bounds);
            offset += key->gap;
//	        name = name2 = NULL;

//		    fprintf(stderr, "      key: %d\n", k);

            if (row->vertical) {
/*	            if (state->args->wantColor) {
                    if (key->color_ndx != state->white) {
                    	fprintf(stderr, "        color: %s\n", getcolor(key->color_ndx, state));
//                      PSSetColor(out, state, key->color_ndx);
                        fprintf(out, "true 0 %d %d %s %% %s\n",
                                row->left, offset,
                                XkbAtomGetString(dpy, shape->name),
                                XkbKeyNameText(key->name.name, XkbMessage));
                    }
//                  PSSetColor(out, state, state->black);
                }
                fprintf(out, "false 0 %d %d %s %% %s\n", row->left, offset,
                        XkbAtomGetString(dpy, shape->name),
                        XkbKeyNameText(key->name.name, XkbMessage));
                offset += shape->bounds.y2;
*/	        }
	        else {
				float yoffset = 0;

                if (key->color_ndx != state->white) {
                   	keycolor = getcolor(key->color_ndx, state);
//					fprintf(stderr, "        color: %s\n", keycolor);

                   	if (strcmp(keycolor, state->lastkey.baseColor) != 0) {
                   		json_object_object_add(properties, "c", json_object_new_string(keycolor));
                   		strcpy(state->lastkey.baseColor, keycolor);
                   		flag = 1;
					}

//                        fprintf(out, "true 0 %d %d %s %% %s\n", offset, row->top, XkbAtomGetString(dpy, shape->name), XkbKeyNameText(key->name.name, XkbMessage));
//                  PSSetColor(out, state, state->black);
                }
//	            fprintf(out, "false 0 %d %d %s %% %s\n", offset, row->top, XkbAtomGetString(dpy, shape->name), XkbKeyNameText(key->name.name, XkbMessage));

//	            if (state->args->label == LABEL_SYMBOLS) {
					if (k == 0) {
	                   	char textcolorstr[50];
						yoffset = (double) (section->top + row->top) / 190.0;
						yoffset -= state->lastkey.y;
						yoffset -= 1.0;

		                if (state->args->wantColor) {
							textcolor = getcolor(state->geom->label_color->pixel, state); // never changes, this function needs to be restructured
	                    	if (state->args->group2Color) {
								sprintf(textcolorstr, "%s\n\n#0000ff\n#0000ff\n\n\n\n#0000ff", textcolor);
								textcolor = textcolorstr;
	   	                 	}
							json_object_object_add(properties, "t", json_object_new_string(textcolor));
						}

						json_object_object_add(properties, "a", json_object_new_int(0));
						json_object_object_add(properties, "y", json_object_new_double((double) yoffset));
						json_object_object_add(properties, "x", json_object_new_double(((double) section->left + (double) key->gap + (double) row->left) / 190.0));
//					    fprintf(stderr, "      left: %d, gap: %d\n", section->left, key->gap);
						flag = 1;
					}
					if (key->gap && (k != 0)) {
						json_object_object_add(properties, "x", json_object_new_double((double) key->gap / 190.0));
						flag = 1;
					}
					if (state->lastkey.ghosted) {
						json_object_object_add(properties, "g", json_object_new_boolean(False));
						state->lastkey.ghosted = False;
						flag = 1;
					}
					if (state->lastkey.profile != state->args->profile) {
						switch (state->args->profile) {
						case PROFILE_SA:
							json_object_object_add(properties, "p", json_object_new_string("SA"));
							break;
						case PROFILE_DSA:
							json_object_object_add(properties, "p", json_object_new_string("DSA"));
							break;
						case PROFILE_DCS:
							json_object_object_add(properties, "p", json_object_new_string("DCS"));
							break;
						case PROFILE_OEM:
							json_object_object_add(properties, "p", json_object_new_string("OEM"));
							break;
						case PROFILE_CHICKLET:
							json_object_object_add(properties, "p", json_object_new_string("CHICKLET"));
							break;
						case PROFILE_FLAT:
							json_object_object_add(properties, "p", json_object_new_string("FLAT"));
							break;
						case PROFILE_DEFAULT:
						default:
//							fprintf(stderr, "The default profile can be buggy. If you see inconsistencies, specify a profile with -p");
							json_object_object_add(properties, "p", json_object_new_string(" ")); // can't be an empty string because ???
							break;
						}
						state->lastkey.profile = state->args->profile;
						flag = 1;
					}
					if (shape->bounds.x2 != 190) {
						json_object_object_add(properties, "w", json_object_new_double((double) shape->bounds.x2 / 190.0));
						flag = 1;
					}
					if (shape->bounds.y2 != 190) {
						json_object_object_add(properties, "h", json_object_new_double((double) shape->bounds.y2 / 190.0));
						flag = 1;
					}
					if (flag == 1) {
						json_object_array_add(rowjson, properties);
					}
					json_object_array_add(rowjson, FindKeysymsByName(xkb, key->name.name, state, &top));

					if (shape->num_outlines > 2) {
						fprintf(stderr, "Irregular key found. Irregular keys are not supported yet, so the bounds will be used instead. Nothing else should break.\n");
					}

//				    fprintf(stderr, "        outline: %d,%d,%d,%d\n", shape->outlines->points[0].x, shape->outlines->points[0].y, shape->outlines->points[1].x, shape->outlines->points[1].y);
//				    fprintf(stderr, "        bounds: %d,%d,%d,%d\n", shape->bounds.x1, shape->bounds.y1, shape->bounds.x2, shape->bounds.y2);
//				    fprintf(stderr, "        gap: %d\n", key->gap);
			        state->lastkey.height = ((double) shape->bounds.y2 / 190.0);
			        state->lastkey.y = ((double) (row->top + section->top) / 190.0);
//			        state->lastkey.x = ((double) row->left / 190.0);
//	            }

                offset += shape->bounds.x2;
//                state->lastkey.x += ((float)shape->bounds.x2 / 190.0);
            }

/*	        if (state->args->label == LABEL_SYMBOLS) {
            }
            else {
                char *olKey;

            	fprintf(stderr, "I don't know exactly what this code does. You should probably remove the \"-label <what>\"");

                if (section->num_overlays > 0)
                    olKey = XkbFindOverlayForKey(xkb->geom, section,
                                                 key->name.name);
                else
                    olKey = NULL;

                if (state->args->label == LABEL_KEYNAME) {
                    name = XkbKeyNameText(key->name.name, XkbMessage);
                    if (olKey)
                        name2 = XkbKeyNameText(olKey, XkbMessage);
                }
                else if (state->args->label == LABEL_KEYCODE) {
                    name = buf;
                    snprintf(name, sizeof(buf), "%d",
                            XkbFindKeycodeByName(xkb, key->name.name, True));
                    if (olKey) {
                        name2 = buf2;
                        snprintf(name2, sizeof(buf2), "%d",
                                 XkbFindKeycodeByName(xkb, olKey, True));
                    }
                }

                bzero(&top, sizeof(KeyTop));
                if (name2 != NULL) {
//	                top.present |= G1LX_MASK;
//	                strncpy(top.label[G1L1], name, LABEL_LEN - 1);
//	                top.label[G1L1][LABEL_LEN - 1] = '\0';
//	                strncpy(top.label[G1L2], name2, LABEL_LEN - 1);
//	                top.label[G1L2][LABEL_LEN - 1] = '\0';
	            }
                else if (name != NULL) {
//	                top.present |= CENTER_MASK;
//	                strncpy(top.label[CENTER], name, LABEL_LEN - 1);
//	                top.label[CENTER][LABEL_LEN - 1] = '\0';
	            }
                else {
//                    fprintf(out, "%% No label for %s\n",
//                            XkbKeyNameText(key->name.name, XkbMessage));
                }
            }*/
            if (row->vertical) {
//                x = row->left;
//                y = offset;
                offset += shape->bounds.y2;
            }
            else {
//                x = offset;
//                y = row->top;
                offset += shape->bounds.x2;
            }
//	        name = key->name.name;
//	        fprintf(out, "%% %s\n", XkbKeyNameText(name, XkbMessage));
//	        PSLabelKey(out, state, &top, x, y, &bounds, kc, shape->bounds.y2);
        }
		json_object_array_add(sectionjson, rowjson);

        state->lastkey.x = 0;
    }
    return sectionjson;
}


static json_object *metadata(PSState *state) {
	int p;
	Bool drawBorder;
	XkbDrawablePtr draw, first;
	XkbPropertyPtr prop;
    json_object *metadata = json_object_new_object();

    json_object_object_add(metadata, "author", json_object_new_string("xkbprint-kle"));

    for (p = 0, prop = state->geom->properties; p < state->geom->num_properties; p++, prop++) {
		char *name;
		name = NULL;
		if ((prop->value != NULL) && (uStrCaseEqual(prop->name, "description"))) {
            name = prop->value;
		    json_object_object_add(metadata, "name", json_object_new_string(name));
            break;
        }
    }

	first = XkbGetOrderedDrawables(state->geom, NULL);

	for (draw = first, drawBorder = True; draw != NULL; draw = draw->next) { // check if default border/background should be drawn.
        if ((draw->type != XkbDW_Section) && ((draw->u.doodad->any.type == XkbOutlineDoodad) || (draw->u.doodad->any.type == XkbSolidDoodad))) {
            char *name;
            name = XkbAtomGetString(state->dpy, draw->u.doodad->any.name);
            if ((name != NULL) && (uStrCaseEqual(name, "edges"))) {
                drawBorder = False;
                break;
            }
        }
    }
	if (drawBorder && (state->args->wantColor)) {
	    json_object_object_add(metadata, "backcolor", json_object_new_string(getcolor(state->geom->base_color->pixel, state)));
    }

    // more metadata?
    return metadata;
}

static Bool
copyArray(json_object *array, json_object *input, const size_t start, const Bool get) {
	if (!json_object_is_type(array, json_type_array) ||
		!json_object_is_type(input, json_type_array) ||
		start > json_object_array_length(input))
		return False;

	for (size_t i = start; i < json_object_array_length(input); i++) {
		if (get)
			json_object_get(json_object_array_get_idx(input, i));
		json_object_array_add(array, json_object_array_get_idx(input, i));
	}
	return True;
}

static void
clearArray(json_object *array) {
	size_t size = json_object_array_length(array);
	json_object_array_del_idx(array, 0, size);
}

// original is the array to be split and contains the first part after. half is the object to store the second part in. keep is the number of objects in the first part.
// I don't like these variable names but I can't think of anything better.
static Bool
splitArray(json_object *original, json_object *half, size_t keep) {
	size_t i;

	if (keep == json_object_array_length(original))
		return True;

	// copy 2nd half of original into half
	if (!copyArray(half, original, keep, True))
		return False;

	// delete the 2nd part of original, or more specifically, delete (original's length - keep) objects, starting with the object at keep's value.
	i = json_object_array_length(original) - keep;
	json_object_array_del_idx(original, keep, i);

	return True;
}

// If I implemented vertical rows, this would give 2nd value for a change in y after the start of the row.
// gets y offset of a row
static float
getYValue(json_object *row) {
	json_object *yValue;

	if (json_object_object_get_ex(json_object_array_get_idx(row, 0), "y", &yValue)) {
		return (float) json_object_get_double(yValue);
	}
	return 0.0;
}

static void
setYValue(json_object *row, float y) {
	json_object *properties = json_object_array_get_idx(row, 0);

	if (properties == NULL) {
		fprintf(stderr, "Something broke: Found a row with zero keys. Please report it.");
		return;
	}

	if (json_object_is_type(properties, json_type_object)) {
		json_object_object_add(properties, "y", json_object_new_double(y));
	}
	else if (!json_object_is_type(properties, json_type_object)) {
		json_object *temp = json_object_new_array();
		size_t i;

		properties = json_object_new_object();
		json_object_object_add(properties, "y", json_object_new_double(y));
		json_object_array_add(temp, properties);

		copyArray(temp, row, 0, True); // copy row to temp

		i = json_object_array_length(row);
		json_object_array_del_idx(row, 0, i); // delete all entries in row

		copyArray(row, temp, 0, False); // copy temp back to row
	}
}

static Bool
moveArrayItem(json_object *array, size_t from, size_t to) {
	json_object *item = json_object_new_array();
	json_object *temp = json_object_new_array();

	if (!json_object_is_type(array, json_type_array) ||
		from > json_object_array_length(array) ||
		to > json_object_array_length(array))
		return False;

	// split array into 3 groups: everything before the specified index, the specified index, and everything after.
	splitArray(array, item, from);
	splitArray(item, temp, 1);

	// merge the before and after, and clear the temp array.
	copyArray(array, temp, 0, True);
	clearArray(temp);

	// split the main array into two sections, before the specified "to" index and after.
	splitArray(array, temp, to);

	// merge the before section, the moved item, and after, to get the output.
	copyArray(array, item, 0, False);
	copyArray(array, temp, 0, False);

	return True;
}


Bool
GeometryToJSON(FILE *out, XkbFileInfo *pResult, XKBPrintArgs *args)
{
    XkbDrawablePtr first, draw;
    PSState state;
    int i;

    json_object *keyboardjson = json_object_new_array();


    if ((!pResult) || (!pResult->xkb) || (!pResult->xkb->geom))
        return False;
    state.xkb = pResult->xkb;
    state.dpy = pResult->xkb->dpy;
    state.geom = pResult->xkb->geom;
    state.color = state.black = state.white = -1;
    state.font = -1;
    state.nPages = 0;
    state.x1 = state.y1 = state.x2 = state.y2 = 0;
    state.args = args;
    state.lastkey.x = 0;
    state.lastkey.y = -1.0;
    state.lastkey.ghosted = False;
//    state.lastkey.profile = PROFILE_DEFAULT;

    json_object_array_add(keyboardjson, metadata(&state)); // add metadata to main array

//    PSProlog(out, &state);

	first = XkbGetOrderedDrawables(state.geom, NULL);

    for (draw = first; draw != NULL; draw = draw->next) {
        if (draw->type == XkbDW_Section) {
            //json_object_array_add(keyboardjson, PSSection(out, &state, draw->u.section)); // the actual keycodes and stuff
            json_object *tempobject = PSSection(out, &state, draw->u.section);
            for (i = 0; i < json_object_array_length(tempobject); i++) {
               	json_object_array_add(keyboardjson, json_object_array_get_idx(tempobject, i));
			}
		}
	    else if (args->doodads) {
	    	json_object *tempobject = PSDoodad(out, &state, draw->u.doodad);
	    	if (json_object_is_type(tempobject, json_type_array))
				json_object_array_add(keyboardjson, tempobject);
	    }
    }
    XkbFreeOrderedDrawables(first);

	fprintf(out, "%s\n", json_object_to_json_string_ext(keyboardjson, JSON_C_TO_STRING_PRETTY));

    return True;
}
