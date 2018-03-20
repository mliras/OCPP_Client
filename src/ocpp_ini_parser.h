/*
 * ocpp_ini_parser.h
 *
 *  Created on: Nov 8, 2017
 *      Author: root
 */

#ifndef OCPP_INI_PARSER_H_
#define OCPP_INI_PARSER_H_

/* Make this header file easier to include in C++ code */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "ocpp_client.h"
#include "aux.h"
#include "client.h"

/* Nonzero if ini_handler callback should accept lineno parameter. */
#ifndef INI_HANDLER_LINENO
#define INI_HANDLER_LINENO 0
#endif

/* Typedef for prototype of handler function. */
#if INI_HANDLER_LINENO
typedef int (*ini_handler)(void* user, const char* section,
                           const char* name, const char* value,
                           int lineno);
#else
typedef int (*ini_handler)(void* user, const char* section,
                           const char* name, const char* value);
#endif

/* Typedef for prototype of fgets-style reader function. */
typedef char* (*ini_reader)(char* str, int num, void* stream);

//Esta funcion permite almacenar en la ruta indicada (en modo binario) como parámetro el estado del Chargepoint en todo momento.
//La informacion que se almacena es: el numero de conectores, el valor de 'currentChargePointState' y 'currentChargePointErrorStatus'
//Asi como el estado de cada conector con el array 'connectorStatus' y por ultimo los valores de cada conector 'connectorValues'
int writeToDiskBin(char *filename);

//Identica funcion a la anterior pero almacena el fichero en modo texto. Solo se usa una de las dos
int writeToDisk(char *filename);

//Funcion que lee del fichero que se le pasa como parámetro y en modo texto los siguientes valores:
//La informacion que se almacena es: el numero de conectores, el valor de 'currentChargePointState' y 'currentChargePointErrorStatus'
//Asi como el estado de cada conector con el array 'connectorStatus' y por ultimo los valores de cada conector 'connectorValues'
int readFromDisk(char *filename);

//Identica funcion a la anterior, pero que permite leer valores binarios.
int readFromDiskBin(char *filename);

//Funcion auxiliar usada por writeToDiskBin para almacenar un unico entero en disco
int writeIntToFile(FILE *file, int value);

//Funcion auxiliar usada por writeToDiskBin para almacenar un unico doble en disco
int writeDoubleToFile(FILE *file, double value);


//int handler(void* user, const char* section, const char* name,const char* value);


/* Parse given INI-style file. May have [section]s, name=value pairs
   (whitespace stripped), and comments starting with ';' (semicolon). Section
   is "" if name=value pair parsed before any section heading. name:value
   pairs are also supported as a concession to Python's configparser.
   For each name=value pair parsed, call handler function with given user
   pointer as well as section, name, and value (data only valid for duration
   of handler call). Handler should return nonzero on success, zero on error.
   Returns 0 on success, line number of first error on parse error (doesn't
   stop on first error), -1 on file open error, or -2 on memory allocation
   error (only when INI_USE_STACK is zero).
*/
int ini_parse(const char* filename, ini_handler handler, void* user);

/* Same as ini_parse(), but takes a FILE* instead of filename. This doesn't
   close the file when it's finished -- the caller must do that. */
int ini_parse_file(FILE* file, ini_handler handler, void* user);

/* Same as ini_parse(), but takes an ini_reader function pointer instead of
   filename. Used for implementing custom or string-based I/O (see also
   ini_parse_string). */
int ini_parse_stream(ini_reader reader, void* stream, ini_handler handler,void* user);

/* Same as ini_parse(), but takes a zero-terminated string with the INI data
instead of a file. Useful for parsing INI data from a network socket or
already in memory. */
int ini_parse_string(const char* string, ini_handler handler, void* user);


int handler_ini(void* user, const char* section, const char* name,const char* value);

/* Nonzero to allow multi-line value parsing, in the style of Python's
   configparser. If allowed, ini_parse() will call the handler with the same
   name for each subsequent line parsed. */
#ifndef INI_ALLOW_MULTILINE
#define INI_ALLOW_MULTILINE 1
#endif

/* Nonzero to allow a UTF-8 BOM sequence (0xEF 0xBB 0xBF) at the start of
   the file. See http://code.google.com/p/inih/issues/detail?id=21 */
#ifndef INI_ALLOW_BOM
#define INI_ALLOW_BOM 1
#endif

/* Nonzero to allow inline comments (with valid inline comment characters
   specified by INI_INLINE_COMMENT_PREFIXES). Set to 0 to turn off and match
   Python 3.2+ configparser behaviour. */
#ifndef INI_ALLOW_INLINE_COMMENTS
#define INI_ALLOW_INLINE_COMMENTS 1
#endif
#ifndef INI_INLINE_COMMENT_PREFIXES
#define INI_INLINE_COMMENT_PREFIXES ";"
#endif

/* Nonzero to use stack, zero to use heap (malloc/free). */
#ifndef INI_USE_STACK
#define INI_USE_STACK 1
#endif

/* Stop parsing on first error (default is to keep parsing). */
#ifndef INI_STOP_ON_FIRST_ERROR
#define INI_STOP_ON_FIRST_ERROR 0
#endif

/* Maximum line length for any line in INI file. */
#ifndef INI_MAX_LINE
#define INI_MAX_LINE 200
#endif

#ifdef __cplusplus
}
#endif

#endif /* __OCPP_INI_PARSER_H___ */
