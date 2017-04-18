
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "config.h"

/* XXX sigh */
#include "demod.h"

#include "low_pass.h"

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
low_pass(struct demod_state *d)
/* simple square window FIR */
{
	int i=0, i2=0;
	while (i < d->lp_len) {
		d->now_r += d->lowpassed[i];
		d->now_j += d->lowpassed[i+1];
		i += 2;
		d->prev_index++;
		if (d->prev_index < d->downsample) {
			continue;
		}
		d->lowpassed[i2]   = d->now_r; // * d->output_scale;
		d->lowpassed[i2+1] = d->now_j; // * d->output_scale;
		d->prev_index = 0;
		d->now_r = 0;
		d->now_j = 0;
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
low_pass_real(struct demod_state *s)
/* simple square window FIR */
// add support for upsampling?
{
	int i=0, i2=0;
	int fast = (int)s->rate_out;
	int slow = s->rate_out2;
	while (i < s->result_len) {
		s->now_lpr += s->result[i];
		i++;
		s->prev_lpr_index += slow;
		if (s->prev_lpr_index < fast) {
			continue;
		}
		s->result[i2] = (int16_t)(s->now_lpr / (fast/slow));
		s->prev_lpr_index -= fast;
		s->now_lpr = 0;
		i2 += 1;
	}
	s->result_len = i2;
}
