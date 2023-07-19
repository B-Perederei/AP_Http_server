#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <esp_http_server.h>
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_SSID "AP-ESP32S3"
#define WIFI_PASS "qwerty123"

static const char *TAG = "wifi_softAP";

// ================== WIFI SETUP AND CONNECTIONS ========================
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);

        /*
        // Need to write code for redirection to welcome page here
        // Redirect connected device to the welcome page
        httpd_handle_t server = (httpd_handle_t) arg;
        char redirect_uri[128]; // Use a buffer to hold the URI path
        snprintf(redirect_uri, sizeof(redirect_uri), "http://192.168.4.1/welcome");

        httpd_req_t *req = httpd_req_from_sockfd(event->socket_fd);
        if (req) {
            httpd_resp_set_status(req, "307 Temporary Redirect");
            httpd_resp_set_hdr(req, "Location", redirect_uri);
            httpd_resp_send(req, NULL, 0);
        }*/

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void) {

    // Initialize Wi-Fi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Create a default event loop
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
                                                        
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 10, // Maximum number of clients connected to the AP
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// ================== HTTP SERVER ========================

esp_err_t welcome_get_handler(httpd_req_t *req) {
    const char *html_response = (const char *)req->user_ctx;
    httpd_resp_send(req, html_response, strlen(html_response));
    return ESP_OK;
}


static const httpd_uri_t welcome = {
    .uri       = "/welcome",
    .method    = HTTP_GET,
    .handler   = welcome_get_handler,
    .user_ctx  = "<html><body><h1>Welcome to ESP32-S3 server</h1></body></html>"
};

static httpd_handle_t start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    config.lru_purge_enable = true;
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &welcome);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    ESP_LOGI(TAG, "AP is up");

    // Start the server
    static httpd_handle_t server = NULL;
    server = start_http_server();
    ESP_LOGI(TAG, "HTTP Server is up");
}
