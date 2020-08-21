char *userlev; /* nazev uzivatelova levelu (zadan jako parametr prikazove radky),
		  nebyl-li zadan, hodnota je NULL */

// zpristupneni argv a argc i jinym funkcim nez main
int gargc;
char **gargv;

int main();
void escape(); // unik z mistnosti nebo hry
