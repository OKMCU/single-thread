/*******************************************************************************
 * Copyright (c) 2021-2022, PEOS Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date         Author       Notes
 * 2021-10-28   Wentao SUN   first version
 * 
 ******************************************************************************/

#ifndef __HAL_PIN_H__
#define __HAL_PIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -------------------------------------------------------------------*/
#include "os.h"

/* Exported define ------------------------------------------------------------*/
#define HAL_PIN_MODE_OUTPUT         0x00
#define HAL_PIN_MODE_INPUT          0x01
#define HAL_PIN_MODE_INPUT_PU       0x02
#define HAL_PIN_MODE_INPUT_PD       0x03
#define HAL_PIN_MODE_OUTPUT_OD      0x04
#define HAL_PIN_MODE_QUASI_PU       0x05
#define HAL_PIN_MODE_ANALOG         0x06

#define HAL_PIN_LOW                 0
#define HAL_PIN_HIGH                1

#define HAL_GPIO_PORT_A             0
#define HAL_GPIO_PORT_B             1
#define HAL_GPIO_PORT_C             2
#define HAL_GPIO_PORT_H             3

/* Exported typedef -----------------------------------------------------------*/
/* Exported macro -------------------------------------------------------------*/
#define HAL_PIN_GET(port, pin)      (os_uint8_t)(port*16+pin)

/* Exported variables ---------------------------------------------------------*/
/* Exported function prototypes -----------------------------------------------*/
void hal_pin_mode( os_uint8_t pin, os_uint8_t mode );
void hal_pin_write( os_uint8_t pin, os_uint8_t value );
void hal_pin_toggle( os_uint8_t pin );
os_uint8_t hal_pin_read( os_uint8_t pin );

#ifdef __cplusplus
}
#endif

#endif //__HAL_PIN_H__
/****** (C) COPYRIGHT 2021 PEOS Development Team. *****END OF FILE****/
