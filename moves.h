#define FLIPCHANGE 0 // ryba se otocila
#define LIVECHANGE 1 // ryba zemrela

int moves; // aktualni pocet tahu
int minmoves; // minimalni pocet tahu: -1 nebo 0 podle
int maxmoves; // maximalni pocet tahu v teto situaci, tedy kam az dosahne redo

char room_sol_esc;  /* mistnost byla vyresena, a proto se nyni
		       ma hra vratit do menu */

void init_moves(); /* inicializuje struktury pro undo historii,
		      nutno spoustet po init_rules() */

/*
  Undo historie bude ukladana tak, ze na zacatku kazdeho tahu se zavola
  funkce startmove s parametrem, ktera ryba se hybe kterym smerem. Dale
  v prubehu tahu budou volany funkce addobjmove(), ktera se pripravi na
  skutecnost, ze se objekt pohne, a addobjchange() s parametrem zmeny
  vlastnosti ryby (viz definice na zacatku tohoto souboru). Po dovrseni
  tahu (vsechny predmety dopadaji) je zavolana funkce finishmove().
 */
void startmove(struct ff_object *fish, int dir);
void addobjmove(struct ff_object *obj);
void addobjchange(struct ff_object *obj, char type);
void finishmove();


char setmove(int move); /* skoci do jineho mista (tah move) v undo historii
			   funkci je mozne dat i cislo mimo interval
			   [minmoves, maxmoves], ona si poradi. */

char savemoves(char *filename); // zapise ulozenou undo historii do souboru
char loadmoves(char *filename); /* nacte undo historii (ulozenou pozici)
				   ze souboru */
void free_moves(); /* uvolni pamet pouzivanou timto modulem
		      (volano pri opousteni mistnosti) */
