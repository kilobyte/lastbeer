/* Allegro port Copyright (C) 2014 Gavin Smith. */
#include <allegro.h>

// Header file for sound support module.


#define SND_PACKED4		0x0001
struct sndstrc {

   short    priority;			// Priority of sample.
   short    samplerate;			// Sample rate in kHz.
   short    flags;			// Raw or packed sample?
   long    len;		// Sample len in bytes.
   unsigned char   *data;		// Sampled data.

};

void create_sndstrc (struct sndstrc *s, unsigned char *data);
SAMPLE *create_SAMPLE (struct sndstrc *s);

int initsound(int snd_io, int snd_irq, int snd_dma);
void shutsound(void);
void playsample(SAMPLE *snd);
void playloop(SAMPLE *snd);
void playfile(int filvar, void *buffer, int bs);
void playfileloop(int filvar, void *buffer, int bs);
void haltsound(void);
int soundbusy(void);
int speaker(int state);

