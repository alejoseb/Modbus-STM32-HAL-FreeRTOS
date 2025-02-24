/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
modbus_t telegram[2];
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

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
	uint32_t u32NotificationValue;
	telegram[0].u8id = 1; // slave address
	telegram[0].u8fct = MB_FC_WRITE_REGISTER; // function code (this one is registers read)
	telegram[0].u16RegAdd = 0x0; // start address in slave
	telegram[0].u16CoilsNo = 1; // number of elements (coils or registers) to read
	telegram[0].u16reg = ModbusDATA; // pointer to a memory array

    telegram[1].u8id = 1; // slave address
	telegram[1].u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
	telegram[1].u16RegAdd = 0x0; // start address in slave
	telegram[1].u16CoilsNo = 1; // number of elements (coils or registers) to read
	telegram[1].u16reg = ModbusDATA; // pointer to a memory array
	ModbusDATA[0] =1;




  uint16_t delayCount = 1000;
  /* Infinite loop */
  for(;;)
  {
		/*   this section is for the slave example

	    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

		if (ModbusDATA[0])
		{
			delayCount = ModbusDATA[0];
		}
		else
		{
			delayCount = 250;
		}

		osDelay(delayCount);

		*/

		/* this section is for the master section */


	 	//ModbusQuery(&ModbusH, telegram[1]); // make a query
	 	//u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY ); // block until query finishes

	  u32NotificationValue = ModbusQueryV2(&ModbusH, telegram[1]); // make a query with the new function ModbusQueryV2 prototype
	    if(u32NotificationValue != OP_OK_QUERY )
	 	{
	 	  		//handle error
	 	  		//  while(1);
	 		u32NotificationValue = 0;
	 	}



	 	osDelay(delayCount);

	 	ModbusDATA[0] = ModbusDATA[0] + 1;
	 	ModbusQuery(&ModbusH, telegram[0]); // make a query with old function
	 	u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY ); // block until query finishes this is embedded in V2
        if(u32NotificationValue != OP_OK_QUERY)
	 	{
	 	  		//handle error
	 	   	  	//  while(1);
        	u32NotificationValue = 0;
	 	}


  }


  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

