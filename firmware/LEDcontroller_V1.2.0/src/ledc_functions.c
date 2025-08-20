/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_functions.h"

/***************************************************************************************************************/
/*** FUNCTION DEFINITIONS **************************************************************************************/

/*** EVENT HANDLER ***/
uint8_t event_handler_init(void) // COMPLETE
{
    ESP_LOGI(TAG_EVENT_HANDLER_INIT, "Starting event_handler_init");

    esp_err_t ret = ESP_OK;

    // Create default event loop
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_EVENT_HANDLER_INIT, "Error: esp_event_loop_create_default");
        return EVENT_HANDLER_INIT_ERROR;
    }

    // Register an instance of event handler to the default loop
    // ARGUMENTS: event_base, event_id, event_handler, event_handler_arg, instance
    ret |= esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &handler_instance_wifi);
    ret |= esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &handler_instance_ip);
    ret |= esp_event_handler_instance_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &handler_instance_http_server);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_EVENT_HANDLER_INIT, "Error: esp_event_handler_instance_register");
        return EVENT_HANDLER_INIT_ERROR;
    }

    return EVENT_HANDLER_INIT_OK;
}

/*** WIFI ***/
uint8_t wifi_station_init(void) // COMPLETE
{
    ESP_LOGI(TAG_WIFI_STATION_INIT, "In function: wifi_station_init");

    esp_err_t ret = ESP_OK;

    // Stop WiFi and remove netif underlying TCP/IP stack if already exists (to support reentrancy)
    device_wifi_stop();
    esp_netif_deinit();

    // Initialize the underlying TCP/IP stack
    ret = esp_netif_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_STATION_INIT, "Error: esp_netif_init");
        return WIFI_STATION_INIT_ERROR;
    }

    // Create default WiFi Station
    static uint8_t reentrancy = 0;
    if (reentrancy == 0)
    {
        esp_netif_create_default_wifi_sta();
        reentrancy = 1;
    }

    // ESP WiFi INIT with default settings
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_STATION_INIT, "Error: esp_wifi_init");
        return WIFI_STATION_INIT_ERROR;
    }

    // WiFi configuration (only for station mode)
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "0",
            .password = "0",
            .threshold.authmode = WIFI_AUTHMODE_TRESHOLD,
            .failure_retry_cnt = 2,
        },
    };
    memcpy(wifi_config.sta.ssid, wifi_ssid, strlen((const char *)wifi_ssid) + 1);
    memcpy(wifi_config.sta.password, wifi_password, strlen((const char *)wifi_password) + 1);

    // Set WiFi operation mode to station mode
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_STATION_INIT, "Error: esp_wifi_init");
        return WIFI_STATION_INIT_ERROR;
    }

    // Set WiFI configuration to station mode
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_STATION_INIT, "Error: esp_wifi_init");
        return WIFI_STATION_INIT_ERROR;
    }

    // Start WiFi according to configuration
    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_STATION_INIT, "Error: esp_wifi_init");
        return WIFI_STATION_INIT_ERROR;
    }

    return WIFI_STATION_INIT_OK;
}

uint8_t wifi_ap_init(void) // COMPLETE
{
    ESP_LOGI(TAG_WIFI_AP_INIT, "In function: wifi_ap_init");

    esp_err_t ret = ESP_OK;

    // Stop WiFi and remove netif underlying TCP/IP stack if already exists (to support reentrancy)
    device_wifi_stop();
    esp_netif_deinit();

    // Initialize the underlying TCP/IP stack
    ret = esp_netif_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_AP_INIT, "Error: esp_netif_init");
        return WIFI_AP_INIT_ERROR;
    }

    // Create default WiFi AP
    static uint8_t reentrancy = 0;
    if (reentrancy == 0)
    {
        esp_netif_create_default_wifi_ap();
        reentrancy = 1;
    }

    // ESP WiFi INIT with default settings
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_AP_INIT, "Error: esp_wifi_init");
        return WIFI_AP_INIT_ERROR;
    }

    // WiFi configuration (only for station mode)
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "0",
            .password = "0",
            .channel = 1,
            .authmode = WIFI_AUTHMODE_TRESHOLD,
            .ssid_hidden = 0,
            .max_connection = 3,
        },
    };
    memcpy(wifi_config.ap.ssid, wifi_ap_ssid, strlen((char *)wifi_ap_ssid) + 1);
    memcpy(wifi_config.ap.password, wifi_ap_password, strlen((char *)wifi_ap_password) + 1);

    // Set WiFi operation mode to station mode
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_AP_INIT, "Error: esp_wifi_init");
        return WIFI_AP_INIT_ERROR;
    }

    // Set WiFI configuration to station mode
    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_AP_INIT, "Error: esp_wifi_init");
        return WIFI_AP_INIT_ERROR;
    }

    // Start WiFi according to configuration
    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_WIFI_AP_INIT, "Error: esp_wifi_init");
        return WIFI_AP_INIT_ERROR;
    }

    return WIFI_AP_INIT_OK;
}

void device_wifi_station_start(void) // COMPLETE
{
    /*** WIFI STATION INIT ***/
    if (wifi_station_init() != WIFI_STATION_INIT_OK)
    {
        error_handler_function(CRITICAL_ERROR_WIFI);

        // Error -> WiFi cannot function anymore -> reset wifi_hw_init_flag "flag"
        device_wifi_status.wifi_hw_init_flag = 0;

        // Register new error for error_blink_task
        device_error_status.new_error_level = ERROR_LEVEL_WIFI;
        device_error_status.new_error_flag = 1;
    }
}

void device_wifi_ap_start(void) // COMPLETE
{
    /*** WIFI STATION INIT ***/
    if (wifi_ap_init() != WIFI_AP_INIT_OK)
    {
        error_handler_function(CRITICAL_ERROR_WIFI);

        // Error -> WiFi cannot function anymore -> reset wifi_hw_init_flag "flag"
        device_wifi_status.wifi_hw_init_flag = 0;

        // Register new error for error_blink_task
        device_error_status.new_error_level = ERROR_LEVEL_WIFI;
        device_error_status.new_error_flag = 1;
    }
}

void device_wifi_stop(void) // COMPLETE
{
    ESP_LOGI(TAG_DEVICE_WIFI_STOP, "In function: device_wifi_stop");

    // Stop WiFi and free all allocated resources
    esp_wifi_stop();
    esp_wifi_deinit();
}

/*** HTTP SERVER ***/
uint8_t start_http_server(void) // COMPLETE
{
    // printf("start_http_server running on core %d\n\r", xPortGetCoreID());

    // If HTTP server active -> stop server
    if (http_server_handle != NULL)
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Stopping HTTP server");
        httpd_stop(http_server_handle);
        http_server_handle = NULL;
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Start HTTP server with default config on port HTTP_SERVER_PORT
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_SERVER_PORT;
    config.max_uri_handlers = HTTP_SERVER_MAX_URI_HANDLERS;
    config.uri_match_fn = httpd_uri_match_wildcard;

    // HTTP server started successfully
    if (httpd_start(&http_server_handle, &config) == ESP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_err_t ret = ESP_OK;
        // Register OPTIONS handlers
        ret |= httpd_register_uri_handler(http_server_handle, &options_api_ch);
        ret |= httpd_register_uri_handler(http_server_handle, &options_api_ch_all);
        ret |= httpd_register_uri_handler(http_server_handle, &options_api_conf);
        ret |= httpd_register_uri_handler(http_server_handle, &options_api_cred);
        ret |= httpd_register_uri_handler(http_server_handle, &options_api_reboot);
        // Register API POST handlers
        ret |= httpd_register_uri_handler(http_server_handle, &post_api_ch);
        ret |= httpd_register_uri_handler(http_server_handle, &post_api_ch_all);
        ret |= httpd_register_uri_handler(http_server_handle, &post_api_conf);
        ret |= httpd_register_uri_handler(http_server_handle, &post_api_cred);
        ret |= httpd_register_uri_handler(http_server_handle, &post_api_reboot);
        // Register API GET handlers
        ret |= httpd_register_uri_handler(http_server_handle, &get_api_ch);
        ret |= httpd_register_uri_handler(http_server_handle, &get_api_ch_all);
        ret |= httpd_register_uri_handler(http_server_handle, &get_api_conf);
        // Register WEB handlers
        ret |= httpd_register_uri_handler(http_server_handle, &get_web);

        // Check if all handlers registered successfully
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG_HTTP_SERVER, "Error(s) / Warning(s) in httpd_register_uri_handler function(s)");
        }
    }
    // HTTP server start error
    else
    {
        ESP_LOGE(TAG_HTTP_SERVER, "Error starting HTTP server");
        http_server_handle = NULL;
        return START_HTTP_SERVER_ERROR;
    }
    return START_HTTP_SERVER_OK;
}

/*** GPIO & PERIPHERALS ***/
uint8_t gpio_init(void) // COMPLETE
{
    esp_err_t ret = ESP_OK;

    // Init GPIO_LED_WIFI
    ret |= gpio_reset_pin(GPIO_LED_WIFI);
    ret |= gpio_set_direction(GPIO_LED_WIFI, GPIO_MODE_OUTPUT);
    ret |= gpio_set_level(GPIO_LED_WIFI, 0);

    // Init GPIO_LED_ERROR
    ret |= gpio_reset_pin(GPIO_LED_ERROR);
    ret |= gpio_set_direction(GPIO_LED_ERROR, GPIO_MODE_OUTPUT);
    ret |= gpio_set_level(GPIO_LED_ERROR, 0);

    // Init GPIO_BUTTON_1
    ret |= gpio_reset_pin(GPIO_BUTTON_1);
    ret |= gpio_set_direction(GPIO_BUTTON_1, GPIO_MODE_INPUT);
    ret |= gpio_set_pull_mode(GPIO_BUTTON_1, GPIO_PULLUP_ONLY);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_GPIO_INIT, "Error in gpio_init function");
        return GPIO_INIT_ERROR;
    }
    return GPIO_INIT_OK;
}

uint8_t pwm_init(void) // COMPLETE
{
    esp_err_t ret = ESP_OK;

    // Init LED pins for all channels and set to LOW (NOT LIMITED TO: device_conf.channels_used)
    for (uint8_t i = 0; i < LED_PIN_CH_N_size; i++)
    {
        ret |= gpio_reset_pin(LED_PIN_CH_N[i]);
        ret |= gpio_set_direction(LED_PIN_CH_N[i], GPIO_MODE_OUTPUT);
        ret |= gpio_set_level(LED_PIN_CH_N[i], 0);
    }

    // Initialize the LEDC peripheral
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_PWM_INIT, "Error: ledc_timer_config");
        return PWM_INIT_ERROR;
    }

    // Initialize LEDC channels (LIMITED TO: device_conf.channels_used)
    ledc_channel_config_t ledc_config_CH_N[6];
    uint8_t i = 0;
    for (i = 0; (i < LED_PIN_CH_N_size) && (i < device_conf.channels_used); i++)
    {
        ledc_config_CH_N[i] = (ledc_channel_config_t){
            .gpio_num = LED_PIN_CH_N[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LED_CH_N[i],
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0};
        ret = ledc_channel_config(&ledc_config_CH_N[i]);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG_PWM_INIT, "Error: ledc_channel_config on ch%d", i);
            return PWM_INIT_ERROR;
        }
    }

    // Stop channels that should not be active (important after esp_restart(); is called)
    for (; i < LED_PIN_CH_N_size; i++)
    {
        ret |= ledc_stop(LEDC_LOW_SPEED_MODE, LED_CH_N[i], 0);
    }
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_PWM_INIT, "Error in pwm_init function");
        return PWM_INIT_ERROR;
    }
    return PWM_INIT_OK;
}

uint8_t get_button_state(uint8_t button_gpio_num) // COMPLETE
{
    static uint8_t current_button_state = GPIO_BUTTON_RELEASED_VALUE;
    static uint8_t previous_button_state = GPIO_BUTTON_RELEASED_VALUE;

    current_button_state = gpio_get_level(button_gpio_num);

    // The position of the button changed compared to the state in the previous function call
    if (current_button_state != previous_button_state)
    {
        if (current_button_state == GPIO_BUTTON_PRESSED_VALUE)
        {
            previous_button_state = current_button_state;
            return BUTTON_STATE_POSEDGE;
        }
        else
        {
            previous_button_state = current_button_state;
            return BUTTON_STATE_NEGEDGE;
        }
    }
    else // The position of the button remained the same as in the previous function call
    {
        if (current_button_state == GPIO_BUTTON_PRESSED_VALUE)
        {
            previous_button_state = current_button_state;
            return BUTTON_STATE_PRESSED;
        }
        else
        {
            previous_button_state = current_button_state;
            return BUTTON_STATE_RELEASED;
        }
    }
    return BUTTON_STATE_ERROR;
}

/*** JSON ***/
uint8_t json_start_read_config(void) // COMPLETE
{
    // Load DEVICE CONFIGURATION to buffer
    char buffer[JSON_CONFIG_FILE_MAX_LEN + 1] = "0";
    switch (read_spiffs_file_to_buffer(JSON_CONFIG_FILE_CONFIGURATION_PATH, buffer, JSON_CONFIG_FILE_MAX_LEN))
    {
    case READ_SPIFFS_OK:
    {
        break;
    }
    default:
    {
        return JSON_START_READ_ERROR;
    }
    }

    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        // Cannot parse json -> return error
        ESP_LOGE(TAG_JSON_START_READ, "Cannot parse %s file", JSON_CONFIG_FILE_CONFIGURATION_PATH);
        return JSON_START_READ_ERROR;
    }

    // Save configuration to TMP variables first
    uint8_t i = 0;
    struct device_config_struct device_conf_tmp;
    uint16_t *const device_conf_tmp_ptr[] = {&device_conf_tmp.rgb_outputs, &device_conf_tmp.w_outputs, &device_conf_tmp.enable_wifi};

    // Read all confuguration values
    for (i = 0; i < config_name_size; i++)
    {
        cJSON *config_param_json = cJSON_GetObjectItem(json, config_name[i]);
        if (config_param_json == NULL)
        {
            ESP_LOGE(TAG_JSON_START_READ, "Cannot find all values in %s file", JSON_CONFIG_FILE_CONFIGURATION_PATH);
            return JSON_START_READ_ERROR;
        }
        // Save to TMP variables
        *device_conf_tmp_ptr[i] = (uint16_t)(config_param_json->valueint);
    }

    // If all within limits -> save configuration to global variables
    if ((device_conf_tmp.rgb_outputs <= NO_RGB_OUTPUTS_LIMIT) && (device_conf_tmp.w_outputs <= (NO_W_OUTPUTS_LIMIT - 3 * device_conf_tmp.rgb_outputs)) && (device_conf_tmp.enable_wifi <= ENABLE_MAX_VALUE))
    { // Copy to global variables
        device_conf.w_outputs = device_conf_tmp.w_outputs;
        device_conf.rgb_outputs = device_conf_tmp.rgb_outputs;
        device_conf.enable_wifi = device_conf_tmp.enable_wifi;
        device_conf.channels_used = 3 * device_conf_tmp.rgb_outputs + device_conf_tmp.w_outputs;
    }
    else
    {
        ESP_LOGE(TAG_JSON_START_READ, "Configuration in %s file not valid: RGB outputs: %d, W outputs: %d, Enable WiFi: %d",
                 JSON_CONFIG_FILE_CONFIGURATION_PATH, device_conf_tmp.rgb_outputs, device_conf_tmp.w_outputs, device_conf_tmp.enable_wifi);
        return JSON_START_READ_ERROR;
    }
    ESP_LOGI(TAG_JSON_START_READ, "Configuration in %s file: RGB outputs: %d, W outputs: %d, Total channels: %d, Enable WiFi: %d",
             JSON_CONFIG_FILE_CONFIGURATION_PATH, device_conf.rgb_outputs, device_conf.w_outputs, device_conf.channels_used, device_conf.enable_wifi);

    return JSON_START_READ_OK;
}

uint8_t json_start_read_channel_data(void) // COMPLETE
{
    // Load CHANNELS DATA to buffer
    char buffer[JSON_CONFIG_FILE_MAX_LEN + 1] = "0";
    switch (read_spiffs_file_to_buffer(JSON_CONFIG_FILE_CH_DATA_PATH, buffer, JSON_CONFIG_FILE_MAX_LEN))
    {
    case READ_SPIFFS_OK:
    {
        break;
    }
    default:
    {
        return JSON_START_READ_ERROR;
    }
    }

    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        // Cannot parse json -> return error
        ESP_LOGE(TAG_JSON_START_READ, "Cannot parse %s file", JSON_CONFIG_FILE_CH_DATA_PATH);
        return JSON_START_READ_ERROR;
    }

    // Save LED PWM values to global variables
    uint8_t i = 0;
    // Read all values (not limited by device_conf.channels_used) to preserve config file format
    for (i = 0; i < channel_data_name_duty_size; i++)
    {
        const char *chN_duty = channel_data_name_duty[i];
        const char *chN_enable = channel_data_name_enable[i];
        cJSON *duty_json = cJSON_GetObjectItem(json, chN_duty);
        cJSON *enable_json = cJSON_GetObjectItem(json, chN_enable);

        if ((duty_json == NULL) || (enable_json == NULL)) // Check if all values exist
        {
            ESP_LOGE(TAG_JSON_START_READ, "Cannot find all values in %s file", JSON_CONFIG_FILE_CH_DATA_PATH);
            return JSON_START_READ_ERROR;
        }

        // Save DUTY
        uint16_t val = 0;
        val = (uint16_t)(duty_json->valueint);
        *channel_value_duty[i] = (val < DUTY_MAX_VALUE) ? val : DUTY_MAX_VALUE;
        // Save ENABLE
        val = (uint16_t)(enable_json->valueint);
        *channel_value_enable[i] = (val < ENABLE_MAX_VALUE) ? val : ENABLE_MAX_VALUE;
    }
    ESP_LOGI(TAG_JSON_START_READ, "Channel data read successful");
    return JSON_START_READ_OK;
}

uint8_t json_start_read_credentials(void) // COMPLETE
{
    // Load WIFI CREDENTIALS to buffer
    char buffer[JSON_CONFIG_FILE_MAX_LEN + 1] = "0";
    switch (read_spiffs_file_to_buffer(JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH, buffer, JSON_CONFIG_FILE_MAX_LEN))
    {
    case READ_SPIFFS_OK:
    {
        break;
    }
    default:
    {
        return JSON_START_READ_ERROR;
    }
    }

    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        // Cannot parse json -> return error
        ESP_LOGE(TAG_JSON_START_READ, "Cannot parse %s file", JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH);
        return JSON_START_READ_ERROR;
    }

    // Read all credential values
    for (uint8_t i = 0; i < credentials_name_size; i++)
    {
        cJSON *credential_json = cJSON_GetObjectItem(json, credentials_name[i]);
        // Check if all values exist
        if (credential_json == NULL)
        {
            ESP_LOGE(TAG_JSON_START_READ, "Cannot find all values in %s file", JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH);
            return JSON_START_READ_ERROR;
        }

        uint8_t credential_len = strlen((char *)credential_json->valuestring);

        if (credential_len <= CREDENTIALS_MAX_LEN)
        {
            memcpy(credentials_value[i], credential_json->valuestring, credential_len + 1);
            // printf("%s: %s\n\r", credentials_name[i], credentials_value[i]);

            if (credential_len == 0)
            {
                device_wifi_status.wifi_credentials_empty_flag = 1;
                ESP_LOGI(TAG_JSON_START_READ, "WiFi credentials empty");
            }
        }
        else
        {
            ESP_LOGE(TAG_JSON_START_READ, "WiFi credentials are too long");
            return JSON_START_READ_ERROR;
        }
    }
    ESP_LOGI(TAG_JSON_START_READ, "WiFi credentials read successful");
    return JSON_START_READ_OK;
}

uint8_t json_start_read(void) // COMPLETE
{
    switch (json_start_read_config())
    {
    case JSON_START_READ_OK:
    {
        break;
    }
    default: // JSON_START_READ_ERROR
    {
        return JSON_START_READ_ERROR;
    }
    }

    switch (json_start_read_channel_data())
    {
    case JSON_START_READ_OK:
    {
        break;
    }
    default: // JSON_START_READ_ERROR
    {
        return JSON_START_READ_ERROR;
    }
    }

    switch (json_start_read_credentials())
    {
    case JSON_START_READ_OK:
    {
        break;
    }
    default: // JSON_START_READ_ERROR
    {
        return JSON_START_READ_ERROR;
    }
    }

    return JSON_START_READ_OK;
}

/*** STORAGE ***/
uint8_t nvs_init(void) // COMPLETE
{
    // Init NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_NVS_INIT, "Error: nvs_flash_init");

        // Erese NVS if truncated or contains data in new format that cannot be recognized
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ret = nvs_flash_erase();
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG_NVS_INIT, "Error: nvs_flash_erase");
                return NVS_INIT_ERROR;
            }
            ret = nvs_flash_init();
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG_NVS_INIT, "Error (2): nvs_flash_init");
                return NVS_INIT_ERROR;
            }
            ESP_LOGI(TAG_NVS_INIT, "Attempt 2: nvs_flash_init successful");
            return NVS_INIT_OK;
        }
        return NVS_INIT_ERROR;
    }
    return NVS_INIT_OK;
}

uint8_t spiffs_init(void) // COMPLETE
{
    // Register and mount SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_SPIFFS_INIT, "Error: esp_vfs_spiffs_register");
        return SPIFFS_INIT_ERROR;
    }

    // LOG spiffs partition info
    /*
    size_t total_size = 0;
    size_t used_size = 0;
    ret = esp_spiffs_info(spiffs_config.partition_label, &total_size, &used_size);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_SPIFFS_INIT, "Error reading SPIFFS info");
        return SPIFFS_INIT_ERROR;
    }
    ESP_LOGI(TAG_SPIFFS_INIT, "Total SPIFFS size: %d", total_size);
    ESP_LOGI(TAG_SPIFFS_INIT, "Used SPIFFS size: %d", used_size);
    */

    return SPIFFS_INIT_OK;
}

uint8_t read_spiffs_file_to_buffer(const char *path, char *buffer, const uint16_t buffer_len) // COMPLETE
{
    // Open file in read mode
    FILE *f = fopen(path, "r");
    if (f != NULL) // File opened successfully
    {
        size_t read_words;
        read_words = fread(buffer, sizeof(char), (buffer_len - 1), f);
        buffer[read_words] = '\0';
        fclose(f);
        // printf(">>>%s<<<\n\r", buffer);
    }
    else // Cant open file -> return error
    {
        ESP_LOGE(TAG_READ_SPIFFS, "Cannot open file: %s", path);
        return READ_SPIFFS_ERROR;
    }
    return READ_SPIFFS_OK;
}

uint8_t write_buffer_to_spiffs_file(const char *path, const char *buffer) // COMPLETE
{
    // Open file in write mode
    FILE *f = fopen(path, "w");
    if (f != NULL) // File opened successfully
    {
        size_t write_words;
        write_words = fwrite(buffer, sizeof(char), strlen(buffer), f);
        fclose(f);
        // printf("Buffer written to file: %s, %d words written\n\r", path, write_words);

        if (write_words == 0)
        {
            ESP_LOGW(TAG_WRITE_SPIFFS, "No words were written to file %s", path);
        }
    }
    else // Cant open file -> return error
    {
        ESP_LOGE(TAG_WRITE_SPIFFS, "Cannot open file: %s", path);
        return WRITE_SPIFFS_ERROR;
    }
    return WRITE_SPIFFS_OK;
}

/*** ERROR HANDLER FUNCTION ***/
void error_handler_function(uint8_t error) // COMPLETE
{
    /***
     * This function is called when error occur during device bootup process
     */

    ESP_LOGW(TAG_ERROR_HANDLER_FUNC, "In function: error_handler_function. Error ID: %d", error);

    switch (error)
    {
    case CRITICAL_ERROR_WIFI:
    case CRITICAL_ERROR_EVENT_HANDLER:
    {
        // Stop WiFi and free all allocated resources
        device_wifi_stop();

        esp_err_t ret = ESP_OK;

        // Unregister handler from the system event loop
        ret |= esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, handler_instance_ip);
        ret |= esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, handler_instance_ip);
        ret |= esp_event_handler_instance_unregister(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, handler_instance_http_server);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG_ERROR_HANDLER_FUNC, "Error: esp_event_handler_instance_unregister");
        }

        // Delete default event loop
        ret = esp_event_loop_delete_default();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG_ERROR_HANDLER_FUNC, "Error: esp_event_loop_delete_default");
        }

        // Delete wifi_reconnect_task if it exists
        if (WiFiReconnectTaskHandle != NULL)
        {
            ESP_LOGW(TAG_ERROR_HANDLER_FUNC, "DELETING TASK: wifi_reconnect_task");
            vTaskDelete(WiFiReconnectTaskHandle);
            WiFiReconnectTaskHandle = NULL;
        }

        // Delete device_main_task if it exists
        if (DeviceMainTaskHandle != NULL)
        {
            ESP_LOGW(TAG_ERROR_HANDLER_FUNC, "DELETING TASK: device_main_task");
            vTaskDelete(DeviceMainTaskHandle);
            DeviceMainTaskHandle = NULL;
        }

        /**
         *  If another way (besides WiFi) to control PWM channels is added to this project
         *  channels_control_task and spiffs_auto_save_channel_data_task should NOT be deleted
         *  In other case controlling PWM channels will not be possible
         */
        // Delete channels_control_task
        if (ChannelsControlTaskHandle != NULL)
        {
            ESP_LOGW(TAG_ERROR_HANDLER_FUNC, "DELETING TASK: channels_control_task");
            vTaskDelete(ChannelsControlTaskHandle);
            ChannelsControlTaskHandle = NULL;
        }

        // Delete spiffs_auto_save_channel_data_task
        if (SpiffsAutoSaveTaskHandle != NULL)
        {
            ESP_LOGW(TAG_ERROR_HANDLER_FUNC, "DELETING TASK: spiffs_auto_save_channel_data_task");
            vTaskDelete(SpiffsAutoSaveTaskHandle);
            SpiffsAutoSaveTaskHandle = NULL;
        }

        break;
    }
    case CRITICAL_ERROR_PWM:
    case CRITICAL_ERROR_JSON:
    case CRITICAL_ERROR_SPIFFS:
    case CRITICAL_ERROR_NVS:
    case CRITICAL_ERROR_GPIO:
    {
        // Nothing to do.
        break;
    }
    default:
    {
        // Unknown error -> Delete "main" task
        ESP_LOGE(TAG_ERROR_HANDLER_FUNC, "Unknown error");
        ESP_LOGW(TAG_ERROR_HANDLER_FUNC, "DELETING MAIN TASK (SELF DELETE)");
        vTaskDelete(NULL);
        break;
    }
    }
}

/*** FACTORY RESET FUNCTION ***/
uint8_t factory_reset_function(void) // COMPLETE
{
    ESP_LOGI(TAG_FACTORY_RESET_FUNC, "In function: factory_reset_function");

    /* Partition should not be formatted, otherwise webpage files will be lost.*/
    // Format SPIFFS partition
    // esp_err_t ret = esp_spiffs_format(spiffs_config.partition_label);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Error: esp_spiffs_format");
    //     return FACTORY_RESET_ERROR;
    // }
    // ESP_LOGI(TAG_FACTORY_RESET_FUNC, "Formating SPIFFS done");

    // SPIFFS init
    if (spiffs_init() != SPIFFS_INIT_OK)
    {
        ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Error: spiffs_init");
        return FACTORY_RESET_ERROR;
    }
    ESP_LOGI(TAG_FACTORY_RESET_FUNC, "Initialization of SPIFFS done");

    /*** Write defaults to channel_data.json ***/
    // Prepare buffer
    char buffer[JSON_CONFIG_FILE_MAX_LEN + 61] = "0";
    buffer[0] = '\0';
    uint16_t cnt = sprintf(buffer, "{ ");

    // Here all values are stored and are not limited by device_conf.channels_used to preserve config file format
    for (uint8_t i = 0; i < channel_data_name_duty_size; i++)
    {
        cnt += sprintf(buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[i], FACTORY_RESET_DEFAULT_DUTY);
        cnt += sprintf(buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[i], FACTORY_RESET_DEFAULT_ENABLE);

        // Buffer overflow protection -> return error
        if (cnt > JSON_CONFIG_FILE_MAX_LEN)
        {
            ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Buffer overflow protection: factory_reset_function (%d chars, but %d is upper limit)", cnt, JSON_CONFIG_FILE_MAX_LEN);
            return FACTORY_RESET_ERROR;
        }
    }
    cnt += sprintf(buffer + cnt - 3, " }");
    // printf(">>>%s<<<\n\r", buffer);

    // Write buffer to channel_data.json
    if (write_buffer_to_spiffs_file(JSON_CONFIG_FILE_CH_DATA_PATH, buffer) != WRITE_SPIFFS_OK)
    {
        ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Error: write_buffer_to_spiffs_file");
        return FACTORY_RESET_ERROR;
    }
    ESP_LOGI(TAG_FACTORY_RESET_FUNC, "Writing %s to SPIFFS done", JSON_CONFIG_FILE_CH_DATA_PATH);

    /*** Write defaults to config.json ***/
    // Prepare buffer
    buffer[0] = '\0';
    cnt = sprintf(buffer, "{ ");

    for (uint8_t i = 0; i < config_name_size; i++)
    {
        cnt += sprintf(buffer + cnt, "\"%s\":%d , ", config_name[i], config_factory_reset_values[i]);

        // Buffer overflow protection -> return error
        if (cnt > JSON_CONFIG_FILE_MAX_LEN)
        {
            ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Buffer overflow protection: factory_reset_function (%d chars, but %d is upper limit)", cnt, JSON_CONFIG_FILE_MAX_LEN);
            return FACTORY_RESET_ERROR;
        }
    }
    cnt += sprintf(buffer + cnt - 3, " }");
    // printf(">>>%s<<<\n\r", buffer);

    // Write buffer to config.json
    if (write_buffer_to_spiffs_file(JSON_CONFIG_FILE_CONFIGURATION_PATH, buffer) != WRITE_SPIFFS_OK)
    {
        ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Error: write_buffer_to_spiffs_file");
        return FACTORY_RESET_ERROR;
    }
    ESP_LOGI(TAG_FACTORY_RESET_FUNC, "Writing %s to SPIFFS done", JSON_CONFIG_FILE_CONFIGURATION_PATH);

    /*** Write defaults to credentials.json ***/
    // Prepare buffer
    buffer[0] = '\0';
    cnt = sprintf(buffer, "{ ");

    for (uint8_t i = 0; i < credentials_name_size; i++)
    {
        cnt += sprintf(buffer + cnt, "\"%s\":\"\" , ", credentials_name[i]);

        // Buffer overflow protection -> return error
        if (cnt > JSON_CONFIG_FILE_MAX_LEN)
        {
            ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Buffer overflow protection: factory_reset_function (%d chars, but %d is upper limit)", cnt, JSON_CONFIG_FILE_MAX_LEN);
            return FACTORY_RESET_ERROR;
        }
    }
    cnt += sprintf(buffer + cnt - 3, " }");
    // printf(">>>%s<<<\n\r", buffer);

    // Write buffer to credentials.json
    if (write_buffer_to_spiffs_file(JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH, buffer) != WRITE_SPIFFS_OK)
    {
        ESP_LOGE(TAG_FACTORY_RESET_FUNC, "Error: write_buffer_to_spiffs_file");
        return FACTORY_RESET_ERROR;
    }
    ESP_LOGI(TAG_FACTORY_RESET_FUNC, "Writing %s to SPIFFS done", JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH);

    return FACTORY_RESET_OK;
}

/***  MISCELLANEOUS FUNCTIONS ***/
uint8_t path_parse(const char *full_path, char *file_name_prefix, char *file_name, uint16_t path_name_size, uint16_t file_name_size) // COMPLETE
{
    /**
     *  This function parses a full path into a file name prefix and a file name.
     *  It replaces '/' in the prefix with '_' (SPIFFS does not support directories)
     *  It checks for errors (null pointers or insufficient buffer size)
     *  It return an error code if any occur, or PATH_PARSE_OK if parsing successful.
     */

    // Check for null pointers
    if (full_path == NULL || file_name_prefix == NULL || file_name == NULL)
    {
        return PATH_PARSE_ERROR;
    }

    // Get the length of the full path
    uint16_t full_path_len = strlen(full_path);

    // Find the last '/' in the full path
    uint16_t idx = full_path_len;
    for (; (full_path[idx] != '/'); idx--)
    {
        if (idx == 0)
        {
            return PATH_PARSE_ERROR;
        }
    }

    // Check if the prefix or file name would exceed the buffer size
    if ((idx > path_name_size) || ((full_path_len - idx) > file_name_size))
    {
        return PATH_PARSE_ERROR;
    }

    // Copy the file name into the file_name buffer
    strcpy(file_name, full_path + idx + 1); // NULL cahracter is added

    // Copy the prefix into the file_name_prefix buffer
    strncpy(file_name_prefix, full_path + 1, idx); // NULL cahracter is NOT added
    file_name_prefix[idx] = '\0';

    // Replace any '/' characters in the prefix with '_'
    for (uint16_t i = 0; file_name_prefix[i] != '\0'; i++)
    {
        if (file_name_prefix[i] == '/')
        {
            file_name_prefix[i] = '_';
        }
    }

    return PATH_PARSE_OK;
}

/***************************************************************************************************************/
/*** END *******************************************************************************************************/
/***************************************************************************************************************/