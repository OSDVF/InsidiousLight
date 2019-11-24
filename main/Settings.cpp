#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "Settings.hpp"

using namespace Settings;

template <>
void SettingsItem<char *>::Value(char *valToSet)
{
    this->length = strlen(valToSet) + 1;
    if (this->value != NULL)
        this->value = (char *)realloc(this->value, this->length);
    else
        this->value = (char *)malloc(this->length);

    memcpy(this->value, valToSet, this->length);
}
template <>
void SettingsItem<IPv4>::Value(IPv4 valToSet)
{
    this->value.ip = valToSet.ip;
}
template <typename T>
void SettingsItem<T>::Value(T valToSet)
{
    this->value = valToSet;
}
#ifdef DEBUG_CONFIG_WRITES
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
template <>
int SettingsItem<char *>::Write(FILE *f)
{
#ifdef DEBUG_CONFIG_WRITES
    ESP_LOGD(TAG, "Writing string %s", this->value);
#endif
    return fwrite(this->value, sizeof(char), this->length, f);
}
template <>
int SettingsItem<int>::Write(FILE *f)
{
#ifdef DEBUG_CONFIG_WRITES
    ESP_LOGD(TAG, "Writing int %d", this->value);
#endif
    return fwrite(&this->value, sizeof(int), 1, f);
}
template <>
int SettingsItem<IPv4>::Write(FILE *f)
{
#ifdef DEBUG_CONFIG_WRITES
    ESP_LOGD(TAG, "Writing %d.%d.%d.%d", this->value.octets[0], this->value.octets[1], this->value.octets[2], this->value.octets[3]);
#endif
    return fwrite(&this->value.ip, sizeof(uint32_t), 1, f);
}

template <>
int SettingsItem<int>::Load(FILE *f)
{
    auto result = fread(&this->value, sizeof(int), 1, f);
    ;
#ifdef DEBUG_CONFIG_READS
    ESP_LOGD(TAG, "Reading int %d", this->value);
#endif
    return result;
}
template <>
int SettingsItem<IPv4>::Load(FILE *f)
{
    auto result = fread(&this->value.ip, sizeof(uint32_t), 1, f);
#ifdef DEBUG_CONFIG_READS
    ESP_LOGD(TAG, "Reading %d.%d.%d.%d", this->value.octets[0], this->value.octets[1], this->value.octets[2], this->value.octets[3]);
#endif
    return result;
}
template <>
int SettingsItem<char *>::Load(FILE *f)
{
    int readed = 0;
    int i = 0;
    int bufferLen = 20;
    char *buffer = (char *)malloc(sizeof(char) * bufferLen);
    while ((readed = fgetc(f)) != 0 && readed != EOF)
    {
        buffer[i] = (char)readed;
        i++;
        if (i >= bufferLen)
        {
            bufferLen += 20;
            buffer = (char *)realloc(buffer, sizeof(char) * bufferLen);
        }
    }
    buffer[i] = 0;
    this->length = i;
    this->value = buffer;
#ifdef DEBUG_CONFIG_READS
    ESP_LOGD(TAG, "Readed string %s", this->value);
#endif
    return i;
}

template <>
SettingsItem<char *>::~SettingsItem()
{
    free(this->value);
}

template <>
SettingsItem<IPv4>::SettingsItem()
{
    this->length = 0;
    this->value = IPv4();
}

LocalSettings Storage::ActualSettings;
void Storage::Unmount()
{
    // Unmount FATFS
    ESP_LOGD(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount(BasePath.c_str(), s_wl_handle));

    ESP_LOGI(TAG, "FATFS Unmounted");
}
esp_err_t Storage::SaveConfig()
{
    ESP_LOGI(TAG, "Opening config for write");
    FILE *f = fopen(_settingsFile, "wb");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open");
        return ERR_FILE_UNOPENED;
    }
#define FILE_SAVE_CHECK(x, SETTINGS_NAME)                                                     \
    {                                                                                         \
        result = x;                                                                           \
        if (!result)                                                                          \
            ESP_LOGE(TAG, "Could not save setting " SETTINGS_NAME " with result %d", result); \
    }
    int result;
    FILE_SAVE_CHECK(ActualSettings.ApSsid.Write(f), "ApSsid");
    FILE_SAVE_CHECK(ActualSettings.ApPassword.Write(f), "ApPassword");
    FILE_SAVE_CHECK(ActualSettings.MaxClients.Write(f), "MaxClients");
    FILE_SAVE_CHECK(ActualSettings.Channel.Write(f), "Channel");
    FILE_SAVE_CHECK(ActualSettings.Hidden.Write(f), "Hidden");
    FILE_SAVE_CHECK(ActualSettings.ApIP.Write(f), "ApIP");
    FILE_SAVE_CHECK(ActualSettings.ApGateway.Write(f), "ApGateway");
    FILE_SAVE_CHECK(ActualSettings.ApMask.Write(f), "ApMask");

    FILE_SAVE_CHECK(ActualSettings.StaSsid.Write(f), "StaSSID");
    FILE_SAVE_CHECK(ActualSettings.StaPassword.Write(f), "StaPassword");
    fclose(f);

    if (result)
    {
        ESP_LOGI(TAG, "Successfully saved all settings with result %d", result);
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "Could not save all configuration settings");
        return ERR_COULD_NOT_SAVE;
    }
}
void Storage::ResetSettings()
{
    ActualSettings.ApSsid.Value((char*)default_ap_ssid);
    ActualSettings.ApPassword.Value((char*)default_ap_pass);
    ActualSettings.MaxClients.Value(default_max_clients);
    ActualSettings.Channel.Value(default_channel);
    ActualSettings.Hidden.Value(default_hidden);
    ActualSettings.ApIP.Value(default_ap_ip);
    ActualSettings.ApGateway.Value(default_ap_gateway);
    ActualSettings.ApMask.Value(default_ap_mask);

    ActualSettings.StaSsid.Value((char*)default_sta_ssid);
    ActualSettings.StaPassword.Value((char*)default_sta_pass);
}
esp_err_t Storage::OpenConfig(bool createIfDoesNotExist)
{
    bool allRight = false;
    ESP_LOGI(TAG, "Reading file");
    FILE *f = fopen(_settingsFile, "rb");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        if (!createIfDoesNotExist)
            return ERR_FILE_UNOPENED;
        f = fopen(_settingsFile, "wb");
        fclose(f); //Create file
        f = fopen(_settingsFile, "rb");
    }
    else
        allRight = true;
#ifdef DEBUG_FILE_READ
    int readed = EOF;
    int i = 0;
    while ((readed = fgetc(f)) != EOF)
    {
        if (readed == 0)
            printf("\\0");
        else
            putchar(readed);
        i++;
    }
    ESP_LOGI(TAG, "Config file has %d length", i);
    rewind(f);
#endif

#define FILE_READ_CHECK(setting, defaultValue)                                                                               \
    {                                                                                                                        \
        result = setting.Load(f);                                                                                            \
        if (!result)                                                                                                         \
        {                                                                                                                    \
            ESP_LOGE(TAG, "Could not load setting " #setting " with result %d. Using default value " #defaultValue, result); \
            setting.Value(defaultValue);                                                                                     \
            allRight = false;                                                                                                \
        }                                                                                                                    \
    }
#define FILE_READ_CHECK_CONSISTENCE(setting, defaultValue, condition)                                                        \
    {                                                                                                                        \
        result = setting.Load(f);                                                                                            \
        if (!result)                                                                                                         \
        {                                                                                                                    \
            ESP_LOGE(TAG, "Could not load setting " #setting " with result %d. Using default value " #defaultValue, result); \
            setting.Value(defaultValue);                                                                                     \
            allRight = false;                                                                                                \
        }                                                                                                                    \
        if (setting.Value() condition)                                                                                       \
        {                                                                                                                    \
            ESP_LOGW(TAG, "Inconsistent setting " #setting " changing to " #defaultValue);                                   \
            setting.Value(defaultValue);                                                                                     \
        }                                                                                                                    \
    }
    int result = 1;

    FILE_READ_CHECK(ActualSettings.ApSsid, (char*)default_ap_ssid);
    FILE_READ_CHECK(ActualSettings.ApPassword, (char*)default_ap_pass);
    FILE_READ_CHECK_CONSISTENCE(ActualSettings.MaxClients, default_max_clients, <= 0);
    FILE_READ_CHECK(ActualSettings.Channel, default_channel);
    FILE_READ_CHECK(ActualSettings.Hidden, default_hidden);
    FILE_READ_CHECK_CONSISTENCE(ActualSettings.ApIP, default_ap_ip, .ip == 0);
    FILE_READ_CHECK_CONSISTENCE(ActualSettings.ApGateway, default_ap_gateway, .ip == 0);
    FILE_READ_CHECK_CONSISTENCE(ActualSettings.ApMask, default_ap_mask, .ip == 0);

    FILE_READ_CHECK(ActualSettings.StaSsid, (char*)default_sta_ssid);
    FILE_READ_CHECK(ActualSettings.StaPassword, (char*)default_sta_pass);
    fclose(f);

    if (!allRight)
    {
        ESP_ERROR_CHECK(SaveConfig());
    }

    if (result)
    {
        ESP_LOGI(TAG, "Successfully loaded all settings with result %d", result);
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "Could not load all configuration settings");
        return ERR_COULD_NOT_LOAD;
    }

    //ActualSettings.DomainCount.Load(f);
    //ActualSettings.Domains.Load(f);
}
esp_err_t Storage::Mount()
{
    ActualSettings = LocalSettings();
    ESP_LOGI(TAG, "Mounting virtual FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before
    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 4,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};
    esp_err_t err = esp_vfs_fat_spiflash_mount(BasePath.c_str(), "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}