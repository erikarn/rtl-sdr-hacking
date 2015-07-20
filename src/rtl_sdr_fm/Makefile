
PROG=main
SRCS=fm_demod.c fm_output.c fm_dongle.c fm_main.c convenience.c
NO_MAN=1
LDADD=-lSDL -lGL -lGLU -lfftw3 -lm -lpthread -lrtlsdr
CFLAGS+=	-I/usr/local/include/
LDFLAGS+=	-L/usr/local/lib/

.include <bsd.prog.mk>

