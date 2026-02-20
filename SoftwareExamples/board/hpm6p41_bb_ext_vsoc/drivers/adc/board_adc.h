#ifndef _BOARD_ADC_H
#define _BOARD_ADC_H

#include "hpm_adc16_drv.h"
#include "hpm_common.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

void board_adc_init(void);
adc16_pmt_dma_data_t* get_adc0_buf(void);
adc16_pmt_dma_data_t* get_adc1_buf(void);
bool get_adc0_conv_stat(void);
bool get_adc1_conv_stat(void);
void clr_adc0_conv_stat(void);
void clr_adc1_conv_stat(void);

double get_ipeak(void);
double get_iavg(void);
double get_vin(void);
double get_vout(void);

#if defined(__cplusplus)
}

#endif    /* __cplusplus */
#endif    //_BOARD_ADC_H