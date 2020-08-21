#include <string.h>
#include <cairo/cairo.h>
#include <X11/keysym.h>
#include "warning.h"
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
#include "menuevents.h"
#include "infowindow.h"

// typy klaves
#define NO_KEY   -1 // zadna

// 0 az 3 jsou smery definovane v object.h

#define MINUS     4 // minus
#define PLUS      5 // plus, '=', ...

#define BLOCKANIM 7 // klavesa, co blokuje animaci
#define SPACE     8 // prepnuti ryby

// pocet typu klaves, u kterych me zajima, zda jsou drzeny
#define KEYNUM    6

/*****************************************************************************/
//*************************   Drzeni klaves   *********************************/

/*
  Abych mel prehled, jake typy klaves jsou jak drzeny, udrzuji si
  v poli key_holded[] u kazdeho typu klavesy, kolika klavesami je prave drzen.
  Vedle toho v poli holded_code[] si udrzuji u kazdeho drzeneho key_code, ktery
  zastupuje nejaky typ klavesy, o ktery se zajimam, ktery to vlastne je. Ve zbytku
  holded_code je NO_KEY. Pri pusteni klavesy se tedy orientuji pouze podle key_code
  a nerozhodi me tak ani napriklad prepnuti na ceskou klavesnici, vypnuti
  numlocku, ...

  Typ BLOCKANIM funguje trochu jinak, animaci blokuje vzdy jen posledni zmacknuta
  klavesa s timto typem. V promenne keyboard_blockanim si tedy uchovavam cislo o jedna
  vetsi nez index prislusne klavesy v holded_code.
 */
static int min_keycode, max_keycode; /* hranice key_code ziskane od X-serveru,
					max_keycode kvuli pohodlnejsi manipulace
					o jedna vetsi nez skutecne maximum */
static int *holded_code; /* indexace: 0 (nejmensi key_code) az
			    max_keycode-min_keycode-1 (nejvetsi key_code) */
static int key_holded[KEYNUM];

// zaregistrovani pusteni daneho key_code
static void key_up(unsigned int keycode)
{
  int i;

  if(keycode >= max_keycode || keycode < min_keycode){
    warning("keycode %d out of bounds [%d, %d)", keycode, max_keycode, min_keycode);
    return;
  }

  i = keycode - min_keycode;

  if(i+1 == keyboard_blockanim) keyboard_blockanim = 0;

  if(holded_code[i] == NO_KEY) return;

  if(!(--key_holded[holded_code[i]])){ // pustene vsechny minusy / plusy -> rusim previjeni tahu
    if(holded_code[i] == MINUS) rewind_stop(-1);
    else if(holded_code[i] == PLUS) rewind_stop(1);
  }
  holded_code[i] = NO_KEY;

  return;
}

// zaregistrovani zmacknuti daneho key_code
static void key_down(unsigned int keycode, int keyaction)
{
  int i;

  if(keycode >= max_keycode || keycode < min_keycode){
    warning("keycode %d out of bounds [%d, %d)", keycode, max_keycode, min_keycode);
    return;
  }

  i = keycode - min_keycode;

  if(holded_code[i] != NO_KEY){
    warning("double pressed keycode %d", keycode);
    key_up(keycode);
  }

  if(keyaction == BLOCKANIM) keyboard_blockanim = i+1;
  else if(keyaction >= 0 && keyaction < KEYNUM){
    holded_code[i] = keyaction;
    if(!key_holded[keyaction]++){ // zahajuji previjeni tahu
      if(keyaction == MINUS) rewind_moves(-1);
      else if(keyaction == PLUS) rewind_moves(1);
    }
  }
}

/*
  U sipek navic cekam nekolik framu nez nasadim auto repeat. U kazdeho smeru
  pri zmacknuti nastavim do prislusne polozky v key_counter hodnotu HOLDWAIT.
  Tuto hodnotu v kazdem frame snizim a teprve, az dojde do nuly, prohlasim sipku
  za opravdu drzenou.
*/
int HOLDWAIT;
static int key_counter[4];

static char arrowpressed(int dir) // vrati, jestli je dana sipka drzena
{
  return key_holded[dir] && key_counter[dir] == 0;
}

/*************************   Drzeni klaves   *********************************/
/*****************************************************************************/
/*************************   Fronta klaves   *********************************/
/*
  Do fronty si ukladam zmacknute mezery a sipky. Kazde klavese ve fronte vydrzi
  nejakou dobu, po te je odstranena. Druha varianta je, ze bude mozne klavesu provest
  (prepnuti ryb je mozne provest vzdy, posunuti ryby jen, kdyz je mistnost v klidu).

  Fronta je implementovana jako cyklicke pole queue_key, na zacatek fronty ukazuje
  index queue_start, jeji delku udava queue_length.
 */

int QUEUEWAIT;  // jak dlouho vydrzi klavesa ve fronte
#define MAXQUEUE 50  // maximalni mozna delka fronty

static char queue_key[MAXQUEUE];
static int queue_start;
static int queue_length;
static int queue_counter[MAXQUEUE]; // zbyvajici cas dane klavese, nez bude odstranena

static void queuepush(int key) // vlozi klavesu do fronty
{
  if(queue_length >= MAXQUEUE) return;
  queue_key[(queue_start+queue_length)%MAXQUEUE] = key;
  queue_counter[(queue_start+queue_length)%MAXQUEUE] = QUEUEWAIT;

  queue_length++;
}

static void queuepop() // odebere klavesu z fronty
{
  queue_start++;
  if(queue_start == MAXQUEUE) queue_start = 0;
  queue_length--;
}

void keyboard_erase_queue() // smaze frontu
{
  queue_start = queue_length = 0;
}

/*************************   Fronta klaves   *********************************/
/*****************************************************************************/

/*
  Nasledujici funkce volana kazdy frame provede nasledujici cinnosti:
  1) Projde frontu klaves, smaze klavesy, kterym dosel cas a pripadne provede klavesy na rade.
  2) Podiva se na drzene sipky a podle toho vyvola posun ryby (pokud se jeste nehybe).
 */
static char last_axis = 0; // posledni pouzity smer modulo 2
void keyboard_step()
{
  int i, dir;
  char enabled;

  enabled = (!rewinding && !gsaves_blockanim && !slider_hold && room_state == ROOM_IDLE);

  for(i=0; i<queue_length; i++) // projdu frontu
    queue_counter[(queue_start+i)%MAXQUEUE]--; // a zkratim pocitadla cekatelum

  while(queue_length){
    if(queue_counter[queue_start] == 0){ // vyhodim klavesu s proslou lhutou
      dir = queue_key[queue_start];
      queuepop();
      // nasledujici radek zarizuje, aby rychle stridani klaves znamenalo pouhe cik cak chozeni ryby
      if(queue_length && dir < 4 && queue_key[queue_start] == backdir(dir)) queuepop();
      continue;
    }
    if(queue_key[queue_start] == SPACE){ // zmena ryby muze probehnout i kdyz se ryba hybe
      changefish();
      queuepop();
      continue;
    }
    if(!enabled) break; // ale posunuti ryby uz ne
    movefish(queue_key[queue_start]);
    if(room_state != ROOM_IDLE) last_axis = dir%2;
    enabled = enabled && room_state == ROOM_IDLE; /* pokud se ryba zacala posouvat,
						     neni mozne posouvat jeste jinam */
    queuepop();
  }

  for(i=0; i<4; i++) if(key_counter[i] > 0) key_counter[i]--; // cekani na autorepeat

  if(enabled){
    /* Posunu rybu drzenym smerem. Jsou li zmacknuty dve opacne sipky,
       je to jako by nebyla zmacknuta ani jedna. Jsou-li zmacknuty dve sipky
       v ruznych osach, je preferovana ta osa, ktera nebyla pouzita naposledy.
     */
    dir = (last_axis+1)%2;
    for(i=0; enabled && i<2; i++){
      if(!arrowpressed(dir) || !arrowpressed(dir+2))
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

void kb_apply_safemode() // zaridi, aby konstanty tohoto modulu odpovidaly stavu promenne safemode
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

static int get_keyaction(KeySym ks) // Vrati typ klavesy podle KeySym
{
  switch(ks){
  case XK_Up:
  case XK_KP_Up:
    return UP;
  case XK_Down:
  case XK_KP_Down:
    return DOWN;
  case XK_Left:
  case XK_KP_Left:
    return LEFT;
  case XK_Right:
  case XK_KP_Right:
    return RIGHT;
  case XK_space:
  case XK_Tab:
    return SPACE;
  case XK_minus:
  case XK_KP_Subtract:
    return MINUS;
  case XK_plus:
  case XK_equal:
  case XK_KP_Add:
    return PLUS;
  default:
    return NO_KEY;
  }
}

/*
  Pri zmacknuti 'G', kdyz je zrovna mrizka na popredi, se mrizka vrati do
  sveho posledniho stavu. Ten je ulozeny v promenne ogridmode: false (0) znamena
  vypnuta, true znamena na pozadi.
 */
static char ogridmode = 0;

char key_press(XKeyEvent xev) // funkce volana pri udalosti zmacknuti klavesy, vrati, zda klavesa mela efekt
{
  KeySym ks;
  char result = 0;
  int keyaction = NO_KEY;

  XLookupString (&xev, NULL, 0, &ks, NULL);
  if(!ks) return 0;

  if(ks == XK_F1){
    show_help();
    return 0;
  }

  if(!menumode){
    result = 1;
    switch(ks){
    case XK_Page_Up: // zmeny tahu v undo historii jine nez -/+ provadim primo
    case XK_KP_Prior:
      if(setmove(moves-100)) keyaction = BLOCKANIM;
      break;
    case XK_Page_Down:
    case XK_KP_Next:
      if(setmove(moves+100)) keyaction = BLOCKANIM;
      break;
    case XK_Home:
    case XK_KP_Home:
      if(setmove(minmoves)) keyaction = BLOCKANIM;
      break;
    case XK_End:
    case XK_KP_End:
      if(setmove(maxmoves)) keyaction = BLOCKANIM;
      break;
    case XK_F2:
      gsaves_save(); break;
    case XK_F3:
      gsaves_load(); break;
    case XK_r:
    case XK_R:
      if(xev.state & ControlMask) refresh_user_level();
      break;
    default: keyaction = get_keyaction(ks); // -, +, mezera, sipky
      result = 0;
    }
  }

  if(keyaction != NO_KEY){
    key_down(xev.keycode, keyaction);
    if(keyaction == BLOCKANIM) unanim_fish_rectangle();
    if(keyaction != MINUS && keyaction != PLUS) rewind_stop(rewinding); // rusim previjeni tahu

    if(keyaction == SPACE || keyaction < 4){ // mezera a sipky
      if(!gsaves_blockanim && !slider_hold) queuepush(keyaction); // patri do fronty
    }
    else keyboard_erase_queue();
    if(keyaction < 4) key_counter[keyaction] = HOLDWAIT;

    return 1;
  }

  switch(ks){
  case XK_g: // mrizka na pozadi
    if(gridmode != GRID_OFF) gridmode = GRID_OFF;
    else gridmode = GRID_BG;
    img_change |= 1;
    break;
  case XK_G: // mrizka na popredi
    if(gridmode == GRID_FG) gridmode = ogridmode ? GRID_BG : GRID_OFF;
    else{
      ogridmode = (gridmode == GRID_BG);
      gridmode = GRID_FG;
    }
    img_change |= 1;
    break;
  case XK_F11: if(ks == XK_F11) fullscreen_toggle(); break;
  case XK_F12:
    safemode = !safemode;
    apply_safemode();
    break;
  case XK_Escape: escape(); break;
  default: return result;
  }

  return 1;
}

void key_release(XKeyEvent xev) // funkce volana pri udalosti pusteni klavesy
{
  XEvent nextev;

  if(XEventsQueued(display, QueuedAfterReading)){ // ignoruji autorepeat od X
    XPeekEvent(display, &nextev);
    if (nextev.type == KeyPress && nextev.xkey.time == xev.time &&
	nextev.xkey.keycode == xev.keycode){
      XNextEvent(display, &nextev);
      return;
    }
  }
  key_up(xev.keycode);
}

void level_keys_init() // volano pri spusteni levelu
{
  last_axis = 0;
  keyboard_blockanim = 0;
  keyboard_erase_queue();
}

void init_keyboard()
{
  int i;

  // ziskam hranice key_code
  XDisplayKeycodes(display, &min_keycode, &max_keycode);
  max_keycode++;

  // inicializace pole holded_code
  holded_code = mymalloc(sizeof(int)*(max_keycode-min_keycode));
  for(i = 0; i < max_keycode - min_keycode; i++) holded_code[i] = NO_KEY;

  for(i = 0; i < KEYNUM; i++) key_holded[i] = 0; // inicializace key_holded
  for(i=0; i<4; i++) key_counter[i] = 0;    // inicializace key_counter
}

// funkce, ktera prezkouma, ktere klavesy jsou drzene a ktere ne (pri zmene focusu)
void key_remap()
{
  int i, j, spc, keyaction;
  char key_vector[32], i_holded, i_oriholded;
  KeySym *ks_field;

  XQueryKeymap(display, key_vector); // ziskam zmacknuta tlacitka

  // ziskam rozlozeni klavesnice
  ks_field = XGetKeyboardMapping(display, min_keycode, max_keycode - min_keycode, &spc);

  for(i = min_keycode; i < max_keycode; i++){
    i_holded = (key_vector[i/8] & (1 << i%8)); // je drzeny key_code i?
    i_oriholded = (holded_code[i - min_keycode] != NO_KEY) ||
      (i - min_keycode + 1 == keyboard_blockanim);

    if(i_holded && i_oriholded) continue;
    // klavesa je ve stejnem stavu jako byla

    if(i_oriholded) key_up(i); // klavesa byla pustena

    if(i_holded){ // klavesa byla zmacknuta
      for(j = spc*(i-min_keycode); j < spc*(i-min_keycode+1); j++)
	if((keyaction = get_keyaction(ks_field[j])) != NO_KEY) break;
      key_down(i, keyaction);
    }
  }
  XFree(ks_field);
}
