/**
 * Sensor interface codes. This part connects the sensor drivers with the application (app_main, My_MQTT_task)
 * Author: Shalihuddin Al Fatah
 */

#ifndef SENSOR_INTERFACE_H_
#define SENSOR_INTERFACE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "error_handler.h"
#include "ADS111x.h"
#include "soil_moisture.h"
#include "DHT22.h"

#define SENSOR_INTERFACE_TASK_STACK_SIZE 4096
#define SENSOR_INTERFACE_TASK_PRIORITY   2
#define SENSOR_INTERFACE_TASK_CORE_ID    1

// ESP32-S3 GPIO config
// #define DHT_GPIO   GPIO_NUM_15

// #define ADC_SDA    GPIO_NUM_18
// #define ADC_SCL    GPIO_NUM_17

// ESP32 GPIO config
#define DHT_GPIO   GPIO_NUM_25

#define ADC_SDA    GPIO_NUM_27
#define ADC_SCL    GPIO_NUM_26

#define ADC_I2C    I2C_NUM_0

// ADS111x config structure
// Must be accessible from soil_moisture.c, so it uses extern
extern ads111x_cfg_t my_ads111x_cfg;

// ADC parameters
#define I2C_SPEED_HZ 100000

/**
 * @brief Get temperature data from any temperature sensor 
 * @return Temperature sensor data
 */
float get_temperature(void);

/**
 * @brief Get humidity data from any humidity sensor 
 * @return Humidity sensor data
 */
float get_humidity(void);

/**
 * @brief Get soil moisture data from any soil moisture sensor 
 * @return Soil moisture sensor data 
 */
float get_soil_moisture(void);

/**
 * @brief Start sensor interface task
 */
void sensor_interface_start(void);

#endif /* SENSOR_INTERFACE_H_ */