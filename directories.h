char *datadir; // cesta k datum
char *homedir; // cesta do skryteho adresare v home (vcetne cesty k home)
char *savedir; // cesta do adresare s ulozenymi pozicemi

char *datafile(char *filename); // vrati datadir/filename
char *homefile(char *filename); // vrati homedir/filename
char *savefile(char *filename, char *ext); // vrati savedir/filename.ext

void init_directories(); // nastavi homedir, pripravi k nemu cestu
void level_savedir_init(); // nastavi savedir, pripravi k nemu cestu
