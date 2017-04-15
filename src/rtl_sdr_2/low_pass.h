#ifndef	__LOW_PASS_H__
#define	__LOW_PASS_H__

extern	void low_pass(struct demod_state *d);
extern	int low_pass_simple(int16_t *signal2, int len, int step);
extern	void low_pass_real(struct demod_state *s);

#endif	/* __LOW_PASS_H__ */
