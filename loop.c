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
#include "menuevents.h"
#include "menudraw.h"
#include "menuscript.h"
#include "loop.h"
#include "gsaves.h"

static int sec = 1; // pocet sekund mezi jednotlivymi framy
static int usec = 0; // pocet mikrosekund mezi jednotlivymi framy

char focus = 0;

char mouse_pressed = 0;
int mouse_x = 0;
int mouse_y = 0;

void setmouse() // nastavi souradnice mysi na mouse_x, mouse_y
{
  XWarpPointer(display, None, win, 0, 0, 0, 0, mouse_x, mouse_y);
}

static char noskip; // viz loop()

static void events() /* projde nahromadene udalosti, ve specialnich pripadech dokonce ceka,
			az nejaka prijde */
{
  XEvent xev;

  for(;;){
    if(!XEventsQueued(display, QueuedAfterReading)){ // uz zadna udalost nezbyva
      // v normalni hre se vraci do loop()
      if(!menumode && (focus || room_state != ROOM_IDLE)) break;

      if(menumode) menu_draw(); // ale ne v menu

      else{ // nebo, kdyz nema fokus
	unanim_fish_rectangle();
	gsaves_unanim();
	keyboard_erase_queue();
	draw();
      }

      noskip = 1;
    }
    XNextEvent(display, &xev); // ziskam dalsi udalost
    if(xev.xany.window != real_win) continue; // zajima me jen hlavni okno

    if(xev.type == ButtonPress || xev.type == ButtonRelease ||
       xev.type == MotionNotify || xev.type == EnterNotify ||
       xev.type == LeaveNotify){ // udalosti, co davaji souradnice mysi
      mouse_x = xev.xmotion.x;
      mouse_y = xev.xmotion.y;
      if(slider_hold) moveslider(&xev.xmotion); // posunuti slideru
      if(menumode) menu_pointer(); // najiti tlacitka pod mysi na mape
      else gsaves_pointer(0, xev.type == LeaveNotify); // posunuti mac menu
    }

    switch (xev.type){
    case ClientMessage: // zavreni krizkem
      if(xev.xclient.data.l[0] == wmDeleteMessage)
	for(;;) escape();
      break;
    case ConfigureNotify: // zmena velikosti okna
      if(xev.xconfigure.send_event) break;
      if(win_width == xev.xconfigure.width &&
	 win_height == xev.xconfigure.height) break; // falesny poplach, velikost je porad stejna
      win_width = xev.xconfigure.width;
      win_height = xev.xconfigure.height;
      free_win_cr(); // zmena kreslici plochy
      create_win_cr();
      win_resize = 1;
      calculatewin(); // prepocitani rozmisteni prvku v okne
      break;
    case KeyPress: // zmacknuti klavesy
      key_press(xev.xkey);
      break;
    case KeyRelease: // pusteni klavesy
      key_release(xev.xkey);
      break;
    case ButtonPress: // zmacknuti tlacitka mysi
      if(!menumode) gmoves_click(&xev.xbutton); // undo pasecek reaguje i na jina tlacitka nez leve

      if(xev.xbutton.button != 1) break; // nic jineho ne
      mouse_pressed = 1;
      if(menumode) menu_click();
      else gsaves_click();
      break;
    case ButtonRelease: // pusteni tlacitka mysi
      if(!menumode) gmoves_unclick(&xev.xbutton);

      if(xev.xbutton.button != 1) break;
      mouse_pressed = 0;
      if(menumode) menu_unclick();
      else gsaves_unclick();
      break;
    case FocusOut: focus = 0; break; // prijiti o fokus -> zastaveni hry
    case FocusIn: // ziskani fokusu
      focus = 1;
      key_remap(); // prozkoumam drzene klavesy
      break;
    }
  }
}

// upravi timeval do korektniho fotrmatu, tedy tv->tv_usec bude v intervalu [0, 1000000)
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

/*
  Nasledujici funkce resi hlavni cyklus hry:
   1) vykresli okno
   2) ceka sec sekund a usec mikrosekund
   3) zpracuje udalosti
   4) provede animace
  Pokud by se nemelo cekat na timer, ale misto toho cekat jen na udalosti
  (v menu nebo kdyz nema fokus), tak se hra misto v loop() se toci v events().

  Snazim se, aby cekani melo opravdu spravnou dobu, tedy kompenzuji i dobu
  vykreslovani. Pokud by kvuli kompenzaci bylo treba cekat zapornou dobu,
  je cekani preskoceno a vcetne nej i vykreslovani, ktere je vypocetne
  nejnarocnejsi.

  Muze se stat, ze je nejaka operace natolik narocna, ze abych kompenzoval jeji
  cas, budu muset preskocit cekani a vykreslovani vicekrat. Toto preskakovano je\
  vsak omezeno dvema faktory: 1) nebudu preskakovat vice nez MAXSKIP framu,
  2) Napriklad cekani vzesleho z toho, ze okno nemelo focus a hra tak byla
  "pausnuta" je zbytecne kompenzovat. V takovych pripadech je nastavena promenna
  noskip na true, coz loop() zaregistruje a nepreskakuje.
 */
#define MAXSKIP 20

void loop()
{
  struct timeval wait; // doba cekani
  struct timeval nexttime; // pristi casovy okamzik, do ktereho se chci prenest
  struct timeval curtime;
  int skipnum; // pocet preskoceni

  noskip = 1;
  skipnum = 0;
  for(;;){
    if(skipnum == MAXSKIP) noskip = 1;

    if(noskip) gettimeofday(&nexttime, NULL);

    if(menumode) menu_draw();
    else if(wait.tv_sec >= 0 || noskip){
      skipnum = 0;
      draw();
    }
    else skipnum++;

    nexttime.tv_sec += sec; // posunu casovy okamzik, do ktereho jsem se prenesl minule
    nexttime.tv_usec += usec; // dobu mezi framy
    fixtv(&nexttime);

    gettimeofday(&curtime, NULL);
    wait.tv_sec = nexttime.tv_sec - curtime.tv_sec; // spocitam delku cekani
    wait.tv_usec = nexttime.tv_usec - curtime.tv_usec;
    fixtv(&wait);

    if(wait.tv_sec >= 0 && !menumode) select(0, NULL, NULL, NULL, &wait); // samotne cekani

    noskip = 0;
    events(); // zpracovani udalosti
    gsaves_anim();
    keyboard_step();
    anim_step();
    if(room_sol_esc){ // byla vyresena mistnost -> navrat do menu
      gsaves_save();
      menu_solved_node(active_menu_node, moves);
      escape();
    }
  }
}

void loop_noskip() // aby mohly noskip nastavit i jine moduly
{
  noskip = 1;
}

void setdelay (int seconds, int mikro) // nastavi cas mezi framy
{
  long int time;

  time = mikro + seconds*1000000;
  if(time > 0){
    sec = time/1000000;
    usec = time%1000000;
  }
}
