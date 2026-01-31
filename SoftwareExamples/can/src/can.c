#include <SEGGER_RTT.h>
#include <board.h>
#include <stdio.h>

#include "can.h"
#include "hpm_mcan_drv.h"

#define LED1_FLASH_PERIOD_IN_MS 200

#define LOG(args...) SEGGER_RTT_printf(0, ##args)

static bool can_buf_compare(const mcan_tx_frame_t *tx_buf, const mcan_rx_message_t *rx_buf);
bool can_loopback_test(MCAN_Type *base, bool enable_canfd);
void can_send_test(uint16_t can_id);

static volatile bool has_new_rcv_msg;
static volatile bool has_sent_out;
static volatile bool has_error;
static volatile bool tx_event_occurred;
static volatile bool timeout_event_occurred;
static volatile bool rxfifo0_event_occurred;
static volatile bool rxfifo1_event_occurred;
static volatile mcan_rx_message_t s_can_rx_buf;
static volatile mcan_tx_event_fifo_elem_t s_can_tx_evt;

int main(void) {
  board_init_pmp();
  board_init_clock();
  SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
  board_print_clock_freq();
  LOG("SDK_VERSION:%s\r\n", SDK_VERSION_STRING);
  board_init_led_pins();
  board_init_btn();
  board_timer_create(LED1_FLASH_PERIOD_IN_MS, board_led0_toggle);

  uint32_t can_src_clk_freq = board_init_can();

  mcan_config_t can_config;
  hpm_stat_t status       = status_fail;
  uint32_t interrupt_mask = 0;

CAN_TEST_START:
  mcan_deinit(BOARD_APP_CAN_BASE);
  mcan_get_default_config(BOARD_APP_CAN_BASE, &can_config);
  can_config.mode = mcan_mode_loopback_external;
  intc_m_disable_irq(IRQn_MCAN1);
  status = mcan_init(BOARD_APP_CAN_BASE, &can_config, can_src_clk_freq);
  assert(status == status_success);
  bool result = can_loopback_test(BOARD_APP_CAN_BASE, false);
  mcan_deinit(BOARD_APP_CAN_BASE);
  printf("    CAN1 blocking CAN2.0 loopback test %s\n", result ? "PASSED" : "FAILED");

  mcan_get_default_config(BOARD_APP_CAN_BASE, &can_config);
  can_config.enable_canfd = true;
  can_config.mode         = mcan_mode_loopback_external;
  mcan_get_default_ram_config(BOARD_APP_CAN_BASE, &can_config.ram_config, true);
  status = mcan_init(BOARD_APP_CAN_BASE, &can_config, can_src_clk_freq);
  assert(status == status_success);
  (void)status; /* Suppress compiling warning in release build */
  result = can_loopback_test(BOARD_APP_CAN_BASE, true);
  mcan_deinit(BOARD_APP_CAN_BASE);
  printf("    CAN1 blocking CANFD loopback test %s\n", result ? "PASSED" : "FAILED");

  mcan_get_default_config(BOARD_APP_CAN_BASE, &can_config);
  can_config.mode = mcan_mode_loopback_external;
  mcan_get_default_ram_config(BOARD_APP_CAN_BASE, &can_config.ram_config, true);
  interrupt_mask = MCAN_EVENT_RECEIVE | MCAN_EVENT_TRANSMIT;
  status         = mcan_init(BOARD_APP_CAN_BASE, &can_config, can_src_clk_freq);
  assert(status == status_success);
  (void)status; /* Suppress compiling warning in release build */
  mcan_enable_interrupts(BOARD_APP_CAN_BASE, interrupt_mask);
  mcan_enable_txbuf_transmission_interrupt(BOARD_APP_CAN_BASE, ~0UL);
  intc_m_enable_irq_with_priority(BOARD_APP_CAN_IRQn, 1);
  can_send_test(0x114);
  mcan_deinit(BOARD_APP_CAN_BASE);
  printf("    CAN1 interrupt CAN2.0 loopback test %s\n", has_sent_out & rxfifo0_event_occurred ? "PASSED" : "FAILED");
  has_sent_out           = false;
  rxfifo0_event_occurred = false;

  mcan_get_default_config(BOARD_APP_CAN_BASE, &can_config);
  can_config.enable_canfd = true;
  can_config.mode         = mcan_mode_loopback_external;
  mcan_get_default_ram_config(BOARD_APP_CAN_BASE, &can_config.ram_config, true);
  interrupt_mask = MCAN_EVENT_RECEIVE | MCAN_EVENT_TRANSMIT;
  status         = mcan_init(BOARD_APP_CAN_BASE, &can_config, can_src_clk_freq);
  assert(status == status_success);
  (void)status; /* Suppress compiling warning in release build */
  mcan_enable_interrupts(BOARD_APP_CAN_BASE, interrupt_mask);
  mcan_enable_txbuf_transmission_interrupt(BOARD_APP_CAN_BASE, ~0UL);
  intc_m_enable_irq_with_priority(BOARD_APP_CAN_IRQn, 1);
  can_send_test(0x514);
  mcan_deinit(BOARD_APP_CAN_BASE);
  printf("    CAN1 interrupt CANFD loopback test %s\n", has_sent_out & rxfifo0_event_occurred ? "PASSED" : "FAILED");
  has_sent_out           = false;
  rxfifo0_event_occurred = false;

  while (1) {
    if (board_btn1_get_stat()) {
      goto CAN_TEST_START;
    }
    if (board_btn2_get_stat()) {
      printf("\033[2J\033[;H. \033[2J");
      printf("Terminal cleared.\r\n");
    }
  }
}

SDK_DECLARE_EXT_ISR_M(BOARD_APP_CAN_IRQn, board_can_isr)
void board_can_isr(void) {
  MCAN_Type *base = BOARD_APP_CAN_BASE;
  uint32_t flags  = mcan_get_interrupt_flags(base);
  /* New message is available in RXFIFO0 */
  if ((flags & MCAN_INT_RXFIFO0_NEW_MSG) != 0) {
    mcan_read_rxfifo(base, 0, (mcan_rx_message_t *)&s_can_rx_buf);
    has_new_rcv_msg        = true;
    rxfifo0_event_occurred = true;
  }
  /* New message is available in RXFIFO1 */
  if ((flags & MCAN_INT_RXFIFO1_NEW_MSG) != 0U) {
    mcan_read_rxfifo(base, 1, (mcan_rx_message_t *)&s_can_rx_buf);
    has_new_rcv_msg        = true;
    rxfifo1_event_occurred = true;
  }
  /* New message is available in RXBUF */
  if ((flags & MCAN_INT_MSG_STORE_TO_RXBUF) != 0U) {
    has_new_rcv_msg = true;
    /* NOTE: Below code is for demonstration purpose, the performance is not optimized
     *       Users should optimize the performance according to real use case.
     */
    for (uint32_t buf_index = 0; buf_index < MCAN_RXBUF_SIZE_CAN_DEFAULT; buf_index++) {
      if (mcan_is_rxbuf_data_available(base, buf_index)) {
        mcan_read_rxbuf(base, buf_index, (mcan_rx_message_t *)&s_can_rx_buf);
        mcan_clear_rxbuf_data_available_flag(base, buf_index);
      }
    }
  }
  /* New TX Event occurred */
  if ((flags & MCAN_INT_TX_EVT_FIFO_NEW_ENTRY) != 0) {
    mcan_read_tx_evt_fifo(BOARD_APP_CAN_BASE, (mcan_tx_event_fifo_elem_t *)&s_can_tx_evt);
    tx_event_occurred = true;
  }
  /* Transmit completed */
  if ((flags & MCAN_EVENT_TRANSMIT) != 0U) {
    has_sent_out = true;
  }
  /* Error happened */
  if ((flags & MCAN_EVENT_ERROR) != 0) {
    has_error = true;
  }

  if ((flags & MCAN_INT_TIMEOUT_OCCURRED) != 0) {
    timeout_event_occurred = true;
  }
  mcan_clear_interrupt_flags(BOARD_APP_CAN_BASE, flags);
}

static bool can_buf_compare(const mcan_tx_frame_t *tx_buf, const mcan_rx_message_t *rx_buf) {
  bool result = false;

  do {
    HPM_BREAK_IF(tx_buf->dlc != rx_buf->dlc);
    HPM_BREAK_IF(tx_buf->use_ext_id != rx_buf->use_ext_id);
    if (tx_buf->use_ext_id) {
      HPM_BREAK_IF(tx_buf->ext_id != rx_buf->ext_id);
    } else {
      HPM_BREAK_IF(tx_buf->std_id != rx_buf->std_id);
    }

    HPM_BREAK_IF(tx_buf->rtr != rx_buf->rtr);
    bool data_matched = true;

    uint32_t data_bytes = mcan_get_message_size_from_dlc(rx_buf->dlc);
    for (uint32_t i = 0; i < data_bytes; i++) {
      if (tx_buf->data_8[i] != rx_buf->data_8[i]) {
        data_matched = false;
        break;
      }
    }

    result = data_matched;

  } while (false);

  return result;
}

bool can_loopback_test(MCAN_Type *base, bool enable_canfd) {
  uint32_t error_cnt = 0;
  bool result        = false;
  mcan_tx_frame_t tx_buf;
  mcan_rx_message_t rx_buf;
  memset(&tx_buf, 0, sizeof(tx_buf));
  memset(&rx_buf, 0, sizeof(rx_buf));

  /* Test Transmission and Reception of Standard Frame */
  tx_buf.std_id = 0x123;
  if (!enable_canfd) {
    tx_buf.dlc = 8;
    for (uint32_t i = 0; i < 8; i++) {
      tx_buf.data_8[i] = (uint8_t)i | (i << 4);
    }
  } else {
    tx_buf.dlc            = MCAN_DATA_FIELD_SIZE_64BYTES;
    tx_buf.canfd_frame    = 1;
    tx_buf.bitrate_switch = 1;
    for (uint32_t i = 0; i < mcan_get_message_size_from_dlc(tx_buf.dlc); i++) {
      tx_buf.data_8[i] = i;
    }
  }
  mcan_transmit_blocking(base, &tx_buf);
  mcan_receive_from_fifo_blocking(base, 0, &rx_buf);
  result = can_buf_compare(&tx_buf, &rx_buf);
  if (!result) {
    error_cnt++;
  }
  printf("    CAN loopback test for standard frame %s\n", result ? "passed" : "failed");

  /* Test Transmission and Reception of Extended Frame */
  tx_buf.use_ext_id = 1U;
  tx_buf.ext_id     = 0x12345678U;

  mcan_transmit_blocking(base, &tx_buf);
  mcan_receive_from_fifo_blocking(base, 0, &rx_buf);

  result = can_buf_compare(&tx_buf, &rx_buf);
  if (!result) {
    error_cnt++;
  }
  printf("    CAN loopback test for extend frame %s\n", result ? "passed" : "failed");

  return (error_cnt < 1);
}

void can_send_test(uint16_t can_id) {
  mcan_tx_frame_t tx_buf;
  memset(&tx_buf, 0, sizeof(tx_buf));
  tx_buf.dlc = 8;

  for (uint32_t i = 0; i < 8; i++) {
    tx_buf.data_8[i] = (uint8_t)i | (i << 4);
  }
  tx_buf.std_id = can_id & 0x7FF;
  mcan_transmit_blocking(BOARD_APP_CAN_BASE, &tx_buf);
}

void canfd_send_test(uint16_t can_id) {
  mcan_tx_frame_t tx_buf;
  memset(&tx_buf, 0, sizeof(tx_buf));
  tx_buf.canfd_frame      = true;
  tx_buf.use_ext_id       = true;
  tx_buf.ext_id           = 0x114514;
  tx_buf.bitrate_switch   = true;
  tx_buf.message_marker_h = 0xAB;
  tx_buf.message_marker_h = 0xCD;
  tx_buf.dlc              = 15;

  for (uint32_t i = 0; i < 64; i++) {
    tx_buf.data_8[i] = (uint8_t)i | (i << 4);
  }
  mcan_transmit_blocking(BOARD_APP_CAN_BASE, &tx_buf);
}