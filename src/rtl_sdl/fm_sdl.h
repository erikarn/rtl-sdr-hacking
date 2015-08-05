#ifndef	__FM_SDL_H__
#define	__FM_SDL_H__

struct fm_sdl_state {
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
	pthread_t thread;

	int *db_in;	/* per-bin dB values */
	int num_db;	/* how many entries */
	int s_ready;	/* yes, we're ready */

	float freq_centre;

	int scr_xsize;
	int scr_ysize;
};

extern	int fm_sdl_init(struct fm_sdl_state *fs, int npoints);
extern	int fm_scr_init(struct fm_sdl_state *fs);
extern	int fm_sdl_update_db(struct fm_sdl_state *fs, int *ns, int n);
extern	void fm_sdl_display_update(struct fm_sdl_state *fm);
extern	void fm_sdl_set_freq_centre(struct fm_sdl_state *fs, float freq);
extern	void fm_sdl_process_events(struct fm_sdl_state *fs);

#endif
