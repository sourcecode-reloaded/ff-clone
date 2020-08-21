#include <cairo/cairo.h>
#include "X.h"
#include "script.h"
#include "window.h"
#include "menudraw.h"
#include "menuscript.h"
#include "menuevents.h"
#include "keyboard.h"
#include "loop.h"
#include "draw.h"
#include "levelscript.h"

static menu_node *last_active = NULL; // drzene tlacitko

void menu_click() /* pri kliknuti se nestane nic slozitejsiho, nez ze si
		     zapamatuji kolecko, na ktere se kliklo */
{
  last_active = active_menu_node;
}

void menu_unclick() // pri pusteni mysi
{
  last_active = NULL;

  if(active_menu_node){ // je-li mys nad drzenym tlacitkem,
    menumode = 0;
    open_level(active_menu_node->codename); // spustim level
  }
  else menu_pointer(mouse_x, mouse_y); // jinak se podivam, ktere tlacitko je pod mysi
}

void menu_pointer() // pri pohybu mysi
{
  float x, y, bestdist, dist;
  menu_node *node, *n;

  x = mouse_x;
  y = mouse_y;

  // transformace souradnic v okne na menu souradnice
  x  -= win_width;
  x *= -MENU_HEIGHT; y *= MENU_HEIGHT;
  x /= win_height; y /= win_height;

  /* Projdu vsechna kolecka a najdu to nejblizsi mysi. Druha mocnina
     dosud nejkratsi vzdalenosti je uchovavana v promenne bestdist,
     kolecko, kteremu tato vzdalenost prislusi je node. */

  node = NULL;     // zadne kolecko nebude aktivni,
  bestdist = 1000; // je-li mys od kazdeho dal nez sqrt(1000)

  for(n = first_menu_node; n; n = n->next){
    if(n->state == NODE_DISABLED) continue; // hneda kolecka preskakuji
    dist = (x-n->x)*(x-n->x) + (y-n->y)*(y-n->y);
    if(bestdist > dist){
      node = n;
      bestdist = dist;
    }
  }

  // je-li drzene nejake tlacitko, zadne jine nesmi byt aktivni
  if(mouse_pressed && node != last_active) node = NULL;

  if(node != active_menu_node) img_change |= CHANGE_ALL;

  active_menu_node = node;
}
