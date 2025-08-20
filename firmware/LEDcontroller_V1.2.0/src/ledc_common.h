#ifndef LEDC_COMMON_H
#define LEDC_COMMON_H
/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/

/*** FREERTOS ***/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*** WIFI ***/
#include "esp_wifi.h"

/*** HTTP SERVER ***/
#include "esp_http_server.h"

/*** GPIO & PERIPHERALS ***/
#include "driver/gpio.h"
#include "driver/ledc.h" // Channels PWM control

/*** STORAGE ***/
#include "nvs_flash.h"  // NVS partition for WiFi credentials
#include "esp_spiffs.h" // SPIFFS partition for data storage

/*** JSON ***/
#include "cJSON.h"

/*** LOGGING ***/
#include "esp_log.h"

/***************************************************************************************************************/
/*** DEFINITIONS ***********************************************************************************************/

/*** EVENT HANDLER ***/
// Return values
#define EVENT_HANDLER_INIT_OK 0
#define EVENT_HANDLER_INIT_ERROR 1

/*** WIFI ***/
// Configuration
#define WIFI_AUTHMODE_TRESHOLD WIFI_AUTH_WPA2_PSK
#define WIFI_RECONNECTING_ATTEMPTS 3
// Return values
#define WIFI_STATION_INIT_OK 0
#define WIFI_STATION_INIT_ERROR 1
#define WIFI_AP_INIT_OK 0
#define WIFI_AP_INIT_ERROR 1
// WiFi modes
#define DEVICE_WIFI_MODE_NONE 0
#define DEVICE_WIFI_MODE_AP 1
#define DEVICE_WIFI_MODE_STATION 2

/*** HTTP SERVER ***/
// Configuration
#define HTTP_SERVER_PORT 80
#define HTTP_MAX_REQUEST_LEN 100
#define HTTP_MAX_RESPOND_LEN 600
#define HTTP_MAX_POST_DATA_LEN 400
#define HTTP_SERVER_MAX_URI_HANDLERS 15
#define HTTP_MAX_URI_LEN HTTPD_MAX_URI_LEN
// Return values
#define START_HTTP_SERVER_OK 0
#define START_HTTP_SERVER_ERROR 1

/*** GPIO & PERIPHERALS ***/
// Configuration
#define PWM_FREQ 5000
#define PWM_RESOLUTION 10
#define PWM_FADE_TIME_MS 1500
#define PWM_TRANSITION_STEPS 150
// GPIOs for LED indicators
#define GPIO_LED_WIFI 26  // GPIO number to which BLUE LED is connected
#define GPIO_LED_ERROR 27 // GPIO number to which RED LED is connected
// GPIOs for buttons
#define GPIO_BUTTON_1 25             // GPIO number to which BUTTON is connected
#define GPIO_BUTTON_PRESSED_VALUE 0  // GPIO value when button is pressed
#define GPIO_BUTTON_RELEASED_VALUE 1 // GPIO value when button is released
// Button states
#define BUTTON_STATE_ERROR 0
#define BUTTON_STATE_PRESSED 1
#define BUTTON_STATE_RELEASED 2
#define BUTTON_STATE_POSEDGE 3
#define BUTTON_STATE_NEGEDGE 4
// GPIOs for PWM channels
#define GPIO_CH_0 4
#define GPIO_CH_1 16
#define GPIO_CH_2 17
#define GPIO_CH_3 18
#define GPIO_CH_4 19
#define GPIO_CH_5 21
// PWM channels
#define PWM_CH_0 LEDC_CHANNEL_0
#define PWM_CH_1 LEDC_CHANNEL_1
#define PWM_CH_2 LEDC_CHANNEL_2
#define PWM_CH_3 LEDC_CHANNEL_3
#define PWM_CH_4 LEDC_CHANNEL_4
#define PWM_CH_5 LEDC_CHANNEL_5
// Return values
#define GPIO_INIT_OK 0
#define GPIO_INIT_ERROR 1
#define PWM_INIT_OK 0
#define PWM_INIT_ERROR 1

/*** JSON ***/
// Configuration
#define JSON_CONFIG_FILE_CH_DATA_PATH "/storage/channel_data.json"         // Stores channels last values
#define JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH "/storage/credentials.json" // Stores WiFi credentials
#define JSON_CONFIG_FILE_CONFIGURATION_PATH "/storage/config.json"         // Stores configuration
#define JSON_CONFIG_FILE_MAX_LEN 400
// Return values
#define JSON_START_READ_OK 0
#define JSON_START_READ_ERROR 1

/*** STORAGE ***/
// Configuration
#define SPIFFS_PARTITION_BASE_PATH "/storage" // Name of SPIFFS partition
#define SPIFFS_PARTITION_MAX_FILES 20
// Return values
#define NVS_INIT_OK 0
#define NVS_INIT_ERROR 1
#define SPIFFS_INIT_OK 0
#define SPIFFS_INIT_ERROR 1
#define READ_SPIFFS_OK 0
#define READ_SPIFFS_ERROR 1
#define WRITE_SPIFFS_OK 0
#define WRITE_SPIFFS_ERROR 1

/*** SPIFFS AUTO SAVE TASK ***/
// Configuration
#define SPIFF_AUTO_SAVE_PERIOD_MS 30000 // 30 sec

/*** ERROR HANDLER FUNCTION ***/
// Function parameters
#define CRITICAL_ERROR_GPIO 1
#define CRITICAL_ERROR_NVS 2
#define CRITICAL_ERROR_SPIFFS 3
#define CRITICAL_ERROR_JSON 4
#define CRITICAL_ERROR_PWM 5
#define CRITICAL_ERROR_EVENT_HANDLER 6
#define CRITICAL_ERROR_WIFI 7
// Error levels
#define ERROR_LEVEL_NONE 0
#define ERROR_LEVEL_WARNING 1
#define ERROR_LEVEL_WIFI 2
#define ERROR_LEVEL_CRITICAL 3

/*** FACTORY RESET FUNCTION & BLINK TASK***/
// Configuration
#define FACTORY_RESET_DEFAULT_DUTY 100
#define FACTORY_RESET_DEFAULT_ENABLE 1
#define FACTORY_RESET_DEFAULT_RGB_OUTPUTS 0
#define FACTORY_RESET_DEFAULT_W_OUTPUTS 6
#define FACTORY_RESET_DEFAULT_ENABLE_WIFI 1
// Return values
#define FACTORY_RESET_OK 0
#define FACTORY_RESET_ERROR 1
// Factory reset states
#define FACTOR_RESET_STATE_NONE 0
#define FACTORY_RESET_STATE_BEGIN 1
#define FACTORY_RESET_STATE_VALID 2
#define FACTORY_RESET_STATE_INVALID 3
#define FACTORY_RESET_STATE_RESETTING 4
#define FACTORY_RESET_STATE_ERROR 5
#define FACTORY_RESET_STATE_ABORT 6
#define FACTORY_RESET_STATE_COMPLETE 7

/***  MISCELLANEOUS FUNCTIONS ***/
// Return values
#define PATH_PARSE_OK 0
#define PATH_PARSE_ERROR 1

/*** GENERAL CONFIGURATION ***/
#define NO_CHANNELS_LIMIT 6                          // MIN: 1, MAX: max supported by esp32
#define NO_RGB_OUTPUTS_LIMIT (NO_CHANNELS_LIMIT / 3) // Maximum number of RGB outputs (= 3 channels)
#define NO_W_OUTPUTS_LIMIT NO_CHANNELS_LIMIT         // Maximum number of W outputs (= 1 channel)
#define DUTY_MAX_VALUE ((1 << PWM_RESOLUTION) - 1)   // Maximum value of DUTY parameter
#define ENABLE_MAX_VALUE 1                           // Maximum value of ENABLE parameter

#define MAX_QUERY_PARAMS 5     // Maximum number of QUERY parameters in GET request
#define MAX_POST_PARAMS 10     // Maximum number of parameters send by POST request
#define CREDENTIALS_MAX_LEN 30 // Maximum length of ssid / password

#define WIFI_MODE_SWITCH_AP_MIN_TIME_MS 5000         // Minimum button press duration to switch from or to AP wifi mode
#define WIFI_MODE_SWITCH_MIN_TIME_MS 100             // Minimum button press duration to switch STATION / NONE wifi mode
#define WIFI_AP_MODE_MAX_INACTIVITY_TIME_MS 120000UL // Maximum time of inactivity after which the AP mode should stop and switch to STATION / NONE mode
#define FACTORY_RESET_MIN_TIME_MS 10000              // Minimum button press duration to factory reset device
#define FACTORY_RESET_WINDOW_TIME_MS 5000            // Time window (after minimum button press time) when factory reset is valid

/***************************************************************************************************************/
#endif // LEDC_COMMON_H