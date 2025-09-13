#ifndef SOIL_MOISTURE_H_
#define SOIL_MOISTURE_H_

#include "esp_log.h"

#include "ADS111x.h"
#include "sensor_interface_task.h"

/**
 * @brief Get soil moisture value by calling appropriate ADC functions
 * @return Soil moisture value
 */
float getSoilMoisture(void);

#endif /* SOIL_MOISTURE_H_ */