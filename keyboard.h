char key_press(XKeyEvent xev);  // reakce na udalost zmacknuti klavesy, vrati, zda klavesa mela efekt
void key_release(XKeyEvent xev);  // reakce na udalost pusteni klavesy
void key_remap(); // prezkoumani pustenych klaves (pri zmene focusu)
void keyboard_step(); // spusteno kazdy frame, aplikuje zmacknutou sipku
void kb_apply_safemode(); // zaridi, aby konstanty tohoto modulu odpovidaly stavu promenne safemode

void level_keys_init(); // volano pri spusteni levelu
void init_keyboard();  // inicializace modulu

void keyboard_erase_queue(); // promaze frontu klaves

int keyboard_blockanim; /* Pri drzeni klaves Home, End, PgUp, PgDown je zablokovana animace,
			   aby u levelu, ve kterych na zacatku pada predmet, bylo mozne si tento
			   predmet prohlednot pred padem. Animace je takto blokovana pri
			   keyboard_blockanim != 0 */
