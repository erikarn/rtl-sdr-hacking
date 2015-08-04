#ifndef	__FM_SDL_H__
#define	__FM_SDL_H__

struct fm_sdl_state {
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
	pthread_t thread;

	int nsamples;	/* Number of samples per second */
	int16_t *s_in;	/* incoming samples for fft - we keep up to a second */
	int s_n;	/* how far into the array are we, in samples */
	int s_ready;	/* XXX temp hack */

	int fft_npoints;	/* how big is this FFT? */
	fftw_complex *fft_in;
	fftw_complex *fft_out;
	fftw_plan fft_p;
	int *fft_db;		/* fft result, converted to dB */

	float freq_centre;

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
extern	void fm_sdl_set_freq_centre(struct fm_sdl_state *fs, float freq);
extern	void fm_sdl_process_events(struct fm_sdl_state *fs);

#endif
