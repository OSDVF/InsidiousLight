#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "Settings.hpp"

using namespace Settings;

template <>
void SettingsItem<char *>::Value(char *valToSet)
{
    this->length = strlen(valToSet) + 1;
    if (this->value != nullptr)
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

template <>
int SettingsItem<char *>::Write(FILE *f)
{
    return fwrite(this->value, sizeof(char), this->length, f);
}
template <>
int SettingsItem<int>::Write(FILE *f)
{
    return fwrite(&this->value, sizeof(int), 1, f);
}
template <>
int SettingsItem<IPv4>::Write(FILE *f)
{
    return fwrite(&this->value.ip, sizeof(uint32_t), 1, f);
}

template <>
int SettingsItem<int>::Load(FILE *f)
{
    return fread(&this->value, sizeof(int), 1, f);
}
template <>
int SettingsItem<IPv4>::Load(FILE *f)
{
    return fread(&this->value.ip, sizeof(uint32_t), 1, f);
}
template <>
int SettingsItem<char *>::Load(FILE *f)
{
    int readed = 0;
    int i = 0;
    int bufferLen = 20;
    char *buffer= (char *)malloc(sizeof(char) * bufferLen);
    do
    {
        readed = fgetc(f);
        buffer[i] = (char)readed;
        i++;
        if (i >= bufferLen)
        {
            bufferLen += 20;
            buffer = (char *)realloc(buffer, sizeof(char) * bufferLen);
        }
    } while (readed != 0 && readed != EOF);
    this->length = i;
    this->value = buffer;
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

template <typename T>
SettingsItem<T>::~SettingsItem()
{
}
template <typename T>
SettingsItem<T>::SettingsItem()
{
    this->length = 0;
    this->value = NULL;
}
template <typename T>
const T SettingsItem<T>::Value()
{
    return this->value;
}
template <typename T>
void SettingsItem<T>::Value(T valToSet)
{
    this->value = valToSet;
}

LocalSettings Storage::ActualSettings;
void Storage::Unmount()
{
    // Unmount FATFS
    ESP_LOGD(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount(_base_path, s_wl_handle));

    ESP_LOGI(TAG, "FATFS Unmounted");
}
esp_err_t Storage::SaveConfig()
{
    ESP_LOGI(TAG, "Opening config");
    FILE *f = fopen(_settingsFile, "wb");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
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
esp_err_t Storage::OpenConfig()
{
    ESP_LOGI(TAG, "Reading file");
    FILE *f = fopen(_settingsFile, "rb");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ERR_FILE_UNOPENED;
    }
#define FILE_READ_CHECK(x, SETTINGS_NAME)                                                     \
    {                                                                                         \
        result = x;                                                                           \
        if (!result)                                                                          \
            ESP_LOGE(TAG, "Could not load setting " SETTINGS_NAME " with result %d", result); \
    }
    int result;
    FILE_READ_CHECK(ActualSettings.ApSsid.Load(f), "ApSsid");
    FILE_READ_CHECK(ActualSettings.ApPassword.Load(f), "ApPassword");
    FILE_READ_CHECK(ActualSettings.MaxClients.Load(f), "MaxClients");
    FILE_READ_CHECK(ActualSettings.Channel.Load(f), "Channel");
    FILE_READ_CHECK(ActualSettings.Hidden.Load(f), "Hidden");
    FILE_READ_CHECK(ActualSettings.ApIP.Load(f), "ApIP");
    FILE_READ_CHECK(ActualSettings.ApGateway.Load(f), "ApGateway");
    FILE_READ_CHECK(ActualSettings.ApMask.Load(f), "ApMask");

    FILE_READ_CHECK(ActualSettings.StaSsid.Load(f), "StaSSID");
    FILE_READ_CHECK(ActualSettings.StaPassword.Load(f), "StaPassword");
    fclose(f);

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
    esp_err_t err = esp_vfs_fat_spiflash_mount(_base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}