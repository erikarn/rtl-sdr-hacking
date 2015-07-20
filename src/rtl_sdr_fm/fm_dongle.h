#ifndef	__FM_DONGLE_H__
#define	__FM_DONGLE_H__


struct dongle_state
{
        int      exit_flag;
        pthread_t thread;
        rtlsdr_dev_t *dev;
        int      dev_index;
        uint32_t freq;
        uint32_t rate;
        int      gain;
        uint16_t buf16[MAXIMUM_BUF_LENGTH];
        uint32_t buf_len;
        int      ppm_error;
        int      offset_tuning;
        int      direct_sampling;
        int      mute;
        struct demod_state *demod_target;
};

extern	void dongle_init(struct dongle_state *s, struct demod_state *demod);

#endif
