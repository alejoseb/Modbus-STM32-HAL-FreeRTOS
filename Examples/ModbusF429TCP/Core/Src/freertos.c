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
#include "ip_addr.h"
#include "ethernetif.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#include "semphr.h"
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
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTaskModbusTes */
osThreadId_t myTaskModbusTesHandle;
const osThreadAttr_t myTaskModbusTes_attributes = {
  .name = "myTaskModbusTes",
  .stack_size = 256 * 8,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);

extern void MX_LWIP_Init(void);
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

  /* creation of myTaskModbusTes */
  myTaskModbusTesHandle = osThreadNew(StartTask02, NULL, &myTaskModbusTes_attributes);

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
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTaskModbusTes thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
	modbus_t telegram[2];
	uint32_t u32NotificationValue;


	telegram[0].u8id = 1; // slave address
	telegram[0].u8fct = MB_FC_WRITE_MULTIPLE_REGISTERS; // function code (this one is registers read)
	//telegram[0].u16RegAdd = 0x160; // start address in slave
	telegram[0].u16RegAdd = 0x0; // start address in slave
	telegram[0].u16CoilsNo = 10; // number of elements (coils or registers) to read
	telegram[0].u16reg = ModbusDATA; // pointer to a memory array in the microcontroller
	IP_ADDR4((ip4_addr_t *)&telegram[0].xIpAddress, 10, 75, 15, 61); //address of the slave
	telegram[0].u16Port = 5020;
	telegram[0].u8clientID = 0; // this identifies the TCP client session. The library supports up to "NUMBERTCPCONN"
							 //	simultaneous connections to different slaves. The value is defined in ModbusConfig.h file.
							 // The library uses the IP and port to open the TCP connection and keep the connection open regardless
							 // of later changes to those values. To change the IP address and Port, close the connection
							 // using the corresponding u8clientID, update the IP and Port and execute a new ModbusQuery.

	telegram[1].u8id = 1; // slave address
	telegram[1].u8fct = MB_FC_WRITE_MULTIPLE_REGISTERS; // function code (this one is registers read)
	//telegram[0].u16RegAdd = 0x160; // start address in slave
	telegram[1].u16RegAdd = 0x0; // start address in slave
	telegram[1].u16CoilsNo = 10; // number of elements (coils or registers) to read
	telegram[1].u16reg = ModbusDATA; // pointer to a memory array in the microcontroller
	IP_ADDR4((ip4_addr_t *)&telegram[1].xIpAddress, 10, 75, 15, 61); //address of the slave
	telegram[1].u16Port = 5024;
	telegram[1].u8clientID = 1; //this telegram will use the second connection slot

	for(;;)
	{


		/* Send query Modbus TCP */


		ModbusDATA[0]++;
		ModbusQuery(&ModbusH, telegram[0]); // make a query
	    u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes or timeouts
	    if(u32NotificationValue != ERR_OK_QUERY)
	    {
	     //handle error

	    	ModbusDATA[1]++;
	    }

	    ModbusDATA[0]++;
	    ModbusQuery(&ModbusH, telegram[1]); // make a query
	    u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes or timeouts
	    if(u32NotificationValue != ERR_OK_QUERY)
	    {
	    	ModbusDATA[2]++;
	    }



	    /* Update input from */
/*
	    xSemaphoreTake(ModbusH2.ModBusSphrHandle , portMAX_DELAY);
	    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, ModbusH2.u16regs[0] & 0x1);
	    xSemaphoreGive(ModbusH2.ModBusSphrHandle);
*/
	    osDelay(250);



	}
  /* USER CODE END StartTask02 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void ethernetif_notify_conn_changed(struct netif *netif)
{
  /* NOTE : This is function could be implemented in user file
            when the callback is needed,
  */

	ethernetif_set_link(&netif);

}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
