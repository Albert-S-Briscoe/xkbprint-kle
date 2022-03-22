# XKBPrint-KLE

xkbprint-kle is a program that converts the XKB Geometry section of a keyboard configuration to a keyboard-layout-editor.com JSON file.
xkbprint-kle is a fork of xkbprint (https://gitlab.freedesktop.org/xorg/app/xkbprint).

## What is this tool for?

On Linux, XKB handles keyboards under X11 and Wayland.
XKB has an extremely obscure format for configuring a physical layout, something along the lines of keyboard-layout-editor.com if it didn't have a GUI.
Most people configure XKB indirectly using rules or XModMap, and xkbprint-kle will work with those, but it's mainly for custom keymaps made using xkbcomp.
The output can be directly imported to keyboard-layout-editor.com.
It includes the key placement specified in the XKB geometry section and the keysyms assigned to each key in the symbols section.
If you have a custom XKB configuration, but no custom geometry, xkbprint-kle will still print that configuration on the default XKB geometry.

## Unsupported parts of XKB

Planned support:
- Ireggularly shaped keys (creates one big rectangular key)
- Doodads
- Outputting only a specific group
- starting from a layer other than 1

If your keyboard layout has any of the following, please open an issue, preferably with your XKB geometry or configuration.
I probably won't add support otherwise.
- Vertical rows
- Rotated sections
- Keys with multiple colors/specific shapes (or just keys that don't look correct)

## Compiling

`git clone`
`cd xkbprint-kle`
`./autogen.sh`
`make`

## Examples
For an up to date list of options:
`./xkbprint-kle -h`

Get information from X server, output layout using the first two layers:
`./xkbprint-kle $DISPLAY output.json`

Get information from X server, print keycodes on the fronts of keys, put unicode arrows on arrow keys, disable treating unicode alphabetical characters as alphabetical, print 2 groups with 4 layers each and ouptut to output.json:
`./xkbprint-kle -k -u --no-unicode-alpha -f extended $DISPLAY output.json`
