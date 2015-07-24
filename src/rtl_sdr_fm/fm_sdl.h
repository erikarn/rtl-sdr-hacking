#ifndef	__FM_SDL_H__
#define	__FM_SDL_H__

struct fm_sdl_state {
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
	pthread_t thread;

	fftw_complex *fft_in;
	fftw_complex *fft_out;
	int nsamples;	/* Number of samples per second */
	int cur;	/* Current endpoint */
	fftw_plan fft_p;
};

extern	int fm_sdl_init(struct fm_sdl_state *fs);
extern	int fm_sdl_set_samplerate(struct fm_sdl_state *fs, int n);
extern	int fm_scr_init(struct fm_sdl_state *fs);
extern	int fm_sdl_update(struct fm_sdl_state *fs, int16_t *ns, int n);
extern	int fm_sdl_run(struct fm_sdl_state *fs);
extern	void fm_sdl_display_update(struct fm_sdl_state *fm);

#endif
