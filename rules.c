#include<stdlib.h>
#include<cairo/cairo.h>
#include "warning.h"
#include "script.h"
#include "object.h"
#include "moves.h"
#include "rules.h"
#include "main.h"
#include "menuscript.h"

// Zde  pouzivane objlisty: (a pak jeste FISHLIST v rules.h)

// polozka out v ff_object muze nabyvat:
#define OUT_NONE     0   // objekt je cely uvnitr
#define OUT_PART     1   // objekt je castecne venku
#define OUT_ENTIRE   2   // objekt je cely venku
#define OUT_HIDDEN  -1   // objekt je skryty, viz kapitola "hraci plan"

static void printfield();
static void printmfield();

/*****************************************************************************/
/*************************   prepinani ryb   *********************************/

/*
  Ryby jsou ulozeny na zacatku objlistu FISHLIST, nejprve typu SMALL, pak typu BIG.
  Jejich celkovy pocet je udan promennou fishnum.
 */

static void nextfish() // posune se na dalsi rybu (ve FISHLISTu)
{
  if(!active_fish) return;
  active_fish = objlist[FISHLIST][(active_fish->index[FISHLIST]+1)%fishnum];
}

static void availfish() /* Zkontroluje, zda ma aktivni ryba pravo zustat aktivni
			   (napr. neni mrtva). Pokud ne, najde dalsi */
{
  ff_object *oact;

  if(!active_fish) return;

  oact = active_fish;
  while(!active_fish->live || active_fish->out == OUT_ENTIRE){
    nextfish();
    if(active_fish == oact){
      active_fish = NULL;
      break;
    }
  }
}

void changefish() // prepne na dalsi zivou, neodeslou rybu
{
  nextfish();
  availfish();
}

void setactivefish(struct ff_object *fish) // nastavi aktivni rybu
{
  active_fish = fish;
  availfish();
}

/*************************   prepinani ryb   *********************************/
/*****************************************************************************/
/***************************   hraci plan   **********************************/

/*
  Cílem je, aby bylo mo¾né rychle zjistit o daném políèku, zda (a který)
  na nìm je pøedmìt. Od toho mám pole ff_object *field[], ve kterém jsou po øádcích
  ulo¾ena políèka místnosti - buï je na nìm ukazatel na objekt nebo NULL - prázdno.

  O toto pole v¹ak je tøeba peèovat: v¾dy kdy¾ mám v plánu posunout nìkolik objektù,
  musím v¹echny tyto objekty nejprve skrýt - smazat z tohoto pole funkcí hideobject() a
  po posunutí zase do tohoto pole vrátit - funkcí showobject()

  Je¹tì je tøeba umìt posoudit políèka mimo místnost, která jsou tedy i mimo toto pole. Od
  toho existuje objlist[OUTLIST], ve kterém mám na zaèátku ty objekty, které jsou èástí
  mimo místnost. Pro dotaz na políèko mimo místnost tedy staèí projít nì. Poèet takto
  vysunutých objektù je ulo¾en v promìnné outnum. Jak moc je objekt vysunutý poèítá funkce
  showobject, která ho vykresluje do field.
 */

#define OUTLIST      3

static ff_object **field;
static int outnum;
static int remainfish; // pocet ryb, ktere jeste nejsou venku

void hideobject(ff_object *obj)
{
  int x, y;

  if(obj->out == OUT_PART) swapobject(obj, --outnum, OUTLIST);
  if(obj->out != OUT_ENTIRE && (obj->type == SMALL || obj->type == BIG))
    remainfish--;

  obj->out = OUT_HIDDEN;

  for(x=0; x<obj->width; x++)
    for(y=0; y<obj->height; y++)
      if(obj->data[x+y*obj->width] &&
	 x+obj->c.x >= 0 && x+obj->c.x < room_width &&
	 y+obj->c.y >= 0 && y+obj->c.y < room_height)
	field[(x+obj->c.x) + (y+obj->c.y)*room_width] = NULL;
}

void showobject(ff_object *obj)
{
  int x, y;

  for(x=0; x<obj->width; x++)
    for(y=0; y<obj->height; y++)
      if(obj->data[x+y*obj->width]){
	if(x+obj->c.x >= 0 && x+obj->c.x < room_width &&
	   y+obj->c.y >= 0 && y+obj->c.y < room_height){
	  field[(x+obj->c.x) + (y+obj->c.y)*room_width] = obj;
	  if(obj->out == OUT_HIDDEN) obj->out = OUT_NONE;
	  else if(obj->out == OUT_ENTIRE) obj->out = OUT_PART;
	}
	else{
	  if(obj->out == OUT_HIDDEN) obj->out = OUT_ENTIRE;
	  else if(obj->out == OUT_NONE) obj->out = OUT_PART;
	}
      }
  if(obj->out == OUT_PART) swapobject(obj, outnum++, OUTLIST);
  if(obj->out == OUT_ENTIRE){
    if(obj->live) availfish();
  }
  else if(obj->type == SMALL || obj->type == BIG) remainfish++;
}

void hideallobjects() // skryje vsechno - pronuluje field
{
  ff_object *obj;
  int i;

  for(obj = first_object; obj; obj = obj->next) obj->out = OUT_HIDDEN;
  for(i=0; i<room_width*room_height; i++) field[i] = NULL;
  outnum = 0;
  remainfish = 0;
}

static ff_object *coorobj(coor c) // vrati objekt na danem policku
{
  int i;
  ff_object *obj;

  if(c.x >= 0 && c.x < room_width && c.y >= 0 && c.y < room_height)
    return field[c.x + c.y*room_width];

  for(i=0; i<outnum; i++){
    obj = objlist[OUTLIST][i];
    if(c.x >= obj->c.x && c.x < obj->c.x+obj->width &&
       c.y >= obj->c.y && c.y < obj->c.y+obj->height &&
       obj->data[(c.x-obj->c.x) + (c.y-obj->c.y)*obj->width])
      return obj;
  }

  return NULL;
}

/****************************   hraci plan   *********************************/
/*****************************************************************************/
/*********************  formalni zabiti a oziveni  ***************************/

static int killedfish; // pocet zabitych ryb

void killfish(ff_object *fish)  // nastavi rybu jako mrtvou
{
  fish->gflip = fish->flip; // mrtva ryba se nebude plynule otacet
  fish->live = 0;
  addobjchange(fish, LIVECHANGE); // ulozeni do undo historie
  availfish();  // prepnuti na jinou rybu

  // skript dostane informaci, ze ryba zemrela
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_killfish");
  lua_pushlstring(luastat, fish->color, 4);
  lua_call(luastat, 1, 0);
  // cimz vyda pokyn, aby se zmenil jeji obrazek na kostricku

  killedfish++;
}

void vitalizefish(ff_object *fish) // oziveni ryby (napr. pri undo)
{
  fish->live = 1;

  // skript dostane informaci, ze ryba ozila
  lua_getfield(luastat, LUA_GLOBALSINDEX, "script_vitalizefish");
  lua_pushlstring(luastat, fish->color, 4);
  lua_call(luastat, 1, 0);
  // cimz vyda pokyn, aby se zmenil jeji obrazek zpatky na zivou rybu

  killedfish--;
}

/*********************  formalni zabiti a oziveni  ***************************/
/*****************************************************************************/
/*************************  stavy objektu  ***********************************/

/*
  Jedna se o nadstavbu k objlistum. Cilem je umet objektu priradit stav a soucasne
  sebrat jen objekty daneho stavu. Vezmu si tedy nejaky objlist a k nemu pridam cisla,
  ktera rikaji, kolik objektu je daneho stavu. V objlistu budou objekty setridene podle
  stavu, takze podle indexu objektu poznam i jeho stavu.

  Konkretne statenum[list][state] udava pocet objektu stavu mensiho nebo rovno state.

  Timto zpusobem rozsiruji dva objlisty - HEAPLIST a MOVELIST:
 */

#define HEAPLIST     0 // udava, na cem objekt lezi
// stavy jsou:
#define NOHEAP       0 // na nicem nelezi, bude padat
#define UNPROCESSED  1 // jeste to mam v planu prozkoumat
#define SMALLHEAP    2 // lezi pouze na malych rybach
#define BIGHEAP      3 // lezi i na velke rybe, ale uz na na zdi
#define SOLIDHEAP    4 // lezi na pevne podlozce

#define MOVELIST     1 // objekty, ktere jsou posouvany
#define MOVING       0 // objekt se hybe
// UNPROCESSED je spolecne
#define STAYING      2 // objekt se nehybe

#define MAXSTATE  4

static int statenum[2][MAXSTATE];

static void objstate(ff_object *obj, int state, int list)
                 // nastavi objektu 'obj' stav 'state' v objlistu 'list'
{
  int i;

  if(state > 0 && obj->index[list] < statenum[list][state-1]){
    for(i=state-2; i >= 0 && obj->index[list] < statenum[list][i]; i--);
    i++;
    for(; i < state; i++) swapobject(obj, --statenum[list][i], list);
  }
  else if(state < MAXSTATE && obj->index[list] >= statenum[list][state]){
    for(i=state+1; i < MAXSTATE && obj->index[list] >= statenum[list][i]; i++);
    i--;
    for(; i >= state; i--) swapobject(obj, statenum[list][i]++, list);
  }
}


/*************************  stavy objektu  ***********************************/
/*****************************************************************************/
/************************  hromadky na rybach  *******************************/

/*
  Hlavni funkce pro praci s objekty je singlepush(). Zakladni vlastnost teto
  funkce je, ze se podiva na vsechnyo objekty, ktere se daneho objektu (parametr obj)
  dotykaji v danem smeru (parametr dir), a nastavi jim dany stav (parametr state)
  v danem objlistu (parametr list).

  Jelikoz je to vsak mnohoucelova funkce, ma jeste pro jiste specialni ucely dalsi
  parametry, ktere rikaji, ktere objekty by mely byt pri tomto konani preskakovany.

  Singlepush() bere objekty jen z jedne strany (v objlistu) od daneho objektu.
  Bud jen ty, co maji vyssi stav (je-li front == true) nebo jen ty, co maji nizsi
  stav (je-li front == 0).

  Tato funkce nikdy "neustrci" zed (vzdycky ji preskoci). Ale co ocel a ryby?
  je-li ryba ziva, ustrci ji jen v pripade zapnuteho kill. Ocel neustrci v pripade,
  ze je na true nastaveny parametr weak.

  Dale muze preskakovat objekty, ktere nemaji nastaveny dany stav na HEAPLISTu
  (parametr onlystate), pripadne objekty ktere maji prilis nizky index
  (parametr keep).

  Navratova hodnota singlepush udava, zda funkce potkala predmet, ktery neustrcila
  (zed, ryba, ocel pokud weak)
 */

static char singlepush(ff_object *obj, // ktery objekt
		       int dir,        // kterym smerem
		       int list,       // MOVELIST nebo HEAPLIST
		       char front,     // brat objekty zepredu (s vetsimi indexy)?
		       int state,      // stav, na ktery se ma objekt nastavit
		       char weak,      // neustrci ocel
		       char kill,      // ustrci rybu
		       int onlystate,  // hromadka, na ktere musi byt, pouzito v expandheap
		       int keep)       // viz cropheap
{
  int i;
  ff_object *o;
  char result = 0;    // prekazi neco?

  for(i=0; i<obj->dirnum[dir]; i++){
    o = coorobj(coorsum(obj->c, obj->dirsquares[dir][i]));
    if(!o) continue;
    if(front){
      if(state >= MAXSTATE || o->index[list] < statenum[list][state]) continue;
    }
    else{
      if(state <= 0 || o->index[list] >= statenum[list][state-1]) continue;
      if(list == HEAPLIST && o->index[HEAPLIST] >= statenum[HEAPLIST][NOHEAP] &&
	 o->index[HEAPLIST] < statenum[HEAPLIST][UNPROCESSED]) continue; // viz cropheap
    }
    if(o->index[list] < keep) continue;
    if((!kill && o->live) || o->type == SOLID || (weak && o->type == STEEL)){
      result = 1;
      continue;
    }
    if(onlystate){
      if(o->index[HEAPLIST] < statenum[HEAPLIST][onlystate-1]) continue;
      if(o->index[HEAPLIST] >= statenum[HEAPLIST][onlystate]) continue;
    }
    objstate(o, state, list);
  }

  return result;
}

/*
  Funkce singleheap() se podiva pod objekt (vyjimecne do strany,
  parametr dir) a na zaklade toho zjisti, na jake hromadce objekt lezi
  (jaky stav by mel mit). Objekty dosud nezpracovane ignoruje, takze
  nemusi vratit zcela spravny vysledek, nicmene tento problem se
  spravi po prozkoumani onoho zatim nezpracovaneho objektu.
 */

static int singleheap(ff_object *obj, int list, int dir)
{
  int i;
  ff_object *o, *omax;

  if(obj->live){
    if(obj->type == SMALL) return SMALLHEAP;
    else if(obj->type == BIG) return BIGHEAP;
    else warning("singleheap: Something wrong ... unknown fish type %d", obj->type);
  }
  else if(obj->type == SOLID) return SOLIDHEAP;

  omax = NULL;
  for(i=0; i<obj->dirnum[dir]; i++){
    o = coorobj(coorsum(obj->c, obj->dirsquares[dir][i]));
    if(!o) continue;
    if(!omax || o->index[list] > omax->index[list]) omax = o;
  }

  if(!omax) return NOHEAP;

  for(i=0; i<MAXSTATE; i++)
    if(omax->index[list] < statenum[list][i]) break;
  return i;
}

/*
  Mame nejake nezpracovane objekty, o kterych chceme zjistit, na jake ze hromadce
  to vlastne jsou (jaky stav v HEAPLISTu maji mit). Krom tech tu jsou jeste
  zpracovane, ale spatne, protoze lezely na nezpracovanych. Proto je treba
  u kazdeho nezpracovaneho objektu se podivat nad nej a vsem nad nim nastavit
  spravny stav, cili MAX(stavajici stav, novy stav).

  Pri tomto nastavovani objektu nahore je vsak treba vynechat ty maji stav
  UNPROCESSED (to je ta divna podminka v singlepush()). Mohlo by se totiz stat,
  ze bych tak opomnel projit takove neprozkoumane objekty, kterym ve skutecnosti
  nalezi jeste vyssi stav.

  Dale tato funkce nikdy nedovoli oceli lezet na hromadce male ryby, misto toho
  oznaci ocel jako padajici (aby rybu zabila). Podobne, je-li parametr dangerous
  nastaveny na true, nedovoli zadnemu objektu byt na hromadce ryby. Tento parametr
  je pouzivan padaji-li objekty (a zabiji tak ryby).

  Priklad:

     11
    4433
    X  22
       ===

XX    - zed
===   - mala ryba
1 - 4 - UNPROCESSED objekty, ktere bude cropheap brat v tomto poradi

1) bod objektem 1 neni zadny objekt stavu alespon SMALLHEAP
   -> je mu nastaven stav NOHEAP
2) pod objektem 2 je mala ryba
   -> je mu nastaven stav SMALLHEAP
     ovsem tesne nad nim je jen objekt 3, ktery je unprocessed,
     tedy nic dalsiho se nedeje
3) pod objektem 3 je objekt stavu SMALLHEAP
   -> je mu nastaven stav SMALLHEAP
     tesne nad nim je objekt 1 typu NOHEAP (nizsi typ), je tedy i jemu nastaven
     SMALLHEAP. Toto nastavovani by pokracovalo, byli-by nad nim dalsi objekty
     typu NOHEAP
4) pod objektem 3 je zed
   -> je mu nastaven stav SOLIDHEAP
     tesne nad nim je objekt 1 typu SMALLHEAP (nizsi typ), je tedy i jemu nastaven
     SOLIDHEAP

Zaver: objekty 1, 4 lezi na zdi, objekty 2, 3 lezi na male rybe.

  Funkce cropheap se pouziva i v pripade, kdy se hleda seznam predmetu, ktery se
  posune do boku. Od toho jsou parametry 'dirmode' - osekavam objekty, co se opiraji
  bokem o zed, a 'keep' - ten rika, kolik objektu (na zacatku seznamu) je strkanych,
  tedy mam u nich ignorovat skutecnost, ze lezi na zemi.

 */

static void cropheap(int list, char dirmode, char dangerous, int keep)
{
  ff_object *obj;
  int state, i;
  char weak;
  int dir = dirmode ? room_movedir : DOWN;

  while(statenum[list][NOHEAP] < statenum[list][UNPROCESSED]){
    obj = objlist[list][statenum[list][NOHEAP]];
    state = singleheap(obj, list, dir);
    weak = (list == HEAPLIST && state == SMALLHEAP);
    if(state <= UNPROCESSED || (dangerous && state <= BIGHEAP) ||
       (weak && obj->type == STEEL)){
      statenum[list][NOHEAP]++; // objekt na nicem nelezi, zahrnu ho mezi NOHEAP
      continue;
    }

    objstate(obj, state, list); // uspesne zmenim objektu stav

    // a s nim i tem objektum nad nim
    for(i=statenum[list][state-1]; i >= statenum[list][state-1]; i--){
      obj = objlist[list][i];
      singlepush(obj, UP, list, 0, state, weak, 0, 0, keep);
      if(dirmode)
	singlepush(obj, backdir(room_movedir), list, 0, state, weak, 0, 0, keep);
    }
  }
}

/*
  Nasledujici funkce prida k neprozkoumanym objektum vsechny, ktere na nich lezi.
  Parametr onlystate muze pruchod omezit jen na danou hromadku v HEAPLISTu.
*/
static void expandheap(int list, int onlystate)
{
  int i;

  for(i=statenum[list][NOHEAP]; i<statenum[list][UNPROCESSED]; i++)
    singlepush(objlist[list][i], UP, list, 1, UNPROCESSED, 0, 0, onlystate, 0);
}

/*
  Funkce killpush nepredpoklada zadna UNPROCESSED objekty. Vsechny objekty, ktere
  maji nastaveny NOHEAP tlaci dolu a vznikaji tak UNPROCESSED objekty. To ovsem
  jen za predpokladu, ze zemre nejaka ryba, protoze jinak objekty NOHEAP pod sebou
  maji prazdno. Nasledne tlaci dolu i nove vznikle UNPROCESSED, az se protlaci k rybam,
  kterym je souzeno zemrit.

  U ryby musi zjistit, co je pod ni - zda hromadka ryby, kterou to opet zabije, nebo
  hromadka zdi. Podle toho skrz onu rybu bude dale strkat ci nikoli.

  Nakonec opet hromadky nastavi na spravne hodnoty.
 */
static void killpush(char dangerous)
{
  int i, state;
  ff_object *obj;

  for(i=0; i < statenum[HEAPLIST][UNPROCESSED]; i++){
    obj = objlist[HEAPLIST][i];
    if(obj->live){
      killfish(obj);
      state = singleheap(obj, HEAPLIST, DOWN);
      if(state >= SOLIDHEAP || (!dangerous && state >= BIGHEAP)) continue;
    }
    singlepush(obj, DOWN, HEAPLIST, 1, UNPROCESSED, 0, 1, 0, 0);
  }

  if(statenum[HEAPLIST][UNPROCESSED] == statenum[HEAPLIST][NOHEAP]) return;

  expandheap(HEAPLIST, 0);
  cropheap(HEAPLIST, 0, 0, 0);
  if(dangerous) killpush(0); /* situace: zemre velka ryba drzici
				stojici ocel spolu s malou rybou */
}

/************************  hromadky na rybach  *******************************/
/*****************************************************************************/
/****************************  bezne hrani  **********************************/

/*
  Pri zmacknuti sipky se zavola funkce movefish s parametrem, o kterou sipku se jedna.
  Ta spocita, ktere predmety se pohnou a nastavi v pripade uspechu room_state na
  ROOM_MOVE. V opacnem pripade (v pohybu treba prekazi zed) ponecha ROOM_IDLE.

  Tato funkce nepohne zatim s zadnymi predmety, jejim vysledekm je pouze uprava
  objlist MOVELIST. Prvnich statenum[MOVELIST][MOVING] objektu bude udavat
  objekty ktere se hybou smerem room_movedir (ktery funkce take nastavi).

  Dalsim jejim vystupem je promenna superheap >= statenum[MOVELIST][MOVING]. Ta udava
  s kolika prvnimi predmety v objlist[MOVELIST] se neco stalo tak, ze bude treba prepocitat
  jejich stav v HEAPLIST.
 */

static int superheap;

void movefish(int dir)
{
  int i;
  int keep;

  if(room_state != ROOM_IDLE) return;
  if(!active_fish) return;

  statenum[MOVELIST][MOVING] = statenum[MOVELIST][UNPROCESSED] = 0;

  objstate(active_fish, UNPROCESSED, MOVELIST);
  for(i=0; i<statenum[MOVELIST][UNPROCESSED]; i++)
    if(singlepush(objlist[MOVELIST][i], dir, MOVELIST, 1, UNPROCESSED,
		  active_fish->type == SMALL, 0, 0, 0)) break;
     // predmety, ktere jsou strkany -> stav UNPROCESSED v MOVELIST

  if(i < statenum[MOVELIST][UNPROCESSED]) return; // neco jsem neustrcil

  keep = statenum[MOVELIST][UNPROCESSED]; // spocitane predmety uz se urcite pohnou
  room_movedir = dir;

  if(dir == UP)
    superheap = statenum[MOVELIST][MOVING] = statenum[MOVELIST][UNPROCESSED];

  else if(dir == DOWN){          // najdu hromadku, ktera jde s rybou dolu
    expandheap(MOVELIST, active_fish->type == SMALL ? SMALLHEAP : BIGHEAP);
    statenum[MOVELIST][MOVING] = keep;

    if(active_fish->type == BIG) superheap = statenum[MOVELIST][UNPROCESSED];
      // bude treba prepocitat predmety, co lezely na obou rybach

    cropheap(MOVELIST, 0, 0, keep);
    if(active_fish->type == SMALL) superheap = statenum[MOVELIST][UNPROCESSED];
  }
  else{        // najdu predmety, ktere se posouvaji trenim do strany
    expandheap(MOVELIST, 0);
    statenum[MOVELIST][MOVING] = keep;
    superheap = statenum[MOVELIST][UNPROCESSED];
    cropheap(MOVELIST, 0, 0, keep); // hromadka, co lezi na strkanych predmetech
    if(active_fish->type == SMALL)  // nesmi obsahovat ocel v pripade, ze strka mala ryba
      for(i=0; i<statenum[MOVELIST][MOVING]; i++)
	if(objlist[MOVELIST][i]->type == STEEL) return;
    statenum[MOVELIST][MOVING] = keep;
    cropheap(MOVELIST, 1, 0, keep);  // zrusi se pohyb u predmetu, kterym neco prekazi ze strany
  }

  room_state = ROOM_MOVE;
  startmove(active_fish, dir);  // vzkaz undo historii

  if(dir == LEFT && active_fish->flip){  // otoceni ryby
    active_fish->flip = 0;
    addobjchange(active_fish, FLIPCHANGE);
  }
  else if(dir == RIGHT && !active_fish->flip){
    active_fish->flip = 1;
    addobjchange(active_fish, FLIPCHANGE);
  }
}

static void move(int list, int dir)  // Posune ve field predmety, ktere se hybou.
{
  int i;

  for(i=0; i<statenum[list][MOVING]; i++){
    addobjmove(objlist[list][i]); // vzkaz undo historii
    hideobject(objlist[list][i]);
    if(dir == RIGHT) objlist[list][i]->c.x++;
    else if(dir == LEFT) objlist[list][i]->c.x--;
    else if(dir == DOWN) objlist[list][i]->c.y++;
    else if(dir == UP) objlist[list][i]->c.y--;
    else warning("move: Something wrong unknown direction %d", room_movedir);
  }
  for(i=0; i<statenum[list][MOVING]; i++) showobject(objlist[list][i]);
}

/*
  Nasledujici funkce posune mistnost do dalsiho stavu, tedy pokud roomstate ==
  a) ROOM_IDLE, nedeje se nic
  b) ROOM_FALL, ROOM_MOVE - posune predmety co padaji / posouvaji se a spocita,
                            ktere premety budou dale padat.
  c) ROOM_BEGIN - predmety, ktere budou padat jsou jiz spocitane. Jen jeste nejsou
                  zabite male ryby nesouci ocel, tedy tato funkce je zabije a zmeni
		  room_state na ROOM_FALL.
*/
void room_step()
{
  char dangerous = 0;
  int i;
  ff_object *obj;

  if(room_state == ROOM_IDLE) return;
  else if(room_state == ROOM_MOVE){
    move(MOVELIST, room_movedir);
    for(i=0; i<superheap; i++){
      obj = objlist[MOVELIST][i];
      objstate(obj, obj->out == OUT_ENTIRE ? SOLID : UNPROCESSED, HEAPLIST);
    }
  }
  else if(room_state == ROOM_FALL){
    dangerous = 1;
    move(HEAPLIST, DOWN);
    statenum[HEAPLIST][MOVING] = 0;
    i = 0;
    while(i < statenum[HEAPLIST][UNPROCESSED]){
      obj = objlist[HEAPLIST][i];
      if(obj->out == OUT_ENTIRE) objstate(obj, SOLIDHEAP, HEAPLIST);
      else i++;
    }
  }

  if(room_state != ROOM_BEGIN) cropheap(HEAPLIST, 0, dangerous, 0);
  killpush(dangerous);

  if(statenum[HEAPLIST][MOVING]){
    room_state = ROOM_FALL;
    room_movedir = DOWN;
  }
  else{
    room_state = ROOM_IDLE;
    finishmove();  // vzkaz undo historii
  }
}

/****************************  bezne hrani  **********************************/
/*****************************************************************************/
/************************** inicializace atp.  *******************************/

struct ff_object *start_active_fish = NULL; // ryba aktivni pri startu

void init_rules() // funkce volana tesne po nacteni mistnosti
{
  int i, smallnum;
  ff_object *obj;

  smallnum = fishnum = 0;
  for(i=0; i<objnum; i++){ // vytvorim FISHLIST
    obj = objlist[FISHLIST][i];
    if(obj->type == SMALL || obj->type == BIG){
      obj->live = 1;
      swapobject(obj, fishnum++, FISHLIST);
      if(obj->type == SMALL) swapobject(obj, smallnum++, FISHLIST);
    }
    else obj->live = 0;
  }

  if(!fishnum) error("init_rules: no fish found");

  if(!start_active_fish) active_fish = objlist[FISHLIST][0];
  else active_fish = start_active_fish;

  killedfish = 0;
  remainfish = fishnum;

  field = (ff_object **)mymalloc(sizeof(ff_object *)*room_width*room_height);

  hideallobjects();
  for(obj = first_object; obj; obj = obj->next) showobject(obj);

  statenum[MOVELIST][STAYING] = objnum;
  fallall();
}

/*
  Nasledujici funkce je volana bud na zacatku nebo pri navratu do tahu -1. Funkce zjisti,
  ktere objekty lezi na kterych hromadkach (stav v HEAPLISTu). Ocel nemuze lezet na male
  rybe, tedy takova ocel dostane stav NOHEAP, ale zatim malou rybu nezabije. To se stane
  az pri volani room_step().
 */
void fallall()
{
  int i;
  ff_object *obj;

  statenum[HEAPLIST][MOVING] = 0;
  statenum[HEAPLIST][UNPROCESSED] = statenum[HEAPLIST][SMALLHEAP] =
    statenum[HEAPLIST][BIGHEAP] = objnum; // na zacatku je vsechno unprocessed

  for(i=objnum-1; i >= 0; i--){ // nastavim hromadky objektum, u kterych o tom neni pochyb
    obj = objlist[HEAPLIST][i];
    if(obj->type == SMALL) objstate(obj, SMALLHEAP, HEAPLIST);
    else if(obj->type == BIG) objstate(obj, BIGHEAP, HEAPLIST);
    else if(obj->type == SOLID) objstate(obj, SOLIDHEAP, HEAPLIST);
  }
  cropheap(HEAPLIST, 0, 0, 0); // ten podstatny krok

  if(statenum[HEAPLIST][MOVING]) room_state = ROOM_BEGIN; // existuje tah -1
  else room_state = ROOM_IDLE; // neexistuje tah -1
}

/*
  Nasledujici funkce je volana po provedeni undo, kdy je treba prepocitat hromadky
  na rybach. Narozdil od funkce fallall ale zadny predmet nebude padat, tedy staci se jen
  podivat nad ryby.
 */
void findfishheap()
{
  int i;

  statenum[HEAPLIST][SMALLHEAP] = statenum[HEAPLIST][BIGHEAP] = 0;
  for(i=0; i<fishnum; i++)
    if(objlist[FISHLIST][i]->live)
      objstate(objlist[FISHLIST][i], UNPROCESSED, HEAPLIST); // rybam nastavim UNPROCESSED
  expandheap(HEAPLIST, 0);  // stejne tak predmetum nad nimi
  cropheap(HEAPLIST, 0, 0, 0);  // a najdu prislusne hromadky
}

void free_rules()   //  funkce volana pri ukonceni mistnosti
{
  start_active_fish = NULL;
  free(field);
}

/************************** inicializace atp.  *******************************/
/*****************************************************************************/
/*******************  jednoduche verejne testovaci fce  **********************/

char ismoving(ff_object *obj)  // vrati, zda se objekt prave hybe
{
  int list;

  if(room_state == ROOM_IDLE || room_state == ROOM_BEGIN) return 0;
  if(room_state == ROOM_MOVE) list = MOVELIST;
  else if(room_state == ROOM_FALL) list = HEAPLIST;

  return obj->index[list] < statenum[list][MOVING];
}

char isonfish(ff_object *obj)  // vrati, zda objekt lezi na hromadce ryb
{
  return obj->index[HEAPLIST] >= statenum[HEAPLIST][UNPROCESSED] &&
    obj->index[HEAPLIST] < statenum[HEAPLIST][BIGHEAP];
}

char issolved()  // vrati, zda je mistnost prave vyresena
{
  return !killedfish && !remainfish;
}

/*******************  jednoduche verejne testovaci fce  **********************/
/*****************************************************************************/
/*************************  pozvolne otaceni ryb  ****************************/

#define FLIPMOVE 0.1  // posun gflip v jednom frame

void animflip() // posune rybam gflip blize k skutecnemu flip (tedy pootoci je)
{
  int i;

  for(i=0; i<fishnum; i++)
    if(objlist[FISHLIST][i] && objlist[FISHLIST][i]->gflip != objlist[FISHLIST][i]->flip){
      if(objlist[FISHLIST][i]->flip){
	objlist[FISHLIST][i]->gflip += FLIPMOVE;
	if(objlist[FISHLIST][i]->gflip > 1)
	  objlist[FISHLIST][i]->gflip = objlist[FISHLIST][i]->flip;
      }
      else{
	objlist[FISHLIST][i]->gflip -= FLIPMOVE;
	if(objlist[FISHLIST][i]->gflip < 0)
	  objlist[FISHLIST][i]->gflip = objlist[FISHLIST][i]->flip;
      }
    }
}

void unanimflip() // nastavi rybam gflip na skutecny flip (napr. pri undo)
{
  int i;
  for(i=0; i<fishnum; i++)
    if(objlist[FISHLIST][i])
      objlist[FISHLIST][i]->gflip = objlist[FISHLIST][i]->flip;
}

/*************************  pozvolne otaceni ryb  ****************************/
/*****************************************************************************/
/****************************  ladici tisky  *********************************/

static void printfield()  //  Vypise pole field podle stavu HEAPLIST
{
  int i;

  printf("\n\n");
  for(i=0; i < room_width*room_height; i++){
    if(!field[i]) printf(" ");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][NOHEAP]) printf("V");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][UNPROCESSED]) printf("?");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][SMALLHEAP]) printf("x");
    else if(field[i]->index[HEAPLIST] < statenum[HEAPLIST][BIGHEAP]) printf("X");
    else printf("#");

    if((i+1)%room_width == 0) printf("\n");
  }
  printf("\n\n");
}

static void printmfield()  //  Vypise pole field podle stavu MOVELIST
{
  int i;

  printf("\n\n");
  for(i=0; i < room_width*room_height; i++){
    if(!field[i]) printf(" ");
    else if(field[i]->index[MOVELIST] < statenum[MOVELIST][NOHEAP]) printf("M");
    else if(field[i]->index[MOVELIST] < statenum[MOVELIST][UNPROCESSED]) printf("?");
    else if(field[i]->index[MOVELIST] < statenum[MOVELIST][STAYING]) printf("#");
    else printf("!");

    if((i+1)%room_width == 0) printf("\n");
  }
  printf("\n\n");
}
