#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ic2Initialize.c"
#include "readAndDisplayTimerValue.c"
#include "wifi_starter.c"
#include "accelReadAndCheckPosition.c"
#include <stdio.h>
#include "sender.c"
#include "esp_eth.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_tls_crypto.h"
#include "esp_wifi_types.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "lwip/dns.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_sleep.h"


#define BUTTON_GPIO 18   // GPIO number for the button (for example, GPIO0)
#define ESP_INTR_FLAG_DEFAULT 0

/* LED COLORS
RED - wifi connection error
GREEN - wifi started in hosting website mode
OFF - everything is working correctly

*/


static QueueHandle_t gpio_evt_queue = NULL;  // Declaration of the queue handle


bool isServerRunning = true;
bool isSleeping = true;

uint8_t oldWallPosition = 8;
uint8_t newWallPosition = 8;

char *cube_id = NULL;

#pragma region NVS

void save_wifi_credentials(const char *ssid, const char *password, const char *cube_id) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS handle
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    // Write SSID
    err = nvs_set_str(my_handle, "wifi_ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) setting WiFi SSID!", esp_err_to_name(err));
    }

    // Write Password
    err = nvs_set_str(my_handle, "wifi_password", password);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) setting WiFi password!", esp_err_to_name(err));
    }

    // Write Cube ID
    err = nvs_set_str(my_handle, "cube_id", cube_id);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) setting Cube ID!", esp_err_to_name(err));
    }

    // Commit written value
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) committing NVS!", esp_err_to_name(err));
    }

    // Close
    nvs_close(my_handle);
    reboot_device();
}

bool read_wifi_credentials(char **ssid, char **password, char **cube_id) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS handle
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return false;
    }

    // Read SSID
    size_t ssid_size;
    err = nvs_get_str(my_handle, "wifi_ssid", NULL, &ssid_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI("NVS", "Wi-Fi SSID not found.");
        nvs_close(my_handle);
        return false;
    } else if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) reading WiFi SSID size!", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }

    *ssid = malloc(ssid_size);
    err = nvs_get_str(my_handle, "wifi_ssid", *ssid, &ssid_size);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) reading WiFi SSID!", esp_err_to_name(err));
        free(*ssid);
        nvs_close(my_handle);
        return false;
    }

    // Read Password
    size_t password_size;
    err = nvs_get_str(my_handle, "wifi_password", NULL, &password_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI("NVS", "Wi-Fi password not found.");
        free(*ssid);
        nvs_close(my_handle);
        return false;
    } else if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) reading WiFi password size!", esp_err_to_name(err));
        free(*ssid);
        nvs_close(my_handle);
        return false;
    }

    *password = malloc(password_size);
    err = nvs_get_str(my_handle, "wifi_password", *password, &password_size);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) reading WiFi password!", esp_err_to_name(err));
        free(*ssid);
        free(*password);
        nvs_close(my_handle);
        return false;
    }

    // Read Cube ID
    size_t cube_id_size;
    err = nvs_get_str(my_handle, "cube_id", NULL, &cube_id_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI("NVS", "Cube ID not found.");
        free(*ssid);
        free(*password);
        nvs_close(my_handle);
        return false;
    } else if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) reading Cube ID size!", esp_err_to_name(err));
        free(*ssid);
        free(*password);
        nvs_close(my_handle);
        return false;
    }

    *cube_id = malloc(cube_id_size);
    err = nvs_get_str(my_handle, "cube_id", *cube_id, &cube_id_size);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error (%s) reading Cube ID!", esp_err_to_name(err));
        free(*ssid);
        free(*password);
        free(*cube_id);
        nvs_close(my_handle);
        return false;
    }

    // Close
    nvs_close(my_handle);
    return true;
}



#pragma endregion



#pragma region Wifi_Ap

#define EXAMPLE_ESP_WIFI_SSID "esp32"      // CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS "mypassword" // CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN 5             // CONFIG_ESP_MAX_STA_CONN

char field1[50], field2[50], field3[50];

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

//static const char *TAG = "wifi_AP_WEBserver";

/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req) {
  char *buf;
  size_t buf_len;

  /* Get header value string length and allocate memory for length + 1,
   * extra byte for null termination */
  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    /* Copy null terminated value string into buffer */
    if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
      ESP_LOGI(TAG, "Found header => Host: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) ==
        ESP_OK) {
      ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) ==
        ESP_OK) {
      ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
    }
    free(buf);
  }

  /* Read URL query string length and allocate memory for length + 1,
   * extra byte for null termination */
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      ESP_LOGI(TAG, "Found URL query => %s", buf);
      char param[32];
      /* Get value of expected key from query string */
      if (httpd_query_key_value(buf, "query1", param, sizeof(param)) ==
          ESP_OK) {
        ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
      }
      if (httpd_query_key_value(buf, "query3", param, sizeof(param)) ==
          ESP_OK) {
        ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
      }
      if (httpd_query_key_value(buf, "query2", param, sizeof(param)) ==
          ESP_OK) {
        ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
      }
    }
    free(buf);
  }

  /* Set some custom headers */
  httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
  httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

  /* Send response with custom headers and body set as the
   * string passed in user context*/
  // const char *resp_str = (const char *)req->user_ctx;
  // httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
  uint8_t mac[6];
    get_mac_address(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

   const char *html_resp_template = "<html><body>"
                                     "<h1>Simple Web Server</h1>"
                                     "<p>Mac: %s</p>"
                                     "<form method=\"post\">"
                                     "SSID: <input type=\"text\" name=\"field1\" minlength=\"1\"><br>"
                                     "Haslo: <input type=\"text\" name=\"field2\" minlength=\"1\"><br>"
                                     "ID kostki: <input type=\"text\" name=\"field3\" minlength=\"1\"><br>"
                                     "<input type=\"submit\" value=\"Submit\">"
                                     "</form>"
                                     "</body></html>";
    char html_resp[512];
    snprintf(html_resp, sizeof(html_resp), html_resp_template, mac_str);

    /* Send response with custom headers and body */
    httpd_resp_send(req, html_resp, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }

  return ESP_OK;
}


static esp_err_t hello_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    // Buffer to store the received data
    char data[100];
    int data_len = 0;

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }

        memcpy(data + data_len, buf, ret);
        data_len += ret;
        remaining -= ret;

        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");
    }

    data[data_len] = '\0';

    if (sscanf(data, "field1=%49[^&]&field2=%49[^&]&field3=%49s", field1, field2, field3) == 3) {
        ESP_LOGI(TAG, "Field 1: %s", field1);
        ESP_LOGI(TAG, "Field 2: %s", field2);
        ESP_LOGI(TAG, "Field 3: %s", field3);
    } else {
        ESP_LOGE(TAG, "Failed to extract field values");
    }

    save_wifi_credentials(field1, field2, field3);

    return ESP_OK;
}

static const httpd_uri_t hello = {.uri = "/config",
                                  .method = HTTP_GET,
                                  .handler = hello_get_handler,
                                  /* Let's pass response string in user
                                   * context to demonstrate it's usage */
                                  .user_ctx = "Hello World"};
static const httpd_uri_t hello_post = {.uri = "/config",
                                       .method = HTTP_POST,
                                       .handler = hello_post_handler,
                                       /* Let's pass response string in user
                                        * context to demonstrate it's usage */
                                       .user_ctx = "Hello World"};

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
  if (strcmp("/hello", req->uri) == 0) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND,
                        "/hello URI is not available");
    /* Return ESP_OK to keep underlying socket open */
    return ESP_OK;
  } else if (strcmp("/echo", req->uri) == 0) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
    /* Return ESP_FAIL to close underlying socket */
    return ESP_FAIL;
  }
  /* For any other URI send 404 and close socket */
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
  return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
static httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &hello);
    httpd_register_uri_handler(server, &hello_post);
#if CONFIG_EXAMPLE_BASIC_AUTH
    httpd_register_basic_auth(server);
#endif
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server) {
  // Stop the httpd server
  return httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  httpd_handle_t *server = (httpd_handle_t *)arg;
  if (*server) {
    ESP_LOGI(TAG, "Stopping webserver");
    if (stop_webserver(*server) == ESP_OK) {
      *server = NULL;
    } else {
      ESP_LOGE(TAG, "Failed to stop http server");
    }
  }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data) {
  httpd_handle_t *server = (httpd_handle_t *)arg;
  if (*server == NULL) {
    ESP_LOGI(TAG, "Starting webserver");
    *server = start_webserver();
  }
}

//------------------------------------------------------------------------------

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
             event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
             event->aid);
  }
}

esp_err_t wifi_init_softap(void) {
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));

  wifi_config_t wifi_config = {
      .ap = {.ssid = EXAMPLE_ESP_WIFI_SSID,
             .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
             .password = EXAMPLE_ESP_WIFI_PASS,
             .max_connection = EXAMPLE_MAX_STA_CONN,
             .authmode = WIFI_AUTH_WPA_WPA2_PSK},
  };
  if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
           EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
  return ESP_OK;
}
//------------------------------------------------------------------------------

void start_server_wifi_host(void) {
    static httpd_handle_t server = NULL;

  ESP_LOGI(TAG, "NVS init");
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_LOGI(TAG, "Eventloop create");
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /* This helper function configures Wi-Fi or Ethernet, as selected in
   * menuconfig. Read "Establishing Wi-Fi or Ethernet Connection" section in
   * examples/protocols/README.md for more information about this function.
   */

  ESP_LOGI(TAG, "init softAP");
  ESP_ERROR_CHECK(wifi_init_softap());

  server = start_webserver();
}

#pragma endregion


#pragma region Accel
gptimer_handle_t gptimer;
gptimer_handle_t gptimerGlobal;
u_int8_t wallPositionTab[5] = {0, 0, 0, 0, 0};
u_int8_t wallTimeTab[6] = {0, 0, 0, 0, 0, 0};
int16_t AccelX, AccelY, AccelZ;
bool isWallPositionUnchanged;
bool wallSend;

void MMA8452Q_init()
{
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x2A, 0x00);
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x2A, 0x01);
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x0E, 0x00);
}

//TODO FIX
//zmienić error code z 0 na -1
void checkIfPositionWallHasChanged(u_int16_t timerValue)
{
  if (timerValue > 5000)
  {
    wallPositionTab[4] = checkPosition(AccelX, AccelY, AccelZ);
    printValues(AccelX, AccelY, AccelZ);
    resetTimer(gptimer);
    for (size_t i = 0; i < 5; i++)
    {
      printf("Wall postion %d\n", wallPositionTab[i]);
      if (wallPositionTab[i] != wallPositionTab[0])
      {
        printf("Wall position changed\n");
        isWallPositionUnchanged = false; 
      }
      else{
        newWallPosition = wallPositionTab[0];
      }
    }
  }
  else if (timerValue > 4000)
  {
    wallPositionTab[3] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
  else if (timerValue > 3000)
  {
    wallPositionTab[2] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
  else if (timerValue > 2000)
  {
    wallPositionTab[1] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
  else if (timerValue > 1000)
  {
    wallPositionTab[0] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
}

void countTimeOnCurrentWall()
{
  if (isWallPositionUnchanged == true && wallPositionTab[0] != 0)
  {

    wallSend = true;
    resetTimer(gptimerGlobal);
    wallTimeTab[wallPositionTab[0] - 1] += 5;
    printf("\n--------------------\n");
    for (size_t i = 0; i <= 5; i++)
    {
      printf("Wall %d time %d\n", i + 1, wallTimeTab[i]);
    }
  }
}

void SendAndPositionTabToServer(char *cube_id)
{
  oldWallPosition = wallPositionTab[0];
  send_http_post_request(wallPositionTab[0], cube_id);
  
}

#pragma endregion


#pragma region button

void button_press(){
  printf("Button pressed\n");
  erase_nvs();
}

// ISR handler for the button press
static void IRAM_ATTR button_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task to handle button press
static void button_task(void *arg) {
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
          button_press();
        }
    }
}

#pragma endregion

#pragma region deepSleep
#define BUTTON_GPIO_DEEP_SLEEP 35   // Replace with your actual GPIO pin number
#define ESP_INTR_FLAG_DEFAULT 0
#define DEBOUNCE_TIME_MS 200  // Debounce time for button press
#define WAKEUP_DELAY_MS 1000  // 10 seconds delay before accepting button press as wake-up

// Queue to handle GPIO events
static QueueHandle_t gpio_switch_evt_queue = NULL;
// Flag to track whether the wake-up delay timer has expired

// Tag for logging
static const char *TAG_SLEEP = "BUTTON_PRESS";

// FreeRTOS software timer handle for wake-up delay
// Function to handle button press
void switch_deep_sleep() {
        send_http_post_request(-1, cube_id);
        isSleeping = true;
        ESP_LOGI(TAG_SLEEP, "Button pressed, going to deep sleep...");

        // Disable interrupt before sleep
        gpio_intr_disable(BUTTON_GPIO_DEEP_SLEEP);

        // Configure the GPIO as a wake-up source
        esp_sleep_enable_ext0_wakeup(BUTTON_GPIO_DEEP_SLEEP, 1);
        gpio_wakeup_enable(BUTTON_GPIO_DEEP_SLEEP, GPIO_INTR_LOW_LEVEL);

        // Small delay to ensure the message is printed before sleep
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));

        // Enter deep sleep
        esp_deep_sleep_start();
}

// ISR handler for the button press
static void IRAM_ATTR switch_deep_sleep_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_switch_evt_queue, &gpio_num, NULL);
}

// Task to handle button press with debounce logic
static void switch_deep_sleep_task(void *arg) {
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_switch_evt_queue, &io_num, portMAX_DELAY)) {
            switch_deep_sleep();
        }
    }
}
#pragma endregion

bool runLedOnce = true;

void app_main()
{
  isSleeping = false;
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO)
  {
    ESP_LOGI(TAG, "Woke up from deep sleep by GPIO");

    // Wait a bit to avoid immediate sleep
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
    }

    // Configure button GPIO as input with pull-up resistor
    gpio_config_t io_conf_switch;
    io_conf_switch.intr_type = GPIO_INTR_DISABLE;  // Disable interrupt
    io_conf_switch.mode = GPIO_MODE_INPUT;         // Set as input mode
    io_conf_switch.pin_bit_mask = (1ULL << BUTTON_GPIO_DEEP_SLEEP);  // Bit mask of the pins to set
    io_conf_switch.pull_up_en = 0;                 // Enable pull-up
    io_conf_switch.pull_down_en = 1;               // Disable pull-down
    gpio_config(&io_conf_switch);

    // Change interrupt type for the button GPIO
    gpio_set_intr_type(BUTTON_GPIO_DEEP_SLEEP, GPIO_INTR_POSEDGE);  // Interrupt on rising edge

    // Create a queue to handle GPIO events
    gpio_switch_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // Start a task to handle the button press
    xTaskCreate(switch_deep_sleep_task, "switch_deep_sleep_task", 8096, NULL, 10, NULL);

    // Install ISR service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook ISR handler for specific GPIO pin
    gpio_isr_handler_add(BUTTON_GPIO_DEEP_SLEEP, switch_deep_sleep_isr_handler, (void *) BUTTON_GPIO_DEEP_SLEEP);

////
  led_init();
  led_setColor(red);
  uint8_t mac[6];
    get_mac_address(mac);
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);



    // Configure the button GPIO
   gpio_config_t io_conf;
    // Disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // Bit mask of the pins to set
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    // Enable pull-up mode
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Change interrupt type for the button GPIO
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE);

    // Create a queue to handle GPIO events
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // Start a task to handle the button press
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    // Hook ISR handler for specific GPIO pin
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void *) BUTTON_GPIO);

    ESP_LOGI(TAG, "Button example configured. Press the button to run function_x.");


/////


 esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Save Wi-Fi credentials

   //save_wifi_credentials("your_ssid", "your_password", "your_cube_id");


    // Read and print Wi-Fi credentials and Cube ID
    char *ssid = NULL;
    char *password = NULL;
    
    if (read_wifi_credentials(&ssid, &password, &cube_id)) {
        printf("SSID: %s\n", ssid);
        printf("Password: %s\n", password);
        printf("Cube ID: %s\n", cube_id);

        isServerRunning = false;
    }
    else
    {
      printf("No Wi-Fi credentials found, entering configuration mode...\n");
      // Your code here to handle absence of Wi-Fi credentials
      // e.g., enter configuration mode
      isServerRunning = true;
      
    }

  if (isServerRunning)
  {
    led_setColor(blue);
    start_server_wifi_host();
  }
  else {
    startWifi(ssid, password);
  

  main_i2c_init(MMA8452Q_SDA, MMA8452Q_SCL);
  MMA8452Q_init();

  gptimer = timer_init();
  gptimerGlobal = timer_init();

  while (1)
  {
    if (/* condition */ runLedOnce)
    {
      led_turn_off();
      runLedOnce = false;
    }
    if (isSleeping == false){
    readAccel(&AccelX, &AccelY, &AccelZ);
    u_int16_t timerValue = getTimerValueMs(gptimer);
    isWallPositionUnchanged = true;
    
    checkIfPositionWallHasChanged(timerValue);

    if (oldWallPosition != newWallPosition && newWallPosition != 0)
    {
      SendAndPositionTabToServer(cube_id);
    }
  }
  }
  }
}