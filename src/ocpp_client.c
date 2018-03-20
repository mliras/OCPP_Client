#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ocpp_client.h"


char *generateIdTag()
{
	int idTokenLength=20;
	char *idToken=(char *)calloc(1, sizeof(char) * idTokenLength);

	//Here we should prepare the idTag. It's not well explained neither in 4.8 nor in 6.45, nor in 7.28 sections
	static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	if (idToken) {
		for (int n = 0;n < idTokenLength; n++) {
	    	int key = rand() % (int)(sizeof(charset) -1);
	        idToken[n] = charset[key];
		}
	            idToken[idTokenLength] = '\0';
    }

	return idToken;
}

char *getCurrentTime()
{
    //Recuperamos fecha y hora
    time_t now;
    time(&now);
    struct tm* now_tm;
    now_tm = localtime(&now);
    char *currentTime=(char *)calloc(1, sizeof(char)*80);
    strftime (currentTime, 80, "%Y-%m-%dT%H:%M:%S.", now_tm);
    currentTime[79]='\0';
    return currentTime;
}

//Esta funcion crea un mensaje de JSON de peticion vacío con el MessageType, el UniqueId y la action que se le pasan como parámetro
//El mensaje creado no tiene payload, que se le debera añadir mas adelante
//Tiene en cuenta si la variable send_tags esta activada o no
char *prepareWrapper(int MessageType, int UniqueId, char *action)
{
  char *string=(char *) calloc(1, sizeof(char) * 190);

  if (send_tags){
	  sprintf(string, "{\"MessageTypeId\":%d,\"UniqueId\":\"%d\",\"Action\":\"%s\",\"Payload\":", MessageType, UniqueId, action); //Optional
	}
  else
  {
	  sprintf(string, "[%d,\"%d\",\"%s\",",MessageType, UniqueId, action); //Optional);
  }
  string[strlen(string)]='\0';
  return string;
}

//Esta funcion crea un mensaje JSON de respuesta vacío con el MessageType, el UniqueId que se le pasan como parámetro
//Los mensajes de respuesta no tienen action.
//El mensaje creado no tiene payload, que se le debera añadir mas adelante
//Tiene en cuenta si la variable send_tags esta activada o no
char *prepareResponseWrapper(int MessageType, const char *UniqueId)
{
  char *string=(char *) calloc(1, sizeof(char) * 190);

  if (send_tags){
	  sprintf(string, "{\"MessageTypeId\":%d,\"UniqueId\":\"%s\",\"Payload\":", MessageType, UniqueId); //Optional
	}
  else
  {
	  sprintf(string, "[%d,\"%s\",",MessageType, UniqueId); //Optional);
  }
  string[strlen(string)]='\0';
  return string;
}

//Igual que el anterior, pero el UniqueId que se le pasa es un entero y no una cadena.
char *prepareResponseWrapper_i(int MessageType, int UniqueId)
{
  char *string=(char *) calloc(1, sizeof(char) * 190);

  if (send_tags){
	  sprintf(string, "{\"MessageTypeId\":%d,\"UniqueId\":\"%d\",\"Payload\":", MessageType, UniqueId); //Optional
	}
  else
  {
	  sprintf(string, "[%d,\"%d\",",MessageType, UniqueId); //Optional);
  }
  string[strlen(string)]='\0';
  return string;
}

//
// 4.1. AUTHORIZE
//
char * prepare_authorize_request(int n, const char * idtagtext)
{
    char *message=(char *) calloc(1, sizeof(char) * 200);
    char *string=prepareWrapper(_CALLMESSAGE, n, "Authorize");
    sprintf(message,"%s{\"idTag\":\"%s\"}", string, idtagtext);

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

//
// 4.2. BOOT NOTIFICATION
//
//Este método prepara el JSON que se enviará desde el CP al CS para la solicitd "Boot Notification".
//Coge los datos de constantes en código y del fichero .INI
char * prepare_bootNotification_request(char * vendor_data, char *chargepointmodel_data, char *chargeBoxSerialNumber_data, char *chargePointSerialNumber_data, char *firmwareVersion_data, char *ICCID_data, char *IMSI_data, char *meterSerialNumber_data, char *meterType_data)
{
  	  char *string=(char *) calloc(1, sizeof(char) * 3000);
	  char *chargeBoxSerialNumber=(char *) calloc(1, sizeof(char) * 256);
	  char *chargePointSerialNumber=(char *) calloc(1, sizeof(char) * 256);
	  char *firmwareVersion=(char *) calloc(1, sizeof(char) * 256);
	  char *ICCID=(char *) calloc(1, sizeof(char) * 256);
	  char *IMSI=(char *) calloc(1, sizeof(char) * 256);
	  char *meterSerialNumber=(char *) calloc(1, sizeof(char) * 256);
	  char *meterType=(char *) calloc(1, sizeof(char) * 256);

	  queue_node *nuevo_mensaje;
	  int n=getNextUniqueID();
	  nuevo_mensaje = (queue_node *)calloc(1, sizeof(queue_node));
	  nuevo_mensaje->MessageAction=BOOT_NOTIFICATION;
	  nuevo_mensaje->UniqueId=n;

  //Ver Pag 34. Recuperamos los valores del Payload desde el fichero de configuración
  if (chargeBoxSerialNumber_data!=NULL){
  		  sprintf(chargeBoxSerialNumber, ",\"chargeBoxSerialNumber\":\"%s\"", chargeBoxSerialNumber_data);
  	  }
  else sprintf(chargeBoxSerialNumber, "", "");

  if (chargePointSerialNumber_data!=NULL){
	  sprintf(chargePointSerialNumber, ",\"chargePointSerialNumber\":\"%s\"", chargePointSerialNumber_data);
  }
  else sprintf(chargePointSerialNumber, "", "");


  if (firmwareVersion_data!=NULL){
  	  sprintf(firmwareVersion, ",\"firmwareVersion\":\"%s\"", firmwareVersion_data);
    }
    else sprintf(firmwareVersion, "", "");

  if (ICCID_data!=NULL){
  	  sprintf(ICCID, ",\"iccid\":\"%s\"", ICCID_data);
    }
    else sprintf(ICCID, "", "");

  if (IMSI_data!=NULL){
  	  sprintf(IMSI, ",\"imsi\":\"%s\"", IMSI_data);
    }
    else sprintf(IMSI, "", "");


  if (meterSerialNumber_data!=NULL){
   	  sprintf(meterSerialNumber, ",\"meterSerialNumber\":\"%s\"", meterSerialNumber_data);
     }
     else sprintf(meterSerialNumber, "", "");


  if (meterType_data!=NULL){
   	  sprintf(meterType, ",\"meterType\":\"%s\"", meterType_data);
     }
     else sprintf(meterType, "", "");


  if (send_tags){
	  sprintf(string, "{\"MessageTypeId\":%d,\"UniqueId\":\"%d\",\"Action\":\"BootNotification\",\"Payload\":{\"chargePointVendor\":\"%s\",\"chargePointModel\":\"%s\"%s%s%s%s%s%s%s}}",
			  2, n, vendor_data, chargepointmodel_data, //Mandatory
			  chargeBoxSerialNumber, chargePointSerialNumber, firmwareVersion, ICCID, IMSI, meterSerialNumber,meterType); //Optional
	}
  else
  {
	  sprintf(string, "[%d,%d,\"BootNotification\",{\"chargePointVendor\":\"%s\",\"chargePointModel\":\"%s\"%s%s%s%s%s%s%s}]",
			  2, n, vendor_data, chargepointmodel_data, //Mandatory
			  chargeBoxSerialNumber, chargePointSerialNumber, firmwareVersion, ICCID, IMSI, meterSerialNumber,meterType); //Optional);
  }

  nuevo_mensaje->payload=string;

   Enqueue(nuevo_mensaje);

  return string;
}

//
// 4.3. DATA TRANSFER
//
// prepare_dataTransfer_request: Este método prepara la cadena JSON de una petición de dataTransfer que se mandará por JSON
//
// NOTAS: Lo dejo preparado para que tire de lo que le pasamos como parámetro, pero en la primera prueba, obtiene los datos de _DATA
//

char * prepare_dataTransfer_request(int n, const char *vendorId, const char *messageId, const char *data)
{
	char *string=(char *) calloc(strlen(data)+strlen(messageId)+strlen(vendorId)+12+16, sizeof(char));

	if (string)
	{
//		string[strlen(string)]='\0';

		//TAGS OPCIONALES
		char *messageIdtext,*datatext, *vendorText;
		vendorText=(char *) calloc(1, strlen(vendorId));
		strncpy(vendorText, vendorId, strlen(vendorId));

		//messageId & data
		messageIdtext=(char *) calloc(strlen(messageId)+16, sizeof(char)); //Tamaño de 50 indicado en la pag
		datatext=(char *) calloc(strlen(data)+9, sizeof(char));
		//messageIdtext[strlen(messageIdtext)]='\0';
		//datatext[strlen(datatext)]='\0';

		if (strlen(messageId)>0)
		{
			sprintf(messageIdtext, ",\"messageId\":\"%s\"", messageId);

		}

		if (strlen(data)>0)
		{
			sprintf(datatext, ",\"data\":\"%s\"", data);
		}

		//Preparamos el mensaje con tags o sin ellos.
		if (send_tags){
			strcpy(string, "{\"MessageTypeId\":2,\"UniqueId\":\"");
			strcat(string, convert(n));
			strcat(string, "\",\"Action\":\"DataTransfer\",\"Payload\":{\"vendorId\":\"");
			strcat(string, vendorText);
			strcat(string, "\"");
			strcat(string, messageIdtext);
			strcat(string, datatext);
			strcat(string, "}}");
			string[strlen(string)]='\0';
		}
		else
		{
			int n=0;
			sprintf(string, "[%d,%d,\"DataTransfer\",{\"vendorId\":\"%s\"%s%s}]",2, n, vendorId,  messageIdtext, datatext); //Optional
		}

		if (messageIdtext) free(messageIdtext);
		if (datatext) free(datatext);
		if (vendorText) free(vendorText);
	}
	return string;
}


//
// 4.4. DIAGNOSTICS STATUS NOTIFICATION
//
//Este método prepara el JSON que se enviará desde el CP al CS para la solicitd "Diagnostics Status Notification".
//Coge los datos de constantes en código y del fichero .INI
//
// Pag 36:
// Charge Point sends a notification to inform the Central System about the status of a diagnostics upload.
// The  Charge  Point  SHALL  send  a DiagnosticsStatusNotification.req   PDU  to  inform  the  Central  System
// that  the  upload  of  diagnostics  is  busy  or  has  finished  successfully  or  failed.  The  Charge  Point  SHALL
// only send the status Idle after receipt of a TriggerMessage for a Diagnostics Status Notification, when it
// is not busy uploading diagnostics.
////
char * prepare_diagnosticsStatusNotification_request(int n, int status_val)
{
	char *wrapper=prepareWrapper(_CALLMESSAGE, n, "DiagnosticsStatusNotification");

    //El mensaje tiene como longitud máxima la suma de:
    //120 es la cadena de abajo como maximo
    //10 de buf
    //10 de la suma de UniqueId+MessageTtypeId
    //La longitud de status que es variable
    char *message=(char *) calloc(190, sizeof(char));

    if (enums_as_integers)
    {
    	sprintf(message, "%s{\"Status\":%d}",wrapper, status_val);
    }
    else
    {
    	const char *status=DiagnosticsStatus_texts[status_val];

    	sprintf(message, "%s{\"Status\":\"%s\"}",wrapper, status);
    }

    if (send_tags) strcat(message,"}");
    else strcat(message,"]");


	free(wrapper);

    return message;
}

//
// 4.5 Firmware Status Notification
//
// A  Charge  Point  sends  notifications  to  inform  the  Central  System  about  the  progress  of  the  firmware
//update. The Charge Point SHALL send a FirmwareStatusNotification.req  PDU for informing the Central System about
// the progress of the downloading and installation of a firmware update. The Charge Point SHALL  only  send  the
// status  Idle  after  receipt  of  a  TriggerMessage  for  a  Firmware  Status  Notification, when it is not busy
// downloading/installing firmware.

char * prepare_firmwareStatusNotification_request(int uniqueId, int status_val)
{
	char *wrapper=prepareWrapper(_CALLMESSAGE, uniqueId, "FirmwareStatusNotification");

	char *message=(char *) calloc(190, sizeof(char));

	if (enums_as_integers)
	{
	   	sprintf(message, "%s{\"Status\":%d}",wrapper, status_val);
	}
	else
	{
		const char *status=FirmwareStatus_texts[status_val];

    	sprintf(message, "%s{\"Status\":\"%s\"}",wrapper, status);
    }

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");


	free(wrapper);

    return message;
}


//
// 4.6 Heartbeat
//
// To let the Central System know that a Charge Point is still connected, a Charge Point sends a heartbeat
// after a configurable time interval.
//

char * prepare_heartbeat_request(int n) {

	char *wrapper=prepareWrapper(_CALLMESSAGE, n, "Heartbeat");

    //El mensaje tiene como longitud máxima la suma de:
    //120 es la cadena de abajo como maximo
    //10 de buf
    //10 de la suma de UniqueId+MessageTtypeId
    //La longitud de status que es variable
    char *message=(char *) calloc(1, sizeof(char) * 96);

    sprintf(message, "%s{}",wrapper);

    if (send_tags) strcat(message,"}");
    	else strcat(message,"]");


    free(wrapper);

    return message;
}

//
// 4.7 Meter values
//
// A  Charge  Point  MAY  sample  the  energy  meter  or  other  sensor/transducer  hardware  to  provide  extra
// information  about  its  meter  values.  It  is  up  to  the  Charge  Point  to  decide  when  it  will  send  meter
// values.   This   can   be   configured   using   the  ChangeConfiguration.req   message   to   data   acquisition
// intervals and specify data to be acquired & reported.
//

//ConnectorId has a value from 0 to N-1
char * prepare_metervalues_request(int transactionId, int numMeterValues, int uniqueId, int connectorId, int numSamples, struct SampledValue value_list[1][numSamples])
{
	char *wrapper=prepareWrapper(_CALLMESSAGE, uniqueId, "MeterValues");
	char *message=(char *) calloc(1, sizeof(char) * 4096);
    char *currentTime=getCurrentTime();
   /// printf("en el ultimo sv es %s",value_list[0][0].value);

    if (transactionId>=0)
    {
    	sprintf(message, "%s{\"connectorId\":%d,\"transactionId\":%d,\"meterValue\":[",wrapper,connectorId+1,transactionId);
    }
    else
    {
    	sprintf(message, "%s{\"connectorId\":%d,\"meterValue\":[",wrapper,connectorId+1);
    }

    for (int i=0; i<numMeterValues; i++)
    {
    	if (i==0) strcat(message, "{\"timestamp\":\"");
    	else strcat(message, ",{\"timestamp\":\"");

		strcat(message, currentTime);
		strcat(message, "\",\"sampledValue\":[");
		for (int j=0; j<numSamples; j++)
		{
			if (value_list[i][j].value!=NULL)
			{
				if (j==0)
				{
					//Si es el primer sample, no ponemos coma
					strcat(message, "{");
				}
				else
				{
					//Si no es el primer sample, ponemos una coma
					strcat(message, ",{");
				}

				strcat(message, "\"value\":\"");
				strncat(message, value_list[i][j].value, strlen(value_list[i][j].value));

				if (value_list[i][j].measurand>=0 && value_list[i][j].measurand<22)
				{
					strcat(message, "\",\"measurand\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].measurand));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, Measurand_texts[value_list[i][j].measurand]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].unit>=0 && value_list[i][j].unit<17)
				{
					strcat(message, ",\"unit\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].unit));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, UnitOfMeasureTexts[value_list[i][j].unit]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].context>=0 && value_list[i][j].context<8)
				{
					strcat(message, ",\"context\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].context));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, ReadingContextTexts[value_list[i][j].context]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].location>=0 &&  value_list[i][j].location<5)
				{
					strcat(message, "\",\"location\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].location));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, Location_texts[value_list[i][j].location]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].formato>=0 &&  value_list[i][j].formato<2)
				{
					strcat(message, ",\"format\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].formato));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, ValueFormatTexts[value_list[i][j].formato]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].phase>=0 &&  value_list[i][j].phase<10)
				{
					strcat(message, ",\"phase\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].phase));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, Phase_texts[value_list[i][j].phase]);
						strcat(message,"\"");
					}
				}

				strcat(message, "}");

    		}
    	}//FIN FOR NUMSAMPLES
		strcat(message, "]}");
    }

    strcat(message, "]}");

	if (send_tags) strcat(message,"}");
    else strcat(message,"]");

    free(wrapper);

    //printf("\n\nmesssage es: %s", message);
    return message;
}

//
// 4.8 Start Transaction
//
//The  Charge  Point  SHALL  send  a StartTransaction.req   PDU  to  the  Central  System  to  inform  about  a
//transaction  that  has  been  started.  If  this  transaction  ends  a  reservation  (see Reserve  Now operation),
//then the StartTransaction.req MUST contain the reservationId.
//
//ConnectorId es un valor de 1 a N
char * prepare_starttransaction_request(int n, int connectorId, int meterStartValue, const char *idTag, int reservationId) {

	char *wrapper=prepareWrapper(_CALLMESSAGE, n, "StartTransaction");
    char *currentTime=getCurrentTime();
    char buf[10];
    sprintf(buf, "%d", n);

    //El mensaje tiene como longitud máxima la suma de:
    //120 es la cadena de abajo como maximo
    //10 de buf
    //10 de la suma de UniqueId+MessageTtypeId
    //La longitud de status que es variable
    char *message=(char *) calloc(400,sizeof(char));

    if (reservationId>0)
    {
    	sprintf(message, "%s{\"connectorId\":%d, \"idTag\":\"%s\", \"meterStart\":%d, \"reservationId\":%d, \"timestamp\":\"%s\"}",wrapper, connectorId, idTag, meterStartValue, reservationId , currentTime);
    }
    else
    {
    	sprintf(message, "%s{\"connectorId\":%d, \"idTag\":\"%s\", \"meterStart\":%d, \"timestamp\":\"%s\"}",wrapper, connectorId, idTag, meterStartValue, currentTime);
    }

    if (send_tags) strcat(message,"}");
    	else strcat(message,"]");

   	free(wrapper);
    return message;
}

//
// 4.9 Status Notification. Ver 6.47
//
// A  Charge  Point  sends  a  notification  to  the  Central  System  to  inform  the  Central  System  about  a  status
// change or an error within the Charge Point.

char * prepare_statusnotification_request(int n, const int connectorId, int newStatus, int errorCode, char *extraInfo, char *vendorId, char *vendorErrorCode) {

    char *wrapper=prepareWrapper(_CALLMESSAGE, n, "StatusNotification");

	char *currentTime=getCurrentTime();

	//Max Payload Size is 4+64+50+64+8+255+50=495. See 6.47
    char *payload=(char *) calloc(1, sizeof(char) * 495);

    sprintf(payload, "\"connectorId\":%d, \"errorCode\":\"%s\", \"status\":\"%s\", \"timestamp\":\"%s\"",connectorId, ChargePointErrorCodeTexts[errorCode], ChargePointStatusTexts[newStatus], currentTime);
    if (extraInfo)
    {
    	strcat(payload, ",\"info\":\"");
    	strncat(payload, extraInfo, 50);
    	strcat(payload, "\"");
    }

    if (vendorId)
    {
       	strcat(payload, ",\"vendorId\":\"");
       	strncat(payload, vendorId, 255);
       	strcat(payload, "\"");
    }

    if (vendorErrorCode)
    {
    	strcat(payload, ",\"vendorErrorCode\":");
        strncat(payload, vendorErrorCode, 50);
        strcat(payload, "\"");
    }

    //Max Message Size is 495 (Payload) + 10 + 93 (wrapper)
    char *message=(char *) calloc(1, sizeof(char) * 600);

    sprintf(message, "%s{%s}",wrapper, payload);

    if (send_tags) strcat(message,"}");
    	else strcat(message,"]");


    free(wrapper);
    return message;
}


// 4.10 Start Transaction
//
//When  a  transaction  is  stopped,  the  Charge  Point  SHALL  send  a StopTransaction.req  PDU,  notifying  to
//the Central System th	at the transaction has stopped.
//
char *prepare_stoptransaction_request(int n, int meterStopValue, int numMeterValues, int numSamples, int transactionId, int reason, char *idTag, struct SampledValue value_list[numMeterValues][numSamples]) {

	char *wrapper=prepareWrapper(_CALLMESSAGE, n, "StopTransaction");

    char *currentTime=getCurrentTime();
  //  printf("\nNUM SAMPLES ES %d", numSamples);
   // printf("\nvalues list 0,0 es %s",value_list[0][0].value);
    //El mensaje tiene como longitud máxima la suma de:
    //203 de la cadena de abajo
    //6 por cada entero 6*5=30
    //6 del UniqueId
    //20 del idTag
    //20 del timeStamp
    //TOTAL: 255
    char *message=(char *) calloc(4096, sizeof(char));

    //reason
    if (reason>=0)
    	sprintf(message, "%s{\"meterStop\":%d, \"timestamp\":\"%s\", \"transactionId\":%d, \"reason\":%d",wrapper, meterStopValue, currentTime, transactionId, reason);
    else
    	sprintf(message, "%s{\"meterStop\":%d, \"timestamp\":\"%s\", \"transactionId\":%d",wrapper, meterStopValue, currentTime, transactionId);

    //idTag
   	if (idTag)
   	{
   		strcat(message,", \"idTag\":\"");
    	strncat(message, idTag, strlen(idTag));
    	strcat(message,"\"");
   	}

   	if (numMeterValues>0)
   		strcat(message, ",\"transactionData\":");


    for (int i=0; i<numMeterValues; i++)
    {
		strcat(message, "{\"timestamp\":\"");
		strcat(message, currentTime);
		strcat(message, "\",\"sampledValue\":[");

		for (int j=0; j<numSamples; j++)
		{
			if (value_list[i][j].value!=NULL)
			{
				if (j==0)
				{
					//Si es el primer sample, no ponemos coma
					strcat(message, "{");
				}
				else
				{
					//Si no es el primer sample, ponemos una coma
					strcat(message, ",{");
				}

				strcat(message, "\"value\":\"");
				strncat(message, value_list[i][j].value, strlen(value_list[i][j].value));

				if (value_list[i][j].measurand>=0 && value_list[i][j].measurand<22)
				{
					strcat(message, "\",\"measurand\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].measurand));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, Measurand_texts[value_list[i][j].measurand]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].unit>=0 && value_list[i][j].unit<16)
				{
					strcat(message, ",\"unit\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].unit));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, UnitOfMeasureTexts[value_list[i][j].unit]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].context>=0 && value_list[i][j].context<8)
				{
					strcat(message, ",\"context\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].context));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, ReadingContextTexts[value_list[i][j].context]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].location>=0 &&  value_list[i][j].location<5)
				{
					strcat(message, "\",\"location\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].location));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, Location_texts[value_list[i][j].location]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].formato>=0 &&  value_list[i][j].formato<2)
				{
					strcat(message, ",\"format\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].formato));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, ValueFormatTexts[value_list[i][j].formato]);
						strcat(message,"\"");
					}
				}

				if (value_list[i][j].phase>=0 &&  value_list[i][j].phase<10)
				{
					strcat(message, ",\"phase\":");
					if (enums_as_integers)
					{
						strcat(message, convert(value_list[i][j].phase));
					}
					else
					{
						strcat(message,"\"");
						strcat(message, Phase_texts[value_list[i][j].phase]);
						strcat(message,"\"");
					}
				}

				//strcat(message, "}");

				if (i==(numMeterValues-1))
				{
					//strcat(message, "]}");
					strcat(message, "}");
				}
				else
				{
					//strcat(message, "]},");
					strcat(message, "},");
				}
    		}
    	}//FIN NUMSAMPLES
    }

    strcat(message, "]}");

	if (send_tags) strcat(message,"}}");
   		else strcat(message,"}]");

 //  	free(wrapper);
    return message;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
//ERROR RESPONSE. Los error codes estan en
//
char *prepareErrorResponse(const char * UniqueId_str, int error_code)
{
	if (error_code >19) return NULL;  //Invalid error code. Se supone que son valores de 0 a 9. Indico 19 por si en el futuro se añaden nuevos.

	char *uniqueid;
	if (!UniqueId_str)
	{
		uniqueid="0";
	}
	else
	{
		uniqueid=strdup(UniqueId_str);
	}

	char *message=(char *) calloc(3000, sizeof(char));

	char *errorCode=(char *) calloc(3, sizeof(char));  //Se entiende que el error code nunca va a tener mas de 2 digitos.
	char *errorDescription;

	snprintf (errorCode, sizeof(errorCode), "%d",error_code);

	switch (error_code){
		case _NOT_IMPLEMENTED:
			errorDescription="Requested Action is not known by receiver";
			break;
		case _NOT_SUPPORTED:
			errorDescription="Requested Action is recognized but not supported by the receiver";
			break;
		case _INTERNAL_ERROR:
			errorDescription="An internal error occurred and the receiver was not able to process the requested Action successfully";
			break;
		case _PROTOCOL_ERROR:
			errorDescription="Payload for Action is incomplete";
			break;
		case _SECURITY_ERROR:
			errorDescription="During the processing of Action a security issue occurred preventing receiver from completing the Action successfully";
			break;
		case _FORMATION_VIOLATION:
			errorDescription="Payload for Action is syntactically incorrect or not conform the PDU structure for Action";
			break;
		case _PROPERTY_CONSTRAINT_VIOLATION:
			errorDescription="Payload is syntactically correct but at least one field contains an invalid value";
			break;
		case _OCCURENCE_CONSTRAINT_VIOLATION:
			errorDescription="Payload for Action is syntactically correct but at least one of the fields violates ocurence constraints";
			break;
		case _TYPE_CONSTRAINT_VIOLATION:
			errorDescription="Payload for Action is syntactically correct but at least one of the fields violates data type constraints";
			break;
		case _GENERIC_ERROR:
			errorDescription="Any other error not covered by the previous ones";
			break;
	}

	 if (send_tags)
	 {
		sprintf(message, "{\"MessageTypeId\":%d,\"UniqueId\":\"%s\",\"errorCode\":\"%s\",\"errorDescription\":\"%s\",\"errorDetails\":%s}",_CALLERROR, uniqueid, errorCode, errorDescription, "{}"); //Mandatory
	 }
	 else
	 {
		sprintf(message, "[%d,\"%s\",\"%s\",\"%s\",{\"%s\"}]",_CALLERROR, uniqueid, errorCode, errorDescription, ""); //Mandatory
	 }

	// free (uniqueid);

	 return message;
}

//5.1
char *prepare_cancelreservation_response(const char *uniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, uniqueId_str);

	char *message=(char *) calloc(256,sizeof(char));
	sprintf(message, "%s{\"status\":%d}",wrapper, status );

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

//5.2
char *prepareChangeAvailabilityResponse(const char *uniqueId, int responseStatus)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, uniqueId);

	 char *message=(char *) calloc(256, sizeof(char));
	 sprintf(message, "%s{\"status\":%d}}",wrapper, responseStatus );

	return message;
}

//5.3
char *prepareChangeConfigurationResponse(const char *UniqueId_str, int responseStatus)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));
	sprintf(message, "%s{\"status\":%d}",wrapper, responseStatus );

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

//5.4
char * prepareClearCacheResponse(const char *UniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

    char *message=(char *) calloc(256, sizeof(char));
	sprintf(message, "%s{\"status\":%d}",wrapper, status );

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

//5.5
char *prepareClearChargingProfileResponse(const char *UniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		sprintf(message, "%s{\"status\":\"%s\"}",wrapper, clearChargingProfileStatustexts[status]);
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

//5.6
char * prepareDataTransferResponse(const char * UniqueId_str, int status, char *data)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	//printf("PQR");
    char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
    {
	    if (!data)
	    {
	    	sprintf(message, "%s{\"status\":%d}",wrapper, status);
	    }
	    else
	    {
	    	sprintf(message, "%s{\"status\":%d, \"data\":\"%s\"}",wrapper, status, data );
	    }
    }
	else
	{
	    if (!data)
	    {
	    	sprintf(message, "%s{\"status\":\"%s\"}",wrapper, dataTransferStatus_texts[status]);
	    }
	    else
	    {
	    	sprintf(message, "%s{\"status\":\"%s\", \"data\":\"%s\"}",wrapper, dataTransferStatus_texts[status], data );
	    }
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

// 5.8
char *prepare_getconfiguration_response(const char *UniqueId_str, char *response_string, char *response_error_string)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);
	char *message=(char *) calloc(4096,1);

	sprintf(message, "%s{", wrapper);
	if (strlen(response_string)>0)
	{
			strcat(message, "\"configurationKey\":");
			strncat(message, response_string,strlen(response_string));
			strcat(message, "");
	}

	if (strlen(response_error_string)>0)
	{
		if (strlen(response_string)>0) strcat(message, ",");
		strcat(message, "\"unknownKey\":");
		strncat(message, response_error_string,strlen(response_error_string));
		strcat(message, "");
	}

	if (send_tags) strcat(message,"}}");
	else strcat(message,"}]");

	return message;
}

////////////////////////////
//5.9
char *prepare_getDiagnostics_response(const char *UniqueId_str, char *filename)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	 char *message=(char *) calloc(256, sizeof(char));

	 if (filename)
	 {
		 sprintf(message, "%s{\"fileName\":\"%s\"}",wrapper, filename);
	 }
	 else sprintf(message, "%s{}",wrapper);

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.10
char *prepare_getLocalListVersion_response(const char *UniqueId_str, int listVersion)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));
	sprintf(message, "%s{\"listVersion\":%d}",wrapper, listVersion );

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.11
char *prepare_remotestarttransaction_response(const char *uniqueid_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, uniqueid_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		sprintf(message, "%s{\"status\":\"%s\"}",wrapper, RemoteStartStopStatus_texts[status]);
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.12
char *prepare_remotestoptransaction_response(const char *uniqueid_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, uniqueid_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		sprintf(message, "%s{\"status\":\"%s\"}",wrapper, RemoteStartStopStatus_texts[status]);
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.13
char * prepare_reservenow_response(const char * UniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		 sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		 sprintf(message, "%s{\"status\":\"%s\"}",wrapper, ReservationStatus_texts[status] );
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.14
char * prepare_reset_response(const char * UniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		 sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		 sprintf(message, "%s{\"status\":\"%s\"}",wrapper, ResetType_texts[status] );
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.15
char *prepare_sendlocallist_response(const char *UniqueId_str, int retorno)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		sprintf(message, "%s{\"status\":%d}",wrapper, retorno );
	}
	else
	{
		sprintf(message, "%s{\"status\":\"%s\"}",wrapper, UpdateStatus_texts[retorno] );
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.16
char *prepare_setchargingprofile_response(char *UniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		 sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		 sprintf(message, "%s{\"status\":\"%s\"}",wrapper, chargingProfileStatusTexts[status] );
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.17
char * prepare_triggermessage_response(const char * UniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		 sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		 sprintf(message, "%s{\"status\":\"%s\"}",wrapper, TriggerMessageStatus_texts[status] );
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.18
char * prepare_unlockconnector_response(const char *UniqueId_str, int status)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	if (enums_as_integers)
	{
		sprintf(message, "%s{\"status\":%d}",wrapper, status );
	}
	else
	{
		sprintf(message, "%s{\"status\":\"%s\"}",wrapper, UnlockStatus_texts[status] );
	}

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}

////////////////////////////
//5.19
char *prepare_updatefirmware_response(const char *UniqueId_str)
{
	char *wrapper=prepareResponseWrapper(_CALLRESULT, UniqueId_str);

	char *message=(char *) calloc(256, sizeof(char));

	sprintf(message, "%s{}",wrapper);

	if (send_tags) strcat(message,"}");
	else strcat(message,"]");

	return message;
}
