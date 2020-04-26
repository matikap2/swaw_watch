/**
 *  @file   debug_log.h.h
 *  @brief  -
 */

//--------------------------------------------------------------------------------

#ifndef _DEBUG_LOG_H_
#define _DEBUG_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------

/* Includes */
#include <stdint.h>
#include <stdbool.h>

//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------------

void debug_log_init(void);
bool debug_log_send(const char data[], size_t len);
bool debug_log(const char format[], ...);

//--------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /* _DEBUG_LOG_H_ */
