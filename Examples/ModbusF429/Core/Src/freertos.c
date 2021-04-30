/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include "semphr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for ModbusTestTask */
osThreadId_t ModbusTestTaskHandle;
const osThreadAttr_t ModbusTestTask_attributes = {
  .name = "ModbusTestTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartModbusTestTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of ModbusTestTask */
  ModbusTestTaskHandle = osThreadNew(StartModbusTestTask, NULL, &ModbusTestTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartModbusTestTask */
/**
  * @brief  Function implementing the ModbusTestTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartModbusTestTask */
void StartModbusTestTask(void *argument)
{
  /* USER CODE BEGIN StartModbusTestTask */
  /* Infinite loop */
/* master example */
	/*
	modbus_t telegram;
	uint32_t u32NotificationValue;

	telegram.u8id = 1; // slave address
	telegram.u8fct = MB_FC_READ_REGISTERS; // function code
	//telegram.u16RegAdd = 0x160; // start address in slave
	telegram.u16RegAdd = 0x0; // start address in slave
	telegram.u16CoilsNo = 10; // number of elements (coils or registers) to read
	telegram.u16reg = ModbusDATA; // pointer to a memory array in the Arduino

	for(;;)
	{

	    ModbusQuery(&ModbusH, telegram); // make a query
	    u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
	    if(u32NotificationValue)
	    {
	     //handle error
	       while(1);
	    }

	    osDelay(500);

	}

*/
	/*  slave example */

  for(;;)
  {
	  xSemaphoreTake(ModbusH.ModBusSphrHandle , portMAX_DELAY);
	  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, ModbusH.u16regs[0] & 0x1);
	  xSemaphoreGive(ModbusH.ModBusSphrHandle);
	  osDelay(200);
  }

  /* USER CODE END StartModbusTestTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
