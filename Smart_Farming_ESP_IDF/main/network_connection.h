#ifndef NETWORK_CONNECTION_H_
#define NETWORK_CONNECTION_H_

#include "esp_err.h"

#include "esp_event.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"

// Callback typedef
typedef void (*network_connected_event_callback_t)(void);

// Network task config
#define NETWORK_TASK_STACK_SIZE 4096
#define NETWORK_TASK_PRIORITY	3
#define NETWORK_TASK_CORE_ID	0

// Enter the Wi-Fi credentials here
#define WIFI_SSID 				"My_WiFi_SSID"
#define WIFI_PASSWORD 			"My_WiFi_Password"
#define WIFI_AUTHMODE 			WIFI_AUTH_WPA2_PSK

// esp_err_t network_init(void);

// esp_err_t network_connect(char* wifi_ssid, char* wifi_password);

// esp_err_t network_disconnect(void);

// esp_err_t network_deinit(void);

/**
 * Sets the callback function.
 */
void network_connection_set_callback(network_connected_event_callback_t cb);

/**
 * Calls the callback function.
 */
void network_connection_call_callback(void);

/* 
 * Start of network connection task
 */
void network_start(void);

#endif /* NETWORK_CONNECTION_H_ */