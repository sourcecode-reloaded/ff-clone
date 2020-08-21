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
                     // nastavi ve skriptu do daneho nazvu promenne dane cislo.
{
  lua_pushinteger(L, value);
  lua_setglobal(L, name);
}

static void setluastring(lua_State *L, char *name, char *str)
                     // nastavi ve skriptu do daneho nazvu promenne dany string.
{
  lua_pushstring(L, str);
  lua_setglobal(L, name);
}

void setluadirs(lua_State *L)
             // Nastavi promenne C_datadir a C_homedir (vyuzivano i v menuscript)
{
  setluastring(L, "C_datadir", datafile(""));
  if(homedir) setluastring(L, "C_homedir", homedir);
}

void *my_luaL_checkludata(lua_State *L, int narg)
   /* V klasicke luaxlib funkce, ktera otestuje ludata, chybi.
      Pouziti stejne jako ostatni luaL_check... */
{
  luaL_checktype(L, narg, LUA_TLIGHTUSERDATA);
  return lua_touserdata(L, narg);
}

void initlua() // inicializuje luastat
{
  int error;

  luastat = luaL_newstate();
  luaL_openlibs(luastat);  // nacteni zakladnich funkci

  // predani C-ckovych funkci skriptu
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

  // predani hodnot typu objektu skriptu
  setluaint(luastat, "C_SMALL", SMALL);
  setluaint(luastat, "C_BIG", BIG);
  setluaint(luastat, "C_SOLID", SOLID);
  setluaint(luastat, "C_STEEL", STEEL);
  setluaint(luastat, "C_LIGHT", LIGHT);

  setluadirs(luastat); // predani cest k adresarum

  error = luaL_dofile(luastat, datafile("scripts/levelscript.lua")); // nacteni hlavniho souboru
  if(error){
    const char *message = lua_tostring(luastat, -1);
    warning("LUA: %s\n", message);
    lua_pop(luastat, 1);
  }
}
