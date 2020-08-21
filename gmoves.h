int gmoves_width, gmoves_height;
char gmoves_vertical;
int rewinding;
char slider_hold;

void init_gmoves();
void level_gmoves_init();
void drawgmoves(cairo_t *cr, char repaint);
void gmoves_click(XButtonEvent *xbutton);
void gmoves_unclick(XButtonEvent *xbutton);
void moveslider(XMotionEvent *xmotion);
void rewind_moves(int d, char keyon);
void rewind_stop();
