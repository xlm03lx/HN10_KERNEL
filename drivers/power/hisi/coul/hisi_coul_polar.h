#ifndef _HISI_COUL_POLAR_H_
#define _HISI_COUL_POLAR_H_

extern void stop_polar_sample(void);
extern void start_polar_sample(void);
extern void get_resume_polar_info(int sleep_cc, int sleep_time);
extern void sync_sample_info(void);
extern int polar_params_calculate(struct polar_calc_info* polar,
                                       int ocv_soc_mv, int vol_now, int cur);

#endif
