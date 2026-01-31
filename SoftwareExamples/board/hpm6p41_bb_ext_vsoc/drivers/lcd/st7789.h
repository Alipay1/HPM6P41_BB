#ifndef _ST7789_H
#define _ST7789_H

#include "hpm_common.h"

#define LCD_W (240UL)
#define LCD_H (135UL)

#define USE_HORIZONTAL 3

void board_lcd_init_spi(void);
void lcd_init_sequences(void);
void lcd_flush(void);
void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void lcd_print(uint32_t x, uint32_t y, uint32_t font_num, uint16_t color, char *fmt, ...);
void lcd_print_bgc(uint32_t x, uint32_t y, uint32_t font_num, uint16_t color, uint16_t BGcolor, char *fmt, ...);

#endif    //_ST7789_H