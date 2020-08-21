void loop(); // hlavni cyklus
void setdelay (int vteriny, int mikro); // nastavi casovou prodlevy mezi jednotlivymi framy

int mouse_x, mouse_y; // souradnice mysi v okne
void setmouse(); // nastavi souradnice mysi na mouse_x, mouse_y

char mouse_pressed; // je zmacknute leve tlacitko mysi?

void loop_noskip(); // oznami modulu, ze se cekalo dobu, kterou neni vhodno kompenzovat
