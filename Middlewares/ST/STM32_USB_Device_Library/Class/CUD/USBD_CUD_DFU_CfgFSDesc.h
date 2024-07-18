/**
* @file USBD_CUD_DFU_CfgFSDesc.h
* @author Liu Yuanlin (liuyuanlins@outlook.com)
* @brief
* @version 0.1
* @date 2024-07-08
* @last modified 2024-07-08
*
* @copyright Copyright (c) 2024 Liu Yuanlin Personal.
*
*/
#ifndef USBD_CUD_DFU_CFGFSDESC_H
#define USBD_CUD_DFU_CFGFSDESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_cud.h"

__ALIGN_BEGIN static uint8_t USBD_CUD_DFU_CfgFSDesc[USB_DFU_CONFIG_DESC_SIZ] __ALIGN_END =
    {
        0x09,                        /* bLength: Configuration Descriptor size */
        USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType: Configuration */
        USB_DFU_CONFIG_DESC_SIZ,

        0x00,
        0x01, /* bNumInterfaces: 1 interface */
        0x01, /* bConfigurationValue: */
        0x04, /* iConfiguration: */
#if (USBD_SELF_POWERED == 1U)
        0xC0, /* bmAttributes: Bus Powered according to user configuration */
#else
        0x80, /* bmAttributes: Bus Powered according to user configuration */
#endif
        USBD_MAX_POWER, /* MaxPower 100 mA */
#if ENBALE_DUF_CFGDESC
        /**********  Descriptor of DFU interface 0 Alternate setting 0 **************/
        USBD_DFU_IF_DESC(0U), /* This interface is mandatory for all devices */

#if (USBD_DFU_MAX_ITF_NUM > 1U)
        /**********  Descriptor of DFU interface 0 Alternate setting 1 **************/
        USBD_DFU_IF_DESC(1),
#endif /* (USBD_DFU_MAX_ITF_NUM > 1) */

#if (USBD_DFU_MAX_ITF_NUM > 2U)
        /**********  Descriptor of DFU interface 0 Alternate setting 2 **************/
        USBD_DFU_IF_DESC(2),
#endif /* (USBD_DFU_MAX_ITF_NUM > 2) */

#if (USBD_DFU_MAX_ITF_NUM > 3U)
        /**********  Descriptor of DFU interface 0 Alternate setting 3 **************/
        USBD_DFU_IF_DESC(3),
#endif /* (USBD_DFU_MAX_ITF_NUM > 3) */

#if (USBD_DFU_MAX_ITF_NUM > 4U)
        /**********  Descriptor of DFU interface 0 Alternate setting 4 **************/
        USBD_DFU_IF_DESC(4),
#endif /* (USBD_DFU_MAX_ITF_NUM > 4) */

#if (USBD_DFU_MAX_ITF_NUM > 5U)
        /**********  Descriptor of DFU interface 0 Alternate setting 5 **************/
        USBD_DFU_IF_DESC(5),
#endif /* (USBD_DFU_MAX_ITF_NUM > 5) */

#if (USBD_DFU_MAX_ITF_NUM > 6U)
#error "ERROR: usbd_dfu_core.c: Modify the file to support more descriptors!"
#endif /* (USBD_DFU_MAX_ITF_NUM > 6) */

        /******************** DFU Functional Descriptor********************/
        0x09,                /* blength = 9 Bytes */
        DFU_DESCRIPTOR_TYPE, /* DFU Functional Descriptor */
        0x0B,                /* bmAttribute:
                                bitCanDnload             = 1      (bit 0)
                                bitCanUpload             = 1      (bit 1)
                                bitManifestationTolerant = 0      (bit 2)
                                bitWillDetach            = 1      (bit 3)
                                Reserved                          (bit4-6)
                                bitAcceleratedST         = 0      (bit 7) */
        0xFF,                /* DetachTimeOut= 255 ms*/
        0x00,
        /* WARNING: In DMA mode the multiple MPS packets feature is still not supported
         ==> In this case, when using DMA USBD_DFU_XFER_SIZE should be set to 64 in usbd_conf.h */
        TRANSFER_SIZE_BYTES(USBD_DFU_XFER_SIZE), /* TransferSize = 1024 Byte */
        0x1A,                                    /* bcdDFUVersion */
        0x01,
/***********************************************************/
/* 9*/
#endif
};
#ifdef __cplusplus
}
#endif
#endif //!USBD_CUD_DFU_CFGFSDESC_H
