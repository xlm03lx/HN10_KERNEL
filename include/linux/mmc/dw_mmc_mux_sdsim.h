#ifndef _DW_MMC_MUX_SD_SIM_
#define _DW_MMC_MUX_SD_SIM_

#include <linux/semaphore.h>


#define MODULE_SD  0
#define MODULE_SIM  1

#define GPIO_VALUE_LOW     0
#define GPIO_VALUE_HIGH    1

#define MUX_SDSIM_LOG_TAG "[MUX_SDSIM][mmc1][hi_mci.1]"


enum sdsim_gpio_mode {
	SDSIM_MODE_GPIO_DETECT = 0, /*gpio detect mode for detect sd or sim*/
	SDSIM_MODE_SD_NORMAL = 1, /*sd normal mode*/
	SDSIM_MODE_SD_IDLE = 2, /*sd idle/lowpower mode*/
	SDSIM_MODE_SIM_NORMAL = 3, /*sim normal mode*/
	SDSIM_MODE_SIM_IDLE = 4, /*sim idle/lowpower mode*/
};

/*hisilicon iomux xml and pinctrl framework can't support such SD-SIM-IO-MUX case,wo need config different five modes here manully in code*/
int config_sdsim_gpio_mode(enum sdsim_gpio_mode gpio_mode);

char* detect_status_to_string(void);

/*
status=1 means plug out;
status=0 means plug in;
*/
#define STATUS_PLUG_IN 0
#define STATUS_PLUG_OUT 1

/*
Description: while sd/sim plug in or out, gpio_cd detect pin is actived,we need call this sd_sim_detect_run function to make sure sd or sim which is inserted

dw_mci_host: MODULE_SD use dw_mci_host argu as input, while MODULE_SIM just use NULL
status: use STATUS_PLUG_IN or STATUS_PLUG_OUT by gpio_cd detect pin's value
current_module: sd or sim which module is calling this function

return value:return STATUS_PLUG_IN or STATUS_PLUG_OUT,just tell current_module sd or sim is inserted or not, and current_module can update gpio_cd detect pin value by this return value
*/
int sd_sim_detect_run(void *dw_mci_host, int status, int current_module, int need_sleep);

extern struct semaphore sem_mux_sdsim_detect;
extern struct dw_mci *host_from_sd_module;


#define SD_SIM_DETECT_STATUS_UNDETECTED            0
#define SD_SIM_DETECT_STATUS_SD                    1
#define SD_SIM_DETECT_STATUS_SIM                   2
#define SD_SIM_DETECT_STATUS_ERROR                 3

extern int sd_sim_detect_status_current;

#define SLEEP_MS_TIME_FOR_DETECT_UNSTABLE   400

#endif
