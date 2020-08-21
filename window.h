Atom wmDeleteMessage;
Window win, real_win;
int win_width, win_height, surf_width, surf_height;
int owin_width, owin_height;
cairo_t *win_cr;
char win_fs, win_resize;
cairo_surface_t *win_surf;

void createwin();
void free_win_cr();
void create_win_cr();
void repaint_win(int x, int y, int width, int height);
void fullscreen_toggle();
void calculatewin();
