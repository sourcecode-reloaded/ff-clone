/*
  Promenna room_codename udava, jak se bude jmenovat adresar s ulozenymi pozicemi a
  je nastavena lua skriptem. Bud bude rovna codename prislusneho node v menu nebo
  v pripade uzivatelova levelu "user_<jmeno souboru bez pripony>".
 */
char *room_codename;
int script_set_codename(lua_State *L); // nastavi room_codename (ocekava jeden parametr -- string)

/*
  Funkce script_openpng se stara o prvotni dekodovani PNG obrazku pro ucely vyrobeni mistnosti.
  Skript ji zavola s parametrem nazvu souboru, ona pomoci cairo otevre obrazek a vrati skriptu
  trojici (string data, int width, int height). Polozky width a height udavaji sirku a vysku.
  Polozka data jsou po radcich zapsane pixely, kazdy pixel se sklada z ctyrech byteu:
  postupne modra slozka, zelena slozka, cervena slozka, alpha kanal.
 */
int script_openpng(lua_State *L);

void open_level(char *codename); // otevre level z menu
void open_user_level(); // otevre uzivatelsky level, cestu najde v promenne userlev z main.h
void end_level(); // ukonci level (kompletne se postara o uvolneni prislusnych struktur vsech modulu)
void refresh_user_level(); // obnovi uzivatelsky level (zkratka Ctrl-R)

int script_flip_object(lua_State *L); // zmeni flip u zadaneho objektu
int script_start_active_fish(lua_State *L); // nastavi rybu aktivni na zacatku (dostane ji jako parametr)
