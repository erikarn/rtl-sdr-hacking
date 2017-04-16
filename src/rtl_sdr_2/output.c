#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <pthread.h>

#include "config.h"
#include "output.h"

/*
 * Generic output handling and thread stuff.
 */

/* more cond dumbness */
#define safe_cond_signal(n, m) pthread_mutex_lock(m); pthread_cond_signal(n); pthread_mutex_unlock(m)
#define safe_cond_wait(n, m) pthread_mutex_lock(m); pthread_cond_wait(n, m); pthread_mutex_unlock(m)

static void
*output_thread_fn(void *arg)
{
	struct output_state *s = arg;
	while (! s->do_exit) {
		// use timedwait and pad out under runs
		safe_cond_wait(&s->ready, &s->ready_m);
		pthread_rwlock_rdlock(&s->rw);

		/* NOTE: This is supposed to write lots of int16_t entries .. */
		output_write(s);
//		fwrite(s->result, 2, s->result_len, s->file);

		pthread_rwlock_unlock(&s->rw);
	}
	return 0;
}

void
output_run(struct output_state *s)
{
	pthread_create(&s->thread, NULL, output_thread_fn, (void *) s);
}

void
output_set_exit(struct output_state *s)
{

	/* XXX locking */
	s->do_exit = 1;
}

void
output_append(struct output_state *s, const int16_t *buf, int len)
{

	/* XXX TODO: this doesn't /really/ append ... */
	pthread_rwlock_wrlock(&s->rw);
	memcpy(s->result, buf, len * 2);
	s->result_len = len;
	pthread_rwlock_unlock(&s->rw);
	safe_cond_signal(&s->ready, &s->ready_m);
}


void output_init(struct output_state *s)
{

	s->rate = DEFAULT_SAMPLE_RATE;
	pthread_rwlock_init(&s->rw, NULL);
	pthread_cond_init(&s->ready, NULL);
	pthread_mutex_init(&s->ready_m, NULL);
}

void output_cleanup(struct output_state *s)
{

	pthread_rwlock_destroy(&s->rw);
	pthread_cond_destroy(&s->ready);
	pthread_mutex_destroy(&s->ready_m);
}


int
output_close(struct output_state *s)
{

	return (s->output_close_cb(s));
}

int
output_write(struct output_state *s)
{

	return (s->output_write_cb(s));
}

int
output_set_rate(struct output_state *s, int rate)
{

	s->rate = rate;
	return (0);
}
