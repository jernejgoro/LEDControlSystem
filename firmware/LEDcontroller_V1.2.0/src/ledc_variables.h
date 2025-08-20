#ifndef LEDC_VARIABLES_H
#define LEDC_VARIABLES_H
/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_common.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** LOGGING ***/
// Tasks
extern const char *TAG_DEVICE_MAIN_TASK;         // Logging TAG: device_main_task
extern const char *TAG_BUTTON_TASK;              // Logging TAG: button_task
extern const char *TAG_WIFI_BLINK_TASK;          // Logging TAG: wifi_blink_task
extern const char *TAG_ERROR_BLINK_TASK;         // Logging TAG: error_blink_task
extern const char *TAG_FACTORY_RESET_BLINK_TASK; // Logging TAG: factory_reset_blink_task
extern const char *TAG_SPIFFS_AUTO_SAVE_TASK;    // Logging TAG: spiffs_auto_save_channel_data_task
extern const char *TAG_WIFI_RECONNECT_TASK;      // Logging TAG: wifi_reconnect_task
extern const char *TAG_CHANNELS_TASK;            // Logging TAG: channels_duty_change_task, channels_control_task
// Event handlers
extern const char *TAG_EVENT_HANDLER; // Logging TAG: event_handler
// HTTP server init and handlers
extern const char *TAG_HTTP_SERVER; // Logging TAG: start_http_server + all http handlers
// Functions
extern const char *TAG_APP_MAIN;           // Logging TAG: app_main
extern const char *TAG_EVENT_HANDLER_INIT; // Logging TAG: event_handler_init
extern const char *TAG_WIFI_STATION_INIT;  // Logging TAG: wifi_station_init
extern const char *TAG_WIFI_AP_INIT;       // Logging TAG: wifi_ap_init
extern const char *TAG_DEVICE_WIFI_STOP;   // Logging TAG: device_wifi_stop
extern const char *TAG_GPIO_INIT;          // Logging TAG: gpio_init
extern const char *TAG_PWM_INIT;           // Logging TAG: pwm_init
extern const char *TAG_JSON_START_READ;    // Logging TAG: json_start_read + json_start_read_xxxx
extern const char *TAG_NVS_INIT;           // Logging TAG: nvs_init
extern const char *TAG_SPIFFS_INIT;        // Logging TAG: spiffs_init
extern const char *TAG_READ_SPIFFS;        // Logging TAG: read_spiffs_file_to_buffer
extern const char *TAG_WRITE_SPIFFS;       // Logging TAG: write_buffer_to_spiffs_file
extern const char *TAG_ERROR_HANDLER_FUNC; // Logging TAG: error_handler_function
extern const char *TAG_FACTORY_RESET_FUNC; // Logging TAG: factory_reset_function

/***************************************************************************************************************/
/*** DEVICE CONFIGURATION & STATE VARIABLES ********************************************************************/

// SPIFFS configuration structure
extern esp_vfs_spiffs_conf_t spiffs_config;

// WiFi STATION credentials
extern uint8_t wifi_ssid[];
extern uint8_t wifi_password[];
extern uint8_t *const credentials_value[];

// WiFi AP credentials
extern uint8_t wifi_ap_ssid[];
extern uint8_t wifi_ap_password[];

// Stores device configuration
struct device_config_struct
{
    // Stored in config.json, R: many, W: app_main (at initialization)
    uint16_t rgb_outputs;   // Number of RGB outputs (1 output uses 3 PWM channel)
    uint16_t w_outputs;     // Number of white outputs (1 output uses 1 PWM channel)
    uint16_t enable_wifi;   // WiFi enabled in configuration by user (Controls whether the WiFi STATION will turn ON or OFF on boot/reboot)
    uint16_t channels_used; // Total number of PWM channels used (calculated value at boot/reboot)
};
extern struct device_config_struct device_conf;
extern uint16_t *const config_value[];

// Stores desired values for each channel
struct channel_config_struct
{
    uint16_t enable;
    uint16_t duty;
};
extern struct channel_config_struct ch0, ch1, ch2, ch3, ch4, ch5;
extern uint16_t *const channel_value_duty[];
extern const uint8_t channel_value_duty_size;
extern uint16_t *const channel_value_enable[];
extern const uint8_t channel_value_enable_size;

// Stores necessary values to smoothly change duty cycle of channels that need to be changed
struct channel_change_list_struct
{
    // R: channels_duty_change_task, W: channels_control_task
    uint8_t LED_CH_N[NO_CHANNELS_LIMIT + 1];  // List of PWM channels
    uint16_t duty_new[NO_CHANNELS_LIMIT + 1]; // List of desired duty cycle values
    // RW: channels_duty_change_task
    uint16_t duty_current[NO_CHANNELS_LIMIT + 1]; // List of current duty cycle values
};
extern struct channel_change_list_struct channel_change_list;

// Stores current WiFi mode, flags and activity counter
struct device_wifi_status_struct
{
    // RW: app_main, wifi_blink_task, W: event_handler, json_start_read_credentials
    uint16_t wifi_mode;                   // Stores mode in which WiFi is operating: AP, STATION or NONE
    uint16_t wifi_mode_ch_flag;           // Should be set to 1 on mode change (wifi_blink_task should reset it to 0)
    uint16_t wifi_sta_connected_flag;     // Should be set to 1 when device in STATION mode and connected to AP
    uint16_t wifi_activity_cnt;           // Counter of WiFi/HTTP server activities (for WiFi LED blinking)
    uint16_t wifi_hw_init_flag;           // Should be set to 1 if WiFi hardware initialization IS successful
    uint16_t wifi_credentials_empty_flag; // Should be set to 1 if WiFi credentials are empty (do not exist)
};
extern struct device_wifi_status_struct device_wifi_status;

// Stores button current state and last or current pressed time
struct device_button_status_struct
{
    // W: button_task, R: factory_reset_blink_task, app_main
    uint16_t button_pressed_ms; // Last button press duration
    uint16_t button_state;      // Current button state: BUTTON_STATE_PRESSED or BUTTON_STATE_RELEASED
};
extern struct device_button_status_struct device_button_status;

// Stores device error status
struct device_error_status_struct
{
    // R: error_blink_task,  W: many
    uint16_t new_error_level; // Last reported error level
    // RW: error_blink_task,  W: many
    uint16_t new_error_flag; // Should be set to 1 on new error level report
};
extern struct device_error_status_struct device_error_status;

// Stores device factory reset status
struct factory_reset_status_struct
{
    // R: factory_reset_blink_task,  RW: app_main
    uint16_t state; // Current state of factory reset process
};
extern struct factory_reset_status_struct factory_reset_status;

// List of CREDENTIAL NAMES (credentials.json)
extern const char *const credentials_name[];
extern const uint8_t credentials_name_size;

// List of CONFIG NAMES (config.json)
extern const char *const config_name[];
extern const uint8_t config_name_size;
extern const uint8_t config_factory_reset_values[];
extern const uint8_t config_factory_reset_values_size;

// List of CHANNEL PARAMETER NAMES (channel_data.json)
extern const char *const channel_data_name_duty[];
extern const uint8_t channel_data_name_duty_size;
extern const char *const channel_data_name_enable[];
extern const uint8_t channel_data_name_enable_size;

// List of LED PINS and LED CHANNELSs
extern const uint8_t LED_PIN_CH_N[];
extern const uint8_t LED_PIN_CH_N_size;
extern const uint8_t LED_CH_N[];
extern const uint8_t LED_CH_N_size;

// List of PARAMETER NAMES only used for API
extern const char *const api_all_param_names[];
extern const uint8_t api_all_param_names_size;

// WEB URI defaults
extern const char *const web_uri_defult_page;     // If request equals "/<path>/" -> it is interpreted as "/<path>/index.html"
extern const char *const web_uri_defult_404_page; // If request not fond return "pnf.html"

// API HTTP respond messages
extern const char *const http_response_message_error[];
extern const char *const http_response_message_warning[];
extern const char *const http_response_message_ok[];

/***************************************************************************************************************/
#endif // LEDC_VARIABLES_H