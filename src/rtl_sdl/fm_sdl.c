
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

/*
 * Draw just the raw signal line - signal in the time domain.
 */
static void
draw_signal_line(struct fm_sdl_state *fm)
{
	int y;
	int64_t x, o;
	int curofs;
	int size;
	int ys;

	/*
	 * Plot a line series for the last 320 elements,
	 * two pixel wide (640 pixels.)
	 */

	ys = fm->scr_ysize / 2;

	glBegin(GL_LINE_STRIP);
	glColor3f(1, 0, 0);
	glVertex3f(0, ys, 0);
	glVertex3f(fm->scr_xsize, ys, 0);
	glEnd();

	glBegin(GL_LINE_STRIP);
	glColor3f(1, 1, 0);
	curofs = fm->nsamples - 1;
	if (curofs < 0)
		curofs = size - 1;
	for (x = 0; x < fm->scr_xsize; x++) {
		o = ((int64_t) x * (int64_t) fm->nsamples) / (int64_t) fm->scr_xsize;
		y = fm->fft_in[o][0];

		y = y * ys / 128;
		/* Clip */
		if (y < -ys)
			y = -ys;
		else if (y > ys)
			y = ys;

		/* Set offset to middle of the screen */
		y += ys;

		glVertex3f(x, y, 0);

		/* Go backwards in samples */
		curofs--;
		if (curofs < 0)
			break;
	}
	glEnd();

	/* and Q */
	glBegin(GL_LINE_STRIP);
	glColor3f(1, 0, 1);
	curofs = fm->nsamples - 1;
	if (curofs < 0)
		curofs = size - 1;
	for (x = 0; x < fm->scr_xsize; x++) {
		o = ((int64_t) x * (int64_t) fm->nsamples) / (int64_t) fm->scr_xsize;
		y = fm->fft_in[o][1];

		y = y * ys / 128;
		/* Clip */
		if (y < -ys)
			y = -ys;
		else if (y > ys)
			y = ys;

		/* Set offset to middle of the screen */
		y += ys;

		glVertex3f(x, y, 0);

		/* Go backwards in samples */
		curofs--;
		if (curofs < 0)
			break;
	}
	glEnd();

}

static float
fft_mag(float r, float i)
{
	return (sqrtf(r*r + i*i));
}

static int
bin_to_idx(int i, int nsamples, int midpoint)
{
	int j;

	/* XXX TODO: simplify */
	if (i < midpoint)
	    j = (nsamples / 2) + (i * nsamples) / nsamples;
	else
	   j = (i - midpoint);
	return (j);
}

/*
 * Draw the FFT data out.
 */
static void
draw_fft_data(struct fm_sdl_state *fm)
{
	int i, j;
	double dbm;
	float x, y;
	int mul = 256;
	int midpoint;

	/* XXX not locked? */
	if (fm->nsamples == 0)
		return;

	glBegin(GL_LINE_STRIP);
	glColor3f(1.0, 0.0, 0.0);
	glVertex3f(fm->scr_xsize / 2, 0, 0);
	glVertex3f(fm->scr_xsize / 2, fm->scr_ysize, 0);
	glEnd();

	/*
	 * Map the FFT data into 'xsize', taking into account -ve and +ve
	 * frequency data (with DC in the middle.)
	 */
	glBegin(GL_LINE_STRIP);
	glColor3f(0.0, 1.0, 1.0);
	midpoint = fm->fft_npoints / 2;

	/*
	 * The fft_out bins are ordered 0..BW/2, then -BW/2..0.
	 * This should be done for the purposes of the plot!
	 */
	for (i = 0; i < fm->fft_npoints; i++) {
		/*
		 * If we're below midpoint, map 'i' into the -ve space.
		 * If we're at/above midpoint, map 'i' into the +ve space.
		 *
		 * Translate it into the right sample offset for the given
		 * position.  For now we just skip; we don't try to sum
		 * entries and plot the average/min/max.
		 * We should do that later.
		 */
		j = bin_to_idx(i, fm->fft_npoints, midpoint);

		x = i;
		dbm = fft_mag(fm->fft_out[j][0], fm->fft_out[j][1]);

		/* DC filter */
		if (i == midpoint)
			dbm = fft_mag(fm->fft_out[j+1][0], fm->fft_out[j+1][1]);

#if 0
		if (i == midpoint)
			fprintf(stderr, "DC: %f, %f/%f\n", y, fm->fft_out[j][0], fm->fft_out[j][1]);
#endif

		/* Convert to dB, totally non-legit */
		/* XXX rtl_power.c does /rate, /samples; not sure exactly what .. */
		dbm /= fm->nsamples;
		dbm = 10 * log10(dbm);

		/* it's in dB, and that's small, so scale it up a little to see */
		y = dbm * 4;

		/* y is now in dBm, so scale/invert it appropriately */
		/* XXX hard-coded hack */
		y = fm->scr_ysize/2 - y;

		/* hard-clip */
		if (y > fm->scr_ysize)
			y = fm->scr_ysize;
		if (y < 0)
			y = 0;

		glVertex3f(x, y, 0);

	}
	fprintf(stderr, " %f\n", dbm);
	glEnd();
}

void
fm_sdl_display_update(struct fm_sdl_state *fm)
{

	process_events();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

//	draw_signal_line(fm);
	draw_fft_data(fm);

	/* This waits for vertical refresh before flipping, so it sleeps */
	SDL_GL_SwapBuffers();
}

int
fm_scr_init(struct fm_sdl_state *fs)
{
	const SDL_VideoInfo *info = NULL;
	int bpp, flags;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init failed: %s\n", SDL_GetError());
	return 0;
	}

	info = SDL_GetVideoInfo();
	if (info == NULL) {
		fprintf(stderr,"SDL_GetVideoInfo failed: %s\n", SDL_GetError());
		return (0);
	}
	bpp = info->vfmt->BitsPerPixel;

	fprintf(stderr, "%s: screen=%dx%d\n", __func__,
	    fs->scr_xsize,
	    fs->scr_ysize);


	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,            5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,          5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,           5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,          16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,	1);

	/* XXX no SDL_HWSURFACE, SDL_GL_DOUBLEBUFFER? */
	flags = SDL_OPENGL | SDL_HWSURFACE | SDL_ANYFORMAT;

	if (SDL_SetVideoMode(fs->scr_xsize, fs->scr_ysize, bpp, flags) == NULL) {
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return 1;
}

int
fm_sdl_set_samplerate(struct fm_sdl_state *fs, int n)
{

	pthread_rwlock_wrlock(&fs->rw);

	/*
	 * For now, the FFT is hard-coded to 1024 points.
	 */
	fs->fft_npoints = 1024;
	fs->fft_in = fftw_malloc(sizeof(fftw_complex) * n);
	fs->fft_out = fftw_malloc(sizeof(fftw_complex) * n);
	fs->fft_p = fftw_plan_dft_1d(fs->fft_npoints, fs->fft_in,
	    fs->fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
	fs->fft_db = calloc(n, sizeof(int));

	/* However, s_in is the size of our sample rate */
	fs->s_in = calloc(n*2, sizeof(int16_t));
	fs->nsamples = n;

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
	fs->scr_xsize = 1024;
	fs->scr_ysize = 480;

	return (0);
}

/*
 * Add in samples to the pending queue.
 */
int
fm_sdl_update(struct fm_sdl_state *fs, int16_t *s, int n)
{
	int i;

	/* Run too early? */
	/* XXX locking? */
	if (fs->nsamples == 0)
		return (-1);

	/*
	 * We receive samples in small chunks; so treat the fft_in[]
	 * array as a sliding window and move things along as appropriate.
	 */

	/*
	 * Copy the data into our incoming area.
	 * Overwrite what's currently there.
	 */
	pthread_rwlock_wrlock(&fs->rw);
	fs->s_n = 0;
	for (i = 0; i < n; i++) {
		fs->s_in[fs->s_n] = s[i];
		fs->s_n++;
	}

#if 0
	fprintf(stderr, "%s: n=%d, nsamples=%d, s_n=%d\n",
	    __func__,
	    n,
	    fs->nsamples,
	    fs->s_n);
#endif

#if 0
	/* Signal if we're ready for the FFT bit */
	if (fs->s_n >= fs->nsamples * 2)
		fs->s_ready = 1;
#endif
	fs->s_ready = 1;

	pthread_rwlock_unlock(&fs->rw);

	return (0);
}

/*
 * Called with write lock held.
 */
int
fm_sdl_update_fft_samples(struct fm_sdl_state *fs)
{
	int i, j, k;

	if (fs->nsamples == 0)
		return (-1);

	if (fs->s_ready == 0)
		return (-1);

#if 0
	fprintf(stderr, "%s: ready; nsamples=%d, s_n=%d\n",
	    __func__,
	    fs->nsamples,
	    fs->s_n);
#endif

	/* Copy these samples in */
	/* XXX assumes we have enough! */
	for (i = 0, j = 0; i < fs->fft_npoints; i++) {
		fs->fft_in[i][0] = fs->s_in[j++];
		fs->fft_in[i][1] = fs->s_in[j++];
	}

	/* .. now we're done with s_in */
	fs->s_n = 0;
	fs->s_ready = 0;
	return (0);
}

/*
 * XXX TODO: should have a separate rw lock
 * protecting fft_in[].
 */
int
fm_sdl_run(struct fm_sdl_state *fs)
{

	if (fs->nsamples == 0)
		return (-1);

	/* Run FFT */
	fftw_execute(fs->fft_p);

	return (0);
}