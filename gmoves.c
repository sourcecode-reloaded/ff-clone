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
#include "gsaves.h"
#include "menuevents.h"

/*
  Undo pasecek ma dva hlavni ovladaci prvky:
    1) panel - to, kde jsou tlacitka -, + a pocitadlo tahu
    2) samotny pasecek, na kterem je posuvnik
  Panel ma ve vertikalnim modu velikost 4x4 policka (tato policka v gmoves
  nijak nesouvisi s policky v mistnosti) horni dve 2x2 policka jsou tlacitka -, +
  a spodni 4x2 je pocitadlo tahu.
  V horizovntalnim modu ma velikost 8x2 policka, krajni dve 2x2 jsou tlacitka -, +
  a prostredni 4x2 je pocitadlo tahu.
 */

#define SCALE 21 // velikost gmoves policka

#define PANELBORDER 0.2 // tloustka car na panelu
#define DARKTHICKNESS 0.2 // tloustka pasecku a posuvniku
#define LIGHTTHICKNESS 0.1 // tloustka stinovani na pasecku a posuvniku

/*
  tlacitka -, + jejsou kreslena pomoci cairo fontu, ale rucne po obdelnickach
*/
#define MARK_THICKNESS 0.2 // tloustka cary v - / + (v gmoves polickach)
#define MARK_LENGTH    1.3 // delka cary v - / + (v gmoves polickach)
#define TEXT_BORDER    0.2 // okraj v pocitadle tahu, do ktereho nezasahuje ono cislo

// 3 moznosti stavu tlacitka
#define DISABLED 0 // nezmacknutelne
#define ENABLED  1 // zmacknutelne
#define PRESSED  2 // zmacknute

static char button_pressed; // nejake tlacitko je drzeno levym tlacitkem mysi

static float slider_length; // delka pasecku v polickach

// nasledujici ctyri barvy jsou jedine pouzite v gmoves (tedy pak jeste cerne pozadi)
static cairo_pattern_t *lorange = NULL; // svetle oranzova
static cairo_pattern_t *dorange = NULL; // tmave oranzova
static cairo_pattern_t *lblue = NULL; // svetla modra
static cairo_pattern_t *dblue = NULL; // tmave modra

void init_gmoves() // inicializni funkce - spousteno pri startu programu
{
  gmoves_width = ceil((PANELBORDER+4)*SCALE);
  gmoves_height = ceil((PANELBORDER+2)*SCALE);

  lorange = cairo_pattern_create_rgb(1, 0.68, 0);
  dorange = cairo_pattern_create_rgb(0.5, 0.34, 0);
  lblue = cairo_pattern_create_rgb(0.2, 0.2, 1);
  dblue = cairo_pattern_create_rgb(0, 0, 0.5);
}

void level_gmoves_init() // pri startu levelu nastavi undo pasecek na necinnost
{
  slider_hold = 0;
  button_pressed = 0;
  rewinding = 0;
}

static void drawslider(cairo_t *cr)
  /* Nakresli pasecek s posuvnikem na patricne misto, predpoklada, jiz za jednotky
     policka a pozici na zacatku pasecku (tedy stred a zacatek) */
{
  float pos; // pozice posuvniku: 0 = zacatek, 1 = konec
  float x; // opravdova souradnice posuvniku

  if(slider_length <= 4*DARKTHICKNESS){ // posuvnik se nevejde
    return;
  }

  if(moves <= 0) pos = 0; // -1 je stejny zacatek jako 0
  else pos = ((float)moves)/maxmoves;

  cairo_save(cr);

  // svisly pasecek vypada stejne, jen je pootoceny
  if(gmoves_vertical) cairo_rotate(cr, M_PI/2);

  cairo_move_to(cr, DARKTHICKNESS, 0); // pasecek
  cairo_line_to(cr, slider_length-DARKTHICKNESS, 0);

  cairo_set_source(cr, dblue); // nakresleni pasecku
  cairo_set_line_width(cr, DARKTHICKNESS*2);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_stroke_preserve(cr);

  cairo_set_source(cr, lblue); // stinovani pasecku
  cairo_set_line_width(cr, LIGHTTHICKNESS*2);
  cairo_stroke(cr);

  x = pos*(slider_length-4*DARKTHICKNESS)+2*DARKTHICKNESS;

  cairo_move_to(cr, x, -1+DARKTHICKNESS);  // posuvnik
  cairo_line_to(cr, x, 1-DARKTHICKNESS);

  cairo_set_source(cr, dorange); // nakresleni posuvniku
  cairo_set_line_width(cr, DARKTHICKNESS*2);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_stroke_preserve(cr);

  cairo_set_source(cr, lorange); // stinovani posuvniku
  cairo_set_line_width(cr, LIGHTTHICKNESS*2);
  cairo_stroke(cr);

  cairo_restore(cr);
}

static void drawhpanelshape(cairo_t *cr, char inside)
  /* Vytvori cary pro vodorovny panel (ale jeste je neobtahne ani nevyplni).
     Parametr inside rika, jestli se maji vytvorit i vnitrni cary. Predpoklada
     pozici v levem hornim rohu pocitadla tahu. */
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

static void drawvpanelshape(cairo_t *cr, char inside)
  /* Vytvori cary pro svisly panel (ale jeste je neobtahne ani nevyplni).
     Parametr inside rika, jestli se maji vytvorit i vnitrni cary. Predpoklada
     pozici v levem hornim rohu pocitadla tahu. */
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

/*
  Kresleni cisla je trochu slozitejsi nez obycejne vypsani textu. V cairo
  fontu totiz maji cisla ruznou velikost a tak pri zmene cisla poposkoci
  vsechny cifry, coz neni zadouci. Proto mam vlastni funkci, ktera vykresli
  cislo. Projde vsechny znaky daneho stringu a misto o sirku znaku se vzdy
  posune o dany parametr translate.
*/

static void drawnumber(cairo_t *cr, char *str, float translate)
{
  int i;
  char digit[2];

  digit[1] = 0;
  for(i=0; str[i]; i++){
    if(str[i] == '-'){ // poradne minus, ne jen spojovnik
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

// nakresli panel, predpoklada pozici v levem hornim rohu pocitadla tahu
static void drawpanel(cairo_t *cr)
{
  cairo_text_extents_t text_ext;
  float ratiox, ratioy;
  char movestring[50];

  char minusstate;
  char plusstate;

  // zjistim, jak na tom jsou jednotliva tlacitka
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

  // Pozadi
  cairo_save(cr);

  if(gmoves_vertical) drawvpanelshape(cr, 0);
  else drawhpanelshape(cr, 0);
  cairo_clip(cr);
  cairo_set_source(cr, lorange);
  cairo_paint(cr);

  // Tlacitko minus
  cairo_save(cr);
  if(gmoves_vertical) cairo_translate(cr, 0, -2); // urcim jeho pozici
  else cairo_translate(cr, -2, 0);

  if(minusstate == PRESSED){ // modre pozadi, je-li zmacknute
    cairo_set_source(cr, lblue);
    cairo_rectangle(cr, 0, 0, 2, 2);
    cairo_fill(cr);
  }

  if(minusstate == DISABLED) cairo_set_source(cr, dorange);
  else cairo_set_source(cr, dblue); // vykreslim minusko
  cairo_rectangle(cr, 1 - MARK_LENGTH/2, 1 - MARK_THICKNESS/2,
		  MARK_LENGTH, MARK_THICKNESS);
  cairo_fill(cr);
  cairo_restore(cr);

  // Tlacitko plus
  cairo_save(cr);
  if(gmoves_vertical) cairo_translate(cr, 2, -2); // urcim jeho pozici
  else cairo_translate(cr, 4, 0);

  if(plusstate == PRESSED){
    cairo_set_source(cr, lblue); // modre pozadi, je-li zmacknute
    cairo_rectangle(cr, 0, 0, 2, 2);
    cairo_fill(cr);
  }

  if(plusstate == DISABLED) cairo_set_source(cr, dorange);
  else cairo_set_source(cr, dblue);  // vykreslim plusko
  cairo_rectangle(cr, 1 - MARK_LENGTH/2, 1 - MARK_THICKNESS/2,
		  MARK_LENGTH, MARK_THICKNESS);
  cairo_rectangle(cr, 1 - MARK_THICKNESS/2, 1 - MARK_LENGTH/2,
		  MARK_THICKNESS, MARK_LENGTH);
  cairo_fill(cr);
  cairo_restore(cr);

  // Pocitadlo tahu
  sprintf(movestring, "%d", moves);

  // velikost znaku urcim na velikost nuly
  cairo_text_extents(cr, "0", &text_ext);
  text_ext.width += text_ext.x_advance*(strlen(movestring)-1);

  ratiox = (4.0-2*TEXT_BORDER)/text_ext.width;  // zvetseni, aby text sedel na sirku
  ratioy = (2.0-2*TEXT_BORDER)/text_ext.height; // zvetseni, aby text sedel na vysku
  if(ratioy < ratiox){ // velikost textu je zavisla na jeho vysce
    cairo_translate(cr, 4-TEXT_BORDER, 2-TEXT_BORDER);
    cairo_scale(cr, ratioy, ratioy);
    cairo_translate(cr, -text_ext.width, 0);
  }
  else{ // velikost textu je zavisla na jeho sirce
    cairo_translate(cr, TEXT_BORDER, 1+ratiox*text_ext.height/2);
    cairo_scale(cr, ratiox, ratiox);
  }

  // vykreslim text
  cairo_translate(cr, -text_ext.x_bearing, 0);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  drawnumber(cr, movestring, text_ext.x_advance);

  cairo_restore(cr);

  // Obtazeni panelu
  if(gmoves_vertical) drawvpanelshape(cr, 1);
  else drawhpanelshape(cr, 1);

  cairo_set_source(cr, dblue);
  cairo_set_line_width(cr, PANELBORDER);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
  cairo_stroke(cr);
}

void drawgmoves(cairo_t *cr, char repaint)
  // vykresli cele gmoves, repaint znamena, ze je pred tim treba premazat kreslici plochu
{
  int x, y, width, height; // vyska a sirka v pixelech

  cairo_save(cr);

  if(gmoves_vertical){
    x = win_width - gmoves_width; y = 0;
    width = gmoves_width; height = win_height;
  }
  else{
    x = 0; y = win_height - gmoves_height;
    width = win_width; height = gmoves_height;
  }
  cairo_rectangle(cr, x, y, width, height);
  cairo_clip(cr);

  cairo_set_source_rgb(cr, 0,0,0);
  if(repaint) cairo_paint(cr);

  if(gmoves_vertical){
    slider_length = ((float)win_height)/SCALE - (4+PANELBORDER);
    cairo_translate(cr, win_width-SCALE*(4+PANELBORDER)/2.0, 0);
  }
  else{
    slider_length = ((float)win_width)/SCALE - (8+PANELBORDER);
    cairo_translate(cr, 0, win_height-SCALE*(2+PANELBORDER)/2.0);
  }

  cairo_scale(cr, SCALE, SCALE);

  drawslider(cr);

  if(gmoves_vertical) cairo_translate(cr, -2, slider_length+2+PANELBORDER/2);
  else cairo_translate(cr, slider_length+2+PANELBORDER/2, -1);

  drawpanel(cr);

  cairo_restore(cr);
}

/* Nasledujici funkce se podiva se, jestli souradnice kliknuti jsou na pasecku
   a jestli ano, tak se patricne zachova. Souradnice jsou v polickach s pocatkem
   na zacatku pasecku uprostred (tedy stejne jako se vykresluje). */
static void slider_click(float x, float y)
{
  float pos; // pozice kliknuti: 0 = zacatek pasecku, 1 = konec pasecku

  if(gmoves_vertical){ // zkontroluje, jestli jsou souradnice na pasecku
    if(x < -1 || x > 1) return;
    if(y < DARKTHICKNESS || y > slider_length-DARKTHICKNESS) return;
    pos = (y-DARKTHICKNESS)/(slider_length-2*DARKTHICKNESS);
  }
  else{
    if(y < -1 || y > 1) return;
    if(x < DARKTHICKNESS || x > slider_length-DARKTHICKNESS) return;
    pos = (x-DARKTHICKNESS)/(slider_length-2*DARKTHICKNESS);
  }

  // upravime pos na pocet tahu, kam se ma skocit
  pos *= maxmoves;
  pos += 0.5; // kvuli zaokrouhlovani

  setmove(floor(pos));
  unanim_fish_rectangle();

  slider_hold = 1; // a ted je drzen posuvnik
  if(rewinding){   // tedy ne tlacitko
    img_change |= CHANGE_GMOVES;
    rewinding = button_pressed = 0;
  }
}

/* Nasledujici funkce se podiva se, jestli souradnice kliknuti jsou na nejakem tlacitku
   a jestli ano, tak se patricne zachova. Souradnice jsou v polickach s pocatkem
   v levem hornim rohu pocitadla tahu (tedy stejne jako se vykresluje). */
static void panel_click(float x, float y, unsigned int button)
{
  int d;

  d = 0;

  if(gmoves_vertical){
    if(y > 0 || y < -2 || x < 0 || x > 4) return;
    if(x <= 2){ // Tlacitko minus
      if(y < -1 && x < 1 && (x-1)*(x-1) + (y+1)*(y+1) > 1) return;
      d = -1;
    }
    else{ // Tlacitko plus
      if(y < -1 && x > 3 && (x-3)*(x-3) + (y+1)*(y+1) > 1) return;
      d = 1;
    }
  }
  else{
    if(x < -2 || x > 6 || y < 0 || y > 2) return;
    if(x < 0){ // Tlacitko minus
      if(x < -1 && (x+1)*(x+1) + (y-1)*(y-1) > 1) return;
      d = -1;
    }
    else if(x > 4){ // Tlacitko plus
      if(x > 5 && (x-5)*(x-5) + (y-1)*(y-1) > 1) return;
      d = 1;
    }
  }

  if(!d) return;
  if(d != rewinding){ // kliknuti na DISABLED tlacitko
    if(d == -1 && moves <= minmoves) return;
    if(d == 1 && moves >= maxmoves) return;
  }

  if(button == 1){ // levym tlacitkem mysi se tlacitka -/+ drzi
    rewind_moves(d);
    button_pressed = 1;
  }
  else{ // pravym se spousti
    if(button_pressed) return;
    if(d == rewinding){
      rewinding = 0;
      img_change |= CHANGE_GMOVES;
    }
    else rewinding = d;
  }
}

// identifikuje, zda se kliklo na panel / slider a patricne se zachova
void gmoves_click(XButtonEvent *xbutton)
{
  float x, y;
  int shift;

  x = xbutton->x;
  y = xbutton->y;

  if(x < 0 || y < 0 || x >= win_width || y >= win_height) return;
  if(gmoves_vertical){ // overim, ze se kliklo do prostoru gmoves
    x -= win_width;
    if(x < -gmoves_width) return;
  }
  else{
    y -= win_height;
    if(y < -gmoves_height) return;
  }

  if(xbutton->button == 4 || xbutton->button == 5){ // scrollovani koleckem mysi
    if(rewinding) return;
    shift = (maxmoves+100)/100;
    if(xbutton->button == 4) setmove(moves-shift);
    else setmove(moves+shift);
    unanim_fish_rectangle();
    return;
  }

  x /= SCALE; // zmena souradnic na policka
  y /= SCALE;

  if(gmoves_vertical) x += (4+PANELBORDER)/2.0; // posunuti na zacatek pasecku
  else y += (2+PANELBORDER)/2.0;

  if(xbutton->button == 1) slider_click(x, y);

  if(gmoves_vertical){ // posunuti do leveho horniho rohu pocitadla
    x += 2;
    y -= slider_length+PANELBORDER/2+2;
  }
  else{
    x -= slider_length+PANELBORDER/2+2;
    y += 1;
  }

  if(xbutton->button == 1 || xbutton->button == 3) panel_click(x, y, xbutton->button);
}

void gmoves_unclick(XButtonEvent *xbutton) // pusteni tlacitka mysi
{
  if(xbutton->button != 1) return;
  slider_hold = 0; // poustim posuvnik
  if(button_pressed){ // poustim tlacitko
    rewinding = 0;
    button_pressed = 0;
    img_change |= CHANGE_GMOVES;
  }
}

void moveslider(XMotionEvent *xmotion)
  // volano pri drzenem posouvatku pri posunu mysi, prepocita pozici posouvatka
{
  float pos;

  if(gmoves_vertical) pos = xmotion->y;
  else pos = xmotion->x;

  pos /= SCALE;
  pos -= DARKTHICKNESS;
  pos /= slider_length-2*DARKTHICKNESS;
  if(pos < 0) pos = minmoves; // muze byt i -1. tah
  else if(pos > 1) pos = maxmoves;
  else{
    pos *= maxmoves;
    pos += 0.5;
    pos = floor(pos);
  }

  setmove(pos);
  unanim_fish_rectangle();
}

static int undowait; /* pocita auto repeat u tlacitek '-' a '+', bezi do nuly
			kdy se do ni dostane 'stiskne' ono tlacitko. */

void rewind_step() // krok animace pri rewinding != 0, emuluje autorepeat
{
  if(undowait == 0){
    setmove(moves+rewinding);
    undowait = UNDOWAIT;
  }
  else undowait--;
}


void rewind_moves(int d) // zahaji prehravani tahu v danem smeru
{
  if(menumode || slider_hold) return;

  if(button_pressed) button_pressed = 0;

  undowait = FIRSTUNDOWAIT;

  if(d == -1 && moves == minmoves) return;
  if(d == 1 && moves == maxmoves) return;
  rewinding = d;
  if(!gsaves_blockanim) setmove(moves+rewinding);

  img_change |= CHANGE_GMOVES;
}

void rewind_stop(int d) // ukonci prehravani tahu v danem smeru
{
  if(menumode || slider_hold) return;

  if(!rewinding || rewinding != d) return;
  if(button_pressed) return;

  rewinding = 0;
  img_change |= CHANGE_GMOVES;
}
