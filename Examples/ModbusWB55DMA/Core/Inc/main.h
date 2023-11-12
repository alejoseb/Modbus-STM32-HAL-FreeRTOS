/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Modbus.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
extern modbusHandler_t ModbusH;
extern uint16_t ModbusDATA[128];

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DE_EN_Pin GPIO_PIN_3
#define DE_EN_GPIO_Port GPIOC
#define B1_Pin GPIO_PIN_4
#define B1_GPIO_Port GPIOC
#define LD2_Pin GPIO_PIN_0
#define LD2_GPIO_Port GPIOB
#define LD3_Pin GPIO_PIN_1
#define LD3_GPIO_Port GPIOB
#define JTMS_Pin GPIO_PIN_13
#define JTMS_GPIO_Port GPIOA
#define JTCK_Pin GPIO_PIN_14
#define JTCK_GPIO_Port GPIOA
#define B2_Pin GPIO_PIN_0
#define B2_GPIO_Port GPIOD
#define B3_Pin GPIO_PIN_1
#define B3_GPIO_Port GPIOD
#define JTDO_Pin GPIO_PIN_3
#define JTDO_GPIO_Port GPIOB
#define LD1_Pin GPIO_PIN_5
#define LD1_GPIO_Port GPIOB
#define STLINK_RX_Pin GPIO_PIN_6
#define STLINK_RX_GPIO_Port GPIOB
#define STLINK_TX_Pin GPIO_PIN_7
#define STLINK_TX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
