/**
 *  @file   ui.c
 *  @brief  -
 */

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm32l1xx_hal.h"

#include "ui.h"
#include "debug_log.h"
#include "hr_app.h"

//--------------------------------------------------------------------------------

#ifndef CFG_UI_LOG_EN
#define CFG_UI_LOG_EN 1
#endif

#if CFG_UI_LOG_EN
#define LOG(fmt, ...)   debug_log("[UI] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   do { } while (0)
#endif

//--------------------------------------------------------------------------------

/* Defines */
#define USER_LED_PORT       GPIOA
#define USER_LED_PIN        GPIO_PIN_5

#define USER_BUTTON_PORT    GPIOB
#define USER_BUTTON_PIN     GPIO_PIN_3

//--------------------------------------------------------------------------------

/* Static */
struct ui_context
{
    char dummy;
    bool test;
};

static struct ui_context ctx;

//--------------------------------------------------------------------------------

/* Static function declarations */

//--------------------------------------------------------------------------------

/* Static functions */

//--------------------------------------------------------------------------------

/* Global functions */

void led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = USER_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(USER_LED_PORT, &GPIO_InitStruct);
}

void led_change_state(bool state)
{
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, state);
}

void button_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = USER_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(USER_BUTTON_PORT, &GPIO_InitStruct);
}

bool button_polling_readstate(void)
{
    return HAL_GPIO_ReadPin(USER_BUTTON_PORT, USER_BUTTON_PIN);
//    return ctx.test;
}

void button_interrupt_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = USER_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(USER_BUTTON_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI3_IRQn, 15, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == USER_BUTTON_PIN)
    {
//        for (uint32_t i = 0; i < 333000; i++)
//        {
//            asm("NOP");
//        }
//
//        if (!button_polling_readstate())
//        {
            LOG("Button irq!");
            hr_app_switch_on_off();
//        }
    }
}
