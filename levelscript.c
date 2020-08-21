#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include "warning.h"
#include "script.h"
#include "object.h"
#include "rules.h"
#include "layers.h"
#include "gsaves.h"
#include "levelscript.h"
#include "main.h"

int script_set_codename(lua_State *L)
{
  const char *codename = luaL_checkstring(L, 1);

  if(room_codename) free(room_codename);
  room_codename = strdup(codename);

  return 0;
}

int script_openpng(lua_State *L)
{
  const char *filename = luaL_checkstring(L, 1);

  cairo_surface_t *surf, *tmpsurf;
  cairo_t *cr;
  unsigned char *data;
  int width, height, datalength;

  surf = cairo_image_surface_create_from_png (filename);

  if(cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
    error("Opening map \"%s\" failed: %s",
	  filename, cairo_status_to_string(cairo_surface_status(surf)));

  width = cairo_image_surface_get_width(surf);
  height = cairo_image_surface_get_height(surf);

  if(cairo_image_surface_get_format(surf) != CAIRO_FORMAT_ARGB32){
    tmpsurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    cr = cairo_create(tmpsurf);
    cairo_set_source_surface (cr, surf, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    surf = tmpsurf;
  }

  data = cairo_image_surface_get_data(surf);
  datalength = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width) * height;

  lua_pushlstring(L, (char *)data, datalength);

  cairo_surface_destroy(surf);

  lua_pushinteger(L, width);
  lua_pushinteger(L, height);

  return 3;
}

void open_level(char *codename)
{
  room_codename = NULL;
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_open_level");
  lua_pushstring(luastat, codename);
  lua_call(luastat, 1, 0);

  finish_objects();
}

void open_user_level()
{
  room_codename = NULL;
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_open_user_level");
  lua_pushstring(luastat, userlev);
  lua_call(luastat, 1, 0);

  finish_objects();
}

void end_level()
{
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_end_level");
  lua_call(luastat, 0, 0);
  delete_layers();
  delete_objects();
  delete_gsaves();
  if(room_codename) free(room_codename);
}

void refresh_user_level()
{
  if(!userlev) return;

  end_level();
  open_user_level(userlev);
}

int script_flip_object(lua_State *L)
{
  ff_object *obj = (ff_object *)my_luaL_checkludata(L, 1);

  obj->gflip = obj->flip = 1-obj->flip;

  return 0;
}

int script_start_active_fish(lua_State *L)
{
  start_active_fish = (ff_object *)my_luaL_checkludata(L, 1);
  return 0;
}
