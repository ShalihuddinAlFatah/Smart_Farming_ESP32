/**
 * MQTT task code for Smart Farming ESP-IDF
 * Author: Shalihuddin Al Fatah
 */

#ifndef MY_MQTT_TASK_H_
#define MY_MQTT_TASK_H_

#define MY_MQTT_TASK_STACK_SIZE    8192
#define MY_MQTT_TASK_PRIORITY      5

#define BROKER_ADDRESS          "mqtt://192.168.0.248:1883"
#define BROKER_USERNAME         "Your_MQTT_Broker_Username"
#define BROKER_PASSWORD         "Your_MQTT_Broker_Password"
#define MY_MQTT_CLIENT_ID       "ESP32-SmartFarming"
#define MY_MQTT_PROTOCOL        MQTT_PROTOCOL_V_3_1_1
#define MY_MQTT_KEEPALIVE       30
#define MY_MQTT_QOS             0
#define MY_MQTT_TOPIC           "/smartfarming"

// MQTT task message enum
typedef enum mqtt_task_message
{
    MY_MQTT_TASK_CONNECTED = 0,
    MY_MQTT_TASK_DISCONNECTED,
    MY_MQTT_TASK_SUBSCRIBED,
    MY_MQTT_TASK_UNSUBSCRIBED,
    MY_MQTT_TASK_PUBLISHED,
    MY_MQTT_TASK_DATA_RECEIVED,
    MY_MQTT_TASK_ERROR,
} mqtt_task_message_e;

typedef struct mqtt_task_queue_message
{
    mqtt_task_message_e msgID;
} mqtt_task_queue_message_t;

/**
 * @brief Start MQTT task
 */
void My_MQTT_task_start(void);

#endif /* MY_MQTT_TASK_H_ */