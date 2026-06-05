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
#define FSBB_DUTY_MIN (0.10f)
#define FSBB_DUTY_MAX (0.90f)
#define FSBB_DUTY_STARTUP (0.50f)
#define FSBB_VIN_MIN_ENABLE_V (2.0f)
#define FSBB_CONTROL_DECIMATION (100U)
#define FSBB_FILTER_ALPHA (0.02f)
#define FSBB_KP (0.0015f)
#define FSBB_KI (0.000015f)
#define FSBB_INTEGRATOR_LIMIT (0.10f)
#define FSBB_DUTY_SLEW_PER_UPDATE (0.0005f)

typedef struct {
  float vin;
  float vout;
  float iin;
  float iout;
  float duty;
  float integrator;
  float error;
  uint32_t decimation;
  bool filter_ready;
} fsbb_control_t;

static fsbb_control_t fsbb = {
    .duty = FSBB_DUTY_STARTUP,
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

static void fsbb_apply_duty(float duty) {
  duty = clampf(duty, FSBB_DUTY_MIN, FSBB_DUTY_MAX);
  fsbb.duty = duty;

  /* Pure FSBB: left high-side duty is D, right high-side duty is 1-D. */
  board_pwm_set_duty_phase(duty, 1.0f - duty, 0.0f);
}

static void fsbb_control_init(void) {
  fsbb.duty = FSBB_DUTY_STARTUP;
  fsbb.integrator = 0.0f;
  fsbb.error = 0.0f;
  fsbb.decimation = 0;
  fsbb.filter_ready = false;
  fsbb_apply_duty(fsbb.duty);
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
    fsbb_apply_duty(slew_limit(FSBB_DUTY_STARTUP, fsbb.duty, FSBB_DUTY_SLEW_PER_UPDATE));
    return;
  }

  fsbb.error = FSBB_VOUT_TARGET_V - fsbb.vout;
  fsbb.integrator += FSBB_KI * fsbb.error;
  fsbb.integrator = clampf(fsbb.integrator, -FSBB_INTEGRATOR_LIMIT, FSBB_INTEGRATOR_LIMIT);

  /*
   * Ideal non-inverting buck-boost feed-forward:
   * Vout/Vin = D/(1-D), so D = Vout_ref / (Vin + Vout_ref).
   * The PI term is intentionally small and slew-limited for low bandwidth.
   */
  float feedforward = FSBB_VOUT_TARGET_V / (fsbb.vin + FSBB_VOUT_TARGET_V);
  float demand = feedforward + FSBB_KP * fsbb.error + fsbb.integrator;
  float limited = clampf(demand, FSBB_DUTY_MIN, FSBB_DUTY_MAX);
  fsbb_apply_duty(slew_limit(limited, fsbb.duty, FSBB_DUTY_SLEW_PER_UPDATE));
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
    vofa_ptr->fdata[4] = fsbb.duty;
    vofa_ptr->fdata[5] = 1.0f - fsbb.duty;
    vofa_ptr->fdata[6] = FSBB_VOUT_TARGET_V;
    vofa_ptr->fdata[7] = fsbb.error;
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
