/* Allegro port Copyright (C) 2014 Gavin Smith. */

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define MENUX           142     // X-offset for first menupoint
#define MENUY           58      // Y-offset for first menupoint
#define MSTEP           3       // How far to move at once
#define MENUDELTA       9/MSTEP // 1/MSTEP distance between menupoints
#define MENXTXT		160	// X-offset for menutext sprite
#define MENYTXT		46	// Y-offset for menutext sprite
#define MENYTEXT	60	// Y-offset for menutext
#define MENXTEXT	159	// X-offset for menutext
#define MENYERR         99

#define ENDLOGOX		20
#define ENDLOGOY		110

#define CREDITDELAY		100
#define CREDITSPEED		2
#define STORYSPEED		2
#define BACKDELAY		502
#define TITLEDELAY		460

#include "xmode.h"
#include "sound.h"
#include "fileman.h"
#include "baller.h"

void loadconfig(void)
{
   int filvar;
   int sound;

   char *fullpath = alloca (strlen (LOCALSTATEDIR) + 1
                            + strlen ("CONFIG.HIG") + 1);
   sprintf (fullpath, "%s/%s", LOCALSTATEDIR,  "CONFIG.HIG");

   /* Look for installed high score file; failing that, look in current
      directory. */
   if ((filvar = open(fullpath, O_RDONLY | O_BINARY, S_IREAD)) == -1
       && (filvar = open("CONFIG.HIG", O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      key_up = KEY_UP;
      key_down = KEY_DOWN;
      key_left = KEY_LEFT;
      key_right = KEY_RIGHT;
      key_fire = KEY_SPACE;
      key_pause = KEY_P;
      return;
   }

   /* FIXME: Don't assume that a short int is 2 bytes and little-endian. */
   read(filvar, &key_up, sizeof(short));
   read(filvar, &key_down, sizeof(short));
   read(filvar, &key_left, sizeof(short));
   read(filvar, &key_right, sizeof(short));
   read(filvar, &key_fire, sizeof(short));
   read(filvar, &key_pause, sizeof(short));
   read(filvar, &sound, sizeof(short));
   speaker(sound);
   loadhighscore(&filvar);
   close(filvar);
}

void saveconfig(void)
{
   int filvar;
   int sound;

   if ((filvar = open(LOCALSTATEDIR "/CONFIG.HIG",
                      O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE)) == -1
       && (filvar = open("CONFIG.HIG",
                      O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE)) == -1)
     return;

   write(filvar, &key_up, sizeof(short));
   write(filvar, &key_down, sizeof(short));
   write(filvar, &key_left, sizeof(short));
   write(filvar, &key_right, sizeof(short));
   write(filvar, &key_fire, sizeof(short));
   write(filvar, &key_pause, sizeof(short));
   sound = speaker(-1);
   write(filvar, &sound, sizeof(short));
   savehighscore(&filvar);
   close(filvar);
}

static void closingtime(void)
{
   char *logo_file;
   struct sprstrc *logo;
   char *snd_data;
   struct sndstrc snd;
   SAMPLE *snd_handle;

   while (keypressed()) readkey();
   clearscreen();
   logo_file = loadfile(datapool, "CLOSE.SPR");
   create_sprstrc (&logo, logo_file);
   snd_data  = loadfile(datapool, "CLOSE.SND");
   create_sndstrc (&snd, snd_data);
   snd_handle = create_SAMPLE (&snd);
   playsample(snd_handle);
   putspritedirect(logo, ENDLOGOX, ENDLOGOY, 0);
   glowin(0);

   //while(!pressedkeys && soundbusy());
   clear_keybuf();
   while (!keypressed());

   glowout();
   haltsound();
   free(snd_handle);
   unloadfile((void *) snd_data);
   unloadfile((void *) logo_file); free(logo);
   clearscreen();
   setstandardpalette();
}

static void credits(void)
{
   void  *ptr;
   int   d, line, i;

   ptr = loadfile(datapool, "CREDIT.PCX");
   glowout();
   setpage(0);
   clearscreen();
   showpcx256(ptr, 0);
   //unloadfile(ptr);
   showpage(0);
   glowin(1);

   d = CREDITSPEED;			// Direction and speed of movement.
   line = 0;
   i = 0;
   if(!waitdelayedkey(CREDITDELAY)){
      do {
         waitfortick();
         showpcx256(ptr,-line);
         showpage(page);
	 line += d;
	 if (line < 0) {
	    i++;
	    d = -d; line += 2*d;
	    if (waitdelayedkey(CREDITDELAY)) break;
	 }
	 // 204 ist die linie, bei der das scrolling umgekehrt wird.
	 // Es ist ein proebli wert, der sehr stark vom kunstlerischen
	 // koennen des grafikers abhaengt.
	 if (line > 204) {
	    d = -d; line += 2*d;
	    if (waitdelayedkey(CREDITDELAY)) break;
	 }
      } while (i!=2 && !keypressed());
   }
}


void showtitle(void)
{
   void  *ptr;
   int filvar;
   void *buffer;

   struct sndstrc snd;
   SAMPLE *s;

   setpage(0);
   setvanillapalette(0);
   clearscreen();
   ptr = loadfile(datapool, "BACK.PCX");
   showpcx256(ptr, 0);
   unloadfile(ptr);

   ptr = loadfile(datapool, "TITLE.PCX");
   setpage(1);
   showpcx256(ptr, 10);
   unloadfile(ptr);

   ptr = loadfile(datapool, "TITLE.SND");
   create_sndstrc (&snd, ptr);
   s = create_SAMPLE (&snd);
   playsample(s);

   setpage(0);
   glowin(0);
   waitdelayedkey(BACKDELAY);
   glowout();

   setpage(1);
   glowin(0);

   if (!waitdelayedkey(TITLEDELAY)) {
      credits();
   }
   glowout();

   stop_sample (s);
   unloadfile(ptr);
   //closefile(filvar);
   //free(buffer);
}

static int initmenu(SAMPLE **s, int *pobj, int *pspr, struct sprstrc **mspr)
{
   char  *ptr;
   struct sprstrc *sps;
   struct sndstrc ss;

// Load Menu, show and unload it

   clearscreen();
   setpage(0);
   ptr = loadfile(datapool, "MENU.PCX");
   showpcx256(ptr, 10);
   copypage(0,1);
   unloadfile(ptr);

// Load Pointer and place it

   ptr = loadfile(datapool, "MENCUR.SPR");
   create_sprstrc (&sps, ptr);
   *pspr = defsprite(sps, 2);
   unloadfile(ptr);
   *pobj = defobject(*pspr, MENUX, MENUY, OBJ_LOW);

// Load Menutext sprite and put it

   ptr = loadfile(datapool, "MENTXT.SPR");
   create_sprstrc (mspr, ptr);
   putspritedirect(*mspr, MENXTXT, MENYTXT, 0);

// Load Sound, play it and display all

   ptr = loadfile(datapool, "MENU.SND");
   create_sndstrc (&ss, ptr);
   *s = create_SAMPLE (&ss);
   playsample (*s);

   glowin(0);
   waitfortick();
   updatescreen();

   return 1;  // first menupoint is selected
}

static void endmenu(SAMPLE *snd, int pobj, int pspr, struct sprstrc *mspr)
{
   glowout();
   killobject(pobj);
   killsprite(pspr);
   unloadfile(mspr);
   stop_sample(snd);
   free(snd);
   haltsound();
}

static int newkey(int y, const char *s, struct sprstrc *f1)
{
  unsigned short int k, ks, ka;

  /* Empty key press buffer, but slowly. This is not ideal. */
  while(keypressed()) readkey();

  writetext(MENXTXT,y,s,f1);
  showpage(page);

  k = readkey();
  ks = (0xFF00 & k) >> 8; /* Key scan code. */
  ka = 0x00FF & k; /* Key ASCII value */

  if (ka >= 0x21) {
    if (ka >= 0x60)
      ka &= 0x5F; /* Convert to upper case */
    putspritedirect(f1, MENXTXT+40, y, ka - 0x20);
  } else {
    char *n; /* Key name */

    switch (ks) {
    case KEY_ESC: 
      n = "ESC"; break;
    case KEY_SPACE: 
      n = "SPACE"; break;
    case KEY_CAPSLOCK: 
      n = "CPSLCK"; break;
    case KEY_BACKSPACE: 
      n = "BKSP"; break;
    case KEY_TAB: 
      n = "TAB"; break;
    case KEY_NUMLOCK: 
      n = "NUMLCK"; break;
    case KEY_SCRLOCK: 
      n = "SCRLCK"; break;
    case KEY_ENTER:
      n = "ENTER"; break;
    case KEY_LCONTROL:
    case KEY_RCONTROL:
      n = "CTRL"; break;
    case KEY_UP:
      n = "UP"; break;
    case KEY_PGUP:
      n = "PGUP"; break;
    case KEY_LSHIFT:
      n = "LSHFT"; break;
    case KEY_RSHIFT:
      n = "RSHFT"; break;
    case KEY_ASTERISK:
      n = "[*]"; break;
    case KEY_ALT:
      n = "ALT"; break;
    case KEY_LEFT:
      n = "LEFT"; break;
    case KEY_5_PAD:
      n = "[5]"; break;
    case KEY_RIGHT:
      n = "RIGHT"; break;
    case KEY_END:
      n = "END"; break;
    case KEY_DOWN:
      n = "DOWN"; break;
    case KEY_PGDN:
      n = "PGDN"; break;
    case KEY_INSERT:
      n = "INSERT"; break;
    case KEY_DEL:
      n = "DELETE"; break;
    case KEY_MINUS_PAD:
      n = "[-]"; break;
    case KEY_PLUS_PAD:
      n = "[+]"; break;
    case KEY_F1:
      n = "F1"; break;
    case KEY_F2:
      n = "F2"; break;
    case KEY_F3:
      n = "F3"; break;
    case KEY_F4:
      n = "F4"; break;
    case KEY_F5:
      n = "F5"; break;
    case KEY_F6:
      n = "F6"; break;
    case KEY_F7:
      n = "F7"; break;
    case KEY_F8:
      n = "F1"; break;
    case KEY_F9:
      n = "F9"; break;
    case KEY_F10:
      n = "F10"; break;
    case KEY_F11:
      n = "F11"; break;
    case KEY_F12:
      n = "F12"; break;
    default:
      n = "";
    }
    writetext(MENXTXT+40,y,n,f1);
  }
  return ks;
}

static void defkeys(struct sprstrc *font1)
{

// Left

   key_left = newkey(MENYTEXT-7, "LEFT", font1);

// Right

   key_right = newkey(MENYTEXT+3, "RIGHT", font1);

// Up

   key_up = newkey(MENYTEXT+13, "UP", font1);

// Down

   key_down = newkey(MENYTEXT+23, "DOWN", font1);

// Fire

   key_fire = newkey(MENYTEXT+33, "FIRE", font1);

// Pause

   key_pause = newkey(MENYTEXT+43, "PAUSE", font1);

   showpage(page);

   waitforkey();
   saveconfig();
}

static int showoptions(struct sprstrc *ofont, struct sprstrc *mspr)
{
   putspritedirect(mspr, MENXTXT, MENYTXT, 1);    // Clear mentxt

   writetext(MENXTEXT, MENYTEXT-8, "OPTIONS", ofont);
   if(speaker(-1)) writetext(MENXTEXT, MENYTEXT+3, "1 SOUND ON", ofont);
   else writetext(MENXTEXT, MENYTEXT+3, "1 SOUND OFF", ofont);
   writetext(MENXTEXT, MENYTEXT+MSTEP*MENUDELTA+3, "2 CHOOSE KEYS", ofont);
   writetext(MENXTEXT, MENYTEXT+MSTEP*MENUDELTA*2+3, "3 MAINMENU", ofont);
   return 1;
}

static int options(SAMPLE *snd, int *pobj, int pspr, struct sprstrc *mspr)
{
   char *ofont_data;
   struct sprstrc *ofont;
   int    quit, selected;
   int i;

   //haltsound();

   ofont_data = loadfile(datapool, "OFONT.SPR");
   create_sprstrc (&ofont, ofont_data);

// Clear old menu and write new one

   selected = showoptions(ofont, mspr);
   moveobject(*pobj, MENUX, MENUY+7);

   quit = FALSE;

// The Selection starts

   do{
      while (keypressed()) {
         clear_keybuf();
	 waitfortick();
	 updatescreen();
      }
      while (!keypressed()) {
	 waitfortick();
	 updatescreen();
      }
      if(key[KEY_1] || (selected==1 && key[key_fire])){   // Sound on/off
	 if(speaker(-1)){ 	// Turn Sound off
	    writetext(MENXTEXT, MENYTEXT+3, "1 SOUND OFF", ofont);
	    speaker(0);
	    saveconfig();
	 }else{			// Turn Sound on
	    if(speaker(1)){
	       writetext(MENXTEXT, MENYTEXT+3, "1 SOUND ON ", ofont);
	       saveconfig();
	    }else{
	       writetext(MENXTEXT, MENYERR, "SOUNDBLASTER", ofont);
	       writetext(MENXTEXT, MENYERR+ofont->ys+1, " NOT FOUND", ofont);
	    }
	 }
      }
      if(key[KEY_2] || (selected==2 && key[key_fire])){   // Redefine Keys
	 killobject(*pobj);
	 putspritedirect(mspr, MENXTXT, MENYTXT, 1); 	   // Clear mentxt
	 defkeys(ofont);
	 selected = showoptions(ofont, mspr);
         showpage(page);
	 *pobj = defobject(pspr, MENUX, MENUY+7, OBJ_LOW);
      }
      if(key[KEY_3] || (selected==3 && key[key_fire])){ // Quit Options
	 quit = TRUE;
      }
      if(key[key_up] && selected>1){                       // Up
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(*pobj, 0, -MSTEP);
	    waitfortick();
	    updatescreen();
	 }
	 selected--;
      }
      if(key[key_down] && selected<3){                     // Down
	 if(selected == 1){
	    writetext(MENXTEXT, MENYERR, "            ", ofont);
	    writetext(MENXTEXT, MENYERR+ofont->ys+1, "          ", ofont);
	 }
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(*pobj, 0, MSTEP);
	    waitfortick();
	    updatescreen();
	 }
	 selected++;
      }
   }while(!quit);

// Restore mainmenu

   play_sample(snd, 255, 128, 1000, 0);
   putspritedirect(mspr, MENXTXT, MENYTXT, 0);
   moveobject(*pobj, MENUX, MENUY);
   waitfortick();
   updatescreen();

   unloadfile(ofont);

   return 1;  // first menupoint is selected
}

static void story(void)
{
   void *ptr;
   void *sound;
   int d, line;
   SAMPLE *s;
   struct sndstrc ss;

   clearscreen();
   setpage(0);
   ptr = loadfile(datapool, "LONGTIME.PCX");
   showpcx256(ptr,0);

   sound = loadfile(datapool, "LONGTIME.SND");
   create_sndstrc (&ss, sound);
   s = create_SAMPLE (&ss);
   playloop(s);

   glowin(0);

   d = CREDITSPEED;			// Direction and speed of movement.
   line = 0; 				// Starting at line 0.
   waitforkey();
   
   do {
      //blit (full_pages[0], screen, 0, line, 0, 0, screen->w, screen->h);
      showpcx256(ptr,-line);
      showpage(0);
      /* This slows it down but makes sure it proceeds at a steady pace. */
      waitfortick();
      line += d;
   } while (line < 198);

   waitforkey();

   // Stop sound and free memory
   glowout();
   stop_sample (s);
   free (s);
   unloadfile(sound);
   unloadfile(ptr);
}

 //----------------//
//      Menu	    //
 //----------------//

void menu(void)
{
   int    quit, selected, i;
   int    pointobj, pointspr;
   struct sprstrc *menuspr;
   SAMPLE *snd;
   int    c;

   showtitle();

   quit = FALSE;
   selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
   c = 0;
   do{
      clear_keybuf();
      while (!keypressed()) {
	 c = cyclepalette(232, 254, c);
	 waitfortick();
	 updatescreen();
      }
      if(key[KEY_1] || (selected==1 && key[key_fire])){   // Play
	 endmenu(snd, pointobj, pointspr, menuspr);
	 clearscreen();
	 setstandardpalette();
	 playthegame();
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEY_2] || (selected==2 && key[key_fire])){   // Show Highscore
	 endmenu(snd, pointobj, pointspr, menuspr);
	 highscore(FALSE);
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEY_3] || (selected==3 && key[key_fire])){ // Show Title
	 endmenu(snd, pointobj, pointspr, menuspr);
	 showtitle();
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEY_4] || (selected==4 && key[key_fire])){  // Story
	 endmenu(snd, pointobj, pointspr, menuspr);
	 story();
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEY_5] || (selected==5 && key[key_fire])){  // Options
	 selected = options(snd, &pointobj, pointspr, menuspr);
      }
      if(key[KEY_6] || (selected==6 && key[key_fire])){   // Quit
	 endmenu(snd, pointobj, pointspr, menuspr);
	 closingtime();
	 quit = TRUE;
      }
      if(key[key_up] && selected>1){                       // Up
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(pointobj, 0, -MSTEP);
	    c = cyclepalette(232, 254, c);
	    waitfortick();
	    updatescreen();
	 }
	 selected--;
      }
      if(key[key_down] && selected<6){                     // Down
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(pointobj, 0, MSTEP);
	    c = cyclepalette(232, 254, c);
	    waitfortick();
	    updatescreen();
	 }
	 selected++;
      }
   }while(!quit);
}



