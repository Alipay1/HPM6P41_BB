/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _HPM_BOARD_H
#define _HPM_BOARD_H
#include "hpm_clock_drv.h"
#include "hpm_common.h"
#include "hpm_lobs_drv.h"
#include "hpm_soc.h"
#include "hpm_soc_feature.h"
#include "pinmux.h"
#include <stdio.h>
#if !defined(CONFIG_NDEBUG_CONSOLE) || !CONFIG_NDEBUG_CONSOLE
#include "hpm_debug_console.h"
#endif

#define BOARD_NAME "hpm6p41_bb"
#define BOARD_UF2_SIGNATURE (0x0A4D5048UL)
#define BOARD_CPU_FREQ (600000000UL)

#define SEC_CORE_IMG_START CORE1_ILM_LOCAL_BASE

#ifndef BOARD_RUNNING_CORE
#define BOARD_RUNNING_CORE HPM_CORE0
#endif

/* uart section */
#ifndef BOARD_APP_UART_BASE
#define BOARD_APP_UART_BASE HPM_UART2
#define BOARD_APP_UART_IRQ IRQn_UART2
#define BOARD_APP_UART_BAUDRATE (115200UL)
#define BOARD_APP_UART_CLK_NAME clock_uart2
#define BOARD_APP_UART_RX_DMA_REQ HPM_DMA_SRC_UART2_RX
#define BOARD_APP_UART_TX_DMA_REQ HPM_DMA_SRC_UART2_TX
#endif

#if !defined(CONFIG_NDEBUG_CONSOLE) || !CONFIG_NDEBUG_CONSOLE
#ifndef BOARD_CONSOLE_TYPE
#define BOARD_CONSOLE_TYPE CONSOLE_TYPE_UART
#endif

#if BOARD_CONSOLE_TYPE == CONSOLE_TYPE_UART
#ifndef BOARD_CONSOLE_UART_BASE
#if BOARD_RUNNING_CORE == HPM_CORE0
#define BOARD_CONSOLE_UART_BASE HPM_UART2
#define BOARD_CONSOLE_UART_CLK_NAME clock_uart2
#define BOARD_CONSOLE_UART_IRQ IRQn_UART2
#define BOARD_CONSOLE_UART_TX_DMA_REQ HPM_DMA_SRC_UART2_TX
#define BOARD_CONSOLE_UART_RX_DMA_REQ HPM_DMA_SRC_UART2_RX
#endif
#endif
#define BOARD_CONSOLE_UART_BAUDRATE (115200UL)
#endif
#endif

/* BTN */
#define BOARD_BTN_GPIO_IRQ IRQn_GPIO0_D

#define BOARD_BTN1_GPIO_CTRL HPM_GPIO0
#define BOARD_BTN1_GPIO_INDEX GPIO_DI_GPIOD
#define BOARD_BTN1_GPIO_PIN 21
#define BOARD_BTN1_PRESSED_VALUE 0

#define BOARD_BTN2_GPIO_CTRL HPM_GPIO0
#define BOARD_BTN2_GPIO_INDEX GPIO_DI_GPIOD
#define BOARD_BTN2_GPIO_PIN 20
#define BOARD_BTN2_PRESSED_VALUE 0

/*
 * timer for board delay
 */
#define BOARD_DELAY_TIMER (HPM_GPTMR0)
#define BOARD_DELAY_TIMER_CH 0
#define BOARD_DELAY_TIMER_CLK_NAME (clock_gptmr0)

#define BOARD_CALLBACK_TIMER (HPM_GPTMR0)
#define BOARD_CALLBACK_TIMER_CH 1
#define BOARD_CALLBACK_TIMER_IRQ IRQn_GPTMR0
#define BOARD_CALLBACK_TIMER_CLK_NAME (clock_gptmr0)

/* LED */
#define BOARD_LED_ON_LEVEL 0
#define BOARD_LED_OFF_LEVEL 1
/* LED0 */
#define BOARD_LED0_GPIO_CTRL HPM_GPIO0
#define BOARD_LED0_GPIO_INDEX GPIO_DI_GPIOY
#define BOARD_LED0_GPIO_PIN 0
#define BOARD_LED0_OFF_LEVEL BOARD_LED_OFF_LEVEL
#define BOARD_LED0_ON_LEVEL BOARD_LED_ON_LEVEL
/* LED1 */
#define BOARD_LED1_GPIO_CTRL HPM_GPIO0
#define BOARD_LED1_GPIO_INDEX GPIO_DI_GPIOY
#define BOARD_LED1_GPIO_PIN 1
#define BOARD_LED1_OFF_LEVEL BOARD_LED_OFF_LEVEL
#define BOARD_LED1_ON_LEVEL BOARD_LED_ON_LEVEL
/* LED2 */
#define BOARD_LED2_GPIO_CTRL HPM_GPIO0
#define BOARD_LED2_GPIO_INDEX GPIO_DI_GPIOY
#define BOARD_LED2_GPIO_PIN 2
#define BOARD_LED2_OFF_LEVEL BOARD_LED_OFF_LEVEL
#define BOARD_LED2_ON_LEVEL BOARD_LED_ON_LEVEL

#ifndef BOARD_SHOW_CLOCK
#define BOARD_SHOW_CLOCK 1
#endif
#ifndef BOARD_SHOW_BANNER
#define BOARD_SHOW_BANNER 1
#endif

/* LCD */
#define BOARD_LCD_SPI HPM_SPI0
#define BOARD_LCD_SPI_FRQ (40000000UL)

#define BOARD_LCD_RES_CTRL HPM_GPIO0
#define BOARD_LCD_RES_INDEX GPIO_DO_GPIOD
#define BOARD_LCD_RES_PIN 26
#define BOARD_LCD_RES_RUN_LEVEL 1
#define BOARD_LCD_RES_RST_LEVEL 0

#define BOARD_LCD_BKL_CTRL HPM_GPIO0
#define BOARD_LCD_BKL_INDEX GPIO_DO_GPIOA
#define BOARD_LCD_BKL_PIN 0
#define BOARD_LCD_BKL_ON_LEVEL 1
#define BOARD_LCD_BKL_OFF_LEVEL 0

#define BOARD_LCD_DC_CTRL HPM_FGPIO
#define BOARD_LCD_DC_INDEX GPIO_DO_GPIOD
#define BOARD_LCD_DC_PIN 27
#define BOARD_LCD_DC_D_LEVEL 1
#define BOARD_LCD_DC_C_LEVEL 0

/* CAN section */
#define BOARD_APP_CAN_BASE HPM_MCAN1
#define BOARD_APP_CAN_IRQn IRQn_MCAN1

/* FreeRTOS Definitions */
#define BOARD_FREERTOS_TIMER HPM_GPTMR2
#define BOARD_FREERTOS_TIMER_CHANNEL 1
#define BOARD_FREERTOS_TIMER_IRQ IRQn_GPTMR2
#define BOARD_FREERTOS_TIMER_CLK_NAME clock_gptmr2

#define BOARD_FREERTOS_LOWPOWER_TIMER HPM_PTMR
#define BOARD_FREERTOS_LOWPOWER_TIMER_CHANNEL 1
#define BOARD_FREERTOS_LOWPOWER_TIMER_IRQ IRQn_PTMR
#define BOARD_FREERTOS_LOWPOWER_TIMER_CLK_NAME clock_ptmr

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef void (*board_timer_cb)(void);

void board_init(void);
void board_init_console(void);
void board_print_clock_freq(void);
void board_init_uart(UART_Type *ptr);

void board_write_spi_cs(uint32_t pin, uint8_t state);
uint8_t board_get_led_gpio_off_level(void);
void board_init_led_pins(void);
void board_led0_write(uint8_t state);
void board_led1_write(uint8_t state);
void board_led2_write(uint8_t state);
void board_led0_toggle(void);
void board_led1_toggle(void);
void board_led2_toggle(void);

void board_init_btn(void);
bool board_btn1_get_stat(void);
bool board_btn2_get_stat(void);

/* Initialize SoC overall clocks */
void board_init_clock(void);
uint32_t board_init_uart_clock(UART_Type *ptr);

void board_init_usb(USB_Type *ptr);
uint32_t board_init_can(void);

/*
 * @brief Initialize PMP and PMA for but not limited to the following purposes:
 *      -- non-cacheable memory initialization
 */
void board_init_pmp(void);
void board_delay_us(uint32_t us);
void board_delay_ms(uint32_t ms);
void board_timer_create(uint32_t ms, board_timer_cb cb);

/*
 * Get GPIO pin level of onboard LED
 */
uint8_t board_get_led_gpio_off_level(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* _HPM_BOARD_H */

/* To fix SDK version below 1.9.0's problem with PinMux generated code */
#if defined(HPM_GPIOM_H) && SDK_VERSION_MAJOR <= 1 && SDK_VERSION_MINOR <= 9

#ifndef GPIOM_ASSIGN_GPIOC
#define GPIOM_ASSIGN_GPIOC (2UL)
#endif

#ifndef GPIOM_ASSIGN_GPIOD
#define GPIOM_ASSIGN_GPIOD (3UL)
#endif

#ifndef GPIOM_ASSIGN_GPIOE
#define GPIOM_ASSIGN_GPIOE (4UL)
#endif

#ifndef GPIOM_ASSIGN_GPIOF
#define GPIOM_ASSIGN_GPIOF (5UL)
#endif

#ifndef GPIOM_ASSIGN_GPIOG
#define GPIOM_ASSIGN_GPIOG (6UL)
#endif

#endif