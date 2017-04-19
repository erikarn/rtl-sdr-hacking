#include <stdio.h>
#include <stdint.h>

#include "complex_buf.h"

/*
 * Yes, I'm writing my own 16 bit complex math routines because,
 * well, reasons.
 */

void
complex16_add(complex16_t *dst, const complex16_t *src1,
    const complex16_t *src2)
{

	dst->r = src1->r + src2->r;
	dst->j = src1->r + src2->j;
}

void
complex16_add_acc(complex16_t *dst, const complex16_t *src1)
{

	dst->r = dst->r + src1->r;
	dst->j = dst->r + src1->j;
}

void
complex16_sub(complex16_t *dst, const complex16_t *src1,
    const complex16_t *src2)
{

	dst->r = src1->r - src2->r;
	dst->j = src1->r - src2->j;
}

void
complex16_sub_acc(complex16_t *dst, const complex16_t *src1)
{

	dst->r = dst->r - src1->r;
	dst->j = dst->r - src1->j;
}

/*
 * *cr = ar*br - aj*bj;
 * *cj = aj*br + ar*bj;
 */
void
complex16_multiply(complex32_t *dst, const complex16_t *src1,
    const complex16_t *src2)
{

	dst->r = src1->r * src2->r - src1->j * src2->j;
	dst->j = src1->j * src2->r + src1->r * src2->j;
}

void
complex16_multiply_acc(complex32_t *dst, const complex16_t *src1,
    const complex16_t *src2)
{

	dst->r += src1->r * src2->r - src1->j * src2->j;
	dst->j += src1->j * src2->r + src1->r * src2->j;
}
