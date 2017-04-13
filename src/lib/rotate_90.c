#include <stdio.h>
#include <stdint.h>

#include "rotate_90.h"

void
rotate_90(int16_t *buf, uint32_t len)
/* 90 rotation is 1+0j, 0+1j, -1+0j, 0-1j
   or [0, 1, -3, 2, -4, -5, 7, -6] */
{
	uint32_t i;
	int16_t tmp;
	for (i=0; i<len; i+=8) {
		tmp = - buf[i+3];
		buf[i+3] = buf[i+2];
		buf[i+2] = tmp;

		buf[i+4] = - buf[i+4];
		buf[i+5] = - buf[i+5];

		tmp = - buf[i+6];
		buf[i+6] = buf[i+7];
		buf[i+7] = tmp;
	}
}
