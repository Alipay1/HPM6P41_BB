#include "board_i2c.h"
#include "board.h"

#include "hpm_i2c.h"

#include "def_rtt_printf.h"

static hpm_i2c_context_t board_i2c_context = {0};
static bool board_is_i2c_init = false;

/**
 * @brief 返回一个hpm_i2c_context_t*的指针 指向板i2c实例
 */
void *board_get_i2c_context(void){
  if (board_is_i2c_init == false) {
    board_i2c_init();
  }
  return &board_i2c_context;
}

void board_i2c_init(void) {
  init_i2c_pins();
  clock_add_to_group(clock_i2c0, 0);
  LOG("clock clock_i2c0 \t : %u\r\n", clock_get_frequency(clock_i2c0));

  hpm_i2c_get_default_init_context(&board_i2c_context);
  board_i2c_context.init_config.communication_mode  = i2c_master;
  board_i2c_context.init_config.is_10bit_addressing = false;
  board_i2c_context.init_config.speed               = i2c_speed_1Mhz;
  board_i2c_context.addr_endianness                 = i2c_master_addr_big_endian;
  board_i2c_context.base                            = HPM_I2C0;
  assert(hpm_i2c_initialize(&board_i2c_context) == status_success);
  board_is_i2c_init = true;
}