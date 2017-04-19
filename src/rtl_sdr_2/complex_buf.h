#ifndef	__COMPLEX_BUF_H__
#define	__COMPLEX_BUF_H__

/*
 * Yes, I'm writing my own 16 bit complex math routines because,
 * well, reasons.
 */

struct complex16 {
	int16_t r;	/* real */
	int16_t j;	/* imaginary */
};

typedef struct complex16 complex16_t;

struct complex32 {
	int32_t r;	/* real */
	int32_t j;	/* imaginary */
};

typedef struct complex32 complex32_t;


struct complex16_ring {
	complex16_t *buf;
	int head;		/* head entry */
	int tail;		/* tail entry */
	int size;		/* how big buf is, in complex16's */
};

/*
 * Single entry operations.
 */
extern	void complex16_add(complex16_t *dst, const complex16_t *src1,
	    const complex16_t *src2);
extern	void complex16_add_acc(complex16_t *dst, const complex16_t *src);

extern	void complex16_sub(complex16_t *dst, const complex16_t *src1,
	    const complex16_t *src2);
extern	void complex16_sub_acc(complex16_t *dst, const complex16_t *src);

extern	void complex16_multiply(complex32_t *dst, const complex16_t *src1,
	    const complex16_t *src2);
extern	void complex16_multiply_acc(complex32_t *dst, const complex16_t *src1,
	    const complex16_t *src2);

extern	void complex16_conjugate(complex16_t *dst, const complex16_t *src);
extern	void complex16_conjugatei(complex16_t *dst);

/*
 * List operators on a complex16_ring list.
 */

#endif	/* __COMPLEX_BUF__ */

