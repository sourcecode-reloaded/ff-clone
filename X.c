#include<stdio.h>
#include<stdlib.h>
#include "warning.h"
#include "X.h"

//: initX

void initX ()
/*******************************/
{
  char *display_name = NULL;

  if ((display=XOpenDisplay(display_name)) == NULL)
    error("cannot connect to X server %s", XDisplayName(display_name));

  screen = DefaultScreen(display);
}
