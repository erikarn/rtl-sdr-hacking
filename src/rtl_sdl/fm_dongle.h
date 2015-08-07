#ifndef	__FM_DONGLE_H__
#define	__FM_DONGLE_H__

struct dongle_state;
typedef	void dongle_data_cb(struct dongle_state *d, void *cbdata,
	    unsigned char *buf, uint32_t len);

struct dongle_state
{
        pthread_t thread;
        rtlsdr_dev_t *dev;
        int      dev_index;
        uint32_t freq;
        uint32_t rate;
        int      gain;
        int16_t buf16[MAXIMUM_BUF_LENGTH];
        uint32_t buf_len;
        int      ppm_error;
        int      offset_tuning;
        int      direct_sampling;
        int      mute;
	int do_exit;
	struct {
		dongle_data_cb *cb;
		void *cbdata;
	} cb;
};

extern	void dongle_init(struct dongle_state *s);
extern	void dongle_shutdown(struct dongle_state *s);
extern	void dongle_set_callback(struct dongle_state *s, dongle_data_cb *cb,
	    void *cbdata);
extern	int dongle_thread_start(struct dongle_state *s);
extern	void dongle_thread_join(struct dongle_state *s);

#endif
