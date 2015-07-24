
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <err.h>
#include <pthread.h>
#include <math.h>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <fftw3.h>

#include "fm_sdl.h"

static void
quit_tutorial(int code)
{
	SDL_Quit();
	exit(code);
}

static void
process_events(void)
{
	SDL_Event event;

	while(SDL_PollEvent(&event)) {

		switch(event.type) {
//		case SDL_KEYDOWN:
//			/* Handle key presses. */
//			handle_key_down( &event.key.keysym );
//			break;
		case SDL_QUIT:
			/* Handle quit requests (like Ctrl-c). */
			quit_tutorial( 0 );
			break;
		}
	}
}

#if 0
/*
 * Draw just the raw signal line - signal in the time domain.
 */
static void
draw_signal_line(struct mw_app *app)
{
	int x, y;
	int curofs;
	int size;

	/*
	 * Plot a line series for the last 320 elements,
	 * two pixel wide (640 pixels.)
	 */
	curofs = app->mw->raw_samples.offset - 1;
	size = app->mw->raw_samples.len;
	if (curofs < 0)
		curofs = size - 1;

	glBegin(GL_LINE_STRIP);
	glColor3f(1, 1, 0);
	for (x = 0; x < 320; x++) {
		/*
		 * Y axis is 480 high, so let's split it into
		 * +/- 240.  The dynamic range of the raw
		 * values is signed 16-bit value, so scale that.
		 */
		y = app->mw->raw_samples.s[curofs].sample;
		y = y * 240 / 1024;

		/* Clip */
		if (y < -240)
			y = -240;
		else if (y > 240)
			y = 240;

		/* Set offset to middle of the screen */
		y += 240;

		/* Go backwards in samples */
		curofs--;
		if (curofs < 0)
			curofs = size - 1;
		glVertex3f(x * 2, y, 0);
	}
	glEnd();
}
#endif

static float
fft_mag(float r, float i, float d)
{
	return (sqrtf(r*r + i*i) / d);
}

/*
 * Draw the FFT data out.
 */
static void
draw_fft_data(struct fm_sdl_state *fm)
{
	int size, i, j;
	float x, y;
	int mul = 256;
	int midpoint;

	/* XXX not locked? */
	if (fm->nsamples == 0)
		return;

	/*
	 * Map the FFT data into 'xsize', taking into account -ve and +ve
	 * frequency data (with DC in the middle.)
	 */
	glBegin(GL_LINE_STRIP);
	glColor3f(0.0, 1.0, 1.0);
	size = fm->scr_xsize;
	midpoint = size / 2;
	/*
	 * The fft_out bins are ordered 0..BW/2, then -BW/2..0.
	 * This should be done for the purposes of the plot!
	 */

	for (i = 0; i < size; i++) {
		/*
		 * If we're below midpoint, map 'i' into the -ve space.
		 * If we're at/above midpoint, map 'i' into the +ve space.
		 *
		 * Translate it into the right sample offset for the given
		 * position.  For now we just skip; we don't try to sum
		 * entries and plot the average/min/max.
		 * We should do that later.
		 */
		if (i < midpoint)
			j = (fm->nsamples / 2) + (i * fm->nsamples) / size;
		else
			j = ((i - midpoint) * fm->nsamples) / size;

		x = i;
		y = fft_mag(fm->fft_out[j][0], fm->fft_out[j][1], 128.0);
		/* y starts at the bottom */
		y = fm->scr_ysize - y;
		glVertex3f(x, y, 0);

	}
	glEnd();
}

void
fm_sdl_display_update(struct fm_sdl_state *fm)
{

    process_events();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    /*
     * let's draw three strips, for quality, attention and meditation
     */

#if 0
    /* Quality */
    glBegin(GL_QUADS);
     glColor3f(1, 0, 0);
     glVertex3f(0, 480, 0);
     glVertex3f(50, 480, 0);
     glVertex3f(50, 480 - app->quality, 0);
     glVertex3f(0, 480 - app->quality, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
     glColor3f(1, 1, 1);
     glVertex3f(0, 480, 0);
     glVertex3f(50, 480, 0);
     glVertex3f(50, 480 - 200, 0);
     glVertex3f(0, 480 - 200, 0);
     glVertex3f(0, 480, 0);
    glEnd();

    /* Meditation */
    glBegin(GL_QUADS);
     glColor3f(0, 1, 0);
     glVertex3f(100, 480, 0);
     glVertex3f(150, 480, 0);
     glVertex3f(150, 480 - app->meditation, 0);
     glVertex3f(100, 480 - app->meditation, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
     glColor3f(1, 1, 1);
     glVertex3f(100, 480, 0);
     glVertex3f(150, 480, 0);
     glVertex3f(150, 480 - 100, 0);
     glVertex3f(100, 480 - 100, 0);
     glVertex3f(100, 480, 0);
    glEnd();

    /* Attention */
    glBegin(GL_QUADS);
     glColor3f(0, 0, 1);
     glVertex3f(200, 480, 0);
     glVertex3f(250, 480, 0);
     glVertex3f(250, 480 - app->attention, 0);
     glVertex3f(200, 480 - app->attention, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
     glColor3f(1, 1, 1);
     glVertex3f(200, 480, 0);
     glVertex3f(250, 480, 0);
     glVertex3f(250, 480 - 100, 0);
     glVertex3f(200, 480 - 100, 0);
     glVertex3f(200, 480, 0);
    glEnd();
#endif

#if 0
	draw_signal_line(app);
#endif
	draw_fft_data(fm);

	/* This waits for vertical refresh before flipping, so it sleeps */
	SDL_GL_SwapBuffers();
}

int
fm_scr_init(struct fm_sdl_state *fs)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init failed: %s\n", SDL_GetError());
	return 0;
	}

	fprintf(stderr, "%s: screen=%dx%d\n", __func__,
	    fs->scr_xsize,
	    fs->scr_ysize);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,            5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,          5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,           5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,          16);

	if (SDL_SetVideoMode(fs->scr_xsize, fs->scr_ysize, 15,
	    SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL) == NULL) {
		printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
		return 0;
	}

	glClearColor(0, 0, 0, 0);

	glViewport(0, 0, fs->scr_xsize, fs->scr_ysize);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, fs->scr_xsize, fs->scr_ysize, 0, 1, -1);

	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();

	return 1;
}

int
fm_sdl_set_samplerate(struct fm_sdl_state *fs, int n)
{

	pthread_rwlock_wrlock(&fs->rw);

	fs->fft_in = fftw_malloc(sizeof(fftw_complex) * n);
	fs->fft_out = fftw_malloc(sizeof(fftw_complex) * n);
	fs->s_in = calloc(n*2, sizeof(int16_t));
	fs->fft_p = fftw_plan_dft_1d(n, fs->fft_in, fs->fft_out,
	    FFTW_FORWARD, FFTW_ESTIMATE);
	fs->nsamples = n;
	fs->cur = 0;

	pthread_rwlock_unlock(&fs->rw);
	return (0);

}

int
fm_sdl_init(struct fm_sdl_state *fs)
{

	bzero(fs, sizeof(*fs));

	(void) pthread_rwlock_init(&fs->rw, NULL);
	(void) pthread_mutex_init(&fs->ready_m, NULL);
	(void) pthread_cond_init(&fs->ready, NULL);
	fs->scr_xsize = 1280;
	fs->scr_ysize = 480;

	return (0);
}

int
fm_sdl_update(struct fm_sdl_state *fs, int16_t *s, int n)
{
	int i;

	if (fs->nsamples == 0)
		return (-1);

	/*
	 * We receive samples in small chunks; so treat the fft_in[]
	 * array as a sliding window and move things along as appropriate.
	 */

	/*
	 * s[] is a series of I, Q, I, Q etc samples.
	 * De-interleave them into the required FFT format.
	 */

	pthread_rwlock_wrlock(&fs->rw);
	for (i = 0; i < n; i++) {
		fs->s_in[i] = s[i];
	}
	fs->s_n = n;

	pthread_rwlock_unlock(&fs->rw);

	return (0);
}

int
fm_sdl_update_fft_samples(struct fm_sdl_state *fs)
{
	int i, j, k;
	int n = fs->s_n;
	int num = n / 2;

	if (fs->nsamples == 0)
		return (-1);

	if (fs->s_n == 0) {
		return (-1);
	}

	/* Shuffle enough samples down to make space for 'num' samples */
	if (fs->nsamples - fs->cur < num) {
		int nm;

		nm = num - (fs->nsamples - fs->cur);
#if 0
		fprintf(stderr, "%s: nsamples: %d, cur: %d, num=%d, need=%d\n",
		    __func__,
		    fs->nsamples,
		    fs->cur,
		    num,
		    nm);
#endif

		for (i = nm, j = 0; i < fs->cur; i++, j++) {
			fs->fft_in[j][0] = fs->fft_in[i][0];
			fs->fft_in[j][1] = fs->fft_in[i][1];
		}
		fs->cur -= nm;
	}

	/* Copy these samples in from cur -> cur + num */
	for (i = 0, j = 0; i < num; i++) {
		fs->fft_in[i + fs->cur][0] = fs->s_in[j++];
		fs->fft_in[i + fs->cur][1] = fs->s_in[j++];
	}

	/* And now update cur */
	fs->cur += num;

	/* .. now we're done with s_in */
	fs->s_n = 0;

	return (0);
}

int
fm_sdl_run(struct fm_sdl_state *fs)
{

	if (fs->nsamples == 0)
		return (-1);

	/* Run FFT */
	fftw_execute(fs->fft_p);
	return (0);
}
