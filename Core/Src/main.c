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
#define USER_LED_PORT   GPIOA
#define USER_LED_PIN    GPIO_PIN_5

//--------------------------------------------------------------------------------

/* Static function declarations */
static void system_clock_config(void);
static void led_init(void);
static void led_change_state(bool state);
static void task_led(void* params);

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

static void led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = USER_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(USER_LED_PORT, &GPIO_InitStruct);
}

static void led_change_state(bool state)
{
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, state);
}

static void task_led(void* params)
{
    ssd1306_init();
    LOG("Writing\n\r");
    ssd1306_write_string("OLED test", Font_11x18, COLOR_WHITE);
    ssd1306_update_screen();
    LOG("Written!\n\r");

    while (1)
    {
        LOG("TEST\n\r");
        led_change_state(true);
        vTaskDelay(1000);
        led_change_state(false);
        vTaskDelay(1000);
    }
}

//--------------------------------------------------------------------------------

/* Global functions */

int main(void)
{
    HAL_Init();

    system_clock_config();
    debug_log_init();
    led_init();
    ssd1306_i2c_init();

    if (xTaskCreate(task_led, "led", configMINIMAL_STACK_SIZE*4, NULL, 3, NULL) != pdPASS) {
        led_change_state(true);
    }

    vTaskStartScheduler();
}
