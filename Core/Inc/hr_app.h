/**
 *  @file   hr_app.h
 *  @brief  -
 */

//--------------------------------------------------------------------------------

#ifndef _HR_APP_H_
#define _HR_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

bool hr_app_task_create(void);
void hr_app_task(void* params);
void hr_app_switch_on_off(void);
bool hr_app_create_timer(void);
bool hr_app_start_timer(void);
void hr_app_stop_timer(void);

//--------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* _HR_APP_H_ */
