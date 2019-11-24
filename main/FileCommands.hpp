#pragma once
#include <stdio.h>
#include <cstring>
#include <string>
#include <stdint.h>
#include "esp_vfs.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_vfs_fat.h"
#include "dirent.h"
#include "Systems.cpp"
#include "Settings.hpp"

namespace FileCommands
{
class DeleteCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        if (argc == 1)
            ESP_LOGE(TAG, "Takhelejó? Používá se to tagle:" LOG_RESET_COLOR " rip /spiflash/ahoj.txt");
        else if(argc == 2)
            if (remove(argv[1]) == 0)
            {
                ESP_LOGI(TAG, "File %s deleted.", argv[1]);
                return 0;
            }
            else
            {
                ESP_LOGE(TAG, "Could not delete file %s", argv[1]);
                return -1;
            }
        else
        {
            return -2;
        }
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "rip",
        .help = "Pohřbi soubora",
        .hint = "<full-path>",
        .func = &DeleteCommand::Execute,
        .argtable = NULL};

    DeleteCommand() : ConsoleCommand(cmd)
    {
    }
};

class ListCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        if (argc < 2)
        {
            ESP_LOGW(TAG, "Musíte říct ve kterém direktáři se máme hrabat");
            return ESP_ERR_INVALID_ARG;
        }
        int returnVal = 0;
        bool recursive;
        if (argc == 3)
        {
            recursive = argv[2][0] == '-' && argv[2][1] == 'r';
        }
        else
            recursive = false;
        std::string path = std::string(Settings::Storage::BasePath);
        path += argv[1];
        returnVal = std_scan((char *)path.c_str());
        return returnVal;
    }
    static int std_scan(char *path)
    {
        struct dirent *de; // Pointer for directory entry
        DIR *dr = opendir(path);
        if (dr == NULL) // opendir returns NULL if couldn't open directory
        {
            ESP_LOGE(TAG, "Could not open directory %s", path);
            return ESP_ERR_FLASH_OP_FAIL;
        }
        auto len = strlen(path);
        if (path[len - 1] == '/')
            path[len - 1] = 0;
        while ((de = readdir(dr)) != NULL)
#if CONFIG_LOG_COLORS
            printf(LOG_COLOR(LOG_COLOR_CYAN) "%s/" LOG_RESET_COLOR "%s\n", path, de->d_name);
#else
            printf("%s/%s\n", path, de->d_name);
#endif

        return closedir(dr);
    }
    static FRESULT ff_scanDir(
        char *path /* Start node to be scanned (***also used as work area***) */, bool recursive)
    {
        FRESULT res;
        FF_DIR dir;
        UINT i;
        static FILINFO fno;

        res = f_opendir(&dir, path); /* Open the directory */
        if (res == FR_OK)
        {
            for (;;)
            {
                res = f_readdir(&dir, &fno); /* Read a directory item */
                if (res != FR_OK)
                {
                    ESP_LOGE(TAG, "Could not get file info");
                    break;
                }
                if (fno.fname[0] == 0)
                    break; /* Break on error or end of dir */
                if (fno.fattrib & AM_DIR)
                { /* It is a directory */
                    i = strlen(path);
                    sprintf(&path[i], "/%s", fno.fname);
                    res = ff_scanDir(path, recursive); /* Enter the directory */
                    if (res != FR_OK)
                        break;
                    path[i] = 0;
                }
                else
                { /* It is a file. */
#if CONFIG_LOG_COLORS
                    printf(LOG_COLOR(LOG_COLOR_BROWN) "%s" LOG_RESET_COLOR "/%s\n", path, fno.fname);
#else
                    printf("%s%s\n", path, fno.fname);
#endif
                }
            }
            f_closedir(&dir);
        }
        else
        {
            ESP_LOGE(TAG, "Error \"%d\" when reading directory %s", res, path);
        }

        return res;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "hrab",
        .help = "Pohrab se v direktáři",
        .hint = "<full-path> [-r]",
        .func = &ListCommand::Execute,
        .argtable = NULL};

    ListCommand() : ConsoleCommand(cmd)
    {
    }
};

class UsageCommand : public ConsoleCommand
{
private:
    inline static void get_fatfs_usage(size_t *out_total_bytes, size_t *out_free_bytes)
    {
        FATFS *fs;
        size_t free_clusters;
        int res = f_getfree("0:", &free_clusters, &fs);
        assert(res == FR_OK);
        size_t total_sectors = (fs->n_fatent - 2) * fs->csize;
        size_t free_sectors = free_clusters * fs->csize;

        // assuming the total size is < 4GiB, should be true for SPI Flash
        if (out_total_bytes != NULL)
        {
            *out_total_bytes = total_sectors * fs->ssize;
        }
        if (out_free_bytes != NULL)
        {
            *out_free_bytes = free_sectors * fs->ssize;
        }
    }

public:
    static int Execute(int argc, char **argv)
    {
        size_t total, freeB;
        get_fatfs_usage(&total, &freeB);
        printf("Totálně: %d\nSvobodné: %d\n", total, freeB);
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "velkost",
        .help = "Řekne jak velké jsou soubory ve flešu.",
        .hint = NULL,
        .func = &UsageCommand::Execute,
        .argtable = NULL};

    UsageCommand() : ConsoleCommand(cmd)
    {
    }
};

class CatCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        if (argc != 2)
            return ESP_ERR_INVALID_ARG;
        FILE *f;
        auto path = Settings::Storage::BasePath + argv[1];
        f = fopen(path.c_str(), "r");
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Mňááu: %s", strerror(errno));
            return errno;
        }
        int readed;
        while ((readed = fgetc(f)) != EOF)
        {
            putchar(readed);
        }
        printf(_NewLine);
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "kocur",
        .help = "Zavolá kocura, který umí číst soubory",
        .hint = "<cestice>",
        .func = &CatCommand::Execute,
        .argtable = NULL};

    CatCommand() : ConsoleCommand(cmd)
    {
    }
};

class NewFolderCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        if (argc != 2)
            return ESP_ERR_INVALID_ARG;
        std::string path = Settings::Storage::BasePath + argv[1];
        auto ret = mkdir(path.c_str(), S_IRWXG | S_IRWXO | S_IRWXU);
        if (ret == 0)
            ESP_LOGI(TAG, "Úspěšně zrobeno");
        else
            ESP_LOGE(TAG, "Nepodařilo se zrobit složku: %s", strerror(errno));
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "zrob",
        .help = "Zrobí takou ďuru do kterej potom můžete házet soubory.",
        .hint = "<cestice>",
        .func = &NewFolderCommand::Execute,
        .argtable = NULL};

    NewFolderCommand() : ConsoleCommand(cmd)
    {
    }
};
static void RegisterAll()
{
    ListCommand();
    CatCommand();
    DeleteCommand();
    UsageCommand();
    NewFolderCommand();
}
} // namespace FileCommands
