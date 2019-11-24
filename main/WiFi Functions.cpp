#pragma once
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "cmd_system.h"
#define DEFAULT_SCAN_LIST_SIZE 20
class WifiFunctions
{
    private:
    static constexpr const char *SCAN_TAG = "Zarovka:Scan";
    public: 
    static void Scan()
    {
        Scan(NULL,0,0);
    }
    static int Scan(EventGroupHandle_t eventGroupToChange,const int bit,const int stoppedBit)
    {
        if(eventGroupToChange)
            xEventGroupSetBits(eventGroupToChange, bit);
        uint16_t number = DEFAULT_SCAN_LIST_SIZE;
        wifi_ap_record_t *ap_info = new wifi_ap_record_t[DEFAULT_SCAN_LIST_SIZE];
        const wifi_scan_config_t scConfig = {
            .ssid = 0,
            .bssid = 0,
            .channel = 0,
            .show_hidden = true,
            .scan_type = WIFI_SCAN_TYPE_PASSIVE,
            .scan_time = {.passive=0}
        };
        ESP_LOGI(SCAN_TAG,"Starting scan...");
        esp_err_t result = esp_wifi_scan_start(&scConfig, true);
        if(result == ESP_FAIL)
        {
            printf("Cannot start scan now. Wifi driver is probably busy.\n");
            return 1;
        }
        ESP_ERROR_CHECK(result);
        #define STRING(s) #s
        ESP_LOGI(SCAN_TAG,"Started scan with list size %d", DEFAULT_SCAN_LIST_SIZE);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

        for (int i = 0; i < number; i++)
        {
            printf(LOG_COLOR(LOG_COLOR_CYAN) "SSID \t\t%s\t" LOG_RESET_COLOR, ap_info[i].ssid);
            printf("RSSI: %d\n", ap_info[i].rssi);
            print_auth_mode(ap_info[i].authmode);
            printf(_NewLine);
            if (ap_info[i].authmode != WIFI_AUTH_WEP)
            {
                print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
            }
            printf( "Channel: \t%d\n\n", ap_info[i].primary);
        }
        delete[] ap_info;
        xEventGroupClearBits(eventGroupToChange, bit);
        if(stoppedBit)
            xEventGroupSetBits(eventGroupToChange, stoppedBit);
        return 0;
    }
    static void print_auth_mode(int authmode)
    {
        switch (authmode)
        {
        case WIFI_AUTH_OPEN:
            printf( "Authmode \tWIFI_AUTH_OPEN");
            break;
        case WIFI_AUTH_WEP:
            printf( "Authmode \tWIFI_AUTH_WEP");
            break;
        case WIFI_AUTH_WPA_PSK:
            printf( "Authmode \tWIFI_AUTH_WPA_PSK");
            break;
        case WIFI_AUTH_WPA2_PSK:
            printf( "Authmode \tWIFI_AUTH_WPA2_PSK");
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            printf( "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            printf( "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
            break;
        default:
            printf( "Authmode \tWIFI_AUTH_UNKNOWN");
            break;
        }
    }

    static void print_cipher_type(int pairwise_cipher, int group_cipher)
    {
        switch (pairwise_cipher)
        {
        case WIFI_CIPHER_TYPE_NONE:
            printf("Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE\n");
            break;
        case WIFI_CIPHER_TYPE_WEP40:
            printf( "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40\n");
            break;
        case WIFI_CIPHER_TYPE_WEP104:
            printf( "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104\n");
            break;
        case WIFI_CIPHER_TYPE_TKIP:
            printf( "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP\n");
            break;
        case WIFI_CIPHER_TYPE_CCMP:
            printf( "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP\n");
            break;
        case WIFI_CIPHER_TYPE_TKIP_CCMP:
            printf( "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP\n");
            break;
        default:
            printf( "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN\n");
            break;
        }

        switch (group_cipher)
        {
        case WIFI_CIPHER_TYPE_NONE:
            printf( "Group Cipher \tWIFI_CIPHER_TYPE_NONE\n");
            break;
        case WIFI_CIPHER_TYPE_WEP40:
            printf( "Group Cipher \tWIFI_CIPHER_TYPE_WEP40\n");
            break;
        case WIFI_CIPHER_TYPE_WEP104:
            printf( "Group Cipher \tWIFI_CIPHER_TYPE_WEP104\n");
            break;
        case WIFI_CIPHER_TYPE_TKIP:
            printf( "Group Cipher \tWIFI_CIPHER_TYPE_TKIP\n");
            break;
        case WIFI_CIPHER_TYPE_CCMP:
            printf( "Group Cipher \tWIFI_CIPHER_TYPE_CCMP\n");
            break;
        case WIFI_CIPHER_TYPE_TKIP_CCMP:
            printf( "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP\n");
            break;
        default:
            printf( "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN\n");
            break;
        }
    }
};