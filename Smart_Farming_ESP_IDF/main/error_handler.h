#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

/**
 * @brief General error handler
 * @param error_source Appropriate TAG where the error happened
 * @note Just restart the device for now
 */
void my_error_handler(const char* error_source);

#endif /* ERROR_HANDLER_H_ */