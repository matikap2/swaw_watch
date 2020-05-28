/**
 *  @file   oled_app.h
 *  @brief  -
 */

//--------------------------------------------------------------------------------

#ifndef _OLED_APP_H_
#define _OLED_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>

//--------------------------------------------------------------------------------

enum oled_state
{
    OLED_OFF,
    OLED_STARTUP,
    OLED_TIME_DISPLAY = 99,
    OLED_HR_MEASURMENT = 2,
    OLED_HR_DISPLAY,
    OLED_SHUTDOWN
};

struct oled_queue_msg
{
    enum oled_state new_state;
    uint8_t heart_rate;
    uint8_t sp02;
};

//--------------------------------------------------------------------------------

bool oled_app_queue_create(void);
bool oled_app_queue_add(struct oled_queue_msg *msg);

bool oled_app_task_create(void);
void oled_app_task(void* params);

//--------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* _OLED_APP_H_ */
