#include <SEGGER_RTT.h>
#include <stdio.h>
#include <stdlib.h>

#include "hpm_pwmv2_drv.h"
#include "hpm_synt_drv.h"
#include "hpm_trgm_drv.h"

#include "board.h"
#include "board_pwmv2.h"

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

uint32_t a = 1, b = 4;

void pwm_console_set(void);

int main(void) {
  board_init_pmp();
  board_init_clock();
  SEGGER_RTT_Init();
  SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
  board_print_clock_freq();
  LOG("SDK_VERSION:%s\r\n", SDK_VERSION_STRING);
  board_init_led_pins();
  board_init_btn();
  board_timer_create(LED1_FLASH_PERIOD_IN_MS, board_led0_toggle);

  board_init_pwm();
  board_pwmv2_start_out();
  board_pwm_set_reload(4);
  board_pwm_set_duty_phase(0.1,0.1,0);

  while (1) {
    //pwm_console_set();

    for (int i=0; i<360; i++) {
      board_pwm_set_duty_phase(0.1,0.1,i);
      board_delay_ms(10);
    }
  }
}

double get_key_ret_num(char *in, uint32_t size){
  while (0 < SEGGER_RTT_GetKey())
    ;
  for (uint32_t i = 0; i < size - 1; i++) {
    in[i] = SEGGER_RTT_WaitKey();
    if (in[i] == '\r' || in[i] == '\n') {
      in[i + 1] = '\0';
      break;
    }
  }
  return strtod(in, NULL);
}

void pwm_console_set(void) {
  char buf[64];

  LOG("Set period:\r\n");
  double period = get_key_ret_num(buf, ARRAY_SIZE(buf));

  LOG("Set duty1:\r\n");
  double duty1 = get_key_ret_num(buf, ARRAY_SIZE(buf));

  LOG("Set duty2:\r\n");
  double duty2 = get_key_ret_num(buf, ARRAY_SIZE(buf));

  LOG("Set phase:\r\n");
  double phase = get_key_ret_num(buf, ARRAY_SIZE(buf));

  board_pwm_set_reload(period);
  board_pwm_set_duty_phase(duty1, duty2, phase);

  LOG("Parameters set to %lf %lf %lf %lf\r\n",period, duty1, duty2, phase);
  LOG("pwm_calc_reload:%u\r\n",board_pwm_calc_reload(period));
}