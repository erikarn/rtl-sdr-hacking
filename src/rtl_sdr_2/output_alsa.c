#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <alsa/asoundlib.h>
#include <poll.h>

#include <pthread.h>

#include "config.h"
#include "output.h"
#include "output_alsa.h"

struct output_state_alsa {
	snd_pcm_t *phdl;
};

static int
output_alsa_write_cb(struct output_state *s)
{
	struct output_state_alsa *sa = s->cbdata;
	int err;

	if (sa->phdl == NULL)
		return (0);

#if 1
	/*
	 * writei is "interleaved".  we pass in the number of samples,
	 * not the number of bytes.
	 */
	err = snd_pcm_writei(sa->phdl, s->result, s->result_len);
	if (err < 0) {
		fprintf(stderr, "%s: snd_pcm_writei failed (%s)\n",
		    __func__,
		    snd_strerror (err));
	}

	if (err < 0) {
		snd_pcm_recover(sa->phdl, err, 0);
	}
#endif

	return (0);
}

static int
output_alsa_close_cb(struct output_state *s)
{
	struct output_state_alsa *sa = s->cbdata;

	if (sa->phdl != NULL)
		snd_pcm_close(sa->phdl);
	return (0);
}

int
output_alsa_init(struct output_state *s)
{
	struct output_state_alsa *sa;

	sa = calloc(1, sizeof(*sa));
	if (sa == NULL) {
		warn("%s: calloc", __func__);
		return (-1);
	}

	s->cbdata = sa;
	s->output_write_cb = output_alsa_write_cb;
	s->output_close_cb = output_alsa_close_cb;

	return (0);
}

/* Set the playback config - for now, 32,000, int16_t, mono */
/* XXX TODO: le16? be16? interleaved? etc */
static int
output_alsa_set_hw_params(struct output_state *s)
{
	struct output_state_alsa *sa = s->cbdata;
	snd_pcm_hw_params_t *hw_params;
	int err;

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	if ((err = snd_pcm_hw_params_any(sa->phdl, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	if ((err = snd_pcm_hw_params_set_access(sa->phdl,
	    hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	/* XXX TODO: does the actual math generator /generate/ LE samples? */
	/* XXX or is it generating host-order samples */
	if ((err = snd_pcm_hw_params_set_format(sa->phdl,
	    hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	/*
	 * Attempt to set exact sample rate. There's a "set_rate_near"
	 * which takes a speed/dir and returns what the hardware
	 * was actually configured as.
	 */
	if ((err = snd_pcm_hw_params_set_rate(sa->phdl,
	    hw_params, s->rate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	/*
	 * XXX hard-coded number of channels
	 */
	if ((err = snd_pcm_hw_params_set_channels(sa->phdl,
	    hw_params, 1)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	/*
	 * Setup the buffer size. It defaults to 1MB?
	 * XXX hard-coded size
	 */
	err = snd_pcm_hw_params_set_buffer_size(sa->phdl,
	    hw_params, 65536);
	if (err < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
		 snd_strerror (err));
		return (err);
	}

	if ((err = snd_pcm_hw_params(sa->phdl, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
		 snd_strerror (err));
		return (err);
	}

	snd_pcm_hw_params_free(hw_params);

	return (0);
}

/*
 * Set thresholds, we will start playback.
 */
static int
output_alsa_set_sw_params(struct output_state *s)
{
	struct output_state_alsa *sa = s->cbdata;
	snd_pcm_sw_params_t *sw_params;
	int err;

	/*
	 * tell ALSA to wake us up whenever CLIENT_PLAYBACK_BUF_MAXSIZE or
	 * more frames of playback data can be delivered. Also, tell
	 * ALSA that we'll start the device ourselves.
	 */
	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		fprintf (stderr, "cannot allocate software parameters structure (%s)\n",
			 snd_strerror (err));
		return (err);
	}
	if ((err = snd_pcm_sw_params_current(sa->phdl, sw_params)) < 0) {
		fprintf (stderr, "cannot initialize software parameters structure (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	/* XXX hard-coded min */
	if ((err = snd_pcm_sw_params_set_avail_min(sa->phdl,
	    sw_params, 32768)) < 0) {
		fprintf (stderr, "cannot set minimum available count (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	/* XXX hard-coded start threshold */
	if ((err = snd_pcm_sw_params_set_start_threshold(sa->phdl,
	    sw_params, 16384)) < 0) {
		fprintf (stderr, "cannot set start mode (%s)\n",
			 snd_strerror (err));
		return (err);
	}
	if ((err = snd_pcm_sw_params(sa->phdl, sw_params)) < 0) {
		fprintf (stderr, "cannot set software parameters (%s)\n",
			 snd_strerror (err));
		return (err);
	}

	snd_pcm_sw_params_free(sw_params);

	return (0);
}

int
output_alsa_set_device(struct output_state *s, const char *devname)
{
	struct output_state_alsa *sa = s->cbdata;
	int err;

	if ((err = snd_pcm_open(&sa->phdl, devname,
	    SND_PCM_STREAM_PLAYBACK,
	    0 /* SND_PCM_NONBLOCK */)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n",
		    devname,
		    snd_strerror (err));
		return (-1);
	}

	printf("%s: output rate: %d\n", __func__, s->rate);

	/* XXX TODO: explicitly set non-blocking by calling snd_pcm_nonblock() ? */

	(void) output_alsa_set_hw_params(s);
	(void) output_alsa_set_sw_params(s);

	/*
	 * Prepare for starting playback.
	 */
	if ((err = snd_pcm_prepare(sa->phdl)) < 0) {
		fprintf(stderr,
		"%s: cannot prepare audio interface for use (%s)\n",
		__func__,
		snd_strerror(err));
	}

	return (0);
}
