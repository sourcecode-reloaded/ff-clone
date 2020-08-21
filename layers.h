#define SQUARE_WIDTH 50
#define SQUARE_HEIGHT SQUARE_WIDTH

typedef struct ff_image{
  cairo_surface_t *surf;
  int width, height;
  float x, y;
  float scalex, scaley;       // cca. 1/50
  float curscalex, curscaley; // cca. 1/30
  cairo_surface_t *scaled;

  struct ff_image *next;
} ff_image;

typedef struct ff_layer{
  ff_image *img;
  struct ff_object *obj;
  struct{
    int x, y, width, height;
  } boundary[2];

  float last_x, last_y, depth;
  ff_image *last_img;

  char change;

  struct ff_layer *next, *nextdepth;
} ff_layer;

char local_image_mode;

ff_image *new_image(cairo_surface_t *surf, float x, float y,
		    float scalex, float scaley);
ff_image *load_png_image(const char *filename, float x, float y,
			 float scalex, float scaley);
ff_layer *new_layer(struct ff_object *obj, float depth);
void draw_layers(cairo_t *cr, float scalex, float scaley);
void draw_layers_noanim(cairo_t *cr);
void count_layers_boundary(int index, char first);
char layers_change();
void delete_layers();

int script_new_layer(lua_State *L);
int script_load_png_image(lua_State *L);
int script_setlayerimage(lua_State *L);
