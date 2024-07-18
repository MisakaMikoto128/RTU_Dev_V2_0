/**
 ******************************************************************************
 * @file    usbd_cud.c
 * @author  MCD Application Team
 * @brief   This file provides all the CUD core functions.
 *
 * @verbatim
 *
 *          ===================================================================
 *                                CUD Class  Description
 *          ===================================================================
 *           This module manages the CUD class V1.0 following the "Universal
 *           Serial Bus Mass Storage Class (CUD) Bulk-Only Transport (BOT) Version 1.0
 *           Sep. 31, 1999".
 *           This driver implements the following aspects of the specification:
 *             - Bulk-Only Transport protocol
 *             - Subclass : SCSI transparent command set (ref. SCSI Primary Commands - 3 (SPC-3))
 *
 *  @endverbatim
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                      www.st.com/SLA0044
 *
 ******************************************************************************
 */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
- "stm32xxxxx_{eval}{discovery}{adafruit}_sd.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cud.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
 * @{
 */

/** @defgroup CUD_CORE
 * @brief Mass storage core module
 * @{
 */

/** @defgroup CUD_CORE_Private_TypesDefinitions
 * @{
 */
/**
 * @}
 */

/** @defgroup CUD_CORE_Private_Defines
 * @{
 */

/**
 * @}
 */

/** @defgroup CUD_CORE_Private_Macros
 * @{
 */
/**
 * @}
 */

/** @defgroup CUD_CORE_Private_FunctionPrototypes
 * @{
 */
uint8_t USBD_CUD_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
uint8_t USBD_CUD_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
uint8_t USBD_CUD_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_CUD_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
    return USBD_DFU_EP0_TxReady(pdev);
}
static uint8_t USBD_CUD_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    uint8_t res = 0;
    res         = USBD_CDC_EP0_RxReady(pdev);
    return res;
}
uint8_t USBD_CUD_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t USBD_CUD_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CUD_SOF(USBD_HandleTypeDef *pdev)
{
    return USBD_DFU_SOF(pdev);
}
#if (USBD_SUPPORT_USER_STRING_DESC == 1U)
static uint8_t *USBD_CUD_GetUsrStringDesc(USBD_HandleTypeDef *pdev,
                                          uint8_t index, uint16_t *length)
{
    return USBD_DFU_GetUsrStringDesc(pdev, index, length);
}
#endif
uint8_t *USBD_CUD_GetHSCfgDesc(uint16_t *length);
uint8_t *USBD_CUD_GetFSCfgDesc(uint16_t *length);
uint8_t *USBD_CUD_GetOtherSpeedCfgDesc(uint16_t *length);
uint8_t *USBD_CUD_GetDeviceQualifierDescriptor(uint16_t *length);

/**
 * @}
 */

/** @defgroup CUD_CORE_Private_Variables
 * @{
 */

USBD_ClassTypeDef USBD_CUD =
    {
        USBD_CUD_Init,
        USBD_CUD_DeInit,
        USBD_CUD_Setup,
        NULL,                 // USBD_CUD_EP0_TxReady, /*EP0_TxSent*/
        USBD_CUD_EP0_RxReady, /*EP0_RxReady*/
        USBD_CUD_DataIn,
        USBD_CUD_DataOut,
        NULL, // USBD_CUD_SOF, /*SOF */
        NULL,
        NULL,
        USBD_CUD_GetHSCfgDesc,
        USBD_CUD_GetFSCfgDesc,
        USBD_CUD_GetOtherSpeedCfgDesc,
        USBD_CUD_GetDeviceQualifierDescriptor,
#if (USBD_SUPPORT_USER_STRING_DESC == 1U)
        NULL, // USBD_CUD_GetUsrStringDesc
#endif
};

#include "USBD_CUD_CfgHSDesc.h"
#include "USBD_CUD_DFU_CfgFSDesc.h"
#include "USBD_CUD_CfgFSDesc.h"
#include "USBD_CUD_OtherSpeedCfgDesc.h"
#include "USBD_CUD_DeviceQualifierDesc.h"

/**
 * @}
 */

/** @defgroup CUD_CORE_Private_Functions
 * @{
 */

/**
 * @brief  USBD_CUD_Init
 *         Initialize  the mass storage configuration
 * @param  pdev: device instance
 * @param  cfgidx: configuration index
 * @retval status
 */
uint8_t USBD_CUD_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    uint8_t res = USBD_OK;
    res         = USBD_MSC_Init(pdev, cfgidx);
    res         = USBD_CDC_Init(pdev, cfgidx);
    return res;
}

/**
 * @brief  USBD_CUD_DeInit
 *         DeInitialize  the mass storage configuration
 * @param  pdev: device instance
 * @param  cfgidx: configuration index
 * @retval status
 */
uint8_t USBD_CUD_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    uint8_t res = USBD_OK;

    res = USBD_MSC_DeInit(pdev, cfgidx);
    res = USBD_CDC_DeInit(pdev, cfgidx);

    return res;
}

/**
 * @brief  USBD_CUD_Setup
 *         Handle the CUD specific requests
 * @param  pdev: device instance
 * @param  req: USB request
 * @retval status
 */
uint8_t USBD_CUD_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint8_t res = USBD_OK;
    switch (req->wIndex) {
        case USBD_MSC_INTERFACE_NUM:
            res = USBD_MSC_Setup(pdev, req);
            break;

        case USBD_CDC_INTERFACE_NUM:
            res = USBD_CDC_Setup(pdev, req);
            break;
        default:
            // 未识别的接口请求
            USBD_CtlError(pdev, req);
            break;
    }

    return res;
}

/**
 * @brief  USBD_CUD_DataIn
 *         handle data IN Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
uint8_t USBD_CUD_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    uint8_t ep_addr = epnum & 0x7F;
    if (ep_addr == MSC_EPOUT_ADDR) {
        return USBD_MSC_DataIn(pdev, epnum);
    } else if (ep_addr == CDC_OUT_EP || ep_addr == (CDC_CMD_EP & 0x7F)) {
        return USBD_CDC_DataIn(pdev, epnum);
    }

    return USBD_OK;
}

/**
 * @brief  USBD_CUD_DataOut
 *         handle data OUT Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
uint8_t USBD_CUD_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{

    uint8_t ep_addr = epnum & 0x7F;
    if (ep_addr == MSC_EPOUT_ADDR) {
        return USBD_MSC_DataOut(pdev, epnum);
    } else if (ep_addr == CDC_OUT_EP || ep_addr == (CDC_CMD_EP & 0x7F)) {
        return USBD_CDC_DataOut(pdev, epnum);
    }
    return USBD_OK;
}

/**
 * @brief  USBD_CUD_GetHSCfgDesc
 *         return configuration descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
uint8_t *USBD_CUD_GetHSCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CUD_CfgHSDesc);

    return USBD_CUD_CfgHSDesc;
}

/**
 * @brief  USBD_CUD_GetFSCfgDesc
 *         return configuration descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
uint8_t *USBD_CUD_GetFSCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CUD_CfgFSDesc);
    return USBD_CUD_CfgFSDesc;
}

/**
 * @brief  USBD_CUD_GetOtherSpeedCfgDesc
 *         return other speed configuration descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
uint8_t *USBD_CUD_GetOtherSpeedCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CUD_OtherSpeedCfgDesc);

    return USBD_CUD_OtherSpeedCfgDesc;
}
/**
 * @brief  DeviceQualifierDescriptor
 *         return Device Qualifier descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
uint8_t *USBD_CUD_GetDeviceQualifierDescriptor(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CUD_DeviceQualifierDesc);

    return USBD_CUD_DeviceQualifierDesc;
}

#include "usbd_storage_if.h"
#include "usbd_dfu_flash.h"
#include "usbd_cdc_if.h"
/**
 * @brief  USBD_CUD_Register
 * @param  fops: storage callback
 * @retval status
 */
uint8_t USBD_CUD_Register(USBD_HandleTypeDef *pdev)
{
    pdev->pUserDatas[USBD_MSC_USERDATA_ID] = &USBD_Storage_Interface_fops_FS;
    pdev->pUserDatas[USBD_CDC_USERDATA_ID] = &USBD_Interface_fops_FS;
#if ENBALE_DUF_CFGDESC
    pdev->pUserDatas[USBD_DFU_USERDATA_ID] = &USBD_DFU_Flash_fops;
#endif
    return USBD_OK;
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
