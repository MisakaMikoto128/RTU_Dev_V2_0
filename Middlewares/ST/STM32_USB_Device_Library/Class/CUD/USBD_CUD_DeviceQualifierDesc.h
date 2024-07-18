/**
* @file USBD_CUD_DeviceQualifierDesc.h
* @author Liu Yuanlin (liuyuanlins@outlook.com)
* @brief
* @version 0.1
* @date 2024-07-08
* @last modified 2024-07-08
*
* @copyright Copyright (c) 2024 Liu Yuanlin Personal.
*
*/
#ifndef USBD_CUD_DEVICEQUALIFIERDESC_H
#define USBD_CUD_DEVICEQUALIFIERDESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_cud.h"

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CUD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
    {
        USB_LEN_DEV_QUALIFIER_DESC,
        USB_DESC_TYPE_DEVICE_QUALIFIER,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        CUD_MAX_FS_PACKET,
        0x01,
        0x00,
};
#ifdef __cplusplus
}
#endif
#endif //!USBD_CUD_DEVICEQUALIFIERDESC_H
