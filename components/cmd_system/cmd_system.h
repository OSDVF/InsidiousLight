/* Based on esp-idf Console example
Now licensed under CC BY-NC 4.0
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
