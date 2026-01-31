#ifndef _BOARD_PWMV2_H
#define _BOARD_PWMV2_H

#include "hpm_common.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

void board_init_pwm(void);
uint32_t board_pwm_calc_reload(uint32_t period_us);
void board_pwm_set_reload(uint32_t period_us);
void board_pwmv2_start_out(void);
void board_pwm_set_duty_phase(double dutyh1, double dutyh2, double phase);


#if defined(__cplusplus)
}

#endif    /* __cplusplus */
#endif    //_BOARD_PWMV2_H