/**
 *  @file   max30100.h
 *  @brief  -
 */

//--------------------------------------------------------------------------------

#ifndef _MAX30100_H_
#define _MAX30100_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>

//--------------------------------------------------------------------------------

enum max30100_sample_rate
{
    SAMPLE_RATE_50 = 0x00,
    SAMPLE_RATE_100,
    SAMPLE_RATE_167,
    SAMPLE_RATE_200,
    SAMPLE_RATE_400,
    SAMPLE_RATE_600,
    SAMPLE_RATE_800,
    SAMPLE_RATE_1000
};

enum max30100_led_pulse_width
{
    PULSE_WIDTH_200_uS = 0x00,
    PULSE_WIDTH_400_uS,
    PULSE_WIDTH_800_uS,
    PULSE_WIDTH_1600_uS
};

enum max30100_led_current
{
    LED_0 = 0x00,
    LED_4_4,
    LED_7_6,
    LED_11,
    LED_14_2,
    LED_17_4,
    LED_20_8,
    LED_24,
    LED_27_1,
    LED_30_6,
    LED_33_8,
    LED_37,
    LED_40_2,
    LED_43_6,
    LED_46_8,
    LED_50
};

enum max30100_mode
{
    MODE_HR_ONLY = 0x02,
    MODE_SPO2_HR = 0x03
};



//--------------------------------------------------------------------------------

void max30100_i2c_init(void);
void max30100_startup(void);
void max30100_reset(void);
void max30100_shutdown(void);
uint8_t max30100_get_rev_id(void);
uint8_t max30100_get_part_id(void);
void max30100_set_leds(enum max30100_led_pulse_width pw, enum max30100_led_current red, enum max30100_led_current ir);
void max30100_set_mode(enum max30100_mode mode);
void max30100_set_sample_rate(enum max30100_sample_rate sr);
void max30100_set_highres(bool enabled);
uint8_t max30100_get_sample_number(void);
void max30100_read_sensor(uint16_t *ir, uint16_t *red);

//--------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* _MAX30100_H_ */
