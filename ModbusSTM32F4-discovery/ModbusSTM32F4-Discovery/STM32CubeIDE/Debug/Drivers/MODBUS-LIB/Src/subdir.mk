################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/Modbus.c \
C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/UARTCallback.c 

C_DEPS += \
./Drivers/MODBUS-LIB/Src/Modbus.d \
./Drivers/MODBUS-LIB/Src/UARTCallback.d 

OBJS += \
./Drivers/MODBUS-LIB/Src/Modbus.o \
./Drivers/MODBUS-LIB/Src/UARTCallback.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/MODBUS-LIB/Src/Modbus.o: C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/Modbus.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../../Core/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/BSP -I../../TouchGFX/target -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../TouchGFX/App -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../TouchGFX/target/generated -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Middlewares/ST/touchgfx/framework/include -I../../TouchGFX/generated/fonts/include -I../../TouchGFX/generated/gui_generated/include -I../../TouchGFX/generated/images/include -I../../TouchGFX/generated/texts/include -I../../TouchGFX/gui/include -I"C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/MODBUS-LIB/Src/Modbus.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/MODBUS-LIB/Src/UARTCallback.o: C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/UARTCallback.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../../Core/Inc -I../../Drivers/CMSIS/Include -I../../Drivers/BSP -I../../TouchGFX/target -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../TouchGFX/App -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../TouchGFX/target/generated -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Middlewares/ST/touchgfx/framework/include -I../../TouchGFX/generated/fonts/include -I../../TouchGFX/generated/gui_generated/include -I../../TouchGFX/generated/images/include -I../../TouchGFX/generated/texts/include -I../../TouchGFX/gui/include -I"C:/Users/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/MODBUS-LIB/Src/UARTCallback.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

