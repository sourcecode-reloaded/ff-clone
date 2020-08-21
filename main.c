#include<stdio.h>
#include <cairo/cairo.h>
#include <stdlib.h>
#include <string.h>

#include "warning.h"
#include "X.h"
#include "window.h"
#include "loop.h"
#include "menuloop.h"
#include "script.h"
#include "levelscript.h"
#include "menuscript.h"
#include "draw.h"
#include "main.h"
#include "rules.h"
#include "gmoves.h"
#include "keyboard.h"
#include "directories.h"

static void usage()
{
  printf("Usage: ff-clone [-d <data directory>] [<user level path>]\n");
  exit(0);
}

int main(int argc, char **argv)
{
  FILE *f;
  int width, height, g, fs, sf;

  if(argc > 4) usage();
  if(argc > 2){
    if(strcmp(argv[1], "-d")) usage();
    datadir = argv[2];
  }

  if(argc % 2 == 0) userlev = argv[argc-1];
  else userlev = NULL;

  gargc = argc;
  gargv = argv;

  init_directories();

  win_width  = 800;
  win_height = 600;
  gridmode = 0;
  win_fs = 0;
  safemode = 0;

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

  initX();
  init_keyboard();
  initlua();
  init_gmoves();
  init_draw();

  createwin();
  create_win_cr();

  apply_safemode();

  if(userlev){
    menumode = 0;
    open_user_level();
  }
  else{
    menumode = 1;
    menu_initlua();
  }

  loop();

  return 0;
}

/*
void escape()
{
  endlua();
  exit(0);
}
*/

void escape()
{
  FILE *f;

  if(menumode || userlev){
    f = fopen(homefile("lastconf.txt"), "w");
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
  else{
    end_level();
    menumode = 1;
    img_change |= 2;
    menu_pointer();
  }
}
