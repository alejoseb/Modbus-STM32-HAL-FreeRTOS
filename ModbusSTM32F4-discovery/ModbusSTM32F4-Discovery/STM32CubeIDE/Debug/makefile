################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/FreeRTOS/subdir.mk
-include Drivers/STM32F4xx_HAL_Driver/subdir.mk
-include Drivers/MODBUS-LIB/Src/subdir.mk
-include Drivers/CMSIS/subdir.mk
-include Drivers/BSP/Components/subdir.mk
-include Application/User/gui/subdir.mk
-include Application/User/generated/subdir.mk
-include Application/User/TouchGFX/target/generated/subdir.mk
-include Application/User/TouchGFX/target/subdir.mk
-include Application/User/TouchGFX/App/subdir.mk
-include Application/User/subdir.mk
-include Application/Startup/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
STM32F429I-DISCO.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
STM32F429I-DISCO.list \

OBJCOPY_HEX += \
STM32F429I-DISCO.hex \

OBJCOPY_BIN += \
STM32F429I-DISCO.bin \


# All Target
all: STM32F429I-DISCO.elf secondary-outputs

# Tool invocations
STM32F429I-DISCO.elf: $(OBJS) $(USER_OBJS) /home/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/ModbusSTM32F4-discovery/ModbusSTM32F4-Discovery/STM32CubeIDE/STM32F429ZITX_FLASH.ld
	arm-none-eabi-g++ -o "STM32F429I-DISCO.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/home/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/ModbusSTM32F4-discovery/ModbusSTM32F4-Discovery/STM32CubeIDE/STM32F429ZITX_FLASH.ld" --specs=nosys.specs -Wl,-Map="STM32F429I-DISCO.map" -Wl,--gc-sections -static -L/home/alejandro/Documents/Modbus-STM32-HAL-FreeRTOS/ModbusSTM32F4-discovery/ModbusSTM32F4-Discovery/Middlewares/ST/touchgfx/lib/core/cortex_m4f/gcc --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

STM32F429I-DISCO.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "STM32F429I-DISCO.list"
	@echo 'Finished building: $@'
	@echo ' '

STM32F429I-DISCO.hex: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "STM32F429I-DISCO.hex"
	@echo 'Finished building: $@'
	@echo ' '

STM32F429I-DISCO.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "STM32F429I-DISCO.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
