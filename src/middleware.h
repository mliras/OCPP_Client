/*
 * middleware.h
 *
 *  Created on: Nov 21, 2017
 *      Author: root
 */

#ifndef MIDDLEWARE_H_
#define MIDDLEWARE_H_

#include <string.h>
#include <time.h>
#include "ocpp_client.h"
#include "ocpp_gtk.h"
#include "localAuthorization.h"
#include "chargingProfile.h"
#include "aux.h"
#include "ftpDiagnostics.h"

/////////////////////////////////
//   CONFIGURATION
/////////////////////////////////
struct configurationKey{
	char *keyValue;
	char *stringValue;
	int intValue;
};

#define CONFIGURATION_KEY_LIST_SIZE 75
struct configurationKey configurationValues[CONFIGURATION_KEY_LIST_SIZE];
static char *configurationKeys[CONFIGURATION_KEY_LIST_SIZE]={
		//9.1 CORE PROFILE
		"AllowOfflineTxForUnknownId","AuthorizationCacheEnabled","AuthorizeRemoteTxRequests","BlinkRepeat","ClockAlignedDataInterval",
		"ConnectionTimeOut","GetConfigurationMaxKeys","HeartbeatInterval","LightIntensity","LocalAuthorizeOffline","LocalPreAuthorize","MaxEnergyOnInvalidId",
		"MeterValuesAlignedData", "MeterValuesAlignedDataMaxLength", "MeterValuesSampledData", "MeterValuesSampledDataMaxLength", "MeterValueSampleInterval", "MinimumStatusDuration",
		"NumberOfConnectors","ResetRetries","ConnectorPhaseRotation","ConnectorPhaseRotationMaxLength","StopTransactionOnEVSideDisconnect","StopTransactionOnInvalidId",
		"StopTxnAlignedData","StopTxnAlignedDataMaxLength","StopTxnSampledData","StopTxnSampledDataMaxLength","SupportedFeatureProfiles","SupportedFeatureProfilesMaxLength",
		"TransactionMessageAttempts","TransactionMessageRetryInterval","UnlockConnectorOnEVSideDisconnect","WebSocketPingInterval",
		//9.2 LOCAL AUTH LIST MANAGEMENT PROFILE
		"LocalAuthListEnabled","LocalAuthListMaxLength","SendLocalListMaxLength",
		//9.3 RESERVATION PROFILE
		"ReserveConnectorZeroSupported",
		//9.4 SMART CHARGING PROFILE
		"ChargeProfileMaxStackLevel","ChargingScheduleAllowedChargingRateUnit","ChargingScheduleMaxPeriods","ConnectorSwitch3to1PhaseSupported","MaxChargingProfilesInstalled",
		//THIS IMPLEMENTATION SPECIFIC CONFIGURATION KEYS
		"ProtocolName", "CentralSystemIP", "CentralSystemPort", "CentralSystemURL",  "MaxDataTransferBytes", "ProtocolVersion","ChargePointModel","ChargePointVendor",
		"ChargePointSerialNumber","ChargeBoxSerialNumber", "FirmwareVersion", "ChargePointICCID", "ChargePointIMSI", "ChargePointMeterType", "ChargePointMeterSerialNumber",
		"ChargeBoxIP","ChargeBoxPort","ChargeBoxURL","ChargeBoxID","LocalAuthListFile", "LocalAuthorizationCacheFile","ChargePointVendorID", "PermanentAttachment",
		"vendorIdList","messageIdList","FTPUser","FTPPassword","LogFile","LogLevel",

		//NOT USED
		NULL,NULL,NULL
};

//If n%10 is 3 the key RW
//If n%10 is 2 the key is Read Only
//A value over 50 means that it's a boolean (int)
//A value over 10 means that it's a string
//A value over 20 means that it's a CSL
//A value over 1000 means that reboot is required on change
//A value over 100 means that the value is optional
static int configurationKeyCharacteristics[CONFIGURATION_KEY_LIST_SIZE]={
		//9.1
		153,153,53,103,3,
		3,2,3,103,53,53,103,
		23,102,23,102 ,3,103,
		2,3,23,102,53,53,
		23,102,23,102,22,102,
		3,3, 53,103,
		//9.2
		53,2,2,
		//9.3
		152,
		//9.4
		2,22,2,152,2,
		//SPECIFIC
		1013,1013,1013,1013,13,1013,12,12,12,12,12,12,12,12,12,12,12,12,12,1113,1113,12,102,
		123,123,113,113,12,3
		//NOT USED
		-1,-1,-1
};

#define DIAGNOSTICSFILE "/etc/ocpp/diagnostics.data"

/////////////////////////////////
//   CONNECTORS
/////////////////////////////////
struct connectorRecord{
	int connectorId;
	char *idTag;
};

//Esta a cero si no esta siendo usado, y a 1 si lo esta siendo
int connectorUsageRecord[NUM_CONNECTORS];

//Este es el numero real de conectores:
int num_connectors;
struct connectorRecord connectorAuthorizations[NUM_CONNECTORS];

/////////////////////////////////
//   RESERVATIONS
/////////////////////////////////

//En la estructura se almacena toda la información necesaria para identificar un gestionar una reserva
struct connectorReservation
{
	int connectorId;
	int reservationId;
	char *expiryDate;
	char *parentIdTag;
	char *idTag;
};

//Es posible que un conector permita o no reservas. Este array booleano gestiona dichos permisos.
int allowReservations[NUM_CONNECTORS];

//Los conectores pueden o no estar reservados, pero solo pueden estar reservados una vez porque solo habrá un Sitema central.
//El conector 1 es la posición 0.
struct connectorReservation *connectorReservations[NUM_CONNECTORS];

/////////////////////////////////
//   CHARGEPOINT STATUS
/////////////////////////////////
enum communication_status{
	OFFLINE,
	ONLINE
};

int communicationStatus;  //Ver pag 15
int chargePointStatus;

/////////////////////////////////
//   TRANSACTIONS
/////////////////////////////////
//
static int localTransactionId=900001;

//Una transaccion tiene que tener un charging profile asociado. Ya sea el de por defecto,
//el de por defecto del connector o uno especifico que nos mandan en la 5.11
struct transactionData{
	//connector debe tener el numero de conector (1 a n). Nunca deberia tener un cero
	int connector;
	char *startTime;
	char *idTag;
	int transactionId;
	int meterStart;
	struct ChargingProfile* chargingProfile;
};

struct transactionData *transactions[NUM_CONNECTORS];

struct pendingTransaction{
	int connector;
	int transactionId;
	struct transactionData *transaction;
	struct pendingTransaction *next;
};

struct pendingTransaction *pendingTransactions;

/////////////////////////////////
//   AUXILIAR FUNCTIONS
/////////////////////////////////
int isCPOffline();



/////////////////////////////////
//INITIALIZATION
/////////////////////////////////
void middleware_initialize();
queue_node * create_empty_queue_node(int messageType);
int handler22(void* user, const char* section, const char* name,const char* value);

/////////////////////////////////
//   CONNECTORS
/////////////////////////////////
//A 4.9 message should be sent for EVERY Status Change. This function is called when a status change occurs
//Connector debe ser ya la posicion del array (0..n). Status es un valor entero correspondiente al enumerado explicado en la seccion 7.7
void changeConnectorStatus(int connector, int status);

//getter del array connectorStatus para un conector dado
int getConnectorStatus(int connector);

//This is a debug function to understand which is the status value of every connector at every moment
void showConnectors();

//Nos devuelve si un conector se encuentra actualmente en una transaccion o no
//connector debe ser la posicion del array (valor 0..n-1)
int isConnectorFree(int connector);

//Esta funcion asigna un valor 1 a la posicion "connector" del array connectorUsageRecord
//connector debe ser la posicion del array (valor 0..n-1)
void useConnector(int connector);

//Esta funcion asigna un valor 1 a la posicion "connector" del array connectorUsageRecord
//connector debe ser la posicion del array (valor 0..n-1)
void freeConnector(int connector);

//Esta funcion es un getter() que devuelve el numero de conectores de un punto de carga
int get_num_connectors();

//Esta funcion Mantiene un array de autorizaciones a conectores.
//Cuando llega una mensaje de respuesta Authorize, actualiza este array
int assignConnectorAuthorization(const char *idTag);




//////////////////////////////////////////////////////////////////////////////////////////////////////
//RESERVATIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////
//Devuelve el valor del ID de la reserva realizada para un determinado connector
//O bien -1 si no hay reserva
//Si el numero de conector de entrada es cero, devuelve el reservationId de la primera reserva que encuentre
int getReservationId(int conn);

//Devuelve el connector que esta reservado (0 seria el conector 1) con un determinado reservationId
//Devuelve un valor de 0 a n-1
//Devuelve -1 si no encuentra la reserva
int check_reservationId(int reservationId);

//Actualiza el expiryDate, IdTag y ParentIdTag de una reserva que haya en el connector indicado
//Devuelve 0 si pudo actualizar con exito o -1 en caso de error
//Ninguno de los tres ultimos parámetros es obligatorio, pero al menos uno debe de ser no NULL o devolverá -1
//connectorId debe ser un valor de 0 a n-1
int update_reservationId(int connectorId, const char *expiryDate, const char *idTag, const char *parentIdTag);

//Esta funcion añade una reserva para un conector
//connectorId debe ser un valor de 0 a N
//Devuelve 1 si la reserva fue añadida con exito o 0 si NO fue añadida con exito
//El parentId es un campo opcional, el resto son obligatorios
//OJO: No se comprueba si el connector "Allows" reservations. Debe hacerse fuera de esta funcion.
int addReservation(int ConnectorId, char *expiryDate, char *idTag, int reservationId, const char *parentIdTag);

//Esta funcion elimina una reserva con el reservationId indicado. No se especifica el conector al que se refiere
//Devuelve 1 si pudo eliminar la reserva con exito o 0 en caso de que no pudiera.
int removeReservation(int reservationId);

//Funcion de debug que muestra por pantalla (mediante printf) el listado de reservas de un chargepoint
void showReservations();


//////////////////////////////////////////////////////////////////////////////////////////////////////
//CONFIGURATION
//////////////////////////////////////////////////////////////////////////////////////////////////////

//Esta funcion devuelve el tipo de una clave dada. Devuelve uno de los posibles siguientes valores:
//Returns 0 for INT, Returns 1 for BOOL, Returns 2 for STRING
//Returns 3 for CSL, Returns -1 for Error (not found or bad configured)
int getConfigurationKeyType(char *key);

//Esta funcion devuelve el tipo de una clave dada su posicion dentro del array. Devuelve uno de los posibles siguientes valores:
//Returns 0 for INT, Returns 1 for BOOL, Returns 2 for STRING
//Returns 3 for CSL, Returns -1 for Error (not found or bad configured)
int getConfigurationKeyType_i(int location);

//Esta funcion obtiene un valor de cadena a partir de la clave de configuracion indicada como parámetro
//Devuelve NULL en caso de error o de no encontrar la clave
//En caso contrario devuelve el valor de la clave pedida.
//No comprueba si la clave solicitada es de tipo cadena.
char* getConfigurationKeyStringValue(char *key);

//Esta funcion obtiene un valor entero a partir de la clave de configuracion indicada como parámetro
//Devuelve -1 en caso de error o de no encontrar la clave
//En caso contrario devuelve el valor de la clave pedida.
//No comprueba si la clave solicitada es de tipo numerico.
int getConfigurationKeyIntValue(char *key);

//Esta funcion devuelve la posicion de una clave dentro del array. Devuelve por tanto un valor entre 0 y 69.
//Devuelve -1 en caso de error o de no encontrar la clave
int getConfigurationKeyLocation(char *key);

//Esta funcion nos indica si una determinada clave es opcional o no.
//Como parametro no se pasa la clave si no la posicion dentro del array
//Devuelve 1 si es opcional, y 0 en caso contrario
int getConfigurationKeyIsOptional(int location);

//Esta funcion nos indica si una determinada clave es de tipo Solo Lectura o no.
//Como parametro no se pasa la clave si no la posicion dentro del array
//Devuelve 0 para RW, y 1 para RO
int getConfigurationKeyRequiresReboot(int location);

//Esta funcion nos permite modificar una clave asignandole un determinado valor (de cadena)
//La funcion comprueba si la clave es de tipo entero, booleano, cadena o CSL y hace la conversion que haga falta
//Devuelve 0 si se pudo modificar exitosamente, o -1 en caso de no encontrar la clave
int modifyConfigurationKey(char *key, char *value);

//Hay una serie de claves que son obligatorias. Esta funcion comprueba si todas esas claves obligatorias tiene valor.
//Esta funcion se llama en la fase de inicializacion del ChargePoint para asegurarnos de que todos los
//configuration Keys necesarios estan asignados. Devuelve -1 en caso de que todo OK y un valor positivo,
//indicando la posicion de la clave erronea dentro del array, en caso contrario.
int checkAllRequiredConfigurationKeys();

//Esta funcion nos indica si una determinada clave es de tipo Solo Lectura o no.
//Como parametro no se pasa la clave si no la posicion dentro del array
//Devuelve 0 para RW, y 1 para RO
int getConfigurationKeyIsReadOnly(int location);

//This function checks if the 'value' is present in the CSL list named 'key'
//Returns 1 if the Value is present in the key
//Returns 0 if the Value is NOT present in the key
int presentConfigurationCSLValue(char *key, char *value);

//Esta funcion nos indica si una determinada clave de tipo CSL (key) contiene un valor (value) o no
//devuelve 0 o 1. 0 en caso de que la clave aparezca devuelve 1, en caso contrario cero.
//No comprueba si la clave pasada es de tipo CSL
int containsCSL(char *key, char *value);



//////////////////////////////////////////////////////////////////////////////////////////////////////
//DATA TRANSFER
//////////////////////////////////////////////////////////////////////////////////////////////////////
int checkDataTransferMessageId(const char *MessageId);   //<-- NOT IMPLEMENTED
int manageDataTransfer(const char *vendorId, const char *MessageId, const char *Data);  //<-- NOT IMPLEMENTED



//////////////////////////////////////////////////////////////////////////////////////////////////////
//TRANSACTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//Esta funcion devuelve 1 si un determinado conector se encuentra actualmente en estado charging y 0 en caso contrario
int isCurrentlyUnderTransaction(int connector);

//Connector debe ser un valor de 0 a N-1
//returns -1 if no transaction found
int getTransactionId(int connector);

//Connector sera un valor de 0 a n-1
//returns -1 if no transaction found
int getConnectorFromTransaction(int transactionId);

//Añade la transaccion a la estructura de datos 'transactions'
//connectorId debe ser un valor de 1 a N
void addTransaction(int connectorId, const char *idTag, int meterStart, const char *timeStamp, const char *parentIdTag,
																		const char *expiryDate_str, int transactionId);

//Esta funcion elimina una transacción del array transactions. OJO, no pone el estado del conector a available
//connectorId debe ser un valor de 0 a n-1
int removeTransaction(int connectorId);

//Esta es una funcion de debug que muestra, mediante printf el estado de las diferentes transacciones
void showTransactions();

//Esta funcion devuelve el valor del ultimo transactionId y lo incrementa en uno.
int getNextTransactionId();

//Esta funcion crea un hilo que se encarga de tratar, durante un tiempo, la transaccion que se encuentra en estado pendiente.
void addPendingTransaction(char *idTag, int connectorId, json_object *obj_chargingProfile, int chargingProfileId, int stackLevel, int transactionId, char *purpose_str, char *kind_str, char *recurrency_str, char *valid_from, char *valid_to, json_object *obj_chargingSchedule);

//Esta funcion crea un hilo que se encarga de tratar, durante un tiempo, la transaccion que se encuentra en estado pendiente.
void removePendingTransaction(int connectorId, int transactionId);

//Esta funcion libera la memoria de una transaccion dada
void freeTransaction(struct transactionData *td);

//Esta funcion obtiene la transaccion de una lista de transacciones
struct transactionData *getPendingTransaction(int transactionId);

void startTransaction(int connector, const char *idTag);


//////////////////////////////////////////////////////////////////////////////////////////////////////
//AUTHORIZATIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////
int authorize(const char *idtag);

//ERROR MESSAGE
void sendErrorMessage(const char *uniqueid_str, int error_code);
/////////////////////////////////
//   OCPP
/////////////////////////////////

//4.1
void send_authorize_request(const char *idTagtext);
//4.2
void send_bootNotification_request(const char * vendor_data,const  char *chargepointmodel_data, const char *chargeBoxSerialNumber_data, const char *chargePointSerialNumber_data,
		const char *firmwareVersion_data, const char *ICCID_data, const char *IMSI_data, const char *meterSerialNumber_data, const char *meterType_data);
//4.3
void send_dataTransfer_request(const char *vendorId, const char *messageId, const char *data);
//4.4
void send_diagnosticsstatus_request(int status);
//4.5
void send_firmwareStatusNotification_request(int status_val);
//4.6
void send_heartbeat_request();
//4.7
void send_metervalues_request (int transactionId, int connectorId, int numMeterValues, int numSamples, struct SampledValue value_list[numMeterValues][numSamples]);

//4.8 START TRANSACTION
void send_starttransaction_request(int connectorId, int meterStartValue, const char *idTag, int reservationId);
// 4.9 STATUS NOTIFICATION
void send_statusnotification_request(const int connectorId, int newStatus, int errorCode, char *extraInfo, char *vendorId, char *vendorErrorCode);
// 4.10 STOP TRANSACTION
//void send_stoptransaction_request(int meterStopValue, int numSamples, int transactionId, int reason, char *idTag, struct MeterValue *transactionData);
//void send_stoptransaction_request(int meterStopValue, int numMeterValues, int numSamples, int transactionId, int reason, char *idTag, struct SampledValue value_list[numMeterValues][numSamples]);
void send_stoptransaction_request(int meterStopValue, int transactionId, int reason, char *idTag);
/////////////////////////////////////////////////////////////////////////////////////////
//5.1
char *manageCancelReservationRequest(const char *UniqueId_str, int reservationId);
//5.2
char *manageChangeAvailabilityRequest(const char *uniqueId_str, int connector, int avail);
//5.3
char *manageChangeConfigurationRequest(const char *UniqueId_str, const char *key, const char *value);
//5.4
char *manageClearCacheRequest(const char *UniqueId_str);
//5.5
char *manageClearChargingProfileRequest(const char *UniqueId_str, int chargingprofileid, int connectorid, const char *purpose_str, int stacklevel);
//5.6
char *manageCSDataTransferRequest(const char *uniqueId_str, const char *vendorId, const char *MessageId, const char *Data);
//5.8
char *manageGetConfigurationRequest(const char *uniqueId_str, json_object *obj_Payload);
//5.9
char *manageGetDiagnosticsRequest(const char *UniqueId_str, const char *location, int retries, int retryInterval, const  char *startTime, const char *stopTime);
//5.10
char *respondGetLocalListVersionRequest(const char *UniqueId_str);
//5.11
char *respondRemoteStartTransactionRequest(const char *UniqueId_str, const char *idTag, int connectorId, json_object *obj_chargingProfile, int chargingProfileId, int stackLevel, int transactionId, const char *purpose_str,const char * kind_str, const char *recurrency_str, const char *valid_from, const char *valid_to, json_object *obj_chargingSchedule);
//5.12
char *respondRemoteStopTransactionRequest(const char *UniqueId_str, int transactionId);
//5.13
char *manageReserveNowRequest(const char *UniqueId_str, int connectorId, const char *expirydate, const char *idtag, int reservationId, const char *parentIdTag);
//5.14
char *manageResetRequest(const char *UniqueId_str, const char *resetType_str);
//5.15
char *manageSendLocalListRequest(const char *UniqueId_str, int version, json_object *obj_localAuthorizationList, const char *updateType_str);
//5.16
char *respondSetChargingProfileRequest(const char *UniqueId_str, int connectorId, json_object *obj_chargingProfile, int chargingProfileId, int stackLevel, int transactionId, const char *purpose_str, const char * kind_str, const char *recurrency_str, const char *valid_from, const char *valid_to, json_object *obj_chargingSchedule);
//5.17
char *manageTriggerMessageRequest(const char *UniqueId_str, int connector, const char *MessageId_Str);
//5.18
char *manageUnlockConnectorRequest(const char *UniqueId_str, int connector);
//5.19
char *manageUpdateFirmwareRequest(const char *UniqueId_str, const char *location, const char *retrieveDate, int retries, int retryInterval);


#endif /* MIDDLEWARE_H_ */
