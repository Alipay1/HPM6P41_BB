#include <SEGGER_RTT.h>
#include <board.h>
#include <stdio.h>
#include <stdlib.h>

#include "board_pwmv2.h"
#include "vofa.h"

#include "hpm_adc16_drv.h"
#include "hpm_trgm_drv.h"
#include "hpm_pcfg_drv.h"

#include "usb_config.h"

#define LED1_FLASH_PERIOD_IN_MS 200

#define VNAME(value) (#value)
// #define LOG(args...) SEGGER_RTT_printf(0, ##args)
#define LOG(args...) printf(args)
#define GET(x) LOG("%s=%d\r\n", VNAME(x), x);

void init_preemption_config(void);
hpm_stat_t process_pmt_data(uint32_t *buff, int32_t start_pos, uint32_t len);
void board_adc_init(void);

__IO uint8_t seq_complete_flag;
__IO uint8_t trig_complete_flag;
__IO uint32_t res_out_of_thr_flag;
uint8_t trig_adc_channel[] = {6, 7, 8, 9};
uint8_t current_cycle_bit;
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
adc16_pmt_dma_data_t adc0_pmt_buff[ADC_SOC_PMT_MAX_DMA_BUFF_LEN_IN_4BYTES];
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
adc16_pmt_dma_data_t adc1_pmt_buff[ADC_SOC_PMT_MAX_DMA_BUFF_LEN_IN_4BYTES];

extern volatile bool dtr_enable;
extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);
extern void cdc_acm_data_send_with_dtr_test(uint8_t busid);

vofa_frame *vofa_ptr = NULL;
 float filter(float new, float old, float a) {
   return a * new + (1 - a) * old;
 };

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

  board_init_pwm();
  board_pwmv2_start_out();
  board_pwm_set_duty_phase(0.5, 0.5, 0 * 3.141592654 / 180);

  board_adc_init();
  while (1) {
    /* Wait for a complete of conversion */
    while (trig_complete_flag == 0)
      ;
    /* Process data */
    // process_pmt_data((uint32_t *)adc0_pmt_buff, ADC16_CONFIG_TRG0A * sizeof(adc16_pmt_dma_data_t),
    // sizeof(trig_adc_channel));
    vofa_ptr           = vofa_get_frame_ptr();
    vofa_ptr->fdata[0] = adc0_pmt_buff[0].result;//Vin
    vofa_ptr->fdata[1] = adc0_pmt_buff[1].result;//Vout
    vofa_ptr->fdata[2] = adc1_pmt_buff[12].result;//peak
    vofa_ptr->fdata[3] = adc1_pmt_buff[13].result;//avg
    vofa_send_frame();
  }
}

SDK_DECLARE_EXT_ISR_M(IRQn_ADC0, isr_adc0)
void isr_adc0(void) {
  uint32_t status;

  status = adc16_get_status_flags(HPM_ADC0);
  adc16_clear_status_flags(HPM_ADC0, status);

  if (ADC16_INT_STS_TRIG_CMPT_GET(status)) {
    trig_complete_flag = 1;
  }
}
SDK_DECLARE_EXT_ISR_M(IRQn_ADC1, isr_adc1)
void isr_adc1(void) {
  uint32_t status;

  status = adc16_get_status_flags(HPM_ADC1);
  adc16_clear_status_flags(HPM_ADC1, status);

  if (ADC16_INT_STS_TRIG_CMPT_GET(status)) {
    // trig_complete_flag = 1;
  }
}

#define CHANNEL_VIN   6
#define CHANNEL_IPK   7
#define CHANNEL_IAVGE 8
#define CHANNEL_VOUT  9

void board_adc_init(void) {
  adc16_config_t cfg            = {0};
  adc16_channel_config_t ch_cfg = {0};
  adc16_pmt_config_t pmt_cfg    = {0};
  trgm_output_t trgm_output_cfg = {0};
  uint32_t sample_cycle         = 0;

  // 配置引脚复用功能
  HPM_IOC->PAD[IOC_PAD_PD06].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
  HPM_IOC->PAD[IOC_PAD_PD07].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
  HPM_IOC->PAD[IOC_PAD_PD08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
  HPM_IOC->PAD[IOC_PAD_PD09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

  // 配置ADC时钟（电机控制应配置在AHB下）
  clock_add_to_group(clock_adc0, 0);
  clock_set_adc_source(clock_adc0, clk_adc_src_ahb0);
  clock_get_frequency(clock_adc0);
  LOG("ADC0 CLK:%u\r\n", clock_get_frequency(clock_adc0));
  clock_add_to_group(clock_adc1, 0);
  clock_set_adc_source(clock_adc1, clk_adc_src_ahb0);
  clock_get_frequency(clock_adc1);
  LOG("ADC1 CLK:%u\r\n", clock_get_frequency(clock_adc1));

  // 配置ADC外设功能
  adc16_get_default_config(&cfg);
  cfg.res          = adc16_res_16_bits;
  cfg.conv_mode    = adc16_conv_mode_preemption;
  cfg.adc_clk_div  = adc16_clock_divider_2;
  cfg.sel_sync_ahb = true;
  cfg.adc_ahb_en   = true;
  adc16_init(HPM_ADC0, &cfg);
  adc16_init(HPM_ADC1, &cfg);

  // 配置各通道
  adc16_get_channel_default_config(&ch_cfg);
  sample_cycle = clock_get_frequency(clock_adc0) / adc16_clock_divider_2 / 500000 - 5;
  LOG("sample_cycle:%u\r\n", sample_cycle);

  // 电压采样 ADC0
  ch_cfg.ch           = CHANNEL_VIN;
  ch_cfg.sample_cycle = sample_cycle;
  adc16_init_channel(HPM_ADC0, &ch_cfg);
  ch_cfg.ch           = CHANNEL_VOUT;
  ch_cfg.sample_cycle = sample_cycle;
  adc16_init_channel(HPM_ADC0, &ch_cfg);

  // 电压采样 ADC1
  ch_cfg.ch           = CHANNEL_IPK;
  ch_cfg.sample_cycle = sample_cycle;
  adc16_init_channel(HPM_ADC1, &ch_cfg);
  ch_cfg.ch           = CHANNEL_IAVGE;
  ch_cfg.sample_cycle = sample_cycle;
  adc16_init_channel(HPM_ADC1, &ch_cfg);

  pmt_cfg.adc_ch[0] = CHANNEL_VIN;
  pmt_cfg.adc_ch[1] = CHANNEL_VOUT;
  pmt_cfg.inten[0]  = false;
  pmt_cfg.inten[1]  = true;
  pmt_cfg.trig_ch   = ADC16_CONFIG_TRG0A;
  pmt_cfg.trig_len  = 2;
  adc16_enable_pmt_queue(HPM_ADC0, CHANNEL_VIN);
  adc16_enable_pmt_queue(HPM_ADC0, CHANNEL_VOUT);
  adc16_set_pmt_config(HPM_ADC0, &pmt_cfg);

  pmt_cfg.adc_ch[0] = CHANNEL_IPK;
  pmt_cfg.adc_ch[1] = CHANNEL_IAVGE;
  pmt_cfg.inten[0]  = false;
  pmt_cfg.inten[1]  = true;
  pmt_cfg.trig_ch   = ADC16_CONFIG_TRG1A;
  pmt_cfg.trig_len  = 2;
  adc16_enable_pmt_queue(HPM_ADC1, CHANNEL_IPK);
  adc16_enable_pmt_queue(HPM_ADC1, CHANNEL_IAVGE);
  adc16_set_pmt_config(HPM_ADC1, &pmt_cfg);

  trgm_output_cfg.invert = false;
  trgm_output_cfg.type   = trgm_output_pulse_at_input_rising_edge;
  trgm_output_cfg.input  = HPM_TRGM0_INPUT_SRC_PWM1_TRGO_0;
  trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_ADCX_PTRGI0A, &trgm_output_cfg);

  trgm_output_cfg.invert = false;
  trgm_output_cfg.type   = trgm_output_pulse_at_input_rising_edge;
  trgm_output_cfg.input  = HPM_TRGM0_INPUT_SRC_PWM1_TRGO_1;
  trgm_output_config(HPM_TRGM0, HPM_TRGM0_OUTPUT_SRC_ADCX_PTRGI1A, &trgm_output_cfg);

  adc16_set_pmt_queue_enable(HPM_ADC0, ADC16_CONFIG_TRG0A, true);
  adc16_set_pmt_queue_enable(HPM_ADC1, ADC16_CONFIG_TRG1A, true);

  adc16_init_pmt_dma(HPM_ADC0, core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)adc0_pmt_buff));
  adc16_init_pmt_dma(HPM_ADC1, core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)adc1_pmt_buff));

  intc_m_enable_irq_with_priority(IRQn_ADC0, 1);
  //intc_m_enable_irq_with_priority(IRQn_ADC1, 1);
  adc16_enable_interrupts(HPM_ADC0, adc16_event_trig_complete);
  //adc16_enable_interrupts(HPM_ADC1, adc16_event_trig_complete);
}

hpm_stat_t process_pmt_data(uint32_t *buff, int32_t start_pos, uint32_t len) {
  adc16_pmt_dma_data_t *dma_data = (adc16_pmt_dma_data_t *)buff;

   for (uint32_t i = start_pos; i < start_pos + len; i++) {
     if (i < VOFA_CH_CNT) {
       vofa_ptr->fdata[i] = dma_data[i].result;
       // vofa_ptr->fdata[i] = filter(dma_data[i].result,cdc_frame.fdata[i],0.025);
     }
   }

  vofa_send_frame();

  return status_success;
}