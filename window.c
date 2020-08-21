#include <stdlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "X.h"
#include "window.h"
#include "draw.h"
#include "script.h"
#include "levelscript.h"
#include "object.h"
#include "gmoves.h"
#include "menuevents.h"
#include "window.h"
#include "gsaves.h"
#include "warning.h"
#include "main.h"

char win_resize = 0;
cairo_t *win_cr = NULL;
static Pixmap win_pixmap; /* pixmapa, na kterou se ve skutecnosti
			     kresli, aby neblikalo okno */

static void apply_win_fs();

int asprintf(char **strp, const char *fmt, ...);

void createwin()
{
  int width, height;

  if(win_fs){ /* zapinam-li hru ve fullscreenu, dam oknu win spravnou velikost, ale
		 hlavnimu oknu dam ze zacatku puvodni velikost, abych nematl WM */
    owin_width = win_width;
    owin_height = win_height;
    width = DisplayWidth(display,screen);
    height = DisplayHeight(display,screen);
  }
  else{
    width = win_width;
    height = win_height;
  }

  real_win = XCreateSimpleWindow(display, RootWindow(display,screen),
			    0, 0,
			    win_width, win_height,
			    0, BlackPixel(display,screen), BlackPixel(display,screen));

  win = XCreateSimpleWindow(display, real_win,
			    0, 0,
			    width, height,
			    0, BlackPixel(display,screen), BlackPixel(display,screen));

  XSelectInput (display, real_win, StructureNotifyMask | ButtonPressMask |
		ButtonReleaseMask | PointerMotionMask | KeyPressMask |
		KeyReleaseMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask);

  // Budu umet odchytavat kliknuti na krizek
  wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, real_win, &wmDeleteMessage, 1);

  XMapWindow(display, win);
  XMapWindow(display, real_win);

  if(win_fs) apply_win_fs(); // zaciname ve fullscreenu

  win_width = width;
  win_height = height;
}

/*
  Nasledujici funkce spocita, jak se do zadane oblasti vejde resena mistnost, kdyz vime,
  ze jeji pomer stran je room_width:room_height. Vrati velikost takoveho mistnosti a
  jeji souradnice, aby byla vycentrovana.
 */
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

/*
  Jsou 4 moznosti, jak bude okno usporadane:
    pasecek ulozenych pozic bude a) nahore, b) vlevo
    undo pasecek bude: a) nahore, b) vpravo
  Pritom resena mistnost bude uprostred zbyleho prostoru. Zvolena bude ta moznost,
  ktera dava resene mistnosti nejvetsi velikost. Tento vyber resi nasledujici funkce.
 */
void calculatewin()
{
  int max_width, max_height, max_x, max_y, width, height, x, y;

  img_change |= CHANGE_ALL; // bude treba prekreslovat
  if(menumode) return;  // v menu nic nepocitame

  gsaves_vertical = gmoves_vertical = 1; // saves vlevo, undo vpravo
  gsaves_length = win_height;
  calcularea(win_width - gmoves_width - gsaves_width, win_height, 0, 0,
	     &max_width, &max_height, &max_x, &max_y);
  max_x += gsaves_width;

  // saves vlevo, undo nahore
  calcularea(win_width - gsaves_width, win_height - gmoves_height, 0, 0,
	     &width, &height, &x, &y);

  if(width+height >= max_width+max_height){
    gsaves_vertical = 1; gmoves_vertical = 0;
    gsaves_length = win_height-gmoves_height;
    max_width = width;
    max_height = height;
    max_x = x+gsaves_width;
    max_y = y;
  }

  // saves nahore, undo vpravo
  calcularea(win_width - gmoves_width, win_height - gsaves_height, 0, 0,
	     &width, &height, &x, &y);

  if(width+height >= max_width+max_height){
    gsaves_vertical = 0; gmoves_vertical = 1;
    gsaves_length = win_width-gmoves_width;
    max_width = width;
    max_height = height;
    max_x = x;
    max_y = y+gsaves_height;
  }

  // saves nahore, undo dole
  calcularea(win_width, win_height - gmoves_height - gsaves_height, 0, 0,
	     &width, &height, &x, &y);

  if(width+height >= max_width+max_height){
    gsaves_vertical = gmoves_vertical = 0;
    gsaves_length = win_width;
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
    calculate_gsaves(); // prepocita umisteni a velikost ulozenych pozic
    gsaves_pointer(1, 0);
  }
}

void create_win_cr() // vyrobi pro okno spravne velkou pixmapu a cairo kontext
{
  win_pixmap = XCreatePixmap (display, win, win_width, win_height, DefaultDepth (display, screen));
  XSetWindowBackgroundPixmap(display, win, win_pixmap);

  win_surf = cairo_xlib_surface_create (display, win_pixmap, DefaultVisual(display, screen),
					win_width, win_height);

  win_cr = cairo_create(win_surf);
}

void free_win_cr()  // smaze pixmapu a cairo kontext (pri zmene velikosti)
{
  XSetWindowBackground(display, win, BlackPixel(display,screen));
  cairo_surface_destroy(win_surf);
  cairo_destroy(win_cr);
  XFreePixmap (display, win_pixmap);
}

// prekresli pixmapu do okna na zadanem obdelniku
void repaint_win(int x, int y, int width, int height) 
{
  XClearArea (display, win, x, y, width, height, 0);

  XFlush(display);
}

static void apply_win_fs()
  // Pozada WM o fullscreen, volano z fullscreen_toggle() a createwin()
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
}

void fullscreen_toggle() // zapne/vypne fullscreen
{
  win_fs = !win_fs; // ono prepnuti

  apply_win_fs(); // zmena okna

  if(win_fs){
    owin_width = win_width;   // ulozim puvodni velikost
    owin_height = win_height;
    win_width = DisplayWidth(display,screen);
    win_height = DisplayHeight(display,screen);
  }
  else{
    win_width = owin_width;
    win_height = owin_height;
  }

  free_win_cr(); // Zmena velikosti
  create_win_cr();
  win_resize = 1; // zmenime velikost win, jen, co vykreslime spravne velkou Pixmapu
  calculatewin();
}

void update_window_title()
  // nastavi titulek okna: Fish Fillets Clone[ - level '$codename']
{
  char *title, *iconname;

  if(room_codename){
    asprintf(&title, "Fish Fillets Clone -- level '%s'", room_codename);
    iconname = room_codename;
  }
  else{
    title = "Fish Fillets Clone";
    iconname = "ff-clone";
  }

  XSetStandardProperties(display, real_win, title, iconname, None,
			 gargv, gargc, NULL);

  if(room_codename) free(title);
}
