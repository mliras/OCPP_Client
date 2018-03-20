/*
 * localAuthorization.h
 *
 *  Created on: Nov 27, 2017
 *      Author: root
 */

#ifndef LOCALAUTHORIZATION_H_
#define LOCALAUTHORIZATION_H_

#include "ocpp_client.h"
#include "client.h"

////////////////////////
//AUTHORIZATION CACHE
////////////////////////
#define AUTHORIZATION_CACHE_SIZE 16

struct authorization_cache_record{
	char IdTag[20];
	int status;
	struct tm *expiryDate;
	char parentIdTag[20];
};

struct authorization_cache_record *authorization_Cache[AUTHORIZATION_CACHE_SIZE];

////////////////////////
//AUTHORIZATION LIST
////////////////////////
struct authorization_record{
	int status;
	struct tm *expiryDate;
	char parentIdTag[20];
};

struct authorization_list_entry{
	struct authorization_record *entry;
	char *idTag;
	int version;
	struct authorization_list_entry *next;
};

int localListVersion;
struct authorization_list_entry *authorizationList;

/////////////////////////////////////////////////////
//Devuelve un 0 o un 1 dependiendo de si la clave "AuthorizationCacheEnabled" esta activada o no
int isAuthorizationCacheEnabled();
//Devuelve un 0 o un 1 dependiendo de si la clave "LocalAuthListEnabled" esta activada o no
int isAuthorizationListEnabled();

//Checks if an IdTag is in the authorization list in state accepted or concurrent
//returns 1 if valid, 0 if not valid.
int isIdTagInAuthorizationList(const char *idTag);
//Checks if an IdTag is in the Cache in state accepted or concurrent
//returns 1 if valid, 0 if not valid.
int isIdTagInAuthorizationCache(const char *idTag);
//return status of a specific idTag in the authorization CACHE
//Or -1 if not found
int getAuthorizationCacheEntryStatus(const char *idTag);
//return status of a specific idTag in the authorization LIST
//Or -1 if not found
int getAuthorizationListEntryStatus(const char *idTag);

//Esta funcion comprueba si un IdTag que se le pasa como parámetro se encuentra
//en la cache de autorizacion con un status _CP_AUTHORIZATION_CONCURRENT_TX || _CP_AUTHORIZATION_ACCEPTED
//returns 1 if valid. returns 0 if not valid
int checkValidAuthorizationCacheEntry(const char *idTag);
//Esta funcion comprueba si un IdTag que se le pasa como parámetro se encuentra
//en la lista de autorizacion con un status _CP_AUTHORIZATION_CONCURRENT_TX || _CP_AUTHORIZATION_ACCEPTED
//returns 1 if valid. returns 0 if not valid
int checkValidAuthorizationListEntry(const char *idTag);

/////////////////////////////////////////////////////
//     AUTH LIST & CACHE ON DISK
/////////////////////////////////////////////////////
//Una de las funcionalidades requeridas es que la lista y cache de autorización se encuentren almacenadas en disco y se recuperen
//cuando se inicie el CP y se almacenen cada vez que haya un cambio en las mismas. Las siguientes funciones realizan dichas tareas:
//Lee la Authorization Cache desde el fichero que se le pasa como parámetro
void read_cache_from_disk(char *filename);
//Lee la autorization List del fichero que se le pasa como parámetro
void read_list_from_disk(char *filename);
//Escribe la autorization cache al fichero indicado en la clave "LocalAuthorizationCacheFile"
//En caso de no indicar nombre de fichero, se escribe al fichero (hardodeado) /tmp/local_auth_cache.dat.
void write_cache_to_disk();
//Escribe la autorization List al fichero indicado en la clave "LocalAuthListFile"
//En caso de no indicar nombre de fichero, se escribe al fichero (hardodeado) /tmp/local_auth_cache.dat.
void write_list_to_disk();

/////////////////////////////////////////////////////
//Pag 15:
//When the validity of a cache entry expires, it SHALL be changed to "expired" in the Cache. This is checked every heartbeat.
//Esto significa que esta validez debe ser comprobada cada cierto tiempo. El lugar desde donde se comprueba esta validez
//es desde el hilo que manda los heartbeats. Es decir, esta funcion es llamada cada 'heartbeat interval'
void checkAuthorizationCacheEntriesValidity();

/////////////////////////////////////////////////////
//Esta funcion vacia la caché de autorización, liberando los objetos
//Es llamada desde manageClearCacheRequest() cuando se recibe una peticion 5.4 CLEAR CACHE
int clearAuthorizationCache();

//Esta funciona vacía la lista de autorizacion liberando los objetos almacenados
//Es llamada desde manageSendLocalListRequest() cuando se recibe una peticion 5.15 SEND LOCAL de tipo FULL (hay que sustituir la lista)
void emptyAuthorizationList();

//Updates the authorization cache with the data passed as arguments. If the IdTag is there, it updates
//the entry, otherwise, it adds a new entry with the information provided
int update_authorization_cache(const char* idTag, const char *expiryDate, const char *parentIdTag, int status);

//Updates the authorization list with the data passed as arguments. If the IdTag is there, it updates
//the entry, otherwise, it adds a new entry with the information provided
//Devuelve 0 si lo puede actualizar
//Devuelve 1 si hay un version mismatch
int update_authorization_list(const char* idTag, const char *expiryDate, const char *parentIdTag, int status, int version, int reading);

//añade una entrada a la lista de autorizacion con la información indicada
//Llamado desde manageSendLocalListRequest() ante una peticion 5.15, para cuando es necesario añadir una entrada en la lista
//Devuelve 0 si lo puede actualizar
//Devuelve 1 si hay un version mismatch
int add_authorization_list_entry(const char* idTag, const char *expiryDate, const char *parentIdTag, int status, int version);

/////////////////////////////////////////////////////
//Esta funcion muestra por pantalla (con printf) la lista de autorizacion. Es una funcion de debug
void show_authorization_list();
//Esta funcion muestra por pantalla (con printf) la cache de autorizacion. Es una funcion de debug
void show_authorization_cache();

#endif /* LOCALAUTHORIZATION_H_ */
