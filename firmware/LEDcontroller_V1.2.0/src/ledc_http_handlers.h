#ifndef LEDC_HTTP_HANDLERS_H
#define LEDC_HTTP_HANDLERS_H
/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_common.h"
#include "ledc_variables.h"
#include "ledc_functions.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** HTTP SERVER HANDLES ***/
extern httpd_handle_t http_server_handle;

/*** HTTP SERVER STRUCTURES FOR URI HANDLERS***/
// OPTIONS handlers (for HTTP_OPTIONS header to allow CORS)
extern httpd_uri_t options_api_ch;
extern httpd_uri_t options_api_ch_all;
extern httpd_uri_t options_api_conf;
extern httpd_uri_t options_api_cred;
extern httpd_uri_t options_api_reboot;
// API handlers
extern httpd_uri_t post_api_ch;
extern httpd_uri_t post_api_ch_all;
extern httpd_uri_t post_api_conf;
extern httpd_uri_t post_api_cred;
extern httpd_uri_t post_api_reboot;
extern httpd_uri_t get_api_ch;
extern httpd_uri_t get_api_ch_all;
extern httpd_uri_t get_api_conf;
// WEB handlers
extern httpd_uri_t get_web;

/***************************************************************************************************************/
/*** FUNCTION DECLARATIONS *************************************************************************************/

/*** HTTP SERVER ***/
// OPTIONS handlers (for HTTP_OPTIONS header to allow CORS)
esp_err_t options_api_handler(httpd_req_t *req);
// API handlers
esp_err_t post_api_ch_handler(httpd_req_t *req);
esp_err_t post_api_ch_all_handler(httpd_req_t *req);
esp_err_t post_api_conf_handler(httpd_req_t *req);
esp_err_t post_api_cred_handler(httpd_req_t *req);
esp_err_t post_api_reboot_handler(httpd_req_t *req);
esp_err_t get_api_ch_handler(httpd_req_t *req);
esp_err_t get_api_ch_all_handler(httpd_req_t *req);
esp_err_t get_api_conf_handler(httpd_req_t *req);
// WEB handlers
esp_err_t get_web_handler(httpd_req_t *req);

/***************************************************************************************************************/
#endif // LEDC_HTTP_HANDLERS_H