#include<stdlib.h>
#include<cairo/cairo.h>

#include "warning.h"
#include "script.h"
#include "object.h"
#include "rules.h"
#include "moves.h"
#include "gsaves.h"
#include "draw.h"
#include "main.h"

/*
  Undo historie je ulozena ve spojovem seznamu struktury movesblock,
  kazda tato struktura ma ulozeno MOVESBLOCK (100) tahu a pozici vsech
  objektu na jejim zacatku.

  Ulozene pozice objektu jsou v poli struktury possct. Pocet prvku tohoto
  pole je stejne jako objektu a je serazeno tak, jak objekty ve spojovem
  seznamu.

  Tah je ulozeny jako seznam (pole) objektu, ktere zmenily pozici a dale
  dalsi seznam (pole) ryb, ktere zemrely nebo se otocily. Nakonec tam je
  jeste (pro redo) ulozene, ktera ryba se hybala kterym smerem.
 */

#define MOVESBLOCK 100

struct movedsct{
  ff_object *obj; // objekt
  coor oric;      // a jeho puvodni souradnice
};

struct changesct{
  ff_object *obj; // objekt
  char type;      // a typ jeho zmeny (FLIPCHANGE, LIVECHANGE)
};

struct possct{
  coor c;      // souradnice objektu
  char flip, live;  // dalsi jeho vlastnosti
};

struct movesct{
  struct movedsct *moved;  // zmeny pozic objektu
  int movednum; // pocet prvku moved

  struct changesct *changed; // zmeny stavu ryb
  int changednum; // pocet prvku changed

  char dir;  // V tahu sla timto smerem
  ff_object *fish;    // tato ryba.
};

struct movesblock{
  struct movesct move[MOVESBLOCK]; // tahy

  struct possct *saved; // ulozena pozice

  struct movesblock *next; // dalsi blok
  struct movesblock *prev; // predchozi blok
};

/*
  Pokud na zacatku mistnosti nektere predmety padaji, existuje tah -1 a objekty,
  ktere se timto zmenily jsou ulozeny v promenne beforestart. Polozky fish a dir
  zde nejsou vyuzivany.
*/
struct movesct beforestart;

struct movesblock *first; // prvni blok (ve spojaku)
struct movesblock *last;  // posledni blok ve spojaku
struct movesblock *current;  // blok, ve kterem je ulozeny tah promenne moves

static char nosave; /* Je-li tato promenna nastavena na nenulovou hodnotu, neprovadi
		       se ukladani undo-historie. Pokud je to nastaveno ne 1, je
		       jeho hodnota vynulovana po skonceni tahu. */

static void savepos();
static void loadpos(int block);
static void undoto(int move);
static void redoto(int move);
static void freeredo();

/************************************************************************/
/**********************  pouzivani undo historie  ***********************/

static void undoto(int move) // po jednom vraci tahy az do daneho
{
  int i;
  struct movesct *m;

  for(; moves > move; moves--){
    if(moves == 0) m = &beforestart;
    else{
      if(moves%MOVESBLOCK == 0) current = current->prev;     
      m = &current->move[(moves-1)%MOVESBLOCK];
    }

    for(i=0; i<m->movednum; i++){ // posouvam objekty
      hideobject(m->moved[i].obj);
      m->moved[i].obj->c = m->moved[i].oric;
    }
    for(i=0; i<m->movednum; i++){
      showobject(m->moved[i].obj);
    }

    for(i=0; i<m->changednum; i++){ // otacim a ozivuji ryby
      if(m->changed[i].type == FLIPCHANGE)
	m->changed[i].obj->flip = !m->changed[i].obj->flip;
      else if(m->changed[i].type == LIVECHANGE)
	vitalizefish(m->changed[i].obj);
      else warning("undoto: unknown change type %d",
		   m->changed[i].type);
    }
  }
}

static void redoto(int move) // po jednom provadi tahy az do daneho
{
  while(moves < move){
    setactivefish(current->move[moves%MOVESBLOCK].fish);
    movefish(current->move[moves%MOVESBLOCK].dir);
    while(room_state != ROOM_IDLE) room_step();
  }
}

static void loadpos(int block) // z bloku daneho indexu nacte ulozenou pozici
{
  int i;
  ff_object *obj;

  // nejdriv vyzkouma, odkud to ma nejbliz
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

  while(i < block){ // posune current na spravny index:
    current = current->next;
    i++;
  }
  while(i > block){
    current = current->prev;
    i--;
  }

  // prenacte pozice objektu a stavy ryb
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

/*
  Vyvrcholenim teto kapitoly je funkce setmove(). Tato funkce najde nejjednodusi zpusob,
  jak se k zadanemu tahu dostat, a ten pak realizuje. Navratova hodnota udava, zda ke zmene tahu opravdu
  doslo.
 */

#define UNDOCOST  1 // cena jednoho unda
#define REDOCOST  1 // cena jednoho reda
#define LOADCOST 10 // cena nacteni cele ulozene pozice

// moznosti, jak se dopracovat k pozici:
#define UNDO     0  // postupne pouzivat undo, nez se tam dostanu
#define REDO     1  // postupne pouzivat redo, nez se tam dostanu
#define LOADUNDO 2  // nacist nejblizsi vyssi ulozenou pozici vsech objektu a teprv potom opakovat undo
#define LOADREDO 3  // nacist nejblizsi nizsi ulozenou pozici vsech objektu a teprv potom opakovat redo

char setmove(int move)
{
  int cost[4], loadredopos, loadundopos;
  int i, min;
  char forward;

  if(move > maxmoves) move = maxmoves; // oseka tah do spravnych mezi
  if(move < minmoves) move = minmoves;

  if(move == moves) return 0;  // neni, co menit
  while(room_state != ROOM_IDLE) room_step(); // je-li neco v pohybu, preskocim na konec tahu
  if(room_sol_esc) return 0;  // neblazni s vracenim tahu, vzdyt jsi ted vyresil mistnost
  if(move == moves) return 0; // neni, co menit

  img_change |= CHANGE_GMOVES;  // zmeni se undo pasecek

  // spocita ceny jednotlivych zpusobu dosazeni pozice (-1 = nepouzitelny zpusob)
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

  // najde nejlepsi zpusob dosazeni pozice
  min = 0;
  for(i=1; i<4; i++)
    if(cost[i] >= 0 && (cost[min] < 0 || cost[i] < cost[min])) min = i;
  if(cost[min] == -1){
    warning("No way to move %d found", move);
    return 0;
  }

  // pouzije nalezeny zpusob
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

  if(move == minmoves){ // vymysli, jaka ryba by mela byt aktivni
    if(start_active_fish) setactivefish(start_active_fish);
    else if(maxmoves > 0) setactivefish(first->move[0].fish);
  }
  else if(forward){
    if(move%MOVESBLOCK) setactivefish(current->move[(move-1)%MOVESBLOCK].fish);
    else setactivefish(current->prev->move[(move-1)%MOVESBLOCK].fish);
  }
  else setactivefish(current->move[move%MOVESBLOCK].fish);

  moves = move;
  if(move == -1){
    fallall();
    nosave = 1;  // nebudeme znovu ukladat padani predmetu pred startem
  }
  else room_state = ROOM_IDLE;

  start_moveanim();
  unanimflip(); // nebudeme animovat, jak se ryby pozvolne otaceji zpatky

  return 1;
}

/**********************  pouzivani undo historie  ***********************/
/************************************************************************/
/***********************  ukladani undo historie  ***********************/

static void savepos() // vytvori v current polozku saved
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

/*
  Nechci si ukladat zmenu pozice u jednoho objektu vicekrat. Proto pouzivam objlist MOVEDLIST,
  kde mam na zacatku movednum objektu, u kterych jiz mam zaznamenanou puvodni souradnici,
  v poli oric na prislusnem indexu.
 */
static int movednum;
static coor *oric;
#define MOVEDLIST  4

/*
  U zmen ryb nepredpokladam, ze jich bude za tah vice nez pocet ryb plus jedna
  (jedno otoceni a vsechna umrti). Pro jistotu mam stejne delku provizorniho pole, kam si
  budu zmeny ryb ukladat, v promenne maxchanges.
 */
static int changednum;
static struct changesct *changed;
static int maxchanges;

static void createfirstblock() // Vytvori prvni blok, od te doby uz tu bude az do ukonceni levelu.
{
  first = current = last = (struct movesblock *)mymalloc(sizeof(struct movesblock));
  savepos();
}

void init_moves() /* Inicializuje struktury pro undo historii, nutno spoustet
		     az po init_rules(), aby vedel, zda bude existovat tah -1. */
{
  first = current = last = NULL;
  movednum = changednum = 0;
  room_sol_esc = 0;

  maxchanges = fishnum+1;
  oric = (coor *)mymalloc(objnum*sizeof(coor));
  changed = (struct changesct *)mymalloc(maxchanges*sizeof(struct changesct));
  maxmoves = 0;

  if(room_state == ROOM_BEGIN) minmoves = moves = -1; // prvni blok zalozim, az predmety popadaji
  else{
    minmoves = moves = 0;
    createfirstblock();
    beforestart.movednum = beforestart.changednum = 0; // tentokrat se pred startem nic nedelo
    beforestart.moved = NULL;
    beforestart.changed = NULL;
  }
}

void startmove(ff_object *fish, int dir)
{
  start_moveanim();  // viz draw.c

  img_change |= CHANGE_GMOVES; // zmena poctu tahu, nutno prekreslit undopasecek
  if(nosave){
    moves++;
    return;
  }
  if(maxmoves > moves){
    if(current->move[moves%MOVESBLOCK].fish != fish ||
       current->move[moves%MOVESBLOCK].dir != dir) freeredo();
    else{ // pokud je tah stejny jako je redo, neni treba redo prepisovat
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

void addobjmove(ff_object *obj) // ulozi pozici objektu, bude se hybat
{
  if(nosave) return;
  if(obj->index[MOVEDLIST] < movednum) return;
  swapobject(obj, movednum, MOVEDLIST);
  oric[movednum] = obj->c;
  movednum++;
}

void addobjchange(ff_object *obj, char type) // ulozi zmenu ryby
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

void finishmove() // presune provizorne ukladana data do prislusneho bloku
{
  int i;
  struct movesct *m;

  if(!nosave){
    if(!current) m = &beforestart;
    else m = &current->move[(moves-1)%MOVESBLOCK];

    m->movednum = movednum;
    if(movednum) m->moved = (struct movedsct *)mymalloc(movednum*sizeof(struct movedsct));
    else m->moved = NULL;
    for(i=0; i<movednum; i++){
      m->moved[i].obj = objlist[MOVEDLIST][i];
      m->moved[i].oric = oric[i];
    }

    m->changednum = changednum;
    if(changednum) m->changed = (struct changesct *)mymalloc(changednum*sizeof(struct changesct));
    else m->changed = NULL;
    for(i=0; i<changednum; i++){
      m->changed[i].obj = changed[i].obj;
      m->changed[i].type = changed[i].type;
    }

    if(issolved() && !userlev) room_sol_esc = 1;  // mistnost je vyresena, vratime se do menu
  }

  if(moves == -1){  // konci tah -1
    if(!nosave) createfirstblock();
    img_change |= CHANGE_GMOVES;
    moves = 0;
  }
  else if(moves % MOVESBLOCK == 0){ // dosel-li blok, jdu na dalsi
    if(!nosave){
      last = current->next = (struct movesblock *)mymalloc(sizeof(struct movesblock));
      last->prev = current;
    }
    current = current->next;
    if(!nosave) savepos();
  }
  if(nosave == 1) nosave = 0;
}

/***********************  ukladani undo historie  ***********************/
/************************************************************************/
/************************  zapisovani do souboru  ***********************/

/*
  Format souboru, do ktereho je zapisovano reseni:
  Na zacatku je index (ve FISHLIST) aktivni ryby 0, dale kazdy znak znamena
  akci podle nasledujici tabulky:

  '+' - index aktivni ryby se zvysi o 1 (tedy zmena aktivni ryby)
  '-' - index aktivni ryby se snizi o 1 (tedy zmena aktivni ryby)
  '^', 'v', '<', '>' - posun nahoru, dolu, doleva, doprava
  '.' - Do tohoto tahu skocit po nacteni vseho. (neni-li, zustanu v poslednim tahu)
 */

char savemoves(char *filename)
{
  FILE *f;
  int i, icur;  // prave ukladany tah, tah, kde napisu tecku
  int af, last_af;  // index aktivni ryby, minuly index aktivni ryby
  struct movesblock *mb; // blok, ktery prave ctu

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
  char dir; // smer pohybu, -1 = byla jina udalost nez pohyb
  int af; // index aktivni ryby
  int ch; // cteny znak
  int move; // tah, kam mam na konci skocit
  int badmoves = 0; // pocet tahu, ktere se nepovedlo provest

  f = fopen(filename, "r");
  if(!f) return 0;

  room_sol_esc = 0;
  setmove(0);
  freeredo();

  af = 0;
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
      if(active_fish != objlist[FISHLIST][af]) badmoves++;
      else{
	movefish(dir);
	if(room_state == ROOM_IDLE) badmoves++;
	while(room_state != ROOM_IDLE) room_step();
      }
    }
    ch = fgetc(f);
  }
  fclose(f);

  if(badmoves) warning("loadmoves: %d moves failed", badmoves);

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

/************************  zapisovani do souboru  ***********************/
/************************************************************************/
/*************************   uvolnovani pameti   ************************/

static void freeredo() /* zrusi redo, tedy maxmoves bude zas jen moves
			  pouzito pred provedenim jineho tahu nez toho z redo */
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

void free_moves()  /* uvolni pamet pouzivanou timto modulem
		      (volano pri opousteni mistnosti) */
{
  struct movesblock *mb, *tmp;
  int i;

  if(room_state != ROOM_IDLE) finishmove(); // nesmim mit rozpracovany tah

  if(beforestart.moved) free(beforestart.moved);
  if(beforestart.changed) free(beforestart.changed);

  if(oric) free(oric);

  for(mb = first; maxmoves >= 0;){ // prochazim bloky od prvniho a rusim
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

/*************************   uvolnovani pameti   ************************/
/************************************************************************/
