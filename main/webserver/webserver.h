#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t webserver_start(void);

esp_err_t webserver_stop(void);

esp_err_t webserver_add_handler(const httpd_uri_t *uri_handler);

#endif
