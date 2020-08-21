#include <cairo/cairo.h>
#include <math.h>
#include "X.h"
#include "window.h"
#include "script.h"
#include "menuscript.h"
#include "directories.h"
#include "draw.h"
#include "object.h"
#include "imgsave.h"
#include "menudraw.h"

static cairo_pattern_t *menu_bg_pattern = NULL; // hnede pozadi mapy

static void draw_levelinfo() // vykresli nazev a nahled oznacene mistnosti
{
  int B1 = 15; // tmavy okraj u okraje okna
  int B2 = 20; // tmavy okraj jinde

  int text_width, text_height; // rozmery nazvu
  int img_width, img_height;  //  rozmery obrazku

  cairo_text_extents_t text_ext;

  if(!active_menu_node->name && !active_menu_node->icon) return;  // neni, co vykreslit

  cairo_save(win_cr);

  // velikost nahledu je zavisla na velikosti mapy
  cairo_scale(win_cr, win_height/900.0, win_height/900.0);

  if(active_menu_node->name){ // nazev je k dispozici
    cairo_set_font_size(win_cr, 60);
    cairo_text_extents(win_cr, active_menu_node->name, &text_ext);
    text_width = text_ext.x_advance;
    text_height = text_ext.height + text_ext.y_advance;
  }
  else{ // nazev vykreslovan nebude
    text_width = 0;
    text_height = -B2;
  }

  if(active_menu_node->icon){ // nahled je k dispozici
    img_width = cairo_image_surface_get_width(active_menu_node->icon);
    img_height = cairo_image_surface_get_height(active_menu_node->icon);
  }
  else img_height = img_width = -B2; // obrazek vykreslovan nebude

  // prepocitani rozmeru textu, pro tmave pozadi
  if(text_width < img_width) text_width = img_width; // text nebude mit mensi ramecek nez nahled
  else if(text_width < img_width+B2) text_width = img_width+B2; // aby se vesel obloucek

  // kresleni tmaveho podkladu
  cairo_move_to(win_cr, 0, 0);
  cairo_line_to(win_cr, B1+text_width+B2, 0);
  if(text_width > img_width){
    cairo_line_to(win_cr, B1+text_width+B2, B1+text_height);
    cairo_arc(win_cr, B1+text_width, B1+text_height, B2, 0, M_PI/2);
  }
  if(active_menu_node->icon){
    cairo_line_to(win_cr, B1+img_width+B2, B1+text_height+B2);
    cairo_line_to(win_cr, B1+img_width+B2, B1+text_height+B2+img_height);
    cairo_arc(win_cr, B1+img_width, B1+text_height+B2+img_height, B2, 0, M_PI/2);
  }
  cairo_line_to(win_cr, 0, B1+text_height+B2+img_height+B2);
  cairo_close_path(win_cr);
  cairo_set_source_rgba(win_cr, 0, 0, 0, 0.5); // vybarveni tmaveho podkladu
  cairo_fill(win_cr);

  if(active_menu_node->name){ // vykresleni nazvu
    cairo_move_to(win_cr, B1, B1+text_ext.height);
    cairo_set_source_rgb(win_cr, 1, 1, 1);
    cairo_show_text(win_cr, active_menu_node->name);
  }

  if(active_menu_node->icon){ // vykresleni nahledu
    cairo_set_source_surface(win_cr, active_menu_node->icon,
			     B1, B1+text_height+B2);
    cairo_paint(win_cr);
  }

  cairo_restore(win_cr);
}

void menu_create_icon() // doplni nahled, pokud chybi (pri zapnute mistnosti)
{
  int width, height; // rozmery nahledu
  int size = 500; // obvod nahledu

  if(!active_menu_node->icon_name || active_menu_node->icon) return;

  width = size*room_width/(room_width+room_height);
  height = size*room_height/(room_width+room_height);

  active_menu_node->icon = imgsave(active_menu_node->icon_name, width, height, 0, 0);
  // ulozim do prislusneho menu_node a soucasne do spravneho souboru
}

void menu_draw() // vykresli mapu mistnosti
{
  cairo_surface_t *s;
  menu_line *l;
  menu_node *n;

  if(!img_change) return;

  if(!menu_bg_pattern){ // nacteni pozadi (pokud jeste nebylo)
    s = cairo_image_surface_create_from_png (datafile("gimages/menu_bg.png"));
    menu_bg_pattern = cairo_pattern_create_for_surface(s);
    cairo_pattern_set_extend(menu_bg_pattern, CAIRO_EXTEND_REPEAT);
    cairo_surface_destroy(s);
  }

  cairo_save(win_cr);

  // vykreslene pozadi
  cairo_set_source(win_cr, menu_bg_pattern);
  cairo_paint(win_cr);

  // transformace souradnic
  cairo_translate(win_cr, win_width, 0);
  cairo_scale(win_cr, -((float)win_height)/MENU_HEIGHT,
	      ((float)win_height)/MENU_HEIGHT);

  for(l = first_menu_line; l; l = l->next){ // vykresleni car mezi kolecky
    cairo_move_to(win_cr, l->x1, l->y1);
    cairo_line_to(win_cr, l->x2, l->y2);
  }
  cairo_set_source_rgb(win_cr, 1, 0.7, 0);
  cairo_set_line_width(win_cr, 5);
  cairo_stroke(win_cr);

  // vykresleni kolecek
  for(n=first_menu_node; n; n=n->next){
    cairo_arc (win_cr, n->x, n->y, 12, 0, 2*M_PI); // cerne obtazeni kolecka
    cairo_set_source_rgb(win_cr, 0, 0, 0);
    cairo_fill(win_cr);

    cairo_arc (win_cr, n->x, n->y, 10, 0, 2*M_PI); // barevne kolecko
    if(n->state == NODE_DISABLED) cairo_set_source_rgb(win_cr, 0.7, 0.5, 0); // hnede
    if(n->state == NODE_ENABLED){
      if(n == active_menu_node) cairo_set_source_rgb(win_cr, 0.5, 0.5, 1); // oznacene modre
      else cairo_set_source_rgb(win_cr, 0, 0, 1); // modre
    }
    if(n->state == NODE_SOLVED){
      if(n == active_menu_node) cairo_set_source_rgb(win_cr, 1, 1, 0.7); // oznacene zlute
      else cairo_set_source_rgb(win_cr, 1, 1, 0); // zlute
    }
    cairo_fill(win_cr);
  }

  cairo_restore(win_cr); // zpet na puvodni souradnice

  if(active_menu_node) draw_levelinfo(); // nahled a nazev oznacene mistnosti

  if(win_resize){ // zmenu velikosti je treba provest az po vykresleni
    XResizeWindow(display, win, win_width, win_height);
    win_resize = 0;
  }
  else repaint_win(0, 0, win_width, win_height);

  img_change = 0;
}
