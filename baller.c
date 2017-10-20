/* Allegro port Copyright (C) 2014 Gavin Smith. */
 /*-----------------------------------------------------*/
/*                                                       */
/*            T H E   L A S T   E I C H H O F            */
/*                                                       */
/*           [c] copyrigth 1993 by ALPHA-HELIX           */
/*          This module written by Dany Schoch		 */
/*                                                       */
 /*-----------------------------------------------------*/

#define MAIN_MODULE

#include <allegro.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include "xmode.h"
#include "fileman.h"
#include "sound.h"
#include "baller.h"


#define MEMORYREQUIRED		500000L	// Memory used to run.
#define CMDLEN			40	// Max length of command line.

static char   cmd[CMDLEN];		// Command line given at startup.
extern unsigned _stklen;                // Programs stacklength.


// Code.

// Error handling routines.

void error(char *text, int code, ...)
{
   void powerdown(void);	// Prototype.
   va_list   ap;		// Variable for variable argument list.

   va_start(ap, code);
   powerdown();			// Leave graphics, close files, ...
   switch (code) {
      case ENOMEM : printf("%s : NO MORE MEMORY !\n\n", text);
		    break;
      case ENOENT: printf("%s : File '%s' not found !\n\n",
		    text, va_arg(ap, char *));
		    break;
      case E2BIG  : printf("%s : Out of index table space.\n\n", text);
		    break;
      case EINVAL: printf("%s : Wrong level.\n\n", text);
		    break;
      case -1     :
      case EFAULT : printf("James Bond quitting style !!\n\n");
		    break;
      default     : perror(text);
   }
   va_end(ap);
   exit(1);
}


/*------------------------------------------------------
Function: powerup & powerdown

powerup   : Initializes the system.
powerdown : Undoes the effect of powerup.
	    Please note that powerdown should not be
	    called within powerup or the sytem will
	    hang. So errors encountered in powerup
	    can't be handled by the function 'error',
	    because this would call powerdown indirectly.
------------------------------------------------------*/
static void create_pages(void);

void powerup(void)
{
   allegro_init();

   install_keyboard();

   if (install_sound(DIGI_AUTODETECT, MIDI_NONE, 0)) {
     printf ("%s\n", allegro_error);
     exit(2);
   }

// Clear NUM_LOCK, SCROLL_LOCK and CAPS_LOCK.
   set_leds (0);

   install_timer();
   setspeed(GAMESPEED);

   initxmode();				// Enter graphic mode.
   windowy1 = BARY;			// Game window y-Size.
   create_pages ();
   page = 0;

}

static void create_pages (void)
{
  full_pages[0] = create_system_bitmap (320, 240);
  full_pages[1] = create_system_bitmap (320, 240);
  
  pages[0] = create_sub_bitmap(full_pages[0], 0, 0, windowx1, windowy1);
  pages[1] = create_sub_bitmap(full_pages[1], 0, 0, windowx1, windowy1);
}

void powerdown(void)
{
   shutxmode();

   shutsound();
   shutfilemanager();
}



/*------------------------------------------------------
Function: cmdline

Description: Do first steps on the command line.
------------------------------------------------------*/
void cmdline(int argc, char *argv[])
{
   int   i;
   char  *e1;

// Concatenate all command strings together in 'cmd'.
   cmd[0] = '\0';			// Clear string.
   for (i = 1; i < argc; i++) {
      if (strlen(cmd)+strlen(argv[i]) < CMDLEN)
	 strcat(cmd, argv[i]);
   }
   strupr(cmd);

// Help?
   if (strstr(cmd, "/?") || strstr(cmd, "-?")) {	// Help?
      printf("Syntax:   BALLER [options]\n");
      printf("  /vga    Override VGA detection.\n");
      printf("  /ns     Play without sound.\n");
      printf("\n");
      printf("To force SoundBlaster on, use the BLASTER environment variable.\n");
      printf("  e.g. set BLASTER = A220 I7 D1\n");
      printf("\n\n");
      exit(0);                          // Exit nicely.
   }

// Get cheat level.

   if ((e1 = strstr(cmd, "007.")) == NULL) {
      cheatlevel = 0;			// No cheating this time.
   } else {
      cheatlevel = e1[4] - '0';
      printf("Cheat level:\n");
      if(cheatlevel & CHEATLIFES) printf("    - Unlimited lives\n");
      if(cheatlevel & CHEATMONEY) printf("    - Unlimited money\n");
      if(cheatlevel & CHEATCRASH) printf("    - Eichhof can't be destroyed\n");
      printf("\n");
   }

}

int main(int argc, char *argv[])
{
#if 0
   printf("\n\n\nTHE LAST EICHHOF [c] copyright 1993 by ALPHA-HELIX.\n");
   printf("Release 1.1\n\n");
   printf("This game is FREEWARE. Please copy like crazy.\n");

   /* Don't know if this is still valid. */
   printf("If you like it, just send a postcard to ALPHA-HELIX\n");
   printf("                                        Rehhalde 18\n");
   printf("                                        6332 Hagendorn\n");
   printf("                                        Switzerland\n");
   printf("\nInternet contact address: tritone@ezinfo.vmsmail.ethz.ch\n\n\n");
#endif

// Process command line.
   cmdline(argc, argv);

// Do initialization.
   powerup();

// Load options of last time.
   loadconfig();
// Check command line for 'nosound' option.
   if (strstr(cmd, "/NS") || strstr(cmd, "-NS")) speaker(0);

// Open date bases.
   initfilemanager(40, 512, 8192, error);
   datapool = opendatabase("BEER.DAT");

   intro();				// Show Blick intro.


   menu();

   closedatabase(datapool);

// Going down and return to OS.
   powerdown();
   return 0;
}
