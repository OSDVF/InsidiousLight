#pragma once
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/uart.h"

#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"

#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "cmd_system.h"

#include "sdkconfig.h"
#ifndef _SYS_COMMANDS
#define _SYS_COMMANDS
#if CONFIG_ESP_CONSOLE_UART_NUM == 0
#undef CONFIG_ESP_CONSOLE_UART_NUM
#define CONFIG_ESP_CONSOLE_UART_NUM UART_NUM_0
#elif CONFIG_ESP_CONSOLE_UART_NUM == 1
#undef CONFIG_ESP_CONSOLE_UART_NUM
#define CONFIG_ESP_CONSOLE_UART_NUM UART_NUM_1
#endif

class ConsoleCommand
{
protected:
    static const constexpr char *TAG = "CMD";

public:
    ConsoleCommand(const esp_console_cmd_t cmd)
    {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    }
};


namespace SystemCommands
{
class VersionCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        esp_chip_info_t info;
        esp_chip_info(&info);
        printf("IDF Version:%s\r\n", esp_get_idf_version());
        printf("Chip info:\r\n");
        printf("\tmodel:%s\r\n", info.model == CHIP_ESP32 ? "ESP32" : "Unknow");
        printf("\tcores:%d\r\n", info.cores);
        printf("\tfeature:%s%s%s%s%d%s\r\n",
               info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
               info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
               info.features & CHIP_FEATURE_BT ? "/BT" : "",
               info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
               spi_flash_get_chip_size() / (1024 * 1024), " MB");
        printf("\trevision number:%d\r\n", info.revision);
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Get version of chip and SDK",
        .hint = NULL,
        .func = &VersionCommand::Execute,
        .argtable = NULL};
    VersionCommand() : ConsoleCommand(cmd)
    {
    }
};

class RestartCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        ESP_LOGW(TAG, "Restarting");
        esp_restart();
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Software reset of the chip",
        .hint = NULL,
        .func = &RestartCommand::Execute,
        .argtable = NULL};
    RestartCommand() : ConsoleCommand(cmd) {}
};

class FreeCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        printf("%d\n", esp_get_free_heap_size());
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "free",
        .help = "Get the current size of free heap memory",
        .hint = NULL,
        .func = &FreeCommand::Execute,
        .argtable = NULL};
    FreeCommand() : ConsoleCommand(cmd) {}
};

class HeapCommand : public ConsoleCommand
{
public:
    static int Execute(int argc, char **argv)
    {
        uint32_t heap_size = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
        ESP_LOGI(TAG, "min heap size: %u", heap_size);
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "heap",
        .help = "Get minimum size of free heap memory that was available during program execution",
        .hint = NULL,
        .func = &HeapCommand::Execute,
        .argtable = NULL};
    HeapCommand() : ConsoleCommand(cmd) {}
};

/** 'tasks' command prints the list of tasks and related information */
#if CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
class TasksInfoCommand : public ConsoleCommand
{
private:
#if CONFIG_LOG_COLORS
    static void vColorTaskList(char *pcWriteBuffer)
    {
        static const char constexpr *states[] = {LOG_COLOR(LOG_COLOR_GREEN) "R" LOG_RESET_COLOR, LOG_COLOR(LOG_COLOR_RED) "B" LOG_RESET_COLOR, LOG_COLOR(LOG_COLOR_BLUE) "S" LOG_RESET_COLOR, LOG_COLOR(LOG_COLOR_BROWN) "D" LOG_RESET_COLOR, LOG_COLOR(LOG_COLOR_RED) "U" LOG_RESET_COLOR};
        TaskStatus_t *pxTaskStatusArray;
        volatile UBaseType_t uxArraySize, x;
        /* Make sure the write buffer does not contain a string. */
        *pcWriteBuffer = 0x00;

        /* Take a snapshot of the number of tasks in case it changes while this
		function is executing. */
        uxArraySize = uxTaskGetNumberOfTasks();

        /* Allocate an array index for each task.  NOTE!  if
		configSUPPORT_DYNAMIC_ALLOCATION is set to 0 then pvPortMalloc() will
		equate to NULL. */
        pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

        if (pxTaskStatusArray != NULL)
        {
            /* Generate the (binary) data. */
            uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);

            /* Create a human readable table from the binary data. */
            for (x = 0; x < uxArraySize; x++)
            {
                /* Write the task name to the string, padding with spaces so it
				can be printed in tabular form more easily. */
                BaseType_t y;

                /* Start by copying the entire string. */
                strcpy(pcWriteBuffer, pxTaskStatusArray[x].pcTaskName);

                /* Pad the end of the string with spaces to ensure columns line up when
		printed out. */
                for (y = strlen(pcWriteBuffer); y < (configMAX_TASK_NAME_LEN - 1); y++)
                {
                    pcWriteBuffer[y] = ' ';
                }

                /* Terminate. */
                pcWriteBuffer[y] = 0x00;

                /* Return the new end of string. */
                pcWriteBuffer = &(pcWriteBuffer[y]);

                int stIndex = pxTaskStatusArray[x].eCurrentState - 1;
                if (stIndex > 4 || stIndex < 0)
                    stIndex = 5;

                    /* Write the rest of the string. */
#if configTASKLIST_INCLUDE_COREID
                sprintf(pcWriteBuffer, "\t%s\t%u\t%u\t%u\t%hd\r\n", states[stIndex], (unsigned int)pxTaskStatusArray[x].uxCurrentPriority, (unsigned int)pxTaskStatusArray[x].usStackHighWaterMark, (unsigned int)pxTaskStatusArray[x].xTaskNumber, (int)pxTaskStatusArray[x].xCoreID);
#else
                sprintf(pcWriteBuffer, "\t%s\t%u\t%u\t%u\r\n", states[stIndex], (unsigned int)pxTaskStatusArray[x].uxCurrentPriority, (unsigned int)pxTaskStatusArray[x].usStackHighWaterMark, (unsigned int)pxTaskStatusArray[x].xTaskNumber);
#endif
                pcWriteBuffer += strlen(pcWriteBuffer);
            }

            /* Free the array again.  NOTE!  If configSUPPORT_DYNAMIC_ALLOCATION
			is 0 then vPortFree() will be #defined to nothing. */
            vPortFree(pxTaskStatusArray);
        }
        else
        {
            ESP_LOGE(TAG, "Prázdný status array");
        }
    }
#endif
public:
    static int Execute(int argc, char **argv)
    {
        const size_t bytes_per_task =
#if CONFIG_LOG_COLORS
            50;
#else
            40; /* see vTaskList description */
#endif
        char *task_list_buffer = (char *)malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
        if (task_list_buffer == NULL)
        {
            ESP_LOGE(TAG, "failed to allocate buffer for vTaskList output");
            return 1;
        }
        fputs("Task Name\tStatus\tPrio\tHWM\tTask#", stdout);
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
        fputs("\tAffinity", stdout);
#endif
        fputs(_NewLine, stdout);
#if CONFIG_LOG_COLORS
        vColorTaskList(task_list_buffer);
#else
        vTaskList(task_list_buffer);
#endif
        fputs(task_list_buffer, stdout);
        free(task_list_buffer);
        return 0;
    }
    static constexpr const esp_console_cmd_t cmd = {
        .command = "tasks",
        .help = "Get information about running tasks",
        .hint = NULL,
        .func = &TasksInfoCommand::Execute,
        .argtable = NULL};
    TasksInfoCommand() : ConsoleCommand(cmd) {}
};

#endif // WITH_TASKS_INFO

/** 'deep_sleep' command puts the chip into deep sleep mode */
class SleepCommand : public ConsoleCommand
{
protected:
    typedef struct
    {
        struct arg_int *wakeup_time;
        struct arg_int *wakeup_gpio_num;
        struct arg_int *wakeup_gpio_level;
        struct arg_end *end;
    } sleep_args;

public:
    SleepCommand(const esp_console_cmd_t cmd) : ConsoleCommand(cmd) {}
};
class DeepSleepCommand : public SleepCommand
{
private:
    inline static sleep_args *_args;
    static esp_console_cmd_t InitializeArgTable()
    {
        static sleep_args args =
            {
                .wakeup_time = arg_int0("t", "time", "<t>", "Wake up time, ms"),
                .wakeup_gpio_num = arg_int0(NULL, "io", "<n>",
                                            "If specified, wakeup using GPIO with given number"),
                .wakeup_gpio_level = arg_int0(NULL, "io_level", "<0|1>", "GPIO level to trigger wakeup"),
                .end = arg_end(3)};
        static constexpr const esp_console_cmd_t cmd = {
            .command = "deep_sleep",
            .help = "Enter deep sleep mode. "
                    "Two wakeup modes are supported: timer and GPIO. "
                    "If no wakeup option is specified, will sleep indefinitely.",
            .hint = NULL,
            .func = &DeepSleepCommand::Execute,
            .argtable = &args};
        _args = &args;
        return cmd;
    }

public:
    static int Execute(int argc, char **argv)
    {
        int nerrors = arg_parse(argc, argv, (void **)_args);
        if (nerrors != 0)
        {
            arg_print_errors(stderr, _args->end, argv[0]);
            return 1;
        }
        if (_args->wakeup_time->count)
        {
            uint64_t timeout = 1000ULL * _args->wakeup_time->ival[0];
            ESP_LOGI(TAG, "Enabling timer wakeup, timeout=%lluus", timeout);
            ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(timeout));
        }
        if (_args->wakeup_gpio_num->count)
        {
            gpio_num_t io_num = (gpio_num_t)_args->wakeup_gpio_num->ival[0];
            if (!rtc_gpio_is_valid_gpio(io_num))
            {
                ESP_LOGE(TAG, "GPIO %d is not an RTC IO", io_num);
                return 1;
            }
            int level = 0;
            if (_args->wakeup_gpio_level->count)
            {
                level = _args->wakeup_gpio_level->ival[0];
                if (level != 0 && level != 1)
                {
                    ESP_LOGE(TAG, "Invalid wakeup level: %d", level);
                    return 1;
                }
            }
            ESP_LOGI(TAG, "Enabling wakeup on GPIO%d, wakeup on %s level",
                     io_num, level ? "HIGH" : "LOW");

            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << io_num, (esp_sleep_ext1_wakeup_mode_t)level));
        }
        rtc_gpio_isolate(GPIO_NUM_12);
        esp_deep_sleep_start();
    }
    DeepSleepCommand() : SleepCommand(InitializeArgTable()) {}
};

/** 'light_sleep' command puts the chip into light sleep mode */
class LightSleepCommand : public SleepCommand
{
private:
    inline static sleep_args *_args;
    static esp_console_cmd_t InitializeArgTable()
    {
        static sleep_args args =
            {
                .wakeup_time = arg_int0("t", "time", "<t>", "Wake up time, ms"),
                .wakeup_gpio_num = arg_intn(NULL, "io", "<n>", 0, 8,
                                            "If specified, wakeup using GPIO with given number"),
                .wakeup_gpio_level = arg_intn(NULL, "io_level", "<0|1>", 0, 8, "GPIO level to trigger wakeup"),
                .end = arg_end(3)};
        static constexpr const esp_console_cmd_t cmd = {
            .command = "light_sleep",
            .help = "Enter light sleep mode. "
                    "Two wakeup modes are supported: timer and GPIO. "
                    "Multiple GPIO pins can be specified using pairs of "
                    "'io' and 'io_level' arguments. "
                    "Will also wake up on UART input.",
            .hint = NULL,
            .func = &LightSleepCommand::Execute,
            .argtable = &args};
        _args = &args;
        return cmd;
    }

public:
    static int Execute(int argc, char **argv)
    {
        int nerrors = arg_parse(argc, argv, (void **)_args);
        if (nerrors != 0)
        {
            arg_print_errors(stderr, _args->end, argv[0]);
            return 1;
        }
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        if (_args->wakeup_time->count)
        {
            uint64_t timeout = 1000ULL * _args->wakeup_time->ival[0];
            ESP_LOGI(TAG, "Enabling timer wakeup, timeout=%lluus", timeout);
            ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(timeout));
        }
        int io_count = _args->wakeup_gpio_num->count;
        if (io_count != _args->wakeup_gpio_level->count)
        {
            ESP_LOGE(TAG, "Should have same number of 'io' and 'io_level' arguments");
            return 1;
        }
        for (int i = 0; i < io_count; ++i)
        {
            int io_num = _args->wakeup_gpio_num->ival[i];
            int level = _args->wakeup_gpio_level->ival[i];
            if (level != 0 && level != 1)
            {
                ESP_LOGE(TAG, "Invalid wakeup level: %d", level);
                return 1;
            }
            ESP_LOGI(TAG, "Enabling wakeup on GPIO%d, wakeup on %s level",
                     io_num, level ? "HIGH" : "LOW");

            ESP_ERROR_CHECK(gpio_wakeup_enable((gpio_num_t)io_num, level ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL));
        }
        if (io_count > 0)
        {
            ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
        }
        if (CONFIG_ESP_CONSOLE_UART_NUM <= UART_NUM_1)
        {
            ESP_LOGI(TAG, "Enabling UART wakeup (press ENTER to exit light sleep)");
            ESP_ERROR_CHECK(uart_set_wakeup_threshold(CONFIG_ESP_CONSOLE_UART_NUM, 3));
            ESP_ERROR_CHECK(esp_sleep_enable_uart_wakeup(CONFIG_ESP_CONSOLE_UART_NUM));
        }
        fflush(stdout);
        uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);
        esp_light_sleep_start();
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        const char *cause_str;
        switch (cause)
        {
        case ESP_SLEEP_WAKEUP_GPIO:
            cause_str = "GPIO";
            break;
        case ESP_SLEEP_WAKEUP_UART:
            cause_str = "UART";
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            cause_str = "timer";
            break;
        default:
            cause_str = "unknown";
            printf("%d\n", cause);
        }
        ESP_LOGI(TAG, "Woke up from: %s", cause_str);
        return 0;
    }
    LightSleepCommand() : SleepCommand(InitializeArgTable()) {}
};

static void RegisterAll()
{
    FreeCommand();
    HeapCommand();
    RestartCommand();
    DeepSleepCommand();
    VersionCommand();
    LightSleepCommand();
#if WITH_TASKS_INFO
    TasksInfoCommand();
#endif
}
} // namespace SystemCommands
#endif