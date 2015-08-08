#ifndef	__FM_CONTROLLER_H__
#define	__FM_CONTROLLER_H__

struct controller_state {
//	int exit_flag; /* XXX? */
	int do_exit;
	pthread_t thread;
	uint32_t freqs[FREQUENCIES_LIMIT];
	struct dongle_state *dongle;
	int freq_len;
	int freq_time;	/* how long, in milliseconds, before hopping */
	int freq_now;
	int edge;
	int wb_mode;
	pthread_cond_t hop;
	pthread_mutex_t hop_m;
};


extern	void controller_init(struct controller_state *s, struct dongle_state *d);
extern	void controller_cleanup(struct controller_state *s);
extern	void controller_shutdown(struct controller_state *s);
extern	int controller_thread_start(struct controller_state *s);
extern	void controller_thread_join(struct controller_state *s);

#endif
