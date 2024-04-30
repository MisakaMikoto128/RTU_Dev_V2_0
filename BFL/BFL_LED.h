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
    LED1,
    LED2,
    LED3,
    LED4,
    LED5,
    LED6,
    LED_NUM,
} LED_t;

void BFL_LED_init();
void BFL_LED_on(LED_t led_id);
void BFL_LED_off(LED_t led_id);
void BFL_LED_toggle(LED_t led_id);
#ifdef __cplusplus
}
#endif
#endif //! BFL_LED_H