#include<cairo/cairo.h>
#include "script.h"
#include "object.h"
#include "moves.h"
#include "draw.h"
#include "layers.h"
#include "imgsave.h"
#include "gsaves.h"

cairo_surface_t *imgsave(char *filename, int width, int height, char draw_tick, char draw_moves)
{
  char movestring[50];
  cairo_text_extents_t text_ext;
  cairo_t *cr;

  cairo_surface_t *surf =
    cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height); // hlavni obrazek
  cairo_surface_t *surf2 =
    cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
       // vrstva s poctem tahu a fajfkou

  if(draw_tick || draw_moves) cr = cairo_create(surf2);

  // zelena fajfka

  if(draw_tick){
    cairo_save(cr);
    cairo_translate(cr, ((float)width)/2, ((float)height/2));

    cairo_move_to(cr, -26,  -7);
    cairo_line_to(cr,  -6,  25);
    cairo_line_to(cr,  34, -25);

    cairo_set_line_width(cr, 9);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_source_rgb (cr, 0, 1, 0);
    cairo_stroke(cr);

    cairo_restore(cr);
  }

  // pocet tahu

  if(draw_moves){
    cairo_save(cr);

    sprintf(movestring, "%d", moves);
    cairo_set_font_size (cr, 20);
    cairo_text_extents(cr, movestring, &text_ext);
    cairo_move_to(cr, width-text_ext.width-text_ext.x_bearing-5,
		  height-text_ext.height-text_ext.y_bearing-5);
    cairo_text_path(cr, movestring);

    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_set_line_width(cr, 2);
    cairo_stroke_preserve(cr);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_fill(cr);

    cairo_restore(cr);
  }

  if(draw_tick || draw_moves) cairo_destroy(cr);

  // zmensena mistnost

  cr = cairo_create(surf);

  cairo_save(cr);

  cairo_set_source(cr, bg_pattern); // pozadi
  cairo_paint(cr);
  cairo_scale(cr, ((float)width)/room_width,
	      ((float)height)/room_height);
  draw_layers_noanim(cr); // zdi, predmety, ryby, ...

  cairo_restore(cr);

  // pridam vrstvu s fajfkou a tahy

  if(draw_tick || draw_moves){
    cairo_set_source_surface(cr, surf2, 0, 0);
    cairo_paint_with_alpha(cr, 0.7); // bude trochu pruhledna
    cairo_surface_destroy(surf2);
  }

  cairo_destroy(cr);

  // ulozeni do PNG

  cairo_surface_write_to_png(surf, filename);

  return surf;
}
