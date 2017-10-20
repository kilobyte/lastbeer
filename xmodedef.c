/* Allegro port Copyright (C) 2014 Gavin Smith. */
/*-------------------------------------------------------*/
/*                                                       */
/*  Combined definiton file for XMODEC.C & XMODEASM.ASM  */
/*                                                       */
/*                                                       */


#include "xmode.h"

struct lowspr	  _sprite[MAXSPRITES];
struct objstrc	  _obj[MAXOBJS];
struct sfieldstrc _sfield;
unsigned	  base = 0xa000;
unsigned          page = 0;

int	          objflashcolor = 15;	// Object flashes in white first.
int               backgrndcolor = 0;	// Background is black.
PALETTE           palette;	// Palette currently in use.

