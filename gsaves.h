/*
  Gsaves je mac menu ulozenych pozic, veskere animace v tomto mac menu resim sam.
*/

double GSAVES_ACT_SPEED; // za kolik framu se tlacitko zvetsi / zmensi
double GSAVES_SPACE_SPEED; /* za kolik framu se presune mezera mezi dvema
			      tlacitky (pri presouvani tlacitka) */

char gsaves_vertical; // gsaves jsou svisle
int gsaves_width; // sirka svislych neaktivnich gsaves
int gsaves_height; // vyska vodorovnych neaktivnich gsaves
int gsaves_x_size, gsaves_y_size; // rozmery aktivnich gsaves v aktualni orientaci
int gsaves_length; // delka, tj. vyska (jsou-li gsaves svisle) nebo sirka (jsou-li vodorovne)

char gsaves_blockanim; // gsaves jsou aktivni, tak nebudeme zatezovat procesor dalsimi animacemi

/*
  Gsaves mohou precuhovat pres ostatni ovladaci prvky. V takovem pripade je treba prekreslit
  gsaves i v okamziku, kdy se zmenilo neco pod nimi. Promenna gsaves_change_mask tedy
  udava, prez ktere prvky gsaves precuhuje (v analogickem formatu jako promenna img_change
  z draw.h).
*/
unsigned char gsaves_change_mask;

void level_gsaves_init(); /* pri startu levelu spocita nemenne velikosti (napriklad velikost
			     zvetseneho tlacitka) a nacte seznam ulozenych pozic */

void drawgsaves(cairo_t *cr, char bg); /* Vykresli gsaves. Parametr bg udava, jestli se zmenil
					  nejaky prvek, pres ktery gsaves precuhuje, a je tedy
					  treba znovu ulozit pozadi gsaves. */

void calculate_gsaves(); // prepocita parametry gsaves zavisle na velikosti okna

void gsaves_click(); // zprava tomuto modulu o kliknuti mysi
void gsaves_unclick(); // zprava tomuto modulu o pusteni tlacitka mysi

/* Nasledujici funkce zpracuje zmenu polohy kurzoru mysi. Prvni parametr force udava, jestli
   to je opravdu treba prepocitat, i kdyz se mys z minula nepohnula. Druhy rika, jestli volani
   teto funkce pochazi od udalosti LeaveNotify. Ta je totiz nekdy volana v nevhodnych situacich
   (treba pred tim, nez posunu mys), takze nektere veci se s ni ignoruji.
 */
void gsaves_pointer(char force, char leave_ev);

void gsaves_anim(); // provede jeden krok animace
void hidegsaves(cairo_t *cr); // smaze gsaves z okna, aby tam uz nic nikde neprecuhovalo
void delete_gsaves(); // uvolni pamet pouzivanou timto modulem (pri opousteni mistnosti)
void gsaves_unanim(); // okamzite deaktivuje gsaves

void gsaves_save(); // ulozeni pozice, volano pri stisku F2
void gsaves_load(); // nacteni pozice, volano pri stisku F3
