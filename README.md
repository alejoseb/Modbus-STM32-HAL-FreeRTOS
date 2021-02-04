# Modbus Library for STM32 Microcontrollers

Owner: Brendan Chang - [@BrendanSChang](https://github.com/BrendanSChang)

StandardBots fork of
[alejoseb/Modbus-STM32-HAL-FreeRTOS](https://github.com/alejoseb/Modbus-STM32-HAL-FreeRTOS).
This is used to provide Modbus RTU support for STM32 microcontrollers. The
library in `MODBUS-LIB/` has been modified as follows:

- Added C++ compatibility.
- `mHandlers` has been made extern and defined in `Modbus.c` to avoid errors
  related to the One Definition Rule.
- UART interrupt handler logic has been moved into
  `ModbusNotifyTransmitComplete()`.

This repository is intended to be included as a submodule.

## Getting Started

1. Add `firmware-modbus` as a submodule.
   ```
   git submodule add git@github.com:standardbots/firmware-modbus <path>
   ```
2. Add the `firmware-modbus` path to the include path in
   "Properties -> C/C++ General -> Paths and Symbols -> Includes".
3. Disable the examples using a source filter in
   "Properties -> C/C++ General -> Paths and Symbols -> Source Location".
   - `<path>/ModbusBluePill/`
   - `<path>/ModbusF103/`
   - `<path>/ModbusF303/`
   - `<path>/ModbusF429/`
   - `<path>/ModbusH743/`
   - `<path>/ModbusSTM32F4-discovery/`
     > TODO(brendan): Consider removing the examples.
4. Override `HAL_UART_TxCpltCallback`:
    ```c
    void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
      ModbusNotifyTransmitComplete();
    }
    ```
