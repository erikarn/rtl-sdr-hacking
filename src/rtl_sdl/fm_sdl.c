
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
#include <pthread_np.h>
#include <math.h>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <fftw3.h>

#include "fm_sdl.h"

/* XXX for hacking */
#include "fm_cfg.h"
#include "rtl-sdr.h"
#include "fm_dongle.h" /* XXX for hacking */

#define	FM_SDL_EV_FFT_READY			0x1

/* XXX for hacking */
extern struct dongle_state dongle;

static void
quit_tutorial(int code)
{
	SDL_Quit();
	exit(code);
}

static void
handle_key_down(struct fm_sdl_state *fm, SDL_keysym *key)
{

	//fprintf(stderr ,"%s: key=%d\n", __func__, key->sym);
	/* 122 = z, 120 = x */

	/* This is just a total hack right now to move the frequency bits around */
	switch (key->sym) {
	case 122:	/* z */
		(void) dongle_change_freq(&dongle, dongle.freq - (1 * 100 * 1000));
		break;
	case 120:	/* x */
		(void) dongle_change_freq(&dongle, dongle.freq + (1 * 100 * 1000));
		break;
	default:
		break;
	}
}

static void
handle_windowevent(struct fm_sdl_state *fm, SDL_Event *e)
{

	fm_sdl_display_update(fm);
}

static void
handle_userevent(struct fm_sdl_state *fm, SDL_Event *e)
{

	switch (e->user.code) {
	case FM_SDL_EV_FFT_READY:
		fm_sdl_display_update(fm);
		break;
	default:
		break;
	}
}

/*
 * Called by the UI thread - call it without locks held.
 */
void
fm_sdl_process_events(struct fm_sdl_state *fm)
{
	SDL_Event event;

	while(SDL_WaitEvent(&event)) {
		switch(event.type) {
		case SDL_KEYDOWN:
			/* Handle key presses. */
			handle_key_down(fm, &event.key.keysym);
			break;
		case SDL_QUIT:
			/* Handle quit requests (like Ctrl-c). */
			quit_tutorial( 0 );
			break;
		case SDL_VIDEOEXPOSE:
			handle_windowevent(fm, &event);
			break;
		case SDL_USEREVENT:
			handle_userevent(fm, &event);
			break;
		default:
			break;
		}
	}
}

#if 0
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
#endif

/*
 * Draw the FFT data out.
 */
static void
draw_fft_data(struct fm_sdl_state *fm)
{
	int i, j;
	double dbm;
	float x, y;

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

	/* XXX TODO: plot the window size in pixels; translate each to bin id */
	for (i = 0; i < fm->num_db; i++) {
		x = i;

		/* it's in dB, and that's small, so scale it up a little to see */
		y = fm->db_in[i] * 4;

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
	//fprintf(stderr, " %f\n", dbm);
	glEnd();
}

void
fm_sdl_display_update(struct fm_sdl_state *fm)
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

//	draw_signal_line(fm);
	draw_fft_data(fm);

	/* This waits for vertical refresh before flipping, so it sleeps */
	SDL_GL_SwapBuffers();
}

static int
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

static void *
fm_sdl_thr(void *arg)
{
	struct fm_sdl_state *fm = arg;
	int i;

	/* Init the SDL/OpenGL screen in this context */
	fm_scr_init(fm);

	/* XXX locking? */
	while (! fm->do_exit) {
		/* Handle events; will sleep until it's done */
		fm_sdl_process_events(fm);
	}

	return (NULL);
}

int
fm_sdl_thread_start(struct fm_sdl_state *fm)
{
	int r;

	r = pthread_create(&fm->thread, NULL, fm_sdl_thr, fm);
	if (r != 0) {
		warn("%s: pthread_create", __func__);
		return (-1);
	}
	pthread_set_name_np(fm->thread, "fm_sdl");

	return (r);
}

int
fm_sdl_thread_join(struct fm_sdl_state *fm)
{

	(void) pthread_join(fm->thread, NULL);
	return (0);
}

int
fm_sdl_init(struct fm_sdl_state *fs, int npoints)
{

	bzero(fs, sizeof(*fs));

	(void) pthread_rwlock_init(&fs->rw, NULL);
	(void) pthread_mutex_init(&fs->ready_m, NULL);
	(void) pthread_cond_init(&fs->ready, NULL);
	/*
	 * For now, the screen size must equal the number of FFT points.
	 */
	fs->scr_xsize = 1024;
	fs->scr_ysize = 480;

	fs->db_in = calloc(npoints, sizeof(int));
	fs->num_db = npoints;
	/* XXX NULL check? */

	return (0);
}

void
fm_sdl_set_freq_centre(struct fm_sdl_state *fs, float freq)
{

	pthread_rwlock_wrlock(&fs->rw);
	fs->freq_centre = freq;
	pthread_rwlock_unlock(&fs->rw);
}

int
fm_sdl_update_db(struct fm_sdl_state *fs, int *db, int n)
{
	int i;

	/* XXX verify we have been handed a valid bin number! */
	if (n != fs->num_db) {
		fprintf(stderr, "%s: n (%d) != num_db (%d) !\n",
		    __func__,
		    n,
		    fs->num_db);
		return (-1);
	}
	pthread_rwlock_wrlock(&fs->rw);
	for (i = 0; i < fs->num_db; i++) {
		fs->db_in[i] = db[i];
	}
	pthread_rwlock_unlock(&fs->rw);

	return (0);
}

/*
 * XXX TODO: turn this into an SDL event, and have
 * the main SDL loop switch to blocking event reads.
 */
int
fm_sdl_signal_ready(struct fm_sdl_state *fs)
{
	SDL_Event event;

	event.type = SDL_USEREVENT;
	event.user.code = FM_SDL_EV_FFT_READY;
	SDL_PushEvent(&event);

	return (0);
}

void
fm_sdl_thread_signal_exit(struct fm_sdl_state *fs)
{
	SDL_Event event;

	pthread_mutex_lock(&fs->ready_m);
	fs->do_exit = 1;
	pthread_mutex_unlock(&fs->ready_m);

	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}
