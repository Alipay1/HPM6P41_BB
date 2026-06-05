#include <SEGGER_RTT.h>
#include <board.h>
#include <stdio.h>
#include <stdlib.h>

#include "board_pwmv2.h"
#include "board_adc.h"
#include "board_cp.h"
#include "vofa.h"

#include "hpm_adc16_drv.h"


#include "usb_config.h"

#define LED1_FLASH_PERIOD_IN_MS 200

#define VNAME(value) (#value)
// #define LOG(args...) SEGGER_RTT_printf(0, ##args)
#define LOG(args...) printf(args)
#define GET(x) LOG("%s=%d\r\n", VNAME(x), x);

void init_preemption_config(void);
hpm_stat_t process_pmt_data(uint32_t *buff, int32_t start_pos, uint32_t len);
void board_adc_init(void);


extern volatile bool dtr_enable;
extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);
extern void cdc_acm_data_send_with_dtr_test(uint8_t busid);

vofa_frame *vofa_ptr = NULL;

#define FSBB_VOUT_TARGET_V (12.0f)
#define FSBB_H1_MIN (0.05f)
#define FSBB_H1_MAX (0.92f)
#define FSBB_H1_STARTUP (0.20f)
#define FSBB_H2_MIN (0.0f)
#define FSBB_H2_MAX (0.55f)
#define FSBB_H2_STARTUP (0.0f)
#define FSBB_VIN_MIN_ENABLE_V (2.0f)
#define FSBB_CONTROL_DECIMATION (100U)
#define FSBB_FILTER_ALPHA (0.02f)
#define FSBB_KP (0.0015f)
#define FSBB_KI (0.000015f)
#define FSBB_INTEGRATOR_LIMIT (0.10f)
#define FSBB_H1_SLEW_PER_UPDATE (0.0005f)
#define FSBB_H2_SLEW_PER_UPDATE (0.0002f)

typedef struct {
  float vin;
  float vout;
  float iin;
  float iout;
  float h1_buck_duty;
  float h2_boost_duty;
  float integrator;
  float error;
  float demand_ratio;
  uint32_t decimation;
  bool filter_ready;
} fsbb_control_t;

static fsbb_control_t fsbb = {
    .h1_buck_duty = FSBB_H1_STARTUP,
    .h2_boost_duty = FSBB_H2_STARTUP,
};

static float clampf(float value, float min, float max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

static float slew_limit(float target, float current, float max_step) {
  float delta = target - current;

  if (delta > max_step) {
    return current + max_step;
  }
  if (delta < -max_step) {
    return current - max_step;
  }
  return target;
}

static void fsbb_apply_bridge(float h1_buck_duty, float h2_boost_duty) {
  h1_buck_duty = clampf(h1_buck_duty, FSBB_H1_MIN, FSBB_H1_MAX);
  h2_boost_duty = clampf(h2_boost_duty, FSBB_H2_MIN, FSBB_H2_MAX);
  fsbb.h1_buck_duty = h1_buck_duty;
  fsbb.h2_boost_duty = h2_boost_duty;

  /* H1 is the buck leg, H2 is the boost leg. Keep the original H1-based phase. */
  board_pwm_set_duty_phase(h1_buck_duty, h2_boost_duty, h1_buck_duty * 180.0f);
}

static void fsbb_control_init(void) {
  fsbb.h1_buck_duty = FSBB_H1_STARTUP;
  fsbb.h2_boost_duty = FSBB_H2_STARTUP;
  fsbb.integrator = 0.0f;
  fsbb.error = 0.0f;
  fsbb.demand_ratio = 0.0f;
  fsbb.decimation = 0;
  fsbb.filter_ready = false;
  fsbb_apply_bridge(fsbb.h1_buck_duty, fsbb.h2_boost_duty);
}

static void fsbb_control_update(void) {
  float vin = (float)get_input_voltage();
  float vout = (float)get_output_voltage();
  float iin = (float)get_input_current();
  float iout = (float)get_output_current();

  if (!fsbb.filter_ready) {
    fsbb.vin = vin;
    fsbb.vout = vout;
    fsbb.iin = iin;
    fsbb.iout = iout;
    fsbb.filter_ready = true;
  } else {
    fsbb.vin = FSBB_FILTER_ALPHA * vin + (1.0f - FSBB_FILTER_ALPHA) * fsbb.vin;
    fsbb.vout = FSBB_FILTER_ALPHA * vout + (1.0f - FSBB_FILTER_ALPHA) * fsbb.vout;
    fsbb.iin = FSBB_FILTER_ALPHA * iin + (1.0f - FSBB_FILTER_ALPHA) * fsbb.iin;
    fsbb.iout = FSBB_FILTER_ALPHA * iout + (1.0f - FSBB_FILTER_ALPHA) * fsbb.iout;
  }

  fsbb.decimation++;
  if (fsbb.decimation < FSBB_CONTROL_DECIMATION) {
    return;
  }
  fsbb.decimation = 0;

  if (fsbb.vin < FSBB_VIN_MIN_ENABLE_V) {
    fsbb.integrator = 0.0f;
    fsbb.error = 0.0f;
    fsbb.demand_ratio = 0.0f;
    float safe_h1 = slew_limit(FSBB_H1_STARTUP, fsbb.h1_buck_duty, FSBB_H1_SLEW_PER_UPDATE);
    float safe_h2 = slew_limit(FSBB_H2_STARTUP, fsbb.h2_boost_duty, FSBB_H2_SLEW_PER_UPDATE);
    fsbb_apply_bridge(safe_h1, safe_h2);
    return;
  }

  fsbb.error = FSBB_VOUT_TARGET_V - fsbb.vout;
  fsbb.integrator += FSBB_KI * fsbb.error;
  fsbb.integrator = clampf(fsbb.integrator, -FSBB_INTEGRATOR_LIMIT, FSBB_INTEGRATOR_LIMIT);

  /*
   * Non-inverting FSBB relation:
   * Vout / Vin = H1 / (1 - H2), where H1 is the buck leg and H2 is the boost leg.
   * Stay buck-only while H1 can regulate; only raise H2 after H1 reaches its limit.
   */
  float max_ratio = FSBB_H1_MAX / (1.0f - FSBB_H2_MAX);
  float demand_ratio = FSBB_VOUT_TARGET_V / fsbb.vin + FSBB_KP * fsbb.error + fsbb.integrator;
  float target_h1;
  float target_h2;

  fsbb.demand_ratio = clampf(demand_ratio, FSBB_H1_MIN, max_ratio);
  if (fsbb.demand_ratio <= FSBB_H1_MAX) {
    target_h1 = fsbb.demand_ratio;
    target_h2 = FSBB_H2_MIN;
  } else {
    target_h1 = FSBB_H1_MAX;
    target_h2 = 1.0f - FSBB_H1_MAX / fsbb.demand_ratio;
  }

  target_h1 = clampf(target_h1, FSBB_H1_MIN, FSBB_H1_MAX);
  target_h2 = clampf(target_h2, FSBB_H2_MIN, FSBB_H2_MAX);

  float next_h1 = slew_limit(target_h1, fsbb.h1_buck_duty, FSBB_H1_SLEW_PER_UPDATE);
  float next_h2 = slew_limit(target_h2, fsbb.h2_boost_duty, FSBB_H2_SLEW_PER_UPDATE);
  fsbb_apply_bridge(next_h1, next_h2);
}

void send_something(void){
    /* Wait for a complete of conversion */
    while (get_adc0_conv_stat() == 0)
      ;
    clr_adc0_conv_stat();
    fsbb_control_update();

    /* Process data */
    vofa_ptr           = vofa_get_frame_ptr();
    vofa_ptr->fdata[0] = fsbb.vin;
    vofa_ptr->fdata[1] = fsbb.vout;
    vofa_ptr->fdata[2] = fsbb.iin;
    vofa_ptr->fdata[3] = fsbb.iout;
    vofa_ptr->fdata[4] = fsbb.h1_buck_duty;
    vofa_ptr->fdata[5] = fsbb.h2_boost_duty;
    vofa_ptr->fdata[6] = FSBB_VOUT_TARGET_V;
    vofa_ptr->fdata[7] = fsbb.error;
    vofa_ptr->fdata[8] = fsbb.demand_ratio;
    vofa_send_frame();
}

int main(void) {
  board_init();
  //board_clock_full_speed();

  board_init_usb((USB_Type *)HPM_USB0_BASE);
  intc_set_irq_priority(IRQn_USB0, 1);
  cdc_acm_init(0, HPM_USB0_BASE);
  vofa_init_frame();

  board_init_led_pins();
  board_init_btn();
  board_timer_create(LED1_FLASH_PERIOD_IN_MS, board_led0_toggle);

  //init_gdrive_cp_pins();
  //board_charge_pump_init();

  board_init_pwm();
  board_pwmv2_start_out();
  fsbb_control_init();

  board_adc_init();
  while (1) {
    //for (double i = 0; i<1.0; i+=0.01) {
    //  board_pwm_set_duty_phase(i, H2, i*360); 
    //  board_delay_ms(10);
    //}
    send_something();
  }
}
