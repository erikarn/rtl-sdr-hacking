
PROG=main
SRCS=fm_demod.c fm_output.c fm_dongle.c fm_main.c convenience.c fm_sdl.c
NO_MAN=1
LDADD=-lSDL -lGL -lGLU -lfftw3 -lm -lpthread -lrtlsdr
CFLAGS+=	-I/usr/local/include/ -O0 -g -ggdb
LDFLAGS+=	-L/usr/local/lib/

.include <bsd.prog.mk>

