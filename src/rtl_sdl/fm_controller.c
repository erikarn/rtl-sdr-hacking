
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <unistd.h>

#include <math.h>
#include <pthread.h>
#include <libusb.h>

#include "rtl-sdr.h"

#include "convenience.h"

#include "fm_cfg.h"
#include "fm_dongle.h"
#include "fm_controller.h"

static void
optimal_settings(struct controller_state *cs, int freq, int rate)
{
	// giant ball of hacks
	// seems unable to do a single pass, 2:1
	int capture_freq, capture_rate;
	struct dongle_state *d = cs->dongle;

	fprintf(stderr, "Freq: %d, rate: %d\n", freq, rate);
	capture_freq = freq;
	capture_rate = rate;

	d->freq = (uint32_t)capture_freq;
	d->rate = (uint32_t)capture_rate;
}

/*
 * implement a controller.
 *
 * A controller controls a single dongle, and handles requests
 * for changing frequency and scheduling dongle configuration updates.
 */
static void *
controller_thread_fn(void *arg)
{
	// thoughts for multiple dongles
	// might be no good using a controller thread if retune/rate blocks
	int i;
	struct controller_state *s = arg;
	struct dongle_state *dongle = s->dongle;

	if (s->wb_mode) {
		for (i=0; i < s->freq_len; i++) {
			s->freqs[i] += 16000;}
	}

	/* set up primary channel */
	/* XXX hard-code sample rate for now */
	optimal_settings(s, s->freqs[0], dongle->rate);

	if (dongle->direct_sampling) {
		verbose_direct_sampling(dongle->dev, 1);}
	if (dongle->offset_tuning) {
		verbose_offset_tuning(dongle->dev);}

	/* Set the frequency */
	verbose_set_frequency(dongle->dev, dongle->freq);
#if 0
	fprintf(stderr, "Oversampling input by: %ix.\n", demod.downsample);
	fprintf(stderr, "Oversampling output by: %ix.\n", demod.post_downsample);
#endif

	/* Set the sample rate */
	verbose_set_sample_rate(dongle->dev, dongle->rate);

	fprintf(stderr, "Output at %u Hz.\n", dongle->rate);

	while (! s->do_exit) {
		/* XXX locking? */
		pthread_mutex_lock(&s->hop_m);
		pthread_cond_wait(&s->hop, &s->hop_m);
		pthread_mutex_unlock(&s->hop_m);

		if (s->freq_len <= 1) {
			continue;}
#if 0
		/* hacky hopping */
		s->freq_now = (s->freq_now + 1) % s->freq_len;
		optimal_settings(s->freqs[s->freq_now], demod.rate_in);
		rtlsdr_set_center_freq(dongle.dev, dongle.freq);
		dongle.mute = BUFFER_DUMP;
#endif
	}
	return 0;
}

void
controller_init(struct controller_state *s, struct dongle_state *d)
{
	s->freqs[0] = 100000000;
	s->freq_len = 0;
	s->edge = 0;
	s->wb_mode = 0;
	s->do_exit = 0;
	s->dongle = d;
	pthread_cond_init(&s->hop, NULL);
	pthread_mutex_init(&s->hop_m, NULL);
}

void
controller_cleanup(struct controller_state *s)
{
	pthread_cond_destroy(&s->hop);
	pthread_mutex_destroy(&s->hop_m);
}

void
controller_shutdown(struct controller_state *s)
{


	pthread_mutex_lock(&s->hop_m);
	s->do_exit = 1;
	pthread_cond_signal(&s->hop);
	pthread_mutex_unlock(&s->hop_m);
}

int
controller_thread_start(struct controller_state *s)
{
	int r;

	r = pthread_create(&s->thread, NULL, controller_thread_fn, s);
	if (r != 0) {
		warn("%s: pthread_create", __func__);
		return (-1);
	}
	return (0);
}

void
controller_thread_join(struct controller_state *s)
{

	(void) pthread_join(s->thread, NULL);
}
