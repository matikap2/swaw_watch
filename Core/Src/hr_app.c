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
bool checkForBeat(int32_t sample);
int16_t averageDCEstimator(int32_t *p, uint16_t x);
int16_t lowPassFIRFilter(int16_t din);
int32_t mul16(int16_t x, int16_t y);
//--------------------------------------------------------------------------------

/* Static functions */
/* https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/blob/master/src/heartRate.h */
int16_t IR_AC_Max = 20;
int16_t IR_AC_Min = -20;

int16_t IR_AC_Signal_Current = 0;
int16_t IR_AC_Signal_Previous;
int16_t IR_AC_Signal_min = 0;
int16_t IR_AC_Signal_max = 0;
int16_t IR_Average_Estimated;

int16_t positiveEdge = 0;
int16_t negativeEdge = 0;
int32_t ir_avg_reg = 0;

int16_t cbuf[32];
uint8_t offset = 0;

static const uint16_t FIRCoeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};

//  Heart Rate Monitor functions takes a sample value and the sample number
//  Returns true if a beat is detected
//  A running average of four samples is recommended for display on the screen.
bool checkForBeat(int32_t sample)
{
  bool beatDetected = false;

  //  Save current state
  IR_AC_Signal_Previous = IR_AC_Signal_Current;

  //This is good to view for debugging
  //Serial.print("Signal_Current: ");
  //Serial.println(IR_AC_Signal_Current);

  //  Process next data sample
  IR_Average_Estimated = averageDCEstimator(&ir_avg_reg, sample);
  IR_AC_Signal_Current = lowPassFIRFilter(sample - IR_Average_Estimated);

  //  Detect positive zero crossing (rising edge)
  if ((IR_AC_Signal_Previous < 0) & (IR_AC_Signal_Current >= 0))
  {

    IR_AC_Max = IR_AC_Signal_max; //Adjust our AC max and min
    IR_AC_Min = IR_AC_Signal_min;

    positiveEdge = 1;
    negativeEdge = 0;
    IR_AC_Signal_max = 0;

    //if ((IR_AC_Max - IR_AC_Min) > 100 & (IR_AC_Max - IR_AC_Min) < 1000)
    if ((IR_AC_Max - IR_AC_Min) > 20 & (IR_AC_Max - IR_AC_Min) < 1000)
    {
      //Heart beat!!!
      beatDetected = true;
    }
  }

  //  Detect negative zero crossing (falling edge)
  if ((IR_AC_Signal_Previous > 0) & (IR_AC_Signal_Current <= 0))
  {
    positiveEdge = 0;
    negativeEdge = 1;
    IR_AC_Signal_min = 0;
  }

  //  Find Maximum value in positive cycle
  if (positiveEdge & (IR_AC_Signal_Current > IR_AC_Signal_Previous))
  {
    IR_AC_Signal_max = IR_AC_Signal_Current;
  }

  //  Find Minimum value in negative cycle
  if (negativeEdge & (IR_AC_Signal_Current < IR_AC_Signal_Previous))
  {
    IR_AC_Signal_min = IR_AC_Signal_Current;
  }

  return(beatDetected);
}

//  Average DC Estimator
int16_t averageDCEstimator(int32_t *p, uint16_t x)
{
  *p += ((((long) x << 15) - *p) >> 4);
  return (*p >> 15);
}

//  Low Pass FIR Filter
int16_t lowPassFIRFilter(int16_t din)
{
  cbuf[offset] = din;

  int32_t z = mul16(FIRCoeffs[11], cbuf[(offset - 11) & 0x1F]);

  for (uint8_t i = 0 ; i < 11 ; i++)
  {
    z += mul16(FIRCoeffs[i], cbuf[(offset - i) & 0x1F] + cbuf[(offset - 22 + i) & 0x1F]);
  }

  offset++;
  offset %= 32; //Wrap condition

  return(z >> 15);
}

//  Integer multiplier
int32_t mul16(int16_t x, int16_t y)
{
  return((long)x * (long)y);
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
    uint8_t i2c_read;
    uint16_t ir, red;
    bool beat;

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

    debug_log("ir,red,IR_AC_Signal_Current,IR_Average_Estimated,beat\n");
    while (1)
    {
        max30100_read_sensor(&ir, &red);
        LOG("ir: %d/red: %d\n", ir, red);
        //        debug_log("%d\n", red);
        beat = checkForBeat(ir);
//        LOG("IR_AC_Signal_Current: %d\n", IR_AC_Signal_Current);
//        LOG("IR_Average_Estimated: %d\n", IR_Average_Estimated);
//        LOG("beat: %s\n", beat ? "true" : "false");

//        debug_log("%d,%d,%d,%d,%d\n", ir, red, IR_AC_Signal_Current, IR_Average_Estimated, beat);
        vTaskDelay(10);
    }
}
