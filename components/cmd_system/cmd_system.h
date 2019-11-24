/* Console example — various system commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
static const constexpr char *_NewLine = "\n";
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Enters into endless loop which listens to incoming commands
 * @note   Really endless!
 */
void initialize_console();

#ifdef __cplusplus
}
#endif
