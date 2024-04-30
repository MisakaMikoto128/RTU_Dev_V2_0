/**
 * @file APP_Main.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-12
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#ifndef APP_MAIN_H
#define APP_MAIN_H

void APP_Main();
void APP_Main_test();
// #define RTU_DEV_ID (0x0102030405060708ULL)

#define RTU_DEV_ID (0x0000000000000001ULL)

void BFL_4G_test();
#define MAX_REC_NUM_IN_ONE_PACK 60

#endif //! APP_MAIN_H