#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/timeb.h>

#include<stdio.h>
#include<string.h>
#include<cairo/cairo.h>

#include "script.h"
#include "X.h"
#include "window.h"
#include "draw.h"
#include "object.h"
#include "rules.h"
#include "moves.h"
#include "main.h"
#include "gmoves.h"
#include "keyboard.h"
#include "menuloop.h"
#include "menudraw.h"
#include "menuscript.h"
#include "loop.h"
#include "gsaves.h"

static int sec = 1;
static int usec = 0;

char focus = 0;

char mouse_pressed = 0;
int mouse_x = 0;
int mouse_y = 0;
static char noskip;

void setmouse()
{
  XWarpPointer(display, None, win, 0, 0, 0, 0, mouse_x, mouse_y);
}

static void events()
{
  XEvent xev;

  for(;;){
    if(!XEventsQueued(display, QueuedAfterReading)){
      if(!menumode && (focus || room_state != ROOM_IDLE)) break;

      if(menumode) menu_draw();
      else{
	unanim_fish_rectangle();
	gsaves_unanim();
	keyboard_erase_queue();
	draw();
      }
      noskip = 1;
    }
    XNextEvent(display, &xev);

    if(xev.type == ButtonPress || xev.type == ButtonRelease ||
       xev.type == MotionNotify || xev.type == EnterNotify ||
       xev.type == LeaveNotify){
      mouse_x = xev.xmotion.x;
      mouse_y = xev.xmotion.y;
      if(slider_hold) moveslider(&xev.xmotion);
      if(menumode) menu_pointer();
      else gsaves_pointer(0, xev.type == LeaveNotify);
    }

    switch (xev.type){
    case ClientMessage:
      if(xev.xclient.data.l[0] == wmDeleteMessage)
	for(;;) escape();
      break;
    case ConfigureNotify:
      win_width = xev.xconfigure.width;
      win_height = xev.xconfigure.height;
      free_win_cr();
      create_win_cr();
      win_resize = 1;
      if(!menumode) gsaves_unanim();
      calculatewin();
      break;
    case KeyPress:
    case KeyRelease:
      key_event(xev.xkey);
      break;
    case ButtonPress:
      if(xev.xbutton.button == 1) mouse_pressed = 1;
      if(!menumode){
	gmoves_click(&xev.xbutton);
	if(xev.xbutton.button == 1) gsaves_click();
      }
      else if(xev.xbutton.button == 1) menu_click();
      break;
    case ButtonRelease:
      if(xev.xbutton.button == 1) mouse_pressed = 0;
      if(!menumode){
	gmoves_unclick(&xev.xbutton);
	if(xev.xbutton.button == 1) gsaves_unclick();
      }
      else if(xev.xbutton.button == 1) menu_unclick();
      break;
    case FocusOut: focus = 0; break;
    case FocusIn:
      focus = 1;
      key_remap();
      break;
    }
  }
}

static void fixtv(struct timeval *tv)
{
  while(tv->tv_usec < 0){
    tv->tv_usec += 1000000;
    tv->tv_sec--;
  }
  while(tv->tv_usec >= 1000000){
    tv->tv_usec -= 1000000;
    tv->tv_sec++;
  }
}

#define MAXSKIP 20

void loop ()
/**********/
{
  struct timeval wait, curtime, nexttime;
  int skipnum;

  gettimeofday(&nexttime, NULL);

  noskip = 0;
  skipnum = 0;
  for(;;){
    if(noskip || skipnum == MAXSKIP) gettimeofday(&nexttime, NULL);

    if(menumode) menu_draw();
    else if(wait.tv_sec >= 0 || noskip || skipnum == MAXSKIP){
      skipnum = 0;
      draw();
    }
    else{
      skipnum++;
      printf("skip\n");
    }

    gettimeofday(&curtime, NULL);

    nexttime.tv_sec += sec;
    nexttime.tv_usec += usec;
    fixtv(&nexttime);

    wait.tv_sec = nexttime.tv_sec - curtime.tv_sec;
    wait.tv_usec = nexttime.tv_usec - curtime.tv_usec;
    fixtv(&wait);

    if(wait.tv_sec >= 0 && !menumode) select(0, NULL, NULL, NULL, &wait); // wait

    noskip = 0;
    events();
    gsaves_anim();
    keyboard_step();
    anim_step();
    if(room_sol_esc){
      gsaves_save();
      menu_solved_node(active_menu_node);
      escape();
    }
  }
}

void setdelay (int seconds, int mikro)
/***********************************/
{
  long int time;

  time = mikro + seconds*1000000;
  if(time > 0){
    sec = time/1000000;
    usec = time%1000000;
  }
}
