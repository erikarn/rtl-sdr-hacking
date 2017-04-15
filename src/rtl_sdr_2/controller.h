#ifndef	__CONTROLLER_H__
#define	__CONTROLLER_H__

struct controller_state
{
	int      exit_flag;
	pthread_t thread;
	uint32_t freqs[FREQUENCIES_LIMIT];
	int      freq_len;
	int      freq_now;
	int      edge;
	int      wb_mode;
	pthread_cond_t hop;
	pthread_mutex_t hop_m;
};

#endif	/* __CONTROLLER_H__ */
