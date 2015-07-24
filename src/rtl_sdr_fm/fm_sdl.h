#ifndef	__FM_SDL_H__
#define	__FM_SDL_H__

struct fm_sdl_state {
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
	pthread_t thread;

	int16_t *s_in;	/* incoming samples for fft */
	int s_n;	/* how many were provided this round */
	int s_ready;	/* ready to do the actual FFT */

	fftw_complex *fft_in;
	fftw_complex *fft_out;
	int nsamples;	/* Number of samples per second */
	fftw_plan fft_p;

	int scr_xsize;
	int scr_ysize;
};

extern	int fm_sdl_init(struct fm_sdl_state *fs);
extern	int fm_sdl_set_samplerate(struct fm_sdl_state *fs, int n);
extern	int fm_scr_init(struct fm_sdl_state *fs);
extern	int fm_sdl_update(struct fm_sdl_state *fs, int16_t *ns, int n);
extern	int fm_sdl_run(struct fm_sdl_state *fs);
extern	void fm_sdl_display_update(struct fm_sdl_state *fm);
extern	int fm_sdl_update_fft_samples(struct fm_sdl_state *fs);

#endif
