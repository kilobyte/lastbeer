/* Allegro port Copyright (C) 2014 Gavin Smith. */

// module 'SUPPORT.C'
// [c] copyright by ALPHA-HELIX 1993

#include <allegro.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "xmode.h"
#include "baller.h"
#include "sound.h"

/*------------------------------------------------------
Function: setspeed

Description: Sets the speed of the timer.
------------------------------------------------------*/
void setspeed(unsigned speed)
{
   install_int_ex (settick, BPS_TO_TIMER(speed * RESOLUTION));
   LOCK_VARIABLE (tick);
   LOCK_VARIABLE (_subtick);
   LOCK_VARIABLE (mstthsbevalidp);
   LOCK_VARIABLE (updates_due);
   LOCK_FUNCTION (settick);
}



void waitforkey(void)
{
   while (keypressed()) clear_keybuf();
   while (!keypressed());
}

int waitdelayedkey(int count)
{
   while (keypressed()) clear_keybuf();
   while (!keypressed() && --count) waitfortick();
   return (count) ? 1 : 0;
}


void writetext(int x, int y, const char *s, struct sprstrc *font)
{
   int   i;
   int   xs;

   xs = font->xs;
   i = 0;
   while (s[i] != 0) {
      putspritedirect(font, x, y, s[i++] - ' ');
      x += xs;
   }

}


void writenumber(int x, int y, long number, struct sprstrc *font)
{
   int j, xs, i = 0;
   int zahl;

   xs = font->xs;
   for(j = 0; j < 2; j++){
      zahl = number % 10;
      number /= 10;
      putspritedirect(font, x, y, 16 + zahl);
      x -= xs;
   }
   putspritedirect(font, x, y, '.'-' ');
   for(; j < 10; j++){
      x -= xs;
      zahl = number % 10;
      number /= 10;
      if(zahl != 0 || j == 2){
	 putspritedirect(font, x, y, 16 + zahl);
	 if(i!=0) for(; i > 0; i--) putspritedirect(font, x+i*xs, y, 16);
	 i = 0;
      }else{
	 i ++;
      }
   }
}

void killallbuddies(void)
{
   int   i;

   for (i = 0; i < MAXARMS; i++) _arm[i].object = -1;
   for (i = 0; i < MAXFOES; i++) _foe[i].object = -1;
   for (i = 0; i < MAXSHOTS; i++) _shot[i].object = -1;
   for (i = 0; i < MAXEXPLS; i++) _expl[i].object = -1;

}

