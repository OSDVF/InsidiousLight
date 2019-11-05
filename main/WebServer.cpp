#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "sys/param.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"

#include "esp_https_server.h"

class WebServer
{
private:
    /* An HTTP GET handler */
    static esp_err_t root_get_handler(httpd_req_t *req)
    {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, "<h1>Hello Secure World!</h1>", -1); // -1 = use strlen()

        return ESP_OK;
    }
    static constexpr const char *TAG = "Zarovka:Https";

public:
    static httpd_handle_t Start()
    {
        const httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL};
        httpd_handle_t server = NULL;

        // Start the httpd server
        ESP_LOGI(TAG, "Starting server");

        httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

        extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
        extern const unsigned char cacert_pem_end[] asm("_binary_cacert_pem_end");
        conf.cacert_pem = cacert_pem_start;
        conf.cacert_len = cacert_pem_end - cacert_pem_start;

        extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
        extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");
        conf.prvtkey_pem = prvtkey_pem_start;
        conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

        esp_err_t ret = httpd_ssl_start(&server, &conf);
        if (ESP_OK != ret)
        {
            ESP_LOGI(TAG, "Error starting server!");
            return NULL;
        }

        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        return server;
    }
    static void Stop(httpd_handle_t server)
    {
        // Stop the httpd server
        httpd_ssl_stop(server);
    }
};