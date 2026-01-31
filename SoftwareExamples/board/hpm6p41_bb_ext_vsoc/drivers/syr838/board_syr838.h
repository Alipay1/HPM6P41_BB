#ifndef _BOARD_SYR838_H
#define _BOARD_SYR838_H

#include "hpm_common.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


hpm_stat_t syr838_set_vout_uv(uint32_t vout_uv);
hpm_stat_t board_syr838_reg_i2c_context(void * ptr); //hpm_i2c_context_t * ptr


#if defined(__cplusplus)
}

#endif    /* __cplusplus */
#endif    //_BOARD_SYR838_H