/*
 * SQLiteConnection.h
 *
 *  Created on: 23 Feb 2018
 *      Author: pi
 */

#ifndef SRC_SQLITECONNECTION_H_
#define SRC_SQLITECONNECTION_H_

#include <sqlite3.h>
#include "aux.h"
#include "localAuthorization.h"

static int SQLITEMutex=0;


//Auxuliar functions
int SQLiteConnect();
int isSQLiteConnected();

//Logs
int insertSQLiteLog(char *Message, int warning_level);

//hEARTBEAT
void updateHeartbeatInDB();

//Logins
int insertSQLiteLogin(char *Idtag, int status, int connector);
int isAnyBodyLoggedIn();
int isLoggedIn(char *idtag);
int isLoggedInConnector(int connector);
int updateLogoutIntoSQLite(char *idtag);
void checkLoginTimeout(char *idTag);

//int insertSQLiteLoginRejected(char *Idtag, int status, int conector);
char * whoDidLastLoggedIn();
char * whoDidLastLogInConnector(int connector);
void closeLoginsAndTransactions();

//Configuration Keys
int readSQLiteConfigurationKeys();
int insertSQLiteConfigurationKey(char *Key, char *value);
void updateSQLiteConfigurationKey(char *key, char *value);

//ChargePointValues
void writeChargePointValuesToSQLite();
void readChargePointValuesFromSQLite();
char *readChargePointValueFromSQLite(char *key);
//void updateSQLiteChargePointValue(char *Key, char *Value);
void insertSQLiteChargePointValue(char *Key, char *Value);
void updateSQLiteChargePointValue(char *key, char *value);
void updateSqliteChargePointFloatValue(char *key, double valor);


//Auhtnetication List & Cache
void writeAuthCacheToSQLite();
void writeAuthListToSQLite();
void readListFromSQLite();
void readCacheFromSQLite();

void insertSQLiteAuthorizationCacheEntry(char *idtag, int status, char *expiryDate, char *parentidtag);
void insertSQLiteAuthorizationListEntry(const char *idtag, int status, const char *expiryDate, const char *parentidtag);
void updateSQLiteAuthorizationCacheEntry(char *idtag, int status, char *expiryDate, char *parentidtag);
void updateSQLiteAuthorizationListEntry(const char *idtag, int status, const char *expiryDate, const char *parentidtag);

//Reservations
void insertReservationInSQLite(int reservationId, int connector, char *expiryDate, char *Idtag);
void updateReservationInSQLite(int reservationId, int connector, char *expiryDate, char *Idtag);
void removeReservationFromSQLite(int reservationId);
void closeReservationInSQLite(int reservationId);

//Transactions
void insertTransactionInSQLite(int transactionId, int connector, char *startTime, char *stopTime, char *Idtag, int currentDelivered);
void updateStopTransactionInSQLite(int transactionId, char *stopTime);
void updateCurrentDuringTransactionInSQLite(int transactionId, float currentDelivered);
void removeTransactionFromSQLite(int transactionId);

#endif /* SRC_SQLITECONNECTION_H_ */
