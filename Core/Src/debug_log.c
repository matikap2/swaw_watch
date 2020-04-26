/**
 *  @file   debug_log.c
 *  @brief  -
 */

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "stm32l1xx_hal.h"

#include "debug_log.h"

//--------------------------------------------------------------------------------

/* Defines */
#define DEBUG_UART_PORT     GPIOA
#define DEBUG_UART_PIN_TX   GPIO_PIN_2
#define DEBUG_UART_PIN_RX   GPIO_PIN_3
#define DEBUG_UARTx         USART2

//--------------------------------------------------------------------------------

/* Static */
struct debug_log_context
{
    UART_HandleTypeDef handle;
};

static struct debug_log_context ctx;

//--------------------------------------------------------------------------------

/* Static function declarations */

//--------------------------------------------------------------------------------

/* Static functions */

//--------------------------------------------------------------------------------

/* Global functions */

void debug_log_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    GPIO_InitStruct.Pin = DEBUG_UART_PIN_TX | DEBUG_UART_PIN_RX;

    HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

    __USART2_CLK_ENABLE();

    ctx.handle.Instance = DEBUG_UARTx;

    ctx.handle.Init.BaudRate = 115200;
    ctx.handle.Init.WordLength = UART_WORDLENGTH_8B;
    ctx.handle.Init.StopBits = UART_STOPBITS_1;
    ctx.handle.Init.Parity = UART_PARITY_NONE;
    ctx.handle.Init.Mode = UART_MODE_TX_RX;
    ctx.handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    ctx.handle.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&ctx.handle);
    __HAL_UART_ENABLE(&ctx.handle);

}

bool debug_log_send(const char data[], size_t len)
{
    return HAL_UART_Transmit(&ctx.handle, data, len, 0xFFFFFF) == HAL_OK ? true : false;
}

bool debug_log(const char format[], ...)
{
    char buffer[64];
    bool ret;

    va_list args;
    va_start(args, format);

    vsnprintf(buffer, sizeof(buffer), format, args);
    ret = debug_log_send(buffer, strlen(buffer));

    va_end (args);

    return ret;
}
