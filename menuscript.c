#include <cairo/cairo.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "warning.h"
#include "directories.h"
#include "script.h"
#include "menuscript.h"
#include "menudraw.h"

static int menuscript_add_line(lua_State *L)
{
  menu_line *line;

  line = (menu_line *)malloc(sizeof(menu_line));
  line->next = first_menu_line;
  first_menu_line = line;

  line->x1 = luaL_checknumber(L, 1);
  line->y1 = luaL_checknumber(L, 2);
  line->x2 = luaL_checknumber(L, 3);
  line->y2 = luaL_checknumber(L, 4);

  return 0;
}

static int menuscript_add_node(lua_State *L)
{
  menu_node *node;

  node = (menu_node *)malloc(sizeof(menu_node));
  node->next = first_menu_node;
  first_menu_node = node;

  node->x = luaL_checknumber(L, 1);
  node->y = luaL_checknumber(L, 2);
  node->name = NULL;
  node->codename = strdup(luaL_checklstring(L, 3, NULL));
  node->state = NODE_DISABLED;
  node->icon = NULL;
  node->icon_name = NULL;

  lua_pushlightuserdata(L, node);

  return 1;
}

static int menuscript_enable_node(lua_State *L)
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);

  if(node->state == NODE_DISABLED) node->state = NODE_ENABLED;
  else warning("Double enabled node %s.", node->codename);

  return 0;
}

static int menuscript_solved_node(lua_State *L)
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);

  if(node->state == NODE_ENABLED) node->state = NODE_SOLVED;
  else if(node->state == NODE_DISABLED)
    warning("Disabled node can't be solved %s.", node->codename);
  else warning("Double solved node %s.", node->codename);

  return 0;
}

static int menuscript_set_level_name(lua_State *L)
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);
  if(node->name) free(node->name);
  node->name = strdup(luaL_checklstring(L, 2, NULL));

  return 0;
}

static int menuscript_load_level_icon(lua_State *L)
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);

  node->icon_name = strdup(luaL_checklstring(L, 2, NULL));
  node->icon = cairo_image_surface_create_from_png(node->icon_name);

  if(cairo_surface_status(node->icon) != CAIRO_STATUS_SUCCESS)
    node->icon = NULL;

  return 0;
}

void menu_initlua()
{
  int err;

  first_menu_node = NULL;
  active_menu_node = NULL;
  first_menu_line = NULL;

  menu_luastat = luaL_newstate();
  luaL_openlibs(menu_luastat);

  lua_register(menu_luastat, "C_add_line", menuscript_add_line);
  lua_register(menu_luastat, "C_add_node", menuscript_add_node);
  lua_register(menu_luastat, "C_enable_node", menuscript_enable_node);
  lua_register(menu_luastat, "C_solved_node", menuscript_solved_node);
  lua_register(menu_luastat, "C_set_level_name", menuscript_set_level_name);
  lua_register(menu_luastat, "C_load_level_icon", menuscript_load_level_icon);

  setluastring(menu_luastat, "C_datadir", datafile(""));
  if(homedir) setluastring(menu_luastat, "C_homedir", homedir);

  err = luaL_dofile(menu_luastat, datafile("scripts/menuscript.lua"));
  if(err){
    const char *message = lua_tostring(menu_luastat, -1);
    error("LUA: %s\n", message);
    lua_pop(menu_luastat, 1);
  }
}

void menu_solved_node(menu_node *n)
{
  if(n->state == NODE_SOLVED) return;

  lua_getfield(menu_luastat, LUA_GLOBALSINDEX, "script_solved_node");
  lua_pushstring(menu_luastat, n->codename);
  lua_call(menu_luastat, 1, 0);

  menu_draw();
}
