char *datadir, *homedir, *savedir;

char *datafile(char *filename);
char *homefile(char *filename);
char *savefile(char *filename, char *ext);
void init_directories();
void level_savedir_init();
