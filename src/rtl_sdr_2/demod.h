#ifndef	__DEMOD_H__
#define	__DEMOD_H__

struct demod_state
{
	int      exit_flag;
	pthread_t thread;
	int16_t  lowpassed[MAXIMUM_BUF_LENGTH];
	int      lp_len;
	int16_t  lp_i_hist[10][6];
	int16_t  lp_q_hist[10][6];
	int16_t  result[MAXIMUM_BUF_LENGTH];
	int16_t  droop_i_hist[9];
	int16_t  droop_q_hist[9];
	int      result_len;
	int      rate_in;
	int      rate_out;
	int      rate_out2;

	/* low pass filter state */
	struct low_pass *lp;

	int      pre_r, pre_j;

	int      post_downsample;
	int      output_scale;
	int      squelch_level, conseq_squelch, squelch_hits, terminate_on_squelch;
	int      downsample_passes;
	int      comp_fir_size;
	int      custom_atan;
	int      deemph, deemph_a;

	/* low pass real state */
	struct low_pass_real *lpr;
	int      dc_block, dc_avg;
	void     (*mode_demod)(struct demod_state*);
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
	struct output_state *output_target;
};

extern	void fm_demod(struct demod_state *fm);
extern	void am_demod(struct demod_state *fm);
extern	void lsb_demod(struct demod_state *fm);
extern	void usb_demod(struct demod_state *fm);
extern	void raw_demod(struct demod_state *fm);
extern	void full_demod(struct demod_state *d);

#endif	/* __DEMOD_H__ */
