/*
 * chargingProfile.h
 *
 *  Created on: Dec 19, 2017
 *      Author: root
 */

#ifndef CHARGINGPROFILE_H_
#define CHARGINGPROFILE_H_

#include "aux.h"

#define MAX_PERIODS 32

///////////
//Ver 7.37
enum RecurrencyKindType{
	_RKT_DAILY,
	_RKT_WEEKLY,
};

static const char *RecurrencyKindTypeTexts[]={
	"Daily",
	"Weekly",
};

///////////
//Ver 7.9
enum ChargingProfileKindType{
	_CPKT_ABSOLUTE,
	_CPKT_RECURRING,
	_CPKT_RELATIVE,
};

static const char *ChargingProfileKindTypeTexts[]={
	"Absolute",
	"Recurring",
	"Relative"
};

///////////
//Ver 7.10
enum ChargingProfilePurposeType{
	_CPPT_CHARGEPOINTMAXPROFILE,
	_CPPT_TXDEFAULTPROFILE,
	_CPPT_TXPROFILE,
};

static const char *ChargingProfilePurposeTypeTexts[]={
	"ChargePointMaxProfile",
	"TxDefaultProfile",
	"TxProfile"
};

///////////
//Ver 7.12
enum ChargingRateUnitType{
	_CRUT_W,
	_CRUT_A
};

static const char *ChargingRateUnitTypeTexts[]={
	"W",
	"A",
};

///////////
//Ver 7.14
struct ChargingSchedulePeriod{
	int startPeriod;
	float limit;
	int *numPhases;
	struct ChargingSchedulePeriod *next;
};

///////////
//7.13
struct ChargingSchedule{
	int *duration;
	struct tm *startSchedule; //[0..1]
	enum ChargingRateUnitType chargingRateUnit;
	struct ChargingSchedulePeriod chargingSchedulePeriods;
	float *minChargingRate;
};

///////////
//Ver 7.8
struct ChargingProfile{
	int chargingProfileId;
	int *transactionId;  //[0..1] - Only valid if ChargingProfilePurpose is set to TxProfile,the transactionId
						  // MAY be used to match the	profile to a specific transaction.
	int stackLevel;
	enum ChargingProfilePurposeType chargingProfilePurpose;
	enum ChargingProfileKindType chargingProfileKind;
	enum RecurrencyKindType *recurrencyKind;  //[0..1]
	struct tm *validFrom; //[0..1]
	struct tm *validTo; //[0..1]
	struct ChargingSchedule chargingSchedule;
};

struct ChargingProfileList{
	struct ChargingProfile *chargingProfile;
	struct ChargingProfileList *next;
};

//Pag 24: Hay chargingProfiles para cada connector y otro para el connectorID 0
struct ChargingProfileList *currentChargingProfiles[NUM_CONNECTORS];

////////////////////////////////
//
//Esta funcion inicializa toda a NULL la tabla currentChargingProfiles
void chargingProfileInitialize();

//Devuelve 1 si se añadio exitosamente
//Devuelve 0 si NO pudo añadir el charging profile
//connector debe ser un valor de 0 a N-1
int addChargingProfile(struct ChargingProfile *ChPr, int connector);

//Llamado desde la funcion respondSetChargingProfileRequest() que gestion la solicitud 5.16 SET CHARGING PROFILE.
//Actualmente no hace nada
void reevaluate_chargingProfiles(int connectorId);

//Esta funcion reemplaza un charging profile para un connectorId dado.
//Si actualmente no tiene ningun chargingProfile, se lo asigna.
//Si no, libera el anterior (para lo que llama a la funcion freeChargingProfile) y alloca el nuevo
//connector debe ser un valro de 0 a N-1
void replaceChargingProfile(struct ChargingProfile *ChPr, int connector);

//Libera de memoria toda la compleja estructura de la ChargingProfileList pasada como parámetro
void freeChargingProfileList(struct ChargingProfileList *temp);

//Libera de memoria toda la compleja estructura de la ChargingProfile pasada como parámetro
void freeChargingProfile(struct ChargingProfile *cp);
//
//5.5.  Ver pag 67
//
// This function removes charging profiles from the currentChargingProfiles array depending on the parameters passed
// Parameters are integers. If their values are negative, they are not taken into account.
// Returns 0 if it finds any charging profile to remove
// Returns 1 in other case
// Connector debe ser un valor de 1 a N. Si se recibe un 0 es que no llego el connectorId
int removeChargingProfilesFromConnector(int connector, int purpose, int stacklevel);

//5.5. Ver pag 67
// This function removes charging profiles given the profileId passed
// It browse the whole currentChargingProfiles array looking for that specific charging profile.
// Returns 0 if it finds any charging profile to remove
// Returns 1 in other case
int removeChargingProfilesFromId(int ProfileId);

#endif /* CHARGINGPROFILE_H_ */
