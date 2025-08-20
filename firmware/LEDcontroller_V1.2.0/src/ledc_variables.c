/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_variables.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** LOGGING ***/
// Tasks
const char *TAG_DEVICE_MAIN_TASK = "(LOG) DEVICE MAIN TASK";                 // Logging TAG: device_main_task
const char *TAG_BUTTON_TASK = "(LOG) BUTTON TASK";                           // Logging TAG: button_task
const char *TAG_WIFI_BLINK_TASK = "(LOG) WIFI BLINK TASK";                   // Logging TAG: wifi_blink_task
const char *TAG_ERROR_BLINK_TASK = "(LOG) ERROR BLINK TASK";                 // Logging TAG: error_blink_task
const char *TAG_FACTORY_RESET_BLINK_TASK = "(LOG) FACTORY RESET BLINK TASK"; // Logging TAG: factory_reset_blink_task
const char *TAG_SPIFFS_AUTO_SAVE_TASK = "(LOG) SPIFFS SAVE TASK";            // Logging TAG: spiffs_auto_save_channel_data_task
const char *TAG_WIFI_RECONNECT_TASK = "(LOG) WIFI RECONNECT TASK";           // Logging TAG: wifi_reconnect_task
const char *TAG_CHANNELS_TASK = "(LOG) CHANNELS TASK";                       // Logging TAG: channels_duty_change_task, channels_control_task
// Event handlers
const char *TAG_EVENT_HANDLER = "(LOG) EVENT HANDLER"; // Logging TAG: event_handler
// HTTP server init and handlers
const char *TAG_HTTP_SERVER = "(LOG) HTTP SERVER"; // Logging TAG: start_http_server + all http handlers
// Functions
const char *TAG_APP_MAIN = "(LOG) APP MAIN";                         // Logging TAG: app_main
const char *TAG_EVENT_HANDLER_INIT = "(LOG) EVENT HANDLER INIT";     // Logging TAG: event_handler_init
const char *TAG_WIFI_STATION_INIT = "(LOG) WIFI STATION INIT";       // Logging TAG: wifi_station_init
const char *TAG_WIFI_AP_INIT = "(LOG) WIFI AP INIT";                 // Logging TAG: wifi_ap_init
const char *TAG_DEVICE_WIFI_STOP = "(LOG) DEVICE WIFI STOP";         // Logging TAG: device_wifi_stop
const char *TAG_GPIO_INIT = "(LOG) GPIO INIT";                       // Logging TAG: gpio_init
const char *TAG_PWM_INIT = "(LOG) PWM INIT";                         // Logging TAG: pwm_init
const char *TAG_JSON_START_READ = "(LOG) JSON START READ";           // Logging TAG: json_start_read + json_start_read_xxxx
const char *TAG_NVS_INIT = "(LOG) NVS INIT";                         // Logging TAG: nvs_init
const char *TAG_SPIFFS_INIT = "(LOG) SPIFFS INIT";                   // Logging TAG: spiffs_init
const char *TAG_READ_SPIFFS = "(LOG) READ SPIFFS";                   // Logging TAG: read_spiffs_file_to_buffer
const char *TAG_WRITE_SPIFFS = "(LOG) WRITE SPIFFS";                 // Logging TAG: write_buffer_to_spiffs_file
const char *TAG_ERROR_HANDLER_FUNC = "(LOG) ERROR HANDLER FUNCTION"; // Logging TAG: error_handler_function
const char *TAG_FACTORY_RESET_FUNC = "(LOG) FACTORY RESET FUNCTION"; // Logging TAG: factory_reset_function

/***************************************************************************************************************/
/*** DEVICE CONFIGURATION & STATE VARIABLES ********************************************************************/

// SPIFFS configuration structure
esp_vfs_spiffs_conf_t spiffs_config = {
    .base_path = SPIFFS_PARTITION_BASE_PATH,
    .partition_label = NULL,
    .max_files = SPIFFS_PARTITION_MAX_FILES,
    .format_if_mount_failed = false,
};

// WiFi STATION credentials
uint8_t wifi_ssid[CREDENTIALS_MAX_LEN + 2] = "0";
uint8_t wifi_password[CREDENTIALS_MAX_LEN + 2] = "0";
uint8_t *const credentials_value[] = {wifi_ssid, wifi_password};

// WiFi AP credentials
uint8_t wifi_ap_ssid[] = "LedController";
uint8_t wifi_ap_password[] = "abc123456789";

// Stores device configuration
struct device_config_struct device_conf;
uint16_t *const config_value[] = {&device_conf.rgb_outputs, &device_conf.w_outputs, &device_conf.enable_wifi};

// Stores desired values for each channel
struct channel_config_struct ch0, ch1, ch2, ch3, ch4, ch5;
uint16_t *const channel_value_duty[] = {&ch0.duty, &ch1.duty, &ch2.duty, &ch3.duty, &ch4.duty, &ch5.duty};
const uint8_t channel_value_duty_size = sizeof(channel_value_duty) / sizeof(channel_value_duty[0]);
uint16_t *const channel_value_enable[] = {&ch0.enable, &ch1.enable, &ch2.enable, &ch3.enable, &ch4.enable, &ch5.enable};
const uint8_t channel_value_enable_size = sizeof(channel_value_enable) / sizeof(channel_value_enable[0]);

// Stores necessary values to smoothly change duty cycle of channels that need to be changed
struct channel_change_list_struct channel_change_list;

// Stores current WiFi mode, flags and activity counter
struct device_wifi_status_struct device_wifi_status = {DEVICE_WIFI_MODE_NONE, 0, 0, 0, 1, 0};

// Stores button current state and last or current pressed time
struct device_button_status_struct device_button_status;

// Stores device error status
struct device_error_status_struct device_error_status = {ERROR_LEVEL_NONE, 0};

// Stores device factory reset status
struct factory_reset_status_struct factory_reset_status = {FACTOR_RESET_STATE_NONE};

// List of CREDENTIAL NAMES (credentials.json)
const char *const credentials_name[] = {"WIFI_SSID", "WIFI_PASSWORD"};
const uint8_t credentials_name_size = sizeof(credentials_name) / sizeof(credentials_name[0]);

// List of CONFIG NAMES (config.json)
const char *const config_name[] = {"rgb_outputs", "w_outputs", "enable_wifi"};
const uint8_t config_name_size = sizeof(config_name) / sizeof(config_name[0]);
const uint8_t config_factory_reset_values[] = {FACTORY_RESET_DEFAULT_RGB_OUTPUTS, FACTORY_RESET_DEFAULT_W_OUTPUTS, FACTORY_RESET_DEFAULT_ENABLE_WIFI};
const uint8_t config_factory_reset_values_size = sizeof(config_factory_reset_values) / sizeof(config_factory_reset_values[0]);

// List of CHANNEL PARAMETER NAMES (channel_data.json)
const char *const channel_data_name_duty[] = {"ch0_duty", "ch1_duty", "ch2_duty", "ch3_duty", "ch4_duty", "ch5_duty"};
const uint8_t channel_data_name_duty_size = sizeof(channel_data_name_duty) / sizeof(channel_data_name_duty[0]);
const char *const channel_data_name_enable[] = {"ch0_enable", "ch1_enable", "ch2_enable", "ch3_enable", "ch4_enable", "ch5_enable"};
const uint8_t channel_data_name_enable_size = sizeof(channel_data_name_enable) / sizeof(channel_data_name_enable[0]);

// List of LED PINS and LED CHANNELSs
const uint8_t LED_PIN_CH_N[] = {GPIO_CH_0, GPIO_CH_1, GPIO_CH_2, GPIO_CH_3, GPIO_CH_4, GPIO_CH_5};
const uint8_t LED_PIN_CH_N_size = sizeof(LED_PIN_CH_N) / sizeof(LED_PIN_CH_N[0]);
const uint8_t LED_CH_N[] = {PWM_CH_0, PWM_CH_1, PWM_CH_2, PWM_CH_3, PWM_CH_4, PWM_CH_5};
const uint8_t LED_CH_N_size = sizeof(LED_CH_N) / sizeof(LED_CH_N[0]);

// List of PARAMETER NAMES only used for API
const char *const api_all_param_names[] = {"ch_duty", "ch_enable"};
const uint8_t api_all_param_names_size = sizeof(api_all_param_names) / sizeof(api_all_param_names[0]);

// WEB URI defaults
const char *const web_uri_defult_page = "index.html";   // If request equals "/<path>/" -> it is interpreted as "/<path>/index.html"
const char *const web_uri_defult_404_page = "pnf.html"; // If request not fond return "pnf.html"

// API HTTP respond messages
const char *const http_response_message_error[] = {"ERROR: Unknown error",
                                                   "ERROR: Data length exceeded the limit",
                                                   "ERROR: Number of parameters exceeded the limit",
                                                   "ERROR: Not all parameters are valid",
                                                   "ERROR: Received data parsing error",
                                                   "ERROR: No data received",
                                                   "ERROR: Not all parameters were received",
                                                   "ERROR: Invalid configuration",
                                                   "ERROR: Maximum number of characters exceeded",
                                                   "ERROR: Query length exceeded the limit",
                                                   "ERROR: Number of requested parameters exceeded the limit",
                                                   "ERROR: Not all requested parameters are valid",
                                                   "ERROR: Cannot return channel values",
                                                   "ERROR: The credentials field is empty",
                                                   "ERROR: URI length exceeded the limit",
                                                   "ERROR: URI not found"};

const char *const http_response_message_warning[] = {"WARNING: Unknown error",
                                                     "WARNING: Data length exceeded the limit",
                                                     "WARNING: Number of parameters exceeded the limit",
                                                     "WARNING: Not all parameters are valid",
                                                     "WARNING: Received data parsing error",
                                                     "WARNING: No data received",
                                                     "WARNING: Not all parameters were received",
                                                     "WARNING: Invalid configuration",
                                                     "WARNING: Maximum number of characters exceeded",
                                                     "WARNING: Query length exceeded the limit",
                                                     "WARNING: Number of requested parameters exceeded the limit",
                                                     "WARNING: Not all requested parameters are valid",
                                                     "WARNING: Cannot return channel values",
                                                     "WARNING: The credentials field is empty",
                                                     "WARNING: URI length exceeded the limit",
                                                     "WARNING: URI not found"};

const char *const http_response_message_ok[] = {"Update successful. Reboot device to apply new configuration.",
                                                "Rebooting"};

/***************************************************************************************************************/
/*** END *******************************************************************************************************/
/***************************************************************************************************************/