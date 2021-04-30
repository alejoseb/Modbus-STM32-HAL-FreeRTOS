/*
 * Modbus.c
 *  Modbus RTU Master and Slave library for STM32 CUBE with FreeRTOS
 *  Created on: May 5, 2020
 *      Author: Alejandro Mera
 *      Adapted from https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino
 */

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "Modbus.h"
#include "timers.h"
#include "semphr.h"

#if ENABLE_TCP == 1
#include "api.h"
#include "ip4_addr.h"
#include "netif.h"
#endif


#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define lowByte(w) ((w) & 0xff)
#define highByte(w) ((w) >> 8)


///Queue Modbus telegrams for master
const osMessageQueueAttr_t QueueTelegram_attributes = {
       .name = "QueueModbusTelegram"
};


const osThreadAttr_t myTaskModbusA_attributes = {
    .name = "TaskModbusSlave",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 128 * 4
};

const osThreadAttr_t myTaskModbusA_attributesTCP = {
    .name = "TaskModbusSlave",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 256 * 4
};



//Task Modbus Master
//osThreadId_t myTaskModbusAHandle;
const osThreadAttr_t myTaskModbusB_attributes = {
    .name = "TaskModbusMaster",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 128 * 4
};


const osThreadAttr_t myTaskModbusB_attributesTCP = {
    .name = "TaskModbusMaster",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 256 * 4
};




//Semaphore to access the Modbus Data
const osSemaphoreAttr_t ModBusSphr_attributes = {
    .name = "ModBusSphr"
};


uint8_t numberHandlers = 0;


static void sendTxBuffer(modbusHandler_t *modH);
static int8_t getRxBuffer(modbusHandler_t *modH);
static uint8_t validateAnswer(modbusHandler_t *modH);
static void buildException( uint8_t u8exception, modbusHandler_t *modH );
static uint8_t validateRequest(modbusHandler_t * modH);
static uint16_t word(uint8_t H, uint8_t l);
static void get_FC1(modbusHandler_t *modH);
static void get_FC3(modbusHandler_t *modH);
static int8_t process_FC1(modbusHandler_t *modH );
static int8_t process_FC3(modbusHandler_t *modH );
static int8_t process_FC5( modbusHandler_t *modH);
static int8_t process_FC6(modbusHandler_t *modH );
static int8_t process_FC15(modbusHandler_t *modH );
static int8_t process_FC16(modbusHandler_t *modH);
static void vTimerCallbackT35(TimerHandle_t *pxTimer);
static void vTimerCallbackTimeout(TimerHandle_t *pxTimer);
static int8_t getRxBuffer(modbusHandler_t *modH);
static int8_t SendQuery(modbusHandler_t *modH ,  modbus_t telegram);

#if ENABLE_TCP ==1

static bool TCPwaitConnData(modbusHandler_t *modH);
static void  TCPinitserver(modbusHandler_t *modH);
static mb_errot_t TCPconnectserver(modbusHandler_t * modH, uint32_t address, uint16_t port);
static mb_errot_t TCPgetRxBuffer(modbusHandler_t * modH);

#endif




/* Ring Buffer functions */
void RingAdd(modbusRingBuffer_t *xRingBuffer, uint8_t u8Val)
{

	xRingBuffer->uxBuffer[xRingBuffer->u8end] = u8Val;
	xRingBuffer->u8end = (xRingBuffer->u8end + 1) % MAX_BUFFER;
	if (xRingBuffer->u8available == MAX_BUFFER)
	{
		xRingBuffer->u8start = (xRingBuffer->u8start + 1) % MAX_BUFFER;
	}
	else
	{
		xRingBuffer->u8available++;
	}

}

uint8_t RingGetAllBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer)
{
	return RingGetNBytes(xRingBuffer, buffer, xRingBuffer->u8available);
}

uint8_t RingGetNBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer, uint8_t uNumber)
{
	uint8_t uCounter;
	if(xRingBuffer->u8available == 0  || uNumber == 0 ) return 0;
	if(uNumber > MAX_BUFFER) return 0;

	for(uCounter = 0; uCounter < uNumber && uCounter< xRingBuffer->u8available ; uCounter++)
	{
		buffer[uCounter] = xRingBuffer->uxBuffer[xRingBuffer->u8start];
		xRingBuffer->u8start = (xRingBuffer->u8start + 1) % MAX_BUFFER;
	}
	xRingBuffer->u8available = xRingBuffer->u8available - uCounter;

	return uCounter;
}

uint8_t RingCountBytes(modbusRingBuffer_t *xRingBuffer)
{
return xRingBuffer->u8available;
}

void RingClear(modbusRingBuffer_t *xRingBuffer)
{
xRingBuffer->u8start = 0;
xRingBuffer->u8end = 0;
xRingBuffer->u8available = 0;
}

/* End of Ring Buffer functions */



const unsigned char fctsupported[] =
{
    MB_FC_READ_COILS,
    MB_FC_READ_DISCRETE_INPUT,
    MB_FC_READ_REGISTERS,
    MB_FC_READ_INPUT_REGISTER,
    MB_FC_WRITE_COIL,
    MB_FC_WRITE_REGISTER,
    MB_FC_WRITE_MULTIPLE_COILS,
    MB_FC_WRITE_MULTIPLE_REGISTERS
};


/**
 * @brief
 * Initialization for a Master/Slave.
 *
 * For hardware serial through USB/RS232C/RS485 set port to Serial, Serial1,
 * Serial2, or Serial3. (Numbered hardware serial ports are only available on
 * some boards.)
 *
 * For software serial through RS232C/RS485 set port to a SoftwareSerial object
 * that you have already constructed.
 *
 * ModbusRtu needs a pin for flow control only for RS485 mode. Pins 0 and 1
 * cannot be used.
 *
 * First call begin() on your serial port, and then start up ModbusRtu by
 * calling start(). You can choose the line speed and other port parameters
 * by passing the appropriate values to the port's begin() function.
 *
 * @param u8id   node address 0=master, 1..247=slave
 * @param port   serial port used
 * @param EN_Port_v port for txen RS-485
 * @param EN_Pin_v pin for txen RS-485 (NULL means RS232C mode)
 * @ingroup setup
 */
void ModbusInit(modbusHandler_t * modH)
{

  if (numberHandlers < MAX_M_HANDLERS)
  {

	  //Initialize the ring buffer

	  RingClear(&modH->xBufferRX);

	  if(modH->uModbusType == MB_SLAVE)
	  {
		  //Create Modbus task slave
#if ENABLE_TCP == 1
		  if( modH->xTypeHW == TCP_HW)
		  {
			  modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusSlave, modH, &myTaskModbusA_attributesTCP);
		  }
		  else{
			  modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusSlave, modH, &myTaskModbusA_attributes);
		  }
#else
		  modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusSlave, modH, &myTaskModbusA_attributes);
#endif


	  }
	  else if (modH->uModbusType == MB_MASTER)
	  {
		  //Create Modbus task Master  and Queue for telegrams

#if ENABLE_TCP == 1
		  if( modH->xTypeHW == TCP_HW)
		  {
		     modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusMaster, modH, &myTaskModbusB_attributesTCP);
		  }
		  else
		  {
		     modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusMaster, modH, &myTaskModbusB_attributes);
		  }
#else
		  modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusMaster, modH, &myTaskModbusB_attributes);
#endif



		  modH->xTimerTimeout=xTimerCreate("xTimerTimeout",  // Just a text name, not used by the kernel.
				  	  	modH->u16timeOut ,     		// The timer period in ticks.
						pdFALSE,         // The timers will auto-reload themselves when they expire.
						( void * )modH->xTimerTimeout,     // Assign each timer a unique id equal to its array index.
						(TimerCallbackFunction_t) vTimerCallbackTimeout  // Each timer calls the same callback when it expires.
                  	  	);

		  if(modH->xTimerTimeout == NULL)
		  {
			  while(1); //error creating timer, check heap and stack size
		  }


		  modH->QueueTelegramHandle = osMessageQueueNew (MAX_TELEGRAMS, sizeof(modbus_t), &QueueTelegram_attributes);

		  if(modH->QueueTelegramHandle == NULL)
		  {
			  while(1); //error creating queue for telegrams, check heap and stack size
		  }

	  }
	  else
	  {
		  while(1); //Error Modbus type not supported choose a valid Type
	  }

	  if  (modH->myTaskModbusAHandle == NULL)
	  {
		  while(1); //Error creating Modbus task, check heap and stack size
	  }


	  modH->xTimerT35 = xTimerCreate("TimerT35",         // Just a text name, not used by the kernel.
		  	  	  	  	  	  	  	T35 ,     // The timer period in ticks.
                                    pdFALSE,         // The timers will auto-reload themselves when they expire.
									( void * )modH->xTimerT35,     // Assign each timer a unique id equal to its array index.
                                    (TimerCallbackFunction_t) vTimerCallbackT35     // Each timer calls the same callback when it expires.
                                    );
	  if (modH->xTimerT35 == NULL)
	  {
		  while(1); //Error creating the timer, check heap and stack size
	  }


	  modH->ModBusSphrHandle = osSemaphoreNew(1, 1, &ModBusSphr_attributes);

	  if(modH->ModBusSphrHandle == NULL)
	  {
		  while(1); //Error creating the semaphore, check heap and stack size
	  }

	  mHandlers[numberHandlers] = modH;
	  numberHandlers++;
  }
  else
  {
	  while(1); //error no more Modbus handlers supported
  }

}

/**
 * @brief
 * Start object.
 *
 * Call this AFTER calling begin() on the serial port, typically within setup().
 *
 * (If you call this function, then you should NOT call any of
 * ModbusRtu's own begin() functions.)
 *
 * @ingroup setup
 */
void ModbusStart(modbusHandler_t * modH)
{

	if(modH->xTypeHW != USART_HW && modH->xTypeHW != TCP_HW && modH->xTypeHW != USB_CDC_HW)
	{

		while(1); //ERROR select the type of hardware
	}



	if (modH->xTypeHW == USART_HW )
	{

	      if (modH->EN_Port != NULL )
          {
              // return RS485 transceiver to transmit mode
          	HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
          }

          if (modH->uModbusType == MB_SLAVE &&  modH->u16regs == NULL )
          {
          	while(1); //ERROR define the DATA pointer shared through Modbus
          }

          //check that port is initialized
          while (HAL_UART_GetState(modH->port) != HAL_UART_STATE_READY)
          {
          }
          // Receive data from serial port for Modbus using interrupt
          if(HAL_UART_Receive_IT(modH->port, &modH->dataRX, 1) != HAL_OK)
          {
              while(1)
              {
            	  //error in your initialization code
              }
          }

          if(modH->u8id !=0 && modH->uModbusType == MB_MASTER )
          {
        	  while(1)
        	  {
        	     	  //error Master ID must be zero
        	  }

          }

          if(modH->u8id ==0 && modH->uModbusType == MB_SLAVE )
          {
             	  while(1)
               	  {
                  	     	  //error Master ID must be zero
               	  }

           }


	}

#if ENABLE_TCP == 1



#endif


    modH->u8lastRec = modH->u8BufferSize = 0;
    modH->u16InCnt = modH->u16OutCnt = modH->u16errCnt = 0;

}

#if ENABLE_USB_CDC == 1
extern void MX_USB_DEVICE_Init(void);
void ModbusStartCDC(modbusHandler_t * modH)
{


    if (modH->uModbusType == MB_SLAVE &&  modH->u16regs == NULL )
    {
    	while(1); //ERROR define the DATA pointer shared through Modbus
    }

    modH->u8lastRec = modH->u8BufferSize = 0;
    modH->u16InCnt = modH->u16OutCnt = modH->u16errCnt = 0;
}
#endif


void vTimerCallbackT35(TimerHandle_t *pxTimer)
{
	//Notify that a stream has just arrived
	int i;
	//TimerHandle_t aux;
	for(i = 0; i < numberHandlers; i++)
	{

		if( (TimerHandle_t *)mHandlers[i]->xTimerT35 ==  pxTimer ){
			if(mHandlers[i]->uModbusType == MB_MASTER)
			{
				xTimerStop(mHandlers[i]->xTimerTimeout,0);
			}
			xTaskNotify(mHandlers[i]->myTaskModbusAHandle, 0, eSetValueWithOverwrite);
		}

	}
}

void vTimerCallbackTimeout(TimerHandle_t *pxTimer)
{
	//Notify that a stream has just arrived
	int i;
	//TimerHandle_t aux;
	for(i = 0; i < numberHandlers; i++)
	{

		if( (TimerHandle_t *)mHandlers[i]->xTimerTimeout ==  pxTimer ){
				xTaskNotify(mHandlers[i]->myTaskModbusAHandle, ERR_TIME_OUT, eSetValueWithOverwrite);
		}

	}

}


#if ENABLE_TCP ==1

bool TCPwaitConnData(modbusHandler_t *modH)
{
  struct netbuf *inbuf;
  err_t recv_err, accept_err;
  char* buf;
  uint16_t buflen;
  uint16_t uLength;
  bool xTCPvalid;
  xTCPvalid = false;


  while(1) //wait for incoming connection
  {
       /* accept any incoming connection */
	   accept_err = netconn_accept(modH->conn, &modH->newconn);
	   if(accept_err == ERR_OK)
	   {
           break; //break the loop and continue with validations of TCP frame
       }

  }

  netconn_set_recvtimeout(modH->newconn, modH->u16timeOut );
  recv_err = netconn_recv(modH->newconn, &inbuf);

  if (recv_err == ERR_OK)
  {
      if (netconn_err(modH->newconn) == ERR_OK)
      {
    	  /* Read the data from the port, blocking if nothing yet there.
    	  We assume the request (the part we care about) is in one netbuf */
   	      netbuf_data(inbuf, (void**)&buf, &buflen);
		  if (buflen>11) // minimum frame size for modbus TCP
		  {
			  if(buf[2] == 0 || buf[3] == 0 ) //validate protocol ID
			  {
			  	  uLength = (buf[4]<<8 & 0xff00) | buf[5];
			  	  if(uLength< (MAX_BUFFER-2)  && (uLength + 6) <= buflen)
			   	  {
			          for(int i = 0; i < uLength; i++)
			          {
			        	  modH->u8Buffer[i] = buf[i+6];
			          }
			          modH->u16TransactionID = (buf[0]<<8 & 0xff00) | buf[1];
			          modH->u8BufferSize = uLength + 2; //add 2 dummy bytes for CRC
			          xTCPvalid = true;

			      }
			  }

		  }
		  netbuf_delete(inbuf); // delete the buffer always
	   }
   }

  return xTCPvalid;

}


void  TCPinitserver(modbusHandler_t *modH)
{
      err_t err;

	  /* Create a new TCP connection handle */
	  if(modH-> xTypeHW == TCP_HW)
	  {
		  modH->conn = netconn_new(NETCONN_TCP);
		  if (modH->conn!= NULL)
		  {
		     /* Bind to port (502) Modbus with default IP address */
			 if(modH->uTcpPort == 0) modH->uTcpPort = 502; //if port not defined
		     err = netconn_bind(modH->conn, NULL, modH->uTcpPort);
		     if (err == ERR_OK)
		     {
		    	 /* Put the connection into LISTEN state */
		    	 netconn_listen(modH->conn);
		     }
		     else{
		    		  while(1)
		    		  {
		    			  // error binding the TCP Modbus port check your configuration
		    		  }
		    	  }
		  }
		  else{
			  while(1)
			  {
				  // error creating new connection check your configuration,
				  // this function must be called after the scheduler is started
			  }
		  }
	  }
}

#endif



void StartTaskModbusSlave(void *argument)
{

  modbusHandler_t *modH =  (modbusHandler_t *)argument;

#if ENABLE_TCP ==1
  if( modH->xTypeHW == TCP_HW )
  {
	  TCPinitserver(modH); // start the Modbus server slave
  }
#endif

  for(;;)
  {

	modH->i8lastError = 0;


#if ENABLE_USB_CDC ==1

	  if(modH-> xTypeHW == USB_CDC_HW)
	  {
		      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Block indefinitely until a Modbus Frame arrives */
			  if (modH->u8BufferSize == ERR_BUFF_OVERFLOW) // is this necessary?
			  {
			     modH->i8lastError = ERR_BUFF_OVERFLOW;
			  	 modH->u16errCnt++;
			  	 continue;
			  }
	  }
#endif

#if ENABLE_TCP ==1
	  if(modH-> xTypeHW == TCP_HW)
	  {

		  if(TCPwaitConnData(modH) == false) // wait for connection and receive data
		  {
			  netconn_close(modH->newconn);
			  netconn_delete(modH->newconn);
			  continue; // TCP package was not validated
		  }

	  }
#endif


   if(modH-> xTypeHW == USART_HW)
   {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Block indefinitely until a Modbus Frame arrives */
	  modH->u8BufferSize = RingCountBytes(&modH->xBufferRX);
	  if (modH->EN_Port != NULL )
	  {
	     HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET); // is this required?
	   }
	   getRxBuffer(modH);
   }

   if (modH->u8BufferSize < 7)
   {
      //The size of the frame is invalid
      modH->i8lastError = ERR_BAD_SIZE;
      modH->u16errCnt++;

  #if ENABLE_TCP ==1
	  if(modH-> xTypeHW == TCP_HW)
	  {
		  netconn_close(modH->newconn);
		  netconn_delete(modH->newconn);
	  }
  #endif
	  continue;
    }

		// check slave id
    if ( modH->u8Buffer[ID] !=  modH->u8id)   //for Modbus TCP this is not validated, user should modify accordingly if needed
	{
        #if ENABLE_TCP ==1
		if(modH-> xTypeHW == TCP_HW)
		{
		    netconn_close(modH->newconn);
			netconn_delete(modH->newconn);
		}
        #endif
		continue;
	 }

	  // validate message: CRC, FCT, address and size
    uint8_t u8exception = validateRequest(modH);
	if (u8exception > 0)
	{
	    if (u8exception != ERR_TIME_OUT)
		{
		    buildException( u8exception, modH);
			sendTxBuffer(modH);
		}
		modH->i8lastError = u8exception;
		//return u8exception

        #if ENABLE_TCP ==1
		if(modH-> xTypeHW == TCP_HW)
		{
		    netconn_close(modH->newconn);
		  	netconn_delete(modH->newconn);
		}
        #endif
		continue;
	 }

	 modH->i8lastError = 0;
	 xSemaphoreTake(modH->ModBusSphrHandle , portMAX_DELAY); //before processing the message get the semaphore

	 // process message
	 switch(modH->u8Buffer[ FUNC ] )
	 {
			case MB_FC_READ_COILS:
			case MB_FC_READ_DISCRETE_INPUT:
				modH->i8state = process_FC1(modH);
				break;
			case MB_FC_READ_INPUT_REGISTER:
			case MB_FC_READ_REGISTERS :
				modH->i8state = process_FC3(modH);
				break;
			case MB_FC_WRITE_COIL:
				modH->i8state = process_FC5(modH);
				break;
			case MB_FC_WRITE_REGISTER :
				modH->i8state = process_FC6(modH);
				break;
			case MB_FC_WRITE_MULTIPLE_COILS:
				modH->i8state = process_FC15(modH);
				break;
			case MB_FC_WRITE_MULTIPLE_REGISTERS :
				modH->i8state = process_FC16(modH);
				break;
			default:
				break;
	 }

   #if ENABLE_TCP ==1
	 if(modH-> xTypeHW == TCP_HW)
	 {
	    netconn_close(modH->newconn);
	  	netconn_delete(modH->newconn);
	 }
   #endif

	 xSemaphoreGive(modH->ModBusSphrHandle); //Release the semaphore
	 continue;

   }

}



void ModbusQuery(modbusHandler_t * modH, modbus_t telegram )
{
	//Add the telegram to the TX tail Queue of Modbus
	if (modH->uModbusType == MB_MASTER)
	{
	telegram.u32CurrentTask = (uint32_t *) osThreadGetId();
	xQueueSendToBack(modH->QueueTelegramHandle, &telegram, 0);
	}
	else{
		while(1);// error a slave cannot send queries as a master
	}
}



void ModbusQueryInject(modbusHandler_t * modH, modbus_t telegram )
{
	//Add the telegram to the TX head Queue of Modbus
	xQueueReset(modH->QueueTelegramHandle);
	telegram.u32CurrentTask = (uint32_t *) osThreadGetId();
	xQueueSendToFront(modH->QueueTelegramHandle, &telegram, 0);
}

/**
 * @brief
 * *** Only Modbus Master ***
 * Generate a query to an slave with a modbus_t telegram structure
 * The Master must be in COM_IDLE mode. After it, its state would be COM_WAITING.
 * This method has to be called only in loop() section.
 *
 * @see modbus_t
 * @param modH  modbus handler
 * @param modbus_t  modbus telegram structure (id, fct, ...)
 * @ingroup loop
 */
int8_t SendQuery(modbusHandler_t *modH ,  modbus_t telegram )
{


	uint8_t u8regsno, u8bytesno;
	uint8_t  error = 0;
	xSemaphoreTake(modH->ModBusSphrHandle , portMAX_DELAY); //before processing the message get the semaphore

	if (modH->u8id!=0) error = ERR_NOT_MASTER;
	if (modH->i8state != COM_IDLE) error = ERR_POLLING ;
	if ((telegram.u8id==0) || (telegram.u8id>247)) error = ERR_BAD_SLAVE_ID;

	if(error)
	{
		 modH->i8lastError = error;
		 xSemaphoreGive(modH->ModBusSphrHandle);
		 return error;
	}


	modH->u16regs = telegram.u16reg;

	// telegram header
	modH->u8Buffer[ ID ]         = telegram.u8id;
	modH->u8Buffer[ FUNC ]       = telegram.u8fct;
	modH->u8Buffer[ ADD_HI ]     = highByte(telegram.u16RegAdd );
	modH->u8Buffer[ ADD_LO ]     = lowByte( telegram.u16RegAdd );

	switch( telegram.u8fct )
	{
	case MB_FC_READ_COILS:
	case MB_FC_READ_DISCRETE_INPUT:
	case MB_FC_READ_REGISTERS:
	case MB_FC_READ_INPUT_REGISTER:
	    modH->u8Buffer[ NB_HI ]      = highByte(telegram.u16CoilsNo );
	    modH->u8Buffer[ NB_LO ]      = lowByte( telegram.u16CoilsNo );
	    modH->u8BufferSize = 6;
	    break;
	case MB_FC_WRITE_COIL:
	    modH->u8Buffer[ NB_HI ]      = (( telegram.u16reg[0]> 0) ? 0xff : 0);
	    modH->u8Buffer[ NB_LO ]      = 0;
	    modH->u8BufferSize = 6;
	    break;
	case MB_FC_WRITE_REGISTER:
	    modH->u8Buffer[ NB_HI ]      = highByte( telegram.u16reg[0]);
	    modH->u8Buffer[ NB_LO ]      = lowByte( telegram.u16reg[0]);
	    modH->u8BufferSize = 6;
	    break;
	case MB_FC_WRITE_MULTIPLE_COILS: // TODO: implement "sending coils"
	    u8regsno = telegram.u16CoilsNo / 16;
	    u8bytesno = u8regsno * 2;
	    if ((telegram.u16CoilsNo % 16) != 0)
	    {
	        u8bytesno++;
	        u8regsno++;
	    }

	    modH->u8Buffer[ NB_HI ]      = highByte(telegram.u16CoilsNo );
	    modH->u8Buffer[ NB_LO ]      = lowByte( telegram.u16CoilsNo );
	    modH->u8Buffer[ BYTE_CNT ]    = u8bytesno;
	    modH->u8BufferSize = 7;

	    for (uint16_t i = 0; i < u8bytesno; i++)
	    {
	        if(i%2)
	        {
	        	modH->u8Buffer[ modH->u8BufferSize ] = lowByte( telegram.u16reg[ i/2 ] );
	        }
	        else
	        {
	        	modH->u8Buffer[  modH->u8BufferSize ] = highByte( telegram.u16reg[ i/2 ] );

	        }
	        modH->u8BufferSize++;
	    }
	    break;

	case MB_FC_WRITE_MULTIPLE_REGISTERS:
	    modH->u8Buffer[ NB_HI ]      = highByte(telegram.u16CoilsNo );
	    modH->u8Buffer[ NB_LO ]      = lowByte( telegram.u16CoilsNo );
	    modH->u8Buffer[ BYTE_CNT ]    = (uint8_t) ( telegram.u16CoilsNo * 2 );
	    modH->u8BufferSize = 7;

	    for (uint16_t i=0; i< telegram.u16CoilsNo; i++)
	    {

	        modH->u8Buffer[  modH->u8BufferSize ] = highByte(  telegram.u16reg[ i ] );
	        modH->u8BufferSize++;
	        modH->u8Buffer[  modH->u8BufferSize ] = lowByte( telegram.u16reg[ i ] );
	        modH->u8BufferSize++;
	    }
	    break;
	}

	xSemaphoreGive(modH->ModBusSphrHandle);

	sendTxBuffer(modH);
	modH->i8state = COM_WAITING;
	modH->i8lastError = 0;
	return 0;


}

#if ENABLE_TCP == 1

static mb_errot_t TCPconnectserver(modbusHandler_t * modH, uint32_t address, uint16_t port)
{
	 err_t err;

	 modH->newconn = netconn_new(NETCONN_TCP);
	 if (modH->newconn == NULL)
	 {
		 while(1)
		 {
	 	  // error creating new connection check your configuration and heap size
	 	 }
	 }

	 err = netconn_connect(modH->newconn, (ip_addr_t *) &address, port);

	 if (err  != ERR_OK )
	 {
	 	   netconn_close(modH->newconn);
	       netconn_delete(modH->newconn);
	       return ERR_TIME_OUT;
	  }

	  return ERR_OK;
}


static mb_errot_t TCPgetRxBuffer(modbusHandler_t * modH)
{

	struct netbuf *inbuf;
	err_t err = ERR_TIME_OUT;
	char* buf;
	uint16_t buflen;
	uint16_t uLength;

	netconn_set_recvtimeout(modH->newconn, modH->u16timeOut );
	err = netconn_recv(modH->newconn, &inbuf);

	uLength = 0;

    if (err == ERR_OK)
    {
    	err = netconn_err(modH->newconn) ;
    	if (err == ERR_OK)
    	{
    		/* Read the data from the port, blocking if nothing yet there.
   	  	  	We assume the request (the part we care about) is in one netbuf */
  	        err = netbuf_data(inbuf, (void**)&buf, &buflen);
  	        if(err == ERR_OK )
  	        {
                 if (buflen>11) // minimum frame size for modbus TCP
                 {
        	         if(buf[2] == 0 || buf[3] == 0 ) //validate protocol ID
        	         {
        	  	          uLength = (buf[4]<<8 & 0xff00) | buf[5];
        	  	          if(uLength< (MAX_BUFFER-2)  && (uLength + 6) <= buflen)
        	  	          {
        	  	  	          for(int i = 0; i < uLength; i++)
        	  	  	          {
        	  	  	  	         modH->u8Buffer[i] = buf[i+6];
        	  	  	          }
        	  	  	          modH->u16TransactionID = (buf[0]<<8 & 0xff00) | buf[1];
        	  	  	          modH->u8BufferSize = uLength + 2; //include 2 dummy bytes for CRC
        	  	           }
        	          }
                  }
  	        } // netbuf_data
  	        netbuf_delete(inbuf); //delete the buffer always
    	}
    }

    netconn_close(modH->newconn);
	netconn_delete(modH->newconn);
	return err;
}

#endif

void StartTaskModbusMaster(void *argument)
{

  modbusHandler_t *modH =  (modbusHandler_t *)argument;
  uint32_t ulNotificationValue;
  modbus_t telegram;



  for(;;)
  {
	  /*Wait indefinitely for a telegram to send */
	  xQueueReceive(modH->QueueTelegramHandle, &telegram, portMAX_DELAY);


#if ENABLE_TCP ==1
     if(modH->xTypeHW == TCP_HW)
     {

    	  ulNotificationValue = TCPconnectserver( modH, telegram.xIpAddress, telegram.u16Port );
 	      if(ulNotificationValue == ERR_OK)
	      {
 		      SendQuery(modH, telegram);
 		     /* Block until a Modbus Frame arrives or query timeouts*/
 		      ulNotificationValue = TCPgetRxBuffer(modH); // TCP receives the data and the notification simultaneously since it is synchronous
	      }
     }
     else // send a query for USART and USB_CDC
     {
   	   SendQuery(modH, telegram);
       /* Block until a Modbus Frame arrives or query timeouts*/
   	   ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
     }
#else
     // This is the case for implementations with only USART support
     SendQuery(modH, telegram);
     /* Block indefinitely until a Modbus Frame arrives or query timeouts*/
     ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

#endif

	  // notify the task the request timeout
      modH->i8lastError = 0;
      if(ulNotificationValue)
      {
    	  modH->i8state = COM_IDLE;
    	  modH->i8lastError = ERR_TIME_OUT;
    	  modH->u16errCnt++;
    	  xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
    	  continue;
      }


#if ENABLE_USB_CDC ==1 || ENABLE_TCP ==1

      if(modH->xTypeHW == USART_HW) //TCP and USB_CDC use different methods to get the buffer
      {
    	  getRxBuffer(modH);
      }

#else
      getRxBuffer(modH);
#endif



	  if ( modH->u8BufferSize < 6){

		  modH->i8state = COM_IDLE;
		  modH->i8lastError = ERR_BAD_SIZE;
		  modH->u16errCnt++;
		  xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
		  continue;
	  }

	  xTimerStop(modH->xTimerTimeout,0); // cancel timeout timer


	  // validate message: id, CRC, FCT, exception
	  int8_t u8exception = validateAnswer(modH);
	  if (u8exception != 0)
	  {
		 modH->i8state = COM_IDLE;
         modH->i8lastError = u8exception;
		 xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
	     continue;
	  }

	  modH->i8lastError = u8exception;

	  xSemaphoreTake(modH->ModBusSphrHandle , portMAX_DELAY); //before processing the message get the semaphore
	  // process answer
	  switch( modH->u8Buffer[ FUNC ] )
	  {
	  case MB_FC_READ_COILS:
	  case MB_FC_READ_DISCRETE_INPUT:
	      //call get_FC1 to transfer the incoming message to u16regs buffer
	      get_FC1(modH);
	      break;
	  case MB_FC_READ_INPUT_REGISTER:
	  case MB_FC_READ_REGISTERS :
	      // call get_FC3 to transfer the incoming message to u16regs buffer
	      get_FC3(modH);
	      break;
	  case MB_FC_WRITE_COIL:
	  case MB_FC_WRITE_REGISTER :
	  case MB_FC_WRITE_MULTIPLE_COILS:
	  case MB_FC_WRITE_MULTIPLE_REGISTERS :
	      // nothing to do
	      break;
	  default:
	      break;
	  }
	  modH->i8state = COM_IDLE;

	  xSemaphoreGive(modH->ModBusSphrHandle); //Release the semaphore
	  xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
	  continue;
	 }

}

/**
 * This method processes functions 1 & 2 (for master)
 * This method puts the slave answer into master data buffer
 *
 * @ingroup register
 */
void get_FC1(modbusHandler_t *modH)
{
    uint8_t u8byte, i;
    u8byte = 3;
     for (i=0; i< modH->u8Buffer[2]; i++) {

        if(i%2)
        {
        	modH->u16regs[i/2]= word(modH->u8Buffer[i+u8byte], lowByte(modH->u16regs[i/2]));
        }
        else
        {

        	modH->u16regs[i/2]= word(highByte(modH->u16regs[i/2]), modH->u8Buffer[i+u8byte]);
        }

     }
}

/**
 * This method processes functions 3 & 4 (for master)
 * This method puts the slave answer into master data buffer
 *
 * @ingroup register
 */
void get_FC3(modbusHandler_t *modH)
{
    uint8_t u8byte, i;
    u8byte = 3;

    for (i=0; i< modH->u8Buffer[ 2 ] /2; i++)
    {
    	modH->u16regs[ i ] = word(modH->u8Buffer[ u8byte ], modH->u8Buffer[ u8byte +1 ]);
        u8byte += 2;
    }
}



/**
 * @brief
 * This method validates master incoming messages
 *
 * @return 0 if OK, EXCEPTION if anything fails
 * @ingroup buffer
 */
uint8_t validateAnswer(modbusHandler_t *modH)
{
    // check message crc vs calculated crc

#if ENABLE_TCP ==1
	if(modH->xTypeHW != TCP_HW)
	{
#endif
	uint16_t u16MsgCRC =
        ((modH->u8Buffer[modH->u8BufferSize - 2] << 8)
         | modH->u8Buffer[modH->u8BufferSize - 1]); // combine the crc Low & High bytes
    if ( calcCRC(modH->u8Buffer,  modH->u8BufferSize-2) != u16MsgCRC )
    {
    	modH->u16errCnt ++;
        return ERR_BAD_CRC;
    }
#if ENABLE_TCP ==1
	}
#endif


    // check exception
    if ((modH->u8Buffer[ FUNC ] & 0x80) != 0)
    {
    	modH->u16errCnt ++;
        return ERR_EXCEPTION;
    }

    // check fct code
    bool isSupported = false;
    for (uint8_t i = 0; i< sizeof( fctsupported ); i++)
    {
        if (fctsupported[i] == modH->u8Buffer[FUNC])
        {
            isSupported = 1;
            break;
        }
    }
    if (!isSupported)
    {
    	modH->u16errCnt ++;
        return EXC_FUNC_CODE;
    }

    return 0; // OK, no exception code thrown
}


/**
 * @brief
 * This method moves Serial buffer data to the Modbus u8Buffer.
 *
 * @return buffer size if OK, ERR_BUFF_OVERFLOW if u8BufferSize >= MAX_BUFFER
 * @ingroup buffer
 */
int8_t getRxBuffer(modbusHandler_t *modH)
{
    bool bBuffOverflow = false;

    if (modH->EN_Port)
    {
    	HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
    }

    modH->u8BufferSize = RingCountBytes(&modH->xBufferRX);
    RingGetAllBytes(&modH->xBufferRX, modH->u8Buffer);

    modH->u16InCnt++;


    if (bBuffOverflow)
    {
    	modH->u16errCnt++;
        return ERR_BUFF_OVERFLOW;  //using queues this will not happen
    }
    return modH->u8BufferSize;
}





/**
 * @brief
 * This method validates slave incoming messages
 *
 * @return 0 if OK, EXCEPTION if anything fails
 * @ingroup modH Modbus handler
 */
uint8_t validateRequest(modbusHandler_t *modH)
{
	// check message crc vs calculated crc

#if ENABLE_TCP ==1
	    uint16_t u16MsgCRC;
		    u16MsgCRC= ((modH->u8Buffer[modH->u8BufferSize - 2] << 8)
		   	         | modH->u8Buffer[modH->u8BufferSize - 1]); // combine the crc Low & High bytes

	    if (modH->xTypeHW != TCP_HW)
	    {
	    	if ( calcCRC( modH->u8Buffer,  modH->u8BufferSize-2 ) != u16MsgCRC )
	    	{
	    		modH->u16errCnt ++;
	    		return ERR_BAD_CRC;
	    		}
	    }
#else
	    uint16_t u16MsgCRC;
	    u16MsgCRC= ((modH->u8Buffer[modH->u8BufferSize - 2] << 8)
	    		   	         | modH->u8Buffer[modH->u8BufferSize - 1]); // combine the crc Low & High bytes


	    if ( calcCRC( modH->u8Buffer,  modH->u8BufferSize-2 ) != u16MsgCRC )
	    {
	       		modH->u16errCnt ++;
	       		return ERR_BAD_CRC;
	    }


#endif

	    // check fct code
	    bool isSupported = false;
	    for (uint8_t i = 0; i< sizeof( fctsupported ); i++)
	    {
	        if (fctsupported[i] == modH->u8Buffer[FUNC])
	        {
	            isSupported = 1;
	            break;
	        }
	    }
	    if (!isSupported)
	    {
	    	modH->u16errCnt ++;
	        return EXC_FUNC_CODE;
	    }

	    // check start address & nb range
	    uint16_t u16regs = 0;
	    //uint8_t u8regs;
	    switch ( modH->u8Buffer[ FUNC ] )
	    {
	    case MB_FC_READ_COILS:
	    case MB_FC_READ_DISCRETE_INPUT:
	    case MB_FC_WRITE_MULTIPLE_COILS:
	        u16regs = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ]) / 16;
	        u16regs += word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ]) /16;
	        if (u16regs > modH->u16regsize) return EXC_ADDR_RANGE;
	        break;
	    case MB_FC_WRITE_COIL:
	        u16regs = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ]) / 16;
	        if (u16regs > modH->u16regsize) return EXC_ADDR_RANGE;
	        break;
	    case MB_FC_WRITE_REGISTER :
	        u16regs = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ]);
	        if (u16regs > modH-> u16regsize) return EXC_ADDR_RANGE;
	        break;
	    case MB_FC_READ_REGISTERS :
	    case MB_FC_READ_INPUT_REGISTER :
	    case MB_FC_WRITE_MULTIPLE_REGISTERS :
	        u16regs = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ]);
	        u16regs += word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ]);
	        if (u16regs > modH->u16regsize) return EXC_ADDR_RANGE;
	        break;
	    }
	    return 0; // OK, no exception code thrown

}

/**
 * @brief
 * This method creates a word from 2 bytes
 *
 * @return uint16_t (word)
 * @ingroup H  Most significant byte
 * @ingroup L  Less significant byte
 */
uint16_t word(uint8_t H, uint8_t L)
{
	bytesFields W;
	W.u8[0] = L;
	W.u8[1] = H;

	return W.u16[0];
}


/**
 * @brief
 * This method calculates CRC
 *
 * @return uint16_t calculated CRC value for the message
 * @ingroup Buffer
 * @ingroup u8length
 */
uint16_t calcCRC(uint8_t *Buffer, uint8_t u8length)
{
    unsigned int temp, temp2, flag;
    temp = 0xFFFF;
    for (unsigned char i = 0; i < u8length; i++)
    {
        temp = temp ^ Buffer[i];
        for (unsigned char j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp >>=1;
            if (flag)
                temp ^= 0xA001;
        }
    }
    // Reverse byte order.
    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;
    // the returned value is already swapped
    // crcLo byte is first & crcHi byte is last
    return temp;

}


/**
 * @brief
 * This method builds an exception message
 *
 * @ingroup u8exception exception number
 * @ingroup modH modbus handler
 */
void buildException( uint8_t u8exception, modbusHandler_t *modH )
{
    uint8_t u8func = modH->u8Buffer[ FUNC ];  // get the original FUNC code

    modH->u8Buffer[ ID ]      = modH->u8id;
    modH->u8Buffer[ FUNC ]    = u8func + 0x80;
    modH->u8Buffer[ 2 ]       = u8exception;
    modH->u8BufferSize         = EXCEPTION_SIZE;
}


#if ENABLE_USB_CDC == 1
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
#endif

/**
 * @brief
 * This method transmits u8Buffer to Serial line.
 * Only if u8txenpin != 0, there is a flow handling in order to keep
 * the RS485 transceiver in output state as long as the message is being sent.
 * This is done with TC bit.
 * The CRC is appended to the buffer before starting to send it.
 *
 * @return nothing
 * @ingroup modH Modbus handler
 */
static void sendTxBuffer(modbusHandler_t *modH)
{
    // append CRC to message

#if  ENABLE_TCP == 1
if(modH->xTypeHW != TCP_HW)
	 {
#endif

	uint16_t u16crc = calcCRC(modH->u8Buffer, modH->u8BufferSize);
    modH->u8Buffer[ modH->u8BufferSize ] = u16crc >> 8;
    modH->u8BufferSize++;
    modH->u8Buffer[ modH->u8BufferSize ] = u16crc & 0x00ff;
    modH->u8BufferSize++;

#if ENABLE_TCP == 1
	 }
#endif


#if ENABLE_USB_CDC == 1 || ENABLE_TCP == 1
    if(modH->xTypeHW == USART_HW)
    {
#endif
    	if (modH->EN_Port != NULL)
        {
            // set RS485 transceiver to transmit mode
        	HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_SET);
        }

        // transfer buffer to serial line
        HAL_UART_Transmit_IT(modH->port, modH->u8Buffer,  modH->u8BufferSize);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait notification from TXE interrupt


         if (modH->EN_Port != NULL)
         {
             // must wait transmission end before changing pin state
             //return RS485 transceiver to receive mode

        	 #if defined(STM32H745xx) || defined(STM32H743xx)  || defined(STM32F303xE)
        	    while((modH->port->Instance->ISR & USART_ISR_TC) ==0 )
             #else
        	    while((modH->port->Instance->SR & USART_SR_TC) ==0 )
	    	 #endif
        	 {
        		taskYIELD();
        	 }
        	 HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
         }


         // set timeout for master query
         if(modH->uModbusType == MB_MASTER )
         {
 	    	xTimerReset(modH->xTimerTimeout,0);
         }
#if ENABLE_USB_CDC == 1 || ENABLE_TCP == 1
    }

#if ENABLE_USB_CDC == 1
    else if(modH->xTypeHW == USB_CDC_HW)
	{
    	CDC_Transmit_FS(modH->u8Buffer,  modH->u8BufferSize);
    	// set timeout for master query
    	if(modH->uModbusType == MB_MASTER )
    	{
    	   	xTimerReset(modH->xTimerTimeout,0);
    	}

	}
#endif

#if ENABLE_TCP == 1

    else if(modH->xTypeHW == TCP_HW)
    	{

    	  struct netvector  xNetVectors[2];
    	  uint8_t u8MBAPheader[6];
    	  size_t uBytesWritten;


    	  u8MBAPheader[0] = highByte(modH->u16TransactionID);
    	  u8MBAPheader[1] = lowByte(modH->u16TransactionID);
    	  u8MBAPheader[2] = 0; //protocol ID
    	  u8MBAPheader[3] = 0; //protocol ID
    	  u8MBAPheader[4] = 0; //highbyte data length always 0
    	  u8MBAPheader[5] = modH->u8BufferSize; //highbyte data length

    	  xNetVectors[0].len = 6;
    	  xNetVectors[0].ptr = (void *) u8MBAPheader;

    	  xNetVectors[1].len = modH->u8BufferSize;
    	  xNetVectors[1].ptr = (void *) modH->u8Buffer;

    	  netconn_set_sendtimeout(modH->newconn, modH->u16timeOut);
    	  netconn_write_vectors_partly(modH->newconn, xNetVectors, 2, NETCONN_COPY, &uBytesWritten);
    	  if(modH->uModbusType == MB_MASTER )
    	  {
    	    xTimerReset(modH->xTimerTimeout,0);
    	  }
    	}

#endif

#endif

     modH->u8BufferSize = 0;
     // increase message counter
     modH->u16OutCnt++;

}


/**
 * @brief
 * This method processes functions 1 & 2
 * This method reads a bit array and transfers it to the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup discrete
 */
int8_t process_FC1(modbusHandler_t *modH )
{
    uint8_t u8currentRegister, u8currentBit, u8bytesno, u8bitsno;
    uint8_t u8CopyBufferSize;
    uint16_t u16currentCoil, u16coil;

    // get the first and last coil from the message
    uint16_t u16StartCoil = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
    uint16_t u16Coilno = word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ] );

    // put the number of bytes in the outcoming message
    u8bytesno = (uint8_t) (u16Coilno / 8);
    if (u16Coilno % 8 != 0) u8bytesno ++;
    modH->u8Buffer[ ADD_HI ]  = u8bytesno;
    modH->u8BufferSize         = ADD_LO;
    modH->u8Buffer[modH->u8BufferSize + u8bytesno - 1 ] = 0;

    // read each coil from the register map and put its value inside the outcoming message
    u8bitsno = 0;

    for (u16currentCoil = 0; u16currentCoil < u16Coilno; u16currentCoil++)
    {
        u16coil = u16StartCoil + u16currentCoil;
        u8currentRegister = (uint8_t) (u16coil / 16);
        u8currentBit = (uint8_t) (u16coil % 16);

        bitWrite(
        	modH->u8Buffer[ modH->u8BufferSize ],
            u8bitsno,
		    bitRead( modH->u16regs[ u8currentRegister ], u8currentBit ) );
        u8bitsno ++;

        if (u8bitsno > 7)
        {
            u8bitsno = 0;
            modH->u8BufferSize++;
        }
    }

    // send outcoming message
    if (u16Coilno % 8 != 0) modH->u8BufferSize ++;
    u8CopyBufferSize = modH->u8BufferSize +2;
    sendTxBuffer(modH);
    return u8CopyBufferSize;
}


/**
 * @brief
 * This method processes functions 3 & 4
 * This method reads a word array and transfers it to the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup register
 */
int8_t process_FC3(modbusHandler_t *modH)
{

    uint8_t u8StartAdd = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
    uint8_t u8regsno = word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ] );
    uint8_t u8CopyBufferSize;
    uint8_t i;

    modH->u8Buffer[ 2 ]       = u8regsno * 2;
    modH->u8BufferSize         = 3;

    for (i = u8StartAdd; i < u8StartAdd + u8regsno; i++)
    {
    	modH->u8Buffer[ modH->u8BufferSize ] = highByte(modH->u16regs[i]);
    	modH->u8BufferSize++;
    	modH->u8Buffer[ modH->u8BufferSize ] = lowByte(modH->u16regs[i]);
    	modH->u8BufferSize++;
    }
    u8CopyBufferSize = modH->u8BufferSize +2;
    sendTxBuffer(modH);

    return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 5
 * This method writes a value assigned by the master to a single bit
 *
 * @return u8BufferSize Response to master length
 * @ingroup discrete
 */
int8_t process_FC5( modbusHandler_t *modH )
{
    uint8_t u8currentRegister, u8currentBit;
    uint8_t u8CopyBufferSize;
    uint16_t u16coil = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );

    // point to the register and its bit
    u8currentRegister = (uint8_t) (u16coil / 16);
    u8currentBit = (uint8_t) (u16coil % 16);

    // write to coil
    bitWrite(
    	modH->u16regs[ u8currentRegister ],
        u8currentBit,
		modH->u8Buffer[ NB_HI ] == 0xff );


    // send answer to master
    modH->u8BufferSize = 6;
    u8CopyBufferSize =  modH->u8BufferSize +2;
    sendTxBuffer(modH);

    return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 6
 * This method writes a value assigned by the master to a single word
 *
 * @return u8BufferSize Response to master length
 * @ingroup register
 */
int8_t process_FC6(modbusHandler_t *modH )
{

    uint8_t u8add = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
    uint8_t u8CopyBufferSize;
    uint16_t u16val = word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ] );

    modH->u16regs[ u8add ] = u16val;

    // keep the same header
    modH->u8BufferSize = RESPONSE_SIZE;

    u8CopyBufferSize = modH->u8BufferSize +2;
    sendTxBuffer(modH);

    return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 15
 * This method writes a bit array assigned by the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup discrete
 */
int8_t process_FC15( modbusHandler_t *modH )
{
    uint8_t u8currentRegister, u8currentBit, u8frameByte, u8bitsno;
    uint8_t u8CopyBufferSize;
    uint16_t u16currentCoil, u16coil;
    bool bTemp;

    // get the first and last coil from the message
    uint16_t u16StartCoil = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
    uint16_t u16Coilno = word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ] );


    // read each coil from the register map and put its value inside the outcoming message
    u8bitsno = 0;
    u8frameByte = 7;
    for (u16currentCoil = 0; u16currentCoil < u16Coilno; u16currentCoil++)
    {

        u16coil = u16StartCoil + u16currentCoil;
        u8currentRegister = (uint8_t) (u16coil / 16);
        u8currentBit = (uint8_t) (u16coil % 16);

        bTemp = bitRead(
        			modH->u8Buffer[ u8frameByte ],
                    u8bitsno );

        bitWrite(
            modH->u16regs[ u8currentRegister ],
            u8currentBit,
            bTemp );

        u8bitsno ++;

        if (u8bitsno > 7)
        {
            u8bitsno = 0;
            u8frameByte++;
        }
    }

    // send outcoming message
    // it's just a copy of the incomping frame until 6th byte
    modH->u8BufferSize         = 6;
    u8CopyBufferSize = modH->u8BufferSize +2;
    sendTxBuffer(modH);
    return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 16
 * This method writes a word array assigned by the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup register
 */
int8_t process_FC16(modbusHandler_t *modH )
{
    uint8_t u8StartAdd = modH->u8Buffer[ ADD_HI ] << 8 | modH->u8Buffer[ ADD_LO ];
    uint8_t u8regsno = modH->u8Buffer[ NB_HI ] << 8 | modH->u8Buffer[ NB_LO ];
    uint8_t u8CopyBufferSize;
    uint8_t i;
    uint16_t temp;

    // build header
    modH->u8Buffer[ NB_HI ]   = 0;
    modH->u8Buffer[ NB_LO ]   = u8regsno;
    modH->u8BufferSize         = RESPONSE_SIZE;

    // write registers
    for (i = 0; i < u8regsno; i++)
    {
        temp = word(
        		modH->u8Buffer[ (BYTE_CNT + 1) + i * 2 ],
				modH->u8Buffer[ (BYTE_CNT + 2) + i * 2 ]);

        modH->u16regs[ u8StartAdd + i ] = temp;
    }
    u8CopyBufferSize = modH->u8BufferSize +2;
    sendTxBuffer(modH);

    return u8CopyBufferSize;
}





