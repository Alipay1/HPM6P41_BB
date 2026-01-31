#include "stdarg.h"
#include "stdio.h"

#include "fonts.h"
#include "st7789.h"

#include "board.h"

#include <hpm_gpio_drv.h>
#include <hpm_spi_drv.h>

ATTR_PLACE_AT_NONCACHEABLE uint16_t frame_buf[LCD_W * LCD_H] = {0};

extern spi_timing_config_t lcd_spi_timing_config   ;
extern spi_format_config_t lcd_spi_format          ;
extern spi_control_config_t lcd_spi_control_config ;

void LCD_Fill_Region(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint16_t color) {
  uint32_t i;
  uint32_t tempx       = 1 * x1;
  uint32_t tempy       = 0;
  uint32_t base_bias   = 1 * LCD_W * y1 + 1 * x1;
  uint32_t iTotalPixel = (x2 - x1) * (y2 - y1);
  for (i = 0; i < iTotalPixel;) {
    frame_buf[base_bias + tempx] = color;
    tempx += 1;
    if (tempx >= 1 * x2) {
      tempx = 1 * x1;
      tempy++;
      base_bias += (LCD_W * 1);
    }
    i++;
  }
  // LCD_Upgrade_Gram();
}

// void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
// {
//     LCD_SetRegion(x, x + 1, y, y + 1);
//     LCD_WriteHalfWord(color);
// }

void LCD_DrawPoint(uint32_t x, uint32_t y, uint16_t color) {
  frame_buf[x * 1 + y * (LCD_W) * 1] = color;
}

// glib库中的画线函数，可以画斜线，线两端分别是(x1, y1)和(x2, y2)
void LCD_DrawLine(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2, uint16_t color) {
  int dx, dy, e;
  dx = x2 - x1;
  dy = y2 - y1;

  if (dx >= 0) {
    if (dy >= 0)       // dy>=0
    {
      if (dx >= dy)    // 1/8 octant
      {
        e = dy - dx / 2;
        while (x1 <= x2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            y1 += 1;
            e -= dx;
          }
          x1 += 1;
          e += dy;
        }
      } else    // 2/8 octant
      {
        e = dx - dy / 2;
        while (y1 <= y2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            x1 += 1;
            e -= dy;
          }
          y1 += 1;
          e += dx;
        }
      }
    } else             // dy<0
    {
      dy = -dy;        // dy=abs(dy)
      if (dx >= dy)    // 8/8 octant
      {
        e = dy - dx / 2;
        while (x1 <= x2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            y1 -= 1;
            e -= dx;
          }
          x1 += 1;
          e += dy;
        }
      } else    // 7/8 octant
      {
        e = dx - dy / 2;
        while (y1 >= y2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            x1 += 1;
            e -= dy;
          }
          y1 -= 1;
          e += dx;
        }
      }
    }
  } else               // dx<0
  {
    dx = -dx;          // dx=abs(dx)
    if (dy >= 0)       // dy>=0
    {
      if (dx >= dy)    // 4/8 octant
      {
        e = dy - dx / 2;
        while (x1 >= x2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            y1 += 1;
            e -= dx;
          }
          x1 -= 1;
          e += dy;
        }
      } else    // 3/8 octant
      {
        e = dx - dy / 2;
        while (y1 <= y2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            x1 -= 1;
            e -= dy;
          }
          y1 += 1;
          e += dx;
        }
      }
    } else             // dy<0
    {
      dy = -dy;        // dy=abs(dy)
      if (dx >= dy)    // 5/8 octant
      {
        e = dy - dx / 2;
        while (x1 >= x2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            y1 -= 1;
            e -= dx;
          }
          x1 -= 1;
          e += dy;
        }
      } else    // 6/8 octant
      {
        e = dx - dy / 2;
        while (y1 >= y2) {
          LCD_DrawPoint(x1, y1, color);
          if (e > 0) {
            x1 -= 1;
            e -= dy;
          }
          y1 -= 1;
          e += dx;
        }
      }
    }
  }
}

void LCD_DrawFromIMG(uint32_t x, uint32_t y, const uint8_t *IMG, uint8_t IMG_W, uint8_t IMG_H, uint16_t color) {
  uint32_t TotalCounter = 0;
  uint32_t BitCounter   = 0;
  uint32_t Buffer       = 0;
  uint32_t tempx        = x;
  uint32_t tempy        = y;

  IMG_W += IMG_W % 8;

  IMG_H += IMG_H % 8;

  for (TotalCounter = 0; TotalCounter < IMG_W * IMG_H / 8;) {
    Buffer = *(IMG + TotalCounter);
    for (BitCounter = 0; BitCounter < 8;) {
      if (Buffer & 0x01) {
        LCD_DrawPoint(tempx, tempy, color);
      }

      tempx++;
      if (tempx >= x + IMG_W) {
        tempx = x;
        tempy++;
      }
      Buffer >>= 1;
      BitCounter++;
    }

    TotalCounter++;
  }
}

void LCD_DrawFromIMGWithBackGround(
    uint32_t x, uint32_t y, const uint8_t *IMG, uint8_t IMG_W, uint8_t IMG_H, uint16_t color, uint16_t BGcolor) {
  uint32_t TotalCounter = 0;
  uint32_t BitCounter   = 0;
  uint32_t Buffer       = 0;
  uint32_t tempx        = x;
  uint32_t tempy        = y;

  IMG_W += IMG_W % 8;

  IMG_H += IMG_H % 8;

  for (TotalCounter = 0; TotalCounter < IMG_W * IMG_H / 8;) {
    Buffer = *(IMG + TotalCounter);
    for (BitCounter = 0; BitCounter < 8;) {
      if (Buffer & 0x01) {
        LCD_DrawPoint(tempx, tempy, color);
      } else {
        LCD_DrawPoint(tempx, tempy, BGcolor);
      }

      tempx++;
      if (tempx >= x + IMG_W) {
        tempx = x;
        tempy++;
      }
      Buffer >>= 1;
      BitCounter++;
    }

    TotalCounter++;
  }
}

void lcd_print(uint32_t x, uint32_t y, uint32_t font_num, uint16_t color, char *fmt, ...) {
  uint32_t temp = 0;

  uint32_t tempx = x;
  uint32_t tempy = y;
  uint32_t font_width;
  uint32_t font_height;
  uint32_t font_unit_size;
  uint8_t buf[100];
  uint32_t len;

  unsigned char *pfont = NULL;
  va_list ap;
  va_start(ap, fmt);
  len = vsnprintf((char *)buf ,100 , fmt, ap);
  va_end(ap);
  switch (font_num) {
  default:
  case 1:
    /* code */
    pfont          = (unsigned char *)ASCII_8_16;
    font_width     = 8;
    font_height    = 16;
    font_unit_size = 16;
    break;
  case 2:
    /* code */
    pfont          = (unsigned char *)ASCII_12_32;
    font_width     = 12;
    font_height    = 32;
    font_unit_size = 96;
    break;
  case 3:
    pfont          = (unsigned char *)JetBrainMono;
    font_width     = 8;
    font_height    = 16;
    font_unit_size = 16;
    break;
  case 4:
    pfont          = (unsigned char *)ArkPix16px;
    font_width     = 8;
    font_height    = 16;
    font_unit_size = 16;
    break;
  }
  for (temp = 0; temp < len;) {
    if (tempx >= LCD_W - font_width) {
      tempx = x;
      tempy += font_height;
    }
    if ((buf[temp] == '\r') | (buf[temp] == '\n')) {
      temp++;
      tempx = x;
      tempy += font_height;
    }
    if (buf[temp] == '\0') {
      break;
    }
    LCD_DrawFromIMG(tempx, tempy, pfont + ((buf[temp] - 32) * (font_unit_size)), font_width, font_height, color);
    tempx += font_width;

    temp++;
  }
}

void lcd_print_bgc(uint32_t x, uint32_t y, uint32_t font_num, uint16_t color, uint16_t BGcolor, char *fmt, ...) {
  uint32_t temp = 0;

  uint32_t tempx = x;
  uint32_t tempy = y;
  uint32_t font_width;
  uint32_t font_height;
  uint32_t font_unit_size;
  uint8_t buf[100];
  uint32_t len;

  unsigned char *pfont = NULL;
  va_list ap;
  va_start(ap, fmt);
  len = vsnprintf((char *)buf ,100 , fmt, ap);
  va_end(ap);
  switch (font_num) {
  default:
  case 1:
    pfont          = (unsigned char *)ASCII_8_16;
    font_width     = 8;
    font_height    = 16;
    font_unit_size = 16;
    break;
  case 2:
    pfont          = (unsigned char *)ASCII_12_32;
    font_width     = 12;
    font_height    = 32;
    font_unit_size = 96;
    break;
  case 3:
    pfont          = (unsigned char *)JetBrainMono;
    font_width     = 8;
    font_height    = 16;
    font_unit_size = 16;
    break;
  case 4:
    pfont          = (unsigned char *)ArkPix16px;
    font_width     = 8;
    font_height    = 16;
    font_unit_size = 16;
    break;
  }

  //    xnum = (LCD_W - x) / font_width;
  //    ynum = 1 + len / xnum;
  for (temp = 0; temp < len;) {
    if (tempx >= LCD_W - font_width) {
      tempx = x;
      tempy += font_height;
    }
    if ((buf[temp] == '\r') | (buf[temp] == '\n')) {
      temp++;
      tempx = x;
      tempy += font_height;
    }
    if (buf[temp] == '\0') {
      break;
    }
    LCD_DrawFromIMGWithBackGround(
        tempx, tempy, pfont + ((buf[temp] - 32) * (font_unit_size)), font_width, font_height, color, BGcolor);
    tempx += font_width;

    temp++;
  }
}

void LCD_WR_REG(uint8_t data) {
  gpio_write_pin(BOARD_LCD_DC_CTRL, BOARD_LCD_DC_INDEX, BOARD_LCD_DC_PIN, BOARD_LCD_DC_C_LEVEL);
  spi_transfer(BOARD_LCD_SPI, &lcd_spi_control_config, NULL, NULL, (uint8_t *)&data, 1, NULL, 0);
  gpio_write_pin(BOARD_LCD_DC_CTRL, BOARD_LCD_DC_INDEX, BOARD_LCD_DC_PIN, BOARD_LCD_DC_D_LEVEL);
}

void LCD_WR_DATA8(uint8_t data) {
  spi_transfer(BOARD_LCD_SPI, &lcd_spi_control_config, NULL, NULL, (uint8_t *)&data, 1, NULL, 0);
}

void LCD_WR_DATA(uint16_t data) {
  uint16_t swap = data >> 8 | data << 8;
  spi_transfer(BOARD_LCD_SPI, &lcd_spi_control_config, NULL, NULL, (uint8_t *)&swap, 2, NULL, 0);
}

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
#if USE_HORIZONTAL == 0
  LCD_WR_REG(0x2a);    // 列地址设置
  LCD_WR_DATA(x1 + 52);
  LCD_WR_DATA(x2 + 52);
  LCD_WR_REG(0x2b);    // 行地址设置
  LCD_WR_DATA(y1 + 40);
  LCD_WR_DATA(y2 + 40);
  LCD_WR_REG(0x2c);    // 储存器写
#elif USE_HORIZONTAL == 1
  LCD_WR_REG(0x2a);    // 列地址设置
  LCD_WR_DATA(x1 + 53);
  LCD_WR_DATA(x2 + 53);
  LCD_WR_REG(0x2b);    // 行地址设置
  LCD_WR_DATA(y1 + 40);
  LCD_WR_DATA(y2 + 40);
  LCD_WR_REG(0x2c);    // 储存器写
#elif USE_HORIZONTAL == 2
  LCD_WR_REG(0x2a);    // 列地址设置
  LCD_WR_DATA(x1 + 40);
  LCD_WR_DATA(x2 + 40);
  LCD_WR_REG(0x2b);    // 行地址设置
  LCD_WR_DATA(y1 + 53);
  LCD_WR_DATA(y2 + 53);
  LCD_WR_REG(0x2c);    // 储存器写
#elif USE_HORIZONTAL == 3
  LCD_WR_REG(0x2a);    // 列地址设置
  LCD_WR_DATA(x1 + 40);
  LCD_WR_DATA(x2 + 40);
  LCD_WR_REG(0x2b);    // 行地址设置
  LCD_WR_DATA(y1 + 52);
  LCD_WR_DATA(y2 + 52);
  LCD_WR_REG(0x2c);    // 储存器写
#else
#error "invalid USE_HORIZONTAL"
#endif
}

void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color) {
  uint16_t i, j;
  for (i = ysta; i < yend; i++) {
    for (j = xsta; j < xend; j++) {
      frame_buf[LCD_W * i + j] = color;
    }
  }
}

void lcd_flush(void) {
  LCD_Address_Set(0, 0, LCD_W - 1, LCD_H - 1);

  spi_transfer(
      BOARD_LCD_SPI, &lcd_spi_control_config, NULL, NULL, (uint8_t *)&frame_buf, ARRAY_SIZE(frame_buf) * 2, NULL, 0);
}

void lcd_init_sequences(void) {
  gpio_write_pin(BOARD_LCD_RES_CTRL, BOARD_LCD_RES_INDEX, BOARD_LCD_RES_PIN, BOARD_LCD_RES_RST_LEVEL);
  board_delay_ms(10);
  gpio_write_pin(BOARD_LCD_RES_CTRL, BOARD_LCD_RES_INDEX, BOARD_LCD_RES_PIN, BOARD_LCD_RES_RUN_LEVEL);
  board_delay_ms(10);

  LCD_WR_REG(0x11);
  board_delay_ms(10);
  LCD_WR_REG(0x36);

#if USE_HORIZONTAL == 0
  LCD_WR_DATA8(0x00 | 1 << 3);
#elif USE_HORIZONTAL == 1
  LCD_WR_DATA8(0xC0 | 1 << 3);
#elif USE_HORIZONTAL == 2
  LCD_WR_DATA8(0x70 | 1 << 3);
#elif USE_HORIZONTAL == 3
  LCD_WR_DATA8(0xA0 | 1 << 3);
#else
#error "invalid USE_HORIZONTAL"
#endif

  LCD_WR_REG(0x3A);
  LCD_WR_DATA8(0x05);

  LCD_WR_REG(0xB2);
  LCD_WR_DATA8(0x0C);
  LCD_WR_DATA8(0x0C);
  LCD_WR_DATA8(0x00);
  LCD_WR_DATA8(0x33);
  LCD_WR_DATA8(0x33);

  LCD_WR_REG(0xB7);
  LCD_WR_DATA8(0x35);

  LCD_WR_REG(0xBB);
  LCD_WR_DATA8(0x19);

  LCD_WR_REG(0xC0);
  LCD_WR_DATA8(0x2C);

  LCD_WR_REG(0xC2);
  LCD_WR_DATA8(0x01);

  LCD_WR_REG(0xC3);
  LCD_WR_DATA8(0x12);

  LCD_WR_REG(0xC4);
  LCD_WR_DATA8(0x20);

  LCD_WR_REG(0xC6);
  LCD_WR_DATA8(0x0F);

  LCD_WR_REG(0xD0);
  LCD_WR_DATA8(0xA4);
  LCD_WR_DATA8(0xA1);

  LCD_WR_REG(0xE0);
  LCD_WR_DATA8(0xD0);
  LCD_WR_DATA8(0x04);
  LCD_WR_DATA8(0x0D);
  LCD_WR_DATA8(0x11);
  LCD_WR_DATA8(0x13);
  LCD_WR_DATA8(0x2B);
  LCD_WR_DATA8(0x3F);
  LCD_WR_DATA8(0x54);
  LCD_WR_DATA8(0x4C);
  LCD_WR_DATA8(0x18);
  LCD_WR_DATA8(0x0D);
  LCD_WR_DATA8(0x0B);
  LCD_WR_DATA8(0x1F);
  LCD_WR_DATA8(0x23);

  LCD_WR_REG(0xE1);
  LCD_WR_DATA8(0xD0);
  LCD_WR_DATA8(0x04);
  LCD_WR_DATA8(0x0C);
  LCD_WR_DATA8(0x11);
  LCD_WR_DATA8(0x13);
  LCD_WR_DATA8(0x2C);
  LCD_WR_DATA8(0x3F);
  LCD_WR_DATA8(0x44);
  LCD_WR_DATA8(0x51);
  LCD_WR_DATA8(0x2F);
  LCD_WR_DATA8(0x1F);
  LCD_WR_DATA8(0x1F);
  LCD_WR_DATA8(0x20);
  LCD_WR_DATA8(0x23);

  LCD_WR_REG(0x21);

  LCD_WR_REG(0x29);

  gpio_write_pin(BOARD_LCD_BKL_CTRL, BOARD_LCD_BKL_INDEX, BOARD_LCD_BKL_PIN, BOARD_LCD_BKL_ON_LEVEL);    // 打开背光
}
