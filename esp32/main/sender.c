#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "esp_netif.h"
#include "lwip/sys.h"
#include "esp_http_client.h"

// Update server_url to use HTTP instead of HTTPS
static char *server_url = "http://dergcube.westus.cloudapp.azure.com/update";

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        // Handle response data if needed
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}

void get_mac_address(uint8_t *mac)
{
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
}

static void send_http_post_request(int currentWall, char *id_koskta)
{
    uint8_t mac[6];
    get_mac_address(mac);

    char post_data[100];
    snprintf(post_data, sizeof(post_data), "{\"currentWall\":%d,\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"id\":\"%s\"}",
             currentWall, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], id_koskta);

    printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_http_client_config_t config_post = {
        .url = server_url,
        .method = HTTP_METHOD_POST,
        .event_handler = client_event_post_handler,
        .transport_type = HTTP_TRANSPORT_OVER_TCP, 
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);
    if (client == NULL) {
        ESP_LOGE("HTTP_CLIENT", "Failed to initialize HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_set_post_field(client, post_data, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE("HTTP_CLIENT", "Failed to set POST data: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Access-Control-Allow-Origin", "*");

    err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE("HTTP_CLIENT", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}