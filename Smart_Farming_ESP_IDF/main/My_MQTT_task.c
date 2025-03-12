#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "cJSON.h"  // Include cJSON library for JSON handling

#include "esp_log.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "My_MQTT_task.h"
#include "sensor_interface_task.h"

static const char TAG[] = "MQTT";

static bool mqtt_started = false;

// MQTT client handle
esp_mqtt_client_handle_t client = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t mqtt_task_queue_handle;

static BaseType_t My_MQTT_task_send_message(mqtt_task_message_e msgID)
{
    mqtt_task_queue_message_t msg;
    msg.msgID = msgID;
    BaseType_t ret = xQueueSend(mqtt_task_queue_handle, &msg, pdMS_TO_TICKS(100));
    if(ret != pdTRUE)
    {
        ESP_LOGW(TAG, "MQTT queue full, message not sent");
    }
    return ret;
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) 
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            My_MQTT_task_send_message(MY_MQTT_TASK_CONNECTED);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            My_MQTT_task_send_message(MY_MQTT_TASK_DISCONNECTED);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            My_MQTT_task_send_message(MY_MQTT_TASK_SUBSCRIBED);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            My_MQTT_task_send_message(MY_MQTT_TASK_PUBLISHED);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) 
            {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            else if(event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
            {
                ESP_LOGE(TAG, "MQTT_ERROR_TYPE_CONNECTION_REFUSED");
            }
            My_MQTT_task_send_message(MY_MQTT_TASK_ERROR);
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void publish_sensor_data(void)
{
    char topic[] = MY_MQTT_TOPIC;

    // Create JSON object
    cJSON *json_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_data, "temperature", get_temperature());
    cJSON_AddNumberToObject(json_data, "humidity", get_humidity());

    // Convert JSON object to string
    char *json_string = cJSON_PrintUnformatted(json_data);

    if (json_string != NULL)
    {
        // Publish JSON data
        esp_mqtt_client_publish(client, topic, json_string, strlen(json_string), MY_MQTT_QOS, 0);
        ESP_LOGI(TAG, "Published: %s", json_string);

        // Free allocated memory
        cJSON_free(json_string);
    }

    // Delete JSON object
    cJSON_Delete(json_data);
}

static void go_to_deep_sleep(void)
{
    const int wakeup_time_sec = 60;
    ESP_LOGI(TAG, "Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
    rtc_gpio_isolate(GPIO_NUM_12);
    esp_wifi_stop();
    ESP_LOGW(TAG, "Entering deep sleep...");
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_deep_sleep_start();
}

void My_MQTT_task(void *pvParameters)
{
    mqtt_task_queue_message_t msg;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_ADDRESS,
        .credentials.username = BROKER_USERNAME,
        .credentials.authentication.password = BROKER_PASSWORD,
        .credentials.client_id = MY_MQTT_CLIENT_ID,
        .session.keepalive = MY_MQTT_KEEPALIVE,
        .session.disable_clean_session = true,
        .session.protocol_ver = MY_MQTT_PROTOCOL,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    while(1)
    {
        if(xQueueReceive(mqtt_task_queue_handle, &msg, portMAX_DELAY))
        {
            switch(msg.msgID)
            {
                case MY_MQTT_TASK_CONNECTED:
                case MY_MQTT_TASK_PUBLISHED:
                    // Publish data using QoS = 0
                    publish_sensor_data();
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    esp_mqtt_client_disconnect(client);
                    go_to_deep_sleep();
                    break;

                case MY_MQTT_TASK_DISCONNECTED:
                case MY_MQTT_TASK_ERROR:
                    // Wait 5 seconds before reconnecting
                    vTaskDelay(pdMS_TO_TICKS(5000));  
                    esp_mqtt_client_reconnect(client);
                    break;

                default:
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void My_MQTT_task_start(void)
{
    if (mqtt_started) {
        ESP_LOGW(TAG, "MQTT task already started, skipping.");
        return;
    }
    mqtt_started = true;

    mqtt_task_queue_handle = xQueueCreate(5, sizeof(mqtt_task_queue_message_t));
    if (mqtt_task_queue_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT task queue");
        return;
    }

    xTaskCreate(&My_MQTT_task, "My_MQTT_task", MY_MQTT_TASK_STACK_SIZE, NULL, MY_MQTT_TASK_PRIORITY, NULL);
}