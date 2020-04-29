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
#include "queue.h"

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
#define OLED_QUEUE_LEN          5
#define OLED_TASK_PRIORITY      (tskIDLE_PRIORITY + 1)

#define MAX_MEAS_CNT            10

//--------------------------------------------------------------------------------

/* Static */
struct oled_app_context
{
    enum oled_state state;
    QueueHandle_t oled_queue;
};

static struct oled_app_context ctx;

//--------------------------------------------------------------------------------

/* Static function declarations */

//--------------------------------------------------------------------------------

/* Static functions */

//--------------------------------------------------------------------------------

/* Global functions */
bool oled_app_queue_create(void)
{
    ctx.oled_queue = NULL;

    ctx.oled_queue = xQueueCreate(OLED_QUEUE_LEN, sizeof(struct oled_queue_msg*));

    if (ctx.oled_queue != NULL)
    {
        return true;
    }

    return false;
}

bool oled_app_queue_add(struct oled_queue_msg *msg)
{
    xQueueSend(ctx.oled_queue, msg, portMAX_DELAY);
}

bool oled_app_task_create(void)
{
    if (xTaskCreate(oled_app_task, "oled", configMINIMAL_STACK_SIZE*4, NULL, 3, NULL) != pdPASS)
    {
        return false;
    }

    return true;
}

void oled_app_task(void* params)
{
    struct oled_queue_msg msg;
    char buffer[16];
    volatile uint8_t meas_cnt = 0;

    LOG("===> OLED task started!\n");

    ctx.state = OLED_OFF;

    ssd1306_i2c_init();
    ssd1306_init();
    ssd1306_update_screen();

    while (1)
    {
        xQueueReceive(ctx.oled_queue, &msg, portMAX_DELAY );

        LOG("Received msg! ID #%d\n", msg.new_state);
        switch (msg.new_state)
        {
        case OLED_OFF:
            ctx.state = OLED_OFF;
            ssd1306_fill(COLOR_BLACK);
            break;

        case OLED_STARTUP:
            ctx.state = OLED_STARTUP;
            ssd1306_fill(COLOR_BLACK);
            ssd1306_set_cursor(2, 24);
            ssd1306_write_string("SWAW watch", Font_11x18, COLOR_WHITE);
            break;

        case OLED_SHUTDOWN:
            ctx.state = OLED_SHUTDOWN;
            ssd1306_fill(COLOR_BLACK);
            ssd1306_set_cursor(16, 24);
            ssd1306_write_string("Goodbye!", Font_11x18, COLOR_WHITE);
            break;

        case OLED_TIME_DISPLAY:
            ctx.state = OLED_TIME_DISPLAY;
            ssd1306_fill(COLOR_BLACK);
            ssd1306_set_cursor(24, 24);
            ssd1306_write_string("hh:mm", Font_16x26, COLOR_WHITE);
            break;

        case OLED_HR_MEASURMENT:
            ssd1306_fill(COLOR_BLACK);
            if (ctx.state != OLED_HR_MEASURMENT)
            {
                ctx.state = OLED_HR_MEASURMENT;
                meas_cnt = 1;
                ssd1306_draw_fill_rectangle(0, 60, (SSD1306_WIDTH/MAX_MEAS_CNT) * meas_cnt, 4, COLOR_WHITE);
            }
            else
            {
                if (meas_cnt < MAX_MEAS_CNT - 1)
                {
                    meas_cnt++;
                    ssd1306_draw_fill_rectangle(0, 60, (SSD1306_WIDTH/MAX_MEAS_CNT) * meas_cnt, 4, COLOR_WHITE);
                }
                else
                {
                    ssd1306_draw_fill_rectangle(0, 60, SSD1306_WIDTH, 4, COLOR_WHITE);
                }
            }
            ssd1306_set_cursor(0, 8);
            ssd1306_write_string("Measurment", Font_11x18, COLOR_WHITE);
            ssd1306_set_cursor(0, 32);
            ssd1306_write_string("in progress", Font_11x18, COLOR_WHITE);
            break;

        case OLED_HR_DISPLAY:
            ctx.state = OLED_HR_DISPLAY;
            ssd1306_fill(COLOR_BLACK);
            ssd1306_set_cursor(0, 8);
            snprintf(buffer, sizeof(buffer), "Sp02: %d%%", msg.sp02);
            ssd1306_write_string(buffer, Font_11x18, COLOR_WHITE);
            ssd1306_set_cursor(0, 32);
            snprintf(buffer, sizeof(buffer), "HR: %d BPM", msg.heart_rate);
            ssd1306_write_string(buffer, Font_11x18, COLOR_WHITE);
            break;

        default:
            break;
        }

        ssd1306_update_screen();
    }
}
