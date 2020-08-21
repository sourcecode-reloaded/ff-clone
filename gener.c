#include <stdio.h>
#include <stdlib.h>
#include <cairo/cairo.h>
#include "warning.h"
#include "script.h"
#include "directories.h"
#include "layers.h"
#include "gener.h"

/*
  Zdi a lehke predmety jsou vykreslovany velmi podobne. Nejjprve je nejakym patternem vykreslena
  plocha uvnitr (zdi strakate, predmety svetle zlute), a pak je obrys predmetu obtahnut nejakou
  barvou (zdi cerne, predmety tmave zlute). Nasledujici dve funkce nedy predpokladaji, ze je
  nejaka barva jiz nastavena.
 */
static void drawborder(cairo_t *cr, int width, int height, const char *data,
		       int borderwidth) // vykresli okraj tvaru predmetu
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

static void drawarea(cairo_t *cr, int width, int height, const char *data) // vykresli vypln predmetu
{
  int i;

  for(i=0; i<width*height; i++)
    if(data[i])
      cairo_rectangle(cr, (i%width)*SQUARE_WIDTH, (i/width)*SQUARE_HEIGHT, SQUARE_WIDTH, SQUARE_HEIGHT);

  cairo_fill(cr);
}

static cairo_pattern_t *foreground = NULL; // pattern pro plochu zdi

ff_image *generwall(int width, int height, const char *data) // vytvori obrazek pro objekt zdi
{
  cairo_surface_t *result;
  cairo_t *cr;

  cairo_surface_t *s; // jen za ucelem nacteni foreground

  if(!foreground){ // nacte foreground, pokud to jeste neudelal
    s = cairo_image_surface_create_from_png (datafile("gimages/foreground.png"));
    foreground = cairo_pattern_create_for_surface(s);
    cairo_pattern_set_extend(foreground, CAIRO_EXTEND_REPEAT);
    cairo_surface_destroy(s);
    if(cairo_pattern_status(foreground) != CAIRO_STATUS_SUCCESS)
      warning("Creating foreground pattern failed");
  }

  result = cairo_image_surface_create // zalozi obrazek spravnych rozmeru
    (CAIRO_FORMAT_ARGB32, width*SQUARE_WIDTH, height*SQUARE_HEIGHT);
  cr = cairo_create(result);

  cairo_set_source(cr, foreground); // vyplni vnitrek
  drawarea(cr, width, height, data);

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // nakresli okraj
  drawborder(cr, width, height, data, 4);

  cairo_destroy(cr);

  return new_image(result, 0, 0, 1.0/SQUARE_WIDTH, 1.0/SQUARE_HEIGHT);
}

/*
  Ocel je generovana odlisne od zdi a lehkych predmetu. Zakladem je obrazek
  gimages/steel.png, ktery se ulozi do promenne steel. Jsou to
  4x4 policka, kde jsou nakreslena vsechna mozna policka ocelovych trubek. Radky jsou
  rozdeleny podle toho, kam ma trubka vest ve vodorovnem smeru a sloupce podle toho,
  jak ma vest ve svislem smeru.
 */
static cairo_surface_t *steel = NULL;

ff_image *genersteel(int width, int height, const char *data)
{
  cairo_surface_t *result;
  cairo_t *cr;
  int i, surfx, surfy;

  if(!steel){
    steel = cairo_image_surface_create_from_png (datafile("gimages/steel.png"));
    if(cairo_surface_status(steel) != CAIRO_STATUS_SUCCESS)
      warning("Creating steel surface failed.");
  }

  result = cairo_image_surface_create  // zalozi obrazek spravnych rozmeru
    (CAIRO_FORMAT_ARGB32, width*SQUARE_WIDTH, height*SQUARE_HEIGHT);
  cr = cairo_create(result);

  for(i=0; i<width*height; i++)
    if(data[i]){
      surfx = surfy = 0;
      if(i < width*(height-1) && data[i+width]) surfx += 1; // prozkouma sousedy
      if(i >= width && data[i-width]) surfx += 2;
      if((i+1) % width && data[i+1]) surfy +=1;
      if(i % width && data[i-1]) surfy +=2;

      // zkopiruje policko do obrazku
      cairo_set_source_surface(cr, steel, (i%width - surfx)*SQUARE_WIDTH, (i/width - surfy)*SQUARE_HEIGHT);
      cairo_rectangle(cr, (i%width)*SQUARE_WIDTH, (i/width)*SQUARE_HEIGHT, SQUARE_WIDTH, SQUARE_HEIGHT);
      cairo_fill(cr);
    }

  cairo_destroy(cr);

  return new_image(result, 0, 0, 1.0/SQUARE_WIDTH, 1.0/SQUARE_HEIGHT);
}

ff_image *genernormal(int width, int height, const char *data) // vytvori obrazek pro lehky objekt
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

// lua varianty funkci vyse
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
