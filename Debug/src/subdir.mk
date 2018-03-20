################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ChargePoint.c \
../src/HardwareInterface.c \
../src/SQLiteConnection.c \
../src/aux.c \
../src/chargingProfile.c \
../src/client.c \
../src/example-client.c \
../src/exploreNFC.c \
../src/ftpDiagnostics.c \
../src/ini_parser.c \
../src/localAuthorization.c \
../src/middleware.c \
../src/ocpp_client.c \
../src/ocpp_gtk.c \
../src/ocpp_ini_parser.c 

O_SRCS += \
../src/HardwareInterface.o \
../src/aux.o \
../src/chargingProfile.o \
../src/example-client.o \
../src/ftpDiagnostics.o \
../src/localAuthorization.o \
../src/middleware.o \
../src/ocpp_client.o \
../src/ocpp_gtk.o \
../src/ocpp_ini_parser.o 

OBJS += \
./src/ChargePoint.o \
./src/HardwareInterface.o \
./src/SQLiteConnection.o \
./src/aux.o \
./src/chargingProfile.o \
./src/client.o \
./src/example-client.o \
./src/exploreNFC.o \
./src/ftpDiagnostics.o \
./src/ini_parser.o \
./src/localAuthorization.o \
./src/middleware.o \
./src/ocpp_client.o \
./src/ocpp_gtk.o \
./src/ocpp_ini_parser.o 

C_DEPS += \
./src/ChargePoint.d \
./src/HardwareInterface.d \
./src/SQLiteConnection.d \
./src/aux.d \
./src/chargingProfile.d \
./src/client.d \
./src/example-client.d \
./src/exploreNFC.d \
./src/ftpDiagnostics.d \
./src/ini_parser.d \
./src/localAuthorization.d \
./src/middleware.d \
./src/ocpp_client.d \
./src/ocpp_gtk.d \
./src/ocpp_ini_parser.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -Wno-unused-variable -o "$@" "$<"  `pkg-config --cflags --libs gtk+-3.0`
	@echo 'Finished building: $<'
	@echo ' '


