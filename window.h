Atom wmDeleteMessage; // pomoci tohoto se pozna zavreni krizkem
Window real_win; // hlavni okno
Window win; // podokno hlavniho okna, ktere ma stejne rozmery a je mozne do nej krelit

int win_width, win_height;  // velikost okna
int owin_width, owin_height; // velikost okna pred fullscreenem

char win_fs;  // hra je ted ve fullscreenu

/* Aby bylo prekreslovani co mozna nejmene rusive, je poreseno nasledovne:
   Kreslici funkce vykresluji pomoci caira (win_cr) na surface win_surf, coz je
   ve skutecnosti Pixmapa X serveru. Teprve po dokresleni je prekresleno okno
   win touto pixmapou. Pri zmene velikosti hlavniho okna neni okamzite zmenena
   velikost, ale pocka se az na prvni vykresleni nove jinak velke pixmapy. Do te
   doby je na true nastavena promenna win_resize.
 */
cairo_t *win_cr;
cairo_surface_t *win_surf;
char win_resize;

void createwin(); // vyrobi hlavni okno
void create_win_cr(); // vyrobi pro okno spravne velkou pixmapu a cairo kontext
void free_win_cr(); // smaze pixmapu a cairo kontext (pri zmene velikosti)

void repaint_win(int x, int y, int width, int height);
  // prekresli pixmapu do okna na zadanem obdelniku

void fullscreen_toggle(); // zapne/vypne fullscreen
void calculatewin(); // Spocita, jak budou v okne rozmisteny ovladaci prvky
void update_window_title();
   // nastavi titulek okna: Fish Fillets Clone[ - level '$codename']
