
#include <SEGGER_RTT.h>
#include <board.h>
#include <stdio.h>

#include "spi_lcd.h"
#include "st7789.h"

#include "hpm_gpio_drv.h"

#define LED1_FLASH_PERIOD_IN_MS 200

#define LOG(args...) SEGGER_RTT_printf(0, ##args)

int main(void) {
  board_init_pmp();
  board_init_clock();
  SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
  board_print_clock_freq();
#if defined (SDK_VERSION_STRING)
  LOG("SDK_VERSION:%s\r\n", SDK_VERSION_STRING);
#endif
  board_init_led_pins();
  board_init_btn();
  board_timer_create(LED1_FLASH_PERIOD_IN_MS, board_led0_toggle);

  board_lcd_init_spi();
  lcd_init_sequences();

  int press_cnt = 0;

  while (1) {
    lcd_fill(0, 0, 240, 135, 0);
    lcd_print(0, 00, 2, 0xFFFF, "HPMicro HPM6P41");
    lcd_print(0, 33, 3, 0xFFFF, "Hello world!");
    lcd_print(0, 50, 3, 0xFFFF, "Press Count:%+d    ", press_cnt);
    lcd_print(0, 67, 4, 0xFFFF, "Ark Pixel 16px");
    lcd_flush();
    if (board_btn1_get_stat()) {
      press_cnt--;
    }
    if (board_btn2_get_stat()) {
      press_cnt++;
    }
  }
}
