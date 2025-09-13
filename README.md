# ESP32_ESP_IDF_Smart_Farming

This is a simple data acquisition device I built for my portfolio. I used a DHT22 temperature sensor to measure air temperature and relative humidity. ESP32-WROOM-32U was used to take the sensor data and publish the data to the Mosquitto Broker server. The sensor data formatted using JSON library. Then, a custom NodeRED flows subscribe to the topic and parse the sensor data and put them inside PostgreSQL database. I also built a simple dashboard to visualize the data in Grafana.

The current consumption during active mode is around 128-130mA. Sleep mode consume around 20mA because I have power LED in the development board and I did not turn off the DHT22. See Pictures directory for more detail.

My setup is the device wakes up from deep sleep to acquire and publish data every one minutes. It is possible to extend the sleep time to 5 minutes to further reduce power consumption. 
Adding Mosfet switch to turn off the sensor and the power LED will help too.

I added new capacitive soil moisture sensor. It uses ADS111x to connect the sensor to the ESP32. I haven't found a reliable source explaining how to interpret the capacitive soil moisture sensor data. So, for now it only display the RAW ADC value.

This project tested using ESP-IDF 5.5.1 in Visual Studio Code. Development board used ESP32 DevKitC and ESP32-S3 DevKitC.

## Files explanation
Device driver and low level code thing:
1. ADS111x.h .c -> ADC111x library
2. DHT22.h .c -> DHT22 library
3. network_connection.h .c -> handles WiFi settings
4. soil_moisture.h .c -> capacitive soil moisture sensor library

Interface file:
1. sensor_interface_task.h .c -> connecting sensor driver to application layer

Application layer:
1. app_main.c
2. error_handler.h .c -> handling error
3. My_MQTT_task.h .c -> collecting sensor data and send them to MQTT broker

## Library used
1. DHT22 library -> https://github.com/Andrey-m/DHT22-lib-for-esp-idf
2. ADS111x library -> https://github.com/ShalihuddinAlFatah/ADS111x_ESP-IDF_9177
