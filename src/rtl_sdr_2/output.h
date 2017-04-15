#ifndef	__OUTPUT_H__
#define	__OUTPUT_H__

struct output_state;

typedef int output_write_cb_t(struct output_state *);

struct output_state {
	int do_exit;
	int      exit_flag;
	pthread_t thread;
	FILE     *file;
	char     *filename;
	int16_t  result[MAXIMUM_BUF_LENGTH];
	int      result_len;
	int      rate;
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;

	/* Callback state */
	void *cbdata;
	output_write_cb_t *output_write_cb;
};

extern	void output_run(struct output_state *s);
extern	void output_set_exit(struct output_state *s);
extern	void output_append(struct output_state *s, const char *buf, int len);
extern	void output_init(struct output_state *s);
extern	void output_cleanup(struct output_state *s);

#endif	/* __OUTPUT_H__ */
