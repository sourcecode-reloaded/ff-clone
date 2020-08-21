#include<stdio.h>
#include<string.h>
#include<math.h>
#include<cairo/cairo.h>

#include "X.h"
#include "window.h"
struct ff_object;
#include "draw.h"
#include "moves.h"
#include "gmoves.h"

#define SCALE 21

#define PANELBORDER 0.2
#define DARKTHICKNESS 0.2
#define LIGHTTHICKNESS 0.1

#define MARK_THICKNESS 0.2
#define MARK_LENGTH    1.3
#define TEXT_BORDER    0.2

#define DISABLED 0
#define ENABLED  1
#define PRESSED  2

static char button_pressed;

static float slider_length;
static cairo_pattern_t *lorange = NULL;
static cairo_pattern_t *dorange = NULL;
static cairo_pattern_t *lblue = NULL;
static cairo_pattern_t *dblue = NULL;

void init_gmoves()
{
  gmoves_width = ceil((PANELBORDER+4)*SCALE);
  gmoves_height = ceil((PANELBORDER+2)*SCALE);

  lorange = cairo_pattern_create_rgb(1, 0.68, 0);
  dorange = cairo_pattern_create_rgb(0.5, 0.34, 0);
  lblue = cairo_pattern_create_rgb(0.2, 0.2, 1);
  dblue = cairo_pattern_create_rgb(0, 0, 0.5);
}

void level_gmoves_init()
{
  slider_hold = 0;
  button_pressed = 0;
  rewinding = 0;
}

static void drawslider(cairo_t *cr)
{
  float pos, x;

  if(slider_length <= 4*DARKTHICKNESS){
    //slider_length = 0;
    return;
  }

  if(moves <= 0) pos = 0;
  else pos = ((float)moves)/maxmoves;

  cairo_save(cr);
  if(gmoves_vertical) cairo_rotate(cr, M_PI/2);

  cairo_move_to(cr, DARKTHICKNESS, 0);
  cairo_line_to(cr, slider_length-DARKTHICKNESS, 0);

  cairo_set_source(cr, dblue);
  cairo_set_line_width(cr, DARKTHICKNESS*2);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_stroke_preserve(cr);

  cairo_set_source(cr, lblue);
  cairo_set_line_width(cr, LIGHTTHICKNESS*2);
  cairo_stroke(cr);

  x = pos*(slider_length-4*DARKTHICKNESS)+2*DARKTHICKNESS;

  cairo_move_to(cr, x, -1+DARKTHICKNESS);
  cairo_line_to(cr, x, 1-DARKTHICKNESS);

  cairo_set_source(cr, dorange);
  cairo_set_line_width(cr, DARKTHICKNESS*2);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_stroke_preserve(cr);

  cairo_set_source(cr, lorange);
  cairo_set_line_width(cr, LIGHTTHICKNESS*2);
  cairo_stroke(cr);

  cairo_restore(cr);
}

void drawhpanelshape(cairo_t *cr, char inside)
{
  cairo_move_to(cr, -1, 2);
  cairo_arc(cr, -1, 1, 1, M_PI/2, 1.5*M_PI);
  cairo_line_to(cr, 5, 0);
  cairo_arc(cr, 5, 1, 1, -M_PI/2, M_PI/2);
  cairo_close_path(cr);

  if(inside){
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 0, 2);

    cairo_move_to(cr, 4, 0);
    cairo_line_to(cr, 4, 2);
  }
}

void drawvpanelshape(cairo_t *cr, char inside)
{
  cairo_move_to(cr, 0, -1);
  cairo_arc(cr, 1, -1, 1, M_PI, 1.5*M_PI);
  cairo_line_to(cr, 3, -2);
  cairo_arc(cr, 3, -1, 1, 1.5*M_PI, 0);
  cairo_line_to(cr, 4, 2);
  cairo_line_to(cr, 0, 2);
  cairo_close_path(cr);

  if(inside){
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 4, 0);

    cairo_move_to(cr, 2, -2);
    cairo_line_to(cr, 2, 0);
  }
}

static void drawnumber(cairo_t *cr, char *str, float translate)
{
  int i;
  char digit[2];

  digit[1] = 0;
  for(i=0; str[i]; i++){
    if(str[i] == '-'){
      cairo_rectangle(cr, 1, -4, 5, 1);
      cairo_fill(cr);
    }
    else{
      digit[0] = str[i];
      cairo_move_to(cr, 0, 0);
      cairo_show_text(cr, digit);
    }
    cairo_translate(cr, translate, 0);
  }
}

void drawpanel(cairo_t *cr)
{
  cairo_text_extents_t text_ext;
  float ratiox, ratioy;
  char movestring[50];

  char minusstate;
  char plusstate;

  if(rewinding == -1) minusstate = PRESSED;
  else{
    if(moves > minmoves) minusstate = ENABLED;
    else minusstate = DISABLED;
  }
  if(rewinding == 1) plusstate = PRESSED;
  else{
    if(moves < maxmoves) plusstate = ENABLED;
    else plusstate = DISABLED;
  }

  // background
  cairo_save(cr);

  if(gmoves_vertical) drawvpanelshape(cr, 0);
  else drawhpanelshape(cr, 0);
  cairo_clip(cr);
  cairo_set_source(cr, lorange);
  cairo_paint(cr);

  // Minus button
  cairo_save(cr);
  if(gmoves_vertical) cairo_translate(cr, 0, -2);
  else cairo_translate(cr, -2, 0);

  if(minusstate == PRESSED){
    cairo_set_source(cr, lblue);
    cairo_rectangle(cr, 0, 0, 2, 2);
    cairo_fill(cr);
  }

  if(minusstate == DISABLED) cairo_set_source(cr, dorange);
  else cairo_set_source(cr, dblue);
  cairo_rectangle(cr, 1 - MARK_LENGTH/2, 1 - MARK_THICKNESS/2,
		  MARK_LENGTH, MARK_THICKNESS);
  cairo_fill(cr);
  cairo_restore(cr);

  // Plus button
  cairo_save(cr);
  if(gmoves_vertical) cairo_translate(cr, 2, -2);
  else cairo_translate(cr, 4, 0);

  if(plusstate == PRESSED){
    cairo_set_source(cr, lblue);
    cairo_rectangle(cr, 0, 0, 2, 2);
    cairo_fill(cr);
  }

  if(plusstate == DISABLED) cairo_set_source(cr, dorange);
  else cairo_set_source(cr, dblue);
  cairo_rectangle(cr, 1 - MARK_LENGTH/2, 1 - MARK_THICKNESS/2,
		  MARK_LENGTH, MARK_THICKNESS);
  cairo_rectangle(cr, 1 - MARK_THICKNESS/2, 1 - MARK_LENGTH/2,
		  MARK_THICKNESS, MARK_LENGTH);
  cairo_fill(cr);
  cairo_restore(cr);

  // Text
  sprintf(movestring, "%d", moves);

  cairo_text_extents(cr, "0", &text_ext);
  text_ext.width += text_ext.x_advance*(strlen(movestring)-1);

  //printf("%f, %f\n", text_ext.width, text_ext.x_bearing);

  ratiox = (4.0-2*TEXT_BORDER)/text_ext.width;
  ratioy = (2.0-2*TEXT_BORDER)/text_ext.height;
  if(ratioy < ratiox){
    cairo_translate(cr, 4-TEXT_BORDER, 2-TEXT_BORDER);
    cairo_scale(cr, ratioy, ratioy);
    cairo_translate(cr, -text_ext.width, 0);
  }
  else{
    cairo_translate(cr, TEXT_BORDER, 1+ratiox*text_ext.height/2);
    cairo_scale(cr, ratiox, ratiox);
  }
  /*
  cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
  cairo_rectangle(cr, 0, 0, text_ext.width, -text_ext.height);
  cairo_fill(cr);
  cairo_move_to(cr, -text_ext.x_bearing, 0);
  */
  cairo_translate(cr, -text_ext.x_bearing, 0);

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  
  drawnumber(cr, movestring, text_ext.x_advance);

  cairo_restore(cr);

  // Border
  if(gmoves_vertical) drawvpanelshape(cr, 1);
  else drawhpanelshape(cr, 1);

  cairo_set_source(cr, dblue);
  cairo_set_line_width(cr, PANELBORDER);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
  cairo_stroke(cr);
}

void drawgmoves(cairo_t *cr, char repaint)
{
  int x, y, width, height;

  cairo_save(cr);

  if(gmoves_vertical){
    x = surf_width - gmoves_width; y = 0;
    width = gmoves_width; height = surf_height;
  }
  else{
    x = 0; y = surf_height - gmoves_height;
    width = surf_width; height = gmoves_height;
  }
  cairo_rectangle(cr, x, y, width, height);
  cairo_clip(cr);

  cairo_set_source_rgb(cr, 0,0,0);
  if(repaint) cairo_paint(cr);

  if(gmoves_vertical){
    slider_length = ((float)surf_height)/SCALE - (4+PANELBORDER);
    cairo_translate(cr, surf_width-SCALE*(4+PANELBORDER)/2, 0);
  }
  else{
    slider_length = ((float)surf_width)/SCALE - (8+PANELBORDER);
    cairo_translate(cr, 0, surf_height-SCALE*(2+PANELBORDER)/2);
  }

  cairo_scale(cr, SCALE, SCALE);

  drawslider(cr);

  if(gmoves_vertical) cairo_translate(cr, -2, slider_length+2+PANELBORDER/2);
  else cairo_translate(cr, slider_length+2+PANELBORDER/2, -1);

  drawpanel(cr);

  cairo_restore(cr);

  repaint_win(x, y, width, height);
}

void slider_click(float x, float y)
{
  float pos;

  if(gmoves_vertical){
    if(x < -1 || x > 1) return;
    if(y < DARKTHICKNESS || y > slider_length-DARKTHICKNESS) return;
    pos = (y-DARKTHICKNESS)/(slider_length-2*DARKTHICKNESS);
  }
  else{
    if(y < -1 || y > 1) return;
    if(x < DARKTHICKNESS || x > slider_length-DARKTHICKNESS) return;
    pos = (x-DARKTHICKNESS)/(slider_length-2*DARKTHICKNESS);
  }

  pos *= maxmoves;
  pos += 0.5;

  setmove(pos);
  unanim_fish_rectangle();

  slider_hold = 1;
  if(button_pressed){
    img_change |= CHANGE_GMOVES;
    button_pressed = rewinding = 0;
  }
}

void panel_click(float x, float y, unsigned int button)
{
  int d;

  d = 0;

  if(gmoves_vertical){
    if(y > 0 || y < -2 || x < 0 || x > 4) return;
    if(x <= 2){
      if(y < -1 && x < 1 && (x-1)*(x-1) + (y+1)*(y+1) > 1) return;
      d = -1;
    }
    else{
      if(y < -1 && x > 3 && (x-3)*(x-3) + (y+1)*(y+1) > 1) return;
      d = 1;
    }
  }
  else{
    if(x < -2 || x > 6 || y < 0 || y > 2) return;
    if(x < 0){
      if(x < -1 && (x+1)*(x+1) + (y-1)*(y-1) > 1) return;
      d = -1;
    }
    else if(x > 4){
      if(x > 5 && (x-5)*(x-5) + (y-1)*(y-1) > 1) return;
      d = 1;
    }
  }

  if(!d) return;
  if(d != rewinding){
    if(d == -1 && moves <= minmoves) return;
    if(d == 1 && moves >= maxmoves) return;
  }

  if(button == 1){
    button_pressed = 1;
    rewinding = d;
    setmove(moves+rewinding);
  }
  else{
    if(button_pressed) return;
    if(d == rewinding){
      rewinding = 0;
      img_change |= CHANGE_GMOVES;
    }
    else rewinding = d;
  }
}

void gmoves_click(XButtonEvent *xbutton)
{
  float x, y;
  int shift;

  x = xbutton->x;
  y = xbutton->y;

  if(x < 0 || y < 0 || x >= surf_width || y >= surf_height) return;
  if(gmoves_vertical){
    x -= surf_width;
    if(x < -gmoves_width) return;
  }
  else{
    y -= surf_height;
    if(y < -gmoves_height) return;
  }

  if(xbutton->button == 4 || xbutton->button == 5){
    shift = (maxmoves+100)/100;
    if(xbutton->button == 4) setmove(moves-shift);
    else setmove(moves+shift);
    unanim_fish_rectangle();
  }

  x /= SCALE;
  y /= SCALE;

  if(gmoves_vertical) x += (4+PANELBORDER)/2;
  else y += (2+PANELBORDER)/2;

  if(xbutton->button == 1) slider_click(x, y);

  if(gmoves_vertical){
    x += 2;
    y -= slider_length+PANELBORDER/2+2;
  }
  else{
    x -= slider_length+PANELBORDER/2+2;
    y += 1;
  }

  if(xbutton->button == 1 || xbutton->button == 3)
    panel_click(x, y, xbutton->button);
}

void gmoves_unclick(XButtonEvent *xbutton)
{
  if(xbutton->button != 1) return;
  slider_hold = 0;
  if(button_pressed){
    rewinding = 0;
    button_pressed = 0;
    img_change |= CHANGE_GMOVES;
  }
}

void moveslider(XMotionEvent *xmotion)
{
  float pos;

  if(gmoves_vertical) pos = xmotion->y;
  else pos = xmotion->x;

  pos /= SCALE;
  pos -= DARKTHICKNESS;
  pos /= slider_length-2*DARKTHICKNESS;
  if(pos < 0) pos = minmoves;
  else if(pos > 1) pos = maxmoves;
  else{
    pos *= maxmoves;
    pos += 0.5;
  }

  setmove(pos);
  unanim_fish_rectangle();
}

void rewind_moves(int d, char keyon)
{
  if(button_pressed) return;

  if(keyon){
    if(d == -1 && moves == minmoves) return;
    if(d == 1 && moves == maxmoves) return;
    rewinding = d;
    setmove(moves+rewinding);
  }
  else if(rewinding == d) rewinding = 0;

  img_change |= CHANGE_GMOVES;
}

void rewind_stop()
{
  if(button_pressed || !rewinding) return;

  img_change |= CHANGE_GMOVES;
  rewinding = 0;
}
