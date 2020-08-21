#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "X.h"
#include"window.h"
#include"draw.h"
#include "script.h"
#include "object.h"
#include "gmoves.h"
#include "menuloop.h"
#include "window.h"
#include "gsaves.h"

int win_width;
int win_height;
char win_resize = 0;
cairo_t *win_cr = NULL;
static Pixmap win_pixmap;

static void apply_win_fs();

void createwin()
{
  real_win = XCreateSimpleWindow(display, RootWindow(display,screen),
			    0, 0,
			    win_width, win_height,
			    0, BlackPixel(display,screen), BlackPixel(display,screen));

  win = XCreateSimpleWindow(display, real_win,
			    0, 0,
			    win_width, win_height,
			    0, BlackPixel(display,screen), BlackPixel(display,screen));

  XSelectInput (display, real_win, StructureNotifyMask | ButtonPressMask |
		ButtonReleaseMask | PointerMotionMask | KeyPressMask |
		KeyReleaseMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask);
  wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, real_win, &wmDeleteMessage, 1);

  XMapWindow(display, real_win);
  XMapWindow(display, win);

  if(win_fs) apply_win_fs();
}

static void calcularea(int area_width, int area_height, int area_x, int area_y,
		       int *width, int *height, int *x, int *y)
{
  if(room_width*area_height < room_height*area_width){
    *height = area_height;
    *width = room_width*area_height/room_height;
  }
  else{
    *width = area_width;
    *height = room_height*area_width/room_width;
  }
  *x = area_x + (area_width - *width)/2;
  *y = area_y + (area_height - *height)/2;
}

void calculatewin()
{
  int max_width, max_height, max_x, max_y, width, height, x, y;

  img_change |= CHANGE_ALL;
  if(menumode) return;

  gsaves_vertical = gmoves_vertical = 1;
  gsaves_length = surf_height;
  calcularea(surf_width - gmoves_width - gsaves_width, surf_height, 0, 0,
	     &max_width, &max_height, &max_x, &max_y);
  max_x += gsaves_width;

  calcularea(surf_width - gsaves_width, surf_height - gmoves_height, 0, 0,
	     &width, &height, &x, &y);

  if(width+height >= max_width+max_height){
    gsaves_vertical = 1; gmoves_vertical = 0;
    gsaves_length = surf_height-gmoves_height;
    max_width = width;
    max_height = height;
    max_x = x+gsaves_width;
    max_y = y;
  }

  calcularea(surf_width - gmoves_width, surf_height - gsaves_height, 0, 0,
	     &width, &height, &x, &y);

  if(width+height >= max_width+max_height){
    gsaves_vertical = 0; gmoves_vertical = 1;
    gsaves_length = surf_width-gmoves_width;
    max_width = width;
    max_height = height;
    max_x = x;
    max_y = y+gsaves_height;
  }

  calcularea(surf_width, surf_height - gmoves_height - gsaves_height, 0, 0,
	     &width, &height, &x, &y);

  if(width+height >= max_width+max_height){
    gsaves_vertical = gmoves_vertical = 0;
    gsaves_length = surf_width;
    max_width = width;
    max_height = height;
    max_x = x;
    max_y = y+gsaves_height;
  }

  if(max_width == 0) max_width = 1;
  if(max_height == 0) max_height = 1;

  room_x_size = max_width;
  room_y_size = max_height;
  room_x_scale = ((float)max_width)/room_width;
  room_y_scale = ((float)max_height)/room_height;
  room_x_translate = max_x;
  room_y_translate = max_y;

  if(!menumode){
    calculate_gsaves();
    gsaves_pointer(1, 0);
  }
}

void create_win_cr()
{
  surf_width = win_width;
  surf_height = win_height;

  win_pixmap = XCreatePixmap (display, win, win_width, win_height, DefaultDepth (display, screen));
  XSetWindowBackgroundPixmap(display, win, win_pixmap);

  win_surf = cairo_xlib_surface_create (display, win_pixmap, DefaultVisual(display, screen),
					win_width, win_height);

  win_cr = cairo_create(win_surf);
}

void free_win_cr()
{
  XSetWindowBackground(display, win, BlackPixel(display,screen));
  cairo_surface_destroy(win_surf);
  cairo_destroy(win_cr);
  XFreePixmap (display, win_pixmap);
}

void repaint_win(int x, int y, int width, int height)
{
  XClearArea (display, win, x, y, width, height, 0);

  XFlush(display);
}

static void apply_win_fs()
{
  Atom wmState = XInternAtom(display, "_NET_WM_STATE", 0);
  Atom fullScreen = XInternAtom(display,
				"_NET_WM_STATE_FULLSCREEN", 0);
  XEvent xev;

  xev.xclient.type=ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event=True;
  xev.xclient.window=real_win;
  xev.xclient.message_type=wmState;
  xev.xclient.format=32; 
  xev.xclient.data.l[0] = (win_fs ? 1 : 0);
  xev.xclient.data.l[1] = fullScreen;
  xev.xclient.data.l[2] = 0;

  XSendEvent(display, RootWindow(display,screen), False,
	     SubstructureRedirectMask | SubstructureNotifyMask, &xev);

  XFlush(display);

  if(win_fs){
    owin_width = win_width;
    owin_height = win_height;
    win_width = DisplayWidth(display,screen);
    win_height = DisplayHeight(display,screen);
  }
  else{
    win_width = owin_width;
    win_height = owin_height;
  }
}

void fullscreen_toggle()
{
  win_fs = !win_fs;

  apply_win_fs();

  free_win_cr();
  create_win_cr();
  win_resize = 1;
  calculatewin();
}
