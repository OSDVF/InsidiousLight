#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#define DEFAULT_SCAN_LIST_SIZE 20
static const char *SCAN_TAG = "Zarovka:Scan";
class WifiFunctions
{
    public: static void Scan()
    {
        uint16_t number = DEFAULT_SCAN_LIST_SIZE;
        wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
        const wifi_scan_config_t scConfig = {
            .ssid = 0,
            .bssid = 0,
            .channel = 0,
            .show_hidden = true,
            .scan_type = WIFI_SCAN_TYPE_PASSIVE,
            .scan_time = {.passive=0}
        };
        ESP_LOGI(SCAN_TAG,"Starting scan...");
        
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scConfig, true));
        #define STRING(s) #s
        ESP_LOGI(SCAN_TAG,"Started scan with list size %d", DEFAULT_SCAN_LIST_SIZE);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

        for (int i = 0; i < number; i++)
        {
            ESP_LOGI(SCAN_TAG, "SSID \t\t%s", ap_info[i].ssid);
            ESP_LOGI(SCAN_TAG, "RSSI \t\t%d", ap_info[i].rssi);
            print_auth_mode(ap_info[i].authmode);
            if (ap_info[i].authmode != WIFI_AUTH_WEP)
            {
                print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
            }
            ESP_LOGI(SCAN_TAG, "Channel \t\t%d\n", ap_info[i].primary);
        }
    }
    static void print_auth_mode(int authmode)
    {
        switch (authmode)
        {
        case WIFI_AUTH_OPEN:
            ESP_LOGI(SCAN_TAG, "Authmode \tWIFI_AUTH_OPEN");
            break;
        case WIFI_AUTH_WEP:
            ESP_LOGI(SCAN_TAG, "Authmode \tWIFI_AUTH_WEP");
            break;
        case WIFI_AUTH_WPA_PSK:
            ESP_LOGI(SCAN_TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
            break;
        case WIFI_AUTH_WPA2_PSK:
            ESP_LOGI(SCAN_TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            ESP_LOGI(SCAN_TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            ESP_LOGI(SCAN_TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
            break;
        default:
            ESP_LOGI(SCAN_TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
            break;
        }
    }

    static void print_cipher_type(int pairwise_cipher, int group_cipher)
    {
        switch (pairwise_cipher)
        {
        case WIFI_CIPHER_TYPE_NONE:
            ESP_LOGI(SCAN_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
            break;
        case WIFI_CIPHER_TYPE_WEP40:
            ESP_LOGI(SCAN_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
            break;
        case WIFI_CIPHER_TYPE_WEP104:
            ESP_LOGI(SCAN_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
            break;
        case WIFI_CIPHER_TYPE_TKIP:
            ESP_LOGI(SCAN_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
            break;
        case WIFI_CIPHER_TYPE_CCMP:
            ESP_LOGI(SCAN_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
            break;
        case WIFI_CIPHER_TYPE_TKIP_CCMP:
            ESP_LOGI(SCAN_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
            break;
        default:
            ESP_LOGI(SCAN_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
            break;
        }

        switch (group_cipher)
        {
        case WIFI_CIPHER_TYPE_NONE:
            ESP_LOGI(SCAN_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
            break;
        case WIFI_CIPHER_TYPE_WEP40:
            ESP_LOGI(SCAN_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
            break;
        case WIFI_CIPHER_TYPE_WEP104:
            ESP_LOGI(SCAN_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
            break;
        case WIFI_CIPHER_TYPE_TKIP:
            ESP_LOGI(SCAN_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
            break;
        case WIFI_CIPHER_TYPE_CCMP:
            ESP_LOGI(SCAN_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
            break;
        case WIFI_CIPHER_TYPE_TKIP_CCMP:
            ESP_LOGI(SCAN_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
            break;
        default:
            ESP_LOGI(SCAN_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
            break;
        }
    }
};