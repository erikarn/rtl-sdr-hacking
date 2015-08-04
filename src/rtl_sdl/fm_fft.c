
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <err.h>
#include <pthread.h>
#include <math.h>

#include <fftw3.h>

#include "fm_fft.h"

static float
fft_mag(float r, float i)
{
	return (sqrtf(r*r + i*i));
}

static int
bin_to_idx(int i, int nsamples, int midpoint)
{
	int j;

	/* XXX TODO: simplify */
	if (i < midpoint)
	    j = (nsamples / 2) + (i * nsamples) / nsamples;
	else
	   j = (i - midpoint);
	return (j);
}

/*
 * Convert FFT results into dB values, with a per-sample divisor supplied.
 */
void
fft_to_db(struct fm_fft_state *fm, int nsamples)
{
	int i, j;
	double db;
	float x, y;

	for (i = 0; i < fm->fft_npoints; i++) {
		/*
		 * If we're below midpoint, map 'i' into the -ve space.
		 * If we're at/above midpoint, map 'i' into the +ve space.
		 *
		 * Translate it into the right sample offset for the given
		 * position.  For now we just skip; we don't try to sum
		 * entries and plot the average/min/max.
		 * We should do that later.
		 */
		j = bin_to_idx(i, fm->fft_npoints, fm->fft_npoints / 2);

		x = i;
		db = fft_mag(fm->fft_out[j][0], fm->fft_out[j][1]);

		/* DC filter */
		if (i == fm->fft_npoints / 2)
			db = fft_mag(fm->fft_out[j+1][0], fm->fft_out[j+1][1]);

		/* Convert to dB, totally non-legit and not yet fixed.. */
		/* XXX rtl_power.c does /rate, /samples; not sure exactly what .. */
		db /= nsamples;
		db = 10 * log10(db);
		fm->fft_db[i] = db;
	}
}

int
fm_fft_init(struct fm_fft_state *fs, int nbins)
{

	bzero(fs, sizeof(*fs));

	fs->fft_npoints = nbins;
	fs->fft_in = fftw_malloc(sizeof(fftw_complex) * fs->fft_npoints);
	fs->fft_out = fftw_malloc(sizeof(fftw_complex) * fs->fft_npoints);
	fs->fft_p = fftw_plan_dft_1d(fs->fft_npoints, fs->fft_in,
	    fs->fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
	fs->fft_db = calloc(fs->fft_npoints, sizeof(int));

	return (0);
}

/*
 * Set the fft_in samples based on given interleaved sample data.
 * n is the number of samples, not the number of int16_t's.
 * (Each sample is I and then Q.)
 */
int
fm_fft_update_fft_samples(struct fm_fft_state *fs, int16_t *s, int n)
{
	int i, j, k;

#if 0
	fprintf(stderr, "%s: ready; nsamples=%d, s_n=%d\n",
	    __func__,
	    fs->nsamples,
	    fs->s_n);
#endif
	for (i = 0, j = 0; i < fs->fft_npoints; i++) {
		fs->fft_in[i][0] = 0;
		fs->fft_in[i][1] = 0;
	}

	if (n * 2 < fs->fft_npoints) {
		fprintf(stderr, "%s: not enough; n=%d, npoints=%d\n",
		    __func__,
		    n,
		    fs->fft_npoints);
		return (-1);
	}

	/* Copy these samples in */
	/* XXX assumes we have enough! */
	for (i = 0, j = 0; i < fs->fft_npoints; i++) {
		fs->fft_in[i][0] = s[j++];
		fs->fft_in[i][1] = s[j++];
	}

	return (0);
}

/*
 * XXX TODO: should have a separate rw lock
 * protecting fft_in[].
 */
int
fm_fft_run(struct fm_fft_state *fs)
{

	/* Run FFT */
	fftw_execute(fs->fft_p);

	return (0);
}

