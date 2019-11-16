#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "Systems.cpp"

#ifndef _WIFI_COMMANDS
#define _WIFI_COMMANDS

class ClientsListCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
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
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "clients",
        .help = "Show list of Wifi stations connected to me",
        .hint = NULL,
        .func = &ClientsListCommand::Execute,
    };
    ClientsListCommand() : ConsoleCommand(cmd)
    {
    }
};

#endif