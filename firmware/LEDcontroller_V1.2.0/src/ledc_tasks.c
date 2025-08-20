/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_tasks.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** TASK HANDLES ***/
TaskHandle_t DeviceMainTaskHandle = NULL;
TaskHandle_t ButtonTaskHandle = NULL;
TaskHandle_t WifiBlinkTaskHandle = NULL;
TaskHandle_t ErrorBlinkTaskHandle = NULL;
TaskHandle_t FactoryResetBlinkTaskHandle = NULL;
TaskHandle_t SpiffsAutoSaveTaskHandle = NULL;
TaskHandle_t WiFiReconnectTaskHandle = NULL;
TaskHandle_t ChannelsDutyChangeTaskHandle = NULL;
TaskHandle_t ChannelsControlTaskHandle = NULL;

/***************************************************************************************************************/
/*** TASK DEFINITIONS ******************************************************************************************/

/*** DEVICE MAIN TASK ***/
void device_main_task(void *pvParameters) // COMPLETE
{
    /**
     *  This task switches mode in which WiFi is operating
     *  Switching is triggered by button press or by timer if WiFi in AP mode and no users connected
     *  Which WiFi mode will be activated depends on current WiFi mode, device configuration and buttton press duration
     *  This task is not created or deleted (if already created) if WiFi hardware init was NOT successful
     */
    ESP_LOGI(TAG_DEVICE_MAIN_TASK, "TASK STARTED: device_main_task");
    // printf("device_main_task running on core %d\n\r", xPortGetCoreID());

    // Timers for auto stopping AP mode
    uint32_t ap_mode_timer_new = 0;
    uint32_t ap_mode_timer_old = 0;
    // Holding current number of stations connected to AP
    wifi_sta_list_t sta_list;
    int num_stations = 0;

    while (1)
    {
        // Update ap_mode_timer_new
        ap_mode_timer_new = pdTICKS_TO_MS(xTaskGetTickCount());

        // Only if WiFi in AP mode and clients connected -> update ap_mode_timer_old -> prevent WiFi mode switch (= exit from AP mode)
        if (device_wifi_status.wifi_mode == DEVICE_WIFI_MODE_AP)
        {
            // Number of stations connected to AP
            esp_wifi_ap_get_sta_list(&sta_list);
            num_stations = sta_list.num;
            // ESP_LOGI(TAG_APP_MAIN, "Number of stations connected: %d", num_stations);

            if (num_stations != 0)
            {
                ap_mode_timer_old = ap_mode_timer_new;
            }
        }
        // If not in AP mode -> update ap_mode_timer_old -> prevent WiFi mode switch
        else
        {
            ap_mode_timer_old = ap_mode_timer_new;
        }

        // Auto exit AP mode if inactivity detected
        if ((device_wifi_status.wifi_mode == DEVICE_WIFI_MODE_AP) && ((ap_mode_timer_new - ap_mode_timer_old) >= WIFI_AP_MODE_MAX_INACTIVITY_TIME_MS))
        {
            if (device_conf.enable_wifi && !device_wifi_status.wifi_credentials_empty_flag)
            {
                device_wifi_station_start();
            }
            else
            {
                device_wifi_stop();
            }
        }

        // Depending on button press duration -> change WiFi mode
        if (device_button_status.button_state == BUTTON_STATE_PRESSED)
        {
            // While button pressed -> loop here
            while (device_button_status.button_state == BUTTON_STATE_PRESSED)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
                if (device_button_status.button_pressed_ms > WIFI_MODE_SWITCH_AP_MIN_TIME_MS)
                {
                    break;
                }
            }

            // Enter or exit AP mode
            if (device_button_status.button_pressed_ms >= WIFI_MODE_SWITCH_AP_MIN_TIME_MS)
            {
                if (device_wifi_status.wifi_mode == DEVICE_WIFI_MODE_AP)
                {
                    if (device_conf.enable_wifi && !device_wifi_status.wifi_credentials_empty_flag)
                    {
                        device_wifi_station_start();
                    }
                    else
                    {
                        device_wifi_stop();
                    }
                }
                else
                {
                    device_wifi_ap_start();
                }
            }
            // Switch STATION and NONE mode
            else if ((device_button_status.button_pressed_ms >= WIFI_MODE_SWITCH_MIN_TIME_MS) && (device_wifi_status.wifi_mode != DEVICE_WIFI_MODE_AP))
            {
                if (device_wifi_status.wifi_mode == DEVICE_WIFI_MODE_STATION)
                {
                    device_wifi_stop();
                }
                else if (!device_wifi_status.wifi_credentials_empty_flag)
                {
                    device_wifi_station_start();
                }
            }

            // While button pressed -> loop here
            while (device_button_status.button_state == BUTTON_STATE_PRESSED)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/*** BUTTON TASK ***/
void button_task(void *pvParameters) // COMPLETE
{
    /**
     *  If button press is detected, function writes press duration to global variable "device_conf.button_pressed_ms"
     *  and current button state to global variable "device_conf.button_state"
     *  Current button state can be: BUTTON_STATE_PRESSED or BUTTON_STATE_RELEASED
     */
    ESP_LOGI(TAG_BUTTON_TASK, "TASK STARTED: button_task");
    // printf("error_blink_task running on core %d\n\r", xPortGetCoreID());

    uint8_t button_local_state = 0;
    uint16_t counter = 0;

    while (1)
    {
        button_local_state = get_button_state(GPIO_BUTTON_1);
        counter = 0;
        // If posedge detected -> start counter
        if (button_local_state == BUTTON_STATE_POSEDGE)
        {
            device_button_status.button_pressed_ms = 0;
            device_button_status.button_state = BUTTON_STATE_PRESSED;
            counter++;
            vTaskDelay(pdMS_TO_TICKS(100));
            while (get_button_state(GPIO_BUTTON_1) == BUTTON_STATE_PRESSED)
            {
                if (counter < 600)
                {
                    counter++;
                    // Max value = 60000ms (60 sec), Resolution = 100ms
                    device_button_status.button_pressed_ms = counter * 100;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            device_button_status.button_state = BUTTON_STATE_RELEASED;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/*** WIFI BLINK TASK ***/
void wifi_blink_task(void *pvParameters) // COMPLETE
{
    /**
     *  This task controls GPIO_LED_WIFI
     *  Blinking mode is determined by DEVICE_WIFI_MODE_xxxx
     *  Mode exits when "device_wifi_status.wifi_mode_ch_flag == 1"
     *  Modes support WiFi activity blinking. Number of blinks is determined by "device_wifi_status.wifi_activity_cnt"
     */

    ESP_LOGI(TAG_WIFI_BLINK_TASK, "TASK STARTED: wifi_blink_task");
    // printf("wifi_blink_task running on core %d\n\r", xPortGetCoreID());

    while (1)
    {
        device_wifi_status.wifi_mode_ch_flag = 0;
        device_wifi_status.wifi_activity_cnt = 0;
        switch (device_wifi_status.wifi_mode)
        {
        case DEVICE_WIFI_MODE_AP:
        {
            // Exit current mode if mode change flag == 1
            while (device_wifi_status.wifi_mode_ch_flag == 0)
            {
                // LED blinking
                gpio_set_level(GPIO_LED_WIFI, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(GPIO_LED_WIFI, 1);
                vTaskDelay(pdMS_TO_TICKS(500));

                // Blinks on WiFi activity
                while (device_wifi_status.wifi_activity_cnt)
                {
                    // Limit to max 7 consecutive blinks
                    device_wifi_status.wifi_activity_cnt &= 0x0007;
                    gpio_set_level(GPIO_LED_WIFI, 0);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    gpio_set_level(GPIO_LED_WIFI, 1);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    device_wifi_status.wifi_activity_cnt--;
                }
            }
            break;
        }
        case DEVICE_WIFI_MODE_STATION:
        {
            // Exit current mode if mode change flag == 1
            while (device_wifi_status.wifi_mode_ch_flag == 0)
            {
                // Device connected to AP
                if (device_wifi_status.wifi_sta_connected_flag)
                {
                    // LED always ON
                    gpio_set_level(GPIO_LED_WIFI, 1);
                    vTaskDelay(pdMS_TO_TICKS(500));

                    // Blinks on WiFi activity
                    while (device_wifi_status.wifi_activity_cnt)
                    {
                        // Limit to max 7 consecutive blinks
                        device_wifi_status.wifi_activity_cnt &= 0x0007;
                        gpio_set_level(GPIO_LED_WIFI, 0);
                        vTaskDelay(pdMS_TO_TICKS(50));
                        gpio_set_level(GPIO_LED_WIFI, 1);
                        vTaskDelay(pdMS_TO_TICKS(50));
                        device_wifi_status.wifi_activity_cnt--;
                    }
                }
                // Device NOT connected to AP
                else
                {
                    // 1 short LED blink every 5 sec
                    gpio_set_level(GPIO_LED_WIFI, 0);
                    for (uint8_t i = 0; i < 10; i++)
                    {
                        vTaskDelay(pdMS_TO_TICKS(500));
                        // If mode change flag is set or station connests to AP -> break
                        if (device_wifi_status.wifi_mode_ch_flag || device_wifi_status.wifi_sta_connected_flag)
                        {
                            break;
                        }
                    }
                    gpio_set_level(GPIO_LED_WIFI, 1);
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
            break;
        }
        case DEVICE_WIFI_MODE_NONE:
        default: // No blink mode selected
        {
            // ESP_LOGI(TAG_WIFI_BLINK_TASK, "No blink mode selected");
            gpio_set_level(GPIO_LED_WIFI, 0);
            // Exit current mode if mode change flag == 1
            while (device_wifi_status.wifi_mode_ch_flag == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;
        }
        }

        // Blinks on mode change
        for (uint8_t i = 0; i < 3; i++)
        {
            gpio_set_level(GPIO_LED_WIFI, 0);
            vTaskDelay(pdMS_TO_TICKS(150));
            gpio_set_level(GPIO_LED_WIFI, 1);
            vTaskDelay(pdMS_TO_TICKS(150));
        }
    }
}

/*** ERROR BLINK TASK ***/
void error_blink_task(void *pvParameters) // COMPLETE
{
    /**
     *  This task controls GPIO_LED_ERROR
     *  Blinking mode is determined by ERROR_LEVEL_xxxx
     *  New mode is applied only if it has higher level (is more critical) that current mode
     */

    ESP_LOGI(TAG_ERROR_BLINK_TASK, "TASK STARTED: error_blink_task");
    // printf("error_blink_task running on core %d\n\r", xPortGetCoreID());

    uint16_t current_mode_level = ERROR_LEVEL_NONE;
    while (1)
    {
        // Reset new_error_flag
        device_error_status.new_error_flag = 0;

        // If new_error_level higher than current_mode_level -> update current_mode_level
        if (device_error_status.new_error_level > current_mode_level)
        {
            current_mode_level = device_error_status.new_error_level;
        }

        // Go to appropriate blinking mode
        switch (current_mode_level)
        {
        case ERROR_LEVEL_WARNING:
        {
            // Exit current mode if new_error_flag == 1
            while (device_error_status.new_error_flag == 0)
            {
                for (uint8_t i = 0; i < 3; i++)
                {
                    gpio_set_level(GPIO_LED_ERROR, 1);
                    vTaskDelay(pdMS_TO_TICKS(250));
                    gpio_set_level(GPIO_LED_ERROR, 0);
                    vTaskDelay(pdMS_TO_TICKS(250));
                }
                vTaskDelay(pdMS_TO_TICKS(3000));
            }
            break;
        }
        case ERROR_LEVEL_WIFI:
        {
            // Exit current mode if new_error_flag == 1
            while (device_error_status.new_error_flag == 0)
            {
                gpio_set_level(GPIO_LED_ERROR, 1);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(GPIO_LED_ERROR, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
            }
            break;
        }
        case ERROR_LEVEL_CRITICAL:
        {
            gpio_set_level(GPIO_LED_ERROR, 1);
            // Exit current mode if new_error_flag == 1
            while (device_error_status.new_error_flag == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;
        }
        case ERROR_LEVEL_NONE:
        default:
        {
            gpio_set_level(GPIO_LED_ERROR, 0);
            // Exit current mode if new_error_flag == 1
            while (device_error_status.new_error_flag == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;
        }
        }
    }
}

/*** FACTORY RESET BLINK TASK ***/
void factory_reset_blink_task(void *pvParameters) // COMPLETE
{
    /**
     *  In case of FACTOR RESET this task controls GPIO_LED_ERROR and GPIO_LED_WIFI
     */

    ESP_LOGI(TAG_FACTORY_RESET_BLINK_TASK, "TASK STARTED: factory_reset_blink_task");
    // printf("factory_reset_blink_task running on core %d\n\r", xPortGetCoreID());

    uint16_t currnet_state = FACTORY_RESET_STATE_BEGIN;

    while (1)
    {
        switch (currnet_state)
        {
        case FACTORY_RESET_STATE_BEGIN:
        case FACTORY_RESET_STATE_INVALID:
        {
            gpio_set_level(GPIO_LED_ERROR, 1);
            gpio_set_level(GPIO_LED_WIFI, 1);
            while (currnet_state == factory_reset_status.state)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            currnet_state = factory_reset_status.state;
            break;
        }
        case FACTORY_RESET_STATE_VALID:
        {
            while (currnet_state == factory_reset_status.state)
            {
                gpio_set_level(GPIO_LED_ERROR, 0);
                gpio_set_level(GPIO_LED_WIFI, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(GPIO_LED_ERROR, 1);
                gpio_set_level(GPIO_LED_WIFI, 1);
                vTaskDelay(pdMS_TO_TICKS(250));
            }
            currnet_state = factory_reset_status.state;
            break;
        }
        case FACTORY_RESET_STATE_RESETTING:
        {
            while (currnet_state == factory_reset_status.state)
            {
                gpio_set_level(GPIO_LED_ERROR, 0);
                gpio_set_level(GPIO_LED_WIFI, 1);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(GPIO_LED_ERROR, 1);
                gpio_set_level(GPIO_LED_WIFI, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
            }
            currnet_state = factory_reset_status.state;
            break;
        }
        case FACTORY_RESET_STATE_ERROR:
        {
            gpio_set_level(GPIO_LED_ERROR, 1);
            gpio_set_level(GPIO_LED_WIFI, 0);
            ESP_LOGI(TAG_FACTORY_RESET_BLINK_TASK, "DELETING TASK (SELF DELETE): factory_reset_blink_task");
            FactoryResetBlinkTaskHandle = NULL;
            vTaskDelete(NULL);
            break;
        }
        case FACTORY_RESET_STATE_ABORT:
        {
            gpio_set_level(GPIO_LED_WIFI, 0);
            for (uint8_t i = 0; i < 5; i++)
            {
                gpio_set_level(GPIO_LED_ERROR, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_set_level(GPIO_LED_ERROR, 1);
                vTaskDelay(pdMS_TO_TICKS(250));
            }
            gpio_set_level(GPIO_LED_ERROR, 0);

            ESP_LOGI(TAG_FACTORY_RESET_BLINK_TASK, "DELETING TASK (SELF DELETE): factory_reset_blink_task");
            FactoryResetBlinkTaskHandle = NULL;
            vTaskDelete(NULL);
            break;
        }
        case FACTORY_RESET_STATE_COMPLETE:
        {
            gpio_set_level(GPIO_LED_ERROR, 0);
            gpio_set_level(GPIO_LED_WIFI, 0);
            ESP_LOGI(TAG_FACTORY_RESET_BLINK_TASK, "DELETING TASK (SELF DELETE): factory_reset_blink_task");
            FactoryResetBlinkTaskHandle = NULL;
            vTaskDelete(NULL);
            break;
        }
        case FACTOR_RESET_STATE_NONE:
        default:
        {
            gpio_set_level(GPIO_LED_ERROR, 0);
            gpio_set_level(GPIO_LED_WIFI, 0);
            while (currnet_state == factory_reset_status.state)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            currnet_state = factory_reset_status.state;
            break;
        }
        }
    }
}

/*** SPIFFS AUTO SAVE TASK ***/
void spiffs_auto_save_channel_data_task(void *pvParameters) // COMPLETE
{
    /**
     *  Task for auto saving current PWM channels values
     *  Values are saved to JSON_CONFIG_FILE_CH_DATA_PATH in SPIFFS, if they have changed
     *  Minimum saving period is determined by SPIFF_AUTO_SAVE_PERIOD_MS
     */

    ESP_LOGI(TAG_SPIFFS_AUTO_SAVE_TASK, "TASK STARTED: spiffs_auto_save_channel_data_task");
    // printf("spiffs_auto_save_channel_data_task running on core %d\n\r", xPortGetCoreID());

    // Stores latest stored values for each channel in SPIFFS
    struct channel_config_struct ch0_old, ch1_old, ch2_old, ch3_old, ch4_old, ch5_old;
    uint16_t *const channel_value_duty_old[] = {&ch0_old.duty, &ch1_old.duty, &ch2_old.duty, &ch3_old.duty, &ch4_old.duty, &ch5_old.duty};
    uint16_t *const channel_value_enable_old[] = {&ch0_old.enable, &ch1_old.enable, &ch2_old.enable, &ch3_old.enable, &ch4_old.enable, &ch5_old.enable};

    // First update old values
    for (uint8_t ch_n = 0; (ch_n < LED_CH_N_size) && (ch_n < device_conf.channels_used); ch_n++)
    {
        *channel_value_duty_old[ch_n] = *channel_value_duty[ch_n];     // Update old DUTY
        *channel_value_enable_old[ch_n] = *channel_value_enable[ch_n]; // Update old ENABLE
    }

    // Create data_buffer for storing configuration in JSON format
    char data_buffer[JSON_CONFIG_FILE_MAX_LEN + 61] = "0";

    // change_detected flag
    uint8_t change_detected = 0;

    vTaskDelay(pdMS_TO_TICKS(SPIFF_AUTO_SAVE_PERIOD_MS));
    while (1)
    {
        // Check for parameter change (LIMITED TO: device_conf.channels_used)
        for (uint8_t ch_n = 0; (ch_n < LED_CH_N_size) && (ch_n < device_conf.channels_used); ch_n++)
        {
            if (*channel_value_duty_old[ch_n] != *channel_value_duty[ch_n])
            {
                change_detected = 1;
                break;
            }
            if (*channel_value_enable_old[ch_n] != *channel_value_enable[ch_n])
            {
                change_detected = 1;
                break;
            }
        }

        // Auto save only if parameters changed
        if (change_detected)
        {
            // Reset change_detected flag
            change_detected = 0;
            ESP_LOGI(TAG_SPIFFS_AUTO_SAVE_TASK, "Auto saving channel values");

            // Write values to data_buffer
            data_buffer[0] = '\0';
            uint16_t cnt = sprintf(data_buffer, "{ ");

            // Here all values are stored and are not limited by device_conf.channels_used to preserve config file format
            for (uint8_t i = 0; i < channel_data_name_duty_size; i++)
            {
                cnt += sprintf(data_buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[i], *channel_value_duty[i]);
                cnt += sprintf(data_buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[i], *channel_value_enable[i]);

                // Buffer overflow protection -> break;
                if (cnt > JSON_CONFIG_FILE_MAX_LEN)
                {
                    ESP_LOGE(TAG_SPIFFS_AUTO_SAVE_TASK, "Auto saving error");
                    ESP_LOGE(TAG_SPIFFS_AUTO_SAVE_TASK, "Buffer overflow protection: spiffs_auto_save_channel_data_task (%d chars, but %d is upper limit)", cnt, JSON_CONFIG_FILE_MAX_LEN);

                    // Register new error for error_blink_task
                    device_error_status.new_error_level = ERROR_LEVEL_WARNING;
                    device_error_status.new_error_flag = 1;
                    break;
                }
            }
            cnt += sprintf(data_buffer + cnt - 3, " }");
            // printf(">>>%s<<<\n\r", data_buffer);

            // Write data_buffer to channel_data.json
            if (cnt <= JSON_CONFIG_FILE_MAX_LEN)
            {
                ESP_LOGI(TAG_SPIFFS_AUTO_SAVE_TASK, "Writing data_buffer to file: %s", JSON_CONFIG_FILE_CH_DATA_PATH);
                switch (write_buffer_to_spiffs_file(JSON_CONFIG_FILE_CH_DATA_PATH, data_buffer))
                {
                case WRITE_SPIFFS_OK:
                {
                    break;
                }
                default:
                {
                    ESP_LOGE(TAG_SPIFFS_AUTO_SAVE_TASK, "Error writing file to SPIFFS");

                    // Register new error for error_blink_task
                    device_error_status.new_error_level = ERROR_LEVEL_WARNING;
                    device_error_status.new_error_flag = 1;
                }
                }
            }

            // Update old values even if writing to file failed to prevent endless retrying (LIMITED TO: device_conf.channels_used)
            for (uint8_t ch_n = 0; (ch_n < LED_CH_N_size) && (ch_n < device_conf.channels_used); ch_n++)
            {
                *channel_value_duty_old[ch_n] = *channel_value_duty[ch_n];     // Update old DUTY
                *channel_value_enable_old[ch_n] = *channel_value_enable[ch_n]; // Update old ENABLE
            }
        }
        else // Skip if non of the parameters has changed
        {
            ESP_LOGI(TAG_SPIFFS_AUTO_SAVE_TASK, "Skipping auto saving");
        }

        vTaskDelay(pdMS_TO_TICKS(SPIFF_AUTO_SAVE_PERIOD_MS));
    }
}

/*** WIFI RECONNECT TASK ***/
void wifi_reconnect_task(void *pvParameters) // COMPLETE
{
    /**
     *  Task that reconnects to the WiFi network if connection was lost
     */

    ESP_LOGI(TAG_WIFI_RECONNECT_TASK, "TASK STARTED: wifi_reconnect_task");
    // printf("wifi_reconnect_task running on core %d\n\r", xPortGetCoreID());

    uint8_t wifi_ap_retry_counter = 1;
    while (1)
    {
        if (wifi_ap_retry_counter <= 6)
        {
            vTaskDelay(pdMS_TO_TICKS(20000)); // 20 sec
        }
        else
        {
            // Stop HTTP server if active
            if (http_server_handle != NULL)
            {
                ESP_LOGI(TAG_WIFI_RECONNECT_TASK, "Stopping HTTP server");
                httpd_stop(http_server_handle);
                http_server_handle = NULL;
            }
            vTaskDelay(pdMS_TO_TICKS(60000)); // 1 min
        }

        if (wifi_ap_retry_counter < 99)
        {
            ESP_LOGI(TAG_WIFI_RECONNECT_TASK, "AP reconnecting attempt %d", wifi_ap_retry_counter);
            wifi_ap_retry_counter++;
        }
        else
        {
            ESP_LOGI(TAG_WIFI_RECONNECT_TASK, "AP reconnecting attempt 100+");
        }

        // Trying to reconnect to AP
        for (uint8_t i = 0; i < WIFI_RECONNECTING_ATTEMPTS; i++)
        {
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_wifi_connect();
        }
    }
}

/*** CHANNELS CONTROL & DUTY CHANGE TASK ***/
void channels_duty_change_task(void *pvParameters) // COMPLETE
{
    /**
     *  Task for controlling all PWM channels
     *  This task smoothly change duty cycle of all channels that are in "queue" (channel_change_list) to desired value
     *  Time to transition form starting duty cycle to desired value is determined by PWM_FADE_TIME_MS
     *  Number of steps in transition is determined by PWM_TRANSITION_STEPS
     *  Number of valid channels in "queue" is determined by "pvParameters"
     */

    ESP_LOGI(TAG_CHANNELS_TASK, "TASK STARTED: channels_duty_change_task");
    // printf("channels_duty_change_task running on core %d\n\r", xPortGetCoreID());

    // Number of channels in "queue"
    const uint8_t cnt = *(uint8_t *)pvParameters;

    // Calculate gradient
    int16_t delta[NO_CHANNELS_LIMIT + 1];
    float slope[NO_CHANNELS_LIMIT + 1];
    for (uint8_t i = 0; i < cnt; i++)
    {
        channel_change_list.duty_current[i] = (uint16_t)ledc_get_duty(LEDC_LOW_SPEED_MODE, channel_change_list.LED_CH_N[i]);
        delta[i] = (int16_t)((int16_t)channel_change_list.duty_new[i] - (int16_t)channel_change_list.duty_current[i]);
        slope[i] = (float)((float)delta[i] / (float)PWM_TRANSITION_STEPS);
    }

    // Begin duty change
    for (uint16_t fade_steps_cnt = 1; fade_steps_cnt <= PWM_TRANSITION_STEPS; fade_steps_cnt++)
    {
        for (uint8_t i = 0; i < cnt; i++)
        {
            uint16_t value = (uint16_t)((int16_t)channel_change_list.duty_current[i] + (int16_t)(slope[i] * (float)fade_steps_cnt));
            // printf("\n\r%d   %d", channel_change_list.LED_CH_N[i], value);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, channel_change_list.LED_CH_N[i], value);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, channel_change_list.LED_CH_N[i]);
        }
        vTaskDelay(pdMS_TO_TICKS(PWM_FADE_TIME_MS / PWM_TRANSITION_STEPS));
    }

    // Duty changing done -> task self delete
    ESP_LOGI(TAG_CHANNELS_TASK, "DELETING TASK (SELF DELETE): channels_duty_change_task");
    ChannelsDutyChangeTaskHandle = NULL;
    vTaskDelete(NULL);
}

void channels_control_task(void *pvParameters) // COMPLETE
{
    /**
     *  Task for controlling all PWM channels
     *  If at least on channel parameter has changed this task add all channels that are not at their desired value to "queue" (channel_change_list)
     *  and creates channels_duty_change_task (this task smoothly change channels duty cycle to desired value)
     */

    ESP_LOGI(TAG_CHANNELS_TASK, "TASK STARTED: channels_control_task");
    // printf("channels_control_task running on core %d\n\r", xPortGetCoreID());

    // Store current (active, not desired) values for each channel
    struct channel_config_struct ch0_old, ch1_old, ch2_old, ch3_old, ch4_old, ch5_old;
    uint16_t *const channel_value_duty_old[] = {&ch0_old.duty, &ch1_old.duty, &ch2_old.duty, &ch3_old.duty, &ch4_old.duty, &ch5_old.duty};
    uint16_t *const channel_value_enable_old[] = {&ch0_old.enable, &ch1_old.enable, &ch2_old.enable, &ch3_old.enable, &ch4_old.enable, &ch5_old.enable};

    while (1)
    {
        uint8_t queue_counter = 0;
        // Loop over all API parameters (LIMITED TO: device_conf.channels_used)
        for (uint8_t i = 0; (i < channel_value_duty_size) && (i < device_conf.channels_used); i++)
        {
            // If any parameter changed -> check state of all channels state -> if current duty != required duty -> add to "queue"
            if ((*channel_value_duty_old[i] != *channel_value_duty[i]) || (*channel_value_enable_old[i] != *channel_value_enable[i]))
            {
                // Loop all channels and get current state -> if current duty != required duty -> add to "queue" (LIMITED TO: device_conf.channels_used)
                for (uint8_t ch_n = 0; (ch_n < LED_CH_N_size) && (ch_n < device_conf.channels_used); ch_n++)
                {
                    uint16_t current_duty_value = (uint16_t)ledc_get_duty(LEDC_LOW_SPEED_MODE, LED_CH_N[ch_n]);
                    uint16_t mask_new = 0x0000;
                    mask_new = (*channel_value_enable[ch_n]) ? 0xFFFF : 0x0000;

                    // If current duty != required duty -> add to "queue"
                    if (current_duty_value == (uint16_t)LEDC_ERR_DUTY)
                    {
                        ESP_LOGE(TAG_CHANNELS_TASK, "Error reading duty value on channel: ch%d", ch_n);

                        // Register new error for error_blink_task
                        device_error_status.new_error_level = ERROR_LEVEL_WARNING;
                        device_error_status.new_error_flag = 1;
                    }
                    else if ((*channel_value_duty[ch_n] & mask_new) != current_duty_value)
                    {
                        // Add to "queue"
                        channel_change_list.LED_CH_N[queue_counter] = LED_CH_N[ch_n];
                        channel_change_list.duty_new[queue_counter] = (*channel_value_duty[ch_n]) & mask_new;
                        queue_counter++;
                    }
                    // Update old values
                    *channel_value_duty_old[ch_n] = *channel_value_duty[ch_n];     // Update old DUTY
                    *channel_value_enable_old[ch_n] = *channel_value_enable[ch_n]; // Update old ENABLE
                }
            }
        }
        // Call channels_duty_change_task to fade channels in "queue"
        if (queue_counter)
        {
            // If channels_duty_change_task running, it must be first deleted
            if (ChannelsDutyChangeTaskHandle != NULL)
            {
                ESP_LOGI(TAG_CHANNELS_TASK, "DELETING TASK: channels_duty_change_task");
                vTaskDelete(ChannelsDutyChangeTaskHandle);
                ChannelsDutyChangeTaskHandle = NULL;
            }
            ESP_LOGI(TAG_CHANNELS_TASK, "CREATING TASK: channels_duty_change_task");
            xTaskCreate(channels_duty_change_task, "chng_duty_tsk", 4096, &queue_counter, 5, &ChannelsDutyChangeTaskHandle);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

/***************************************************************************************************************/
/*** END *******************************************************************************************************/
/***************************************************************************************************************/