#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "warning.h"

void warning (char *s, ...)
{
  va_list vl;

  fprintf(stderr, "%s warning: ", gargv[0]);
  va_start(vl, s);
  vfprintf(stderr, s, vl);
  va_end(vl);
  fprintf(stderr, "\n");
}

void error (char *s, ...)
{
  va_list vl;

  fprintf(stderr, "%s error: ", gargv[0]);
  va_start(vl, s);
  vfprintf(stderr, s, vl);
  va_end(vl);
  fprintf(stderr, "\n");

  exit(-1);
}

void *mymalloc (unsigned int size)
{
  void *vysledek;

  if(!(vysledek = malloc(size))) error("Allocation of size %d failed", size);
  return vysledek;
}
