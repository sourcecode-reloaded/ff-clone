void warning(char *s, ...);
  /* vypise varovani na stderr, parametry podobne jako printf, jen
     neni treba psat '\n' pro ukonceni radku */
void error(char *s, ...);
  /* podobne jako warning, ale ukonci program a
     na zacatek radku misto "warning:" napise "error:" */
void *mymalloc(unsigned int size);
  /*  pouzije malloc, selze-li, skonci na error: memory allocation failed */
