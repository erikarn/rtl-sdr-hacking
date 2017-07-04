
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include <pthread.h>

#include "config.h"

#include "low_pass.h"

/* XXX sigh */
#include "demod.h"

/*
 * This implements a simple boxcar FIR filter and decimator.
 * It operates on both the I and Q samples.
 *
 * Yes, like a lot of things in rtl_fm, it's not entirely well
 * named.
 *
 * The 'downsample' value is both the decimation count (2 = 1/2,
 * 3 = 1/3, 4 = 1/4, etc) and the filter window/order.
 *
 * The resulting data buffer (lowpassed/lp_len) is 1/downsample sized.
 *
 * Note: the resulting data value isn't averaged; each low_passed value
 * is the sum of the decimation. So a downsample=4 filter will
 * decimate by 4, each sample however is in the range of (-ve..0..+ve)*4.
 * (where -ve/+ve is the max values of the input range.)
 *
 * A normal boxcar FIR should be averaging each step so the range of the
 * output doesn't change; I am guessing it's not done here to avoid
 * precision loss (and for FM the magnitude doesn't matter.)
 */
void
low_pass(struct demod_state *d, struct low_pass *lp)
/* simple square window FIR */
{
	int i=0, i2=0;
	while (i < d->lp_len) {
		lp->now_r += d->lowpassed[i];
		lp->now_j += d->lowpassed[i+1];
		i += 2;
		lp->prev_index++;
		if (lp->prev_index < lp->downsample) {
			continue;
		}
		d->lowpassed[i2]   = lp->now_r; // * d->output_scale;
		d->lowpassed[i2+1] = lp->now_j; // * d->output_scale;
		lp->prev_index = 0;
		lp->now_r = 0;
		lp->now_j = 0;
		i2 += 2;
	}
	d->lp_len = i2;
}

int
low_pass_simple(int16_t *signal2, int len, int step)
// no wrap around, length must be multiple of step
{
	int i, i2, sum;
	for(i=0; i < len; i+=step) {
		sum = 0;
		for(i2=0; i2<step; i2++) {
			sum += (int)signal2[i + i2];
		}
		//signal2[i/step] = (int16_t)(sum / step);
		signal2[i/step] = (int16_t)(sum);
	}
	signal2[i/step + 1] = signal2[i/step];
	return len / step;
}

void
low_pass_real(struct demod_state *s, struct low_pass_real *lpr)
/* simple square window FIR */
// add support for upsampling?
{
	int i=0, i2=0;
	int fast = (int)s->rate_out;
	int slow = s->rate_out2;
	while (i < s->result_len) {
		lpr->now_lpr += s->result[i];
		i++;
		lpr->prev_lpr_index += slow;
		if (lpr->prev_lpr_index < fast) {
			continue;
		}
		s->result[i2] = (int16_t)(lpr->now_lpr / (fast/slow));
		lpr->prev_lpr_index -= fast;
		lpr->now_lpr = 0;
		i2 += 1;
	}
	s->result_len = i2;
}

struct low_pass *
low_pass_create(void)
{
	struct low_pass *lp;

	lp = calloc(1, sizeof(*lp));
	if (lp == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	return (lp);
}

void
low_pass_free(struct low_pass *lp)
{

	free(lp);
}

void
low_pass_set_downsample(struct low_pass *lp, int downsample)
{

	lp->downsample = downsample;
}

int
low_pass_get_downsample(struct low_pass *lp)
{

	return (lp->downsample);
}

struct low_pass_real *
low_pass_real_create(void)
{
	struct low_pass_real *lpr;

	lpr = calloc(1, sizeof(*lpr));
	if (lpr == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	return (lpr);
}

void
low_pass_real_free(struct low_pass_real *lpr)
{

	free(lpr);
}
