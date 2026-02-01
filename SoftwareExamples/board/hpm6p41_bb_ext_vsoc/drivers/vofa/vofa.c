#include "vofa.h"

/*!< endpoint address */
#define CDC_IN_EP 0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x83

extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);
extern void cdc_acm_data_send_with_dtr_test(uint8_t busid);

ATTR_PLACE_AT_FAST_RAM_BSS vofa_frame cdc_frame_0[VOFA_BUF_CNT] = {0};
ATTR_PLACE_AT_FAST_RAM_BSS vofa_frame cdc_frame_1[VOFA_BUF_CNT] = {0};
volatile bool vofa_frame_sel                                    = false;
volatile uint32_t vofa_frame_cnt                                = 0;

void vofa_init_frame(void) {
  uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7F};
  for (uint32_t i = 0; i < VOFA_BUF_CNT; i++) {
    memcpy(cdc_frame_0[i].tail, tail, 4);
    memset(cdc_frame_0[i].fdata, 0, VOFA_CH_CNT * sizeof(float));
  }
  for (uint32_t i = 0; i < VOFA_BUF_CNT; i++) {
    memcpy(cdc_frame_1[i].tail, tail, 4);
    memset(cdc_frame_1[i].fdata, 0, VOFA_CH_CNT * sizeof(float));
  }
}

vofa_frame *vofa_get_frame_ptr(void) {
  return vofa_frame_sel ? &cdc_frame_1[vofa_frame_cnt] : &cdc_frame_0[vofa_frame_cnt];
}

extern bool ep_tx_busy_flag;
extern int usbd_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len);

 void vofa_send_frame(void) {
  vofa_frame *frame_ptr = vofa_frame_sel ? cdc_frame_1 : cdc_frame_0;

  vofa_frame_cnt++;

  if (vofa_frame_cnt == VOFA_BUF_CNT) {
    ep_tx_busy_flag = true;
    usbd_ep_start_write(0, CDC_IN_EP, (uint8_t *)frame_ptr, sizeof(cdc_frame_0));
    vofa_frame_sel = !vofa_frame_sel;    // 切换
    vofa_frame_cnt = 0;
    while (ep_tx_busy_flag)
      ;
  }
}
