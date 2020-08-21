/*
  O postupne odkryvani vetvi se stara LUA skript. Postupne kolecka, ktera
  jsou jiz odkryta jsou ve spojovem seznamu struktury menu_node. Cary spojujici
  tato kolecka jsou ve spojovem seznamu struktury menu_line.

  Tento LUA skript ovsem neni ten samy jako v modulu script, pouziva zcela odlisne
  funkce a je udrzovan v promenne menu_luastat, coz je ovsem staticka promenna.
  Obdobne jako v hlavnim skriptu maji funkce volane timto skriptem predponu
  "menuscript_", v samotnem skriptu misto ni vsak "C_". Vetsina LUA funkci je v
  .c souboru tohoto modulu

  Skript se nacte ze souboru data/scripts/menuscript.lua
 */

typedef struct menu_node{ // kolecko na mape
  double x, y;  // souradnice stredu kolecka, viz menudraw.h

  char *codename;
  char *name;  // cesky nazev mistnosti

  enum {
    NODE_DISABLED, // hnede kolecko
    NODE_ENABLED,  // modre kolecko
    NODE_SOLVED    // zlute kolecko
  } state;

  cairo_surface_t *icon; // nahled
  char *icon_name; // nazev souboru, ze ktereho mela byt nacten nahled

  struct menu_node *next;
} menu_node;
menu_node *first_menu_node;  // prvni prvek seznamu

menu_node *active_menu_node; /* mistnost, nad kterou je mys, pripadne
				spustena mistnost */

typedef struct menu_line{  // usecka na mape
  double x1, y1; // souradnice pocatku caru
  double x2, y2; // souradnice konce cary
  struct menu_line *next;
} menu_line;
menu_line *first_menu_line; // prvni prvek spojoveho seznamu

void menu_initlua(); // inicializuje skript a promenne tohoto modulu

void menu_solved_node(menu_node *n, int m);
   /* oznami LUA skriptu vyreseni mistnosti (a ten se postara o zbytek)
      n = vyresena mistnost, m = pocet tahu tohoto reseni */
