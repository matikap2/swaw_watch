/**
 * 	@file	main.c
 * 	@brief	-
 */

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm32l1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "debug_log.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "max30100.h"
#include "oled_app.h"
#include "hr_app.h"
#include "ui.h"

//--------------------------------------------------------------------------------

#ifndef CFG_MAIN_LOG_EN
#define CFG_MAIN_LOG_EN 1
#endif

#if CFG_MAIN_LOG_EN
#define LOG(fmt, ...)   debug_log("[MAIN] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   do { } while (0)
#endif

//--------------------------------------------------------------------------------

/* Defines */

//--------------------------------------------------------------------------------

/* Static function declarations */
static void system_clock_config(void);
static void hardware_init(void);
static void task_test(void* params);

//--------------------------------------------------------------------------------

/* Static functions */

static void system_clock_config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    /* Configure the main internal regulator output voltage */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* nothing */
    }

    /* Initializes the CPU, AHB and APB busses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        /* nothing */
    }
}

static void hardware_init(void)
{
    HAL_Init();
    system_clock_config();
    debug_log_init();
    led_init();
//    button_init();
}

static void task_test(void* params)
{
    struct oled_queue_msg test;
    uint8_t i = 0;
    uint8_t cnt = 0;

    while (1)
    {
//        bool btn = button_polling_readstate();
//        LOG("BUTTON: %d\n", btn);
        /* QUEUE TEST */
        if (i > OLED_SHUTDOWN)
        {
            i = 0;
        }
        if (i == OLED_HR_MEASURMENT)
        {
            test.new_state = i;
            cnt++;
            if (cnt > 9)
            {
                cnt = 0;
                i++;
            }
        }
        else
        {
            test.new_state = i++;
        }

        test.heart_rate = 69;
        test.sp02 = 98;
//        oled_app_queue_add(&test);

        /* LEDS */
        led_change_state(true);
        vTaskDelay(100);
        led_change_state(false);
        vTaskDelay(100);
    }
}

//--------------------------------------------------------------------------------

/* Global functions */

int main(void)
{
    hardware_init();
    button_interrupt_init();

    if (oled_app_queue_create())
    {
        xTaskCreate(task_test, "test", configMINIMAL_STACK_SIZE*4, NULL, 3, NULL);

//        oled_app_task_create();

//        hr_app_task_create();
    }

    vTaskStartScheduler();
}
