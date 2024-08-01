

# Modbus library for STM32 Microcontrollers
TCP, USART and USB-CDC Modbus RTU Master and Slave library for STM32 microcontrollers 
based on Cube HAL and FreeRTOS.

Includes multiple examples for popular development boards including BluePill, NUCLEO-64, 
NUCLEO-144 and Discovery Boards (Cortex-M3/M4/M7).

This is a port of the Modbus library for Arduino: https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino

Video demo for STM32F4-discovery board and TouchGFX: https://youtu.be/XDCQvu0LirY

`NEW` Script examples to test the library based on Pymodbus

`NEW` TCP slave (server) multi-client with configurable auto-aging algorithm for management of TCP connections


## Translations supported by the community:
Traditional Chinese: [繁體中文](TraditionalChineseREADME.md) 

## Characteristics:
- Portable to any STM32 MCU supported by ST Cube HAL.
- Portable to other Microcontrollers, like the [Raspberry PI Pico](https://github.com/alejoseb/Modbus-PI-Pico-FreeRTOS), requiring little engineering effort.
- Multithread-safe implementation based on FreeRTOS. 
- Multiple instances of Modbus (Master and/or Slave) can run concurrently in the same MCU,
  only limited by the number of available UART/USART of the MCU.
- RS232 and RS485 compatible.
- USART DMA support for high baudrates with idle-line detection.
- USB-CDC RTU master and Slave support for F103 Bluepill board. 
- TCP master and slave support with examples for F429 and H743 MCUs


## File structure
```
├── LICENSE
├── README.md
├── Examples
    ├── ModbusBluepill --> STM32F103C8 USART Slave
    ├── ModbusBluepillUSB --> STM32F103C8 USART + USB-CDC Master and Slave 
    ├── ModbusF103 --> NUCLEO-F103RB Modbus Master and Slave
    ├── ModbusF429 --> NUCLEO-F429ZI Modbus Slave 
    ├── ModbusF429TCP --> NUCLEO-F429ZI Modbus TCP
    ├── ModbusF429DMA --> NUCLEO-F429ZI Modbus RTU DMA master and slave 
    ├── ModbusL152DMA --> NUCLEO-L152RE Modbus RTU DMA slave
    ├── ModbusH743 --> NUCLEO-H743ZI Modbus Slave
    ├── ModbusH743TCP --> NUCLEO-H743ZI Modbus TCP
    ├── ModbusF303 --> NUCLEO-F303RE Modbus Slave
    ├── ModbusSTM32F4-discovery --> STM32F4-discovery TouchGFX + Modbus Master
    ├── ModbusWB55DMA --> P-NUCLEO-WB55 Modbus RTU DMA slave with RS485 
    ├── ModbusG070 --> NUCLEO-G070RB Modbus Slave
    ├── ModbusF030 --> STM32F030RCT6 USART Slave
    ├── ModbusH503 --> STM32H503RBTx USART Slave
    ├── ModbusG431 --> NUCLEO-G431KB USART Slave
├── Script
    ├── *.ipynb --> various master and slave Jupyter notebooks for testing
├── MODBUS-LIB --> Library Folder
    ├── Inc
    │   └── Modbus.h 
    ├── Config
    │   └── ModbusConfigTemplate.h --> Configuration Template
    └── Src
        ├── Modbus.c 
        └── UARTCallback.c
 
```
## How to use the examples
Examples provided for STM32CubeIDE Version: 1.8.0 https://www.st.com/en/development-tools/stm32cubeide.html.

- Import the examples in the STM32Cube IDE from the system folder
- Connect your NUCLEO board
- Compile and start your debugging session!
- If you need to adjust the Baud rate or any other parameter use the Cube-MX assistant (recommended). If you change the USART port you need to enable the interrupts for the selected USART. Check UARTCallback.c for more details.

### Notes and Known issues :
- The standard interrupt mode for Modbus RTU USART is suitable for 115200 bps or lower baud rates. 
For Higher baud rates---tested up to 2 Mbps---it is recommended to use the DMA mode. Check the corresponding examples. It will require 
extra configurations for the DMA channels in the Cube HAL.

- The USB-CDC example supports only the Bluepill development board. It has not been validated with other development boards.
To use this example, you need to activate USB-CDC in your ModbusConfig.h file.

- The TCP examples have been validated with NUCLEO F429ZI and H743ZI. 
To use these examples, you need to activate TCP in your ModbusConfig.h file.
 
- The HAL implementation for LWIP TCP of the CubeMX generates code that might not work if the cable is not connected from the very beginning.
This is a known issue that can be solved manually changing the generated code as detailed here: https://community.st.com/s/question/0D50X0000CDolzDSQR/ethernet-does-not-work-if-uc-starts-with-the-cable-disconnected

Check the TCP example for the NUCLEO F429, which includes the manual modifications. 

## How to port to your own MCU
- Create a new project in STM32Cube IDE for your MCU
- Enable FreeRTOS CMSIS_V2 in the middleware section of Cube-MX
- Configure a USART and activate the global interrupt
- If you are using the DMA mode for USART, configure the DMA requests for RX and TX
- Configure the `Preemption priority` of USART interrupt to a lower priority (5 or a higher number for a standard configuration) than your FreeRTOS scheduler. This parameter is changed in the NVIC configuration pane.
- Import the Modbus library folder (MODBUS-LIB) using drag-and-drop from your host operating system to your STM32Cube IDE project
- When asked, choose link folders and files
- Update the include paths in the project's properties to include the `Inc` folder of MODBUS-LIB folder
- Create a ModbusConfig.h using the ModbusConfigTemplate.h and add it to your project in your include path
- Instantiate a new global modbusHandler_t and follow the examples provided in the repository 
- `Note:` If your project uses the USART interrupt service for other purposes you have to modify the UARTCallback.c file accordingly


## Recommended Modbus Master and Slave testing tools for Linux and Windows

### Master and slave Python library

Linux/Windows: https://github.com/riptideio/pymodbus

### Master client Qmodbus
Linux:    https://launchpad.net/~js-reynaud/+archive/ubuntu/qmodbus

Windows:  https://sourceforge.net/projects/qmodbus/

### Slave simulator
Linux: https://sourceforge.net/projects/pymodslave/

Windows: https://sourceforge.net/projects/modrssim2/

## TODOs:
- Implement isolated memory spaces for coils, inputs and holding registers.
- Implement wrapper functions for Master function codes. Currently, telegrams are defined manually. 
- Improve function documentation
- ~~MODBUS TCP implementation improvement to support multiple clients and TCP session management~~ (10/24/2021)
- ~~Improve the queue for data reception; the current method is too heavy it should be replaced with a simple buffer, a stream, or another FreeRTOS primitive.~~ Solved Queue replaced by a Ring Buffer (03/19/2021)
- ~~Test with Rs485 transceivers (implemented but not tested)~~ Verified with MAX485 transceivers (01/03/2021)
- ~~MODBUS TCP implementation~~ (28/04/2021)
