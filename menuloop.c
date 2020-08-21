#include <cairo/cairo.h>
#include "X.h"
#include "window.h"
#include "menudraw.h"
#include "menuscript.h"
#include "menuloop.h"
#include "keyboard.h"
#include "loop.h"
#include "draw.h"
#include "levelscript.h"

static menu_node *last_active = NULL;

void menu_click()
{
  last_active = active_menu_node;
}

void menu_unclick()
{
  if(!active_menu_node) menu_pointer(mouse_x, mouse_y);
  else{
    last_active = NULL;
    open_level(active_menu_node->codename);
    menumode = 0;
    calculatewin();
  }
}

void menu_pointer()
{
  float x, y, bestdist, dist;
  menu_node *node, *n;

  x = mouse_x;
  y = mouse_y;

  x  -= win_width;
  x *= -300; y *= 300;
  x /= win_height; y /= win_height;
  node = NULL;
  bestdist = 1000;
  for(n = first_menu_node; n; n = n->next){
    if(n->state == NODE_DISABLED) continue;
    dist = (x-n->x)*(x-n->x) + (y-n->y)*(y-n->y);
    if(bestdist > dist){
      node = n;
      bestdist = dist;
    }
  }
  if(mouse_pressed && node != last_active) node = NULL;

  if(node != active_menu_node) img_change |= CHANGE_ALL;

  active_menu_node = node;
}
