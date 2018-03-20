/*
 * SQLiteConnection.c
 *
 *  Created on: 23 Feb 2018
 *      Author: pi
 */
#include "SQLiteConnection.h"

#define BD "/etc/ocpp/chargepoint.db"
sqlite3 *db;


//Esta funcion lee las configuration Keys de sqlite y llama a modifyConfigurationKey que es la funcion que las introduce en la
//estructura de datos interna
int readSQLiteConfigurationKeys()
{
	if (db==NULL) return -1;

	sqlite3_stmt *res;

	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	int rc=sqlite3_prepare_v2(db,"SELECT KEY,VALUE FROM ConfigurationKeys",-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT KEY,VALUE FROM ConfigurationKeys'");
		return -1;
	}

	rc=sqlite3_step(res);

	while (rc==SQLITE_ROW)
	{
		modifyConfigurationKey(sqlite3_column_text(res,0),sqlite3_column_text(res,1));
		rc=sqlite3_step(res);
	}

	sqlite3_finalize(res);

	SQLITEMutex=0;


	return 0;
}

//Esta funcion lee una serie de valores de la base de datos en el arranque. No se incluye aqui la lectura de la lista y la cache de autorizacion
char *readChargePointValueFromSQLite(char *key)
{
	if (db==NULL) return NULL;

//	printf("ZZZ");

	sqlite3_stmt *res;

	char *sql=(char *)calloc(1,256);

	strcpy(sql,"SELECT StringValue FROM ChargePointValues where ValueKey='");
	strcat(sql,key);
	strcat(sql,"';");

	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT StringValue FROM ChargePointValues where ValueKey='");
		return NULL;
	}

	rc=sqlite3_step(res);

	if ( rc!= SQLITE_ROW)
	{
		SQLITEMutex=0;
			return NULL;
	}

	char *encontrado=sqlite3_column_text(res,0);
	char *msg=(char *)calloc(1,256);
	strncpy(msg, encontrado, strlen(encontrado));
	sqlite3_finalize(res);

	free(sql);
	SQLITEMutex=0;

	return msg;
}

//Esta funcion lee una serie de valores de la base de datos en el arranque. No se incluye aqui la lectura de la lista y la cache de autorizacion
void readChargePointValuesFromSQLite()
{
	if (db==NULL) return ;

//	printf("WWW");
	sqlite3_stmt *res;

	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	int rc=sqlite3_prepare_v2(db,"SELECT ValueKey,IntValue,StringValue,FloatValue FROM ChargePointValues",-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT ValueKey,IntValue,StringValue,FloatValue FROM ChargePointValues'");
		return;
	}

	rc=sqlite3_step(res);

	while (rc==SQLITE_ROW)
	{
		const char *valuekey=sqlite3_column_text(res,0);
		const char *text=sqlite3_column_text(res,1);

		//POR AHORA SE PONEN ESTOS VALORES, ES POSIBLE QUE MAS TARDE HAYA QUE AÑADIR OTROS
		if (strcmp(valuekey,"currentChargePointState")==0)
		{
			currentChargePointState=atoi(sqlite3_column_text(res,1));
		}
		else if (strcmp(valuekey,"currentChargePointErrorStatus")==0)
		{
			currentChargePointErrorStatus=atoi(text);
		}
		else if (strcmp(valuekey,"connector1Status")==0)
		{
			connectorStatus[0]=atoi(text);
		}
		else if (strcmp(valuekey,"connector2Status")==0)
		{
			connectorStatus[1]=atoi(text);
		}
		else if (strcmp(valuekey,"connector3Status")==0)
		{
			connectorStatus[2]=atoi(text);
		}
		else if (strcmp(valuekey,"connector4Status")==0)
		{
			connectorStatus[3]=atoi(text);
		}
		else if (strcmp(valuekey,"connector5Status")==0)
		{
			connectorStatus[4]=atoi(text);
		}
		else if (strcmp(valuekey,"connector6Status")==0)
		{
			connectorStatus[5]=atoi(text);
		}

		rc=sqlite3_step(res);
	}

	sqlite3_finalize(res);

	SQLITEMutex=0;
	return;
}

void updateSQLiteChargePointValue(char *Key, char *Value)
{
	if (db==NULL) return ;

	//printf("YYY");
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM ChargePointValues where Key=");
	strcat(sql,Key);
	strcat(sql,";");

	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	//Primero busco si ya existe. Y si ya existe, no lo añado.
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);
	SQLITEMutex=0;
	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM ChargePointValues'");
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	if (!encontrado)
	{
		//No existe ya la reserva, la insertamos
		insertSQLiteChargePointValue(Key, Value);
	}
	else
	{
		//Si existe, la actualizamos
		memset(sql, 0, 256);
		strcpy(sql, "UPDATE ChargePointValues set Value=\"");
		strcat(sql, Value);
		strcat(sql,"\" where Key=");
		strcat(sql,Key);
		strcat(sql,";");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
		SQLITEMutex=0;

		if (rc!=SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			//	return -1;
		}
	}
}

void insertSQLiteChargePointValue(char *Key, char *Value)
{
	if (db==NULL) return;
//	printf("VVV");
	int rc;
	char *currentTime=getCurrentTime();
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;

	//NUM_CONNECTORS
	strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp,ValueKey,IntValue) VALUES (datetime('now'), \"");
	strcat(sql,Key);
	strcat(sql,"\",");
	strcat(sql, Value);
	strcat(sql, ");");

	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
	SQLITEMutex=0;
	if (rc!=SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
		return;
	}

}

//Esta funcion permite almacenar en SQLite el estado del Chargepoint en todo momento.
//La informacion que se almacena es: el numero de conectores, el valor de 'currentChargePointState' y 'currentChargePointErrorStatus'
//Asi como el estado de cada conector con el array 'connectorStatus' y por ultimo los valores de cada conector 'connectorValues'
void writeChargePointValuesToSQLite()
{
	if (db==NULL) return;
	//printf("UUU");
	int rc;
	char *currentTime=getCurrentTime();
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;

	//NUM_CONNECTORS
	strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp,ValueKey,IntValue) VALUES (datetime('now'), \"NumConnectors\",");
	strcat(sql, convert(NUM_CONNECTORS));
	strcat(sql, ");");

	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
		return;
	}

	//currentChargePointState
	memset(sql,0,256);
	strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"currentChargePointState\",");
	strcat(sql, convert(currentChargePointState));
	strcat(sql, ");");

	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
		return;
	}

	//currentChargePointErrorStatus
	memset(sql,0,256);
	strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"currentChargePointErrorStatus\",");
	strcat(sql, convert(currentChargePointErrorStatus));
	strcat(sql, ");");

	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
		return;
	}

	//connector1Status
	if (NUM_CONNECTORS>0)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"connector1Status\",");
		strcat(sql, convert(connectorStatus[0]));
		strcat(sql, ");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			return;
		}
	}

	//connector2Status
	if (NUM_CONNECTORS>1)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"connector2Status\",");
		strcat(sql, convert(connectorStatus[1]));
		strcat(sql, ");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			return;
		}
	}

	//connector3Status
	if (NUM_CONNECTORS>2)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"connector3Status\",");
		strcat(sql, convert(connectorStatus[2]));
		strcat(sql, ");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			return;
		}
	}

	//connector4Status
	if (NUM_CONNECTORS>3)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"connector4Status\",");
		strcat(sql, convert(connectorStatus[3]));
		strcat(sql, ");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			return;
		}
	}

	//connector5Status
	if (NUM_CONNECTORS>4)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"connector5Status\",");
		strcat(sql, convert(connectorStatus[4]));
		strcat(sql, ");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			return;
		}
	}

	//connector6Status
	if (NUM_CONNECTORS>5)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO ChargePointValues (TimeStamp, ValueKey,IntValue) VALUES (datetime('now'), \"connector6Status\",");
		strcat(sql, convert(connectorStatus[5]));
		strcat(sql, ");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			return;
		}
	}
	SQLITEMutex=0;
}

int insertSQLiteConfigurationKey(char *Key, char *value)
{
	if (db==NULL) return -1;
	if ((!Key)||(!value)) return -1;
	//printf("TTT");
	int rc;
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;

	strcpy(sql, "INSERT INTO ConfigurationKeys (Key,Value) VALUES ");
	strcat(sql, "(\"");
	strcat(sql, Key);

	strcat(sql, "\",\"");
	strcat(sql, value);
	strcat(sql, "\");");

	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
		return -1;
	}
	SQLITEMutex=0;
	return 0;
}


void updateSQLiteConfigurationKey(char *key, char *value)
{
	if (db==NULL) return ;
	//printf("SSS");
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM ConfigurationKeys where Key=\"");
	strcat(sql,key);
	strcat(sql,"\";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	//Primero busco si ya existe. Y si ya existe, no lo añado.
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM ConfigurationKeys'");
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);
	SQLITEMutex=0;
	if (!encontrado)
	{
		//Si no existe la clave, la insertamos
		insertSQLiteConfigurationKey(key,value);
	}
	else
	{
		//Si existe, la actualizamos
		memset(sql, 0, 256);
		strcpy(sql, "UPDATE ConfigurationKeys set Value=\"");
		strcat(sql,value);
		strcat(sql,"\" where key=\"");
		strcat(sql,key);
		strcat(sql,"\";");

		int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
		SQLITEMutex=1;

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
		SQLITEMutex=0;

		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			//	return -1;
		}
	}
	SQLITEMutex=0;
}

int SQLiteConnect()
{
	return sqlite3_open(BD, &db);
}

int isSQLiteConnected()
{
	return (db!=NULL);
}

void checkLoginTimeout(char *idTag)
{
	if (db==NULL) return ;
//	printf("RRR");
	char *sql=(char *)calloc(1,256);
	char *encontrado;
	sqlite3_stmt *res;

	strcpy(sql, "select round((julianday(CURRENT_TIMESTAMP)-julianday(LoginTimeStamp)) *86400.0),IdTag  from Logins where LogoutTimeStamp is NULL Limit 1;");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) addLog("Ha fallado la query 'select IdTag from Logins where LogoutTimeStamp is NULL ORDER BY ID DESC limit 1'", NULL,NULL, LOG_ERROR, ANY);
		return;
	}

	rc=sqlite3_step(res);

	if ( rc!= SQLITE_ROW)
	{
		SQLITEMutex=0;
		return;
	}

	char *timePassed_str=sqlite3_column_text(res,0);
	float timePassed=atof(timePassed_str);

	if ((timePassed>=560.0) && (timePassed<=570.0))
	{
		sendToDisplay("You will be logged out in 30 seconds");
	}

	if (timePassed>600.0)
	{
		char *IdTag=sqlite3_column_text(res,1);
		char *msg=calloc(1,256);
		sprintf(msg, "User %s Timed out. Logging out.", IdTag);
		//Log
		addLog(msg, NULL,NULL, LOG_WARNING, ANY);
		//Display
		sendToDisplay(msg);

		LogOut(IdTag);
		free(msg);
	}

	sqlite3_finalize(res);

	free(sql);
	SQLITEMutex=0;
}

//connector es un valor 1 a N
char * whoDidLastLoggedIn()
{
	if (db==NULL) return -1;
//	printf("QQQ");
	char *sql=(char *)calloc(1,256);
	char *encontrado;
	sqlite3_stmt *res;

	strcpy(sql, "select IdTag from Logins where LogoutTimeStamp is NULL ORDER BY ID DESC limit 1;");

	//addLog("Se lanza la query %s", sql, NULL, LOG_DEBUG, ANY);

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) addLog("Ha fallado la query 'select IdTag from Logins where LogoutTimeStamp is NULL ORDER BY ID DESC limit 1'", NULL,NULL, LOG_ERROR, ANY);
		return NULL;
	}

	rc=sqlite3_step(res);

	if ( rc!= SQLITE_ROW)
	{
		SQLITEMutex=0;
		return NULL;
	}

	encontrado=sqlite3_column_text(res,0);
	sqlite3_finalize(res);

	free(sql);
	SQLITEMutex=0;
	return encontrado;
}


//connector es un valor 1 a N. Esta funcion busca el ultimo usuario que se logó con ese conector
//La razon de hacerse asi es que como marcos puede tardar un tiempo largo en hacer sus pruebas, el usuario se puede haber deslogado.
char * whoDidLastLogInConnector(int connector)
{
	if (db==NULL) return NULL;
	//printf("PPP");
	char *sql=(char *)calloc(1,256);

	char *encontrado;
	sqlite3_stmt *res;

	strcpy(sql, "select IdTag from Logins where connector=");
	strcat(sql, convert(connector));
	strcat(sql, " LIMIT 1;");

	addLog("Se lanza la query %s", sql, NULL, LOG_DEBUG, ANY);

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;


	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) addLog("Ha fallado la query 'select IdTag from Logins where LogoutTimeStamp is NULL ORDER BY ID DESC limit 1'", NULL,NULL, LOG_ERROR, ANY);
		return NULL;
	}

	rc=sqlite3_step(res);

	if ( rc!= SQLITE_ROW)
	{
		SQLITEMutex=0;
			return NULL;
	}

	encontrado=sqlite3_column_text(res,0);
	char *msg=(char *)calloc(1,256);
	strncpy(msg, encontrado, strlen(encontrado));
	sqlite3_finalize(res);

	free(sql);
	SQLITEMutex=0;
	return msg;
}

//Esta funcion se lanza en el arranque del chargepoint y cierra (actualiza el logout timestamp) de todos los logins y transacciones
//que por una parada inesperada pudieran haberse quedado sin cerrar. Logicamente pone como timestamp el momento del arranque. Se podría
//hacer que al recibir heartbeats se actualizara un campo del sqlite y utilizara ese tiempo...
void closeLoginsAndTransactions()
{
	if (db==NULL) return;
//	printf("OOO");
	char *currentTime=getCurrentTime();
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;
	sqlite3_stmt *res;
	char *lastheartbeat;


	//Obtenemos el ultimo heartbeat
	strcpy(sql, "update Logins set LogoutTimeStamp=(select TimeStamp from ChargepointValues where  ValueKey='LastHeartbeat') where LogoutTimeStamp is NULL;");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	int rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) addLog("Ha fallado la query 'update Logins set LogoutTimeStamp=(select TimeStamp from ChargepointValues where  ValueKey='LastHeartbeat') where LogoutTimeStamp is NULL;", NULL,NULL, LOG_ERROR, ANY);
		return;
	}

	memset(sql, 0, 256);

	strcpy(sql, "update Transactions set StopTimeStamp=(select TimeStamp from ChargepointValues where ValueKey='LastHeartbeat') where StopTimeStamp is NULL;");
	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) addLog("Ha fallado la query 'update Transactions set StopTimeStamp=(select TimeStamp from ChargepointValues where ValueKey='LastHeartbeat') where StopTimeStamp is NULL;'", NULL,NULL, LOG_ERROR, ANY);
		return;
	}

	SQLITEMutex=0;
}


void updateSqliteChargePointFloatValue(char *key, double valor)
{
	if (db==NULL) return;
//	printf("\nNNN: %d", SQLITEMutex);
	int rc;
	char *valor_str=(char *)calloc(1,16);
	ftoa(valor, valor_str,2);
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;

	//Preparamos la query
	sprintf(sql, "insert or replace into ChargePointValues (Id,ValueKey,TimeStamp,FloatValue) VALUES ((select MAX(Id) from ChargePointValues where ValueKey=\"%s\"),\"%s\",datetime('now'),%2.2lf);", key,key,valor);

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
	}

	SQLITEMutex=0;
//	if (sql) free(sql);
}

//Esta funcion actualiza un campo del sqlite en cada heartbeat. El objetivo es que, en caso de caida del chargepoint, saber el momento de la caida para actualizar campos
void updateHeartbeatInDB()
{
	if (db==NULL) return;
//	printf("\nMMM: %d", SQLITEMutex);
	int rc;
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;

	//Preparamos la query
	strcpy(sql, "insert or replace into ChargePointValues (Id,ValueKey,TimeStamp,IntValue) VALUES ((select MAX(Id) from ChargePointValues where ValueKey=\"LastHeartbeat\"),\"LastHeartbeat\", datetime('now'), 0);");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
	}

	SQLITEMutex=0;
//	if (sql) free(sql);
}

int isAnyBodyLoggedIn()
{
	return isLoggedIn(NULL);
}

//Si Idtag es NULL nos indica si hay alguien conectado.
//Connector debe ser un valor 1 a N
int isLoggedInConnector(int connector)
{
	if (db==NULL) return -1;
//	printf("LLL");
	if (connector<0 || connector>NUM_CONNECTORS) return -1;

	int encontrado=0;
	sqlite3_stmt *res;
	char *sql=(char *)calloc(1,256);

	//Primero busco si ya existe. Y si ya existe, no lo añado.

	strcpy(sql, "SELECT count(*) FROM Logins where LogoutTimeStamp is NULL and connector=");
	strcat(sql, convert(connector));
	strcat(sql, ";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM Logins'");
		return 0;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	SQLITEMutex=0;
	return encontrado;
}

//Si Idtag es NULL nos indica si hay alguien conectado.
//En caso de que haya alguno, nos devuelve el conector en el que esta logado
int isLoggedIn(char *idtag)
{
	if (db==NULL) return -1;

//	printf("\nKKK con idTag %s", idtag);

	int encontrado=0;
	sqlite3_stmt *res;
	char *sql=(char *)calloc(1,256);
	//Primero busco si ya existe. Y si ya existe, no lo añado.

	if (idtag)
	{
		//Para saber si alguien esta logado actualmente, la query realziada es que tenga login time pero no logout time
		strcpy(sql, "SELECT connector FROM Logins where IdTag=\"");
		strcat(sql,idtag);
		strcat(sql,"\" AND LogoutTimeStamp is NULL limit 1;");
	}
	else
	{
		//Para saber si alguien esta logado actualmente, la query realziada es que tenga login time pero no logout time
		strcpy(sql, "SELECT count(*) FROM Logins where LogoutTimeStamp is NULL;");
	}

	//addLog("Se lanza la query %s", sql, NULL, LOG_DEBUG, ANY);

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
			if (debug) printf("Ha fallado la query 'SELECT count(*) FROM Logins'");
			return 0;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	SQLITEMutex=0;
	return encontrado;
}


int updateLogoutIntoSQLite(char *idtag)
{
	if (db==NULL) return -1;

	//printf("JJJ");
	if (!idtag) return -1;

	int encontrado=0;
	sqlite3_stmt *res;
	char *sql=(char *)calloc(1,1024);
	char *ErrMsg;

	strcpy(sql, "UPDATE Logins set LogoutTimeStamp=datetime(\"now\") where IdTag=\"");
	strcat(sql,idtag);
	strcat(sql,"\" AND LogoutTimeStamp is NULL;");

	addLog("Se lanza la query %s", sql, NULL, LOG_DEBUG, ANY);

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	int numretries=0;
	if ( rc!= SQLITE_OK)
	{
			while (rc==SQLITE_LOCKED && numretries<3)
			{
				//lo reintenta cada 50ms
				usleep(50000);
				rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
				numretries++;
			}

			if (numretries==3)
			{
				SQLITEMutex=0;
				if (debug) addLog("Ha fallado la query 'UPDATE Logins set LogoutTimeStamp=datetime('now') where IdTag'", NULL,NULL,LOG_ERROR, ANY);
				return 0;
			}
	}

	SQLITEMutex=0;
	return encontrado;
}

//status ser 0 para REJECTED y 1 para ACCEPTED
//returns 0 if succesfully logged in or -1 otherwise
int insertSQLiteLogin(char *Idtag, int status, int connector)
{
	if (db==NULL) return -1;
//	printf("III");
	if (!Idtag) return -1;

	int rc;
	char *sql=(char *)calloc(1,512);
	char *ErrMsg;

	if (status==0)
	{
		//Si se ha rechazado el login, directamente ponemos la fecha de logout la misma:
		strcpy(sql, "INSERT INTO Logins (LoginTimeStamp,LogoutTimeStamp,Status,IdTag, connector) VALUES (datetime('now'),datetime('now'),");
	}
	else
	{
		//Si el acceso esta permitido, hacemos el login y lo dejamos con el momento de salida a NULL
		strcpy(sql, "INSERT INTO Logins (LoginTimeStamp,LogoutTimeStamp,Status,IdTag, connector) VALUES (datetime('now'),NULL,");
	}
	//con convert convertimos un entero a cadena
	strcat(sql, convert(status));
	strcat(sql, ",\"");
	strcat(sql, Idtag);
	strcat(sql, "\",");
	strcat(sql, convert(connector));
	strcat(sql, ");");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		sqlite3_free(ErrMsg);
		SQLITEMutex=0;
		return -1;
	}

	SQLITEMutex=0;
	return 0;
}
/*
//status ser 0 para REJECTED y 1 para ACCEPTED
//returns 0 if succesfully logged in or -1 otherwise
int insertSQLiteLoginRejected(char *Idtag, int status, int conector)
{
	if (db==NULL) return -1;
	if (!Idtag) return -1;

	int rc;
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;

	strcpy(sql, "INSERT INTO Logins (LoginTimeStamp,LogoutTimeStamp,Status,IdTag, Connector) VALUES (datetime('now'),datetime('now'),");
	//con convert convertimos un entero a cadena
	strcat(sql, convert(status));
	strcat(sql, ",\"");
	strcat(sql, Idtag);
	strcat(sql, "\",");
	strcat(sql, convert(conector));
	strcat(sql, ");");

	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		sqlite3_free(ErrMsg);
		return -1;
	}

	return 0;
}
*/

int insertSQLiteLog(char *Message, int warning_level)
{
	//printf("\nLOGAMOS %s", Message);
	if (db==NULL) return -1;
	if (!Message) return -1;
	//printf("HHH: %d", SQLITEMutex);
	int rc;
	char *currentTime=(char *)getCurrentTime();
	char *sql=(char *)calloc(1,1024);
	char *ErrMsg;

	strcpy(sql, "INSERT INTO Logs (TimeStamp,WarningLevel,Message) VALUES");
	strcat(sql, "(\"");
	strcat(sql, currentTime);
	strcat(sql, "\",");
	//con convert convertimos un entero a cadena
	strcat(sql, convert(warning_level));
	strcat(sql, ",'");
	strcat(sql, Message);
	strcat(sql, "');");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if (rc!=SQLITE_OK)
	{
		sqlite3_free(ErrMsg);
		SQLITEMutex=0;
		printf("\nFALLO LA QUERY %s", sql);
		return -1;
	}

	SQLITEMutex=0;
	return 0;
}

//Escribe la autorization cache al SQLite
//AUTHORIZATION_CACHE_SIZE
void writeAuthCacheToSQLite(int authorization_cache_size)
{
	if (db==NULL) return;
	//printf("GGG");
	int rc;
	char *currentTime=getCurrentTime();
	char *sql=(char *)calloc(1,256);
	char *ErrMsg;
	struct authorization_cache_record *object;

	addLog("WRITING AUTHORIZATION CACHE TO SQLITE", NULL,NULL,LOG_INFO,ANY);

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;

	if (authorization_cache_size>0)
	{
		strcpy(sql, "DELETE FROM AuthorizationCache;");
		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nFalló al ejecutar la query: %s", sql);
			sqlite3_free(ErrMsg);
		}
	}

	object=NULL;
	for (int i=0; i<authorization_cache_size; i++)
	{
		object=authorization_Cache[i];

		//Me da igual que luego cuando lea, el orden de array authorization_cache no sea el mismo...
		if (object)
		{
			memset(sql,0,256);
			strcpy(sql, "INSERT INTO AuthorizationCache (IdTag,Status,ExpiryDate,parentIdTag) VALUES");
			strcat(sql, "(\"");
			strcat(sql, object->IdTag);
			strcat(sql, "\",");
			strcat(sql, convert(object->status));

			if (object->expiryDate)
			{
				strcat(sql, ",datetime(\"");
				char *Hour=(char *)calloc(1, sizeof(char)*80);
				strftime (Hour, 80, "%Y-%m-%dT%H:%M:%S", object->expiryDate);
				strcat(sql, Hour);
				strcat(sql, "\")");
			}
			else
			{
				strcat(sql, ",NULL");
			}

			strcat(sql, ",\"");
			strcat(sql, object->parentIdTag);
			strcat(sql, "\");");

			//printf("\n\n\nSE ENVIA LA QUERY: %s\n\n\n", sql);

			rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);


			if (rc!=SQLITE_OK)
			{
				sqlite3_free(ErrMsg);
				SQLITEMutex=0;
				return ;
			}

		}
	}
	SQLITEMutex=0;
}

void writeAuthListToSQLite()
{
	if (db==NULL) return;
	//printf("FFF");
	int rc;
	char *currentTime=getCurrentTime();
	char *sql=(char *)calloc(1,4096);
	char *ErrMsg;

	struct authorization_list_entry *l=authorizationList;

	addLog("WRITING AUTHORIZATION LIST TO SQLITE.", NULL,NULL,LOG_INFO,ANY);

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	if (l != NULL)
	{
		strcpy(sql, "DELETE FROM AuthorizationList;");
		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nFalló al ejecutar la query: %s", sql);
			sqlite3_free(ErrMsg);
		}
	}


	while (l != NULL)
	{
		memset(sql,0,4096);
		strcpy(sql, "INSERT INTO AuthorizationList (IdTag,Status,ExpiryDate,parentIdTag) VALUES");
		strcat(sql, "(\"");
		strcat(sql, l->idTag);
		if (!l->entry)
		{
				strcat(sql,"\");");
		}
		else
		{
				strcat(sql, "\",");
				strcat(sql, convert(l->entry->status));

				if (l->entry->expiryDate)
				{
					strcat(sql, ",datetime(\"");
					char *Hour=(char *)calloc(1, sizeof(char)*80);
					strftime (Hour, 80, "%Y-%m-%dT%H:%M:%S", l->entry->expiryDate);
					strcat(sql, Hour);
					strcat(sql, "\")");
				}
				else
				{
					strcat(sql, ",NULL");
				}

				strcat(sql, ",\"");
				strcat(sql, l->entry->parentIdTag);
				strcat(sql, "\");");

		}

	//	printf("\n\n\nSE ENVIA LA QUERY: %s\n\n\n", sql);
		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nFalló al ejecutar la query: %s", sql);
			sqlite3_free(ErrMsg);
		//	return ;
		}

		l = l->next;
	}

	SQLITEMutex=0;
}


//Lee la autorization List del fichero que se le pasa como parámetro
void readListFromSQLite()
{
	struct authorization_record *registro;
	struct authorization_list_entry *list, *r,*origen;
	//printf("EEE");

	//Creamos la primera entrada de la lista
	list=(struct authorization_list_entry *)calloc(1, sizeof(struct authorization_list_entry));
	list->entry=NULL;
	list->next=NULL;

	//origen sera un puntero a la primera entrada de la lista
	origen=list;
	//r sera un puntero a la ultima entrada de la lista.
	r=list;

	//////////////////////////////BD/////////////////////////////////////

	if (db==NULL) return ;

	sqlite3_stmt *res;

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_prepare_v2(db,"SELECT IdTag,Status,expiryDate,parentIdTag FROM AuthorizationList",-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		printf("IIII: %d", SQLITEMutex);
		if (debug) printf("Ha fallado la query 'SELECT IdTag,Status,expiryDate,parentIdTag FROM AuthorizationList'");
		return;
	}

	rc=sqlite3_step(res);
	SQLITEMutex=0;

	int version;

	while (rc==SQLITE_ROW)
	{
		version=1;
		//version=getVersionFromSQLiteTableChargePointValues(); <-- NOT IMPLEMENTED. HARDCODED TO 1
		update_authorization_list(sqlite3_column_text(res,0), sqlite3_column_text(res,2), sqlite3_column_text(res,3), sqlite3_column_int(res,1), version,1);

		rc=sqlite3_step(res);

	}

//	show_authorization_list();
	sqlite3_finalize(res);

	//Si la lista esta vacia, tambien liberamos la entrada vacia que creamos arriba del todo
	if (!list->entry)
	{
		 free(list);
		 list=NULL;
	}
	SQLITEMutex=0;

}

//Lee la Authorization Cache desde el fichero que se le pasa como parámetro
void readCacheFromSQLite()
{
	struct authorization_cache_record *object3=calloc(1, sizeof(struct authorization_record));

	if (db==NULL) return ;
//	printf("DDD");

	sqlite3_stmt *res;

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_prepare_v2(db,"SELECT IdTag,Status,expiryDate,parentIdTag FROM AuthorizationCache",-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT IdTag,Status,expiryDate,parentIdTag FROM AuthorizationCache'");
		return;
	}

	int i=0;

	rc=sqlite3_step(res);

	while (rc==SQLITE_ROW)
	{
		update_authorization_cache(sqlite3_column_text(res,0), sqlite3_column_text(res,2), sqlite3_column_text(res,2), atoi(sqlite3_column_text(res,1)));
		/*
		strncpy(object3->IdTag, sqlite3_column_text(res,0), 20);
		object3->IdTag[19]='\0';

		object3->status=atoi(sqlite3_column_text(res,1));

		object3->expiryDate=(struct tm *)calloc(1, sizeof(struct tm));
		strptime(sqlite3_column_text(res,2), "%Y-%m-%dT%H:%M:%S.", object3->expiryDate);

		strncpy(object3->parentIdTag, sqlite3_column_text(res,2), 20);

		authorization_Cache[i]=object3;
		i++;
		*/
		rc=sqlite3_step(res);
	}

	sqlite3_finalize(res);
	SQLITEMutex=0;
}

void insertSQLiteAuthorizationListEntry(const char *idtag, int status, const char *expiryDate, const char *parentidtag)
{
	if (db==NULL) return ;
//	printf("CCC");
		sqlite3_stmt *res;
		char *ErrMsg;
		char *sql=(char *)calloc(1,256);
		int encontrado=0;

		//Primero busco si ya existe. Y si ya existe, no lo añado.
		strcpy(sql, "SELECT count(*) FROM AuthorizationList where IdTag=\"");
		strcat(sql,idtag);
		strcat(sql,"\";");


		//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
		int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
		SQLITEMutex=1;
		//Lanzamos la query
		int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

		if ( rc!= SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("Ha fallado la query 'SELECT count(*) FROM AuthorizationList'");
			return;
		}

		rc=sqlite3_step(res);
		encontrado=sqlite3_column_int(res,0);
		sqlite3_finalize(res);

		//Si no se encuentra este idtag en el SQLite, se inserta:
		if (!encontrado)
		{
			memset(sql,0,256);
			strcpy(sql, "INSERT INTO AuthorizationList (IdTag, status, expiryDate, parentIdTag) VALUES(\"");
			strcat(sql, idtag);
			strcat(sql, "\",");
			strcat(sql, convert(status));
			strcat(sql, ",datetime(\"");
			strcat(sql,expiryDate);
			strcat(sql,"\"),\"");
			strcat(sql, parentidtag);
			strcat(sql, "\");");

			rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

			if (rc!=SQLITE_OK)
			{
					if (debug) printf("\nFalló al ejecutar la query: %s", sql);
					sqlite3_free(ErrMsg);
				//	return ;
			}
		}
		SQLITEMutex=0;
}

void insertSQLiteAuthorizationCacheEntry(char *idtag, int status, char *expiryDate, char *parentidtag)
{
	if (db==NULL) return ;
//	printf("\n111: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM AuthorizationCache where IdTag=\"");
	strcat(sql,idtag);
	strcat(sql,"\";");

	//Primero busco si ya existe. Y si ya existe, no lo añado.

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM AuthorizationCache'");
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	//Si no se encuentra este idtag en el SQLite, se inserta:
	if (!encontrado)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO AuthorizationCache (IdTag, status, expiryDate, parentIdTag) VALUES(\"");
		strcat(sql, idtag);
		strcat(sql, "\",");
		strcat(sql, convert(status));
		strcat(sql, ",datetime(\"");
		strcat(sql,expiryDate);
		strcat(sql,"\"),\"");
		strcat(sql, parentidtag);
		strcat(sql, "\");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
				if (debug) printf("\nFalló al ejecutar la query: %s", sql);
				sqlite3_free(ErrMsg);
			//	return ;
		}
	}
	SQLITEMutex=0;
}


void updateSQLiteAuthorizationCacheEntry(char *idtag, int status, char *expiryDate, char *parentidtag)
{
	if (db==NULL) return ;
//	printf("\n222: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM AuthorizationCache where IdTag=\"");
	strcat(sql,idtag);
	strcat(sql,"\";");

	//Primero busco si ya existe. Y si ya existe, no lo añado.

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM AuthorizationCache'");
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	if (!encontrado)
	{
		insertSQLiteAuthorizationCacheEntry(idtag, status, expiryDate,parentidtag);
	}
	else
	{
		//Si existe, la actualizamos
		memset(sql, 0, 256);
		strcpy(sql, "UPDATE AuthorizationCache set status=");
		strcat(sql,convert(status));

		if (expiryDate)
		{
			strcat(sql,",ExpiryDate=datetime(\"");
			strcat(sql,expiryDate);
			strcat(sql,"\")");
		}

		if (parentidtag)
		{
			strcat(sql,",ParentIdTag=\"");
			strcat(sql,parentidtag);
			strcat(sql,"\"");
		}

		strcat(sql," where IdTag=\"");
		strcat(sql,idtag);
		strcat(sql,"\";");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			//	return -1;
		}
	}
	SQLITEMutex=0;
}

void updateSQLiteAuthorizationListEntry(const char *idtag, int status, const char *expiryDate, const char *parentidtag)
{
	if (db==NULL) return ;
//	printf("\n333: %d", SQLITEMutex);
		sqlite3_stmt *res;
		char *ErrMsg;
		char *sql=(char *)calloc(1,256);
		int encontrado=0;

		strcpy(sql, "SELECT count(*) FROM AuthorizationList where IdTag=\"");
		strcat(sql,idtag);
		strcat(sql,"\";");

		//Primero busco si ya existe. Y si ya existe, no lo añado.

		//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
		int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
		SQLITEMutex=1;
		//Lanzamos la query
		int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

		if ( rc!= SQLITE_OK)
		{
			SQLITEMutex=0;
			if (debug) printf("Ha fallado la query 'SELECT count(*) FROM AuthorizationList'");
			return;
		}

		rc=sqlite3_step(res);
		encontrado=sqlite3_column_int(res,0);
		sqlite3_finalize(res);

		if (!encontrado)
		{
			insertSQLiteAuthorizationCacheEntry(idtag, status, expiryDate,parentidtag);
		}
		else
		{
			//Si existe, la actualizamos
			memset(sql, 0, 256);
			strcpy(sql, "UPDATE AuthorizationList set status=");
			strcat(sql,convert(status));

			if (expiryDate)
			{
				strcat(sql,",ExpiryDate=datetime(\"");
				strcat(sql,expiryDate);
				strcat(sql,"\")");
			}

			if (parentidtag)
			{
				strcat(sql,",ParentIdTag=\"");
				strcat(sql,parentidtag);
				strcat(sql,"\"");
			}

			strcat(sql," where IdTag=");
			strcat(sql,idtag);
			strcat(sql,";");

			rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

			if (rc!=SQLITE_OK)
			{
				if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
				sqlite3_free(ErrMsg);
				//	return -1;
			}
		}
		SQLITEMutex=0;
}


void insertReservationInSQLite(int reservationId, int connector, char *expiryDate, char *Idtag)
{
	if (db==NULL) return ;
//	printf("\n444: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM Reservations where ReservationId=");
	strcat(sql,convert(reservationId));
	strcat(sql,";");

	//Primero busco si ya existe. Y si ya existe, no lo añado.
	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		if (debug) printf("Ha fallado la query %s con error %d", sql, rc);
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	//Si no se encuentra este idtag en el SQLite, se inserta:
	if (!encontrado)
	{
		memset(sql,0,256);
		strcpy(sql, "INSERT INTO Reservations (ReservationId, Connector, ExpiryDate, IdTag) VALUES(");
		strcat(sql, convert(reservationId));
		strcat(sql, ",");
		strcat(sql, convert(connector));
		strcat(sql, ",datetime(\"");
		strcat(sql,expiryDate);
		strcat(sql,"\"),\"");
		strcat(sql, Idtag);
		strcat(sql, "\");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
					if (debug) printf("\nFalló al ejecutar la query: %s", sql);
					sqlite3_free(ErrMsg);
				//	return ;
		}
	}
	SQLITEMutex=0;
}

void closeReservationInSQLite(int reservationId)
{
	if (db==NULL) return ;
//	printf("\n555: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);

	strcpy(sql, "UPDATE Reservations set ExpiryDate=datetime('now') where reservationId=");
	strcat(sql,convert(reservationId));
	strcat(sql,";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	int rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
	SQLITEMutex=0;

	if (rc!=SQLITE_OK)
	{
		if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
		sqlite3_free(ErrMsg);
		//	return -1;
	}
}

void updateReservationInSQLite(int reservationId, int connector, char *expiryDate, char *Idtag)
{
	if (db==NULL) return ;
//	printf("\n666: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM Reservations where ReservationId=");
	strcat(sql,convert(reservationId));
	strcat(sql,";");

	//Primero busco si ya existe. Y si ya existe, no lo añado.

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query %s con error %d", sql, rc);
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	if (!encontrado)
	{
		//No existe ya la reserva, la insertamos
		insertReservationInSQLite(reservationId, connector, expiryDate, Idtag);
	}
	else
	{
		//Si existe, la actualizamos
		memset(sql, 0, 256);
		strcpy(sql, "UPDATE Reservations set connector=");
		strcat(sql,convert(connector));
		strcat(sql,",ExpiryDate=datetime(\"");
		strcat(sql,expiryDate);
		strcat(sql,"\")");
		strcat(sql,",IdTag=\"");
		strcat(sql,Idtag);
		strcat(sql,"\" where reservationId=");
		strcat(sql,convert(reservationId));
		strcat(sql,";");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
				sqlite3_free(ErrMsg);
					//	return -1;
		}
	}
	SQLITEMutex=0;
}

void removeReservationFromSQLite(int reservationId)
{
	if (db==NULL) return ;
//	printf("\n777: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);

	int encontrado=0;

	strcpy(sql, "DELETE * FROM Reservations where ReservationId=");
	strcat(sql,convert(reservationId));
	strcat(sql,";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query
	int rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

	if ( rc!= SQLITE_OK)
	{
			SQLITEMutex=0;
			if (debug) printf("Ha fallado la query 'DELETE * FROM Reservations con error %d", rc);
			return;
	}
	SQLITEMutex=0;
}

//Transactions
void insertTransactionInSQLite(int transactionId, int connector, char *startTime, char *stopTime, char *Idtag, int currentDelivered)
{
	if (db==NULL) return ;

	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM Transactions where TransactionId=");
	strcat(sql,convert(transactionId));
	strcat(sql,";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	//Primero busco si ya existe. Y si ya existe, no lo añado.
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM Transactions  con error %d", rc);
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	//Si no se encuentra este idtag en el SQLite, se inserta:
	if (!encontrado)
	{
		memset(sql,0,256);
		if (stopTime)
		{
				strcpy(sql, "INSERT INTO Transactions(TransactionId, Connector, StartTimeStamp, StopTimeStamp, currentDelivered, IdTag) VALUES(");
		}
		else
		{
				strcpy(sql, "INSERT INTO Transactions(TransactionId, Connector, StartTimeStamp, currentDelivered, IdTag) VALUES(");
		}

		//transactionId
		strcat(sql, convert(transactionId));
		strcat(sql, ",");

		//connector
		strcat(sql, convert(connector));

		if (startTime) startTime[strlen(startTime)-1]='\0';
		//startTime
		strcat(sql, ",datetime(\"");
		strcat(sql,startTime);
		strcat(sql,"\")");

		if (stopTime)
		{
			strcat(sql, ",datetime(\"");
			strcat(sql,stopTime);
			strcat(sql,"\")");
		}

		//currentdelivered
		strcat(sql, ",");
		strcat(sql,convert(currentDelivered));

		//IdTag
		strcat(sql, ",\"");
		strcat(sql, Idtag);
		strcat(sql, "\");");

		//addLog("SE EJECUTA LA QUERY: %s", sql,NULL,LOG_INFO,ANY);

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
		SQLITEMutex=0;

		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nFalló al ejecutar la query: %s", sql);
			sqlite3_free(ErrMsg);
			//	return ;
		}
	}
	SQLITEMutex=0;
}

void updateStopTransactionInSQLite(int transactionId, char *stopTime)
{
	if (db==NULL) return ;
//	printf("\n888: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM Transactions where TransactionId=");
	strcat(sql,convert(transactionId));
	strcat(sql,";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	//Primero busco si ya existe. Y si ya existe, no lo añado.
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM Transactions con error %d", rc);
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	//Si no exsite esa transaccion. Escribimos un mensaje y salimos
	if (!encontrado)
	{
		SQLITEMutex=0;
		addLog("No se ha encontrado la transaccion %s en el SQLite", convert(transactionId), NULL, LOG_ERROR,ANY);

		return;
	}
	else
	{
			//Si existe, la actualizamos
		memset(sql, 0, 256);
		strcpy(sql, "UPDATE Transactions set StopTimeStamp=datetime(\"");
		strcat(sql,stopTime);
		strcat(sql,"\") where TransactionId=");
		strcat(sql,convert(transactionId));
		strcat(sql,";");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
		SQLITEMutex=0;
		if (rc!=SQLITE_OK)
		{
				if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
				sqlite3_free(ErrMsg);
				//	return -1;
		}
	}
}

void updateCurrentDuringTransactionInSQLite(int transactionId, float currentDelivered)
{
	if (db==NULL) return ;
	//printf("\n999: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);
	int encontrado=0;

	strcpy(sql, "SELECT count(*) FROM Transactions where TransactionId=");
	strcat(sql,convertF(transactionId));
	strcat(sql,";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	//Primero busco si ya existe. Y si ya existe, no lo añado.
	int rc=sqlite3_prepare_v2(db,sql,-1, &res,0);

	if ( rc!= SQLITE_OK)
	{
		SQLITEMutex=0;
		if (debug) printf("Ha fallado la query 'SELECT count(*) FROM Transactions con error %d", rc);
		return;
	}

	rc=sqlite3_step(res);
	encontrado=sqlite3_column_int(res,0);
	sqlite3_finalize(res);

	//Si no exsite esa transaccion. Escribimos un mensaje y salimos
	if (!encontrado)
	{
		SQLITEMutex=0;
		addLog("No se ha encontrado la transaccion %s en el SQLite", convert(transactionId), NULL, LOG_ERROR,ANY);
		return;
	}
	else
	{
		//Si existe la transaccion, asi que la actualizamos
		memset(sql, 0, 256);
		strcpy(sql, "UPDATE Transactions set currentDelivered=");
		strcat(sql,convert(currentDelivered));
		strcat(sql,"\");");

		rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);

		SQLITEMutex=0;
		if (rc!=SQLITE_OK)
		{
			if (debug) printf("\nHA FALLADO LA QUERY %s con error %d", sql, rc);
			sqlite3_free(ErrMsg);
			//	return -1;
		}
	}
}

void removeTransactionFromSQLite(int transactionId)
{
	if (db==NULL) return ;
	printf("\n101010: %d", SQLITEMutex);
	sqlite3_stmt *res;
	char *ErrMsg;
	char *sql=(char *)calloc(1,256);

	int encontrado=0;

	strcpy(sql, "DELETE * FROM Transactions where TransactionId=");
	strcat(sql,transactionId);
	strcat(sql,";");

	//Obtenemos el Semaforo (y nos aseguramos de que no se quede infinitamente parado - en 5 segundos libera el semaforo)
	int cc=0; while (SQLITEMutex && cc<100) { usleep(50); cc++;}
	SQLITEMutex=1;
	//Lanzamos la query

	int rc=sqlite3_exec(db,sql,NULL, NULL,&ErrMsg);
	SQLITEMutex=0;

	if ( rc!= SQLITE_OK) if (debug) printf("Ha fallado la query 'DELETE * FROM Transactions where TransactionId con error %d", rc);
}
