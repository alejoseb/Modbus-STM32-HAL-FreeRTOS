/*
 * Modbus.h
 *
 *  Created on: May 5, 2020
 *      Author: Alejandro Mera
 */

#ifndef THIRD_PARTY_MODBUS_INC_MODBUS_H_
#define THIRD_PARTY_MODBUS_INC_MODBUS_H_


#include "ModbusConfig.h"
#include <inttypes.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "timers.h"


typedef enum
{
    USART_HW = 1,
    USB_CDC_HW = 2,
    TCP_HW = 3,
	USART_HW_DMA = 4,
}mb_hardware_t ;


typedef enum
{
    MB_SLAVE = 3,
    MB_MASTER = 4
}mb_masterslave_t ;

typedef enum
{
    ADDRESS_BROADCAST = 0,  //!< broadcast mode -> modH->u8Buffer[ID] == 0
    ADDRESS_NORMAL = 1,     //!< normal mode -> modH->u8Buffer[ID] > 0
}mb_address_t ;

/**
 * @enum MB_FC
 * @brief
 * Modbus function codes summary.
 * These are the implement function codes either for Master or for Slave.
 *
 * @see also fctsupported
 * @see also modbus_t
 */
typedef enum MB_FC
{
    MB_FC_READ_COILS               = 1,	 /*!< FCT=1 -> read coils or digital outputs */
    MB_FC_READ_DISCRETE_INPUT      = 2,	 /*!< FCT=2 -> read digital inputs */
    MB_FC_READ_REGISTERS           = 3,	 /*!< FCT=3 -> read registers or analog outputs */
    MB_FC_READ_INPUT_REGISTER      = 4,	 /*!< FCT=4 -> read analog inputs */
    MB_FC_WRITE_COIL               = 5,	 /*!< FCT=5 -> write single coil or output */
    MB_FC_WRITE_REGISTER           = 6,	 /*!< FCT=6 -> write single register */
    MB_FC_WRITE_MULTIPLE_COILS     = 15, /*!< FCT=15 -> write multiple coils or outputs */
    MB_FC_WRITE_MULTIPLE_REGISTERS = 16	 /*!< FCT=16 -> write multiple registers */
}mb_functioncode_t;


typedef struct
{
uint8_t uxBuffer[MAX_BUFFER];
uint8_t u8start;
uint8_t u8end;
uint8_t u8available;
bool    overflow;
}modbusRingBuffer_t;




/**
 * @enum MESSAGE
 * @brief
 * Indexes to telegram frame positions
 */
typedef enum MESSAGE
{
    ID                             = 0, //!< ID field
    FUNC, //!< Function code position
    ADD_HI, //!< Address high byte
    ADD_LO, //!< Address low byte
    NB_HI, //!< Number of coils or registers high byte
    NB_LO, //!< Number of coils or registers low byte
    BYTE_CNT  //!< byte counter
}mb_message_t;

typedef enum COM_STATES
{
    COM_IDLE                     = 0,
    COM_WAITING                  = 1,

}mb_com_state_t;

typedef enum ERR_OP_LIST
{
    // Errors
	ERR_NOT_MASTER                = 10,
    ERR_POLLING                   = 11,
    ERR_BUFF_OVERFLOW             = 12,
    ERR_BAD_CRC                   = 13,
    ERR_EXCEPTION                 = 14,
    ERR_BAD_SIZE                  = 15,
    ERR_BAD_ADDRESS               = 16,
    ERR_TIME_OUT		          = 17,
    ERR_BAD_SLAVE_ID		      = 18,
	ERR_BAD_TCP_ID		          = 19,
	// Operations
	OP_OK_QUERY				      = 20  // this value is not an error, it is a number different than zero to acknowledge a correct operation,
                                        // which is needed because FreeRTOS notifications return zero on timeout.
	                                    // Therefore we define our own Error and Operation codes and keep zero exclusively for FreeRTOS primitives
}mb_err_op_t;








enum
{
    EXC_FUNC_CODE = 1,
    EXC_ADDR_RANGE = 2,
    EXC_REGS_QUANT = 3,
    EXC_EXECUTE = 4
};

typedef union {
	uint8_t  u8[4];
	uint16_t u16[2];
	uint32_t u32;

} bytesFields ;





/**
 * @struct modbus_t
 * @brief
 * Master query structure:
 * This structure contains all the necessary fields to make the Master generate a Modbus query.
 * A Master may keep several of these structures and send them cyclically or
 * use them according to program needs.
 */
typedef struct
{
    uint8_t u8id;          /*!< Slave address between 1 and 247. 0 means broadcast */
    mb_functioncode_t u8fct;         /*!< Function code: 1, 2, 3, 4, 5, 6, 15 or 16 */
    uint16_t u16RegAdd;    /*!< Address of the first register to access at slave/s */
    uint16_t u16CoilsNo;   /*!< Number of coils or registers to access */
    uint16_t *u16reg;     /*!< Pointer to memory image in master */
    uint32_t *u32CurrentTask; /*!< Pointer to the task that will receive notifications from Modbus */
#if ENABLE_TCP ==1
    uint32_t   xIpAddress;
    uint16_t u16Port;
    uint8_t  u8clientID;
#endif
}
modbus_t;


#if ENABLE_TCP == 1
typedef struct
{
	struct netconn *conn;
	uint32_t aging;
}
tcpclients_t;

#endif


/**
 * @struct modbusHandler_t
 * @brief
 * Modbus handler structure
 * Contains all the variables required for Modbus daemon operation
 */
typedef struct
{

	mb_masterslave_t uModbusType;
	UART_HandleTypeDef *port; //HAL Serial Port handler
	uint8_t u8id; //!< 0=master, 1..247=slave number
	GPIO_TypeDef* EN_Port; //!< flow control pin: 0=USB or RS-232 mode, >1=RS-485 mode
	uint16_t EN_Pin;  //!< flow control pin: 0=USB or RS-232 mode, >1=RS-485 mode
	mb_err_op_t i8lastError;
	uint8_t u8Buffer[MAX_BUFFER]; //Modbus buffer for communication
	uint8_t u8BufferSize;
	uint8_t u8lastRec;
	uint16_t *u16regs;
	uint16_t u16InCnt, u16OutCnt, u16errCnt; //keep statistics of Modbus traffic
	uint16_t u16timeOut;
	uint16_t u16regsize;
	uint8_t dataRX;
	int8_t i8state;

    mb_address_t u8AddressMode; //!< 0=broadcast, 1..247=normal

	//FreeRTOS components

	//Queue Modbus Telegram
	osMessageQueueId_t QueueTelegramHandle;

	//Task Modbus slave
	osThreadId_t myTaskModbusAHandle;
	//Timer RX Modbus
	xTimerHandle xTimerT35;
	//Timer MasterTimeout
	xTimerHandle xTimerTimeout;
	//Semaphore for Modbus data
	osSemaphoreId_t ModBusSphrHandle;
	// RX ring buffer for USART
	modbusRingBuffer_t xBufferRX;
	// type of hardware  TCP, USB CDC, USART
	mb_hardware_t xTypeHW;

#if ENABLE_TCP == 1

	tcpclients_t newconns[NUMBERTCPCONN];
	struct netconn *conn;
	uint32_t xIpAddress;
	uint16_t u16TransactionID;
	uint16_t uTcpPort; // this is only used for the slave (i.e., the server)
	uint8_t newconnIndex;

#endif

}
modbusHandler_t;


enum
{
    RESPONSE_SIZE = 6,
    EXCEPTION_SIZE = 3,
    CHECKSUM_SIZE = 2
};



extern modbusHandler_t *mHandlers[MAX_M_HANDLERS];

// Function prototypes
void ModbusInit(modbusHandler_t * modH);
void ModbusStart(modbusHandler_t * modH);

#if ENABLE_USB_CDC == 1
void ModbusStartCDC(modbusHandler_t * modH);
#endif

void setTimeOut( uint16_t u16timeOut); //!<write communication watch-dog timer
uint16_t getTimeOut(); //!<get communication watch-dog timer value
bool getTimeOutState(); //!<get communication watch-dog timer state
void ModbusQuery(modbusHandler_t * modH, modbus_t telegram ); // put a query in the queue tail
uint32_t ModbusQueryV2(modbusHandler_t * modH, modbus_t telegram ); // put a query in the queue tail and waits for the notification
void ModbusQueryInject(modbusHandler_t * modH, modbus_t telegram); //put a query in the queue head
void StartTaskModbusSlave(void *argument); //slave
void StartTaskModbusMaster(void *argument); //master
uint16_t calcCRC(uint8_t *Buffer, uint8_t u8length);

#if ENABLE_TCP == 1
void ModbusCloseConn(struct netconn *conn); //close the TCP connection
void ModbusCloseConnNull(modbusHandler_t * modH); //close the TCP connection and cleans the modbus handler
#endif


//Function prototypes for ModbusRingBuffer
void RingAdd(modbusRingBuffer_t *xRingBuffer, uint8_t u8Val); // adds a byte to the ring buffer
uint8_t RingGetAllBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer); // gets all the available bytes into buffer and return the number of bytes read
uint8_t RingGetNBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer, uint8_t uNumber); // gets uNumber of bytes from ring buffer, returns the actual number of bytes read
uint8_t RingCountBytes(modbusRingBuffer_t *xRingBuffer); // return the number of available bytes
void RingClear(modbusRingBuffer_t *xRingBuffer); // flushes the ring buffer

extern uint8_t numberHandlers; //global variable to maintain the number of concurrent handlers




/* prototypes of the original library not implemented

uint16_t getInCnt(); //!<number of incoming messages
uint16_t getOutCnt(); //!<number of outcoming messages
uint16_t getErrCnt(); //!<error counter
uint8_t getID(); //!<get slave ID between 1 and 247
uint8_t getState();
uint8_t getLastError(); //!<get last error message
void setID( uint8_t u8id ); //!<write new ID for the slave
void setTxendPinOverTime( uint32_t u32overTime );
void ModbusEnd(); //!<finish any communication and release serial communication port

*/



#endif /* THIRD_PARTY_MODBUS_INC_MODBUS_H_ */
