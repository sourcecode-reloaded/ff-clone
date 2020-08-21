#include <lualib.h>
#include <lauxlib.h>

/*
  Dva oddelene LUA skripty se staraji o dve zalezitosti. Prvni hlavni skript resi
  nacitani levelu. Presto se tento skript spusti pouze jednou. Pri inicializaci nacte
  $datadir/scripts/levelscript.lua, tedy napriklad i obrazky ryb. Po te tento skript
  otvira jednotlive mistnosti, a po ukonceni se opet zbavuje nabranych dat. Otevirani
  a zavirani levelu je reseno v modulu levelscript.

  Vedle hlavniho skriptu je jeste skript, ktery definuje, jak bude zobrazena mapa mistnosti,
  a kontroluje halu slavy. O tento druhy skript se modul script takrka vubec nestara,
  podrobnosti viz menuscript.h.

  Hlavni skript je uchovavan v promenne luastat. Funkce pripravene pro tento skript
  (tedy takove, ktere dostanou jako parametr lua_State a vrati pocet navratovych hodnot)
  zacinaji predponou "script_" a jsou roztrousene v modulech gener, layers, levelscript
  a object. V lue se pak tyto funkce budou jmenovat stejne s tim rozdilem, ze predpona
  "script_" bude nahrazena predponou "C_". A opet, funkce z LUA skriptu, ktere budou
  spousteny z C budou mit rovnez predponu "script_".

  Vedle funkci budou jeste do hlavniho skriptu preneseny cisla typu predmetu (z object.h) a
  stringy datadir a homedir. Opet jejich nazev v lue bude zacinat predponou "C_".
 */

lua_State *luastat;
void initlua(); // inicializuje luastat

void *my_luaL_checkludata(lua_State *L, int narg);
  /* V klasicke luaxlib funkce, ktera otestuje ludata, chybi.
     Pouziti stejne jako ostatni luaL_check... */

void setluadirs(lua_State *L);
  // Nastavi promenne C_datadir a C_homedir (vyuzivano i v menuscript)
