#define SQUARE_WIDTH 50 // Velikost jednoho policka v plnem rozliseni
#define SQUARE_HEIGHT SQUARE_WIDTH

/* Nasledujici struktura uchovava obrazek, ktery bude pri hre vykreslovan.
   v polozce surf je ve sve puvodni velikosti, ve polozce scaled pak v takove
   velikosti, ve ktere bude vykreslovan do okna. */
typedef struct ff_image{
  cairo_surface_t *surf;
  float x, y;  // offset pri vykreslovani (v polickach)
  float scalex, scaley;       /* cca. 1/50, kolikrat se musi zmensit surf,
				 aby jednotky bula policka */

  cairo_surface_t *scaled;
  int width, height;  // Velikost upravene pixmapy
  float curscalex, curscaley; /* cca. 1/30, kolikrat se musi zmensit scaled,
				 aby jednotky byla policka */

  struct ff_image *next;
} ff_image;

/*
  Pro zobrazeni ff_image je treba takovy obrazek umistit do tzv. ff_layer.
  Tato struktura vedle obrazku jeste obsahuje objekt, ke kteremu je tento obrazek
  vazan. Souradnice obrazku a objektu se sectou, a tam pak bude ona surface
  vykreslena. Jeden obrazek muze totiz zastupovat vice objektu a taky jeden objekt muze
  obsahovat vice obrazku (ma-li nektera jeho cast byt pod a jina nad).

  Objekt vrstvy take muze byt NULL, pak jsou pouzity jen souradnice obrazku.
 */
typedef struct ff_layer{
  ff_image *img; // Hlavni dve polozky
  struct ff_object *obj;

  float depth; // Vetsi cislo znamena, ze je zobrazovana vice vpredu

  struct{ // v soucasne dobe nema vyznam, nepovedeny pokus o optimalizaci
    int x, y, width, height;
  } boundary[2];
  char change;

  float last_x, last_y; // posledni stav, abych vedel, zda se neco zmenilo
  ff_image *last_img;

  struct ff_layer *next, *nextdepth; // dalsi ve spojaku, vice viz new_layer() v layers.c
} ff_layer;

/*
  Po nacteni globalnich obrazku spolecnych vsem levelum, se budou nacitat lokalni
  obrazky prave spusteneho levelu. Tyto obrazky ale maji byt zniceny po vypnuti levelu.
  Ze je nacitany obrazek lokalni se pozna podle toho, ze je promena local_image_mode
  nastavena na true.
*/
char local_image_mode;

ff_image *new_image(cairo_surface_t *surf, float x, float y,
		    float scalex, float scaley);
   // Vyrobi novy obrazek (ale potrebuje uz existujici sairo_surface) a vrati na nej pointer

ff_image *load_png_image(const char *filename, float x, float y, // Nacte obrazek z PNG
			 float scalex, float scaley);

ff_layer *new_layer(struct ff_object *obj, float depth);
  // vytvori novou ff_layer a vrati na ni pointer

void draw_layers(cairo_t *cr, float scalex, float scaley);
  // Vykresli vsechny vrstvy (pri hre)

void draw_layers_noanim(cairo_t *cr);
  // Vykresli vsechny vrstvy, ovsem bez animaci, za ucelem vytvoreni nahledu

char layers_change(); /* vrati CHANGE_ROOM, jestli se nejaka vrstva zmenila
			 od posledniho volani teto funkce, jinak vrati 0 */
void delete_layers(); // smaze vrstvy a lokalni obrazky

int script_new_layer(lua_State *L); // analogie funkci vyse, pro lua
int script_load_png_image(lua_State *L);
int script_setlayerimage(lua_State *L); // nastavi vrstve obrazek

// v soucasne dobe nema vyznam, nepovedeny pokus o optimalizaci
void count_layers_boundary(int index, char first, cairo_t *cr);
