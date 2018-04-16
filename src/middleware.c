/*
 * middleware.c
 *
 *  Created on: Nov 21, 2017
 *      Author: root
 */

#include <stdio.h>
#include <string.h>
#include "middleware.h"

/////////////////////////////////
//
// Auxiliary functions
//
int isCPOffline()
{
	if (communicationStatus==ONLINE)
	{
		return 0;
	}

	return 1;
}

//////////////////////////////////////////////
//      CONFIGURATION KEYS
//////////////////////////////////////////////
//
//Esta funcion devuelve el tipo de una clave dada. Devuelve uno de los posibles siguientes valores:
//Returns 0 for INT
//Returns 1 for BOOL
//Returns 2 for STRING
//Returns 3 for CSL
//Returns -1 for Error (not found or bad configured)
int getConfigurationKeyType(char *key)
{
	if (!key) return -1;
	int i =0;
	while (configurationKeys[i])
	{
		if (strcmp(configurationKeys[i], key)==0)
		{
			if (configurationKeyCharacteristics[i]%100<0) return -1;
			if (configurationKeyCharacteristics[i]%100<10) return 1; //INT
			if (configurationKeyCharacteristics[i]%100<20) return 2; //STRING
			if (configurationKeyCharacteristics[i]%100<30) return 3; //CSL
			if (configurationKeyCharacteristics[i]%100>30) return 0; //BOOL
		}
		i++;
	}

	return -1;
}


//If n%10 is 3 the key RW
//If n%10 is 2 the key is Read Only
//A value over 50 means that it's a boolean (int)
//A value over 10 means that it's a string
//A value over 20 means that it's a CSL
//A value over 1000 means that reboot is required on change
//A value over 100 means that the value is optional
//Esta funcion devuelve el tipo de una clave dada su posicion dentro del array. Devuelve uno de los posibles siguientes valores:
//Returns 0 for INT
//Returns 1 for BOOL
//Returns 2 for STRING
//Returns 3 for CSL
//Returns -1 for Error (not found or bad configured)
int getConfigurationKeyType_i(int location)
{
	if (location<0) return -1;
	if (configurationKeyCharacteristics[location]%100<0) return -1;
	if (configurationKeyCharacteristics[location]%100<10) return 1; //INT
	if (configurationKeyCharacteristics[location]%100<20) return 2; //STRING
	if (configurationKeyCharacteristics[location]%100<30) return 3; //CSL
	if (configurationKeyCharacteristics[location]%100>30) return 0; //BOOL

	return -1;
}

//Esta funcion devuelve la posicion de una clave dentro del array. Devuelve por tanto un valor entre 0 y 69.
//Devuelve -1 en caso de error o de no encontrar la clave
int getConfigurationKeyLocation(char *key)
{
	if (!key) return -1;

	int i =0;
	while (configurationKeys[i])
	{
		if (strcmp(configurationKeys[i], key)==0) return i;

		i++;
	}

	return -1;
}

//Esta funcion obtiene un valor de cadena a partir de la clave de configuracion indicada como parámetro
//Devuelve NULL en caso de error o de no encontrar la clave
//En caso contrario devuelve el valor de la clave pedida.
//No comprueba si la clave solicitada es de tipo cadena.
char* getConfigurationKeyStringValue(char *key)
{
	if (!key) return NULL;
	int i=0;

	while (configurationKeys[i])
	{

		if (strcmp(configurationKeys[i], key)==0)
		{
			return configurationValues[i].stringValue;
		}
		i++;
	}

	return NULL;
}

//Esta funcion obtiene un valor entero a partir de la clave de configuracion indicada como parámetro
//Devuelve -1 en caso de error o de no encontrar la clave
//En caso contrario devuelve el valor de la clave pedida.
//No comprueba si la clave solicitada es de tipo numerico.
int getConfigurationKeyIntValue(char *key)
{
	if (!key) return -1;

	int i=0;
	while (configurationKeys[i])
	{
		if (strcmp(configurationKeys[i], key)==0)
		{
			return configurationValues[i].intValue;
		}
		i++;
	}

	return -1;
}

//Esta funcion nos indica si una determinada clave de tipo CSL (key) contiene un valor (value) o no
//devuelve 0 o 1. 0 en caso de que la clave aparezca devuelve 1, en caso contrario cero.
//No comprueba si la clave pasada es de tipo CSL
int containsCSL(char *key, char *value)
{
	if ((!key) || (!value)) return 0;

	if(strstr(key, value) != NULL) {
		    //manageMessageId(messageId); <-- Not implemented
		    return 1;
	}
	return 0;
}

//This function checks if the 'value' is present in the CSL list named 'key'
//Returns 1 if the Value is present in the key
//Returns 0 if the Value is NOT present in the key
int presentConfigurationCSLValue(char *key, char *value)
{
	if ((!key) || (!value)) return 0;

	if(strstr(getConfigurationKeyStringValue(key), value) != NULL) {
	    return 1;
	}

	return 0;
}


int getCSLNumElements(char *key)
{
	//Si no es valida la clave devuelve menos uno.
	if (!key || getConfigurationKeyType(key)==-1) return -1;

	//Si no, cuenta el numero de comas y devuelve una mas.
	return charCounter(getConfigurationKeyStringValue(key) , ',')+1;
}

//Esta funcion nos indica si una determinada clave es de tipo Solo Lectura o no.
//Como parametro no se pasa la clave si no la posicion dentro del array
//Devuelve 0 para RW, y 1 para RO
int getConfigurationKeyIsReadOnly(int location)
{
	if (location<0) return 0;
	return (configurationKeyCharacteristics[location]%10==2);
}

//Esta funcion nos indica si una determinada clave es opcional o no.
//Como parametro no se pasa la clave si no la posicion dentro del array
//Devuelve 1 si es opcional, y 0 en caso contrario
int getConfigurationKeyIsOptional(int location)
{
	if (location<0) return 0;
	return (configurationKeyCharacteristics[location]%1000>100);
}

//Esta funcion nos indica si una determinada clave es de tipo Solo Lectura o no.
//Como parametro no se pasa la clave si no la posicion dentro del array
//Devuelve 0 para RW, y 1 para RO
int getConfigurationKeyRequiresReboot(int location)
{
	if (location<0) return 0;
	return (configurationKeyCharacteristics[location]/1000>0);
}

//Esta funcion nos permite modificar una clave asignandole un determinado valor (de cadena)
//La funcion comprueba si la clave es de tipo entero, booleano, cadena o CSL y hace la conversion que haga falta
//Devuelve 0 si se pudo modificar exitosamente, o -1 en caso de no encontrar la clave
int modifyConfigurationKey(char *key, char *value)
{
	int location=0;
	while (configurationKeys[location])
	{
		if (strcmp(configurationKeys[location], key)==0)
		{
			if (configurationKeyCharacteristics[location]>0)
			{
				if (getConfigurationKeyType_i(location)==1)
			    {
			    	//INTEGER
			    	configurationValues[location].intValue=atoi(value);
			    	return 0;
			    }

			    if (getConfigurationKeyType_i(location)==0)
			    {
			    	//BOOLEAN
			    	int tmp=atoi(value);
			    	if ((tmp==0) || (tmp==1))
			    	{
			    		configurationValues[location].intValue=tmp;
			    		return 0;
			    	}
			    }

			    if (getConfigurationKeyType_i(location)==2)
			    {
			    	//STRING
			    	configurationValues[location].stringValue=strdup(value);

			    	return 0;
	    		}

	    		if (getConfigurationKeyType_i(location)==3)
	    		{
	    		    //CSL
	    			configurationValues[location].stringValue=strdup(value);
	    		    return 0;
			    }
			}
		}
		location++;
	}

	//No se encontro la clave
	return -1;
}

//Hay una serie de claves que son obligatorias. Esta funcion comprueba si todas esas claves obligatorias tiene valor.
//Esta funcion se llama en la fase de inicializacion del ChargePoint para asegurarnos de que todos los
//configuration Keys necesarios estan asignados
//Devuelve -1 en caso de que todo OK y un valor positivo, indicando la posicion de la clave erronea dentro del array, en caso contrario.
int checkAllRequiredConfigurationKeys()
{
	int i=0;
	int keyType;
	while (configurationKeys[i])
	{
		if (((configurationKeyCharacteristics[i]%1000)/100)==0)
		{
			keyType=getConfigurationKeyType_i(i);

			//printf("\nSe comrpueba la clave %d", i);
			//printf("\nEl KeyType es %d", keyType);
			if (keyType==0 || keyType==1)
			{
		//		printf("\nEl valor es %d", configurationValues[i].intValue);
				if (configurationValues[i].intValue==-1) return i;
			}

			if (keyType==2 || keyType==3)
			{
				if (configurationValues[i].stringValue==NULL) return i;
			}
		}
		i++;
	}
//	printf("BBBB");
	//Everything OK
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////
//FIRMWARE
////////////////////////////////////////////////////////////////////////////////////
void firmwareUpdate(void *parameters)
{
	lastFirmwareUploadStatus=_FS_DOWNLOADING;
	if (!getFirmwareFile(parameters))
	{
		lastFirmwareUploadStatus=_FS_DOWNLOADFAILED;
	}
	else
	{
		lastFirmwareUploadStatus=_FS_DOWNLOADED;

		lastFirmwareUploadStatus=_FS_INSTALLING;
		if (!installFirmware()) //<--NOT IMPLEMENTED
		{
			lastFirmwareUploadStatus=_FS_INSTALLATIONFAILED;
		}
		else
		{
			lastFirmwareUploadStatus=_FS_INSTALLED;
			//reboot(); //<-- NOT IMPLEMENTED
		}
	}
}

int installFirmware()
{
	return 1; //<-- NOT IMPLEMENTED
}

////////////////////////////////////////////////////////////////////////////////////
//DATA TRANSFERS
////////////////////////////////////////////////////////////////////////////////////
int checkDataTransferMessageId(const char *MessageId)
{
	return 1;   //<-- NOT IMPLEMENTED
}

int manageDataTransfer(const char *vendorId, const char *MessageId, const char *Data)
{
	return _DT_ACCEPTED;
}


////////////////////////////////////////////////////////////////////////////////////
//AUTHORIZATIONS
////////////////////////////////////////////////////////////////////////////////////
int authorize(const char *idtag)
{
//Pag 15: To  improve  the  experience  for  users,  a  Charge  Point  MAY  support  local  authorization  of  identifiers,	using an
//	Authorization Cache	 and/or a Local Authorization List. This  allows  (a)  authorization  of  a  user  when	offline,  and  (b)  faster  (apparent)  authorization  response
//	time when communication between Charge Point and Central System is slow.

// The	LocalAuthorizeOffline  configuration  key  controls  whether  a  Charge  Point  will  authorize
	//	a  user	when offline using the Authorization Cache and/or the Local Authorization List.
	printf("\nAA");
	if (isCPOffline())
	{
		printf("\nBB");
		if (getConfigurationKeyIntValue("LocalAuthorizeOffline"))
		{
			printf("\nCC");
			//Primero lo busca en la lista, luego en la cache y por ultimo manda un mensaje
			if (isAuthorizationListEnabled())
			{
				printf("\nDD");
				if (getAuthorizationListEntryStatus(idtag)>=0)
				{
					printf("\nEE");
					//addSendAuthorizeRequestWhenOnline()  <-- NOT IMPLEMENTED: Cuando se restaure la conexion, hay que mandar un sendAuthorize
					return checkValidAuthorizationListEntry(idtag);
				}
				else
				{
					//Si no lo encuentra lo intentamos con la cache
				}
			}

			if (isAuthorizationCacheEnabled())
			{
				printf("\nFF");
				if (getAuthorizationCacheEntryStatus(idtag)>=0)
				{
					printf("\nGG");
					//addSendAuthorizeRequestWhenOnline()  <-- NOT IMPLEMENTED: Cuando se restaure la conexion, hay que mandar un sendAuthorize
					return checkValidAuthorizationCacheEntry(idtag);
				}
				else
				{
					//No esta en la lista y estamos offline. Rechazado
					return 0;
				}
			}
		}
		else return _CP_AUTHORIZATION_INVALID;
	}
	else
	{
		//El chargepoint NO esta offline
		if (getConfigurationKeyIntValue("LocalPreAuthorize"))
		{
			printf("\nHH");
			show_authorization_list();
			show_authorization_cache();

			//Primero lo busca en la lista, luego en la cache y por ultimo manda un mensaje
			if (isAuthorizationListEnabled())
			{
				if (getAuthorizationListEntryStatus(idtag)<0)
				{
					//no esta en la lista.
					send_authorize_request(idtag);
					//lo preautorizamos
					return 1;
				}
				else
				{
					//devuelve 0 o 1 si esta ACCEPTED o CONCURRENT_TX
					return(checkValidAuthorizationListEntry(idtag));
				}
			}
			printf("\nKK");
			if (isAuthorizationCacheEnabled())
			{
				if (getAuthorizationListEntryStatus(idtag)<0)
				{
					send_authorize_request(idtag);
					//lo preautorizamos
					return 1;
				}
				else
				{
					return checkValidAuthorizationCacheEntry(idtag);
				}
			}
		}
		else
		{
			// estamos online y el localpreauthorize no esta activado
			show_authorization_list();
			show_authorization_cache();

			if (isAuthorizationListEnabled())
			{
				if (getAuthorizationListEntryStatus(idtag)<0)
				{
					send_authorize_request(idtag);
					return 0;
				}
				else
				{
					//devuelve 0 o 1 si esta ACCEPTED o CONCURRENT_TX
					return(checkValidAuthorizationListEntry(idtag));
				}
			}

			if (isAuthorizationCacheEnabled())
			{
				if (getAuthorizationListEntryStatus(idtag)<0)
				{
					send_authorize_request(idtag);
					return 0;
				}
				else
				{
					return checkValidAuthorizationCacheEntry(idtag);
				}
			}

			//send_authorize_request(idtag);
			return -1;
		}
	}
	//	The	LocalPreAuthorize  configuration  key  controls  whether  a  Charge  Point  will  use  the  AuthorizationCache  and/or  the  Local  Authorization  List  to  start  a
	//	transaction  without  waiting  for  an  authorization	response from the Central System.

	return -1;
}

//Mantiene un array de autorizaciones a conectores... Lo que no se es si esto realmente esta en la documentacion...
//Cuando llega una mensaje de respuesta Authorize, actualiza este array
int assignConnectorAuthorization(const char *idTag){
	char *newId;
	for (int i=0; i<num_connectors; i++)
	{
		if (!connectorAuthorizations[i].idTag)
		{
			newId=(char *)calloc(1, sizeof(char)*strlen(idTag+1));
			strncpy(newId, idTag, strlen(idTag));
			connectorAuthorizations[i].idTag=newId;

			supplyEnergy(i);
			return i;
		}
	}

	return -1;
}

////////////////////////////////////////////////////////////
//CONNECTORS
////////////////////////////////////////////////////////////
void showConnectors()
{
	puts("\n===============================\nConnector status: ");
		for (int i=0; i<NUM_CONNECTORS; i++)
		{
			printf("\nConector %d: %s", i, ChargePointStatusTexts[connectorStatus[i]]);
		}

		puts("\n===============================\nConnector values: ");
		for (int i=0; i<NUM_CONNECTORS; i++)
		{
			printf("\nValor de %d: %f", i, connectorValues[i]);
		}

		puts("\n===============================\nConnector usage: ");
		for (int i=0; i<NUM_CONNECTORS; i++)
		{
					printf("\nConector Usage de %d: %d", i, connectorUsageRecord[i]);
		}
}


//Nos devuelve si un conector se encuentra actualmente en una transaccion o no
//connector debe ser la posicion del array (valor 0..n-1)
int isConnectorFree(int connector)
{
	return !connectorUsageRecord[connector];
}

//connector debe ser la posicion del array (valor 0..n-1)
void useConnector(int connector)
{
	connectorUsageRecord[connector]=1;
}

//connector debe ser la posicion del array (valor 0..n-1)
void freeConnector(int connector)
{
	connectorUsageRecord[connector]=0;
}


int get_num_connectors()
{
	return num_connectors;
}

//A 4.9 message should be sent for EVERY Status Change. This function is called when a status change occurs
//CONNECTOR DEbe ser ya la posicion del array (0..n)
void changeConnectorStatus(int connector, int status)
{
	//if (debug) printf("AQUI DEBERIA CAMBIARLO. Status es: %d", status);
	connectorStatus[connector]=status;

	int errorCode=0; //<-NOT IMPLEMENTED
	char *extraInfo=NULL; //<-NOT IMPLEMENTED
	char *vendor=getVendor();//<-NOT IMPLEMENTED
	char *vendorErrorCode=NULL;//<-NOT IMPLEMENTED

	//if (debug) printf("\nY AQUI DEBERIA ENVIAR UN STATUS NOTIFICATION");
	send_statusnotification_request(connector+1, status, errorCode, extraInfo, vendor, vendorErrorCode);
}

//CONNECTOR DEbe ser ya la posicion del array (0..n)
int getConnectorStatus(int connector)
{
	if (connector<0 || connector>NUM_CONNECTORS) return -1;
	return connectorStatus[connector];
}

void showConnectorStatuses()
{
	puts("\n===============================\nConnector Status: ");
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		printf("\nConector %d: Status: %d", i+1, connectorStatus[i]);
	}
	printf("\n");
}

///////////////////////////////////////////////
// RESERVATIONS
///////////////////////////////////////////////
void showReservations()
{
	puts("\n===============================\nReservas realizadas: ");
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		printf("\nConector %d: ", i);
		if (!connectorReservations[i]) printf("NULL");
		else printf("Reserva %d, para el Id %s que expira el %s", connectorReservations[i]->reservationId, connectorReservations[i]->idTag, connectorReservations[i]->expiryDate );
	}

	puts("\n===============================\nReservas permitidas en conectores: ");
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		printf("\nConector %d: ", i);
		if (!allowReservations[i]) printf("NO");
		else printf("SI");
	}
	printf("\n");
}

//Devuelve el valor del ID de la reserva realizada para un determinado connector
//O bien -1 si no hay reserva
//Si el numero de conector de entrada es cero, devuelve el reservationId de la primera reserva que encuentre
int getReservationId(int conn)
{
	if (conn>0)
	{
		if (connectorReservations[conn])
		{
			return connectorReservations[conn]->reservationId;
		}

		return -1;
	}
	else
	{
		for (int i=1; i<get_num_connectors(); i++)
		{
			if (connectorReservations[i]) return connectorReservations[i]->reservationId;
			break;
		}
	}

	return -1;
}

//Esta funcion elimina una reserva con el reservationId indicado. No se especifica el conector al que se refiere
//Devuelve 1 si pudo eliminar la reserva con exito o 0 en caso de que no pudiera.
int removeReservation(int reservationId)
{
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		if (connectorReservations[i])
		{
			if (connectorReservations[i]->reservationId==reservationId)
			{
				struct connectorReservation *r=connectorReservations[i];

				if (r->expiryDate) free(r->expiryDate);
				if (r->idTag) free(r->idTag);
				if (r->parentIdTag) free(r->parentIdTag);

				connectorReservations[i]=NULL;
				free(r);

				return 1;
			}
		}
	}
	return 0;
}

//Esta funcion añade una reserva para un conector
//connectorId debe ser un valor de 0 a N
//Devuelve 1 si la reserva fue añadida con exito o 0 si NO fue añadida con exito
//El parentId es un campo opcional, el resto son obligatorios
int addReservation(int ConnectorId, char *expiryDate, char *idTag, int reservationId, const char *parentIdTag)
{
	//Si ya existe una reserva para dicho connector, devuelve 0.
	if (connectorReservations[ConnectorId]) return 0;

	struct connectorReservation *r;
	r=(struct connectorReservation *)calloc(1, sizeof(struct connectorReservation));

	r->connectorId=ConnectorId+1;
	r->expiryDate=strdup(expiryDate);
	r->idTag=strdup(idTag);
	if (parentIdTag) r->parentIdTag=strdup(parentIdTag);
	r->reservationId=reservationId;

	connectorReservations[ConnectorId]=r;

	//if (debug) printf("El connector %d ha sido reservado hasta %s por %s", r->connectorId, r->expiryDate, r->idTag);

	return 1;
}

//Devuelve el connector que esta reservado (0 seria el conector 1) con un determinado reservationId
//Devuelve un valor de 0 a n-1
//Devuelve -1 si no encuentra la reserva
int check_reservationId(int reservationId)
{
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		//El conector 1 es la posicion 0.
		if (connectorReservations[i])
		{
			if (debug) printf("\nSe comprueba si el reservation Id %d es igual al almacenado %d", reservationId, connectorReservations[i]->reservationId);
			if (connectorReservations[i]->reservationId==reservationId)
			{
				if (debug) printf("Lo es en %d", i);
				return i;
			}
		}
	}

	return -1;
}

//Actualiza el expiryDate, IdTag y ParentIdTag de una reserva que haya en el connector indicado
//Devuelve 0 si pudo actualizar con exito o -1 en caso de error
//Ninguno de los tres ultimos parámetros es obligatorio, pero al menos uno debe de ser no NULL o devolverá -1
//connectorId debe ser un valor de 0 a n-1
int update_reservationId(int connectorId, const char *expiryDate, const char *idTag, const char *parentIdTag)
{
	if (!connectorReservations[connectorId]) return -1;

	if ((!expiryDate) && (!idTag) && (!parentIdTag)) return -1;

	if (expiryDate)
	{
		if (connectorReservations[connectorId]->expiryDate)
		{
			free(connectorReservations[connectorId]->expiryDate);
		}
		connectorReservations[connectorId]->expiryDate=strdup(expiryDate);
	}

	if (idTag)
	{
		if (connectorReservations[connectorId]->idTag)
		{
			free(connectorReservations[connectorId]->idTag);
		}

		connectorReservations[connectorId]->idTag=strdup(idTag);
	}

	if (parentIdTag)
	{
		if (connectorReservations[connectorId]->parentIdTag)
		{
			free(connectorReservations[connectorId]->parentIdTag);
		}
		connectorReservations[connectorId]->parentIdTag=strdup(parentIdTag);
	}

	return 0;
}


/////////////////////////////////////////////////////////////
// MESSAGE QUEUE
/////////////////////////////////////////////////////////////

queue_node * create_empty_queue_node(int messageAction)
{
	queue_node *nuevo_mensaje;
	int n=getNextUniqueID();
	nuevo_mensaje = (queue_node *)calloc(1, sizeof(queue_node));
	nuevo_mensaje->MessageAction=messageAction;
	nuevo_mensaje->UniqueId=n;

	return nuevo_mensaje;
}

queue_node * create_queue_node(int messageAction, int n)
{
	queue_node *nuevo_mensaje;
	//int n=getNextUniqueID();
	nuevo_mensaje = (queue_node *)calloc(1, sizeof(queue_node));
	nuevo_mensaje->MessageAction=messageAction;
	nuevo_mensaje->UniqueId=n;

	return nuevo_mensaje;
}

////////////////////////////////////////////////////////////
//INITIALIZATION
//
////////////////////////////////////////////////////////////
void middleware_initialize(int n){
	num_connectors=n;
	chargePointStatus=_CP_STATUS_AVAILABLE;

	//////////////////////////
	//AUTHORIZATIONS
    authorizationList=NULL;
    if (getConfigurationKeyIntValue("AuthorizationcacheEnabled")==1)
    {
    	char *LocalAuthorizationCacheFile=getConfigurationKeyStringValue("LocalAuthorizationCacheFile");
    	if (LocalAuthorizationCacheFile)
    	{
    		read_cache_from_disk(LocalAuthorizationCacheFile);
    	}
    }

    if (getConfigurationKeyIntValue("LocalAuthListEnabled")==1)
    {
    	char *LocalAuthListFile=NULL;
    	LocalAuthListFile=getConfigurationKeyStringValue("LocalAuthListFile");
    	if (LocalAuthListFile)
    	{
    		read_list_from_disk(LocalAuthListFile);
        }
    }

	for (int i =0; i<num_connectors; i++)
	{
		connectorAuthorizations[i].connectorId=i+1;
		connectorAuthorizations[i].idTag=NULL;
	}

	for (int i=0; i<NUM_CONNECTORS; i++)
	{
			connectorUsageRecord[i]=0;
			connectorReservations[i]=NULL;
			connectorStatus[i]=_CP_STATUS_AVAILABLE;

			allowReservations[i]=1;
	}

//	if (debug) show_authorization_cache();
//	if (debug) show_authorization_list();

	//////////////
	// CONFIGURATION

	int j=0;
	while (configurationKeys[j])
	{
		configurationValues[j].keyValue=strdup(configurationKeys[j]);
		configurationValues[j].intValue=-1;
		configurationValues[j].stringValue=NULL;
		j++;
	}

	//////////////////
	//   TRANSACTIONS

	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		transactions[i]=NULL;
	}
	pendingTransactions=NULL;

}

void sendErrorMessage(const char *uniqueid_str, int error_code)
{
	queue_node *nuevo_mensaje=create_empty_queue_node(ERROR_MESSAGE);

	nuevo_mensaje->payload=prepareErrorResponse(uniqueid_str, error_code);

	Enqueue(nuevo_mensaje);
}

//////////////////////////////////
//         TRANSACTIONS
//////////////////////////////////
void showTransactions()
{
	puts("\n===============================\nTransacciones en curso: ");
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		printf("\nConector %d: ", i+1);

		if (!transactions[i]) printf("NULL");
		else printf("Transaccion %d, para el Id %s que empezo el %s con un valor inicial de %d.\n", transactions[i]->transactionId, transactions[i]->idTag,  transactions[i]->startTime, transactions[i]->meterStart);
	}
}

int getNextTransactionId()
{
	localTransactionId++;
	return localTransactionId;
}

//Connector debe ser un valor de 0 a N-1
int isCurrentlyUnderTransaction(int connector)
{
	if (connector<0 || connector>=NUM_CONNECTORS) return 0;

	if (connectorStatus[connector]==_CP_STATUS_CHARGING) return 1;
	else return 0;
}

//Connector debe ser un valor de 0 a N-1
//returns -1 if no transaction found
int getTransactionId(int connector)
{
	if (connector<0 || connector>=NUM_CONNECTORS) return -1;
	if (!transactions[connector]) return -1;

	return transactions[connector]->transactionId;
}

int getConnectorFromTransaction(int transactionId)
{
	if (transactionId<0) return -1;

	for (int i=0;i<NUM_CONNECTORS; i++)
		if (transactions[i] && transactions[i]->transactionId==transactionId) return i;

	return -1;
}

//Añade la transaccion a la estructura de datos 'transactions'
//connectorId debe ser un valor de 1 a N
void addTransaction(int connectorId, const char *idTag, int meterStart, const char *timeStamp, const char *parentIdTag,
		const char *expiryDate_str, int transactionId)
{
	struct transactionData *transaccion=(struct transactionData *)calloc(1, sizeof(struct transactionData));

	transaccion->connector=connectorId;
	transaccion->idTag=strdup(idTag);
	transaccion->meterStart=meterStart;
	transaccion->startTime=strdup(timeStamp);
	transaccion->transactionId=transactionId;
	transaccion->chargingProfile=currentChargingProfiles[connectorId-1];

	transactions[connectorId-1]=transaccion;

	showTransactions();
	updateStopTransactionConnectorCombo();
}

//Esta funcion elimina una transacción del array transactions. OJO, no pone el estado del conector a available
//connectorId debe ser un valor de 0 a n-1
int removeTransaction(int connectorId)
{
	if (transactions[connectorId])
	{
		freeTransaction(transactions[connectorId]);
		transactions[connectorId]=NULL;
		return 1;
	}

	return 0;
}

//Devuelve un numero de conector, que es la posicion del array (valor 0  n-1)
int removeTransactionFromTransactionId(int transactionId)
{
	for (int i=0; i<NUM_CONNECTORS; i++)
	if (transactions[i])
	{
		if (transactions[i]->transactionId==transactionId)
		{
			freeTransaction(transactions[i]);
			transactions[i]=NULL;
			return i;
		}
	}
	return -1;
}

//Esta funcion elimina una transaccion de la lista de pendingTransactions
void removePendingTransaction(int connectorId, int transactionId)
{
	//Recorremos la lista pendingTransactions buscando el transactionID
	struct pendingTransaction *pt=pendingTransactions;
	struct pendingTransaction *ant=pt;

	while (pt && (pt->connector!=connectorId || pt->transactionId!=transactionId))
	{
		ant=pt;
		pt=pt->next;
	}

	if (pt)
	{
		//Transaccion encontrada.

		//Lo eliminamos de la lista:
		ant->next=pt->next;

		//Lo liberamos de memoria:
		if (pt->transaction)
		{
			freeTransaction(pt->transaction);
		}
		free(pt);
	}
}


struct transactionData *getPendingTransaction(int transactionId)
{
	struct pendingTransaction *pt=pendingTransactions;
	while (pt)
	{
		if (pt->transactionId==transactionId) return pt->transaction;
		pt=pt->next;
	}

	return NULL;
}

void freeTransaction(struct transactionData *td)
{
	if (!td) return;

	if (td->idTag) free(td->idTag);
	if (td->startTime) free(td->startTime);
	freeChargingProfile(td->chargingProfile);
	free(td);
}

//connector sera el numero de conector (1..n)
void addPendingTransaction(char *idTag, int connectorId, json_object *obj_chargingProfile, int chargingProfileId, int stackLevel, int transactionId,
		char *purpose_str, char *kind_str, char *recurrency_str, char *valid_from, char *valid_to, json_object *obj_chargingSchedule)
{
	//Vamos hasta el final de la lista pendingTransactions
	struct pendingTransaction *pt=pendingTransactions;
	struct pendingTransaction *temp=(struct pendingTransaction *)calloc(1, sizeof(struct pendingTransaction *));
	while (pt && pt->next) pt=pt->next;
	pt->next=temp;

	temp->connector=connectorId;
	temp->transactionId=transactionId;
	temp->next=NULL;
	struct transactionData *td=calloc(1, sizeof(struct transactionData));
	temp->transaction=td;


	//Ahora rellenamos los datos del transactionData y todo lo que va dentro.
	td->connector=connectorId;
	td->meterStart=connectorValues[connectorId-1];
	td->transactionId=transactionId;
	td->startTime=getCurrentTime();
	td->idTag=strdup(idTag);

	struct ChargingProfile *cp=(struct ChargingProfile *)calloc(1,sizeof(struct ChargingProfile *));
	td->chargingProfile=cp;

	cp->chargingProfileId=chargingProfileId;
	//Asignamos valor al Kind, Purpose y recurrency kid, que las hemos recibido como cadenas.
	if (kind_str) for (int i=0; i<3; i++) if (strcmp(ChargingProfileKindTypeTexts[i], kind_str)==0) cp->chargingProfileKind=i;
	if (purpose_str) for (int i=0; i<3; i++) if (strcmp(ChargingProfilePurposeTypeTexts[i], purpose_str)==0) cp->chargingProfilePurpose=i;
	if (recurrency_str) for (int i=0; i<3; i++)
		if (strcmp(RecurrencyKindTypeTexts[i], recurrency_str)==0)
		{
			cp->recurrencyKind=calloc(1,sizeof(int));
			*cp->recurrencyKind=i;
		}

	cp->stackLevel=stackLevel;

	cp->transactionId=(int *)calloc(1, sizeof(int));
	*cp->transactionId=transactionId;

	if (valid_from)
	{
		strftime (valid_from, 80, "%Y-%m-%dT%H:%M:%S.", cp->validFrom);
	}
	else
	{
		strftime (getCurrentTime(), 80, "%Y-%m-%dT%H:%M:%S.", cp->validFrom);
	}

	if (valid_to)
	{
		strftime (valid_to, 80, "%Y-%m-%dT%H:%M:%S.", cp->validTo);
	}

	struct ChargingSchedule cs;
	if (obj_chargingSchedule)
	{


		//duration
		int duration=-1;
		json_object *obj_duration=json_object_object_get(obj_chargingSchedule, "duration");
		if (obj_duration) duration=json_object_get_int(obj_duration);
		if (duration>0)
		{
			int *f=(int*)calloc(1,sizeof(int));
			*f=duration;
			cs.duration=f;
		}

		///startSchedule
		const char *startSchedule=NULL;
		cs.startSchedule=NULL;
		json_object *obj_startsched=json_object_object_get(obj_chargingSchedule, "startSchedule");
		if (obj_startsched) startSchedule=json_object_get_string(obj_startsched);
		if (startSchedule) strftime(startSchedule, 80, "%Y-%m-%dT%H:%M:%S.",cs.startSchedule);

		//chargingRateUnit
		const char *chargingrateUnitStr=NULL;
		json_object *obj_chargingrateUnit=json_object_object_get(obj_chargingSchedule, "chargingRateUnit");
		if (obj_chargingrateUnit)
		{
			if (enums_as_integers)
			{
				cs.chargingRateUnit=json_object_get_int(obj_chargingrateUnit);
			}
			else
			{
				chargingrateUnitStr=json_object_get_string(obj_chargingrateUnit);
				//if (strcmp(chargingrateUnitStr,"A")==0) cs.chargingRateUnit=0;
				//if (strcmp(chargingrateUnitStr,"W")==0) cs.chargingRateUnit=1;
				cs.chargingRateUnit=(strcmp(chargingrateUnitStr,"A")==0) ? 0:1;
			}
		}

		///minChargingRate
		double minChargingRate=-1.0;
		json_object *obj_minChargingRate=json_object_object_get(obj_chargingSchedule, "minChargingRate");
		if (obj_minChargingRate) minChargingRate=json_object_get_double(obj_minChargingRate);
		if (minChargingRate!=-1.0)
		{
			double *f=(double *)calloc(1,sizeof(double));
			*f=minChargingRate;
			cs.minChargingRate=(float *)f;
		}

		json_object *obj_chargingSchedulePeriod=json_object_object_get(obj_chargingProfile, "chargingSchedulePeriod");

		//OJO ESTO ES UN ARRAY!!!

		if (obj_chargingSchedulePeriod)
		{
			json_object *obj_limit=json_object_object_get(obj_chargingSchedulePeriod, "limit");
			cs.chargingSchedulePeriods.limit=json_object_get_double(obj_limit);

			json_object *obj_startPeriod=json_object_object_get(obj_chargingSchedulePeriod, "startPeriod");
			cs.chargingSchedulePeriods.startPeriod=json_object_get_int(obj_startPeriod);

			json_object *obj_numberPhases=json_object_object_get(obj_chargingSchedulePeriod, "numberPhases");
			if (obj_numberPhases)
			{
				const int temp=json_object_get_int(obj_numberPhases);
				cs.chargingSchedulePeriods.numPhases=calloc(1,sizeof(int));
				*cs.chargingSchedulePeriods.numPhases=temp;
			}

			cs.chargingSchedulePeriods.next=NULL;
		}
	}
	cp->chargingSchedule=cs;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//      	PETICIONES
////////////////////////////////////////////////////////////////////////////////////////////////
//
// 4.1 AUTHORIZE
//
void send_authorize_request(const char *idTagText)
{

	if (communicationStatus==ONLINE)
	{
		//Si hay conexion enviamos la solicitud
		queue_node *nuevo_mensaje=create_empty_queue_node(AUTHORIZE);

		char *string=prepare_authorize_request(nuevo_mensaje->UniqueId, idTagText);
		nuevo_mensaje->payload=string;
		Enqueue(nuevo_mensaje);

		char *LocalPreauthorize=NULL, *AuthorizationCacheEnabled=NULL, *LocalAuthListEnabled=NULL;

		LocalPreauthorize=getConfigurationKeyStringValue("LocalPreAuthorize");
		AuthorizationCacheEnabled=getConfigurationKeyStringValue("AuthorizationCacheEnabled");
		LocalAuthListEnabled=getConfigurationKeyStringValue("LocalAuthListEnabled");

		if (LocalPreauthorize && strcmp(strlwr(LocalPreauthorize), "true")==0)
		{
			if (AuthorizationCacheEnabled && strcmp(strlwr(AuthorizationCacheEnabled), "true")==0)
			{
				if (checkValidAuthorizationCacheEntry(idTagText))
				{
					assignConnectorAuthorization(idTagText);
				}
			}

			if (LocalAuthListEnabled && strcmp(strlwr(getConfigurationKeyStringValue("LocalAuthListEnabled")), "true")==0)
			{
				if (checkValidAuthorizationListEntry(idTagText))
				{
					assignConnectorAuthorization(idTagText);
				}
			}
		}
	}
	else //(OFFLINE)
	{
	//Pag 34:A Charge Point MAY authorize identifier locally without involving the Central System, as described in
	//Local Authorization List (Pag 18).

	//Comprobamos si existe Authorization Cache, List y si esta activada la autorizacion offline
		int status=-1;
		if (strcmp(strlwr(getConfigurationKeyStringValue("LocalAuthorizeOffline")), "true")==0)
		{
			//COMPROBAMOS PRIMERO LA LISTA
			if (strcmp(strlwr(getConfigurationKeyStringValue("LocalAuthListEnabled")), "true")==0)
			{
					status=getAuthorizationListEntryStatus(idTagText);
			}

			if (strcmp(strlwr(getConfigurationKeyStringValue("AuthorizationCacheEnabled")), "true")==0)
			{
				//Authorization list tiene prevalencia sobre la authorization cache
				if (status==-1)
				{
					status=getAuthorizationCacheEntryStatus(idTagText);
				}
			}

			if (status==_CP_AUTHORIZATION_CONCURRENT_TX || status==_CP_AUTHORIZATION_ACCEPTED)
			{
				//Si el idtag esta en la lista o en la cache, añade esa autorizacion a la lista de autorizaciones del conector
				assignConnectorAuthorization(idTagText);

				//Pag 34:  If the idTag is present in the Local Authorization List or Authorization Cache,
				//then the Charge Point MAY send an Authorize.req PDU to the Central System.
				//
				//Yo no lo estoy haciendo porque si no el comportamiento es igual en ambos casos.

				//Pag 34: The Charge Point SHALL only supply energy after authorization.

			}
			else //unknownAuthorization. Pag 18
			{
				if (status==-1) //NO APARECE ESE IDTAG NI EL LOCAL LIST NI LA LOCAL CACHE
				{
					if (getConfigurationKeyIntValue("AllowOfflineTxForUnknownId"))
					{
						if (strcmp(strlwr(getConfigurationKeyIntValue("LocalAuthListEnabled")), "true")==0)
						{
							update_authorization_cache(idTagText, getExpiryDateFromCurrentTime(), NULL, _CP_AUTHORIZATION_ACCEPTED);
						}

						if (strcmp(strlwr(getConfigurationKeyIntValue("AuthorizationCacheEnabled")), "true")==0)
						{
							update_authorization_list(idTagText, getExpiryDateFromCurrentTime(), NULL, _CP_AUTHORIZATION_ACCEPTED, -1);
						}

						assignConnectorAuthorization(idTagText);
					}
				}
			}
		}
	}
}

//
// 4.2 BOOT NOTIFICATION
//
void send_bootNotification_request(const char * vendor_data, const char *chargepointmodel_data, const char *chargeBoxSerialNumber_data,
		const char *chargePointSerialNumber_data, const char *firmwareVersion_data, const char *ICCID_data, const char *IMSI_data,
		const char *meterSerialNumber_data, const char *meterType_data) {
	//queue_node *nuevo_mensaje=create_empty_queue_node(BOOT_NOTIFICATION);
	//char *string=prepare_bootNotification_request(nuevo_mensaje->UniqueId, vendor_data, chargepointmodel_data,chargeBoxSerialNumber_data, chargePointSerialNumber_data, firmwareVersion_data, ICCID_data, IMSI_data, meterSerialNumber_data, meterType_data);
	//nuevo_mensaje->payload=string;
	//Enqueue(nuevo_mensaje);

	char *string=prepare_bootNotification_request(vendor_data, chargepointmodel_data,chargeBoxSerialNumber_data, chargePointSerialNumber_data, firmwareVersion_data, ICCID_data, IMSI_data, meterSerialNumber_data, meterType_data);

}

//
// 4.3 DATA TRANSFER
//
void send_dataTransfer_request(const char *vendorId, const char *messageId, const char *data)
{
		queue_node *nuevo_mensaje=create_empty_queue_node(DATA_TRANSFER);

		char *vendor_from_config=NULL;
		char *string;

		//Aunque VENDOR es una clave obligatoria, en caso de que no aparezca, pues enviamos lo que hayamos puesto en el GUI:
		vendor_from_config=getConfigurationKeyStringValue("ChargePointVendorID");
		if (vendor_from_config)
		{
			string=prepare_dataTransfer_request(nuevo_mensaje->UniqueId, vendor_from_config, messageId, data);
		}
		else
		{
			string=prepare_dataTransfer_request(nuevo_mensaje->UniqueId, vendorId, messageId, data);
		}

		//char *string2=(char *)calloc(1, strlen(string));
		//strcpy (string2, string);
		//string2[strlen(string2)]='\0';
		//nuevo_mensaje->payload=string2;

		nuevo_mensaje->payload=string;
		Enqueue(nuevo_mensaje);

}

//
// 4.4 DIAGNOSTICS STATUS NOTIFICATION
//
void send_diagnosticsstatus_request(int status)
{
	queue_node *nuevo_mensaje=create_empty_queue_node(DIAGNOSTICS_STATUS_NOTIFICATION);

	char *string=prepare_diagnosticsStatusNotification_request(nuevo_mensaje->UniqueId, status);

	nuevo_mensaje->payload=string;
	Enqueue(nuevo_mensaje);
}

//
// 4.5 FIRMWARE DIAGNOSTICS
//
void send_firmwareStatusNotification_request(int status_val) {
	queue_node *nuevo_mensaje=create_empty_queue_node(FIRMWARE_STATUS_NOTIFICATION);

	nuevo_mensaje->payload=prepare_firmwareStatusNotification_request(nuevo_mensaje->UniqueId, status_val);

	Enqueue(nuevo_mensaje);
}

//
// 4.6 HEARTBEAT
//
void send_heartbeat_request()
{
	queue_node *nuevo_mensaje;
	int n=getNextUniqueID();
	nuevo_mensaje = (queue_node *)calloc(1, sizeof(queue_node));
	nuevo_mensaje->MessageAction=HEARTBEAT;
	nuevo_mensaje->UniqueId=n;

	nuevo_mensaje->payload=prepare_heartbeat_request(n);
	Enqueue(nuevo_mensaje);
}

//
// 4.7 METER VALUES
//
void send_metervalues_request (int transactionId, int connectorId, int numMeterValues, int numSamples, struct SampledValue value_list[numMeterValues][numSamples])
{
	if (connectorId<0 || connectorId>=NUM_CONNECTORS || numSamples<=0) return;

//	printf("en la siguiente sv es %s",value_list[0][0].value);
	queue_node *nuevo_mensaje=create_empty_queue_node(METER_VALUES);

	nuevo_mensaje->payload=prepare_metervalues_request(transactionId, 1, nuevo_mensaje->UniqueId, connectorId, numSamples, value_list);

	Enqueue(nuevo_mensaje);
}

//Este hilo va comprobando durante un minuto si se ha recibido o no, la autorizacion de transaccion solicitada.
static void *treatPendingTransaction(int transactionId)
{
	struct transactionData *td=getPendingTransaction(transactionId);

	if (!td || !td->idTag) return NULL;

	int i;
	int authorization;

	for (i=0; i<60; i++)
	{
		authorization=authorize(td->idTag);

		if (authorization!=-1)
		{
			if ((authorization==_CP_AUTHORIZATION_BLOCKED)||(authorization==_CP_AUTHORIZATION_EXPIRED)||(authorization==_CP_AUTHORIZATION_INVALID))
			{
				if (debug) printf("\nSe rechaza porque la autorizacion fue negativa");
				removePendingTransaction(td->connector, transactionId);
			}
			else
			{
				if (debug) printf("\nAutorizacion recibida");
				addTransaction(td->connector, td->idTag, td->meterStart, getCurrentTime(), NULL,NULL, transactionId);
				removePendingTransaction(td->connector, transactionId);

				if (td->chargingProfile)
				{
					int connectorId=td->connector-1; //En transactionData el conector es un valor de 1 a N

					if (currentChargingProfiles[connectorId])
					{
						//Ya hay (al menos) un charging profile. Si es del mismo stackLevel y Purpose, lo sustituimos.
						//Eliminamos los charging profiles de ese conector y le ponemos este.
						struct ChargingProfileList *temp=currentChargingProfiles[connectorId];
						struct ChargingProfileList *next=temp;
						while (temp)
						{
							next=temp->next;
							free(temp);
							temp=next;
						}

						currentChargingProfiles[connectorId]=NULL;
					}

					//Y luego añadimos el nuestro.
					addChargingProfile(td->chargingProfile, connectorId);
				}

				//Iniciamos fisicamente la transaccion
				//el conector debe ser un valor de 0 a N-1
				startTransaction(td->connector-1, td->idTag);

				//Enviamos el mensaje. td->conector debe ser un valor de 1 a N
				send_starttransaction_request(td->connector, getCurrentMeterValueOfConnector(connectorId), td->idTag, -1);
			}

			break;
		}
		usleep(1000*1000); // 1 seg
	}

	if (i==60 && authorization==-1)
	{
		removePendingTransaction(td->connector, transactionId);
	}
}

//connector debe ser una valor de 0 a N-1
static void *sendPeriodicMeterValues(int connector)
{
//9.1.17--> Interval between sampling of metering (or other) data, intended to be
//	transmitted by "MeterValues" PDUs. For charging session data
//	(ConnectorId>0), samples are acquired and transmitted periodically at this
//	interval from the start of the charging transaction.
//	A value of "0" (numeric zero), by convention, is to be interpreted to mean
//	that no sampled data should be transmitted.

	int transactionId;
	if (debug) printf("\n[LOG] EN SENDPERIODICMETERVALUES el conector es %d!!!", connector);

	if (connector>=0)
	{
		int interval=getConfigurationKeyIntValue("MeterValueSampleInterval");

		if (interval>0)
		{
			//Nos quedamos primero esperando a que el connector comience la transaccion
			while (!transactions[connector])
			{
				usleep(1000*interval);
			}

			if (debug) printf("\n[LOG] EN SENDPERIDIC EL CONNECTOR STATUS ES: %d", connectorStatus[connector]);


			transactionId=getTransactionId(connector);
			while (isCurrentlyUnderTransaction(connector))
			{
				if (debug) printf("\n[LOG] EN SENDPERIODICMETERVALUES el transactionId es %d!!!", transactionId );

				int numSamplesCK=getConfigurationKeyIntValue("MeterValuesSampledDataMaxLength");
				int numSamples=-1;

				//Como reboot is not required en esta configuration key, la lectura debe hacer en cada iteracion:
				char *sampledData=getConfigurationKeyStringValue("MeterValuesSampledData");

				printf("\nA");
				if (sampledData)
				{
					printf("\nB");
					if (numSamplesCK<0)
					{
						numSamples=getCSLNumElements("MeterValuesSampledData");
					}
					else
					{
						numSamples=MIN(getCSLNumElements("MeterValuesSampledData"),numSamplesCK);
					}
					struct SampledValue value_list[1][numSamples];


					char *misampledData=strdup(sampledData);
					char** measurands;
					measurands= str_split(misampledData, ',');
					if (debug) printf("\n[LOG] EN SENDPERIODICMETERVALUES!!!");

					for (int i=0; i<numSamples && *measurands; i++)
					{
					//	if (debug) printf("\n[LOG] EN SENDPERIODICMETERVALUES el measurand es %s!!!", *measurands );
						if (debug) printf("\n[LOG] EN SENDPERIODICMETERVALUES!!!");
						struct SampledValue sv;
						sv.unit=-1;
						sv.context=-1;
						sv.location=-1;
						sv.phase=-1;
						sv.formato=-1;
						//sv.value=NULL;
						sv.value=(char *)calloc(1,16);

						//printf("\n%s",*measurands);

						if (strcmp(*measurands,"Voltage")==0)
						{
							sv.measurand=_MV_MEASURAND_VOLTAGE;
							sv.unit=_MV_UNIT_V;
							ftoa(get_Voltage(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Temperature")==0)
						{
							sv.measurand=_MV_MEASURAND_TEMPERATURE;
							sv.unit=_MV_UNIT_CELSIUS;
							ftoa(get_Temperature(), sv.value, 2);
						}
						else if (strcmp(*measurands,"SoC")==0)
						{
							sv.measurand=_MV_MEASURAND_SOC;
							sv.unit=_MV_UNIT_PERCENTAGE;
							ftoa(get_State_Of_Charge(), sv.value, 2);
						}
						else if (strcmp(*measurands,"RPM")==0)
						{
							sv.measurand=_MV_MEASURAND_RPM;
							sv.unit=_MV_UNIT_HZ;
							ftoa(get_Fan_Speed(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Power.Reactive.Import")==0)
						{
							sv.measurand=_MV_MEASURAND_POWER_REACTIVE_IMPORT;
							sv.unit=_MV_UNIT_KVARH;
							ftoa(get_Power_Reactive_Import(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Power.Reactive.Export")==0)
						{
							sv.measurand=_MV_MEASURAND_POWER_REACTIVE_EXPORT;
							sv.unit=_MV_UNIT_KVARH;
							ftoa(get_Power_Reactive_Export(), sv.value, 2);

						}
						else if (strcmp(*measurands,"Power.Offered")==0)
						{
							sv.measurand=_MV_MEASURAND_POWER_OFFERED;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Power_Offered(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Power.Factor")==0)
						{
							sv.measurand=_MV_MEASURAND_POWER_FACTOR;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Power_Factor(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Power.Active.Import")==0)
						{
							sv.measurand=_MV_MEASURAND_POWER_ACTIVE_IMPORT;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_active_power_exported_by_EV(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Power.Active.Export")==0)
						{
							sv.measurand=_MV_MEASURAND_POWER_ACTIVE_EXPORT;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_active_power_exported_by_EV(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Frequency")==0)
						{
							sv.measurand=_MV_MEASURAND_FREQUENCY;
							sv.unit=_MV_UNIT_HZ;
							ftoa(get_Powerline_Frequency(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Reactive.Export.Interval")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_INTERVAL;
							sv.unit=_MV_UNIT_KVARH;
							ftoa(get_Reactive_Exported_Energy_Interval(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Reactive.Import.Interval")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_INTERVAL;
							sv.unit=_MV_UNIT_KVARH;
							ftoa(get_Reactive_Imported_Energy_Interval(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Active.Import.Interval")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_INTERVAL;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Imported_Energy_Interval(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Active.Export.Interval")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_INTERVAL;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Exported_Energy_Interval(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Reactive.Import.Register")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_REGISTER;
							sv.unit=_MV_UNIT_KVARH;
							ftoa(get_Reactive_Energy_Imported_by_EV(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Reactive.Export.Register")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_REGISTER;
							sv.unit=_MV_UNIT_KVARH;
							ftoa(get_Reactive_Energy_Exported_by_EV(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Active.Import.Register")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Energy_Imported_by_EV(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Energy.Active.Export.Register")==0)
						{
							sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_REGISTER;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Energy_Exported_by_EV(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Current.Offered")==0)
						{
							sv.measurand=_MV_MEASURAND_CURRENT_OFFERED;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Current_Offered(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Current.Import")==0)
						{
							sv.measurand=_MV_MEASURAND_CURRENT_IMPORT;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Current_Import(), sv.value, 2);
						}
						else if (strcmp(*measurands,"Current.Export")==0)
						{
							sv.measurand=_MV_MEASURAND_CURRENT_EXPORT;
							sv.unit=_MV_UNIT_KWH;
							ftoa(get_Current_Export(), sv.value, 2);
						}

						value_list[0][i]=sv;
						*(measurands++);

					}//FOR

					printf("\nC");
					send_metervalues_request(transactionId, connector, 1, numSamples, value_list);
					usleep(1000*interval);

					free(misampledData);
				}
			}
		}
	}
	printf("\nE");
}

//Este hilo intenta enviar tres veces un mensaje de inicio de transaccion
static void *sendTransactionRequests2(int unique)
{

	printf("EL UNIQUE ID DENTRO DE sendTransactionRequests2: %d", unique);

int interval,attempts;
int attemptsTried=0;
enum json_tokener_error err;
int connectorId=-1;
const char *UniqueId_Str=NULL;
int uniqueid=(int)unique;

	//Ver Pag 20:  Error responses to transaction-related messages
	// The   number   of   times   and   the   interval   with   which   the   Charge   Point   should   retry   such   failed
	//transaction-related    messages    MAY    be    configured    using    the TransactionMessageAttempts    and
	//TransactionMessageRetryInterval configuration keys.

	char *payloadpointer=getPayloadFromMessage(unique);
	if (!payloadpointer) return NULL;

	char *payload=strdup(payloadpointer);

	interval=getConfigurationKeyIntValue("TransactionMessageRetryInterval");
	//ESTE INTERVALO DEBERIA SER AMYOR AL HEARTBEAT SIEMPRE
	if (interval<getConfigurationKeyIntValue("HeartbeatInterval")) interval=getConfigurationKeyIntValue("HeartbeatInterval");

	attempts=getConfigurationKeyIntValue("TransactionMessageAttempts");

	//Obtenemos el conector y el UniqueId al que se refiere
	json_object *req = json_tokener_parse_verbose(payload, &err);
	json_object *obj_reqpayl, *obj_UniqueId;

	if (send_tags)
	{
		obj_UniqueId= json_object_object_get(req, "UniqueId");
		obj_reqpayl= json_object_object_get(req , "Payload");
	}
	else
	{
		obj_UniqueId=json_object_array_get_idx(req, 1);
		obj_reqpayl=json_object_array_get_idx(req, 3);
	}

	if (obj_UniqueId)
	{
		UniqueId_Str=json_object_get_string(obj_UniqueId);
	}

	if (obj_reqpayl)
    {
		//connector
		json_object *obj_connector = json_object_object_get(obj_reqpayl, "connectorId");
		connectorId=json_object_get_int(obj_connector);
    }
	//Ya tenemos el conector y el UniqueId!!!

	//No empezamos a contar el tiempo hasta que no se envíe el primer mensaje (que se envia desde middleware.c
	while (isThereAMessageOfType(START_TRANSACTION)) usleep(1000);

	usleep(1000*interval * (attemptsTried));
//Pag 20: When  the  Charge  Point  encounters  a  first  failure  to  deliver  a  certain  transaction-related  message,  it
//SHOULD send this message again as long as it keeps resulting in a failure to process the message and it
//has  not  yet  encountered  as  many  failures  to  process  the  message  for  this  message  as  specified  in  its TransactionMessageAttempts
//  configuration  key.  Before  every  retransmission,  it  SHOULD  wait  as  many seconds as specified in its TransactionMessageRetryInterval
// key, multiplied by the number of preceding transmissions of this same message.
//
//As  an  example,  consider  a  Charge  Point  that  has  the  value  "3"  for  the TransactionMessageAttempts
//configuration  key  and  the  value  "60"  for  the TransactionMessageRetryInterval  configuration  key.  It
//sends  a  StopTransaction  message  and  detects  a  failure  to  process  the  message  in  the  Central  System.
//The  Charge  Point  SHALL  wait  for  60  seconds,  and  resend  the  message.  In  the  case  when  there  is  a
//second  failure,  the  Charge  Point  SHALL  wait  for  120  seconds,  before  resending  the  message.  If  this
//final  attempt  fails,  the  Charge  Point  SHOULD  discard  the  message  and  continue  with  the  next
//transaction-related message, if there is any.
	if (attempts>0 && interval>0 && attemptsTried<attempts && connectorId>=0)
	{
		//Entramos en el bucle del hilo.

		//En el momento que OKReceived[connectorId-1] no sea 0, sale del bucle. En ese caso no desencola porque
		//desencolaria por el otro lado (en el switch principal
		if (debug) printf("\n[LOG] attempts tried es %d. attempts es %d y OK received[connectorId-1] es %d con connectorId siendo %d:", attemptsTried, attempts, OKReceived[connectorId-1], connectorId);
		while (attemptsTried<attempts && !OKReceived[connectorId-1])
		{
				//	p=checkMessageinMessageQueue(esteUniqueId);
			queue_node *nuevo_mensaje=create_empty_queue_node(START_TRANSACTION);
			char *cad=replace(payload,UniqueId_Str,convert(nuevo_mensaje->UniqueId));
			//UniqueId_Str=convert(nuevo_mensaje->UniqueId);

			nuevo_mensaje->payload=cad;

			Enqueue(nuevo_mensaje);
			attemptsTried++;
			usleep(1000*interval * (attemptsTried));
		}
	}

	OKReceived[connectorId-1]=0;

	if (attemptsTried==attempts)
	{
		//Si se intento tres veces y no se pudo. Hay que parar la transaccion (si se inicio)
		changeConnectorStatus(connectorId-1, _CP_STATUS_SUSPENDEDEVSE);
		if (transactions[connectorId-1])
		{
			removeTransaction(connectorId-1);
		}

		changeConnectorStatus(connectorId-1, _CP_STATUS_AVAILABLE);
	}
/*
	//Tras los tres intentos, desencola el mensaje
	if ((attemptsTried==attempts) && attempts>0)
	{
		p=checkMessageinMessageQueue(esteUniqueId);

		if (p)
		{
			//Se ha intentado todas las veces que es necesario: Eliminamos la entrada de la cola
			Dequeue_id(esteUniqueId);
			OKReceived[connectorId-1]=0;
		}
	}
*/
	//usleep(1000*getConfigurationKeyIntValue("HeartbeatInterval"));

}

//
// 4.8 START TRANSACTION
//
//connectorId es un valor de 1 a N
void send_starttransaction_request(int connectorId, int meterStartValue, const char *idTag, int reservationId)
{

	queue_node *nuevo_mensaje;
	if (authorize(idTag))
	{
		printf("\nNOS HA AUTORIZADO!!!!");
		//Creamos la peticion en la cola
		nuevo_mensaje=create_empty_queue_node(START_TRANSACTION);

		nuevo_mensaje->payload=prepare_starttransaction_request(nuevo_mensaje->UniqueId,connectorId, meterStartValue, idTag, reservationId);

		Enqueue(nuevo_mensaje);
	}

	printf("EL UNIQUE ID DEL NUEVO MENSAJE ES: %d", nuevo_mensaje->UniqueId);

	pthread_t pid;
	struct lws* wsi;
	pthread_create(&pid, NULL, sendTransactionRequests2, nuevo_mensaje->UniqueId);
	pthread_detach(pid);
}

//
// 4.9 STATUS NOTIFICATION
//
//connectorId debe ser un valor de 1 a N
void send_statusnotification_request(const int connectorId, int newStatus, int errorCode, char *extraInfo, char *vendorId, char *vendorErrorCode) {

	//Creamos la peticion en la cola
	queue_node *nuevo_mensaje=create_empty_queue_node(STATUS_NOTIFICATION);

	nuevo_mensaje->payload=prepare_statusnotification_request(nuevo_mensaje->UniqueId, connectorId, newStatus, errorCode, extraInfo, vendorId, vendorErrorCode);

    Enqueue(nuevo_mensaje);
}

//
// 4.10 STOP TRANSACTION
//
void send_stoptransaction_request(int meterStopValue, int transactionId, int reason, char *idTag)
{
	char *currentTime=getCurrentTime();

		int numSamplesCK=getConfigurationKeyIntValue("MeterValuesSampledDataMaxLength");
		int numSamples=-1;

		char *sampledData=getConfigurationKeyStringValue("MeterValuesSampledData");
		queue_node *nuevo_mensaje=create_empty_queue_node(STOP_TRANSACTION);

		if (sampledData)
		{
		//	printf("\nB. sampledData es %s", sampledData);
			if (numSamplesCK<0)
			{
				numSamples=getCSLNumElements("MeterValuesSampledData");
			}
			else
			{
				numSamples=MIN(getCSLNumElements("MeterValuesSampledData"),numSamplesCK);
			}

			struct SampledValue value_list[1][numSamples];

			char** measurands;
			char *misampledData=strdup(sampledData);
			measurands= str_split(misampledData, ',');

//			printf("\nNUM SAMPLES ES %d", numSamples);
			for (int i=0; i<numSamples && *measurands; i++)
			{
			//	printf("\nD: measurands es : %s", *measurands);
				struct SampledValue sv;
				sv.unit=-1;
				sv.context=-1;
				sv.location=-1;
				sv.phase=-1;
				sv.formato=-1;
				sv.value=(char *)calloc(1,16);

				if (strcmp(*measurands,"Voltage")==0)
				{
					sv.measurand=_MV_MEASURAND_VOLTAGE;
					sv.unit=_MV_UNIT_PERCENTAGE;
					ftoa(get_Voltage(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Temperature")==0)
				{
					sv.measurand=_MV_MEASURAND_TEMPERATURE;
					sv.unit=_MV_UNIT_CELSIUS;
					ftoa(get_Temperature(), sv.value, 2);
				}
				else if (strcmp(*measurands,"SoC")==0)
				{
					sv.measurand=_MV_MEASURAND_SOC;
					sv.unit=_MV_UNIT_PERCENTAGE;
					ftoa(get_State_Of_Charge(), sv.value, 2);
				}
				else if (strcmp(*measurands,"RPM")==0)
				{
					sv.measurand=_MV_MEASURAND_RPM;
					sv.unit=_MV_UNIT_HZ;
					ftoa(get_Fan_Speed(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Power.Reactive.Import")==0)
				{
					sv.measurand=_MV_MEASURAND_POWER_REACTIVE_IMPORT;
					sv.unit=_MV_UNIT_KVARH;
					ftoa(get_Power_Reactive_Import(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Power.Reactive.Export")==0)
				{
					sv.measurand=_MV_MEASURAND_POWER_REACTIVE_EXPORT;
					sv.unit=_MV_UNIT_KVARH;
					ftoa(get_Power_Reactive_Export(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Power.Offered")==0)
				{
					sv.measurand=_MV_MEASURAND_POWER_OFFERED;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_Power_Offered(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Power.Factor")==0)
				{
					sv.measurand=_MV_MEASURAND_POWER_FACTOR;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_Power_Factor(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Power.Active.Import")==0)
				{
					sv.measurand=_MV_MEASURAND_POWER_ACTIVE_IMPORT;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_active_power_exported_by_EV(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Power.Active.Export")==0)
				{
					sv.measurand=_MV_MEASURAND_POWER_ACTIVE_EXPORT;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_active_power_exported_by_EV(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Frequency")==0)
				{
					sv.measurand=_MV_MEASURAND_FREQUENCY;
					sv.unit=_MV_UNIT_HZ;
					ftoa(get_Powerline_Frequency(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Reactive.Export.Interval")==0)
				{
					sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_INTERVAL;
					sv.unit=_MV_UNIT_KVARH;
					ftoa(get_Reactive_Exported_Energy_Interval(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Reactive.Import.Interval")==0)
				{
					sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_INTERVAL;
					sv.unit=_MV_UNIT_KVARH;
					ftoa(get_Reactive_Imported_Energy_Interval(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Active.Import.Interval")==0)
				{
					sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_INTERVAL;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_Imported_Energy_Interval(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Active.Export.Interval")==0)
				{
					sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_INTERVAL;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_Exported_Energy_Interval(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Reactive.Import.Register")==0)
				{
					sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_REGISTER;
					sv.unit=_MV_UNIT_KVARH;
					ftoa(get_Reactive_Energy_Imported_by_EV(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Reactive.Export.Register")==0)
				{
					sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_REGISTER;
					sv.unit=_MV_UNIT_KVARH;
					ftoa(get_Reactive_Energy_Exported_by_EV(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Active.Import.Register")==0)
				{
					sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_Energy_Imported_by_EV(), sv.value, 2);
				}
				else if (strcmp(*measurands,"Energy.Active.Export.Register")==0)
								{
									sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_REGISTER;
									sv.unit=_MV_UNIT_KWH;
									ftoa(get_Energy_Exported_by_EV(), sv.value, 2);
								}
								else if (strcmp(*measurands,"Current.Offered")==0)
								{
									sv.measurand=_MV_MEASURAND_CURRENT_OFFERED;
									sv.unit=_MV_UNIT_KWH;
									ftoa(get_Current_Offered(), sv.value, 2);
								}
								else if (strcmp(*measurands,"Current.Import")==0)
								{
									sv.measurand=_MV_MEASURAND_CURRENT_IMPORT;
									sv.unit=_MV_UNIT_KWH;
									ftoa(get_Current_Import(), sv.value, 2);
								}
				else if (strcmp(*measurands,"Current.Export")==0)
				{
					sv.measurand=_MV_MEASURAND_CURRENT_EXPORT;
					sv.unit=_MV_UNIT_KWH;
					ftoa(get_Current_Export(), sv.value, 2);
				}

				value_list[0][i]=sv;
				*(measurands++);
			}//FOR

			free(misampledData);


		//Creamos la peticion en la cola
	//	printf("\nNUM SAMPLES ES %d", numSamples);
	//	printf("\nEEEES %s",value_list[0][0].value);

		nuevo_mensaje->payload=prepare_stoptransaction_request(nuevo_mensaje->UniqueId, meterStopValue, 1, numSamples, transactionId, reason, idTag, value_list);
		}
		else nuevo_mensaje->payload=prepare_stoptransaction_request(nuevo_mensaje->UniqueId, meterStopValue, 1, numSamples, transactionId, reason, idTag, NULL);

    Enqueue(nuevo_mensaje);


}




////////////////////////////////////////////////////////////////////////////////////////////////
//      	RESPUESTAS
////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.1  CANCEL RESERVATION
//
//Pag 48: If the Charge Point has a reservation matching the reservationId in the request PDU, it SHALL return
//status ‘Accepted’. Otherwise it SHALL return ‘Rejected’.
char *manageCancelReservationRequest(const char *UniqueId_str, int reservationId)
{
	//Reserva será la posicion del array de la reserva. Es decir, un valor entre 0 y N.
	//Si devuelve -1 es que NO existe esa reserva
	int reserva=check_reservationId(reservationId);
	int status;

	if (debug) printf("\nreserva es %d", reserva);

	if (reserva>=0)
	{
		if (removeReservation(reservationId))
		{
			status=_CR_ACCEPTED;
		}
	}
	else
	{
		status=_CR_REJECTED;
	}

	return prepare_cancelreservation_response(UniqueId_str, status);
}


//
//5.2
//
//connector es un valor de 1 a N. Si es 0 es que no se recibe conector
char *manageChangeAvailabilityRequest(const char *uniqueId_str, int connector, int avail)
{
//Pag 48: Upon receipt of a	ChangeAvailability.req PDU, the Charge Point SHALL respond with a ChangeAvailability.conf PDU. The response PDU SHALL
//  indicate  whether  the  Charge  Point  is  able  to	change  to  the  requested  availability  or  not.  When  a  transaction  is  in  progress  Charge  Point  SHALL
//	respond   with   availability   status   'Scheduled'   to   indicate   that   it   is   scheduled   to   occur   after   the
//	transaction has finished.

//Pag 49: In  the  case  the ChangeAvailability.req  contains  ConnectorId  =  0,  the  status  change applies to the Charge Point and all Connectors.

	int response;
	if (avail)
	{
		//operative
		avail=_CP_STATUS_AVAILABLE;
	}
	else
	{
		//inoperative
		avail=_CP_STATUS_UNAVAILABLE;
	}

	if (connector==0)
	{
		for (int i=0; i<num_connectors; i++)
		{
			if (isCurrentlyUnderTransaction(i))
			{
				//tasksAfterTransaction(set status to avail) <-- NOT IMPLEMENTED
				response=_CP_AVAILABILITY_SCHEDULED;
			}
			else
			{
				if (avail==_CP_STATUS_UNAVAILABLE) changeConnectorStatus(i, _CP_STATUS_UNAVAILABLE);  //<-- Pag 41

				response=_CP_AVAILABILITY_ACCEPTED;
			}
		}

		//Set Charge Point to Unavailable
		chargePointStatus=avail;
	}
	else
	{
		if (isCurrentlyUnderTransaction(connector-1))
		{
			//tasksAfterTransaction(set status to avail) <-- NOT IMPLEMENTED
			response=_CP_AVAILABILITY_SCHEDULED;
		}
		else
		{
			if (avail==_CP_STATUS_UNAVAILABLE) changeConnectorStatus(connector-1, _CP_STATUS_UNAVAILABLE);  //<-- Pag 41

			response=_CP_AVAILABILITY_ACCEPTED;
		}
	}

//Pag 48: When  an  availability  change  requested  with  a ChangeAvailability.req PDU  has  happened,  the  Charge
//Point  SHALL  inform  Central  System  of  its  new  availability  status  with  a StatusNotification.req.

//NOT IMPLEMENTED

	return prepareChangeAvailabilityResponse(uniqueId_str, response);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 5.3
//
char *manageChangeConfigurationRequest(const char *UniqueId_str, const char *key, const char *value)
{
//Pag 49:	Upon receipt of a	ChangeConfiguration.req	 Charge Point SHALL reply with a ChangeConfiguration.conf
//	indicating whether it was able to executed the change. Content of "key" and "value" is not prescribed. If
//	"key" does not correspond to a configuration setting supported by Charge Point, it SHALL reply with a
//	status NotSupported. If the change was executed successfully, the Charge Point SHALL respond with a
//	status Accepted. If the change was executed successfully, but a reboot is needed to apply it, the Charge
//	Point SHALL respond with status RebootRequired.
//  In case of failure to set the configuration, the Charge	Point SHALL respond with status Rejected. <-- NOT IMPLEMENTED

	int response=_CC_ACCEPTED;
	int location=-1;

	//configurationValues[i].stringValue;

	location=getConfigurationKeyLocation(key);

	if (location>0)
	{
		//La clave existe
    	if (configurationKeyCharacteristics[location]%50!=3)
    	{
    		///ES READ ONLY
    		response=_CC_REJECTED;
    	}
    	else
    	{
    		if (getConfigurationKeyType_i(location)==0)
    		{
    			//INTEGER
    			configurationValues[location].intValue=atoi(value);
    			response=_CC_ACCEPTED;
    		}

    		if (getConfigurationKeyType_i(location)==1)
    		{
    			//BOOLEAN
    			int tmp=atoi(value);
    			if ((tmp<0) || (tmp>1)) response=_CC_REJECTED;
    			else
    			{
    				configurationValues[location].intValue=tmp;
    				response=_CC_ACCEPTED;
    			}
    		}

    		if (getConfigurationKeyType_i(location)==2)
    		{
    			//STRING
    			configurationValues[location].stringValue=strdup(value);
    			response=_CC_ACCEPTED;
    		}

    		//Si es un CSL, copiamos toda la cadena
    		if (getConfigurationKeyType_i(location)==3)
    		{
    		    //CSL
    		    configurationValues[location].stringValue=strdup(value);
    		    response=_CC_ACCEPTED;
    		}

    		free(value);
    	}
	}
	else
	{
		//La clave no existe
	   response=_CC_NOTSUPPORTED;
	}

	if (response==_CC_ACCEPTED &&getConfigurationKeyRequiresReboot(location))
	{
		response=_CC_REBOOT_REQUIRED;

		//Planificar el reboot en 60sg <-- NOT IMPLEMENTED <-- OJO!! EN LA PAG 96 dice que el CP no se reiniciará por sí solo.
	}

	return prepareChangeConfigurationResponse(UniqueId_str, response);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.4
//
char *manageClearCacheRequest(const char * uniqueId_str)
{
//Pag 49:	Upon  receipt  of  a	ClearCache.req  PDU,  the  Charge  Point  SHALL  respond  with  a	ClearCache.conf  PDU.
//The response PDU SHALL indicate whether the Charge Point was able to clear its Authorization Cache.
//
	int status=clearAuthorizationCache();
	return prepareClearCacheResponse(uniqueId_str, status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 5.5
//
//connectorid debe ser un valor de 1 a N. Si es 0 es que no le llega ningun connector
char *manageClearChargingProfileRequest(const char *UniqueId_str, int chargingprofileid, int connectorid, const char *purpose_str, int stacklevel)
{
	int status;

	int purpose=-1;

	//Obtenemos el id del purpose. En caso de no encontrarlo, el id será -1
	int i=0;
	while (i<sizeof(ChargingProfilePurposeTypeTexts))
	{
		if (strcmp(purpose_str, ChargingProfilePurposeTypeTexts[i])==0)
		{
			purpose=i;
			break;
		}
	}

	if (chargingprofileid>=0)
	{
		status=removeChargingProfilesFromId(chargingprofileid);
	}
	else
	{
		//connectorid debe ser un valor de 1 a N. Si es 0 es que no le llega ningun connector
		status=removeChargingProfilesFromConnector(connectorid, purpose, stacklevel);
	}

	return prepareClearChargingProfileResponse(UniqueId_str, status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.6
//
char *manageCSDataTransferRequest(const char *uniqueId_str, const char *vendorId, const char *MessageId, const char *Data)
{
	//Esta funcion gestionará lo que queramos hacer cuando recibamos el mensaje. Lo interesante llega en "Data"
	int status=manageDataTransfer(vendorId, MessageId, Data);

	char *vendor_str,*local_vendor;
	vendor_str=strlwr_ex(vendorId);
	local_vendor=strlwr_ex(getConfigurationKeyStringValue("ChargePointVendorID"));

	if (strcmp(vendor_str,local_vendor)!=0)
	{
		status=_DT_UNKNOWN_VENDORID;
	}
	else if (!checkDataTransferMessageId(MessageId))
	{
		status=_DT_UNKNOWN_MESSAGEID;
	}
	else if (!manageDataTransfer(vendorId, MessageId, Data))
	{
		status=_DT_REJECTED;
	}

	//Liberamos las cadenas en minusculas
	free(vendor_str);
	free(local_vendor);

	return prepareDataTransferResponse(uniqueId_str, status, Data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.8
//
char *manageGetConfigurationRequest(const char *uniqueId_str, json_object *obj_Payload)
{
	//Presupongo que se recibe una lista de token JSON con la clave "key"

	char *response_string=(char *)calloc(4096, sizeof(char));
	char *response_error_string=(char *)calloc(4096, sizeof(char));

	char* stringCopy;
    json_object *obj_key= json_object_object_get(obj_Payload, "key");
    json_object *val;
    int errors=0;
    int found=0;
    int num_keys, max_num_keys=-1;
    strcpy(response_string, "[");
    strcpy(response_error_string, "[");
    int first=1;

	////////////////////////////////////////////////////
    //Pag 51
    //The number of configuration keys requested in a single PDU MAY be limited by the Charge Point. This 	maximum can be retrieved by reading the configuration key
    //	GetConfigurationMaxKeys
    if (getConfigurationKeyLocation("GetConfigurationMaxKeys")>=0)
    {
   	 max_num_keys=getConfigurationKeyIntValue("GetConfigurationMaxKeys");
    }
	///////////////////////////////////////////////////

    int num=json_object_array_length(obj_key);

    if (num==0)
    {
    	//Nos piden toooodas las claves
    	num_keys=sizeof(configurationValues)/(sizeof(configurationValues[0]));

    	//Si se habia declarado la configuration key 'GetConfigurationMaxKeys', solo se responderá a las 'max_num_keys' primeras
    	if (max_num_keys>0) num_keys=max_num_keys;

    	for (int j=0; j<num_keys; j++)
    	{
    		if (configurationKeys[j])
			{
    			//Si o bien el entero o el string tienen algun valor...
    			if (configurationValues[j].stringValue)
    			{
    				if (!first) strcat(response_string, ",");
    				strcat(response_string, "{\"key\":\"");
    				strcat(response_string, configurationKeys[j]);
    				strcat(response_string, "\",\"readonly\":");
    				strcat(response_string, convert(getConfigurationKeyIsReadOnly(j)));
    				strcat(response_string, ",\"value\":\"");
    				strcat(response_string, configurationValues[j].stringValue);
    				strcat(response_string, "\"}");
    				first=0;
    			}
    			else
    			{
        			if (configurationValues[j].intValue!=-1)
        			{
        				if (!first) strcat(response_string, ",");
        				strcat(response_string, "{\"key\":\"");
        				strcat(response_string, configurationKeys[j]);
        				strcat(response_string, "\",\"readonly\":");
        				strcat(response_string, convert(getConfigurationKeyIsReadOnly(j)));
        				strcat(response_string, ",\"value\":\"");
        				strcat(response_string, convert(configurationValues[j].intValue));
        				strcat(response_string, "\"}");
        				first=0;
        			}
    			}
			}
    	}
    }
    else
    {
     //Nos estan pidiendo un listado de claves

     //Pag 51
     //The number of configuration keys requested in a single PDU MAY be limited by the Charge Point. This 	maximum can be retrieved
     // by reading the configuration key	GetConfigurationMaxKeys.
     if (num>max_num_keys) num=max_num_keys;

 	 int firstinresponse=1;

     for (int i=0; i<num;i++)
     {
    	//Declaracion e inicializacion de variables
    	found=0;

    	//Obtenemos la key en cuestion
    	val=json_object_array_get_idx (obj_key, i);
    	const char *value=json_object_get_string(val);

    	//extraemos el tamaño del lista de clave (75 en estos momentos)
    	num_keys=sizeof(configurationValues)/(sizeof(configurationValues[0]));

    	for (int j=0; j<num_keys; j++)
    	{
    	   if (configurationKeys[j])
    	   {
    		stringCopy=strdup(configurationKeys[j]);

    		//Si se trata de la cadena que se ha pedido:
    		if (strcmp(value, stringCopy)==0)
    		{
    			found=1;
    			if (configurationKeyCharacteristics[j]>0)
    			{
        			//Si existe esa clave Y el valor
     			   if (configurationValues[j].stringValue)
     			   {
     				  if (!firstinresponse) strcat(response_string, ",");
     				  strcat(response_string, "{\"key\":\"");
     				  strcat(response_string, configurationKeys[j]);
     				  strcat(response_string, "\",\"readonly\":");
     				  strcat(response_string, convert(getConfigurationKeyIsReadOnly(j)));
     				  strcat(response_string, ",\"value\":\"");
     				  strcat(response_string, configurationValues[j].stringValue);
     				  strcat(response_string, "\"}");
     				  firstinresponse=0;
     			   }
     			   else
     			   {
     				  if (configurationValues[j].intValue!=-1)
     				  {
     					  //Es un entero valido
         				  if (!firstinresponse) strcat(response_string, ",");
         				  strcat(response_string, "{\"key\":\"");
         				  strcat(response_string, configurationKeys[j]);
         				  strcat(response_string, "\",\"readonly\":");
         				  strcat(response_string, convert(getConfigurationKeyIsReadOnly(j)));
         				  strcat(response_string, ",\"value\":\"");
         				  strcat(response_string, convert(configurationValues[j].intValue));
         				  strcat(response_string, "\"}");
         				  firstinresponse=0;
     				  }
     				  else
     				  {
     					  //HA HABIDO ALGUN PROBLEMA AL LEER ESTA CLAVE. Ambos stringValue es null y intvalue es -1
     				   	   if (errors==0)
     				   	   {
     				   		   strncat(response_error_string, value, 50);  //las claves como maximo tienen 50 bytes
     				   	   }
     				   	   else
     				   	   {
     				   		   strcat(response_error_string, ",") ;
     				   		   strncat(response_error_string, value, 50);
     				   	   }
     				   	   errors++;
     				  }
     			   }
    			}
    		}

    		free(stringCopy);
    	  }
    	}

    	//Hemos recorrido los 75 configuration keys y la solicitada no es una de ellas. Lo añadimos al response error
    	if (!found)
    	{
		   	   if (errors==0)
		   	   {
		   		   strcat(response_error_string, "\"") ;
		   	   }
		   	   else
		   	   {
		   		   strcat(response_error_string, ",\"") ;
		   	   }

		   	   strncat(response_error_string, value, strlen(value)) ;
	   		   strcat(response_error_string, "\"") ;

		   	   errors++;
    	}
    }
    }
    strcat(response_error_string, "]") ;
    strcat(response_string, "]") ;

    //Evitamos buffer overflows
    response_string[4095]='\0';
    response_error_string[4095]='\0';

    //if (strlen(response_string)>4094 || strlen(response_error_string)>4094) <--Error no hemos pasado de largo. <-- NOT IMPLEMENTED

   // long id=atol();
	//AHORA HAY QUE PREPARAR LA RESPUESTA
    return(prepare_getconfiguration_response(uniqueId_str , response_string, response_error_string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.9
//

char *manageGetDiagnosticsRequest(const char *UniqueId_str, const char *location, int retries, int retryInterval, const char *startTime, const char *stopTime)
{
	//Envio del Fichero

	char *diagnosticsfilename=DIAGNOSTICSFILE;
	FILE *pf;
	unsigned long fsize;
	pf = fopen(diagnosticsfilename, "rb");
	if (pf == NULL)
	{
		//Pag 52:
		//If no diagnostics file is available, then	GetDiagnostics.conf
		// SHALL NOT contain a	file name.

		return prepare_getDiagnostics_response(UniqueId_str, NULL);
	}

	struct ftp_parameters *parameters=calloc(1, sizeof(struct ftp_parameters));
	parameters->location=strdup(location);
	parameters->retries=retries;
	parameters->retryInterval=retryInterval;
	parameters->startTime=NULL;
	parameters->stopTime=NULL;
	parameters->originalfilename=strdup(diagnosticsfilename);

	//Por defecto usa el usuario y password indicado en el fichero INI. Si no, lo intenta con OCPP:OCPP
	char *ftppass=getConfigurationKeyStringValue("FTPPassword");
	char *ftpuser=getConfigurationKeyStringValue("FTPUser");
	if (ftpuser) parameters->username=strdup(ftpuser);
	else parameters->username=strdup("OCPP");
	if (ftppass) parameters->password=strdup(ftppass);
	else parameters->password=strdup("OCPP");

	if (startTime) parameters->startTime=strdup(startTime);
	if (stopTime) parameters->stopTime=strdup(stopTime);

	//Comprobamos si la fecha de inicio es posterior a la fecha de fin
	if (startTime && stopTime)
	{
		struct tm* startTimeTM=(struct tm *)calloc(1, sizeof(struct tm));
		struct tm* stopTimeTM=(struct tm *)calloc(1, sizeof(struct tm));
		strptime(parameters->startTime, "%Y-%m-%dT%H:%M:%S.", startTimeTM);
		strptime(parameters->stopTime, "%Y-%m-%dT%H:%M:%S.", stopTimeTM);
		__time_t fechastarttime = mktime(startTimeTM);
		__time_t fechastoptime = mktime(stopTimeTM);
		double diffSecs = difftime(fechastarttime, fechastoptime);

		free(startTimeTM);
		free(stopTimeTM);

		if (diffSecs>0.0)
		{
			//Si la fecha de inicio es mayor que la de fin, devolvemos mensaje de error.
			return prepare_getDiagnostics_response(UniqueId_str, NULL);
		}
	}

	////
	char *filename=calloc(32, sizeof(char));
	strcpy(filename, "ocpp_diagnostics");
	strncat(filename, getConfigurationKeyStringValue("ChargePointSerialNumber"),(size_t)10);
	strcat(filename, ".dat");
	parameters->destinationfilename=strdup(filename);

	//Creamos el hilo que mandará el fichero
	pthread_t pidFTP;
	pthread_create(&pidFTP, NULL, sendDiagnosticsFile, parameters);
	pthread_detach(pidFTP);

	//Pag 52:
	//Upon receipt of a	GetDiagnostics.req PDU, and if diagnostics information is available then Charge Point
	//SHALL  respond  with  a	GetDiagnostics.conf  PDU  stating  the  name  of  the  file  containing  the  diagnostic
	//information that will be uploaded. Charge Point SHALL upload a single file.

	return prepare_getDiagnostics_response(UniqueId_str, filename);

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.10
//

char *respondGetLocalListVersionRequest(const char *UniqueId_str)
{
//Upon    receipt    of    a	GetLocalListVersion.req    PDU    Charge    Point    SHALL    respond    with    a	GetLocalListVersion.conf
//	 PDU containing the version number of its Local Authorization List. A version number of 0 (zero) SHALL be used to indicate that the local authorization list
//	is empty, and a version	number  of  -1  SHALL  be  used  to  indicate  that  the  Charge  Point  does  not  support  Local  Authorization Lists.

	if (getConfigurationKeyIntValue("LocalAuthListEnabled")==0)
	{
		//Local auth list is not enabled
		return prepare_getLocalListVersion_response(UniqueId_str, -1);
	}
	else
	{
		if (authorizationList==NULL) return prepare_getLocalListVersion_response(UniqueId_str, 0);
	}

	return prepare_getLocalListVersion_response(UniqueId_str, localListVersion);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.11
//
//connector sera el numero de conector (1..n)
char *respondRemoteStartTransactionRequest(const char *UniqueId_str,const char *idTag, int connectorId, json_object *obj_chargingProfile,
		int chargingProfileId, int stackLevel, int transactionId, const char *purpose_str, const char * kind_str, const char *recurrency_str,
		const char *valid_from, const char *valid_to, json_object *obj_chargingSchedule)
{
	//Pag 52: Upon	receipt, the	Charge	Point	SHALL	reply	with RemoteStartTransaction.conf and a status
	//indicating whether it is able to start a transaction or not.
	int status;
	int pending=0;

	//Pag 90. TransactionId. Only valid if Purpose is set to TXProfile. Otherwise, the transactionId should be chosen by the CP
	if (!purpose_str)
	{
		transactionId=getNextTransactionId();
	}

	//
	//The transactionId MAY be used to match the profile to a specific transaction
	//
	//¿¿¿¿¿¿?????? NOT IMPLEMENTED
	//
	///////////

	//Pag 52: The effect of the RemoteStartTransaction.req message depends on the value of the AuthorizeRemoteTxRequests configuration key in the Charge Point.
	if (!getConfigurationKeyIntValue("AuthorizeRemoteTxRequests"))
	{
		//Pag 53: If the value of "AuthorizeRemoteTxRequests" is false, the Charge Point SHALL immediately try to start a
		//transaction  for  the  idTag  given  in  the  RemoteStartTransaction.req  message.

		//Note  that  after  the transaction  has  been  started,  the  Charge  Point  will  send  a StartTransaction request  to  the  Central
		//System,  and  the  Central  System  will  check  the  authorization  status  of  the  idTag  when  processing this StartTransaction request.

		//Esto comprueba, a nivel de interfaz gráfica, si el conector esta o no, en estado de error.
		if (checkErrorStateOfConnector(connectorId-1))
		{
//			if (debug) printf("\nSe rechaza porque el connector dio error");
			status=_RSSS_REJECTED;
		}
		else
		{
			status=_RSSS_ACCEPTED;
		}
	}
	else
	{
		//Pag 53: If the value of AuthorizeRemoteTxRequests is true, the Charge Point SHALL behave as if in response
		//to   a   local   action   at   the   Charge   Point   to   start   a   transaction   with   the   idTag   given   in   the
		//RemoteStartTransaction.req  message.  This  means  that  the  Charge  Point  will  first  try  to  authorize
		//the idTag, using the Local Authorization List, Authorization Cache and/or an Authorize.req request.
		//A transaction will only be started after authorization was obtained.

		int authorization=authorize(idTag);

		if (authorization==-1)
		{
			//No podemos iniciar la transaccion ahora mismo, hay que esperar a que llegue la respuesta del AUTHORIZE
			pending=1;
			status=_RSSS_ACCEPTED;
		}
		else
		{

			if ((authorization==_CP_AUTHORIZATION_BLOCKED)||(authorization==_CP_AUTHORIZATION_EXPIRED)||(authorization==_CP_AUTHORIZATION_INVALID))
			{
				if (debug) printf("\nSe rechaza porque la autorizacion fue negativa");
				status=_RSSS_REJECTED;
			}
			else
			{
				status=_RSSS_ACCEPTED;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	//
	//Una vez que vemos que esta aceptado y autorizado el idtag, comprobamos si el connectorId es valido
	if (status==_RSSS_ACCEPTED)
	{
		int conn;

		if (connectorId<=0)
		{
			//Nos piden que busquemos un conector valido
			for (conn=0; conn<num_connectors; conn++)
			{
				if (isConnectorFree(conn) && connectorStatus[conn]==0){
					useConnector(conn);
					//conn es un valor de 0 a n-1, pero connector Id, que se va a usar mas tarde debe ser un valor de 1 a n
					connectorId=conn+1;
					break;
				}
			}

			if (conn==num_connectors)
			{
				if (debug) printf("\nSe rechaza porque se pidio el conector 0 y no hay conectores vacios");
				status=_RSSS_REJECTED;
			}
			else
			{
				//Pag 88: A connector is set to PREPARING when a user presents a Tag, inserts a cable, or a vehicle Occupies Parking Bay.
				changeConnectorStatus(connectorId-1, _CP_STATUS_PREPARING);
			}
		}
		else
		{
			//Pag 88: When the EVSE is ready to deliver energy but contactor is open, the EV is not ready and status should be SuspendedEV
			if (!isCableLocked(connectorId))
			{
				changeConnectorStatus(connectorId-1, _CP_STATUS_SUSPENDEDEV);
				status=_RSSS_REJECTED;
			}
			else
			{
				//El cable esta conectado. Por ultimo comprobamos si es parte ya de una transaccion
				if (!isConnectorFree(connectorId-1))
				{
					if (debug) printf("\nSe rechaza porque ya se encuentra siendo parte de una transaccion");
					status=_RSSS_REJECTED;
				}
				//Todo OK. Actualizamos ConnectorUsage[]
				else useConnector(connectorId-1);

				//Pag 88: A connector is set to PREPARING when a user presents a Tag, inserts a cable, or a vehicle Occupies Parking Bay.
				changeConnectorStatus(connectorId-1, _CP_STATUS_PREPARING);
			}
		}
	}

	if (debug) showConnectors();

	struct ChargingProfile *chargprof=NULL;
	//ACTIVAMOS EL CHARGING PROFILE
	//Y si todo es valido hasta el momento comrpobamos si el charging profile que nos mandaron nos vale y lo usamos.
	if (status==_RSSS_ACCEPTED)
	{
		struct transactionData *td=(struct transactionData *)calloc(1, sizeof(struct transactionData));
		td->connector=connectorId;
		td->idTag=strdup(idTag);
		td->transactionId=transactionId;

		//Pag 53: The Central System MAY include a ChargingProfile in the RemoteStartTransaction request. The purpose
		//of  this ChargingProfile SHALL  be  set  to TxProfile. If  accepted,  the  Charge  Point  SHALL  use  this ChargingProfile for the transaction.

		//Una transaccion tiene que tener un charging profile asociado. Ya sea el de por defecto, el de por defecto del
		//connector o uno especifico que nos mandan en la 5.11
		if (obj_chargingProfile)
		{
			//Pag 53: if a Charge Point without support for Smart Charging receives a RemoteStartTransaction.req  with a Charging Profile,
			//this parameter SHOULD be ignored.
			//
			if (CPhasSupportForSmartCharging())
			{
				if (strcmp(purpose_str,_CPPT_TXPROFILE)!=0)
				{
					if (debug) printf("\nSe rechaza porque purpose_str es _CPPT_TXPROFILE");
					status=_RSSS_REJECTED;
					free(td->idTag);
					free(td);
				}
				else
				{
					//Por si algun otro parametro del charging profile me pudiera cambiar el status:
					//
					//status=checkChargingProfileCharacteristics() <--NOT IMPLEMENTED
					//

					///////////
					//Pag 90
					//If validFrom is absent, the profile is valid as soon as it is received by the charge point
					//
					if (!valid_from) td->startTime=getCurrentTime();
					///////////

					td->meterStart=connectorValues[connectorId-1];

					chargprof=(struct ChargingProfile *)calloc(1, sizeof(struct ChargingProfile));
					chargprof->chargingProfileId=chargingProfileId;
					chargprof->stackLevel=stackLevel;
					chargprof->transactionId=transactionId;

					if (recurrency_str) //opcional
					{
						//recurrency
						for (int i=0; i<7; i++)
						{
							if (strcmp(recurrency_str, RecurrencyKindTypeTexts[i])==0)
							{
								chargprof->recurrencyKind=i;
								break;
							}
						}
					}

					for (int i=0; i<7; i++)
					{
						if (strcmp(kind_str, ChargingProfileKindTypeTexts[i])==0)
						{
							chargprof->chargingProfileKind=i;
							break;
						}
					}

					//purpose
					for (int i=0; i<7; i++)
					{
						if (strcmp(purpose_str, ChargingProfilePurposeTypeTexts[i])==0)
						{
							chargprof->chargingProfilePurpose=i;
							break;
						}
					}

					if (valid_from)
					{
						strftime (valid_from, 80, "%Y-%m-%dT%H:%M:%S.", chargprof->validFrom);
					}
					else
					{
						strftime (getCurrentTime(), 80, "%Y-%m-%dT%H:%M:%S.", chargprof->validFrom);
					}

					if (valid_to)
					{
						strftime (valid_to, 80, "%Y-%m-%dT%H:%M:%S.", chargprof->validTo);
					}

					if (obj_chargingSchedule)
					{
						//struct ChargingSchedule *cs=(struct ChargingSchedule *)calloc(1, sizeof(struct ChargingSchedule));

						int duration=-1;
						json_object *obj_duration=json_object_object_get(obj_chargingSchedule, "duration");
						if (obj_duration) duration=json_object_get_int(obj_duration);
						if (duration>0)
						{
							int *f=(int*)calloc(1,sizeof(int));
							*f=duration;
							chargprof->chargingSchedule.duration=f;
						}

						///
						char *startSchedule=NULL;
						json_object *obj_startsched=json_object_object_get(obj_chargingSchedule, "startSchedule");
						if (obj_startsched) startSchedule=json_object_get_string(obj_startsched);
						if (startSchedule) strftime(startSchedule, 80, "%Y-%m-%dT%H:%M:%S.",chargprof->chargingSchedule.startSchedule);
						else chargprof->chargingSchedule.startSchedule=NULL;

						///
						char *chargingrateUnitStr=NULL;
						json_object *obj_chargingrateUnit=json_object_object_get(obj_chargingSchedule, "chargingRateUnit");
						if (obj_chargingrateUnit)
						{
							if (enums_as_integers)
							{
								chargprof->chargingSchedule.chargingRateUnit=json_object_get_int(obj_chargingrateUnit);
							}
							else
							{
								chargingrateUnitStr=json_object_get_string(obj_chargingrateUnit);
								if (strcmp(chargingrateUnitStr,"A")==0) chargprof->chargingSchedule.chargingRateUnit=0;
								if (strcmp(chargingrateUnitStr,"W")==0) chargprof->chargingSchedule.chargingRateUnit=1;
							}
						}

						///
						double minChargingRate=-1.0;
						json_object *obj_minChargingRate=json_object_object_get(obj_chargingSchedule, "minChargingRate");
						if (obj_minChargingRate) minChargingRate=json_object_get_double(obj_minChargingRate);
						if (minChargingRate!=-1.0)
						{
							double *f=(double *)calloc(1,sizeof(double));
							*f=minChargingRate;
							chargprof->chargingSchedule.minChargingRate=f;
						}

					}
				}
			}
			else
			{
				//Lo ignoramos todo porque realmente no tiene capacidad de smartchrging:
				free(obj_chargingProfile);//<--Free all the strings in memory
				obj_chargingProfile=NULL;
			}
		}
	}

	//
	//Si todo es valido, hacemos la transaction y si esta pending la ponemos en pending
	if (status==_RSSS_ACCEPTED)
	{
		if (pending)
		{
			//Tenemos una transaccion pendiente de que llegue una respuesta de autorizacion por parte del server.
			//Lo añadimos a la lista de transacciones pendientes y creamos un hilo que, durante 60 segundos comprobará
			//si se ha actualizado la lista local de autorizacion (en respuesta al mensaje Authorize antes mandado
			addPendingTransaction(idTag, connectorId, obj_chargingProfile, chargingProfileId, stackLevel, transactionId, purpose_str,kind_str, recurrency_str,
					valid_from, valid_to, obj_chargingSchedule);

			pthread_t pidPendingTransactions;
			pthread_create(&pidPendingTransactions, NULL, treatPendingTransaction, transactionId);
			pthread_detach(pidPendingTransactions);

			return NULL;
		}
		else
		{
			//Si teniamos chargingprofile, se le pasa el charging profile. Si no, se le pasa NULL.
			//Update Transaction Charging Profile
			if (chargprof)
			{
				if (currentChargingProfiles[connectorId])
				{
					//Ya hay (al menos) un charging profile. Si es del mismo stackLevel y Purpose, lo sustituimos.
					//Eliminamos los charging profiles de ese conector y le ponemos este.
					struct ChargingProfileList *temp=currentChargingProfiles[connectorId];
					struct ChargingProfileList *next=temp;
					while (temp)
					{
						next=temp->next;
						free(temp);
						temp=next;
					}
					currentChargingProfiles[connectorId]=NULL;
				}

				//Y luego añadimos el nuestro.
				if (!addChargingProfile(chargprof, connectorId))
				{
				//Devuelve 1 si se añadio exitosamente. Devuelve 0 si NO pudo añadir el charging profile
						status=_CPS_NOTSUPPORTED;
				}
				else status=_CPS_ACCEPTED;
			}

			//Iniciamos fisicamente la transaccion
			startTransaction(connectorId, idTag);

			//Enviamos el mensaje
			send_starttransaction_request(connectorId, getCurrentMeterValueOfConnector(connectorId), idTag, -1);
		}
	}

	return prepare_remotestarttransaction_response(UniqueId_str, status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.12 REMOTE STOP TRANSACTION
//
char *respondRemoteStopTransactionRequest(const char *UniqueId_str, int transactionId)
{
	int reason=_ST_REMOTE;

	//Pag 54: Therefore the transaction SHALL be stopped. The charge point SHALL send a StopTransaction.req and, if applicable, unlock the connector.
	//connectorId sera un valor de 0 a n-1 o -1 en caso de error
	int connectorId=removeTransactionFromTransactionId(transactionId);

	if (connectorId>=0)
	{
		//Si se pudo eliminar la transaccion exitosamente nos devuelve la conexion donde estaba

		int meterStopValue=-1;
		meterStopValue=connectorValues[connectorId];

		changeConnectorStatus(connectorId, _CP_STATUS_FINISHING); //<-- Pag 42

		//Mandamos el mensaje

		send_stoptransaction_request(meterStopValue, transactionId, reason, NULL);


		if (!unlockCable(connectorId))
		{
			currentChargePointState=_CP_STATUS_FAULTED;
			currentChargePointErrorStatus=_CP_ERROR_CONNECTORLOCKFAILURE;
		}

		return prepare_remotestoptransaction_response(UniqueId_str, _RSSS_ACCEPTED);
	}

	//NO SE ENCONTRO NINGUNA TRANSACCION CON EL TRANSACTION_ID INDICADO:
	return prepare_remotestoptransaction_response(UniqueId_str, _RSSS_REJECTED);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  5.13 RESERVE NOW
//
//connectorId debe ser un valor de 0 a n-1
char *manageReserveNowRequest(const char *UniqueId, int connectorId, const char *expiryDate, const char *idTag, int reservationId, const char *parentIdTag)
{
	int ok=0, status=-1, reserva=0;

	//Primero comprueba si esa reserva ya esta en el chargepoint
	//Devuelve -1 si NO encuentra la reserva
	reserva=check_reservationId(reservationId);

	//reserva es una valor de 0 a n-1
	//Si ya existe esa reserva y coincide con el connectorId que enviamos, la actualizamos (actualizando expirydate y demas).
	if (reserva>=0 && reserva==connectorId)
	{
		//Ya se encontró una reserva con el reservationId. Actualizamos la reserva
		update_reservationId(reserva, expiryDate, idTag, parentIdTag);
		ok=_CP_RESERVATION_ACCEPTED;

		changeConnectorStatus(connectorId, _CP_STATUS_RESERVED);
	}
	else
	{
		if (reserva>=0 && reserva!=connectorId)
		{
			//Si la reserva con ese reservationId esta asociada con otro connector... devolvemos mensaje de error
			return prepareErrorResponse(UniqueId, _PROPERTY_CONSTRAINT_VIOLATION);
		}
		else
		{
			//No se encontró ninguna reserva con ese reservationId y ese conector.

			//Si el connector es 0 prueba con todos los conectores
			if (connectorId==0)
			{
				int conn;
				//Se recorre el array hasta que se encuentra un connector que no se encuentra actualmente en una transaccion y
				//que permite ser reservado
				for (conn=0; conn<num_connectors && (!isConnectorFree(conn) || !allowReservations[conn]); conn++);

				//Si i==num_Connectors, es que estan todos en uso
				if (conn==num_connectors)
				{
					ok=_CP_RESERVATION_OCCUPIED;
				}
				else
				{
					//Hay alguno libre (i). ConnectorId es un valor de 0 a N
					connectorId=conn;
					status=connectorStatus[connectorId];

					if (status==_CP_STATUS_AVAILABLE)
					{
						  if (addReservation(connectorId, expiryDate, idTag, reservationId, parentIdTag))
						  {
							  ok=_CP_RESERVATION_ACCEPTED;
							  changeConnectorStatus(connectorId, _CP_STATUS_RESERVED);
						  }
						  else
						  {
							  ok=_CP_RESERVATION_OCCUPIED;
						  }
					}
					else
					{
						  if (status==_CP_STATUS_FAULTED) ok=_CP_RESERVATION_FAULTED;
						  if (status==_CP_STATUS_UNAVAILABLE) ok=_CP_RESERVATION_UNAVAILABLE;
						  if (status==_CP_STATUS_RESERVED) ok=_CP_RESERVATION_OCCUPIED;
					}
				}
			}
			else
			{
				status=connectorStatus[connectorId];
				//El conector indicado es mayor que cero
				//Primero comprueba si el connector ya esta reservado. Si lo esta devuelve OCUPADO
				if (connectorReservations[connectorId]) return prepare_reservenow_response(UniqueId, _CP_RESERVATION_OCCUPIED);
				//En caso contrario, realiza la reserva
				else
				{
					if (status==_CP_STATUS_AVAILABLE)
					{
						  if (addReservation(connectorId, expiryDate, idTag, reservationId, parentIdTag))
						  {

							  ok=_CP_RESERVATION_ACCEPTED;
							  changeConnectorStatus(connectorId, _CP_STATUS_RESERVED);
						  }
						  else
						  {
							  ok=_CP_RESERVATION_OCCUPIED;
						  }
					}
					else
					{
						  if (status==_CP_STATUS_FAULTED) ok=_CP_RESERVATION_FAULTED;
						  if (status==_CP_STATUS_UNAVAILABLE) ok=_CP_RESERVATION_UNAVAILABLE;
						  if (status==_CP_STATUS_RESERVED) ok=_CP_RESERVATION_OCCUPIED;
					}
				}
			}
		}
	}

	//AHORA HAY QUE PREPARAR LA RESPUESTA
	return prepare_reservenow_response(UniqueId, ok);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.14 RESET
//
// Pag 56:  Upon  receipt  of  a Reset.req  PDU,  the  Charge  Point SHALL respond with a Reset.conf PDU. The response PDU SHALL include whether the Charge Point is
//will attempt to reset itself.
char *manageResetRequest(const char *UniqueId_str, const char *resetType_str)
{
	int meterStopValue, connectorId, transactionId;

//REALMENTE LAS TAREAS A REALIZAR CON EL HARD Y EL SOPFT RESET SON SIMILARES:

//Pag 56: At  receipt  of  a  hard  reset  the  Charge  Point  SHALL  attempt  to  terminate  any  transaction  in  progress
//		normally as in	StopTransaction	and then perform a reboot.

//Pag 56: At  receipt  of  a  soft  reset,  the  Charge  Point  SHALL  return  to  a  state  that  behaves  as  just  having  been
//		booted. If any transaction is in progress it SHALL be terminated normally, before the reset, as in		Stop		Transaction		.

	//Paramos todas las transacciones:
		for (int i=0; i<NUM_CONNECTORS; i++)
		{
			if (transactions[i])
			{
				connectorId=transactions[i]->connector;
				//conectorId es por tanto un valor de 1 a N

				if (connectorId>0)
				{
					//Si la transaccion tenia un numero de conector asociado...

					transactionId=transactions[i]->transactionId;

					//Lo eliminamos de la estructura interna
					removeTransaction(connectorId-1);

					//Stop physically the transaction:
					stopTransaction(connectorId-1); //<--NOT IMPLEMENTED
					updateStopTransactionInGUI();
					meterStopValue=connectorValues[connectorId-1];

					//		Pag 46: The  idTag  in  the  request  PDU  MAY  be  omitted  when  the  Charge  Point  itself  needs  to  stop  the
					//		transaction. For instance, when the Charge Point is requested to reset.

					//Mandamos el mensaje (uno por cada parada de transaccion.

					if (strcmp(resetType_str, "HardReset")==0)
					{
						send_stoptransaction_request(meterStopValue, transactionId, _ST_HARD_RESET, NULL);
					}
					else
					{
						send_stoptransaction_request(meterStopValue, transactionId, _ST_SOFT_RESET, NULL);
					}

					if (!unlockCable(connectorId-1))
					{
						currentChargePointState=_CP_STATUS_FAULTED;
						currentChargePointErrorStatus=_CP_ERROR_CONNECTORLOCKFAILURE;
					}
				}
			}

//Pag 56: Persistent states: for example: Connector set to Unavailable shall persist

//		Before rebooting the settings should be store in disk or Database
		//saveStatetoDisk()  <-- NOT IMPLEMENTED

		if (strcmp(resetType_str, "SoftReset")==0)
		{
			int error=CP_initialize();
			if (error>0)
			{
				currentChargePointState=_CP_STATUS_FAULTED;
				currentChargePointErrorStatus=_CP_ERROR_RESETFAILURE;
			}

			middleware_initialize(num_connectors);
		}

		//DUDA:
		//NO SE EN QUE SITUACION PODRIA MANDARSE UN RESET "REJECTED"...
		return prepare_reset_response(UniqueId_str, _RSSS_ACCEPTED);
	}
}

//
// 5.15
//
char *manageSendLocalListRequest(const char *UniqueId_str, int version, json_object *obj_localAuthorizationList, const char *updateType_str)
{
	struct json_object *authorizationDataElement;
	json_object *obj_idTag, *obj_idTagInfo, *obj_expiryDate, *obj_parentIdTag, *obj_status;
	int retorno=0;
	//Ver Pag 77
	if (strcmp(updateType_str, UpdateType_texts[_UT_FULL])==0)
	{
		//Hay que sustituirla. Primero la eliminamos
		emptyAuthorizationList();
		//..y luego añadimos una nueva lista.
		int num=json_object_array_length(obj_localAuthorizationList);

		//Que vamos recorriendo...
		for (int i=0; i<num; i++)
		{

			authorizationDataElement=json_object_array_get_idx(obj_localAuthorizationList, i);
			obj_idTag= json_object_object_get(authorizationDataElement, "idTag");
			obj_idTagInfo=json_object_object_get(authorizationDataElement, "idTagInfo");
			const char *idTag=json_object_get_string(obj_idTag);
			const char *expiryDate=NULL;
			const char *parentIdTag=NULL;
			const char *status_str=NULL;
			int status;

			if (obj_idTagInfo) //optional
			{

				obj_expiryDate=json_object_object_get(obj_idTagInfo, "expiryDate");
				if (obj_expiryDate) expiryDate=json_object_get_string(obj_expiryDate);

				obj_parentIdTag= json_object_object_get(obj_idTagInfo, "parentIdTag");
				if (obj_parentIdTag) parentIdTag=json_object_get_string(obj_parentIdTag);

				obj_status= json_object_object_get(obj_idTagInfo, "status");

				if (enums_as_integers)
				{
					status=json_object_get_int(obj_status);
				}
				else
				{
					status_str=json_object_get_string(obj_status);

					for (int i=0; i<sizeof(AuthorizationStatusTexts)/sizeof(AuthorizationStatusTexts[0]); i++)
					{
						if (strcmp(AuthorizationStatusTexts[i],status_str)) status=i;
					}
				}

				if (!presentConfigurationCSLValue("SupportedFeatureProfiles", "LocalAuthListManagement")) retorno=2;

				if (!isAuthorizationListEnabled()) retorno=1;

				//...y añadiendo una a una
				if (retorno==0)
				{
					int i=add_authorization_list_entry(idTag, expiryDate, parentIdTag, status, version);
				}
			}
		}
	}
	else //De tipo DIFFERENTIAL
	{
		//Recorremos las autorizaciones.
		int num=json_object_array_length(obj_localAuthorizationList);

		//Que vamos recorriendo...
		for (int i=0; i<num; i++)
		{
			authorizationDataElement=json_object_array_get_idx(obj_localAuthorizationList, i);
			obj_idTag= json_object_object_get(authorizationDataElement, "idTag");
			obj_idTagInfo=json_object_object_get(authorizationDataElement, "idTagInfo");
			const char *idTag=json_object_get_string(obj_idTag);
			const char *expiryDate=NULL;
			const char *parentIdTag=NULL;
			const char *status_str=NULL;
			int status;

			if (obj_idTagInfo) //optional
			{
				obj_expiryDate=json_object_object_get(obj_idTagInfo, "expiryDate");
				if (obj_expiryDate) expiryDate=json_object_get_string(obj_expiryDate);

				obj_parentIdTag= json_object_object_get(obj_idTagInfo, "parentIdTag");
				if (obj_parentIdTag) parentIdTag=json_object_get_string(obj_parentIdTag);

				obj_status= json_object_object_get(obj_idTagInfo, "status");

				if (enums_as_integers)
				{
					status=json_object_get_int(obj_status);
				}
				else
				{
					status_str=json_object_get_string(obj_status);

					for (int i=0; i<sizeof(AuthorizationStatusTexts); i++)
					{
						if (strcmp(AuthorizationStatusTexts[i],status_str)) status=i;
					}
				}

				//Ya tenemos toda la info.

				//Actualziar la version.  <-- NOT IMPLEMENTED

				if (!isAuthorizationListEnabled()) retorno=1;

				if (!presentConfigurationCSLValue("SupportedFeatureProfiles", "LocalAuthListManagement")) retorno=2;

				if (retorno==0) retorno=update_authorization_list(idTag, expiryDate, parentIdTag, status,-1); //<--OJO ESTA FUNCION NO ESTA BIEN
			}
		}
	}

	if (debug) show_authorization_list();
	//if (debug) show_cache_list();

	if (retorno==0) write_list_to_disk();

	return prepare_sendlocallist_response(UniqueId_str, retorno);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.16 SET CHARGING PROFILE
//
//connectorId será un valor de 0 a N-1
char *respondSetChargingProfileRequest(const char *UniqueId_str, int connectorId, json_object *obj_chargingProfile, int chargingProfileId, int stackLevel, int transactionId,
		const char *purpose_str, const char * kind_str, const char *recurrency_str, const char *valid_from, const char *valid_to, json_object *obj_chargingSchedule)
{
	//Pag 58: The  Central  System  MAY  send  a  charging  profile  to  a  Charge  Point  to  update  the  charging  profile  for	a transaction.  The  Central  System  SHALL  use  the
	//SetChargingProfile.req PDU  for  that  purpose.  If  a charging   profile   with   the   same	chargingProfileId, or the same combination of stackLevel/ChargingProfilePurpose,
	//exists  on  the  Charge  Point,  the  new  charging  profile  SHALL  replace  the	existing    charging  profile,  otherwise  it  SHALL  be  added.  The  Charge  Point  SHALL
	//then  re-evaluate  its collection of charge profiles to determine which charging profile will become active.

	int status=0;
	int purpose;

	//////////////////////////
	//CREAMOS EL OBJETO CHPR
	/////////////////////////
	struct ChargingProfile *ChPr=(struct ChargingProfile *)calloc(1, sizeof(struct ChargingProfile));

	ChPr->chargingProfileId=chargingProfileId;
	ChPr->stackLevel=stackLevel;

	if (transactionId>0)
	{
		int *tid=(int *)calloc(1, sizeof(int));
		*tid=transactionId;
		ChPr->transactionId=tid;
	}

	if (valid_from)
	{
		strptime(valid_from, "%Y-%m-%dT%H:%M:%S.", ChPr->validFrom);
	}

	if (valid_to)
	{
		strptime(valid_to, "%Y-%m-%dT%H:%M:%S.", ChPr->validTo);
	}

	//purpose - mandatory
	for (int i=0; i<sizeof(ChargingProfilePurposeTypeTexts); i++)
	{
		if (strcmp(ChargingProfilePurposeTypeTexts[i], purpose_str)==0) ChPr->chargingProfilePurpose=i;
		purpose=i;
	}

	//kind - mandatory
	for (int i =0; i<sizeof(ChargingProfileKindTypeTexts); i++)
	{
		if (strcmp(ChargingProfileKindTypeTexts[i], kind_str)==0) ChPr->chargingProfileKind=i;
	}

	//recurrency -optional
	if (recurrency_str)
	{
		for (int i =0; i<sizeof(RecurrencyKindTypeTexts); i++)
		{
			if (strcmp(RecurrencyKindTypeTexts[i], recurrency_str)==0) ChPr->recurrencyKind=i;
		}
	}

	//ChPr->chargingSchedule=(struct ChargingSchedule *)calloc(1, sizeof(struct ChargingSchedule));

	if (obj_chargingSchedule)
	{
		json_object *obj_duration=json_object_object_get(obj_chargingProfile, "duration");
		if (obj_duration) ChPr->chargingSchedule.duration=json_object_get_int(obj_duration);

		json_object *obj_startsched=json_object_object_get(obj_chargingProfile, "startSchedule");

		const char *startsched;
		if (obj_startsched)
		{
			startsched=json_object_get_string(obj_duration);
			strptime(startsched,  "%Y-%m-%dT%H:%M:%S.", ChPr->chargingSchedule.startSchedule);
		}

		json_object *obj_chargingrateunit=json_object_object_get(obj_chargingProfile, "chargingRateUnit");
		ChPr->chargingSchedule.chargingRateUnit=json_object_get_int(obj_chargingrateunit);

		json_object *obj_minchargingrate=json_object_object_get(obj_chargingProfile, "minChargingRate");
		ChPr->chargingSchedule.chargingRateUnit=json_object_get_double(obj_minchargingrate);

		json_object *obj_chargingSchedulePeriod=json_object_object_get(obj_chargingProfile, "chargingSchedulePeriod");

		//OJO ESTO ES UN ARRAY!!!

		if (obj_chargingSchedulePeriod)
		{
			json_object *obj_limit=json_object_object_get(obj_chargingSchedulePeriod, "limit");
			ChPr->chargingSchedule.chargingSchedulePeriods.limit=json_object_get_double(obj_limit);

			json_object *obj_startPeriod=json_object_object_get(obj_chargingSchedulePeriod, "startPeriod");
			ChPr->chargingSchedule.chargingSchedulePeriods.startPeriod=json_object_get_int(obj_startPeriod);

			json_object *obj_numberPhases=json_object_object_get(obj_chargingSchedulePeriod, "numberPhases");
			if (obj_numberPhases) ChPr->chargingSchedule.chargingSchedulePeriods.numPhases=json_object_get_int(obj_numberPhases);

			ChPr->chargingSchedule.chargingSchedulePeriods.next=NULL;
		}
	}

	//////////////////////
	// OBJETO CREADO!!!
	//////////////////////

	if (transactionId>0)
	{
		//Estamos en una transaccion
		//5.16.3
		//Pag 58:In order to ensure that the updated charging profile applies only to the current transaction, the chargingProfilePurpose
		// of the ChargingProfile MUST be set to TxProfile. (See section: Charging Profile Purposes)
		ChPr->chargingProfilePurpose=_CPPT_TXPROFILE;
	}
	else
	{
		//Pag 58: It is not possible to set a ChargingProfile with purpose set to TxProfile without presence of an active transaction, or in advance of a transaction.
		if (ChPr->chargingProfilePurpose==_CPPT_TXPROFILE)
		{
			status=_CPS_REJECTED;
		}
	}

	/////////////////////

	if (status==0)
	{
		//Si aun no ha habido alguna causa para rechazar la actualización del charging profile...

		if (!currentChargingProfiles[connectorId])
		{
			//Actualmente el connector no tenia un charging profile propio. Se lo ponemos.

			if (!addChargingProfile(ChPr, connectorId))
			{
				//Devuelve 1 si se añadio exitosamente
				//Devuelve 0 si NO pudo añadir el charging profile
				status=_CPS_NOTSUPPORTED;
			}
			else status=_CPS_ACCEPTED;
		}
		else
		{
			//Ya hay (al menos) un charging profile. Si es del mismo stackLevel y Purpose, lo sustituimos.

			//Vamos recorreindo los chargingprofiles de ese connector:
			struct ChargingProfileList *temp=currentChargingProfiles[connectorId];
			int found=0;
			while (temp)
			{
				if (temp->chargingProfile)
				{
					//Pag 58: If  a charging   profile   with   the   same	chargingProfileId, or the same combination of stackLevel/ChargingProfilePurpose,
					//exists  on  the  Charge  Point,  the  new  charging  profile  SHALL  replace  the	existing    charging  profile,  otherwise  it  SHALL  be  added.
					if ((temp->chargingProfile->chargingProfileId==chargingProfileId) || ((temp->chargingProfile->stackLevel==stackLevel) && (temp->chargingProfile->chargingProfilePurpose==purpose)))
					{
						found=1;
						replaceChargingProfile(ChPr, connectorId);
					}
				}

				temp=temp->next;
			}

			///...otherwise it shall be added...
			if (!found)
			{
				if (!addChargingProfile(ChPr, connectorId))
				{
					//Devuelve 1 si se añadio exitosamente
					//Devuelve 0 si NO pudo añadir el charging profile
					status=_CPS_NOTSUPPORTED;
				}
				else
				{
					status=_CPS_ACCEPTED;
				}
			}

			//The  Charge  Point  SHALL then  re-evaluate  its collection of charge profiles to determine which charging profile will become active.
			reevaluate_chargingProfiles(connectorId);    //<--NOT IMPLEMENTED
		}
	}

	return prepare_setchargingprofile_response(UniqueId_str, status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.17 TRIGGER MESSAGE
//
//connector es un valor de 0 a N-1
char *manageTriggerMessageRequest(const char *UniqueId_str, int connector, const char *MessageId_Str)
{
	//Pag 59:  if  the  specified  connectorId  is  not	relevant to the message, it should be ignored. In such cases the requested message should still be sent.
	int status;

	if (strcmp(MessageId_Str, MessageTrigger_texts[_MT_BOOTNOTIFICATION])==0)
	{
		send_bootNotification_request(getVendor(), getModel(), getGUIData(cbsntext), getGUIData(cpsntext), getGUIData(fwvertext), getGUIData(iccidtext), getGUIData(imsitext), getGUIData(metersntext), getGUIData(metertypetext));
		status=_TMS_ACCEPTED;
	}
	else if (strcmp(MessageId_Str, MessageTrigger_texts[_MT_DIAGNOSTICSSTATUSNOTIFICATION])==0)
	{
		send_diagnosticsstatus_request(lastDiagnosticsFileStatus);
		status=_TMS_ACCEPTED;
	}
	else if (strcmp(MessageId_Str, MessageTrigger_texts[_MT_FIRMWARESTATUSNOTIFICATION])==0)
	{
		send_firmwareStatusNotification_request(lastFirmwareUploadStatus);
		status=_TMS_ACCEPTED;
	}
	else if (strcmp(MessageId_Str, MessageTrigger_texts[_MT_HEARTBEAT])==0)
	{
		send_heartbeat_request();
		status=_TMS_ACCEPTED;
	}
	else if (strcmp(MessageId_Str, MessageTrigger_texts[_MT_METERVALUES])==0)
	{
		//Pag 60: The  TriggerMessage  mechanism  is  not  intended  to  retrieve  historic  data.  The  messages  it  triggers
		//should  only  give  current  information. A MeterValues message  triggered  in  this  way  for  instance SHOULD  return
		//the  most  recent  measurements (numSampleValues=1) for  all  measurands  configured  in  configuration  key "MeterValuesSampledData".

		status=_TMS_ACCEPTED;

		int numSamplesCK=getConfigurationKeyIntValue("MeterValuesSampledDataMaxLength");
		int numSamples=-1;

		char *sampledData=getConfigurationKeyStringValue("MeterValuesSampledData");

		if (sampledData)
		{
			if (numSamplesCK<0)
			{
				numSamples=getCSLNumElements("MeterValuesSampledData");
			}
			else
			{
				numSamples=MIN(getCSLNumElements("MeterValuesSampledData"),numSamplesCK);
			}

			struct SampledValue value_list[1][numSamples];

			char** measurands;
			measurands= str_split(sampledData, ',');

			for (int i=0; i<numSamples && *measurands; i++)
			{
							struct SampledValue sv;
							sv.unit=-1;
							sv.context=-1;
							sv.location=-1;
							sv.phase=-1;
							sv.formato=-1;
							//sv.value=NULL;
							sv.value=(char *)calloc(1,16);

							if (strcmp(*measurands,"Voltage")==0)
							{
								sv.measurand=_MV_MEASURAND_VOLTAGE;
								sv.unit=_MV_UNIT_PERCENTAGE;
								ftoa(get_Voltage(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Temperature")==0)
							{
								sv.measurand=_MV_MEASURAND_TEMPERATURE;
								sv.unit=_MV_UNIT_CELSIUS;
								ftoa(get_Temperature(), sv.value, 2);
							}
							else if (strcmp(*measurands,"SoC")==0)
							{
								sv.measurand=_MV_MEASURAND_SOC;
								sv.unit=_MV_UNIT_PERCENTAGE;
								ftoa(get_State_Of_Charge(), sv.value, 2);
							}
							else if (strcmp(*measurands,"RPM")==0)
							{
								sv.measurand=_MV_MEASURAND_RPM;
								sv.unit=_MV_UNIT_HZ;
								ftoa(get_Fan_Speed(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Power.Reactive.Import")==0)
							{
								sv.measurand=_MV_MEASURAND_POWER_REACTIVE_IMPORT;
								sv.unit=_MV_UNIT_KVARH;
								ftoa(get_Power_Reactive_Import(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Power.Reactive.Export")==0)
							{
								sv.measurand=_MV_MEASURAND_POWER_REACTIVE_EXPORT;
								sv.unit=_MV_UNIT_KVARH;
								ftoa(get_Power_Reactive_Export(), sv.value, 2);

							}
							else if (strcmp(*measurands,"Power.Offered")==0)
							{
								sv.measurand=_MV_MEASURAND_POWER_OFFERED;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Power_Offered(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Power.Factor")==0)
							{
								sv.measurand=_MV_MEASURAND_POWER_FACTOR;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Power_Factor(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Power.Active.Import")==0)
							{
								sv.measurand=_MV_MEASURAND_POWER_ACTIVE_IMPORT;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_active_power_exported_by_EV(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Power.Active.Export")==0)
							{
								sv.measurand=_MV_MEASURAND_POWER_ACTIVE_EXPORT;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_active_power_exported_by_EV(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Frequency")==0)
							{
								sv.measurand=_MV_MEASURAND_FREQUENCY;
								sv.unit=_MV_UNIT_HZ;
								ftoa(get_Powerline_Frequency(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Reactive.Export.Interval")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_INTERVAL;
								sv.unit=_MV_UNIT_KVARH;
								ftoa(get_Reactive_Exported_Energy_Interval(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Reactive.Import.Interval")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_INTERVAL;
								sv.unit=_MV_UNIT_KVARH;
								ftoa(get_Reactive_Imported_Energy_Interval(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Active.Import.Interval")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_INTERVAL;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Imported_Energy_Interval(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Active.Export.Interval")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_INTERVAL;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Exported_Energy_Interval(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Reactive.Import.Register")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_REGISTER;
								sv.unit=_MV_UNIT_KVARH;
								ftoa(get_Reactive_Energy_Imported_by_EV(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Reactive.Export.Register")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_REGISTER;
								sv.unit=_MV_UNIT_KVARH;
								ftoa(get_Reactive_Energy_Exported_by_EV(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Active.Import.Register")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Energy_Imported_by_EV(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Energy.Active.Export.Register")==0)
							{
								sv.measurand=_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_REGISTER;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Energy_Exported_by_EV(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Current.Offered")==0)
							{
								sv.measurand=_MV_MEASURAND_CURRENT_OFFERED;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Current_Offered(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Current.Import")==0)
							{
								sv.measurand=_MV_MEASURAND_CURRENT_IMPORT;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Current_Import(), sv.value, 2);
							}
							else if (strcmp(*measurands,"Current.Export")==0)
							{
								sv.measurand=_MV_MEASURAND_CURRENT_EXPORT;
								sv.unit=_MV_UNIT_KWH;
								ftoa(get_Current_Export(), sv.value, 2);
							}
							value_list[0][i]=sv;

							*(measurands++);
			}//FOR

			send_metervalues_request(-1, connector, 1, numSampleValues, value_list);

		}
	}
	else if (strcmp(MessageId_Str, MessageTrigger_texts[_MT_STATUSNOTIFICATION])==0)
	{
		//Pag 59: if  the  connectorId  is  relevant  but  absent,  this  should  be  interpreted  as  “for  all  allowed
		//connectorId values”. For example, a request for a statusNotification for connectorId 0 is a request for
		//the  status  of  the  Charge  Point.  A  request  for  a  statusNotification  without  connectorId  is  a  request  for
		//multiple statusNotifications: the notification for the Charge Point itself and a notification for each of its connectors.
		status=_TMS_ACCEPTED;
		if (connector<0)
		{
			//connector value is absent
			for (int i=0; i<num_connectors; i++)
			{
				send_statusnotification_request(i+1, connectorStatus[i], _CP_ERROR_NOERROR, NULL, getVendor(), NULL);
			}
		}
		else
		{
			send_statusnotification_request(connector+1, connectorStatus[connector], _CP_ERROR_NOERROR, NULL, getVendor(), NULL);
		}
	}
	//Pag 59:  If  the  requested  message  is  unknown  or  not  implemented  the  Charge  Point  SHALL  return	NOT_IMPLEMENTED
	else status=_TMS_NOTIMPLEMENTED;

/////////////////
//OJO NO LO HE HECHO ASI PORQUE IMPLICARIA CREAR UN NUEVO HILO Y ESPERAR A QUE SE ENVIARA EL CONF:
	//Pag 59: The  Charge  Point  SHALL  first  send  the  TriggerMessage  response,  before  sending  the  requested
	//message. In the TriggerMessage.conf the Charge Point SHALL indicate whether it will send it or not, by
	//returning  ACCEPTED  or  REJECTED.  It  is  up  to  the  Charge  Point  if  it  accepts  or  rejects  the  request  to
	//send (EN FUNCION DE QUE¿¿???).

	//Messages  that  the  Charge  Point  marks  as  accepted  SHOULD  be  sent.  The  situation  could  occur  that,
	//between  accepting  the  request  and  actually  sending  the  requested  message,  that  same  message  gets
	//sent  because  of  normal  operations.  In  such  cases  the  message  just  sent  MAY  be  considered  as
	//complying with the request.

	//Esto significa que, actualmente, envia antes todo lo solicitado y luego la respuesta del trigger
//////////////////

	return prepare_triggermessage_response(UniqueId_str, status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.18 UNLOCK CONNECTOR
//
char *manageUnlockConnectorRequest(const char *UniqueId_str, int connector)
{
	//Pag 60: Upon receipt of an UnlockConnector.req PDU, the Charge Point SHALL respond with a	UnlockConnector.conf
	//  PDU.  The  response  PDU  SHALL  indicate  whether  the  Charge  Point  was  able  to unlock its connector.
	int status;

	//Ver pag 107: "NotSupported: ChargePoint has no connectorLock"
	if (hasConnectorLock(connector))
	{
		status=_US_NOTSUPPORTED;
	}
	else if (transactions[connector])
	{
		int transactionId=transactions[connector]->transactionId;

		//Lo eliminamos de la estructura interna
		removeTransaction(connector);

		//Stop physically the transaction:
		stopTransaction(connector); //<--NOT IMPLEMENTED

		updateStopTransactionInGUI();
		int meterStopValue=connectorValues[connector];

		send_stoptransaction_request(meterStopValue, transactionId, _ST_UNLOCK_COMMAND, NULL);

		int couldUnlock=unlockCable(connector);

		if (couldUnlock)
		{
			status=_US_UNLOCKED;
		}
		else
		{
			status=_US_UNLOCKFAILED;
			currentChargePointErrorStatus=_CP_ERROR_CONNECTORLOCKFAILURE;
		}
	}
	else
	{
		int couldUnlock=unlockCable(connector);

		if (couldUnlock)
		{
			status=_US_UNLOCKED;
		}
		else
		{
			status=_US_UNLOCKFAILED;
			currentChargePointState=_CP_STATUS_FAULTED;
			currentChargePointErrorStatus=_CP_ERROR_CONNECTORLOCKFAILURE;
		}
	}

	return prepare_unlockconnector_response(UniqueId_str, status);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.19 UPDATE FIRMWARE
//
char *manageUpdateFirmwareRequest(const char *UniqueId_str, const char *location, const char *retrieveDate, int retries, int retryInterval)
{
	//Upon receipt of an UpdateFirmware.req PDU, the Charge Point SHALL respond with an UpdateFirmware.conf
	//PDU. The Charge Point SHOULD start retrieving the firmware as soon as possible after retrieve-date.

	struct ftp_parameters *parameters=calloc(1, sizeof(struct ftp_parameters));
	parameters->location=strdup(location);
	parameters->retries=retries;
	parameters->retryInterval=retryInterval;
	parameters->startTime=NULL;
	parameters->stopTime=NULL;
	parameters->originalfilename=NULL;

	char *ftpu=getConfigurationKeyStringValue("FTPUser");
	if (ftpu) parameters->username=strdup(ftpu);
	else parameters->username=strdup("OCPP");

	char *ftpp=getConfigurationKeyStringValue("FTPPassword");
	if (ftpp) parameters->username=strdup(ftpp);
	else parameters->username=strdup("OCPP");

	if (retrieveDate) parameters->startTime=strdup(retrieveDate);

	//Creamos el hilo que actualizará el firmware
	pthread_t pidFTP;

	pthread_create(&pidFTP, NULL, firmwareUpdate, parameters);
	pthread_detach(pidFTP);

	//Pag 52:
	//Upon receipt of a	GetDiagnostics.req PDU, and if diagnostics information is available then Charge Point
	//SHALL  respond  with  a	GetDiagnostics.conf  PDU  stating  the  name  of  the  file  containing  the  diagnostic
	//information that will be uploaded. Charge Point SHALL upload a single file.

	return prepare_updatefirmware_response(UniqueId_str);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//Called for every file line
int handler22(void* user, const char* section, const char* name,const char* value)
{
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0



	//PROTOCOL
   if (MATCH("Protocol", "Version")) {
	   modifyConfigurationKey("ProtocolVersion",strndup(value, strlen(value)-1));
   } else if (MATCH("Protocol", "Name")) {
	   modifyConfigurationKey("ProtocolName",strndup(value, strlen(value)-1));

//CHARGEPOINT
   } else if (MATCH("ChargePoint", "Vendor")) {
	   modifyConfigurationKey("ChargePointVendor",strndup(value, strlen(value)-1));
   } else   if (MATCH("ChargePoint", "Model")) {
	   modifyConfigurationKey("ChargePointModel",strndup(value, strlen(value)-1));
   } else   if (MATCH("ChargePoint", "SerialNumber")) {
	   modifyConfigurationKey("ChargePointSerialNumber",strndup(value, strlen(value)-1));
   } else   if (MATCH("ChargePoint", "FirmwareVersion")) {
	   modifyConfigurationKey("FirmwareVersion",strndup(value, strlen(value)-1));
   } else   if (MATCH("ChargePoint", "ChargeBoxSerialNumber")) {
	    modifyConfigurationKey("ChargeBoxSerialNumber",strndup(value, strlen(value)-1));
   } else	if (MATCH("ChargePoint", "ICCID")) {
	   modifyConfigurationKey("ChargePointICCID",strndup(value, strlen(value)-1));
   } else  if (MATCH("ChargePoint", "IMSI")) {
	   modifyConfigurationKey("ChargePointIMSI",strndup(value, strlen(value)-1));
   } else   if (MATCH("ChargePoint", "MeterType")) {
	   modifyConfigurationKey("ChargePointMeterType",strndup(value, strlen(value)-1));
   } else  if (MATCH("ChargePoint", "MeterSerialNumber")) {
	   modifyConfigurationKey("ChargePointMeterSerialNumber",strndup(value, strlen(value)-1));
   } else  if (MATCH("ChargePoint", "VendorID")) {
	   modifyConfigurationKey("ChargePointVendorID",strndup(value, strlen(value)-1));
   //CHARGEBOX
   } else  if (MATCH("ChargeBox", "CB_Endpoint_IP")) {
	   modifyConfigurationKey("ChargeBoxIP",strndup(value, strlen(value)-1));
  //     __CBENDPOINTIP = strndup(value, strlen(value)-1);
   } else  if (MATCH("ChargeBox", "CB_Endpoint_Port")) {
	   modifyConfigurationKey("ChargeBoxPort",strndup(value, strlen(value)-1));
   //    __CBENDPOINTPORT = strndup(value, strlen(value)-1);
   } else  if (MATCH("ChargeBox", "CB_Endpoint_URL")) {
	   modifyConfigurationKey("ChargeBoxURL",strndup(value, strlen(value)-1));
   //    __CBENDPOINTURL = strndup(value, strlen(value)-1);
   } else  if (MATCH("ChargeBox", "ID")) {
	   modifyConfigurationKey("ChargeBoxID",strndup(value, strlen(value)-1));
   } else  if (MATCH("ChargeBox", "PermanentAttachment")) {
	   modifyConfigurationKey("PermanentAttachment",strndup(value, strlen(value)-1));

   //CENTRAL SYSTEM
   } else  if (MATCH("CentralSystem", "CS_Endpoint_IP")) {
	   modifyConfigurationKey("CentralSystemIP",strndup(value, strlen(value)-1));
   } else  if (MATCH("CentralSystem", "CS_Endpoint_Port")) {
	   modifyConfigurationKey("CentralSystemPort",strndup(value, strlen(value)-1));
   } else  if (MATCH("CentralSystem", "CS_Endpoint_URL")) {
	   modifyConfigurationKey("CentralSystemURL",strndup(value, strlen(value)-1));

   } else {

	   for (int k=0; k<CONFIGURATION_KEY_LIST_SIZE; k++)
	   {
		   if (configurationKeys[k])
		   {
			   if (MATCH("Configuration", configurationKeys[k]))
			   {
				   modifyConfigurationKey(configurationKeys[k],strndup(value, strlen(value)-1));
			//	   printf("\nLA CLAVE ES: %s",configurationKeys[k]);
		//		   if (configurationValues[k].stringValue) printf("\nVALUE ES: %s",configurationValues[k].stringValue);
			//	   printf("\nINT VALUE ES: %d",configurationValues[k].intValue);
				   return 1;
			   }
		   }
	   }

//	   printf("\nERROR con Name: %s y value: %s", name, value);
	   return 0; // 0 es error
   }

   return 1;  // 1 es OK
}

//Se pide iniciar físicamente la transaccion, cambia el estado del conector, bloquea el cable, envia el mensaje
//y comienza a proporcionar energia.
//connector es un valor de 0 a N-1
void startTransaction(int connector, const char *idTag)
{
	lockCable(connector);
	//Con esto comienza el thread a contar
	changeConnectorStatus(connector, _CP_STATUS_CHARGING);


	printf("\nAQUI LLAMAMOS AL HILO SEND PERIODIC");
	//Create a thread that sends meterValues messages every "MeterValuesSampleInterval". See 9.1.15
 	pthread_t meterValuesThread;
 	pthread_create(&meterValuesThread, NULL, sendPeriodicMeterValues, connector);
	pthread_detach(meterValuesThread);

	//Other tasks that should be implemented when physically stopping a transaction
	// NOT IMPLEMENTED

	//Mandamos aviso al dispositivo para que abra el grifo:
	supplyEnergy(connector);
	//updates GUI. Actualiza el combo con los conectores que estan bajo transaccion
	startTransactionInGUI(connector, idTag);
}

