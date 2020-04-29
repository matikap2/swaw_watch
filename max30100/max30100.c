/**
 *  @file   max30100.c
 *  @brief  -
 */

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "stm32l1xx_hal.h"

#include "max30100.h"
#include "debug_log.h"

//--------------------------------------------------------------------------------

#ifndef CFG_MAX30100_LOG_EN
#define CFG_MAX30100_LOG_EN 1
#endif

#if CFG_MAX30100_LOG_EN
#define LOG(fmt, ...)   debug_log("[MAX30100] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   do { } while (0)
#endif

//--------------------------------------------------------------------------------

/* Defines */

/** @brief Registers of Maxim MAX30100 sensor. */
#define MAX30100_INT_STATUS     0x00  // Which interrupts are tripped
#define MAX30100_INT_ENABLE     0x01  // Which interrupts are active
#define MAX30100_FIFO_WR_PTR    0x02  // Where data is being written
#define MAX30100_OVRFLOW_CTR    0x03  // Number of lost samples
#define MAX30100_FIFO_RD_PTR    0x04  // Where to read from
#define MAX30100_FIFO_DATA      0x05  // Ouput data buffer
#define MAX30100_MODE_CONFIG    0x06  // Control register
#define MAX30100_SPO2_CONFIG    0x07  // Oximetry settings
#define MAX30100_LED_CONFIG     0x09  // Pulse width and power of LEDs
#define MAX30100_TEMP_INTG      0x16  // Temperature value, whole number
#define MAX30100_TEMP_FRAC      0x17  // Temperature value, fraction
#define MAX30100_REV_ID         0xFE  // Part revision
#define MAX30100_PART_ID        0xFF  // Part ID, normally 0x11

#define MAX30100_I2C_ADDR       0xAE

#define MAX30100_I2C_PORT       GPIOB
#define MAX30100_I2C_PIN_SCL    GPIO_PIN_10
#define MAX30100_I2C_PIN_SDA    GPIO_PIN_11
#define MAX30100_I2Cx           I2C2

//--------------------------------------------------------------------------------

/* Static */
struct max30100_context
{
    I2C_HandleTypeDef handle;
};

static struct max30100_context ctx;

//--------------------------------------------------------------------------------

/* Static function declarations */
static uint8_t max30100_read(uint8_t device_register);
static void max30100_write(uint8_t device_register, uint8_t reg_data);

//--------------------------------------------------------------------------------

/* Static functions */
static uint8_t max30100_read(uint8_t device_register)
{
   uint8_t read_data;
   HAL_I2C_Mem_Read(&ctx.handle, MAX30100_I2C_ADDR, device_register, I2C_MEMADD_SIZE_8BIT, &read_data, 1, 250);
   return read_data;
}

static void max30100_write(uint8_t device_register, uint8_t reg_data)
{
    HAL_I2C_Mem_Write(&ctx.handle, MAX30100_I2C_ADDR, device_register, I2C_MEMADD_SIZE_8BIT, &reg_data, 1, 250);
}

//--------------------------------------------------------------------------------

/* Global functions */
void max30100_i2c_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    GPIO_InitStruct.Pin = MAX30100_I2C_PIN_SCL | MAX30100_I2C_PIN_SDA;

    HAL_GPIO_Init(MAX30100_I2C_PORT, &GPIO_InitStruct);

    __I2C2_CLK_ENABLE();

    ctx.handle.Instance = MAX30100_I2Cx;

    ctx.handle.Init.ClockSpeed = 100000;
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

void max30100_startup(void)
{
  uint8_t reg = max30100_read(MAX30100_MODE_CONFIG);  // Get the current register
  max30100_write(MAX30100_MODE_CONFIG, reg & 0x7F);   // mask the SHDN bit
}

void max30100_reset(void)
{
   uint8_t reg = max30100_read(MAX30100_MODE_CONFIG);
   max30100_write(MAX30100_MODE_CONFIG, reg | 0x40);
}

void max30100_shutdown(void)
{
  uint8_t reg = max30100_read(MAX30100_MODE_CONFIG);  // Get the current register
  max30100_write(MAX30100_MODE_CONFIG, reg | 0x80);   // mask the SHDN bit
}

uint8_t max30100_get_rev_id(void)
{
  return max30100_read(MAX30100_REV_ID);
}

uint8_t max30100_get_part_id(void)
{
  return max30100_read(MAX30100_PART_ID);
}

void max30100_set_leds(enum max30100_led_pulse_width pw, enum max30100_led_current red, enum max30100_led_current ir)
{
  uint8_t reg = max30100_read(MAX30100_SPO2_CONFIG);
  reg = reg & 0xFC; // Set LED_PW to 00
  max30100_write(MAX30100_SPO2_CONFIG, reg | pw);     // Mask LED_PW
  max30100_write(MAX30100_LED_CONFIG, (red << 4) | ir); // write LED configs
}

void max30100_set_mode(enum max30100_mode mode)
{
  uint8_t reg = max30100_read(MAX30100_MODE_CONFIG);
  reg = reg & 0xf8; // Set Mode to 000
  max30100_write(MAX30100_MODE_CONFIG, reg | mode); // Mask MODE
}

void max30100_set_sample_rate(enum max30100_sample_rate sr)
{
  uint8_t reg = max30100_read(MAX30100_SPO2_CONFIG);
  reg = reg & 0xE3; // Set SPO2_SR to 000
  max30100_write(MAX30100_SPO2_CONFIG, reg | (sr << 2)); // Mask SPO2_SR
}

void max30100_set_highres(bool enabled)
{
    uint8_t reg = max30100_read(MAX30100_SPO2_CONFIG);

    if (enabled)
    {
        max30100_write(MAX30100_SPO2_CONFIG, reg | (1 << 6));
    }
    else
    {
        max30100_write(MAX30100_SPO2_CONFIG, reg & ~(1 << 6));
    }
}

uint8_t max30100_get_sample_number(void)
{
    uint8_t wr = max30100_read(MAX30100_FIFO_WR_PTR);
    uint8_t rd = max30100_read(MAX30100_FIFO_RD_PTR);
    return (abs(16 + wr - rd) % 16);
}

void max30100_read_sensor(uint16_t *ir, uint16_t *red)
{
  uint8_t temp[4] = {0};  // Temporary buffer for read values

  HAL_I2C_Mem_Read(&ctx.handle, MAX30100_I2C_ADDR, MAX30100_FIFO_DATA, I2C_MEMADD_SIZE_8BIT, &temp[0], 4, 250);

  *ir = (temp[0]<<8) | temp[1];    // Combine values to get the actual number
  *red = (temp[2]<<8) | temp[3];   // Combine values to get the actual number
}
