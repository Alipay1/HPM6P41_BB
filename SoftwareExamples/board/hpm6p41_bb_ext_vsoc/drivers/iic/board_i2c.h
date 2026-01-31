#ifndef _BOARD_I2C_H
#define _BOARD_I2C_H

#include "hpm_common.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

void *board_get_i2c_context(void);
void board_i2c_init(void);


#if defined(__cplusplus)
}

#endif    /* __cplusplus */
#endif    //_BOARD_I2C_H