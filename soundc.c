/* Allegro port Copyright (C) 2014 Gavin Smith. */
#include "sound.h"
#include <stdio.h>

int soundon = 1;

/*speaker:
   state == 0  : Turn sound off.
   state == 1  : Turn sound on.
   state == -1 : Query sound. */

int speaker(int state)
{
  if (state == -1)
    return soundon;

  soundon = state;
  if (!soundon)
    remove_sound ();
  return soundon;
}

static SAMPLE *playsample_ext (struct sndstrc *s, int loop);

void shutsound (void)
{
  /* Nothing yet. */
}

/* Stop all playing sounds. */
void haltsound(void)
{
  remove_sound();
  if (soundon)
    install_sound(DIGI_AUTODETECT, MIDI_NONE, 0);
}

void playsample (SAMPLE *s)
{
  play_sample (s, 255, 128, 1000, 0);
}

void playloop (SAMPLE *s)
{
  play_sample (s, 255, 128, 1000, 1);
}


/* Returns a pointer to SAMPLE. This value should be
   freed when no longer required. destroy_sample must
   not be called, because this attempts to free the data
   field of SAMPLE. */
SAMPLE *create_SAMPLE (struct sndstrc *s)
{
  struct SAMPLE *sample = malloc (sizeof (struct SAMPLE));

  sample->bits = 8;
  sample->stereo = 0;
  sample->freq = s->samplerate * 1000;
  sample->priority = s->priority;
  sample->len = s->len;
  sample->loop_start = 0;
  sample->data = s->data;

  /* This is something called 8 to 4 bit ADPCM */
  /* Decoding algorithm taken from
  /* http://wiki.multimedia.cx/index.php?title=Creative_8_bits_ADPCM */
  if (s->flags & SND_PACKED4)
    {
      unsigned char *new_sample = malloc(1 + 2 * (sample->len-1));
      int byte;
      unsigned char value;
      int i;

      int step, shift, limit, sign;
      step = 0;
      shift = 0;
      limit = 5;

      byte = s->data[0];
      new_sample[0] = byte;

      for (i = 1; i < sample->len; i++) {
        value = (s->data[i] & 0xf0) >> 4;

        sign = (value & 0x08) ? -1 : 1; /* Test bit 3 */
        value &= 0x07; /* Clear bit 3 */

        byte += sign * (value << (step + shift));
        if (byte > 0xff)
          byte = 0xff;
        if (byte < 0x00)
          byte = 0x00;

        new_sample[2*i] = byte;

        if (value >= limit)
          step++;
        else if (value == 0)
          step--;
        if (step > 3)
          step = 3;
        if (step < 0) 
          step = 0;

        /*********************************************/

        /* Exactly the same, but for the second nybble */
        value = (s->data[i] & 0x0f);

        sign = (value & 0x08) ? -1 : 1; /* Test bit 3 */
        value &= 0x07; /* Clear bit 3 */

        byte += sign * (value << (step + shift));
        if (byte > 0xff)
          byte = 0xff;
        if (byte < 0x00)
          byte = 0x00;
        new_sample[2*i + 1] = byte;

        if (value >= limit)
          step++;
        else if (value == 0)
          step--;
        if (step > 3)
          step = 3;
        if (step < 0)
          step = 0;
      }

      sample->data = new_sample;
      sample->len = 1 + 2 * (sample->len - 1);

    }

  sample->loop_end = sample->len - 1;

  return sample;
}

/* Initialize s with the data read from the file.*/
void create_sndstrc (struct sndstrc *s, unsigned char *data)
{
  s->priority   = data[0] + (data[1] << 8);
  s->samplerate = data[2] + (data[3] << 8);
  s->flags = data[4] + (data[5] << 8);
  s->len = data[6] + (data[7] << 8) + (data[8] << 16)
           + (data[9] << 24);
  s->data = data + 10;
}

