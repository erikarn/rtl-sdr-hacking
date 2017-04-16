
PROG=main

SRCS=	rtl_fm.c convenience.c rotate_90.c
SRCS+=	output.c
SRCS+=	low_pass.c fifth_order.c generic_fir.c
SRCS+=	polar_disc.c

SRCS+=	output_file.c
SRCS+=	output_alsa.c

MK_MAN=no
LDADD=-lSDL -lGL -lGLU -lfftw3 -lm -lpthread -lrtlsdr -lasound
CFLAGS+=	-I/usr/local/include/ -g -ggdb -Wall
LDFLAGS+=	-L/usr/local/lib/

.include <bsd.prog.mk>

