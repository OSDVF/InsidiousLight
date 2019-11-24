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
#include "esp_console.h"
#include "nvs_flash.h"
#include "WiFi Functions.cpp"
#include "WebServer.cpp"
#include "DnsServer.cpp"
#include "cmd_system.h"
#include "Systems.cpp"
#include "WifiCommands.cpp"
#include "Settings.hpp"
#include "FileCommands.hpp"

#define LOG_COLOR_WHITE "37"
#define LOG_UNDERLINED "\033[4;m"
#define LOG_BOLD_UNDERLINED(COLOR) "\033[1;4;" COLOR "m"
#define LOG_RESET_BOLD "\033[21m"
#define LOG_RESET_UNDERLINE "\033[24m"
#define LOG_RESET_ALL "\033[0;21;24;m"

Settings::LocalSettings& _actualSettings = Settings::Storage::ActualSettings;
char **_domainNames;
uint8_t _domainCount = 2;
uint8_t _domainMaxLen = 23;

// Event group
#define ENUM_HAS_BIT(e, b) (e & b)
static EventGroupHandle_t _event_group;
static httpd_handle_t _httpsServer = NULL;
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
		{
			ESP_LOGI(_Scheduler_Tag, "Own AP started");
			if (_httpsServer == NULL)
			{
				_httpsServer = WebServer::Start();
			}
			if(!DnsServer::Running())
				DnsServer::Init(TCPIP_ADAPTER_IF_AP, _domainNames, _domainCount, _domainMaxLen);
		}
		
		if (event_id == WIFI_EVENT_STA_START)
			xEventGroupSetBits(_event_group, STA_READY);
		else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
		{
			xEventGroupSetBits(_event_group, STA_DISCONNECTED_BIT);
		}
	}
	else if (event_base == IP_EVENT)
	{
		if (event_id == IP_EVENT_STA_GOT_IP)
		{
			xEventGroupSetBits(_event_group, STA_CONNECTED_BIT);
			if (_httpsServer == NULL)
			{
				_httpsServer = WebServer::Start();
			}
		}
	}
}

//A tady budeme čekat až někdo něco zahlásí ať můžeme v klidu reagovat a nechta ho dál hlásit
void scanSchedulerTask(void *pvParameter)
{
	while (1)
	{
		EventBits_t staBits = xEventGroupWaitBits(_event_group, STA_BITS, pdTRUE, pdFALSE, portMAX_DELAY);
		if (!ENUM_HAS_BIT(staBits, STA_DISCONNECTED_BIT))
		{
			if (ENUM_HAS_BIT(staBits, STA_CONNECTED_BIT))
			{
				ESP_LOGI(_Scheduler_Tag, "Connected to an AP\n");
			}
			else if (ENUM_HAS_BIT(staBits, STA_SCANNING_BIT))
			{
				xEventGroupWaitBits(_event_group, STA_SCAN_END_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
				ESP_LOGI(_Scheduler_Tag, "Connecting to an AP (after scan)...");
				ESP_ERROR_CHECK(esp_wifi_connect());
			}
			else
			{
				ESP_LOGI(_Scheduler_Tag, "Connecting to an AP...");
				ESP_ERROR_CHECK(esp_wifi_connect());
			}
		}
		else
		{
			ESP_LOGW(_Scheduler_Tag, "Disconnected from the AP\n");
		}
	}
}

extern "C"
{
	void app_main(void);
}
// Main application
void app_main()
{
	//Get the SETTINGS
	_domainNames = new char *[_domainCount];
	constexpr const char *n[] = {"chvaly.dorostmladez.cz", "zarovka.cz"};
	for (int i = 0; i < 2; i++)
	{
		_domainNames[i] = new char[_domainMaxLen];
		strcpy(_domainNames[i], n[i]);
	}
	// create the event group to handle wifi events
	_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// initialize the tcp stack
	tcpip_adapter_init();

	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

	Settings::Storage::Mount();
	Settings::Storage::OpenConfig(true);

	Settings::IPv4 apIP = _actualSettings.ApIP.Value();
	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	info.ip = {.addr = apIP.ip};
	info.gw = {.addr = _actualSettings.ApGateway.Value().ip};
	info.netmask = {.addr = _actualSettings.ApMask.Value().ip};
	ESP_LOGI(_Scheduler_Tag, "Own IP: %d.%d.%d.%d (%dB)", apIP.octets[0], apIP.octets[1], apIP.octets[2], apIP.octets[3], apIP.ip);
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
	strncpy((char *)ap_config.ap.ssid, _actualSettings.ApSsid.Value(), strlen(_actualSettings.ApSsid.Value())+1);
	strncpy((char *)ap_config.ap.password, _actualSettings.ApPassword.Value(), strlen(_actualSettings.ApPassword.Value())+1);
	ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
	ap_config.ap.ssid_len = 0;
	ap_config.ap.ssid_hidden = _actualSettings.Hidden.Value();
	ap_config.ap.max_connection = _actualSettings.MaxClients.Value();
	ap_config.ap.channel = _actualSettings.Channel.Value();

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

	memset(&sta_config, 0, sizeof(wifi_config_t));
	strncpy((char *)sta_config.sta.ssid, _actualSettings.StaSsid.Value(), strlen(_actualSettings.StaSsid.Value()));
	strncpy((char *)sta_config.sta.password, _actualSettings.StaPassword.Value(), strlen(_actualSettings.StaPassword.Value()));
	//sta_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
	//sta_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	printf("\nAhooj šéfe, vítej v ovládacím panelu " LOG_BOLD_UNDERLINED(LOG_COLOR_GREEN) "Zá" LOG_BOLD_UNDERLINED(LOG_COLOR_BROWN) "keř" LOG_BOLD_UNDERLINED(LOG_COLOR_CYAN) "né " LOG_BOLD_UNDERLINED(LOG_COLOR_RED) "Žá\033[1;4;38;5;214mrov" LOG_BOLD_UNDERLINED(LOG_COLOR_BLUE) "ki." LOG_RESET_ALL "\n\t--Kdo by nechtěl být žárovkouu? Já!--\nSSID:%s\nSTA:%s\n\n", 
	_actualSettings.ApSsid.Value(), _actualSettings.StaSsid.Value());

	// Sranda začíná. Muhahahahah
	xTaskCreate(&scanSchedulerTask, "scanSchedulerTask", 4096, NULL, 5, NULL);
	
	xEventGroupWaitBits(_event_group, STA_DISCONNECTED_BIT | STA_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	WifiCommands::RegisterAll(_event_group, STA_SCANNING_BIT, STA_SCAN_END_BIT);
	FileCommands::RegisterAll();
	SystemCommands::RegisterAll();
	initialize_console();
}
