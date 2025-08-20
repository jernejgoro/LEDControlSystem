/***************************************************************************************************************/
/*** INCLUDED LIBRARIES ****************************************************************************************/
#include "ledc_http_handlers.h"

/***************************************************************************************************************/
/*** VARIABLES *************************************************************************************************/

/*** HTTP SERVER HANDLES ***/
httpd_handle_t http_server_handle = NULL;

/*** HTTP SERVER STRUCTURES FOR URI HANDLERS***/
// OPTIONS handlers (for HTTP_OPTIONS header to allow CORS)
httpd_uri_t options_api_ch = {
    .uri = "/api/ch",
    .method = HTTP_OPTIONS,
    .handler = options_api_handler,
    .user_ctx = NULL,
};

httpd_uri_t options_api_ch_all = {
    .uri = "/api/ch/all",
    .method = HTTP_OPTIONS,
    .handler = options_api_handler,
    .user_ctx = NULL,
};

httpd_uri_t options_api_conf = {
    .uri = "/api/conf",
    .method = HTTP_OPTIONS,
    .handler = options_api_handler,
    .user_ctx = NULL,
};

httpd_uri_t options_api_cred = {
    .uri = "/api/cred",
    .method = HTTP_OPTIONS,
    .handler = options_api_handler,
    .user_ctx = NULL,
};

httpd_uri_t options_api_reboot = {
    .uri = "/api/reboot",
    .method = HTTP_OPTIONS,
    .handler = options_api_handler,
    .user_ctx = NULL,
};

// API handlers
httpd_uri_t post_api_ch = {
    .uri = "/api/ch",
    .method = HTTP_POST,
    .handler = post_api_ch_handler,
    .user_ctx = NULL,
};

httpd_uri_t post_api_ch_all = {
    .uri = "/api/ch/all",
    .method = HTTP_POST,
    .handler = post_api_ch_all_handler,
    .user_ctx = NULL,
};

httpd_uri_t post_api_conf = {
    .uri = "/api/conf",
    .method = HTTP_POST,
    .handler = post_api_conf_handler,
    .user_ctx = NULL,
};

httpd_uri_t post_api_cred = {
    .uri = "/api/cred",
    .method = HTTP_POST,
    .handler = post_api_cred_handler,
    .user_ctx = NULL,
};

httpd_uri_t post_api_reboot = {
    .uri = "/api/reboot",
    .method = HTTP_POST,
    .handler = post_api_reboot_handler,
    .user_ctx = NULL,
};

httpd_uri_t get_api_ch = {
    .uri = "/api/ch",
    .method = HTTP_GET,
    .handler = get_api_ch_handler,
    .user_ctx = NULL,
};

httpd_uri_t get_api_ch_all = {
    .uri = "/api/ch/all",
    .method = HTTP_GET,
    .handler = get_api_ch_all_handler,
    .user_ctx = NULL,
};

httpd_uri_t get_api_conf = {
    .uri = "/api/conf",
    .method = HTTP_GET,
    .handler = get_api_conf_handler,
    .user_ctx = NULL,
};

// WEB handlers
httpd_uri_t get_web = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = get_web_handler,
    .user_ctx = NULL,
};

/***************************************************************************************************************/
/*** FUNCTION DEFINITIONS **************************************************************************************/

// OPTIONS handlers (for HTTP_OPTIONS header to allow CORS)
esp_err_t options_api_handler(httpd_req_t *req) // COMPLETE
{
    // printf("options_api_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// API handlers
esp_err_t post_api_ch_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Update chN_duty and/or chN_enable values of channels specified in received data (JSON)
     *  Return all channel values in JSON format
     *  In case of error return: "Error: <reason>"
     */

    // printf("post_api_ch_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Buffer for received data
    char data_buffer[HTTP_MAX_POST_DATA_LEN + 1] = "0";

    // Get received data size (upper limit for reading: HTTP_MAX_POST_DATA_LEN)
    size_t recv_size = (req->content_len);
    if (recv_size > HTTP_MAX_POST_DATA_LEN) // Received data size exceeded -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[1], strlen(http_response_message_error[1]));
        return ESP_OK;
    }

    // Write received data to data_buffer
    int post_data_len = httpd_req_recv(req, data_buffer, recv_size);
    data_buffer[post_data_len] = '\0';
    if (post_data_len < 0) // Error -> send a response back to the client
    {
        ESP_LOGW(TAG_HTTP_SERVER, "Warning httpd_req_recv returned: %d", post_data_len);
        if (post_data_len == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req); // Send HTTP 408 (request timeout)
        }
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_FAIL; // Returning ESP_FAIL will ensure that the underlying socket is closed
    }
    else if (post_data_len == 0) // No data received -> send a response back to the client
    {
        ESP_LOGI(TAG_HTTP_SERVER, "No data received");
        httpd_resp_send(req, http_response_message_error[5], strlen(http_response_message_error[5]));
        return ESP_OK;
    }
    else // Data received
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Received data (POST): >>>%s<<<", data_buffer);

        cJSON *json = cJSON_Parse(data_buffer);
        if (json != NULL) // Parsing successful
        {
            uint8_t valid_cnt = 0;
            uint8_t param_cnt = 0;

            // Count json elements
            cJSON *item = json->child;
            while (item)
            {
                param_cnt++;
                // If number of parametrs received exceeded the limit -> send a response back to the client
                if (param_cnt > MAX_POST_PARAMS)
                {
                    httpd_resp_send(req, http_response_message_warning[2], strlen(http_response_message_warning[2]));
                    return ESP_OK;
                }

                // Update chN_duty and/or chN_enable values of channels specified in received data (LIMITED TO: device_conf.channels_used)
                for (uint8_t i = 0; (i < channel_data_name_duty_size) && (i < device_conf.channels_used); i++)
                {
                    // Update "duty" value of appropriate channel
                    if (strcmp(item->string, channel_data_name_duty[i]) == 0)
                    {
                        uint16_t val = 0;
                        val = (uint16_t)(item->valueint);
                        *channel_value_duty[i] = (val < DUTY_MAX_VALUE) ? val : DUTY_MAX_VALUE;
                        valid_cnt++;
                        break;
                    }
                    // Update "enable" value of appropriate channel
                    if (strcmp(item->string, channel_data_name_enable[i]) == 0)
                    {
                        uint16_t val = 0;
                        val = (uint16_t)(item->valueint);
                        *channel_value_enable[i] = (val < ENABLE_MAX_VALUE) ? val : ENABLE_MAX_VALUE;
                        valid_cnt++;
                        break;
                    }
                }
                item = item->next;
            }

            // At least one parameter is not valid -> send a response back to the client
            if ((valid_cnt == 0) || (param_cnt != valid_cnt))
            {
                httpd_resp_send(req, http_response_message_warning[3], strlen(http_response_message_warning[3]));
                return ESP_OK;
            }
        }
        else // Parsing NOT successful -> send a response back to the client
        {
            httpd_resp_send(req, http_response_message_error[4], strlen(http_response_message_error[4]));
            return ESP_OK;
        }
    }

    // Prepare response_buffer (LIMITED TO: device_conf.channels_used)
    char response_buffer[HTTP_MAX_RESPOND_LEN + 61] = "0";
    response_buffer[HTTP_MAX_RESPOND_LEN + 60] = '\0';
    uint16_t cnt = 0;
    cnt = sprintf(response_buffer, "{ ");
    for (uint8_t i = 0; (i < channel_data_name_duty_size) && (i < device_conf.channels_used); i++)
    {
        cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[i], *channel_value_duty[i]);
        cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[i], *channel_value_enable[i]);

        if (cnt > HTTP_MAX_RESPOND_LEN) // Buffer overflow protection -> send a response back to the client
        {
            ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: post_api_ch_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
            httpd_resp_send(req, http_response_message_warning[12], strlen(http_response_message_warning[12]));
            return ESP_OK;
        }
    }
    sprintf(response_buffer + cnt - 3, " }");

    // Send response back to the client
    httpd_resp_send(req, response_buffer, strlen(response_buffer));
    return ESP_OK;
}

esp_err_t post_api_ch_all_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Update all chN_duty and/or chN_enable values
     *  Return all channel values in JSON format
     *  In case of error return: "Error: <reason>"
     */

    // printf("post_api_ch_all_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Buffer for received data
    char data_buffer[HTTP_MAX_POST_DATA_LEN + 1] = "0";

    // Get received data size (upper limit for reading: HTTP_MAX_POST_DATA_LEN)
    size_t recv_size = (req->content_len);
    if (recv_size > HTTP_MAX_POST_DATA_LEN) // Received data size exceeded -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[1], strlen(http_response_message_error[1]));
        return ESP_OK;
    }

    // Write received data to data_buffer
    int post_data_len = httpd_req_recv(req, data_buffer, recv_size);
    data_buffer[post_data_len] = '\0';
    if (post_data_len < 0) // Error -> send a response back to the client
    {
        ESP_LOGW(TAG_HTTP_SERVER, "Warning httpd_req_recv returned: %d", post_data_len);
        if (post_data_len == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req); // Send HTTP 408 (request timeout)
        }
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_FAIL; // Returning ESP_FAIL will ensure that the underlying socket is closed
    }
    else if (post_data_len == 0) // No data received -> send a response back to the client
    {
        ESP_LOGI(TAG_HTTP_SERVER, "No data received");
        httpd_resp_send(req, http_response_message_error[5], strlen(http_response_message_error[5]));
        return ESP_OK;
    }
    else // Data received
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Received data (POST): >>>%s<<<", data_buffer);

        cJSON *json = cJSON_Parse(data_buffer);
        if (json != NULL) // Parsing successful
        {
            uint8_t valid_cnt = 0;
            uint8_t param_cnt = 0;

            // Count json elements
            cJSON *item = json->child;
            while (item)
            {
                param_cnt++;
                // If number of parametrs received exceeded the limit -> send a response back to the client
                if (param_cnt > MAX_POST_PARAMS)
                {
                    httpd_resp_send(req, http_response_message_warning[2], strlen(http_response_message_warning[2]));
                    return ESP_OK;
                }

                // Update all chN_duty and/or chN_enable values of channels specified in received data (LIMITED TO: device_conf.channels_used)
                for (uint8_t i = 0; i < api_all_param_names_size; i++)
                {
                    if (strcmp(item->string, api_all_param_names[i]) == 0)
                    {
                        for (uint8_t j = 0; (j < channel_value_duty_size) && (j < device_conf.channels_used); j++)
                        {
                            uint16_t val = 0;
                            val = (uint16_t)(item->valueint);
                            // Update "duty" value of all channels
                            if (i == 0)
                            {
                                val = (val < DUTY_MAX_VALUE) ? val : DUTY_MAX_VALUE;
                                *channel_value_duty[j] = val;
                            }
                            // Update "enable" value of all channels
                            else if (i == 1)
                            {
                                val = (val < ENABLE_MAX_VALUE) ? val : ENABLE_MAX_VALUE;
                                *channel_value_enable[j] = val;
                            }
                        }
                        valid_cnt++;
                    }
                }
                item = item->next;
            }
            ESP_LOGI(TAG_HTTP_SERVER, "Received %d valid value(s)", valid_cnt);

            // At least one parameter is not valid -> send a response back to the client
            if ((valid_cnt == 0) || (param_cnt != valid_cnt))
            {
                httpd_resp_send(req, http_response_message_warning[3], strlen(http_response_message_warning[3]));
                return ESP_OK;
            }
        }
        else // Parsing NOT successful -> send a response back to the client
        {
            httpd_resp_send(req, http_response_message_error[4], strlen(http_response_message_error[4]));
            return ESP_OK;
        }
    }

    // Prepare response_buffer (LIMITED TO: device_conf.channels_used)
    char response_buffer[HTTP_MAX_RESPOND_LEN + 61] = "0";
    response_buffer[HTTP_MAX_RESPOND_LEN + 60] = '\0';
    uint16_t cnt = 0;
    cnt = sprintf(response_buffer, "{ ");
    for (uint8_t i = 0; (i < channel_data_name_duty_size) && (i < device_conf.channels_used); i++)
    {
        cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[i], *channel_value_duty[i]);
        cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[i], *channel_value_enable[i]);

        if (cnt > HTTP_MAX_RESPOND_LEN) // Buffer overflow protection -> send a response back to the client
        {
            ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: post_api_ch_all_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
            httpd_resp_send(req, http_response_message_warning[12], strlen(http_response_message_warning[12]));
            return ESP_OK;
        }
    }
    if (device_conf.channels_used == 0)
    {
        cnt += 2;
    }
    sprintf(response_buffer + cnt - 3, " }");

    // Send response back to the client
    httpd_resp_send(req, response_buffer, strlen(response_buffer));
    return ESP_OK;
}

esp_err_t post_api_conf_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Update configuration in config.json
     *  Reboot is needed to apply settings
     *  Return <http_response_message_ok>
     *  In case of error return: "Error: <reason>"
     */

    // printf("post_api_conf_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Buffer for received data
    char data_buffer[HTTP_MAX_POST_DATA_LEN + 1] = "0";

    // Get received data size (upper limit for reading: HTTP_MAX_POST_DATA_LEN)
    size_t recv_size = (req->content_len);
    if (recv_size > HTTP_MAX_POST_DATA_LEN) // Received data size exceeded -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[1], strlen(http_response_message_error[1]));
        return ESP_OK;
    }

    // To temporarily store received configuration
    struct device_config_struct device_conf_tmp;
    uint16_t *const device_conf_tmp_ptr[] = {&device_conf_tmp.rgb_outputs, &device_conf_tmp.w_outputs, &device_conf_tmp.enable_wifi};

    // Write received data to data_buffer
    int post_data_len = httpd_req_recv(req, data_buffer, recv_size);
    data_buffer[post_data_len] = '\0';
    if (post_data_len < 0) // Error -> send a response back to the client
    {
        ESP_LOGW(TAG_HTTP_SERVER, "Warning httpd_req_recv returned: %d", post_data_len);
        if (post_data_len == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req); // Send HTTP 408 (request timeout)
        }
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_FAIL; // Returning ESP_FAIL will ensure that the underlying socket is closed
    }
    else if (post_data_len == 0) // No data received -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[5], strlen(http_response_message_error[5]));
        return ESP_OK;
    }
    else // Data received
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Received data (POST): >>>%s<<<", data_buffer);

        cJSON *json = cJSON_Parse(data_buffer);
        if (json != NULL) // Parsing successful
        {
            // Count json elements
            uint8_t param_cnt = 0;
            cJSON *item = json->child;
            while (item)
            {
                param_cnt++;
                // If number of parametrs received exceeded the limit -> send a response back to the client
                if (param_cnt > config_name_size)
                {
                    httpd_resp_send(req, http_response_message_error[2], strlen(http_response_message_error[2]));
                    return ESP_OK;
                }
                item = item->next;
            }

            // If number of parametrs received less than expected -> send a response back to the client
            if (param_cnt < config_name_size)
            {
                httpd_resp_send(req, http_response_message_error[6], strlen(http_response_message_error[6]));
                return ESP_OK;
            }

            // Read all values and save to temporary variables
            for (uint8_t i = 0; i < config_name_size; i++)
            {
                cJSON *config_param_json = cJSON_GetObjectItem(json, config_name[i]);
                // At least one parameter is not valid -> send a response back to the client
                if (config_param_json == NULL)
                {
                    httpd_resp_send(req, http_response_message_error[3], strlen(http_response_message_error[3]));
                    return ESP_OK;
                }
                // Save to temporary variable
                *device_conf_tmp_ptr[i] = (uint16_t)(config_param_json->valueint);
            }

            // If config parameter values not within limits -> send a response back to the client
            if ((device_conf_tmp.rgb_outputs > NO_RGB_OUTPUTS_LIMIT) || (device_conf_tmp.w_outputs > (NO_W_OUTPUTS_LIMIT - 3 * device_conf_tmp.rgb_outputs)) || (device_conf_tmp.enable_wifi > ENABLE_MAX_VALUE))
            {
                httpd_resp_send(req, http_response_message_error[7], strlen(http_response_message_error[7]));
                return ESP_OK;
            }
        }
        else // Parsing NOT successful -> send a response back to the client
        {
            httpd_resp_send(req, http_response_message_error[4], strlen(http_response_message_error[4]));
            return ESP_OK;
        }
    }

    // Prepare config_buffer
    char config_buffer[JSON_CONFIG_FILE_MAX_LEN + 61] = "0";
    config_buffer[JSON_CONFIG_FILE_MAX_LEN + 60] = '\0';
    uint16_t cnt = 0;
    cnt = sprintf(config_buffer, "{ ");
    for (uint8_t i = 0; i < config_name_size; i++)
    {
        cnt += sprintf(config_buffer + cnt, "\"%s\":%d , ", config_name[i], *device_conf_tmp_ptr[i]);

        // Buffer overflow protection -> send a response back to the client
        if (cnt > JSON_CONFIG_FILE_MAX_LEN)
        {
            ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: post_api_conf_handler (%d chars, but %d is upper limit)", cnt, JSON_CONFIG_FILE_MAX_LEN);
            httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
            return ESP_OK;
        }
    }
    sprintf(config_buffer + cnt - 3, " }");
    // printf("\n\r>>>%s<<<\n\r", config_buffer);

    // Write config_buffer to the config.json
    ESP_LOGI(TAG_HTTP_SERVER, "Writing config_buffer to file: %s", JSON_CONFIG_FILE_CONFIGURATION_PATH);
    switch (write_buffer_to_spiffs_file(JSON_CONFIG_FILE_CONFIGURATION_PATH, config_buffer))
    {
    case WRITE_SPIFFS_OK:
    {
        // Writing configuration to config.json successful
        break;
    }
    default:
    {
        // Error occurred when trying to write config_buffer to config.json -> send a response back to the client
        ESP_LOGE(TAG_HTTP_SERVER, "Error writing file to SPIFFS");
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_OK;
    }
    }

    // Send response back to the client
    httpd_resp_send(req, http_response_message_ok[0], strlen(http_response_message_ok[0]));
    return ESP_OK;
}

esp_err_t post_api_cred_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Update credentials in credentials.json
     *  Reboot is needed to apply settings
     *  Return <http_response_message_ok>
     *  In case of error return: "Error: <reason>"
     */

    // printf("post_api_cred_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Buffer for received data
    char data_buffer[HTTP_MAX_POST_DATA_LEN + 1] = "0";

    // Get received data size (upper limit for reading: HTTP_MAX_POST_DATA_LEN)
    size_t recv_size = (req->content_len);
    if (recv_size > HTTP_MAX_POST_DATA_LEN) // Received data size exceeded -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[1], strlen(http_response_message_error[1]));
        return ESP_OK;
    }

    // To temporarily store received credentials
    uint8_t wifi_ssid_tmp[CREDENTIALS_MAX_LEN + 2] = "0";
    uint8_t wifi_password_tmp[CREDENTIALS_MAX_LEN + 2] = "0";
    uint8_t *const credentials_value_tmp[] = {wifi_ssid_tmp, wifi_password_tmp};

    // Write received data to data_buffer
    int post_data_len = httpd_req_recv(req, data_buffer, recv_size);
    data_buffer[post_data_len] = '\0';
    if (post_data_len < 0) // Error -> send a response back to the client
    {
        ESP_LOGW(TAG_HTTP_SERVER, "Warning httpd_req_recv returned: %d", post_data_len);
        if (post_data_len == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req); // Send HTTP 408 (request timeout)
        }
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_FAIL; // Returning ESP_FAIL will ensure that the underlying socket is closed
    }
    else if (post_data_len == 0) // No data received -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[5], strlen(http_response_message_error[5]));
        return ESP_OK;
    }
    else // Data received
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Received data (POST): >>>%s<<<", data_buffer);

        cJSON *json = cJSON_Parse(data_buffer);
        if (json != NULL) // Parsing successful
        {
            // Count json elements
            uint8_t param_cnt = 0;
            cJSON *item = json->child;
            while (item)
            {
                param_cnt++;
                // If number of parametrs received exceeded the limit -> send a response back to the client
                if (param_cnt > credentials_name_size)
                {
                    httpd_resp_send(req, http_response_message_error[2], strlen(http_response_message_error[2]));
                    return ESP_OK;
                }
                item = item->next;
            }

            // If number of parametrs received less than expected -> send a response back to the client
            if (param_cnt < credentials_name_size)
            {
                httpd_resp_send(req, http_response_message_error[6], strlen(http_response_message_error[6]));
                return ESP_OK;
            }

            // Read all values and save to temporary variables
            for (uint8_t i = 0; i < credentials_name_size; i++)
            {
                cJSON *credentials_param_json = cJSON_GetObjectItem(json, credentials_name[i]);
                // At least one parameter is not valid -> send a response back to the client
                if (credentials_param_json == NULL)
                {
                    httpd_resp_send(req, http_response_message_error[3], strlen(http_response_message_error[3]));
                    return ESP_OK;
                }

                uint8_t credential_len = strlen((char *)credentials_param_json->valuestring);
                if (credential_len == 0) // If credential length equal zero -> send a response back to the client
                {
                    httpd_resp_send(req, http_response_message_error[13], strlen(http_response_message_error[13]));
                    return ESP_OK;
                }
                else if (credential_len <= CREDENTIALS_MAX_LEN) // If credential length less or equal to the limit -> save to the temporary variable
                {
                    memcpy(credentials_value_tmp[i], credentials_param_json->valuestring, credential_len + 1);
                }
                else // If credential length exceeded the limit -> send a response back to the client
                {
                    httpd_resp_send(req, http_response_message_error[8], strlen(http_response_message_error[8]));
                    return ESP_OK;
                }
            }
        }
        else // Parsing NOT successful
        {
            // Send a response back to the client
            httpd_resp_send(req, http_response_message_error[4], strlen(http_response_message_error[4]));
            return ESP_OK;
        }
    }

    // Prepare config_buffer
    char config_buffer[JSON_CONFIG_FILE_MAX_LEN + 61] = "0";
    config_buffer[JSON_CONFIG_FILE_MAX_LEN + 60] = '\0';
    uint16_t cnt = 0;
    cnt = sprintf(config_buffer, "{ ");
    for (uint8_t i = 0; i < credentials_name_size; i++)
    {
        cnt += sprintf(config_buffer + cnt, "\"%s\":\"%s\" , ", credentials_name[i], credentials_value_tmp[i]);

        // Buffer overflow protection -> send a response back to the client
        if (cnt > JSON_CONFIG_FILE_MAX_LEN)
        {
            ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: post_api_cred_handler (%d chars, but %d is upper limit)", cnt, JSON_CONFIG_FILE_MAX_LEN);
            httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
            return ESP_OK;
        }
    }
    sprintf(config_buffer + cnt - 3, " }");
    // printf("\n\r>>>%s<<<\n\r", config_buffer);

    // Write config_buffer to the credentials.json
    ESP_LOGI(TAG_HTTP_SERVER, "Writing config_buffer to file: %s", JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH);
    switch (write_buffer_to_spiffs_file(JSON_CONFIG_FILE_WIFI_CREDENTIALS_PATH, config_buffer))
    {
    case WRITE_SPIFFS_OK:
    {
        // Writing configuration to credentials.json successful
        break;
    }
    default:
    {
        // Error occurred when trying to write config_buffer to credentials.json -> send a response back to the client
        ESP_LOGE(TAG_HTTP_SERVER, "Error writing file to SPIFFS");
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_OK;
    }
    }

    // Send response back to the client
    httpd_resp_send(req, http_response_message_ok[0], strlen(http_response_message_ok[0]));
    return ESP_OK;
}

esp_err_t post_api_reboot_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Reboot device
     *  Return <http_response_message_ok>
     */

    // printf("post_api_reboot_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Send response back to the client
    httpd_resp_send(req, http_response_message_ok[1], strlen(http_response_message_ok[1]));

    // Restart device
    ESP_LOGI(TAG_HTTP_SERVER, "Received POST request: Rebooting ...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

esp_err_t get_api_ch_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Return requested channel values in JSON fromat
     *  If query NOT found, return values of all channels
     */

    // printf("get_api_ch_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Buffer for received query
    char query_buffer[HTTP_MAX_REQUEST_LEN + 1] = "0";
    query_buffer[HTTP_MAX_REQUEST_LEN] = '\0';

    // Response buffer
    char response_buffer[HTTP_MAX_RESPOND_LEN + 61] = "0";
    response_buffer[HTTP_MAX_RESPOND_LEN + 60] = '\0';
    uint16_t cnt = 0;
    cnt = sprintf(response_buffer, "{ ");

    // Get query length (upper limit: HTTP_MAX_REQUEST_LEN)
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len > HTTP_MAX_REQUEST_LEN) // Query length exceeded the limit -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[9], strlen(http_response_message_error[9]));
        return ESP_OK;
    }

    // Write query to the query_buffer
    switch (httpd_req_get_url_query_str(req, query_buffer, query_len + 1))
    {
    case ESP_OK: // Query found -> prepare response_buffer containing requested channel values
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Received query (GET): >>>%s<<<", query_buffer);
        uint8_t valid_cnt = 0;
        uint8_t param_cnt = 0;

        // Parse query
        char *param = strtok(query_buffer, "&");
        while (param != NULL)
        {
            param_cnt++;
            if (param_cnt > MAX_QUERY_PARAMS) // Number of requested parameters exceeded the limit -> send a response back to the client
            {
                httpd_resp_send(req, http_response_message_error[10], strlen(http_response_message_error[10]));
                return ESP_OK;
            }

            // Checks if query parameter match to any channel parameter -> write parameter and value to the response_buffer (LIMITED TO: device_conf.channels_used)
            for (uint8_t i = 0; (i < channel_data_name_duty_size) && (i < device_conf.channels_used); i++)
            {
                if (strcmp(param, channel_data_name_duty[i]) == 0)
                {
                    cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[i], *channel_value_duty[i]);
                    valid_cnt++;
                    break;
                }
                if (strcmp(param, channel_data_name_enable[i]) == 0)
                {
                    cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[i], *channel_value_enable[i]);
                    valid_cnt++;
                    break;
                }
            }
            // ESP_LOGI(TAG_HTTP_SERVER, "Parameter: %s", param);

            // Buffer overflow protection -> send a response back to the client
            if (cnt > HTTP_MAX_RESPOND_LEN)
            {
                ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: get_api_ch_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
                httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
                return ESP_OK;
            }

            // Get next query parameter
            param = strtok(NULL, "&");
        }
        ESP_LOGI(TAG_HTTP_SERVER, "Received %d valid value(s)", valid_cnt);

        if (valid_cnt && (param_cnt == valid_cnt)) // All received parameters are valid
        {
            sprintf(response_buffer + cnt - 3, " }");
        }
        else // One (or more) parameter is not valid -> send a response back to the client
        {
            httpd_resp_send(req, http_response_message_error[11], strlen(http_response_message_error[11]));
            return ESP_OK;
        }
        break;
    }
    case ESP_ERR_NOT_FOUND: // Query not found -> prepare response_buffer containing all channel values (LIMITED TO: device_conf.channels_used)
    {
        for (uint8_t i = 0; (i < channel_data_name_duty_size) && (i < device_conf.channels_used); i++)
        {
            cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[i], *channel_value_duty[i]);
            cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[i], *channel_value_enable[i]);

            // Buffer overflow protection -> send a response back to the client
            if (cnt > HTTP_MAX_RESPOND_LEN)
            {
                ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: get_api_ch_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
                httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
                return ESP_OK;
            }
        }
        if (device_conf.channels_used == 0)
        {
            cnt += 2;
        }
        sprintf(response_buffer + cnt - 3, " }");
        break;
    }
    default: // Error -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_OK;
    }
    }

    // Send response back to the client
    httpd_resp_send(req, response_buffer, strlen(response_buffer));
    return ESP_OK;
}

esp_err_t get_api_ch_all_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Returns all channel values in JSON fromat
     *  If query found, return only requested values of all channels
     */

    // printf("get_api_ch_all_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Buffer for received query
    char query_buffer[HTTP_MAX_REQUEST_LEN + 1] = "0";
    query_buffer[HTTP_MAX_REQUEST_LEN] = '\0';

    // Response buffer
    char response_buffer[HTTP_MAX_RESPOND_LEN + 61] = "0";
    response_buffer[HTTP_MAX_RESPOND_LEN + 60] = '\0';
    uint16_t cnt = 0;
    cnt = sprintf(response_buffer, "{ ");

    // Get query length (upper limit: HTTP_MAX_REQUEST_LEN)
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len > HTTP_MAX_REQUEST_LEN) // Query length exceeded the limit -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[9], strlen(http_response_message_error[9]));
        return ESP_OK;
    }

    // Write query to the query_buffer
    switch (httpd_req_get_url_query_str(req, query_buffer, query_len + 1))
    {
    case ESP_OK: // Query found -> prepare response_buffer containing requested channel values
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Received query (GET): >>>%s<<<", query_buffer);
        uint8_t valid_cnt = 0;
        uint8_t param_cnt = 0;

        // Parse query
        char *param = strtok(query_buffer, "&");
        while (param != NULL)
        {
            param_cnt++;
            if (param_cnt > MAX_QUERY_PARAMS) // Number of requested parameters exceeded the limit -> send a response back to the client
            {
                httpd_resp_send(req, http_response_message_error[10], strlen(http_response_message_error[10]));
                return ESP_OK;
            }

            // Checks if query parameter match to any channel parameters -> write parameters and values to the response_buffer (LIMITED TO: device_conf.channels_used)
            for (uint8_t i = 0; i < api_all_param_names_size; i++)
            {
                if (strcmp(param, api_all_param_names[i]) == 0)
                {
                    for (uint8_t j = 0; (j < channel_data_name_duty_size) && (j < device_conf.channels_used); j++)
                    {
                        if (i == 0) // write all "duty" parameters and values to the response_buffer
                        {
                            cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[j], *channel_value_duty[j]);
                        }
                        else if (i == 1) // write all "enable" parameters and values to the response_buffer
                        {
                            cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[j], *channel_value_enable[j]);
                        }
                    }
                    valid_cnt++;

                    // Buffer overflow protection -> send a response back to the client
                    if (cnt > HTTP_MAX_RESPOND_LEN)
                    {
                        ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: get_api_ch_all_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
                        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
                        return ESP_OK;
                    }
                }
            }
            // ESP_LOGI(TAG_HTTP_SERVER, "Parameter: %s", param);

            // Get next query parameter
            param = strtok(NULL, "&");
        }
        ESP_LOGI(TAG_HTTP_SERVER, "Received %d valid value(s)", valid_cnt);

        if (valid_cnt && (param_cnt == valid_cnt)) // All received parameters are valid
        {
            if (device_conf.channels_used == 0)
            {
                cnt += 2;
            }
            sprintf(response_buffer + cnt - 3, " }");
        }
        else // One (or more) parameter is not valid -> send a response back to the client
        {
            // Send a response back to the client
            httpd_resp_send(req, http_response_message_error[11], strlen(http_response_message_error[11]));
            return ESP_OK;
        }
        break;
    }
    case ESP_ERR_NOT_FOUND: // Query not found -> prepare response_buffer containing all channel values (LIMITED TO: device_conf.channels_used)
    {
        for (uint8_t i = 0; (i < channel_data_name_duty_size) && (i < device_conf.channels_used); i++)
        {
            cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_duty[i], *channel_value_duty[i]);
            cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", channel_data_name_enable[i], *channel_value_enable[i]);

            // Buffer overflow protection -> send a response back to the client
            if (cnt > HTTP_MAX_RESPOND_LEN)
            {
                ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: get_api_ch_all_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
                httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
                return ESP_OK;
            }
        }
        if (device_conf.channels_used == 0)
        {
            cnt += 2;
        }
        sprintf(response_buffer + cnt - 3, " }");
        break;
    }
    default: // Error -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_OK;
    }
    }

    // Send response back to the client
    httpd_resp_send(req, response_buffer, strlen(response_buffer));
    return ESP_OK;
}

esp_err_t get_api_conf_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  Returns all configuration parameters in JSON fromat
     *  If query found, return only requested values
     */

    // printf("get_api_conf_handler running on core %d\n\r", xPortGetCoreID());

    // Enable CORS (Cross-Origin Resource Sharing) -> append additional headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept");

    // Buffer for received query
    char query_buffer[HTTP_MAX_REQUEST_LEN + 1] = "0";
    query_buffer[HTTP_MAX_REQUEST_LEN] = '\0';

    // Response buffer
    char response_buffer[HTTP_MAX_RESPOND_LEN + 61] = "0";
    response_buffer[HTTP_MAX_RESPOND_LEN + 60] = '\0';
    uint16_t cnt = 0;
    cnt = sprintf(response_buffer, "{ ");

    // Get query length (upper limit: HTTP_MAX_REQUEST_LEN)
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len > HTTP_MAX_REQUEST_LEN) // Query length exceeded the limit -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[9], strlen(http_response_message_error[9]));
        return ESP_OK;
    }

    // Write query to the query_buffer
    switch (httpd_req_get_url_query_str(req, query_buffer, query_len + 1))
    {
    case ESP_OK: // Query found -> prepare response_buffer containing requested configuration parameters
    {
        ESP_LOGI(TAG_HTTP_SERVER, "Received query (GET): >>>%s<<<", query_buffer);
        uint8_t valid_cnt = 0;
        uint8_t param_cnt = 0;

        // Parse query
        char *param = strtok(query_buffer, "&");
        while (param != NULL)
        {
            param_cnt++;
            if (param_cnt > MAX_QUERY_PARAMS) // Number of requested parameters exceeded the limit -> send a response back to the client
            {
                httpd_resp_send(req, http_response_message_error[10], strlen(http_response_message_error[10]));
                return ESP_OK;
            }

            // Checks if query parameter match to any config parameter -> write parameter and value to the response_buffer
            for (uint8_t i = 0; i < config_name_size; i++)
            {
                if (strcmp(param, config_name[i]) == 0)
                {
                    cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", config_name[i], *config_value[i]);
                    valid_cnt++;
                }
            }
            // ESP_LOGI(TAG_HTTP_SERVER, "Parameter: %s", param);

            // Buffer overflow protection -> send a response back to the client
            if (cnt > HTTP_MAX_RESPOND_LEN)
            {
                ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: get_api_conf_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
                httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
                return ESP_OK;
            }

            // Get next query parameter
            param = strtok(NULL, "&");
        }
        ESP_LOGI(TAG_HTTP_SERVER, "Received %d valid value(s)", valid_cnt);

        if (valid_cnt && (param_cnt == valid_cnt)) // All parameters are valid
        {
            sprintf(response_buffer + cnt - 3, " }");
        }
        else // At least one parameter is not valid -> send a response back to the client
        {
            httpd_resp_send(req, http_response_message_error[11], strlen(http_response_message_error[11]));
            return ESP_OK;
        }
        break;
    }
    case ESP_ERR_NOT_FOUND: // Query not found -> prepare response_buffer containing all configuration parameters
    {
        for (uint8_t i = 0; i < config_name_size; i++)
        {
            cnt += sprintf(response_buffer + cnt, "\"%s\":%d , ", config_name[i], *config_value[i]);

            // Buffer overflow protection -> send a response back to the client
            if (cnt > HTTP_MAX_RESPOND_LEN)
            {
                ESP_LOGE(TAG_HTTP_SERVER, "Buffer overflow protection: get_api_conf_handler (%d chars, but %d is upper limit)", cnt, HTTP_MAX_RESPOND_LEN);
                httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
                return ESP_OK;
            }
        }
        sprintf(response_buffer + cnt - 3, " }");
        break;
    }
    default: // Error -> send a response back to the client
    {
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_OK;
    }
    }

    // Send response back to the client
    httpd_resp_send(req, response_buffer, strlen(response_buffer));
    return ESP_OK;
}

// WEB handlers
esp_err_t get_web_handler(httpd_req_t *req) // COMPLETE
{
    /**
     *  WEB handler that returns website content to user
     *  If content does not exist, "Oops! Page not found." page is returned to user
     */

    // printf("get_web_handler running on core %d\n\r", xPortGetCoreID());

    // URI length exceeded the limit -> send a response back to the client
    uint16_t uri_len = strlen(req->uri);
    if (uri_len > HTTP_MAX_URI_LEN)
    {
        httpd_resp_send(req, http_response_message_error[14], strlen(http_response_message_error[14]));
        return ESP_OK;
    }

    // Allocate file_name_prefix and file_name
    char *file_name_prefix = malloc(uri_len + 21);
    if (file_name_prefix == NULL) // Error allocating file_name_prefix -> send a response back to the client
    {
        httpd_resp_send_404(req);
        return ESP_OK;
    }
    char *file_name = malloc(uri_len + 21);
    if (file_name == NULL) // Error allocating file_name -> send a response back to the client
    {
        free(file_name_prefix); // Free allocated memory
        httpd_resp_send_404(req);
        return ESP_OK;
    }
    file_name_prefix[0] = '\0';
    file_name[0] = '\0';

    // Path parsing error -> send a response back to the client
    if (path_parse(req->uri, file_name_prefix, file_name, uri_len, uri_len) != PATH_PARSE_OK)
    {
        free(file_name_prefix); // Free allocated memory
        free(file_name);        // Free allocated memory
        httpd_resp_send(req, http_response_message_error[0], strlen(http_response_message_error[0]));
        return ESP_OK;
    }

    // If no file specified -> interpret as web_uri_defult_page ("index.html")
    if (strlen(file_name) == 0)
    {
        strcpy(file_name, web_uri_defult_page);
    }

    // Allocate spiffs_file_name for requested "elements"
    char *spiffs_file_name = malloc(strlen(file_name_prefix) + strlen(file_name) + 21);
    if (spiffs_file_name == NULL) // Error allocating spiffs_file_name -> send a response back to the client
    {
        free(file_name_prefix); // Free allocated memory
        free(file_name);        // Free allocated memory
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    // Generate full name for file saved in SPIFFS (NOTE: favicon* do not get prefix)
    if (strncmp("favicon", file_name, strlen("favicon")) == 0)
    {
        sprintf(spiffs_file_name, "%s%s%s", SPIFFS_PARTITION_BASE_PATH, "/", file_name);
    }
    else
    {
        sprintf(spiffs_file_name, "%s%s%s%s", SPIFFS_PARTITION_BASE_PATH, "/", file_name_prefix, file_name);
    }
    // printf(">>%s<<\n\r>>%s<<\n\r>>%s<<\n\r>>%s<<\n\r", req->uri, file_name_prefix, file_name, spiffs_file_name);

    // Free allocated memory
    free(file_name_prefix);

    // Read requested file from SPIFFS
    FILE *f;
    f = fopen(spiffs_file_name, "rb");
    if (!f) // Error reading file -> send a response back to the client
    {
        file_name[0] = '\0';
        strcpy(file_name, web_uri_defult_404_page);
        spiffs_file_name[0] = '\0';
        sprintf(spiffs_file_name, "%s%s%s", SPIFFS_PARTITION_BASE_PATH, "/", web_uri_defult_404_page);
        f = fopen(spiffs_file_name, "rb");
        if (!f) // Error reading file -> send a response back to the client
        {
            free(file_name);        // Free allocated memory
            free(spiffs_file_name); // Free allocated memory
            httpd_resp_send_404(req);
            return ESP_OK;
        }
    }

    // Free allocated memory
    free(spiffs_file_name);

    // Get file length
    fseek(f, 0, SEEK_END);
    int32_t file_size = ftell(f); // Number of bytes -> file opened in binary mode ("rb")
    rewind(f);                    // Set file position to the beginnings
    // Error getting file length -> send a response back to the client
    if (file_size <= 0)
    {
        free(file_name); // Free allocated memory
        fclose(f);       // Close file
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    // Allocate file_buffer for requested file
    char *file_buffer = malloc(file_size + 21);
    // Error allocating file_buffer -> send a response back to the client
    if (file_buffer == NULL)
    {
        free(file_name); // Free allocated memory
        fclose(f);       // Close file
        httpd_resp_send_404(req);
        return ESP_OK;
    }
    file_buffer[0] = '\0';

    // Write file content to file_buffer
    fread(file_buffer, file_size, 1, f);
    if (ferror(f)) // Error reading -> send a response back to the client
    {
        free(file_name);   // Free allocated memory
        fclose(f);         // Close file
        free(file_buffer); // Free allocated memory
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    // Close file
    fclose(f);

    // Determine type of requested webpage "element"
    const char *element_type = NULL;
    if (strstr(file_name, ".html") != NULL || strstr(file_name, ".htm") != NULL)
    {
        element_type = "text/html";
    }
    else if (strstr(file_name, ".css") != NULL)
    {
        element_type = "text/css";
    }
    else if (strstr(file_name, ".js") != NULL)
    {
        element_type = "application/javascript";
    }
    else if (strstr(file_name, ".jpg") != NULL || strstr(file_name, ".jpeg") != NULL)
    {
        element_type = "image/jpeg";
    }
    else if (strstr(file_name, ".png") != NULL)
    {
        element_type = "image/png";
    }
    else if (strstr(file_name, ".ico") != NULL)
    {
        element_type = "image/x-icon";
    }
    else
    {
        element_type = "application/octet-stream";
    }

    // Free allocated memory
    free(file_name);

    // Set response type and send response
    httpd_resp_set_type(req, element_type);
    httpd_resp_send(req, file_buffer, file_size);

    // Free allocated memory
    free(file_buffer);

    return ESP_OK;
}

/***************************************************************************************************************/
/*** END *******************************************************************************************************/
/***************************************************************************************************************/