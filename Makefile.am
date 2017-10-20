AUTOMAKE_OPTIONS = foreign

beer_CPPFLAGS = \
  -DLOCALSTATEDIR=\"$(localstatedir)\" \
  -DDATADIR=\"$(datadir)\"

bin_PROGRAMS = beer
beer_SOURCES = fileman.c fileman.h xmode.h xmodeasm.c xmodec.c \
		 xmodedef.c soundc.c sound.h gameasm.c baller.h \
		 support.c gameplay.c shop.c menu.c baller.c hiscore.c \
		 intro.c

dist_localstate_DATA = CONFIG.HIG
dist_data_DATA = BEER.DAT

EXTRA_DIST = README LICENSE.GPL

beer_CFLAGS = `allegro-config --cflags`
beer_LDFLAGS = `allegro-config --libs`
beer_CPPFLAGS += `allegro-config --cppflags`
