#include "board_pwmv2.h"
#include "board.h"

#include "hpm_pwmv2_drv.h"
#include "hpm_synt_drv.h"
#include "hpm_trgm_drv.h"
#include <math.h>

#include "def_rtt_printf.h"

#define RLD2 PWMV2_SHADOW_INDEX(0)
#define RLD3 PWMV2_SHADOW_INDEX(1)
#define CMP1 PWMV2_SHADOW_INDEX(2)
#define CMP2 PWMV2_SHADOW_INDEX(3)
#define CMP3 PWMV2_SHADOW_INDEX(4)
#define CMP4 PWMV2_SHADOW_INDEX(5)
#define PHASE PWMV2_SHADOW_INDEX(21)
#define ADC_TRG1 PWMV2_SHADOW_INDEX(6)
#define ADC_TRG2 PWMV2_SHADOW_INDEX(7)

#define PWM_PRIVATE_CLAC_0 0
#define PWM_PRIVATE_CLAC_1 1
#define PWM_PRIVATE_CLAC_2 2
#define PWM_PRIVATE_CLAC_3 3

static volatile uint32_t reload = 0;

uint32_t board_pwm_calc_reload(uint32_t period_us) {
  uint32_t t_reload, t_freq;
  t_freq   = clock_get_frequency(clock_ahb0);
  t_reload = t_freq / 1000 / 1000 * period_us - 1;
  return t_reload;
}

void board_init_pwm(void) {
  init_pwm_pins();

  LOG("PWM1 CLK:%lu\r\n", clock_get_frequency(clock_ahb0));
}

void board_pwm_set_reload(uint32_t period_us){
  if (period_us <=0) {
    return;
  }
  reload = board_pwm_calc_reload(period_us);
  synt_set_reload(HPM_SYNT, reload);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_0, reload);
  pwmv2_shadow_register_unlock(HPM_PWM1);
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, reload, 0, false);
  pwmv2_shadow_register_lock(HPM_PWM1);
}

void board_pwm_set_duty_phase(double dutyh1, double dutyh2, double phase) {
  pwmv2_shadow_register_unlock(HPM_PWM1);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, reload * dutyh1, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP4, reload * dutyh2, 0, false);
  //pwmv2_set_shadow_val(HPM_PWM1, PHASE, phase * reload / (2 * M_PI), 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, PHASE, phase * reload / 360.0f, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, ADC_TRG1, reload * ((1 - dutyh1) * 0.5 + dutyh1), 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, ADC_TRG2, reload * ((1 - dutyh2) * 0.5 + dutyh2), 0, false);
  pwmv2_shadow_register_lock(HPM_PWM1);
}

void board_pwmv2_start_out(void) {
  reload = board_pwm_calc_reload(5);

  pwmv2_deinit(HPM_PWM1);

  trgm_output_t trgm0_io_config = {0};

  trgm0_io_config.invert = 0;
  trgm0_io_config.type   = trgm_output_pulse_at_input_rising_edge;
  trgm0_io_config.input  = HPM_TRGM0_INPUT_SRC_SYNT_CH00;
  trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_PWM1_TRIG_IN2, &trgm0_io_config);

  trgm0_io_config.invert = 0;
  trgm0_io_config.type   = trgm_output_pulse_at_input_rising_edge;
  trgm0_io_config.input  = HPM_TRGM0_INPUT_SRC_SYNT_CH00;
  trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_PWM1_TRIG_IN3, &trgm0_io_config);

  pwmv2_shadow_register_unlock(HPM_PWM1);

  // 用清空的影子寄存器来清空重载值和比较值
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_shlk);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_3, pwm_reload_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(12), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(13), pwm_shadow_register_update_on_shlk);

  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(10), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(14), pwm_shadow_register_update_on_shlk);

  // counter 2 and counter 3 using same reload source
  pwmv2_counter_select_data_offset_from_shadow_value(HPM_PWM1, pwm_counter_2, RLD2);
  pwmv2_counter_select_data_offset_from_shadow_value(HPM_PWM1, pwm_counter_3, RLD2);

  // clear shadow register value
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP3, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP4, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, PHASE, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, ADC_TRG1, 1, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, ADC_TRG2, 1, 0, false);

  // 触发SHLK更新然后解锁影子寄存器继续进行配置
  pwmv2_shadow_register_lock(HPM_PWM1);
  pwmv2_shadow_register_unlock(HPM_PWM1);

  // 持续发波 关闭BURST模式
  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_2);
  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_3);

  // CNT CFG0 更新重载值的时机
  //pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_reload);
  //pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_3, pwm_reload_update_on_reload);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_trigger);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_3, pwm_reload_update_on_trigger);
  pwmv2_reload_select_input_trigger(HPM_PWM1, pwm_counter_2, 2);
  pwmv2_reload_select_input_trigger(HPM_PWM1, pwm_counter_3, 3);

  //pwmv2_counter_enable_reload_by_trig(HPM_PWM1, pwm_counter_2);
  //pwmv2_counter_enable_reload_by_trig(HPM_PWM1, pwm_counter_3);
  //pwmv2_set_counter_reload_trigmux_index(HPM_PWM1, pwm_counter_2, 2);
  //pwmv2_set_counter_reload_trigmux_index(HPM_PWM1, pwm_counter_3, 3);

   pwmv2_counter_start_trigger_enable(HPM_PWM1, pwm_counter_2);
   pwmv2_counter_start_trigger_enable(HPM_PWM1, pwm_counter_3);
   pwmv2_counter_start_select_trigger_index(HPM_PWM1, pwm_counter_2, 2);
   pwmv2_counter_start_select_trigger_index(HPM_PWM1, pwm_counter_3, 3);

  // config shadow register as internal compare source
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(8), cmp_value_from_shadow_val, CMP1);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(9), cmp_value_from_shadow_val, CMP2);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(12), cmp_value_from_shadow_val, CMP3);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(13), cmp_value_from_shadow_val, CMP4);

  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(10), cmp_value_from_shadow_val, ADC_TRG1);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(14), cmp_value_from_shadow_val, ADC_TRG2);

  //pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(8), 2);
  //pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(9), 2);
  //pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(12), 3);
  //pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(13), 3);

  // enable complementraity of channel 4&5 6&7
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_4);
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_5);
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_6);
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_7);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_4);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_6);

  // config update timing
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(12), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(13), pwm_shadow_register_update_on_reload);

  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(10), pwm_shadow_register_update_on_modify);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(14), pwm_shadow_register_update_on_modify);

  pwmv2_calculate_set_period_parameter(HPM_PWM1, PWMV2_CALCULATE_INDEX(0), 0);
  pwmv2_calculate_set_dac_value_parameter(HPM_PWM1, PWMV2_CALCULATE_INDEX(0), 0);
  pwmv2_calculate_select_counter_calculate_index(HPM_PWM1, PWMV2_CALCULATE_INDEX(0), pwm_counter_2);
  pwmv2_calculate_select_in_offset(HPM_PWM1, PWMV2_CALCULATE_INDEX(0), PWMV2_SHADOW_INDEX(20));
  pwmv2_counter_update_trig0(HPM_PWM1, pwm_counter_2, 2);
  pwmv2_counter_enable_update_trig0(HPM_PWM1, pwm_counter_2);
  pwmv2_counter_set_trig0_calculate_cell_index(HPM_PWM1, pwm_counter_2, PWMV2_CALCULATE_INDEX(0));

  //参考手册UM p654通用计算单元内容
  /*
  PWMV2_CALCULATE_INDEX(1)      通用计算单元1
  */
  //设置 通用计算单元1 的pT为0（关闭CAL_T_INDEX输入）即关闭计数器重载值输入
  pwmv2_calculate_set_period_parameter(HPM_PWM1, PWMV2_CALCULATE_INDEX(1), 0);
  //同上，但设置pD为0（关闭CAL_IN_INDEX输入）
  pwmv2_calculate_set_dac_value_parameter(HPM_PWM1, PWMV2_CALCULATE_INDEX(1), 0);
  //设置 通用计算单元1 的输入为 PWM私有计数器3
  pwmv2_calculate_select_counter_calculate_index(HPM_PWM1, PWMV2_CALCULATE_INDEX(1), pwm_counter_3);
  //设置 通用计算单元1 的 CAL_IN_OFF 为 PHASE（某个影子寄存器）
  pwmv2_calculate_select_in_offset(HPM_PWM1, PWMV2_CALCULATE_INDEX(1), PHASE);
  //设置 计数器更新外部触发源0 选择 3号触发源（即PWM6/7的同步输入） (参考手册UM p686 p658)
  pwmv2_counter_update_trig0(HPM_PWM1, pwm_counter_3, 3);
  //接续上一行，启用触发更新
  pwmv2_counter_enable_update_trig0(HPM_PWM1, pwm_counter_3);
  //更新触发时 计数器3 使用 通用计算单元1 的结果
  pwmv2_counter_set_trig0_calculate_cell_index(HPM_PWM1, pwm_counter_3, PWMV2_CALCULATE_INDEX(1));

  pwmv2_set_shadow_val(HPM_PWM1, RLD2, reload, 0, false);

  pwmv2_set_shadow_val(HPM_PWM1, ADC_TRG1, reload * 0.8, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, ADC_TRG2, reload * 0.8, 0, false);

  pwmv2_shadow_register_lock(HPM_PWM1);

  // set dead zone
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_4, 0x7);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_5, 0x7);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_6, 0x7);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_7, 0x7);
  pwmv2_add_delay_tick_after_dead_area(HPM_PWM1, 0);

  ////反转右侧桥臂输出极性
  //pwmv2_enable_output_invert(HPM_PWM1, pwm_channel_6);
  //pwmv2_enable_output_invert(HPM_PWM1, pwm_channel_7);

  // start output
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_6);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_7);
  pwmv2_reset_multi_counter_sync(HPM_PWM1, (1 << pwm_counter_2 | 1 << pwm_counter_3));
  pwmv2_start_pwm_output_sync(HPM_PWM1, (1 << pwm_counter_2 | 1 << pwm_counter_3));
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_2);
  board_delay_us(1);
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_3);//pwmv2_enable_multi_counter_sync(HPM_PWM1, (1 << pwm_counter_2 | 1 << pwm_counter_3));

  // set synt to sync 2 counters (this can only set initial phase)
  synt_reset_counter(HPM_SYNT);
  synt_set_reload(HPM_SYNT, reload);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_0, reload);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_1, reload >> 2);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_2, reload >> 3);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_3, reload >> 4);
  synt_enable_counter(HPM_SYNT, true);

  pwmv2_set_trigout_cmp_index(HPM_PWM1, 0, PWMV2_CMP_INDEX(10));
  pwmv2_set_trigout_cmp_index(HPM_PWM1, 1, PWMV2_CMP_INDEX(14));

   HPM_IOC->PAD[IOC_PAD_PB00].FUNC_CTL = IOC_PB00_FUNC_CTL_TRGM_P_31;
   HPM_IOC->PAD[IOC_PAD_PB00].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(0) | IOC_PAD_PAD_CTL_PE_SET(0) |
   IOC_PAD_PAD_CTL_DS_SET(7) | IOC_PAD_PAD_CTL_OD_SET(0) | IOC_PAD_PAD_CTL_KE_SET(0) | IOC_PAD_PAD_CTL_SR_SET(1) |
   IOC_PAD_PAD_CTL_SPD_SET(3);

   HPM_IOC->PAD[IOC_PAD_PB01].FUNC_CTL = IOC_PB01_FUNC_CTL_TRGM_P_30;
   HPM_IOC->PAD[IOC_PAD_PB01].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(0) | IOC_PAD_PAD_CTL_PE_SET(0) |
   IOC_PAD_PAD_CTL_DS_SET(7) | IOC_PAD_PAD_CTL_OD_SET(0) | IOC_PAD_PAD_CTL_KE_SET(0) | IOC_PAD_PAD_CTL_SR_SET(1) |
   IOC_PAD_PAD_CTL_SPD_SET(3);

  // trgm_output_t trgm0_io_config0 = {0};
  // trgm0_io_config0.invert = 0;
  // trgm0_io_config0.type = trgm_output_same_as_input;
  // trgm0_io_config0.input = HPM_TRGM0_INPUT_SRC_PWM1_TRGO_0;
  // trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_TRGM0_P31, &trgm0_io_config0);

  // trgm_enable_io_output(HPM_TRGM0, 1 << 31);

  // trgm_output_t trgm0_io_config1 = {0};
  // trgm0_io_config1.invert = 0;
  // trgm0_io_config1.type = trgm_output_same_as_input;
  // trgm0_io_config1.input = HPM_TRGM0_INPUT_SRC_PWM1_TRGO_1;
  // trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_TRGM0_P30, &trgm0_io_config1);

  // trgm_enable_io_output(HPM_TRGM0, 1 << 30);

   trgm_output_t trgm0_io_config1 = {0};
   trgm0_io_config1.invert = 0;
   trgm0_io_config1.type = trgm_output_same_as_input;
   trgm0_io_config1.input = HPM_TRGM0_INPUT_SRC_SYNT_CH00;
   trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_TRGM0_P30, &trgm0_io_config1);
   trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_TRGM0_P31, &trgm0_io_config1);

   trgm_enable_io_output(HPM_TRGM0, 1 << 30);
   trgm_enable_io_output(HPM_TRGM0, 1 << 31);
}
