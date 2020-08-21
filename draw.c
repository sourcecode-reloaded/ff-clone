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
#include "menuevents.h"

static void draw_fish_rectangle(); 

#define GRID_THICKNESS 2   // tloustka mrizky v padesatinach policka
#define GRID_MINTHICK  1.2 /* minimalni tlouska v pixelech
			      (kdyby GRID_THICKNESS vyslo mensi) */

static void draw_grid() // vykresli mrizku do okna
{
  cairo_t *c;
  cairo_surface_t *square; // jeden ctverecek mrizky
  cairo_pattern_t *pat;

  int thickness = GRID_THICKNESS;

  if(thickness*room_x_scale/SQUARE_WIDTH < GRID_MINTHICK)
    thickness = SQUARE_WIDTH*GRID_MINTHICK/room_x_scale;

  square = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, SQUARE_WIDTH, SQUARE_HEIGHT);
  c = cairo_create(square);

  cairo_rectangle(c, 0, 0, SQUARE_WIDTH, thickness); // bile carecky v mrizce
  cairo_rectangle(c, 0, thickness, thickness, SQUARE_HEIGHT-2*thickness);
  cairo_set_source_rgba(c, 1, 1, 1, 0.3);
  cairo_fill(c);

  cairo_rectangle(c, 0, SQUARE_HEIGHT, SQUARE_WIDTH, -thickness); // cerne carecky v mrizce
  cairo_rectangle(c, SQUARE_WIDTH, thickness, -thickness, SQUARE_HEIGHT-2*thickness);
  cairo_set_source_rgba(c, 0, 0, 0, 0.3);
  cairo_fill(c);

  cairo_destroy(c);

  pat = cairo_pattern_create_for_surface(square); // vytvorim pattern z ctverecku
  cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);

  c = win_cr;

  //cairo_scale(c, room_x_scale, room_y_scale);

  cairo_save(c);
  cairo_scale(c, 1.0/SQUARE_WIDTH, 1.0/SQUARE_HEIGHT);
  cairo_set_source(c, pat);
  cairo_paint(c);
  cairo_restore(c);

  cairo_pattern_destroy(pat);
  cairo_surface_destroy(square);
}

static void draw_grid2() // alternativni puntikata mrizka, nepouziva se
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

void init_draw() // zinicializuje promennou bg_pattern - barvu pozadi
{
  bg_pattern = cairo_pattern_create_rgb(0, 0, 0.5);
}

static void drawroom(cairo_t *cr) // vykresli hlavni okno
{
  cairo_save(cr);

  cairo_translate(cr, room_x_translate, room_y_translate);

  cairo_rectangle(cr, 0, 0, room_x_size, room_y_size); // oriznu na spravnou vzdalenost
  cairo_clip(cr);

  cairo_set_source(cr, bg_pattern); // vybarvim pozadi
  cairo_paint(cr);

  cairo_scale(cr, room_x_scale, room_y_scale);

  if(gridmode == GRID_BG) draw_grid();
  draw_layers(cr, room_x_scale, room_y_scale); // objekty
  if(gridmode == GRID_FG) draw_grid();
  draw_fish_rectangle(cr);      // ramecek kolem aktivni ryby

  cairo_restore(cr);
}

void draw() // fykrasli hlavni okno
{
  img_change |= layers_change(); // zmenila se vrstva?

  if(!img_change) return; // nic neni treba prekreslovat

  if(img_change == CHANGE_ALL){ // zmenilo se vsechno
    cairo_set_source_rgb(win_cr, 0, 0, 0); // smazu okno
    cairo_paint(win_cr);

    drawroom(win_cr); // a nakreslim jednotlive prvky
    drawgmoves(win_cr, 0);
    drawgsaves(win_cr, 1);
  }
  else{
    /* Jestli gsaves couha pres neco, co se zmenilo, pripadne se zmenilo samotne gsaves,
       je treba gsaves schovat.
     */
    if(img_change & (gsaves_change_mask | CHANGE_GSAVES)) hidegsaves(win_cr);

    if(img_change & CHANGE_ROOM) drawroom(win_cr); // hlavni okno
    if(img_change & CHANGE_GMOVES) drawgmoves(win_cr, 1); // undo pasecek
    if(img_change & (gsaves_change_mask | CHANGE_GSAVES)) // opetovne vraceni gsaves
      drawgsaves(win_cr, img_change & gsaves_change_mask);
  }

  if(win_resize){ // zmenu velikosti je treba provest az po vykresleni
    XResizeWindow(display, win, win_width, win_height);
    win_resize = 0;
  }

  // prekresleni okna
  if(img_change == CHANGE_ALL) repaint_win(0, 0, win_width, win_height); // prekresleni celeho okna
  else{
    if(img_change & CHANGE_ROOM)
      repaint_win(room_x_translate, room_y_translate,
		  room_x_size, room_y_size);
    if(img_change & CHANGE_GMOVES){
      if(gmoves_vertical)
 	repaint_win(win_width - gmoves_width, 0, gmoves_width, win_height);
      else
	repaint_win(0, win_height - gmoves_height, win_width, gmoves_height);
    }
    if(img_change & CHANGE_GSAVES)
      repaint_win(0, 0, gsaves_x_size, gsaves_y_size);
  }

  img_change = 0;
}

/******************************************************************************/
/********************  Animace obdelnicku kolem aktivni ryby  *****************/

/*
  Pokud je nejaka ryba aktivni, obdelnicek je kolem ni, jenak neni vubec.
  Prechody mezi stavy jsou:
    a) Zmena aktivni ryby, pak se ramecek postupne presouva na novou aktivni rybu.
       Tento presun je velmi rychly, takze kreslim jeste jakesi rozostreni pohybem --
       nakresli ramecku vic s nizsi opacity.
    b) zadna ryba uz neni / nebyla, pak ramecek postupne mizi / objevuje se
 */

#define RECT_OPAC_STEPS 5 // jak rychle obdelnicek zmizi / objevi se
#define RECT_MOVE_STEPS 5 // jak rychle se presune z jedne ryby na druhou
#define RECT_MBLUR_STEPS 10 // pocet obdelnicku vykreslenych behem presouvani
#define RECT_MBLUR_OPAC   3 // pruhlednost jednoho takoveho obdelnicku

static ff_object *last_actfish; // aktivni ryba pri poslednim volani [un]anim_fish_rectangle()
static int rect_opacity; // 0 = pruhledny, RECT_OPAC_STEPS = plna opacity
static int rect_pos; // 0 = na spravne rybe, RECT_MOVE_STEPS = posledni vyznacna poloha

static float rect_x, rect_y, rect_width, rect_height; // soucasna poloha ramecku
static float rect_x2, rect_y2, rect_width2, rect_height2; // minula poloha ramecku

static char rect_mblur; // rozostrovat pohybem v tomto frame

static void anim_fish_rectangle() // jeden krok v animaci ramecku
{
  float x, y, width, height;

  if(rect_mblur) img_change |= CHANGE_ROOM;
  rect_mblur = 0;

  if(active_fish != last_actfish){ // zmena aktivni ryby
    if(active_fish && rect_opacity) rect_pos = RECT_MOVE_STEPS;
    last_actfish = active_fish;
  }

  if(!active_fish){ // ramecek mizi
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
    if(rect_opacity < RECT_OPAC_STEPS){ // ramecek se objevuje
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
    if(rect_pos > 0){ // ramecek se presouva
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
    else{ // ramecek zustava na aktivni rybe
      rect_x = x;
      rect_y = y;
      rect_width = width;
      rect_height = height;
    }
  }
}

void unanim_fish_rectangle() // da ramecek primo na aktivni rybu
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

static void draw_fish_rectangle(cairo_t *cr) // nakresli ramecek
{
  int i;
  float x, y, width, height;

  if(!rect_opacity) return;

  cairo_set_line_width(cr, 0.03); // tloustka ramecku v polickach
  if(!rect_mblur){ // jednoduche vykresleni
    cairo_set_source_rgba(cr, 1, 1, 1, ((float)rect_opacity)/RECT_OPAC_STEPS);
    cairo_rectangle(cr, rect_x, rect_y, rect_width, rect_height);
    cairo_stroke(cr);
  }
  else{ // rozostreni pohybem - mezi soucasnou a minulou polohou
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

/********************  Animace obdelnicku kolem aktivni ryby  *****************/
/******************************************************************************/

/*
  Ryby a hromadky na nich (podle funkce isonfish()) se (neni-li zapnuty safemode)
  houpou po Lissajousove krivce.
 */

#define HEAP_X_MAX 61 // perioda x-ove osy ve framech
#define HEAP_Y_MAX 42 // perioda y-ove osy ve framech
#define HEAP_X_AMPLITUDE 0.02 // maximalni vychyleni jedne souradnice v polickach
#define HEAP_Y_AMPLITUDE HEAP_X_AMPLITUDE

static int heap_x_phase; // stav v ramci periody
static int heap_y_phase;

static void animheap() // jeden krok animace houpani ryb
{
  if(safemode){ // pri safemode se nehoupa
    heap_x_advance = heap_y_advance = 0;
    return;
  }

  heap_x_phase++; // posunu faze
  if(heap_x_phase == HEAP_X_MAX) heap_x_phase = 0;
  heap_y_phase++;
  if(heap_y_phase == HEAP_Y_MAX) heap_y_phase = 0;

  // a spocitam vychyleni
  heap_x_advance = HEAP_X_AMPLITUDE*sinf(M_PI*2*heap_x_phase/HEAP_X_MAX);
  heap_y_advance = HEAP_Y_AMPLITUDE*sinf(M_PI*2*heap_y_phase/HEAP_Y_MAX);
}

static int   MOVEFRAMES; // pocet framu, za ktere se posune ryba
static float MAXFALLSPEED; // max. rychlost padu -- pocet policek za frame
static float FALLACCEL; // 0 = zadne zrychleni, 1 = okamzite na maximum

void apply_safemode() // na zaklade promenne safemode nastavi patricne promenne
{
  if(safemode){
    MOVEFRAMES         =   1;
    FALLACCEL          =   1;
    MAXFALLSPEED       =   1;
    FIRSTUNDOWAIT      =   2;
    UNDOWAIT           =   0;
    GSAVES_ACT_SPEED   =  0.5;
    GSAVES_SPACE_SPEED =  20;
    setdelay(0, 100000);
  }
  else{
    MOVEFRAMES         =   5;
    FALLACCEL          = 0.1;
    MAXFALLSPEED       =   1;
    FIRSTUNDOWAIT      =  11;
    UNDOWAIT           =   3;
    GSAVES_ACT_SPEED   =   0.1;
    GSAVES_SPACE_SPEED =   5;
    setdelay(0, 20000);
  }
  kb_apply_safemode();
}

static char first_anim; // prvni spusteni anim_step v tomto levelu

static char phase; // 0 = ryba se zacina hybat, MOVEFRAMES = uz poposla o policko
static float fallspeed; // rychlost padani

void anim_step() // posune animaci o jeden krok
{
  if(menumode || slider_hold || keyboard_blockanim ||
     (gsaves_blockanim && room_state == ROOM_IDLE)){
    first_anim = 0;
    return;
  }

  if(rewinding) rewind_step(); // drzene tlacitko '-' nebo '+'
  else if(room_state == ROOM_BEGIN){ // zacatek mistnosti, neco bude padat
    room_step();
    anim_phase = fallspeed = 0;
  }
  else{
    if(room_state == ROOM_FALL){ // padaji predmety
      fallspeed *= 1-FALLACCEL; // rychlost padu bude vazeny prumer MAXFALLSPEED a soucasne rychlosti
      fallspeed += MAXFALLSPEED * FALLACCEL;
      anim_phase += fallspeed;
      while(anim_phase >= 1 && room_state == ROOM_FALL){
	anim_phase--; // spadnuti o policko
	room_step();
      }
    }
    if(room_state == ROOM_MOVE){ // hybe se ryba
      phase++;
      if(phase >= MOVEFRAMES){ // uz poposla o policko
	room_step();
	anim_phase = fallspeed = 0;
      }
      else anim_phase = ((float)phase)/MOVEFRAMES;
    }

    animheap(); // houpani ryb
    if(safemode) unanimflip(); // otaceni ryby
    else animflip();
  }
  if(first_anim || safemode) unanim_fish_rectangle(); // obdelnicek aktivni ryby
  else anim_fish_rectangle();

  first_anim = 0;
}

void level_anim_init()  // inicializuje animace
{
  heap_x_phase = 0;
  heap_y_phase = 20;
  heap_x_advance = 0;
  heap_y_advance = 0;
  first_anim = 1;
}

void start_moveanim()  // ryba se zacina hybat
{
  phase = 0;
  anim_phase = 0;
}
