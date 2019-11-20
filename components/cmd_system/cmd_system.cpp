/* Console example — various system commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "cmd_system.h"
#include "esp_vfs_dev.h"
#include "esp_wifi.h"
#include "Systems.cpp"
#include "sdkconfig.h"

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define WITH_TASKS_INFO 1
#endif
const char *TAG = "Console";
void consoleLoop();
void initialize_console()
{
	/* Disable buffering on stdin */
	setvbuf(stdin, NULL, _IONBF, 0);

	/* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
	esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
	/* Move the caret to the beginning of the next line on '\n' */
	esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

	/* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
	const uart_config_t uart_config = {
		.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.use_ref_tick = true};
	ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));

	/* Install UART driver for interrupt-driven reads and writes */
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0 ,
										256, 0, 0, NULL, 0));

	/* Tell VFS to use UART driver */
	esp_vfs_dev_uart_use_driver(UART_NUM_0);

	/* Initialize the console */
	esp_console_config_t console_config = {
		.max_cmdline_length = 256,
		.max_cmdline_args = 8,
#if CONFIG_LOG_COLORS
		.hint_color = atoi(LOG_COLOR_BLUE),
#endif
		.hint_bold = 0
	};
	ESP_ERROR_CHECK(esp_console_init(&console_config));

	/* Configure linenoise line completion library */
	/* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
	linenoiseSetMultiLine(1);

	/* Tell linenoise where to get command completions and hints */
	linenoiseSetCompletionCallback(&esp_console_get_completion);
	linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

	/* Set command history size */
	linenoiseHistorySetMaxLen(100);
	consoleLoop();
}
void consoleLoop()
{
	/* Register commands */
	esp_console_register_help_command();
	register_cmd_system();

	/* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
	#if CONFIG_LOG_COLORS
	const char *prompt = LOG_BOLD(LOG_COLOR_BROWN) "ZŽ> " LOG_RESET_COLOR;
	#else
	const char *prompt = "ZŽ> ";
	#endif
	int probe_status = linenoiseProbe();
	if (probe_status)
	{ /* zero indicates success */
		ESP_LOGW(TAG,"Tvůj terminátor nepodpořuje špeciálné sekvence.\n"
			   "Kchůl featury jako nápověda a historje budou hrát schovku.\n"
			   "Na Woknech zkus PuTTy.");
		linenoiseSetDumbMode(1);
		prompt = "[ZŽ :P]> ";
	}
	/* Main loop */
	while (true)
	{
		/* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
		char *line = linenoise(prompt);
		if (line == NULL)
		{ /* Ignore empty lines */
			continue;
		}
		/* Add the command to the history */
		linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
		/* Save command history to filesystem */
		linenoiseHistorySave(HISTORY_PATH);
#endif

		/* Try to run the command */
		int ret;
		esp_err_t err = esp_console_run(line, &ret);
		if (err == ESP_ERR_NOT_FOUND)
		{
			ESP_LOGE(TAG,"Téndle příkaz něznam");
		}
		else if (err == ESP_ERR_INVALID_ARG)
		{
			ESP_LOGW(TAG,"Invalidní argumentace");
		}
		else if (err == ESP_OK && ret != ESP_OK)
		{
			ESP_LOGE(TAG,"Příkaz řekl něco co něbyla nula: 0x%x (%s)", ret, esp_err_to_name(err));
		}
		else if (err != ESP_OK)
		{
			ESP_LOGE(TAG,"Internátní chybka: %s", esp_err_to_name(err));
		}
		/* linenoise allocates line buffer on the heap, so need to free it */
		linenoiseFree(line);
	}
}

void register_cmd_system()
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
