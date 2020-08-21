#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <X11/keysym.h>
#include "X.h"
#include "window.h"
#include "keyboard.h"
#include "script.h"
#include "loop.h"
#include "infowindow.h"

#define FONT_SIZE    20 // velikost fontu
#define BORDER       10 // vzdalenost mezi textem a krajem okna
#define BASELINESKIP 30 // vzdalenost radku

static char iw_on = 0; // promenna, ktera rika, ze infowindow je otevrene (neda se otevrit dvakrat)

void infowindow(const char *str, char helpmode)
{
  cairo_t *cr;
  cairo_surface_t *surf;
  cairo_text_extents_t text_ext;
  Window w;
  int x, y;
  Pixmap pxm;
  int width, curwidth, height;
  double fheight, firstheight;
  int start, i;
  XSizeHints size_hints;
  XEvent xev;
  int len;
  long event_mask;
  KeySym ks;
  char b;

  if(iw_on) return;
  iw_on = 1;

  char *dup = strdup(str); /* vytvorim kopii stringu, kde '\n' nahradim nulami, abych
			      ziskal vlastne vice oddelenych radku (protoze cairo vykresli
			      vzdy jen jeden radek) */

  surf = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);
  // pomocna surface, abych mohl vytvorit cairo context, z ktereho vydoluji velokost textu

  cr = cairo_create(surf);
  cairo_surface_destroy(surf);
  cairo_set_font_size(cr, FONT_SIZE);

  // spocitam rozmery okna, k fheight postupne pricitam radky a hledam maximalni width
  len = width = fheight = 0;
  for(i=0; !len; i++){ // cyklus ukoncim v okamziku, kdy dojdu na konec -- zjistim delku
    start = i;
    for(; dup[i] && dup[i] != '\n'; i++);
    if(dup[i] == '\n') dup[i] = 0; // nahradim '\n' nulou
    else len = i+1;

    cairo_text_extents(cr, &dup[start], &text_ext); // ziskam rozmery

    curwidth = ceil(BORDER*2 + text_ext.x_advance); // sirka tohoto radku
    if(curwidth > width) width = curwidth;

    if(!fheight){ // na zacatku
      firstheight = fheight = BORDER - text_ext.y_bearing;
    }
    if(!len) fheight += BASELINESKIP;
    else fheight += text_ext.y_bearing+text_ext.height + BORDER; // na konci
  }
  height = ceil(fheight);
  cairo_destroy(cr);

  // ziskam souradnice hlavniho okna
  XTranslateCoordinates(display, real_win, RootWindow(display,screen), 0, 0,
			&x, &y, &w);
  // vycentruji infowindow
  x += (win_width-width)/2; y += (win_height-height)/2;

  // vyrobim infowindow
  w = XCreateSimpleWindow(display, RootWindow(display,screen),
			  x, y,
			  width, height,
			  0, BlackPixel(display,screen), BlackPixel(display,screen));

  size_hints.flags = PSize | PMinSize | PMaxSize; // fixace velikosti okna
  size_hints.width  = size_hints.min_width  = size_hints.max_width  = width;
  size_hints.height = size_hints.min_height = size_hints.max_height = height;
  XSetWMNormalHints(display, w, &size_hints);

  XSetWMProtocols(display, w, &wmDeleteMessage, 1); // zavirani krizkem

  // pri zobrazovani napovedy akceptuje klavesy, jinak jen kliknuti
  event_mask = ButtonReleaseMask | ButtonPressMask;
  if(helpmode) event_mask |= KeyReleaseMask | KeyPressMask | FocusChangeMask;
  XSelectInput(display, w, event_mask);

  // pixmapa, na kterou bude nakreslen text
  pxm = XCreatePixmap(display, w, width, height, DefaultDepth(display, screen));
  XSetWindowBackgroundPixmap(display, w, pxm);
  surf = cairo_xlib_surface_create (display, pxm, DefaultVisual(display, screen),
				    width, height);

  cr = cairo_create(surf);
  cairo_set_source_rgb(cr, 0.86, 0.85, 0.84); // sede pozadi
  cairo_paint(cr);
  cairo_set_source_rgb(cr, 0, 0, 0); // text
  cairo_set_font_size(cr, FONT_SIZE);
  cairo_translate(cr, BORDER, firstheight);
  for(i=0; i < len; i++){
    start = i;
    for(; dup[i]; i++);
    cairo_move_to(cr, 0, 0);
    cairo_show_text(cr, &dup[start]); // kresleny po radkach
    cairo_translate(cr, 0, BASELINESKIP);
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surf);

  XMapWindow(display, w); // zobrazeni okna
  XMoveWindow(display, w, x, y); // jeste jednou vycentruji

  for(;;){
    XPeekEvent(display, &xev); // meni-li se velikost hlavniho okna, vypinam a prenechavam udalost
    if(xev.type == ConfigureNotify && !xev.xconfigure.send_event) break;
    XNextEvent(display, &xev);
    if(xev.xany.window == w &&  // zavreni krizkem nebo kliknutim
       (xev.type == ButtonRelease || xev.type == ClientMessage)) break;
    if(helpmode){ // pri zobrazene napovede
      b = 0;
      if(xev.type == FocusIn) key_remap();
      else if(xev.type == KeyPress){
	XLookupString (&xev.xkey, NULL, 0, &ks, NULL);
	if(ks == XK_Escape) break; // pri escape zaviram jen infowindow, ne celou hru
	b = key_press(xev.xkey);
      }
      else if(xev.type == KeyRelease) key_release(xev.xkey);
      if(b) break; // se zavre, ma-li stisknuta klavesa efekt
    }
  }
  // vsechno zase pozaviram
  XDestroyWindow(display, w);
  XFreePixmap(display, pxm);
  key_remap();
  loop_noskip();

  free(dup);

  iw_on = 0;
}

int menuscript_infowindow(lua_State *L) // LUA verze infowindow, neumoznuje helpmode
{
  const char *str = luaL_checkstring(L, 1);

  infowindow(str, 0);

  return 0;
}

void show_help() // zobrazi okno s napovedou
{
  infowindow(
"arrows:   move active fish\n\
space:   change active fish\n\
\n\
F1:   This help\n\
F2:   Save position\n\
F3:   Load position\n\
F11:   Fullscreen\n\
F12:   Safe mode\n\
Escape:   Exit level / game\n\
- / +:   Undo / redo\n\
PgUp / PgDown:   Undo / redo by 100 moves\n\
Home / End:   Start / End of undo history\n\
'g' / 'G':   Grid to background / foreground\n\
Ctrl-r:   Refresh user level\n\
\n\
See http://www.olsak.net/mirek/ff-clone/manual_en.html for more info.\n\
\n\
    Click to close", 1);
}
