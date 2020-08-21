#include <cairo/cairo.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "warning.h"
#include "directories.h"
#include "script.h"
#include "menuscript.h"
#include "draw.h"
#include "menudraw.h"
#include "infowindow.h"


// C_add_line(x1, y1, x2, y3) - prida usecku podle zadanych souradnic
static int menuscript_add_line(lua_State *L)
{
  menu_line *line;

  line = (menu_line *)mymalloc(sizeof(menu_line));
  line->next = first_menu_line;
  first_menu_line = line;

  line->x1 = luaL_checknumber(L, 1);
  line->y1 = luaL_checknumber(L, 2);
  line->x2 = luaL_checknumber(L, 3);
  line->y2 = luaL_checknumber(L, 4);

  return 0;
}

// C_add_node(x, y, codename) - zalozi nove kolecko, zatim hnede (DISABLED)
static int menuscript_add_node(lua_State *L)
{
  menu_node *node;

  node = (menu_node *)mymalloc(sizeof(menu_node));
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

  img_change |= CHANGE_ALL;
  return 1;
}

static int menuscript_enable_node(lua_State *L) // prebarvi kolecko na modro
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);

  if(node->state == NODE_DISABLED) node->state = NODE_ENABLED;
  else warning("Double enabled node %s.", node->codename);

  img_change |= CHANGE_ALL;
  return 0;
}

static int menuscript_solved_node(lua_State *L) // prebarvi kolecko na zluto
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);

  if(node->state == NODE_ENABLED) node->state = NODE_SOLVED;
  else if(node->state == NODE_DISABLED)
    warning("Disabled node can't be solved %s.", node->codename);
  else warning("Double solved node %s.", node->codename);

  img_change |= CHANGE_ALL;
  return 0;
}

static int menuscript_set_level_name(lua_State *L) // C_set_level_name(node, "Name of this level")
    /* Nastavi titulek mistnosti -- napis zobrazovany vlevo nahore pri najeti mysi pripadne
       pocty tahu jsou jiz v tomto stringu zahrnuty */
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);
  if(node->name) free(node->name);
  node->name = strdup(luaL_checklstring(L, 2, NULL));

  return 0;
}

static int menuscript_load_level_icon(lua_State *L)
      // C_load_level_icon(node, "data/levels/tutor/icon.png")
{
  menu_node *node = (menu_node *)my_luaL_checkludata(L, 1);

  node->icon_name = strdup(luaL_checklstring(L, 2, NULL));
  node->icon = cairo_image_surface_create_from_png(node->icon_name);

  /*
    V pripade, ze se obrazek nacist nepodarilo, vratim do polozky icon NULL
    Ovsem v icon_name zustane cesta k onomu souboru, ktery se nepovedlo nacist.
    To proto, abych v okamziku, kdy budu vedet, jak mistnost vypada, mohl do daneho
    souboru nahled ulozit.
   */
  if(cairo_surface_status(node->icon) != CAIRO_STATUS_SUCCESS){
    cairo_surface_destroy(node->icon);
    node->icon = NULL;
  }

  return 0;
}

static lua_State *menu_luastat; // spusteny LUA skript pro menu

void menu_initlua()
{
  int err;

  first_menu_node = NULL; // inicializace spojovych seznamu
  active_menu_node = NULL;
  first_menu_line = NULL;

  menu_luastat = luaL_newstate(); // vytvoreni skriptu
  luaL_openlibs(menu_luastat);

  lua_register(menu_luastat, "C_add_line", menuscript_add_line);
  lua_register(menu_luastat, "C_add_node", menuscript_add_node);
  lua_register(menu_luastat, "C_enable_node", menuscript_enable_node);
  lua_register(menu_luastat, "C_solved_node", menuscript_solved_node);
  lua_register(menu_luastat, "C_set_level_name", menuscript_set_level_name);
  lua_register(menu_luastat, "C_load_level_icon", menuscript_load_level_icon);
  lua_register(menu_luastat, "C_infowindow", menuscript_infowindow);

  setluadirs(menu_luastat); // predam skriptu C_homedir a C_datadir

  err = luaL_dofile(menu_luastat, datafile("scripts/menuscript.lua")); // nactu LUA soubor
  if(err){
    const char *message = lua_tostring(menu_luastat, -1);
    error("LUA: %s\n", message);
    lua_pop(menu_luastat, 1);
  }
}

/*
  Situace s funkcemi nazvu "solved_node" je trochu slozitejsi. V okamziku vyreseni
  mistnosti je zavolana nasledujici funkce "menu_solved_node". Ta udela pouze to,
  ze zavola funkci LUA skriptu "script_solved_node" se stejnymi parametry. Skript
  provede vice operaci: porovnani se sini slavy, pripadne odkryti novych vetvi,
  ... Mimo jine ovsem taky prebarvi to vyresene kolecko na zluto -- funkci
  menuscript_solved_node(), ktera se ovsem ve skriptu jmenuje C_solved_node().
 */
void menu_solved_node(menu_node *n, int m) // n = vyresena mistnost, m = pocet tahu tohoto reseni
{
  lua_getfield(menu_luastat, LUA_GLOBALSINDEX, "script_solved_node");
  lua_pushstring(menu_luastat, n->codename);
  lua_pushinteger (menu_luastat, m);
  lua_call(menu_luastat, 2, 0);
}
