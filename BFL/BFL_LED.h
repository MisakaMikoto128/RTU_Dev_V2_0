/**
 * @file BFL_LED.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-12-13
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#ifndef BFL_LED_H
#define BFL_LED_H
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    LED1 = 0,
    LED_NUM,
} LED_t;

void BFL_LED_Init();
void BFL_LED_On(LED_t led_id);
void BFL_LED_Off(LED_t led_id);
void BFL_LED_Toggle(LED_t led_id);
#ifdef __cplusplus
}
#endif
#endif //! BFL_LED_H