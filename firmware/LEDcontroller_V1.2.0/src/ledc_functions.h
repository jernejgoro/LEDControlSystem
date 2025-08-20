#ifndef LEDC_FUNCTIONS_H
#define LEDC_FUNCTIONS_H
/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_common.h"
#include "ledc_variables.h"
#include "ledc_tasks.h"
#include "ledc_http_handlers.h"
#include "ledc_event_handlers.h"

/***************************************************************************************************************/
/*** FUNCTION DECLARATIONS *************************************************************************************/

/*** EVENT HANDLER ***/
uint8_t event_handler_init(void);

/*** WIFI ***/
uint8_t wifi_station_init(void);
uint8_t wifi_ap_init(void);
void device_wifi_station_start(void);
void device_wifi_ap_start(void);
void device_wifi_stop(void);

/*** HTTP SERVER ***/
uint8_t start_http_server(void);

/*** GPIO & PERIPHERALS ***/
uint8_t gpio_init(void);
uint8_t pwm_init(void);
uint8_t get_button_state(uint8_t button_gpio_num);

/*** JSON ***/
uint8_t json_start_read_config(void);
uint8_t json_start_read_channel_data(void);
uint8_t json_start_read_credentials(void);
uint8_t json_start_read(void);

/*** STORAGE ***/
uint8_t nvs_init(void);
uint8_t spiffs_init(void);
uint8_t read_spiffs_file_to_buffer(const char *path, char *buffer, const uint16_t buffer_len);
uint8_t write_buffer_to_spiffs_file(const char *path, const char *buffer);

/*** ERROR HANDLER FUNCTION ***/
void error_handler_function(uint8_t error);

/*** FACTORY RESET FUNCTION ***/
uint8_t factory_reset_function(void);

/***  MISCELLANEOUS FUNCTIONS ***/
uint8_t path_parse(const char *full_path, char *file_name_prefix, char *file_name, uint16_t path_name_size, uint16_t file_name_size);

/***************************************************************************************************************/
#endif // LEDC_FUNCTIONS_H