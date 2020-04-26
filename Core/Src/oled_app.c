/**
 *  @file   oled_app.c
 *  @brief  -
 */

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm32l1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include "oled_app.h"
#include "debug_log.h"

//--------------------------------------------------------------------------------

#ifndef CFG_OLED_APP_LOG_EN
#define CFG_OLED_APP_LOG_EN 1
#endif

#if CFG_OLED_APP_LOG_EN
#define LOG(fmt, ...)   debug_log("[OLED_APP] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   do { } while (0)
#endif

//--------------------------------------------------------------------------------

/* Defines */

//--------------------------------------------------------------------------------

/* Static function declarations */

//--------------------------------------------------------------------------------

/* Static functions */

//--------------------------------------------------------------------------------

/* Global functions */
void task_oled(void* params)
{

    while (1)
    {

    }
}
