################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/PRU_bridgeClient.c \
../src/ntp.c \
../src/ntp2.c 

OBJS += \
./src/PRU_bridgeClient.o \
./src/ntp.o \
./src/ntp2.o 

C_DEPS += \
./src/PRU_bridgeClient.d \
./src/ntp.d \
./src/ntp2.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -O0 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


