#include<stdlib.h>
#include<cairo/cairo.h>

#include "warning.h"
#include "script.h"
#include "object.h"
#include "rules.h"
#include "moves.h"
#include "gsaves.h"
#include "draw.h"
#include "menuloop.h"
#include "main.h"

#define MOVESBLOCK 100

#define MOVEDLIST  4

static int maxchanges;

struct movedsct{
  ff_object *obj;
  coor oric;
};

struct changesct{
  ff_object *obj;
  char type;
};

struct possct{
  coor c;
  char flip, live;
};

struct movesblock{
  struct{
    int movednum;
    struct movedsct *moved;

    int changednum;
    struct changesct *changed;

    char dir;
    ff_object *fish;
  } move[MOVESBLOCK];

  struct possct *saved;

  struct movesblock *next;
  struct movesblock *prev;
};

struct{
  int movednum;
  struct movedsct *moved;

  int changednum;
  struct changesct *changed;
} beforestart;

struct movesblock *first, *last, *current;

static char nosave;

static void freeredo()
{
  int i;
  struct movesblock *mb;

  mb = current;
  for(i=moves; i<maxmoves; i++){
    if(mb->move[i%MOVESBLOCK].moved) free(mb->move[i%MOVESBLOCK].moved);
    if(mb->move[i%MOVESBLOCK].changed) free(mb->move[i%MOVESBLOCK].changed);

    if((i+1)%MOVESBLOCK == 0){
      if(mb != current){
	free(mb->saved);
	free(mb);
      }
      mb = mb->next;
    }
  }
  if(mb != current){
    free(mb->saved);
    free(mb);
    last = current;
  }
  maxmoves = moves;
}

static void undoto(int move)
{
  int i;

  while(moves > move){
    if(moves == 0){
      for(i=0; i<beforestart.movednum; i++){
	hideobject(beforestart.moved[i].obj);
	beforestart.moved[i].obj->c = beforestart.moved[i].oric;
      }
      for(i=0; i<beforestart.movednum; i++){
	showobject(beforestart.moved[i].obj);
      }
      for(i=0; i<beforestart.changednum; i++){
	if(beforestart.changed[i].type == FLIPCHANGE)
	  beforestart.changed[i].obj->flip =
	    beforestart.changed[i].obj->flip;
	else if(beforestart.changed[i].type == LIVECHANGE)
	  vitalizefish(beforestart.changed[i].obj);
	else warning("undoto: unknown change type %d",
		     beforestart.changed[i].type);
      }
      moves = -1;
      return;
    }

    if((moves--)%MOVESBLOCK == 0) current = current->prev;

    for(i=0; i<current->move[moves%MOVESBLOCK].movednum; i++){
      hideobject(current->move[moves%MOVESBLOCK].moved[i].obj);
      current->move[moves%MOVESBLOCK].moved[i].obj->c =
	current->move[moves%MOVESBLOCK].moved[i].oric;
    }
    for(i=0; i<current->move[moves%MOVESBLOCK].movednum; i++)
      showobject(current->move[moves%MOVESBLOCK].moved[i].obj);

    for(i=0; i<current->move[moves%MOVESBLOCK].changednum; i++){
      if(current->move[moves%MOVESBLOCK].changed[i].type == FLIPCHANGE)
	current->move[moves%MOVESBLOCK].changed[i].obj->flip =
	  1-current->move[moves%MOVESBLOCK].changed[i].obj->flip;
      else if(current->move[moves%MOVESBLOCK].changed[i].type == LIVECHANGE)
	vitalizefish(current->move[moves%MOVESBLOCK].changed[i].obj);
      else warning("undoto: unknown change type %d",
		   current->move[moves%MOVESBLOCK].changed[i].type);
    }
  }
}

static void redoto(int move)
{
  while(moves < move){
    setactivefish(current->move[moves%MOVESBLOCK].fish);
    movefish(current->move[moves%MOVESBLOCK].dir);
    while(room_state != ROOM_IDLE) room_step();
  }
}

static void loadpos(int block)
{
  int i;
  ff_object *obj;

  if(block <= abs(block-moves/MOVESBLOCK) && block <= abs(block-maxmoves/MOVESBLOCK)){
    current = first;
    i = 0;
  }
  else if(abs(block-moves/MOVESBLOCK) <= abs(block-maxmoves/MOVESBLOCK))
    i = moves/MOVESBLOCK;
  else{
    current = last;
    i = maxmoves/MOVESBLOCK;
  }

  while(i < block){
    current = current->next;
    i++;
  }
  while(i > block){
    current = current->prev;
    i--;
  }

  hideallobjects();
  i=0;
  for(obj = first_object; obj; obj = obj->next){
    obj->c = current->saved[i].c;
    obj->flip = current->saved[i].flip;
    if(obj->live && !current->saved[i].live) killfish(obj);
    else if(!obj->live && current->saved[i].live) vitalizefish(obj);
    showobject(obj);
    i++;
  }

  moves = block*MOVESBLOCK;
}

static void savepos()
{
  int i;
  ff_object *obj;

  current->saved = (struct possct *)mymalloc(objnum*sizeof(struct possct));
  i=0;
  for(obj = first_object; obj; obj = obj->next){
    current->saved[i].c = obj->c;
    current->saved[i].flip = obj->flip;
    current->saved[i].live = obj->live;
    i++;
  }
}

static int movednum;
static int changednum;
static coor *oric;
static struct changesct *changed;

static void createfirstblock()
{
  first = current = last = (struct movesblock *)mymalloc(sizeof(struct movesblock));
  savepos();
}

void finishmove()
{
  int i;
  int index = (moves-1)%MOVESBLOCK;

  if(!nosave){
    if(!current){
      beforestart.changednum = changednum;
      beforestart.movednum = movednum;

      if(movednum) beforestart.moved = (struct movedsct *)mymalloc(movednum*sizeof(struct movedsct));
      else beforestart.moved = NULL;

      if(changednum) beforestart.changed = (struct changesct *)mymalloc(changednum*sizeof(struct changesct));
      else beforestart.changed = NULL;

      for(i=0; i<movednum; i++){
	beforestart.moved[i].obj = objlist[MOVEDLIST][i];
	beforestart.moved[i].oric = oric[i];
      }
      for(i=0; i<changednum; i++){
	beforestart.changed[i].obj = changed[i].obj;
	beforestart.changed[i].type = changed[i].type;
      }

      createfirstblock();
      moves = 0;
      img_change |= CHANGE_GMOVES;
      return;
    }

    current->move[index].movednum = movednum;
    current->move[index].changednum = changednum;

    if(movednum)
      current->move[index].moved = (struct movedsct *)mymalloc(movednum*sizeof(struct movedsct));
    else current->move[index].moved = NULL;

    if(changednum)
      current->move[index].changed = (struct changesct *)mymalloc(changednum*sizeof(struct changesct));
    else current->move[index].changed = NULL;

    for(i=0; i<movednum; i++){
      current->move[index].moved[i].obj = objlist[MOVEDLIST][i];
      current->move[index].moved[i].oric = oric[i];
    }
    for(i=0; i<changednum; i++){
      current->move[index].changed[i].obj = changed[i].obj;
      current->move[index].changed[i].type = changed[i].type;
    }

    if(issolved() && !userlev) room_sol_esc = 1;
  }

  if(moves % MOVESBLOCK == 0){
    if(!nosave){
      last = current->next = (struct movesblock *)mymalloc(sizeof(struct movesblock));
      last->prev = current;
    }
    current = current->next;
    if(!nosave) savepos();
  }

  if(moves == -1){
    img_change |= CHANGE_GMOVES;
    moves = 0;
  }
  if(nosave == 1) nosave = 0;
}

void init_moves()
{
  first = current = last = NULL;
  movednum = changednum = 0;
  room_sol_esc = 0;

  maxchanges = fishnum+1;
  oric = (coor *)mymalloc(objnum*sizeof(coor));
  changed = (struct changesct *)mymalloc(maxchanges*sizeof(struct changesct));
  maxmoves = 0;

  if(room_state == ROOM_BEGIN) minmoves = moves = -1;
  else{
    minmoves = moves = 0;
    createfirstblock();
    beforestart.movednum = beforestart.changednum = 0;
    beforestart.moved = NULL;
    beforestart.changed = NULL;
  }
  recent_saved = 0;
}

void addobjmove(ff_object *obj)
{
  if(nosave) return;
  if(obj->index[MOVEDLIST] < movednum) return;
  swapobject(obj, movednum, MOVEDLIST);
  oric[movednum] = obj->c;
  movednum++;
}

void addobjchange(ff_object *obj, char type)
{
  if(nosave) return;
  if(changednum >= maxchanges){
    warning("more then %d changes -> undo list exceded", maxchanges);
    return;
  }
  changed[changednum].obj = obj;
  changed[changednum].type = type;
  changednum++;
}

void startmove(ff_object *fish, int dir)
{
  recent_saved = 0;
  start_moveanim();

  img_change |= CHANGE_GMOVES;
  if(nosave){
    moves++;
    return;
  }
  if(maxmoves > moves){
    if(current->move[moves%MOVESBLOCK].fish != fish ||
       current->move[moves%MOVESBLOCK].dir != dir) freeredo();
    else{
      nosave = 1;
      moves++;
      return;
    }
  }
  current->move[moves%MOVESBLOCK].fish = fish;
  current->move[moves%MOVESBLOCK].dir = dir;
  changednum = movednum = 0;
  moves++;
  maxmoves = moves;
}

#define UNDOCOST  1
#define REDOCOST  1
#define LOADCOST 10

#define UNDO     0
#define REDO     1
#define LOADUNDO 2
#define LOADREDO 3

char setmove(int move)
{
  int cost[4], loadredopos, loadundopos;
  int i, min;
  char forward;

  if(move > maxmoves) move = maxmoves;
  if(move < minmoves) move = minmoves;

  if(move == moves) return 0;
  while(room_state != ROOM_IDLE) room_step();
  if(room_sol_esc) return 0;
  if(menumode) return 0;
  if(move == moves) return 0;

  img_change |= CHANGE_GMOVES;

  if(move < moves){
    cost[UNDO] = (moves-move)*UNDOCOST;
    cost[REDO] = -1;
    forward = 0;
  }
  else{
    cost[REDO] = (move-moves)*REDOCOST;
    cost[UNDO] = -1;
    forward = 1;
  }
  if(move == -1) cost[LOADREDO] = -1;
  else{
    loadredopos = move/MOVESBLOCK;
    cost[LOADREDO] = LOADCOST+(move-loadredopos*MOVESBLOCK)*REDOCOST;
  }
  if(maxmoves/MOVESBLOCK == move/MOVESBLOCK) cost[LOADUNDO] = -1;
  else{
    loadundopos = (move+(MOVESBLOCK-1))/MOVESBLOCK;
    cost[LOADUNDO] = LOADCOST+(loadundopos*MOVESBLOCK-move)*UNDOCOST;
  }

  min = 0;
  for(i=1; i<4; i++)
    if(cost[i] >= 0 && (cost[min] < 0 || cost[i] < cost[min])) min = i;
  if(cost[min] == -1){
    warning("No way to move %d found", move);
    return 0;
  }

  nosave = 2;
  if(min == UNDO){
    undoto(move);
    if(move != -1) findfishheap();
  }
  else if(min == REDO) redoto(move);
  else if(min == LOADUNDO){
    loadpos(loadundopos);
    undoto(move);
    findfishheap();
  }
  else if(min == LOADREDO){
    loadpos(loadredopos);
    findfishheap();
    redoto(move);
  }
  nosave = 0;

  if(forward){
    if(move%MOVESBLOCK) setactivefish(current->move[(move-1)%MOVESBLOCK].fish);
    else setactivefish(current->prev->move[(move-1)%MOVESBLOCK].fish);
  }
  else if(move >= 0) setactivefish(current->move[move%MOVESBLOCK].fish);
  else setactivefish(start_active_fish);

  moves = move;
  if(move == -1){
    fallall();
    nosave = 1;
  }
  else room_state = ROOM_IDLE;

  unanimflip();
  recent_saved = 0;

  return 1;
}

char savemoves(char *filename)
{
  FILE *f;
  int i, icur;
  int af, last_af;
  struct movesblock *mb;

  f = fopen(filename, "w");
  if(!f) return 0;

  if(moves == -1) icur = 0;
  else icur = moves;  

  mb = first;
  last_af = 0;
  for(i=0; i<maxmoves;){
    if(i == icur) fputc('.', f);

    af = mb->move[i%MOVESBLOCK].fish->index[FISHLIST];
    while(last_af < af){ last_af++; fputc('+', f);  }
    while(last_af > af){ last_af--; fputc('-', f);  }

    if(mb->move[i%MOVESBLOCK].dir == UP) fputc('^', f);
    else if(mb->move[i%MOVESBLOCK].dir == DOWN) fputc('v', f);
    else if(mb->move[i%MOVESBLOCK].dir == LEFT) fputc('<', f);
    else if(mb->move[i%MOVESBLOCK].dir == RIGHT) fputc('>', f);
    i++;
    if(i % MOVESBLOCK == 0) mb = mb->next;
  }
  if(active_fish){
    af = active_fish->index[FISHLIST];
    while(last_af < af){ last_af++; fputc('+', f);  }
    while(last_af > af){ last_af--; fputc('-', f);  }
  }

  fclose(f);

  return 1;
}

char loadmoves(char *filename)
{
  FILE *f;
  char dir;
  int af;
  int ch, move;

  f = fopen(filename, "r");
  if(!f) return 0;

  room_sol_esc = 0;
  setmove(0);
  freeredo();

  move = -1;
  ch = fgetc(f);
  while(ch != EOF){
    dir = -1;
    switch(ch){
    case '-': af--; break;
    case '+': af++; break;
    case '.': move = moves; break;
    case '^': dir = UP; break;
    case 'v': dir = DOWN; break;
    case '<': dir = LEFT; break;
    case '>': dir = RIGHT; break;
    default: warning("loadmoves(%s): unknown symbol '%c'", filename, ch);
    }
    if(dir >= 0){
      if(af < 0 || af >= objnum){
	warning("active fish out of bounds: %d", af);
	af = 0;
      }
      setactivefish(objlist[FISHLIST][af]);
      if(active_fish != objlist[FISHLIST][af])
	warning("move %d: failed to set active fish %d");
      movefish(dir);
      if(room_state == ROOM_IDLE)
	warning("move %d failed (%d, %c)", moves, af, ch);
      while(room_state != ROOM_IDLE) room_step();
    }
    ch = fgetc(f);
  }
  fclose(f);

  room_sol_esc = 0;
  if(move >= 0) setmove(move);
  if(af < 0 || af >= objnum){
    warning("active fish out of bounds: %d", af);
    af = 0;
  }
  setactivefish(objlist[FISHLIST][af]);

  unanimflip();
  unanim_fish_rectangle();
  img_change |= CHANGE_GMOVES;

  return 1;
}

void free_moves()
{
  struct movesblock *mb, *tmp;
  int i;

  if(room_state != ROOM_IDLE) finishmove();

  if(beforestart.moved) free(beforestart.moved);
  if(beforestart.changed) free(beforestart.changed);

  if(oric) free(oric);

  for(mb = first; maxmoves >= 0;){
    if(mb->saved) free(mb->saved);
    for(i=0; i<MOVESBLOCK && i < maxmoves; i++){
      if(mb->move[i%MOVESBLOCK].moved) free(mb->move[i%MOVESBLOCK].moved);
      if(mb->move[i%MOVESBLOCK].changed) free(mb->move[i%MOVESBLOCK].changed);
    }
    maxmoves -= MOVESBLOCK;

    tmp = mb;
    mb = mb->next;
    free(tmp);
  }
}
