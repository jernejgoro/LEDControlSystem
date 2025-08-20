#ifndef LEDC_TASKS_H
#define LEDC_TASKS_H
/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_common.h"
#include "ledc_variables.h"
#include "ledc_functions.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** TASK HANDLES ***/
extern TaskHandle_t DeviceMainTaskHandle;
extern TaskHandle_t ButtonTaskHandle;
extern TaskHandle_t WifiBlinkTaskHandle;
extern TaskHandle_t ErrorBlinkTaskHandle;
extern TaskHandle_t FactoryResetBlinkTaskHandle;
extern TaskHandle_t SpiffsAutoSaveTaskHandle;
extern TaskHandle_t WiFiReconnectTaskHandle;
extern TaskHandle_t ChannelsDutyChangeTaskHandle;
extern TaskHandle_t ChannelsControlTaskHandle;

/***************************************************************************************************************/
/*** TASK DECLARATIONS *****************************************************************************************/

/*** DEVICE MAIN TASK ***/
void device_main_task(void *pvParameters);

/*** BUTTON TASK ***/
void button_task(void *pvParameters);

/*** WIFI BLINK TASK ***/
void wifi_blink_task(void *pvParameters);

/*** ERROR BLINK TASK ***/
void error_blink_task(void *pvParameters);

/*** FACTORY RESET BLINK TASK ***/
void factory_reset_blink_task(void *pvParameters);

/*** SPIFFS AUTO SAVE TASK ***/
void spiffs_auto_save_channel_data_task(void *pvParameters);

/*** WIFI RECONNECT TASK ***/
void wifi_reconnect_task(void *pvParameters);

/*** CHANNELS CONTROL & DUTY CHANGE TASK ***/
void channels_duty_change_task(void *pvParameters);
void channels_control_task(void *pvParameters);

/***************************************************************************************************************/
#endif // LEDC_TASKS_H