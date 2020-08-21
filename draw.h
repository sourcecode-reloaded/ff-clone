float room_x_scale;   // cca 30
float room_y_scale;
int room_x_translate;
int room_y_translate;
int room_x_size;
int room_y_size;
float anim_phase;
float heap_x_advance, heap_y_advance;
char safemode;
char img_change;

#define CHANGE_ROOM   1
#define CHANGE_GMOVES 2
#define CHANGE_GSAVES 4
#define CHANGE_ALL    (CHANGE_ROOM | CHANGE_GMOVES | CHANGE_GSAVES)

enum {GRID_OFF, GRID_BG, GRID_FG} gridmode;
cairo_pattern_t *bg_pattern;

void draw();
void anim_step();
void unanim_fish_rectangle();
void apply_safemode();
void level_anim_init();
void start_moveanim();
void init_draw();
