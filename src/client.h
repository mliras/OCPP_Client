/*
 * client.h
 *
 *  Created on: Nov 8, 2017
 *      Author: root
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdio.h>
#include "ocpp_gtk.h"
#include "aux.h"
#include "HardwareInterface.h"
#include "exploreNFC.h"
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include "neardal/neardal.h"

#define _XOPEN_SOURCE 500
//Para este caso solo tenemos dos conectores
#define NUM_CONNECTORS 1

//For reset
int original_argc;
char **original_argv;


int OKReceived[NUM_CONNECTORS];

//For websockets and heartbeat
static int destroy_flag = 0;
static int connection_flag = 0;
static int writeable_flag = 0;
int anothermessagesent;
int sendConnectorStatusWhenOnline;

static int currentTransaction=-1;
static char *currentIdTag=NULL;

static int initializedConnection=0;
static int writable=0;

static int UniqueId=100000;
static int lastSentAcceptedUniqueId=100000;

int lastDiagnosticsFileStatus;
int lastFirmwareUploadStatus;

//7.6
int currentChargePointErrorStatus;
int currentConnectorErrorStatus[NUM_CONNECTORS];

//7.7
int currentChargePointState;

//El connector 1 esta en la posicion 0
int connectorStatus[NUM_CONNECTORS];
double connectorValues[NUM_CONNECTORS];

////////////////////////////////////////////////
//	WEBSOCKETS
////////////////////////////////////////////////
struct session_data {
    int fd;
};

struct pthread_routine_tool {
    struct lws_context *context;
    struct lws *wsi;
};

//Esta funcion realiza todo el proceso de inicialización por websockets con el servidor (sistema central)
void websocket_connect(struct lws_context **context2, struct lws **wsi2);

//Esta función se utiliza para enviar un mensaje (ristras de caracteres definida por str) al websocket (definido por wsi_in)
static int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in);

//Esta funcion contiene el bucle principal del programa. Recibe los mensajes y los despacha a donde debe, finalmente envía los mensajes que tenga en la cola.
//Es lanzado por el hilo principal del cliente.
static int service_callback(struct lws *wsi,enum lws_callback_reasons reason, void *user,void *in, size_t len);

////////////////////////////////////////////////////////
//          THREADS
////////////////////////////////////////////////////////
//Esta función es llamada por el hilo que reinicia el Chargepoint. Basicamente escribe toda la información a disco, corta la comunicación con el servidor y envia una señal al dispositivo
//para que se reinicie. Se trata de un reinicio software solicitado por el sistema central mediante un mensaje reset.
static void *rebootChargePoint(struct lws* wsi);

//Esta funcion es llamada por el hilo que inicia una transacción. Esta funcion realiza lo indicado en la página 20. Lee los valores  de configuración TransactionMessageRetryInterval
//y TransactionMessageAttempts e intenta el inicio de la transacción según lo indicado. Se tiene que realizar en un hilo porque se queda esperando un tiempo.
static void *sendTransactionRequests(struct lws* wsi);
static void *hilo_conexion(void *tool_in);

////////////////////////////////////////////////////////
//          CHARGEPOINT INITIALIZATION
////////////////////////////////////////////////////////
int CP_initialize();
int readCPStatusFromFile();

////////////////////////////////////////////////////////
//          CHARGEPOINT CHARACTERISTICS
////////////////////////////////////////////////////////
//Simplemente devuelve True. Esta cogido del punto 5.11 de [1]. La idea es que si no queremos que funcionen las características de ChargingPRofiles, etc.
//Esta función devuelva cero. Aun no esta implementado pero lo ideal es que reciba el valor desde el fichero de configuración .INI.
int CPhasSupportForSmartCharging();

//Simplemente devuelve True(1). Esta cogido del punto 7.46 de [1]. Aun no esta implementado pero lo ideal es que reciba el valor desde el fichero de configuración .INI.
int hasConnectorLock(int connector);

//Devuelve 1 si el conector esta continuamente attached y no se puede desbloquear. Es lo que hace ahora mismo. Esto se podría configurar a partir de ConfigurationKey
int isConnectorPermanentlyAttached();

//Devuelve el siguiente UniqueId siempre que se haya recibido su respuesta
int getNextUniqueID();

//Similar al nterior pero devuelve una caedna con el numero. Utilizado para meterlo directamente en el mensaje JSON.
char *getNextUniqueID_char();

#endif /* CLIENT_H_ */
