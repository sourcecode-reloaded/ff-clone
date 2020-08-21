#define SMALL 1
#define BIG   2
#define SOLID 3
#define STEEL 4
#define LIGHT 5

#define UP    0
#define RIGHT 1
#define DOWN  2
#define LEFT  3

#define NUMOBJLISTS 5 // 0,1,2,3 - rules.c, 4 - moves.c

typedef struct{
  int x, y;
} coor;

typedef struct ff_object{
  char color[4];
  char type;
  coor c;
  int width, height;
  char *data;
  int dirnum[4];
  coor *dirsquares[4];
  int index[NUMOBJLISTS];

  char out, live, flip;
  float gflip, ogflip;

  struct ff_object *next;
} ff_object;

ff_object *first_object, **objlist[NUMOBJLISTS];
int objnum;

int room_width;
int room_height;

ff_object *new_object(const char *color, char type, int x, int y,
		      int width, int height, const char *data);
int script_new_object(lua_State *L);
int script_setroomsize(lua_State *L);
void finish_objects();
void swapobject(ff_object *obj1, int index2, int list);
int backdir(int dir);
coor coord(int x, int y);
coor coorsum(coor c1, coor c2);
void delete_objects();
