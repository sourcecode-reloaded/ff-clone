/*
  Souradnicovy system menu, podle ktereho je urcovana poloha kolecek a car je:
  (0, 0) = pravy horni roh okna
  (0, MENU_HEIGHT) = pravy dolni roh okna
  x-ova osa smeruje doleva
 */
#define MENU_HEIGHT   300

void menu_draw(); // vykresli mapu -- cary, kolecka, pripadne nahled oznacene mistnosti

void menu_create_icon();
     // Podiva se, jestli mistnosti chybel nahled. Jestli ano, tak ho vytvori.
