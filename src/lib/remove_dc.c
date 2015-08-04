
#include <stdio.h>
#include <sys/types.h>

#include "remove_dc.h"

void
remove_dc(int16_t *data, int length)
{
	int i;
	int16_t avg_i, avg_q;
	long long sum_i, sum_q;

	sum_i = sum_q = 0LL;

#if 0
	fprintf(stderr, "%s: called; length=%d\n", __func__, length);
#endif

	for (i = 0; i < length; i += 2) {
		sum_i += data[i];
		sum_q += data[i+1];
	}

	avg_i = (int16_t)(sum_i / (long long) (length/2));
	avg_q = (int16_t)(sum_q / (long long) (length/2));

	/* shortcut if they're both 0 */
	if (avg_i == 0)
		return;
	if (avg_q == 0)
		return;

	fprintf(stderr, "%s: avg_i=%d, avg_q=%d\n",
	    __func__,
	    (int) avg_i,
	    (int) avg_q);


	for (i = 0; i < length; i += 2) {
		data[i] -= avg_i;
		data[i+1] -= avg_q;
	}
}
