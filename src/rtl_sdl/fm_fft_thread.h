#ifndef	__FM_FFT_THREAD_H__
#define	__FM_FFT_THREAD_H__

struct fm_fft_thread {
	pthread_mutex_t ft_mtx;
	pthread_cond_t ft_c;
	pthread_t ft_thr;
	int do_exit;

	int nsamples;		/* This is required for the dB conversion */

	struct fm_fft_state fft;
};

extern	struct fm_fft_thread *fm_fft_thread_create(int nbins, int nsamples);
extern	int fm_fft_thread_start(struct fm_fft_thread *ft);
extern	int fm_fft_thread_add_samples(struct fm_fft_thread *ft, int16_t *s,
	    int nsamples);
extern	int fm_fft_thread_start_fft(struct fm_fft_thread *ft);
extern	int fm_fft_thread_signal_exit(struct fm_fft_thread *ft);

#endif	/* __FM_FFT_THREAD_H__ */
