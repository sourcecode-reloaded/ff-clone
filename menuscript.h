#include <lualib.h>
#include <lauxlib.h>

typedef struct menu_node{
  double x, y;
  char *codename, *name;
  enum {NODE_DISABLED, NODE_ENABLED, NODE_SOLVED} state;
  cairo_surface_t *icon;
  char *icon_name;
  struct menu_node *next;
} menu_node;
menu_node *first_menu_node;
menu_node *active_menu_node;

typedef struct menu_line{
  double x1, y1, x2, y2;
  struct menu_line *next;
} menu_line;
menu_line *first_menu_line;

lua_State *menu_luastat;

void menu_initlua();
void menu_endlua();
void menu_solved_node(menu_node *n);
