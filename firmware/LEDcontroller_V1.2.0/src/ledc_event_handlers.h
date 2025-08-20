#ifndef LEDC_EVENT_HANDLERS_H
#define LEDC_EVENT_HANDLERS_H
/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_common.h"
#include "ledc_variables.h"
#include "ledc_functions.h"
#include "ledc_http_handlers.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** EVENT HANDLER INSTANCES ***/
extern esp_event_handler_instance_t handler_instance_wifi;
extern esp_event_handler_instance_t handler_instance_ip;
extern esp_event_handler_instance_t handler_instance_http_server;

/***************************************************************************************************************/
/*** FUNCTION DECLARATIONS *************************************************************************************/

/*** EVENT HANDLER ***/
void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/***************************************************************************************************************/
#endif // LEDC_EVENT_HANDLERS_H