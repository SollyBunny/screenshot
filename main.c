#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <png.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define MOUSEMASK (ButtonPressMask|ButtonReleaseMask|PointerMotionMask)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#define MAX(a, b) ((a) > (b)) ? (a) : (b)

// CONFIG
#define BORDER_WIDTH 1
#define COLOR        "#aaaaaa"

// UNCOMMENT TO ENABLE DEBUG O-O
//#define DEBUG 

static Display *display;
static int screen;
static Window root;

/* LSBFirst: BGRA -> RGBA */
static void
convertrow_lsb(unsigned char *drow, unsigned char *srow, XImage *img) {
	int sx, dx;

	for(sx = 0, dx = 0; dx < img->bytes_per_line; sx += 4) {
		drow[dx++] = srow[sx + 2]; /* B -> R */
		drow[dx++] = srow[sx + 1]; /* G -> G */
		drow[dx++] = srow[sx];     /* R -> B */
		if(img->depth == 32)
			drow[dx++] = srow[sx + 3]; /* A -> A */
		else
			drow[dx++] = 255;
	}
}

/* MSBFirst: ARGB -> RGBA */
static void
convertrow_msb(unsigned char *drow, unsigned char *srow, XImage *img) {
	int sx, dx;

	for(sx = 0, dx = 0; dx < img->bytes_per_line; sx += 4) {
		drow[dx++] = srow[sx + 1]; /* G -> R */
		drow[dx++] = srow[sx + 2]; /* B -> G */
		drow[dx++] = srow[sx + 3]; /* A -> B */
		if(img->depth == 32)
			drow[dx++] = srow[sx]; /* R -> A */
		else
			drow[dx++] = 255;
	}
}

static void
writeaspng(XImage *img, FILE *fp)
{
	png_structp png_struct_p;
	png_infop png_info_p;
	void (*convert)(unsigned char *, unsigned char *, XImage *);
	unsigned char *drow = NULL, *srow;
	int h;

	

	png_struct_p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
	                                       NULL);
	png_info_p = png_create_info_struct(png_struct_p);

	//if(!png_struct_p || !png_info_p || setjmp(png_jmpbuf(png_struct_p)))
	//	printf("failed to initialize libpng");

	png_init_io(png_struct_p, fp);
	
	png_set_IHDR(png_struct_p, png_info_p, img->width, img->height, 8,
	             PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	             
	png_write_info(png_struct_p, png_info_p);

	srow = (unsigned char *)img->data;
	drow = calloc(1, img->width * 4); /* output RGBA */
	//if(!drow)
	//	die("Can't calloc");

	if(img->byte_order == LSBFirst)
		convert = convertrow_lsb;
	else
		convert = convertrow_msb;

	for(h = 0; h < img->height; h++) {
		convert(drow, srow, img);
		srow += img->bytes_per_line;
		png_write_row(png_struct_p, drow);
	}
	png_write_end(png_struct_p, NULL);

	free(drow);
	png_free_data(png_struct_p, png_info_p, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&png_struct_p, NULL);
}

int main() {

	Window window_return;

	display = XOpenDisplay(NULL);
	screen  = XDefaultScreen(display);
	root    = RootWindow(display, screen);
	XEvent e;

	XGCValues gcv = {0};
    GC gc;

	int sw, x1, y1, x2, y2, x, y, w, h;
	unsigned int usw;

	bool state = 0;
	if (XGrabPointer(display, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, None, CurrentTime) != GrabSuccess)
		return 0;

	do {
		XMaskEvent(display, MOUSEMASK|ExposureMask, &e);
		switch(e.type) {
		case ConfigureRequest:
		case MapRequest:
			break;

		case ButtonPressMask:
			XQueryPointer(
				display, root, &window_return, &window_return, 
				&x1, &y1, 
				&sw, 
				&sw, 
				&usw
			);
			// x1 += BORDER_WIDTH;
			// y1 += BORDER_WIDTH;
			
		    XSetWindowAttributes attr = {0};
		    XVisualInfo vinfo;
		
		    XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo);
		    attr.colormap = XCreateColormap(display, root, vinfo.visual, AllocNone);
		
		    XColor color;
		    XParseColor(display, attr.colormap, COLOR, &color);
		    XAllocColor(display, attr.colormap, &color);
		    window_return = XCreateWindow(
		    	display, root, 
		    	0, 0, 1, 1, 
		    	1, vinfo.depth,
				InputOutput, vinfo.visual,
				CWColormap | CWBorderPixel | CWBackPixel, &attr
			);

			
			
			/*long value = XInternAtom(
				display, "_NET_WM_WINDOW_TYPE_DOCK", False
			);
			XChangeProperty(
				display, window_return, 
				XInternAtom(display, "_NET_WM_WINDOW_TYPE", False),
				XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1
			);*/

			XStoreName(display, window_return, "screenshot");
			
			XSelectInput(display, window_return, ExposureMask);	
			XMapRaised(display, window_return);
			XSync(display, False);	
			
			gcv.line_width = BORDER_WIDTH - 1;
			gc = XCreateGC(display, window_return, GCLineWidth, &gcv);
									
			state = 1;
			break;

		case Expose:
			XPoint points[] = {
				/*{ x, y, w, BORDER_WIDTH },
		        { x, y, BORDER_WIDTH, h },
		        { x, h - BORDER_WIDTH, w, BORDER_WIDTH },
		        { w - BORDER_WIDTH, y, BORDER_WIDTH, h }
		        { w - BORDER_WIDTH, 0               , BORDER_WIDTH, h            },
		        { 0               , h - BORDER_WIDTH, w           , BORDER_WIDTH },
		        { 0               , 0               , w           , BORDER_WIDTH },
   		        { 0               , 0               , BORDER_WIDTH, h            },
		        // dots to make it good with circular windows

		        // { 1, 1, 1, 2 },
		        // { 2, 1, 1, 1 },
		        // {w - 2, 1, 1, 2},
		        // {w - 1, 1, 1, 1}
				{0, 0},
				{w, 0},
				{w, h},
				{0, h},
				{0, 0}*/

//                X      Y           P
				{ 1    , 0     },
				{ w - 1, 0     },
				{ w - 1, 1     }, // p
				{ w    , 1     }, // pr
				{ w    , h - 1 },
				{ w - 1, h - 1 }, // p
				{ w - 1, h     }, // pr
				{ 1    , h     },
				{ 1    , h - 1 }, // p
				{ 0    , h - 1 }, // pr
				{ 0    , 1     },
				{ 1    , 1     }, // p

			};

			XSetForeground(display, gc, color.pixel);
			/*if (x1 == 0) {
				XDrawRectangles(display, window_return, gc, rectangles, 2);
				XFillRectangles(display, window_return, gc, rectangles, 2);
			} else {*/
	            XDrawLines(display, window_return, gc, points, 12, CoordModeOrigin);
	            //XFillRectangles(display, window_return, gc, rectangles, 4);
			//}
            XSync(display, False);
			break;
			
		case MotionNotify:
			if (!state)
				break;
			x2 = e.xmotion.x;
			y2 = e.xmotion.y;
			if (x1 > x2) {
				x = x2;
				w = x1 - x2;
			} else {
				x = x1;
				w = x2 - x1;
			}

			if (y1 > y2) {
				y = y2;
				h = y1 - y2;
			} else {
				y = y1 ;
				h = y2 - y1;
			}
			#ifdef DEBUG
				printf("%d %d, %d %d\n", x, y, w, h);
			#endif
			XMoveWindow  (display, window_return, x - BORDER_WIDTH, y - BORDER_WIDTH);
			XResizeWindow(display, window_return, w + BORDER_WIDTH + BORDER_WIDTH, h + BORDER_WIDTH + BORDER_WIDTH);
			break;
		}
		continue; // stop from breaking for debug
	} while (e.type != ButtonRelease);
	
	XUngrabPointer(display, CurrentTime);
	XDestroyWindow(display, window_return);

	#ifdef DEBUG
		printf("Width: %d Height: %d\n", w, h);
	#endif
	
	XImage *image;
	image = XGetImage(
		display, DefaultRootWindow(display), 
		//x1, y1, x2 - x1, y2 - y1,
		x + BORDER_WIDTH, y + BORDER_WIDTH, 
		w - BORDER_WIDTH - BORDER_WIDTH, h - BORDER_WIDTH - BORDER_WIDTH,
		//1, 1, 50, 50,
		AllPlanes,
		ZPixmap
	);
	
	struct stat sb;
	char filename[64];

	if (stat("~/Screenshots", &sb) == 0 && S_ISDIR(sb.st_mode)) {
		
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);

		sprintf(filename, "~/Screenshots/%d-%02d-%02d %02d:%02d:%02d.png", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	} else {

		printf("Warn: ~/Screenshots doesn't exist\n");
		//sprintf(filename, "/tmp/screenshot%u", (unsigned)time(NULL));
		sprintf(filename, "/tmp/sc.png");
		
	}

	#ifdef DEBUG
		printf("Info: filename is %s\n", filename);
	#endif
	FILE *fp;
	fp = fopen(filename, "wb");
	writeaspng(image, fp);
	fclose(fp);

	char *args[]={"xclip", "-selection", "clipboard", "-t", "image/png", filename, NULL};


	//xclip -selection clipboard -t image/png -verbose "/home/solly/Screenshots/2021-08-03 13:15:22.png"
	execvp(args[0],args);
	/*Atom clipboard;

	clipboard = XInternAtom(
		display,
		"CLIPBOARD",
		true
	);

	window_return = XCreateSimpleWindow(
		display, root,
		0, 0,
		1, 1,
		0,
		CopyFromParent,	CopyFromParent
	);

	XSetSelectionOwner(display, clipboard, window_return, CurrentTime);
	XFlush(display);
	while (1) {
		XNextEvent(display, &e);
		printf("WOOOO %d", e.type);
	}*/

	XCloseDisplay(display);
	
	return 0;
	
}