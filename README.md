# Modbus library for STM32 Microcontrollers
Modbus RTU Master and Slave library for STM32 microcontrollers 
based on Cube HAL and FreeRTOS.
This is a port of the Modbus library for Arduino: https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino

Video demo for STM32F4-dicovery board and TouchGFX: https://youtu.be/XDCQvu0LirY

## Characteristics:
- Portable to any STM32 MCU supported by ST Cube HAL.
- Multithread-safe implementation based on FreeRTOS. 
- Multiple instances of Modbus (Master and/or Slave) can run concurrently in the same MCU,
  only limited by the number of available UART/USART of the MCU.
- RS485 compatible.

## File structure
```
├── LICENSE
├── README.md
├── ModbusF103 --> NUCLEO64-F103RB Modbus Master and Slave example
├── ModbusF429 --> NUCLEO144-F429ZI Modbus Slave example
├── ModbusH743 --> NUCLEO144-H743ZI Modbus Slave example
├── ModbusF303 --> NUCLEO64-F303RE Modbus Slave example
├── ModbusSTM32F4-discovery --> STM32F4-discovery TouchGFX + Modbus Master example
├── MODBUS-LIB --> Library Folder
    ├── Inc
    │   └── Modbus.h 
    └── Src
        ├── Modbus.c 
        └── UARTCallback.c 
```
## How to use the examples
Examples provided for STM32CubeIDE Version: 1.3.0 https://www.st.com/en/development-tools/stm32cubeide.html.

- Import the examples in the STM32Cube IDE from the system folder
- Connect your NUCLEO board
- Compile and start your debugging session!
- If you need to adjust the Baud rate or any other parameter use the Cube assistant (recommended). If you change the USART you need to enable the interrupts for the selected USART. Check UARTCallback.c for more details.
- For the ModbusF103 example, you can use external USB-to-serial adapters or connect the Master and Slave instances in a loopback (USART1 <--> USART3).
- The ModbusF429 example uses the ST-Link serial port. To test this example you don't need an external USB-to-serial adapter.

## How to port to your own MCU
- Create a new project in STM32Cube IDE
- Configure a USART and activate the global interrupt of it
- Configure the `Preemption priority` of USART interrupt to a lower priority (5 or higher number for a standard configuration) than your FreeRTOS scheduler. This parameter is changed in the NVIC configuration pane.
- Import the Modbus library folder (MODBUS-LIB) using drag-and-drop from your host operating system to your STM32Cube IDE project
- When asked, choose link folders and files
- Update the include paths in the project's properties to include the `Inc` folder of MODBUS-LIB folder
- Instantiate a new modbusHandler_t and follow the examples provided in the repository 
- `Note:` If you use USART interrupts for other purposes you have to modify the UARTCalback.c file accordingly


## Recommended Modbus Master and Slave testing tools for Linux and Windows

### Master client Qmodbus
Linux:    https://launchpad.net/~js-reynaud/+archive/ubuntu/qmodbus

Windows:  https://sourceforge.net/projects/qmodbus/

### Slave simulator
Linux: https://sourceforge.net/projects/pymodslave/

Windows: https://sourceforge.net/projects/modrssim2/

## TODOs:
- Implement wrapper functions for Master function codes. Currently, telegrams are defined manually. 
- Improve function documentation
- Test with Rs485 transceivers (implemented but not tested)
- MODBUS TCP implementation
