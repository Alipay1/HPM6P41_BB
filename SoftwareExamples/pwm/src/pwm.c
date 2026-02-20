#include <SEGGER_RTT.h>
#include <board.h>
#include <stdio.h>
#include <stdlib.h>

#include "board_pwmv2.h"

#include "hpm_pwmv2_drv.h"
#include "hpm_synt_drv.h"
#include "hpm_trgm_drv.h"

#define LED1_FLASH_PERIOD_IN_MS 200

#define VNAME(value) (#value)
// #define LOG(args...) SEGGER_RTT_printf(0, ##args)
#define LOG(args...) printf(args)
#define GET(x) LOG("%s=%d\r\n", VNAME(x), x);

#define PWM HPM_PWM1

#define RLD2 PWMV2_SHADOW_INDEX(0)
#define RLD3 PWMV2_SHADOW_INDEX(1)
#define CMP1 PWMV2_SHADOW_INDEX(2)
#define CMP2 PWMV2_SHADOW_INDEX(3)
#define CMP3 PWMV2_SHADOW_INDEX(4)
#define CMP4 PWMV2_SHADOW_INDEX(5)
#define PHASE PWMV2_SHADOW_INDEX(21)

#define PWM_PRIVATE_CLAC_0 0
#define PWM_PRIVATE_CLAC_1 1
#define PWM_PRIVATE_CLAC_2 2
#define PWM_PRIVATE_CLAC_3 3

uint32_t pwm_calc_reload(uint32_t period_us);
void pwm_console_select_test(void);
void pwm_hrpwm_test(void);
void pwm_duty_test(void);
void pwm_freq_test(void);
void pwm_dead_test(void);
void pwm_phase_test(void);

uint32_t a = 1, b = 4;

int main(void) {
  board_init_pmp();
  board_init_clock();
  SEGGER_RTT_Init();
  SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
  board_print_clock_freq();
  LOG("SDK_VERSION:%d.%d.%d\r\n", SDK_VERSION_STRING);
  board_init_led_pins();
  board_init_btn();
  board_timer_create(LED1_FLASH_PERIOD_IN_MS, board_led0_toggle);

  board_init_pwm();

  while (1) {
    pwm_console_select_test();
  }
}

void hrpwm_init_calibration(TRGM_Type *trgm) {
  uint8_t times = 0;
  trgm_pwmv2_calibration_mode_t calibration_mode;

  calibration_mode = trgm_pwmv2_calibration_mode_begin;
  while (calibration_mode != trgm_pwmv2_calibration_mode_end) {
    trgm_pwmv2_calibrate_delay_chain(trgm, &calibration_mode);
    board_delay_us(10);
    times++;
    if (times > 200) {
      printf("calibration failed\n");
      while (1) {
      }
    }
  }
}
uint32_t pwm_calc_reload(uint32_t period_us) {
  uint32_t t_reload, t_freq;
  t_freq   = clock_get_frequency(clock_pwm1);
  t_reload = t_freq / 1000 / 1000 * period_us - 1;
  return t_reload;
}

void pwm_console_select_test(void) {
  char buf[64];

  LOG("PWMv2 tests:\r\n");
  LOG("1: %s\r\n", "pwm_hrpwm_test");
  LOG("2: %s\r\n", "pwm_duty_test");
  LOG("3: %s\r\n", "pwm_freq_test");
  LOG("4: %s\r\n", "pwm_dead_test");
  LOG("5: %s\r\n", "pwm_phase_test");
  LOG("Input test num\r\n");

  while (0 < SEGGER_RTT_GetKey())
    ;
  for (uint32_t i = 0; i < ARRAY_SIZE(buf) - 1; i++) {
    buf[i] = SEGGER_RTT_WaitKey();
    if (buf[i] == '\r' || buf[i] == '\n') {
      buf[i + 1] = '\0';
      break;
    }
  }
  int test_selection = atoi(buf);

  switch (test_selection) {
  case 1:
    pwm_hrpwm_test();
    break;
  case 2:
    pwm_duty_test();
    break;
  case 3:
    pwm_freq_test();
    break;
  case 4:
    pwm_dead_test();
    break;
  case 5:
    pwm_phase_test();
    break;
  default:
    LOG("Invalid num.\r\n");
    break;
  }
}

/**
 * @brief generate 2 complementary PWM with variable pulse width by hrpwm feature.
 */
void pwm_hrpwm_test(void) {
  uint32_t restore_colck_div  = clock_get_divider(clock_ahb0);
  clk_src_t restore_clock_src = clock_get_source(clock_ahb0);
  uint32_t reload             = 0;

  pwmv2_deinit(HPM_PWM1);

  // switch clock source to a slower clock
  clock_set_source_divider(clock_ahb0, clk_src_osc24m, 1);

  pwmv2_shadow_register_unlock(HPM_PWM1);

  // clear shadow register value
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, 0, 0, false);

  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_2);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_reload);

  // config shadow register as internal compare source
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(8), cmp_value_from_shadow_val, CMP1);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(9), cmp_value_from_shadow_val, CMP2);
  // config update timing
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_reload);

  // config reload value from shadow register
  pwmv2_counter_select_data_offset_from_shadow_value(PWM, pwm_counter_2, RLD2);

  pwmv2_shadow_register_lock(HPM_PWM1);

  // enable complementraity of channel 4&5
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_4);

  // set reload value
  reload = pwm_calc_reload(1000);
  pwmv2_shadow_register_unlock(HPM_PWM1);
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, reload, 0, false);
  pwmv2_shadow_register_lock(HPM_PWM1);

  // set dead zone
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_4, 0x8);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_5, 0x8);
  pwmv2_add_delay_tick_after_dead_area(PWM, 0);

  // start output
  pwmv2_reset_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_2);

  pwmv2_enable_hrpwm(HPM_PWM1);
  //hrpwm_init_calibration(HPM_TRGM0);
  pwmv2_start_pwm_output(HPM_PWM1, pwm_counter_2);

  //     pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  //     pwmv2_set_shadow_val(HPM_PWM1, CMP2, reload/8, 0, false);
  //while (1) {
  //  for (uint16_t i = 0; i < 255; i++) {
  //    pwmv2_shadow_register_unlock(PWM);
  //    board_delay_ms(100);
  //    pwmv2_set_shadow_val(PWM, CMP2, reload / 8, i, false);
  //    pwmv2_shadow_register_lock(PWM);
  //  }
  //}


   // 每轮测试2560ms，进行100轮测试
   LOG("Start hrpwm test, should see 1us pulse width difference.\r\n");
   for (int i = 0; i < 10; i++) {
    for (uint8_t hr = 0; hr < UINT8_MAX; hr++) {
     pwmv2_shadow_register_unlock(HPM_PWM1);
     pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
     pwmv2_set_shadow_val(HPM_PWM1, CMP2, 15, hr, false);
     pwmv2_shadow_register_lock(HPM_PWM1);
     board_delay_ms(10);
    }
  }
  LOG("Stop hrpwm test.\r\n");


  // // start test 5 loop
  // LOG("Start hrpwm test, should see 1us pulse width difference.\r\n");
  // for (int i = 0; i < 5; i++) {
  //   pwmv2_shadow_register_unlock(HPM_PWM1);
  //   pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  //   pwmv2_set_shadow_val(HPM_PWM1, CMP2, 15, 0, false);
  //   pwmv2_shadow_register_lock(HPM_PWM1);

  //  board_delay_ms(1000);

  //  pwmv2_shadow_register_unlock(HPM_PWM1);
  //  pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  //  pwmv2_set_shadow_val(HPM_PWM1, CMP2, 15, 0b01111111, false);
  //  pwmv2_shadow_register_lock(HPM_PWM1);

  //  board_delay_ms(1000);

  //   pwmv2_shadow_register_unlock(HPM_PWM1);
  //   pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  //   pwmv2_set_shadow_val(HPM_PWM1, CMP2, 15, 0b11111111, false);
  //   pwmv2_shadow_register_lock(HPM_PWM1);

  //  board_delay_ms(1000);
  //}
  //LOG("Stop hrpwm test.\r\n");

  // disable output
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_deinit(HPM_PWM1);

  // restore clock souce
  clock_set_source_divider(clock_ahb0, restore_clock_src, restore_colck_div);
}

/**
 * @brief generate 2 complementary PWM with variable pulse width by shadow register.
 */
void pwm_duty_test(void) {
  uint32_t reload = 0;
  pwmv2_deinit(HPM_PWM1);

  pwmv2_shadow_register_unlock(HPM_PWM1);

  // clear shadow register value
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, 0, 0, false);

  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_2);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_reload);

  // config shadow register as internal compare source
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(8), cmp_value_from_shadow_val, CMP1);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(9), cmp_value_from_shadow_val, CMP2);
  // config update timing
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_reload);

  // config reload value from shadow register
  pwmv2_counter_select_data_offset_from_shadow_value(PWM, pwm_counter_2, RLD2);

  pwmv2_shadow_register_lock(HPM_PWM1);

  // enable complementraity of channel 4&5
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_4);

  // set reload value
  reload = pwm_calc_reload(600);
  pwmv2_shadow_register_unlock(HPM_PWM1);
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, reload, 0, false);
  pwmv2_shadow_register_lock(HPM_PWM1);

  // set dead zone
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_4, 0x8);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_5, 0x8);
  pwmv2_add_delay_tick_after_dead_area(PWM, 0);

  // start output
  pwmv2_reset_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_start_pwm_output(HPM_PWM1, pwm_counter_2);
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_2);

  // start test 5 loop
  LOG("Start duty test, should see pulse width increse & decrease.\r\n");
  for (int j = 0; j < 5; j++) {
    for (double i = 0; i < 0.99; i += 0.01) {
      pwmv2_shadow_register_unlock(HPM_PWM1);
      pwmv2_set_shadow_val(HPM_PWM1, CMP2, (uint32_t)(i * reload), 0, false);
      pwmv2_shadow_register_lock(HPM_PWM1);
      board_delay_ms(10);
    }
    for (double i = 0.99; i > 0.01; i -= 0.01) {
      pwmv2_shadow_register_unlock(HPM_PWM1);
      pwmv2_set_shadow_val(HPM_PWM1, CMP2, (uint32_t)(i * reload), 0, false);
      pwmv2_shadow_register_lock(HPM_PWM1);
      board_delay_ms(10);
    }
  }
  LOG("Stop duty test.\r\n");

  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_deinit(HPM_PWM1);
}

/**
 * @brief generate 2 complementary PWM with variable frequency by shadow register.
 */
void pwm_freq_test(void) {
  uint32_t reload = 0;
  pwmv2_deinit(HPM_PWM1);

  pwmv2_shadow_register_unlock(HPM_PWM1);

  // clear shadow register value
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, 0, 0, false);

  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_2);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_reload);

  // config shadow register as internal compare source
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(8), cmp_value_from_shadow_val, CMP1);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(9), cmp_value_from_shadow_val, CMP2);
  // config update timing
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_reload);

  // config reload value from shadow register
  pwmv2_counter_select_data_offset_from_shadow_value(PWM, pwm_counter_2, RLD2);

  pwmv2_shadow_register_lock(HPM_PWM1);

  // enable complementraity of channel 4&5
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_4);

  // set reload value
  reload = pwm_calc_reload(600);
  pwmv2_shadow_register_unlock(HPM_PWM1);
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, reload, 0, false);
  pwmv2_shadow_register_lock(HPM_PWM1);

  // set dead zone
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_4, 0x8);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_5, 0x8);
  pwmv2_add_delay_tick_after_dead_area(PWM, 0);

  // start output
  pwmv2_reset_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_start_pwm_output(HPM_PWM1, pwm_counter_2);
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_2);

  // start test 5 loop
  LOG("Start freq test, should see frequency increse & decrease.\r\n");
  for (int j = 0; j < 5; j++) {
    for (double i = 0; i < 0.99; i += 0.01) {
      pwmv2_shadow_register_unlock(HPM_PWM1);
      pwmv2_set_shadow_val(HPM_PWM1, RLD2, (uint32_t)(i * reload), 0, false);
      pwmv2_set_shadow_val(HPM_PWM1, CMP1, (uint32_t)(0), 0, false);
      pwmv2_set_shadow_val(HPM_PWM1, CMP2, (uint32_t)(0.5 * i * reload), 0, false);
      pwmv2_shadow_register_lock(HPM_PWM1);
      board_delay_ms(10);
    }
    for (double i = 0.99; i > 0.01; i -= 0.01) {
      pwmv2_shadow_register_unlock(HPM_PWM1);
      pwmv2_set_shadow_val(HPM_PWM1, RLD2, (uint32_t)(i * reload), 0, false);
      pwmv2_set_shadow_val(HPM_PWM1, CMP1, (uint32_t)(0), 0, false);
      pwmv2_set_shadow_val(HPM_PWM1, CMP2, (uint32_t)(0.5 * i * reload), 0, false);
      pwmv2_shadow_register_lock(HPM_PWM1);
      board_delay_ms(10);
    }
  }
  LOG("Stop freq test.\r\n");

  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_deinit(HPM_PWM1);
}

/**
 * @brief generate 2 complementary PWM with variable dead zone.
 */
void pwm_dead_test(void) {
  uint32_t reload = 0;
  pwmv2_deinit(HPM_PWM1);

  pwmv2_shadow_register_unlock(HPM_PWM1);

  // clear shadow register value
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, 0, 0, false);

  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_2);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_reload);

  // config shadow register as internal compare source
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(8), cmp_value_from_shadow_val, CMP1);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(9), cmp_value_from_shadow_val, CMP2);
  // config update timing
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_reload);

  // config reload value from shadow register
  pwmv2_counter_select_data_offset_from_shadow_value(PWM, pwm_counter_2, RLD2);

  pwmv2_shadow_register_lock(HPM_PWM1);

  // enable complementraity of channel 4&5
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_4);

  // set reload value
  reload = pwm_calc_reload(600);
  pwmv2_shadow_register_unlock(HPM_PWM1);
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, reload, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, (uint32_t)(0), 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, (uint32_t)(0.5 * reload), 0, false);
  pwmv2_shadow_register_lock(HPM_PWM1);

  // set dead zone
  pwmv2_add_delay_tick_after_dead_area(PWM, 0);

  // start output
  pwmv2_reset_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_start_pwm_output(HPM_PWM1, pwm_counter_2);
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_2);

  // start test 5 loop
  LOG("Start dead zone test, should see varing dead zone.\r\n");
  for (uint32_t dead_zone = 0x2; dead_zone < UINT16_MAX; dead_zone += 8) {
    pwmv2_set_dead_area(HPM_PWM1, pwm_channel_4, dead_zone);
    pwmv2_set_dead_area(HPM_PWM1, pwm_channel_5, dead_zone);
    board_delay_ms(1);
  }
  for (uint32_t dead_zone = UINT16_MAX; dead_zone > 0xF; dead_zone -= 8) {
    pwmv2_set_dead_area(HPM_PWM1, pwm_channel_4, dead_zone);
    pwmv2_set_dead_area(HPM_PWM1, pwm_channel_5, dead_zone);
    board_delay_ms(1);
  }
  LOG("Stop dead zone test.\r\n");

  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_disable_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_deinit(HPM_PWM1);
}
/**
 * @brief generate 2 PWM with shifting phase.
 */
void pwm_phase_test(void) {
  uint32_t reload = pwm_calc_reload(600);

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

  trgm0_io_config.invert = 0;
  trgm0_io_config.type   = trgm_output_pulse_at_input_rising_edge;
  trgm0_io_config.input  = HPM_TRGM0_INPUT_SRC_SYNT_CH03;
  trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_PWM1_TRIG_IN4, &trgm0_io_config);

  pwmv2_shadow_register_unlock(HPM_PWM1);

  // 用清空的影子寄存器来清空重载值和比较值
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_shlk);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_3, pwm_reload_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(12), pwm_shadow_register_update_on_shlk);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(13), pwm_shadow_register_update_on_shlk);

  // counter 2 and counter 3 using same reload source
  pwmv2_counter_select_data_offset_from_shadow_value(PWM, pwm_counter_2, RLD2);
  pwmv2_counter_select_data_offset_from_shadow_value(PWM, pwm_counter_3, RLD2);

  // clear shadow register value
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP3, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP4, 0, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, PHASE, 0, 0, false);

  // 触发SHLK更新然后解锁影子寄存器继续进行配置
  pwmv2_shadow_register_lock(HPM_PWM1);
  pwmv2_shadow_register_unlock(HPM_PWM1);

  // 持续发波 关闭BURST模式
  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_2);
  pwmv2_counter_burst_disable(HPM_PWM1, pwm_counter_3);

  // CNT CFG0 更新重载值的时机
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_2, pwm_reload_update_on_reload);
  pwmv2_set_reload_update_time(HPM_PWM1, pwm_counter_3, pwm_reload_update_on_reload);

  // pwmv2_counter_start_trigger_enable(HPM_PWM1, pwm_counter_2);
  // pwmv2_counter_start_trigger_enable(HPM_PWM1, pwm_counter_3);
  // pwmv2_counter_start_select_trigger_index(HPM_PWM1, pwm_counter_2, 2);
  // pwmv2_counter_start_select_trigger_index(HPM_PWM1, pwm_counter_3, 3);

  // config shadow register as internal compare source
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(8), cmp_value_from_shadow_val, CMP1);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(9), cmp_value_from_shadow_val, CMP2);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(12), cmp_value_from_shadow_val, CMP3);
  pwmv2_select_cmp_source(HPM_PWM1, PWMV2_CMP_INDEX(13), cmp_value_from_shadow_val, CMP4);

  pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(8), 2);
  pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(9), 2);
  pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(12), 3);
  pwmv2_select_cmp_trigmux(HPM_PWM1, PWMV2_CMP_INDEX(13), 3);

  // enable complementraity of channel 4&5 6&7
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_5);
  pwmv2_enable_pair_mode(HPM_PWM1, pwm_channel_7);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_4);
  pwmv2_disable_four_cmp(HPM_PWM1, pwm_channel_6);

  // config update timing
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(8), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(9), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(12), pwm_shadow_register_update_on_reload);
  pwmv2_cmp_update_trig_time(HPM_PWM1, PWMV2_CMP_INDEX(13), pwm_shadow_register_update_on_reload);

  pwmv2_calculate_set_period_parameter(PWM, PWMV2_CALCULATE_INDEX(0), 0);
  pwmv2_calculate_set_dac_value_parameter(PWM, PWMV2_CALCULATE_INDEX(0), 0);
  pwmv2_calculate_select_counter_calculate_index(PWM, PWMV2_CALCULATE_INDEX(0), pwm_counter_2);
  pwmv2_calculate_select_in_offset(PWM, PWMV2_CALCULATE_INDEX(0), PWMV2_SHADOW_INDEX(20));
  pwmv2_counter_update_trig0(PWM, pwm_counter_2, 2);
  pwmv2_counter_enable_update_trig0(PWM, pwm_counter_2);
  pwmv2_counter_set_trig0_calculate_cell_index(PWM, pwm_counter_2, PWMV2_CALCULATE_INDEX(0));


  //参考手册UM p654通用计算单元内容
  /*
  PWMV2_CALCULATE_INDEX(1)      通用计算单元1
  */
  //设置 通用计算单元1 的pT为0（关闭CAL_T_INDEX输入）即关闭计数器重载值输入
  pwmv2_calculate_set_period_parameter(PWM, PWMV2_CALCULATE_INDEX(1), 0);
  //同上，但设置pD为0（关闭CAL_IN_INDEX输入）
  pwmv2_calculate_set_dac_value_parameter(PWM, PWMV2_CALCULATE_INDEX(1), 0);
  //设置 通用计算单元1 的输入为 PWM私有计数器3
  pwmv2_calculate_select_counter_calculate_index(PWM, PWMV2_CALCULATE_INDEX(1), pwm_counter_3);
  //设置 通用计算单元1 的 CAL_IN_OFF 为 PHASE（某个影子寄存器）
  pwmv2_calculate_select_in_offset(PWM, PWMV2_CALCULATE_INDEX(1), PHASE);
  //设置 计数器更新外部触发源0 选择 3号触发源（即PWM6/7的同步输入） (参考手册UM p686 p658)
  pwmv2_counter_update_trig0(PWM, pwm_counter_3, 3);
  //接续上一行，启用触发更新
  pwmv2_counter_enable_update_trig0(PWM, pwm_counter_3);
  //更新触发时 计数器3 使用 通用计算单元1 的结果
  pwmv2_counter_set_trig0_calculate_cell_index(PWM, pwm_counter_3, PWMV2_CALCULATE_INDEX(1));

  pwmv2_shadow_register_lock(HPM_PWM1);

  // set dead zone
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_4, 0x8);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_5, 0x8);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_6, 0x8);
  pwmv2_set_dead_area(HPM_PWM1, pwm_channel_7, 0x8);
  pwmv2_add_delay_tick_after_dead_area(PWM, 0);

  // start output
  // pwmv2_enable_multi_counter_sync(HPM_PWM1, (1 << 2) | (1 << 3));
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_6);
  pwmv2_channel_enable_output(HPM_PWM1, pwm_channel_7);
  pwmv2_start_pwm_output(HPM_PWM1, pwm_counter_2);
  pwmv2_start_pwm_output(HPM_PWM1, pwm_counter_3);
  pwmv2_reset_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_reset_counter(HPM_PWM1, pwm_counter_3);
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_2);
  // board_delay_ms(1);    // test sync feature (test passed)
  pwmv2_enable_counter(HPM_PWM1, pwm_counter_3);

  // set synt to sync 2 counters (this can only set initial phase)
  synt_reset_counter(HPM_SYNT);
  synt_set_reload(HPM_SYNT, reload);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_0, reload >> 1);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_1, reload >> 1);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_2, reload >> 1);
  synt_set_comparator(HPM_SYNT, SYNT_CMP_3, reload >> 1);
  synt_enable_counter(HPM_SYNT, true);

  // set reload value
  pwmv2_shadow_register_unlock(HPM_PWM1);
  pwmv2_set_shadow_val(HPM_PWM1, RLD2, reload, 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP1, (uint32_t)(0), 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP2, (uint32_t)(0.125 * reload), 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP3, (uint32_t)(0), 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, CMP4, (uint32_t)(0.125 * reload), 0, false);
  pwmv2_set_shadow_val(HPM_PWM1, PHASE, 0, 0, false);
  pwmv2_shadow_register_lock(HPM_PWM1);

  // show sync pulses and hold for a while
  LOG("Start phase test by hardware calc unit, LED1 light up.\r\n");
  board_led1_write(BOARD_LED1_ON_LEVEL);
  board_delay_ms(1000);
  for (int i = 0; i < 5; i++) {
    for (double phase = 0; phase < 1; phase += 0.01) {
      pwmv2_set_shadow_val(HPM_PWM1, PHASE, phase * reload, 0, false);
      board_delay_ms(10);
    }
    for (double phase = 1; phase > 0; phase -= 0.01) {
      pwmv2_set_shadow_val(HPM_PWM1, PHASE, phase * reload, 0, false);
      board_delay_ms(10);
    }
  }
  board_delay_ms(1000);
  board_led1_write(BOARD_LED1_OFF_LEVEL);

  board_led2_write(BOARD_LED2_ON_LEVEL);
  // start test 5 loop
  LOG("Start phase test by software compare value, LED2 light up.\r\n");
  for (int i = 0; i < 5; i++) {
    for (double phase = 0; phase < 1; phase += 0.01) {
      pwmv2_set_shadow_val(HPM_PWM1, CMP3, (uint32_t)(phase * reload + 0), 0, false);
      pwmv2_set_shadow_val(HPM_PWM1, CMP4, (uint32_t)(phase * reload + 0.125 * reload), 0, false);
      board_delay_ms(10);
    }
    for (double phase = 1; phase > 0; phase -= 0.01) {
      pwmv2_set_shadow_val(HPM_PWM1, CMP3, (uint32_t)(phase * reload + 0), 0, false);
      pwmv2_set_shadow_val(HPM_PWM1, CMP4, (uint32_t)(phase * reload + 0.125 * reload), 0, false);
      board_delay_ms(10);
    }
  }
  // show sync pulses and hold for a while
  board_delay_ms(1000);
  board_led2_write(BOARD_LED2_OFF_LEVEL);
  LOG("Stop phase test.\r\n");

  synt_enable_counter(HPM_SYNT, false);
  synt_reset_counter(HPM_SYNT);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_4);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_5);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_6);
  pwmv2_channel_disable_output(HPM_PWM1, pwm_channel_7);
  pwmv2_disable_counter(HPM_PWM1, pwm_counter_2);
  pwmv2_disable_counter(HPM_PWM1, pwm_counter_3);
  pwmv2_deinit(HPM_PWM1);
}
