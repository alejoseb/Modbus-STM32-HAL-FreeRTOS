#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "main.h"
#include "modbus.h"
#include "cmsis_os.h"
#include "semphr.h"
Model::Model() : modelListener(0)
{
 counter = 0;
 reg = 0;
}

void Model::tick()
{

   if(counter>=10) // this is for slow down the frequency for reading variables
   {

	   counter = 0;
	   if(xSemaphoreTake((QueueHandle_t)ModbusH.ModBusSphrHandle , 300)==pdTRUE)
	   {
	       modbus_t telegram[1];
	       telegram[0].u8id = 1; // slave address
	       telegram[0].u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers write)
	       telegram[0].u16RegAdd = 0x0;
	       telegram[0].u16CoilsNo = 1; // number of elements (coils or registers) to read
	       telegram[0].au16reg = ModbusDATARX; // pointer to a memory array
	       ModbusQuery(&ModbusH, telegram[0]);
	       //if(reg != ModbusDATARX[0])
	       //{
		   modelListener->registerUpdate(ModbusDATARX[0]);
		   reg = ModbusDATARX[0];
	       // }
	       xSemaphoreGive((QueueHandle_t)ModbusH.ModBusSphrHandle);
	   }


   }
   counter++;

}

void Model::RegisterUpDown(int value)
{

	 modbus_t telegram[1];


	 if(xSemaphoreTake((QueueHandle_t)ModbusH.ModBusSphrHandle , 300)==pdTRUE)
	 {
		 reg = reg + value;
		 ModbusDATATX[0] = reg;
		 telegram[0].u8id = 1; // slave address
		 telegram[0].u8fct = MB_FC_WRITE_REGISTER; // function code (this one is registers write)
		 telegram[0].u16RegAdd = 0x0;
		 telegram[0].u16CoilsNo = 1; // number of elements (coils or registers) to read
		 telegram[0].au16reg = ModbusDATATX; // pointer to a memory array
		 ModbusQueryInject(&ModbusH, telegram[0]);
		 xSemaphoreGive((QueueHandle_t)ModbusH.ModBusSphrHandle);

	 }


	//xSemaphoreGive((QueueHandle_t)ModbusH.ModBusSphrHandle);

}

void Model::LedToggleRequested(bool value)
{
	modbus_t telegram[1];

	ModbusDATATX[0] = value;

	//xSemaphoreTake((QueueHandle_t)ModbusH.ModBusSphrHandle , portMAX_DELAY);
	telegram[0].u8id = 1; // slave address
	telegram[0].u8fct = MB_FC_WRITE_COIL; // function code (this one is registers write)
	telegram[0].u16RegAdd = 0x0;
	telegram[0].u16CoilsNo = 1; // number of elements (coils or registers) to read
	telegram[0].au16reg = ModbusDATATX; // pointer to a memory array
	//xSemaphoreGive((QueueHandle_t)ModbusH.ModBusSphrHandle);
	ModbusQueryInject(&ModbusH, telegram[0]);



}
