/***************************************************************************************************************/
/** LED CONTROLLER
 * VERSION: 1.2.0
 * AUTHOR: Jernej
 * DATE: 08 2025
 * ESPIDF: framework 5.4.0
 *
 * TODO: (version 1.3)
 *  Edit "API HTTP respond messages"
 *  Edit ESP_LOGI messages
 *  Look if all variable types are as they should be (uint8_t, uint16_t, char, ...)
 *  Optimise vTaskDelay() times (Add idle task handler and look/measure its active time)
 *  Edit HTTP respond codes and messages (httpd_resp_send(), httpd_resp_send_404(), ...)
 *
 * TODO: (version 2.0)
 *  Rewrite http server event handlers (use wildcard for POST requests)
 *  Assign tasks to specific core
 *  Use Semaphores / Mutexs / Custom dirty flags if they are needed
 *  Change IP for AP mode
 *  Implement "captive portal" in AP mode (automatically open admin webpage when smartphone connects to AP)
 *  Possible upgrades: task for IR remote controller, e-fuse for default AP credentials
 *
 * NOTES:
 *  "freertos/FreeRTOSConfig.h"
 *  Preemption = 1
 *  Time Slicing = 1
 *  => Rund Robin (for tasks with the same priority)
 *
 *  WIFI runs on core 0
 *  app_main() runs on core 0
 *  other tasks and handlers are dynamicly assined to core 0 or 1
 *
 *  (GET)
 *      curl "http://10.169.50.11/api/ch?ch0_duty&ch1_duty&ch2_enable&ch5_enable&ch5_duty"
 *      curl "http://10.169.50.11/api/ch/all?ch_duty"
 *      curl "http://10.169.50.11/api/conf?rgb_outputs&w_outputs&enable_wifi"
 *
 *  (POST)
 *      curl -H "Content-Type: application/json" -X POST 10.169.50.11/api/ch -d "{\"ch4_enable\":1,\"ch0_enable\":0,\"ch3_duty\":299,\"ch5_duty\":399}"
 *      curl -H "Content-Type: application/json" -X POST 10.169.50.11/api/ch/all -d "{\"ch_enable\":1,\"ch_duty\":100}"
 *      curl -H "Content-Type: application/json" -X POST 10.169.50.11/api/conf -d "{\"w_outputs\":5,\"rgb_outputs\":0,\"enable_wifi\":0}"
 *      curl -H "Content-Type: application/json" -X POST 10.169.50.11/api/cred -d "{\"WIFI_SSID\":\"ssid\",\"WIFI_PASSWORD\":\"asswd\"}"
 *      curl -H "Content-Type: application/json" -X POST 10.169.50.11/api/reboot -d "{}"
 *
 *  CONFIG_LWIP_LOCAL_HOSTNAME "espressif" in sdkconfig.h
 *  CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_0 1 in sdkconfig.h
 *
 * LINKS:
 *  (WIFI) https://esp32tutorials.com/esp32-esp-idf-connect-wifi-station-mode-example/
 *  (LOGGING) https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/log.html
 *  (WINDOWS POST) https://stackoverflow.com/questions/11834238/curl-post-command-line-on-windows-restful-service
 */
/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/

#include "ledc_common.h"
#include "ledc_variables.h"
#include "ledc_functions.h"
#include "ledc_tasks.h"

/***************************************************************************************************************/
/*** MAIN TASK aka DEVICE BOOTUP PROCESS ***********************************************************************/

void app_main(void) // COMPLETE
{
    // printf("app_main running on core %d\n\r", xPortGetCoreID());

    /*** DISABLE LOGGING ***/
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("gpio", ESP_LOG_NONE);
    // esp_log_level_set(TAG_APP_MAIN, ESP_LOG_NONE);
    // esp_log_level_set("*", ESP_LOG_NONE);    // Disable all

    /***********************************************************************************************************/
    /*** DEVICE BOOTUP PROCESS *********************************************************************************/

    /*** GPIO INIT ***/
    if (gpio_init() != GPIO_INIT_OK)
    {
        // Critical error -> Task should be deleted
        error_handler_function(CRITICAL_ERROR_GPIO);
        ESP_LOGW(TAG_APP_MAIN, "DELETING MAIN TASK (SELF DELETE) after gpio_init");
        vTaskDelete(NULL);
    }

    // Create button_task
    xTaskCreate(button_task, "button_tsk", 4096, NULL, 5, &ButtonTaskHandle);

    /*** DEVICE FACTORY RESET ***/
    // If button pressed at boot/reboot -> go to factory reset mode
    if (gpio_get_level(GPIO_BUTTON_1) == GPIO_BUTTON_PRESSED_VALUE)
    {
        factory_reset_status.state = FACTORY_RESET_STATE_BEGIN;
        xTaskCreate(factory_reset_blink_task, "reset_blnk_tsk", 4096, NULL, 5, &FactoryResetBlinkTaskHandle);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Wait for button to be released and in the meantime change the states of factory reset process
        while (device_button_status.button_state == BUTTON_STATE_PRESSED)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (device_button_status.button_pressed_ms >= FACTORY_RESET_MIN_TIME_MS)
            {
                factory_reset_status.state = FACTORY_RESET_STATE_VALID;
                break;
            }
        }
        while (device_button_status.button_state == BUTTON_STATE_PRESSED)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (device_button_status.button_pressed_ms > (FACTORY_RESET_MIN_TIME_MS + FACTORY_RESET_WINDOW_TIME_MS))
            {
                factory_reset_status.state = FACTORY_RESET_STATE_INVALID;
                break;
            }
        }
        while (device_button_status.button_state == BUTTON_STATE_PRESSED)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Factory reset device if button press duration is within the limits
        if (factory_reset_status.state == FACTORY_RESET_STATE_VALID)
        {
            ESP_LOGI(TAG_APP_MAIN, "Starting factory reset process");
            factory_reset_status.state = FACTORY_RESET_STATE_RESETTING;

            // This delay is added to make LED flashing at least 5 sec long because
            // factory_reset_function() should take very little time if SPIFFS formating is disabled
            vTaskDelay(pdMS_TO_TICKS(5000));

            if (factory_reset_function() == FACTORY_RESET_OK)
            {
                factory_reset_status.state = FACTORY_RESET_STATE_COMPLETE;
                ESP_LOGI(TAG_APP_MAIN, "Rebooting in 3 seconds");
                vTaskDelay(pdMS_TO_TICKS(3000));
                esp_restart();
            }
            // If factory reset function failed -> go to error state and delete main task
            ESP_LOGE(TAG_APP_MAIN, "Error: factory_reset_function. CANNOT FACTORY RESET DEVICE");
            factory_reset_status.state = FACTORY_RESET_STATE_ERROR;
            ESP_LOGW(TAG_APP_MAIN, "DELETING MAIN TASK (SELF DELETE) after factory_reset_function");
            vTaskDelete(NULL);
        }
        // Skip factory reset if button press duration is NOT within the limits
        ESP_LOGI(TAG_APP_MAIN, "Factory reset process NOT started");
        factory_reset_status.state = FACTORY_RESET_STATE_ABORT;

        ESP_LOGI(TAG_APP_MAIN, "Waiting factory_reset_blink_task to delete itself");
        while (FactoryResetBlinkTaskHandle != NULL)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    // Create wifi_blink_task
    xTaskCreate(wifi_blink_task, "wifi_blnk_tsk", 4096, NULL, 5, &WifiBlinkTaskHandle);

    // Create error_blink_task
    xTaskCreate(error_blink_task, "error_blnk_tsk", 4096, NULL, 5, &ErrorBlinkTaskHandle);

    /*** NVS INIT ***/
    // Storage for WiFi data
    if (nvs_init() != NVS_INIT_OK)
    {
        error_handler_function(CRITICAL_ERROR_NVS);

        // Error -> WiFi cannot function anymore -> reset wifi_hw_init_flag "flag"
        device_wifi_status.wifi_hw_init_flag = 0;

        // Register new error for error_blink_task
        device_error_status.new_error_level = ERROR_LEVEL_WIFI;
        device_error_status.new_error_flag = 1;
    }

    /*** SPIFFS INIT ***/
    // General storage
    if (spiffs_init() != SPIFFS_INIT_OK)
    {
        error_handler_function(CRITICAL_ERROR_SPIFFS);

        // Register new error for error_blink_task
        device_error_status.new_error_level = ERROR_LEVEL_CRITICAL;
        device_error_status.new_error_flag = 1;

        // Critical error -> Task should be deleted
        ESP_LOGW(TAG_APP_MAIN, "DELETING MAIN TASK (SELF DELETE) after spiffs_init");
        vTaskDelete(NULL);
    }

    /*** JSON START READ ***/
    // read WiFi credentials and DUTY & ENABLE values for LED channels
    if (json_start_read() != JSON_START_READ_OK)
    {
        error_handler_function(CRITICAL_ERROR_JSON);

        // Register new error for error_blink_task
        device_error_status.new_error_level = ERROR_LEVEL_CRITICAL;
        device_error_status.new_error_flag = 1;

        // Critical error -> Task should be deleted
        ESP_LOGW(TAG_APP_MAIN, "DELETING MAIN TASK (SELF DELETE) after json_start_read");
        vTaskDelete(NULL);
    }

    /*** PWM INIT ***/
    if (pwm_init() != PWM_INIT_OK)
    {
        error_handler_function(CRITICAL_ERROR_PWM);

        // Register new error for error_blink_task
        device_error_status.new_error_level = ERROR_LEVEL_CRITICAL;
        device_error_status.new_error_flag = 1;

        // Critical error -> Task should be deleted
        ESP_LOGW(TAG_APP_MAIN, "DELETING MAIN TASK (SELF DELETE) after pwm_init");
        vTaskDelete(NULL);
    }

    // Create channels_control_task
    xTaskCreate(channels_control_task, "ch_cntrl_tsk", 4096, NULL, 5, &ChannelsControlTaskHandle);
    // vTaskDelay is needed for channels_control_task to start channels_duty_change_task for the first time before it may be deleted by error_handler_function
    vTaskDelay(pdMS_TO_TICKS(200));

    // Create spiffs_auto_save_channel_data_task
    xTaskCreate(spiffs_auto_save_channel_data_task, "auto_save_tsk", 4096, NULL, 5, &SpiffsAutoSaveTaskHandle);

    // If WiFi hardware initialization successful
    if (device_wifi_status.wifi_hw_init_flag)
    {
        /*** EVENT HANDLER INIT ***/
        if (event_handler_init() != EVENT_HANDLER_INIT_OK)
        {
            error_handler_function(CRITICAL_ERROR_EVENT_HANDLER);

            // Error -> WiFi cannot function anymore -> reset wifi_hw_init_flag "flag"
            device_wifi_status.wifi_hw_init_flag = 0;

            // Register new error for error_blink_task
            device_error_status.new_error_level = ERROR_LEVEL_WIFI;
            device_error_status.new_error_flag = 1;
        }
    }

    // If WiFi hardware init was successful and WiFi is enabled by user (in config.json file) and credentials are not empty -> start WiFI in STATION mode
    if (device_wifi_status.wifi_hw_init_flag && device_conf.enable_wifi && !device_wifi_status.wifi_credentials_empty_flag)
    {
        device_wifi_station_start();
    }
    // If WiFi hardware init was successful and WiFi is enabled by user (in config.json file) and credentials are empty -> start WiFI in AP mode (should happen at factory reset / first boot)
    else if (device_wifi_status.wifi_hw_init_flag && device_conf.enable_wifi && device_wifi_status.wifi_credentials_empty_flag)
    {
        device_wifi_ap_start();
    }

    // If WiFi hardware init was successful -> create device_main_task
    if (device_wifi_status.wifi_hw_init_flag)
    {
        xTaskCreate(device_main_task, "dev_main_tsk", 4096, NULL, 5, &DeviceMainTaskHandle);
    }

    // Delete app_main
    ESP_LOGI(TAG_APP_MAIN, "DELETING MAIN TASK (SELF DELETE)");
    vTaskDelete(NULL);
}

/***************************************************************************************************************/
/*** END *******************************************************************************************************/
/***************************************************************************************************************/