#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include "warning.h"
#include "X.h"
#include "window.h"
#include "script.h"
#include "object.h"
#include "rules.h"
#include "layers.h"
#include "gsaves.h"
#include "levelscript.h"
#include "main.h"
#include "draw.h"
#include "keyboard.h"
#include "gmoves.h"
#include "moves.h"
#include "menudraw.h"

char *room_codename = NULL;

int script_set_codename(lua_State *L) // nastavi room_codename
{
  const char *codename = luaL_checkstring(L, 1);

  if(room_codename) free(room_codename);
  room_codename = strdup(codename);
  update_window_title();

  return 0;
}

int script_openpng(lua_State *L) // otevre PNG soubor a skriptu preda jeho pixely
{
  const char *filename = luaL_checkstring(L, 1);

  cairo_surface_t *surf;
  unsigned char *data;
  int datalength;
  int width, height;

  cairo_surface_t *tmpsurf; // kdyby nevysel format
  cairo_t *cr;

  surf = cairo_image_surface_create_from_png (filename); // nacteni obrazku
  if(cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
    error("Opening map \"%s\" failed: %s",
	  filename, cairo_status_to_string(cairo_surface_status(surf)));

  width = cairo_image_surface_get_width(surf);   // ziskani velikosti
  height = cairo_image_surface_get_height(surf);

  if(cairo_image_surface_get_format(surf) != CAIRO_FORMAT_ARGB32){
    /*
      Jejda, format surface ziskany z PNG souboru neodpovida formatu, v jakem
      chci pixely predat. Je treba vyrobit tedy surface o spravnem formatu
      (CAIRO_FORMAT_ARGB32), do ktere obrazek zkopiruji.
     */
    tmpsurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    cr = cairo_create(tmpsurf); // kopiruji
    cairo_set_source_surface (cr, surf, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr); // rusim puvodni surface
    cairo_surface_destroy(surf);
    surf = tmpsurf;
  }

  data = cairo_image_surface_get_data(surf); // ziskam pixely
  datalength = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width) * height; // a delku pole

  lua_pushlstring(L, (char *)data, datalength); // predam lue

  cairo_surface_destroy(surf); // spolu se surface se z pameti uvolni i pole data, lua ma nastesi kopii

  lua_pushinteger(L, width); // predam vysku a sirku
  lua_pushinteger(L, height);

  return 3; // vracim 3 parametry
}

static void run_level() // spusti level nezavisle na tom, zda je uzivatelsky nebo ne
{
  level_anim_init();
  level_keys_init();
  level_gmoves_init();
  level_gsaves_init();

  finish_objects();
  init_rules();
  init_moves();

  unanim_fish_rectangle();

  calculatewin();
  if(!userlev) menu_create_icon();  // kdyby mi chybel nahled v mape mistnosti, tak ho vytvorim
}

void open_level(char *codename) /* Otevre mistnost z menu. Jedine, co vlastne dela je,
				   ze spusti stejnojmennou lua funkci. */
{
  room_codename = NULL;
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_open_level");
  lua_pushstring(luastat, codename);
  lua_call(luastat, 1, 0);
  run_level();
}

void open_user_level() /* Otevre mistnost z menu. Jedine, co vlastne dela je, ze spusti stejnojmennou
			  lua funkci s parametrem userlev (tam je cesta, kterou uzivatel zadal). */
{
  room_codename = NULL;
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_open_user_level");
  lua_pushstring(luastat, userlev);
  lua_call(luastat, 1, 0);
  run_level();
}

void end_level() // ukonci level
{
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_end_level"); // zavola stejnojmennou lua funkci
  lua_call(luastat, 0, 0);

  delete_layers(); // a rusi dalsi struktury
  free_rules();
  free_moves();
  delete_objects();
  delete_gsaves();

  if(room_codename){ // Z titulku okna zas zmizi "level '$codename'"
    free(room_codename);
    room_codename = NULL;
    update_window_title();
  }
}

void refresh_user_level() // obnovi uzivatelsky level
{
  if(!userlev) return;

  end_level();
  open_user_level();
}

int script_flip_object(lua_State *L) // zmeni flip u zadaneho objektu
{
  ff_object *obj = (ff_object *)my_luaL_checkludata(L, 1);

  obj->gflip = obj->flip = 1-obj->flip;

  return 0;
}

int script_start_active_fish(lua_State *L) // nastavi rybu aktivni na zacatku
{
  start_active_fish = (ff_object *)my_luaL_checkludata(L, 1);
  return 0;
}
