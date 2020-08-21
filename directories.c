#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/stat.h>

#include "warning.h"
#include "directories.h"
#include "script.h"
#include "levelscript.h"

/*
  Adresarova struktura programu:
    homedir: ~/.ff-clone/
      lastconf.txt   -  ulozene udaje z minula (mrizka, fulscreen, ...)
      solved.txt     -  seznam vyresenych mistnosti
      ulozene pozice: saves/
        adresar se jmenem codename levelu
	  list.txt  -  seznam ulozenych pozic
	  ulozena pozice:
	    $savetype_$moves_$index.fsv  - seznam tahu
	    $savetype_$moves_$index.png  - miniatura mistnosti
	    savetype = "sav" - ulozena pozice, nebo "sol" - vyresena pozice
	    moves = pocet tahu
	    index = rozliseni pro pripad, ze by se ve vsem ostatnim dve ulozene pozice shodovaly
    datadir: /usr/local/share/ff-clone/ (definovano v Makefile), pred instalaci v ./data/
      Lua skripty: gscripts/
        Pomocne lua funkce pro tvorbu levelu: levelscript.lua
	Vykresleni mapy mistnosti: menuscript.lua
      Obrazky ryb, oceli, ...: gimages/
      Adresare jednotlivych levelu: levels/
        v kazdem adresari pak
	  script.lua - skript, ktery se vzdy nacte
	  map.png - mapa, jak vypada ona mistnost, je ctena onim skriptem
	  obrazky predmetu
 */

#define INHOMEDIR "/.ff-clone/"

#ifdef DATADIR
char *datadir = DATADIR;
#else
char *datadir = "./data";
#endif

int asprintf(char **strp, const char *fmt, ...);

char *savedir = NULL;

static char *result_str; /* plna cesta k souboru, kterou nastavi a vrati funkce
			    datafile, homefile, savefile */
static int res_str_len; // kapacita result_str

char *datafile(char *filename) // vrati datadir/filename
{
  if(res_str_len < strlen(datadir) + 1 + strlen(filename)){
    error("Too long file path %d > %d: %s/%s",
	  strlen(datadir) + 1 + strlen(filename), res_str_len,
	  datadir, filename);
  }
  sprintf(result_str, "%s/%s", datadir, filename);
  return result_str;
}

char *homefile(char *filename) // vrati homedir/filename
{
  if(res_str_len < strlen(homedir) + strlen(filename)){
    error("Too long file path %d > %d: %s%s",
	  strlen(homedir) + strlen(filename), res_str_len,
	  homedir, filename);
  }
  sprintf(result_str, "%s%s", homedir, filename);
  return result_str;
}

char *savefile(char *filename, char *ext) // vrati savedir/filename.ext
{
  if(res_str_len < strlen(savedir) + strlen(filename) + 1 + strlen(ext)){
    error("Too long file path %d > %d: %s%s.%s",
	  strlen(savedir) + strlen(filename) + 1 + strlen(ext),
	  res_str_len, savedir, filename, ext);
  }
  sprintf(result_str, "%s%s.%s", savedir, filename, ext);

  return result_str;
}

void init_directories() // nastavi homedir, pripravi k nemu cestu
{
  char *homename;
  int len1, len2;

  homename = getenv("HOME");
  if(homename == NULL){
    warning("home dir unknown");
    homedir = NULL;
    len1 = 0;
    return;
  }
  else{
    asprintf(&homedir, "%s/%s", homename, INHOMEDIR);
    mkdir(homedir, 0777);
    len1 = strlen(homedir);
  }

  len2 = strlen(datadir);

  if(len2 > len1) len1 = len2;
  len1 += 160;
  result_str = (char *)mymalloc(len1*sizeof(char));
  res_str_len = len1-1;
}

void level_savedir_init() // nastavi savedir, pripravi k nemu cestu
{
  if(savedir){
    free(savedir);
    savedir = NULL;
  }
  if(!homedir || !room_codename) return;
  asprintf(&savedir, "%s/saves/%s/", homedir, room_codename);
  sprintf(result_str, "%s/saves", homedir);
  mkdir(result_str, 0777);
  mkdir(savedir, 0777);
}
