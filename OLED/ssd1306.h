/**
 *  @file   ssd1306.h
 *  @brief  Original library: https://github.com/afiskon/stm32-ssd1306
 */

//--------------------------------------------------------------------------------

#ifndef _SSD1306_H_
#define _SSD1306_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------

/* Includes */
#include <stddef.h>

#include "stm32l1xx_hal.h"

#include "ssd1306_fonts.h"

//--------------------------------------------------------------------------------

/* Defines */
#define SSD1306_HEIGHT      64
#define SSD1306_WIDTH       128

//--------------------------------------------------------------------------------

/* Types */
enum ssd1306_color{
    COLOR_BLACK = 0x00, /**< Black color, no pixel */
    COLOR_WHITE = 0x01  /**< Pixel is set. Color depends on OLED */
};

struct ssd1306_transformations {
    uint16_t current_x;
    uint16_t current_y;
    uint8_t inverted;
    uint8_t initialized;
};

//--------------------------------------------------------------------------------

/* Global functions */
void ssd1306_i2c_init(void);
void ssd1306_init(void);
void ssd1306_fill(enum ssd1306_color color);
void ssd1306_update_screen(void);
void ssd1306_draw_pixel(uint8_t x, uint8_t y, enum ssd1306_color color);
char ssd1306_write_char(char ch, FontDef Font, enum ssd1306_color color);
char ssd1306_write_string(char* str, FontDef Font, enum ssd1306_color color);
void ssd1306_set_cursor(uint8_t x, uint8_t y);
void ssd1306_draw_line(int x_start, int y_start, int x_end, int y_end, enum ssd1306_color color);
void ssd1306_draw_rectangle(int x, int y, uint16_t w, uint16_t h, enum ssd1306_color color);
void ssd1306_draw_fill_rectangle(int x, int y, uint16_t w, uint16_t h, enum ssd1306_color color);

//--------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* _SSD1306_H_ */

