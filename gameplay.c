/* Allegro port Copyright (C) 2014 Gavin Smith. */

//  [c] copyright 1992, 1993 by ALPHA-HELIX.
//  written by Dany Schoch

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "xmode.h"
#include "fileman.h"
#include "sound.h"
#include "baller.h"


#define STARTLOGOX		68
#define STARTLOGOY		110
#define STARTTEXTY		40
#define WINTEXTY		204

// Where's the bottle at the beginning ?
#define	XSTART			150
#define	YSTART			190


// Variables local to this module.

static struct sprstrc *scorebar;
static struct sprstrc *barfont;

void displifes (void)
{
  int i;
  for (i = 1; i <= 6; i++)
    putspritedirect (barfont, BARLIFEX - i * 4,
               BARSCOREY, BARCLRDIGIT);

  i = lifes - 1;
  if (i >= 5) i = 5;

  for (; i >= 0; i--)
    putspritedirect (barfont, BARLIFEX - i * 4,
               BARSCOREY, BARLIFEDIGIT);
}

/* Variables used to draw the score. */
short scoredigit;      // # of digits already drawn.
short scorepos;        // Current x position.
unsigned long playscore;       // Remaining score.

void dispscore (void)
{
  int digit;

  if (scoredigit == 0) {
    playscore = score; /* Revitalize everything. */
    scorepos = BARSCOREX;
  } else if (scoredigit == 4) {
    scorepos -= 4; /* Jump over point. */
  } else if (scoredigit == 16) {
    scoredigit = 0;
    playscore = score;
    scorepos = BARSCOREX;
  }

  /* First we clear the digit that should be drawn afterwards. */
  putspritedirect (barfont, scorepos, BARSCOREY, BARCLRDIGIT);
 
  /* Now we calculate the number to be drawn. */
  digit = playscore % 10;

  /* Each digit must be drawn twice, on page 0 and 1.
     So we must check whether it have already been drawn once. */

  if (scoredigit % 2) {
    putspritedirect (barfont, scorepos, BARSCOREY, digit);
    playscore -= digit;
    playscore /= 10;
    scorepos -= BARSCORESPC; /* Advance to next digit. */
  } else {
    putspritedirect (barfont, scorepos, BARSCOREY, digit);
  }

  scoredigit++;
}


/*------------------------------------------------------
Function: newgame

Description: Initializes variables, sprites and
	     whatsoever needed for a brand new game.
------------------------------------------------------*/

unsigned long far_ptr_offset (unsigned char *farptr)
{
  long offset = 0;
  offset += farptr[2] + (farptr[3] << 8); /* Segment */
  offset <<= 4;
  offset += farptr[0] + (farptr[1] << 8); /* Offset */

  return offset;
}

static void load_sprite_library (char *sprite_table, int nsprites)
{
  int i;

  for (i = 0; i < nsprites; i++) {
     char *sprstrc_data; 
     struct sprstrc *s;
     unsigned short flags;

     sprstrc_data = sprite_table + far_ptr_offset (sprite_table + i * 6);
     flags = *(unsigned short *) (sprite_table + i * 6 + 4);

     create_sprstrc (&s, sprstrc_data);

     // Define Weaponsprites and store reference number in 'intindex'.
     if ((intindex[intindexptr++] = defsprite(s, flags)) == -1)
        error("newgame1", ENOMEM);
     free (s);
  }
}

static void load_sound_library (char *data, int nsounds)
{
  int i;

  struct sndstrc ss;

  lsndofs = ptrindexptr;

  for (i = 0; i < level.nsounds; i++) {
     create_sndstrc (&ss, data + far_ptr_offset(data + i*4));
     ptrindex[ptrindexptr++] = create_SAMPLE (&ss);
  }
}

static void load_foe_table (char *data, int nfoes)
{
   int i;

   level.foe = malloc (nfoes * sizeof (struct foestrc *));

   for (i = 0; i < nfoes; i++) {
      level.foe[i] = (struct foestrc * )(data + far_ptr_offset (data + i * 4));
   }

   lfoeofs = ptrindexptr;
   for (i = 0; i < level.nfoes; i++) {
      ptrindex[ptrindexptr++] = level.foe[i];
   }
}

static void load_explosions (char *data, int nexpls)
{
  int i;

  char *expl_offset = data;
  level.expl = malloc (sizeof (struct explstrc *) * nexpls);

  for (i = 0; i < nexpls; i++) {
    level.expl[i] = (struct explstrc *) (data + far_ptr_offset (expl_offset));
    expl_offset += 4;
  }

  lexplofs = ptrindexptr;
  for (i = 0; i < nexpls; i++) {
     ptrindex[ptrindexptr++] = level.expl[i];
  }
}

static void newgame(void)
{
  int   i;			// Just another boring index variable.
  unsigned char  *ptr;

  killallbuddies();
  killallsprites();
  killallobjects();
  killstarfield();

  intindexptr = ptrindexptr = 0;	// Reset index table counters.

// Load weapon characteristics.
  ptr = loadfile(datapool, "WEAPONS.WPN");
  weapon.narms = ptr[0] + (ptr[1] << 8);
  weapon.arm = (struct armstrc *)(ptr+2);

// Load shot pathes.
  ptr = loadfile(datapool, "WEAPONS.SHT");
  weapon.nshots = ptr[0] + (ptr[1] << 8);

  weapon.shot = malloc (sizeof (long) * weapon.nshots);
  weapon.shotfile = ptr;
  for (i = 0; i < weapon.nshots; i++) {
    weapon.shot[i] = far_ptr_offset (ptr + 2 + i*4);
  }

// Load weapon sprite library.
  ptr = loadfile(datapool, "WEAPONS.SLI");
  weapon.nsprites = ptr[0] + (ptr[1] << 8);
  load_sprite_library (ptr + 2, weapon.nsprites);

// Sprites are defined and copied to offscreen memory.
// So we needn't waste main memory by keeping a unecessary copy.
  unloadfile(ptr);

#if 0
// Store explosion reference in 'intindex'.
  for (i = 0; i < weapon.nshots; i++) {
     ptrindex[ptrindexptr++] = weapon.shot[i];
  }
#endif

// Score Bar definition.
  char *scorebar_data = loadfile(datapool, "BAR.SPR");
  create_sprstrc (&scorebar, scorebar_data);

  char *barfont_data = loadfile(datapool, "BARFONT.SPR");
  create_sprstrc (&barfont, barfont_data);
  if ((barfonthandle = defsprite(barfont, 2)) == -1)
     error ("newgame2", ENOMEM);
  //unloadfile(barfont);

// Lall variables initializaton.
  score = scoreold = 0;
  money = 0;
  shipspeed = STARTSHIPSPEED;
  lifes = STARLIFES;
  stage = 0;

// Main weapon definition.
  nweapons = 1;				// First we have 1 weapon.
  weaponlst[0].dx = weaponlst[0].dy = 0;       // At origin (0,0).
  weaponlst[0].arm = weapon.arm[0];

/*
// level 1: Weapon definition during level design.
  shipspeed = 6;
  nweapons += 1;
  weaponlst[1].dx = -18; weaponlst[1].dy = 8;
  weaponlst[1].arm = weapon.arm[4];
*/
/*
// level 2: Weapon definition during level design.
  shipspeed = 6;
  nweapons += 2;
  weaponlst[1].dx = -16; weaponlst[1].dy = 6;
  weaponlst[1].arm = weapon.arm[1];
  weaponlst[2].dx = 16; weaponlst[2].dy = 6;
  weaponlst[2].arm = weapon.arm[4];
*/
/*
// level 3: Weapon definition during level design.
  shipspeed = 6;
  nweapons += 4;
  weaponlst[1].dx = -16; weaponlst[1].dy = 6;
  weaponlst[1].arm = weapon.arm[1];
  weaponlst[2].dx = 16; weaponlst[2].dy = 6;
  weaponlst[2].arm = weapon.arm[4];
  weaponlst[3].dx = 30; weaponlst[3].dy = 8;
  weaponlst[3].arm = weapon.arm[8];
  weaponlst[4].dx = 0; weaponlst[4].dy = 32;
  weaponlst[4].arm = weapon.arm[6];
*/
// level 4: Weapon definition during level design.
  /*
  shipspeed = 6;
  nweapons += 6;
  weaponlst[1].dx = -20; weaponlst[1].dy = 8;
  weaponlst[1].arm = weapon.arm[8];
  weaponlst[2].dx = 30; weaponlst[2].dy = 8;
  weaponlst[2].arm = weapon.arm[8];
  weaponlst[3].dx = 0; weaponlst[3].dy = -18;
  weaponlst[3].arm = weapon.arm[5];
  weaponlst[4].dx = 0; weaponlst[4].dy = 32;
  weaponlst[4].arm = weapon.arm[6];
  weaponlst[5].dx = -36; weaponlst[5].dy = 6;
  weaponlst[5].arm = weapon.arm[2];
  weaponlst[6].dx = 16; weaponlst[6].dy = 6;
  weaponlst[6].arm = weapon.arm[1];
  */
}

static void shutnewgame(void)
{
   short   *ptr;

   killallsprites();
   unloadfile(scorebar);
   free (weapon.shot);
   unloadfile (weapon.shotfile);
   ptr = (short *)weapon.arm;
   unloadfile(ptr-1);
}


static void showstartlogo(char *text)
{
   char *logo_data;
   struct sprstrc *logo;
   char *font_data;
   struct sprstrc *font;

   char *snd_data;
   struct sndstrc snd;

   SAMPLE *sound_handle;

   clearscreen();
   
   /* Ignore any keys presses waiting in the queue. Note that this
      is not perfect. If X11 autorepeat is on (which it probably is),
      we shall get a steady stream of keypresses as long as a key is
      depressed and we shall probably eat them quicker than they are
      generated. */
   clear_keybuf();
   while (keypressed()) readkey();

   logo_data = loadfile(datapool, "GO.SPR");
   create_sprstrc (&logo, logo_data);
   font_data = loadfile(datapool, "FONT3.SPR");
   create_sprstrc (&font, font_data);

   snd_data  = loadfile(datapool, "GO.SND");
   create_sndstrc (&snd, snd_data);
   sound_handle = create_SAMPLE (&snd);
   playsample(sound_handle);

   putspritedirect(logo, STARTLOGOX, STARTLOGOY, 0);
   writetext(XMAX/2-strlen(text)*font->xs/2, STARTTEXTY, text, font);
   showpage(page);

   while (!keypressed());

   clearscreen();

   /* We cannot call destroy_sample because we allocated some
      of the fields in sound_handle ourselves. */
   stop_sample (sound_handle);
   free(sound_handle);


   //unloadfile(snd_data);
   unloadfile(font);
   unloadfile(logo);

}

static void showwinpic(void)
{
   char *font_data;
   struct sprstrc *font;
   void	  *ptr;

   while(pressedkeys);
   setvanillapalette(0);
   setpage(0); clearscreen(); showpage(0);
   ptr = loadfile(datapool, "SKY.PCX");
   showpcx256(ptr, 0);
   unloadfile(ptr);
   font_data = loadfile(datapool, "FONT1.SPR");
   create_sprstrc (&font, font_data);
   writetext(40, WINTEXTY, "SORRY. NO HERO TUNE THIS TIME.", font);
   glowin(1);
   unloadfile(font_data);
   free (font);

   waitforkey();

   glowout();
   clearscreen();
   showpage(0);
   setstandardpalette();
}

static void showendlogo(void)
{
   void *snd_data;
   struct sndstrc snd;
   void   *ptr;

   while(keypressed()) readkey();
   setvanillapalette(0);
   setpage(0); showpage(0); clearscreen();
   ptr = loadfile(datapool, "LANDSCAP.PCX");
   showpcx256(ptr,5);
   unloadfile(ptr);
   snd_data = loadfile(datapool, "TOD.SND");
   create_sndstrc (&snd, snd_data);
   playsample(create_SAMPLE(&snd));
   glowin(0);

   waitforkey();

   glowout();
   haltsound();
   unloadfile(snd_data);
   clearscreen();
   showpage(0);
   setstandardpalette();
}

/* Initialize s with the data read from the file.*/
void create_starstrc (struct starstrc *s, unsigned char *data)
{
  s->x     = data[0] + (data[1] << 8);
  s->y     = data[2] + (data[3] << 8);
  s->color = data[4] + (data[5] << 8);
  s->speed = data[6] + (data[7] << 8);
}

/*------------------------------------------------------
Function: initlevel

Description: Loads and initializes a new level.
------------------------------------------------------*/

static void initlevel(int stage)
{
   int   i;
   unsigned char   *ptr;
   char  levelstr[14], text[14];	// Level filename strings.

   sprintf(levelstr, "LEVEL%d", stage);

// Load Level description.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".DSC"));
   level.descript = (struct descrstrc *)ptr;
// Check for wrong level.
   if (level.descript->level != stage) error("initlevel", EINVAL);
   showstartlogo(level.descript->text);

// Load Sprite Library.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".SLI"));
   level.nsprites = ptr[0] + (ptr[1] << 8);

// Initialize enemy sprites and index tables.
   lsprofs = intindexptr; /* Save offset of this level's sprites */
   load_sprite_library (ptr + 2, level.nsprites);

// Unload unneeded data.
   unloadfile(ptr);

// Load Sound library.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".SND"));
   level.nsounds = ptr[0] + (ptr[1] << 8);

   level.soundfile = ptr;
   load_sound_library (ptr + 2, level.nsounds);

// Load Foe pathes.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".FOE"));
   level.nfoes = ptr[0] + (ptr[1] << 8);
   level.foefile = ptr;
   load_foe_table (ptr + 2, level.nfoes);

// Load Explosions.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".EXP"));
   level.nexpls = ptr[0] + (ptr[1] << 8);
   level.explfile = ptr;
   load_explosions (ptr + 2, level.nexpls);

// Load Attack table.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".TBL"));
   level.nattacks = ptr[0] + (ptr[1] << 8);
   level.attack = (struct attackstrc *)(ptr+2);

// Load Starfield.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".STA"));
   level.nstars = *ptr + (ptr[1] << 8);
   level.star = (struct starstrc *)(ptr+2);

// Starfield definition.
   defstarfield(level.nstars, level.star);
   gostarfield();

// Add bonus score and money.
   score += level.descript->score;
   money += level.descript->money;
}


static void shutlevel(void)
{
   int   i;
   short   *ptr;

   killallbuddies();
   killallobjects();
   killstarfield();
   for (i = 0; i < level.nsprites; i++) {
      killsprite(intindex[i + lsprofs]);
   }
   intindexptr -= level.nsprites;
   ptrindexptr -= (level.nfoes + level.nexpls + level.nsounds);

   unloadfile(level.soundfile);
   unloadfile (level.explfile);
   free (level.expl);
   unloadfile (level.foefile);
   free (level.foe);
   ptr = (short *)level.attack;
   unloadfile(ptr-1);
   ptr = (short *)level.star;
   unloadfile(ptr-1);
}



static void showplayfield(void)
{
   clearscreen();
   putspritedirect(scorebar, 0, BARY, 0);
   killallobjects();
   killallbuddies();
   setpage(0);
   displifes();
   setpage(1);
   displifes();
}


void playthegame(void)
{
   int    feedback;

   newgame();

   do {
      initlevel(stage);
      setplayposition(level.nattacks, level.attack, level.descript->nbigboss);
      
      do {
	 if (!(cheatlevel & CHEATLIFES)) lifes--;
	 showplayfield();
	 defallarms(XSTART, YSTART);
	 feedback = play();
      } while (!feedback && (lifes > 0));
      if (!(cheatlevel & CHEATLIFES)) lifes++;
      shutlevel();
      stage++;
      if (!feedback || (stage == LEVELS)) break;
      weaponmanager();
   } while (1);

   shutnewgame();

// Check if player has fought through all the levels.
   if (feedback && (stage == LEVELS)) {
// Here we will have the winner code.
      showwinpic();
   } else {
// Here goes the looser code.
      showendlogo();
   }

   highscore(TRUE);

}
