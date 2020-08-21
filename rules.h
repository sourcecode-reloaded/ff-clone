enum{
  ROOM_IDLE,   // nic se nedeje
  ROOM_MOVE,   // ryba se posunuje
  ROOM_FALL,   // predmety padaji
  ROOM_BEGIN,  // predmety budou padat (pri startu)
} room_state;

#define FISHLIST        2  // seznam ryb, viz rulec.c, kapitola 
  // zbyle objlisty definovane v rules.c

struct ff_object *active_fish; // prave aktivni ryba (s rameckem)
struct ff_object *start_active_fish; // ryba aktivni pri startu
int room_movedir; /* smer kterym se hybou hybajici se predmety, tedy
		     bud smer posunu ryby, nebo DOWN, padaji-li. */

int fishnum; // pocet ryb

void init_rules(); /* inicializuje tento modul, nutno spoustet po
		      finish_objects() */
void free_rules(); /* uvolni pamet pouzivanou timto modulem
		      (volano pri opousteni mistnosti) */

void changefish(); // prepne na dalsi (zivou, neodeslou) rybu

void movefish(int dir);  /* spocita pohyb aktivni ryby
			    -- volano pri stisku sipky */
void room_step();  /* posune o jedno policko hybajici se predmety
		      a spocita dalsi vyvoj */

char ismoving(struct ff_object *obj); // vrati, zda se objekt prave hybe
char isonfish(struct ff_object *obj); // vrati, zda objekt lezi na hromadce ryb
char issolved();  // vrati, zda je mistnost prave vyresena

void setactivefish(struct ff_object *fish); // nastavi aktivni rybu

void hideobject(struct ff_object *obj);
void hideallobjects();
void showobject(struct ff_object *obj); // viz kapitola "hraci plan" v rules.c

void killfish(struct ff_object *fish); // nastavi rybu do mrtveho stavu
void vitalizefish(struct ff_object *fish); // nastavi rybu do ziveho stavu

void fallall();  // zjisti, ktere objekty budou padat (na zacatku hry)
void findfishheap(); // prepocita, ktere objekty lezi na rybach (po undo)

void animflip(); // posune rybam gflip blize k skutecnemu flip (tedy pootoci je)
void unanimflip(); // nastavi rybam gflip na skutecny flip
