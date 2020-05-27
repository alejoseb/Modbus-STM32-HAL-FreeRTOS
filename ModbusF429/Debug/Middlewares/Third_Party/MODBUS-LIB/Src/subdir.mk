################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/alejandro/Documents/STM32projects/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/Modbus.c \
/home/alejandro/Documents/STM32projects/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/UARTCallback.c 

OBJS += \
./Middlewares/Third_Party/MODBUS-LIB/Src/Modbus.o \
./Middlewares/Third_Party/MODBUS-LIB/Src/UARTCallback.o 

C_DEPS += \
./Middlewares/Third_Party/MODBUS-LIB/Src/Modbus.d \
./Middlewares/Third_Party/MODBUS-LIB/Src/UARTCallback.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/MODBUS-LIB/Src/Modbus.o: /home/alejandro/Documents/STM32projects/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/Modbus.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I"/home/alejandro/Documents/STM32projects/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/Third_Party/MODBUS-LIB/Src/Modbus.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/Third_Party/MODBUS-LIB/Src/UARTCallback.o: /home/alejandro/Documents/STM32projects/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Src/UARTCallback.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -c -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I"/home/alejandro/Documents/STM32projects/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Middlewares/Third_Party/MODBUS-LIB/Src/UARTCallback.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

