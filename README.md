# screenshot
A very smowl xlib c screenshot tool
It will save the file into ~/Screenshots if it exists and will copy the image to clipboard
Just drag the area you want

### Compiling
You need xlib, xclip and libpng
Use `make` to compile
Use `make install` to syslink the executable to bin so you can just run "screenshot"

### Config
In main.c just change values in there
BORDER_WIDTH: the border width. a duh
BORDER_COLOR: the border colour. a duh
BORDER_ROUND: comment to disable rounded corners (for wms without rounded corners)
DEBUG: uncomment to print extra information

### TODO
the rounded corner is just a single pixel on all 4 edges, wana change it to an arc
currently using xclip to copy to clipboard (BLOAT) so wana do it using pure xlib
rewrite in xcb??
