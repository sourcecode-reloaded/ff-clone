/*
  Tento modul obsahuje jedinou funkci - imgsave. Ta ulozi do zadaneho filename soucasny obrazek
  mistnosti o rozmerech width, height. Parametr draw_tick rika, ze se pres to ma jeste nakreslit
  zelena fajfka (na vyresenych ulozenych pozicich) a parametr draw_moves rika, ze se pres to ma
  nakreslit pocet tahu. Vedle toho, ze bude obrazek ulozen do souboru, bude soucasne predan zpet
  jako navratova hodnota.
 */
cairo_surface_t *imgsave(char *filename, int width, int height, char draw_tick, char draw_moves);
