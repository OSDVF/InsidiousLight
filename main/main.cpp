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

#define LOG_COLOR_WHITE "37"
#define LOG_UNDERLINED "\033[4;m"
#define LOG_BOLD_UNDERLINED(COLOR) "\033[1;4;" COLOR "m"
#define LOG_RESET_BOLD "\033[21m"
#define LOG_RESET_UNDERLINE "\033[24m"
#define LOG_RESET_ALL "\033[0;21;24;m"

char _sta_ssid[32] = "OSDVF";
char _sta_password[32] = "ahoj1234";
char _ap_ssid[16] = "Muhahaha";
char _ap_password[32] = "nowyouknowmypassword";
uint8_t _ap_max_clients = 4;
uint8_t _ap_channel = 0;
uint8_t _ap_hidden = 1;

// Event group
#define ENUM_HAS_BIT(e, b) (e & b)
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

TaskHandle_t monitor_task_handle;
//A tady budeme čekat až někdo něco zahlásí ať můžeme v klidu reagovat a nechta ho dál hlásit
void monitor_task(void *pvParameter)
{
	while (1)
	{
		printf("%d", uxTaskGetStackHighWaterMark(monitor_task_handle));
		EventBits_t staBits = xEventGroupWaitBits(_event_group, STA_BITS, pdTRUE, pdFALSE, portMAX_DELAY);
		if ENUM_HAS_BIT(staBits, STA_CONNECTED_BIT)
			ESP_LOGI(_Scheduler_Tag, "Connected to an AP\n");
		else if (!ENUM_HAS_BIT(staBits, STA_SCANNING_BIT))
		{
			if (ENUM_HAS_BIT(staBits, STA_DISCONNECTED_BIT))
			{
				ESP_LOGI(_Scheduler_Tag, "Disconnected from the AP\n");
				xEventGroupSetBits(_event_group, STA_SCANNING_BIT);
				WifiFunctions::Scan();
				xEventGroupClearBits(_event_group, STA_SCANNING_BIT);
				xEventGroupSetBits(_event_group, STA_SCAN_END_BIT);
			}
			else if (ENUM_HAS_BIT(staBits, STA_READY))
			{
				ESP_LOGI(_Scheduler_Tag, "Connecting again to an AP");
				ESP_ERROR_CHECK(esp_wifi_connect());
			}
		}
		
		if(ENUM_HAS_BIT(staBits,STA_SCAN_END_BIT))
		{
			xEventGroupClearBits(_event_group, STA_SCAN_END_BIT);
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
	//ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_AP,WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
	//ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA,WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));

	wifi_config_t ap_config;
	wifi_config_t sta_config;

	memset(&ap_config, 0, sizeof(wifi_config_t));
	strncpy((char *)ap_config.ap.ssid, _ap_ssid, sizeof(_ap_ssid));
	strncpy((char *)ap_config.ap.password, _ap_password, sizeof(_ap_password));
	ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
	ap_config.ap.ssid_len = 0;
	ap_config.ap.ssid_hidden = _ap_hidden;
	ap_config.ap.max_connection = _ap_max_clients;
	ap_config.ap.channel = _ap_channel;

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

	memset(&sta_config, 0, sizeof(wifi_config_t));
	strncpy((char *)sta_config.sta.ssid, _sta_ssid, sizeof(_sta_ssid));
	strncpy((char *)sta_config.sta.password, _sta_password, sizeof(_sta_password));
	//sta_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
	//sta_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	printf("\nAhooj šéfe, vítej v ovládacím panelu " LOG_BOLD_UNDERLINED(LOG_COLOR_GREEN) "Zá" LOG_BOLD_UNDERLINED(LOG_COLOR_BROWN) "keř" LOG_BOLD_UNDERLINED(LOG_COLOR_CYAN) "né " LOG_BOLD_UNDERLINED(LOG_COLOR_RED) "Žá\033[1;4;38;5;214mrov" LOG_BOLD(LOG_COLOR_PURPLE) "ki." LOG_RESET_ALL "\n\t--Kdo by nechtěl být žárovkouu? Já!--\nSSID:%s\nSTA:%s\n\n", _ap_ssid, _sta_ssid);

	// Sranda začíná. Muhahahahah
	xTaskCreate(&monitor_task, "monitor_task", 4096, NULL, 5, &monitor_task_handle);
	xTaskCreate(&station_list_task, "station_list_task", 2048, NULL, 5, NULL);
}
