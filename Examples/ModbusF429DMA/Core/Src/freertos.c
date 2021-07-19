/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
modbus_t telegram[2];
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTaskMaster */
osThreadId_t myTaskMasterHandle;
const osThreadAttr_t myTaskMaster_attributes = {
  .name = "myTaskMaster",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TaskmyTaskSlave */
osThreadId_t TaskmyTaskSlaveHandle;
const osThreadAttr_t TaskmyTaskSlave_attributes = {
  .name = "TaskmyTaskSlave",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTaskMaster(void *argument);
void StartTasSlave(void *argument);

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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myTaskMaster */
  myTaskMasterHandle = osThreadNew(StartTaskMaster, NULL, &myTaskMaster_attributes);

  /* creation of TaskmyTaskSlave */
  TaskmyTaskSlaveHandle = osThreadNew(StartTasSlave, NULL, &TaskmyTaskSlave_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTaskMaster */
/**
* @brief Function implementing the myTaskMaster thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskMaster */
void StartTaskMaster(void *argument)
{
  /* USER CODE BEGIN StartTaskMaster */
	uint32_t u32NotificationValue;

	telegram[0].u8id = 1; // slave address
	telegram[0].u8fct = 3; // function code (this one is registers read)
	//telegram[0].u16RegAdd = 0x160; // start address in slave
	telegram[0].u16RegAdd = 0x0; // start address in slave
	telegram[0].u16CoilsNo = 1; // number of elements (coils or registers) to read
	telegram[0].u16reg = ModbusDATA2; // pointer to a memory array


	// telegram 0: read registers
	telegram[1].u8id = 1; // slave address
	telegram[1].u8fct = 6; // function code (this one is registers write)
	//telegram[1].u16RegAdd = 0x160; // start address in slave
	telegram[1].u16RegAdd = 0x0;
	telegram[1].u16CoilsNo = 1; // number of elements (coils or registers) to read
	telegram[1].u16reg = ModbusDATA2; // pointer to a memory array

    /* Infinite loop */
    for(;;)
    {

    	  ModbusQuery(&ModbusH2, telegram[0]); // make a query
    	  u32NotificationValue = ulTaskNotifyTake(pdTRUE, 500); // block until query finishes or timeout
    	  if(u32NotificationValue)
    	  {
    		//handle error
    		//  while(1);
    	  }
    	  osDelay(500);

    	  ModbusDATA2[0]++;
    	  ModbusQuery(&ModbusH2, telegram[1]); // make a query
    	  u32NotificationValue = ulTaskNotifyTake(pdTRUE, 500); // block until query finishes or timeout
    	  if(u32NotificationValue)
    	  {
    			//handle error
    			//  while(1);
    	  }

    	  osDelay(500);
    }
  /* USER CODE END StartTaskMaster */
}

/* USER CODE BEGIN Header_StartTasSlave */
/**
* @brief Function implementing the TaskmyTaskSlave thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTasSlave */
void StartTasSlave(void *argument)
{
  /* USER CODE BEGIN StartTasSlave */
  /* Infinite loop */
  for(;;)
  {

		xSemaphoreTake(ModbusH.ModBusSphrHandle , portMAX_DELAY);
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, ModbusH.u16regs[0] & 0x1);
		xSemaphoreGive(ModbusH.ModBusSphrHandle);

		osDelay(200);
  }
  /* USER CODE END StartTasSlave */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
