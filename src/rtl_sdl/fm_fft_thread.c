#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <pthread.h>
#include <pthread_np.h>
#include <sys/types.h>
#include <math.h>

#include <fftw3.h>

#include "fm_fft.h"
#include "fm_fft_thread.h"

struct fm_fft_thread *
fm_fft_thread_create(int nbins, int nsamples)
{
	struct fm_fft_thread *ft;

	ft = calloc(1, sizeof(*ft));
	if (ft == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	ft->nsamples = nsamples;
	fm_fft_init(&ft->fft, nbins);

	pthread_mutex_init(&ft->ft_mtx, NULL);
	pthread_cond_init(&ft->ft_c, NULL);

	return (ft);
}

void *
fm_fft_thread_thr(void *arg)
{
	struct fm_fft_thread *ft = arg;
	int r;

	/* Start! */
	while (1) {
		pthread_mutex_lock(&ft->ft_mtx);
		if (ft->do_exit == 1) {
			pthread_mutex_unlock(&ft->ft_mtx);
			break;
		}

		r = pthread_cond_wait(&ft->ft_c, &ft->ft_mtx);
		if (r != 0) {
			warn("%s: pthread_cond_wait", __func__);
			sleep(1);	/* XXX sleep? */
			continue;
		}

		/* Ok, if there's data, run the FFT */
		/*
		 * XXX TODO: it's done inside the lock,
		 * because the lock serialises access to
		 * the fft arrays.  We should eventually
		 * just create an array for incoming samples,
		 * and populate that under the lock.
		 */
		fm_fft_run(&ft->fft);

		/* .. do dB conversion */
		fft_to_db(&ft->fft, ft->nsamples);

		pthread_mutex_unlock(&ft->ft_mtx);

		if (ft->fft_done_cb.cb != NULL) {
			ft->fft_done_cb.cb(ft, ft->fft_done_cb.cbdata,
			    ft->fft.fft_db,
			    ft->fft.fft_npoints);
		}
	}

	return (NULL);
}

int
fm_fft_thread_start(struct fm_fft_thread *ft)
{
	int r;

	r = pthread_create(&ft->ft_thr, NULL, fm_fft_thread_thr, ft);
	if (r != 0) {
		warn("%s: pthread_create", __func__);
		return (-1);
	}

	pthread_set_name_np(ft->ft_thr, "fft_thread");

	return (0);
}

int
fm_fft_thread_add_samples(struct fm_fft_thread *ft, int16_t *s, int nsamples)
{
	int r;

	pthread_mutex_lock(&ft->ft_mtx);
	r = fm_fft_update_fft_samples(&ft->fft, s, nsamples);
	pthread_mutex_unlock(&ft->ft_mtx);

	return (r);
}

int
fm_fft_thread_start_fft(struct fm_fft_thread *ft)
{

	pthread_mutex_lock(&ft->ft_mtx);
	pthread_cond_signal(&ft->ft_c);
	pthread_mutex_unlock(&ft->ft_mtx);

	return (0);
}

int
fm_fft_thread_signal_exit(struct fm_fft_thread *ft)
{
	pthread_mutex_lock(&ft->ft_mtx);
	ft->do_exit = 1;
	pthread_cond_signal(&ft->ft_c);
	pthread_mutex_unlock(&ft->ft_mtx);

	return (0);
}

void
fm_fft_thread_set_callback(struct fm_fft_thread *ft, fm_fft_completed_cb *cb,
    void *cbdata)
{

	pthread_mutex_lock(&ft->ft_mtx);
	ft->fft_done_cb.cb = cb;
	ft->fft_done_cb.cbdata = cbdata;
	pthread_mutex_unlock(&ft->ft_mtx);
}

void
fm_fft_thread_join(struct fm_fft_thread *ft)
{

	(void) pthread_join(ft->ft_thr, NULL);
}
