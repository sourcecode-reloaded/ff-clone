#include <cairo/cairo.h>
#include <math.h>
#include "X.h"
#include "window.h"
#include "menuscript.h"
#include "directories.h"
#include "draw.h"
#include "object.h"
#include "imgsave.h"
#include "menudraw.h"

static cairo_pattern_t *menu_bg_pattern = NULL;

static void draw_levelinfo()
{
  int B1 = 15;
  int B2 = 20;
  int text_width, text_height, img_width, img_height;
  cairo_text_extents_t text_ext;

  if(!active_menu_node->name && !active_menu_node->icon) return;

  cairo_save(win_cr);

  cairo_scale(win_cr, surf_height/900.0, surf_height/900.0);

  if(active_menu_node->name){
    cairo_set_font_size(win_cr, 60);
    cairo_text_extents(win_cr, active_menu_node->name, &text_ext);
    text_width = text_ext.x_advance;
    text_height = text_ext.height + text_ext.y_advance;
  }
  else{
    text_width = 0;
    text_height = -B2;
  }

  if(active_menu_node->icon){
    img_width = cairo_image_surface_get_width(active_menu_node->icon);
    img_height = cairo_image_surface_get_height(active_menu_node->icon);
  }
  else img_height = img_width = -B2;

  if(text_width < img_width) text_width = img_width;
  else if(text_width < img_width+B2) text_width = img_width+B2;

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
  cairo_set_source_rgba(win_cr, 0, 0, 0, 0.5);
  cairo_fill(win_cr);

  cairo_move_to(win_cr, B1, B1+text_ext.height);
  cairo_set_source_rgb(win_cr, 1, 1, 1);

  if(active_menu_node->name)
    cairo_show_text(win_cr, active_menu_node->name);

  if(active_menu_node->icon){
    cairo_set_source_surface(win_cr, active_menu_node->icon,
			     B1, B1+text_height+B2);
    cairo_paint(win_cr);
  }

  cairo_restore(win_cr);
}

void menu_create_icon()
{
  int width, height;
  int size = 500;

  if(!active_menu_node->icon_name || active_menu_node->icon) return;

  width = size*room_width/(room_width+room_height);
  height = size*room_height/(room_width+room_height);

  active_menu_node->icon = imgsave(active_menu_node->icon_name, width, height, 0, 0);
}

void menu_draw()
{
  cairo_surface_t *s;
  menu_line *l;
  menu_node *n;

  if(!img_change) return;

  if(!menu_bg_pattern){
    s = cairo_image_surface_create_from_png (datafile("gimages/menu_bg.png"));
    menu_bg_pattern = cairo_pattern_create_for_surface(s);
    cairo_pattern_set_extend(menu_bg_pattern, CAIRO_EXTEND_REPEAT);
    cairo_surface_destroy(s);
  }

  cairo_save(win_cr);
  cairo_set_source(win_cr, menu_bg_pattern);
  cairo_paint(win_cr);

  cairo_translate(win_cr, surf_width, 0);
  cairo_scale(win_cr, -surf_height/300.0, surf_height/300.0);

  for(l = first_menu_line; l; l = l->next){
    cairo_move_to(win_cr, l->x1, l->y1);
    cairo_line_to(win_cr, l->x2, l->y2);
  }

  cairo_set_source_rgb(win_cr, 1, 0.7, 0);
  cairo_set_line_width(win_cr, 5);
  cairo_stroke(win_cr);

  for(n=first_menu_node; n; n=n->next){
    cairo_arc (win_cr, n->x, n->y, 12, 0, 2*M_PI);
    cairo_set_source_rgb(win_cr, 0, 0, 0);
    cairo_fill(win_cr);
    cairo_arc (win_cr, n->x, n->y, 10, 0, 2*M_PI);
    if(n->state == NODE_DISABLED) cairo_set_source_rgb(win_cr, 0.7, 0.5, 0);
    if(n->state == NODE_ENABLED){
      if(n == active_menu_node) cairo_set_source_rgb(win_cr, 0.5, 0.5, 1);
      else cairo_set_source_rgb(win_cr, 0, 0, 1);
    }
    if(n->state == NODE_SOLVED){
      if(n == active_menu_node) cairo_set_source_rgb(win_cr, 1, 1, 0.7);
      else cairo_set_source_rgb(win_cr, 1, 1, 0);
    }
    cairo_fill(win_cr);
  }
  cairo_restore(win_cr);

  if(active_menu_node) draw_levelinfo();
  if(win_resize) XResizeWindow(display, win, win_width, win_height);

  repaint_win(0, 0, win_width, win_height);

  img_change = 0;
}
