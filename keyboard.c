#include <string.h>
#include <cairo/cairo.h>
#include <X11/keysym.h>
#include "X.h"
#include "window.h"
#include "script.h"
#include "levelscript.h"
#include "object.h"
#include "rules.h"
#include "moves.h"
#include "gmoves.h"
#include "gsaves.h"
#include "main.h"
#include "keyboard.h"
#include "draw.h"
#include "imgsave.h"
#include "menuloop.h"

#define SPACE   4
#define TAB     5
#define MINUS   6
#define PLUS    7
#define PG_UP   8
#define PG_DN   9
#define HOME   10
#define END    11

#define KEYNUM 12

static unsigned long key_sym[KEYNUM];
static unsigned int key_code[KEYNUM];
static char key_state[KEYNUM];
static char key_counter[4];
//static char escape_pressed, space;
static char last_axis = 0;

static char arrowpressed(int dir)
{
  return key_state[dir] == 1 && key_counter[dir] == 0;
}

int QUEUEWAIT;
int HOLDWAIT;

void kb_apply_safemode()
{
  if(safemode){
    QUEUEWAIT = 5;
    HOLDWAIT = 3;
  }
  else{
    QUEUEWAIT = 15;
    HOLDWAIT = 8;
  }
}

#define MAXQUEUE 50

static char queue_key[MAXQUEUE];
static int queue_counter[MAXQUEUE];
static int queue_start;
static int queue_length;

static void queuepush(int key)
{
  if(queue_length >= MAXQUEUE) return;
  queue_key[(queue_start+queue_length)%MAXQUEUE] = key;
  queue_counter[(queue_start+queue_length)%MAXQUEUE] = QUEUEWAIT;

  queue_length++;
}

static void queuepop()
{
  queue_start++;
  if(queue_start == MAXQUEUE) queue_start = 0;
  queue_length--;
}

void keyboard_step()
{
  int i, dir;
  char enabled;

  for(i=0; i<KEYNUM; i++)
    if(key_state[i] < 0){
      key_state[i]++;
      if(key_state[i] == 0){
	if(i == MINUS) rewind_moves(-1, 0);
	else if(i == PLUS) rewind_moves(1, 0);
	else if(i == keyboard_blockanim) keyboard_blockanim = 0;
      }
    }

  enabled = (!rewinding && !gsaves_blockanim && !slider_hold && room_state == ROOM_IDLE);

  for(i=0; i<queue_length; i++) // projdu frontu
    queue_counter[(queue_start+i)%MAXQUEUE]--; // a zkratim pocitadla cekatelum

  while(queue_length){
    if(queue_counter[queue_start] == 0){
      dir = queue_key[queue_start];
      queuepop();
      if(queue_length && dir < 4 && queue_key[queue_start] == backdir(dir)) queuepop();
      continue;
    }
    if(queue_key[queue_start] == SPACE || queue_key[queue_start] == TAB){
      changefish();
      queuepop();
      continue;
    }
    if(!enabled) break;
    movefish(queue_key[queue_start]);
    enabled = enabled && room_state == ROOM_IDLE;
    queuepop();
  }

  /*
  if(key_state[0] != 1 && key_state[1] != 1 &&
     key_state[2] != 1 && key_state[3] != 1) key_counter = -1;
  else if(key_counter > 0) key_counter--;
  */
  for(i=0; i<4; i++) if(key_counter[i] > 0) key_counter[i]--;

  if(enabled){
    dir = (last_axis+1)%2;
    for(i=0; enabled && i<2; i++){
      if(arrowpressed(dir) ^ arrowpressed(dir+2))
	for(; dir < 4; dir += 2)
	  if(arrowpressed(dir)){
	    movefish(dir);
	    if(room_state != ROOM_IDLE){
	      last_axis = dir%2;
	      return;
	    }
	  }

      dir = last_axis;
    }
  }
}

static void change_state(int key, char press)
{
  if(gsaves_blockanim || menumode){
    if(press) key_state[key] = 1;
    else key_state[key] = 0;
    queue_length = 0;

    if(!menumode){
      if(press){
	if(key == MINUS){
	  rewinding = -1; img_change |= CHANGE_GMOVES;
	}
	else if(key == PLUS){
	  rewinding = 1; img_change |= CHANGE_GMOVES;
	}
      }
      else{
	if(key == MINUS || key == PLUS){
	  rewinding = 0; img_change |= CHANGE_GMOVES;
	}
      }
    }
    return;
  }

  if(press){
    if(key_state[key] == 0){
      if(key < 4 || key == SPACE || key == TAB){
	keyboard_blockanim = 0;
	rewind_stop(); 
	queuepush(key);
	if(key < 4){ // arrow
	  if(arrowpressed(UP) || arrowpressed(DOWN) ||
	     arrowpressed(LEFT) || arrowpressed(RIGHT)) key_counter[key] = 0;
	  else key_counter[key] = HOLDWAIT;
	}
      }
      else{
	queue_length = 0;
	if(key == MINUS) rewind_moves(-1, 1);
	else if(key == PLUS) rewind_moves(1, 1);
	else{
	  if(moves >= 0 || (key != PG_UP && key != HOME)) 
	    keyboard_blockanim = key;
	  if(key == PG_UP) setmove(moves-100);
	  else if(key == PG_DN) setmove(moves+100);
	  else if(key == HOME) setmove(minmoves);
	  else if(key == END) setmove(maxmoves);
	  unanim_fish_rectangle();
	}
      }
    }
    key_state[key] = 1;
  }
  else key_state[key] = -1;
}

static char ogridmode = 0;

void key_event(XKeyEvent xev)
{
  KeySym ks;
  char press;
  int i;

  XLookupString (&xev, NULL, 0, &ks, NULL);
  if(!ks) return;

  if(xev.type == KeyPress) press = 1;
  else press = 0;

  for(i=0; i<KEYNUM; i++)
    if(ks == key_sym[i]){
      change_state(i, press);
      return;
    }

  if(!press) return;

  if(ks == XK_g){
    if(gridmode != GRID_OFF) gridmode = GRID_OFF;
    else gridmode = GRID_BG;
    img_change |= 1;
  }
  else if(ks == XK_G){
    if(gridmode == GRID_FG)
      gridmode = ogridmode ? GRID_BG : GRID_OFF;
    else{
      ogridmode = (gridmode == GRID_BG);
      gridmode = GRID_FG;
    }
    img_change |= 1;
  }
  else if(ks == XK_F2 && !menumode) gsaves_save();
  else if(ks == XK_F3 && !menumode) gsaves_load();
  else if(ks == XK_F11) fullscreen_toggle();
  else if(ks == XK_F12){
    safemode = !safemode;
    apply_safemode();
  }
  else if(ks == XK_Escape) escape();
  else if((ks == XK_r || ks == XK_R) && (xev.state & ControlMask))
    refresh_user_level();
}

void key_remap()
{
  int i;
  char key_vector[32];

  XQueryKeymap(display, key_vector);

  for(i=0; i<KEYNUM; i++){
    if(!key_code[i]) continue;
    if(key_vector[key_code[i]/8] & (1 << (key_code[i]%8))) key_state[i] = 1;
    else{
      if(i == keyboard_blockanim) keyboard_blockanim = 0;
      key_state[i] = 0;
    }
  }
}

void keyboard_erase_queue()
{
  queue_length = 0;
}

void level_keys_init()
{
  last_axis = 0;
  keyboard_blockanim = 0;
  queue_start = 0;
  queue_length = 0;
}

void init_keyboard()
{
  KeySym *ks_field;
  int mincode, maxcode;
  int spc, i, j;

  key_sym[UP]    = XK_Up;
  key_sym[DOWN]  = XK_Down;
  key_sym[LEFT]  = XK_Left;
  key_sym[RIGHT] = XK_Right;
  key_sym[SPACE] = XK_space;
  key_sym[TAB]   = XK_Tab;
  key_sym[MINUS] = XK_minus;
  key_sym[PLUS]  = XK_equal;
  key_sym[PG_UP] = XK_Page_Up;
  key_sym[PG_DN] = XK_Page_Down;
  key_sym[HOME]  = XK_Home;
  key_sym[END]   = XK_End;

  XDisplayKeycodes(display, &mincode, &maxcode);
  maxcode++;

  ks_field = XGetKeyboardMapping(display, mincode, maxcode-mincode, &spc);

  for(i=0; i<KEYNUM; i++) key_code[i] = 0;
  for(i = 0; i < (maxcode-mincode)*spc; i++){
    for(j=0; j<KEYNUM; j++)
      if(ks_field[i] == key_sym[j]){
	key_code[j] = i/spc+mincode;
      }
  }
  XFree(ks_field);
}
