#ifndef _DEF_RTT_PRINTF_H
#define _DEF_RTT_PRINTF_H

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

extern int SEGGER_RTT_printf(unsigned BufferIndex, const char * sFormat, ...);

#define LOG(args...)    SEGGER_RTT_printf(0, ##args)
#define printf(args...) SEGGER_RTT_printf(0, ##args)

#if defined(__cplusplus)
}

#endif    /* __cplusplus */
#endif    //_DEF_RTT_PRINTF_H