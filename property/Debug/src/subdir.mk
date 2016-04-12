################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/sqlite3.c 

CPP_SRCS += \
../src/commandLine.cpp \
../src/parseDirective.cpp \
../src/property.cpp 

OBJS += \
./src/commandLine.o \
./src/parseDirective.o \
./src/property.o \
./src/sqlite3.o 

C_DEPS += \
./src/sqlite3.d 

CPP_DEPS += \
./src/commandLine.d \
./src/parseDirective.d \
./src/property.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -I"/home/pnema/Documents/workspace/cpp/property/include" -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I"/home/pnema/Documents/workspace/cpp/property/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


