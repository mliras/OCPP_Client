/*
 * HardwareInterface.c
 *
 *  Created on: Jan 25, 2018
 *      Author: root
 */
#include "HardwareInterface.h"
#include <time.h>

////////////////////////////
//      SHARED MEMORY
////////////////////////////

//Esta funcion crea e inicializa el segmento de memoria compartida.
void create_shared_memory()
{
     key_t key;
     struct shared_data *datos;
     dataFromRaspberry=(struct shared_data *)calloc(1, sizeof(struct shared_data));
     memset(dataFromRaspberry, 0, sizeof(struct shared_data));
     dataFromRaspberry->cable1locked=0;  //Realmente esto no hace falta, porque justo antes hemos hecho un memset
     dataFromRaspberry->cable2locked=0;  //Realmente esto no hace falta, porque justo antes hemos hecho un memset
     dataFromRaspberry->currentOffered=0.0;  //Realmente esto no hace falta, porque justo antes hemos hecho un memset

     key=shared_memory_key;

     //Creamos el segmento
     if ((shmid=shmget(key,sizeof(datos), IPC_CREAT| 0666)) < 0)
     {
    	 addLog("Error creating shared memory. Cannot read from device. Exiting", NULL,NULL, LOG_ERROR, ANY);
         exit(213);
     }
}

//Esta funcion lee la memoria que acabamos de crear y la asigna a la struct dataFromRaspberry.
void read_shared_memory()
{
        key_t key;
        char *shm, *s;

        key=shared_memory_key;

        if ((dataFromRaspberry=(struct shared_Data *)shmat(shmid,NULL,0))==(char *) -1)
        {
        		addLog("Error obtaining shared memory. Cannot read from device. Exiting", NULL,NULL, LOG_ERROR, ANY);
                exit (214);
        }

        dataFromRaspberry->cable1locked=0;
        dataFromRaspberry->cable2locked=0;
}

////////////////////////////////
// GPIO
////////////////////////////////
void initialize_GPIO()
{
	//Inicializamos el acceso al GPIO de la raspberry
	wiringPiSetup();
	wiringPiSetupGpio();

	pinMode(TRANSACTION1ALLOWED_PIN, OUTPUT);
	pinMode(TRANSACTION2ALLOWED_PIN, OUTPUT);
	pinMode(REBOOTING_PIN, OUTPUT);

	pinMode(CABLE1LOCKED_PIN, INPUT);
	pinMode(CABLE2LOCKED_PIN, INPUT);

	//Asi me pueden mandar hasta 8 codigos de error
	pinMode(ERROR1_PIN, INPUT);
	pinMode(ERROR2_PIN, INPUT);
	pinMode(ERROR3_PIN, INPUT);

	digitalWrite(REBOOTING_PIN_PH, LOW);
	///////////SI FINALMENTE USAMOS EL GPIO/////////////
}

//All these return 1 on error and 0 on OK
//INTERFACE WITH HARDWARE
int checkGroundFailure()
{
	return (dataFromRaspberry->error1==_GROUNDFAILURE);
}

int checkTemperatureTooHigh()
{
	return (dataFromRaspberry->error1==_TEMPERATURETOOHIGH);
}

int checkInternalError()
{
	return (dataFromRaspberry->error1==_INTERNALERROR);
}

int checkOverCurrent()
{
	return (dataFromRaspberry->error1==_OVERCURRENT);
}

int checkOverVoltage()
{
	return (dataFromRaspberry->error1==_OVERVOLTAGE);
}

//connectorId debe ser ya la posicion del array (0..n-1)
int  unlockCable(int connectorId)
{
//LEYENDO DE GPIO
//	if (connectorId==0) digitalWrite(CABLE1LOCKED_PIN_PH, LOW);
//	if (connectorId==1) digitalWrite(CABLE2LOCKED_PIN_PH, LOW);

//LEYENDO DE SHARED MEMORY
	if (connectorId==0) dataFromRaspberry->cable1locked=0;
	if (connectorId==1) dataFromRaspberry->cable2locked=0;

	return 0;
}

//connectorId debe ser ya la posicion del array (0..n-1)
int isCableLocked(int connector)
{
	//SI USAMOS GPIO
	//if (connectorId==0) return digitalRead(CABLE1LOCKED_PIN_PH);
	//if (connectorId==1) return digitalRead(CABLE2LOCKED_PIN_PH);

	//SI USAMOS MEMORIA COMPARTIDA
	if (connector==0) return dataFromRaspberry->cable1locked;
	if (connector==1) return dataFromRaspberry->cable2locked;
	return 0;
}

//connectorId debe ser ya la posicion del array (0..n-1)
int  lockCable(int connectorId)
{
	//LEYENDO DE GPIO
	//if (connectorId==0) digitalWrite(CABLE1LOCKED_PIN_PH, HIGH);
	//if (connectorId==1) digitalWrite(CABLE2LOCKED_PIN_PH, HIGH);

	//LEYENDO DE SHARED MEMORY
	if (connectorId==0) dataFromRaspberry->cable1locked=1;
	if (connectorId==1) dataFromRaspberry->cable2locked=1;


	//Pag 88: A connector is set to PREPARING when a user presents a Tag, inserts a cable, or a vehicle Occupies Parking Bay.
	changeConnectorStatus(connectorId, _CP_STATUS_PREPARING);

	return 0;
}

//Pag 87: Failure with IdTag reader
int checkRFIDError()
{
	//SI USAMOS GPIO
	//return digitalRead(ERROR1_PIN)==1 && digitalRead(ERROR2_PIN)==1 && digitalRead(ERROR3_PIN)==1;

	//SI USAMOS SHARED MEMORY
	return (dataFromRaspberry->error1==_RFIDERROR);
}

//Pag 87: Voltage has dropped below an acceptable level
int checkUnderVoltage()
{
	//SI USAMOS GPIO
	//return digitalRead(ERROR1_PIN)==1 && digitalRead(ERROR2_PIN)==1 && digitalRead(ERROR3_PIN)==0;

	//SI USAMOS SHARED MEMORY
	return (dataFromRaspberry->error1==_UNDERVOLTAGE);
}

//Pag 87: Wireless communication device reports a weak signal
int checkWeakSignal()
{
	//SI USAMOS GPIO(1-0-1)
	//return digitalRead(ERROR1_PIN)==1 && digitalRead(ERROR2_PIN)==0 && digitalRead(ERROR3_PIN)==1;

	//SI USAMOS SHARED MEMORY
	return (dataFromRaspberry->error1==_WEAKSIGNAL);
}

int checkEVCommunication()
{
	//SI USAMOS GPIO (1-0-0)
	//return digitalRead(ERROR1_PIN)==1 && digitalRead(ERROR2_PIN)==0 && digitalRead(ERROR3_PIN)==0;

	//SI USAMOS SHARED MEMORY
	return (dataFromRaspberry->error1==_EVCOMMUNICATIONERROR);
}

//getters from hardware
//double get_Current_Export(int connector)
double get_Current_Export(int connector)
{
	if (connector==1) return dataFromRaspberry->currentConnector1;
	if (connector==2) return dataFromRaspberry->currentConnector2;

	return 0.0;
}

//, Obtained from section 7.31 of specification
double get_Current_Import()
{
	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return 3.4;
}

//(Current.Offered  and Power.Offered are  strictly speaking  no  measured  values.  They  indicate  the  maximum  amount  of  current/power
//that is being offered to the EV and are intended for use in smart charging applications.
float get_Current_Offered()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("currentOffered");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return dataFromRaspberry->currentOffered;
}

//"Energy.Active.Export.Register",, Obtained from section 7.31 of specification
double get_Energy_Exported_by_EV()
{
	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In Wh or kWh
	return 9.1;
}

//"Energy.Active.Import.Register",, Obtained from section 7.31 of specification
double get_Energy_Imported_by_EV()
{
	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In Wh or kWh
	return 2.3;
}

//"Energy.Reactive.Export.Register",, Obtained from section 7.31 of specification
double get_Reactive_Energy_Exported_by_EV()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ReactiveEnergyExportedByEV");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In varh or kvarh
	return 4.5;
}

//"Energy.Reactive.Import.Register", Obtained from section 7.31 of specification
double get_Reactive_Energy_Imported_by_EV()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ReactiveEnergyImportedByEV");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In varh or kvarh
	return 6.7;
}

//"Energy.Active.Import.Interval",, Obtained from section 7.31 of specification
double get_Imported_Energy_Interval()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ImportedEnergyInterval");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In varh or kvarh
	return 8.9;
}

//"Energy.Active.Export.Interval",, Obtained from section 7.31 of specification
double get_Exported_Energy_Interval()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ExportedEnergyInterval");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In varh or kvarh
	return 0.2;
}

//"Energy.Reactive.Import.Interval",, Obtained from section 7.31 of specification
double get_Reactive_Imported_Energy_Interval()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ReactiveImportedEnergyInterval");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In varh or kvarh
	return 1.3;
}

//"Energy.Reactive.Export.Interval",, Obtained from section 7.31 of specification
double get_Reactive_Exported_Energy_Interval()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ReactiveExportedEnergyInterval");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In varh or kvarh
	return 2.4;
}

//"Frequency",, Obtained from section 7.31 of specification
double get_Powerline_Frequency()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("PowerlineFrequency");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	//In varh or kvarh
	return dataFromRaspberry->powerlineFrequency;
}

//"Power.Active.Export",, Obtained from section 7.31 of specification
double get_active_power_exported_by_EV()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ActivePowerExportedByEV");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return 4.6;
}

//"Power.Active.Import",, Obtained from section 7.31 of specification
double get_active_power_imported_by_EV()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("ActivePowerImportedByEV");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return 5.7;
}

// Obtained from section 7.31 of specification
double get_Power_Offered()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("PowerOffered");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return dataFromRaspberry->currentOffered;
}

//"Power.Factor",, Obtained from section 7.31 of specification
double get_Power_Factor()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("PowerFactor");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return dataFromRaspberry->powerFactor;
}

//"Power.Reactive.Export"., Obtained from section 7.31 of specification
double get_Power_Reactive_Export()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("PowerReactiveExport");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return 7.9;
}

//"Power.Reactive.Import",, Obtained from section 7.31 of specification
double get_Power_Reactive_Import()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("PowerReactiveImport");
	//if (valor) return atof(valor) else return 0.0;

	//NOT IMPLEMENTED: This should be read from the raspberry. By now I'll just return this value.
	return 8.0;
}

//"RPM",, Obtained from section 7.31 of specification
double get_Fan_Speed()
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("FanSpeed");
	//if (valor) return atof(valor) else return 0.0;

	//SHARED MEMORY
	return dataFromRaspberry->fanSpeed;
}

//devuelve el porcentaje de carga del vehuiculo. , Obtained from section 7.31 of specification
double get_State_Of_Charge(int connector)
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("StateOfCharge");
	//if (valor) return atof(valor) else return 0.0;

	//SHARED MEMORY
	if (connector==1) return dataFromRaspberry->stateOfCharge1;
	if (connector==2) return dataFromRaspberry->stateOfCharge2;
	//return 0.0;
}

//"Temperature", , Obtained from section 7.31 of specification
double get_Temperature()
{
	//Leemos la temperatura del Micro de la raspberry, que tampoco se si es lo que se deseaba aqui

	float systemp, millideg;
	FILE *temperaturefile;
	int n;

	temperaturefile=fopen("/sys/class/thermal/thermal_zone0/temp", "r");

	if (temperaturefile)
	{
			n=fscanf(temperaturefile, "%f", &millideg);
			fclose(temperaturefile);
			systemp=millideg/1000;
	}
	else
	{
			systemp=0.0;
	}

	return systemp;
}

//"Voltage", , Obtained from section 7.31 of specification
double get_Voltage(int connector)
{
	//SQLITE
	//char* valor=readChargePointValueFromSQLite("Voltage");
	//if (valor) return atof(valor) else return 0.0;

	//SHARED MEMORY
	if (connector==1) return dataFromRaspberry->voltageConnector1;
	if (connector==2) return dataFromRaspberry->voltageConnector2;
	return 0.0;
}

int isAnyButtonPressed()
{
	//GPIO
	//if (connectorId==0) digitalRead(BUTTON_PIN_PH);
	//if (connectorId==1) digitalRead(BUTTON_PIN_PH);

	if (dataFromRaspberry->boton1) return 1;
	if (dataFromRaspberry->boton2) return 2;
	return 0;
}

void LogIn(char *idtag, int conector)
{
	//show_authorization_list();
	//Miramos en las listas internas si aparece. Si no, consulta con el servidor central
	int status=authorize(idtag);

	if (isSQLiteConnected())
	{
		if (!status || status==-1)
		{
			//Informamos en BD. El cero es que el estado del login sea rejected
			insertSQLiteLogin(idtag,0, conector);


			//Le decimos a marcos que este usuario NO esta permitido para hacer transacciones.
			if (conector==1) dataFromRaspberry->transaction1allowed=0;
			if (conector==2) dataFromRaspberry->transaction2allowed=0;

			//Informamos en el LOG
			char *msg=(char *)calloc(1,64);
			sprintf(msg,"Log In Rejected for user %s.", idtag);
			addLog(msg, NULL, NULL, LOG_INFO, ANY);


			//Informamos por el display
			sendToDisplay(msg);
			free(msg);
		}
		else
		{
			//Pag 88: A connector is set to PREPARING when a user presents a Tag, inserts a cable, or a vehicle Occupies Parking Bay.
			//Mandamos un mensaje al servidor diciendole que se ha logado un usuario y que ponemos el estado del COnnector a preparing
			changeConnectorStatus(connectorId, _CP_STATUS_PREPARING);

			//Informamos en BD con status 1 (accepted).
			insertSQLiteLogin(idtag,1,conector);

			//Le decimos a marcos que este usuario SI esta permitido para hacer transacciones.
			if (conector==1) dataFromRaspberry->transaction1allowed=1;
			if (conector==2) dataFromRaspberry->transaction2allowed=1;

			//Logamos
			char *msg=(char *)calloc(1,64);
			sprintf(msg,"User %s Logged In Successfully", idtag);
			addLog(msg, NULL, NULL, LOG_INFO, ANY);

			//Escribimos en el display
			sendToDisplay(msg);
			free(msg);
		}
	}
}

void LogOut(char *idtag)
{
	int conn=isLoggedIn(idtag);

	updateLogoutIntoSQLite(idtag);

	//Si el usuario se loga y se desloga sin hacer nada... dejo el conector como estaba:
	if (getConnectorStatus(conn)==_CP_STATUS_PREPARING)	changeConnectorStatus(conn, _CP_STATUS_AVAILABLE);

	if (debug) addLog("Idtag %s on connector %d is Logging Out", idtag,convert(conn),LOG_INFO,ANY);

	char *msg=calloc(1,256);
	sprintf(msg, "User %s logged out", idtag);
	sendToDisplay(msg);
}

void checkLoop()
{
	//declaramos variables que usaremos
	int boton;
	int connector;
	int counter=0;

	//Fuera del bucle creamos y leemos la memoria compartida:
	create_shared_memory();
	read_shared_memory();
	lastIdTagRead=NULL;

	//Was 0 y was se usa unicamente para temas de logging
	int was0=isCableLocked(0);
	int was1=isCableLocked(1);

	while (1)
	{
		counter++;
		//Comprobamos si algun conector se ha descolgado
		for (connector=0;connector<NUM_CONNECTORS; connector++)
		{
			///////FOR LOGGING////////////////////
			if (connector==0 && was0!=isCableLocked(0))
			{
				if (isCableLocked(0)) addLog("Connector 1 WAS LOCKED, Starting Transaction...", NULL,NULL,LOG_INFO, ANY);
				else addLog("Connector 1 WAS UNLOCKED, Stopping transaction...", NULL,NULL,LOG_INFO, ANY);

				was0=isCableLocked(0);
			}

			if (connector==1 && was1!=isCableLocked(1))
			{
				if (isCableLocked(1)) addLog("Connector 2 WAS LOCKED, Starting transaction...", NULL,NULL,LOG_INFO, ANY);
				else addLog("Connector 2 WAS UNLOCKED, Stopping transaction...", NULL,NULL,LOG_INFO, ANY);

				was1=isCableLocked(1);
			}
			///////FOR LOGGING////////////////////


			//PASOS PARA INICIAR UNA TRANSACCION
			//1.- Logarse con el RFID
			//2.- Esperar señal de Marcos
			//3.- Iniciar la transaccion

			////////////////////////////////////
			//SI MARCOS NOS DEVUELVE QUE EL COCHE ESTA LISTO
			//
			int conectorUnderTransaction=isCurrentlyUnderTransaction(connector);

			if (!isCableLocked(connector))
			{
				//Si detecta que se ha desconectado el cable...
				if (conectorUnderTransaction)
				{
					//...y estamos inmersos en una transaccion...

					//...cortamos la energia (para que no haya problemas)
					DisablePermissions(connector);

					//Pag 46: If	StopTransactionOnEVSideDisconnect	  is  set  to	false,
					//the  transaction  SHALL  not  be  stopped  when  the	cable  is  disconnected  from  the  EV.
					if (getConfigurationKeyIntValue("StopTransactionOnEVSideDisconnect")!=0)
					{
						//Pag 45: When a charge point is configured with StopTransactionOnEVSideDisconnect set to true, a transaction is running and the EV becomes disconnected on EV Side, then
						// a StatusNotification.req with the state: 'Finishing"  should be sent to the CS, with the error code to 'NoError' and info field
						//with 'EV Side disconnected'. The current Transaction is stopped.
						char *idtag=strdup(transactions[connector]->idTag);

						if (get_State_Of_Charge(connector)>=100.0)
						{
							if (unavailabilityScheduled[connector])
							{
								//C8 on page 42
								changeConnectorStatusWithErroCode(connector, _CP_STATUS_UNAVAILABLE, _CP_ERROR_NOERROR, "Charging session ended. Unavailability scheduled");
								unavailabilityScheduled[connector]=0;
							}
							else
							{//Si se ha terminado la carga (C1 on page 42)
								changeConnectorStatusWithErroCode(connector, _CP_STATUS_AVAILABLE, _CP_ERROR_NOERROR, "Charging session ended");
							}
						}
						else
						{
							if (unavailabilityScheduled[connector])
							{
								//C8 on page 42
								changeConnectorStatusWithErroCode(connector, _CP_STATUS_UNAVAILABLE, _CP_ERROR_NOERROR, "EV requested transaction stop. Unavailability scheduled");
								unavailabilityScheduled[connector]=0;
							}
							else
							{
								//C4 on page 42
								changeConnectorStatusWithErroCode(connector, _CP_STATUS_SUSPENDEDEV, _CP_ERROR_NOERROR, "EV requested transaction stop");
							}
						}

						int stopTtransactionId=getTransactionId(connector);
						stopTransaction(connector);

						//Con esto nos aseguramos de que ahora no inicie una nueva transaccion:
						unlockCable(connector);

						send_stoptransaction_request(getCurrentMeterValueOfConnector(connector), stopTtransactionId, _ST_LOCAL,idtag);
					}
					else
					{
						//Pag 45: When a Charging Point is configured with StopTransactionOnEVSideDisconnect set to false, a transaction is running and the EV becomes disconnected on EV SIde,
						//then a StatusNotification.req with the state suspendedEV SHOULD be sent with the errorcode field set to NoError.
						//The Charge Point SHOULD add additional information in the info field notifying the Central System with the reason of Suspension: "EV side disconnected".
						//The current Transaction is NOT stopped <-- OJO!

						changeConnectorStatusWithErroCode(connector, _CP_STATUS_SUSPENDEDEV, _CP_ERROR_NOERROR, "EV side disconnect");
					}
				}
			}
			else
			{

				//isCableLocked es 1. O sea, marcos ha acabado con sus pruebas
				if (!conectorUnderTransaction)
				{
					char *idtag=whoDidLastLogInConnector(connector+1);

					if (idtag)
					{
						startTransaction(connector, idtag);
						send_starttransaction_request(connector+1, getCurrentMeterValueOfConnector(connector), idtag, -1);
					}
					else
					{
						addLog("Error while looking for idtag logged in", NULL,NULL,LOG_ERROR, ANY);
					}
				}
			}//else
		}//for

		///////////////////////////////////////////////
		//SI SE PULSA EL BOTON
		//

		//PARA PARAR UNA TRANSACCION
		//1.- Logarse con el RFID
		//2.- Parar la transaccion (pulsando boton en el HMI)
		//3.- Desconectar el cable

		boton=isAnyButtonPressed();
		if (boton)
		{
			if (isLoggedInConnector(boton))
			{
				int cnct=boton-1;
				if (isCurrentlyUnderTransaction(cnct))
				{
					char *idtag=strdup(transactions[cnct]->idTag);

					if (unavailabilityScheduled[cnct])
					{
						//C8 on page 42
						changeConnectorStatusWithErroCode(cnct, _CP_STATUS_UNAVAILABLE, _CP_ERROR_NOERROR, "User disconnect. Unavailability scheduled");
						unavailabilityScheduled[cnct]=0;
					}
					else
					{
						//C6 on page 42
						changeConnectorStatusWithErroCode(cnct, _CP_STATUS_FINISHING, _CP_ERROR_NOERROR, "User disconnect");
					}

					int stopTransactionId=getTransactionId(cnct);
//printf("\n\nTransactionID es: %d\n\n",stopTransactionId);
					stopTransaction(cnct);
					send_stoptransaction_request(getCurrentMeterValueOfConnector(cnct), stopTransactionId, _ST_LOCAL,idtag);
				}
			}

			if (boton==1) dataFromRaspberry->boton1=0;
			if (boton==1) dataFromRaspberry->cable1locked=0;
			if (boton==2) dataFromRaspberry->boton2=0;
			if (boton==2) dataFromRaspberry->cable2locked=0;
		}


		///////////////////////////////////////////////
		//SI SE PASA EL RFID
		//
		//Ahora comprobamos si alguien ha pasado un RFID
		//La lectura del IdTag la comprobamos cada 100 ms
		if (lastIdTagRead) //LastIdTagRead se lee en exploreNFC
		{
			if (isAnyBodyLoggedIn())
			{
				if (isLoggedIn(lastIdTagRead))
				{
					//Si ya se esta logado y se pasa la tarjeta se desloga
					LogOut(lastIdTagRead);
				}
				else
				{
					sendToDisplay("There's other user already logged in");
				}
			}
			else
			{
				//No hay nadie logado
				//HARDCODEAMOS PARA UN UNICO CONECTOR. SI HAY MAS DE UNO ESTO DEBERIA HACERSE DE OTRA FORMA
				//(Deberia haber un boton que nos permitiera escoger el conector)
				int con=1;
				int reservationId=getReservationId(con);
				//if (debug)	printf("ReservationId es: %d", reservationId);
				//Vamos a comprobar si esta reservado:
				if (reservationId>=0)
				{
					//Hay una reserva
					char *reservedIdTag=getReservationIdTag(con-1);

					if (strcmp(connectorReservations[con-1]->idTag, lastIdTagRead)==0)
					{
					//Comparamos las fechas de expiracion
						if (connectorReservations[con-1]->expiryDate)
						{
								//Obtenemos la fecha actual:
								time_t now;
								struct tm* now_tm;
								time(&now);
								now_tm = localtime(&now);

								//Le damos el formato adecuado al expirytime:
								struct tm* expiryTimeTM=(struct tm *)calloc(1, sizeof(struct tm));
								strptime(connectorReservations[con-1]->expiryDate, "%Y-%m-%dT%H:%M:%S.", expiryTimeTM);
								__time_t expirytime = mktime(expiryTimeTM);

								//Comparamos:
								double diffSecs = difftime(expirytime, now);

								free(expiryTimeTM);

								if (diffSecs>0.0)
								{
									//La fecha de expiracoin aun no ha vencido, por lo que la reserva esta activa
									//Hardcodeamos que el login se hace siempre en el conector 1. Esto habra que cambiarlo en el futuro
									LogIn(lastIdTagRead,1);

									removeReservation(reservationId);

								}
								else
								{
									//reservation expired
									removeReservation(reservationId);
								}
						}
						else
						{
							//No hay fecha de expiracion.
							LogIn(lastIdTagRead,1);

				//			removeReservation(reservationId);
						}
					}
					else
					{
				//		showReservations();
						//Esta reservado para otro usuario
						char *msg=(char *)calloc(1,256);
						sprintf(msg, "Connector %d reserved for user %s. Login rejected.", con, connectorReservations[con-1]->idTag);
						sendToDisplay(msg);
					}
				}
				else
				{

					if (isCurrentlyUnderTransaction(con) && (strcmp(transactions[con]->idTag,lastIdTagRead)!=0))
					{
						//Se trata de un usuario diferente
						sendToDisplay("cannot Log In. There's a transaction in course");
					}
					else
					{
						//Hardcodeamos que el login se hace siempre en el conector 1. Esto habra que cambiarlo en el futuro
						LogIn(lastIdTagRead,1);

					//	printf("RESERVATIONID ES %d", reservationId);
						//Actualizamos la reserva en el sqlite:
						if (reservationId>=0) updateReservationInSQLite(reservationId, con+1, getCurrentTime(), lastIdTagRead);
					}
				}
			}

			free(lastIdTagRead);
			lastIdTagRead=NULL;
		}

		///////////TIMEOUT///////////
		//Comprobamos que alguien lleve menos de 600 sg de TIMEOUT. La comprobacion se realiza cada 5 sg.
		if (counter%50==0 && isAnyBodyLoggedIn())
		{
			char *id=whoDidLastLoggedIn();
			checkLoginTimeout(id);
		}

		//////
		//TAMBIEN TENEMOS QUE ESTAR COMPROBANDO DE FORMA CONTINUA LOS POSIBLES ERRORES PARA INFORMAR AL SISTEMA CENTRAL
		//Dado que es memoria compartida los errores los podemos comprobar cada 100ms:

		//Si ha habido un fallo, informamos y paramos el chargepoint
		if (currentChargePointState==_CP_STATUS_FAULTED)
		{
			changeConnectorStatusWithErroCode(connector, _CP_STATUS_FAULTED, currentChargePointErrorStatus, NULL);

			//Paramos todas las transacciones...
			for (int i=0; i<NUM_CONNECTORS; i++)
			{
				if (isCurrentlyUnderTransaction(i))
				{
					char *idtag=whoDidLastLogInConnector(i);
					int stopTtransactionId=getTransactionId(i);
					stopTransaction(i);

					int errorParaelCS;
					if (currentChargePointErrorStatus==_CP_ERROR_UNDERVOLTAGE) errorParaelCS=_ST_POWERLOSS;
					else errorParaelCS=_ST_EMERGENCY_STOP;

					send_stoptransaction_request(getCurrentMeterValueOfConnector(i),stopTtransactionId , errorParaelCS,idtag);
				}
			}

			//...y salimos:
			exit(128);
		}

		//El bucle se realiza cada 100ms
		usleep(100000);
	}
}

void checkErrors()
{
	if (checkGroundFailure())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_GROUNDFAILURE;
	}

	if (checkTemperatureTooHigh())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_HIGHTEMPERATURE;
	}

	if (checkInternalError())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_INTERNALERROR;
	}

	if (checkOverCurrent())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_OVERCURRENTFAILURE;
	}

	if (checkOverVoltage())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_OVERVOLTAGE;
	}

	if (checkUnderVoltage())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_UNDERVOLTAGE;
	}

	if (checkRFIDError())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_READERFAILURE;
	}

	if (checkWeakSignal())
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_WEAKSIGNAL;
	}

}
//This is a low level function to supply Energy to a connector.
//Esta funcion es un interfaz hacia el dispositivo para que comience el proceso de  carga.
//conector es un valor de 0 a N-1
void EnablePermissions(int connector)
{
//	if (connectorId==0) digitalWrite(TRANSACTION1ALLOWED_PIN_PH, HIGH);
//	if (connectorId==1) digitalWrite(TRANSACTION1ALLOWED_PIN_PH, HIGH);

	if (connector==0) { dataFromRaspberry->transaction1allowed=1; dataFromRaspberry->stopTransaction1=0; }
	if (connector==1) { dataFromRaspberry->transaction2allowed=1; dataFromRaspberry->stopTransaction2=0; }
}

//connector es una valor de 0 a N-1
void DisablePermissions(int connector)
{
	//GPIO
	//if (connectorId==0) digitalWrite(TRANSACTION1ALLOWED_PIN_PH, LOW);
	//if (connectorId==1) digitalWrite(TRANSACTION1ALLOWED_PIN_PH, LOW);

	//SHARED MEMORY
	if (connector==0) dataFromRaspberry->transaction1allowed=0;
	if (connector==1) dataFromRaspberry->transaction2allowed=0;
}

void reboot()
{
	//GPIO
	//digitalWrite(REBOOTING_PIN_PH, HIGH);

	//SHARED MEMORY
	dataFromRaspberry->rebooting=1;
	system("sudo shutdown -h now");
}

void sendToDisplay(char *msg)
{
	system("clear");
	printf("\n##########MENSAJE DISPLAY###############");
	printf("\n%s", msg);
	printf("\n########################################");
	printf("\n");

}

/*
//No se esta usando
//connectorId debe ser ya la posicion del array (0..n-1)
void idTagReadFromRFID(int connectorId, char *idTag)
{
	if (connectorStatus[connectorId]== _CP_STATUS_AVAILABLE)
	{
		changeConnectorStatus(connectorId, _CP_STATUS_PREPARING);
		send_authorize_request(idTag);

		//Comprueba hasta 10 veces si el IdTag ha sido admitido
		for (int i=0; i<10; i++)
		{
			if (connectorAuthorizations[connectorId].idTag && strcmp(connectorAuthorizations[connectorId].idTag, idTag)==0)
			{
				sendToDisplay("IdTag autorizado");
				return;
			}
		}

		//En caso de que falle la autenticación, se manda al display
		sendToDisplay("No se pudo autenticar el idTag Solicitado");
	}
	else
	{
		sendToDisplay("Connector inoperativo");
	}
}
*/


//Se pide parar físicamente la transaccion. Esta es la parada "normal". Llama a la funcion que envía el mensaje OCPP.
//connectorId debe ser ya la posicion del array (0..n-1)
void stopTransaction(int connector)
{
	float curdel=transactions[connector]->currentDelivered;

	removeTransaction(connector);

	DisablePermissions(connector);

	if (connector==0) dataFromRaspberry->stopTransaction1=1;
	if (connector==1) dataFromRaspberry->stopTransaction2=1;

	//TODO ESTO ES DIFERENTE EN NUESTRA IMPLEMENTACION
	//Pag 46: As part of the normal transaction Termination, the chargepoint shall unlock the cable (if not permanently attached)
	if (!getConfigurationKeyIntValue("permanentAttachment"))
	{
		if (!unlockCable(connector))
		{
			//TODO ESTO ES DIFERENTE EN NUESTRA IMPLEMENTACION
		}
	}

	char *msg=calloc(1,256);
	sprintf(msg, "Transacción finalizada. Total energía enviada: %f", curdel );
	sendToDisplay(msg);

	//Other tasks that should be implemented when physically stopping a transaction
	// NOT IMPLEMENTED

	//updates GUI. Actualiza el combo con los conectores que estan bajo transaccion
	//updateStopTransactionConnectorCombo();
}
