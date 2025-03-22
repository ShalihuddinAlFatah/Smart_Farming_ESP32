# ESP32_ESP_IDF_Smart_Farming

This is a simple data acquisition device I built for my portfolio. I used a DHT22 temperature sensor to measure air temperature and relative humidity. ESP32-WROOM-32U was used to take the sensor data and publish the data to the Mosquitto Broker server. The sensor data formatted using JSON library. Then, a custom NodeRED flows subscribe to the topic and parse the sensor data and put them inside PostgreSQL database. I also built a simple dashboard to visualize the data in Grafana.

The current consumption during active mode is around 128-130mA. Sleep mode consume around 20mA because I have power LED in the development board and I did not turn off the DHT22. See Pictures directory for more detail.

My setup is the device wakes up from deep sleep to acquire and publish data every one minutes. It is possible to extend the sleep time to 5 minutes to further reduce power consumption. 
Adding Mosfet switch to turn off the sensor and the power LED will help too.

I'll add more sensor soon. The code is modular, so adding new sensor will be easy.

DHT22 library -> https://github.com/Andrey-m/DHT22-lib-for-esp-idf
