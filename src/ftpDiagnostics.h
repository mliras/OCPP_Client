/*
 * ftpDiagnostics.h
 *
 *  Created on: Dec 18, 2017
 *      Author: root
 */

#ifndef FTPDIAGNOSTICS_H_
#define FTPDIAGNOSTICS_H_

#define MAX_BUFSIZE 4096

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>

#include "aux.h"
#include "ocpp_client.h"
#include "client.h"

//Esta estructura nos permite realizar diferentes intenetos de lecturas y escrituras FTP
struct ftp_parameters{
		char *originalfilename;
		char *destinationfilename;
		char *username;
		char *password;
		char *location;
		int retries;
		int retryInterval;
		char *startTime;
		char *stopTime;
};

int busyUploadingDiagnosticsFile;

//HILO QUE SE QUEDA ESPERANDO HASTA LA HORA DEL ENVIO DEL FICHERO Y LUEGO LO ENVIA
//Utiliza la funcion de envio FTP ftpSendProcess
//Devuelve -1, si no puede mandarlo. //Devuelve 0 si se pudo mandar
int sendDiagnosticsFile(void *parameters);

//HILO QUE SE QUEDA ESPERANDO HASTA LA HORA DE LA DESCARGA DEL FICHERO Y LUEGO LO DESCARGA
//Utiliza la funcion de lectura FTP ftpGetProcess
int getFirmwareFile(void *parameters);

//Funcion que abre un socket contra la IP y el puerto solcitado
//Devuelve el socket FD o 1 en caso de error
int startConnection(char *ip, int port);

////////////////////////////////////////////////////////////////////
//     ENVIO Y RECEPCION DE FICHEROS FTP
////////////////////////////////////////////////////////////////////
//
////wait for ftp server to start talking
int ftpRecvResponse(int sock, char * buf);

//Envia un mensaje FTP a un socket
int ftpNewCmd(int sock, char * buf, char * cmd, char * param);

//Convierte lo recibido del servidor en una IP y un puerto, para saber donde nos indica
//que enviemos realmente el fichero. En hostname y port se escribiran estos datos
//...a partir de lo que tenemos en buf
int ftpConvertAddy(char * buf, char * hostname, int * port);

//Envia el fichero que le indcamos al host y al puerto que le pasamos como parámetro
int ftpSendFile(char * filename, char * host, int port);

//Funcion que inicia una conexion FTP contra el HOSTNAME y puerto indicado con el usuario y contraseña
//indicados y envia el fichero que se le pasa como parámetro.
//Para ello utiliza otras tres funciones anteriormente descritas: ftpNewCmd, ftpConvertAddy y ftpSendFile
int ftpSendProcess(char *hostname, int port, char *username, char *password, char *file, char *filename);

//Funcion que se descarga un fichero del hostname y puerto indicados, con el usuario y password indicados
//Hace uso de las funciones anteriores
int ftpGetProcess(char *hostname, int port, char *username, char *password, char *file, char *filename);


#endif /* FTPDIAGNOSTICS_H_ */
