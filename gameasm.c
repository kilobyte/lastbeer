/* Allegro port Copyright (C) 2014 Gavin Smith. */
#define MAIN_MODULE
#include "baller.h"
#include "xmode.h"

/*  Structure to save a complete position during the game.
    This is used to get back to a 'try again' point if the
    bottle broke. */

struct lastposstrc {
  int score;                /* Player's score. */
  short nattacks;           /* Attacks to go. */
  struct attackstrc *attack;              /* ptr to attacks. */
  short nbigboss;           /* N of Big Bosses to go. */
  short count;              /* Frame counter. */
};

struct lastposstrc lastposition;

short invincible; /* Is Eichli invincible? */
short frameinc;   /* Frame count increment step. */
short nbigboss;  /* Number of endlevel monsters that
                    must still be destroyed. */

/* Variables used to draw the score. */
short scoredigit;               /* # of digits already drawn. */
short scorepos;                 /* Current x position. */
int playscore;                /* Remaining score. */

short const10d;               /* Faster Division. */

/* Function to access field in starstrc array. */
int starstrc_member (unsigned char *data, int i, int member)
{
  data += i * STARSTRC_SIZE;
  return data[member] + (data[member + 1] << 8);
}

void set_starstrc_member (unsigned char *data, int i, int member,
                         unsigned short int value)
{
  data += i * STARSTRC_SIZE;
  data[member] = value & 0x00FF;
  data[member + 1] = (value & 0xFF00) >> 8;
}

/*
 defarm: Define a friendly object.
          Arguments are: pointer to a 'armstrc'.
                        x and y as a start location. */
int defarm (int x, int y, struct armstrc *arm)
{
  struct aniarm *a = _arm;
  int narms = MAXARMS;

  /* Get free arm slot. */
  while (narms--) {
    if (a->object == -1) {
      a->flags = arm->flags;
      a->shot = arm->shot;
      a->period = arm->period;
      a->periodcnt = 0;
      a->object = defobject (arm->sprite, x, y, OBJ_HIGH);

      return a->object;
    }
    a++;
  }
}

/* defshot: Define a shot.
   Arguments:
     shot:      Which shot should we define?
     x, y:      Define coordinates.
     addr:      addr of shot, that defines this shot. */
int defshot (int shot, int x, int y, struct anishot *addr)
{
  int ctr = MAXSHOTS;
  struct anishot *ptr = _shot;
  int ss_off;
  struct shotstrc *ss;

  do {
    if (ptr->object == -1) break;
    ptr++;
  } while (--ctr);

  /* If empty slot not found */
  if (ptr - (struct anishot *) &_shot == MAXSHOTS) return;

  /* Really don't know why we do this. Only done when shots are releasing
     other shots. */
  ptr->go = (ptr <= addr) ? 1 : 0;

  ss_off = weapon.shot[shot];
  ss = (struct shotstrc *) ((char *) weapon.shotfile + 2 + ss_off);
  //ss = ptrindex[shot * 2];

  /* Note: ss->data is an array, so we are pointing to the first element. */
  ptr->data = ss->data;
  ptr->power = ss->power;
  ptr->speed = ss->speed;

  return ptr->object = defobject(intindex[ss->sprite],
                           x + ss->shotx, y + ss->shoty, OBJ_LOW);

}

/* This one defines any kind of foe. (Path-followers, Shots, ...). */
void deffoe (short foe, short x, short y, short x0, short y0)
{
  int ctr = MAXFOES;
  struct anifoe *ptr = &_foe[0];
  int fs_off;
  struct foestrc *fs;

  /* Scan for a free enemy slot. */
  do {
    if (ptr->object == -1) break;
    ptr++;
  } while (--ctr);
  
  if (ctr == 0)
    return; /* FIXME: return what? */

  /* lfoeofs is "level foe offset" */
  fs = ptrindex[lfoeofs + foe];

  /* Copy flags and check type of foe. */

  ptr->flags = fs->flags;
  if (ptr->flags & FOE_STOPCOUNT)
    frameinc = 0;

  if (!(ptr->flags & (FOE_TRANSPARENT | FOE_INVINCIBLE))) {
    /* Foe can explode. */
    ptr->shield = fs->shield;
    ptr->score = fs->score;
    ptr->expl = fs->expl;
  }

  if (ptr->flags & FOE_PATH) {
    /* Foe follows a predefined path. */
    ptr->cpath = fs->path;
  }

  if (ptr->flags & FOE_LINE) {
    int our_x0;
    int our_y0;

    int our_z0;
    int our_z1;

    /* Foe uses a runtime defined path (line mode). */
    ptr->speed = fs->speed;

    /* Calculate line draw variables z0, z1, dx, dy, dz. */

    our_x0 = x0; /* Destination */
    our_y0 = y0;

    if (!(ptr->flags & FOE_PATH)) {
      our_x0 = _obj[_arm[0].object].xa; /* Location of player's ship. */
      our_y0 = _obj[_arm[0].object].ya;
    }

    our_z0 = 1;
    our_x0 -= x;

    if (our_x0 < 0) {
      our_x0 *= -1; /* Must be positive. */
      our_z0 *= -1;
    }

    ptr->dx = our_x0; /* Horizontal distance to destination. */
    ptr->z0 = our_z0;

    our_z1 = 1;
    our_y0 -= y;
    if (our_y0 < 0) {
      our_y0 *= -1;
      our_z1 *= -1;
    }

    ptr->dy = our_y0;
    ptr->z1 = our_z1;

    ptr->pixelcnt = max(our_x0, our_y0);
    ptr->dz = our_x0 / 2;
  }

  /* lsprofs == "level sprite offset" */
  ptr->object = defobject(intindex[lsprofs + fs->sprite],
                                 x, y, OBJ_LOW);
}

void defexpl (int obj, int explosion)
{
  struct aniexpl *ptr = _expl;
  int ctr = MAXEXPLS;
  
  do {
    if (ptr->object == -1)
      break;
  } while (ptr++, --ctr);

  if (ctr == 0)
    return;

  ptr->object = obj;
  ptr->data = ptrindex [lexplofs + explosion];
  ptr->x = _obj[obj].xa + _obj[obj].xs / 2;
  ptr->y = _obj[obj].ya + _obj[obj].ys / 2;
}

void a_arm (int x, int y)
{
  int ctr;
  int obj_idx;
  struct objstrc *obj;

  obj_idx = _arm[0].object;
  obj = &_obj[obj_idx];

  if (invincible & 0x0002)
    flash(obj_idx);

  /* Check whether main bottle is off the play field. */
  if (obj->xa + x < windowx0) {
    x = windowx0 - obj->xa;
    /* Now obj->xa + x == windowx0 */
  }
  if (obj->xa + obj->xs + x > windowx1) {
    x = windowx1 - obj->xa - obj->xs;
  }
  if (obj->ya + y < windowy0) {
    y = windowy0 - obj->ya;
  }
  if (obj->ya + obj->ys + y > windowy1) {
    y = windowy1 - obj->ya - obj->ys;
  }

  ctr = MAXARMS - 1;
  do {
    if (_arm[ctr].object != -1) 
      moveobjectdelta(_arm[ctr].object, x, y);
  } while (ctr--);
}

void a_shot (void)
{
  int x, y;

  unsigned short cmd;
  int ctr;
  struct anishot *ptr;

  ctr = MAXSHOTS;
  ptr = _shot;
  
  for (; ctr > 0 ; ptr++, ctr-- ) {
    if (ptr->object == -1) continue;
    
    if (ptr->go != 1)
      {
        ptr->go = 1;
        continue;
      }
  nextcoord:
      cmd = *(ptr->data);

      if ((cmd & 0xfff0) != 0x8000) {
        short our_dx, our_dy;
        ptr->dx = (signed short) ptr->data[0];
        ptr->dy = (signed short) ptr->data[1];

        our_dx = smooth_move (ptr->dx, subtick);
        our_dy = smooth_move (ptr->dy, subtick);
        moveobjectdelta (ptr->object, our_dx, our_dy);

        if (subtick != 0)
          continue;
        ptr->dx = our_dx;
        ptr->dy = our_dy;
        ptr->data += 2;

      } else switch (cmd) {
      case SHOTEND:
        abandonobject (ptr->object);
        ptr->object = -1;
        break;
      case SHOTRELEASE:
        if (subtick != 0)
          continue;
        defshot (ptr->data[1], _obj[ptr->object].xa, _obj[ptr->object].ya, ptr);
        ptr->data += 2;
        goto nextcoord;
      case SHOTHOMING:
        {
        struct anifoe *f = _foe;
        int fctr = MAXFOES;
        int x, y, dx, dy;

        do {
          if (f->object == -1) continue;
          if (f->flags & (FOE_INVINCIBLE | FOE_TRANSPARENT)) continue;
          break;
        } while (f++, --fctr);

        if (fctr == 0) {
          /* No eligible foe found */
          moveobjectdelta (ptr->object, ptr->dx, ptr->dy);
                          //smooth_move (ptr->dx, subtick),
                          //smooth_move (ptr->dy, subtick));
          break;
        }

        x = _obj[f->object].xa;
        y = _obj[f->object].ya;
        
        /* Get mid point of object. */
        x += _obj[f->object].xs / 2;
        y += _obj[f->object].ys / 2;
        
        x -= _obj[ptr->object].xa;
        y -= _obj[ptr->object].ya;
       
        if (abs(x) > abs(y)) {
          y /= abs(x) / ptr->speed + 1;

          if (x < 0)
            x = -ptr->speed;
          else
            x = ptr->speed;
        } else {
          x /= abs(y) / ptr->speed + 1;

          if (y < 0)
            y = -ptr->speed;
          else
            y = ptr->speed;
        }
        
        dx = ptr->dx;
        dy = ptr->dy;

        if (x <= dx)
          dx--;
        else
          dx++;

        if (y <= dy)
          dy--;
        else
          dy++;

        if (subtick == 0)
          {
            ptr->dx = dx;
            ptr->dy = dy;
          }
        moveobjectdelta (ptr->object, smooth_move (dx, subtick),
                         smooth_move (dy, subtick));
        }
        break;
      case SHOTREFLECT:
        {
        int x, y, xs, ys;
        x = _obj[ptr->object].xa;
        y = _obj[ptr->object].ya;
        xs = _obj[ptr->object].xs;
        ys = _obj[ptr->object].ys;

        x += ptr->dx;

        /* Shot reflects at left border. */
        if (x < windowx0) {
          ptr->dx *= -1; /* Change direction */
          x += ptr->dx;
          x += ptr->dx;
        /* Shot reflects at right border. */
        } else if (x + xs > windowx1) {
          ptr->dx *= -1; /* Change direction */
          x += ptr->dx;
          x += ptr->dx;
        }

        y += ptr->dy;

        /* Shot reflects at top border. */
        if (y < windowy0) {
          ptr->dy *= -1; /* Change direction */
          y += ptr->dy;
          y += ptr->dy;
        /* Shot reflects at bottom border. */
        } else if (y + ys > windowy1) {
          ptr->dy *= -1; /* Change direction */
          y += ptr->dy;
          y += ptr->dy;
        }

        moveobject (ptr->object, x, y);

        if (subtick != 0)
          continue;
        ptr->data++;
        break;
        }
      } /* switch */

    if (outofwindow (ptr->object)) {
      abandonobject (ptr->object);
      ptr->object = -1;
    }
  }
}

/* This proc interprets the next command of
   enemy in his path and performs proper
   action. */
void a_foe (void)
{
  unsigned short cmd;
  int ctr;
  struct anifoe *ptr;

  ptr = &_foe[0]; /* was si */
  ctr = MAXFOES; /* was bp */
        
  do {
    if (ptr->object == -1)
      continue;

    if (ptr->flags & FOE_LINE) {
      void a_foeline (struct anifoe *ptr);
      a_foeline(ptr);
      continue;
    }

    do {
      /* Sometimes, we execute the next command right away instead of
         going to the next foe. */
    nextcoord: 

      /* Interpret the command. */
      if (((unsigned short) *ptr->cpath & 0xfff0) != A_COMMAND) {
        moveobjectdelta (ptr->object,
                         smooth_move (ptr->cpath[0], subtick),
                         smooth_move (ptr->cpath[1], subtick));
        if (subtick == 0)
          ptr->cpath += 2;
      } else switch ((unsigned short) *ptr->cpath) { 
      case FOEENDPATH:
        /* Object has reached end of path and must be destroyed. */
        abandonobject (ptr->object);
        ptr->object = -1;
        break;
      case FOECHANGESPRITE:
        /* Object wants to change it's look. */
        changesprite (ptr->object, intindex[lsprofs + ptr->cpath[1]]);
        ptr->cpath += 2;
        goto nextcoord;
      case FOERELEASEFOE:
        /* Object releases another object. */
        deffoe (ptr->cpath[1],
                _obj[ptr->object].xa + ptr->cpath[2],
                _obj[ptr->object].ya + ptr->cpath[3],
                0, 0);
        ptr->cpath += 4;
        goto nextcoord;
      case FOECYCLEPATH:
        ptr->cpath = ptr->path;
        moveobject (ptr->object, ptr->savex, ptr->savey);
        goto nextcoord;
      case FOEMARK:
        ptr->path = ++ptr->cpath; 
        ptr->savex = _obj[ptr->object].xa;
        ptr->savey = _obj[ptr->object].ya;
        goto nextcoord;
      case FOESOUND:
        playsample (ptrindex[lsndofs + ptr->cpath[1]]);
        ptr->cpath += 2;
        goto nextcoord;
      }
    } while (0);
  } while(ptr++, --ctr);
}

void a_foeline (struct anifoe *ptr)
{
  int our_dz = ptr->dz;
  int our_x = 0;
  int our_y = 0;

  int ctr = ptr->speed;
  do {
    if (our_dz < ptr->dx) {
      our_dz += ptr->dy;
      our_x += ptr->z0;
    }

    if (our_dz >= ptr->dx) {
      our_dz -= ptr->dx;
      our_y += ptr->z1;
    }

    if (ptr->flags & FOE_PATH) {
      if (subtick == 0
          && 0 == --ptr->pixelcnt) {
        /* Target coordinates have been reached. */
        ptr->flags &= ~FOE_LINE;
        our_x += _obj[ptr->object].xa;
        our_y += _obj[ptr->object].ya;
        moveobject (ptr->object, our_x, our_y);
        return;
      }
    }
  } while (--ctr);

  if (subtick == 0)
    ptr->dz = our_dz;

  our_x = smooth_move (our_x, subtick);
  our_y = smooth_move (our_y, subtick);

  our_x += _obj[ptr->object].xa;
  our_y += _obj[ptr->object].ya;

  moveobject (ptr->object, our_x, our_y);

  if (outofwindow(ptr->object)) {
    abandonobject (ptr->object);
    ptr->object = -1;
  }
}

void a_expl (void)
{
  int ctr = MAXEXPLS;
  struct aniexpl *ptr = _expl;

  do {
    unsigned short int cmd;

    if (ptr->object == -1)
      continue;

  nextcommand:
    cmd = *ptr->data;

    switch (cmd) {
    case EXPLEND:
      ptr->object = -1;
      break;
    case EXPLNEW:
      defobject (intindex[lsprofs + ptr->data[1]],
                 ptr->x + (signed short) ptr->data[2],
                 ptr->y + (signed short) ptr->data[3],
                 OBJ_HIGH | OBJ_ONECYCLE);
      ptr->data += 4;
      goto nextcommand;
    case EXPLWAIT:
      ptr->data++;
      break;
    case EXPLSOUND:
      playsample(ptrindex[lsndofs + ptr->data[1]]);
      ptr->data++;
      goto nextcommand;
    case EXPLREMOVEOBJ:
      abandonobject (ptr->object);
      ptr->data++;
      goto nextcommand;
    case EXPLRELEASEFOE:
      deffoe (ptr->data[1],
              ptr->x + (signed short) ptr->data[2],
              ptr->y + (signed short) ptr->data[3],
              0, 0); /* No arguments here in original */
      ptr->data += 4;
      goto nextcommand;
    case EXPLNEWPATH:
      deffoe (ptr->data[1],
              ptr->x + (signed short) ptr->data[2],
              ptr->y + (signed short) ptr->data[3],
              ptr->data[4],
              ptr->data[5]);
      ptr->data += 6;
      goto nextcommand;
    default:
      ptr->data++;
      break;
    }

  } while (ptr++, --ctr);
}

void foehit (void)
{
  struct anifoe *ptr = _foe;
  int ctr = MAXFOES;

  struct anishot *s;
  int sctr;

  do {
    if (ptr->object == -1 || (ptr->flags & FOE_TRANSPARENT))
      continue;

    s = _shot;
    sctr = MAXSHOTS;
    do {
      if (s->object == -1)
        continue;
      if (crashtest (ptr->object, s->object)) {
        if (ptr->flags & FOE_INVINCIBLE)
          s->power = 0; /* Kill shot */
        else {
          int tmp_power;

          flash (ptr->object);

          tmp_power = s->power;
          s->power -= ptr->shield;
          ptr->shield -= tmp_power;

          if (ptr->shield <= 0) {
            score += ptr->score;
            defexpl (ptr->object, ptr->expl);

            /* Forget about object - it's controlled by a_expl now */
            ptr->object = -1; 

            /* Was it a big boss? */
            if (ptr->flags & FOE_ENDLEVEL) {
              nbigboss--;
            }

            /* Check for stopped counter */
            if (ptr->flags & FOE_STOPCOUNT) {
              frameinc = 1;
            }
          }
        }

        if (s->power <= 0) {
          abandonobject (s->object);
          s->object = -1;
        }
      }
    } while (s++, --sctr);
  } while (ptr++, --ctr);
}


/* checks whether the heroic bottle of cool fresh Eichhof
   Lager has been hit by a nasty enemy beer. 
   RETURNS: 1 if hit. */
int armhit (void)
{
  int ctr, ctr2;
  struct anifoe *ptr;
  struct aniarm *a;

  if (invincible)
    return 0;

  /* First we check whether Eichli has been hit by a hostile shot. */

  ptr = _foe;
  ctr = MAXFOES;

  do {
    if (ptr->object == -1)
      continue;

    if (!crashtest (ptr->object, _arm[0].object))
      continue;

    /* Eichli has been hit */

    /* Do a beautiful explosion. */
    defexpl (_arm[0].object, 0);

    /* Destroy all weapons. */
    a = _arm;
    ctr2 = MAXARMS;

    do {
      if (a->object != -1) {
        abandonobject (a->object);
        a->object = -1;
      }
    } while (a++, --ctr2);

    /* Indiacte destroyed Eichli. */
    return 1;

  } while (ptr++, --ctr);

  return 0;
}

/* Voll power aus allen Rohren.
   what == TRUE  : es soll geschossen werden.
   what == FALSE : Friede. Kein schuss soll fallen. */

void fire (int what)
{
  int ctr = MAXARMS;
  struct aniarm *ptr = _arm;

  if (what) goto fire_label;

  do {
    if (ptr->object != -1) {
      if (ptr->periodcnt != 0) { /* Gun not already reloaded */
        ptr->periodcnt--; /* Decrement reload delay. */ 
      }
    }
    ptr++;
  } while (--ctr);

  return;

fire_label:
  do {
    if (ptr->object != -1) {
      if (!ptr->periodcnt--) {
        ptr->periodcnt = ptr->period;
        defshot (ptr->shot, 
                 _obj[ptr->object].xa,
                 _obj[ptr->object].ya,
                 (struct anishot *) -1);
      }
    }

    ptr++;
  } while (--ctr);
}

void keyboard (void)
{
  if (subtick == 0)
    fire (key[key_fire]);

  int xchange = 0, ychange = 0;

  if (key[KEY_ALT] && key[KEY_ENTER])
    toggle_fullscreen ();

  if (key[key_left])
    xchange -= shipspeed;
  if (key[key_right])
    xchange += shipspeed;
  if (key[key_up])
    ychange -= shipspeed;
  if (key[key_down])
    ychange += shipspeed;

  xchange /= RESOLUTION;
  ychange /= RESOLUTION;

  if (subtick % RESOLUTION < shipspeed % RESOLUTION)
    {
      if (xchange > 0)
        xchange++;
      if (xchange < 0)
        xchange--;
      if (ychange > 0)
        ychange++;
      if (ychange < 0)
        ychange--;
    }

  if (key[key_pause]) {
    /* Wait for pause key to be released. */
    while (key[key_pause]);

    /* Wait for key to be pressed. */
    clear_keybuf();
    while (!keypressed());

    /* If the player pressed the pause key to unpause, wait until the key
       is lifted to avoid pausing again. */
    while (key[key_pause]);
  }
  a_arm(xchange, ychange);
}

/* Return 0 on subtick, 1 on main tick. */
int waitforsubtick (void)
{
  while (!tick) {
      rest (10);
#if 1
    if (key[KEY_TAB])
      tick = 1;
#endif
  }
  tick = 0;
  if (subtick_ == 0)
    return 1;
  return 0;
}

void waitfortick (void)
{
  while (!(tick && subtick_ == 0))
    rest (10);
  tick = 0;
}

void settick (void)
{
  updates_due++;
  subtick_++;
  if (subtick_ >= RESOLUTION)
    subtick_ = 0;
  tick = 1;
}
END_OF_FUNCTION (settick)

/* set a starting position of a level. */
void setplayposition (short nattacks, struct attackstrc *attack, short nbigb)
{
  lastposition.nattacks = nattacks;
  lastposition.attack = attack;
  lastposition.nbigboss = nbigb;
  
  lastposition.score = score;
  lastposition.count = 0;
}

int play()
{
  short terminate, nattacks;

  short count;
  struct attackstrc *attack;

  score = lastposition.score;
  nbigboss = lastposition.nbigboss;
  nattacks = lastposition.nattacks;

  count =  lastposition.count;
  attack = lastposition.attack;
  
  frameinc = 1;
  scoredigit = 0;

  invincible = SHIELD_CTIME; /* First Eichli is invincible. */
  terminate = 0; /* No reason to terminate. */
  updates_due = 0;
  subtick = 0;
   
  do {
    next_attack:
    if (nattacks != 0 && count == attack->count) {
      if (((unsigned short) attack->foe & 0xfff0) != A_COMMAND) {
        deffoe(attack->foe, attack->x, attack->y, 0, 0);
        nattacks--;
        attack++;
        goto next_attack;
      } else switch ((unsigned short) attack->foe) {
      case A_GOFIELD:
        gostarfield();
        attack++;
        goto next_attack;
      case A_STOPFIELD:
        stopstarfield();
        attack++;
        goto next_attack;
      case A_SOUND:
        playsample (ptrindex[lsndofs + attack->x]);
        attack++;
        goto next_attack;
      case A_MARK:
        lastposition.attack = attack;
        lastposition.count = count;

        lastposition.nattacks = nattacks;
        lastposition.nbigboss = nbigboss;
        lastposition.score = score;
        attack++;
        goto next_attack;
      }
    }

    /* ESC key test. */
    if (key[KEY_ESC]) {
      lifes = -1; /* Request game abort. */
      break;
    }

    do
      {
        waitforsubtick ();

        updatescreen();
        if (terminate != 2)
          keyboard();
    
        foehit();

        if (nbigboss == 0) {
            nbigboss = 1;
            invincible = WIN_CTIME;
            terminate = 1; /* All Endlevel monsters destroyed. */
        }

        if (!(cheatlevel & CHEATCRASH) && !key[KEY_TAB] && armhit()) {
            invincible = LOSE_CTIME;
            terminate = 2; /* Eichli destroyed. */
        }

        dispscore();
        a_shot();
        a_foe();
        subtick++;
        if (subtick == RESOLUTION)
          subtick = 0;

      }
    while (subtick != 0);


    a_expl();

    count += frameinc; /* Inrement frame count. */
    if (invincible) invincible--;
  } while ((invincible != 0) || terminate == 0);

  haltsound();

  if (terminate == 1) {
    return 1; /* next level */
  } else {
    return 0;
  }

}

