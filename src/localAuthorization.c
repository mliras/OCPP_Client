/*
 * localAuthorization.c
 *
 *  Created on: Nov 27, 2017
 *      Author: root
 */
#include <stdio.h>
#include <string.h>
#include "localAuthorization.h"
#include "ChargePoint.h"

//Escribe la autorization List al fichero indicado en la clave "LocalAuthListFile"
//En caso de no indicar nombre de fichero, se escribe al fichero (hardodeado) /tmp/local_auth_cache.dat.
void write_list_to_disk()
{
	struct authorization_list_entry *l=calloc(1, sizeof(struct authorization_list_entry));

	char *filename=NULL;
	FILE * file=NULL;
	filename=getConfigurationKeyStringValue("LocalAuthListFile");

	if (filename)
	{
		file= fopen(filename, "wb");

		if (file) addLog("WRITING AUTHORIZATION LIST TO DISK FILE: %s.", filename,NULL,LOG_INFO,ANY);
		else addLog("COULD NOT OPEN AUTHLIST FILE: %s.", filename,NULL,LOG_INFO,ANY);

	}
	else
	{
		file= fopen("/tmp/local_auth_list", "wb");

		if (file) addLog("WRITING AUTHORIZATION LIST TO DISK FILE: %s.", "/tmp/local_auth_list",NULL,LOG_INFO,ANY);
		else addLog("COULD NOT OPEN AUTHLIST FILE: %s.", "/tmp/local_auth_list",NULL,LOG_INFO,ANY);
	}

	if (file != NULL) {
		while (l != NULL)
		{
		   fwrite(&(l->entry), 1, sizeof(l->entry), file);
		   l = l->next;
		}

		fclose(file);
	}
}

//Lee la autorization List del fichero que se le pasa como parámetro
void read_list_from_disk(char *filename)
{
	if (!filename) return;

	struct authorization_record *registro=calloc(1, sizeof(struct authorization_record));
	struct authorization_list_entry *list, *r,*origen;

	//Creamos la primera entrada de la lista
	list=(struct authorization_list_entry *)calloc(1, sizeof(struct authorization_list_entry));
	list->entry=NULL;
	list->next=NULL;

	//origen sera un puntero a la primera entrada de la lista
	origen=list;
	//r sera un puntero a la ultima entrada de la lista.
	r=list;

	FILE * file= fopen(filename, "rb");
	if (file != NULL)
    {
		 while (fread(registro, sizeof(struct authorization_record), 1, file) == 1)
		 {
			if (list->entry)
			{
				//Si la lista esta vacia
				list->next=NULL;
				list->entry=registro;
			}
			else
			{
				struct authorization_list_entry *object2=calloc(1, sizeof(struct authorization_list_entry));
				object2->next=NULL;
				object2->entry=registro;
				r->next=object2;
				r=r->next;
			}

			registro=calloc(1, sizeof(struct authorization_record));
		 }

		 //Liberamos el ultimo registro que allocamos y que nunca fue leido de disco:
		 free(registro);

/*
		 if (feof(file))
		 {
					printf("Fin de fichero encontrado");
		 }
*/
		 fclose(file);
	}

	 //Si la lista esta vacia, tambien liberamos la entrada vacia que creamos arriba del todo
	 if (!list->entry)
	 {
		 free(list);
		 list=NULL;
	 }
}

//Lee la Authorization Cache desde el fichero que se le pasa como parámetro
void read_cache_from_disk(char *filename)
{
	 if (!filename) return;

	 struct authorization_record *object2=calloc(1, sizeof(struct authorization_record));
	 FILE * file= fopen(filename, "rb");
	 if (file != NULL)
	 {
	 	int i=0;
	    while (fread(object2, sizeof(struct authorization_record), 1, file) == 1)
	    {
	    		authorization_Cache[i]=object2;
	    }
/*
	    if (feof(file))
	    {
	    		printf("Fin de fichero encontrado");
	    }
*/
	    fclose(file);
	 }
}

//Escribe la autorization cache al fichero indicado en la clave "LocalAuthorizationCacheFile"
//En caso de no indicar nombre de fichero, se escribe al fichero (hardodeado) /tmp/local_auth_cache.dat.
void write_cache_to_disk()
{
	struct authorization_cache_record *object;

	char *filename=NULL;

	//Devuelve NULL en caso de que la clave de configuración no exista
	filename=getConfigurationKeyStringValue("LocalAuthorizationCacheFile");
	//if (debug) show_authorization_cache();

	FILE * file=NULL;
	if (filename)
	{
		//Abrimos Fichero
		file= fopen(filename, "wb");

		//Logamos
		if (file) addLog("WRITING AUTHORIZATION CACHE TO DISK FILE: %s.", filename,NULL,LOG_INFO,ANY);
		else addLog("COULD NOT OPEN CACHE FILE: %s.", filename,NULL,LOG_INFO,ANY);
	}
	else
	{
		//Usamos el nombre por defecto
		file= fopen("/tmp/local_auth_cache.dat", "wb");

		//Logamos
		if (file) addLog("WRITING AUTHORIZATION CACHE TO DISK FILE: %s.", "/tmp/local_auth_cache.dat",NULL,LOG_INFO,ANY);
		else addLog("COULD NOT OPEN CACHE FILE: %s.", "/tmp/local_auth_cache.dat",NULL,LOG_INFO,ANY);
	}

	if (file)
	{
		for (int i=0; i<AUTHORIZATION_CACHE_SIZE; i++)
		{
				object=authorization_Cache[i];

				//Me da igual que luego cuando lea, el orden de array authorization_cache no sea el mismo...
				//if (object) fwrite(object, sizeof(*object)+1, 1, file);
				if (object) fwrite(object, sizeof(struct authorization_cache_record)+1, 1, file);
		}

		fclose(file);
	}
}


//AUTHORIZATION CACHE
int isAuthorizationCacheEnabled(){

	int localAuthCacheEnabled=getConfigurationKeyIntValue("AuthorizationCacheEnabled");

	if (localAuthCacheEnabled && localAuthCacheEnabled==1)	return 1;

	return 0;
}

//Esta funcion comprueba si un IdTag que se le pasa como parámetro se encuentra
//en la cache de autorizacion con un status _CP_AUTHORIZATION_CONCURRENT_TX || _CP_AUTHORIZATION_ACCEPTED
//returns 1 if valid. returns 0 if not valid
int checkValidAuthorizationCacheEntry(const char *idTag)
{
	for (int i=0; i<AUTHORIZATION_CACHE_SIZE; i++)
	{
		if (authorization_Cache[i])
		{
			if (strcmp(authorization_Cache[i]->IdTag, idTag)==0)
			{
				if (authorization_Cache[i]->status==_CP_AUTHORIZATION_CONCURRENT_TX || authorization_Cache[i]->status==_CP_AUTHORIZATION_ACCEPTED)
				{
						return 1;
				}
			}
		}
	}

	return 0;
}

//return status of a specific idTag in the authorization CACHE
//Or -1 if not found
int getAuthorizationCacheEntryStatus(const char *idTag)
{
	for (int i; i<AUTHORIZATION_CACHE_SIZE; i++)
	{
		if (authorization_Cache[i])
		{
			if (strcmp(authorization_Cache[i]->IdTag, idTag)==0)
			{
				return authorization_Cache[i]->status;
			}
		}
	}

	return -1;
}

//Pag 15:
//When the validity of a cache entry expires, it SHALL be changed to "expired" in the Cache
//Esto significa que esta validez debe ser comprobada cada cierto tiempo. El lugar desde donde se comprueba esta validez
//es desde el hilo que manda los heartbeats. Es decir, esta funcion es llamada cada 'heartbeat interval'
void checkAuthorizationCacheEntriesValidity()
{
	int changed=0;

	for (int i=0; i<AUTHORIZATION_CACHE_SIZE; i++)
	{
			if (authorization_Cache[i])
			{
				struct tm * _fechaExpiracion=authorization_Cache[i]->expiryDate;
				__time_t fecha = mktime(_fechaExpiracion);
				__time_t now;
				time (&now);
				struct tm * timeinfo= localtime (&now);

				double diffSecs = difftime(fecha, now);

				if (diffSecs<0.0)
				{
					authorization_Cache[i]->status=_CP_AUTHORIZATION_EXPIRED;

					for (int k; k<NUM_CONNECTORS; k++)
					{
						if (strcmp(connectorAuthorizations[k].idTag, authorization_Cache[i]->parentIdTag)==0)
						{
							//Si el idTag de la authorization cache que expira esta actualmente utilizando un connector...

							//send_stoptransaction_request(-1, getTransactionId(k), ¿¿¿???, connectorAuthorizations[k].idTag, NULL);  <-- IMPLEMENTAR
							connectorAuthorizations[k].idTag=NULL;
						}
					}
				}
			}
	}
	if (changed) write_cache_to_disk();
}

////////////////////////////////////
//AUTHORIZATION LIST
//
int isAuthorizationListEnabled(){

	int localAuthListEnabled=getConfigurationKeyIntValue("LocalAuthListEnabled");

	if (localAuthListEnabled && localAuthListEnabled==1) return 1;

	return 0;
}

//returns 1 if it exist
int isIdTagInAuthorizationList(const char *idTag)
{
	struct authorization_list_entry *l;
	for (l=authorizationList;l && strcmp(l->idTag, idTag);  l=l->next);

	if (l)
	{
			return 1;
	}

	return 0;
}

//Checks if an IdTag is in the Cache in state accepted or concurrent
//returns 1 if valid, 0 if not valid.
int isIdTagInAuthorizationCache(const char *idTag)
{
	struct authorization_cache_record *l;
	for (int i=0; i<AUTHORIZATION_CACHE_SIZE; i++)
	{
		if (authorization_Cache[i])
		{
			if (strcmp(authorization_Cache[i]->IdTag, idTag)==0) return 1;
		}
	}

	return 0;
}

//Esta funcion comprueba si un IdTag que se le pasa como parámetro se encuentra
//en la lista de autorizacion con un status _CP_AUTHORIZATION_CONCURRENT_TX || _CP_AUTHORIZATION_ACCEPTED
//returns 1 if valid. returns 0 if not valid
int checkValidAuthorizationListEntry(const char *idTag)
{
	if (!idTag) return 0;
	struct authorization_list_entry *l;
	for (l=authorizationList;l && strcmp(l->idTag, idTag); l=l->next);

	if (l)
	{
			//Comprobamos si es válido
			if (l->entry->status==_CP_AUTHORIZATION_CONCURRENT_TX || l->entry->status==_CP_AUTHORIZATION_ACCEPTED)
			{
				return 1;
			}
	}
	return 0;
}

//return status of a specific idTag in the authorization list
//Or -1 if not found
int getAuthorizationListEntryStatus(const char *idTag)
{
	if (!idTag) return 0;

	struct authorization_list_entry *l;
	for (l=authorizationList;l && strcmp(l->idTag, idTag); l=l->next);

	if (l)
	{
			//Comprobamos si es válido
			return l->entry->status;
	}
	return -1;
}

//Esta funcion vacia la caché de autorización, liberando los objetos
//Es llamada desde manageClearCacheRequest() cuando se recibe una peticion 5.4 CLEAR CACHE
int clearAuthorizationCache()
{
	struct authorization_cache_record *object;

	for (int i=0; i<AUTHORIZATION_CACHE_SIZE; i++)
	{
			if (authorization_Cache[i])
			{
				object=authorization_Cache[i];
				authorization_Cache[i]=NULL;
				free(object);
			}
	}

	return _CCS_ACCEPTED;
}

////////////////////////////////////
//AUTHORIZATION CACHE
//
//
//3.4.1. - CACHE
//
void show_authorization_cache()
{
	printf("\n===================");
	printf("\nAuthorization Cache:\n");

	for (int i=0; i<AUTHORIZATION_CACHE_SIZE; i++)
	{
		printf("Cache entry %d:", i);
		if (!authorization_Cache[i]) printf("NULL\n");
		else
		{
			char *expiry=(char *)calloc(1, sizeof(char)*80);
			if (!authorization_Cache[i]->expiryDate) strcpy(expiry, "NULL");
			else strftime (expiry, 80, "%Y-%m-%dT%H:%M:%S.", authorization_Cache[i]->expiryDate);

			printf("\n	IdTag: %s\n", authorization_Cache[i]->IdTag);
			printf("	status: %d\n", authorization_Cache[i]->status);
			printf("	expiryDate: %s\n", expiry);
			printf("	parentIdTag: %s\n", authorization_Cache[i]->parentIdTag);

			free(expiry);
		}
	}
}

//Updates the Cache . If the IdTag is there, it updates the entry, otherwise, add a new entry with the information provided
int update_authorization_cache(const char* idTag, const char *expiryDate, const char *parentIdTag, int status)
{
	int k;
	//Buscamos la entrada en la authorization cache:
	for (k=0; k<AUTHORIZATION_CACHE_SIZE; k++)
	{
		if (authorization_Cache[k])
		{
			if (strcmp(authorization_Cache[k]->parentIdTag, parentIdTag)==0)
			{
				break;
			}
		}
	}

	//Si la encontramos:
	if (k<AUTHORIZATION_CACHE_SIZE)
	{
		//Actualizamos una ya existente
		authorization_Cache[k]->status=status;
		strptime(expiryDate, "%Y-%m-%dT%H:%M:%S.", authorization_Cache[k]->expiryDate);

		if (isSQLiteConnected())
		{
			updateSQLiteAuthorizationCacheEntry(idTag,status,expiryDate,parentIdTag);
		}
		else
		{
			write_cache_to_disk();
		}
		return 0;
	}

	//Si no existe la entrada del idtag, la añadimos
	int i;

	//buscamos una posicion libre:
	for (i=0; authorization_Cache[i] && i<AUTHORIZATION_CACHE_SIZE; i++);

	//Si esta lleno...
	if (i==AUTHORIZATION_CACHE_SIZE)
	{
		//Es necesario borrar algo
		int j;
		for (j=0; authorization_Cache[j]->status==_CP_AUTHORIZATION_ACCEPTED && j<AUTHORIZATION_CACHE_SIZE; j++);

		//Esta lleno!!
		if (j==AUTHORIZATION_CACHE_SIZE) return -1;
		else
		{
			strncpy(authorization_Cache[j]->IdTag, idTag, 20);
			if (parentIdTag) strncpy(authorization_Cache[j]->parentIdTag, parentIdTag, 20);
			authorization_Cache[j]->status=status; //Posibles valores de enum AuthorizationStatus{

			strptime(expiryDate, "%Y-%m-%dT%H:%M:%S.", authorization_Cache[j]->expiryDate);

			if (isSQLiteConnected())
			{
					writeAuthCacheToSQLite(AUTHORIZATION_CACHE_SIZE);
			}
			else
			{
					write_cache_to_disk();
			}

		//	if (debug) show_authorization_cache();

			return 0;
		}
	}
	else
	{
		//Hemos encontrado un sitio. Creamos el struct en memoria y Lo añadimos


		struct authorization_cache_record *authorization;
		struct tm* time=(struct tm *)calloc(1, sizeof(struct tm));
		authorization=(struct authorization_cache_record*)calloc(1, sizeof(struct authorization_cache_record));

		strncpy(authorization->IdTag, idTag, 19);
		if (parentIdTag)  strncpy(authorization->parentIdTag, parentIdTag, 19);
		authorization->status=status; //Posibles valores de enum AuthorizationStatus{

		strptime(expiryDate, "%Y-%m-%dT%H:%M:%S.", time);
		authorization->expiryDate=time;

		authorization_Cache[i]=authorization;


		if (isSQLiteConnected())
		{
				writeAuthCacheToSQLite(AUTHORIZATION_CACHE_SIZE);
		}
		else
		{
				write_cache_to_disk();
		}

		return 0;
	}
	return -1;
}

//////////////////////////////////////////////
//
//3.4.2 - LIST
//


void show_authorization_list()
{
	struct authorization_list_entry *l, *r;
	addLog("\n===================",NULL,NULL,LOG_DEBUG,ANY);
	addLog("\nAuthorization List: ",NULL,NULL,LOG_DEBUG,ANY);

	if (!authorizationList) addLog("\n*** EMPTY LIST *** ",NULL,NULL,LOG_DEBUG,ANY);
	else
	{
		addLog("\n ",NULL,NULL,LOG_DEBUG,ANY);
		for (l=authorizationList;l; l=l->next)
		{
			addLog("idTag: %s\n",l->idTag,NULL,LOG_DEBUG,ANY);
			addLog("Version: %s\n",convert(l->version),NULL,LOG_DEBUG,ANY);

			if (l->entry)
			{
				char expiry[80];
				if (l->entry->expiryDate) strftime (expiry, 80, "%Y-%m-%dT%H:%M:%S.", l->entry->expiryDate);
				else strncpy(expiry, "NULL", 20);

				addLog("	status: %s\n", convert(l->entry->status), NULL,LOG_DEBUG,ANY);
				addLog("	expiryDate: %s\n", expiry, NULL,LOG_DEBUG,ANY);
				addLog("	parentIdTag: %s\n", l->entry->parentIdTag, NULL,LOG_DEBUG,ANY);
			}
			else
			{
				addLog("Entry: NULL\n", NULL, NULL,LOG_DEBUG,ANY);
			}
		}

		addLog("\nFIN DE LISTA\n============", NULL, NULL,LOG_DEBUG,ANY);
	}
}


//Devuelve 0 si lo puede actualizar
//Devuelve 1 si hay un version mismatch
int update_authorization_list(const char* idTag, const char *expiryDate, const char *parentIdTag, int status, int version, int reading)
{
	struct authorization_list_entry *l, *r;

	//Recorremos la lista hasta llegar a la ultima entrada. r se queda apuntando al ultimo elemento de la lista. l ya es NULL.
	//Tambien puede salir porque encuentre un nodo de la lista en la que este el idtag
	for (l=authorizationList;l && strcmp(l->idTag, idTag); r=l, l=l->next);

	if (l)
	{

		//Ha encontrado el idtag ya en la lista de autorizaciones...

		//Si se trata de la misma (o menor) version, salimos con error 3
		//Una version=-1, es un valor inválido.
		if ((version>=0) && (l->version>=version)) return 3;

		//Si no, actualizamos la entrada ya existente:
		if (version>0) l->version=version;

		//status
		l->entry->status=status;

		//expiryDate
		if (expiryDate){
			l->entry->expiryDate=(struct tm *)calloc(1, sizeof(struct tm));
			strptime(expiryDate, "%Y-%m-%dT%H:%M:%S.", l->entry->expiryDate);
		}
		else l->entry->expiryDate=NULL;
		//parentIdTag
		if (parentIdTag) strncpy(l->entry->parentIdTag, parentIdTag, sizeof(l->entry->parentIdTag)-1);
		else for (int i=0; i<20; i++) l->entry->parentIdTag[i]='\0';

		if (!reading)
		{
			//Escribimos a disco o BD los cambios
			if (isSQLiteConnected())
			{
				writeAuthListToSQLite();
			}
			else
			{
				write_list_to_disk();
			}
		}

		return 0;
	}

	//No existe aun el idtag en la lista. Lo añadimos

	struct authorization_record *object=calloc(1, sizeof(struct authorization_record));
	struct authorization_list_entry *object2=calloc(1, sizeof(struct authorization_list_entry));

	//Creamos los objetos en memoria
	object->status=status;
	if (expiryDate){
		object->expiryDate=(struct tm *)calloc(1, sizeof(struct tm));
		strptime(expiryDate, "%Y-%m-%dT%H:%M:%S.", object->expiryDate);
	}

	if (parentIdTag) strncpy(object->parentIdTag, parentIdTag,20);

	object2->entry=object;
	object2->idTag=strdup(idTag);
	object2->next=NULL;
	if (version>0) object2->version=version;
	else object2->version=0;


	//Lo añadimos a la lista de authorization
	if (!authorizationList)
	{
		if (debug) addLog("Creando la lista de autorizacion.", NULL,NULL, LOG_INFO, ANY);
		authorizationList=object2;
	}
	else
	{
		r->next=object2;
	}

	if (!reading)
	{
		//Escribimos a disco o BD los cambios
		if (isSQLiteConnected())
		{
			writeAuthListToSQLite();
		}
		else
		{
			write_list_to_disk();
		}
	}

	return 0;
}

//añade una entrada a la lista de autorizacion con la información indicada
//Llamado desde manageSendLocalListRequest() ante una peticion 5.15, para cuando es necesario añadir una entrada en la lista
//Devuelve 0 si lo puede actualizar
//Devuelve 1 si hay un version mismatch
int add_authorization_list_entry(const char* idTag, const char *expiryDate, const char *parentIdTag, int status, int version)
{
	struct authorization_list_entry *r, *l=authorizationList;

	//Reservamos memoria para la authorization list entry y el authorization record
	struct authorization_list_entry *aux=(struct authorization_list_entry *)calloc(1, sizeof(struct authorization_list_entry));
	struct authorization_record *p=(struct authorization_record *)calloc(1, sizeof(struct authorization_record));

	//Populamos el authorization record
	p->expiryDate=NULL;
	if (parentIdTag) strcpy(p->parentIdTag, parentIdTag);
	p->status=status;

	if (expiryDate)
	{
		struct tm* expiryDateTM=(struct tm *)calloc(1, sizeof(struct tm));
		strptime(expiryDate, "%Y-%m-%dT%H:%M:%S.", expiryDateTM);
		p->expiryDate=expiryDateTM;
	}

	//Populamos aux
	aux->idTag=strdup(idTag);
	aux->version=version;
	aux->entry=p;
	aux->next=NULL;

	//Nos movemos hasta el final de la lista. En r quedara el ultimo elemento de la lista o NULL si la lista esta vacia
	r=NULL;
	while (l)
	{
		r=l;
		l=l->next;
	}

	if (r) r->next=aux;
	else authorizationList=aux;

	return 0;
}

//Esta funciona vacía la lista de autorizacion liberando los objetos almacenados
//Es llamada desde manageSendLocalListRequest() cuando se recibe una peticion 5.15 SEND LOCAL de tipo FULL (hay que sustituir la lista)
void emptyAuthorizationList()
{
	struct authorization_list_entry *lista=authorizationList;
	struct authorization_list_entry *aux;

	while (lista)
	{
		if (lista->entry)
		{
			if (lista->entry->expiryDate) free(lista->entry->expiryDate);
			free(lista->entry);
		}

		aux=lista;
		lista=lista->next;
		free(aux);
	}

	authorizationList=NULL;
}
