#include<stdlib.h>
#include<cairo/cairo.h>
#include "warning.h"
#include "script.h"
#include "object.h"
#include "moves.h"
#include "rules.h"
#include "main.h"
#include "menuscript.h"

#define HEAPLIST     0
#define MOVELIST     1
#define OUTLIST      3

#define OUT_NONE     0
#define OUT_PART     1
#define OUT_ENTIRE   2
#define OUT_HIDDEN  -1

static void printfield();
static void printmfield();

/*****************************************************************************/
/*************************   prepinani ryb   *********************************/

static int remainfish, killedfish;

static void nextfish()
{
  if(!active_fish) return;
  active_fish = objlist[FISHLIST][(active_fish->index[FISHLIST]+1)%fishnum];
}

static void availfish()
{
  ff_object *oact;

  if(!active_fish) return;

  oact = active_fish;
  while(!active_fish->live || active_fish->out == OUT_ENTIRE){
    nextfish();
    if(active_fish == oact){
      active_fish = NULL;
      break;
    }
  }
}

void changefish()
{
  nextfish();
  availfish();
}

void setactivefish(struct ff_object *fish)
{
  active_fish = fish;
  availfish();
}

/*************************   prepinani ryb   *********************************/
/*****************************************************************************/
/*********************   souradnice a hraci plan   ***************************/

static int outnum;
static ff_object **field;

void hideobject(ff_object *obj)
{
  int x, y;

  if(obj->out == OUT_PART) swapobject(obj, --outnum, OUTLIST);
  if(obj->out != OUT_ENTIRE && (obj->type == SMALL || obj->type == BIG))
    remainfish--;

  obj->out = OUT_HIDDEN;

  for(x=0; x<obj->width; x++)
    for(y=0; y<obj->height; y++)
      if(obj->data[x+y*obj->width] &&
	 x+obj->c.x >= 0 && x+obj->c.x < room_width &&
	 y+obj->c.y >= 0 && y+obj->c.y < room_height)
	field[(x+obj->c.x) + (y+obj->c.y)*room_width] = NULL;
}

void showobject(ff_object *obj)
{
  int x, y;

  for(x=0; x<obj->width; x++)
    for(y=0; y<obj->height; y++)
      if(obj->data[x+y*obj->width]){
	if(x+obj->c.x >= 0 && x+obj->c.x < room_width &&
	   y+obj->c.y >= 0 && y+obj->c.y < room_height){
	  field[(x+obj->c.x) + (y+obj->c.y)*room_width] = obj;
	  if(obj->out == OUT_HIDDEN) obj->out = OUT_NONE;
	  else if(obj->out == OUT_ENTIRE) obj->out = OUT_PART;
	}
	else{
	  if(obj->out == OUT_HIDDEN) obj->out = OUT_ENTIRE;
	  else if(obj->out == OUT_NONE) obj->out = OUT_PART;
	}
      }
  if(obj->out == OUT_PART) swapobject(obj, outnum++, OUTLIST);
  if(obj->out == OUT_ENTIRE){
    if(obj->live) availfish();
  }
  else if(obj->type == SMALL || obj->type == BIG) remainfish++;
}

void hideallobjects()
{
  ff_object *obj;
  int i;

  for(obj = first_object; obj; obj = obj->next) obj->out = OUT_HIDDEN;
  for(i=0; i<room_width*room_height; i++) field[i] = NULL;
  outnum = 0;
  remainfish = 0;
}

static ff_object *coorobj(coor c)
{
  int i;
  ff_object *obj;

  if(c.x >= 0 && c.x < room_width && c.y >= 0 && c.y < room_height)
    return field[c.x + c.y*room_width];

  for(i=0; i<outnum; i++){
    obj = objlist[OUTLIST][i];
    if(c.x >= obj->c.x && c.x < obj->c.x+obj->width &&
       c.y >= obj->c.y && c.y < obj->c.y+obj->height &&
       obj->data[(c.x-obj->c.x) + (c.y-obj->c.y)*obj->width])
      return obj;
  }

  return NULL;
}

/*********************   souradnice a hraci plan   ***************************/
/*****************************************************************************/
/*********************  formalni zabiti a oziveni  ***************************/

void killfish(ff_object *fish)
{
  fish->gflip = fish->flip;
  fish->live = 0;
  addobjchange(fish, LIVECHANGE);
  availfish();
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_killfish");
  lua_pushlstring(luastat, fish->color, 4);
  lua_call(luastat, 1, 0);
  killedfish++;
}

void vitalizefish(ff_object *fish)
{
  fish->live = 1;
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_vitalizefish");
  lua_pushlstring(luastat, fish->color, 4);
  lua_call(luastat, 1, 0);
  killedfish--;
}

/*********************  formalni zabiti a oziveni  ***************************/
/*****************************************************************************/
/*************************  stavy objektu  ***********************************/


#define MOVING       0
#define NOHEAP       0
#define UNPROCESSED  1
#define STAYING      2
#define SMALLHEAP    2
#define BIGHEAP      3
#define SOLIDHEAP    4

#define MAXSTATE  4

static int statenum[2][MAXSTATE];

static void objstate(ff_object *obj, int state, int list)
{
  int i;

  if(state > 0 && obj->index[list] < statenum[list][state-1]){
    for(i=state-2; i >= 0 && obj->index[list] < statenum[list][i]; i--);
    i++;
    for(; i < state; i++) swapobject(obj, --statenum[list][i], list);
  }
  else if(state < MAXSTATE && obj->index[list] >= statenum[list][state]){
    for(i=state+1; i < MAXSTATE && obj->index[list] >= statenum[list][i]; i++);
    i--;
    for(; i >= state; i--) swapobject(obj, statenum[list][i]++, list);
  }
}


/*************************  stavy objektu  ***********************************/
/*****************************************************************************/
/************************  hromadky na rybach  *******************************/

static char singlepush(ff_object *obj, // strkajici ojekt
		       int dir,        // smer, kterym strka
		       int list,       // MOVELIST nebo HEAPLIST
		       char front,     // brat objekty zepredu (s vetsimi indexy)?
		       int state,      // stav, na ktery se ma objekt nastavit
		       char weak,      // neustrci ocel
		       char kill,      // ustrci rybu
		       int onlystate,  // hromadka, na ktere musi byt
		       int keep)       // index, po ktery se nestrka
{
  int i;
  ff_object *o;
  char result = 0;    // prekazi neco?

  for(i=0; i<obj->dirnum[dir]; i++){
    o = coorobj(coorsum(obj->c, obj->dirsquares[dir][i]));
    if(!o) continue;
    if(front){
      if(state >= MAXSTATE || o->index[list] < statenum[list][state]) continue;
    }
    else{
      if(state <= 0 || o->index[list] >= statenum[list][state-1]) continue;
      if(list == HEAPLIST && o->index[HEAPLIST] >= statenum[HEAPLIST][NOHEAP] &&
	 o->index[HEAPLIST] < statenum[HEAPLIST][UNPROCESSED]) continue;
    }
    if(o->index[list] < keep) continue;
    if((!kill && o->live) || o->type == SOLID || (weak && o->type == STEEL)){
      result = 1;
      continue;
    }
    if(onlystate){
      if(o->index[HEAPLIST] < statenum[HEAPLIST][onlystate-1]) continue;
      if(o->index[HEAPLIST] >= statenum[HEAPLIST][onlystate]) continue;
    }
    objstate(o, state, list);
  }

  return result;
}

static int singleheap(ff_object *obj, int list, int dir)
{
  int i;
  ff_object *o, *omax;

  if(obj->live){
    if(obj->type == SMALL) return SMALLHEAP;
    else if(obj->type == BIG) return BIGHEAP;
    else warning("singleheap: Something wrong ... unknown fish type %d", obj->type);
  }
  else if(obj->type == SOLID) return SOLIDHEAP;

  omax = NULL;
  for(i=0; i<obj->dirnum[dir]; i++){
    o = coorobj(coorsum(obj->c, obj->dirsquares[dir][i]));
    if(!o) continue;
    if(!omax || o->index[list] > omax->index[list]) omax = o;
  }

  if(!omax) return NOHEAP;

  for(i=0; i<MAXSTATE; i++)
    if(omax->index[list] < statenum[list][i]) break;
  return i;
}

static void cropheap(int list, char dirmode, char dangerous, int keep)
{
  ff_object *obj;
  int state, i;
  char weak;
  int dir = dirmode ? room_movedir : DOWN;

  while(statenum[list][NOHEAP] < statenum[list][UNPROCESSED]){
    obj = objlist[list][statenum[list][NOHEAP]];
    state = singleheap(obj, list, dir);
    weak = (list == HEAPLIST && state == SMALLHEAP);
    if(state <= UNPROCESSED || (dangerous && state <= BIGHEAP) ||
       (weak && obj->type == STEEL)){
      statenum[list][NOHEAP]++;
      continue;
    }

    objstate(obj, state, list);

    for(i=statenum[list][state-1]; i >= statenum[list][state-1]; i--){
      obj = objlist[list][i];
      singlepush(obj, UP, list, 0, state, weak, 0, 0, keep);
      if(dirmode)
	singlepush(obj, backdir(room_movedir), list, 0, state, weak, 0, 0, keep);
    }
  }
}

static void expandheap(int list, int onlystate)
{
  int i;

  for(i=statenum[list][NOHEAP]; i<statenum[list][UNPROCESSED]; i++)
    singlepush(objlist[list][i], UP, list, 1, UNPROCESSED, 0, 0, onlystate, 0);
}

static void killpush(char dangerous)
{
  int i, state;
  ff_object *obj;

  for(i=0; i < statenum[HEAPLIST][UNPROCESSED]; i++){
    obj = objlist[HEAPLIST][i];
    if(obj->live){
      killfish(obj);
      state = singleheap(obj, HEAPLIST, DOWN);
      if(state >= SOLIDHEAP || (!dangerous && state >= BIGHEAP)) continue;
    }
    singlepush(obj, DOWN, HEAPLIST, 1, UNPROCESSED, 0, 1, 0, 0);
  }

  expandheap(HEAPLIST, 0);
  cropheap(HEAPLIST, 0, 0, 0);
}

/************************  hromadky na rybach  *******************************/
/*****************************************************************************/
/****************************  bezne hrani  **********************************/

static int superheap;
static int movetype;

void movefish(int dir)
{
  int i;
  int keep;

  if(room_state != ROOM_IDLE) return;
  if(!active_fish) return;

  superheap = statenum[MOVELIST][MOVING] = statenum[MOVELIST][UNPROCESSED] = 0;

  objstate(active_fish, UNPROCESSED, MOVELIST);
  for(i=0; i<statenum[MOVELIST][UNPROCESSED]; i++)
    if(singlepush(objlist[MOVELIST][i], dir, MOVELIST, 1, UNPROCESSED,
		   active_fish->type == SMALL, 0, 0, 0)) break;
  if(i < statenum[MOVELIST][UNPROCESSED]) return;

  keep = statenum[MOVELIST][UNPROCESSED];
  room_movedir = dir;

  if(active_fish->type == SMALL) movetype = SMALLHEAP;
  else movetype = BIGHEAP;

  if(dir == UP) statenum[MOVELIST][MOVING] = statenum[MOVELIST][UNPROCESSED];
  else if(dir == DOWN){
    expandheap(MOVELIST, movetype);
    statenum[MOVELIST][MOVING] = keep;
    if(movetype == BIGHEAP) superheap = statenum[MOVELIST][UNPROCESSED];
    cropheap(MOVELIST, 0, 0, keep);
    if(movetype == SMALLHEAP) superheap = statenum[MOVELIST][UNPROCESSED];
  }
  else{
    expandheap(MOVELIST, 0);
    statenum[MOVELIST][MOVING] = keep;
    superheap = statenum[MOVELIST][UNPROCESSED];
    cropheap(MOVELIST, 0, 0, keep);
    if(active_fish->type == SMALL)
      for(i=0; i<statenum[MOVELIST][MOVING]; i++)
	if(objlist[MOVELIST][i]->type == STEEL) return;
    statenum[MOVELIST][MOVING] = keep;
    cropheap(MOVELIST, 1, 0, keep);
  }

  room_state = ROOM_MOVE;
  startmove(active_fish, dir);

  if(dir == LEFT && active_fish->flip){
    active_fish->flip = 0;
    addobjchange(active_fish, FLIPCHANGE);
  }
  else if(dir == RIGHT && !active_fish->flip){
    active_fish->flip = 1;
    addobjchange(active_fish, FLIPCHANGE);
  }
}

static void move(int list, int dir)
{
  int i;

  for(i=0; i<statenum[list][MOVING]; i++){
    addobjmove(objlist[list][i]);
    hideobject(objlist[list][i]);
    if(dir == RIGHT) objlist[list][i]->c.x++;
    else if(dir == LEFT) objlist[list][i]->c.x--;
    else if(dir == DOWN) objlist[list][i]->c.y++;
    else if(dir == UP) objlist[list][i]->c.y--;
    else warning("move: Something wrong unknown direction %d", room_movedir);
  }
  for(i=0; i<statenum[list][MOVING]; i++) showobject(objlist[list][i]);
}

void room_step()
{
  char dangerous = 0;
  int i;
  ff_object *obj;

  if(room_state == ROOM_IDLE) return;
  else if(room_state == ROOM_MOVE){
    move(MOVELIST, room_movedir);
    if(room_movedir == UP){
      for(i=0; i<statenum[MOVELIST][MOVING]; i++){
	obj = objlist[MOVELIST][i];
	objstate(obj, obj->out == OUT_ENTIRE ? UNPROCESSED : movetype, HEAPLIST);
      }
      expandheap(HEAPLIST, 0);
      for(i=0; i < statenum[MOVELIST][UNPROCESSED]; i++){
	obj = objlist[MOVELIST][i];
	if(obj->out == OUT_ENTIRE) objstate(obj, SOLIDHEAP, HEAPLIST);
      }
    }
    else
      for(i=0; i<superheap; i++){
	obj = objlist[MOVELIST][i];
	objstate(obj, obj->out == OUT_ENTIRE ? SOLID : UNPROCESSED, HEAPLIST);
      }
  }
  else if(room_state == ROOM_FALL){
    dangerous = 1;
    move(HEAPLIST, DOWN);
    statenum[HEAPLIST][MOVING] = 0;
    i = 0;
    while(i < statenum[HEAPLIST][UNPROCESSED]){
      obj = objlist[HEAPLIST][i];
      if(obj->out == OUT_ENTIRE) objstate(obj, SOLIDHEAP, HEAPLIST);
      else i++;
    }
  }

  if(room_state != ROOM_BEGIN) cropheap(HEAPLIST, 0, dangerous, 0);
  killpush(dangerous);

  if(statenum[HEAPLIST][MOVING]){
    room_state = ROOM_FALL;
    room_movedir = DOWN;
  }
  else{
    room_state = ROOM_IDLE;
    finishmove();
  }
}

/****************************  bezne hrani  **********************************/
/*****************************************************************************/
/************************** inicializace atp.  *******************************/

struct ff_object *start_active_fish = NULL;

void init_rules()
{
  int i, smallnum;
  ff_object *obj;

  smallnum = fishnum = 0;
  for(i=0; i<objnum; i++){
    obj = objlist[FISHLIST][i];
    if(obj->type == SMALL || obj->type == BIG){
      obj->live = 1;
      swapobject(obj, fishnum++, FISHLIST);
      if(obj->type == SMALL) swapobject(obj, smallnum++, FISHLIST);
    }
    else obj->live = 0;
  }

  if(!fishnum) error("init_rules: no fish found");

  if(!start_active_fish) start_active_fish = objlist[FISHLIST][0];
  active_fish = start_active_fish;

  killedfish = 0;
  remainfish = fishnum;

  field = (ff_object **)mymalloc(sizeof(ff_object *)*room_width*room_height);

  hideallobjects();

  for(obj = first_object; obj; obj = obj->next) showobject(obj);

  statenum[MOVELIST][STAYING] = objnum;
  fallall();
}

void fallall()
{
  int i;
  ff_object *obj;

  statenum[HEAPLIST][MOVING] = 0;
  statenum[HEAPLIST][UNPROCESSED] = statenum[HEAPLIST][SMALLHEAP] =
    statenum[HEAPLIST][BIGHEAP] = objnum;
  for(i=objnum-1; i >= 0; i--){
    obj = objlist[HEAPLIST][i];
    if(obj->type == SMALL) objstate(obj, SMALLHEAP, HEAPLIST);
    else if(obj->type == BIG) objstate(obj, BIGHEAP, HEAPLIST);
    else if(obj->type == SOLID) objstate(obj, SOLIDHEAP, HEAPLIST);
  }
  cropheap(HEAPLIST, 0, 0, 0);

  if(statenum[HEAPLIST][MOVING]) room_state = ROOM_BEGIN;
  else room_state = ROOM_IDLE;
}

void findfishheap()
{
  int i;

  statenum[HEAPLIST][SMALLHEAP] = statenum[HEAPLIST][BIGHEAP] = 0;
  for(i=0; i<fishnum; i++)
    if(objlist[FISHLIST][i]->live)
      objstate(objlist[FISHLIST][i], UNPROCESSED, HEAPLIST);
  expandheap(HEAPLIST, 0);
  cropheap(HEAPLIST, 0, 0, 0);
}

void free_rules()
{
  start_active_fish = NULL;
  free(field);
}

/************************** inicializace atp.  *******************************/
/*****************************************************************************/
/*******************  jednoduche verejne testovaci fce  **********************/

char ismoving(ff_object *obj)
{
  int list;

  if(room_state == ROOM_IDLE || room_state == ROOM_BEGIN) return 0;
  if(room_state == ROOM_MOVE) list = MOVELIST;
  else if(room_state == ROOM_FALL) list = HEAPLIST;

  return obj->index[list] < statenum[list][MOVING];
}

char isonfish(ff_object *obj)
{
  return obj->index[HEAPLIST] >= statenum[HEAPLIST][UNPROCESSED] &&
    obj->index[HEAPLIST] < statenum[HEAPLIST][BIGHEAP];
}

char issolved()
{
  return !killedfish && !remainfish;
}

/*******************  jednoduche verejne testovaci fce  **********************/
/*****************************************************************************/
/*************************  pozvolne otaceni ryb  ****************************/

#define FLIPMOVE 0.1

void animflip()
{
  int i;

  for(i=0; i<fishnum; i++)
    if(objlist[FISHLIST][i] && objlist[FISHLIST][i]->gflip != objlist[FISHLIST][i]->flip){
      if(objlist[FISHLIST][i]->flip){
	objlist[FISHLIST][i]->gflip += FLIPMOVE;
	if(objlist[FISHLIST][i]->gflip > 1)
	  objlist[FISHLIST][i]->gflip = objlist[FISHLIST][i]->flip;
      }
      else{
	objlist[FISHLIST][i]->gflip -= FLIPMOVE;
	if(objlist[FISHLIST][i]->gflip < 0)
	  objlist[FISHLIST][i]->gflip = objlist[FISHLIST][i]->flip;
      }
    }
}

void unanimflip()
{
  int i;
  for(i=0; i<fishnum; i++)
    if(objlist[FISHLIST][i])
      objlist[FISHLIST][i]->gflip = objlist[FISHLIST][i]->flip;
}

/*************************  pozvolne otaceni ryb  ****************************/
/*****************************************************************************/
/****************************  ladici tisky  *********************************/

static void printfield()
{
  int i;

  printf("\n\n");
  for(i=0; i < room_width*room_height; i++){
    if(!field[i]) printf(" ");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][NOHEAP]) printf("V");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][UNPROCESSED]) printf("?");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][SMALLHEAP]) printf("x");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][BIGHEAP]) printf("X");
    else printf("#");

    if((i+1)%room_width == 0) printf("\n");
  }
  printf("\n\n");
}

static void printmfield()
{
  int i;

  printf("\n\n");
  for(i=0; i < room_width*room_height; i++){
    if(!field[i]) printf(" ");
    else if(field[i]->index[MOVELIST] < statenum[MOVELIST][NOHEAP]) printf("M");
    else if(field[i]->index[MOVELIST] < statenum[MOVELIST][UNPROCESSED]) printf("?");
    else if(field[i]->index[MOVELIST] < statenum[MOVELIST][STAYING]) printf("#");
    else printf("!");

    if((i+1)%room_width == 0) printf("\n");
  }
  printf("\n\n");
}
