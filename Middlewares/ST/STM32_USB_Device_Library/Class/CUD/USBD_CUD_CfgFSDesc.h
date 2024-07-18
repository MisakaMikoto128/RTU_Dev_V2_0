/**
 * @file USBD_CUD_CfgFSDesc.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-07-08
 * @last modified 2024-07-08
 *
 * @copyright Copyright (c) 2024 Liu Yuanlin Personal.
 *
 */
#ifndef USBD_CUD_CFGFSDESC_H
#define USBD_CUD_CFGFSDESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_cud.h"

/* USB Mass storage device Configuration Descriptor */
/* All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
__ALIGN_BEGIN static uint8_t USBD_CUD_CfgFSDesc[USB_CUD_CONFIG_DESC_SIZ] __ALIGN_END =
    {
        0x09,                        /* bLength: Configuration Descriptor size */
        USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType: Configuration */
        USB_CUD_CONFIG_DESC_SIZ,

        0x00,
        USBD_INTERFACE_NUM, /* bNumInterfaces: 1 interface */
        0x01,               /* bConfigurationValue: */
        0x04,               /* iConfiguration: */
#if (USBD_SELF_POWERED == 1U)
        0xC0, /* bmAttributes: Bus Powered according to user configuration */
#else
        0x80, /* bmAttributes: Bus Powered according to user configuration */
#endif
        USBD_MAX_POWER, /* MaxPower 100 mA */

        /********************  Mass Storage interface ********************/
        0x09,                    /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE, /* bDescriptorType: */
        USBD_MSC_INTERFACE_NUM,  /* bInterfaceNumber: Number of Interface */
        0x00,                    /* bAlternateSetting: Alternate setting */
        0x02,                    /* bNumEndpoints*/
        0x08,                    /* bInterfaceClass: CUD Class */
        0x06,                    /* bInterfaceSubClass : SCSI transparent*/
        0x50,                    /* nInterfaceProtocol */
        0x05,                    /* iInterface: */
        /********************  Mass Storage Endpoints ********************/
        0x07,          /* Endpoint descriptor length = 7 */
        0x05,          /* Endpoint descriptor type */
        MSC_EPIN_ADDR, /* Endpoint address (IN, address 1) */
        0x02,          /* Bulk endpoint type */
        LOBYTE(CUD_MAX_FS_PACKET),
        HIBYTE(CUD_MAX_FS_PACKET),
        0x00, /* Polling interval in milliseconds */

        0x07,           /* Endpoint descriptor length = 7 */
        0x05,           /* Endpoint descriptor type */
        MSC_EPOUT_ADDR, /* Endpoint address (OUT, address 1) */
        0x02,           /* Bulk endpoint type */
        LOBYTE(CUD_MAX_FS_PACKET),
        HIBYTE(CUD_MAX_FS_PACKET),
        0x00, /* Polling interval in milliseconds */

        /* IAD Descriptor */
        0x08,                   /* bLength: Interface Descriptor size */
        0x0B,                   /* bDescriptorType: IAD */
        USBD_CDC_INTERFACE_NUM, /* bFirstInterface */
        0x02,                   /* bInterfaceCount */
        0x02,                   /* bFunctionClass: CDC */
        0x02,                   /* bFunctionSubClass */
        0x01,                   /* bFunctionProtocol */
        0x01,                   /* iFunction */

        /*-------------------------- CDC ------------------------*/
        /* Interface Descriptor */
        0x09,                    /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE, /* bDescriptorType: Interface */
        USBD_CDC_INTERFACE_NUM,  /* bInterfaceNumber: Number of Interface */
        0x00,                    /* bAlternateSetting: Alternate setting */
        0x01,                    /* bNumEndpoints: One endpoints used */
        0x02,                    /* bInterfaceClass: Communication Interface Class */
        0x02,                    /* bInterfaceSubClass: Abstract Control Model */
        0x01,                    /* bInterfaceProtocol: Common AT commands */
        0x00,                    /* iInterface: */

        /* Header Functional Descriptor */
        0x05, /* bLength: Endpoint Descriptor size */
        0x24, /* bDescriptorType: CS_INTERFACE */
        0x00, /* bDescriptorSubtype: Header Func Desc */
        0x10, /* bcdCDC: spec release number */
        0x01,

        /* Call Management Functional Descriptor */
        0x05, /* bFunctionLength */
        0x24, /* bDescriptorType: CS_INTERFACE */
        0x01, /* bDescriptorSubtype: Call Management Func Desc */
        0x00, /* bmCapabilities: D0+D1 */
        0x01, /* bDataInterface: 1 */

        /* ACM Functional Descriptor */
        0x04, /* bFunctionLength */
        0x24, /* bDescriptorType: CS_INTERFACE */
        0x02, /* bDescriptorSubtype: Abstract Control Management desc */
        0x02, /* bmCapabilities */

        /* Union Functional Descriptor */
        0x05, /* bFunctionLength */
        0x24, /* bDescriptorType: CS_INTERFACE */
        0x06, /* bDescriptorSubtype: Union func desc */
        0x00, /* bMasterInterface: Communication class interface */
        0x01, /* bSlaveInterface0: Data Class Interface */

        /* Endpoint 2 Descriptor */
        0x07,                        /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
        CDC_CMD_EP,                  /* bEndpointAddress */
        0x03,                        /* bmAttributes: Interrupt */
        LOBYTE(CDC_CMD_PACKET_SIZE), /* wMaxPacketSize: */
        HIBYTE(CDC_CMD_PACKET_SIZE),
        CDC_HS_BINTERVAL, /* bInterval: */
        /*---------------------------------------------------------------------------*/

        /* Data class interface descriptor */
        0x09,                          /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_INTERFACE,       /* bDescriptorType: */
        USBD_CDC_INTERFACE_NUM + 0x01, /* bInterfaceNumber: Number of Interface */
        0x00,                          /* bAlternateSetting: Alternate setting */
        0x02,                          /* bNumEndpoints: Two endpoints used */
        0x0A,                          /* bInterfaceClass: CDC */
        0x00,                          /* bInterfaceSubClass: */
        0x00,                          /* bInterfaceProtocol: */
        0x00,                          /* iInterface: */

        /* Endpoint OUT Descriptor */
        0x07,                                /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,              /* bDescriptorType: Endpoint */
        CDC_OUT_EP,                          /* bEndpointAddress */
        0x02,                                /* bmAttributes: Bulk */
        LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), /* wMaxPacketSize: */
        HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
        0x00, /* bInterval: ignore for Bulk transfer */

        /* Endpoint IN Descriptor */
        0x07,                                /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,              /* bDescriptorType: Endpoint */
        CDC_IN_EP,                           /* bEndpointAddress */
        0x02,                                /* bmAttributes: Bulk */
        LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), /* wMaxPacketSize: */
        HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
        0x00, /* bInterval: ignore for Bulk transfer */
};
#ifdef __cplusplus
}
#endif
#endif //! USBD_CUD_CFGFSDESC_H
