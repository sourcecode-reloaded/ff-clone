double GSAVES_ACT_SPEED;
double GSAVES_SPACE_SPEED;

char gsaves_vertical;
int gsaves_width, gsaves_height;
int gsaves_x_size, gsaves_y_size;
int gsaves_length;
char gsaves_blockanim;
unsigned char gsaves_change_mask;
int recent_saved;

void level_gsaves_init();
void drawgsaves(cairo_t *cr, char bg);
void calculate_gsaves();
void gsaves_click();
void gsaves_unclick();
void gsaves_pointer(char force, char leave_ev);
void gsaves_anim();
void hidegsaves(cairo_t *cr);
void delete_gsaves();
void gsaves_unanim();
void gsaves_save();
void gsaves_load();
