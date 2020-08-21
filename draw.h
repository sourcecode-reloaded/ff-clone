float room_x_scale;   // cca 30, transformace pixel->velikost policka
float room_y_scale;

int room_x_translate; // souradnice mistnosti v okne
int room_y_translate;

int room_x_size; // velikost mistnosti v pixelech
int room_y_size;

float anim_phase; // o kolik jsou posunute posouvane objekty

float heap_x_advance, heap_y_advance; // houpani ryb a predmety na nich

char safemode; // je hra v uspornem rezimu?

/*
  Nasledujici promenna udava, ktere komponenty se zmenily
  a je treba je prekreslit.
*/
char img_change;

// Moznosti jsou:
#define CHANGE_ROOM   1 // hlavni okno hry
#define CHANGE_GMOVES 2 // undo pasecek
#define CHANGE_GSAVES 4 // pasecek s ulozenymi pozicemi
#define CHANGE_ALL    (CHANGE_ROOM | CHANGE_GMOVES | CHANGE_GSAVES)

// mrizka je
enum {
  GRID_OFF, // vypnuta
  GRID_BG,  // na pozadi
  GRID_FG   // nebo na popredi
} gridmode;

cairo_pattern_t *bg_pattern; // barva pozadi -- tmave modra

void draw(); // vykresli okno
void anim_step(); // posune animaci o jeden krok

void unanim_fish_rectangle(); // okamzite presune obdelnicek nad aktivni rybu
void apply_safemode(); // na zaklade promenne safemode nastavi patricne promenne
void level_anim_init(); // inicializuje animace
void start_moveanim(); // ryba se zacina hybat
void init_draw(); // nastavi promennou bg_pattern
