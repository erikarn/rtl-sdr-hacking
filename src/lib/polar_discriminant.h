#ifndef	__POLAR_DISCRIMINANT_H__
#define	__POLAR_DISCRIMINANT_H__

extern	int polar_discriminant(int ar, int aj, int br, int bj);
extern	int fast_atan2(int y, int x);
extern	int polar_disc_fast(int ar, int aj, int br, int bj);
extern	int atan_lut_init(void);
extern	int polar_disc_lut(int ar, int aj, int br, int bj);

#endif
