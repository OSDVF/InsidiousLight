#pragma once
#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "Systems.cpp"
#include "Wifi Functions.cpp"

namespace WifiCommands
{
class ClientsListCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        printf("Connected stations:\n");

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

        printf(_NewLine);
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "clients",
        .help = "Show list of Wifi stations connected to me",
        .hint = NULL,
        .func = &ClientsListCommand::Execute,
        .argtable = NULL
    };
    ClientsListCommand() : ConsoleCommand(cmd)
    {
    }
};

class WifiCommand : public ConsoleCommand
{
public:
    static inline EventGroupHandle_t _scanEventGroup;
    static inline int _startBit;
    static inline int _stopBit;
    static int Execute(int argc, char **argv)
    {
        int nerrors = arg_parse(argc, argv, (void **)_args);
        if (nerrors != 0)
        {
            arg_print_errors(stderr, _args->end, argv[0]);
            return 1;
        }
        wifi_mode_t mode;
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_get_mode(&mode));
        if (_args->scan->count)
        {
            WifiFunctions::Scan(_scanEventGroup, _startBit, _stopBit);
        }
        else if (_args->interface->count)
        {
            if (!_args->param->count)
                badcommand();
            if (strcmp(commands[1], _args->interface->sval[0]) == 0) //AP
            {

                if (strcmp(commands[3], _args->action->sval[0]) == 0) //UP
                {
                    switch (mode)
                    {
                    case WIFI_MODE_STA:
                        ESP_LOGI(TAG, "Switching to APSTA");
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_APSTA));
                        break;
                    case WIFI_MODE_AP:
                    case WIFI_MODE_APSTA:
                        ESP_LOGW(TAG, "AP is already up");
                        break;
                    default:
                        ESP_LOGE(TAG, "Driver is in invalid state");
                        [[fallthrough]]
                    case WIFI_MODE_NULL:
                        ESP_LOGI(TAG, "Starting and switching to AP");
                        ESP_ERROR_CHECK(esp_wifi_start());
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_AP));
                        break;
                    }
                }
                else if (strcmp(commands[4], _args->action->sval[0]) == 0) //DOWN
                {
                    switch (mode)
                    {
                    case WIFI_MODE_AP:
                        ESP_LOGI(TAG, "Stopping driver");
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_deauth_sta(0));
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_NULL));
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
                        break;
                    case WIFI_MODE_NULL:
                    [[fallthrough]]
                    case WIFI_MODE_STA:
                        ESP_LOGW(TAG, "AP is already down");
                        break;
                    case WIFI_MODE_APSTA:
                        ESP_LOGI(TAG, "Switching to STA");
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));
                        break;
                    default:
                        ESP_LOGE(TAG, "Driver is in invalid state");
                        break;
                    }
                }
                else
                    badcommand();
            }
            else if (strcmp(commands[2], _args->interface->sval[0]) == 0) //STA
            {
                if (strcmp(commands[3], _args->action->sval[0]) == 0) //UP
                {
                    switch (mode)
                    {
                    case WIFI_MODE_AP:
                        ESP_LOGI(TAG, "Switching to APSTA");
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_APSTA));
                        break;
                    case WIFI_MODE_APSTA:
                        [[fallthrough]]
                    case WIFI_MODE_STA:
                        ESP_LOGW(TAG, "STA is already up");
                        break;
                    default:
                        ESP_LOGE(TAG, "Driver is in invalid state");
                        [[fallthrough]]
                    case WIFI_MODE_NULL:
                        ESP_LOGI(TAG, "Starting and switching to STA");
                        ESP_ERROR_CHECK(esp_wifi_start());
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));
                        break;
                    }
                }
                else if (strcmp(commands[4], _args->action->sval[0]) == 0) //DOWN
                {
                    switch (mode)
                    {
                    case WIFI_MODE_STA:
                        ESP_LOGI(TAG, "Stopping wifi driver");
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_NULL));
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
                        break;
                    case WIFI_MODE_NULL:
                        [[fallthrough]]
                    case WIFI_MODE_AP:
                        ESP_LOGW(TAG, "STA is already down");
                        break;
                    case WIFI_MODE_APSTA:
                        ESP_LOGI(TAG, "Disconnecting and switching to AP");
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
                        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_AP));
                        break;
                    default:
                        ESP_LOGE(TAG, "Driver is in invalid state");
                        break;
                    }
                }
                else
                    badcommand();
            }
        }
        else
        {
            switch (mode)
            {
            case WIFI_MODE_STA:
                ESP_LOGI(TAG, "Actual mode: STA");
                break;
            case WIFI_MODE_AP:
                ESP_LOGI(TAG, "Actual mode: AP");
                break;
            case WIFI_MODE_APSTA:
                ESP_LOGI(TAG, "Actual mode: APSTA");
                break;
            case WIFI_MODE_NULL:
                ESP_LOGI(TAG, "Actual mode: NULL");
                break;
            case WIFI_MODE_MAX:
                ESP_LOGI(TAG, "Actual mode: MAX");
                break;
            }
        }

        return 0;
    }
    WifiCommand(EventGroupHandle_t e, const int startBit, const int stopBit) : ConsoleCommand(InitializeArgTable())
    {
        _scanEventGroup = e;
        _startBit = startBit;
        _stopBit = stopBit;
    }

private:
    static void badcommand()
    {
        ESP_LOGE(TAG, "Bad-formed command. See \"help\"");
    }
    static const constexpr char *commands[] = {
        "scan", "ap", "sta", "up", "down", "ssid", "pass"};
    typedef struct
    {
        struct arg_lit *scan;
        struct arg_str *interface;
        struct arg_str *action;
        struct arg_str *param;
        struct arg_end *end;
    } Args;
    inline static Args *_args;
    static esp_console_cmd_t InitializeArgTable()
    {
        static Args args =
            {
                .scan = arg_lit0(NULL, commands[0], "Perform a scan and show stations in range"),
                .interface = arg_str0(NULL, NULL, "ap | sta | [nothing]", "Interfác, kterého chceš configovat. Pokud [nothing], tak vypíše info"),
                .action = arg_str0(NULL, NULL, "up | down | ssid | pass", "Co v tym interfácu chceš configovat"),
                .param = arg_str0(NULL, NULL, "<ssid> | <password>", "Tu hodíš na jakou hodnotu to ceš hodit"),
                .end = arg_end(4)};
        static constexpr const esp_console_cmd_t cmd = {
            .command = "wifi",
            .help = "WiFi controller features",
            .hint = NULL,
            .func = &WifiCommand::Execute,
            .argtable = &args};
        _args = &args;
        return cmd;
    }
};
static void RegisterAll(EventGroupHandle_t e, const int startBit, const int stopBit)
{
    ClientsListCommand();
    WifiCommand(e,startBit,stopBit);
}
}