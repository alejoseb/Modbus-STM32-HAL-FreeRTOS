/*
 * UARTCallback.c
 *
 *  Created on: May 27, 2020
 *      Author: Alejandro Mera
 */

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "main.h"
#include "Modbus.h"


/**
 * @brief
 * This is the callback for HAL interrupts of UART TX used by Modbus library.
 * This callback is shared among all UARTS, if more interrupts are used
 * user should implement the correct control flow and verification to maintain
 * Modbus functionality.
 * @ingroup UartHandle UART HAL handler
 */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Modbus RTU TX callback BEGIN */
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	int i;
	for (i = 0; i < numberHandlers; i++ )
	{
	   	if (mHandlers[i]->port == huart  )
	   	{
	   		// notify the end of TX
	   		xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
	   		break;
	   	}

	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

	/* Modbus RTU TX callback END */

	/*
	 * Here you should implement the callback code for other UARTs not used by Modbus
	 *
	 * */

}



/**
 * @brief
 * This is the callback for HAL interrupt of UART RX
 * This callback is shared among all UARTS, if more interrupts are used
 * user should implement the correct control flow and verification to maintain
 * Modbus functionality.
 * @ingroup UartHandle UART HAL handler
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* Modbus RTU RX callback BEGIN */
    int i;
    for (i = 0; i < numberHandlers; i++ )
    {
    	if (mHandlers[i]->port == UartHandle  )
    	{

    		if(mHandlers[i]->xTypeHW == USART_HW)
    		{
    			RingAdd(&mHandlers[i]->xBufferRX, mHandlers[i]->dataRX);
    			HAL_UART_Receive_IT(mHandlers[i]->port, &mHandlers[i]->dataRX, 1);
    			xTimerResetFromISR(mHandlers[i]->xTimerT35, &xHigherPriorityTaskWoken);
    		}
    		break;
    	}
    }
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

	/* Modbus RTU RX callback END */

	/*
	 * Here you should implement the callback code for other UARTs not used by Modbus
	 *
	 *
	 * */

}


#if  ENABLE_USART_DMA ==  1
/*
 * DMA requires to handle callbacks for special communication modes of the HAL
 * It also has to handle eventual errors including extra steps that are not automatically
 * handled by the HAL
 * */


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

 int i;

 for (i = 0; i < numberHandlers; i++ )
 {
    	if (mHandlers[i]->port == huart  )
    	{

    		if(mHandlers[i]->xTypeHW == USART_HW_DMA)
    		{
    			while(HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
    		    {
    					HAL_UART_DMAStop(mHandlers[i]->port);
   				}
				__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

    		}

    		break;
    	}
   }
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		/* Modbus RTU RX callback BEGIN */
	    int i;
	    for (i = 0; i < numberHandlers; i++ )
	    {
	    	if (mHandlers[i]->port == huart  )
	    	{


	    		if(mHandlers[i]->xTypeHW == USART_HW_DMA)
	    		{
	    			if(Size) //check if we have received any byte
	    			{
		    				mHandlers[i]->xBufferRX.u8available = Size;
		    				mHandlers[i]->xBufferRX.overflow = false;

		    				while(HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
		    				{
		    					HAL_UART_DMAStop(mHandlers[i]->port);

		    				}
		    				__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

		    				xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0 , eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
	    			}
	    		}

	    		break;
	    	}
	    }
	    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#endif
