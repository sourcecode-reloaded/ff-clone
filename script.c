#include <cairo/cairo.h>

#include "warning.h"
#include "directories.h"
#include "script.h"
#include "levelscript.h"
#include "object.h"
#include "gener.h"
#include "layers.h"
#include "draw.h"

static void setluaint(lua_State *L, char *name, int value)
{
  lua_pushinteger(L, value);
  lua_setglobal(L, name);
}

void setluastring(lua_State *L, char *name, char *str)
{
  lua_pushstring(L, str);
  lua_setglobal(L, name);
}

void *my_luaL_checkludata(lua_State *L, int narg)
{
  luaL_checktype(L, narg, LUA_TLIGHTUSERDATA);
  return lua_touserdata(L, narg);
}

void initlua()
{
  int error;

  luastat = luaL_newstate();
  luaL_openlibs(luastat);

  lua_register(luastat, "C_setroomsize", script_setroomsize);
  lua_register(luastat, "C_openpng", script_openpng);
  lua_register(luastat, "C_new_object", script_new_object);
  lua_register(luastat, "C_flip_object", script_flip_object);
  lua_register(luastat, "C_start_active_fish", script_start_active_fish);
  lua_register(luastat, "C_set_codename", script_set_codename);

  lua_register(luastat, "C_new_layer", script_new_layer);
  lua_register(luastat, "C_load_png_image", script_load_png_image);
  lua_register(luastat, "C_setlayerimage", script_setlayerimage);

  lua_register(luastat, "C_generwall", script_generwall);
  lua_register(luastat, "C_genersteel", script_genersteel);
  lua_register(luastat, "C_genernormal", script_genernormal);


  setluaint(luastat, "C_SMALL", SMALL);
  setluaint(luastat, "C_BIG", BIG);
  setluaint(luastat, "C_SOLID", SOLID);
  setluaint(luastat, "C_STEEL", STEEL);
  setluaint(luastat, "C_LIGHT", LIGHT);

  setluastring(luastat, "C_datadir", datafile(""));
  if(homedir) setluastring(luastat, "C_homedir", homedir);

  error = luaL_dofile(luastat, datafile("scripts/levelscript.lua"));
  if(error){
    const char *message = lua_tostring(luastat, -1);
    warning("LUA: %s\n", message);
    lua_pop(luastat, 1);
  }
}

void endlua()
{
  lua_close(luastat);
}
