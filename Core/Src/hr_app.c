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
#include "timers.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include "hr_app.h"
#include "max30100.h"
#include "debug_log.h"
#include "oled_app.h"

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
#define MINUTE_IN_MS        60000
#define CFG_HR_MEAS_MS      20000

//--------------------------------------------------------------------------------

/* Static */
struct hr_app_context
{
    bool start;
    TimerHandle_t bpm_timer;
    uint32_t beat_cnt;
    uint8_t bpm;
    bool was_first_callback;

    struct
    {
        int16_t ir_ac_max;
        int16_t ir_ac_min;

        int16_t ir_ac_signal_curr;
        int16_t ir_ac_signal_prev;
        int16_t ir_ac_signal_min;
        int16_t ir_ac_signal_max;
        int16_t ir_avg_estimated;

        bool positive_edge;
        bool negative_edge;
        int32_t ir_avg_reg;

        int16_t cbuf[32];
        uint8_t offset;
    } beats;
};

static struct hr_app_context ctx;

static const uint16_t lowpass_fir_coeff[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};

//--------------------------------------------------------------------------------

/* Static function declarations */
static bool check_for_beat(int32_t sample);
static int16_t avg_dc_estimator(int32_t *p, uint16_t x);
static int16_t lowpass_fir(int16_t din);
static int32_t mul16(int16_t x, int16_t y);
static void init_beat_ctx(void);

static void hr_app_timer_callback(TimerHandle_t xTimer);

//--------------------------------------------------------------------------------

/* Static functions */
/* https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/blob/master/src/heartRate.h */

//  Heart Rate Monitor functions takes a sample value and the sample number
//  Returns true if a beat is detected
//  A running average of four samples is recommended for display on the screen.
static bool check_for_beat(int32_t sample)
{
    bool beat_detected = false;

    //  Save current state
    ctx.beats.ir_ac_signal_prev = ctx.beats.ir_ac_signal_curr;

    //  Process next data sample
    ctx.beats.ir_avg_estimated = avg_dc_estimator(&ctx.beats.ir_avg_reg, sample);
    ctx.beats.ir_ac_signal_curr = lowpass_fir(sample - ctx.beats.ir_avg_estimated);

    //  Detect positive zero crossing (rising edge)
    if ((ctx.beats.ir_ac_signal_prev < 0) & (ctx.beats.ir_ac_signal_curr >= 0))
    {

        ctx.beats.ir_ac_max = ctx.beats.ir_ac_signal_max; //Adjust our AC max and min
        ctx.beats.ir_ac_min = ctx.beats.ir_ac_signal_min;

        ctx.beats.positive_edge = true;
        ctx.beats.negative_edge = false;
        ctx.beats.ir_ac_signal_max = 0;

    //if ((ir_ac_max - ir_ac_min) > 100 & (ir_ac_max - ir_ac_min) < 1000)
    if (((ctx.beats.ir_ac_max - ctx.beats.ir_ac_min) > 20) &&
            ((ctx.beats.ir_ac_max - ctx.beats.ir_ac_min) < 1000))
    {
        //Heart beat!!!
        beat_detected = true;
    }
    }

    //  Detect negative zero crossing (falling edge)
    if ((ctx.beats.ir_ac_signal_prev > 0) && (ctx.beats.ir_ac_signal_curr <= 0))
    {
        ctx.beats.positive_edge = false;
        ctx.beats.negative_edge = true;
        ctx.beats.ir_ac_signal_min = 0;
    }

    //  Find Maximum value in positive cycle
    if (ctx.beats.positive_edge && (ctx.beats.ir_ac_signal_curr > ctx.beats.ir_ac_signal_prev))
    {
        ctx.beats.ir_ac_signal_max = ctx.beats.ir_ac_signal_curr;
    }

    //  Find Minimum value in negative cycle
    if (ctx.beats.negative_edge && (ctx.beats.ir_ac_signal_curr < ctx.beats.ir_ac_signal_prev))
    {
        ctx.beats.ir_ac_signal_min = ctx.beats.ir_ac_signal_curr;
    }

    return(beat_detected);
}

//  Average DC Estimator
static int16_t avg_dc_estimator(int32_t *p, uint16_t x)
{
  *p += ((((long) x << 15) - *p) >> 4);
  return (*p >> 15);
}

//  Low Pass FIR Filter
static int16_t lowpass_fir(int16_t din)
{
    ctx.beats.cbuf[ctx.beats.offset] = din;

  int32_t z = mul16(lowpass_fir_coeff[11], ctx.beats.cbuf[(ctx.beats.offset - 11) & 0x1F]);

  for (uint8_t i = 0 ; i < 11 ; i++)
  {
    z += mul16(lowpass_fir_coeff[i], ctx.beats.cbuf[(ctx.beats.offset - i) & 0x1F] + ctx.beats.cbuf[(ctx.beats.offset - 22 + i) & 0x1F]);
  }

  ctx.beats.offset++;
  ctx.beats.offset %= 32; //Wrap condition

  return(z >> 15);
}

//  Integer multiplier
int32_t mul16(int16_t x, int16_t y)
{
  return((long)x * (long)y);
}

static void init_beat_ctx(void)
{
    ctx.beats.ir_ac_max = 20;
    ctx.beats.ir_ac_min = -20;
    ctx.beats.ir_ac_signal_curr = 0;
    ctx.beats.ir_ac_signal_min = 0;
    ctx.beats.ir_ac_signal_max = 0;
    ctx.beats.positive_edge = 0;
    ctx.beats.negative_edge = 0;
    ctx.beats.ir_avg_reg = 0;
    ctx.beats.offset = 0;
}

static void hr_app_timer_callback(TimerHandle_t xTimer)
{
    static struct oled_queue_msg bpm_msg;
    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT(xTimer);

    if (!ctx.was_first_callback)
    {
        ctx.was_first_callback = true;
    }

    LOG("Beat timer elapsed!");
    ctx.bpm = ctx.beat_cnt * (MINUTE_IN_MS / CFG_HR_MEAS_MS);

    bpm_msg.new_state = OLED_HR_DISPLAY;
    bpm_msg.heart_rate = ctx.bpm;
    bpm_msg.sp02 = 0;
    oled_app_queue_add(&bpm_msg);

    ctx.beat_cnt = 0;
}

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
    struct oled_queue_msg oled_msg;
    uint16_t ir, red;
    uint8_t meas_cnt;

    bool ready = false;
    ctx.start = false;

    init_beat_ctx();
    max30100_i2c_init();
    max30100_reset();

    /* MAX30100 */
    uint8_t i2c_read = max30100_get_rev_id();
    LOG("rev id: %#02x\n\r", i2c_read);
    i2c_read = max30100_get_part_id();
    LOG("part id: %#02x\n\r", i2c_read);

    while (1)
    {
        if (ctx.start)
        {
            if (!ready)
            {
                LOG("HR initialization...");
                oled_msg.new_state = OLED_STARTUP;
                oled_app_queue_add(&oled_msg);
                vTaskDelay(2000);

                init_beat_ctx();
                max30100_reset();
                max30100_set_mode(MODE_SPO2_HR);
                max30100_set_sample_rate(SAMPLE_RATE_100);
                max30100_set_leds(PULSE_WIDTH_1600_uS, LED_27_1, LED_27_1);
                max30100_set_highres(true);
                ready = true;
                hr_app_start_timer();
                ctx.beat_cnt = 0;
                ctx.bpm = 0;

                ctx.was_first_callback = false;
                meas_cnt = 0;
            }

            max30100_read_sensor(&ir, &red);
            if (check_for_beat(ir))
            {
                LOG("beat!");
                ctx.beat_cnt++;
            }
        }
        else
        {
            if (ready)
            {
                LOG("HR shutdown...");
                oled_msg.new_state = OLED_SHUTDOWN;
                oled_app_queue_add(&oled_msg);

                hr_app_stop_timer();
                max30100_reset();
                max30100_shutdown();
                ready = false;

                vTaskDelay(2000);

                oled_msg.new_state = OLED_OFF;
                oled_app_queue_add(&oled_msg);
            }
        }

        if (!ctx.was_first_callback && ready)
        {
            if (meas_cnt%10 == 0)
            {
                oled_msg.new_state = OLED_HR_MEASURMENT;
                oled_app_queue_add(&oled_msg);
            }

            meas_cnt++;
        }

        vTaskDelay(10);
    }
}

void hr_app_switch_on_off(void)
{
    ctx.start ^= true;
}

bool hr_app_create_timer(void)
{
    ctx.bpm_timer = xTimerCreate("BPM", (((CFG_HR_MEAS_MS) * configTICK_RATE_HZ*1ULL) / 1000), pdTRUE, (void*) 0, hr_app_timer_callback);

    if(ctx.bpm_timer == NULL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool hr_app_start_timer(void)
{
    if(xTimerStart(ctx.bpm_timer, 0 ) != pdPASS)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void hr_app_stop_timer(void)
{
    xTimerStop(ctx.bpm_timer, 0);
}
