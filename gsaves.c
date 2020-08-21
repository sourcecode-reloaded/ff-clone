#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <math.h>

#include "warning.h"
#include "directories.h"
#include "X.h"
#include "window.h"
#include "script.h"
#include "levelscript.h"
#include "object.h"
#include "rules.h"
#include "moves.h"
#include "gmoves.h"
#include "imgsave.h"
#include "gsaves.h"
#include "draw.h"
#include "loop.h"

#define ICON_SIZE         200
#define MINI_SCALE        0.5
#define BORDER              5
#define RECT_BORDER    BORDER
#define MAGNET            0.5
#define DARKEN            0.5
#define SPACE              40
#define TRASHDIST         150
#define DRAGDIST           25
#define DRAGPOS             5
#define DODGE              20
#define MAX_LINESIZE      100

#define MAXSAVES  100

static struct gsave{
  cairo_surface_t *surf;
  cairo_surface_t *scaled;
  double enlarged, shift, spaceshift;
  char *name;
} data[MAXSAVES], *save[MAXSAVES];

static cairo_surface_t *unknown_icon;

static int savesnum;
static enum {NORMAL, HOLD, DRAG} mode;

static double mini_width, mini_height, mini_len;
static int icon_width, icon_height;
static int icon_len;

static char active;
static double act;           // animovana promenna active (pozvolne zvetsovani a zmensovani)
static int marked;           // save s rameckem
static int focused;          // save, nad kterym je mys
static double mouse_pos;     // = gsaves_vertical ? mouse_y : mouse_x;
static double focused_pos;   // 0 az 1 souradnice mysi v ramci oznaceneho save
static double start;         // souradnice prvniho ulozeneho save
static double shift;         // posunuti prvniho ulozeneho save kvuli efektu lupy
static int spaced;           // save, za kterym je mezera
static double enl_marked;    // zvetsenost oznaceneho save, nezavisle na act

static struct gsave *dragged;          // save odebrany ze seznamu
static int drag_src_x, drag_src_y;    // misto kliku
static Window drag_win;
static Pixmap drag_pxm;
static int drag_x, drag_y;
static void drag();
static void drop();

static double rect_pos, rect_width, rect_height, rect_col, rect_x, rect_y, rect_border;
// udaje o ramecku

static void count_rect();
static void count_shift();
static void get_bg();
static void dodge_gsaves();
static void savelist();
static void loadmarked();
static cairo_surface_t *bg_surf = NULL;

static int gsaves_ext_change_mask;

static void savelist()
{
  FILE *f;
  int i;

  f = fopen(savefile("list", "txt"), "w");
  if(!f){
    warning("Open %s for write -- failed", savefile("list", "txt"));
    return;
  }
  for(i=0; i<savesnum; i++){
    if(i == marked) fputc('@', f);
    fprintf(f, "%s\n", save[i]->name);
  }
  fclose(f);
}

void calculate_gsaves()
{
  int i;
  cairo_t *cr;
  double m_width, m_height;
  char rescale;

  if(!savesnum) return;

  if(gsaves_vertical){
    gsaves_x_size = icon_width + 2*BORDER;
    gsaves_y_size = gsaves_length;
  }
  else{
    gsaves_x_size = gsaves_length;
    gsaves_y_size = icon_height + 2*BORDER;
  }

  gsaves_ext_change_mask = 0;
  if(gsaves_vertical){
    if(gsaves_x_size > room_x_translate) gsaves_ext_change_mask |= CHANGE_ROOM;
    if(gmoves_vertical && gsaves_x_size > surf_width-gmoves_width)
      gsaves_ext_change_mask |= CHANGE_GMOVES;
  }
  else{
    if(gsaves_y_size > room_y_translate) gsaves_ext_change_mask |= CHANGE_ROOM;
    if(!gmoves_vertical && gsaves_y_size > surf_height-gmoves_height)
      gsaves_ext_change_mask |= CHANGE_GMOVES;
  }

  m_width = icon_width*MINI_SCALE;
  m_height = icon_height*MINI_SCALE;

  icon_len = gsaves_vertical ? icon_height : icon_width;
  if(savesnum >= 2){
    mini_len = ((double)(gsaves_length - icon_len - (savesnum+1)*BORDER))/(savesnum-1);
    if(icon_len*MINI_SCALE < mini_len) mini_len = icon_len*MINI_SCALE;
  }
  else mini_len = icon_len*MINI_SCALE;
  if(mini_len < 1) mini_len = 1;

  if(gsaves_vertical) m_height = mini_len;
  else m_width = mini_len;

  start = (gsaves_length - (savesnum*mini_len + (savesnum-1)*BORDER)) / 2.0;

  if(m_width == mini_width && m_height == mini_height) rescale = 0;
  else rescale = 1;

  mini_width = m_width;
  mini_height = m_height;

  for(i=0; i<savesnum; i++){
    if(save[i]->scaled){
      if(rescale) cairo_surface_destroy(save[i]->scaled);
      else continue;
    }

    save[i]->scaled =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, mini_width, mini_height);
    cr = cairo_create(save[i]->scaled);
    cairo_scale(cr, mini_width/icon_width, mini_height/icon_height);
    cairo_set_source_surface(cr, save[i]->surf, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
  }
}

static void draw_unknown_icon()
{
  cairo_t *cr;
  cairo_text_extents_t text_ext;
  float border = icon_height*0.2;
  float scale;

  unknown_icon = cairo_image_surface_create(CAIRO_FORMAT_RGB24, icon_width, icon_height);
  cr = cairo_create(unknown_icon);
  cairo_set_source_rgb(cr, 0, 0, 0.5);
  cairo_paint(cr);

  cairo_text_extents(cr, "?", &text_ext);
  cairo_set_source_rgb(cr, 1, 0.7, 0);
  scale = (icon_height-2*border)/text_ext.height;
  cairo_translate(cr, (icon_width-text_ext.x_advance*scale)/2, icon_height-border);
  cairo_scale(cr, scale, scale);
  cairo_move_to(cr, 0, 0);
  cairo_show_text(cr, "?");

  cairo_destroy(cr);
}

void level_gsaves_init()
{
  int i, ch;
  char line[MAX_LINESIZE+1];
  FILE *f;

  for(i=0; i<MAXSAVES; i++) save[i] = &data[i];

  icon_width = ICON_SIZE*room_width/(room_width+room_height);
  icon_height = ICON_SIZE*room_height/(room_width+room_height);
  mini_height = mini_width = 0;
  gsaves_width = ceil(icon_width*MINI_SCALE + 2*BORDER);
  gsaves_height = ceil(icon_height*MINI_SCALE + 2*BORDER);
  act = 0;
  active = 0;
  gsaves_blockanim = 0;
  gsaves_change_mask = 0;
  enl_marked = 0;

  unknown_icon = NULL;

  savesnum = 0;
  level_savedir_init();
  if(!savedir) return;
  f = fopen(savefile("list", "txt"), "r");
  if(!f) return;

  draw_unknown_icon();

  ch = fgetc(f);
  i = 0;
  for(;;){
    if(i == 0 && ch == '@') marked = savesnum;
    else if(ch == '\n' || ch == EOF){
      if(i > 0){
	if(savesnum >= MAXSAVES){
	  warning("level_gsaves_init: Too many (> %d) saves.", MAXSAVES);
	  break;
	}
	line[i] = 0;
	save[savesnum]->name = strdup(line);
	i = 0;
	save[savesnum]->surf =
	  cairo_image_surface_create_from_png(savefile(line, "png"));
	if(cairo_surface_status(save[savesnum]->surf) != CAIRO_STATUS_SUCCESS){
	  warning("Opening image %s failed", savefile(line, "png"));
	  save[savesnum]->surf = unknown_icon;
	}
	save[savesnum]->scaled = NULL;
	savesnum++;
      }
    }
    else{
      if(i >= MAX_LINESIZE){
	warning("Too long (> %d) line %d of file %s",
		MAX_LINESIZE, i+1, savefile("list", "txt"));
	while(ch != '\n' && ch != EOF) ch = fgetc(f);
      }
      else line[i++] = ch;
    }
    if(ch == EOF) break;
    ch = fgetc(f);
  }
  fclose(f);
}

static void dodge_gsaves()
{
  active = 0;
  if(gsaves_vertical){
    if(mouse_x < gsaves_width+DODGE){
      mouse_x = gsaves_width+DODGE;
      setmouse();
    }
  }
  else{
    if(mouse_y < gsaves_height+DODGE){
      mouse_y = gsaves_height+DODGE;
      setmouse();
    }
  }
}

void drawgsaves(cairo_t *cr, char newbg)
{
  int i;
  double enl;

  if(img_change & CHANGE_GSAVES){
    count_shift();
    count_rect();
  }

  if(bg_surf && (newbg || (!act && !enl_marked))){
    cairo_surface_destroy(bg_surf);
    bg_surf = NULL;
  }

  if(act || enl_marked){
    if(!bg_surf) get_bg();
    gsaves_change_mask = gsaves_ext_change_mask;
  }
  else gsaves_change_mask = 0;

  cairo_save(cr);

  if(act || enl_marked) cairo_rectangle(cr, 0, 0, gsaves_x_size, gsaves_y_size);
  else{
    if(gsaves_vertical) cairo_rectangle(cr, 0, 0, gsaves_width, gsaves_y_size);
    else cairo_rectangle(cr, 0, 0, gsaves_x_size, gsaves_height);
    gsaves_change_mask = 0;
  }
  cairo_clip(cr);

  if(marked >= 0 && savesnum){
    cairo_rectangle(cr, rect_x, rect_y, rect_width, rect_height);
    cairo_set_source_rgb(cr, rect_col, rect_col, rect_col);
    cairo_fill(cr);
  }

  if(gsaves_vertical) cairo_translate(cr, BORDER, start-shift);
  else cairo_translate(cr, start-shift, BORDER);

  for(i=0; i<savesnum; i++){
    enl = act*save[i]->enlarged;
    if(enl_marked && i == marked) enl = 1-(1-enl_marked)*(1-enl);

    if(!enl){
      cairo_set_source_surface(cr, save[i]->scaled, 0, 0);
      cairo_paint(cr);
    }
    else{
      cairo_save(cr);
      cairo_scale(cr, mini_width/icon_width + (1-mini_width/icon_width)*enl,
		  mini_height/icon_height + (1-mini_height/icon_height)*enl);
      cairo_set_source_surface(cr, save[i]->surf, 0, 0);
      cairo_paint(cr);
      if(i != focused && mode == NORMAL){
	if(enl_marked && i == marked)
	  cairo_set_source_rgba(cr, 0, 0, 0, (enl-enl_marked) * DARKEN);
	else cairo_set_source_rgba(cr, 0, 0, 0, enl * DARKEN);
	cairo_rectangle(cr, 0, 0, icon_width, icon_height);
	cairo_fill(cr);
      }
      cairo_restore(cr);
    }
    if(gsaves_vertical) cairo_translate(cr, 0, mini_len+BORDER+save[i]->shift);
    else cairo_translate(cr, mini_len+BORDER+save[i]->shift, 0);
  }

  cairo_restore(cr);

  repaint_win(0, 0, gsaves_x_size, gsaves_y_size);
}

void hidegsaves(cairo_t *cr)
{
  if(!bg_surf) return;

  cairo_rectangle(cr, 0, 0, gsaves_x_size, gsaves_y_size);
  cairo_set_source_surface(cr, bg_surf, 0, 0);
  cairo_fill(cr);

  if(!savesnum){
    cairo_surface_destroy(bg_surf);
    bg_surf = NULL;
  }
}

void gsaves_click()
{
  if(!savesnum) return;

  if(active){
    drag_src_x = mouse_x;
    drag_src_y = mouse_y;
    mode = HOLD;
  }
}

void gsaves_unclick()
{
  if(mode == HOLD){
    if(marked != focused){
      recent_saved = 0;
      marked = focused;
      savelist();
    }
    count_rect();
    loadmarked();
    img_change |= CHANGE_GSAVES;
    mode = NORMAL;
  }
  else if(mode == DRAG){
    drop();
    gsaves_pointer(1, 0);
  }
}

static void count_shift()
{
  int i;
  double enl, s2;

  for(i=0; i<savesnum; i++){
    enl = act*save[i]->enlarged;
    if(enl_marked && i == marked) enl = 1-(1-enl_marked)*(1-enl);
    save[i]->shift = enl*(icon_len-mini_len);
  }

  shift = save[focused]->shift * focused_pos;
  if(mode == DRAG){
    if(focused_pos < 0.5) shift += act* (focused_pos-0.5) * save[focused-1]->spaceshift;
    else if(focused_pos > 0.5) shift += act* (focused_pos-0.5) * save[focused]->spaceshift;
    for(i=0; i<savesnum; i++) save[i]->shift += act*save[i]->spaceshift;
  }
  for(i=0; i<focused; i++) shift += save[i]->shift;
  if(enl_marked && act < 1){
    s2 = save[marked]->shift * 0.5;
    for(i=0; i<marked; i++) s2 += save[i]->shift;
    shift = act*shift + (1-act)*s2;
  }
}

static void count_rect()
{
  int i;
  double enl;

  if(marked < 0) return;

  rect_pos = start-shift-BORDER;
  for(i=0; i<marked; i++) rect_pos += mini_len+BORDER+save[i]->shift;

  enl = 1-(1-act*save[marked]->enlarged)*(1-enl_marked);
  rect_border = (MINI_SCALE + (1-MINI_SCALE)*enl)*RECT_BORDER;
  rect_width = mini_width + enl*(icon_width-mini_width) + 2*rect_border;
  rect_height = mini_height + enl*(icon_height-mini_height) + 2*rect_border;

  rect_x = rect_y = BORDER - rect_border;

  if(gsaves_vertical) rect_y += rect_pos;
  else rect_x += rect_pos;

  if(marked == focused || mode == DRAG) rect_col = 1;
  else rect_col = 1 - (enl-enl_marked) * DARKEN;
}

static void drag()
{
  int i;
  cairo_surface_t *surf;
  cairo_t *cr;
  int width, height;

  mode = DRAG;
  if(marked > focused) marked--;
  else if(marked == focused) marked = -1;
  dragged = save[focused];
  savesnum--;
  for(i=focused; i<savesnum; i++) save[i] = save[i+1];

  for(i=0; i<savesnum; i++) save[i]->spaceshift = 0;
  if(focused > 0) save[focused-1]->spaceshift = SPACE;

  calculate_gsaves();
  i = floor(start + (mini_len+BORDER)*focused - BORDER/2.0);
  if(gsaves_vertical) mouse_y = i;
  else mouse_x = i;
  setmouse();
  img_change |= CHANGE_GSAVES;

  width = icon_width;
  height = icon_height;
  if(marked == -1){
    width += 2*BORDER;
    height += 2*BORDER;
  }
  if(gsaves_vertical){
    drag_x = DRAGPOS;
    drag_y = -height/2;
  }
  else{
    drag_x = -width/2;
    drag_y = DRAGPOS;
  }

  drag_win = XCreateSimpleWindow(display, real_win,
				 mouse_x + drag_x, mouse_y + drag_y,
				 width, height,
				 0, BlackPixel(display,screen), BlackPixel(display,screen));

  drag_pxm = XCreatePixmap (display, drag_win, width, height, DefaultDepth (display, screen));
  XSetWindowBackgroundPixmap(display, drag_win, drag_pxm);
  surf = cairo_xlib_surface_create (display, drag_pxm, DefaultVisual(display, screen),
				    width, height);
  cr = cairo_create(surf);

  if(marked == -1){
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_set_source_surface(cr, dragged->surf, BORDER, BORDER);
    cairo_paint(cr);
  }
  else{
    cairo_set_source_surface(cr, dragged->surf, 0, 0);
    cairo_paint(cr);
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surf);
  XMapWindow(display, drag_win);
}

static void drop()
{
  int i;

  mode = NORMAL;
  XFreePixmap(display, drag_pxm);
  XDestroyWindow(display, drag_win);

  if(active){
    savesnum++;
    for(i=savesnum-1; i>spaced+1; i--){
      save[i] = save[i-1];
    }
    save[i] = dragged;
    if(marked > spaced){
      marked++;
    }
    else if(marked == -1) marked = i;

    calculate_gsaves();
    i = floor(start + (spaced+1)*(mini_len+BORDER) + mini_len/2.0);
    if(gsaves_vertical) mouse_y = i;
    else mouse_x = i;
    setmouse();
  }
  else{
    save[savesnum] = dragged;
    if(dragged->surf) cairo_surface_destroy(dragged->surf);
    if(dragged->scaled) cairo_surface_destroy(dragged->scaled);
    if(marked == -1){
      recent_saved = 0;
      marked = savesnum-1;
    }
  }
  img_change |= CHANGE_GSAVES;
  savelist();
}

void gsaves_pointer(char force, char leave_ev)
{
  double d;
  int i;
  double last_mouse_pos;
  char oac;

  if(mode == HOLD){
    if((mouse_x-drag_src_x)*(mouse_x-drag_src_x) +
       (mouse_y-drag_src_y)*(mouse_y-drag_src_y) > DRAGDIST){
      drag();
      force = 1;
    }
    else return;
  }

  if(mode == DRAG && !leave_ev){
    XMoveWindow(display, drag_win, mouse_x + drag_x, mouse_y + drag_y);
    XFlush(display);

    oac = active;
    if(gsaves_vertical){
      if(active){
	if(mouse_x <= gsaves_x_size + TRASHDIST) active = 1;
	else active = 0;
      }
      else{
	if(mouse_x <= gsaves_width + TRASHDIST) active = 1;
	else active = 0;
      }
    }
    else{
      if(active){
	if(mouse_y <= gsaves_y_size + TRASHDIST) active = 1;
	else active = 0;
      }
      else{
	if(mouse_y <= gsaves_height + TRASHDIST) active = 1;
	else active = 0;
      }
    }
    if(oac != active){
      if(active) XSetWindowBackgroundPixmap(display, drag_win, drag_pxm);
      else XSetWindowBackground(display, drag_win, BlackPixel(display,screen));
      XClearWindow(display, drag_win);
    }
  }

  if(!savesnum) return;

  if(!leave_ev){
    last_mouse_pos = mouse_pos;
    mouse_pos = gsaves_vertical ? mouse_y : mouse_x;
    if(force || mouse_pos != last_mouse_pos){
      focused_pos = mouse_pos;
      focused_pos -= start - BORDER / 2.0;
      focused_pos /= mini_len + BORDER;
      focused = floor(focused_pos);
      if(focused < 0) focused = 0;
      else if(focused >= savesnum) focused = savesnum-1;
      focused_pos -= focused;

      if(focused == 0 && focused_pos < 0.5) focused_pos = 0.5;
      if(focused == savesnum-1 && focused_pos > 0.5) focused_pos = 0.5;

      for(i=0; i<savesnum; i++) save[i]->enlarged = 0;

      save[focused]->enlarged = 1;

      d = MAGNET*focused_pos + (1-MAGNET);
      for(i=focused+1; i<savesnum && d > 0; i++){
	save[i]->enlarged = d;
	d -= MAGNET;
      }
      d = 1-focused_pos*MAGNET;
      for(i=focused-1; i >= 0 && d > 0; i--){
	save[i]->enlarged = d;
	d -= MAGNET;
      }

      if(act || enl_marked) img_change |= CHANGE_GSAVES;
    }
  }

  if(mode == DRAG){
    if(savesnum == 1){
      if(mouse_pos > gsaves_length/2) spaced = 0;
      else spaced = -1;
    }
    else{
      if(focused_pos > 0.5 || (focused_pos == 0.5 && focused == savesnum-1))
	spaced = focused;
      else if(focused > 0) spaced = focused-1;
      else spaced = -1;
    }
  }

  if(mode == NORMAL && !mouse_pressed){
    if(mouse_x < 0 || mouse_x > gsaves_x_size || mouse_y < 0 ||
       mouse_y > gsaves_y_size) active = 0;
    else{
      if(active){
	if(mouse_pos >= start-(icon_len-mini_len)/2-BORDER &&
	   gsaves_length-mouse_pos >= start-(icon_len-mini_len)/2-BORDER)
	  active = 1;
	else active = 0;
      }
      else if(!leave_ev){
	if(mouse_pos < start || gsaves_length-mouse_pos < start) active = 0;
	else if(gsaves_vertical){
	  if(mouse_x < gsaves_width) active = 1;
	  else active = 0;
	}
	else{
	  if(mouse_y < gsaves_height) active = 1;
	  else active = 0;
	}
      }
    }
  }
}

void gsaves_anim()
{
  int i;

  if(enl_marked || active || act || mode != NORMAL){
    if(!gsaves_blockanim){
      unanim_fish_rectangle();
      gsaves_blockanim = 1;
    }
  }
  else gsaves_blockanim = 0;

  if(enl_marked){
    enl_marked -= GSAVES_ACT_SPEED;
    if(enl_marked < 0) enl_marked = 0;
    img_change |= CHANGE_GSAVES;
  }
  if(active != act){
    img_change |= CHANGE_GSAVES;
    if(active){
      act += GSAVES_ACT_SPEED;
      if(act > 1) act = 1;
    }
    else{
      act -= GSAVES_ACT_SPEED;
      if(act < 0) act = 0;
    }
  }
  if(mode == DRAG){
    for(i=0; i<savesnum; i++){
      if(i == spaced){
	if(save[i]->spaceshift < SPACE){
	  if(act) img_change |= CHANGE_GSAVES;
	  save[i]->spaceshift += GSAVES_SPACE_SPEED;
	  if(save[i]->spaceshift > SPACE) save[i]->spaceshift = SPACE;
	}
      }
      else{
	if(save[i]->spaceshift > 0){
	  if(act) img_change |= CHANGE_GSAVES;
	  save[i]->spaceshift -= GSAVES_SPACE_SPEED;
	  if(save[i]->spaceshift < 0) save[i]->spaceshift = 0;
	}
      }
    }
  }
}

static void get_bg()
{
  cairo_t *cr;

  bg_surf = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				       gsaves_x_size, gsaves_y_size);
  if(gsaves_ext_change_mask){
    cr = cairo_create(bg_surf);
    cairo_set_source_surface(cr, win_surf, 0, 0);
    if(gsaves_vertical)
      cairo_rectangle(cr, gsaves_width, 0,
		      gsaves_x_size-gsaves_width, gsaves_y_size);
    else
      cairo_rectangle(cr, 0, gsaves_height, gsaves_x_size,
			 gsaves_y_size-gsaves_height);
    cairo_fill(cr);
    cairo_destroy(cr);
  }
}

void delete_gsaves()
{
  int i;

  if(mode == DRAG) drop();

  for(i=0; i<savesnum; i++){
    if(save[i]->surf && save[i]->surf != unknown_icon)
      cairo_surface_destroy(save[i]->surf);
    if(save[i]->scaled) cairo_surface_destroy(save[i]->scaled);
    free(save[i]->name);
  }
  if(unknown_icon) cairo_surface_destroy(unknown_icon);
}

void gsaves_unanim()
{
  if(act){
    act = 0;
    img_change |= CHANGE_GSAVES;
  }
  if(enl_marked){
    enl_marked = 0;
    img_change |= CHANGE_GSAVES;
  }
}

void gsaves_save()
{
  int i, j;
  char used[MAXSAVES];
  char name[20], fullname[20];
  char *type;
  struct gsave *tmp;

  if(recent_saved) return;

  if(savesnum >= MAXSAVES){
    warning("Save list if full (%d).", savesnum);
    return;
  }

  if(mode != NORMAL) gsaves_unclick();
  while(room_state != ROOM_IDLE) room_step();

  if(issolved()) type = "sol";
  else type = "sav";

  for(i=0; i<MAXSAVES; i++) used[i] = 0;
  sprintf(name, "%s_%05d_", type, moves);
  for(i=0; i<savesnum; i++){
    for(j=0; name[j] && name[j] == save[i]->name[j]; j++);
    if(!name[j] &&
       save[i]->name[j] >= '0' && save[i]->name[j] <= '9' &&
       save[i]->name[j+1] >= '0' && save[i]->name[j+1] <= '9' &&
       !save[i]->name[j+2])
      used[10*(save[i]->name[j]-'0') + (save[i]->name[j+1]-'0')] = 1;
  }
  for(i=0; used[i]; i++);

  savesnum++;
  if(savesnum == 1) j = 0;
  else{
    tmp = save[savesnum-1];
    for(j=savesnum-1; j>marked+1; j--) save[j] = save[j-1];
    save[j] = tmp;
  }
  sprintf(fullname, "%s%02d", name, i);
  save[j]->name = strdup(fullname);
  save[j]->surf = 
    imgsave(savefile(save[j]->name, "png"),
	    icon_width, icon_height, issolved(), 1);
  savemoves(savefile(save[j]->name, "fsv"));
  save[j]->scaled = NULL;
  marked = j;

  savelist();

  calculate_gsaves();
  gsaves_pointer(1, 0);
  enl_marked = 1;
  recent_saved = 1;
}

static void loadmarked()
{
  cairo_t *cr;

  dodge_gsaves();
  if(recent_saved) return;

  if(!loadmoves(savefile(save[marked]->name, "fsv")))
    warning("Loading position %s failed", savefile(save[marked]->name, "fsv"));
  else{
    unanimflip();
    recent_saved = 1;
    if(save[marked]->surf == unknown_icon){
      save[marked]->surf = 
	imgsave(savefile(save[marked]->name, "png"),
		icon_width, icon_height, issolved(), 1);
      if(save[marked]->scaled) cairo_surface_destroy(save[marked]->scaled);
      save[marked]->scaled =
	cairo_image_surface_create(CAIRO_FORMAT_ARGB32, mini_width, mini_height);
      cr = cairo_create(save[marked]->scaled);
      cairo_scale(cr, mini_width/icon_width, mini_height/icon_height);
      cairo_set_source_surface(cr, save[marked]->surf, 0, 0);
      cairo_paint(cr);
      cairo_destroy(cr);
    }
  }
}

void gsaves_load()
{
  if(!savesnum || recent_saved) return;

  if(mode != NORMAL) gsaves_unclick();

  if(marked != focused || act < 1) enl_marked = 1;
  loadmarked();
}
