// Config

#define DEBUG
#define OPTDIR ".local/share/screenshots"
#define OPTQUALITY 80
#define OPTFPS 30
#define OPTWIDTH 3
#define OPTR 255
#define OPTG 0
#define OPTB 255

//

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <webp/encode.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Display   *disp;
static int        scrn;
static Window     root;

static unsigned long int r, g, b;
static Time lasttime;
static Window win;
static XEvent e;
static XImage *img;
static GC gc;
static XGCValues gcv;
static XWindowAttributes _xwa;
#define W _xwa.width
#define H _xwa.height

XPoint rectpts[] = {
   // X  Y
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
}; // pts used to draw rect
#define rectx rectpts[0].x
#define recty rectpts[0].y
int rectw;
int recth;

int main(int argc, char* argv[]) {
	(void)argv;
	(void)argc;

	// Init X
	disp = XOpenDisplay(NULL);
	scrn = XDefaultScreen(disp);
	root = XRootWindow(disp, scrn);
	XGetWindowAttributes(disp, root, &_xwa); // allow use of W, H (screen size)

	// Get Screen
	img = XGetImage(
		disp, root, 
		0, 0,
		W, H,
		AllPlanes,
		ZPixmap
	);
	r = img->red_mask;
	g = img->green_mask;
	b = img->blue_mask;

	// Skip selection step (full screenshot) if arg provided
	if (argc > 1)
		goto l_noselection;

	// Create Window
	win = XCreateWindow(
    	disp, root, 
    	0, 0, W, H, 
    	0, CopyFromParent,
		InputOutput, CopyFromParent,
		0,
		NULL
	);
	XStoreName(disp, win, "screenshot");
	XSelectInput(disp, win, ExposureMask);

	// Create GC
	gcv.function = 0;
	gcv.line_width = OPTWIDTH;
	gc = XCreateGC(disp, win, GCLineWidth, &gcv);
	XSetForeground(disp, gc, (OPTR << 16) + (OPTG << 8) + OPTB);

	// Grab Pointer
	if (XGrabPointer(
		disp, root,
		False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, 
		GrabModeAsync, GrabModeAsync,
		None, XCreateFontCursor(disp, XC_sizing), 
		CurrentTime
	) != GrabSuccess) {
		#ifdef DEBUG
			printf("Debug: Failed to grab mouse\n");
		#endif
		return -1;
	}

	// Raise/Map Window
	XMapRaised(disp, win);
	XSync(disp, False);

	// Wait for first Button Press
	do {
		XMaskEvent(disp, ButtonPressMask, &e);		
	} while (e.type != ButtonPress);
	rectx = e.xbutton.x;
	recty = e.xbutton.y;

	#ifdef DEBUG
		printf("Debug: Starting selection\n");
	#endif
	
	// Draw Rect
	do {
		XMaskEvent(disp, ButtonReleaseMask|PointerMotionMask|ExposureMask, &e);
		if (e.type == MotionNotify) {
			rectw = e.xbutton.x - rectx;
			recth = e.xbutton.y - recty;
			rectpts[1].x =  rectw;
			rectpts[2].y =  recth;
			rectpts[3].x = -rectw;
			rectpts[4].y = -recth;
			#ifdef DEBUG
				printf("Debug: @ %d %d # %d %d\n", rectx, recty, rectw, recth);
			#endif
			if ((e.xmotion.time - lasttime) > (1000 / OPTFPS)) {
				lasttime = e.xmotion.time;
				XPutImage(disp, win, gc, img, 0, 0, 0, 0, W, H);
				XDrawLines(disp, win, gc, rectpts, 5, CoordModePrevious);
				XSync(disp, False);
			}
		}
	} while (e.type != ButtonRelease);

	// Destroy window & ungrab
	XUngrabPointer(disp, CurrentTime);
	XDestroyWindow(disp, win);

	// Fix coords
	if (rectw < 0) {
		rectx += rectw;
		rectw *= -1;
	}
	if (recth < 0) {
		recty += recth;
		recth *= -1;
	}

	// Take subimage
	img = XSubImage(img, rectx, recty, rectw, recth);
	l_noselection:
	XSync(disp, False); // Wait for screenshot to be processed / cropped

	// Change xxxA to xxx (yay array manipulation)
	char *p1 = img->data + 3;
	char *p2 = img->data + 4;
	while (p2 - img->data < img->height * img->bytes_per_line) {
		*p1 = *p2;
		++p1; ++p2;
		*p1 = *p2;
		++p1; ++p2;
		*p1 = *p2;
		++p1;
		p2 += 2;
	}
	/*
		// This is equal to the code above but less effecient
		for (int i = 3, j = 4; j < img->height * img->bytes_per_line; i += 3, j += 4) {
			img->data[i] = img->data[j];
			img->data[i + 1] = img->data[j + 1];
			img->data[i + 2] = img->data[j + 2];
		}
	*/

	// Save file
	FILE *fp;
	char fn[64];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	#ifdef OPTDIR
		sprintf(fn, "%s/"OPTDIR"/%d-%02d-%02d %02d:%02d:%02d.webp",
			getenv("HOME"),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
		);
		fp = fopen(fn, "wb");
		if (fp == NULL) {
	#endif
		sprintf(fn, "/tmp/%d-%02d-%02d %02d:%02d:%02d.webp",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
		);
		fp = fopen(fn, "wb");
		if (fp == NULL) {
			#ifdef DEBUG
				printf("Debug: Failed to write to OPTDIR or /tmp/\n");
			#endif
			return 1;
		}
		#ifdef DEBUG 
		#ifdef OPTDIR
			printf("Debug: Failed to write to OPTDIR\n");
		#endif
		#endif
	#ifdef OPTDIR
		}
	#endif
	#ifdef DEBUG
		printf("Debug: Writing to %s\n", fn);
	#endif

	unsigned char *fd;
	size_t fs;
	if (r < g && g < b) { // RGB
		fs = WebPEncodeRGB((unsigned char *)img->data, img->width, img->height, img->bytes_per_line / 4 * 3, OPTQUALITY, &fd);
		#ifdef DEBUG
			printf("Debug: Pixel format = RGB\n");
		#endif
	} else if (b < g && g < r) { // BGR
		fs = WebPEncodeBGR((unsigned char *)img->data, img->width, img->height, img->bytes_per_line / 4 * 3, OPTQUALITY, &fd);
		#ifdef DEBUG
			printf("Debug: Pixel format = BGR\n");
		#endif
	} else {
		#ifdef DEBUG
			printf("Debug: Unsupported pixel format %lu %lu %lu\n", r, g, b);
		#endif
		return 1;
	}
	#ifdef DEBUG
		printf("Debug: Image size = %dx%d (%dbytes/line)\n", img->width, img->height, img->bytes_per_line);
	#endif
	
	fwrite(fd, 1, fs, fp);
	fclose(fp);
	free(fd);
	XDestroyImage(img);
	
	// Copy to clipboard (for some reason a mime type of image/webp doesn't work)
	char *args[] = {"xclip", "-selection", "clipboard", "-t", "image/png", fn, NULL};
	execvp(args[0], args);

	// Close display
	XCloseDisplay(disp);
	
	return 0;
}
