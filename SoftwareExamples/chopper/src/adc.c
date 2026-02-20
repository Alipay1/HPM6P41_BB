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
 float filter(float new, float old, float a) {
   return a * new + (1 - a) * old;
 };

void send_something(void){
    /* Wait for a complete of conversion */
    while (get_adc0_conv_stat() == 0)
      ;
    /* Process data */
    vofa_ptr           = vofa_get_frame_ptr();
    vofa_ptr->fdata[0] = get_vin();//Vin
    vofa_ptr->fdata[1] = get_vout();//Vout
    vofa_ptr->fdata[2] = get_ipeak();//peak
    vofa_ptr->fdata[3] = get_iavg();//avg
    vofa_send_frame();
}

int main(void) {
  board_init();
  board_clock_full_speed();
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
  #define H1 0.5
  #define H2 0
  board_pwm_set_duty_phase(H1, H2, H1*180);

  board_adc_init();
  while (1) {
    //for (double i = 0; i<1.0; i+=0.01) {
    //  board_pwm_set_duty_phase(i, H2, i*360); 
    //  board_delay_ms(10);
    //}
    send_something();
  }
}

  #undef H1
  #undef H2