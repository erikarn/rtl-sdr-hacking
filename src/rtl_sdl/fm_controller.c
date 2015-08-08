
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

#if 0
	fprintf(stderr, "Freq: %d, rate: %d\n", freq, rate);
#endif
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
		usleep(s->freq_time * 1000);		/* XXX just for now */

		if (s->freq_len <= 1) {
			continue;}

		/*
		 * Wait and see if we've been signaled that the
		 * previous channel change has happened.
		 * This is very hacky - it's done just to ensure
		 * we've synchronised with the completed channel change.
		 *
		 * It totally should be more event driven..
		 */
		pthread_mutex_lock(&s->hop_m);
		if (s->freq_hop_wait == 1 && s->freq_hop_wait_ok == 0) {
			pthread_mutex_unlock(&s->hop_m);
			fprintf(stderr,
			    "%s: waiting for pending channel change..\n",
			    __func__);
			continue;
		}

		s->freq_hop_wait = 1;
		s->freq_hop_wait_ok = 0;
		pthread_mutex_unlock(&s->hop_m);

		/* hacky hopping */
		s->freq_now = (s->freq_now + 1) % s->freq_len;
		optimal_settings(s, s->freqs[s->freq_now], dongle->rate);
		dongle_change_freq(dongle, s->freqs[s->freq_now]);
	}
	return 0;
}

static void
controller_dongle_state_cb(struct dongle_state *d, void *arg,
    dongle_cur_state_t state, int error)
{
	struct controller_state *s = arg;

	//fprintf(stderr, "%s: called; state=%d\n", __func__, state);

	/*
	 * Notify the controller that if we're waiting for a channel
	 * change, we're totally ready for it.
	 *
	 * XXX TODO: handle errors!
	 */
	if (state == FM_SDL_DONGLE_STATE_FREQ_TUNED) {
		pthread_mutex_lock(&s->hop_m);
		if (s->freq_hop_wait) {
			s->freq_hop_wait_ok = 1;
		}
		pthread_mutex_unlock(&s->hop_m);
	}
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
	s->freq_time = 250;	/* 100mS */
	pthread_mutex_init(&s->hop_m, NULL);

	/* Get notifications from the dongle */
	dongle_set_state_callback(d, controller_dongle_state_cb, s);
}

void
controller_cleanup(struct controller_state *s)
{
	pthread_mutex_destroy(&s->hop_m);
}

void
controller_shutdown(struct controller_state *s)
{


	pthread_mutex_lock(&s->hop_m);
	s->do_exit = 1;
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
