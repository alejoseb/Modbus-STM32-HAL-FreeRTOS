# Sponsoring

If you found this library useful or if you have used it in a commercial product, please consider supporting my work 
or buying me a coffee: 

[:heart: sponsor](https://github.com/sponsors/alejoseb) 

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/alejoseb)

[!["Paypal"](https://github.com/alejoseb/Modbus-STM32-HAL-FreeRTOS/blob/master/.github/paypal.png)](https://www.paypal.com/donate/?business=ADTEDK5JHVZ22&no_recurring=0&item_name=I+have+contributed+libraries+for+embedded.+If+you+find+any+of+my+open-source+projects+useful+please+consider+supporting+me.%0A&currency_code=USD)[Paypal](https://www.paypal.com/donate/?business=ADTEDK5JHVZ22&no_recurring=0&item_name=I+have+contributed+libraries+for+embedded.+If+you+find+any+of+my+open-source+projects+useful+please+consider+supporting+me.%0A&currency_code=USD)

I also provide consultations at different sponsoring tiers, thanks!


# Modbus library for STM32 Microcontrollers
TCP, USART and USB-CDC Modbus RTU Master and Slave library for STM32 microcontrollers 
based on Cube HAL and FreeRTOS.

Includes multiple examples for popular development boards including BluePill, NUCLEO-64, 
NUCLEO-144 and Discovery Boards (Cortex-M3/M4/M7).

This is a port of the Modbus library for Arduino: https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino

Video demo for STM32F4-discovery board and TouchGFX: https://youtu.be/XDCQvu0LirY

`NEW` Separate memory regions per data type with configurable start addresses, while remaining fully backward-compatible with the legacy shared memory model

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
- Flexible memory model: shared memory space (legacy) or separate independent memory regions per data type with configurable Modbus start addresses.
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


## Slave Memory Configuration

The library supports two memory models for the slave. Both are configured in `main.c` (or equivalent application file).

### Option 1 — Shared memory (legacy, backward-compatible)

All function codes (FC1/2/3/4/5/6/15/16) share a single `uint16_t` array. Coil addresses map to bits of the array; register addresses map to words. This is the default mode and requires no changes to existing code.

```c
uint16_t ModbusDATA[8]; // shared by all function codes

ModbusH.uModbusType = MB_SLAVE;
ModbusH.port        = &huart2;
ModbusH.u8id        = 1;
ModbusH.u16timeOut  = 1000;
ModbusH.EN_Port     = NULL;
ModbusH.u16regs     = ModbusDATA;
ModbusH.u16regsize  = sizeof(ModbusDATA) / sizeof(ModbusDATA[0]);
ModbusH.xTypeHW     = USART_HW;
```

### Option 2 — Separate memory regions with configurable start addresses

Each data type has its own independent `uint16_t` array and its own Modbus start address. This enables standard Modbus address mapping (e.g. coils at 0, discrete inputs at 10001, holding registers at 40001) or any custom partitioning. When any of the region pointers is set, that function code uses the dedicated region instead of `u16regs`.

```c
// Independent arrays per data type
uint16_t CoilsDATA[2];     // 32 coils,  Modbus addresses   0 –  31
uint16_t DiDATA[2];        // 32 DI,     Modbus addresses 100 – 131
uint16_t HoldingDATA[4];   // 4 HR,      Modbus addresses 200 – 203
uint16_t InputDATA[4];     // 4 IR,      Modbus addresses 300 – 303

ModbusH.uModbusType = MB_SLAVE;
ModbusH.port        = &huart2;
ModbusH.u8id        = 1;
ModbusH.u16timeOut  = 1000;
ModbusH.EN_Port     = NULL;
ModbusH.u16regs     = NULL;  // not used when separate regions are configured
ModbusH.u16regsize  = 0;
ModbusH.xTypeHW     = USART_HW;

// Coils — FC1, FC5, FC15
ModbusH.u16coils            = CoilsDATA;
ModbusH.u16coilsStartAdd    = 0;
ModbusH.u16coilsNregs       = 2;

// Discrete Inputs — FC2
ModbusH.u16discreteInputs          = DiDATA;
ModbusH.u16discreteInputsStartAdd  = 100;
ModbusH.u16discreteInputsNregs     = 2;

// Holding Registers — FC3, FC6, FC16
ModbusH.u16holdingRegs          = HoldingDATA;
ModbusH.u16holdingRegsStartAdd  = 200;
ModbusH.u16holdingRegsNregs     = 4;

// Input Registers — FC4
ModbusH.u16inputRegs          = InputDATA;
ModbusH.u16inputRegsStartAdd  = 300;
ModbusH.u16inputRegsNregs     = 4;
```

With separate regions each block type responds only to its own address range and returns exception code 2 (Illegal Data Address) for any address outside it. Memory is fully isolated between blocks.

A fully working example using separate memory regions is available under `Examples/ModbusG431` (NUCLEO-G431KB).

## Recommended Modbus Master and Slave testing and development tools for Linux and Windows

### NEW MCP server exposing a Modbus Master for agents and AI assisted development
https://github.com/alejoseb/ModbusMCP


### Master and slave Python library

Linux/Windows: https://github.com/riptideio/pymodbus

### Master client Qmodbus
Linux:    https://launchpad.net/~js-reynaud/+archive/ubuntu/qmodbus

Windows:  https://sourceforge.net/projects/qmodbus/

### Slave simulator
Linux: https://sourceforge.net/projects/pymodslave/

Windows: https://sourceforge.net/projects/modrssim2/

## TODOs:
- ~~Implement isolated memory spaces for coils, inputs and holding registers.~~ Separate memory regions with configurable start addresses implemented and backward-compatible with legacy shared memory model.
- ~~Improve function documentation~~
- ~~MODBUS TCP implementation improvement to support multiple clients and TCP session management~~ (10/24/2021)
- ~~Improve the queue for data reception; the current method is too heavy it should be replaced with a simple buffer, a stream, or another FreeRTOS primitive.~~ Solved Queue replaced by a Ring Buffer (03/19/2021)
- ~~Test with Rs485 transceivers (implemented but not tested)~~ Verified with MAX485 transceivers (01/03/2021)
- ~~MODBUS TCP implementation~~ (28/04/2021)
- Refactor the Modbus.c File, it is more than 1200 lines of code and difficult to edit or reaosn about it. Also it is not following the best practices.
