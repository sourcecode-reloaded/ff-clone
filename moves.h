#define FLIPCHANGE 0
#define LIVECHANGE 1

int moves, minmoves, maxmoves;
char room_sol_esc;

void init_moves();

void startmove(struct ff_object *fish, int dir);
void addobjmove(struct ff_object *obj);
void addobjchange(struct ff_object *obj, char type);
void finishmove();

char setmove(int move);

char savemoves(char *filename);
char loadmoves(char *filename);
void free_moves();
