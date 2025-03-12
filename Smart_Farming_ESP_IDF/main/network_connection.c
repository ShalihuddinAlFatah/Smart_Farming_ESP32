#include "network_connection.h"

#include "esp_log.h"
#include <inttypes.h>
#include <string.h>

#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT 	BIT0
#define WIFI_FAIL_BIT 		BIT1

static const char TAG[] = "network";

// Network connection callback
static network_connected_event_callback_t network_connected_event_cb;

static const int WIFI_RETRY_ATTEMPT = 3;
static int wifi_retry_count = 0;

static esp_netif_t *mynetwork_netif = NULL;
static esp_event_handler_instance_t ip_event_handler;
static esp_event_handler_instance_t wifi_event_handler;

static EventGroupHandle_t s_wifi_event_group = NULL;

/*
 * IP events callback
 * @param arg data, aside from event data, that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id fo the event to register the handler for
 * @param event_data event data
 */
static void ip_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Handling IP event, event code 0x%" PRIx32, event_id);
    switch (event_id)
    {
		case (IP_EVENT_STA_GOT_IP):
			ip_event_got_ip_t *event_ip = (ip_event_got_ip_t *)event_data;
			ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event_ip->ip_info.ip));
			wifi_retry_count = 0;
			xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			break;

		case (IP_EVENT_STA_LOST_IP):
			ESP_LOGI(TAG, "Lost IP");
            wifi_retry_count = 0;
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			break;

		case (IP_EVENT_GOT_IP6):
			ip_event_got_ip6_t *event_ip6 = (ip_event_got_ip6_t *)event_data;
			ESP_LOGI(TAG, "Got IPv6: " IPV6STR, IPV62STR(event_ip6->ip6_info.ip));
			wifi_retry_count = 0;
			xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			break;

		default:
			ESP_LOGI(TAG, "IP event not handled");
			break;
    }
}

/*
 * WiFi events callback
 * @param arg data, aside from event data, that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id fo the event to register the handler for
 * @param event_data event data
 */
static void wifi_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Handling Wi-Fi event, event code 0x%" PRIx32, event_id);

    switch (event_id)
    {
		case (WIFI_EVENT_WIFI_READY):
			ESP_LOGI(TAG, "Wi-Fi ready");
			break;

		case (WIFI_EVENT_SCAN_DONE):
			ESP_LOGI(TAG, "Wi-Fi scan done");
			break;

		case (WIFI_EVENT_STA_START):
			ESP_LOGI(TAG, "Wi-Fi started, connecting to AP...");
			esp_wifi_connect();
			break;

		case (WIFI_EVENT_STA_STOP):
			ESP_LOGI(TAG, "Wi-Fi stopped");
			break;

		case (WIFI_EVENT_STA_CONNECTED):
			ESP_LOGI(TAG, "Wi-Fi connected");
			break;

		case (WIFI_EVENT_STA_DISCONNECTED):
			ESP_LOGI(TAG, "Wi-Fi disconnected");

			if (wifi_retry_count < WIFI_RETRY_ATTEMPT) 
			{
				ESP_LOGI(TAG, "Retrying to connect to Wi-Fi network...");
				esp_wifi_connect();
				wifi_retry_count++;
			} 

			else 
			{
				ESP_LOGI(TAG, "Failed to connect to Wi-Fi network");
				xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
			}
			break;

		case (WIFI_EVENT_STA_AUTHMODE_CHANGE):
			ESP_LOGI(TAG, "Wi-Fi authmode changed");
			break;

		default:
			ESP_LOGI(TAG, "Wi-Fi event not handled");
			break;
    }
}

/*
 * Initialize network connection settings before connecting to network
 * @return ESP_OK or ESP_FAIL
 */
esp_err_t network_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCP/IP network stack");
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop");
        return ret;
    }

    ret = esp_wifi_set_default_wifi_sta_handlers();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default handlers");
        return ret;
    }

    mynetwork_netif = esp_netif_create_default_wifi_sta();
    if (mynetwork_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
        return ESP_FAIL;
    }

    // Wi-Fi stack configuration parameters
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_cb,
                                                        NULL,
                                                        &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &ip_event_cb,
                                                        NULL,
                                                        &ip_event_handler));
    return ret;
}

/*
 * Connecting to network and set appropriate event bits
 * @return ESP_OK or ESP_FAIL
 */
esp_err_t network_connect(char* wifi_ssid, char* wifi_password)
{
    wifi_config_t wifi_config = {
        .sta = {
            // this sets the weakest authmode accepted in fast scan mode (default)
            .threshold.authmode = WIFI_AUTHMODE,
        },
    };

    strncpy((char*)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // default is WIFI_PS_MIN_MODEM
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); // default is WIFI_STORAGE_FLASH

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "Connecting to Wi-Fi network: %s", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) 
	{
        ESP_LOGI(TAG, "Connected to Wi-Fi network: %s", wifi_config.sta.ssid);
        return ESP_OK;
    } 
	else if (bits & WIFI_FAIL_BIT) 
	{
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi network: %s", wifi_config.sta.ssid);
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "Unexpected Wi-Fi error");
    return ESP_FAIL;
}

/*
 * Disconnect from network
 * Implement this function as needed
 * @return ESP_OK or ESP_FAIL
 */
// esp_err_t network_disconnect(void)
// {
//     if (s_wifi_event_group) {
//         vEventGroupDelete(s_wifi_event_group);
//     }

//     return esp_wifi_disconnect();
// }

/*
 * Restore network settings to default
 * Implement this function as needed
 * @return ESP_OK
 */
// esp_err_t network_deinit(void)
// {
//     esp_err_t ret = esp_wifi_stop();
//     if (ret == ESP_ERR_WIFI_NOT_INIT) {
//         ESP_LOGE(TAG, "Wi-Fi stack not initialized");
//         return ret;
//     }

//     ESP_ERROR_CHECK(esp_wifi_deinit());
//     ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(mynetwork_netif));
//     esp_netif_destroy(mynetwork_netif);

//     ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler));
//     ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler));

//     return ESP_OK;
// }

void network_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Network connecting...");
    ESP_ERROR_CHECK(network_init());

    esp_err_t ret = network_connect(WIFI_SSID, WIFI_PASSWORD);
    if(ret != ESP_OK) 
	{
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi network");
    }

    wifi_ap_record_t ap_info;
    ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_ERR_WIFI_CONN) 
	{
        ESP_LOGE(TAG, "Wi-Fi station interface not initialized");
    }

    else if (ret == ESP_ERR_WIFI_NOT_CONNECT) 
	{
        ESP_LOGE(TAG, "Wi-Fi station is not connected");
    } 

	else 
	{
        ESP_LOGI(TAG, "--- Access Point Information ---");
        ESP_LOG_BUFFER_HEX("MAC Address", ap_info.bssid, sizeof(ap_info.bssid));
        ESP_LOG_BUFFER_CHAR("SSID", ap_info.ssid, sizeof(ap_info.ssid));
        ESP_LOGI(TAG, "Primary Channel: %d", ap_info.primary);
        ESP_LOGI(TAG, "RSSI: %d", ap_info.rssi);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

	while(1)
	{
        EventBits_t ux_wifi_event = xEventGroupGetBits(s_wifi_event_group);
        if(ux_wifi_event & WIFI_CONNECTED_BIT)
        {
            // Check for connection callback
            if (network_connected_event_cb)
            {
                network_connection_call_callback();
            }
        }
        else
        {
            ESP_LOGI(TAG, "Getting new IP");
        }
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

void network_connection_set_callback(network_connected_event_callback_t cb)
{
	network_connected_event_cb = cb;
}

void network_connection_call_callback(void)
{
	network_connected_event_cb();
}

void network_start(void)
{
	xTaskCreatePinnedToCore(&network_task, "Network_task", NETWORK_TASK_STACK_SIZE, NULL, NETWORK_TASK_PRIORITY, NULL, NETWORK_TASK_CORE_ID);
}