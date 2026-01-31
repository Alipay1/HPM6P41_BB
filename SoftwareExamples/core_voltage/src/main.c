#include <board.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hpm_i2c.h"

#include "hpm_otp_drv.h"
#include "hpm_tsns_drv.h"

#include "board_syr838.h"
#include "board_i2c.h"

#include "driver_at24cxx.h"
#include "driver_at24cxx_interface.h"

#include "SEGGER_RTT.h"

#include "def_rtt_printf.h"

#define LED1_FLASH_PERIOD_IN_MS 200

#define VNAME(value) (#value)
#define GET(x) LOG("%s=%d\r\n", VNAME(x), x);

void board_iic_init(void);
void board_eeprom_test(void);
void board_tmp112_read_all(void);
hpm_stat_t syr838_set_vout_uv(uint32_t vout_uv);
hpm_i2c_context_t* i2c_context = NULL;

uint8_t a[2] = {0xA5,0x5A};

void calibrate_tsns(void);

int main(void) {
  board_init();
  board_clock_full_speed();
  board_init_led_pins();
  board_init_btn();
  board_timer_create(LED1_FLASH_PERIOD_IN_MS, board_led0_toggle);

  clock_add_to_group(clock_tsns, 0);
  tsns_enable_continuous_mode(HPM_TSNS);
  tsns_enable(HPM_TSNS);
  calibrate_tsns();

TEST:
  i2c_context = board_get_i2c_context();
  board_delay_ms(10);
  board_eeprom_test();
  //board_tmp112_read_all();
  while (1) {
    if (board_btn1_get_stat()) {
       //hpm_i2c_master_addr_write_blocking(&i2c_context, 0x5A, 0x01, 1, a, 2, 0xFF);
      //goto TEST;
    }

    if (board_btn2_get_stat()) {
      do {
        //board_tmp112_read_all();
        board_delay_ms(1000 / 8);
      } while (!board_btn2_get_stat());
    }
  }
}


void board_eeprom_test(void) {
  at24cxx_handle_t init_handle;
  at24cxx_info_t info;
  uint8_t res, i, j;
  uint8_t buf[12];
  uint8_t buf_check[12];
  uint32_t inc;

  DRIVER_AT24CXX_LINK_INIT(&init_handle, at24cxx_handle_t);
  DRIVER_AT24CXX_LINK_IIC_INIT(&init_handle, at24cxx_interface_iic_init);
  DRIVER_AT24CXX_LINK_IIC_DEINIT(&init_handle, at24cxx_interface_iic_deinit);
  DRIVER_AT24CXX_LINK_IIC_READ(&init_handle, at24cxx_interface_iic_read);
  DRIVER_AT24CXX_LINK_IIC_WRITE(&init_handle, at24cxx_interface_iic_write);
  DRIVER_AT24CXX_LINK_IIC_READ_ADDRESS16(&init_handle, at24cxx_interface_iic_read_address16);
  DRIVER_AT24CXX_LINK_IIC_WRITE_ADDRESS16(&init_handle, at24cxx_interface_iic_write_address16);
  DRIVER_AT24CXX_LINK_DELAY_MS(&init_handle, at24cxx_interface_delay_ms);
  DRIVER_AT24CXX_LINK_DEBUG_PRINT(&init_handle, (void *)printf);
  /* set chip type */
  res = at24cxx_set_type(&init_handle, AT24C08);
  if (res != 0) {
    at24cxx_interface_debug_print("at24cxx: set type failed.\n");
    return;
  }

  /* set addr pin */
  res = at24cxx_set_addr_pin(&init_handle, AT24CXX_ADDRESS_A000);
  if (res != 0) {
    at24cxx_interface_debug_print("at24cxx: set address pin failed.\n");
    return;
  }

  /* at24cxx init */
  res = at24cxx_init(&init_handle);
  if (res != 0) {
    at24cxx_interface_debug_print("at24cxx: init failed.\n");
    return;
  }

  /* start read test */
  at24cxx_interface_debug_print("at24cxx: start read test.\n");
  inc = ((uint32_t)AT24C08 + 1) / 8;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 12; j++) {
      buf[j] = (uint8_t)(rand() % 256);
    }

    /* write data */
    res = at24cxx_write(&init_handle, i * inc, (uint8_t *)buf, 12);
    if (res != 0) {
      at24cxx_interface_debug_print("at24cxx: write failed.\n");
      (void)at24cxx_deinit(&init_handle);
      return;
    }

    /* read data */
    res = at24cxx_read(&init_handle, i * inc, (uint8_t *)buf_check, 12);
    if (res != 0) {
      at24cxx_interface_debug_print("at24cxx: read failed.\n");
      (void)at24cxx_deinit(&init_handle);
      return;
    }
    for (j = 0; j < 12; j++) {
      /* check data */
      if (buf[j] != buf_check[j]) {
        at24cxx_interface_debug_print("at24cxx: check error.\n");
        (void)at24cxx_deinit(&init_handle);
        return;
      }
    }
    at24cxx_interface_debug_print("at24cxx: 0x%04X read write test passed.\n", i * inc);
  }

  /* finish read test */
  at24cxx_interface_debug_print("at24cxx: finish read test.\n");
  return;
}

ATTR_PLACE_AT_NONCACHEABLE uint16_t test[3] = {0};

uint8_t reverse(uint8_t x) {
  x = (((x & 0xaa) >> 1) | ((x & 0x55) << 1));
  x = (((x & 0xcc) >> 2) | ((x & 0x33) << 2));
  return ((x >> 4) | (x << 4));
}

void board_tmp112_read_all(void) {
  static bool is_inited = false;
  uint8_t tmp_buf_0[2]  = {0};
  uint8_t tmp_ctl_0[2]  = {0x60, 0xF0};
  int16_t tempc         = 0;
  hpm_stat_t result     = status_fail;
  if (!is_inited) {
    for (uint8_t tmp_addr = 0x48; tmp_addr < 0x4B; tmp_addr++) {
      do {
        result = hpm_i2c_master_addr_write_blocking(&i2c_context, tmp_addr, 0x01, 1, tmp_ctl_0, 2, 0xFF);
      } while (result != status_success);
    }
    is_inited = true;
  }

  result = status_fail;
  LOG("start-------TMP112-DATA------------\r\n");
  for (uint8_t tmp_addr = 0x48; tmp_addr < 0x4B; tmp_addr++) {
    do {
      result = hpm_i2c_master_addr_read_blocking(&i2c_context, tmp_addr, 0x00, 1, tmp_buf_0, 2, 0xFF);
    } while (result != status_success);

    tmp_buf_0[1] = tmp_buf_0[1] >> 4;
    tempc        = (tmp_buf_0[0] << 4) | tmp_buf_0[1];
    LOG("tmp112_0x%02X:%f\r\n", tmp_addr, tempc * 0.0625 * 1.8);
  }
  LOG("end---------TMP112-DATA------------\r\n");
        LOG("current temperature %.1f (0x%x)\n", tsns_get_current_temp(HPM_TSNS), tsns_get_current_temp_in_raw(HPM_TSNS));
  LOG("-------------TSNS-DATA-------------\r\n");
}

void calibrate_tsns(void) {
#define OTP_SHADOW_TSNS (21U)
#define OTP_SHADOW_TSNS_BASE_MASK ((1 << 12) - 1)
#define OTP_SHADOW_TSNS_BASE_EN_MASK (1 << 24)
#define OTP_TSNS_25C_BASE (0x956U)
#define OTP_TSNS_RATIO (0x870 << 12)
  volatile float temp;
  uint32_t i = 0;
  i          = otp_read_from_shadow(OTP_SHADOW_TSNS) & OTP_SHADOW_TSNS_BASE_MASK;

  if (i) {
    /* assume calibrated already */
    return;
  }

  i = OTP_TSNS_25C_BASE;
  LOG("before calibration: temp %.1f, read from shadow 0x%x\n", tsns_get_current_temp(HPM_TSNS), i);
  LOG("calibration starts\n");
  while (1) {
    LOG(".");
    temp = tsns_get_current_temp(HPM_TSNS);
    if (temp > 25.1) {
      LOG("%.1f too high: ", temp);
      i -= 1;
    } else if (temp < 24.9) {
      LOG("%.1f too low: ", temp);
      i += 1;
    } else {
      break;
    }
    if (status_success !=
        otp_write_shadow_register(OTP_SHADOW_TSNS,
                                  OTP_SHADOW_TSNS_BASE_EN_MASK | OTP_TSNS_RATIO | (i & OTP_SHADOW_TSNS_BASE_MASK))) {
      while (1) {
        ;
      }
    }
    LOG("set new offset: 0x%X\n", i);
    board_delay_ms(1000);
  }
  LOG("\ncalibration done\n");
  LOG("after calibration: temp %.1f, read from shadow 0x%x\n",
      tsns_get_current_temp(HPM_TSNS),
      otp_read_from_shadow(OTP_SHADOW_TSNS));
}