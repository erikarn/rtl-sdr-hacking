#include <stdio.h>
#include <stdlib.h>

#include <alsa/asoundlib.h>
#include <poll.h>

#include <pthread.h>

#include "fm_output.h"
#include "fm_output_alsa.h"

struct fm_output_alsa {
	int fd;
};

void
fm_output_alsa_init(struct fm_output *output)
{

}
