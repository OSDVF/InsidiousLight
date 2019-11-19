#pragma once
#include <stdio.h>
#include <cstring>
#include <stdint.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#define ERR_STORAGE_BASE 0x9000 /*!< Starting number of storage error codes */
#define ERR_FILE_UNOPENED ERR_STORAGE_BASE + 1;
#define ERR_COULD_NOT_LOAD ERR_STORAGE_BASE + 2;
#define ERR_COULD_NOT_SAVE ERR_STORAGE_BASE + 3;
#define DEFAULT_AP_IP {.octets = {192, 168, 10, 1}};
#define DEFAULT_AP_GATEWAY {.octets = {192, 168, 10, 1}};
#define DEFAULT_AP_MASK {.octets = {255, 255, 255, 0}};

namespace Settings
{
union IPv4 { //Our small cute IPv4 holder :)
    uint32_t ip;
    uint8_t octets[4];
};
template <typename T>
class SettingsItem
{
private:
    //For arrays
    size_t length;
    T value;

public:
    const T Value();
    void Value(T valToSet);

    /**
     * @brief  Writes value verbatim to a file
     * @note   Each of datatypes has its explicit implementation
     * @param  *f: already opened write-enabled file
     * @retval Return value of native fwrite (and similar) functions
     */
    int Write(FILE *f);
    int Load(FILE *f);
    ~SettingsItem();
    SettingsItem();
};
typedef struct
{
    SettingsItem<char *> ApSsid;
    SettingsItem<char *> ApPassword;
    SettingsItem<int> MaxClients;
    SettingsItem<int> Channel;
    SettingsItem<int> Hidden;
    SettingsItem<IPv4> ApIP;
    SettingsItem<IPv4> ApGateway;
    SettingsItem<IPv4> ApMask;

    SettingsItem<char *> StaSsid;
    SettingsItem<char *> StaPassword;

    SettingsItem<char *> *Domains;
    SettingsItem<int> DomainCount;
} LocalSettings;

class Storage
{
private:
    static const constexpr char *TAG = "Storage";
    // Handle of the wear levelling library instance
    inline static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

    // Mount path for the partition
    static const constexpr char *_base_path = "/spiflash";
    static const constexpr char *_settingsFile = "/spiflash/settings.txt";

public:
    static LocalSettings ActualSettings;
    static esp_err_t Mount();
    static void Unmount();
    static esp_err_t SaveConfig();
    static esp_err_t OpenConfig();
};
} // namespace Settings