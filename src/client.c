/*
 ============================================================================
 Name        : client.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>
#include "client.h"
#include "ini_parser.h"
#include <time.h>
#include "ftpDiagnostics.h"
#include <openssl/x509.h>

char crl_path[1024] = "";

void synchronizeTime(char *time)
{
	//NOT IMPLEMENTED
}

static void INT_HANDLER(int signo)
{
    destroy_flag = 1;
}

//Simplemente devuelve True. Esta cogido del punto 5.11 de [1]. La idea es que si no queremos que funcionen las características de ChargingPRofiles, etc.
//Esta función devuelva cero. Aun no esta implementado pero lo ideal es que reciba el valor desde el fichero de configuración .INI.
int CPhasSupportForSmartCharging()
{
	return 1; //<-- NOT IMPLEMENTED. Taken from the paragraph in 5.11. Used by respondRemoteStartTransactionRequest()
}

//Simplemente devuelve True. Esta cogido del punto 7.46 de [1]. Aun no esta implementado pero lo ideal es que reciba el valor desde el fichero de configuración .INI.
int hasConnectorLock(int connector)
{
	return 1; //<-- NOT IMPLEMENTED. Taken from the section 7.46. NotSupported: ChargePoint has no connectorLock. SHOULD BE A CONFIGURATION KEY???
}

//Devuelve 1 si el conector esta continuamente attached y no se puede desbloquear. Es lo que hace ahora mismo. Esto se podría configurar a partir de ConfigurationKey
int isConnectorPermanentlyAttached()
{
	return getConfigurationKeyIntValue("permanentAttachment");
}

//Devuelve el siguiente UniqueId siempre que se haya recibido su respuesta
int getNextUniqueID()
{
		return UniqueId++;
}

char *getNextUniqueID_char(){
	char *b=(char *) calloc(1, sizeof(char) * 8);
	sprintf(b, "%d", UniqueId++);
	return b;
}

char *updateUniqueId(char *payload, char *currentUniqueId)
{
	//int u=getNextUniqueID();
	char *newUniqueId=getNextUniqueID_char();

	return replace(payload,currentUniqueId,newUniqueId);
}


//
//str es la cadena, en claro que se va a mandar
//str_size_in es su longitud (podría ser -1, en cuyo caso, hay que calcularlo)
//Devuelve -1 si hubo algún error y 0 si se realizó el envío correctamente
//
static int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in)
{
    if (str == NULL || wsi_in == NULL) return -1;

    int n;
    int len;
    char *out = NULL;

    //Si str_size_in es -1, lo calculamos con strlen
    if (str_size_in < 1)
        len = strlen(str);
    else
        len = str_size_in;

    //Reservamos memoria para la cadena encriptada
        out = (char *)calloc(1, sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));

//////////////////////////////////////////////////////////////////////////
//      printf("TEXTO en CLARO: %s", str);
//Se hace un doble encriptado del texto enviado:
//      char *textoencriptado=encrypt(encrypt(str,13),12);
//      printf("TEXTO CIFRADO: %s", textoencriptado);
//    preparamos el buffer
//    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, textoencriptado, len );
//////////////////////////////////////////////////////////////////////////

    // preparamos el buffer
    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );

    // Escribimos los datos
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

 //   addLog("[Datos enviados al websocket] %s", str,NULL,LOG_DEBUG, ANY);

    // Limpiamos el buffer
    if (out) free(out);

    return n;
}

//Esta es la funcion principal que se ejecuta cada 50 ms y que comprueba el estado del websocket.
//El websocket puede estar en los estados:
//LWS_CALLBACK_CLIENT_ESTABLISHED (cuando se conecta con el servidor)
//LWS_CALLBACK_CLIENT_CONNECTION_ERROR (Cuando se produce un error en la conexion con el servidor)
//LWS_CALLBACK_CLOSED (Cuando se termina la conexion de forma ordenada)
//LWS_CALLBACK_CLIENT_RECEIVE (Cuando se ha recibido un mensaje)
//LWS_CALLBACK_CLIENT_WRITEABLE (Cuando hay algun mensaje para enviar)
static int service_callback(struct lws *wsi,enum lws_callback_reasons reason, void *user,void *in, size_t len)
{
	char *bootnotificationreply;
	int uniqueId;
	enum json_tokener_error err;
	json_object * jobj;

    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: //Conexion establecida.
        	addLog("[Callback] CONEXION ESTABLECIDA CON EL SISTEMA CENTRAL.", NULL,NULL,LOG_INFO, ANY);
        	//PAG 35
        	//The  Charge  Point  SHALL  send  a BootNotification.req PDU  each  time  it  boots  or  reboots.  Between  the
        	//physical  power-on/reboot  and  the  successful  completion  of  a  BootNotification,  where  Central  System
        	//returns Accepted or Pending,  the  Charge  Point  SHALL  NOT  send  any  other  request  to  the  Central
        	//System. This includes cached messages that are still present in the Charge Point from before.
        	bootnotificationreply=prepare_bootNotification_request(getConfigurationKeyStringValue("ChargePointVendor"),getConfigurationKeyStringValue("ChargePointModel"),
        			getConfigurationKeyStringValue("ChargeBoxSerialNumber"),getConfigurationKeyStringValue("ChargePointSerialNumber"),
					getConfigurationKeyStringValue("FirmwareVersion"),getConfigurationKeyStringValue("ChargePointICCID"), getConfigurationKeyStringValue("ChargePointIMSI"),
					getConfigurationKeyStringValue("ChargePointMeterSerialNumber"),getConfigurationKeyStringValue("ChargePointMeterType"));

        	websocket_write_back(wsi, bootnotificationreply, -1);
        	communicationStatus=ONLINE;
        	connection_flag = 1;
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        	addLog("[Callback] Error en la conexion con el servidor.", NULL,NULL,LOG_ERROR, ANY);
        	//printf("\nLWS_CALLBACK_CLIENT_CONNECTION_ERROR");
        	//Pag 15: In the event of unavailability of the communications or even of the Central System, the Charge Point is designed to operate stand-alone.
        	//destroy_flag = 1;  //<--Cuando se desconecta del servidor no podemos cerrar la conexion...
            connection_flag = 0;
            communicationStatus=OFFLINE;
            _REGISTRATIONSTATUS=_RS_NOTCONNECTED;
            break;

        case LWS_CALLBACK_CLOSED:
        	addLog("[Callback] CONNECTION CLOSED", NULL, NULL, LOG_ERROR, ANY);

        	//Pag 15: In the event of unavailability of the communications or even of thine Central System, the Charge Point is designed to operate stand-alone.
        	//destroy_flag = 1;  //<--Cuando se desconecta del servidor no podemos cerrar la conexion...
            connection_flag = 0;
            communicationStatus=OFFLINE;
            _REGISTRATIONSTATUS=_RS_NOTCONNECTED;
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: //Cuando recibe datos
           if (_REGISTRATIONSTATUS!=_RS_REJECTED)
           {
        	int error=0;

        	char *p=(char *)in;
        	for (int i=0; i<len; i++) p[i]+=128;

        	jobj = json_tokener_parse_verbose((char *)in, &err);
        	int do_not_dequeue=0; //Que desencole todo

        	if (jobj!=NULL)
        	{
        	   json_object *obj_MessageTypeId=NULL;
       	       json_object *obj_UniqueId=NULL;
        	   json_object *obj_Payload=NULL;

        		if (send_tags)
        		{
       		        //Comprobamos que es una respuesta
       		        obj_MessageTypeId = json_object_object_get(jobj, "MessageTypeId");
       		        obj_UniqueId = json_object_object_get(jobj, "UniqueId");
        		}
        		else
        		{
        			obj_MessageTypeId=json_object_array_get_idx(jobj, 0);
        			obj_UniqueId=json_object_array_get_idx(jobj, 1);
        		}

        		if (obj_UniqueId && obj_MessageTypeId)
        		{
        			int messageType=json_object_get_int(obj_MessageTypeId);

        		    /////////////////////////////////////////////////////////////////
        		    //      Si recibimos... RESPUESTAS  (de peticiones 4.X)   ///////
        		    /////////////////////////////////////////////////////////////////
        		    if (messageType==_CALLRESULT)
        		    {
        	        	if (send_tags) obj_Payload = json_object_object_get(jobj, "Payload");
        	        	else obj_Payload=json_object_array_get_idx(jobj, 2);

        		        //Declaracion de variables y punteros que usaremos
        		        json_object *obj_idTagInfo, *obj_status, *req;
        		        json_object *obj_data=NULL;
        		        json_object *obj_time=NULL;
        		        const char *status_str;
        		        const char *time=NULL;
        		        const char *data=NULL;
        		        int status;

        		        //Extraemos el UniqueId
        		        const char *UniqueId_str=json_object_get_string(obj_UniqueId);
        		        int action_code=getActionFromUniqueId(UniqueId_str);

        		        //Log. Evitamos logar los heartbeats
        		        if (in && action_code!=406) addLog("[Callback] El client recibio la respuesta: %s.", (char *)in,NULL, LOG_INFO, ANY);

   ////////////////////////////////////////////////////////////////////////////////
        		        //     SWITCH PETICIONES
        		        //Comenzamos el switch principal
        		        switch (action_code)
        		        {
        		        	//4.1 AUTHORIZE
        		        	case AUTHORIZE:

        		        		//If Charge Point has implemented an Authorization Cache, then upon receipt of an Authorize.conf PDU
								//the Charge Point SHALL update the cache entry, if the idTag is not in the Local Authorization List, with
        		        		//the IdTagInfo value from the response as described under Authorization Cache.
        		        		obj_idTagInfo = json_object_object_get(obj_Payload, "idTagInfo");

        		        		if (obj_idTagInfo)
        		        		{
        		        			//OBTENEMOS EL IDTAG QUE GENERO LA PETICION
									char *request=getPayloadFromMessage(atoi(UniqueId_str));
									json_object *obj_req = json_tokener_parse_verbose(request, &err);

									//Lo que necesitamos es el payload asi que...
									json_object *obj_reqpayl;
									if (send_tags)
									{
										obj_reqpayl= json_object_object_get(obj_req , "Payload");
									}
									else
									{
										obj_reqpayl=json_object_array_get_idx(obj_req, 3);
									}


									//Y ahora cogemos todo del payload
									json_object *obj_idTag= json_object_object_get(obj_reqpayl, "idTag");
									const char *IdTag=NULL;
									if (obj_idTag) IdTag = json_object_get_string(obj_idTag);
									//Extraemos los datos de la respuesta
									int status;
									const char *status_str=NULL;
									const char *expiryDate=NULL;
									const char *parentIdTag=NULL;
									json_object *obj_expiry= json_object_object_get(obj_idTagInfo, "expiryDate");
									json_object *obj_status= json_object_object_get(obj_idTagInfo, "status");
									json_object *obj_parentIdTag = json_object_object_get(obj_idTagInfo, "parentIdTag");

									if (obj_expiry) expiryDate = json_object_get_string(obj_expiry);

									if (enums_as_integers)
									{
										if (obj_status) status=json_object_get_int(obj_status);
										status_str=AuthorizationStatusTexts[status];
									}
									else
									{
										if (obj_status) status_str=json_object_get_string(obj_status);
									}

									if (obj_parentIdTag) parentIdTag=json_object_get_string(obj_parentIdTag);

									addLog("[Reply] AUTHORIZE MESSAGE REPLIED. UPDATING LISTS", NULL,NULL,LOG_INFO, ANY);

									//Si nos llega una autorizacion rechazada y hubieramos pre-autorizado el login, le echamos
									if ((strcmp(status_str,"Blocked")==0)||(strcmp(status_str,"Expired")==0)||(strcmp(status_str,"Invalid")==0))
									{
										if (isLoggedIn(IdTag))
										{
											LogOut(IdTag);
										}
									}

									if (isAuthorizationListEnabled())
									{
										update_authorization_list(IdTag, expiryDate, parentIdTag, status,-1,0);
										//show_authorization_list();
									}

									if (isAuthorizationCacheEnabled())
									{
										update_authorization_cache(IdTag, expiryDate,parentIdTag, status);

										if (debug) show_authorization_cache();
									}
        		        		 }
        		        		 else
        		        		 {
        		        		 		sendErrorMessage(UniqueId_str, _PROTOCOL_ERROR);
        		        		 }

        		        		break;

        		        	case BOOT_NOTIFICATION:
        		        	//PAG 35
        		            //When  the  Central  System  responds  with  a BootNotification.conf with  a  status Accepted ,  the  Charge Point
        		            //will adjust the heartbeat interval in accordance with the interval from the response PDU

        		        	//and it is RECOMMENDED  to
        		            //synchronize  its  internal  clock  with  the  supplied  Central  System’s  current  time.
        		        	//4.2

        		        		obj_status = json_object_object_get(obj_Payload, "status");
        		        		if (enums_as_integers)
        		        		{
        		        			status=json_object_get_int(obj_status);
        		        			status_str=strdup(Registration_Status_texts[status]);
        		        	//		if (debug) printf("\n[LOG] enums as integers status_str ES: %s (ACCEPTED ES 1).", status_str);
        		        		}
        		        		else
        		        		{
        		        			status_str=json_object_get_string(obj_status);
        		        		//	if (debug) printf("\n[LOG] enums as cadenas status_str ES: %s (ACCEPTED ES 1).", status_str);
        		        		}

        		        		json_object *obj_HeartbeatInterval = json_object_object_get(obj_Payload, "heartbeatInterval");
        		        		int h=json_object_get_int(obj_HeartbeatInterval);
        		        		if (h>0) modifyConfigurationKey("HeartbeatInterval", convert(h));

        		        		if (strcmp(status_str, "Accepted")==0)
        		        		{
        		        			addLog("\n[Reply] BOOT NOTIFICATION MESSAGE REPLIED. CONNECTION ACCEPTED\n", NULL,NULL,LOG_INFO, ANY);
        		        			_REGISTRATIONSTATUS=_RS_ACCEPTED;
        		        			sendToDisplay("Conexion con el sistema central realizada con exito");
        		        			//synchronizeInternalClock(); //<--NOT IMPLEMENTED
        		        		}
        		        		else
        		        		{
                		        	//If the Central System returns something other than Accepted, the value of the interval field indicates the
                		        	//minimum  wait  time  before  sending  a  next  BootNotification  request.  If  that  interval  value  is  zero,
        		        			//the Charge  Point  chooses  a  waiting  interval  on  its  own,  in  a  way  that  avoids  flooding  the
        		        			//Central  System with requests. A Charge Point SHOULD NOT send a BootNotification.req
                		        	// earlier, unless requested to do so with a TriggerMessage.req

        		        			if (strcmp(status_str, "Rejected")==0)
        		        			{
        		        				//If the Central System returns the status Rejected, the Charge Point SHALL NOT send any OCPP message
        		        				//to the Central System until the aforementioned retry interval has expired. During this interval the
        		        				//Charge Point may no longer be reachable from the Central System.
        		        				//
        		        				//It MAY for instance close its
        		        				//communication channel or shut down its communication hardware. Also the Central System MAY close
        		        				//the communication channel, for instance to free up system resources. While Rejected, the Charge Point
        		        				//SHALL NOT respond to any Central System initiated message. the Central System SHOULD NOT initiate any.

        		        				//NOT IMPLEMENTED
        		        				addLog("\n[Reply] BOOT NOTIFICATION MESSAGE REPLIED. CONNECTION REJECTED\n", NULL,NULL,LOG_ERROR, ANY);
        		        				_REGISTRATIONSTATUS=_RS_REJECTED;
        		        			}

        		        			if (strcmp(status_str, "Pending")==0)
        		        			{
        		        				//The Central System MAY also return a Pending registration status to indicate that it wants to retrieve or
        		        				//set certain information on the Charge Point before the Central System will accept the Charge Point. If
        		        				//the Central System returns the Pending status, the communication channel SHOULD NOT be closed by
        		        				//either  the  Charge  Point  or  the  Central  System.  The  Central  System  MAY  send  request  messages  to
        		        				//retrieve  information  from  the  Charge  Point  or  change  its  configuration.  The  Charge  Point  SHOULD
        		        				//respond to these messages. The Charge Point SHALL NOT send request messages to the Central System
        		        				//unless it has been instructed by the Central System to do so with a TriggerMessage.req request.

        		        				_REGISTRATIONSTATUS=_RS_PENDING;
        		        			}
        		        		}

        		        		break;
        		        	case DATA_TRANSFER:
        		        	     //4.3

        		        		obj_status=json_object_object_get(obj_Payload, "status");
        		        		status=json_object_get_int(obj_status);

        		        		obj_data=json_object_object_get(obj_Payload, "data");
        		        		data=json_object_get_string(obj_data);
        		        		//CHARGEPOINT SPECIFIC BEHAVIOUR
        		        		//NOTHING DONE HERE
        		        		addLog("\n[Reply] DATA TRANSFER MESSAGE REPLIED.\n", NULL,NULL,LOG_INFO, ANY);

        		        		 break;
        		        	case HEARTBEAT:
        		        	     //4.6
        		        	     obj_time=json_object_object_get(obj_Payload, "currentTime");
        		        	     if (obj_time) time=json_object_get_string(obj_time);
        		        	     synchronizeTime(time);

        		        	     break;
        		        	case START_TRANSACTION:
        		        		//4.8
        		        		//If Charge Point has implemented an Authorization Cache, then upon receipt of a StartTransaction.conf
        		        		//PDU the Charge Point SHALL update the cache entry, if the idTag is not in the Local Authorization List,
        		        		//with the IdTagInfo value from the response as described under Authorization Cache.

        		        		obj_idTagInfo = json_object_object_get(obj_Payload, "idTagInfo");
        		        		obj_status=json_object_object_get(obj_idTagInfo, "status");
        		        		status=json_object_get_int(obj_status);

        		        		//Lo que necesitamos es el payload asi que...
    		        			//Cogemos los datos que nos faltan de la peticion
    		        			char *request=getPayloadFromMessage(atoi(UniqueId_str));
    		        			json_object *req = json_tokener_parse_verbose(request, &err);
        		        		json_object *obj_reqpayl;
        		        		if (send_tags)
        		        		{
        		        			obj_reqpayl= json_object_object_get(req , "Payload");
        		        		}
        		        		else
        		        		{
        		        			obj_reqpayl=json_object_array_get_idx(req, 3);
        		        		}

        		        		if (status==_CP_AUTHORIZATION_ACCEPTED)
        		        		{
        		        			addLog("\n[Reply] START TRANSACTION MESSAGE REPLIED. TRANSACTION AUTHORIZED\n", NULL,NULL,LOG_WARNING, ANY);
        		        			json_object *obj_transaction = json_object_object_get(obj_Payload, "transactionId");
        		        			currentTransaction=json_object_get_int(obj_transaction);
        		        			json_object *idToken = json_object_object_get(obj_idTagInfo, "parentIdTag");
        		        			const char *parentIdTag=json_object_get_string(idToken);

        		        			json_object *obj_expiryDate = json_object_object_get(obj_idTagInfo, "expiryDate");
        		        			const char *expiryDate_str=json_object_get_string(obj_expiryDate);

        		        			if (obj_reqpayl)
        		        			{
										//connector
										json_object *obj_connector = json_object_object_get(obj_reqpayl, "connectorId");
										int connectorId=json_object_get_int(obj_connector);
										OKReceived[connectorId-1]=1;
										//idTag
										json_object *obj_idTag = json_object_object_get(obj_reqpayl, "idTag");
										const char *idTag=json_object_get_string(obj_idTag);

										//meterStart
										json_object *obj_meter = json_object_object_get(obj_reqpayl, "meterStart");
										int meterStart=json_object_get_int(obj_meter);

										//timeStamp
										json_object *obj_time = json_object_object_get(obj_reqpayl, "timestamp");
										const char *timeStamp=json_object_get_string(obj_time);

										//conector Id es un valor de 1 a N
										addTransaction(connectorId, idTag, meterStart, timeStamp, parentIdTag, expiryDate_str, currentTransaction);

										if (debug) showTransactions();
										sendToDisplay("Starting transaction....");
										//Pag 40: If Charge Point has implemented an Authorization Cache, then upon receipt of a
										//StartTransaction.conf PDU the Charge Point SHALL update the cache entry, if the idTag is not in the
										//Local Authorization List, with the IdTagInfo value from the response as described on Pag 16.

										if (isAuthorizationCacheEnabled())
										{
											if (!checkValidAuthorizationListEntry(currentIdTag))
											{
												update_authorization_cache(idTag, expiryDate_str, parentIdTag, status);
											}
										}
        		        			}//req
        		        		}//accepted
        		        		else
        		        		{
        		        			//Pag 20: When  the  Charge  Point  encounters  a  first  failure  to  deliver  a  certain
        		        			//transaction-related  message,  it
        		        			//SHOULD send this message again as long as it keeps resulting in a failure to process the
        		        			//message
        		        			addLog("\n[Reply] START TRANSACTION MESSAGE REPLIED. TRANSACTION REJECTED\n", NULL,NULL,LOG_ERROR, ANY);
        		        			if (obj_reqpayl)
        		        			{
										//connector
										json_object *obj_connector = json_object_object_get(obj_reqpayl, "connectorId");
										int connectorId=json_object_get_int(obj_connector);
										OKReceived[connectorId-1]=0;
        		        			}

        		        			//do_not_dequeue=1;
        		        		}

        		        		break;

        		        	case STOP_TRANSACTION:
        		        	//4.10.. recibimos respuesta de una peticion de parada de transaccion.
        		        		//If Charge Point has implemented an Authorization Cache, then upon receipt of aStopTransaction.conf
        		        		//PDU the Charge Point SHALL update the cache entry, if the idTag is not in the Local Authorization List,
        		        		//with the IdTagInfo value from the response as described under Authorization Cache.

        		        		addLog("[Reply] STOP TRANSACTION MESSAGE REPLIED", NULL,NULL,LOG_INFO, ANY);

        		        	    obj_idTagInfo = json_object_object_get(obj_Payload, "idTagInfo");

        		        	    if (obj_idTagInfo)
        		        	    {
        		        	    	json_object *obj_expiryDate = json_object_object_get(obj_idTagInfo, "expiryDate");
        		        	    	const char *expiryDate=json_object_get_string(obj_expiryDate);

        		        	    	json_object *obj_parentIdTag = json_object_object_get(obj_idTagInfo, "parentIdTag");
        		        	    	const char *parentIdTag=json_object_get_string(obj_parentIdTag);

        		        	        json_object *obj_status = json_object_object_get(obj_idTagInfo, "status");
        		        	        int status;
        		        	        if (enums_as_integers)
        		        	        {
        		        	        	status=json_object_get_int(obj_status);
        		        	        }
        		        	        else
        		        	        {
        		        	        	//Convertimos la cadena en un numero
        		        	        	const char *status_str=json_object_get_string(obj_status);
        		        	        	for (int i=0; sizeof(AuthorizationStatusTexts); i++)
        		        	        	{
        		        	        		if (strcmp(AuthorizationStatusTexts[i], status_str)==0)
        		        	        		{
        		        	        			status=i;
        		        	        			break;
        		        	        		}
        		        	        	}
				        	        }

        		        	        //Cogemos los datos que nos faltan de la peticion
        		        	        char *request=getPayloadFromMessage(atoi(UniqueId_str));
        		        	        json_object *req = json_tokener_parse_verbose(request, &err);

        		        	    	json_object *obj_reqpayl;
        		        	    	if (send_tags)
        		        	        {
        		        	        	obj_reqpayl= json_object_object_get(req , "Payload");
        		        	        }
        		        	        else
        		        	        {
        		        	        	obj_reqpayl=json_object_array_get_idx(req, 3);
        		        	        }

        		        	        //connector
        		        	        json_object *obj_transaction = json_object_object_get(obj_reqpayl, "transactionId");
        		        	        int transactionId=json_object_get_int(obj_transaction);

        		        	        int connectorId=getConnectorFromTransaction(transactionId);
        		        	        json_object *obj_idTag =NULL;
									const char *idTag=NULL;

        		        	        if (connectorId>=0)
        		        	        {
        		        	        	changeConnectorStatus(connectorId, _CP_STATUS_FINISHING);

        		        	        	//Lo eliminamos de la estructura interna. Es posible que ya se haya eliminado anteriormente.
        		        	        	//Pero no pasa nada por tratar de eliminarlo dos veces
        		        	        	removeTransaction(connectorId);

        		        	        	//idTag
        		        	        	obj_idTag = json_object_object_get(obj_reqpayl, "idTag");
        		        	        	idTag=json_object_get_string(obj_idTag);
        		        	        	changeConnectorStatus(connectorId-1, _CP_STATUS_AVAILABLE);
        		        	        }

        		        	        if (status==_CP_AUTHORIZATION_ACCEPTED)
        		        	        {
        		        	        	if (isAuthorizationCacheEnabled())
        		        	        	{
        		        	        		if (!isIdTagInAuthorizationList(parentIdTag))
        		        	        		{
        		        	        			update_authorization_cache(idTag, expiryDate, parentIdTag, status);
        		        	        		}
        		        	        	}
        		        	        }

        		        	    }

        		        	}

        		     /////////////FIN SWITCH PETICIONES///////////////////////////////////////////////////////////////////

        		        	//...y comprobamos si corresponde con el ultimo UniqueId
        		        	char *ptr;
							long thisuniqueId= strtol(UniqueId_str, &ptr, 10);

						//	if (debug) printf("ANTES DE DEQUEUE, lasSentAccepted deberia ser igual que thisUniqueId. Y son %d y %d", lastSentAcceptedUniqueId, thisuniqueId);
        		        	if (lastSentAcceptedUniqueId==thisuniqueId)
        		        	{
        		        		lastSentAcceptedUniqueId++;
        		        		initializedConnection=1;
        		        		writable=1;
        		        		if (!do_not_dequeue) Dequeue();
        		        		else do_not_dequeue=0;
        		        	}
        		        }
        		        else
        		        {
        		        	printf("%s", in);
        		        	//////////////////////////////////////////////////////////////////////
        		        	//Si recibimos.... PETICIONES procedentes del Central System
        		        	if (json_object_get_int(obj_MessageTypeId)==_CALLMESSAGE)
        		        	{
        		        		//Declaracion de variables
        		        		char *message;
        		        		const char *action;
        		        		const char *UniqueId_str=NULL;
        		        		json_object *obj_Action=NULL;

        		        		if (send_tags)
        		        		{
        		       		        obj_Payload = json_object_object_get(jobj, "Payload");
        		       		        obj_Action = json_object_object_get(jobj, "Action");
        		        		}
        		        		else
        		        		{
        		        			obj_Action=json_object_array_get_idx(jobj, 2);
        		        			obj_Payload=json_object_array_get_idx(jobj, 3);
        		        		}

        		        		if (obj_UniqueId && obj_Action)
        		        		{
        		        			//OK, Hemos recibido un mensaje correcto
        		        			//Extraemos el UniqueId y la action
        		        			UniqueId_str=json_object_get_string(obj_UniqueId);
        		        			int UniqueId=-1;
        		        			UniqueId=atoi(UniqueId_str);
        		        			action=json_object_get_string(obj_Action);

        		        			//Log
        		        			addLog("[Callback] El cliente recibio la solicitud:%s de tipo %s.", (char *)in, action, LOG_DEBUG, ANY);

        		        			//Comenzamos el switch principal (como son cadenas, lo implemento con un if--else if -- else if ...
        		        			//La estructura generalmente para cada uno de los tipos de mensajes es igual. Primero extraemos la información recibida
        		        			//y luego se llama a una función que 1.- Gestionará esa información, 2.- Devolverá el mensaje que deberá responder el endpoint.

									///////////////////////////////////////////////////////////////////////////////////
									//5.1 CANCEL RESERVATION REQUEST
									if (strcmp(action,"CancelReservation")==0)
									{
										if (obj_Payload)
										{
											json_object *obj_reservationId = json_object_object_get(obj_Payload, "reservationId");
											if (obj_reservationId)
											{
												reservationId=json_object_get_int(obj_reservationId);

												addLog("[Callback] REQUEST OF TYPE %s RECEIVED. REQUESTING CANCELLATION OF RESERVATION ID %s.", action, convert(reservationId),LOG_INFO, ANY);
												message=manageCancelReservationRequest(UniqueId_str, reservationId);

											//	if (debug) showReservations();
											}
											else
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.2 CHANGE AVAILABILITY
									else if (strcmp(action,"ChangeAvailability")==0)
									{
										//json_object *obj_Payload = json_object_object_get(jobj, "Payload");
										if (obj_Payload)
										{
											int avail, connector=0;
											const char *avail_str;
											json_object *obj_connectorId = json_object_object_get(obj_Payload, "connectorId");

											json_object *obj_type = json_object_object_get(obj_Payload, "type");
											if (enums_as_integers)
											{
												if (obj_type) avail=json_object_get_int(obj_type);
											}
											else
											{
												if (obj_type) avail_str=json_object_get_string(obj_type);

												//Extraemos el entero de la cadena.
												for (int i=0; i<sizeof(availabilityTypeTexts); i++) if (strcmp(avail_str, availabilityTypeTexts[i])==0) avail=i;
											}

											if (obj_connectorId) connector=json_object_get_int(obj_connectorId);

											addLog("REQUESTING CHANGE OF AVAILABILITY OF CONNECTOR %s TO '%s'.", convert(connector),availabilityTypeTexts[avail],LOG_INFO, ANY);

											message=manageChangeAvailabilityRequest(UniqueId_str, connector, avail);
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.3 CHANGE CONFIGURATION
									else if (strcmp(action,"ChangeConfiguration")==0)
									{
										//json_object *obj_Payload = json_object_object_get(jobj, "Payload");
										if (obj_Payload)
										{
											const char *key, *value;
											json_object *obj_key = json_object_object_get(obj_Payload, "key");
											json_object *obj_value = json_object_object_get(obj_Payload, "value");
											if (obj_key) key=json_object_get_string(obj_key);
											if (obj_value) value=json_object_get_string(obj_value);

											addLog("REQUESTING CONFIGURATION UPODATE OF KEY '%s' TO VALUE '%s'.", key ,value,LOG_INFO, ANY);
											message=manageChangeConfigurationRequest(UniqueId_str, key, value);
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.4 CLEAR CACHE
									else if (strcmp(action,"ClearCache")==0)
									{
										addLog("REQUESTING CLEAR CACHE FROM CENTRAL SYSTEM. ", NULL, NULL,LOG_INFO, ANY);
										//Aqui no hay nada que extraer del mensaje de request
										message=manageClearCacheRequest(UniqueId_str);
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.5 CLEAR CHARGING PROFILE
									else if (strcmp(action,"ClearChargingProfile")==0)
									{
										if (obj_Payload)
										{
											const char *purpose_str=NULL;
											int chargingprofileid=-1, connectorid=-1, purpose=-1, stacklevel=-1;

											json_object *obj_chargingprofileid = json_object_object_get(obj_Payload, "id");
											json_object *obj_connectorid= json_object_object_get(obj_Payload, "connectorId");
											json_object *obj_chargingprofilepurpose = json_object_object_get(obj_Payload, "chargingProfilePurpose");
											json_object *obj_stacklevel= json_object_object_get(obj_Payload, "stackLevel");

											if (obj_chargingprofileid) chargingprofileid=json_object_get_int(obj_chargingprofileid);
											if (obj_connectorid) connectorid=json_object_get_int(obj_connectorid);
											if (obj_chargingprofilepurpose)
											{
												if (enums_as_integers)
												{
													purpose=json_object_get_int(obj_chargingprofilepurpose);
													purpose_str=ChargingProfilePurposeTypeTexts[purpose];
												}
												else
												{
													purpose_str=json_object_get_string(obj_chargingprofilepurpose);
												}
											}

											if (obj_stacklevel) stacklevel=json_object_get_int(obj_stacklevel);

											if (chargingprofileid>0) addLog("RECEIVED A ClearChargingProfile REQUEST OF CHARGING PROFILE %s.", convert(chargingprofileid), NULL,LOG_INFO, ANY);
											else if (chargingprofileid==0) addLog("RECEIVED A ClearChargingProfile REQUEST OF CHARGING PROFILE FOR THE OVERALL CHARGE POINT.", convert(chargingprofileid), NULL,LOG_INFO, ANY);
											else addLog("RECEIVED A ClearChargingProfile REQUEST OF ALL CHARGING PROFILES.", NULL, NULL,LOG_INFO, ANY);

											//connectorid debe ser un valor de 1 a N. Si llega un cero es que no se recibio connectorid
											message=manageClearChargingProfileRequest(UniqueId_str, chargingprofileid, connectorid,purpose_str, stacklevel);
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}
									///////////////////////////////////////////////////////////////////////////////////
									//5.6 CS DATA TRANSFER
									else if (strcmp(action,"CSDataTransfer")==0)
									{
										if (obj_Payload)
										{
											const char *vendorId, *MessageId, *Data;
											json_object *obj_vendorId= json_object_object_get(obj_Payload, "vendorId");
											if (!obj_vendorId)
											{
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
											else
											{
												json_object *obj_messageId= json_object_object_get(obj_Payload, "messageId");
												json_object *obj_data= json_object_object_get(obj_Payload, "data");

												vendorId=json_object_get_string(obj_vendorId);
												if (obj_messageId) MessageId=json_object_get_string(obj_messageId);
												if (obj_data) Data=json_object_get_string(obj_data);

												addLog("DATA TRANSFER REQUEST FROM CENTRAL SYSTEM RECEIVED WITH FOLLOWING DATA: '%s'. ", Data, NULL,LOG_INFO, ANY);
												message=manageCSDataTransferRequest(UniqueId_str, vendorId, MessageId, Data);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.8 GET CONFIGURATION
									else if (strcmp(action,"GetConfiguration")==0)
									{
										if (obj_Payload)
										{
											//En este caso el addLog lo meto dentro de la siguiente funcion
											message=manageGetConfigurationRequest(UniqueId_str, obj_Payload);
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//mensaje de error
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.9 GET DIAGNOSTICS
									else if (strcmp(action,"GetDiagnostics")==0)
									{
										if (obj_Payload)
										{
											json_object *obj_location= json_object_object_get(obj_Payload, "location");
											json_object *obj_retries= json_object_object_get(obj_Payload, "retries");
											json_object *obj_retryInterval= json_object_object_get(obj_Payload, "retryInterval");
											json_object *obj_startTime= json_object_object_get(obj_Payload, "startTime");
											json_object *obj_stopTime= json_object_object_get(obj_Payload, "stopTime");

											int retries=-1;
											int retryInterval=-1;
											const char *startTime=NULL;
											const char *stopTime=NULL;
											const char *location;

											if (!obj_location)
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												//Location is a mandatory field
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
											else
											{
												//Extraemos elementos de la peticion
												location=json_object_get_string(obj_location);
												if (obj_retries) retries=json_object_get_int(obj_retries);
												if (obj_retryInterval) retryInterval=json_object_get_int(obj_retryInterval);
												if (obj_startTime) startTime=json_object_get_string(obj_startTime);
												if (obj_stopTime) stopTime=json_object_get_string(obj_stopTime);

												//Generamos Log
												addLog("RECEIVED A GetDiagnostics REQUEST. DIAGNOSTICS FILE WILL BE SENT TO %s.", location, NULL,LOG_INFO, ANY);

												//Tratamos la peticion
												message=manageGetDiagnosticsRequest(UniqueId_str, location, retries, retryInterval, startTime, stopTime);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//mensaje de error
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.10 GET LOCAL LIST VERSION
									else if (strcmp(action, "GetLocalListVersion")==0)
									{
										message=respondGetLocalListVersionRequest(UniqueId_str);
									}
									///////////////////////////////////////////////////////////////////////////////////
									//5.11 REMOTE START TRANSACTION
									else if (strcmp(action, "RemoteStartTransaction")==0)
									{
										//EXTRAEMOS LOS CAMPOS DE LA SOLICITUD
										if (obj_Payload)
										{
											json_object *obj_idTag= json_object_object_get(obj_Payload, "idTag");
											json_object *obj_connectorId= json_object_object_get(obj_Payload, "connectorId");
											json_object *obj_chargingProfile= json_object_object_get(obj_Payload, "chargingProfile");

											if (!obj_idTag)
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												//Campo obligatorio
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
											else
											{
												const char *idTag=json_object_get_string(obj_idTag);
												int chargingProfileId, stackLevel, transactionId;
												int connectorId=-1;
												const char *valid_from, *valid_to;
												const char *purpose_str=NULL, *kind_str=NULL, *recurrency_str=NULL;
												json_object *obj_chargingSchedule;

												if (obj_connectorId) connectorId=json_object_get_int(obj_connectorId);

												if (obj_chargingProfile)
												{
													//7.8
													chargingProfileId=json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingProfileId"));
													stackLevel=json_object_get_int(json_object_object_get(obj_chargingProfile, "stackLevel"));

													//transactionId
													transactionId=-1;
													json_object *obj_transactionId=json_object_object_get(obj_chargingProfile, "transactionId");
													if (obj_transactionId) transactionId=json_object_get_int(obj_transactionId);

													//validFrom
													valid_from=NULL;
													json_object *obj_validFrom=json_object_object_get(obj_chargingProfile, "validFrom");
													if (obj_validFrom) valid_from=json_object_get_string(obj_validFrom);

													//validTo
													valid_to=NULL;
													json_object *obj_validTo=json_object_object_get(obj_chargingProfile, "validTo");
													if (obj_validTo) valid_to=json_object_get_string(obj_validTo);

													//chargingSchedule (7.13)
													obj_chargingSchedule=json_object_object_get(obj_chargingProfile, "chargingSchedule");

													////////////////////////////////////////////
													if (obj_chargingSchedule)
													{
														//duration
														int duration=-1;
														int chargingRateUnit;
														json_object *obj_duration=json_object_object_get(obj_chargingSchedule, "duration");
														if (obj_duration) duration=json_object_get_int(obj_duration);

														//StartSchedule
														const char *startSchedule=NULL;
														const char *chargingRateUnit_str;
														json_object *obj_startSchedule=json_object_object_get(obj_chargingSchedule, "startSchedule");
														if (obj_startSchedule) startSchedule=json_object_get_string(obj_startSchedule);

														//chargingRateUnit
														if (enums_as_integers)
														{
															chargingRateUnit=json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingRateUnit"));
															chargingRateUnit_str=ChargingRateUnitTypeTexts[chargingRateUnit];

														}
														else
														{
															chargingRateUnit_str=json_object_get_string(json_object_object_get(obj_chargingProfile, "chargingRateUnit"));
														}

														//OJO, este dato, los periodos no se esta usando... es una array.
														//chargingSchedulePeriod
														json_object *obj_chargingSchedulePeriod=json_object_object_get(obj_chargingSchedule, "chargingSchedulePeriod");

														//minChargingRate
														int minChargingRate=-1;
														json_object *obj_minChargingRate=json_object_object_get(obj_chargingSchedule, "minChargingRate");
														if (obj_minChargingRate) minChargingRate=json_object_get_int(obj_minChargingRate);
													}
													////////////////////////////////////////////

													if (enums_as_integers)
													{
														purpose_str=ChargingProfilePurposeTypeTexts[json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingProfilePurpose"))];
														kind_str= ChargingProfileKindTypeTexts[json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingProfileKind"))];
														json_object *obj_recurrency=json_object_object_get(obj_chargingProfile, "recurrencyKind");
														if (obj_recurrency) recurrency_str=RecurrencyKindTypeTexts[json_object_get_int(obj_recurrency)];
													}
													else
													{
														recurrency_str=NULL;
														purpose_str=json_object_get_string(json_object_object_get(obj_chargingProfile, "chargingProfilePurpose"));
														kind_str=json_object_get_string(json_object_object_get(obj_chargingProfile, "chargingProfileKind"));
														json_object *obj_recurrency=json_object_object_get(obj_chargingProfile, "recurrencyKind");
														if (obj_recurrency) recurrency_str=json_object_get_string(obj_recurrency);
													}
												}

												addLog("RECEIVED A RemoteStartTransaction REQUEST FOR IDTAG %s on CONNECTOR %s.", idTag, convert(connectorId),LOG_INFO, ANY);

												//YA ESTA, TODOS LOS CAMPOS EXTRAIDOS. AHORA LOS TRATAMOS:
												message=respondRemoteStartTransactionRequest(UniqueId_str, idTag, connectorId, obj_chargingProfile, chargingProfileId, stackLevel, transactionId, purpose_str, kind_str, recurrency_str, valid_from, valid_to, obj_chargingSchedule);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}
									///////////////////////////////////////////////////////////////////////////////////
									//5.12 REMOTE STOP TRANSACTION
									else if (strcmp(action, "RemoteStopTransaction")==0)
									{
										if (obj_Payload)
										{
											json_object *obj_transactionId= json_object_object_get(obj_Payload, "transactionId");

											if (obj_transactionId)
											{
												int transactionId=json_object_get_int(obj_transactionId);

												addLog("RECEIVED A RemoteStopTransaction REQUEST FOR TRANSACTIONID %s.", convert(transactionId),NULL,LOG_INFO, ANY);

												message=respondRemoteStopTransactionRequest(UniqueId_str, transactionId);
											}
											else
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);

											//El campo transactionId es obligatorio
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//El payload es obligatorio
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}
									///////////////////////////////////////////////////////////////////////////////////
									//5.13 RESERVE NOW
									else if (strcmp(action, "ReserveNow")==0)
									{

										//json_object *obj_Payload = json_object_object_get(jobj, "Payload");
										if (obj_Payload)
										{
											json_object *obj_parentIdTag = json_object_object_get(obj_Payload, "parentIdTag");
											json_object *obj_reservationId = json_object_object_get(obj_Payload, "reservationId");
											json_object *obj_idTag = json_object_object_get(obj_Payload, "idTag");
											json_object *obj_expiryDate = json_object_object_get(obj_Payload, "expiryDate");
											json_object *obj_connectorId= json_object_object_get(obj_Payload, "connectorId");

											const char *UniqueId_str=json_object_get_string(obj_UniqueId);

											if (!obj_reservationId || !obj_idTag ||!obj_expiryDate||!obj_connectorId)
											{
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
											else
											{
												const char *parentIdTag=NULL;
												const char *idTag=NULL;
												const char *expiryDate;
												int connector=-1;
												int reservationId=-1;

												if (obj_idTag) idTag=json_object_get_string(obj_idTag);
												if (obj_connectorId) connector=json_object_get_int(obj_connectorId);
												if (obj_parentIdTag ) parentIdTag = json_object_get_string(obj_parentIdTag);
												if (obj_reservationId) reservationId=json_object_get_int(obj_reservationId);
												if (obj_expiryDate) expiryDate=json_object_get_string(obj_expiryDate);

												//El connectorId que llega debe ser un valor de 1 a N
												addLog("RECEIVED A ReserveNow REQUEST FOR IDTAG %s on CONNECTOR %s....", idTag,convert(connector),LOG_INFO, ANY);
												addLog("...using reservationId %s BEFORE %s.", convert(reservationId),expiryDate,LOG_INFO, ANY);

												message=manageReserveNowRequest(UniqueId_str, connector-1,expiryDate, idTag, reservationId, parentIdTag);
												//if (debug) showReservations();
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//DEBE HABER UN PAYLOAD PARA ESTE REQUEST
											char *message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}

									//	if (debug) showReservations();
									}
									///////////////////////////////////////////////////////////////////////////////////
									//5.14 RESET
									else if (strcmp(action, "Reset")==0)
									{
										if (obj_Payload)
										{
											json_object *obj_resetType = json_object_object_get(obj_Payload, "type");
											if (obj_resetType)
											{
												const char *resetType=NULL;

												if (enums_as_integers)
												{
													resetType=ResetType_texts[json_object_get_int(obj_resetType)];
												}
												else
												{
													resetType=json_object_get_string(obj_resetType);
												}

												addLog("RECEIVED A Reset Now REQUEST WITH RESET TYPE '%s'.", resetType,NULL,LOG_INFO, ANY);

												message=manageResetRequest(UniqueId_str, resetType);

												if (strcmp(resetType, "Hard")==0)
												{
													sendToDisplay("Resetting ChargePoint. Please wait until ready");
													usleep(5000000);
													pthread_t pid;
													pthread_create(&pid, NULL, rebootChargePoint, NULL);
													pthread_detach(pid);
												}
											}
											else
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												//DEBE HABER UN reset type
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//DEBE HABER UN PAYLOAD PARA ESTE REQUEST
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.15 SEND LOCAL LIST
									else if (strcmp(action, "SendLocalList")==0)
									{
										int connector=-1;

										if (!obj_Payload)
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//DEBE HABER UN Payload
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
										else
										{
											json_object *obj_listVersion = json_object_object_get(obj_Payload, "listVersion");
											json_object *obj_localAuthorizationList = json_object_object_get(obj_Payload, "localAuthorizationList");
											json_object *obj_updateType = json_object_object_get(obj_Payload, "updateType");

											if ((!obj_updateType) || (!obj_listVersion))
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
											else
											{
												//Todo OK
												int version=json_object_get_int(obj_listVersion);

												const char *updateType_str;
												if (enums_as_integers)
												{
													updateType_str=UpdateType_texts[json_object_get_int(obj_updateType)];
												}
												else
												{
													updateType_str=json_object_get_string(obj_updateType);
												}

												//Creamos el Log
												if (obj_localAuthorizationList)
													addLog("RECEIVED A Send Local List REQUEST WITH VERSION '%s' AND THE FOLLOWING LIST: %s.", convert(version),json_object_to_json_string(obj_localAuthorizationList),LOG_INFO, ANY);
												else
													addLog("RECEIVED A Send Local List REQUEST OF TYPE '%s' AND VERSION '%s'.",updateType_str, convert(version),LOG_INFO, ANY);

												//La tratamos
												message=manageSendLocalListRequest(UniqueId_str, version, obj_localAuthorizationList, updateType_str);
											}
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.16 SET CHARGING PROFILE
									else if (strcmp(action, "SetChargingProfile")==0)
									{
										//json_object *obj_Payload = json_object_object_get(jobj, "Payload");
										if (obj_Payload)
										{
											json_object *obj_connectorId= json_object_object_get(obj_Payload, "connectorId");
											json_object *obj_chargingProfile= json_object_object_get(obj_Payload, "chargingProfile");

											int chargingProfileId, stackLevel, transactionId;
											int connectorId=-1;
											const char *valid_from, *valid_to;
											const char *purpose_str=NULL, *kind_str=NULL, *recurrency_str=NULL;
											json_object *obj_chargingSchedule;

											if (obj_connectorId) connectorId=json_object_get_int(obj_connectorId);

											if (obj_chargingProfile)
											{
												//7.8
												chargingProfileId=json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingProfileId"));
												stackLevel=json_object_get_int(json_object_object_get(obj_chargingProfile, "stackLevel"));

												//transactionId
												transactionId=-1;
												json_object *obj_transactionId=json_object_object_get(obj_chargingProfile, "transactionId");
												if (obj_transactionId) transactionId=json_object_get_int(obj_transactionId);

												//validFrom
												valid_from=NULL;
												json_object *obj_validFrom=json_object_object_get(obj_chargingProfile, "validFrom");
												if (obj_validFrom) valid_from=json_object_get_string(obj_validFrom);

												//validTo
												valid_to=NULL;
												json_object *obj_validTo=json_object_object_get(obj_chargingProfile, "validTo");
												if (obj_validTo) valid_to=json_object_get_string(obj_validTo);

												//chargingSchedule (7.13)
												obj_chargingSchedule=json_object_object_get(obj_chargingProfile, "chargingSchedule");

												////////////////////////////////////////////
												if (obj_chargingSchedule)
												{
												//duration
													int duration;
													int chargingRateUnit;
													json_object *obj_duration=json_object_object_get(obj_chargingSchedule, "duration");
													if (obj_duration) duration=json_object_get_int(obj_duration);
													else duration=-1;

													//StartSchedule
													const char *startSchedule=NULL;
													const char *chargingRateUnit_str;
													json_object *obj_startSchedule=json_object_object_get(obj_chargingSchedule, "startSchedule");
													if (obj_startSchedule) startSchedule=json_object_get_string(obj_startSchedule);

													//chargingRateUnit
													if (enums_as_integers)
													{
														chargingRateUnit=json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingRateUnit"));
														chargingRateUnit_str=ChargingRateUnitTypeTexts[chargingRateUnit];
													}
													else
													{
														chargingRateUnit_str=json_object_get_string(json_object_object_get(obj_chargingProfile, "chargingRateUnit"));
													}

													//chargingSchedulePeriod
													json_object *obj_chargingSchedulePeriod=json_object_object_get(obj_chargingSchedule, "chargingSchedulePeriod");

													//minChargingRate
													int minChargingRate=-1;
													json_object *obj_minChargingRate=json_object_object_get(obj_chargingSchedule, "minChargingRate");
													if (obj_minChargingRate) minChargingRate=json_object_get_int(obj_minChargingRate);
												  }

												  ////////////////////////////////////////////
												  if (enums_as_integers)
												  {
													purpose_str=ChargingProfilePurposeTypeTexts[json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingProfilePurpose"))];
													kind_str= ChargingProfileKindTypeTexts[json_object_get_int(json_object_object_get(obj_chargingProfile, "chargingProfileKind"))];
													json_object *obj_recurrency=json_object_object_get(obj_chargingProfile, "recurrencyKind");
													if (obj_recurrency) recurrency_str=RecurrencyKindTypeTexts[json_object_get_int(obj_recurrency)];
												  }
												  else
												  {
													recurrency_str=NULL;
													purpose_str=json_object_get_string(json_object_object_get(obj_chargingProfile, "chargingProfilePurpose"));
													kind_str=json_object_get_string(json_object_object_get(obj_chargingProfile, "chargingProfileKind"));
													json_object *obj_recurrency=json_object_object_get(obj_chargingProfile, "recurrencyKind");
													if (obj_recurrency) recurrency_str=json_object_get_string(obj_recurrency);
												  }

												  addLog("RECEIVED A Set Charging Profile REQUEST OF CHARGING PROFILE %s ON CONNECTOR %s.", convert(chargingProfileId),convert(connectorId),LOG_INFO, ANY);
												  //connectorId llega un valor de 1 a N, pero en la llamada se debe pasar un valor de 0 a N
												  message=respondSetChargingProfileRequest(UniqueId_str, connectorId-1, obj_chargingProfile, chargingProfileId, stackLevel, transactionId, purpose_str, kind_str, recurrency_str, valid_from, valid_to, obj_chargingSchedule);
												}
												else
												{
													addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
													addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
													//Debe haber un charging profile
													message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
												}
											}
											else
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												//No hay payload
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
									}
									///////////////////////////////////////////////////////////////////////////////////
									//5.17 TRIGGER MESSAGE
									else if (strcmp(action, "TriggerMessage")==0)
									{
										int connector=-1;
										const char *MessageId_Str;

										if (obj_Payload)
										{
											json_object *obj_connector = json_object_object_get(obj_Payload, "connectorId");
											if (obj_connector )
											{
												connector=json_object_get_int(obj_connector);
											}

											json_object *obj_message = json_object_object_get(obj_Payload, "requestedMessage");

											if (obj_message)
											{
												if (enums_as_integers)
												{
													int mess=json_object_get_int(obj_message);
													MessageId_Str=MessageTrigger_texts[mess];
												}
												else
												{
													MessageId_Str=json_object_get_string(obj_message);
												}

												addLog("RECEIVED A Trigger Message REQUEST OF TYPE %s ON CONNECTOR %s.", MessageId_Str,convert(connector),LOG_INFO, ANY);
												message=manageTriggerMessageRequest(UniqueId_str, connector-1, MessageId_Str);
											}
											else
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												//DEBE HABER UN Message
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//DEBE HABER UN payload
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}
									///////////////////////////////////////////////////////////////////////////////////
									//5.18 UNLOCK CONNECTOR
									else if (strcmp(action, "UnlockConnector")==0)
									{
										int connector;
										if (obj_Payload)
										{
											json_object *obj_connector = json_object_object_get(obj_Payload, "connectorId");
											if (obj_connector )
											{
												connector=json_object_get_int(obj_connector);

												addLog("RECEIVED AN Unlock Connector REQUEST FROM THE CENTRAL SYSTEM ON CONNECTOR %s.", convert(connector),NULL,LOG_INFO, ANY);
												message=manageUnlockConnectorRequest(UniqueId_str, connector);
											}
											else
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												//DEBE HABER UN connector
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
										}
										else
										{
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//DEBE HABER UN payload
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										}
									}

									///////////////////////////////////////////////////////////////////////////////////
									//5.19 UPDATE FIRMWARE
									else if (strcmp(action,"UpdateFirmware")==0)
									{
										if (obj_Payload)
										{
											json_object *obj_location= json_object_object_get(obj_Payload, "location");
											json_object *obj_retries= json_object_object_get(obj_Payload, "retries");
											json_object *obj_retryInterval= json_object_object_get(obj_Payload, "retryInterval");
											json_object *obj_retrieveDate= json_object_object_get(obj_Payload, "retrieveDate");

											int retries=-1;
											int retryInterval=-1;
											const char *retrieveDate=NULL;
											const char * location=NULL;

											if (!obj_location||!obj_retrieveDate)
											{
												addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
												addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
												//mandatory fields
												message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
											}
											else
											{
												//Extraemos los campos del mensajes
												location=json_object_get_string(obj_location);
												retrieveDate=json_object_get_string(obj_retrieveDate);
												if (obj_retries) retries=json_object_get_int(obj_retries);
												if (obj_retryInterval) retryInterval=json_object_get_int(obj_retryInterval);

												//Generamos el Log
												addLog("RECEIVED A Update Firmware REQUEST. Firmware WILL BE RETRIEVED FROM %s ON %s.", location, retrieveDate,LOG_INFO, ANY);

												//Los tratamos
												message=manageUpdateFirmwareRequest(UniqueId_str, location, retrieveDate, retries, retryInterval);
											}
										 }
										 else
										 {
											addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED.", action, NULL,LOG_ERROR, ANY);
											addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);
											//mensaje de error
											message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
										 }
									}
									else
									{
										//ACCION DESCONOCIDA. LOGAMOS Y ENVIAMOS MENSAJE DE RESPUESTA
										addLog("[Callback] INVALID REQUEST OF TYPE %s RECEIVED (¿¿??).", action, NULL,LOG_ERROR, ANY);
										addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);

										message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
									}
									//FIN 5.19
        		        		}//if (UniqueId_str && obj_Action)
        		        		else
        		        		{
        		        			//No UniqueId or Action received in message
									addLog("[Callback] INVALID REQUEST WITH UNSUPPPORTED ACTION %s RECEIVED (¿¿??).", action, NULL,LOG_ERROR, ANY);
									addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);

        		        			message=prepareErrorResponse(UniqueId_str, _PROTOCOL_ERROR);
        		        		}
        		        		//Finalmente mandamos el mensaje.
								websocket_write_back(wsi ,message, -1);
        		        	}//if (json_object_get_int(obj_MessageTypeId)==_CALLMESSAGE)

        	 	//////////////////FIN PARTE MENSAJES 5.X//////////////////////////////////////////////////////////////
        		        	else
        		        	{
        		        		//SE LLEGA AQUI SI NO SE RECIBE MENSAJE DE PETICION NI DE RESPUESTA
        		             	addLog("[LOG] MESSAGE TYPE UNSUPPORTED.",(char *)in, NULL, LOG_ERROR, ANY);
        		             	addLog("[Callback] REQUEST RECEIVED IS '%s'.", (char *)in, NULL,LOG_DEBUG, ANY);

        		               	const char *UniqueId_str=json_object_get_string(obj_UniqueId);

        		               	char *ptr;
        		               	long thisuniqueId= strtol(UniqueId_str, &ptr, 10);

        		               	addLog("BEFORE THE DEQUEUE REQUEST, thisUniqueId is %s (and should be zero)", convert(thisuniqueId), NULL, LOG_ERROR, ANY);

        		               	///CUANDO SE RECIBA UN MENSAJE DE RESPUESTA DE ERROR ELIMINAMOS EL REQUEST DE LA COLA
        		               	if (thisuniqueId==0) //Si recibimos un mensaje de error procedente de un mensaje recbido sin ID, eliminamos el ultimo mensaje
        		               	{
        		               		initializedConnection=1;
        		               		writable=1;
        		               		Dequeue();
        		               	}
        		               	else
        		               	{
        		               		//Si tiene ID,
        		               		//RECORREMOS LA COLA Y ELIMINAMOS EL QUE ESTE MAL <--NO IMPLEMENTADO
        		               	}
        		             }//FIN NO SE RECIBE MENSAJE DE PETICION NI DE RESPUESTA
        		        }//else. Cerramos linea 192
        			}//if (obj_UniqueId&&obj_MessageTypeId&&obj_Payload). Linea 185
        			else
        			{
    //    				error=1;
        			}
        		}
        	else//if (jobj)
        	{
        		addLog("Error message received from Central System", NULL,NULL, LOG_ERROR, ANY);
        		Dequeue(Message_queue);
    //    		error=1;
        	}
           }

            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE : //Cuando la conexion dice que podemos escribir, miramos la cola y escribimos.
              writeable_flag = 1;
              if (!Message_queue) return 0;

              //Loga todo menos los heartbeats
              if (Message_queue->MessageAction!=406) addLog("SENDING MESSAGE OF TYPE %s TO CENTRAL SYSTEM: %s",convert(Message_queue->MessageAction),Message_queue->payload, LOG_INFO, ANY );

            	switch (Message_queue->MessageAction)
            	{
            	case (BOOT_NOTIFICATION):
            	    if (_REGISTRATIONSTATUS==_RS_ACCEPTED || _REGISTRATIONSTATUS==_RS_PENDING)
            	    {
            	    	websocket_write_back(wsi, Message_queue->payload, -1);
            	    	chargePointStatus=_CP_STATUS_AVAILABLE;

            	    	//Pag 45: The chargepoint should send a statusnotification.req PDU with its currentStatus if the status changed while the chargepoint was offline.

            	    	if (sendConnectorStatusWhenOnline)
            	    	{
            	    		sendConnectorStatusWhenOnline=0;
            	    		char *vendor=getVendor();
            	    		char *vendorErrorCode=NULL;
            	    		for (int i=0; i<NUM_CONNECTORS; i++)
            	    			send_statusnotification_request(i+1, connectorStatus[i], currentConnectorErrorStatus[i], NULL, vendor, vendorErrorCode);
            	    	}

            	    	//Pag 45: The charge point may send a status notification PDU to report an error that occurred while the charge point was offline.
            	    	for (int i=0; i<NUM_CONNECTORS; i++)
            	    	{
            	    		if (currentConnectorErrorStatus[i]!=_CP_ERROR_NOERROR)
            	    		{
                	    		char *vendor=getVendor();
                	    		char *vendorErrorCode=NULL;
            	    			send_statusnotification_request(i+1, connectorStatus[i], currentConnectorErrorStatus[i], NULL, vendor, vendorErrorCode);
            	    		}
            	    	}

            	    }
            	    writeable_flag = 0;
            	    break;
            	case (AUTHORIZE):
            	case (FIRMWARE_STATUS_NOTIFICATION):
            	case (DATA_TRANSFER):
            	case (DIAGNOSTICS_STATUS_NOTIFICATION):

            	case (METER_VALUES):
            	case (STATUS_NOTIFICATION):
            	case (START_TRANSACTION):
            	case (STOP_TRANSACTION):
					if (_REGISTRATIONSTATUS==_RS_ACCEPTED) websocket_write_back(wsi, Message_queue->payload, -1);
            		anothermessagesent=1;
            	    writeable_flag = 0;
            	    break;
            	case (HEARTBEAT):
					anothermessagesent=0;
					if (_REGISTRATIONSTATUS==_RS_ACCEPTED) websocket_write_back(wsi, Message_queue->payload, -1);
            	    writeable_flag = 0;
            	    break;
                default:
                	//SI NO ES UN MENSAJE DE LOS PERMITIDOS, SIMPLEMENTE LO ELIMINAMOS DE LA COLA
                	addLog("Not allowed message sent to queue", NULL,NULL, LOG_ERROR, ANY);
                	writeable_flag = 0;
                	Dequeue(Message_queue);
                    break;
            }
            break;
       default:
    	   break;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
//          THREADS
////////////////////////////////////////////////////////////////////////////////////////
//Esta es la funcion que se lanza cuando se crea el hilo para reiniciar el Chargepoint.
//This simply allows time to send the reset response message and then reboots.
static void *rebootChargePoint(struct lws* wsi)
{
	usleep(2000*getConfigurationKeyIntValue("HeartbeatInterval"));

	//Al reiniciarse se guarda el estado en SQLite o fichero, si este no estuviera funcional
	if (isSQLiteConnected())
	{
		write_cache_to_disk();
		write_list_to_disk();

		writeChargePointValuesToSQLite();
	}
	else
	{
		write_cache_to_disk();
		write_list_to_disk();

		char *filename=getConfigurationKeyStringValue("ChargePointDataFilename");
		if (!filename)
		{
			filename=(char *)calloc(1,64);
			strcpy(filename, "/tmp/ChargePointDataFilename.dat");
		}

		writeToDisk(filename);
	}

	//Physically reboots the device.
	reboot();
}

static void *sendTransactionRequests(struct lws* wsi)
{
int interval,attempts;
int attemptsTried=0;

	//Ver Pag 20:  Error responses to transaction-related messages
	// The   number   of   times   and   the   interval   with   which   the   Charge   Point   should   retry   such   failed
	//transaction-related    messages    MAY    be    configured    using    the TransactionMessageAttempts    and
	//TransactionMessageRetryInterval configuration keys.

	queue_node *p;
	int esteUniqueId=Message_queue->UniqueId;
	interval=getConfigurationKeyIntValue("TransactionMessageRetryInterval");
	attempts=getConfigurationKeyIntValue("TransactionMessageAttempts");
	enum json_tokener_error err;
	int connectorId=-1;
	const char *UniqueId_Str=NULL;

	//Obtenemos el conector y el UniqueId al que se refiere
	json_object *req = json_tokener_parse_verbose(Message_queue->payload, &err);
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

	//Lo mandamos y nos quedamos esperando el típico Heartbeat Interval
	usleep(1000*getConfigurationKeyIntValue("HeartbeatInterval"));

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
		while (attemptsTried<attempts && !OKReceived[connectorId-1])
		{
			usleep(1000*interval * (++attemptsTried));
			p=checkMessageinMessageQueue(esteUniqueId);

			if (p)
			{
				//Actualizamos el UniqueId en cada nuevo envio
				esteUniqueId=getNextUniqueID_char();
				char *cad=replace(p->payload,UniqueId_Str,convert(esteUniqueId));
				p->UniqueId=esteUniqueId;
				p->payload=cad;
////				websocket_write_back(wsi, cad, -1);
				Enqueue(p);
			}
			else
			{
				break;
			}

			//attemptsTried++;
		}
	}

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

	usleep(1000*getConfigurationKeyIntValue("HeartbeatInterval"));

}

static void *sendHeartbeat(void *tool_in)
{
	//PAG 37
	//To let the Central System know that a Charge Point is still connected, a Charge Point sends a heartbeat
	//after a configurable time interval.
	//The Charge Point SHALL send a	Heartbeat.req PDU for ensuring that the Central System knows that a
	//Charge Point is still alive.

    while (1)
    {
    	if (_REGISTRATIONSTATUS==_RS_ACCEPTED)
    	{
    		//The chargepoint may skip sending a heartbeat PDU if another PDU has been sent to the Central System within the configured heartbeat interval
    		//This implies that a Central System Should assume availability of a CS whenever a PDU has been received.

    		if (anothermessagesent)
    		{
    			anothermessagesent=0;
    		}
    		else
    		{
    			queue_node *nuevo_mensaje;
    			int n=getNextUniqueID();
    			nuevo_mensaje = (queue_node *)calloc(1, sizeof(queue_node));
    			nuevo_mensaje->MessageAction=HEARTBEAT;
    			nuevo_mensaje->UniqueId=n;

    			nuevo_mensaje->payload=prepare_heartbeat_request(n);
    			Enqueue(nuevo_mensaje);
    		}
    	}
    	else
    	{
    		if (_REGISTRATIONSTATUS==_RS_NOTCONNECTED)
    		{
    			int n=1;
    		}
    		else
    		{
    			//Si el central system responde algo diferente a accepted el Heartinterval sera el tiempo que pase hasta que se mande un nuevo bootnotification

    			if (debug) printf("\n[LOG] el central system responde algo diferente a accepted el Heartinterval");


    			_REGISTRATIONSTATUS=_RS_PENDING;
    			queue_node *nuevo_mensaje;
    			int n=getNextUniqueID();
    			nuevo_mensaje = (queue_node *)calloc(1, sizeof(queue_node));
    			nuevo_mensaje->MessageAction=BOOT_NOTIFICATION;
    			nuevo_mensaje->UniqueId=n;

    			//nuevo_mensaje->payload=prepare_bootNotification_request(__VENDOR, __CHARGEPOINTMODEL, __CHARGEBOXSERIALNUMBER, __CHARGEPOINTSERIALNUMBER,__FIRMWAREVERSION, __ICCID, __IMSI, __METERSERIALNUMBER, __METERTYPE);

    			nuevo_mensaje->payload=prepare_bootNotification_request(getConfigurationKeyStringValue("ChargePointVendor"),getConfigurationKeyStringValue("ChargePointModel"),
            		getConfigurationKeyStringValue("ChargeBoxSerialNumber"),getConfigurationKeyStringValue("ChargePointSerialNumber"),
    				getConfigurationKeyStringValue("FirmwareVersion"),getConfigurationKeyStringValue("ChargePointICCID"), getConfigurationKeyStringValue("ChargePointIMSI"),
    				getConfigurationKeyStringValue("ChargePointMeterSerialNumber"),getConfigurationKeyStringValue("ChargePointMeterType"));

    			//char * vendor_data, char *chargepointmodel_data, char *chargeBoxSerialNumber_data, char *chargePointSerialNumber_data, char *firmwareVersion_data, char *ICCID_data, char *IMSI_data, char *meterSerialNumber_data, char *meterType_data) {
    			Enqueue(nuevo_mensaje);
    		}
    	}


    	//int hb=atoi(getConfigurationKeyStringValue("HeartbeatInterval"));
    	int hb=configurationValues[7].intValue;
    	usleep(hb*1000);

    	//Actualizamos el SQLITE en cada Heartbeat
    	updateHeartbeatInDB();

    	//Pag 15:
    	checkAuthorizationCacheEntriesValidity();
    }
}

//Este hilo comprueba si hay mensajes en cola y si los hay llama a onwritable
static void *hilo_conexion(void *tool_in)
{
    struct pthread_routine_tool *tool = tool_in;

    // Esperamos la conexion con el servidor. Esperamos 2 segundos
    int timer=0;
    while ((!connection_flag)&&(timer<2000000))
    {
    	usleep(1000*20);
    	timer+=20000;
    }

    if (timer>1800000)
    {
    	printf (ROJO"\n¡¡ERROR!! No se pudo conectar con el servidor"RESET);
    	exit (3);
    }

    while (1)
    {
    	if (writable)
    	{
    		lws_callback_on_writable(tool->wsi);
    	}

    	//Comprueba 2 veces cada 'Heartbeat interval' si hay nuevos mensajes
    	int hb=configurationValues[7].intValue;
    	usleep(hb*500);
    }
}

int CP_initialize()
{
		//Initializing Queue Message
		Message_queue = NULL;
		srand(time(NULL));
		guiready=0;
		///////////////////////////////////////
		//Authorization List
		//La primera version no puede ser la 0 (Ver pag 52). -1 es no inicializada
		authorizationList=NULL;
		localListVersion=-1;
		SQLITEMutex=0;

		///////////////////////////////////////
		//OTHER VALUES
		_REGISTRATIONSTATUS=_RS_NOTCONNECTED;
		lastDiagnosticsFileStatus=_DSN_IDLE;
		lastFirmwareUploadStatus=_FS_IDLE;
		busyUploadingDiagnosticsFile=0;
		sendConnectorStatusWhenOnline=0;
		chargingProfileInitialize();

		anothermessagesent=0;

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

		if (!SQLiteConnect())
		{
			readSQLiteConfigurationKeys();
		}
		else
		{
			//Ha habido algun problema al conectar a SQLite, asi que leemos de fichero

			///////////////////////
			//Reading INI file & ConfigurationKeys
			int error;
			if ((error=ini_parse("/etc/ocpp/ocpp_client.ini", handler22, NULL)) > 0)
			{
				printf("Can't load '/etc/ocpp/ocpp_client.ini'. Error: %d\n", error);
				return 1;
			}
		}

		//Comprueba si todas las autorization keys obligatorias han sido cargadas:
		//En error2 se almacena el numero de clave en la que ha fallado
		int error2=checkAllRequiredConfigurationKeys();
		if (error2>=0)
		{
			printf("\n[Critical Error]: Could not check all required configuration Keys\n");
			printf("Exiting\n");
			return (100+error2);
		}
		middleware_initialize(NUM_CONNECTORS);


		//LOG
		logfilename=getConfigurationKeyStringValue("LogFile");

		if (strcmp(logfilename,"DB"))
		{
			//log_to=DB;
		}
		else
		{
			//log_to=DB;
			if (!logfilename)
			{
					logfilename=(char *)calloc(1,64);
					strcpy(logfilename, "/var/log/ocpp/ocpp.log");
			}

			logfile=fopen(logfilename, "a");

			if (!logfile)
			{
				addLog("Log File %s not found with error '%s'!\n",logfilename,strerror(errno),LOG_WARNING, SCREEN);
				exit(53);
			}
		}

		if (isSQLiteConnected())
		{
			//Leemos los datos de los conectores de la BD
			readChargePointValuesFromSQLite();
		}
		else
		{
			//SI NO PODEMOS CONECTARNOS A SQLITE, LEE DE FICHERO

			char *filename=getConfigurationKeyStringValue("ChargePointDataFilename");

			if (!filename)
			{
				filename=(char *)calloc(1,64);
				strcpy(filename, "/tmp/ChargePointDataFilename.dat");
			}


			if (readCPStatusFromFile(filename))
			{
				//Existe el fichero
				readFromDisk(filename);
			}
			else
			{
				currentChargePointErrorStatus=_CP_ERROR_NOERROR;
				currentChargePointState=_CP_STATUS_AVAILABLE;
				for (int i=0; i<NUM_CONNECTORS; i++)
				{
					currentConnectorErrorStatus[i]=_CP_ERROR_NOERROR;
					connectorStatus[i]=_CP_STATUS_AVAILABLE;
					connectorValues[i]=0.0;
				}
			}
		}

		///////////SI FINALMENTE USAMOS EL GPIO/////////////
		initialize_GPIO();

		//Pag 44: The chargepoint may use the unavailable status internally
		//for other purposes like while waiting for an initial accepted registration status response.
		chargePointStatus=_CP_STATUS_UNAVAILABLE;

		return 0;
}

int main(int argc, char *argv[])
{
	//For reset purposes, we store original argc and argv values & pointers.
	original_argc=argc;
	original_argv=argv;

	int error=CP_initialize();
	if (error>0)
	{
		currentChargePointState=_CP_STATUS_FAULTED;
		currentChargePointErrorStatus=_CP_ERROR_INTERNALERROR;
		return error;
	}


	///////////////////////////////////////////////////////
	//
	//Websocket initialization
	//
	//  websocket_connect(&context, &wsi); //<--NO HE SIDO CAPAZ DE HACERLO FUNCIONAR
	struct lws_context *context;
	struct lws *wsi;

	context=NULL;
	wsi=NULL;
	struct lws_protocols protocol;
	struct sigaction act;
	struct lws_context_creation_info info;
	struct lws_client_connect_info i;
	static struct lws *wsi_dumb, *wsi_mirror;

	srand((unsigned int)time(NULL));
	act.sa_handler = INT_HANDLER;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction( SIGINT, &act, 0);

	memset(&info, 0, sizeof info);

	info.port = CONTEXT_PORT_NO_LISTEN;
    info.iface = NULL;
	info.protocols = &protocol;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
				       "ECDHE-RSA-AES256-GCM-SHA384:"
				       "DHE-RSA-AES256-GCM-SHA384:"
				       "ECDHE-RSA-AES256-SHA384:"
				       "HIGH:!aNULL:!eNULL:!EXPORT:"
				       "!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
				       "!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
				       "!DHE-RSA-AES128-SHA256:"
				       "!AES128-GCM-SHA256:"
				       "!AES128-SHA256:"
				       "!DHE-RSA-AES256-SHA256:"
				       "!AES256-GCM-SHA384:"
						"!AES256-SHA256";
	//info.ssl_cert_filepath = "/root/eclipse-workspace/example-server/Debug/cert.pem";
	//info.ssl_private_key_filepath = "/root/eclipse-workspace/example-server/Debug/key.pem";  //Fichero .key

	info.extensions = lws_get_internal_extensions();
	info.gid = -1;
	info.uid = -1;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

	char buf[20];
	sprintf(buf, "%s:%s", getConfigurationKeyStringValue("CentralSystemIP"), getConfigurationKeyStringValue("CentralSystemPort"));

	protocol.name="OCPP_Protocol";
	protocol.callback = &service_callback;
	protocol.per_session_data_size = sizeof(struct session_data);
	protocol.rx_buffer_size = 0;
	protocol.id = 0;
	protocol.user = NULL;


	sendToDisplay("Initializing...");
	////////////////////////////////////////////
	//Creamos el contexto Websocket
	context = lws_create_context(&info);

	//En caso de error salimos
	if (context == NULL)
	{
	   	if (debug) printf(ROJO"[Main] context is NULL.\n"RESET);
		return -1;
	}

	////////////////////////////////////////////
	//Nos conectamos con el servidor
	wsi = lws_client_connect(context, getConfigurationKeyStringValue("CentralSystemIP"), atoi(getConfigurationKeyStringValue("CentralSystemPort")), 0,"/webServices/ocpp/CP3211", buf, NULL,protocol.name, -1);

	usleep(50000);
	//En caso de error, salimos
	if (wsi == NULL)
	{
		if (debug) printf(ROJO"[Main] Error de creacion del WSI.\n"RESET);
		sendToDisplay("No se pudo conectar con el Sistema central. Trabajando en local.");
	//    return -1;
	}
	else connection_flag=1;

	////////////////////////////////////////////////
	//Creamos un thread que se conecta y comprueba cada medio segundo si hay mensajes que mandar
	struct pthread_routine_tool tool;
	tool.wsi = wsi;
	tool.context = context;

	pthread_t pid;
	pthread_create(&pid, NULL, hilo_conexion, &tool);
	pthread_detach(pid);

	char *systemName=getSystemName();
	if (systemName && strcmp(systemName,"chargepoint")==0)
	{
		//En el chargepoint no cargamos la interfaz gráfica
		debug=0;
	}
	else
	{
		debug=1;
		//Finalmente creamos el hilo del GUI
	//	gtk_init(&argc, &argv);
	//	pthread_t pidGUI;
		//pthread_create(&pidGUI, NULL, drawGUI, NULL);
		//pthread_detach(pidGUI);
	}

	////////////////////////////////////////////////
	//Creamos un thread que manda un heartbeat cada 'heartbeat' microsegundos
	pthread_t pidheartbeat;
	pthread_create(&pidheartbeat, NULL, sendHeartbeat, &tool);
	pthread_detach(pidheartbeat);

	//Puede haber tantas transacciones simultaneas como conectores
	//Este hilo prueba 3 veces los intentos de conexion y desconexion
	//pthread_t transactionPid[NUM_CONNECTORS];
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		OKReceived[i]=0;
	//	pthread_create(&transactionPid[i], NULL, sendTransactionRequests, &wsi);
	//	pthread_detach(transactionPid[i]);
	}

	//Creamos el thread que se dedicara a tratar con el lector RFID. Basicamente su trabajo consiste en, si se le pasa algun RFID, actualziar una varibale
	pthread_t pidNFC;
	pthread_create(&pidNFC, NULL, checkRFIDPassed, NULL);
	pthread_detach(pidNFC);

	//Creamos el thread de la interaccion entre usuario y raspberry (botones, rfids, etc)
	pthread_t pidHW;
	pthread_create(&pidHW, NULL, checkLoop, NULL);
	pthread_detach(pidHW);

	//Con esto cerramos aquellos logins y transacciones que podrian haberse quedado  sin cerrar por una caida inesperada de la raspberry
	closeLoginsAndTransactions();


	//Comprueba el estado de la conexion cada 500ms. Si se cae, vuelve a intentar iniciar la conexion
	while(!destroy_flag)
	{
		//connection flag se pone a uno cuando el websocket detecta que ha habido establecimiento de la conexion (En LWS_CALLBACK_CLIENT_ESTABLISHED)
		if (!connection_flag)
		{
			Message_queue=NULL;
			lastSentAcceptedUniqueId=UniqueId;

			if (!connection_flag) wsi = lws_client_connect(context, getConfigurationKeyStringValue("CentralSystemIP"),
					atoi(getConfigurationKeyStringValue("CentralSystemPort")), 0,"/webServices/ocpp/CP3211", buf, NULL,protocol.name, -1);

			usleep(500000);
			//wsi=NULL;
		}

		//Ejecuta la funcion service_callback cada 500ms
		lws_service(context, 500);
	}

	//Al salir, limpia todo
//	lws_context_destroy(context);
	return 0;
}
