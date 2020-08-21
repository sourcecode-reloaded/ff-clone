#define SMALL 1  // mala ryba
#define BIG   2  // velka ryba
#define SOLID 3  // zed
#define STEEL 4  // tezky predmet
#define LIGHT 5  // lehky predmet

#define UP    0
#define RIGHT 1
#define DOWN  2
#define LEFT  3

#define NUMOBJLISTS 5 /* 0,1,2,3 - rules.c, 4 - moves.c
			 vice viz objlist nize */

typedef struct{ // souradnice
  int x, y;
} coor;

/*
  Predavani objektu se skriptem funguje tak, ze kdyz C-ckova funkce
  chce po skriptu predat objekt, da mu jeho barvu z PNG mapy (polozka
  color). Naproti tomu, kdyz skript chce predat objekt C-ckove funkci,
  preda ji primo C-ckovy ukazatel.
 */

typedef struct ff_object{  // predmet ve hre (nebo i ryba / zed)

  char color[4];  // barva v PNG mape -- identifikacni kod pro lua
  char type;      // zed / ocel / ryba / ...
  coor c;         // souradnice (v polickach)
  int width, height; // vyska, sirka (v polickach)

  char *data;    // tvar objektu
  int dirnum[4]; // poèet políèek pøiléhajících v daném smìru k objektu
  coor *dirsquares[4]; // políèka pøiléhajících v daném smìru k objektu
  // vice viz analyzeobject v object.c

  int index[NUMOBJLISTS]; // vice objlist nize

  char out;  // nakolik je mimo mistnost viz rules.c
  char live; // je nyni zivy?
  char flip; // orientace ryby (0=vlevo, 1=vpravo)
  float gflip; // animace orientace ryby (mezi 0 a 1)
  float ogflip; /* puvodni hodnota gflip (kvuli zjisteni,
		   zda se je treba prekreslit obrazovku) */

  struct ff_object *next; // dalsi objekt ve spojovem seznamu
} ff_object;

ff_object *first_object; // prvni objekt ve spojovem seznamu
int objnum; // pocet objektu

/* Vedle spojoveho seznamu jsou objekty v nekolika (NUMOBJLISTS) polich
   objlist[], ruzne sprehazeny. Navic kazdy objekt ma ulozeno, na jake
   pozici daneho pole objlist se nachazi, v polozce index.
 */
ff_object **objlist[NUMOBJLISTS];
/*
  Pro prohozeni dvou objektu v danem poli objlist slouzi nasledujici funkce.
  prvni parametr udava objekt, druhy na kterou pozici se ma premistit
  (tedy s kterym objektem vymenit) a treti ktery objlist se vlastne ma pouzit.
 */
void swapobject(ff_object *obj1, int index2, int list);

int room_width;  // celkovy pocet policek na sirku
int room_height; // celkovy pocet policek na vysku

ff_object *new_object(const char *color, char type, int x, int y,
		      int width, int height, const char *data);
   // vyrobi novy objekt a zaradi do spojoveho seznamu

int script_new_object(lua_State *L);  // verze new_object pro lua
int script_setroomsize(lua_State *L); // nastavi rozmery (lua funkci)
void finish_objects();  /* spocita objekty, vytvori objlisty, tato funkce
			   se spusti po vyrobeni vsech objektu */

int backdir(int dir);  // vrati opacny smer
coor coord(int x, int y);  // vrati zadane souradnice ve strukture coor
coor coorsum(coor c1, coor c2); // secte souradnice (po slozkach)
void delete_objects();  // uvolni z pameti vsechny objekty
