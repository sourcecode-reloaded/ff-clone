#define ROOM_IDLE  0
#define ROOM_MOVE  1
#define ROOM_FALL  2
#define ROOM_BEGIN 3

#define FISHLIST   2

struct ff_object *active_fish, *start_active_fish;
char room_state;
int room_movedir;
int fishnum;

void init_rules();
void free_rules();
void changefish();
void movefish(int dir);
void room_step();
char ismoving(struct ff_object *obj);
char isonfish(struct ff_object *obj);
char issolved();

void setactivefish(struct ff_object *fish);
void hideobject(struct ff_object *obj);
void hideallobjects();
void showobject(struct ff_object *obj);
void killfish(struct ff_object *fish);
void vitalizefish(struct ff_object *fish);

void fallall();
void findfishheap();

void animflip();
void unanimflip();
