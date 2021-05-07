

# Modbus library for STM32 Microcontrollers
TCP, USART and USB-CDC Modbus RTU Master and Slave library for STM32 microcontrollers 
based on Cube HAL and FreeRTOS.

Includes multiple examples for popular development boards including BluePill, NUCLEO-64, 
NUCLEO-144 and Discovery Boards (Cortex-M3/M4/M7).

This is a port of the Modbus library for Arduino: https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino

Video demo for STM32F4-dicovery board and TouchGFX: https://youtu.be/XDCQvu0LirY

`NEW` port for the [Raspberry PI Pico](https://github.com/alejoseb/Modbus-PI-Pico-FreeRTOS).

## Translations supported by the community:
Traditional Chinese: [繁體中文](TraditionalChineseREADME.md) 

## Characteristics:
- Directly portable to any STM32 MCU supported by ST Cube HAL.
- Portable to other Microcontrollers, like the [Raspberry PI Pico](https://github.com/alejoseb/Modbus-PI-Pico-FreeRTOS), requiring little effort.
- Multithread-safe implementation based on FreeRTOS. 
- Multiple instances of Modbus (Master and/or Slave) can run concurrently in the same MCU,
  only limited by the number of available UART/USART of the MCU.
- RS232 and RS485 compatible.
- USB-CDC RTU master and Slave support for F103 Bluepill board. 
- TCP master and slave support with examples for F429 and H743 MCUs



## File structure
```
├── LICENSE
├── README.md
├── Examples
    ├── ModbusBluepill --> STM32F103C8 USART Slave example
    ├── ModbusBluepillUSB --> STM32F103C8 USART + USB-CDC Master and Slave example
    ├── ModbusF103 --> NUCLEO64-F103RB Modbus Master and Slave example
    ├── ModbusF429 --> NUCLEO144-F429ZI Modbus Slave example
    ├── ModbusF429TCP --> NUCLEO144-F429ZI Modbus TCP example
    ├── ModbusH743 --> NUCLEO144-H743ZI Modbus Slave example
    ├── ModbusH743TCP --> NUCLEO144-H743ZI Modbus TCP example
    ├── ModbusF303 --> NUCLEO64-F303RE Modbus Slave example
    ├── ModbusSTM32F4-discovery --> STM32F4-discovery TouchGFX + Modbus Master example
├── MODBUS-LIB --> Library Folder
    ├── Inc
    │   └── Modbus.h 
    ├── Config
    │   └── ModbusConfigTemplate.h 
    └── Src
        ├── Modbus.c 
        └── UARTCallback.c
 
```
## How to use the examples
Examples provided for STM32CubeIDE Version: 1.3.0 https://www.st.com/en/development-tools/stm32cubeide.html.

- Import the examples in the STM32Cube IDE from the system folder
- Connect your NUCLEO board
- Compile and start your debugging session!
- If you need to adjust the Baud rate or any other parameter use the Cube-MX assistant (recommended). If you change the USART port you need to enable the interrupts for the selected USART. Check UARTCallback.c for more details.

### NOTE 1:
The USB-CDC example supports only the Bluepill development board. It has not been validated with other development boards.
To use this example, you need to activate USB-CDC in your ModbusConfig.h file.

### NOTE 2:
The TCP examples have been validated with NUCLEO F429ZI and H743ZI. 
To use these examples, you need to activate TCP in your ModbusConfig.h file.


## How to port to your own MCU
- Create a new project in STM32Cube IDE
- Configure a USART and activate the global interrupt of it
- Configure the `Preemption priority` of USART interrupt to a lower priority (5 or a higher number for a standard configuration) than your FreeRTOS scheduler. This parameter is changed in the NVIC configuration pane.
- Import the Modbus library folder (MODBUS-LIB) using drag-and-drop from your host operating system to your STM32Cube IDE project
- When asked, choose link folders and files
- Update the include paths in the project's properties to include the `Inc` folder of MODBUS-LIB folder
- Create a ModbusConfig.h using the ModbusConfigTemplate.h and add it to your project in your include path
- Instantiate a new global modbusHandler_t and follow the examples provided in the repository 
- `Note:` If the USART interrupts service for other purposes you have to modify the UARTCalback.c file accordingly


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
- ~~Improve the queue for data reception; the current method is too heavy it should be replaced with a simple buffer, a stream, or another FreeRTOS primitive.~~ Solved Queue replaced by a Ring Buffer (03/19/2021)
- ~~Test with Rs485 transceivers (implemented but not tested)~~ Verified with MAX485 transceivers (01/03/2021)
- ~~MODBUS TCP implementation~~ (28/04/2021)
