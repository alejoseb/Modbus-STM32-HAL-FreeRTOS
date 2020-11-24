################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/ModbusSTM32F4-discovery/ModbusSTM32F4-Discovery/Drivers/BSP/Components/ili9341/ili9341.c \
C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/ModbusSTM32F4-discovery/ModbusSTM32F4-Discovery/Drivers/BSP/Components/stmpe811/stmpe811.c 

C_DEPS += \
./Drivers/BSP/Components/ili9341.d \
./Drivers/BSP/Components/stmpe811.d 

OBJS += \
./Drivers/BSP/Components/ili9341.o \
./Drivers/BSP/Components/stmpe811.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/ili9341.o: C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/ModbusSTM32F4-discovery/ModbusSTM32F4-Discovery/Drivers/BSP/Components/ili9341/ili9341.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../../Core/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/BSP -I../../TouchGFX/target -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../TouchGFX/App -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../TouchGFX/target/generated -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Middlewares/ST/touchgfx/framework/include -I../../TouchGFX/generated/fonts/include -I../../TouchGFX/generated/gui_generated/include -I../../TouchGFX/generated/images/include -I../../TouchGFX/generated/texts/include -I../../TouchGFX/gui/include -I"C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/BSP/Components/ili9341.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/Components/stmpe811.o: C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/ModbusSTM32F4-discovery/ModbusSTM32F4-Discovery/Drivers/BSP/Components/stmpe811/stmpe811.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../../Core/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/BSP -I../../TouchGFX/target -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../TouchGFX/App -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../TouchGFX/target/generated -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Middlewares/ST/touchgfx/framework/include -I../../TouchGFX/generated/fonts/include -I../../TouchGFX/generated/gui_generated/include -I../../TouchGFX/generated/images/include -I../../TouchGFX/generated/texts/include -I../../TouchGFX/gui/include -I"C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/BSP/Components/stmpe811.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

