#ifndef	__POLAR_DISC_H__
#define	__POLAR_DISC_H__

extern	int polar_discriminant(int ar, int aj, int br, int bj);
extern	int polar_disc_fast(int ar, int aj, int br, int bj);
extern	int atan_lut_init(void);
extern	int polar_disc_lut(int ar, int aj, int br, int bj);

#endif	/* __POLAR_DISC_H__ */
