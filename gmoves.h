int   FIRSTUNDOWAIT; // auto repeat delay u tlacitek '-' a '+' ve framech
int   UNDOWAIT; // auto repeat rate u tlacitek '-' a '+' ve framech

int gmoves_width; // sirka svisleho undo pasecku
int gmoves_height; // vyska vodorovneho undo pasecku
char gmoves_vertical; // je undo pasecek svisly?
int rewinding; // -1 = zmacknute '-', 1 = zmacknute '+', 0 = nezmacknute nic
char slider_hold; // je drzeny posuvnik?

void init_gmoves(); // inicializace promennych undo pasecku, napr gmoves_width
void level_gmoves_init(); // pri startu levelu nastavi undo pasecek na necinnost
void rewind_step(); // jeden krok animace, pokud rewinding != 0

void drawgmoves(cairo_t *cr, char repaint);
  // vykresli undo pasecek, repaint znamena, ze je pred tim treba premazat kreslici plochu
void gmoves_click(XButtonEvent *xbutton);
  // zprava tomuto modulu, ze se kliklo mysi
void gmoves_unclick(XButtonEvent *xbutton);
  // zprava tomuto modulu, ze byla pusteno tlacitko mysi
void moveslider(XMotionEvent *xmotion);
  // zprava tomuto modulu, ze se posunula mys, pokud je drzen posuvnik
void rewind_moves(int d);
  // zahaji prehravani tahu v danem smeru
void rewind_stop(int d);
  // ukonci prehravani tahu v danem smeru
