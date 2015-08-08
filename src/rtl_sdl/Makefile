
PROG=main
SRCS=fm_dongle.c fm_main.c fm_sdl.c fm_fft.c
SRCS+=fm_fft_thread.c fm_controller.c
NO_MAN=1
LDADD=-lSDL -lGL -lGLU -lfftw3 -lm -lpthread -lrtlsdr -lstuff
CFLAGS+=	-I../lib/ -I/usr/local/include/ -g -ggdb
LDFLAGS+=	-L/usr/local/lib/ -L../lib/

.include <bsd.prog.mk>

