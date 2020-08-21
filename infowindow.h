void infowindow(const char *str, char helpmode);
   /* Zobrazi okno se zadanym stringem, do zavreni tohoto okna zablokuje hru.
      Radky jsou oddeleny znakem '\n'. Pokud je helpmode true, zavre se i pri
      prijmuti klavesy v hlavnim okne. */

int menuscript_infowindow(lua_State *L); // LUA verze infowindow, neumoznuje helpmode

void show_help(); // zobrazi okno s napovedou
