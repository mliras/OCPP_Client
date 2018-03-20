/*
 * exploreNFC.c
 *
 *  Created on: 1 Mar 2018
 *      Author: pi
 */


/*
*         Copyright (c), NXP Semiconductors Gratkorn / Austria
*
*                     (C)NXP Semiconductors
*       All rights are reserved. Reproduction in whole or in part is
*      prohibited without the written consent of the copyright owner.
*  NXP reserves the right to make changes without notice at any time.
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
*particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
*                          arising from its use.
*/
/*
 * \file basic.c
 */

#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include "exploreNFC.h"


//Esta funcion prepara todo para que, despues de cada lectura de una tarjeta siga leyendo nuevamente pasados unos milisegundos
static gint startPolling(params_t* pParams)
{
	//List current adapters
	errorCode_t	err;

	int adaptersCount = 0;
	char** adapterNamesArray = NULL;
	err = neardal_get_adapters(&adapterNamesArray, &adaptersCount);
	if( err == NEARDAL_ERROR_NO_ADAPTER )
	{
		addLog("No adapter found\r\n",NULL,NULL,LOG_ERROR,ANY);
		return 1;
	}
	else if(err != NEARDAL_SUCCESS)
	{
		addLog("Error %s when listing adapters (%s)\r\n", convert(err), neardal_error_get_text(err),  LOG_WARNING, ANY);
		return 1;
	}

	gboolean adapterFound = FALSE;

	for(int i = 0; i < adaptersCount; i++)
	{
		if( pParams->adapterObjectPath != NULL )
		{
			if(strcmp(pParams->adapterObjectPath, adapterNamesArray[i]) != 0)
			{
				continue;
			}
		}

		//Reconfigure adapters if needed
		neardal_adapter* pAdapter;
		err = neardal_get_adapter_properties(adapterNamesArray[i], &pAdapter);
		if(err != NEARDAL_SUCCESS)
		{
			addLog("Error when accessing adapter %s (%s)\r\n", adapterNamesArray[i], neardal_error_get_text(err),LOG_WARNING,ANY);
			continue;
		}

		if( !pAdapter->powered )
		{
			addLog("Powering adapter %s\r\n", pAdapter->name, NULL, LOG_INFO,ANY);
			err = neardal_set_adapter_property(pAdapter->name, NEARD_ADP_PROP_POWERED, GUINT_TO_POINTER(1));
			if(err != NEARDAL_SUCCESS)
			{
				addLog("Error when trying to power adapter %s (%s)\r\n", pAdapter->name, neardal_error_get_text(err), LOG_WARNING,ANY);
			}
		}

		if( pAdapter->polling == 0 )
		{
			//Start polling
			addLog("Starting polling for adapter %s\r\n", pAdapter->name, NULL,LOG_INFO, ANY);

			err = neardal_start_poll(pAdapter->name); //Mode NEARD_ADP_MODE_INITIATOR
			if(err != NEARDAL_SUCCESS)
			{
				addLog("Error when trying to start polling on adapter %s (%s)\r\n", pAdapter->name, neardal_error_get_text(err), LOG_WARNING, ANY);
			}
		}

		neardal_free_adapter(pAdapter);

		adapterFound = TRUE;
		if( pParams->adapterObjectPath != NULL )
		{
			break;
		}
	}
	neardal_free_array(&adapterNamesArray);

	if(!adapterFound)
	{
		addLog("Adapter not found\r\n",NULL,NULL,LOG_ERROR,ANY);
		return 1;
	}

	return 0; //OK
}

//Esta funcion convierte una ristra de bytes en un char *.
static gchar* bytesToStr(GBytes* bytes)
{
	gchar* str = g_malloc0( 2*g_bytes_get_size(bytes) + 1 );
	const guint8* data = g_bytes_get_data(bytes, NULL);
	for(int i = 0 ; i < g_bytes_get_size(bytes); i++)
	{
		sprintf(&str[2*i], "%02X", data[i]);
	}
	return str;
}

//Esta funcion asigna el tag leido a lastIdTagRead
static void asignarTag(neardal_tag* pTag)
{
	if( pTag->iso14443aUid != NULL )
	{
		gchar *str = bytesToStr(pTag->iso14443aUid);
		lastIdTagRead=strdup(str);
		g_free(str);
	}
}

static void recordFound(const char *recordName, void *pUserData)
{
        errorCode_t     err;
        neardal_record* pRecord;

        params_t* pParams = (params_t*) pUserData;

        err = neardal_get_record_properties(recordName, &pRecord);
        if(err != NEARDAL_SUCCESS)
        {
                g_warning("Error %d when reading record %s (%s)\r\n", err, recordName, neardal_error_get_text(err));
                return;
        }

        //Dump record's content
        gboolean uriRecord = FALSE;
        gchar* stderrbuf = NULL;
        if( (pRecord->uri != NULL) && (strlen(pRecord->uri) > 0) )
        {
                gchar* args[] = {"xdg-open", pRecord->uri, NULL};
                g_printf("Opening %s\r\n", pRecord->uri);
                uriRecord = TRUE;
                gboolean result = g_spawn_sync(NULL, args, NULL,
                                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_SEARCH_PATH,
                                NULL, NULL, NULL, &stderrbuf, NULL, NULL);
                if(!result)
                {
                        addLog("xdg-open failed\r\n",NULL,NULL,LOG_ERROR,ANY);
                        pParams->returnCode = 1;
                }
                if(strlen(stderrbuf) > 0)
                {
                        addLog("Error: %s\r\n", stderrbuf,NULL,LOG_ERROR,ANY);
                }
                g_free(stderrbuf);
        }
        neardal_free_record(pRecord);

        if( (uriRecord == TRUE) && !pParams->keepPolling )
        {
                //Exit program
                g_main_loop_quit(pParams->pMainLoop);
        }
}

//Esta funcion es llamada cuando se detecta una tarjeta RFID
static void tagFound(const char *tagName, void *pUserData)
{
	errorCode_t	err;
	neardal_tag* pTag;
	params_t* pParams = (params_t*) pUserData;

	err = neardal_get_tag_properties(tagName, &pTag);
	if(err != NEARDAL_SUCCESS)
	{
		addLog("Error when reading tag %s (%s)\r\n", tagName, neardal_error_get_text(err), LOG_WARNING, ANY);
		return;
	}

	asignarTag(pTag);

	neardal_free_tag(pTag);
}

//Esta funcion es llamada por la libreria neardal cuando se detecta el dispositivo. Simplemente añade una linea al Log
static void dispositivoEncontrado(const char *name, void *pUserData)
{
        addLog("NFC Device found\r\n", NULL,NULL,LOG_INFO, ANY);
}

//Esta funcion es llamada cuando se pierde conexion (por proximidad) con la tarjeta NFC
//Tambien es llamado cuando se pierde conectividad con el dispositivo
static void tagLost(const char *name, void *pUserData)
{
        //Go through adapters and restart polling
        params_t* pParams = (params_t*) pUserData;
        pParams->returnCode = startPolling(pParams);
        if(pParams->returnCode != 0)
        {
                g_main_loop_quit(pParams->pMainLoop);
        }
}

//Esta funcion es el 'main' del modulo de manejo de la tarjeta lectora RFID. Espera 5 segundos a que todo este OK.
//la inicializa y asigna varias funciones de callback. Finalmente lanza el bucle inito que se queda esperando a que alguien pase una tarjeta RFID

void checkRFIDPassed()
{
	//Dejamos un tiempo prudencial para que todo el resto de tareas de inicializacion terminen
	usleep(5000000);

	//Ahora inicializamos el RFID
	params_t params;
	params.debug = FALSE;
	params.keepPolling = FALSE;
	params.adapterObjectPath = NULL;
	params.returnCode = 0;

	params.pRcd = NULL;
	params.writeMessage = NULL;
	params.messageType = "";
	params.language = "en";
	params.copy = FALSE;
	params.isCopied = FALSE;

	//Initialize GLib main loop
	params.pMainLoop = g_main_loop_new(NULL, FALSE);

	//Add tag found callback
	neardal_set_cb_tag_found(tagFound, (gpointer)&params);
	neardal_set_cb_dev_found(dispositivoEncontrado, (gpointer)&params);

	//Add record found callback
	neardal_set_cb_record_found(recordFound, (gpointer)&params);

	//Add tag/device lost callbacks
	neardal_set_cb_tag_lost(tagLost, (gpointer)&params);
	neardal_set_cb_dev_lost(tagLost, (gpointer)&params);

	params.returnCode = startPolling(&params);
	if(params.returnCode != 0)
	{
		addLog("Hubo un error al iniciar el polling", NULL,NULL,LOG_ERROR,ANY);
		return;
	}

	//Este mensaje es que el le indica al usuario que ya es posible pasar la tarjeta RFID
	sendToDisplay("ChargePoint Ready");

	//Aqui es donde se lanza el bucle infinito que lee del RFID
	g_main_loop_run(params.pMainLoop);

	//Esta parte no se alcanza nunca mas que si ha habido algun error con el RFID o se cierra el ChargePoint
	if( params.adapterObjectPath != NULL )
	{
		g_free(params.adapterObjectPath);
	}
	if(params.pRcd != NULL)
	{
		neardal_free_record(params.pRcd);
	}

	return;
}

