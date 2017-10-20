/* Allegro port Copyright (C) 2014 Gavin Smith. */
/*-------------------------------------------------------*/
/*                                                       */
/*  Combined definiton file for XMODEC.C & XMODEASM.ASM  */
/*                                                       */
/*                                                       */


// Header file for xmode.

#ifndef __XMODE__
#define __XMODE__

#include <allegro.h>

// Definiton of graphics window.
#define XMIN			0
#define XMAX			319
#define YMIN			0
#define YMAX			219


#define MAXSPRITES		60
#define MAXOBJS			60

#define OBJ_ONECYCLE		0x01	// Destroy object after a cycle.
#define OBJ_HIGH		0x02	// High priority.
#define OBJ_LOW			0x00	// Low priority.


/* --- SPRITE DEFINITION
   A sprite is defined as the following:
   xs  :  Width of sprite in pixels.
   ys  :  Heigth in pixels.
   maxn:  Number of pictures per sprite. (always > 0)

 Located after this header is the sprite data itself.
 Each pixel is ONE byte. (planar video mode)
 First row of first picture, second row of first picture, ...
 First row of second picture, ... and so on.
*/

#define SPR_ALIGN		0x07	// Align Field.
#define SPR_DOUBLE		0x08    // Double paint picture.

#define STAR_X      0
#define STAR_Y      2
#define STAR_COLOR  4
#define STAR_SPEED  6

#define STARSTRC_SIZE 8;

/* Function to access field in starstrc array. */
int starstrc_member (unsigned char *data, int i, int member);

void set_starstrc_member (unsigned char *data, int i, int member,
                         unsigned short int value);

struct sprstrc {

   short   xs;			// x size of sprite.
   short   ys;			// y size.
   short  maxn;			// Number of pictures.
   // Immediately here follows the SPRITEDATA.
   //char  data[];
   char  *data;
};

void load_sprstrc_from_file (struct sprstrc **s, void *database, char *file);

/* --- Low level sprite definition structure.
   If a sprite has been pushed into the following structure it is ready
   to be used by the low level sprite routines.
*/

struct lowspr {

   int   active;		// Is sprite active ?

   /* x size, possibly multiplied by 4, once for each VGA page. */
   int   xs;
   int   ys;                    // y size.
   int   nadd;		// picture increment value.
   int   maxn;          // number of pictures * 2
   int   xsalign;       // x size, but a multiple of 4
   int   picsize; // Size of one image
   int	 seqsize; // Size of all images in animation
   int   fullsize;

   BITMAP **bmp; // Pointer to array of BITMAP *
   BITMAP **bmp_silhouette; // Pointer to array of BITMAP *

    char  *data;		// Sprite data: Ptr into SCREEN MEM

   /* Each byte in mask corresponds to a pixel in the image. There are
    * four bits used in each byte which correspond to the 4 pages of
    * VGA memory. sizeof (mask) == fullsize. */
   /* Is set for plane p if (data[x+p-a] != 0). 0 presumably is a 0 in the
    * image bitmap representing a transparent pixel. */
    char  *mask;         // Mask data: Ptr into MAIN MEM
};



/* --- STAR definition
   Very simple indeed.
   Just a coordinate pair, a color and a base speed. (per star)
   A STARFIELD consists of an array of STARS.
*/

struct starstrc {

   int   x, y;
   int   color;
   int   speed;

};

void create_sprstrc (struct sprstrc **, unsigned char *);

#define retrace() {while(inportb(0x3da)&8); while(!(inportb(0x3da)&8));}

#define FLASH_COLOUR 230

void setpalette(PALETTE ptr);
void setcolor(int c, int r, int g, int b);
void setvanillapalette(int c);
void setstandardpalette(void);
void glowto(int r, int g, int b);
void glowin(int dir);
void glowout(void);
int cyclepalette(int c1, int c2, int pos);

void defstarfield(int n, unsigned char *star);
void gostarfield(void);
void stopstarfield(void);
void killstarfield(void);

int defsprite(void *sprite, unsigned flags);
void killsprite(int sprite);
void killallsprites(void);

int defobject(int sprite, int x, int y, int cycle);
void abandonobject(int obj);
void killobject(int obj);
void killallobjects(void);
void shutxmode(void);
void initxmode(void);
void setpage(int p);
void updatescreen(void);

unsigned long getspritesize(int sprite);
unsigned long getobjectsize(int object);
unsigned long getobjectpos(int object);

#define DISPLAY_SCALE 2

void setxmode(void);
void screenmode(int mode);

int moveobject(int obj, int x, int y);
int moveobjectdelta(int obj, int deltax, int deltay);
void flash(int obj);
int changesprite(short obj, short sprite);
int crashtest(int obj1, int obj2);
int outofwindow(int obj);

void putsprite(int sprite, int x, int y, int n);
void removesprite(int sprite, int x, int y);

void putspritedirect(struct sprstrc *spr, int xp, int yp, int n);

void clearscreen(void);
void clearregion(int y, int n);
void copypage(int src, int dst);
void showpage(int p);
void showline(int line);
void showpcx256(char *pic, int line);

int smooth_move (int difference, int subtick);

// window definition.
extern int   windowx0, windowy0;
extern int   windowx1, windowy1;

extern int   objflashcolor;
extern int   backgrndcolor;

#define FALSE			0
//#define TRUE			1


// VGA ports.

#define SC_INDEX		0x3c4		// Sequence Controller Index.
#define GC_INDEX		0x3ce
#define CRTC_INDEX		0x3d4		// CRT Controller Index.
#define MISC_OUTPUT		0x3c2		// Miscellaneous Output register.
#define MAP_MASK		0x2
#define BIT_MASK		0x8
#define INPUT_STATUS		0x3da
#define START_ADDR_HIGH		0xc
#define START_ADDR_LOW		0xd


// VGA 320x240 low level graphic constants.

#define BYTESPERLINE		80
#define PAGESIZE		(BYTESPERLINE * (YMAX+1))
#define OFFSCREEN		(2*PAGESIZE)
#define PALETTESIZE		768



// Object states.
#define O_FLASH                 0x0001
struct	objstrc {

   int	 active;
   int	 flags;			// State of the object.
   int	 x;			// (x, y)
   int	 y;
   int	 xa;			// (xa, ya) in active screen.
   int	 ya;
   int	 xb;
   int	 yb;
   int	 xs;
   int	 ys;
   int	 n;
   int   nadd;
   int	 maxn;
   int   cycle;			// 0-forward, 1-destroy
   int   destroy;
   int	 sprite;

};


struct sfieldstrc {

   int    active;		// Field active?
   int    go;
   int    n;			// Number of STARS in field.
   unsigned char *star;	// Pointer to array of stars.

};


/* ----- Global data ----- */

extern struct lowspr	  _sprite[MAXSPRITES];
extern struct objstrc	  _obj[MAXOBJS];
extern struct sfieldstrc _sfield;
extern unsigned	  base;
extern unsigned          page;
extern BITMAP *pages[2];
extern BITMAP *full_pages[2];


extern int         objflashcolor;	// Object flashes in white first.
extern int         backgrndcolor;	// Background is black.
extern PALETTE           palette;	// Palette currently in use.

#endif

