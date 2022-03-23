# XKBPrint-KLE

xkbprint-kle is a program that converts the XKB Geometry section of a keyboard configuration to a http://keyboard-layout-editor.com JSON file.
xkbprint-kle is a fork of xkbprint (https://gitlab.freedesktop.org/xorg/app/xkbprint).

[Here's a comparison between xkbprint and xkbprint-kle](https://imgur.com/a/rdvx7u2)

### What is XKB geometry? Can I convent my XModMap layout to JSON?

On Linux, XKB handles keyboards and keyboard configuration under X11 and Wayland.
XKB has an extremely obscure format for configuring a physical layout, which is the geometry section.
It's somewhat similar to keyboard-layout-editor.com if it didn't have a GUI.

Most people configure XKB indirectly using rules or XModMap, and although xkbprint-kle will work with those, it will look best with custom keymaps made using xkbcomp.

### What is the output?

The output includes key arrangement specified in the XKB geometry section, with the keysyms shown on each key, as configured in the symbols section.
If you have a custom XKB configuration, but no custom geometry, xkbprint-kle will still print that configuration on the default XKB geometry.

## Unsupported parts of XKB

Planned support:
- Doodads (partially done, shape and outline doodads use the bounding box of that shape)
- Irregularly shaped keys (uses a bounding box like dododads)
- Outputting only a specific group
- Starting from a layer other than 1

If your keyboard layout has any of the following, please open an issue, preferably with your XKB geometry or configuration.
I probably won't add support otherwise.
- Vertical rows
- Rotated sections
- Keys with multiple colors on the same key
- Keys that are more complicated than an ISO or big-ass enter

## Compiling
```
git clone https://github.com/Albert-S-Briscoe/xkbprint-kle.git
cd xkbprint-kle
./autogen.sh
make
```

## Examples
For an up to date list of options:

`./xkbprint-kle -h`

Get information directly from the X server, output layout using the first two layers:

`./xkbprint-kle $DISPLAY output.json`

Get information directly from the X server, print keycodes on the fronts of keys, put Unicode arrows on arrow keys, disable treating Unicode alphabetical characters as alphabetical, print 2 groups with 4 layers each and output to output.json:

`./xkbprint-kle -k -u --no-unicode-alpha -f extended $DISPLAY output.json`
