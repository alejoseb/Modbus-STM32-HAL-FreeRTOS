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

#ifndef ENABLE_USART_DMA
#define ENABLE_USART_DMA 0
#endif

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define lowByte(w) ((w) & 0xff)
#define highByte(w) ((w) >> 8)


modbusHandler_t *mHandlers[MAX_M_HANDLERS];


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
    .stack_size = 256 * 6
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
static int16_t getRxBuffer(modbusHandler_t *modH);
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
//static int16_t getRxBuffer(modbusHandler_t *modH);
static int8_t SendQuery(modbusHandler_t *modH ,  modbus_t telegram);

/* Internal type that describes a single Modbus memory region */
typedef struct {
    uint16_t *data;         /* pointer to the uint16_t register/coil array */
    uint16_t startAddress;  /* first valid Modbus address in this region     */
    uint16_t size;          /* number of uint16_t words in data[]            */
} mb_region_t;

static mb_region_t getCoilsRegion(modbusHandler_t *modH);
static mb_region_t getDIRegion(modbusHandler_t *modH);
static mb_region_t getHRRegion(modbusHandler_t *modH);
static mb_region_t getIRRegion(modbusHandler_t *modH);

#if ENABLE_TCP ==1

static bool TCPwaitConnData(modbusHandler_t *modH);
static void  TCPinitserver(modbusHandler_t *modH);
static mb_errot_t TCPconnectserver(modbusHandler_t * modH, modbus_t *telegram);
static mb_errot_t TCPgetRxBuffer(modbusHandler_t * modH);

#endif




/* Ring Buffer functions */

/**
 * @brief Appends one byte to the ring buffer.
 * Must be called with the USART RX interrupt disabled or from within the RX ISR.
 * @param xRingBuffer  Pointer to the ring buffer
 * @param u8Val        Byte to append
 */
void RingAdd(modbusRingBuffer_t *xRingBuffer, uint8_t u8Val)
{

	xRingBuffer->uxBuffer[xRingBuffer->u8end] = u8Val;
	xRingBuffer->u8end = (xRingBuffer->u8end + 1) % MAX_BUFFER;
	if (xRingBuffer->u8available == MAX_BUFFER)
	{
		xRingBuffer->overflow = true;
		xRingBuffer->u8start = (xRingBuffer->u8start + 1) % MAX_BUFFER;
	}
	else
	{
		xRingBuffer->overflow = false;
		xRingBuffer->u8available++;
	}

}

/**
 * @brief Reads all available bytes from the ring buffer into a flat buffer.
 * Must be called with the USART RX interrupt disabled.
 * @param xRingBuffer  Pointer to the ring buffer
 * @param buffer       Destination byte array (must be at least MAX_BUFFER bytes)
 * @return Number of bytes copied
 */
uint8_t RingGetAllBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer)
{
	return RingGetNBytes(xRingBuffer, buffer, xRingBuffer->u8available);
}

/**
 * @brief Reads up to uNumber bytes from the ring buffer into a flat buffer.
 * Must be called with the USART RX interrupt disabled.
 * @param xRingBuffer  Pointer to the ring buffer
 * @param buffer       Destination byte array
 * @param uNumber      Maximum number of bytes to read
 * @return Number of bytes actually copied
 */
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
	xRingBuffer->overflow = false;
	RingClear(xRingBuffer);

	return uCounter;
}

/**
 * @brief Returns the number of bytes currently available in the ring buffer.
 * @param xRingBuffer  Pointer to the ring buffer
 * @return Number of available bytes
 */
uint8_t RingCountBytes(modbusRingBuffer_t *xRingBuffer)
{
return xRingBuffer->u8available;
}

/**
 * @brief Resets the ring buffer to its empty state.
 * @param xRingBuffer  Pointer to the ring buffer
 */
void RingClear(modbusRingBuffer_t *xRingBuffer)
{
xRingBuffer->u8start = 0;
xRingBuffer->u8end = 0;
xRingBuffer->u8available = 0;
xRingBuffer->overflow = false;
}

/* End of Ring Buffer functions */


/* ---------------------------------------------------------------------------
 * Memory region helpers
 * Each function returns the mb_region_t that should be used for a given data
 * type.  When the user has configured a dedicated separate array for that type
 * (non-NULL pointer), that region is returned.  Otherwise the legacy shared
 * u16regs array is returned with startAddress = 0, which preserves the
 * original single-memory-space behaviour.
 * -------------------------------------------------------------------------*/

static mb_region_t getCoilsRegion(modbusHandler_t *modH)
{
    mb_region_t region;
    if (modH->u16coils != NULL) {
        region.data         = modH->u16coils;
        region.startAddress = modH->u16coilsStartAdd;
        region.size         = modH->u16coilsNregs;
    } else {
        region.data         = modH->u16regs;
        region.startAddress = 0;
        region.size         = modH->u16regsize;
    }
    return region;
}

static mb_region_t getDIRegion(modbusHandler_t *modH)
{
    mb_region_t region;
    if (modH->u16discreteInputs != NULL) {
        region.data         = modH->u16discreteInputs;
        region.startAddress = modH->u16discreteInputsStartAdd;
        region.size         = modH->u16discreteInputsNregs;
    } else {
        region.data         = modH->u16regs;
        region.startAddress = 0;
        region.size         = modH->u16regsize;
    }
    return region;
}

static mb_region_t getHRRegion(modbusHandler_t *modH)
{
    mb_region_t region;
    if (modH->u16holdingRegs != NULL) {
        region.data         = modH->u16holdingRegs;
        region.startAddress = modH->u16holdingRegsStartAdd;
        region.size         = modH->u16holdingRegsNregs;
    } else {
        region.data         = modH->u16regs;
        region.startAddress = 0;
        region.size         = modH->u16regsize;
    }
    return region;
}

static mb_region_t getIRRegion(modbusHandler_t *modH)
{
    mb_region_t region;
    if (modH->u16inputRegs != NULL) {
        region.data         = modH->u16inputRegs;
        region.startAddress = modH->u16inputRegsStartAdd;
        region.size         = modH->u16inputRegsNregs;
    } else {
        region.data         = modH->u16regs;
        region.startAddress = 0;
        region.size         = modH->u16regsize;
    }
    return region;
}

/* End of memory region helpers */


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
 * @brief Initializes a Modbus master or slave handler.
 * Creates all required FreeRTOS objects (task, timers, semaphore, queue).
 * Must be called before ModbusStart(). Blocks indefinitely on any
 * allocation failure so errors are immediately visible during bring-up.
 * @param modH  Pointer to an initialized modbusHandler_t structure
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
 * @brief Starts Modbus communication on a USART or USART-DMA port.
 * Must be called after ModbusInit() and after the HAL UART peripheral is ready.
 * Validates the handler configuration, starts the UART receive interrupt (or DMA),
 * and resets traffic counters. For USB-CDC use ModbusStartCDC() instead.
 * @param modH  Pointer to a previously initialized modbusHandler_t structure
 */
void ModbusStart(modbusHandler_t * modH)
{

	if(modH->xTypeHW != USART_HW && modH->xTypeHW != TCP_HW && modH->xTypeHW != USB_CDC_HW  && modH->xTypeHW != USART_HW_DMA )
	{

		while(1); //ERROR select the type of hardware
	}

	if (modH->xTypeHW == USART_HW_DMA && ENABLE_USART_DMA == 0  )
	{
		while(1); //ERROR To use USART_HW_DMA you need to enable it in the ModbusConfig.h file
	}



	if (modH->xTypeHW == USART_HW || modH->xTypeHW ==  USART_HW_DMA )
	{

	      if (modH->EN_Port != NULL )
          {
              // return RS485 transceiver to receive mode
          	HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
          }

          if (modH->uModbusType == MB_SLAVE
              && modH->u16regs           == NULL
              && modH->u16coils          == NULL
              && modH->u16discreteInputs == NULL
              && modH->u16inputRegs      == NULL
              && modH->u16holdingRegs    == NULL)
          {
          	while(1); //ERROR define at least one DATA pointer shared through Modbus
          }

          //check that port is initialized
          while (HAL_UART_GetState(modH->port) != HAL_UART_STATE_READY)
          {

          }

#if ENABLE_USART_DMA ==1
          if( modH->xTypeHW == USART_HW_DMA )
          {


        	  if(HAL_UARTEx_ReceiveToIdle_DMA(modH->port, modH->xBufferRX.uxBuffer, MAX_BUFFER ) != HAL_OK)
        	   {
        	         while(1)
        	         {
        	                    	  //error in your initialization code
        	         }
        	   }
        	  __HAL_DMA_DISABLE_IT(modH->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

          }
          else{

        	  // Receive data from serial port for Modbus using interrupt
        	  if(HAL_UART_Receive_IT(modH->port, &modH->dataRX, 1) != HAL_OK)
        	  {
        	           while(1)
        	           {
        	                       	  //error in your initialization code
        	           }
        	  }

          }


#else
          // Receive data from serial port for Modbus using interrupt
          if(HAL_UART_Receive_IT(modH->port, &modH->dataRX, 1) != HAL_OK)
          {
                while(1)
                {
                     	  //error in your initialization code
                 }
          }

#endif

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
/**
 * @brief Starts Modbus communication on a USB-CDC port.
 * Alternative to ModbusStart() for USB-CDC hardware. Validates the data
 * pointer and resets traffic counters. Requires ENABLE_USB_CDC=1 in
 * ModbusConfig.h. Currently validated on the STM32F103 Bluepill only.
 * @param modH  Pointer to a previously initialized modbusHandler_t structure
 */
void ModbusStartCDC(modbusHandler_t * modH)
{


    if (modH->uModbusType == MB_SLAVE
        && modH->u16regs           == NULL
        && modH->u16coils          == NULL
        && modH->u16discreteInputs == NULL
        && modH->u16inputRegs      == NULL
        && modH->u16holdingRegs    == NULL)
    {
    	while(1); //ERROR define at least one DATA pointer shared through Modbus
    }

    modH->u8lastRec = modH->u8BufferSize = 0;
    modH->u16InCnt = modH->u16OutCnt = modH->u16errCnt = 0;
}
#endif


/**
 * @brief FreeRTOS timer callback for the T3.5 inter-frame silence timer.
 * Fired when the T3.5 character-time gap expires, signalling that a complete
 * Modbus RTU frame has been received. Notifies the corresponding Modbus task.
 * For master handlers it also stops the query timeout timer.
 * @param pxTimer  Handle of the expired timer (used to identify the handler)
 */
void vTimerCallbackT35(TimerHandle_t *pxTimer)
{
	//Notify the Modbus task that a complete frame has arrived
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

/**
 * @brief FreeRTOS timer callback for the master query timeout timer.
 * Fired when the slave does not respond within u16timeOut milliseconds.
 * Notifies the corresponding Modbus task with ERR_TIME_OUT.
 * @param pxTimer  Handle of the expired timer (used to identify the handler)
 */
void vTimerCallbackTimeout(TimerHandle_t *pxTimer)
{
	//Notify the Modbus task that the query timed out
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

/**
 * @brief Waits for an incoming TCP connection and receives one Modbus TCP frame (slave).
 * Uses a round-robin slot allocator across NUMBERTCPCONN connection slots.
 * Accepts new connections on free slots, applies an aging/timeout algorithm to
 * idle connections, and parses the MBAP header to fill modH->u8Buffer.
 * @param modH  Pointer to the slave modbusHandler_t structure
 * @return true if a valid Modbus TCP frame was received, false otherwise
 */
bool TCPwaitConnData(modbusHandler_t *modH)
{
  struct netbuf *inbuf;
  err_t recv_err, accept_err;
  char* buf;
  uint16_t buflen;
  uint16_t uLength;
  bool xTCPvalid;
  xTCPvalid = false;
  tcpclients_t *clientconn;

  //select the next connection slot to work with using round-robin
  modH->newconnIndex++;
  if (modH->newconnIndex >= NUMBERTCPCONN)
  {
	  modH->newconnIndex = 0;
  }
  clientconn = &modH->newconns[modH->newconnIndex];


  //NULL means there is a free connection slot, so we can accept an incoming client connection
  if (clientconn->conn == NULL){
      /* accept any incoming connection */
	  accept_err = netconn_accept(modH->conn, &clientconn->conn);
	  if(accept_err != ERR_OK)
	  {
		  // not valid incoming connection at this time
		  //ModbusCloseConn(clientconn->conn);
		  ModbusCloseConnNull(modH);
		  return xTCPvalid;
      }
	  else
	  {
		  clientconn->aging=0;
	  }

  }

  netconn_set_recvtimeout(clientconn->conn ,  modH->u16timeOut);
  recv_err = netconn_recv(clientconn->conn, &inbuf);

  if (recv_err == ERR_CLSD) //the connection was closed
  {
	  //Close and clean the connection
	  //ModbusCloseConn(clientconn->conn);
	  ModbusCloseConnNull(modH);

	  clientconn->aging = 0;
	  return xTCPvalid;

  }

  if (recv_err == ERR_TIMEOUT) //No new data
   {
 	  //continue the aging process
	  modH->newconns[modH->newconnIndex].aging++;

	  // if the connection is old enough and inactive close and clean it up
	  if (modH->newconns[modH->newconnIndex].aging >= TCPAGINGCYCLES)
	  {
		  //ModbusCloseConn(clientconn->conn);
		  ModbusCloseConnNull(modH);
		  clientconn->aging = 0;
	  }

 	  return xTCPvalid;

   }

  if (recv_err == ERR_OK)
  {
      if (netconn_err(clientconn->conn) == ERR_OK)
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
			          xTCPvalid = true; // we have data for the modbus slave

			      }
			  }

		  }
		  netbuf_delete(inbuf); // delete the buffer always
		  clientconn->aging = 0; //reset the aging counter
	   }
   }

  return xTCPvalid;

}


/**
 * @brief Creates and binds the LWIP TCP listening socket for a Modbus TCP slave.
 * Called once at slave task startup. Binds to modH->uTcpPort (defaults to 502)
 * and puts the connection into LISTEN state. Blocks indefinitely on error.
 * @param modH  Pointer to the slave modbusHandler_t structure
 */
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
		    	 netconn_set_recvtimeout(modH->conn, 1); // this is necessary to make it non blocking
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



/**
 * @brief FreeRTOS task body for a Modbus slave (USART, USART-DMA, USB-CDC, or TCP).
 * Waits for an incoming frame, validates CRC and address, dispatches to the
 * appropriate FC handler, and sends the response. Runs indefinitely.
 * @param argument  Pointer to the modbusHandler_t for this slave instance
 */
void StartTaskModbusSlave(void *argument)
{

  modbusHandler_t *modH =  (modbusHandler_t *)argument;
  //uint32_t notification;

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
			continue; // TCP package was not validated
		  }

	  }
#endif


   if(modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA)
   {

	  ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* Block until a Modbus Frame arrives */

	  if (getRxBuffer(modH) == ERR_BUFF_OVERFLOW)
	  {
	      modH->i8lastError = ERR_BUFF_OVERFLOW;
	   	  modH->u16errCnt++;
		  continue;
	  }

   }

   if (modH->u8BufferSize < 7)
   {
      //The size of the frame is invalid
      modH->i8lastError = ERR_BAD_SIZE;
      modH->u16errCnt++;

	  continue;
    }

    //check broadcast mode
    modH->u8AddressMode = ADDRESS_NORMAL;
    if (modH->u8Buffer[ID] == ADDRESS_BROADCAST)
    {
        modH->u8AddressMode = ADDRESS_BROADCAST;
    }

   // check slave id
    if ( modH->u8Buffer[ID] !=  modH->u8id && modH->u8AddressMode != ADDRESS_BROADCAST)
	{

#if ENABLE_TCP == 0
    	continue; // continue this is not for us
#else
    	if(modH->xTypeHW != TCP_HW)
    	{
    		continue; //for Modbus TCP this is not validated, user should modify accordingly if needed
    	}
#endif
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

		continue;
	 }

	 modH->i8lastError = 0;
	xSemaphoreTake(modH->ModBusSphrHandle , portMAX_DELAY); //before processing the message get the semaphore

	 // process message
	 switch(modH->u8Buffer[ FUNC ] )
	 {
		case MB_FC_READ_COILS:
		case MB_FC_READ_DISCRETE_INPUT:
			if (modH->u8AddressMode == ADDRESS_BROADCAST)
			{
				/* broadcast mode should ignore read function */
				break;
			}
			modH->i8state = process_FC1(modH);
			break;
		case MB_FC_READ_INPUT_REGISTER:
		case MB_FC_READ_REGISTERS :
			if (modH->u8AddressMode == ADDRESS_BROADCAST)
			{
				/* broadcast mode should ignore read function */
				break;
			}
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


	 xSemaphoreGive(modH->ModBusSphrHandle); //Release the semaphore

	 continue;

   }

}



/**
 * @brief Enqueues a Modbus telegram for transmission by the master task (non-blocking).
 * The telegram is added to the tail of the TX queue. The calling task must
 * separately wait for a FreeRTOS task notification to know when the query
 * completes. For a blocking version use ModbusQueryV2().
 * @param modH     Pointer to the master modbusHandler_t structure
 * @param telegram Modbus telegram describing the query (id, function code, address, count, data)
 */
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

/**
 * @brief Enqueues a Modbus telegram and blocks until the query completes (blocking).
 * Combines ModbusQuery() and ulTaskNotifyTake() into a single call. Returns the
 * notification value: OP_OK_QUERY on success, or an ERR_* code on failure.
 * @param modH     Pointer to the master modbusHandler_t structure
 * @param telegram Modbus telegram describing the query
 * @return OP_OK_QUERY on success, ERR_TIME_OUT or other ERR_* code on failure
 */
uint32_t ModbusQueryV2(modbusHandler_t * modH, modbus_t telegram )
{
	//Add the telegram to the TX tail Queue of Modbus
	if (modH->uModbusType == MB_MASTER)
	{
	telegram.u32CurrentTask = (uint32_t *) osThreadGetId();
	xQueueSendToBack(modH->QueueTelegramHandle, &telegram, 0);

	return ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	}
	else{
		while(1);// error a slave cannot send queries as a master
	}
}




/**
 * @brief Injects a high-priority Modbus telegram at the head of the TX queue.
 * Resets the existing queue and places the telegram at the front so it is
 * processed before any pending queries. Use for urgent or diagnostic requests.
 * @param modH     Pointer to the master modbusHandler_t structure
 * @param telegram Modbus telegram to inject
 */
void ModbusQueryInject(modbusHandler_t * modH, modbus_t telegram )
{
	//Add the telegram to the TX head Queue of Modbus
	xQueueReset(modH->QueueTelegramHandle);
	telegram.u32CurrentTask = (uint32_t *) osThreadGetId();
	xQueueSendToFront(modH->QueueTelegramHandle, &telegram, 0);
}


#if ENABLE_TCP ==1
/**
 * @brief Closes and deletes an LWIP TCP connection.
 * Safe to call with a NULL pointer (no-op in that case).
 * @param conn  LWIP netconn handle to close
 */
void ModbusCloseConn(struct netconn *conn)
{

	if(conn != NULL)
	{
		netconn_close(conn);
		netconn_delete(conn);
	}

}

/**
 * @brief Closes the current TCP connection slot and sets its pointer to NULL.
 * Operates on modH->newconns[modH->newconnIndex]. After closing, the slot is
 * free to accept a new incoming client connection.
 * @param modH  Pointer to the modbusHandler_t whose active connection slot to close
 */
void ModbusCloseConnNull(modbusHandler_t * modH)
{

	if(modH->newconns[modH->newconnIndex].conn  != NULL)
	{

		netconn_close(modH->newconns[modH->newconnIndex].conn);
		netconn_delete(modH->newconns[modH->newconnIndex].conn);
		modH->newconns[modH->newconnIndex].conn = NULL;
	}

}

#endif


/**
 * @brief Builds and transmits a Modbus RTU/TCP query frame (master only).
 * Serializes the telegram into modH->u8Buffer, appends CRC (RTU), and
 * calls sendTxBuffer(). Sets the master state to COM_WAITING on success.
 * Called internally by StartTaskModbusMaster(); not intended for direct use.
 * @param modH     Pointer to the master modbusHandler_t structure
 * @param telegram Modbus telegram to transmit
 * @return 0 on success, ERR_NOT_MASTER / ERR_POLLING / ERR_BAD_SLAVE_ID on error
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


	sendTxBuffer(modH);

	xSemaphoreGive(modH->ModBusSphrHandle);

	modH->i8state = COM_WAITING;
	modH->i8lastError = 0;
	return 0;


}

#if ENABLE_TCP == 1

/**
 * @brief Opens a TCP connection to the target slave for a master query.
 * Reuses an existing connection in the slot if already open; opens a new one
 * otherwise. Returns ERR_TIME_OUT if the connection attempt fails.
 * @param modH     Pointer to the master modbusHandler_t structure
 * @param telegram Pointer to the telegram containing the target IP and port
 * @return ERR_OK on success, ERR_TIME_OUT or ERR_BAD_TCP_ID on failure
 */
static  mb_errot_t TCPconnectserver(modbusHandler_t * modH, modbus_t *telegram)
{


	err_t err;
	 tcpclients_t *clientconn;


	//select the current connection slot to work with
    clientconn = &modH->newconns[modH->newconnIndex];

	if(telegram->u8clientID >= NUMBERTCPCONN )
	{
		return ERR_BAD_TCP_ID;
	}

	// if the connection is null open a new connection
	if (clientconn->conn == NULL)
	{
		 clientconn->conn = netconn_new(NETCONN_TCP);
	     if (clientconn->conn  == NULL)
	     {
	   	     while(1)
	   	     {
	     	  // error creating new connection check your configuration and heap size
	     	 }
	     }


	     err = netconn_connect(clientconn->conn, (ip_addr_t *)&telegram->xIpAddress, telegram->u16Port);

	     if (err  != ERR_OK )
	     {

	    	   //ModbusCloseConn(clientconn->conn);
	    	   ModbusCloseConnNull(modH);

	           return ERR_TIME_OUT;
	      }
	}
	return ERR_OK;
}


/**
 * @brief Receives one Modbus TCP response frame from the current connection slot (master).
 * Waits up to modH->u16timeOut ms for data, parses the MBAP header, and fills
 * modH->u8Buffer. Used by the master task after sending a query over TCP.
 * @param modH  Pointer to the master modbusHandler_t structure
 * @return ERR_OK if a valid frame was received, ERR_TIMEOUT or LWIP error otherwise
 */
static mb_errot_t TCPgetRxBuffer(modbusHandler_t * modH)
{

	struct netbuf *inbuf;
	err_t err = ERR_TIME_OUT;
	char* buf;
	uint16_t buflen;
	uint16_t uLength;

	 tcpclients_t *clientconn;
	 //select the current connection slot to work with
     clientconn = &modH->newconns[modH->newconnIndex];

	netconn_set_recvtimeout(clientconn->conn, modH->u16timeOut);
	err = netconn_recv(clientconn->conn, &inbuf);

	uLength = 0;

    if (err == ERR_OK)
    {
    	err = netconn_err(clientconn->conn) ;
    	if (err == ERR_OK)
    	{
    		/* Read the data from the port, blocking if nothing yet there.
   	  	  	We assume the request (the part we care about) is in one netbuf */
  	        err = netbuf_data(inbuf, (void**)&buf, &buflen);
  	        if(err == ERR_OK )
  	        {
                 if( (buflen>11  && (modH->uModbusType == MB_SLAVE  ))  ||
                	 (buflen>=10 && (modH->uModbusType == MB_MASTER ))  ) // minimum frame size for modbus TCP
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

    //netconn_close(modH->newconn);
	//netconn_delete(modH->newconn);
	return err;
}

#endif

/**
 * @brief FreeRTOS task body for a Modbus master (USART, USART-DMA, USB-CDC, or TCP).
 * Dequeues telegrams posted by ModbusQuery()/ModbusQueryV2(), calls SendQuery(),
 * waits for the slave response, validates it, and notifies the calling task.
 * Runs indefinitely.
 * @param argument  Pointer to the modbusHandler_t for this master instance
 */
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
    	  modH->newconnIndex = telegram.u8clientID;
    	  ulNotificationValue = TCPconnectserver( modH, &telegram);
 	      if(ulNotificationValue == ERR_OK)
	      {


 	    	  SendQuery(modH, telegram);
 		     /* Block until a Modbus Frame arrives or query timeouts*/
 		      ulNotificationValue = TCPgetRxBuffer(modH); // TCP receives the data and the notification simultaneously since it is synchronous

 		      if (ulNotificationValue != ERR_OK) //close the TCP connection
 		      {

 		    	 //ModbusCloseConn(modH->newconns[modH->newconnIndex].conn);
 		    	 ModbusCloseConnNull(modH);

 		      }
	      }
 	      else
 	      {
 	    	 //ModbusCloseConn(modH->newconns[modH->newconnIndex].conn);
 	    	 ModbusCloseConnNull(modH);
 	      }
     }
     else // send a query for USART and USB_CDC
     {
   	   SendQuery(modH, telegram);
       /* Block until a Modbus Frame arrives or query timeouts*/
   	   ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
     }
#else
     /*Wait period of silence between modbus frame */
	 if(modH->port->Init.BaudRate <= 19200)
	 	osDelay((int)(35000/modH->port->Init.BaudRate) + 2);
	 else
	 	osDelay(3);

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

      if(modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA) //TCP and USB_CDC use different methods to get the buffer
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

	  if (modH->i8lastError ==0) // no error, we use OP_OK_QUERY, since we need to use a different value than 0 to detect the timeout
	  {
		  xSemaphoreGive(modH->ModBusSphrHandle); //Release the semaphore
		  xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, OP_OK_QUERY, eSetValueWithOverwrite);
	  }


	  continue;
	 }

}

/**
 * @brief Decodes a FC1/FC2 (read coils / discrete inputs) response into the master buffer.
 * Unpacks the bit-stream from the slave response and stores each byte into modH->u16regs.
 * @param modH  Pointer to the master modbusHandler_t structure
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
 * @brief Decodes a FC3/FC4 (read holding / input registers) response into the master buffer.
 * Copies the 16-bit register values from the slave response into modH->u16regs.
 * @param modH  Pointer to the master modbusHandler_t structure
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
 * @brief Validates the slave response received by the master.
 * Checks CRC (RTU only), exception flag, and supported function code.
 * @param modH  Pointer to the master modbusHandler_t structure
 * @return 0 if OK, ERR_BAD_CRC, ERR_EXCEPTION, or EXC_FUNC_CODE on failure
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
 * @brief Transfers received bytes from the ring buffer into modH->u8Buffer.
 * Temporarily disables the UART RX interrupt (USART_HW) to avoid race conditions.
 * On overflow, clears the ring buffer and returns ERR_BUFF_OVERFLOW.
 * @param modH  Pointer to the modbusHandler_t structure
 * @return Number of bytes copied, or ERR_BUFF_OVERFLOW on overflow
 */
int16_t getRxBuffer(modbusHandler_t *modH)
{

    int16_t i16result;

    if(modH->xTypeHW == USART_HW)
    {
    	HAL_UART_AbortReceive_IT(modH->port); // disable interrupts to avoid race conditions on serial port
    }

	if (modH->xBufferRX.overflow)
    {
       	RingClear(&modH->xBufferRX); // clean up the overflowed buffer
       	i16result =  ERR_BUFF_OVERFLOW;
    }
	else
	{
		modH->u8BufferSize = RingGetAllBytes(&modH->xBufferRX, modH->u8Buffer);
		modH->u16InCnt++;
		i16result = modH->u8BufferSize;
	}

	if(modH->xTypeHW == USART_HW)
	{
		HAL_UART_Receive_IT(modH->port, &modH->dataRX, 1);
	}

    return i16result;
}





/**
 * @brief Validates a request frame received by the slave.
 * Checks CRC (RTU only), supported function code, and address/count range
 * against the appropriate memory region (coils, DI, HR, or IR).
 * @param modH  Pointer to the slave modbusHandler_t structure
 * @return 0 if OK, or an EXC_* / ERR_* exception code on failure
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

	    // check start address & nb range using the appropriate memory region
	    mb_region_t region;
	    uint16_t u16Ad;   // Modbus address from the frame
	    uint16_t u16N;    // number of coils / registers from the frame
	    uint32_t u32Offset; // address offset relative to region start (uint32 to avoid overflow)
	    uint32_t u32NRegs;  // frame size check (uint32 to avoid overflow)

	    switch ( modH->u8Buffer[ FUNC ] )
	    {
	    case MB_FC_READ_COILS:
	    case MB_FC_WRITE_MULTIPLE_COILS:
	        region = getCoilsRegion(modH);
	        u16Ad = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
	        u16N  = word( modH->u8Buffer[ NB_HI  ], modH->u8Buffer[ NB_LO  ] );
	        if (u16Ad < region.startAddress) return EXC_ADDR_RANGE;
	        u32Offset = (uint32_t)u16Ad - region.startAddress;
	        // verify that all requested bits fit inside the allocated words
	        if (((u32Offset + u16N + 15u) / 16u) > (uint32_t)region.size) return EXC_ADDR_RANGE;
	        // verify answer frame size in bytes
	        u32NRegs = (uint32_t)u16N / 8u + ((u16N % 8u) ? 1u : 0u) + 5u;
	        if (u32NRegs > 256u) return EXC_REGS_QUANT;
	        break;

	    case MB_FC_READ_DISCRETE_INPUT:
	        region = getDIRegion(modH);
	        u16Ad = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
	        u16N  = word( modH->u8Buffer[ NB_HI  ], modH->u8Buffer[ NB_LO  ] );
	        if (u16Ad < region.startAddress) return EXC_ADDR_RANGE;
	        u32Offset = (uint32_t)u16Ad - region.startAddress;
	        if (((u32Offset + u16N + 15u) / 16u) > (uint32_t)region.size) return EXC_ADDR_RANGE;
	        u32NRegs = (uint32_t)u16N / 8u + ((u16N % 8u) ? 1u : 0u) + 5u;
	        if (u32NRegs > 256u) return EXC_REGS_QUANT;
	        break;

	    case MB_FC_WRITE_COIL:
	        region = getCoilsRegion(modH);
	        u16Ad = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
	        if (u16Ad < region.startAddress) return EXC_ADDR_RANGE;
	        u32Offset = (uint32_t)u16Ad - region.startAddress;
	        if ((u32Offset / 16u) >= (uint32_t)region.size) return EXC_ADDR_RANGE;
	        break;

	    case MB_FC_WRITE_REGISTER:
	        region = getHRRegion(modH);
	        u16Ad = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
	        if (u16Ad < region.startAddress) return EXC_ADDR_RANGE;
	        u32Offset = (uint32_t)u16Ad - region.startAddress;
	        if (u32Offset >= (uint32_t)region.size) return EXC_ADDR_RANGE;
	        break;

	    case MB_FC_READ_REGISTERS:
	    case MB_FC_WRITE_MULTIPLE_REGISTERS:
	        region = getHRRegion(modH);
	        u16Ad = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
	        u16N  = word( modH->u8Buffer[ NB_HI  ], modH->u8Buffer[ NB_LO  ] );
	        if (u16Ad < region.startAddress) return EXC_ADDR_RANGE;
	        u32Offset = (uint32_t)u16Ad - region.startAddress;
	        if ((u32Offset + u16N) > (uint32_t)region.size) return EXC_ADDR_RANGE;
	        // verify answer frame size in bytes
	        u32NRegs = (uint32_t)u16N * 2u + 5u;
	        if (u32NRegs > 256u) return EXC_REGS_QUANT;
	        break;

	    case MB_FC_READ_INPUT_REGISTER:
	        region = getIRRegion(modH);
	        u16Ad = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] );
	        u16N  = word( modH->u8Buffer[ NB_HI  ], modH->u8Buffer[ NB_LO  ] );
	        if (u16Ad < region.startAddress) return EXC_ADDR_RANGE;
	        u32Offset = (uint32_t)u16Ad - region.startAddress;
	        if ((u32Offset + u16N) > (uint32_t)region.size) return EXC_ADDR_RANGE;
	        u32NRegs = (uint32_t)u16N * 2u + 5u;
	        if (u32NRegs > 256u) return EXC_REGS_QUANT;
	        break;
	    }
	    return 0; // OK, no exception code thrown

}

/**
 * @brief Combines two bytes into a 16-bit word (big-endian byte order).
 * @param H  Most significant byte
 * @param L  Least significant byte
 * @return 16-bit value with H as the high byte and L as the low byte
 */
uint16_t word(uint8_t H, uint8_t L)
{
	bytesFields W;
	W.u8[0] = L;
	W.u8[1] = H;

	return W.u16[0];
}


/**
 * @brief Calculates the Modbus RTU CRC-16 for a byte buffer.
 * The returned value is already byte-swapped so that the low byte of the CRC
 * comes first, matching the Modbus RTU frame format.
 * @param Buffer    Pointer to the data bytes
 * @param u8length  Number of bytes to include in the calculation
 * @return CRC-16 value (byte-swapped, ready to append to the frame)
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
 * @brief Builds a Modbus exception response in modH->u8Buffer.
 * Sets the function code high bit and writes the exception code byte.
 * @param u8exception  Modbus exception code (EXC_FUNC_CODE, EXC_ADDR_RANGE, etc.)
 * @param modH         Pointer to the slave modbusHandler_t structure
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
 * @brief Transmits modH->u8Buffer over the configured hardware interface.
 * For USART/DMA: appends CRC, drives EN_Port high for RS-485 transmit enable,
 * sends via HAL interrupt or DMA, waits for the TC flag, then returns EN_Port
 * low (receive mode). For USB-CDC: calls CDC_Transmit_FS(). For TCP: prepends
 * the MBAP header and sends via LWIP netconn. Broadcast requests from a slave
 * are silently discarded (no response sent). Resets the timeout timer on master.
 * @param modH  Pointer to the modbusHandler_t structure
 */
static void sendTxBuffer(modbusHandler_t *modH)
{
    // when in slaveType and u8AddressMode == ADDRESS_BROADCAST, do not send anything
    if (modH->uModbusType == MB_SLAVE && modH->u8AddressMode == ADDRESS_BROADCAST)
    {
        modH->u8BufferSize = 0;
        // increase message counter
        modH->u16OutCnt++;
        return;
    }

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
    if(modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA )
    {
#endif

    	if (modH->EN_Port != NULL)
        {
    		//enable transmitter, disable receiver to avoid echo on RS485 transceivers
    		HAL_HalfDuplex_EnableTransmitter(modH->port);
    		HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_SET);
        }

#if ENABLE_USART_DMA ==1
    	if(modH->xTypeHW == USART_HW)
    	{
#endif
    		// transfer buffer to serial line IT
    		HAL_UART_Transmit_IT(modH->port, modH->u8Buffer,  modH->u8BufferSize);

#if ENABLE_USART_DMA ==1
    	}
        else
        {
        	//transfer buffer to serial line DMA
        	HAL_UART_Transmit_DMA(modH->port, modH->u8Buffer, modH->u8BufferSize);

        }
#endif

        ulTaskNotifyTake(pdTRUE, 250); //wait notification from TXE interrupt
/*
* If you are porting the library to a different MCU check the 
* USART datasheet and add the corresponding family in the following
* preprocessor conditions
*/
#if defined(STM32H7)  || defined(STM32F3) || defined(STM32L4) || defined(STM32L082xx) || defined(STM32F7) || defined(STM32WB) || defined(STM32G070xx) || defined(STM32F0) || defined(STM32G431xx) || defined(STM32H5)
          while((modH->port->Instance->ISR & USART_ISR_TC) ==0 )
#else
          // F429, F103, L152 ...
	  while((modH->port->Instance->SR & USART_SR_TC) ==0 )
#endif
         {
 	        //block the task until the the last byte is send out of the shifting buffer in USART
         }


         if (modH->EN_Port != NULL)
         {

             //return RS485 transceiver to receive mode
        	 HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
        	 //enable receiver, disable transmitter
        	 HAL_HalfDuplex_EnableReceiver(modH->port);

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


    	  u8MBAPheader[0] = highByte(modH->u16TransactionID); // this might need improvement the transaction ID could be validated
    	  u8MBAPheader[1] = lowByte(modH->u16TransactionID);
    	  u8MBAPheader[2] = 0; //protocol ID
    	  u8MBAPheader[3] = 0; //protocol ID
    	  u8MBAPheader[4] = 0; //highbyte data length always 0
    	  u8MBAPheader[5] = modH->u8BufferSize; //highbyte data length

    	  xNetVectors[0].len = 6;
    	  xNetVectors[0].ptr = (void *) u8MBAPheader;

    	  xNetVectors[1].len = modH->u8BufferSize;
    	  xNetVectors[1].ptr = (void *) modH->u8Buffer;


    	  netconn_set_sendtimeout(modH->newconns[modH->newconnIndex].conn, modH->u16timeOut);
    	  err_enum_t err;

    	  err = netconn_write_vectors_partly(modH->newconns[modH->newconnIndex].conn, xNetVectors, 2, NETCONN_COPY, &uBytesWritten);
    	  if (err != ERR_OK )
    	  {

    		 // ModbusCloseConn(modH->newconns[modH->newconnIndex].conn);
    		 ModbusCloseConnNull(modH);

    	  }


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
    uint16_t u16currentRegister;
    uint8_t u8currentBit, u8bytesno, u8bitsno;
    uint8_t u8CopyBufferSize;
    uint16_t u16currentCoil, u16coil;

    // select the region based on whether this is FC1 (coils) or FC2 (discrete inputs)
    mb_region_t region = (modH->u8Buffer[ FUNC ] == MB_FC_READ_COILS)
                         ? getCoilsRegion(modH) : getDIRegion(modH);

    // get start coil as an offset from the beginning of the region array
    uint16_t u16StartCoil = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] )
                            - region.startAddress;
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
        u16currentRegister =  (u16coil / 16);
        u8currentBit = (uint8_t) (u16coil % 16);

        bitWrite(
        	modH->u8Buffer[ modH->u8BufferSize ],
            u8bitsno,
		    bitRead( region.data[ u16currentRegister ], u8currentBit ) );
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
    // select the region based on whether this is FC3 (holding regs) or FC4 (input regs)
    mb_region_t region = (modH->u8Buffer[ FUNC ] == MB_FC_READ_REGISTERS)
                         ? getHRRegion(modH) : getIRRegion(modH);

    uint16_t u16StartAdd = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] )
                           - region.startAddress;
    uint8_t u8regsno = word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ] );
    uint8_t u8CopyBufferSize;
    uint16_t i;

    modH->u8Buffer[ 2 ]       = u8regsno * 2;
    modH->u8BufferSize         = 3;

    for (i = u16StartAdd; i < u16StartAdd + u8regsno; i++)
    {
    	modH->u8Buffer[ modH->u8BufferSize ] = highByte(region.data[i]);
    	modH->u8BufferSize++;
    	modH->u8Buffer[ modH->u8BufferSize ] = lowByte(region.data[i]);
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
    uint8_t u8currentBit;
    uint16_t u16currentRegister;
    uint8_t u8CopyBufferSize;

    mb_region_t region = getCoilsRegion(modH);

    // convert Modbus coil address to a region-relative bit index
    uint16_t u16coil = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] )
                       - region.startAddress;

    // point to the register and its bit
    u16currentRegister = (u16coil / 16);
    u8currentBit = (uint8_t) (u16coil % 16);

    // write to coil
    bitWrite(
    	region.data[ u16currentRegister ],
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
    mb_region_t region = getHRRegion(modH);

    uint16_t u16add = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] )
                      - region.startAddress;
    uint8_t u8CopyBufferSize;
    uint16_t u16val = word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ] );

    region.data[ u16add ] = u16val;

    // keep the same header
    modH->u8BufferSize = RESPONSE_SIZE;

    u8CopyBufferSize = modH->u8BufferSize + 2;
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
    uint8_t u8currentBit, u8frameByte, u8bitsno;
    uint16_t u16currentRegister;
    uint8_t u8CopyBufferSize;
    uint16_t u16currentCoil, u16coil;
    bool bTemp;

    mb_region_t region = getCoilsRegion(modH);

    // get start coil as an offset from the beginning of the region array
    uint16_t u16StartCoil = word( modH->u8Buffer[ ADD_HI ], modH->u8Buffer[ ADD_LO ] )
                            - region.startAddress;
    uint16_t u16Coilno = word( modH->u8Buffer[ NB_HI ], modH->u8Buffer[ NB_LO ] );


    // read each coil from the register map and put its value inside the outcoming message
    u8bitsno = 0;
    u8frameByte = 7;
    for (u16currentCoil = 0; u16currentCoil < u16Coilno; u16currentCoil++)
    {

        u16coil = u16StartCoil + u16currentCoil;
        u16currentRegister = (u16coil / 16);
        u8currentBit = (uint8_t) (u16coil % 16);

        bTemp = bitRead(
        			modH->u8Buffer[ u8frameByte ],
                    u8bitsno );

        bitWrite(
            region.data[ u16currentRegister ],
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
    // it's just a copy of the incoming frame until 6th byte
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
    mb_region_t region = getHRRegion(modH);

    uint16_t u16StartAdd = (modH->u8Buffer[ ADD_HI ] << 8 | modH->u8Buffer[ ADD_LO ])
                           - region.startAddress;
    uint16_t u16regsno = modH->u8Buffer[ NB_HI ] << 8 | modH->u8Buffer[ NB_LO ];
    uint8_t u8CopyBufferSize;
    uint16_t i;
    uint16_t temp;

    // build header
    modH->u8Buffer[ NB_HI ]   = 0;
    modH->u8Buffer[ NB_LO ]   = (uint8_t) u16regsno; // answer is always 256 or less bytes
    modH->u8BufferSize         = RESPONSE_SIZE;

    // write registers
    for (i = 0; i < u16regsno; i++)
    {
        temp = word(
        		modH->u8Buffer[ (BYTE_CNT + 1) + i * 2 ],
				modH->u8Buffer[ (BYTE_CNT + 2) + i * 2 ]);

        region.data[ u16StartAdd + i ] = temp;
    }
    u8CopyBufferSize = modH->u8BufferSize +2;
    sendTxBuffer(modH);

    return u8CopyBufferSize;
}





