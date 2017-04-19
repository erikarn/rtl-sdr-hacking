#ifndef	__RESAMPLE_H__
#define	__RESAMPLE_H__

/* linear interpolation, len1 < len2 */
extern	void arbitrary_upsample(int16_t *buf1, int16_t *buf2, int len1,
	    int len2);

/* fractional boxcar lowpass, len1 > len2 */
extern	void arbitrary_downsample(int16_t *buf1, int16_t *buf2, int len1,
	    int len2);

/* up to you to calculate lengths and make sure it does not go OOB
 * okay for buffers to overlap, if you are downsampling */
extern	void arbitrary_resample(int16_t *buf1, int16_t *buf2, int len1,
	    int len2);

#endif	/* __RESAMPLE_H__ */
