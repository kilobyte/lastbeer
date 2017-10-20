// module shop.c
/* Allegro port Copyright (C) 2014 Gavin Smith. */
// [c] copyright 1993 by ALPHA-HELIX

#include <string.h>
#include <stdlib.h>

#include "sound.h"
#include "xmode.h"
#include "fileman.h"
#include "baller.h"


#define SHOPMAXARMS		5	// # of arms displayed on tresen.
#define SHOPXSELL		208	// Position of seller area in shop.
#define SHOPYSELL		137
#define SHOPARMHI		30	// Assumed height of heighest weapon.
#define SHOPXMENU		14	// Position of menu card.
#define SHOPYMENU		106
#define SHOPXEXIT		4	// Position of exit label in shop.
#define SHOPYEXIT               59
#define SHOPSPC			4	// Space between weapons.
#define SHOPTEXT		210	// Y pos of weapon text.
#define SHOPMTEXT		176	// X pos of money text.
#define SHOPXPRICE              58
#define SHOPYPRICE		105

#define MENUX			69      // Menucard font offsets
#define MENUY			12
#define MENCURX			48      // Menucard curser offsets
#define MENCURY			27
#define MENBACKX		48      // Menucard background
#define MENBACKY		26
#define MENUXEXIT		69      // Close card offsets
#define MENUYEXIT		180
#define MENUXPRICE		(XMAX - 65)
#define MSTEP			2

#define NOFWEAPONS		10

#define WEAPONX0		(272 - 120)
#define WEAPONY0		19
#define WEAPONX1		272
#define WEAPONY1		(19 + 80)
#define WEAPONFLASH		(6*16)  // Flash color.
#define WEAPONXTEXT		160
#define WEAPONYTEXT		150
#define WEAPONXMONEY		SHOPMTEXT
#define WEAPONYMONEY		SHOPTEXT
#define WEAPONXPTEXT            160
#define WEAPONYP                170
#define WEAPONXNAME             160
#define WEAPONYNAME             150
#define WEAPONXP                (XMAX - 40)

#define SELLXEXIT		20
#define SELLYEXIT		25
#define NOE			4


int max (int a, int b)
{
  if (a >= b)
    return a;
  else
    return b;
}

int min (int a, int b)
{
  if (a <= b)
    return a;
  else
    return b;
}

/*------------------------------------------------------
Function: defallarms

Description: Defines all active weapons around (x, y).
------------------------------------------------------*/

ulong defallarms(short int x, short int y)
{
   int     i;
   ulong   xy;

   /* weaponlst[0] is the main ship (bottle). */
   struct lowspr *s = &_sprite[intindex[weaponlst[0].arm.sprite]];

   x -= (s->xs / 2);
   x &= 0xfffc;

   y -= (s->ys / 2);
   y &= 0xfffc;
   
   for (i = 0; i < nweapons; i++) {
      weaponlst[i].object = defarm(weaponlst[i].dx + x, weaponlst[i].dy + y,
				   &weaponlst[i].arm);
   }

   xy = x | (unsigned long) y << 16;
   return xy;
}


static void waitforrelease(void)
{
   /* The keypressed() function in Allegro presumably checks for
      "key down" events, but we are interested in "key up" events.
      There seems to be no easy way to see if a key is pressed other
      than by checking every entry in the key array. */
   while (   key[key_up] || key[key_down]
          || key[key_left] || key[key_right]
          || key[key_fire]) {
      waitfortick(); updatescreen();
   }
}


static void placeweapon(int n)
{
   int    handle;	       		// Handle of new weapon.
   int    saveflash;                    // To store old objectflashcolor.
   int    dir, i;                       // Jupeidi.
   int    abortkey;
   short    x, y;                         // Coordinates of new weapon.
   short obj_xs, obj_ys;
   ulong  xy;
   char *font_data;
   struct sprstrc *font;
   void   *sell;

   setvanillapalette(0);
   setpage(0); clearscreen();

// Load and display sell pcx.
   sell = loadfile(datapool, "SELL.PCX");
   showpcx256(sell, 0);
   unloadfile(sell);
   copypage(0, 1);

// Load font
   font_data = loadfile(datapool, "FONT1.SPR");
   create_sprstrc (&font, font_data);

// Define and display all active weapons.
   xy = defallarms(WEAPONX0 + (WEAPONX1 - WEAPONX0)/2,
	      WEAPONY0 + (WEAPONY1 - WEAPONY0)/2);

   writetext(160 - (13*font->xs), 0, "PLACE WEAPON OR PRESS ESC!", font);

   x = (WEAPONX0 + 4) & 0xfffc;		// Start position of new weapon.
   y = (WEAPONY0 + 4) & 0xfffc;
   handle = defobject(intindex[weapon.arm[n].sprite], x, y, OBJ_HIGH);

   obj_xs = _obj[handle].xs;
   obj_ys = _obj[handle].ys;

   saveflash = objflashcolor;
   objflashcolor = WEAPONFLASH; dir = 2;
   abortkey = 0;

// Display screen and begin placing
   glowin(0);
   waitforrelease();
   do {
      updatescreen();
      objflashcolor += dir;
      if ((objflashcolor < WEAPONFLASH) || (objflashcolor > WEAPONFLASH+15)) {
	 dir = -dir;
	 objflashcolor += dir;
      }
      flash(handle);

      if (key[key_up]) {
	 y -= 4;
	 if (y < ((WEAPONY0 + 4) & 0xfffc)) y += 4;
	 writetext(WEAPONXTEXT, WEAPONYTEXT, "       ", font);
      }
      if (key[key_down]) {
	 y += 4;
	 if (y > ((WEAPONY1 - obj_ys - 4) & 0xfffc)) y -= 4;
	 writetext(WEAPONXTEXT, WEAPONYTEXT, "       ", font);
      }
      if (key[key_left]) {
	 x -= 4;
	 if (x < ((WEAPONX0 + 4) & 0xfffc)) x += 4;
	 writetext(WEAPONXTEXT, WEAPONYTEXT, "       ", font);
      }
      if (key[key_right]) {
	 x += 4;
	 if (x > ((WEAPONX1 - obj_xs - 4) & 0xfffc)) x -= 4;
	 writetext(WEAPONXTEXT, WEAPONYTEXT, "       ", font);
      }
      if (key[key_fire]) {
	 abortkey = KEYSPACE;
	 for (i = 0; i < MAXARMS; i++) {
	    if (_arm[i].object != -1) {
	       if (crashtest(handle, _arm[i].object)) {
		  writetext(WEAPONXTEXT, WEAPONYTEXT, "OVERLAP", font);
		  abortkey = 0;
	       }
	    }
	 }
      }
      if (key[KEY_ESC]) {
	 abortkey = KEYESC;
      }

      moveobject(handle, x, y);
      waitfortick();

   } while (!abortkey);

   objflashcolor = saveflash;

   unloadfile(font_data);
   free (font);

   killobject(handle);
   killallobjects();
   killallbuddies();

   if (abortkey == KEYSPACE) {
      weaponlst[nweapons].dx = x - (short int)xy;
      weaponlst[nweapons].dy = y - (short int)(xy >> 16);
      weaponlst[nweapons].arm = weapon.arm[n];
      nweapons++;
   }
   glowout();
}

static void payforweapon(int selected, struct sprstrc *f)
{
   int i;

   if(!(cheatlevel & CHEATMONEY))
      for(i = money; money > (i-weapon.arm[selected].cost); money--){
	 writenumber(XMAX - (f->xs), SHOPTEXT, money-1, f);
         showpage(page);
      }
}

static void givemoneyback(int selected, struct sprstrc *f)
{
   int i;

   for(i = money; money < (i+(weapon.arm[selected].cost*3)/4); money++){
      writenumber(XMAX - (f->xs), SHOPTEXT, money+1, f);
      showpage(page);
   }
}

static void turnsign(int *exit, int exitspr)
{
// Turn Exitsign in
   if((*exit)<NOE && (*exit)>-2){
      if((*exit)<NOE-1){
	 (*exit)++;
	 removesprite(exitspr, SELLXEXIT, SELLYEXIT);
	 putsprite(exitspr, SELLXEXIT, SELLYEXIT, *exit);
      }else{
	 removesprite(exitspr, SELLXEXIT, SELLYEXIT);
	 putsprite(exitspr, SELLXEXIT, SELLYEXIT, *exit);
	 (*exit)++;
      }
   }
// Turn Exitsign out
   if((*exit)<-2){
      (*exit)++;
      removesprite(exitspr, SELLXEXIT, SELLYEXIT);
      if((*exit)<-3) putsprite(exitspr, SELLXEXIT, SELLYEXIT, -(*exit)-4);
   }
}

/*------------------------------------------------------
Function: sellweapon

Description: User may abandon one or more of his weapons.
------------------------------------------------------*/

/*static*/ void sellweapon(void)
{
   int    selected;
   int    saveflash;                    // To store old objectflashcolor.
   int    dir;                          // Jupeidi.
   int    i;
   int	  exitspr;
   char *font_data;
   struct sprstrc *font;
   void   *ptr;
   struct sprstrc *spr;
   int    exit;

/* exit:   -NOE-5 =>  Exitsign starts turning out
	   -2     =>  Exitsign is turned out
	   -1     =>  Exitsign starts turning in
	   NOE    =>  Exitsign is turned in
	   NOE+1  =>  Exit
*/

   setvanillapalette(0);
   clearscreen();
   setpage(0); showpage(0);

// Load and display sell pcx.
   ptr = loadfile(datapool, "SELL.PCX");
   showpcx256(ptr, 0);
   unloadfile(ptr);

// Load and define exit-sign
   ptr = loadfile(datapool, "EXIT2.SPR");
   create_sprstrc (&spr, ptr);
   //exitspr = defsprite(ptr, 4);
   exitspr = defsprite(spr, 4);
   free (spr);
   unloadfile(ptr);

   copypage(0, 1);

   font_data = loadfile(datapool, "FONT1.SPR");
   create_sprstrc (&font, font_data);

// Write Money the Player has and title
   writetext(WEAPONXMONEY, WEAPONYMONEY, "YOUR MONEY", font);
   writenumber(XMAX - (font->xs), WEAPONYMONEY, money, font);

   writetext(160 - (15*font->xs), 0, "CHOOSE WEAPON YOU WANT TO SELL", font);

   defallarms(WEAPONX0 + (WEAPONX1 - WEAPONX0)/2,
	      WEAPONY0 + (WEAPONY1 - WEAPONY0)/2);

   selected = 0; exit = -1;
   saveflash = objflashcolor;
   objflashcolor = WEAPONFLASH; dir = 2;
   writetext(WEAPONXNAME, WEAPONYNAME, "EXIT", font);

   glowin(0);

   while ((nweapons > 1) && (exit != NOE+1)){

      waitforrelease();
      do {
	 turnsign(&exit, exitspr);
// Flash Weapon
	 if(selected != 0){
	    objflashcolor += dir;
	    if ((objflashcolor < WEAPONFLASH) || (objflashcolor > WEAPONFLASH+15)) {
	       dir = -dir;
	       objflashcolor += dir;
	    }
            copycolor (FLASH_COLOUR, objflashcolor);
	    flash(weaponlst[selected].object);
	 }
// Right Key was pressed
	 if (key[key_right]) {
	    selected++;
	    if (selected == 1){                    // Move from Exit
	       exit = (-exit)-4;
	       writetext(WEAPONXPTEXT, WEAPONYP, "VALUE:", font);
	    }
	    if (selected == nweapons){		// Move to Exit
	       selected = 0;
	       exit = ((exit==-2) ? (-1):((-exit)-4));
	       writetext(WEAPONXNAME, WEAPONYNAME, "EXIT                ", font);
	       writetext(WEAPONXPTEXT, WEAPONYP, "                      ", font);
	    }else{
	       writetext(WEAPONXNAME, WEAPONYNAME, "                    ", font);
	       writetext(WEAPONXNAME, WEAPONYNAME,
			weaponlst[selected].arm.armname , font);
	       writenumber(WEAPONXP, WEAPONYP,
			(weaponlst[selected].arm.cost*3)/4, font);
	    }
	    while (key[key_right]) {
	       waitfortick();
	       updatescreen();
	       turnsign(&exit, exitspr);
	    }
	 }
// Left Key was pressed
	 if (key[key_left]) {
	    selected--;
	    if (selected < 0){               	// Move from Exit
	       exit = (-exit)-4;
	       selected = nweapons - 1;
	       writetext(WEAPONXPTEXT, WEAPONYP, "VALUE:", font);
	    }
	    if (selected == 0){                    // Move to Exit
	       exit = ((exit==-2) ? (-1):((-exit)-4));
	       writetext(WEAPONXNAME, WEAPONYNAME, "EXIT                ", font);
	       writetext(WEAPONXPTEXT, WEAPONYP, "                      ", font);
	    }else{
	       writetext(WEAPONXNAME, WEAPONYNAME, "                    ", font);
	       writetext(WEAPONXNAME, WEAPONYNAME,
			weaponlst[selected].arm.armname , font);
	       writenumber(WEAPONXP, WEAPONYP,
			(weaponlst[selected].arm.cost*3)/4, font);
	    }
	    while (key[key_left]) {
	       waitfortick();
	       updatescreen();
	       turnsign(&exit, exitspr);
	    }
	 }
	 waitfortick();
	 updatescreen();
         showpage(page);
      } while (!key[key_fire]);

      if (selected != 0) {
	 givemoneyback(selected, font);
	 abandonobject(weaponlst[selected].object);
	 nweapons--;			// Kill selected weapon from list.
	 for (i = selected; i < nweapons; i++) {
	    weaponlst[i] = weaponlst[i+1];
	 }
	 selected = 0;
	 exit = ((exit==-2) ? (-1):((-exit)-4));
	 writetext(WEAPONXNAME, WEAPONYNAME, "EXIT                ", font);
	 writetext(WEAPONXPTEXT, WEAPONYP, "                      ", font);
      } else {
	 exit = NOE+1;
      }
   }
   unloadfile(font_data);
   free(font);
   killsprite(exitspr);

   objflashcolor = saveflash;

   killallobjects();
   killallbuddies();

   glowout();
   clearscreen();
   showpage(page);
   setstandardpalette();

}

// Checks if player can buy Weapon or not

static int checkforexit(int selected, struct sprstrc *font)
{
   if (weapon.arm[selected].cost > money) {
      writetext(0, SHOPTEXT, "NOT ENOUGH MONEY", font);
      return FALSE;
   }
   if ((weapon.arm[selected].flags & W_SPEEDUP) && (shipspeed>=MAXSHIPSPEED)) {
      writetext(0, SHOPTEXT, "ONLY 2 SPEEDUPS", font);
      return FALSE;
   }
   if (!(weapon.arm[selected].flags & W_ISWEAPON)) return TRUE;
   if (nweapons >= MAXARMS) {
      writetext(0, SHOPTEXT, "NO PLACE TO PUT", font);
      return FALSE;
   }

   return TRUE;
}


/*------------------------------------------------------
Function: menucard

Description: Shows all available upgrades, powerups,
	     bursters, smashers, speeders, ...
	     ... and weapons.
------------------------------------------------------*/
static void menucard(void)
{
   int    i, y, delta;
   int    yw, ye;			// y-pos of weapon- and extra list.
   int    selected, quit;
   int    narms;
   char   text[29];
   char   *font1_data, *font2_data;
   struct sprstrc *font1, *font2;
   int	  weaponnumber[NOFWEAPONS];
   int 	  pointobj, pointspr;
   int	  backobj, backspr;
   void   *ptr;
   struct sprstrc *spr;

// Load Datas
   setvanillapalette(0);
   setpage(0); clearscreen(); showpage(0);

// Load and display menucard pcx.
   ptr = loadfile(datapool, "MENUCARD.PCX");
   showpcx256(ptr, 0);
   unloadfile(ptr);

// Load Objects (Cursor and its background)
   ptr = loadfile(datapool, "MENCUR2.SPR");
   create_sprstrc (&spr, ptr);
   pointspr = defsprite(spr, 4);
   free (spr);
   unloadfile(ptr);

   pointobj = defobject(pointspr, MENCURX, MENCURY, OBJ_HIGH);

   ptr = loadfile(datapool, "MENBACK.SPR");
   create_sprstrc (&spr, ptr);
   backspr = defsprite(spr, 4);
   free (spr);
   unloadfile(ptr);

   backobj = defobject(backspr, MENBACKX, MENBACKY, OBJ_LOW);

// Load Fonts
   font1_data = loadfile(datapool, "MFONT.SPR");
   font2_data = loadfile(datapool, "FONT1.SPR");
   create_sprstrc (&font1, font1_data);
   create_sprstrc (&font2, font2_data);

   copypage(0, 1);

// Making Menucard

   writetext(MENUXEXIT, MENUYEXIT, "BACK TO SHOP", font1);

   y = MENUY;
   writetext(MENUX, y, "COOL AND FRESH DRINKS", font1);
   yw = y += font1->ys + 6;
   narms = selected = 0;
   for (i = min(weapon.narms, SHOPMAXARMS); i < weapon.narms; i++) {
      if (weapon.arm[i].flags & W_ISWEAPON) {
	 memset(text, '.',  28); text[28] = '\0';
	 memcpy(text, weapon.arm[i].armname, strlen(weapon.arm[i].armname));
	 writetext(MENUX, y, text, font1);
	 writenumber(MENUXPRICE, y, weapon.arm[i].cost, font1);
	 y += font1->ys + 3;
	 narms++;
	 weaponnumber[++selected] = i;
      }
   }

   y += 8;
   writetext(MENUX, y, "SCHNAPS BONUS", font1);
   ye = y += font1->ys + 6;
   for (i = min(weapon.narms, SHOPMAXARMS); i < weapon.narms; i++) {
      if (!(weapon.arm[i].flags & W_ISWEAPON)) {
	 memset(text, '.',  28); text[28] = '\0';
	 memcpy(text, weapon.arm[i].armname, strlen(weapon.arm[i].armname));
	 writetext(MENUX, y, text, font1);
	 writenumber(MENUXPRICE, y, weapon.arm[i].cost, font1);
	 y += font1->ys + 3;
	 weaponnumber[++selected] = i;
      }
   }

// Write players money

   writetext(SHOPMTEXT, SHOPTEXT, "YOUR MONEY", font2);
   writenumber(XMAX - (font2->xs), SHOPTEXT, money, font2);

   glowin(0);

   selected = 1;

   do {

// Let the selection begin

      y = yw;
      waitforrelease();
      do {
	 updatescreen();
	 if(key[key_up]){                               	// Up
	    writetext(0, SHOPTEXT, "                 ", font2);
	    if(selected > 1){
	       selected--;
	       if (selected == narms)
		  delta = ye - yw - ((narms-1)*(font1->ys+3));
	       else if(selected == weapon.narms - min(weapon.narms, SHOPMAXARMS))
		  delta = MENUYEXIT - ye - ((selected-1-narms)*font1->ys+3);
	       else
		  delta = font1->ys+3;
	       for (i = 0; i < (delta / MSTEP); i++) {
		  moveobjectdelta(pointobj, 0, -MSTEP);
		  waitfortick();
		  updatescreen();
	       }
	    }else{
	       selected = weapon.narms - min(weapon.narms, SHOPMAXARMS) + 1;
	       moveobject(pointobj, MENCURX, MENUYEXIT);
	       waitforrelease();
	    }
	 }
	 if(key[key_down]){                             	// Down
	    writetext(0, SHOPTEXT, "                 ", font2);
	    if(selected < weapon.narms-min(weapon.narms, SHOPMAXARMS)+1){
	       selected++;
	       if (selected == (narms+1))
		  delta = ye - yw - ((narms-1)*(font1->ys+3));
	       else if(selected == weapon.narms - min(weapon.narms, SHOPMAXARMS)+1)
		  delta = MENUYEXIT - ye - ((selected-2-narms)*font1->ys+3);
	       else
		  delta = font1->ys+3;
	       for (i = 0; i < (delta / MSTEP); i++) {
		  moveobjectdelta(pointobj, 0, MSTEP);
		  waitfortick();
		  updatescreen();
	       }
	    }else{
	       selected = 1;
	       moveobject(pointobj, MENCURX, MENCURY);
	       waitforrelease();
	    }
	 }
	 waitfortick();
	 if (key[key_fire] && selected != weapon.narms-min(weapon.narms, SHOPMAXARMS)+1)
	    quit = checkforexit(weaponnumber[selected], font2);
      } while (!((key[key_fire]) &&
	     ((selected == (weapon.narms-min(weapon.narms, SHOPMAXARMS)+1)) ||
	     quit)));

// Something was selected
      if(selected != (weapon.narms-min(weapon.narms, SHOPMAXARMS)+1)) // Not Exit
	 if (weapon.arm[weaponnumber[selected]].flags & W_ISWEAPON) {
	    payforweapon(weaponnumber[selected], font2);
	    glowout();
	    killobject(backobj);
	    killobject(pointobj);
	    clearscreen();
	    setstandardpalette();
	    placeweapon(weaponnumber[selected]);

	    selected = 1;

// Load Datas
	    setpage(0); clearscreen(); showpage(0);

// Load and display menucard pcx.
	    setvanillapalette(0);
	    ptr = loadfile(datapool, "MENUCARD.PCX");
	    setvanillapalette(0);
	    showpcx256(ptr, 0);
	    unloadfile(ptr);

// Set Cursorposition
	    pointobj = defobject(pointspr, MENCURX, MENCURY, OBJ_HIGH);
	    backobj = defobject(backspr, MENBACKX, MENBACKY, OBJ_LOW);

	    copypage(0, 1);

// Making Menucard

	    writetext(MENUXEXIT, MENUYEXIT, "BACK TO SHOP", font1);
	    y = MENUY;
	    writetext(MENUX, y, "GETRAENKE UND WAFFEN", font1);
	    yw = y += font1->ys + 6;
	    for (i = min(weapon.narms, SHOPMAXARMS); i < weapon.narms; i++) {
	       if (weapon.arm[i].flags & W_ISWEAPON) {
                  memset(text, '.',  28); text[28] = '\0';
		  memcpy(text, weapon.arm[i].armname, strlen(weapon.arm[i].armname));
		  writetext(MENUX, y, text, font1);
		  writenumber(MENUXPRICE, y, weapon.arm[i].cost, font1);
		  y += font1->ys + 3;
	       }
	    }

	    y += 8;
	    writetext(MENUX, y, "SCHNAPS BONUS", font1);
	    ye = y += font1->ys + 6;
	    for (i = min(weapon.narms, SHOPMAXARMS); i < weapon.narms; i++) {
	       if (!(weapon.arm[i].flags & W_ISWEAPON)) {
                  memset(text, '.',  28); text[28] = '\0';
		  memcpy(text, weapon.arm[i].armname, strlen(weapon.arm[i].armname));
		  writetext(MENUX, y, text, font1);
		  writenumber(MENUXPRICE, y, weapon.arm[i].cost, font1);
		  y += font1->ys + 3;
	       }
	    }

// Write players money
	    writetext(SHOPMTEXT, SHOPTEXT, "YOUR MONEY", font2);
	    writenumber(XMAX - (font2->xs), SHOPTEXT, money, font2);

	    glowin(0);

	 } else {
	    payforweapon(weaponnumber[selected], font2);
	    switch (weapon.arm[weaponnumber[selected]].flags) {
	       case W_SPEEDUP:
		  shipspeed += 2;
		  break;
	       case W_EXTRALIFE:
		  lifes++;
		  break;

	    }
	 }

   } while(selected != weapon.narms - min(weapon.narms, SHOPMAXARMS) + 1);

   glowout();

   killobject(backobj);
   killsprite(backspr);
   killobject(pointobj);
   killsprite(pointspr);
   unloadfile(font2);
   unloadfile(font1);

   clearscreen();
   setstandardpalette();
}


static void placeprice(long price, struct sprstrc *font)
{
   int j, xs, x, i = 0;
   int zahl;

   x = SHOPXPRICE;
   xs = font->xs;
   for(j = 0; j < 2; j++){
      putspritedirect(font, x, SHOPYPRICE, (price % 10));
      price /= 10;
      x -= xs;
   }
   putspritedirect(font, x, SHOPYPRICE, 10);  // Write point
   for(; j < 5; j++){
      x -= xs;
      zahl = price % 10;
      if(zahl != 0 || j == 2){
	 putspritedirect(font, x, SHOPYPRICE, zahl);
	 if(i!=0) for(; i > 0; i--) putspritedirect(font, x+i*xs, SHOPYPRICE, 0);
	 i = 0;
      }else{
	 putspritedirect(font, x, SHOPYPRICE, 11);     // Clear unused digits
	 i ++;
      }
      price /= 10;
   }
}

static void closeshop(struct sprstrc *pf, struct sprstrc *f, int obj, int espr, int pspr)
{
   glowout();

   unloadfile(pf);
   unloadfile(f);
   killobject(obj);
   killsprite(espr);
   killsprite(pspr);

}

/*------------------------------------------------------
Function: buyweapon

Description: Opens the shop screen and let the user
	     choose a weapon. If a weapon was bought,
	     it must be placed. If 'exit' was selected,
	     TRUE is returned.
------------------------------------------------------*/

static int buyweapon(void)
{
   int     x, selected;			// x pos and # of selected weapon.
   int     i, j, spr;                   // Lall variablen.
   int     narms;			// # of arms displayed on tresen.
   int     pointspr, obj, exitspr;
   int     yspoint;
   void    *pointer;			// Picture data.
   struct  sprstrc *ss;
   char    *font_data, *pfont_data;
   struct  sprstrc *font, *pfont;
   int 	   value;			// Value of selected weapon
   int     ok;

   setvanillapalette(0);		// Go black.
   setpage(0); 
   clearscreen();

// Load and display store pcx.
   pointer = loadfile(datapool, "STORE.PCX");
   showpcx256(pointer, 0);
   unloadfile(pointer);

// Load and define arrow pointer.
   pointer = loadfile(datapool, "ARROW.SPR");
   create_sprstrc (&ss, pointer);
   pointspr = defsprite(ss, 2);
   free (ss);
   unloadfile(pointer);

// Load and define exit-sign
   pointer = loadfile(datapool, "EXIT.SPR");
   create_sprstrc (&ss, pointer);
   exitspr = defsprite(ss, 4);
   free (ss);
   unloadfile(pointer);

// Display weapons on 'tresen'.
   narms = min(weapon.narms, SHOPMAXARMS) - 1;
   for (i = 1, x = SHOPXSELL; i <= narms; i++) {
      if (weapon.arm[i].flags & W_ISWEAPON) {
	 spr = intindex[weapon.arm[i].sprite];
	 putsprite(spr, x, SHOPYSELL - _sprite[spr].ys, 0);
	 x += _sprite[spr].xs + SHOPSPC;
      } else {
	 narms--;
      }
   }

   copypage(0, 1);			// Copy to second page not to
					// have flickering animation.

// Open fonts
   font_data = loadfile(datapool, "FONT1.SPR");
   create_sprstrc (&font, font_data);
   pfont_data = loadfile(datapool, "CFONT.SPR");
   create_sprstrc (&pfont, pfont_data);

// Set Pointer to selected weapon;
// selected = -1 : exit
// selected =  0 : menu card
// selected >  0 : weapons
   x = SHOPXSELL; selected = 1;
// Adjust arrow position according to it's x-size.
   x -= _sprite[pointspr].xs / 2;
   x += _sprite[intindex[weapon.arm[selected].sprite]].xs / 2;
   yspoint = _sprite[pointspr].ys;
   obj = defobject(pointspr, x, SHOPYSELL-SHOPARMHI-yspoint, OBJ_LOW);
   writetext(0, SHOPTEXT, weapon.arm[selected].armname, font);
   value = weapon.arm[selected].cost;
   placeprice(value, pfont);

// Display money the player has
   writetext(SHOPMTEXT, SHOPTEXT, "YOUR MONEY", font);
   writenumber(XMAX - (font->xs), SHOPTEXT, money, font);

   glowin(0);				// Finally show shop picture.


// Move pointer according to the pressed keys.
   waitforrelease();
   do {
      ok = 1;
      updatescreen();

      if ((key[key_right]) && selected < narms) {
	 writetext(0, SHOPTEXT, "                 ", font);
	 selected++;
	 switch (selected) {
	    case 0:			// Move from exit to menu card.
	       killobject(obj);
	       writetext(0, SHOPTEXT, "MENU CARD", font);
	       putsprite(exitspr, SHOPXEXIT, SHOPYEXIT, 0);
	       waitfortick();
	       updatescreen();
	       putsprite(exitspr, SHOPXEXIT, SHOPYEXIT, 0);
	       obj = defobject(pointspr, SHOPXMENU, SHOPYMENU - yspoint, OBJ_LOW);
	       waitforrelease();
	       break;
	    case 1:			// Move from menu to weapons.
	       moveobject(obj, x, SHOPYSELL - SHOPARMHI - yspoint);
	       writetext(0, SHOPTEXT, weapon.arm[1].armname, font);
	       value = weapon.arm[1].cost;
	       placeprice(value, pfont);
	       waitforrelease();
	       break;
	    default:
	       j = _sprite[intindex[weapon.arm[selected-1].sprite]].xs / 2
		 + _sprite[intindex[weapon.arm[selected].sprite]].xs / 2
		 + SHOPSPC;
	       for (i = 0; i < j; i+=2) {
		  x += 2; moveobjectdelta(obj, 2, 0);
		  waitfortick(); updatescreen();
	       }
	       writetext(0, SHOPTEXT, weapon.arm[selected].armname, font);
	       value = weapon.arm[selected].cost;
	       placeprice(value, pfont);
	       break;
	 }
      }

      if ((key[key_left]) && selected > -1) {
	 writetext(0, SHOPTEXT, "                 ", font);
	 selected--;
	 switch (selected) {
	    case -1:			// Move from menu-card to exit.
	       killobject(obj);
	       obj = defobject(exitspr, SHOPXEXIT, SHOPYEXIT, OBJ_LOW);
	       writetext(0, SHOPTEXT, "EXIT SHOP", font);
	       waitforrelease();
	       break;
	    case 0:			// Move from weapons to menu.
	       moveobject(obj, SHOPXMENU, SHOPYMENU - yspoint);
	       writetext(0, SHOPTEXT, "MENU CARD", font);
	       value = 0;
	       placeprice(value, pfont);
	       waitforrelease();
	       break;
	    default:
               j = _sprite[intindex[weapon.arm[selected+1].sprite]].xs / 2
                 + _sprite[intindex[weapon.arm[selected].sprite]].xs / 2
		 + SHOPSPC;
	       for (i = 0; i < j; i+=2) {
		  x -= 2; moveobjectdelta(obj, -2, 0);
		  waitfortick(); updatescreen();
	       }
	       writetext(0, SHOPTEXT, weapon.arm[selected].armname, font);
	       value = weapon.arm[selected].cost;
	       placeprice(value, pfont);
	       break;
	 }
      }

      waitfortick();


      if (key[key_fire]) {
	 if (selected < 1) break;
	 if (value > money) {
	    ok = 0;
	    writetext(0, SHOPTEXT, "NOT ENOUGH MONEY", font);
	 }
	 if (nweapons >= MAXARMS) {
	    ok = 0;
	    writetext(0, SHOPTEXT, "NO PLACE TO PUT", font);
	 }
	       }
   } while (!key[key_fire] || !ok);

// Player has pressed fire.

   switch (selected) {
      case -1: closeshop(pfont, font, obj, exitspr, pointspr);
	       clearscreen();
	       setstandardpalette();
	       return TRUE;
      case  0: closeshop(pfont, font, obj, exitspr, pointspr);
	       menucard();
	       return FALSE;
      default: payforweapon(selected, font);             // Let Player pay for new weapon
	       closeshop(pfont, font, obj, exitspr, pointspr);
	       placeweapon(selected);
	       clearscreen();
	       setstandardpalette();
	       return FALSE;
   }

}


void weaponmanager(void)
{
   long   deltam;
   char *ptr;
   struct sndstrc ss;
   SAMPLE *s;


// Calculate money
   deltam = (score - scoreold +1500) / 2500;
   deltam *= 5;
   money += deltam;
   scoreold = score;
   if(cheatlevel & CHEATMONEY) money = 999995;

// Sell weapons

   ptr = loadfile(datapool, "SELL.SND");
   create_sndstrc (&ss, ptr);
   s = create_SAMPLE (&ss);
   playloop(s);

   if (nweapons > 1) sellweapon();
   haltsound();
   free (s);
   unloadfile(ptr);

// Buy weapons

   ptr = loadfile(datapool, "BUY.SND");
   create_sndstrc (&ss, ptr);
   s = create_SAMPLE (&ss);
   playloop(s);

   while (!buyweapon());
   stop_sample (s);
   free (s);
   haltsound();
   unloadfile(ptr);
}

