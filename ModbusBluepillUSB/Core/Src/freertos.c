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
#include "usb_device.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
modbus_t telegram[2];
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
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 128 * 4
};
/* Definitions for myTaskSlave */
osThreadId_t myTaskSlaveHandle;
const osThreadAttr_t myTaskSlave_attributes = {
  .name = "myTaskSlave",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTaskSlave(void *argument);

extern void MX_USB_DEVICE_Init(void);
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

  /* creation of myTaskSlave */
  myTaskSlaveHandle = osThreadNew(StartTaskSlave, NULL, &myTaskSlave_attributes);

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
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {

	  osDelay(10);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTaskSlave */
extern USBD_HandleTypeDef hUsbDeviceFS;
/**
* @brief Function implementing the myTaskSlave thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskSlave */
void StartTaskSlave(void *argument)
{
  /* USER CODE BEGIN StartTaskSlave */

#if SLAVE_EX == 0
	  uint32_t u32NotificationValue;
	  telegram[0].u8id = 17; // slave address
	  telegram[0].u8fct = MB_FC_WRITE_REGISTER; // function code (this one is registers read)
	  telegram[0].u16RegAdd = 0x0; // start address in slave
	  telegram[0].u16CoilsNo = 1; // number of elements (coils or registers) to read
	  telegram[0].au16reg = ModbusDATAusb; // pointer to a memory array


	  telegram[1].u8id = 17; // slave address
	  telegram[1].u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
	  telegram[1].u16RegAdd = 0x1; // start address in slave
	  telegram[1].u16CoilsNo = 1; // number of elements (coils or registers) to read
	  telegram[1].au16reg = ModbusDATAusb; // pointer to a memory array
	  ModbusDATAusb[0] =1;

#endif

  /* Infinite loop */
  for(;;)
  {

#if SLAVE_EX == 1
	  /**** Slave example CDC ***/
	  xSemaphoreTake(ModbusUSB.ModBusSphrHandle , 100);
	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, ModbusUSB.au16regs[0] & 0x1);
	  xSemaphoreGive(ModbusUSB.ModBusSphrHandle);

#else
	  /**** Master example CDC ***/
	  if( hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
	  {
	     ModbusQuery(&ModbusUSB, telegram[1]); // make a query
	     u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
	     if(u32NotificationValue)
	     {
	  		//handle error
	  		//  while(1);
	     }

	     osDelay(300);

	     ModbusDATAusb[0] = ModbusDATAusb[0] + 1;
	     ModbusQuery(&ModbusUSB, telegram[0]); // make a query
	     u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes

	     if(u32NotificationValue)
	     {
	    	  		//handle error
	    	  		//  while(1);
	     }






	  }
	  /**** End of master example***/
#endif


	  osDelay(300);
  }
  /* USER CODE END StartTaskSlave */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
