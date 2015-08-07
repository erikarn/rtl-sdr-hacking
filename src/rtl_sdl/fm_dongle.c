/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Hoernchen <la@tfc-server.de>
 * Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
 * Copyright (C) 2013 by Elias Oenal <EliasOenal@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * written because people could not do real time
 * FM demod on Atom hardware with GNU radio
 * based on rtl_sdr.c and rtl_tcp.c
 *
 * lots of locks, but that is okay
 * (no many-to-many locks)
 *
 * todo:
 *       sanity checks
 *       scale squelch to other input parameters
 *       test all the demodulations
 *       pad output on hop
 *       frequency ranges could be stored better
 *       scaled AM demod amplification
 *       auto-hop after time limit
 *       peak detector to tune onto stronger signals
 *       fifo for active hop frequency
 *       clips
 *       noise squelch
 *       merge stereo patch
 *       merge soft agc patch
 *       merge udp patch
 *       testmode to detect overruns
 *       watchdog to reset bad dongle
 *       fix oversampling
 */

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

void dongle_init(struct dongle_state *s)
{
	s->rate = 1024000;
	s->gain = AUTO_GAIN; // tenths of a dB
	s->mute = 0;
	s->direct_sampling = 0;
	s->offset_tuning = 0;
	s->do_exit = 0;
}

static void
*dongle_thread_fn(void *arg)
{
	struct dongle_state *s = arg;
	int r;
	int n_read;
	uint8_t *buffer;

/* XXX can't be bigger than MAXIMUM_BUF_LENGTH */
#define	OUT_BLOCK_SIZE	524288
	buffer = malloc(OUT_BLOCK_SIZE * sizeof(uint8_t));

	/*
	 * Read data in a loop, push it up via the callback.
	 */
	while (! s->do_exit) {
		r = rtlsdr_read_sync(s->dev, buffer, OUT_BLOCK_SIZE, &n_read);
		if (r < 0) {
			fprintf(stderr, "rtlsdr_read_sync: failed\n");
			break;
		}

		s->cb.cb(s, s->cb.cbdata, buffer, n_read);
	}
	//rtlsdr_read_async(s->dev, rtlsdr_callback, s, 0, s->buf_len);
	return 0;
}

void
dongle_set_callback(struct dongle_state *s, dongle_data_cb *cb, void *cbdata)
{

	/* XXX locking */
	s->cb.cb = cb;
	s->cb.cbdata = cbdata;
}

int
dongle_thread_start(struct dongle_state *s)
{
	int r;

	r = pthread_create(&s->thread, NULL, dongle_thread_fn, s);
	if (r != 0) {
		warn("%s: pthread_create", __func__);
		return (-1);
	}
	return (0);
}

void
dongle_shutdown(struct dongle_state *s)
{

	/* XXX locking */
	s->do_exit = 1;
}

void
dongle_thread_join(struct dongle_state *s)
{

	pthread_join(s->thread, NULL);
}
