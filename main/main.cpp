/*
	Pokus o vytvoření fakt hodně zákeřné žárovky z prvních pár měsíců na VUT
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "WiFi Functions.cpp"

#define STA_SSID "OSDVF"
#define STA_PASSWORD "ahoj1234"
#define AP_SSID "Muhahaha"
#define AP_PASSWORD "nowyouknowmypassword"
#define AP_MAX_CONN 4
#define AP_CHANNEL 0
#define WIFI_HIDDEN 1

// Event group
#define ENUM_HAS_BIT(e,b) (e & b)
static EventGroupHandle_t _event_group;
const int STA_READY = BIT0;
const int STA_CONNECTED_BIT = BIT1;
const int STA_DISCONNECTED_BIT = BIT2;
const int STA_SCANNING_BIT = BIT3;
const int STA_SCAN_END_BIT = BIT4;

const int STA_BITS = STA_READY | STA_CONNECTED_BIT | STA_DISCONNECTED_BIT | STA_SCAN_END_BIT | STA_SCANNING_BIT;

static const char *_Scheduler_Tag = "Zarovka:Sched";

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	EventBits_t staBits = xEventGroupGetBits(_event_group);
	if (event_base == WIFI_EVENT)
	{
		if (event_id == WIFI_EVENT_AP_START)
			ESP_LOGI(_Scheduler_Tag, "Own AP started");
		if (event_id == WIFI_EVENT_STA_START)
			xEventGroupSetBits(_event_group, STA_READY);
		else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
		{
			xEventGroupClearBits(_event_group, STA_CONNECTED_BIT);
			xEventGroupSetBits(_event_group, STA_DISCONNECTED_BIT);
		}
	}
	else if (event_base == IP_EVENT)
	{
		if (event_id == IP_EVENT_STA_GOT_IP)
		{
			//ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
			//ESP_LOGI(WIFI_TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
			//Beacuse tcpip_adapter aleready notifies us
			xEventGroupSetBits(_event_group, STA_CONNECTED_BIT);
			xEventGroupClearBits(_event_group, STA_DISCONNECTED_BIT);
		}
	}
}

// print the list of connected stations
void printConnectedClients()
{
	printf(" Connected stations:\n");
	printf("--------------------------------------------------\n");

	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t adapter_sta_list;

	memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
	memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

	ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&wifi_sta_list));
	ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list));

	for (int i = 0; i < adapter_sta_list.num; i++)
	{

		tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
		printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: %s\n", i + 1,
			   station.mac[0], station.mac[1], station.mac[2],
			   station.mac[3], station.mac[4], station.mac[5],
			   ip4addr_ntoa(&(station.ip)));
	}

	printf("\n");
}

//A tady budeme čekat až někdo něco zahlásí ať můžeme v klidu reagovat a nechta ho dál hlásit
void monitor_task(void *pvParameter)
{
	while (1)
	{

		EventBits_t staBits = xEventGroupWaitBits(_event_group, STA_BITS, pdTRUE, pdFALSE, portMAX_DELAY);
		if ENUM_HAS_BIT(staBits, STA_CONNECTED_BIT)
			ESP_LOGI(_Scheduler_Tag,"Connected to a station\n");
		else if ENUM_HAS_BIT(staBits, STA_DISCONNECTED_BIT)
		{
			ESP_LOGI(_Scheduler_Tag,"Disconnected from the station\n");
			WifiFunctions::Scan();
		}
		else if ENUM_HAS_BIT(staBits, STA_READY)
		{
			ESP_LOGI(_Scheduler_Tag, "Connecting again to a station");
			ESP_ERROR_CHECK(esp_wifi_connect());
		}
	}
}

//Tohle tu zatím necháme ale asi ne na dlouho, vypisuje to periodiky seznam připojených klientů
void station_list_task(void *pvParameter)
{
	while (1)
	{

		printConnectedClients();
		vTaskDelay(10000 / portTICK_RATE_MS);
	}
}

extern "C"
{
	void app_main(void);
}
// Main application
void app_main()
{
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// create the event group to handle wifi events
	_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());

	// initialize the tcp stack
	tcpip_adapter_init();

	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 10, 1);
	IP4_ADDR(&info.gw, 192, 168, 10, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));

	// start the DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

	// initialize the WiFi stack in AccessPoint mode with configuration in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

	wifi_config_t ap_config;
	wifi_config_t sta_config;

	memset(&ap_config, 0, sizeof(wifi_config_t));
	strncpy((char *)ap_config.ap.ssid, AP_SSID, sizeof(AP_SSID));
	strncpy((char *)ap_config.ap.password, AP_PASSWORD, sizeof(AP_PASSWORD));
	ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
	ap_config.ap.ssid_len = 0;
	ap_config.ap.max_connection = AP_MAX_CONN;
	ap_config.ap.channel = AP_CHANNEL;

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

	memset(&sta_config, 0, sizeof(wifi_config_t));
	strncpy((char *)sta_config.sta.ssid, STA_SSID, sizeof(STA_SSID));
	strncpy((char *)sta_config.sta.password, STA_PASSWORD, sizeof(STA_PASSWORD));

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	printf("\nAhooj šéfe, vítej v ovládacím panelu Zákeřné Žárovki.\n\t--Kdo by nechtěl být žárovkouu? Já!--\nSSID:%s\nSTA:%s\n\n", AP_SSID,STA_SSID);

	// Sranda začíná. Muhahahahah
	xTaskCreate(&monitor_task, "monitor_task", 2048, NULL, 5, NULL);
	xTaskCreate(&station_list_task, "station_list_task", 2048, NULL, 5, NULL);
}