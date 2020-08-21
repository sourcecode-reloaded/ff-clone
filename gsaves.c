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
#include "keyboard.h"

#define ICON_SIZE         200 // polovina obvodu zvetseneho tlacitka
#define MINI_SCALE        0.5 // zmenseni neaktivniho tlacitka (nenarazime-li na okraje okna)
#define BORDER              5 // mezery mezi tlacitky (konstantni pro zvetsena i nezvetsena)
#define RECT_BORDER    BORDER // tloustka bileho okraje o zvetseneho tlacitka
#define MAGNET            0.5 /* aktivni tlacitko ma zvetseni 1, u sousednich se postupne
				 odecita tato konstanta */
#define DARKEN            0.5 // mira ztmaveni zvetseneho tlacitka, ktere neni focused
#define SPACE              40 // mezera mezi tlacitky, kam bude presunuta presouvane tlacitko
#define TRASHDIST         100 // jak daleko je treba jit s presouvanym tlacitkem, aby se vyhodila
#define DRAGDIST          100 /* (jak daleko je treba s mysi popojet od mista kliku, aby se
				 tlacitko uchopilo)^2 do te doby je to povazovano za pouhe drzeni
				 a pri pusteni se prislusna pozice nacte. */
#define DRAGPOS             5 /* jak moc (gsaves_vertical ? vlevo od mysi : pod mysi) bude
				 prenasene tlacitko */
#define DODGE              20 // pri nacteni pozice se mys oddali od gsaves na tuto vzdalenost
#define MAX_LINESIZE      100 // nejvyssi mozna delka nazvu ulozene pozice

#define MAXSAVES  100 /* nejvyssi mozny pocet ulozenych pozic / tlacitek (mam je v poli),
			 pri zvysovani teto konstanty hrozi kolize poradovych cisel,
			 viz gsaves_save() */

struct gsave{ // struktura jednoho tlacitka ulozene pozice
  cairo_surface_t *surf; // zvetseny obrazek na tlacitku
  cairo_surface_t *scaled; // zmenseny obrazek
  double enlarged; /* jak moc je zrovna toto tlacitko zvetsene kvuli efektu lupy, nezavisi
		      na tom, jak moc aktivni jsou prave gsaves (zvetsene) ani na zvetsene
		      ukladane / nacitane pocici (od toho je promenna enl_marked)
		      0 = zmensene, 1 = zvetsene
		   */
  double shift; /* jak je treba posunout nasledujici tlacitko kvuli efektu lupy,
		   hodnota primo zavisla na skutecnem zvetseni tlacitka (enlarged a enlmarked)
		*/
  double spaceshift; /* posunuti dalsiho tlacitka kvuli tomu, ze se tam zapadne
			presouvane tlacitko */
  char *name; // nazev ulozene pozice urcujici, z jakeho souboru se to bude nahravat
};

/* Abych mohl polozky pole jednoduse premistovat, pracuji s polem ukazatelu save[]. Nicmene
   toto pole musi nekam ukazovat, tedy proto mam pole data[], pri inicializaci presmeruji save[i]
   na data[i] a dale se o pole data nezajimam. */
static struct gsave data[MAXSAVES];
static struct gsave *save[MAXSAVES];
static int savesnum; // aktualni pocet ulozenych pozic

/* Gsaves mohou mit dva zvlastni stavy:
   1) mode HOLD: je drzena mys nad nejakym tlacitkem a jeste se nevi, zda toto
      tlacitko budeme prenaset nebo nacitat. V tomto stavu se nehybe s tlacitky, dokud
      se otazka co dal nevyjasni.
   2) mode DRAG: prenasime nejake tlacitko

   jinak je mode NORMAL
 */
static enum {NORMAL, HOLD, DRAG} mode;

static int icon_width, icon_height; // velikost zvetseneho tlacitka
static int icon_len; // = gsaves_vertical ? icon_height : icon_width
static double mini_width, mini_height, mini_len; // to same u zmenseneho tlacitka

static char active; // mys je nad gsaves
static double act;           // animovana promenna active (pozvolne zvetsovani a zmensovani)
static int marked;           // save s rameckem
static int focused;          // save, nad kterym je mys
static int mouse_pos;     // = gsaves_vertical ? mouse_y : mouse_x;
static double focused_pos;   // 0 az 1 souradnice mysi v ramci oznaceneho save
static double start;         // souradnice prvniho tlacitka
static double shift;         // posunuti prvniho tlacitka dozadu kvuli efektu lupy
static int spaced;           // save, za kterym je mezera
static double enl_marked;    // zvetsenost oznaceneho save, nezavisle na act a enlarged

static struct gsave *dragged;         // save odebrany ze seznamu
static int drag_src_x, drag_src_y;    // misto kliknuti, od ktereho je treba ujet sqrt(DRAGDIST)

/*
  Prenasene tlacitko neresim pomoci caira, je to jen jednoduche okno v X (odpadaji tim
  problemy, co vykreslovat pod prenasenym tlacitkem)
 */
static Window drag_win; // okno prenaseneho tlacitka
static Pixmap drag_pxm; // obrazek prenaseneho tlacitka
static int drag_x, drag_y; // posunuti prenaseneho tlacitka oproti souradnicim mysi

static void drag(); // vytvori z focused tlacitka prenasene
static void drop(); // pusteni prenaseneho tlacitka -- bud ho znici nebo zaradi na spravne misto

/* Udaje o ramecku kolem naposledy pouziteho tlacitka.
   Ve skutecnosti je to obdelnicek vykreslovany pod tlacitkem.
*/
static double rect_x, rect_y; // souradnice
static double rect_width, rect_height; // rozmery
static double rect_col; // barva: 0 = cerna, 1 = bila, kvuli ztmavovani zvetsenych nefocused pozic

static void count_rect();
static void count_shift();
static void get_bg();
static void dodge_gsaves();
static void savelist();
static void loadmarked();

/*
  V surface bg_surf mam ulozene pozadi pod aktivnimi gsaves. Spoleham na to, ze se pri aktivnich
  gsaves ostatni prvky nebudou menit, takze tuto promennou nebudu muset moc casto prepocitavat.
  Pokud gsaves aktivni nejsou, je tato promenna NULL.
 */
static cairo_surface_t *bg_surf = NULL;

/*
  Pokud nejsou gsaves aktivni, tak pres nic neprecuhuji a gsaves_change_mask je tak nastavena na
  nulu. V okamziku, kdy aktivni byt zacnou, rovnou predpokladam plnou velikost -- ze precuhuji pres
  vsechno, pres co muzou. V tom okamziku tedy nastavim gsaves_change_mask na gsaves_ext_change_mask,
  kde mam spocitane, pres co precuhuji aktivni gsaves.
 */
static int gsaves_ext_change_mask;

/* Nasledujici funkce zapise seznam ulozinych pozic do $savedir/list.txt. V tomto souboru
   jsou nazvy pozic zapsana po radcich. Pred nazvem pozice s rameckem je napsan zavinac.
 */
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

/* Pri zmene velikosti okna nebo poctu ulozenych pozic je volana nasledujici funkce, ktera
   spravne nastavi prislusne promene. Predevsim jde o to, ze pokud by se gsaves do okna nevesly,
   budou jejich zmenseniny zplacle v dalen smeru.
 */
void calculate_gsaves()
{
  int i;
  cairo_t *cr;
  double m_width, m_height; // nove mini_width, mini_height
  char rescale; // zmensena tlacitka maji jinou velikost

  if(!savesnum) return;

  if(gsaves_vertical){
    gsaves_x_size = icon_width + 2*BORDER;
    gsaves_y_size = gsaves_length;
  }
  else{
    gsaves_x_size = gsaves_length;
    gsaves_y_size = icon_height + 2*BORDER;
  }

  gsaves_ext_change_mask = 0;  // spocitam, pres co precuhuji
  if(gsaves_vertical){
    if(gsaves_x_size > room_x_translate) gsaves_ext_change_mask |= CHANGE_ROOM;
    if(gmoves_vertical && gsaves_x_size > win_width-gmoves_width)
      gsaves_ext_change_mask |= CHANGE_GMOVES;
  }
  else{
    if(gsaves_y_size > room_y_translate) gsaves_ext_change_mask |= CHANGE_ROOM;
    if(!gmoves_vertical && gsaves_y_size > win_height-gmoves_height)
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
    if(save[i]->scaled){ /* pokud nejake save jeste nema vytvorene scaled (je to nove),
			    tak scale vytvorim i kdyz se velikost nezmenila */
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

static cairo_surface_t *unknown_icon; /* obrazek s otaznickem -- nahrada za nahled mistnosti,
					 pokud se ho nepovedlo nacist */

static cairo_surface_t *draw_unknown_icon() // vytvori unknown_icon a vrati ji
{
  cairo_t *cr;
  cairo_text_extents_t text_ext;
  float border = icon_height*0.2;
  float scale;

  if(unknown_icon) return unknown_icon;

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

  return unknown_icon;
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
  level_savedir_init(); // zjistim spravny adresar
  if(!savedir) return;
  f = fopen(savefile("list", "txt"), "r"); // nactu seznam ulozenych pozic
  if(!f) return;

  ch = fgetc(f);
  i = 0;
  for(;;){ // prochazim znaky, 'i' udava pozici na radku, v 'line' mam nacteny aktualni radek
    if(i == 0 && ch == '@') marked = savesnum; // radek zacina zavinacem -- pozice s rameckem
    else if(ch == '\n' || ch == EOF){ // radek konci -- nactu vyrobim prislusnou pozici
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
	  cairo_surface_destroy(save[savesnum]->surf);
	  save[savesnum]->surf = draw_unknown_icon();
	}
	save[savesnum]->scaled = NULL; // scaled se spocita az v calculate_gsaves
	savesnum++;
      }
    }
    else{ // pridam znak do radku
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

static void dodge_gsaves() // deaktivuje gsaves, tedy i uhne od nich s mysi
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

void drawgsaves(cairo_t *cr, char newbg)/* Vykresli gsaves. Parametr bg udava, jestli se zmenil
					   nejaky prvek, pres ktery gsaves precuhuje, a je tedy
					   treba znovu ulozit bg_surf. */
{
  int i;
  double enl;

  if(img_change & CHANGE_GSAVES){ // jestli se neco zmenilo, tak prepocitam promenne s tim spojene
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

  // oriznuti kreslici plochy
  if(act || enl_marked) cairo_rectangle(cr, 0, 0, gsaves_x_size, gsaves_y_size);
  else{
    if(gsaves_vertical) cairo_rectangle(cr, 0, 0, gsaves_width, gsaves_y_size);
    else cairo_rectangle(cr, 0, 0, gsaves_x_size, gsaves_height);
    gsaves_change_mask = 0;
  }
  cairo_clip(cr);

  // ramecek kolem aktivniho tlacitka
  if(marked >= 0 && savesnum){
    cairo_rectangle(cr, rect_x, rect_y, rect_width, rect_height);
    cairo_set_source_rgb(cr, rect_col, rect_col, rect_col);
    cairo_fill(cr);
  }

  // posunu se na prvni tlacitko
  if(gsaves_vertical) cairo_translate(cr, BORDER, start-shift);
  else cairo_translate(cr, start-shift, BORDER);

  // a postupne vykresluji tlacitka a presouvam se na dalsi
  for(i=0; i<savesnum; i++){
    /* promenna enl udava zvetseni aktualniho (tj. i-teho) tlacitka
       jedna se o kombinaci polozky enlarged s pripadnym enl_marked */
    enl = act*save[i]->enlarged;
    if(enl_marked && i == marked) enl = 1-(1-enl_marked)*(1-enl);

    if(!enl){ // nezvetsena tlacitka kreslim z polozky scaled
      cairo_set_source_surface(cr, save[i]->scaled, 0, 0);
      cairo_paint(cr);
    }
    else{ // ostatni jako zmensena surf
      cairo_save(cr);
      cairo_scale(cr, mini_width/icon_width + (1-mini_width/icon_width)*enl,
		  mini_height/icon_height + (1-mini_height/icon_height)*enl);
      cairo_set_source_surface(cr, save[i]->surf, 0, 0);
      cairo_paint(cr);
      if(i != focused && mode != DRAG){ // ztmaveni zvetsenych (ale ne toho pod mysi)
	if(enl_marked && i == marked) // enl_marked se taky neztmavuje
	  cairo_set_source_rgba(cr, 0, 0, 0, (enl-enl_marked) * DARKEN);
	else cairo_set_source_rgba(cr, 0, 0, 0, enl * DARKEN);
	cairo_rectangle(cr, 0, 0, icon_width, icon_height);
	cairo_fill(cr);
      }
      cairo_restore(cr);
    }
    // posunu se na dalsi tlacitko
    if(gsaves_vertical) cairo_translate(cr, 0, mini_len+BORDER+save[i]->shift);
    else cairo_translate(cr, mini_len+BORDER+save[i]->shift, 0);
  }

  cairo_restore(cr);
}

// jsou-li nakreslene zvetsene (aktivni) gsaves, prekresli jejich plochu, aby tam nebyli
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

// funkce volana pri kliknuti levym tlacitkem mysi
void gsaves_click()
{
  if(!savesnum) return;

  if(active){ // jiz vime,  jake tlacitko je oznacene: focused
    drag_src_x = mouse_x;
    drag_src_y = mouse_y;
    mode = HOLD;
  }
}

// funkce volana pri pusteni leveho tlacitka mysi
void gsaves_unclick()
{
  if(mode == HOLD){ // nacteni pozice
    if(marked != focused){
      marked = focused;
      savelist();
    }
    count_rect();
    loadmarked();
    img_change |= CHANGE_GSAVES;
    mode = NORMAL;
  }
  else if(mode == DRAG){ // presunuti tlacitka
    drop();
    gsaves_pointer(1, 0);
  }
}

/*
  Nasledujici funkce spocita posunuti tlacitek v zavislosti na
  jejich velikostech a rovnez posunuti celeho pasku v zavislosti na poloze
  mysi. Polozky shift jednotlivych tlacitek se jednoduse spocitaji podle
  zvetseni v prvnim for cyklu.

  Zasadni je spocitat posunuti celeho pasecku, ktere je urceno globalni
  promennou shift. Ta udava, o kolik je posunuto dozadu prvni tlacitko.
  Ovsem tlacitko, ktere je zrovna aktivni (promenna focused) je nezavisle
  na soucasnem zvetseni tlacitek. Je pocitano, jako by byla tlacitka zmensena.
  Ve smyslu teto promenne na sebe zmensena tlacitka navazuji a zabiraji tak po
  stranach jeste o BORDER/2 vetsi prostor. Podle promenne focused_pos se pak
  pozna, kde uvnitr focused tlacitka je mys. 0 znamena vlevo, 1 znamena vpravo.

  Promenna focused_pos je ovsem volena tak, aby nikdy nebylo focused_pos < 0.5
  u prvniho tlacitka ani > 0.5 u posledniho tlacitka.

  Na zaklade focused_pos bude tlacitko focused licovano se svym zmensenym vzorem:

  V NORMAL modu se shift nastavi tak, aby pro focused_pos = 0 se leva hrana
  zvetseneho tlacitko (opet vcetne BORDER/2 po stranach) kryla s levou hranou
  zmenseneho tlacitka. Analogicky pro focused_pos = 1 se bude prava strana
  zvetseneho tlacitka kryt s pravou stranou zmenseneho tlacitka. Pro
  focused_pos mezi temito hodnotami bude posunuti na focused_pos zaviset podle
  linearni funkce.

  V DRAG modu (ve kterem se objevuji mezi tlacitky vetsi mezery) je situace
  o neco slozitejsi. Zde nejsou jen dva body, kde jsou dane pozice, ale rovnou
  tri: pro focused_pos = 0 je leva strana zmenseneho tlacitka uprostred leve mezery,
  pro focused_pos = 0.5 se prostredek zmenseneho tlacitka kryje s proctredkem
  zvetseneho tlacitka a opet u focused_pos = 1 je prava strana zmenseneho tlacitka
  uprostred prave mezery.

  A jeste, pokud nejsou gsaves aktivni (zvetsene), ale je enl_marked, je toto
  zvetsene orameckovane tlacitko vycentrovane.
*/
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

// spocita barvu, polohu a velikost ramecku
static void count_rect()
{
  int i;
  double pos;
  double enl; // zvetsenost orameckovaneho tlacitka
  double border; // tloustka ramecku v soucasnem zvetseni

  if(marked < 0) return;

  pos = start-shift-BORDER;
  for(i=0; i<marked; i++) pos += mini_len+BORDER+save[i]->shift;

  enl = 1-(1-act*save[marked]->enlarged)*(1-enl_marked);
  border = (MINI_SCALE + (1-MINI_SCALE)*enl)*RECT_BORDER;
  rect_width = mini_width + enl*(icon_width-mini_width) + 2*border;
  rect_height = mini_height + enl*(icon_height-mini_height) + 2*border;

  rect_x = rect_y = BORDER - border;

  if(gsaves_vertical) rect_y += pos;
  else rect_x += pos;

  if(marked == focused || mode == DRAG) rect_col = 1;
  else rect_col = 1 - (enl-enl_marked) * DARKEN;
}

// prepnuti do modu DRAG a vsechno s tim spojene
static void drag()
{
  int i;
  int m_pos, m_move;
  cairo_surface_t *surf;
  cairo_t *cr;
  int width, height;

  mode = DRAG;
  img_change |= CHANGE_GSAVES;

  // vytazeni presouvaneho tlacitka z pole
  if(marked > focused) marked--;
  else if(marked == focused) marked = -1;
  dragged = save[focused];
  savesnum--;
  for(i=focused; i<savesnum; i++) save[i] = save[i+1];

  // nastaveni mezer mezi tlacitky
  for(i=0; i<savesnum; i++) save[i]->spaceshift = 0;
  if(focused > 0) save[focused-1]->spaceshift = SPACE;

  calculate_gsaves();  // zmenil se pocet tlacitek

  // posunuti mysi doprostred mezery

  mouse_pos = gsaves_vertical ? mouse_y : mouse_x;

  m_move = 1;   // budeme hybat s mysi?  Zatim ano...
  if(!savesnum) m_move = 0;
  else if(focused == 0){ // sebral prvni tlacitko
    m_pos = start - (icon_len-mini_len)/2.0;
    if(m_pos >= mouse_pos) m_move = 0;
  }
  else if(focused == savesnum){ // sebral posledni tlacitko
    m_pos = start + mini_len*savesnum+BORDER*(savesnum-1) + (icon_len-mini_len)/2.0;
    if(m_pos <= mouse_pos) m_move = 0;
  }
  else{ // sebral tlacitko z prostredka
    m_pos = start + (mini_len+BORDER)*focused - BORDER/2.0;
    if(m_pos == mouse_pos) m_move = 0;
  }
  if(m_move){
    if(gsaves_vertical) mouse_y = m_pos;
    else mouse_x = m_pos;
    mouse_pos = m_pos;
    setmouse();
  }
  // gsaves_pointer volat nemusim, nebot je to funkce, z ktere je to spoustene

  // vyroba okna s presouvanou pozici

  width = icon_width; // velikost a umisteni
  height = icon_height;
  if(marked == -1){ // presouvam pozici s rameckem, bude o ramecek vetsi
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

  if(marked == -1){ // pripadne vykresleni ramecku
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_set_source_surface(cr, dragged->surf, BORDER, BORDER);
    cairo_paint(cr);
  }
  else{ // vykresleni nahledu
    cairo_set_source_surface(cr, dragged->surf, 0, 0);
    cairo_paint(cr);
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surf);
  XMapWindow(display, drag_win);
}

// vypnuti DRAG modu a vsechno s tim spojene
static void drop()
{
  int i;
  int m_pos;

  mode = NORMAL;
  img_change |= CHANGE_GSAVES;

  // zniceni presouvaneho tlacitka
  XFreePixmap(display, drag_pxm);
  XDestroyWindow(display, drag_win);

  if(active){ // zarazeni tlacitka do seznamu
    // zarazeni do pole
    if(!savesnum) spaced = -1;
    savesnum++;
    for(i=savesnum-1; i>spaced+1; i--){
      save[i] = save[i-1];
    }
    save[i] = dragged;
    if(marked > spaced){
      marked++;
    }
    else if(marked == -1) marked = i;

    calculate_gsaves(); // zmenil se pocet tlacitek

    // posunuti mysi doprostred presunuteho tlacitka
    mouse_pos = gsaves_vertical ? mouse_y : mouse_x;
    m_pos = floor(start + (spaced+1)*(mini_len+BORDER) + mini_len/2.0);

    if(m_pos != mouse_pos && (spaced > -1 || m_pos < mouse_pos) &&
       (spaced < savesnum-2 || m_pos > mouse_pos)){
      if(gsaves_vertical) mouse_y = m_pos;
      else mouse_x = m_pos;
      mouse_pos = m_pos;
      setmouse();
    }
  }
  else{ // vyhozeni tlacitka
    save[savesnum] = dragged;
    if(dragged->surf != unknown_icon) cairo_surface_destroy(dragged->surf);
    if(dragged->scaled) cairo_surface_destroy(dragged->scaled);
    if(marked == -1){
      marked = savesnum-1;
    }
  }

  savelist(); // zmenil se seznam pozic, je treba ho znovu ulozit
}

/*
  Nasledujici funkce reaguje na pohyb mysi. At uz pri drzeni presouvane pozice,
  zmene tlacitka pod mysi nebo najeti nad samotne gsaves.

  Parametr force rika, ze je treba prepocitat focused a focused_pos i v pripade,
  ze se mouse_pos nezmenilo.

  Parametr leave_ev rika, ze volani pochazi od udalosti LeaveNotify. V takovem
  pripade ignoruji nektere cinnosti, nebot mohou zpusobovat nepekna cukani.
*/
void gsaves_pointer(char force, char leave_ev)
{
  double d;
  int i;
  int last_mouse_pos;
  char oac;

  if(mode == HOLD){ 
    if((mouse_x-drag_src_x)*(mouse_x-drag_src_x) + 
       (mouse_y-drag_src_y)*(mouse_y-drag_src_y) > DRAGDIST){
      drag(); // byla uz dostatecne posunuta drzena pozice, ja na case ji vyradit ze seznamu
      force = 1;
    }
    else return;
  }

  if(mode == DRAG && !leave_ev){ // posouvame drzenou pozici
    XMoveWindow(display, drag_win, mouse_x + drag_x, mouse_y + drag_y);
    XFlush(display);

    oac = active;
    // je mys moc daleko? -> zahazuje se drzena pozice
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
    if(oac != active){ // zmena obrazku na posouvanem okne (nahled <-> cerna)
      if(active) XSetWindowBackgroundPixmap(display, drag_win, drag_pxm);
      else XSetWindowBackground(display, drag_win, BlackPixel(display,screen));
      XClearWindow(display, drag_win);
    }
  }

  if(!savesnum) return;

  last_mouse_pos = mouse_pos;
  mouse_pos = gsaves_vertical ? mouse_y : mouse_x;

  if(mode == NORMAL && !mouse_pressed){ // je mys dost blizko na to, aby gsaves bylo aktivni?
    if(mouse_x < 0 || mouse_x > gsaves_x_size || mouse_y < 0 ||
       mouse_y > gsaves_y_size) active = 0; // mys je uplne mimo
    else{
      if(active){ // mys byla uvnitr -> musi odjet dal, aby se gsaves opet deaktivovaly
	if(mouse_pos >= start-(icon_len-mini_len)/2-BORDER &&
	   gsaves_length-mouse_pos >= start-(icon_len-mini_len)/2-BORDER)
	  active = 1;
	else active = 0;
      }
      else if(!leave_ev){ // gsaves nebyly aktivni
	if(mouse_pos < start || gsaves_length-mouse_pos < start) active = 0;
	else if(gsaves_vertical){
	  if(mouse_x < gsaves_width) active = 1;
	  else active = 0;
	}
	else{
	  if(mouse_y < gsaves_height) active = 1;
	  else active = 0;
	}
	if(active) force = 1;
      }
    }
  }

  if(!leave_ev && (act || active)){
    if(force || mouse_pos != last_mouse_pos){ /* spocitam zvetseni jednotlivych tlacitek
						 blizsi informace k promennym viz count_shift() */
      // spocitam focused a focused_pos
      focused_pos = mouse_pos;
      focused_pos -= start - BORDER / 2.0;
      focused_pos /= mini_len + BORDER;
      focused = floor(focused_pos);
      if(focused < 0) focused = 0;
      else if(focused >= savesnum) focused = savesnum-1;
      focused_pos -= focused;

      if(focused == 0 && focused_pos < 0.5) focused_pos = 0.5;
      if(focused == savesnum-1 && focused_pos > 0.5) focused_pos = 0.5;

      /*
	Zvetseni tlacitka se pocita jako max(0, 1 - MAGNET*d), kde d udava vzdalenost
	mysi od daneho tlacitka v jednotkach tlacitek.
       */
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

  if(mode == DRAG){ // spocita space, tedy misto aktivni mezery mezi tlacitky (kam hodim dragged)
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
}

// jeden krok animace gsaves
void gsaves_anim()
{
  int i;

  // deje se neco? -> budem blokovat ostatni animaci?
  if(enl_marked || active || act || mode != NORMAL){
    if(!gsaves_blockanim){
      unanim_fish_rectangle();
      gsaves_blockanim = 1;
    }
  }
  else gsaves_blockanim = 0;

  // zmensovani enl_marked -- prave nactene/ulozene pozice
  if(enl_marked){
    enl_marked -= GSAVES_ACT_SPEED;
    if(enl_marked < 0) enl_marked = 0;
    img_change |= CHANGE_GSAVES;
  }

  // pozvolne aktivovani / deaktivovani
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

  // pozvolne presouvani mezery (zmensovani na nespravnych mistech a zvetsovani na spravnem)
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

// ulozi pozadi gsaves (vezme to, co prave je v okne)
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

// znici gsaves (pri ukonceni levelu)
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

// okamzite deaktivuje gsaves
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

/*
  Nasledujici funkce ulozi soucasnou pozici (pri stisku klavesy F2)
  a zalozi pro ni tlacitko.

  Nazev je odvozen podle typu pozice a poctu tahu. Typ je bud "sol" (je-li
  to vyresena pozice) nebo sav (neni-li to vyresena pozice). Pro odliseni vice pozic,
  ktere maji stejny typ i pocet tahu je za temito dvema udaji jeste treti -- poradove
  cislo od 00 do 99. (tedy prvni save sveho druhu ma poradove cislo 00)

  Ulozena pozice pak bude ulozena do souboru s priponou fsv, jeji nahled do souboru
  s priponou png.
 */
void gsaves_save()
{
  int i, j;
  char name[20]; // nazev bez poradoveho cisla
  char used[MAXSAVES]; // ktera poradova cisla jsou jiz pouzita
  char fullname[20]; // plny nazev
  char *type; // typ pozice
  cairo_surface_t *surf; // nahled pozice
  struct gsave *tmp; // pomocny ukazatel pro preusporadani pole save

  // dostanu se do normalniho stavu
  if(mode != NORMAL) gsaves_unclick();
  while(room_state != ROOM_IDLE) room_step();
  keyboard_erase_queue();

  // urcim typ
  if(issolved()) type = "sol";
  else type = "sav";

  // vytvorim nazev bez poradoveho cisla
  for(i=0; i<MAXSAVES; i++) used[i] = 0;
  sprintf(name, "%s_%05d_", type, moves);

  // zjistim pouzita poradova cisla
  for(i=0; i<savesnum; i++){
    for(j=0; name[j] && name[j] == save[i]->name[j]; j++);
    if(!name[j] &&
       save[i]->name[j] >= '0' && save[i]->name[j] <= '9' &&
       save[i]->name[j+1] >= '0' && save[i]->name[j+1] <= '9' &&
       !save[i]->name[j+2])
      used[10*(save[i]->name[j]-'0') + (save[i]->name[j+1]-'0')] = 1;
  }
  for(i=0; i < MAXSAVES && used[i]; i++);

  sprintf(fullname, "%s%02d", name, i);
  surf = imgsave(savefile(fullname, "png"), // ulozim nahled
		 icon_width, icon_height, issolved(), 1);
  savemoves(savefile(fullname, "fsv")); // ulozim pozici

  if(savesnum >= MAXSAVES){ // jejda, dosel mi seznam
    warning("Save list if full (%d).", savesnum);
    cairo_surface_destroy(surf);
    return;
  }

  // vlozim prvek do seznamu
  savesnum++;
  if(savesnum == 1) j = 0;
  else{
    tmp = save[savesnum-1];
    for(j=savesnum-1; j>marked+1; j--) save[j] = save[j-1];
    save[j] = tmp;
  }
  save[j]->name = strdup(fullname);
  save[j]->surf = surf;
  save[j]->scaled = NULL;
  marked = j;

  savelist(); // ulozim seznam

  calculate_gsaves(); // zmenil se pocet tlacitek
  gsaves_pointer(1, 0);
  enl_marked = 1;
}

// nacte pozici s rameckem (pri kliknuti nepo pri F3)
static void loadmarked()
{
  cairo_t *cr;

  if(!loadmoves(savefile(save[marked]->name, "fsv"))) // nepovedlo se otevrit ulozenou pozici
    warning("Loading position %s failed", savefile(save[marked]->name, "fsv"));
  else{
    keyboard_erase_queue();
    dodge_gsaves(); // uhnu s mysi mimo gsaves (aby se dalo pokracovat v hrani)

    unanimflip();
    if(save[marked]->surf == unknown_icon){ // pokud chybel nahled, tak ho dovyrobim
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

// funkce volana pri stisknuti F3
void gsaves_load()
{
  if(!savesnum) return;

  if(mode != NORMAL) gsaves_unclick();

  if(marked != focused || act < 1) enl_marked = 1;
  loadmarked();
}
