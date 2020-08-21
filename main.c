#include<stdio.h>
#include <cairo/cairo.h>
#include <stdlib.h>
#include <string.h>

#include "warning.h"
#include "X.h"
#include "window.h"
#include "loop.h"
#include "menuevents.h"
#include "script.h"
#include "levelscript.h"
#include "menuscript.h"
#include "draw.h"
#include "main.h"
#include "rules.h"
#include "gmoves.h"
#include "keyboard.h"
#include "directories.h"

static void usage() // kdyz jsou spatne parametry -- vypise Usage a ukonci program
{
  printf("Usage: ff-clone [-d <data directory>] [<user level path>]\n");
  exit(0);
}

int main(int argc, char **argv)
{
  FILE *f;
  int width, height, g, fs, sf;

  // rozeberu parametry
  if(argc == 2 &&
     (!strcmp(argv[1], "-?") || !strcmp(argv[1], "-h") ||
      !strcmp(argv[1], "-help") || !strcmp(argv[1], "--help")))
    usage();

  if(argc > 4) usage();
  if(argc > 2){
    if(strcmp(argv[1], "-d")) usage();
    datadir = argv[2];
  }

  if(argc % 2 == 0) userlev = argv[argc-1];
  else userlev = NULL;

  gargc = argc;
  gargv = argv;

  init_directories(); // ziskam homedir
  // defaultni hodnoty (kdyby se nepovedlo nacteni lastconf)
  win_width  = 800;
  win_height = 600;
  gridmode = 0;
  win_fs = 0;
  safemode = 0;

  // nactu ~/.ff-clone/lastconf.txt (stav, ve kterem to bylo vypnuto)
  f = fopen(homefile("lastconf.txt"), "r");
  if(f){
    if(fscanf(f, "width %d, height %d, grid %d, fullscreen %d, safemode %d\n",
	      &width, &height, &g, &fs, &sf) == 5){
      fclose(f);
      if(width > 0) win_width = width;
      if(height > 0) win_height = height;
      gridmode = g;
      win_fs = fs;
      safemode = sf;
    }
  }
  apply_safemode();

  // inicializacni zalezitosti
  initX();
  init_keyboard();
  initlua();
  init_gmoves();
  init_draw();

  createwin(); // vytvoreni okna
  create_win_cr(); // a moznost do nej kreslit

  if(userlev){ // spousteno s parametrem mistnosti
    menumode = 0;
    open_user_level();
  }
  else{ // klasicke spusteni
    menumode = 1;
    menu_initlua();
    update_window_title();
    img_change |= CHANGE_ALL;
  }

  loop(); // a vecny cyklus

  return 0;
}

void escape() // ukonceni levelu / hry
{
  FILE *f;

  if(menumode || userlev){ // ukonceni  hry
    f = fopen(homefile("lastconf.txt"), "w"); // ulozeni lastconf.txt (stavu pri vypnuti)
    if(f){
      fprintf(f, "width %d, height %d, grid %d, fullscreen %d, safemode %d\n",
	      win_fs ? owin_width  : win_width,
	      win_fs ? owin_height : win_height,
	      gridmode,
	      win_fs,
	      safemode);
      fclose(f);
    }
    else warning("Failed to write %s\n", homefile("lastconf.txt"));
    exit(0);
  }
  else{ // ukonceni levelu
    end_level();
    menumode = 1;
    menu_pointer(); // ktere kolecko je ted pod mysi?
    img_change |= CHANGE_ALL;
  }
}
