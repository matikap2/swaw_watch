/**
 *  @file   hr_app.c
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
#include "queue.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include "hr_app.h"
#include "max30100.h"
#include "debug_log.h"

//--------------------------------------------------------------------------------

#ifndef CFG_HR_APP_LOG_EN
#define CFG_HR_APP_LOG_EN 1
#endif

#if CFG_HR_APP_LOG_EN
#define LOG(fmt, ...)   debug_log("[HR_APP] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   do { } while (0)
#endif

//--------------------------------------------------------------------------------

/* Defines */


//--------------------------------------------------------------------------------

/* Static */
struct hr_app_context
{
    char dummy;
};

static struct hr_app_context ctx;

//--------------------------------------------------------------------------------

/* Static function declarations */

//--------------------------------------------------------------------------------

/* Static functions */

//--------------------------------------------------------------------------------

/* Global functions */

bool hr_app_task_create(void)
{
    if (xTaskCreate(hr_app_task, "hr", configMINIMAL_STACK_SIZE*4, NULL, 4, NULL) != pdPASS)
    {
        return false;
    }

    return true;
}

void hr_app_task(void* params)
{

    LOG("===> HR task started!\n\r");
    uint8_t i2c_read;
    uint16_t ir, red;

    max30100_i2c_init();
    max30100_reset();

    /* MAX30100 */
    i2c_read = max30100_get_rev_id();
    LOG("rev id: %#02x\n\r", i2c_read);
    i2c_read = max30100_get_part_id();
    LOG("part id: %#02x\n\r", i2c_read);

    max30100_set_mode(MODE_SPO2_HR);
    max30100_set_sample_rate(SAMPLE_RATE_100);
    max30100_set_leds(PULSE_WIDTH_1600_uS, LED_27_1, LED_27_1);
    max30100_set_highres(true);

    while (1)
    {
        max30100_read_sensor(&ir, &red);
        LOG("ir: %d/red: %d\n\r", ir, red);
//        debug_log("%d\n", red);
        vTaskDelay(10);
    }
}
