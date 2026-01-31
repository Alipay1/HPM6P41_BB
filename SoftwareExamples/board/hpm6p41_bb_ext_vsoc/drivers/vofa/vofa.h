#ifndef _VOFA_H
#define _VOFA_H

#include "hpm_common.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#define VOFA_CH_CNT 32
#define VOFA_BUF_CNT (2048 * 2 / sizeof(vofa_frame))

typedef struct {
  float fdata[VOFA_CH_CNT];
  unsigned char tail[4];
} vofa_frame;

void vofa_send_frame(void);
void vofa_init_frame(void);
vofa_frame* vofa_get_frame_ptr(void);

#if defined(__cplusplus)
}

#endif    /* __cplusplus */
#endif    //_VOFA_H