
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <math.h>
#include <pthread.h>
#include <libusb.h>

#include <fftw3.h>

#include "rtl-sdr.h"
#include "convenience.h"
#include "remove_dc.h"

#include "fm_cfg.h"
#include "fm_dongle.h"
#include "fm_controller.h"
#include "fm_sdl.h"
#include "fm_fft.h"
#include "fm_fft_thread.h"

#include "demod.h"
#include "fm_demod.h"
#include "rotate_90.h"

int do_exit = 0;

// multiple of these, eventually
struct dongle_state dongle;
struct controller_state controller;
struct fm_sdl_state state_sdl;
struct fm_fft_thread *state_fm_fft;
struct demod_state state_demod;

void usage(void)
{
	fprintf(stderr,
		"rtl_fm, a simple narrow band FM demodulator for RTL2832 based DVB-T receivers\n\n"
		"Use:\trtl_fm -f freq [-options] [filename]\n"
		"\t-f frequency_to_tune_to [Hz]\n"
		"\t    use multiple -f for scanning (requires squelch)\n"
		"\t    ranges supported, -f 118M:137M:25k\n"
		"\t[-M modulation (default: fm)]\n"
		"\t    fm, wbfm, raw, am, usb, lsb\n"
		"\t    wbfm == -M fm -s 170k -o 4 -A fast -r 32k -l 0 -E deemp\n"
		"\t    raw mode outputs 2x16 bit IQ pairs\n"
		"\t[-s sample_rate (default: 24k)]\n"
		"\t[-d device_index (default: 0)]\n"
		"\t[-g tuner_gain (default: automatic)]\n"
		"\t[-l squelch_level (default: 0/off)]\n"
		//"\t    for fm squelch is inverted\n"
		//"\t[-o oversampling (default: 1, 4 recommended)]\n"
		"\t[-p ppm_error (default: 0)]\n"
		"\t[-E enable_option (default: none)]\n"
		"\t    use multiple -E to enable multiple options\n"
		"\t    edge:   enable lower edge tuning\n"
		"\t    dc:     enable dc blocking filter\n"
		"\t    deemp:  enable de-emphasis filter\n"
		"\t    direct: enable direct sampling\n"
		"\t    offset: enable offset tuning\n"
		"\tfilename ('-' means stdout)\n"
		"\t    omitting the filename also uses stdout\n\n"
		"Experimental options:\n"
		"\t[-r resample_rate (default: none / same as -s)]\n"
		"\t[-t squelch_delay (default: 10)]\n"
		"\t    +values will mute/scan, -values will exit\n"
		"\t[-F fir_size (default: off)]\n"
		"\t    enables low-leakage downsample filter\n"
		"\t    size can be 0 or 9.  0 has bad roll off\n"
		"\t[-A std/fast/lut choose atan math (default: std)]\n"
		//"\t[-C clip_path (default: off)\n"
		//"\t (create time stamped raw clips, requires squelch)\n"
		//"\t (path must have '\%s' and will expand to date_time_freq)\n"
		//"\t[-H hop_fifo (default: off)\n"
		//"\t (fifo will contain the active frequency)\n"
		"\n"
		"Produces signed 16 bit ints, use Sox or aplay to hear them.\n"
		"\trtl_fm ... | play -t raw -r 24k -es -b 16 -c 1 -V1 -\n"
		"\t           | aplay -r 24k -f S16_LE -t raw -c 1\n"
		"\t  -M wbfm  | play -r 32k ... \n"
		"\t  -s 22050 | multimon -t raw /dev/stdin\n\n");
	exit(1);
}

static void sighandler(int signum)
{
	fprintf(stderr, "Signal caught, exiting!\n");
	do_exit = 1;
	rtlsdr_cancel_async(dongle.dev);
}

static void
dongle_data_callback(struct dongle_state *s, void *cbdata,
    const struct dongle_cur_state *cs, unsigned char *buf, uint32_t len)
{
	int i;
	/* XXX global variables, ick */
	struct demod_state *d = &state_demod;

	if (s->mute) {
		for (i=0; i<s->mute; i++) {
			buf[i] = 127;}
		s->mute = 0;
	}
#if 0
	fprintf(stderr, "%s: read %d bytes\n", __func__, len);
#endif

	/* Make signed */
	for (i=0; i<(int)len; i++) {
		s->buf16[i] = buf[i];
		s->buf16[i] -= 127;
	}

	/*
	 * TODO: this isn't yet implemented in the demod or filter
	 * or config phases. But to avoid DC it would be nice to
	 * eventually figure out.
	 */
#if 0
	/* Rotate if required */
	if (!s->offset_tuning) {
		rotate_90(s->buf16, len);}
#endif

	/* DC correct the interleaved data */
	remove_dc(s->buf16, len);

	/*
	 * For now there's no separate thread for demod, so just
	 * direct dispatch into it
	 */
	memcpy(d->lowpassed, s->buf16, 2*len);
	d->lp_len = len;

	/*
	 * Do demodulation.  This puts data into (d->result, 2*d->result_len bytes).
	 */
	full_demod(d);

	/*
	 * Here's where it'd be put into an output thread for
	 * eventual echo'ing to stdout(), or (for what we want)
	 * {audio output, capture}.
	 */

	/* New FFT thread */
	fm_fft_thread_add_samples(state_fm_fft, s->buf16, len);
	fm_fft_thread_start_fft(state_fm_fft);
}

void frequency_range(struct controller_state *s, char *arg)
{
	char *start, *stop, *step;
	int i;
	start = arg;
	stop = strchr(start, ':') + 1;
	stop[-1] = '\0';
	step = strchr(stop, ':') + 1;
	step[-1] = '\0';
	for(i=(int)atofs(start); i<=(int)atofs(stop); i+=(int)atofs(step))
	{
		s->freqs[s->freq_len] = (uint32_t)i;
		s->freq_len++;
		if (s->freq_len >= FREQUENCIES_LIMIT) {
			break;}
	}
	stop[-1] = ':';
	step[-1] = ':';
}

void sanity_checks(void)
{
	if (controller.freq_len == 0) {
		fprintf(stderr, "Please specify a frequency.\n");
		exit(1);
	}

	if (controller.freq_len >= FREQUENCIES_LIMIT) {
		fprintf(stderr, "Too many channels, maximum %i.\n", FREQUENCIES_LIMIT);
		exit(1);
	}
}

static void
fm_fft_thread_cb(struct fm_fft_thread *fm, void *cbdata, int *db, int n)
{

	/* Give the UI thread more data to display */
	fm_sdl_update_db(&state_sdl, db, n);

	/* XXX turn this into an SDL message! */
	fm_sdl_signal_ready(&state_sdl);
}

int main(int argc, char **argv)
{
#ifndef _WIN32
	struct sigaction sigact;
#endif
	int r, opt;
	int dev_given = 0;
	int custom_ppm = 0;

	dongle_init(&dongle);
	dongle.rate = DEF_SAMPLE_RATE;
	dongle_set_callback(&dongle, dongle_data_callback, NULL);
	controller_init(&controller, &dongle);

	/* For now - straight FM demod; default buf length, 24KHz sample rate */
	demod_init(&state_demod, NULL, MAXIMUM_BUF_LENGTH, 24000);
	demod_set(&state_demod, fm_demod);

	/* XXX 1024 - how many FFT bins */
	fm_sdl_init(&state_sdl, DEF_FFT_POINTS);

	state_fm_fft = fm_fft_thread_create(DEF_FFT_POINTS, DEF_SAMPLE_RATE);
	if (state_fm_fft == NULL) {
		fprintf(stderr, "Failed to create FFT thread\n");
		exit(127);
	}

	fm_fft_thread_set_callback(state_fm_fft, fm_fft_thread_cb, NULL);

	while ((opt = getopt(argc, argv, "d:f:g:s:b:l:o:t:r:p:E:F:A:M:h")) != -1) {
		switch (opt) {
		case 'd':
			dongle.dev_index = verbose_device_search(optarg);
			dev_given = 1;
			break;
		case 'f':
			if (controller.freq_len >= FREQUENCIES_LIMIT) {
				break;}
			if (strchr(optarg, ':'))
				{frequency_range(&controller, optarg);}
			else
			{
				controller.freqs[controller.freq_len] = (uint32_t)atofs(optarg);
				controller.freq_len++;
			}
			break;
		case 'g':
			dongle.gain = (int)(atof(optarg) * 10);
			break;
		case 'p':
			dongle.ppm_error = atoi(optarg);
			custom_ppm = 1;
			break;
		case 'E':
			if (strcmp("edge",  optarg) == 0) {
				controller.edge = 1;}
			if (strcmp("direct",  optarg) == 0) {
				dongle.direct_sampling = 1;}
			if (strcmp("offset",  optarg) == 0) {
				dongle.offset_tuning = 1;}
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}

	sanity_checks();

	if (!dev_given) {
		dongle.dev_index = verbose_device_search("0");
	}

	if (dongle.dev_index < 0) {
		exit(1);
	}

	/* XXX ACTUAL_BUF_LEN calculation! */

	r = rtlsdr_open(&dongle.dev, (uint32_t)dongle.dev_index);
	if (r < 0) {
		fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dongle.dev_index);
		exit(1);
	}

	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);

	/* Set the tuner gain */
	if (dongle.gain == AUTO_GAIN) {
		verbose_auto_gain(dongle.dev);
	} else {
		dongle.gain = nearest_gain(dongle.dev, dongle.gain);
		verbose_gain_set(dongle.dev, dongle.gain);
	}

	verbose_ppm_set(dongle.dev, dongle.ppm_error);

	//r = rtlsdr_set_testmode(dongle.dev, 1);

	/* Reset endpoint before we start reading from it (mandatory) */
	verbose_reset_buffer(dongle.dev);

	(void) controller_thread_start(&controller);
	usleep(100000);
	(void) dongle_thread_start(&dongle);
	(void) fm_sdl_thread_start(&state_sdl);
	(void) fm_fft_thread_start(state_fm_fft);

	while (!do_exit) {
		usleep(100000);
	}

	if (do_exit) {
		fprintf(stderr, "\nUser cancel, exiting...\n");}
	else {
		fprintf(stderr, "\nLibrary error %d, exiting...\n", r);}

	fm_fft_thread_signal_exit(state_fm_fft);
	fm_sdl_thread_signal_exit(&state_sdl);
	dongle_shutdown(&dongle);
	pthread_join(dongle.thread, NULL);

	controller_shutdown(&controller);
	controller_thread_join(&controller);

	fm_sdl_thread_join(&state_sdl);
	fm_fft_thread_join(state_fm_fft);

	//dongle_cleanup(&dongle);
	controller_cleanup(&controller);

	rtlsdr_close(dongle.dev);
	return r >= 0 ? r : -r;
}

// vim: tabstop=8:softtabstop=8:shiftwidth=8:noexpandtab
