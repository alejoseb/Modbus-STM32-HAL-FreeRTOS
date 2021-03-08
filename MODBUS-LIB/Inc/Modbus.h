/*
 * Modbus.h
 *
 *  Created on: May 5, 2020
 *      Author: Alejandro Mera
 */

#ifndef THIRD_PARTY_MODBUS_INC_MODBUS_H_
#define THIRD_PARTY_MODBUS_INC_MODBUS_H_

#include <inttypes.h>
#include "main.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "timers.h"


#define T35  5
#define MAX_BUFFER  64	//!< maximum size for the communication buffer in bytes
#define TIMEOUT_MODBUS 1000
#define MAX_M_HANDLERS 2
#define MAX_TELEGRAMS 2 //Max number of Telegrams for master

/**
 * @struct modbus_t
 * @brief
 * Master query structure:
 * This includes all the necessary fields to make the Master generate a Modbus query.
 * A Master may keep several of these structures and send them cyclically or
 * use them according to program needs.
 */
typedef struct
{
    uint8_t u8id;          /*!< Slave address between 1 and 247. 0 means broadcast */
    uint8_t u8fct;         /*!< Function code: 1, 2, 3, 4, 5, 6, 15 or 16 */
    uint16_t u16RegAdd;    /*!< Address of the first register to access at slave/s */
    uint16_t u16CoilsNo;   /*!< Number of coils or registers to access */
    uint16_t *au16reg;     /*!< Pointer to memory image in master */
    uint32_t *u32CurrentTask; /*!< Pointer to the task that will receive notifications from Modbus */
}
modbus_t;

/**
 * @struct modbusHandler_t
 * @brief
 * Modbus handler structure
 * Contains all the variables required for Modbus daemon operation
 */
typedef struct
{
	uint8_t uiModbusType;
	UART_HandleTypeDef *port; //HAL Serial Port handler
	uint8_t u8id; //!< 0=master, 1..247=slave number
	GPIO_TypeDef* EN_Port; //!< flow control pin: 0=USB or RS-232 mode, >1=RS-485 mode
	uint16_t EN_Pin;  //!< flow control pin: 0=USB or RS-232 mode, >1=RS-485 mode
	int8_t i8lastError;
	uint8_t au8Buffer[MAX_BUFFER]; //Modbus buffer for communication
	uint8_t u8BufferSize;
	uint8_t u8lastRec;
	uint16_t *au16regs;
	uint16_t u16InCnt, u16OutCnt, u16errCnt; //keep statistics of Modbus traffic
	uint16_t u16timeOut;
	uint32_t u32time, u32timeOut, u32overTime;
	uint16_t u16regsize;
	uint8_t dataRX;
	int8_t i8state;
	//uint8_t u8exception;

	//FreeRTOS components
	//Queue Modbus RX
	osMessageQueueId_t QueueModbusHandle;
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

}
modbusHandler_t;


enum
{
    RESPONSE_SIZE = 6,
    EXCEPTION_SIZE = 3,
    CHECKSUM_SIZE = 2

};


enum
{
    SLAVE_RTU = 3,
    MASTER_RTU = 4
	//SLAVE_TCP = 5,
	//MASTER_TCP =6,

};




/**
 * @enum MESSAGE
 * @brief
 * Indexes to telegram frame positions
 */
enum MESSAGE
{
    ID                             = 0, //!< ID field
    FUNC, //!< Function code position
    ADD_HI, //!< Address high byte
    ADD_LO, //!< Address low byte
    NB_HI, //!< Number of coils or registers high byte
    NB_LO, //!< Number of coils or registers low byte
    BYTE_CNT  //!< byte counter
};

/**
 * @enum MB_FC
 * @brief
 * Modbus function codes summary.
 * These are the implement function codes either for Master or for Slave.
 *
 * @see also fctsupported
 * @see also modbus_t
 */
enum MB_FC
{
    MB_FC_NONE                     = 0,   /*!< null operator */
    MB_FC_READ_COILS               = 1,	/*!< FCT=1 -> read coils or digital outputs */
    MB_FC_READ_DISCRETE_INPUT      = 2,	/*!< FCT=2 -> read digital inputs */
    MB_FC_READ_REGISTERS           = 3,	/*!< FCT=3 -> read registers or analog outputs */
    MB_FC_READ_INPUT_REGISTER      = 4,	/*!< FCT=4 -> read analog inputs */
    MB_FC_WRITE_COIL               = 5,	/*!< FCT=5 -> write single coil or output */
    MB_FC_WRITE_REGISTER           = 6,	/*!< FCT=6 -> write single register */
    MB_FC_WRITE_MULTIPLE_COILS     = 15,	/*!< FCT=15 -> write multiple coils or outputs */
    MB_FC_WRITE_MULTIPLE_REGISTERS = 16	/*!< FCT=16 -> write multiple registers */
};

enum COM_STATES
{
    COM_IDLE                     = 0,
    COM_WAITING                  = 1

};

enum ERR_LIST
{
    ERR_NOT_MASTER                = -1,
    ERR_POLLING                   = -2,
    ERR_BUFF_OVERFLOW             = -3,
    ERR_BAD_CRC                   = -4,
    ERR_EXCEPTION                 = -5,
	ERR_BAD_SIZE                  = -6,
	ERR_BAD_ADDRESS               = -7,
	ERR_TIME_OUT				  = -8,
	ERR_BAD_SLAVE_ID			  = -9

};

enum
{
    NO_REPLY = 255,
    EXC_FUNC_CODE = 1,
    EXC_ADDR_RANGE = 2,
    EXC_REGS_QUANT = 3,
    EXC_EXECUTE = 4
};

typedef union {
	uint8_t   u8[4];
	uint16_t u16[2];
	uint32_t u32;

} bytesFields ;


modbusHandler_t *mHandlers[MAX_M_HANDLERS];

// Function prototypes
void ModbusInit(modbusHandler_t * modH);
void ModbusStart(modbusHandler_t * modH);
void setTimeOut( uint16_t u16timeOut); //!<write communication watch-dog timer
uint16_t getTimeOut(); //!<get communication watch-dog timer value
bool getTimeOutState(); //!<get communication watch-dog timer state
void ModbusQuery(modbusHandler_t * modH, modbus_t telegram ); // put a query in the queue tail
void ModbusQueryInject(modbusHandler_t * modH, modbus_t telegram); //put a query in the queue head
uint16_t getInCnt(); //!<number of incoming messages
uint16_t getOutCnt(); //!<number of outcoming messages
uint16_t getErrCnt(); //!<error counter
uint8_t getID(); //!<get slave ID between 1 and 247
uint8_t getState();
uint8_t getLastError(); //!<get last error message
void setID( uint8_t u8id ); //!<write new ID for the slave
void setTxendPinOverTime( uint32_t u32overTime );
void ModbusEnd(); //!<finish any communication and release serial communication port
void StartTaskModbusSlave(void *argument); //slave
void StartTaskModbusMaster(void *argument); //master
uint16_t calcCRC(uint8_t *Buffer, uint8_t u8length);
extern uint8_t numberHandlers;


#endif /* THIRD_PARTY_MODBUS_INC_MODBUS_H_ */
