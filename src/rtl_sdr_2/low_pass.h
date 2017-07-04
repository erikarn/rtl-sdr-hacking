#ifndef	__LOW_PASS_H__
#define	__LOW_PASS_H__

struct demod_state;

struct low_pass {
	int downsample;
	int prev_index;
	int now_r, now_j;
};

struct low_pass_real {
	int now_lpr;
	int prev_lpr_index;
};

extern	struct low_pass * low_pass_create(void);
extern	void low_pass_free(struct low_pass *);
extern	struct low_pass_real * low_pass_real_create(void);
extern	void low_pass_real_free(struct low_pass_real *);
extern	void low_pass_set_downsample(struct low_pass *lp, int downsample);
extern	int low_pass_get_downsample(struct low_pass *lp);

extern	void low_pass(struct demod_state *d, struct low_pass *ls);
extern	int low_pass_simple(int16_t *signal2, int len, int step);
extern	void low_pass_real(struct demod_state *s, struct low_pass_real *lpr);

#endif	/* __LOW_PASS_H__ */
