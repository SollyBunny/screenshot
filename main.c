#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#define MAX(a, b) ((a) > (b)) ? (a) : (b)

// CONFIG
#define BORDER_WIDTH  1
#define BORDER_COLOR  "#aaaaaa"
#define FILEDIR       ".local/share/screenshots"

// UNCOMMENT TO ENABLE DEBUG
// #define DEBUG 

static Display   *display;
static int        screen;
static Window     root;
static Window     window;
static GC         gc;
static XGCValues  gcv;

/* LSBFirst: BGRA -> RGBA */
static void convertrow_lsb(unsigned char *drow, unsigned char *srow, XImage *img) {
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
static void convertrow_msb(unsigned char *drow, unsigned char *srow, XImage *img) {
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

static void writeaspng(XImage *img, FILE *fp) {
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
	drow = calloc(1, img->width * 4);

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

int x1, y1_, x2, y2, x, y, w, h;

XPoint points[] = {
//	  X  Y
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
};

void draw() {
	points[1].x = w + BORDER_WIDTH;
	points[2].x = w + BORDER_WIDTH;
	points[2].y = h + BORDER_WIDTH;
	points[3].y = h + BORDER_WIDTH;
	XDrawLines(display, window, gc, points, 5, CoordModeOrigin);
}

int main() {

	XEvent e;

	display = XOpenDisplay(NULL);
	screen  = XDefaultScreen(display);
	root    = RootWindow(display, screen);

	if (XGrabPointer(
		display, root, 
		False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, 
		GrabModeAsync, GrabModeAsync,
		None, XCreateFontCursor(display, XC_sizing), 
		CurrentTime
	) != GrabSuccess) return 0;

	do {
		XMaskEvent(display, ButtonPressMask, &e);
	} while (e.type != ButtonPressMask);

	x1 = e.xbutton.x;
	y1_ = e.xbutton.y;
	w = 1;
	h = 1;
    XSetWindowAttributes attr = {0};
    XVisualInfo vinfo;

    XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo);
    attr.colormap = XCreateColormap(display, root, vinfo.visual, AllocNone);
    
    XColor color;
    XParseColor(display, attr.colormap, BORDER_COLOR, &color);
    XAllocColor(display, attr.colormap, &color);
    window = XCreateWindow(
    	display, root, 
    	0, 0, 1, 1, 
    	1, vinfo.depth,
		InputOutput, vinfo.visual,
		CWColormap | CWBorderPixel | CWBackPixel, &attr
	);

	Atom atoms[2] = { XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False), None };
	XChangeProperty(
		display, 
		window, 
		XInternAtom(display, "_NET_WM_STATE", False),
		XA_ATOM, 32, PropModeReplace, (unsigned char *)atoms, 1
	);

	XStoreName(display, window, "screenshot");
	XSelectInput(display, window, ExposureMask);	

	gcv.function = 0;
	gcv.line_width = BORDER_WIDTH - 1;
	gc = XCreateGC(display, window, GCLineWidth, &gcv);
	XSetForeground(display, gc, color.pixel);

	draw();
	XMoveWindow  (display, window, x - BORDER_WIDTH, y - BORDER_WIDTH);
	XResizeWindow(display, window, 4, 4);
	XMapRaised(display, window);

	Time lasttime = 0;
	do {
		XMaskEvent(display, ButtonReleaseMask|PointerMotionMask|ExposureMask, &e);
		if (e.type == Expose) {
			draw();
		    XSync(display, False);
		} else if (e.type == MotionNotify) {
			if ((e.xmotion.time - lasttime) <= (1000 / 60)) continue;
			lasttime = e.xmotion.time;
			x2 = e.xmotion.x - 1;
			y2 = e.xmotion.y - 1;
			if (x1 > x2) {
				x = x2;
				w = x1 - x2;
			} else {
				x = x1;
				w = x2 - x1;
			}
			if (y1_ > y2) {
				y = y2;
				h = y1_ - y2;
			} else {
				y = y1_;
				h = y2 - y1_;
			}
			#ifdef DEBUG
				printf("%d %d, %d %d\n", x, y, w, h);
			#endif
			XMoveWindow  (display, window, x - BORDER_WIDTH, y - BORDER_WIDTH);
			XResizeWindow(display, window, w + BORDER_WIDTH + BORDER_WIDTH, h + BORDER_WIDTH + BORDER_WIDTH);
			
			XSync(display, False);
		}
	} while (e.type != ButtonRelease);
	
	XUngrabPointer(display, CurrentTime);
	XDestroyWindow(display, window);
	XSync(display, False);

	#ifdef DEBUG
		printf("Info: Width: %d Height: %d\n", w, h);
	#endif
	
	XImage *image;
	image = XGetImage(
		display, DefaultRootWindow(display), 
		x + BORDER_WIDTH, y + BORDER_WIDTH, 
		w - BORDER_WIDTH - BORDER_WIDTH, h - BORDER_WIDTH - BORDER_WIDTH,
		AllPlanes,
		ZPixmap
	);

	
	char filename[64];
	char *homedir = getenv("HOME");

	sprintf(filename, "%s/"FILEDIR, homedir);
	DIR *pdir = opendir(filename);

	if (pdir == NULL) {

		printf("Warn: ~/"FILEDIR"  doesn't exist\n");
		sprintf(filename, "/tmp/sc.png");
		
	} else {

		closedir(pdir);

		time_t t = time(NULL);
		struct tm tm = *localtime(&t);

		sprintf(filename, "%s/"FILEDIR"/%d-%02d-%02d %02d:%02d:%02d.png", homedir, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	}

	#ifdef DEBUG
		printf("Info: filename is %s\n", filename);
	#endif
	FILE *fp;
	fp = fopen(filename, "wb");
	writeaspng(image, fp);
	fclose(fp);

	char *args[]={"xclip", "-selection", "clipboard", "-t", "image/png", filename, NULL};
	execvp(args[0],args);

	XCloseDisplay(display);
	
	return 0;
	
}
