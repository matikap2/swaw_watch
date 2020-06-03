/**
 *  @file   rtc.c
 *  @brief  -
 */

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm32l1xx_hal.h"

#include "debug_log.h"

//--------------------------------------------------------------------------------

#ifndef CFG_RTC_LOG_EN
#define CFG_RTC_LOG_EN 1
#endif

#if CFG_RTC_LOG_EN
#define LOG(fmt, ...)   debug_log("[RTC] " fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   do { } while (0)
#endif

//--------------------------------------------------------------------------------

/* Defines */

//--------------------------------------------------------------------------------

/* Static */
struct rtc_context
{
    RTC_HandleTypeDef rtc_handler;
};

static struct rtc_context ctx;

//--------------------------------------------------------------------------------

/* Static function declarations */

//--------------------------------------------------------------------------------

/* Static functions */

//--------------------------------------------------------------------------------

/* Global functions */
void rtc_init(void)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);

    ctx.rtc_handler.Instance = RTC;
    ctx.rtc_handler.Init.AsynchPrediv = 124;
    ctx.rtc_handler.Init.SynchPrediv = 249;
    ctx.rtc_handler.Init.OutPut = RTC_OUTPUT_DISABLE;
    ctx.rtc_handler.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    ctx.rtc_handler.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    ctx.rtc_handler.Init.HourFormat = RTC_HOURFORMAT_24;

    HAL_RTC_Init(&ctx.rtc_handler);
    __HAL_RCC_RTC_ENABLE();

    time.Hours = 12;
    time.Minutes = 0;
    time.Seconds = 0;
    time.SubSeconds = 0;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(&ctx.rtc_handler, &time, RTC_FORMAT_BIN);

    date.WeekDay = RTC_WEEKDAY_THURSDAY;
    date.Month = RTC_MONTH_JUNE;
    date.Date = 4;
    date.Year = 20;
    HAL_RTC_SetDate(&ctx.rtc_handler, &date, RTC_FORMAT_BIN);
}

void rtc_get_time(RTC_TimeTypeDef *time, RTC_DateTypeDef *date)
{
    HAL_RTC_GetTime(&ctx.rtc_handler, time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&ctx.rtc_handler, date, RTC_FORMAT_BIN);

//    LOG("Time: %d:%d:%d", time->Hours, time->Minutes, time->Seconds);
//    LOG("Date: %d/%d/%d", date->Date, date->Month, date->Year);
}


