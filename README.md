#ESP32_ESP_IDF_Smart_Farming

This is a simple data acquisition device I built for my portfolio. I used a DHT22 temperature sensor to measure air temperature and relative humidity. ESP32-WROOM-32U was used to take the sensor data and publish the data to the Mosquitto Broker server. The sensor data formatted using JSON library. Then, a custom NodeRED flows subscribe to the topic and parse the sensor data and put them inside PostgreSQL database. I also built a simple dashboard to visualize the data in Grafana.

I'll add more sensor soon. The code is modular, so adding new sensor will be easy.

DHT22 library -> https://github.com/Andrey-m/DHT22-lib-for-esp-idf
