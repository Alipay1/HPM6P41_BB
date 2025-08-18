#include "board_cp.h"

#include "hpm_pwmv2_drv.h"
#include "board_pwmv2.h"

#define RLD1_2_CP PWMV2_SHADOW_INDEX(0)
#define CMP1_CP PWMV2_SHADOW_INDEX(1)
#define CMP2_CP PWMV2_SHADOW_INDEX(2)
#define CMP3_CP PWMV2_SHADOW_INDEX(3)
#define CMP4_CP PWMV2_SHADOW_INDEX(4)

void board_charge_pump_init(void) {
  uint32_t reload = pwm_calc_reload(10);

  pwmv2_deinit(HPM_PWM2);

  pwmv2_shadow_register_unlock(HPM_PWM2);

  // 用清空的影子寄存器来清空重载值和比较值
  pwmv2_set_reload_update_time(HPM_PWM2, pwm_counter_0, pwm_reload_update_on_shlk);
  pwmv2_set_reload_update_time(HPM_PWM2, pwm_counter_1, pwm_reload_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(2), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(3), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(6), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(7), pwm_shadow_register_update_on_shlk);

  // counter 2 and counter 3 using same reload source
  pwmv2_counter_select_data_offset_from_shadow_value(HPM_PWM2, pwm_counter_0, RLD1_2_CP);
  pwmv2_counter_select_data_offset_from_shadow_value(HPM_PWM2, pwm_counter_1, RLD1_2_CP);

  // clear shadow register value
  pwmv2_set_shadow_val(HPM_PWM2, RLD1_2_CP, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP1_CP, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP2_CP, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP3_CP, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP4_CP, 0, 0, false);

  // 触发SHLK更新然后解锁影子寄存器继续进行配置
  pwmv2_shadow_register_lock(HPM_PWM2);
  pwmv2_shadow_register_unlock(HPM_PWM2);

  // 持续发波 关闭BURST模式
  pwmv2_counter_burst_disable(HPM_PWM2, pwm_counter_0);
  pwmv2_counter_burst_disable(HPM_PWM2, pwm_counter_1);

  pwmv2_counter_select_data_offset_from_shadow_value(HPM_PWM2, pwm_counter_0, RLD1_2_CP);
  pwmv2_counter_select_data_offset_from_shadow_value(HPM_PWM2, pwm_counter_1, RLD1_2_CP);

  // CNT CFG0 更新重载值的时机
  pwmv2_set_reload_update_time(HPM_PWM2, pwm_counter_0, pwm_reload_update_on_reload);
  pwmv2_set_reload_update_time(HPM_PWM2, pwm_counter_1, pwm_reload_update_on_reload);

  // config shadow register as internal compare source
  pwmv2_select_cmp_source(HPM_PWM2, PWMV2_CMP_INDEX(2), cmp_value_from_shadow_val, CMP1_CP);
  pwmv2_select_cmp_source(HPM_PWM2, PWMV2_CMP_INDEX(3), cmp_value_from_shadow_val, CMP2_CP);
  pwmv2_select_cmp_source(HPM_PWM2, PWMV2_CMP_INDEX(6), cmp_value_from_shadow_val, CMP3_CP);
  pwmv2_select_cmp_source(HPM_PWM2, PWMV2_CMP_INDEX(7), cmp_value_from_shadow_val, CMP4_CP);

  // config update timing
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(2), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(3), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(6), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM2, PWMV2_CMP_INDEX(7), pwm_shadow_register_update_on_reload);
  pwmv2_shadow_register_lock(HPM_PWM2);

  // set dead zone
  // pwmv2_set_dead_area(HPM_PWM2, pwm_channel_1, 0);
  // pwmv2_set_dead_area(HPM_PWM2, pwm_channel_3, 0);
  // pwmv2_add_delay_tick_after_dead_area(HPM_PWM2, 0);

  // start output
  pwmv2_enable_multi_counter_sync(HPM_PWM2, (1 << 1) | (1 << 0));
  pwmv2_channel_enable_output(HPM_PWM2, pwm_channel_1);
  pwmv2_channel_enable_output(HPM_PWM2, pwm_channel_3);
  pwmv2_start_pwm_output(HPM_PWM2, pwm_counter_0);
  pwmv2_start_pwm_output(HPM_PWM2, pwm_counter_1);
  pwmv2_reset_counter(HPM_PWM2, pwm_counter_0);
  pwmv2_reset_counter(HPM_PWM2, pwm_counter_1);
  pwmv2_enable_counter(HPM_PWM2, pwm_counter_0);
  pwmv2_enable_counter(HPM_PWM2, pwm_counter_1);

  // set reload value
  pwmv2_shadow_register_unlock(HPM_PWM2);
  pwmv2_set_shadow_val(HPM_PWM2, RLD1_2_CP, reload, 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP1_CP, (uint32_t)(0.000 * reload), 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP2_CP, (uint32_t)(0.125 * reload), 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP3_CP, (uint32_t)(0.125 * reload), 0, false);
  pwmv2_set_shadow_val(HPM_PWM2, CMP4_CP, (uint32_t)(0.250 * reload), 0, false);
  pwmv2_shadow_register_lock(HPM_PWM2);

  // pwmv2_channel_disable_output(HPM_PWM2, pwm_channel_1);
  // pwmv2_channel_disable_output(HPM_PWM2, pwm_channel_3);
  // pwmv2_disable_counter(HPM_PWM2, pwm_counter_0);
  // pwmv2_disable_counter(HPM_PWM2, pwm_counter_1);
  // pwmv2_deinit(HPM_PWM2);
}
