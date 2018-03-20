/*
 * exploreNFC.h
 *
 *  Created on: 1 Mar 2018
 *      Author: pi
 */

#ifndef SRC_EXPLORENFC_H_
#define SRC_EXPLORENFC_H_

#include "neardal/neardal.h"

#include "HardwareInterface.h"

struct params
{
	gboolean debug;
	gboolean keepPolling;
	gchar* adapterObjectPath;
	GMainLoop* pMainLoop;
	gint returnCode;
	neardal_record* pRcd;
	gchar* writeMessage;
	gchar* messageType;
	gchar* language;
	gboolean copy;
	gboolean isCopied;
};
typedef struct params params_t;

//Esta funcion es llamada por la libreria neardal cuando se detecta el dispositivo. Simplemente añade una linea al Log
static void dispositivoEncontrado(const char *devName, void *pUserData);

//Esta funcion prepara todo para que, despues de cada lectura de una tarjeta siga leyendo nuevamente pasados unos milisegundos
static gint startPolling(params_t* pParams);

//Esta funcion convierte una ristra de bytes en un char *
static gchar* bytes_to_str(GBytes* bytes);

//Esat funcion asigna el Tag a la variable lastIdTagRead que luego será leido por el bucle princpal de la raspberry checkLoop()
static void asignarTag(neardal_tag* pTag);

static void recordFound(const char *recordName, void *pUserData);

//Esta funcion es llamada cuando se detecta una tarjeta RFID
static void tagFound(const char *tagName, void *pUserData);

//Esta funcion es llamada cuando se pierde conexion (por proximidad) con la tarjeta NFC
//Tambien es llamado cuando se pierde conectividad con el dispositivo
static void tagLost(const char *name, void *pUserData);

//Esta funcion es el 'main' del modulo de manejo de la tarjeta lectora RFID. Espera 5 segundos a que todo este OK.
//la inicializa y asigna varias funciones de callback. Finalmente lanza el bucle inito que se queda esperando a que alguien pase una tarjeta RFID
void checkRFIDPassed();

#endif /* SRC_EXPLORENFC_H_ */
