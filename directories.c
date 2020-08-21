#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/stat.h>

#include "warning.h"
#include "directories.h"
#include "script.h"
#include "levelscript.h"

#define INHOMEDIR "/.ff-clone/"

#ifdef DATADIR
char *datadir = DATADIR;
#else
char *datadir = "./data";
#endif

int asprintf(char **strp, const char *fmt, ...);

char *savedir = NULL;

static char *result_str;
static int res_str_len;

char *datafile(char *filename)
{
  if(res_str_len < strlen(datadir) + 1 + strlen(filename)){
    error("Too long file path %d > %d: %s/%s",
	  strlen(datadir) + 1 + strlen(filename), res_str_len,
	  datadir, filename);
  }
  sprintf(result_str, "%s/%s", datadir, filename);
  return result_str;
}

char *homefile(char *filename)
{
  if(res_str_len < strlen(homedir) + strlen(filename)){
    error("Too long file path %d > %d: %s%s",
	  strlen(homedir) + strlen(filename), res_str_len,
	  homedir, filename);
  }
  sprintf(result_str, "%s%s", homedir, filename);
  return result_str;
}

char *savefile(char *filename, char *ext)
{
  if(res_str_len < strlen(savedir) + strlen(filename) + 1 + strlen(ext)){
    error("Too long file path %d > %d: %s%s.%s",
	  strlen(savedir) + strlen(filename) + 1 + strlen(ext),
	  res_str_len, savedir, filename, ext);
  }
  sprintf(result_str, "%s%s.%s", savedir, filename, ext);
  return result_str;
}

void init_directories()
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
  result_str = (char *)malloc(len1*sizeof(char));
  res_str_len = len1-1;
}

void level_savedir_init()
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
