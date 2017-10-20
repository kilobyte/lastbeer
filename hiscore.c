// module hiscore.c
/* Allegro port Copyright (C) 2014 Gavin Smith. */
// [c] copyright 1993 by ALPHA-HELIX

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define	MAXENTRIES	8
#define	NOCP		15	// Number of Chars playername
#define	NOCW		33	// Number of Chars winnertext
#define YOFFS		60	// Y-offset for listbegin
#define SXOFFS		255     // X-offset for centiliter
#define NXOFFS		55	// X-offset for names

#include "xmode.h"
#include "sound.h"
#include "fileman.h"
#include "baller.h"

struct playerdatas{
   long score;
   char name[NOCP+1];
};

struct highscoretext{
   struct playerdatas player[MAXENTRIES];
   char winnertext[NOCW+1];
};

static struct highscoretext hstxt={{{   81000L, "ATOMMUELL"},
				    {   29600L, "POWER"},
				    {   14800L, "JUBEL"},
				    {    8110L, "ZYNAX"},
				    {    8099L, "HEAVY"},
				    {    6367L, "WASSERGLAS"},
				    {    4558L, "TWEETY"},
				    {       1L, "TRITONE"}},
				   "EIN PROSIT AUF ALPHA-HELIX"};

static void cursorblink(int x, int y, char symbol, struct sprstrc *font)
{
   int set;

   set=FALSE;
   if(symbol==0) symbol=' ';

   clear_keybuf();
   while(!keypressed()) {
      if(set==0){
	 set=8;
	 putspritedirect(font, x, y, 3);
      }
      if(set==4){
	 putspritedirect(font, x, y, symbol-' ');
      }
      set--;
      showpage(page);
      waitfortick();
   }
   if(set>3) putspritedirect(font, x, y, symbol-' ');
   showpage(page);
}

static void codescore(void)
{
   int i,j;

   for(j=0;j<MAXENTRIES;j++){
      for(i=0;i<=NOCP;i++){
	 hstxt.player[j].name[i]+=3*i;
      }
      hstxt.player[j].score+=123456789;
   }
   for(j=0;j<=NOCW;j++){
      hstxt.winnertext[j]+=j*2;
   }
}

static void decodescore(void)
{
   int i,j;

   for(j=0;j<MAXENTRIES;j++){
      for(i=0;i<=NOCP;i++){
	 hstxt.player[j].name[i]-=3*i;
      }
      hstxt.player[j].score-=123456789;
   }
   for(j=0;j<=NOCW;j++){
      hstxt.winnertext[j]-=j*2;
   }
}

void loadhighscore(int *filvar)
{
   read(*filvar, &hstxt, sizeof(struct highscoretext));
   decodescore();
}

void savehighscore(int *filvar)
{
   codescore();
   write(*filvar, &hstxt, sizeof(struct highscoretext));
   decodescore();
}

void highscore(char fullmode)
{
   int i,j,x,xs,currentpl,inlist;
   char keys;
   char *font2_file, *font_file;
   struct sprstrc *font2, *font;

   struct sndstrc snd;
   void *ptr;
   SAMPLE *s;

   setvanillapalette(0);
   setpage(0); showpage(0); clearscreen();
   ptr = loadfile(datapool, "SCORE.PCX");
   showpcx256(ptr, 0);
   unloadfile(ptr);

   font_file = loadfile(datapool, "FONT1.SPR");
   create_sprstrc (&font, font_file);
   font2_file = loadfile(datapool, "FONT2.SPR");
   create_sprstrc (&font2, font2_file);

   ptr = loadfile(datapool, "HS.SND");
   create_sndstrc (&snd, ptr);
   s = create_SAMPLE (&snd);
   playloop(s);

// Checking for a place in the highscore

   inlist=FALSE;
   if(fullmode){
      currentpl=0;
      while(!inlist && currentpl<MAXENTRIES){
	 if(score<=hstxt.player[currentpl].score) currentpl++;
	 else inlist=TRUE;
      }

// Is in highscore

      if(inlist){
	 for(i=MAXENTRIES-1; i>currentpl; i--) hstxt.player[i]=hstxt.player[i-1];
	 strcpy(hstxt.player[currentpl].name,"");
	 hstxt.player[currentpl].score=score;
	 if(currentpl==0) strcpy(hstxt.winnertext,"");

      }
   }
// Show new highscore

   for(i=0;i<MAXENTRIES;i++){
      writetext(NXOFFS,YOFFS+i*14,hstxt.player[i].name,font);
      writenumber(SXOFFS,YOFFS+i*14,hstxt.player[i].score,font);
   }
   x=(320-strlen(hstxt.winnertext)*font2->xs)/2;
   writetext(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
   glowin(0);

// Enter name

   if(inlist){
      xs=font->xs;
      i=0;
      x=NXOFFS;
      //setvect(0x09, int09);
      do{
	 cursorblink(x,YOFFS+currentpl*14,hstxt.player[currentpl].name[i],font);
         /* Read ASCII value */
	 keys=readkey() & 0xFF;
	 if(keys<=122 && keys>=97) keys-=32;
	 if((keys<=90 && keys>=65) || keys==63 || keys == 46 ||
		keys==39 || (keys<=57 && keys >=48) ||
		keys==32 || keys==33 || keys==34){
	    hstxt.player[currentpl].name[i]=keys;
	    hstxt.player[currentpl].name[i+1]=0;
	    putspritedirect(font, x, YOFFS+currentpl*14, keys-' ');
	    if(i<NOCP-1){
	       i++;
	       x+=xs;
	    }
	 }
	 else if(keys==0x08 && i>0){
	    if(i!=(NOCP-1) || hstxt.player[currentpl].name[NOCP-1]==0){
	       i--;
	       x-=xs;
	    }
	    hstxt.player[currentpl].name[i]=0;
	    putspritedirect(font, x, YOFFS+currentpl*14, 0);
	 }
         showpage(page); /* Update screen, as we are not drawing
                            directly to video memory. */
      }while(keys!=0x0d);
      if(hstxt.player[currentpl].name[0]==0){
	 strcpy(hstxt.player[currentpl].name,"TOO DRUNK");
	 writetext(x,YOFFS+currentpl*14,hstxt.player[currentpl].name,font);
      }

// Winnertext

      if(currentpl==0){
	 writetext(64, YOFFS+(MAXENTRIES+1)*14+8, "PLEASE ENTER WINNER TEXT", font);
	 i=0;
	 xs=font2->xs;
	 do{
	    if(i == NOCW){
	       x = (320 + (NOCW-3) * xs) / 2;
	       cursorblink(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext[i-1],font2);
	    }else{
	       x = (320 + (i-1) * xs) / 2;
	       cursorblink(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext[i],font2);
	    }
	    keys=readkey() & 0xFF;
	    if(keys<=122 && keys>=97) keys-=32;
	    if((keys<=93 && keys>=65) || (keys<=59 && keys>=39) ||
			keys==32 || keys==33 || keys==34 ||
			keys==61 || keys==63){
	       if(i<NOCW){
		  hstxt.winnertext[i]=keys;
		  hstxt.winnertext[i+1]=0;
		  i++;
	       }else{
		  hstxt.winnertext[NOCW-1]=keys;
		  hstxt.winnertext[NOCW]=0;
	       }
	       clearregion(YOFFS+(MAXENTRIES+3)*14-5,10);
	       writetext((320-(i+1)*xs)/2,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
	    }
	    else if(keys==0x08 && i>0){
	       i--;
	       hstxt.winnertext[i]=0;
	       clearregion(YOFFS+(MAXENTRIES+3)*14-5,10);
	       writetext((320-(i+1)*xs)/2,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
	    }
            showpage(page); /* Update screen, as we are not drawing
                               directly to video memory. */
	 }while(keys!=0x0d);
	 if(hstxt.winnertext[0]==0){
	    strcpy(hstxt.winnertext,"TOO DRUNK TO WRITE DOWN A TEXT!!!");
	    x=(320-strlen(hstxt.winnertext)*xs)/2;
	    writetext(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
	 }

	 writetext(64, YOFFS+(MAXENTRIES+1)*14+8, "                        ", font);
         showpage(page);
      }
      //setvect(0x09, newint09);
      saveconfig();
   }else if(fullmode){

// Not in highscore

      writetext(NXOFFS,YOFFS+(MAXENTRIES+1)*14-3,"YOUR SCORE",font);
      writenumber(SXOFFS,YOFFS+(MAXENTRIES+1)*14-3,score,font);
      showpage(page);
   }
   clear_keybuf();
   while (!keypressed());

   glowout();
   stop_sample (s);
   unloadfile (ptr);

   unloadfile(font2_file); free (font2);
   unloadfile(font_file); free (font);
}

