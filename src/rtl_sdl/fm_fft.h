#ifndef	__FM_FFT_H__
#define	__FM_FFT_H__

struct fm_fft_state {
	int fft_npoints;	/* how big is this FFT? */
	fftw_complex *fft_in;
	fftw_complex *fft_out;
	fftw_plan fft_p;
	int *fft_db;		/* fft result, converted to dB */
};

extern	int fm_fft_init(struct fm_fft_state *fs, int nbins);
extern	int fm_fft_run(struct fm_fft_state *fs);
extern	int fm_fft_update_fft_samples(struct fm_fft_state *fs, int16_t *s,
	    int n);
extern	void fft_to_db(struct fm_fft_state *fm, int nsamples);

#endif
