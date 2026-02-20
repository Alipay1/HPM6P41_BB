#include "board_adc.h"
#include "board.h"

#include "hpm_clock_drv.h"
#include "hpm_trgm_drv.h"

#include "def_rtt_printf.h"

#define CHANNEL_VIN   6
#define CHANNEL_IPK   7
#define CHANNEL_IAVGE 8
#define CHANNEL_VOUT  9

ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
adc16_pmt_dma_data_t adc0_pmt_buff[ADC_SOC_PMT_MAX_DMA_BUFF_LEN_IN_4BYTES];

ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
adc16_pmt_dma_data_t adc1_pmt_buff[ADC_SOC_PMT_MAX_DMA_BUFF_LEN_IN_4BYTES];

adc16_pmt_dma_data_t* get_adc0_buf(void) {
  return adc0_pmt_buff;
};
adc16_pmt_dma_data_t* get_adc1_buf(void) {
  return adc1_pmt_buff;
};

volatile bool adc1_trig_conv_done = false;
volatile bool adc0_trig_conv_done = false;

bool get_adc0_conv_stat(void) {
  return adc0_trig_conv_done;
};

bool get_adc1_conv_stat(void) {
  return adc1_trig_conv_done;
};

void clr_adc0_conv_stat(void) {
  adc0_trig_conv_done = false;
};

void clr_adc1_conv_stat(void) {
  adc1_trig_conv_done = false;
};

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
  sample_cycle = clock_get_frequency(clock_adc0) / adc16_clock_divider_2 / 500000 / 2 - 5;
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
  pmt_cfg.trig_ch   = ADC16_CONFIG_TRG0A;
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
  adc16_set_pmt_queue_enable(HPM_ADC1, ADC16_CONFIG_TRG0A, true);

  adc16_init_pmt_dma(HPM_ADC0, core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)adc0_pmt_buff));
  adc16_init_pmt_dma(HPM_ADC1, core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)adc1_pmt_buff));

  intc_m_enable_irq_with_priority(IRQn_ADC0, 1);
   //intc_m_enable_irq_with_priority(IRQn_ADC1, 7);
  adc16_enable_interrupts(HPM_ADC0, adc16_event_trig_complete);
   //adc16_enable_interrupts(HPM_ADC1, adc16_event_trig_complete);
}

double get_vin(void) {
  return (double)get_adc0_buf()[0].result / UINT16_MAX * 3.0 / 5.1 * (120 + 5.1);
};
double get_vout(void) {
  return (double)get_adc0_buf()[1].result / UINT16_MAX * 3.0 / 5.1 * (120 + 5.1);
};
double get_ipeak(void) {
  return ((double)get_adc1_buf()[0].result / UINT16_MAX * 3.0 - 1.5) / 50 / 0.001;
};
double get_iavg(void) {
  return ((double)get_adc1_buf()[1].result / UINT16_MAX * 3.0 - 1.5) / 50 / 0.001;
};

ATTR_WEAK void adc0_conv_done_cb(void) {};
ATTR_WEAK void adc1_conv_done_cb(void) {};

  #include "hpm_mchtmr_drv.h"
  SDK_DECLARE_EXT_ISR_M(IRQn_ADC0, isr_adc0)
void isr_adc0(void) {
  uint32_t status;
  static uint32_t prc = 0;

  uint64_t tick_start = mchtmr_get_count(HPM_MCHTMR);
  uint64_t tick_period;
  status = adc16_get_status_flags(HPM_ADC0);
  adc16_clear_status_flags(HPM_ADC0, status);

  if (ADC16_INT_STS_TRIG_CMPT_GET(status)) {
    prc++;
    if (prc >= 4) {
      adc0_trig_conv_done = 1;
      adc0_conv_done_cb();
      board_led2_toggle();
      prc = 0;
  tick_period = mchtmr_get_count(HPM_MCHTMR) - tick_start;
  //LOG("TE:%llu\r\n",tick_period);
    }
  }
}
SDK_DECLARE_EXT_ISR_M(IRQn_ADC1, isr_adc1)
void isr_adc1(void) {
  uint32_t status;

  status = adc16_get_status_flags(HPM_ADC1);
  adc16_clear_status_flags(HPM_ADC1, status);

  if (ADC16_INT_STS_TRIG_CMPT_GET(status)) {
    adc1_trig_conv_done = 1;
    adc1_conv_done_cb();
  }
}