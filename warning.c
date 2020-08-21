#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "warning.h"
#include "main.h"

void warning(char *s, ...)
  /* vypise varovani na stderr, parametry podobne jako printf, jen
     neni treba psat '\n' pro ukonceni radku */
{
  va_list vl;

  fprintf(stderr, "%s warning: ", gargv[0]);
  va_start(vl, s);
  vfprintf(stderr, s, vl);
  va_end(vl);
  fprintf(stderr, "\n");
}

void error(char *s, ...)
  /* podobne jako warning, ale ukonci program a
     na zacatek radku misto "warning:" napise "error:" */
{
  va_list vl;

  fprintf(stderr, "%s error: ", gargv[0]);
  va_start(vl, s);
  vfprintf(stderr, s, vl);
  va_end(vl);
  fprintf(stderr, "\n");

  exit(-1);
}

void *mymalloc(unsigned int size)
  /*  pouzije malloc, selze-li, skonci na error: memory allocation failed */
{
  void *result;

  if(!(result = malloc(size))) error("Allocation of size %d failed", size);
  return result;
}
