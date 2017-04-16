#ifndef	__OUTPUT_ALSA_H__
#define	__OUTPUT_ALSA_H__

extern	int output_alsa_init(struct output_state *s);
extern	int output_alsa_set_device(struct output_state *s, const char *devname);

#endif
