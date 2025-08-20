/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_event_handlers.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** EVENT HANDLER INSTANCES ***/
esp_event_handler_instance_t handler_instance_wifi;
esp_event_handler_instance_t handler_instance_ip;
esp_event_handler_instance_t handler_instance_http_server;

/***************************************************************************************************************/
/*** FUNCTION DEFINITIONS **************************************************************************************/

/*** EVENT HANDLER ***/
void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) // COMPLETE
{
    // printf("event_handler running on core %d\n\r", xPortGetCoreID());

    static uint8_t wifi_ap_retry_counter = 0;

    // SEPARATION BASED ON "BASE EVENT"
    if (event_base == WIFI_EVENT)
    {
        // ESP_LOGI(TAG_EVENT_HANDLER, "WIFI EVENT: ");
        // printf("event_handler: WIFI_EVENT running on core %d\n\r", xPortGetCoreID());

        switch (event_id)
        {
        case WIFI_EVENT_WIFI_READY:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_WIFI_READY");
            break;
        }
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_STA_START");
            device_wifi_status.wifi_mode = DEVICE_WIFI_MODE_STATION;
            device_wifi_status.wifi_mode_ch_flag = 1;
            esp_wifi_connect();
            break;
        }
        case WIFI_EVENT_STA_STOP:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_STA_STOP");
            device_wifi_status.wifi_mode = DEVICE_WIFI_MODE_NONE;
            device_wifi_status.wifi_mode_ch_flag = 1;

            // Stop HTTP server if active
            if (http_server_handle != NULL)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "Stopping HTTP server");
                httpd_stop(http_server_handle);
                http_server_handle = NULL;
            }

            // Delete wifi_reconnect_task if active
            wifi_ap_retry_counter = 0;
            if (WiFiReconnectTaskHandle != NULL)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "DELETING TASK: wifi_reconnect_task");
                vTaskDelete(WiFiReconnectTaskHandle);
                WiFiReconnectTaskHandle = NULL;
            }
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_STA_CONNECTED");
            wifi_ap_retry_counter = 0;
            if (WiFiReconnectTaskHandle != NULL)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "DELETING TASK: wifi_reconnect_task");
                vTaskDelete(WiFiReconnectTaskHandle);
                WiFiReconnectTaskHandle = NULL;
            }
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_STA_DISCONNECTED");
            device_wifi_status.wifi_sta_connected_flag = 0;

            if (wifi_ap_retry_counter < WIFI_RECONNECTING_ATTEMPTS)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "Reconnecting attempt form EVENT HANDLER");
                esp_wifi_connect();
                wifi_ap_retry_counter++;
            }
            else if (WiFiReconnectTaskHandle == NULL)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "CREATING TASK: wifi_reconnect_task");
                xTaskCreate(wifi_reconnect_task, "wifi_recn_tsk", 4096, NULL, 5, &WiFiReconnectTaskHandle);
            }
            break;
        }
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
        case WIFI_EVENT_STA_BSS_RSSI_LOW:
        case WIFI_EVENT_STA_BEACON_TIMEOUT:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_STA_xxxx event_id = %ld", event_id);
            break;
        }
        case WIFI_EVENT_AP_START:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_AP_START");
            device_wifi_status.wifi_mode = DEVICE_WIFI_MODE_AP;
            device_wifi_status.wifi_mode_ch_flag = 1;

            // Start HTTP server
            if (http_server_handle == NULL)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "Starting HTTP server");
                switch (start_http_server())
                {
                case START_HTTP_SERVER_OK:
                {
                    break;
                }
                default: // START_HTTP_SERVER_ERROR
                {
                    ESP_LOGE(TAG_EVENT_HANDLER, "Error: start_http_server");

                    // Register new error for error_blink_task
                    device_error_status.new_error_level = ERROR_LEVEL_WARNING;
                    device_error_status.new_error_flag = 1;
                    break;
                }
                }
            }
            break;
        }
        case WIFI_EVENT_AP_STOP:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_AP_STOP");
            device_wifi_status.wifi_mode = DEVICE_WIFI_MODE_NONE;
            device_wifi_status.wifi_mode_ch_flag = 1;

            // Stop HTTP server if active
            if (http_server_handle != NULL)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "Stopping HTTP server");
                httpd_stop(http_server_handle);
                http_server_handle = NULL;
            }
            break;
        }
        case WIFI_EVENT_AP_STACONNECTED:
        case WIFI_EVENT_AP_STADISCONNECTED:
        case WIFI_EVENT_AP_PROBEREQRECVED:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_AP_xxxx (wifi_activity_cnt) event_id = %ld", event_id);
            device_wifi_status.wifi_activity_cnt++;
            break;
        }
        default:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "WIFI_EVENT_xxxx event_id = %ld", event_id);
            break;
        }
        }
    }
    else if (event_base == IP_EVENT)
    {
        // ESP_LOGI(TAG_EVENT_HANDLER, "IP EVENT: ");
        // printf("event_handler: IP_EVENT running on core %d\n\r", xPortGetCoreID());

        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "IP_EVENT_STA_GOT_IP");
            device_wifi_status.wifi_sta_connected_flag = 1;

            // Start HTTP server
            if (http_server_handle == NULL)
            {
                ESP_LOGI(TAG_EVENT_HANDLER, "Starting HTTP server");
                switch (start_http_server())
                {
                case START_HTTP_SERVER_OK:
                {
                    break;
                }
                default: // START_HTTP_SERVER_ERROR
                {
                    ESP_LOGE(TAG_EVENT_HANDLER, "Error: start_http_server");

                    // Register new error for error_blink_task
                    device_error_status.new_error_level = ERROR_LEVEL_WARNING;
                    device_error_status.new_error_flag = 1;
                    break;
                }
                }
            }
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "IP_EVENT_STA_LOST_IP");
            device_wifi_status.wifi_sta_connected_flag = 0;
            break;
        }
        case IP_EVENT_AP_STAIPASSIGNED:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "IP_EVENT_AP_STAIPASSIGNED");
            device_wifi_status.wifi_activity_cnt++;
            break;
        }
        default:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "IP_EVENT_xxxx event_id = %ld", event_id);
            break;
        }
        }
    }
    else if (event_base == ESP_HTTP_SERVER_EVENT)
    {
        // ESP_LOGI(TAG_EVENT_HANDLER, "HTTP SERVER EVENT: ");
        // printf("event_handler: ESP_HTTP_SERVER_EVENT running on core %d\n\r", xPortGetCoreID());

        switch (event_id)
        {
        case HTTP_SERVER_EVENT_ERROR:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "HTTP_SERVER_EVENT_ERROR");
            break;
        }
        case HTTP_SERVER_EVENT_START:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "HTTP_SERVER_EVENT_START");
            break;
        }
        case HTTP_SERVER_EVENT_ON_CONNECTED:
        case HTTP_SERVER_EVENT_ON_HEADER:
        case HTTP_SERVER_EVENT_HEADERS_SENT:
        case HTTP_SERVER_EVENT_ON_DATA:
        case HTTP_SERVER_EVENT_SENT_DATA:
        case HTTP_SERVER_EVENT_DISCONNECTED:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "HTTP_SERVER_EVENT_xxxx (wifi_activity_cnt) event_id = %ld", event_id);
            device_wifi_status.wifi_activity_cnt++;
            break;
        }
        case HTTP_SERVER_EVENT_STOP:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "HTTP_SERVER_EVENT_STOP");
            break;
        }
        default:
        {
            ESP_LOGI(TAG_EVENT_HANDLER, "HTTP_SERVER_EVENT_xxxx event_id = %ld", event_id);
        }
        }
    }
}

/***************************************************************************************************************/
/*** END *******************************************************************************************************/
/***************************************************************************************************************/