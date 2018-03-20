/*
 * aux.h
 *
 *  Created on: Nov 9, 2017
 *      Author: root
 */

#ifndef AUX_H_
#define AUX_H_
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define NUM_CONNECTORS 1

#define MIN(a,b)  ((((a)-(b))&0x80000000) >> 31)? (a) : (b)

////////////////////////////////////////////////////////////////
//   DATA TYPES
////////////////////////////////////////////////////////////////

//Ver 7.15
typedef struct CiString20Type{
	char x[20];
} CiString20Type;

//Ver 7.16
typedef struct CiString25Type{
	char x[25];
} CiString25Type;

//Ver 7.17
typedef struct CiString50Type{
	char x[50];
} CiString50Type;

//Ver 7.18
typedef struct CiString255Type{
	char x[255];
} CiString255Type;

//Ver 7.19
typedef struct CiString500Type{
	char x[500];
} CiString500Type;

////////////////////////////////////////////////////////////////
//   LOGGING
////////////////////////////////////////////////////////////////

//Logging en pantalla
/*
#define VERDE "\033[0;32;32m"
#define CYAN "\033[0;36m"
#define AMARILLO "\033[1;33m"
#define AZUL "\033[0;32;34m"
#define CYAN_L "\033[1;36m"
#define MARRON "\033[0;33m"
*/
#define ROJO "\033[0;32;31m"
#define RESET "\033[0m"

char *logfilename;
FILE *logfile;
static int debug=0;

enum logtype
{
	FICHERO,
	SCREEN,
	GUI,
	DB, //sqlite
	ANY
};

enum log_levels{
	NO_LOG,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

////////////////////////////////////////////////////////////////
//   COLA DE MENSAJES
////////////////////////////////////////////////////////////////
static int mutex=0;

typedef struct {
		int MessageAction;
		int UniqueId;
		char *payload;
		struct queue_node *next;
}queue_node;

//Cola principal
queue_node *Message_queue;


/////////////////////////////////////////////////////////////////
//   FUNCIONES DE COLA DE MENSAJES
/////////////////////////////////////////////////////////////////
//
//Desencola el primer elemento de la cola
void Dequeue();

//Desencola el elemento con el uniqueId indicado. Si no lo encuentra... no lo desencola
void Dequeue_id(int uniqueId);

//Encola un nuevo mensaje al final de la cola
void Enqueue(queue_node *new_node);

//Busca un mensaje con el UniqueId que le indicamos y nos devuelve la cadena del
//mensaje (ojo, no solo la del payload si no la del mensaje completo)
char *getPayloadFromMessage(int uniqueId);

//Comprueba si existe actualmente un mensaje en la cola con el uniqueId que le indicamos.
queue_node * checkMessageinMessageQueue(int uniqueId);

//Comprueba si existe actualmente un mensaje en la cola con el action que le indicamos.
int isThereAMessageOfType(int Action);


/////////////////////////////////////////////////////////////////
//Busca un mensaje con el UniqueId que le indicamos y nos devuelve el action Code asociado.
int getActionFromUniqueId(const char* UniqueId_str);

void writeLogToFile(char *texto, enum log_levels warning_level);

/////////////////////////////////////////////////////////////////
//Funciones de cadena
/////////////////////////////////////////////////////////////////
void reverse(char *str, int len);
int intToStr(int x, char str[], int d);
void ftoa(float n, char *res, int afterpoint);
int charCounter(char* pString, char c);
char *convert(int i);
char *convertF(float myFloat);
char * replace(char const * const original,char const * const pattern,char const * const replacement);
char** str_split(char* a_str, const char a_delim);
char * getExpiryDateFromCurrentTime();
void toLowerCase(char *p);
char * strlwr(char * s);
char * strlwr_ex(char * s);
char *getRandomString(int length, int mins, int mays, int nums);
int checkIsNumber(char *tmp);
char *getSystemName();

char *encrypt(char *text, int last);
char *decrypt(char *text, int last);


#endif /* AUX_H_ */
