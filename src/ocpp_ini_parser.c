/*
 * ocpp_ini_parser.c
 *
 *  Created on: Nov 8, 2017
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "ocpp_ini_parser.h"
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#if !INI_USE_STACK
#include <stdlib.h>
#endif

#define MAX_SECTION 50
#define MAX_NAME 50

/////////////////////////////////////////////////////////////
// CHARGEPOINT STATUS FILE
/////////////////////////////////////////////////////////////
//
//Esta funcion permite almacenar en la ruta indicada (en modo binario) como parámetro el estado del Chargepoint en todo momento.
//La informacion que se almacena es: el numero de conectores, el valor de 'currentChargePointState' y 'currentChargePointErrorStatus'
//Asi como el estado de cada conector con el array 'connectorStatus' y por ultimo los valores de cada conector 'connectorValues'
int writeToDiskBin(char *filename)
{
	 FILE* file;
	 int error=0;

	 file = fopen(filename, "wb");
	 if (!file) return -1;

	 error = writeIntToFile(file, NUM_CONNECTORS);
	 if (!error) error = writeIntToFile(file, currentChargePointState);
	 if (!error) error = writeIntToFile(file, currentChargePointErrorStatus);

	 int i=0;
	 while (!error && i<NUM_CONNECTORS)
	 {
		 error = writeIntToFile(file, connectorStatus[i]);
		 i=i+1;
	 }

	 i=0;
	 while (!error && i<NUM_CONNECTORS)
	 {
		 error = writeDoubleToFile(file, connectorValues[i]);
		 i=i+1;
	 }

	 fclose(file);
	 return error;
}

//Identica funcion a la anterior pero almacena el fichero en modo texto. Solo se usa una de las dos
int writeToDisk(char *filename)
{
	 FILE* file;
	 int error=0;

	 file = fopen(filename, "w");
	 if (!file) return -1;

	 fprintf(file, "%d", NUM_CONNECTORS);
	 if (!ferror(file)) fprintf(file, "\n%d", currentChargePointState);
	 if (!ferror(file)) fprintf(file, "\n%d", currentChargePointErrorStatus);

	 int i=0;
	 while (!ferror(file) && i<NUM_CONNECTORS)
	 {
		 fprintf(file, "\n%d", connectorStatus[i]);
		 i=i+1;
	 }

	 i=0;
	 while (!ferror(file) && i<NUM_CONNECTORS)
	 {
	 	 fprintf(file, "\n%5.3f", connectorValues[i]);
	 	 i=i+1;
	 }

	 fclose(file);
	 return error;
}

//Funcion que lee del fichero que se le pasa como parámetro y en modo texto los siguientes valores:
//La informacion que se almacena es: el numero de conectores, el valor de 'currentChargePointState' y 'currentChargePointErrorStatus'
//Asi como el estado de cada conector con el array 'connectorStatus' y por ultimo los valores de cada conector 'connectorValues'
int readFromDisk(char *filename)
{
	 FILE* file;
	 int error=0;

	 file = fopen(filename, "r");
	 if (!file) return -1;

	 int tmp;
	 fscanf(file, "%d", &tmp);  //NUM_CONNECTORS

	 if (!ferror(file)) fscanf(file, "\n%d", &currentChargePointState);
	 if (!ferror(file)) fscanf(file, "\n%d", &currentChargePointErrorStatus);

	 int i=0;
	 while (!ferror(file) && i<tmp)
	 {
		 fscanf(file, "\n%d", &connectorStatus[i]);
		 i=i+1;
	 }

	 i=0;
	 while (!ferror(file) && i<tmp)
	 {
	 	 fscanf(file, "\n%lf", &connectorValues[i]);
	 	printf("LOAS VALORES SON %f", connectorValues[i]);
	 	 i=i+1;
	 }


	 fclose(file);
	 return error;
}

//Identica funcion a la anterior, pero que permite leer valores binarios.
int readFromDiskBin(char *filename)
{
	 FILE* file;
	 int error=0;

	 file = fopen(filename, "rb");
	 if (!file) return -1;

	 int tmp;
	 error = fread(&tmp, sizeof(tmp), 1, file);
	 if (!error) error = fread(&currentChargePointState, sizeof(currentChargePointState), 1, file);
	 if (!error) error = fread(&currentChargePointErrorStatus, sizeof(currentChargePointErrorStatus), 1, file);

	 int i=0;
	 while (!error && i<NUM_CONNECTORS)
	 {
		 error = fread(&connectorStatus[i], sizeof(connectorStatus[i]), 1, file);
		 i=i+1;
	 }

	 i=0;
	 while (!error && i<NUM_CONNECTORS)
	 {
		 error = fread(&connectorValues[i], sizeof(connectorValues[i]), 1, file);
		 i=i+1;
	 }

	 fclose(file);
	 return error;
}

//Funcion auxiliar usada por writeToDiskBin para almacenar un unico entero en disco
int writeIntToFile(FILE *file, int value)
{
	return fwrite(&value,sizeof(value),1,file);
}

//Funcion auxiliar usada por writeToDiskBin para almacenar un unico doble en disco
int writeDoubleToFile(FILE * file, double value)
{
	double *d=&value;
	return fwrite(d,sizeof(double),1,file);
}

//Funcion no utilizada
double readDoubleFromFile(FILE *file, double value)
{
	int parteentera=0;
	fread(&parteentera,sizeof(parteentera),1,file);
	int partedecimal=0;
	fread(&partedecimal,sizeof(parteentera),1,file);
	double d=parteentera+(partedecimal/100);

	return d;
}

//Funcion no utilizada
int readIntFromFile(FILE *file, int value)
{
	return fread(&value,sizeof(value),1,file);
}

//This function is called during initialization phase to check if there exist a file or not
//if it does not exist returns 0, otherwise returns the file descriptor value
int readCPStatusFromFile(char *filename)
{
	FILE *fp;
	if ((fp = fopen(filename, "rb"))==NULL)
	{
		return 0;
	}
	else
	{
		fclose(fp);
		return 1;
	}

	return 0;
}

////////////////////////////////////////////////////
// INI PARSER
////////////////////////////////////////////////////

//Struct usada por ini_parse_string() para mantener en todo momento el estado del parseo.
typedef struct {
    const char* ptr;
    size_t num_left;
} ini_parse_string_ctx;



// Funcion que elimina los espacios en blanco al final de una cadena dada.
// Añade un '\0 despues del ultimo caracter no blanco y devuelve la propia cadena
static char* rstrip(char* s)
{
    char* p = s + strlen(s);
    while (p > s && (unsigned char)(*--p)==' ')
        *p = '\0';
    return s;
}

//Funcion que devuelve un puntero al primer caracter no blanco de una cadena.
//Sirve para eliminar los caracteres en blanco al principio de una cadena.
static char* lskip(const char* s)
{
    while (*s && (unsigned char)(*s)==' ')
        s++;
    return (char*)s;
}

// Devuelve un puntero al primer caracter o comentario de una cadena dada. O bien
// O bien, devuelve NULL si no encuentra una cadena. Un comentario inline debe estar
//precedido por una espacio en blanco para ser detectado
static char* find_chars_or_comment(const char* s, const char* chars)
{
#if INI_ALLOW_INLINE_COMMENTS
    int was_space = 0;
    while (*s && (!chars || !strchr(chars, *s)) &&
           !(was_space && strchr(INI_INLINE_COMMENT_PREFIXES, *s))) {
        if ((unsigned char)(*s)==' ') was_space=1;
        s++;
    }
#else
    while (*s && (!chars || !strchr(chars, *s))) {
        s++;
    }
#endif
    return (char*)s;
}

// Version de strncpy que asegura que dest (size bytes) termina en NULL.
static char* strncpy0(char* dest, const char* src, size_t size)
{
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

//reader es realmente fgets, stream es realmente el fichero y handler es la funcion que será llamada para parsear
int ini_parse_stream(ini_reader reader, void* stream, ini_handler handler,void* user)
{
/*Por defecto usa el HEAP (char *) pero podemos definir que use la pila*/
#if INI_USE_STACK
    char line[INI_MAX_LINE];
#else
    char* line;
#endif
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";

    char* start;
    char* end;
    char* name;
    char* value;
    int lineno = 0;
    int error = 0;

#if !INI_USE_STACK
    line = (char*)calloc(1, 200);
    if (!line) {
        return -2;
    }
#endif

#if INI_HANDLER_LINENO
#define HANDLER(u, s, n, v) handler(u, s, n, v, lineno)
#else
#define HANDLER(u, s, n, v) handler(u, s, n, v)
#endif

    /* Scan through stream line by line */
    while (reader(line, 200, stream) != NULL) {
        lineno++;

        start = line;
#if INI_ALLOW_BOM
        if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
                           (unsigned char)start[1] == 0xBB &&
                           (unsigned char)start[2] == 0xBF) {
            start += 3;
        }
#endif
        start = lskip(rstrip(start));

        if (*start == ';' || *start == '#') {
        	// Como comentario permite tanto ; como  # al principio de la linea. Cuando sea un comentario, no hagas nada
        }
#if INI_ALLOW_MULTILINE
        else if (*prev_name && *start && start > line) {
            if (!HANDLER(user, section, prev_name, start) && !error)
            {
                error = lineno;
                printf("OJO, Hubo algun problema con el fichero de configuracion en la linea %d", lineno);
            }
        }
#endif
        else if (*start == '[') {
            // Se trata de una linea "[seccion]"
            end = find_chars_or_comment(start + 1, "]");
            if (*end == ']') {
                *end = '\0';
                strncpy0(section, start + 1, sizeof(section));
                *prev_name = '\0';
            }
            else if (!error) {
                /* No se ha encontrado el ']' en una linea de seccion*/
                error = lineno;
                printf("OJO, Hubo algun problema con el fichero de configuracion en la linea %d", lineno);
            }
        }
        else if (*start) {
            // No es un comentario. Debe ser un par nombre[=:]valor
            end = find_chars_or_comment(start, "=:");
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = end + 1;
#if INI_ALLOW_INLINE_COMMENTS
                end = find_chars_or_comment(value, NULL);
                if (*end)
                    *end = '\0';
#endif
                value = lskip(value);
                rstrip(value);

                // Se trata de un para nombre[=:]valor, llamar al handler
                strncpy0(prev_name, name, sizeof(prev_name));
                if (!HANDLER(user, section, name, value) && !error)
                {
                    error = lineno;
                    printf("OJO, Hubo algun problema con el fichero de configuracion en la linea %d", lineno);
                }
            }
            else if (!error) {
                // No se ha encontrado un '=' o un ':' en una linea nombre[=:]valor
                error = lineno;
                printf("OJO, no se encontró un signo '=' o ':' en la linea %d", lineno);
            }
        }

#if INI_STOP_ON_FIRST_ERROR
        if (error)
            break;
#endif
    }

#if !INI_USE_STACK
    free(line);
#endif

    return error;
}

//Esta funcion, es la utilizada a la hora de parsear un fichero .INI
//basicamente llama a la anterior
int ini_parse_file(FILE* file, ini_handler handler, void* user)
{
    return ini_parse_stream((ini_reader)fgets, file, handler, user);
}

//Funcion principal, que abre el fichero que se le indica y que llama a la funcion anterior, la cual llama a su vez a ini_parser_stream
int ini_parse(const char* filename, ini_handler handler, void* user)
{
    FILE* file;
    int error;

    file = fopen(filename, "r");
    if (!file)
        return -1;
    error = ini_parse_file(file, handler, user);
    fclose(file);
    return error;
}

// Esta funcion lee la siguiente linea del stream (fichero)
// Es la alternativa a fgets usada por ini_parse_string().
static char* ini_reader_string(char* str, int num, void* stream)
{
    ini_parse_string_ctx* ctx = (ini_parse_string_ctx*)stream;
    const char* ctx_ptr = ctx->ptr;
    size_t ctx_num_left = ctx->num_left;
    char* strp = str;
    char c;

    if (ctx_num_left == 0 || num < 2)
        return NULL;

    while (num > 1 && ctx_num_left != 0) {
        c = *ctx_ptr++;
        ctx_num_left--;
        *strp++ = c;
        if (c == '\n')
            break;
        num--;
    }

    *strp = '\0';
    ctx->ptr = ctx_ptr;
    ctx->num_left = ctx_num_left;
    return str;
}

int ini_parse_string(const char* string, ini_handler handler, void* user) {
    ini_parse_string_ctx ctx;

    ctx.ptr = string;
    ctx.num_left = strlen(string);
    return ini_parse_stream((ini_reader)ini_reader_string, &ctx, handler,user);
}
