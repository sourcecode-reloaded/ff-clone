#include <stdio.h>
#include <stdlib.h>
#include <cairo/cairo.h>
#include "script.h"
#include "directories.h"
#include "layers.h"
#include "gener.h"

static void drawborder(cairo_t *cr, int width, int height, const char *data,
		  int borderwidth)
{
  int i;

  for(i=0; i<width*height; i++)
    if(data[i]){
      cairo_save(cr);
      cairo_translate(cr, (i%width)*SQUARE_WIDTH, (i/width)*SQUARE_HEIGHT);

      if(!(i < width*(height-1) && data[i+width]))
	cairo_rectangle(cr, 0, SQUARE_HEIGHT-borderwidth, SQUARE_WIDTH, borderwidth);
      if(!(i >= width && data[i-width]))
	cairo_rectangle(cr, 0, 0, SQUARE_WIDTH, borderwidth);
      if(!((i+1) % width && data[i+1]))
	cairo_rectangle(cr, SQUARE_WIDTH-borderwidth, 0, borderwidth, SQUARE_HEIGHT);
      if(!(i % width && data[i-1]))
	cairo_rectangle(cr, 0, 0, borderwidth, SQUARE_HEIGHT);
      if(!(i >= width && i % width && data[i-width-1]))
	cairo_rectangle(cr, 0, 0, borderwidth, borderwidth);
      if(!(i >= width && (i+1) % width && data[i-width+1]))
	cairo_rectangle(cr, SQUARE_WIDTH-borderwidth, 0, borderwidth, borderwidth);
      if(!(i < width*(height-1) && i % width  && data[i+width-1]))
	cairo_rectangle(cr, 0, SQUARE_HEIGHT-borderwidth, borderwidth, borderwidth);
      if(!(i < width*(height-1) && (i+1) % width && data[i+width+1]))
	cairo_rectangle(cr, SQUARE_WIDTH-borderwidth, SQUARE_HEIGHT-borderwidth, borderwidth, borderwidth);

      cairo_restore(cr);
    }
  cairo_fill(cr);
}

static void drawarea(cairo_t *cr, int width, int height, const char *data)
{
  int i;

  for(i=0; i<width*height; i++)
    if(data[i])
      cairo_rectangle(cr, (i%width)*SQUARE_WIDTH, (i/width)*SQUARE_HEIGHT, SQUARE_WIDTH, SQUARE_HEIGHT);

  cairo_fill(cr);
}

static cairo_pattern_t *background = NULL;

ff_image *generwall(int width, int height, const char *data)
{
  cairo_surface_t *result;
  cairo_t *cr;
  cairo_surface_t *s;

  if(!background){
    s = cairo_image_surface_create_from_png (datafile("gimages/background.png"));
    background = cairo_pattern_create_for_surface(s);
    cairo_pattern_set_extend(background, CAIRO_EXTEND_REPEAT);
    cairo_surface_destroy(s);
  }

  result = cairo_image_surface_create
    (CAIRO_FORMAT_ARGB32, width*SQUARE_WIDTH, height*SQUARE_HEIGHT);
  cr = cairo_create(result);

  cairo_set_source(cr, background);
  drawarea(cr, width, height, data);

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  drawborder(cr, width, height, data, 4);

  cairo_destroy(cr);

  return new_image(result, 0, 0, 1.0/SQUARE_WIDTH, 1.0/SQUARE_HEIGHT);
}

static cairo_surface_t *steel = NULL;

ff_image *genersteel(int width, int height, const char *data)
{
  cairo_surface_t *result;
  cairo_t *cr;
  int i, surfx, surfy;

  if(!steel) steel = cairo_image_surface_create_from_png (datafile("gimages/steel.png"));

  result = cairo_image_surface_create
    (CAIRO_FORMAT_ARGB32, width*SQUARE_WIDTH, height*SQUARE_HEIGHT);
  cr = cairo_create(result);

  for(i=0; i<width*height; i++)
    if(data[i]){
      surfx = surfy = 0;
      if(i < width*(height-1) && data[i+width]) surfx += 1;
      if(i >= width && data[i-width]) surfx += 2;
      if((i+1) % width && data[i+1]) surfy +=1;
      if(i % width && data[i-1]) surfy +=2;

      cairo_set_source_surface(cr, steel, (i%width - surfx)*SQUARE_WIDTH, (i/width - surfy)*SQUARE_HEIGHT);
      cairo_rectangle(cr, (i%width)*SQUARE_WIDTH, (i/width)*SQUARE_HEIGHT, SQUARE_WIDTH, SQUARE_HEIGHT);
      cairo_fill(cr);
    }

  cairo_destroy(cr);

  return new_image(result, 0, 0, 1.0/SQUARE_WIDTH, 1.0/SQUARE_HEIGHT);
}

ff_image *genernormal(int width, int height, const char *data)
{
  cairo_surface_t *result;
  cairo_t *cr;

  result = cairo_image_surface_create
    (CAIRO_FORMAT_ARGB32, width*SQUARE_WIDTH, height*SQUARE_HEIGHT);
  cr = cairo_create(result);

  cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
  drawarea(cr, width, height, data);
  cairo_set_source_rgb(cr, 0.7, 0.7, 0.0);
  drawborder(cr, width, height, data, 4);

  cairo_destroy(cr);

  return new_image(result, 0, 0, 1.0/SQUARE_WIDTH, 1.0/SQUARE_HEIGHT);
}

int script_generwall(lua_State *L)
{
  int  width = luaL_checkinteger(L, 1);
  int height = luaL_checkinteger(L, 2);
  const char *data = luaL_checklstring(L, 3, NULL);

  lua_pushlightuserdata(L, generwall(width, height, data));

  return 1;
}

int script_genersteel(lua_State *L)
{
  int  width = luaL_checkinteger(L, 1);
  int height = luaL_checkinteger(L, 2);
  const char *data = luaL_checklstring(L, 3, NULL);

  lua_pushlightuserdata(L, genersteel(width, height, data));

  return 1;
}

int script_genernormal(lua_State *L)
{
  int  width = luaL_checkinteger(L, 1);
  int height = luaL_checkinteger(L, 2);
  const char *data = luaL_checklstring(L, 3, NULL);

  lua_pushlightuserdata(L, genernormal(width, height, data));

  return 1;
}
