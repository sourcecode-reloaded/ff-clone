#include<stdlib.h>
#include<cairo/cairo.h>
#include "script.h"
#include "warning.h"
#include "X.h"
#include "window.h"
#include "rules.h"
#include "draw.h"
#include "moves.h"
#include "gmoves.h"
#include "gsaves.h"
#include "object.h"
#include "keyboard.h"
#include "imgsave.h"
#include "menudraw.h"
#include "main.h"

ff_object *first_object = NULL;

ff_object *new_object(const char *color, char type, int x, int y,
		      int width, int height, const char *data)
{
  ff_object *result;
  int i;

  result = (ff_object *)mymalloc(sizeof(ff_object));
  result->type = type;
  result->c.x = x;
  result->c.y = y;
  result->width = width;
  result->height = height;
  result->gflip = result->flip = 0;
  result->data = (char *)mymalloc(sizeof(char)*width*height);
  for(i=0; i<width*height; i++) result->data[i] = data[i];
  for(i=0; i<4; i++) result->color[i] = color[i];

  result->next = first_object;
  first_object = result;

  return result;
}

int script_new_object(lua_State *L)
{
  const char *color = luaL_checklstring(L, 1, NULL);
  char  type = luaL_checkinteger(L, 2);
  int      x = luaL_checkinteger(L, 3);
  int      y = luaL_checkinteger(L, 4);
  int  width = luaL_checkinteger(L, 5);
  int height = luaL_checkinteger(L, 6);
  const char *data = luaL_checklstring(L, 7, NULL);

  lua_pushlightuserdata(L, new_object(color, type, x, y, width, height, data));

  return 1;
}

int script_setroomsize(lua_State *L)
{
  room_width  = luaL_checkinteger(L, 1);
  room_height = luaL_checkinteger(L, 2);

  return 0;
}

static void analyzeobject(ff_object *obj)
{
  int i, x, y;
  char state;

  for(i=0; i<4; i++) obj->dirnum[i] = 0;

  for(y=0; y<obj->height; y++){
    state = 0;
    for(x=0; x<obj->width; x++){
      if(obj->data[x+y*obj->width]){
	if(!state){
	  obj->dirnum[LEFT]++;
	  obj->dirnum[RIGHT]++;
	  state = 1;
	}
      }
      else state = 0;
    }
  }
  for(x=0; x<obj->width; x++){
    state = 0;
    for(y=0; y<obj->height; y++){
      if(obj->data[x+y*obj->width]){
	if(!state){
	  obj->dirnum[UP]++;
	  obj->dirnum[DOWN]++;
	  state = 1;
	}
      }
      else state = 0;
    }
  }

  obj->dirsquares[UP] = mymalloc(sizeof(coor)*obj->dirnum[UP]);
  obj->dirsquares[DOWN] = mymalloc(sizeof(coor)*obj->dirnum[DOWN]);
  obj->dirsquares[LEFT] = mymalloc(sizeof(coor)*obj->dirnum[LEFT]);
  obj->dirsquares[RIGHT] = mymalloc(sizeof(coor)*obj->dirnum[RIGHT]);

  i = 0;
  for(y=0; y<obj->height; y++){
    state = 0;
    for(x=0; x<obj->width; x++){
      if(obj->data[x+y*obj->width]){
	if(!state){
	  obj->dirsquares[LEFT][i] = coord(x-1, y);
	  state = 1;
	  i++;
	}
      }
      else state = 0;
    }
  }
  i = 0;
  for(y=0; y<obj->height; y++){
    state = 0;
    for(x=obj->width-1; x>=0; x--){
      if(obj->data[x+y*obj->width]){
	if(!state){
	  obj->dirsquares[RIGHT][i] = coord(x+1, y);
	  state = 1;
	  i++;
	}
      }
      else state = 0;
    }
  }
  i = 0;
  for(x=0; x<obj->width; x++){
    state = 0;
    for(y=0; y<obj->height; y++){
      if(obj->data[x+y*obj->width]){
	if(!state){
	  obj->dirsquares[UP][i] = coord(x, y-1);
	  state = 1;
	  i++;
	}
      }
      else state = 0;
    }
  }
  i = 0;
  for(x=0; x<obj->width; x++){
    state = 0;
    for(y=obj->height-1; y>=0; y--){
      if(obj->data[x+y*obj->width]){
	if(!state){
	  obj->dirsquares[DOWN][i] = coord(x, y+1);
	  state = 1;
	  i++;
	}
      }
      else state = 0;
    }
  }
}

void finish_objects()
{
  int i, j;
  ff_object *obj;

  objnum = 0;
  for(obj = first_object; obj; obj = obj->next){
    analyzeobject(obj);
    objnum++;
  }

  for(i=0; i<NUMOBJLISTS; i++)
    objlist[i] = (ff_object **)mymalloc(objnum*sizeof(ff_object *));

  for(i=0, obj=first_object; obj; i++, obj=obj->next)
    for(j=0; j<NUMOBJLISTS; j++){
      objlist[j][i] = obj;
      obj->index[j] = i;
    }

  level_anim_init();
  level_keys_init();
  level_gmoves_init();
  level_gsaves_init();
  init_rules();
  init_moves();
  unanim_fish_rectangle();
  calculatewin();
  if(!userlev) menu_create_icon();
}

void swapobject(ff_object *obj1, int index2, int list)
{
  objlist[list][obj1->index[list]] = objlist[list][index2];
  objlist[list][index2]->index[list] = obj1->index[list];

  obj1->index[list] = index2;
  objlist[list][index2] = obj1;
}

int backdir(int dir)
{
  return (dir+2)%4;
}

coor coord(int x, int y)
{
  coor result;
  result.x = x;
  result.y = y;
  return result;
}

coor coorsum(coor c1, coor c2)
{
  coor result;
  result.x = c1.x+c2.x;
  result.y = c1.y+c2.y;
  return result;
}

void delete_objects()
{
  ff_object *obj;
  int i;

  free_rules();
  free_moves();

  while(first_object){
    obj = first_object;
    first_object = first_object->next;

    for(i=0; i<4; i++) if(obj->dirsquares[i]) free(obj->dirsquares[i]);
    if(obj->data) free(obj->data);
    free(obj);
  }
  for(i=0; i<NUMOBJLISTS; i++) free(objlist[i]);
}
