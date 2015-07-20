#ifndef	__FM_OUTPUT_H__
#define	__FM_OUTPUT_H__

struct output_state
{
        int      exit_flag;
        pthread_t thread;
        FILE     *file;
        char     *filename;
        int16_t  result[MAXIMUM_BUF_LENGTH];
        int      result_len;
        int      rate;
        pthread_rwlock_t rw;
        pthread_cond_t ready;
        pthread_mutex_t ready_m;
};

extern void output_init(struct output_state *s);
extern void output_cleanup(struct output_state *s);

#endif
