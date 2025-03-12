#ifndef SENSOR_INTERFACE_H_
#define SENSOR_INTERFACE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "DHT22.h"

#define SENSOR_INTERFACE_TASK_STACK_SIZE 4096
#define SENSOR_INTERFACE_TASK_PRIORITY   2
#define SENSOR_INTERFACE_TASK_CORE_ID    1

float get_temperature(void);
float get_humidity(void);
void sensor_interface_start(void);

#endif /* SENSOR_INTERFACE_H_ */