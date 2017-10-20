/* Allegro port Copyright (C) 2014 Gavin Smith. */
/*
;---------------------------------------------------------;
;                                                         ;
;                                                         ;
;   This module contains GRAPHICS PRIMITIVES for          ;
;   VGA undocumented 320x240 256 color mode.              ;
;                                                         ;
;       written by Dany Schoch for Alpha-Helix            ;
;   Parts of this code first appeard in Dr.Dobb's         ;
;   September 1991 issue (during my military service).    ;
;                                                         ;
*/

#include "xmode.h"
#include "baller.h"

extern volatile int subtick;

int run_in_fullscreen = 0;

void toggle_fullscreen (void)
{
  run_in_fullscreen = !run_in_fullscreen;
  setxmode ();
  setstandardpalette ();
}

void setxmode (void)
{
  set_color_depth (8);
  request_refresh_rate (60);
  set_gfx_mode (run_in_fullscreen ? GFX_AUTODETECT_FULLSCREEN
                                  : GFX_AUTODETECT_WINDOWED,
                DISPLAY_SCALE * 320,
                DISPLAY_SCALE * 240, 0, 0);
}

#if 0
int
smooth_move (int difference, int subtick)
{
  int ret = 0;
  int sign = 1;

  if (difference < 0)
    {
      sign = -1;
      difference *= -1;
    }

  ret += difference / RESOLUTION;
  if (subtick < difference % RESOLUTION)
    {
      if (difference > 0)
        ret++;
      if (difference < 0)
        ret--;
    }

  return ret * sign;
}
#endif

#if 1
int
smooth_move (int difference, int subtick)
{
  int ret = 0;
  int sign = 1;
  int leftover;

  if (difference < 0)
    {
      sign = -1;
      difference *= -1;
    }

  ret += difference / RESOLUTION;
  leftover = difference % RESOLUTION;

  if (leftover > 0 && leftover <= RESOLUTION/2)
    {
      /* Divide the leftover units to move over RESOLUTION frames. */
      int interval = RESOLUTION / leftover;
      if (subtick % interval == 0)
        ret++;
    }
  else if (leftover > 0)
    {
      /* Divide the frames NOT to move over RESOLUTION frames. */
      int interval = RESOLUTION / (RESOLUTION - leftover);
      if (subtick % interval != 0)
        ret++;
    }

  return ret * sign;
}
#endif

/* Scroll the star field */
void gostarfield(void)
{
  unsigned long write_address;
  int i;

  _sfield.go = TRUE;

  page ^= 1; /* Flip page so to draw to hidden page */

  bmp_select(pages[page]);

  for (i = 0; i < _sfield.n; i++) {
    unsigned short int x, y, speed;

    x = starstrc_member (_sfield.star, i, STAR_X);
    y = starstrc_member (_sfield.star, i, STAR_Y);
    speed = starstrc_member (_sfield.star, i, STAR_SPEED);

    y += speed;

    if (y >= windowy1) {
      y -= windowy1;
    }

    write_address = bmp_write_line(pages[page], y);
    bmp_write8 (write_address + x, backgrndcolor); /* Clear pixel */
  }
  bmp_unwrite_line (pages[page]);

  /* Back to original page */
  page ^= 1;
}

void stopstarfield(void)
{
  int i;
  unsigned long write_address;

  _sfield.go = FALSE;

  page ^= 1;
  for (i = 0; i < _sfield.n; i++) {
    int x, y;
    int speed;
    x = starstrc_member (_sfield.star, i, STAR_X);
    y = starstrc_member (_sfield.star, i, STAR_Y);

    speed = starstrc_member (_sfield.star, i, STAR_SPEED);
    /* FIXME: Broken for RESOLUTION == 1 (no mod 1 arith) */
    y -= smooth_move (speed, (subtick + RESOLUTION - 1) % RESOLUTION);
    if (y < 0)
      y += windowy1;

    write_address = bmp_write_line(pages[page], y);
    bmp_write8 (write_address + x, backgrndcolor); /* Clear pixel */
  }
  page ^= 1;
}

/* Draw the starfield */
void starfield (void)
{
  unsigned long write_address;
  int i;

  if (!_sfield.active) return;

  bmp_select(pages[page]);

  if (_sfield.go == TRUE) {
    for (i = 0; i < _sfield.n; i++) {
      short int x, y, speed, colour;

      x = starstrc_member (_sfield.star, i, STAR_X);
      y = starstrc_member (_sfield.star, i, STAR_Y);
      speed = starstrc_member (_sfield.star, i, STAR_SPEED);
      colour = starstrc_member (_sfield.star, i, STAR_COLOR);

      /* FIXME: Broken for RESOLUTION == 1 (no mod 1 arith) */
      y -= smooth_move (speed, (subtick + RESOLUTION - 1) % RESOLUTION);
      if (y < 0)
        y += windowy1;

      /* Clear pixel drawm on this page two frames ago. */
      write_address = bmp_write_line(pages[page], y);
      bmp_write8 (write_address + x, backgrndcolor);

      y = starstrc_member (_sfield.star, i, STAR_Y);
      y += smooth_move (speed, subtick);
      if (y >= windowy1)
        y -= windowy1;

      write_address = bmp_write_line(pages[page], y);
      bmp_write8 (write_address + x, colour); /* Set pixel */
      /* This saves where the star was drawn in this frame. */
      set_starstrc_member (_sfield.star, i, STAR_Y, y);
    }
  } else {
    for (i = 0; i < _sfield.n; i++) {
      unsigned short int x, y, speed, colour;

      x = starstrc_member (_sfield.star, i, STAR_X);
      y = starstrc_member (_sfield.star, i, STAR_Y);
      speed = starstrc_member (_sfield.star, i, STAR_SPEED);
      colour = starstrc_member (_sfield.star, i, STAR_COLOR);

      write_address = bmp_write_line(pages[page], y);
      bmp_write8 (write_address + x, colour); /* Set pixel */
    }
  }
  bmp_unwrite_line (pages[page]);
}

/*
 defobject.
 This sub defines an animated object.
 args: A HANDLE of a sprite and the xstart and ystart coords.
 ret : HANDLE if free slot found or else ...  have a guess.
*/

int defobject(int sprite, int x, int y, int flags)
{
  struct objstrc *s;
  int nobjs;
  int dir;

  int halves_tested = 0;

start:

  if (halves_tested == 2)
    return -1;

  /* Depending on the priority bit, we either start in the middle and look
     up for an empty slot, or start in the middle and look down. */
  s = (struct objstrc *) _obj + (MAXOBJS/2);

  dir = -1;
  if (flags & OBJ_HIGH) {
    dir = 1;
  }

  nobjs = MAXOBJS/2;
  while (nobjs--) {
    if (s->active == FALSE) {
      s->sprite = sprite;

      struct lowspr *ls = &_sprite[s->sprite];

      s->xs = ls->xs;
      s->ys = ls->ys;
      s->maxn = ls->maxn;
      s->nadd = ls->nadd;
      s->n = 0;

      s->x = x;
      s->xa = x;
      s->xb = x;

      s->y = y;
      s->ya = y;
      s->yb = y;

      s->cycle = (flags & OBJ_ONECYCLE);
      s->flags = 0;

      s->destroy = 0;
      s->active = TRUE;
      
      return s - _obj;
    }
    s += dir;
  }

  flags ^= OBJ_HIGH; /* Invert priority bit. */
  halves_tested++;
  goto start;
}

/* Moves the OBJECT 'obj' to the given location.
   RETURNS: 0 if sprite cycled, non-zero otherwise. (what does that
   mean??) */
int moveobject(int obj, int x, int y)
{
  _obj [obj].x = x;
  _obj [obj].y = y;
}

/* Moves the OBJECT 'obj' relative by the given values. */
int moveobjectdelta(int obj, int deltax, int deltay)
{
  _obj [obj].x += deltax;
  _obj [obj].y += deltay;
}

/* Set the object's flash bit. This will paint it
   in the current flashcolor for one frame. */
void flash(int obj)
{
  _obj[obj].flags |= O_FLASH;
}

/* Change the object's look. */
int changesprite(short obj, short sprite)
{
  _obj[obj].sprite = sprite;
  _obj[obj].maxn = _sprite[sprite].maxn;
  _obj[obj].n = 0;
  _obj[obj].xs = _sprite[sprite].xs;
  _obj[obj].ys = _sprite[sprite].ys;

  /* Probably not used. */
  return _sprite[sprite].xs;
}

/* Kills object and removes it from both screen pages during next
   two calls to 'updatescreen'. */
void abandonobject(int obj)
{
  _obj[obj].destroy = 2;
}

/* Kills object and imediately removes it from both screen pages. */
void killobject(int obj)
{
  removesprite (_obj[obj].sprite, _obj[obj].x, _obj[obj].y);
  page ^= 1;
  removesprite (_obj[obj].sprite, _obj[obj].x, _obj[obj].y);
  page ^= 1;

  _obj[obj].active = FALSE;
}

void killallobjects(void)
{
  struct objstrc *s = _obj;
  int ctr = MAXOBJS;

  while (ctr--) {
    s->active = FALSE;
    s++;
  }
}

/* This procy tests whether the given objects 'obj1' and 'obj2'
   are overlapping each other. If so 1 will be returned, otherwise 0. */
int crashtest(int obj1, int obj2)
{
  struct objstrc *o1, *o2;

  o1 = &_obj[obj1]; /* was si */
  o2 = &_obj[obj2]; /* was di */

  /* Check x coords. */

  if (   o1->x + o1->xs > o2->x
      && o1->x          < o2->x + o2->xs
      && o1->y + o1->ys > o2->y
      && o1->y          < o2->y + o2->ys)
    return 1; /* H I T !!!!! */

  return 0;
}

/* Checks whether the object is out of the currently defined
   action window. */
int outofwindow(int obj)
{
  if (   _obj[obj].x < windowx0
      || _obj[obj].x > windowx1
      || _obj[obj].y < windowy0
      || _obj[obj].y > windowy1)
    return 1;

  return 0;
}

/* Display one of the two pages (p==0 or p==1) */
void showpage(int p)
{
  //show_video_bitmap (pages[p]);
  stretch_blit (full_pages[p], screen,
                0, 0, full_pages[p]->w, full_pages[p]->h,
                0, 0, screen->w, screen->h);
}

/* Draw spr at (xp, yp). n may be a frame number. */
void putspritedirect (struct sprstrc *spr, int xp, int yp, int n)
{
  int width, height, ofs;

  int x,y,p;
  unsigned char *data;
  unsigned char *datap;

  int write_address;

  /* Point at struct sprstrc representing the frame. */

  width = spr->xs;
  height = spr->ys;
  data = spr->data + n * (width * height);

  for (p = 0; p <= 1; p++) {
    datap = data;
    bmp_select(full_pages[p]);
    for (y = 0; y < height; y++) {
      write_address = bmp_write_line(full_pages[p], yp + y);
      for (x = 0; x < width; x++) {
        bmp_write8 (write_address + xp + x, *datap);
        datap++;
      }
    }
    bmp_unwrite_line (full_pages[p]);
  }

}

void putsprite(int sprite, int x, int y, int n)
{
  BITMAP *s = _sprite[sprite].bmp[n];

  masked_blit(s, pages[page], 0, 0, x, y, s->w, s->h);
}

static putflash (int handle, int x, int y, int n)
{
  BITMAP *s = _sprite[handle].bmp_silhouette[n];

  masked_blit(s, pages[page], 0, 0, x, y, s->w, s->h);
}

void removesprite(int sprite, int x, int y)
{
  BITMAP *s = _sprite[sprite].bmp[0];

  rectfill (pages[page],
            x, y,
            x + s->w - 1, y + s->h - 1, backgrndcolor);
}

/* updatescreen.
   updatescreen removes the old objects from the backscreen and
   redraws them at the latest positions. */
void updatescreen(void)
{
  page ^= 1;
  
  starfield();

  struct objstrc *obj = _obj; /* was si */
  int nleft = MAXOBJS; /* was bp */

  /* Remove old objects from backscreen */
  do {
    if (obj->active) {
        int nextn;

        if (subtick == 0) {
            nextn = obj->n + obj->nadd;
            if (nextn >= obj->maxn) {
                if (obj->cycle != 0) {
                    obj->destroy = 2;
                    obj->cycle = 2;
                }
                nextn = 0;
            }
            obj->n = nextn;
        }

      removesprite (obj->sprite, obj->xb, obj->yb);
      if (obj->destroy != 0) {
        obj->destroy--;
        if (obj->destroy == 0) {
          obj->active = FALSE;
        }
      }
    }
    obj++;
  } while (--nleft);

  obj = _obj;
  nleft = MAXOBJS;

  do {
    if (obj->active) {
      obj->xb = obj->xa;
      obj->yb = obj->ya;
      obj->xa = obj->x;
      obj->ya = obj->y;

      if (obj->destroy == 0) {
        if (obj->flags & O_FLASH) {
          copycolor (FLASH_COLOUR, objflashcolor);
          putflash (obj->sprite, obj->x, obj->y, obj->n/2);
          obj->flags &= ~O_FLASH; /* Clear flag */
        } else {
          putsprite (obj->sprite, obj->x, obj->y, obj->n/2);
        }
      }
    }
    obj++;
  } while (--nleft);

  showpage (page);
}

/* copypage: Copies a screen page to another.
             scr: source page (0, or 1)
             dst: destination page (0 or 1) */
void copypage(int src, int dst)
{
  if (src == dst) return;

  blit (full_pages[src], full_pages[dst], 0, 0, 0, 0,
        full_pages[dst]->w, full_pages[dst]->h);
  return;
}

PALETTE tmp_palette;

/* Slowly fades out the screen. */
void glowout()
{
  int i, c;

  showpage(page);

  for (i = 0; i <= 63; i += 6) {
    for (c = 0; c <= 255; c++) {
      RGB rgb;

      rgb.r = max(palette[c].r - i, 0);
      rgb.g = max(palette[c].g - i, 0);
      rgb.b = max(palette[c].b - i, 0);

      tmp_palette[c] = rgb;
    }
    set_palette (tmp_palette);
  }
}

void glowin (int dir)
{
  int i, c;

  for (c = 0; c <= 255; c++) {
    RGB rgb;

    rgb.r = 0;
    rgb.g = 0;
    rgb.b = 0;
    
    tmp_palette[c] = rgb;
  }
  set_palette(tmp_palette);

  showpage(page);

  for (i = 62; i >= 0; i -= 6) {
    for (c = 0; c <= 255; c++) {
      RGB rgb;

      rgb.r = max(palette[c].r - i, 0);
      rgb.g = max(palette[c].g - i, 0);
      rgb.b = max(palette[c].b - i, 0);

      tmp_palette[c] = rgb;
    }
    set_palette(tmp_palette);
  }

  set_palette(palette);
}
