/**
 *  @file   ssd1306.c
 *  @brief  -
 */

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "stm32l1xx_hal.h"

#include "ssd1306.h"
#include "debug_log.h"

//--------------------------------------------------------------------------------

#ifndef CFG_OLED_LOG_EN
#define CFG_OLED_LOG_EN 1
#endif

#if CFG_OLED_LOG_EN
#define LOG(fmt, ...)   debug_log("[OLED] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   do { } while (0)
#endif

//--------------------------------------------------------------------------------

/* Defines */
#define SSD1306_I2C_PORT        GPIOB
#define SSD1306_I2C_PIN_SCL     GPIO_PIN_8
#define SSD1306_I2C_PIN_SDA     GPIO_PIN_9
#define SSD1306_I2Cx            I2C1

#define SSD1306_I2C_ADDR        (0x3C << 1)

#define SWAP_INT(_a, _b) { int t = _a; _a = _b; _b = t; }

//--------------------------------------------------------------------------------

/* Static */
struct ssd1306_context
{
    I2C_HandleTypeDef handle;
    uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
    struct ssd1306_transformations ssd1306;
};

static struct ssd1306_context ctx;

//--------------------------------------------------------------------------------

/* Static function declarations */
static void ssd1306_write_cmd(uint8_t byte);
static void ssd1306_write_data(uint8_t* buffer, size_t buff_size);

static void ssd1306_write_line(int x_start, int y_start, int x_end, int y_end, enum ssd1306_color color);
static void ssd1306_write_fast_vline(int x_start, int y_start, int h, enum ssd1306_color color);
static void ssd1306_write_fast_hline(int x_start, int y_start, int w, enum ssd1306_color color);

//--------------------------------------------------------------------------------

/* Static functions */
static void ssd1306_write_cmd(uint8_t byte) {

	HAL_I2C_Mem_Write(&ctx.handle, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, HAL_MAX_DELAY);
}

static void ssd1306_write_data(uint8_t* buffer, size_t buff_size)
{
	HAL_I2C_Mem_Write(&ctx.handle, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}

static void ssd1306_write_line(int x_start, int y_start, int x_end, int y_end, enum ssd1306_color color)
{
    int16_t steep = abs(y_end - y_start) > abs(x_end - x_start);

    if (steep)
    {
        SWAP_INT(x_start, y_start);
        SWAP_INT(x_end, y_end);
    }

    if (x_start > x_end)
    {
        SWAP_INT(x_start, x_end);
        SWAP_INT(y_start, y_end);
    }

    int16_t dx, dy;
    dx = x_end - x_start;
    dy = abs(y_end - y_start);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y_start < y_end)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for (; x_start<=x_end; x_start++)
    {
        if (steep)
        {

            ssd1306_draw_pixel(y_start, x_start, color);
        }
        else
        {
            ssd1306_draw_pixel(x_start, y_start, color);
        }
        err -= dy;
        if (err < 0)
        {
            y_start += ystep;
            err += dx;
        }
    }
}

static void ssd1306_write_fast_vline(int x_start, int y_start, int h, enum ssd1306_color color)
{
    ssd1306_write_line(x_start, y_start, x_start, y_start+h-1, color);
}

static void ssd1306_write_fast_hline(int x_start, int y_start, int w, enum ssd1306_color color)
{
    ssd1306_write_line(x_start, y_start, x_start+w-1, y_start, color);
}

//--------------------------------------------------------------------------------

/* Global functions */
void ssd1306_i2c_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    GPIO_InitStruct.Pin = SSD1306_I2C_PIN_SCL | SSD1306_I2C_PIN_SDA;

    HAL_GPIO_Init(SSD1306_I2C_PORT, &GPIO_InitStruct);

    __I2C1_CLK_ENABLE();

    ctx.handle.Instance = SSD1306_I2Cx;

    ctx.handle.Init.ClockSpeed = 100000; // 37 FPS
    ctx.handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
    ctx.handle.Init.OwnAddress1 = 0;
    ctx.handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    ctx.handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    ctx.handle.Init.OwnAddress2 = 0;
    ctx.handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    ctx.handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    HAL_I2C_Init(&ctx.handle);
    __HAL_I2C_ENABLE(&ctx.handle);
}

void ssd1306_init(void)
{
    LOG("OLED initialization...\n\r");
    ssd1306_write_cmd(0xAE); //display off

    ssd1306_write_cmd(0x20); //Set Memory Addressing Mode
    ssd1306_write_cmd(0x00); // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                // 10b,Page Addressing Mode (RESET); 11b,Invalid

    ssd1306_write_cmd(0xB0); //Set Page Start Address for Page Addressing Mode,0-7

#ifdef SSD1306_MIRROR_VERT
    ssd1306_write_cmd(0xC0); // Mirror vertically
#else
    ssd1306_write_cmd(0xC8); //Set COM Output Scan Direction
#endif

    ssd1306_write_cmd(0x00); //---set low column address
    ssd1306_write_cmd(0x10); //---set high column address

    ssd1306_write_cmd(0x40); //--set start line address - CHECK

    ssd1306_write_cmd(0x81); //--set contrast control register - CHECK
    ssd1306_write_cmd(0xFF);

#ifdef SSD1306_MIRROR_HORIZ
    ssd1306_write_cmd(0xA0); // Mirror horizontally
#else
    ssd1306_write_cmd(0xA1); //--set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
    ssd1306_write_cmd(0xA7); //--set inverse color
#else
    ssd1306_write_cmd(0xA6); //--set normal color
#endif

// Set multiplex ratio.
#if (SSD1306_HEIGHT == 128)
    // Found in the Luma Python lib for SH1106.
    ssd1306_write_cmd(0xFF);
#else
    ssd1306_write_cmd(0xA8); //--set multiplex ratio(1 to 64) - CHECK
#endif

#if (SSD1306_HEIGHT == 32)
    ssd1306_write_cmd(0x1F); //
#elif (SSD1306_HEIGHT == 64)
    ssd1306_write_cmd(0x3F); //
#elif (SSD1306_HEIGHT == 128)
    ssd1306_write_cmd(0x3F); // Seems to work for 128px high displays too.
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    ssd1306_write_cmd(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    ssd1306_write_cmd(0xD3); //-set display offset - CHECK
    ssd1306_write_cmd(0x00); //-not offset

    ssd1306_write_cmd(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_write_cmd(0xF0); //--set divide ratio

    ssd1306_write_cmd(0xD9); //--set pre-charge period
    ssd1306_write_cmd(0x22); //

    ssd1306_write_cmd(0xDA); //--set com pins hardware configuration - CHECK
#if (SSD1306_HEIGHT == 32)
    ssd1306_write_cmd(0x02);
#elif (SSD1306_HEIGHT == 64)
    ssd1306_write_cmd(0x12);
#elif (SSD1306_HEIGHT == 128)
    ssd1306_write_cmd(0x12);
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    ssd1306_write_cmd(0xDB); //--set vcomh
    ssd1306_write_cmd(0x20); //0x20,0.77xVcc

    ssd1306_write_cmd(0x8D); //--set DC-DC enable
    ssd1306_write_cmd(0x14); //
    ssd1306_write_cmd(0xAF); //--turn on SSD1306 panel

    // Clear screen
    ssd1306_fill(COLOR_BLACK);
    
    // Flush buffer to screen
    ssd1306_update_screen();
    
    // Set default values for screen object
    ctx.ssd1306.current_x = 0;
    ctx.ssd1306.current_y = 0;
    
    ctx.ssd1306.initialized = 1;
    LOG("OLED initialization finished!\n\r");
}

// Fill the whole screen with the given color
void ssd1306_fill(enum ssd1306_color color)
{
    /* Set memory */
    uint32_t i;

    for(i = 0; i < sizeof(ctx.buffer); i++) {
        ctx.buffer[i] = (color == COLOR_BLACK) ? 0x00 : 0xFF;
    }
}

// Write the screenbuffer with changed to the screen
void ssd1306_update_screen(void)
{
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
    for(uint8_t i = 0; i < SSD1306_HEIGHT/8; i++)
    {
        ssd1306_write_cmd(0xB0 + i); // Set the current RAM page address.
        ssd1306_write_cmd(0x00);
        ssd1306_write_cmd(0x10);
        ssd1306_write_data(&ctx.buffer[SSD1306_WIDTH*i], SSD1306_WIDTH);
    }
}

//    Draw one pixel in the screenbuffer
//    X => X Coordinate
//    Y => Y Coordinate
//    color => Pixel color
void ssd1306_draw_pixel(uint8_t x, uint8_t y, enum ssd1306_color color)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
    {
        // Don't write outside the buffer
        return;
    }
    
    // Check if pixel should be inverted
    if(ctx.ssd1306.inverted)
    {
        color = (enum ssd1306_color)!color;
    }
    
    // Draw in the right color
    if(color == COLOR_WHITE)
    {
        ctx.buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    }
    else
    {
        ctx.buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

// Draw 1 char to the screen buffer
// ch       => char om weg te schrijven
// Font     => Font waarmee we gaan schrijven
// color    => Black or White
char ssd1306_write_char(char ch, FontDef Font, enum ssd1306_color color) {
    uint32_t i, b, j;
    
    // Check if character is valid
    if (ch < 32 || ch > 126)
    {
        return 0;
    }
    
    // Check remaining space on current line
    if ((ctx.ssd1306.current_x + Font.FontWidth) > SSD1306_WIDTH ||
        (ctx.ssd1306.current_y + Font.FontHeight) > SSD1306_HEIGHT)
    {
        // Not enough space on current line
        return 0;
    }
    
    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++)
    {
        b = Font.data[(ch - 32) * Font.FontHeight + i];

        for(j = 0; j < Font.FontWidth; j++)
        {
            if((b << j) & 0x8000)
            {
                ssd1306_draw_pixel(ctx.ssd1306.current_x + j, (ctx.ssd1306.current_y + i), (enum ssd1306_color)color);
            }
            else
            {
                ssd1306_draw_pixel(ctx.ssd1306.current_x + j, (ctx.ssd1306.current_y + i), (enum ssd1306_color)!color);
            }
        }
    }
    
    // The current space is now taken
    ctx.ssd1306.current_x  += Font.FontWidth;
    
    // Return written char for validation
    return ch;
}

// Write full string to screenbuffer
char ssd1306_write_string(char* str, FontDef Font, enum ssd1306_color color)
{
    // Write until null-byte
    while (*str)
    {
        if (ssd1306_write_char(*str, Font, color) != *str)
        {
            // Char could not be written
            return *str;
        }
        
        // Next char
        str++;
    }
    
    // Everything ok
    return *str;
}

// Position the cursor
void ssd1306_set_cursor(uint8_t x, uint8_t y) {
    ctx.ssd1306.current_x = x;
    ctx.ssd1306.current_y = y;
}

void ssd1306_draw_line(int x_start, int y_start, int x_end, int y_end, enum ssd1306_color color)
{
    if (x_start == x_end)
    {
        if( y_start > y_end)
        {
            SWAP_INT(y_start, y_end);
        }

        ssd1306_write_fast_vline(x_start, y_start, y_end - y_start + 1, color);
    }
    else if (y_start == y_end)
    {
        if(x_start > x_end)
        {
            SWAP_INT(x_start, x_end);
        }

        ssd1306_write_fast_hline(x_start, y_start, x_end - x_start + 1, color);
    }
    else
    {
        ssd1306_write_line(x_start, y_start, x_end, y_end, color);
    }
}

void ssd1306_draw_rectangle(int x, int y, uint16_t w, uint16_t h, enum ssd1306_color color)
{

    ssd1306_write_fast_hline(x, y, w, color);
    ssd1306_write_fast_hline(x, y+h-1, w, color);
    ssd1306_write_fast_vline(x, y, h, color);
    ssd1306_write_fast_vline(x+w-1, y, h, color);

}

void ssd1306_draw_fill_rectangle(int x, int y, uint16_t w, uint16_t h, enum ssd1306_color color)
{
    for (int i=x; i<x+w; i++)
    {
        ssd1306_write_fast_vline(i, y, h, color);
    }

}
