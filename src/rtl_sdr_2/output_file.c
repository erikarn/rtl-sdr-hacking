#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>

#include <err.h>
#include <pthread.h>

#include "config.h"

#include "output.h"
#include "output_file.h"

struct output_file_state {
	FILE *file;
};

static int
output_file_write_cb(struct output_state *s)
{
	struct output_file_state *sf = s->cbdata;

	/* NOTE: This is supposed to write lots of int16_t entries .. */
	fwrite(s->result, 2, s->result_len, sf->file);
	return (0);
}

static int
output_file_close_cb(struct output_state *s)
{
	struct output_file_state *sf = s->cbdata;

	if (sf->file != stdout) {
		fclose(sf->file);
	}

	return (0);
}

int
output_file_init(struct output_state *s)
{
	struct output_file_state *sf;

	sf = calloc(1, sizeof(*sf));
	if (sf == NULL) {
		warn("%s: calloc", __func__);
		return (-1);
	}

	s->cbdata = sf;
	s->output_write_cb = output_file_write_cb;
	s->output_close_cb = output_file_close_cb;

	return (0);
}

int
output_file_set_filename(struct output_state *s, const char *fn)
{
	struct output_file_state *sf = s->cbdata;

	if (strcmp(fn, "-") == 0) {
		sf->file = stdout;
#ifdef _WIN32
		_setmode(_fileno(sf->file), _O_BINARY);
#endif
		return (0);
	}

	sf->file = fopen(fn, "wb");
	if (sf->file == NULL) {
		fprintf(stderr, "failed to open file %s\n", fn);
		return (-1);
	}
	return (0);
}
