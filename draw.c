#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <cairo/cairo.h>
#include "script.h"
#include "X.h"
#include "window.h"
#include "gener.h"
#include "draw.h"
#include "loop.h"
#include "layers.h"
#include "levelscript.h"
#include "object.h"
#include "rules.h"
#include "moves.h"
#include "gmoves.h"
#include "gsaves.h"
#include "keyboard.h"
#include "menuloop.h"

static void draw_fish_rectangle();

//static int lastbindex = 0;

#define GRID_THICKNESS 2
#define GRID_MINTHICK  1.2

static void draw_grid()
{
  cairo_t *c;
  cairo_surface_t *square;
  cairo_pattern_t *pat;

  int thickness = GRID_THICKNESS;

  if(thickness*room_x_scale/SQUARE_WIDTH < GRID_MINTHICK)
    thickness = SQUARE_WIDTH*GRID_MINTHICK/room_x_scale;

  square = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, SQUARE_WIDTH, SQUARE_HEIGHT);
  c = cairo_create(square);

  cairo_rectangle(c, 0, 0, SQUARE_WIDTH, thickness);
  cairo_rectangle(c, 0, thickness, thickness, SQUARE_HEIGHT-2*thickness);
  cairo_set_source_rgba(c, 1, 1, 1, 0.3);
  cairo_fill(c);

  cairo_rectangle(c, 0, SQUARE_HEIGHT, SQUARE_WIDTH, -thickness);
  cairo_rectangle(c, SQUARE_WIDTH, thickness, -thickness, SQUARE_HEIGHT-2*thickness);
  cairo_set_source_rgba(c, 0, 0, 0, 0.3);
  cairo_fill(c);

  cairo_destroy(c);

  pat = cairo_pattern_create_for_surface(square);
  cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);

  c = win_cr;

  //cairo_scale(c, room_x_scale, room_y_scale);

  cairo_save(c);
  cairo_scale(c, 1.0/50, 1.0/50);
  cairo_set_source(c, pat);
  cairo_paint(c);
  cairo_restore(c);

  cairo_pattern_destroy(pat);
  cairo_surface_destroy(square);
}

static void draw_grid2()
{
  cairo_t *c;
  cairo_surface_t *square;
  cairo_pattern_t *pat;

  square = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, SQUARE_WIDTH, SQUARE_HEIGHT);
  c = cairo_create(square);

  cairo_arc (c, 25, 25, 4, 0, 2*M_PI);
  cairo_close_path (c);  
  cairo_set_source_rgb(c, 0, 0, 0);
  cairo_fill(c);

  cairo_arc (c, 25, 25, 2, 0, 2*M_PI);
  cairo_close_path (c);
  cairo_set_source_rgb(c, 1, 1, 1);
  cairo_fill(c);

  cairo_destroy(c);

  pat = cairo_pattern_create_for_surface(square);
  cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);

  c = win_cr;

  //cairo_scale(c, room_x_scale, room_y_scale);

  cairo_save(c);
  cairo_scale(c, 1.0/50, 1.0/50);
  cairo_translate(c, 25, 25);
  cairo_set_source(c, pat);
  cairo_paint(c);
  cairo_restore(c);

  cairo_pattern_destroy(pat);
  cairo_surface_destroy(square);
}

void init_draw()
{
  bg_pattern = cairo_pattern_create_rgb(0, 0, 0.5);
}

static void drawroom(cairo_t *cr)
{
  cairo_save(cr);

  cairo_rectangle(cr, room_x_translate, room_y_translate,
		  room_x_size, room_y_size);
  cairo_clip (cr);

  cairo_translate(cr, room_x_translate, room_y_translate);
  cairo_scale(cr, room_x_scale, room_y_scale);

  cairo_set_source(cr, bg_pattern);
  cairo_paint(cr);
  if(gridmode == GRID_BG) draw_grid();
  draw_layers(cr, room_x_scale, room_y_scale);
  if(gridmode == GRID_FG) draw_grid();
  draw_fish_rectangle(cr);

  cairo_restore(cr);
}

void draw()
{
  img_change |= layers_change();

  if(!img_change) return;

  if(img_change == CHANGE_ALL){
    cairo_set_source_rgb(win_cr, 0, 0, 0);
    cairo_paint(win_cr);
    drawroom(win_cr);
    drawgsaves(win_cr, 1);
    drawgmoves(win_cr, 0);
  }
  else{
    if(img_change & (gsaves_change_mask | CHANGE_GSAVES)) hidegsaves(win_cr);

    if(img_change & CHANGE_ROOM) drawroom(win_cr);
    if(img_change & CHANGE_GMOVES) drawgmoves(win_cr, 1);
    if(img_change & (gsaves_change_mask | CHANGE_GSAVES))
      drawgsaves(win_cr, img_change & gsaves_change_mask);
  }

  if(img_change == CHANGE_ALL) repaint_win(0, 0, win_width, win_height);
  else{
    if(img_change & CHANGE_ROOM)
      repaint_win(room_x_translate, room_y_translate,
		  room_x_size, room_y_size);
    if(img_change & CHANGE_GMOVES){
      if(gmoves_vertical)
 	repaint_win(surf_width - gmoves_width, 0, gmoves_width, surf_height);
      else
	repaint_win(0, surf_height - gmoves_height, surf_width, gmoves_height);
    }
    if(img_change & CHANGE_GSAVES)
      repaint_win(0, 0, gsaves_x_size, gsaves_y_size);
  }

  if(win_resize) XResizeWindow(display, win, win_width, win_height);
  img_change = 0;
}

#define RECT_OPAC_STEPS 5
#define RECT_MOVE_STEPS 5
#define RECT_MBLUR_STEPS 10
#define RECT_MBLUR_OPAC   3

static ff_object *last_actfish;
static int rect_opacity, rect_pos;
static float rect_x, rect_y, rect_width, rect_height,
  rect_x2, rect_y2, rect_width2, rect_height2;
static char rect_mblur;
static char first_anim;

static void anim_fish_rectangle()
{
  float x, y, width, height;

  if(rect_mblur) img_change |= CHANGE_ROOM;
  rect_mblur = 0;

  if(active_fish != last_actfish){
    if(active_fish && rect_opacity) rect_pos = RECT_MOVE_STEPS;
    last_actfish = active_fish;
  }

  if(!active_fish){
    if(rect_opacity > 0){
      rect_opacity--;
      img_change |= CHANGE_ROOM;
    }
    if(rect_opacity == 0){
      rect_pos = 0;
      return;
    }
  }
  else{
    if(rect_opacity < RECT_OPAC_STEPS){
      rect_opacity++;
      img_change |= CHANGE_ROOM;
    }

    x = active_fish->c.x;
    y = active_fish->c.y;
    width = active_fish->width;
    height = active_fish->height;
    if(room_state == ROOM_MOVE){
      if(room_movedir == UP) y -= anim_phase;
      else if(room_movedir == DOWN) y += anim_phase;
      else if(room_movedir == LEFT) x -= anim_phase;
      else if(room_movedir == RIGHT) x += anim_phase;
    }
    if(rect_pos > 0){
      img_change |= CHANGE_ROOM;
      rect_pos--;
      rect_mblur = 1;

      rect_x2 = rect_x; rect_y2 = rect_y;
      rect_width2 = rect_width; rect_height2 = rect_height;

      rect_x      *= rect_pos; rect_x      += x;      rect_x      /= rect_pos+1;
      rect_y      *= rect_pos; rect_y      += y;      rect_y      /= rect_pos+1;
      rect_width  *= rect_pos; rect_width  += width;  rect_width  /= rect_pos+1;
      rect_height *= rect_pos; rect_height += height; rect_height /= rect_pos+1;
    }
    else{
      rect_x = x;
      rect_y = y;
      rect_width = width;
      rect_height = height;
    }
  }
}

void unanim_fish_rectangle()
{
  if(menumode) return;

  rect_pos = 0;
  if(last_actfish != active_fish){
    last_actfish = active_fish;
    img_change |= CHANGE_ROOM;
  }
  if(!active_fish) rect_opacity = 0;
  else{
    rect_opacity = RECT_OPAC_STEPS;
    anim_fish_rectangle();
  }
}

static void draw_fish_rectangle(cairo_t *cr)
{
  int i;
  float x, y, width, height;

  if(!rect_opacity) return;

  cairo_set_line_width(cr, 0.02);
  if(!rect_mblur){
    cairo_set_source_rgba(cr, 1, 1, 1, ((float)rect_opacity)/RECT_OPAC_STEPS);
    cairo_rectangle(cr, rect_x, rect_y, rect_width, rect_height);
    cairo_stroke(cr);
  }
  else{
    cairo_set_source_rgba(cr, 1, 1, 1,
			  ((float)rect_opacity)/RECT_OPAC_STEPS/RECT_MBLUR_STEPS*RECT_MBLUR_OPAC);
    for(i=0; i<RECT_MBLUR_STEPS; i++){
      x = ((RECT_MBLUR_STEPS-i)*rect_x + i*rect_x2) / RECT_MBLUR_STEPS;
      y = ((RECT_MBLUR_STEPS-i)*rect_y + i*rect_y2) / RECT_MBLUR_STEPS;
      width = ((RECT_MBLUR_STEPS-i)*rect_width + i*rect_width2) / RECT_MBLUR_STEPS;
      height = ((RECT_MBLUR_STEPS-i)*rect_height + i*rect_height2) / RECT_MBLUR_STEPS;
      cairo_rectangle(cr, x, y, width, height);
      cairo_stroke(cr);
    }
  }
}

#define HEAP_X_MAX 61
#define HEAP_Y_MAX 42
#define HEAP_X_AMPLITUDE 0.02
#define HEAP_Y_AMPLITUDE 0.02

static int heap_x_phase;
static int heap_y_phase;

static void animheap()
{
  if(safemode){
    heap_x_advance = heap_y_advance = 0;
    return;
  }

  heap_x_phase++;
  if(heap_x_phase == HEAP_X_MAX) heap_x_phase = 0;
  heap_y_phase++;
  if(heap_y_phase == HEAP_Y_MAX) heap_y_phase = 0;

  heap_x_advance = HEAP_X_AMPLITUDE*sinf(M_PI*2*heap_x_phase/HEAP_X_MAX);
  heap_y_advance = HEAP_X_AMPLITUDE*sinf(M_PI*2*heap_y_phase/HEAP_Y_MAX);
}

static int   MOVEFRAMES;
static float FALLACCEL;
static float MAXFALLSPEED;
static int   FIRSTUNDOWAIT;
static int   UNDOWAIT;

static char phase;
static float fallspeed;
static int undowait;

void apply_safemode()
{
  if(safemode){
    MOVEFRAMES         =   1;
    FALLACCEL          =   1;
    MAXFALLSPEED       =   1;
    FIRSTUNDOWAIT      =   1;
    UNDOWAIT           =   0;
    GSAVES_ACT_SPEED   =  0.5;
    GSAVES_SPACE_SPEED =  20;
    setdelay(0, 100000);
  }
  else{
    MOVEFRAMES         =   5;
    FALLACCEL          = 0.1;
    MAXFALLSPEED       =   1;
    FIRSTUNDOWAIT      =  10;
    UNDOWAIT           =   3;
    GSAVES_ACT_SPEED   =   0.1;
    GSAVES_SPACE_SPEED =   5;
    setdelay(0, 20000);
  }
  kb_apply_safemode();
}

void anim_step()
{
  if(slider_hold || keyboard_blockanim || (gsaves_blockanim && room_state == ROOM_IDLE)){
    first_anim = 0;
    return;
  }

  if(room_state == ROOM_BEGIN && !rewinding){
    room_step();
    anim_phase = fallspeed = 0;
  }

  if(rewinding){
    if(undowait == 0){
      setmove(moves+rewinding);
      undowait = UNDOWAIT;
    }
    else if(undowait == -1){
      undowait = FIRSTUNDOWAIT;
    }
    else undowait--;
  }
  else{
    undowait = -1;

    if(room_state == ROOM_FALL){
      fallspeed *= 1-FALLACCEL;
      fallspeed += MAXFALLSPEED * FALLACCEL;
      anim_phase += fallspeed;
      while(anim_phase >= 1 && room_state == ROOM_FALL){
	anim_phase--;
	room_step();
      }
    }
    if(room_state == ROOM_MOVE){
      phase++;
      if(phase >= MOVEFRAMES){
	room_step();
	anim_phase = fallspeed = 0;
      }
      else anim_phase = ((float)phase)/MOVEFRAMES;
    }
    animheap();
    if(safemode) unanimflip();
    else animflip();
  }
  if(first_anim || safemode) unanim_fish_rectangle();
  else anim_fish_rectangle();

  first_anim = 0;
}

void level_anim_init()
{
  undowait = -1;
  heap_x_phase = 0;
  heap_y_phase = 2;
  heap_x_advance = 0;
  heap_y_advance = 0;
  first_anim = 1;
}

void start_moveanim()
{
  phase = 0;
}
