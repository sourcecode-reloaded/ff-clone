#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>

#include "script.h"
#include "warning.h"
#include "object.h"
#include "rules.h"
#include "layers.h"
#include "draw.h"

static ff_image *local_images = NULL; // Prvni lokalni obrazek ve spojaku
char local_image_mode = 0;

// Vyrobi novy obrazek (ale potrebuje uz existujici sairo_surface) a vrati na nej pointer
ff_image *new_image (cairo_surface_t *surf, float x, float y,
		     float scalex, float scaley)
{
  ff_image *result;

  result = (ff_image *)mymalloc(sizeof(ff_image));
  result->surf = surf;
  result->x = x;
  result->y = y;
  result->scalex = scalex;
  result->scaley = scaley;
  result->scaled = NULL;
  result->width = cairo_image_surface_get_width(surf)*scalex;
  result->height = cairo_image_surface_get_height(surf)*scaley;

  if(local_image_mode){
    result->next = local_images;
    local_images = result;
  }
  else result->next = NULL;

  return result;
}

// znici lokalni obrazky
static void free_limages()
{
  ff_image *img;

  while(local_images){
    img = local_images;
    if(img->surf) cairo_surface_destroy(img->surf);
    if(img->scaled) cairo_surface_destroy(img->scaled);
    free(img);

    local_images = local_images->next;
  }
}

ff_image *load_png_image (const char *filename, float x, float y, // Nacte obrazek z PNG
			  float scalex, float scaley)
{
  ff_image *result;

  result =  new_image(cairo_image_surface_create_from_png(filename),
		      x, y, scalex, scaley);

  if(cairo_surface_status(result->surf) != CAIRO_STATUS_SUCCESS)
    warning("loading image '%s' failed", filename);

  return result;
}

/*
  Vrstvy (struct ff_layer) radim do spojoveho seznamu, podle ktereho jsou pak
  nasledne vykreslovany. Pri zarazovani ff_layer do seznamu pritom dbam na to,
  aby byly serazeny vzestupne podle depth. Pokud tam jiz nejake vrstvy dane depth
  jsou, zaradim tu novou na konec.

  Tento hlavni spojak je uvozen promennou first_layer a navazuje polozkou next.
  Dale tu mam jeste jeden kratsi spojak, ktery skace jen po poslednich prvcich
  jednotlivych hloubek. Ten je uvozen promennou first_depth a navazuje polozkou
  nextdepth.
 */
static ff_layer *first_layer = NULL;
static ff_layer *first_depth = NULL;

ff_layer *new_layer (ff_object *obj, float depth)
{
  ff_layer *l, **d, **ll;

  l = (ff_layer *)mymalloc(sizeof(ff_layer));

  ll = &first_layer;
  for(d = &first_depth; *d && (*d)->depth < depth; d = &((*d)->nextdepth))
    ll = &((*d)->next);
  if(*d && (*d)->depth == depth){ // pridavam do jiz existujici hloubky
    l->next = (*d)->next;
    l->nextdepth = (*d)->nextdepth;
    (*d)->next = l;
    *d = l;
  }
  else{ // zakladam novou hloubku
    l->nextdepth = *d;
    l->next = *ll;
    *ll = *d = l;
  }

  l->img = NULL;
  l->obj = obj;
  l->depth = depth;

  return l;
}

/*
  Podiva se, jestli ma scaled v obrazky spravnou velikost, a jestli ne, tak to prepocita.
  Parametry scalex, scaley udavaji zvetseni, tedy je treba je obratit.
*/
static void rescale_image (ff_image *img, float scalex, float scaley)
{
  float curscalex, curscaley;
  cairo_t *cr;

  curscalex = 1.0/scalex;
  curscaley = 1.0/scaley;

  if(img->scaled && curscalex == img->curscalex && curscaley == img->curscaley) return;
  if(img->scaled) cairo_surface_destroy(img->scaled);

  img->curscalex = curscalex;
  img->curscaley = curscaley;

  img->width = ceil(scalex*img->scalex*cairo_image_surface_get_width(img->surf));
  img->height = ceil(scaley*img->scaley*cairo_image_surface_get_height(img->surf));

  img->scaled = cairo_image_surface_create
    (cairo_image_surface_get_format(img->surf), img->width, img->height);

  cr = cairo_create(img->scaled);
  cairo_scale(cr, scalex*img->scalex, scaley*img->scaley);
  cairo_set_source_surface(cr, img->surf, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);
}

/*
  Vykresli castecne otoceni vrstvy (podle gflip objektu)
 */
static void drawgflip(cairo_t *cr, ff_layer *l)
{
  float width, height;

  width = l->img->scalex*cairo_image_surface_get_width(l->img->surf)/l->img->curscalex;
  height = l->img->scaley*cairo_image_surface_get_height(l->img->surf)/l->img->curscaley;

  cairo_translate(cr, 0, height/2);

  cairo_save(cr);
  cairo_scale(cr, 1, cosf(M_PI/2*l->obj->gflip));
  cairo_translate(cr, 0, -height/2);
  cairo_rectangle(cr, 0, height/2, width, height/2);
  cairo_set_source_surface(cr, l->img->scaled, 0, 0);
  cairo_fill(cr);
  cairo_restore(cr);

  cairo_save(cr);
  cairo_translate(cr, l->obj->width / l->img->curscalex, 0);
  cairo_scale(cr, -1, sinf(M_PI/2*l->obj->gflip));
  cairo_translate(cr, 0, -height/2);
  cairo_set_source_surface(cr, l->img->scaled, 0, 0);
  cairo_paint(cr);

  cairo_restore(cr);

  cairo_save(cr);
  cairo_scale(cr, 1, cosf(M_PI/2*l->obj->gflip));
  cairo_translate(cr, 0, -height/2);
  cairo_rectangle(cr, 0, 0, width, height/2);
  cairo_set_source_surface(cr, l->img->scaled, 0, 0);
  cairo_fill(cr);
  cairo_restore(cr);  
}

// Vykresli vsechny vrstvy (pri hre)
void draw_layers (cairo_t *cr, float scalex, float scaley)
{
  ff_layer *l;

  for(l=first_layer; l; l = l->next){
    if(!l->img) continue; // neni obrazek k vykresleni
    if(l->obj && l->obj->out == 2) continue; // objekt je venku z mistnosti

    rescale_image(l->img, scalex, scaley); // pro jistotu, jetsli ma obrazek spravnou velikost

    cairo_save(cr);
    if(l->obj){ // posunu se podle souradnic objektu
      cairo_translate(cr, l->obj->c.x, l->obj->c.y);
      if(ismoving(l->obj)){
	if(room_movedir == UP) cairo_translate(cr, 0, -anim_phase); // objekt je posouvan
	if(room_movedir == DOWN) cairo_translate(cr, 0, anim_phase);
	if(room_movedir == LEFT) cairo_translate(cr, -anim_phase, 0);
	if(room_movedir == RIGHT) cairo_translate(cr, anim_phase, 0);
      }
      if(isonfish(l->obj)) cairo_translate(cr, heap_x_advance, heap_y_advance);
         // viz draw.c: animheap()

      if(l->obj->gflip == 1){ // zcela otocena ryba
	cairo_translate(cr, l->obj->width, 0);
	cairo_scale(cr, -1, 1);
      }
    }
    cairo_translate(cr, l->img->x, l->img->y); // nastavim spravne souradnice a velikost
    cairo_scale(cr, l->img->curscalex, l->img->curscaley);

    if(l->obj && l->obj->gflip > 0 && l->obj->gflip < 1) drawgflip(cr, l); // castecne otocena ryba
    else{
      cairo_set_source_surface(cr, l->img->scaled, 0, 0); // kreslim
      cairo_paint(cr);
    }
    cairo_restore(cr);
  }
}

void draw_layers_noanim (cairo_t *cr)
    // Vykresli vsechny vrstvy, ovsem bez animaci, za ucelem vytvoreni nahledu
{
  ff_layer *l;

  for(l=first_layer; l; l = l->next){
    if(!l->img) continue;
    if(l->obj && l->obj->out == 2) continue;

    cairo_save(cr);
    if(l->obj){
      cairo_translate(cr, l->obj->c.x, l->obj->c.y);
      if(l->obj->flip){
	cairo_translate(cr, l->obj->width, 0);
	cairo_scale(cr, -1, 1);
      }
    }
    cairo_translate(cr, l->img->x, l->img->y);
    cairo_scale(cr, l->img->scalex, l->img->scaley);
    cairo_set_source_surface(cr, l->img->surf, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
  }
}

char layers_change() /* vrati CHANGE_ROOM, jestli se nejaka vrstva zmenila
			od posledniho volani teto funkce, jinak vrati 0 */
{
  ff_layer *l;
  char result;
  float x, y;

  result = 0;

  for(l = first_layer; l; l = l->next){
    if(!l->img || (l->obj && l->obj->out == 2)){ // zmena existence obrazku
      if(l->last_img) result = CHANGE_ROOM;
      l->last_img = NULL;
      continue;
    }

    x = y = 0; // spocitam nove souradnice

    if(l->obj){ // zapocitam do nich pozici a posunuti objektu
      if(l->obj->gflip != l->obj->ogflip) result = CHANGE_ROOM;
      l->obj->ogflip = l->obj->gflip;

      x += l->obj->c.x; y += l->obj->c.y;
      if(ismoving(l->obj)){
	if(room_movedir == UP) y -= anim_phase;
	if(room_movedir == DOWN) y += anim_phase;
	if(room_movedir == LEFT) x -= anim_phase;
	if(room_movedir == RIGHT) x += anim_phase;
      }
      if(isonfish(l->obj)){ x += heap_x_advance; y += heap_y_advance; }
    }
    x += l->img->x; y += l->img->y; // a posunuti obrazku

    if(x != l->last_x || y != l->last_y || l->img != l->last_img)
      result = CHANGE_ROOM;

    l->last_img = l->img;
    l->last_x = x;
    l->last_y = y;
  }

  return result;
}

int script_new_layer(lua_State *L) // lua verze new_layer()
{
  ff_object *obj = lua_touserdata(L, 1);
  float depth = luaL_checknumber(L, 2);

  lua_pushlightuserdata(L, new_layer(obj, depth));

  return 1;
}

int script_load_png_image(lua_State *L) // lua verze load_png_layer()
{
  const char *filename = luaL_checkstring(L, 1);
  float x = luaL_checknumber(L, 2);
  float y = luaL_checknumber(L, 3);

  ff_image *result = load_png_image(filename, x, y, 1.0/SQUARE_WIDTH, 1.0/SQUARE_HEIGHT);

  if(cairo_surface_status(result->surf) == CAIRO_STATUS_SUCCESS) lua_pushlightuserdata(L, result);
  else lua_pushnil(L); // pri selhani vrati nil

  return 1;
}

int script_setlayerimage(lua_State *L) // nastavi vrstve obrazek (objekt dostala pri vytvoreni)
{
  ff_layer *l = (ff_layer *)my_luaL_checkludata(L, 1);
  ff_image *img = (ff_image *)my_luaL_checkludata(L, 2);
  l->img = img;

  return 0;
}

void delete_layers() // smaze vrstvy a lokalni obrazky
{
  ff_layer *l;

  free_limages();

  while(first_layer){
    l = first_layer;
    first_layer = first_layer->next;
    free(l);
  }
  first_depth = NULL;
}


// v soucasne dobe nema vyznam, nepovedeny pokus o optimalizaci
void count_layers_boundary(int index, char first, cairo_t *cr)
{
  float x, y;
  int x1, y1, x2, y2, i;
  ff_layer *l;

  for(l=first_layer; l; l = l->next){
    if(!l->img || (l->obj && l->obj->out == 2)){
      if(l->last_img) l->change = 1;
      else l->change = 0;
      l->last_img = NULL;
    }

    rescale_image(l->img, room_x_scale, room_y_scale);

    x = y = 0;

    if(l->obj){
      x += l->obj->c.x; y += l->obj->c.y;
      if(ismoving(l->obj)){
	if(room_movedir == UP) y -= anim_phase;
	if(room_movedir == DOWN) y += anim_phase;
	if(room_movedir == LEFT) x -= anim_phase;
	if(room_movedir == RIGHT) x += anim_phase;
      }
      if(isonfish(l->obj)){ x += heap_x_advance; y += heap_y_advance; }
    }
    x += l->img->x; y += l->img->y;

    x *= room_x_scale; y *= room_y_scale;

    if(x != l->last_x || y != l->last_y || l->img != l->last_img) l->change = 1;
    else l->change = 0;

    l->last_img = l->img;
    l->last_x = x;
    l->last_y = y;

    x1 = floor(x);
    y1 = floor(y);
    x2 = ceil(x + l->img->width);
    y2 = ceil(y + l->img->height);
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 > room_x_size) x2 = room_x_size;
    if(y2 > room_y_size) y2 = room_y_size;
    l->boundary[index].x = x1;
    l->boundary[index].y = y1;
    l->boundary[index].width = x2-x1;
    l->boundary[index].height = y2-y1;
  }

  if(first) return;

  x2 = y2 = 0;
  x1 = room_x_size;
  y1 = room_x_size;

  for(l = first_layer; l; l = l->next){
    if(l->change){
      /*
      for(i=0; i<2; i++){
	if(x1 > l->boundary[i].x) x1 = l->boundary[i].x;
	if(y1 > l->boundary[i].y) y1 = l->boundary[i].y;

	if(x2 < l->boundary[i].x+l->boundary[i].width) x2 = l->boundary[i].x+l->boundary[i].width;
	if(y2 < l->boundary[i].y+l->boundary[i].height) y2 = l->boundary[i].y+l->boundary[i].height;
      }
      */
      cairo_rectangle(cr, l->boundary[0].x, l->boundary[0].y,
		      l->boundary[0].width, l->boundary[0].height);
      cairo_rectangle(cr, l->boundary[1].x, l->boundary[1].y,
		      l->boundary[1].width, l->boundary[1].height);
      /*
      printf("+%3d  +%3d: %3d x %3d\n", l->boundary[0].x, l->boundary[0].y,
	     l->boundary[0].width, l->boundary[0].height);
      printf("+%3d  +%3d: %3d x %3d\n", l->boundary[1].x, l->boundary[1].y,
	     l->boundary[1].width, l->boundary[1].height);
      */
    }
  }
  //printf("\n\n\n");
  /*
  if(x1 < x2)
    cairo_rectangle(cr, x1, y1, x2-x1, y2-y1);
  */
}
