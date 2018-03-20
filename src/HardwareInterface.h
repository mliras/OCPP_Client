/*
 * HardwareInterface.h
 *
 *  Created on: Jan 25, 2018
 *      Author: root
 */
#include <wiringPi.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "aux.h"
#include "client.h"
#include "ChargePoint.h"

#ifndef HARDWAREINTERFACE_H_
#define HARDWAREINTERFACE_H_

/////////////////////////////////////////////////
//         ERROR CODES
/////////////////////////////////////////////////
enum hardwareErrorCodes{
	_NONE,
	_GROUNDFAILURE,
	_TEMPERATURETOOHIGH,
	_INTERNALERROR,
	_OVERCURRENT,
	_OVERVOLTAGE,
	_UNDERVOLTAGE,
	_RFIDERROR,
	_WEAKSIGNAL,
	_EVCOMMUNICATIONERROR,
	_FANERROR,
};

/////////////////////////////////////////////////
//         SHARED MEMORY
/////////////////////////////////////////////////
struct shared_data{
        char flag;
    	int boton1;
    	int boton2;

        int transaction1allowed; //Flag que yo activo para indicarle a Marcos que hay permiso de carga
        int transaction2allowed; //idem con el conector 2

        int rebooting; //Flag que se activa para indicar que se va a proceder a reiniciar el dispositivo

        int cable1locked; //Flag que me activa Marcos para indicar que las negociaciones con el coche han llegado a buen puerto
        int cable2locked; //idem con el conector 2

        int statusConnector1; //No usado por ahora. Pretende almacenar el estado del conector
        int statusConnector2;

        int stopTransaction1; //Flag que se activa cuando se va a parar una carga (debe leerlo marcos para desconectar la corriente)
        int stopTransaction2;

        float voltageConnector1; //Valores informativos 1
        float voltageConnector2;

        float currentConnector1;
        float currentConnector2;
        int error1;
        int error2;
        int error3;
        float currentOffered;
        float powerlineFrequency;
        float powerFactor;
        float fanSpeed;
        float stateOfCharge1;
        float stateOfCharge2;
};

//S-I-C-A --> 19-9-3-1
static int shared_memory_key=19931;
int shmid;
struct shared_data *dataFromRaspberry;

void create_shared_memory();
void read_shared_memory();

/////////////////////////////////////////////////
//			RASPBERRY PINS
/////////////////////////////////////////////////
enum RaspberryPins{
	BUTTON_PIN=20,
	TRANSACTION1ALLOWED_PIN=21, //WiringPi 21 --> PIN 29 (GPIO 05). See learn.sparkfun.com/tutorials/raspberry-gpio
	TRANSACTION2ALLOWED_PIN=22, //WiringPi 22 --> PIN 31 (GPIO 06).  See learn.sparkfun.com/tutorials/raspberry-gpio

	REBOOTING_PIN=26, //WiringPi 26 --> PIN 32 (GPIO 12).  See learn.sparkfun.com/tutorials/raspberry-gpio

	CABLE1LOCKED_PIN=23, //WiringPi 23 --> PIN 33 (GPIO 13).  See learn.sparkfun.com/tutorials/raspberry-gpio
	CABLE2LOCKED_PIN=24, //WiringPi 24 --> PIN 35 (GPIO 19).  See learn.sparkfun.com/tutorials/raspberry-gpio

	//Asi me pueden mandar hasta 8 codigos de error
	ERROR1_PIN=25, //WiringPi 25 --> PIN 37 (GPIO 26).  See learn.sparkfun.com/tutorials/raspberry-gpio
	ERROR2_PIN=27, //WiringPi 27 --> PIN 36 (GPIO 16).  See learn.sparkfun.com/tutorials/raspberry-gpio
	ERROR3_PIN=28, //WiringPi 28 --> PIN 38 (GPIO 20).  See learn.sparkfun.com/tutorials/raspberry-gpio
};

enum RaspberryPhysicalPins{
	BUTTON_PIN_PH=28, //WiringPi 21 --> PIN 29. See learn.sparkfun.com/tutorials/raspberry-gpio
	TRANSACTION1ALLOWED_PIN_PH=29, //WiringPi 21 --> PIN 29. See learn.sparkfun.com/tutorials/raspberry-gpio
	TRANSACTION2ALLOWED_PIN_PH=31, //WiringPi 22 --> PIN 31.  See learn.sparkfun.com/tutorials/raspberry-gpio

	REBOOTING_PIN_PH=32, //WiringPi 26 --> PIN 32.  See learn.sparkfun.com/tutorials/raspberry-gpio

	CABLE1LOCKED_PIN_PH=33, //WiringPi 23 --> PIN 33.  See learn.sparkfun.com/tutorials/raspberry-gpio
	CABLE2LOCKED_PIN_PH=35, //WiringPi 24 --> PIN 35.  See learn.sparkfun.com/tutorials/raspberry-gpio

	//Asi me pueden mandar hasta 8 codigos de error
	ERROR1_PIN_PH=37, //WiringPi 25 --> PIN 37.  See learn.sparkfun.com/tutorials/raspberry-gpio
	ERROR2_PIN_PH=36, //WiringPi 27 --> PIN 36.  See learn.sparkfun.com/tutorials/raspberry-gpio
	ERROR3_PIN_PH=38, //WiringPi 28 --> PIN 38.  See learn.sparkfun.com/tutorials/raspberry-gpio
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NFC
char *lastIdTagRead;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INITIALIZATION

//Esta funcion inicializa el uso del interfaz GPIO de la raspberry
void initialize_GPIO();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ERROR CHECKS

//Las siguientes funciones comprueba distintos errores que me tiene que reportar el MRC
//Comprueban una serie de posibles errores del punto de carga. Todas ellas devuelve 1 en caso de error o 0 on OK
int checkGroundFailure();
int checkTemperatureTooHigh();
int checkInternalError();
int checkOverCurrent();
int checkOverVoltage();
int checkRFIDError();
int checkUnderVoltage();
int checkWeakSignal();
int checkEVCommunication();

//////////////////////////////
//   LOCK CABLE COMMUNICATION

//Desbloquea físicamente el conector. Aqui no hace nada. Simplemente llama a una funcion del sistema.
int unlockCable(int connectorId);

//bloquea físicamente el conector. Aqui no hace nada. Simplemente llama a una funcion del sistema.
int lockCable(int connectorId);

//This function returns 1 or 0 depending if the given connector is locked or not.
int isCableLocked(int connector);

////////////////////////////////////////////
//    BUTTON INTERFACE

//Devuelve 0 si no hay ningun boton o 1 o 2 si esta pulsado el boton 1 o 2
int isAnyButtonPressed();

////////////////////////////////////////////
//    ERROR CHECK

void checkErrors();

////////////////////////////////////////////
//     CHARGE PERMISSIONS

//Da permiso para suministrar carga a un conector
void EnablePermissions(int connector);

//Quita permiso para suministrar carga a un conector
void DisablePermissions(int connector);

//////////////////////////////////////////
//      DISPLAY
//Envía un mensaje al display
void sendToDisplay(char *msg);



//Envía al chargepoint la señal de reboot
void reboot();

//Llamado por el dispositivo cuando se lea algo en el RFID
//void idTagReadFromRFID(int connectorId, char *idTag);

//////////////MEASURANDS/////////////////
//getter from measurands
double get_Current_Export(int connector);
double get_Current_Import();
float get_Current_Offered();
double get_Power_Offered();
double get_Energy_Exported_by_EV();
double get_Energy_Imported_by_EV();
double get_Reactive_Energy_Exported_by_EV();
double get_Reactive_Energy_Imported_by_EV();
double get_Imported_Energy_Interval();
double get_Exported_Energy_Interval();
double get_Reactive_Imported_Energy_Interval();
double get_Reactive_Exported_Energy_Interval();
double get_Powerline_Frequency();
double get_active_power_exported_by_EV();
double get_Power_Factor();
double get_Power_Reactive_Export();
double get_Power_Reactive_Import();
double get_Fan_Speed();
double get_State_Of_Charge(int connector);
double get_Temperature();
double get_Voltage(int connector);

/////////////////////////////
//   LOG IN/OUT PROCESS

void LogOut(char *idtag);
void LogIn(char *idtag, int conector);

/////////////////////////////////
//   STOP TRANSACTION
//Esta funcion es llamada por el dispositivo cuando se pide parar físicamente la transaccion, se suele pedir desde la funcion que manda el mensaje.
//Esta es la parada "normal".
void stopTransaction(int connector);

//////////////////////////////
// MAIN LOOP
void checkLoop();

#endif /* HARDWAREINTERFACE_H_ */
