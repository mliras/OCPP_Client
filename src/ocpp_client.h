/*
 * ocpp_client.h
 *
 *  Created on: Nov 8, 2017
 *      Author: root
 */

#ifndef OCPP_CLIENT_H_
#define OCPP_CLIENT_H_

#include "aux.h"
#include <json-c/json.h>

//Pag 10
enum messageTypes{
	_CALLMESSAGE=2,
	_CALLRESULT=3,
	_CALLERROR=4
};

/*
#define _CALLMESSAGE 2
#define _CALLRESULT 3
#define _CALLERROR 4
*/

//Pag 13
enum errorCodes{
	_NOT_IMPLEMENTED,
	_NOT_SUPPORTED,
	_INTERNAL_ERROR,
	_PROTOCOL_ERROR,
	_SECURITY_ERROR,
	_FORMATION_VIOLATION,
	_PROPERTY_CONSTRAINT_VIOLATION,
	_OCCURENCE_CONSTRAINT_VIOLATION,
	_TYPE_CONSTRAINT_VIOLATION,
	_GENERIC_ERROR
};

#define _PORT 5000

int _REGISTRATIONSTATUS;

//MESSAGE ACTIONS
enum action_IDs{
	AUTHORIZE=401,
	BOOT_NOTIFICATION=402,
	DATA_TRANSFER=403,
	DIAGNOSTICS_STATUS_NOTIFICATION=404,
	FIRMWARE_STATUS_NOTIFICATION=405,
	HEARTBEAT=406,
	METER_VALUES=407,
	START_TRANSACTION=408,
	STATUS_NOTIFICATION=409,
	STOP_TRANSACTION=410,
	CANCEL_RESERVATION=501,
	CHANGE_AVAILABILITY=502,
	CHANGE_CONFIGURATION=503,
	CLEAR_CACHE=504,
	CLEAR_CHARGING_PROFILE=505,
	CS_DATA_TRASNFER=506,
	GET_COMPOSITE_SCHEDULE=507,
	GET_CONFIGURATION=508,
	GET_DIAGNOSTICS=509,
	GET_LOCAL_LIST_VERSION=510,
	REMOTE_START_TRANSACTION=511,
	REMOTE_STOP_TRANSACTION=512,
	RESERVE_NOW=513,
	RESET_MSG=514,
	SEND_LOCAL_LIST=515,
	SET_CHARGING_PROFILE=516,
	TRIGGER_MESSAGE=517,
	UNLOCK_CONNECTOR=518,
	UPDATE_FIRMWARE=519,
	ERROR_MESSAGE=601
};

static const char* ActionTexts[] = {
		"Authorize", "BootNotification", "ChangeAvailability", "ChangeConfiguration", "ClearCache","DataTransfer","GetConfiguration",
		"Heartbeat","MeterValues","RemoteStartTransaction","RemoteStopTransaction","Reset","StartTransaction","StatusNotification",
		"StopTransaction","UnlockConnector","GetDiagnostics","DiagnosticsStatusNotification","FirmwareStatusNotification",
		"UpdateFirmware","GetLocalListVersion","SendLocalListVersion","CancelReservation","ReserveNow","ClearChargingProfile",
		"GetCompositeSchedule","SetChargingProfile","TriggerMessage"
};

/////////////////////////////////////////////////
//CHARGEPOINT Authorization. Ver 7.2
enum AuthorizationStatus{
	_CP_AUTHORIZATION_ACCEPTED,
	_CP_AUTHORIZATION_BLOCKED,
	_CP_AUTHORIZATION_EXPIRED,
	_CP_AUTHORIZATION_INVALID,
	_CP_AUTHORIZATION_CONCURRENT_TX
};

static const char* AuthorizationStatusTexts[] = {
		"Accepted", "Blocked", "Expired", "Invalid", "ConcurrentTx"
};

///////////////////////////////////////////////////
//CHARGEPOINT Availability. Ver 7.3
enum AvailabilityStatus{
	_CP_AVAILABILITY_ACCEPTED,
	_CP_AVAILABILITY_REJECTED,
	_CP_AVAILABILITY_SCHEDULED,
};

static const char* AvailabilityStatusTexts[] = {
		"Accepted", "Rejected", "Scheduled"
};

///////////////////////////////////////////////////
//CHARGEPOINT ERRORCODE. Ver 7.4
enum availabilityType{
	_CP_AVAILABILITY_INOPERATIVE,
	_CP_AVAILABILITY_OPERATIVE
};

static const char* availabilityTypeTexts[] = {
		"Inoperative", "Operative"
};

///////////////////////////////////////////////////
//CANCEL RESERVATION STATUS. Ver 7.5
enum CancelReservationStatus{
	_CR_ACCEPTED,
	_CR_REJECTED
};

static const char* CancelReservationStatusTexts[] = {
		"Accepted", "Rejected"
};

///////////////////////////////////////////////////
//CHARGEPOINT ERRORCODE. Ver 7.6
enum ChargePointErrorCode{
	_CP_ERROR_CONNECTORLOCKFAILURE,
	_CP_ERROR_GROUNDFAILURE,
	_CP_ERROR_HIGHTEMPERATURE,
	_CP_ERROR_INTERNALERROR,
	_CP_ERROR_LOCALLISTCONFLICT,
	_CP_ERROR_NOERROR,
	_CP_ERROR_OTHERERROR,
	_CP_ERROR_OVERCURRENTFAILURE,
	_CP_ERROR_OVERVOLTAGE,
	_CP_ERROR_POWERMETERFAILURE,
	_CP_ERROR_POWERSWITCHFAILURE,
	_CP_ERROR_READERFAILURE,
	_CP_ERROR_RESETFAILURE,
	_CP_ERROR_UNDERVOLTAGE,
	_CP_ERROR_WEAKSIGNAL,
	_CP_ERROR_EVCOMMUNICATIONERROR
};

static const char* ChargePointErrorCodeTexts[] = {
		"ConnectorLockFailure",
		"GroundFailure",
		"HighTemperature",
		"InternalError",
		"LocalListConflict",
		"NoError",
		"OtherError",
		"OverCurrentFailure",
		"OverVoltage",
		"PowerMeterFailure",
		"PowerSwitchFailure",
		"ReaderFailure",
		"ResetFailure",
		"UnderVoltage",
		"WeakSignal",
};

///////////////////////////////////////////////////
//CHARGEPOINT STATUS. Ver 7.7
enum ChargePointStatus{
	_CP_STATUS_AVAILABLE,
	_CP_STATUS_PREPARING,
	_CP_STATUS_CHARGING,
	_CP_STATUS_SUSPENDEDEVSE,
	_CP_STATUS_SUSPENDEDEV,
	_CP_STATUS_FINISHING,
	_CP_STATUS_RESERVED,
	_CP_STATUS_UNAVAILABLE,
	_CP_STATUS_FAULTED
};

static const char* ChargePointStatusTexts[] = {
	"Available",
	"Preparing",
	"Charging",
	"SuspendedEVSE",
	"SuspendedEV",
	"Finishing",
	"Reserved",
	"Unavailable",
	"Faulted"
};


///////////////////////////////////////////////////
//CHARGEPOINT STATUS. Ver 7.11
enum ChargingProfileStatus{
	_CPS_ACCEPTED,
	_CPS_REJECTED,
	_CPS_NOTSUPPORTED
};

static const char* chargingProfileStatusTexts[] = {
	"Accepted",
	"Rejected",
	"NotSupported",
};

///////////////////////////////////////////////////
//7.20
enum clearCacheStatus{
	_CCS_ACCEPTED,
	_CCS_REJECTED
};

static const char* clearCacheStatustexts[] = {
	"Accepted",
	"Rejected",
};

///////////////////////////////////////////////////
//7.21
enum clearChargingProfileStatus{
	_CCPS_ACCEPTED,
	_CCPS_UNKNOWN
};

static const char* clearChargingProfileStatustexts[] = {
	"Accepted",
	"Unknown",
};

///////////////////////////////////////////////////
//7.22
enum configurationStatus{
	_CC_ACCEPTED,
	_CC_REJECTED,
	_CC_REBOOT_REQUIRED,
	_CC_NOTSUPPORTED,
};

static const char * configurationStatus_texts[]={
	"Accepted",
	"Rejected",
	"RebootRequired",
	"NotSupported"
};

///////////////////////////////////////////////////
//7.23
enum dataTransferStatus{
	_DT_ACCEPTED,
	_DT_REJECTED,
	_DT_UNKNOWN_MESSAGEID,
	_DT_UNKNOWN_VENDORID,
};

static const char * dataTransferStatus_texts[]={
	"Accepted",
	"Rejected",
	"UnknownMessageId",
	"UnknownVendorId"
};

/////////////////////////////////////////////////
////DIAGNOSTICSNOTIFICATION. Ver 7.24
enum DiagnosticsStatus{
	_DSN_IDLE,
	_DSN_UPLOADED,
	_DSN_UPLOADFAILED,
	_DSN_UPLOADING,
};

static const char * DiagnosticsStatus_texts[]={
	"Idle",
	"Uploaded",
	"UploadFailed",
	"Uploading"
};

/////////////////////////////////////////////////
////FIRMWARESTATUSNOTIFICATION. Ver 7.25
enum FirmwareStatus
{
	_FS_DOWNLOADED,
	_FS_DOWNLOADFAILED,
	_FS_DOWNLOADING,
	_FS_IDLE,
	_FS_INSTALLATIONFAILED,
	_FS_INSTALLING,
	_FS_INSTALLED,
};

static const char * FirmwareStatus_texts[]={
	"Downloaded",
	"DownloadFailed",
	"Downloading",
	"Idle"
	"InstallationFailed"
	"Installing"
	"Installed"
};

///////////////////////////////////////////////////
//7.28 -- OJO: ANTES DE 7.27
struct IdToken{
	char IdToken[20];
};

///////////////////////////////////////////////////
//7.27
struct IdTagInfo{
	char *expiryDate;
	struct IdToken parentIdTag;
	enum AuthorizationStatus status;
};

///////////////////////////////////////////////////
//7.29
struct KeyValue{
	CiString50Type key;
	int readonly;
	CiString500Type value;
};

///////////////////////////////////////////////////
////LOCATION Ver 7.30
enum Location{
	_MV_LOCATION_BODY,
	_MV_LOCATION_CABLE,
	_MV_LOCATION_EV,
	_MV_LOCATION_INLET,
	_MV_LOCATION_OUTLET,
};

static const char * Location_texts[]={
	"Body",
	"Cable",
	"EV",
	"Inlet",
	"Outlet",
};

///////////////////////////////////////////////////
////MEASURAND Ver 7.31
enum Measurand{
	_MV_MEASURAND_CURRENT_EXPORT,
	_MV_MEASURAND_CURRENT_IMPORT,
	_MV_MEASURAND_CURRENT_OFFERED,
	_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_REGISTER,
	_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_REGISTER,
	_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_REGISTER,
	_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_REGISTER,
	_MV_MEASURAND_ENERGY_ACTIVE_EXPORT_INTERVAL,
	_MV_MEASURAND_ENERGY_ACTIVE_IMPORT_INTERVAL,
	_MV_MEASURAND_ENERGY_REACTIVE_IMPORT_INTERVAL,
	_MV_MEASURAND_ENERGY_REACTIVE_EXPORT_INTERVAL,
	_MV_MEASURAND_FREQUENCY,
	_MV_MEASURAND_POWER_ACTIVE_EXPORT,
	_MV_MEASURAND_POWER_ACTIVE_IMPORT,
	_MV_MEASURAND_POWER_FACTOR,
	_MV_MEASURAND_POWER_OFFERED,
	_MV_MEASURAND_POWER_REACTIVE_EXPORT,
	_MV_MEASURAND_POWER_REACTIVE_IMPORT,
	_MV_MEASURAND_RPM,
	_MV_MEASURAND_SOC,
	_MV_MEASURAND_TEMPERATURE,
	_MV_MEASURAND_VOLTAGE,
};

static const char *Measurand_texts[]={
	"Current.Export",
	"Current.Import",
	"Current.Offered",
	"Energy.Active.Export.Register",
	"Energy.Active.Import.Register",
	"Energy.Reactive.Export.Register",
	"Energy.Reactive.Import.Register",
	"Energy.Active.Export.Interval",
	"Energy.Active.Import.Interval",
	"Energy.Reactive.Import.Interval",
	"Energy.Reactive.Export.Interval",
	"Frequency",
	"Power.Active.Export",
	"Power.Active.Import",
	"Power.Factor",
	"Power.Offered",
	"Power.Reactive.Export",
	"Power.Reactive.Import",
	"RPM",
	"SoC",
	"Temperature",
	"Voltage"
};

///////////////////////////////////////////////////
//7.32
enum MessageTrigger{
	_MT_BOOTNOTIFICATION,
	_MT_DIAGNOSTICSSTATUSNOTIFICATION,
	_MT_FIRMWARESTATUSNOTIFICATION,
	_MT_HEARTBEAT,
	_MT_METERVALUES,
	_MT_STATUSNOTIFICATION,
};

static const char * MessageTrigger_texts[]={
	"BootNotification",
	"DiagnosticsStatusNotification",
	"FirmwareStatusNotification",
	"Heartbeat",
	"MeterValues",
	"StatusNotification",
};

///////////////////////////////////////////////////
//7.33. MeterValues
struct MeterValue{
	char *timestamp;
	struct SampledValue *sampledValue;
};

////////////////////////////////////////////////////////////////////////
////PHASE Ver 7.34
enum Phase{
	_MV_PHASE_L1,
	_MV_PHASE_L2,
	_MV_PHASE_L3,
	_MV_PHASE_N,
	_MV_PHASE_L1_N,
	_MV_PHASE_L2_N ,
	_MV_PHASE_L3_N,
	_MV_PHASE_L1_L2,
	_MV_PHASE_L2_L3 ,
	_MV_PHASE_L3_L1 ,
};

static const char * Phase_texts[]={
	"L1",
	"L2",
	"L3",
	"N",
	"L1-N",
	"L2-N",
	"L3-N",
	"L1-L2",
	"L2-L3",
	"L3-L1"
};


////////////////////////////////////////////////////////////////////////
////CONTEXT Ver 7.35
enum ReadingContext{
	_MV_CONTEXT_INTERRUPTION_BEGIN,
	_MV_CONTEXT_INTERRUPTION_END,
	_MV_CONTEXT_OTHER,
	_MV_CONTEXT_SAMPLE_CLOCK,
	_MV_CONTEXT_SAMPLE_PERIODIC,
	_MV_CONTEXT_TRANSACTION_BEGIN,
	_MV_CONTEXT_TRANSACTION_END,
	_MV_CONTEXT_TRIGGER,
};

static const char * ReadingContextTexts[]={
	"Interruption.Begin",
	"Interruption.End",
	"Other",
	"Sample.Clock",
	"Sample.Periodic",
	"Transaction.Begin",
	"Transaction.End",
	"Trigger"
};

////////////////////////////////////////////////////////////////////////
// Ver 7.36
enum Stopping_Transaction_Reasons{
	_ST_EMERGENCY_STOP,
	_ST_EV_DISCONNECTED,
	_ST_HARD_RESET,
	_ST_LOCAL,
	_ST_OTHER,
	_ST_POWERLOSS,
	_ST_REBOOT,
	_ST_REMOTE,
	_ST_SOFT_RESET,
	_ST_UNLOCK_COMMAND,
	_ST_DEAUTHORIZED
};

static const char* Stopping_Transaction_Reason_texts[]={
	"EmergencyStop",
	"EVDisconnected",
	"HardReset",
	"Local",
	"Other",
	"PowerLoss",
	"Reboot",
	"Remote",
	"SoftReset",
	"UnlockCommand",
	"DeAuthorized"
};

//////////////////////////////////////////////
//REGISTRATIONSTATUSVALUESANDTEXTS. 7.38
enum Registration_Status{
	_RS_NOTCONNECTED,
	_RS_ACCEPTED,
	_RS_REJECTED,
	_RS_PENDING
};

static const char* Registration_Status_texts[]={
	"NotConnected",
	"Accepted",
	"Rejected"
	"Pending",
};

////////////////////////////////////////////////////////////////////////
//CHARGEPOINT Resrevation. Ver 7.39
enum RemoteStartStopStatus{
	_RSSS_ACCEPTED,
	_RSSS_REJECTED
};

static const char* RemoteStartStopStatus_texts[]={
		"Accepted", "Rejected"
};

////////////////////////////////////////////////////////////////////////
//CHARGEPOINT Resrevation. Ver 7.40
enum ReservationStatus{
	_CP_RESERVATION_ACCEPTED,
	_CP_RESERVATION_FAULTED,
	_CP_RESERVATION_OCCUPIED,
	_CP_RESERVATION_REJECTED,
	_CP_RESERVATION_UNAVAILABLE,
};

static const char* ReservationStatus_texts[]={
		"Accepted","Faulted", "Occupied", "Rejected", "Unavailable"
};

//7.42
enum resetType{
	_RT_HARD,
	_RT_SOFT,
};

static const char* ResetType_texts[]={
		"Hard","Soft"
};

//7.44
enum TriggerMessageStatus{
	_TMS_ACCEPTED,
	_TMS_REJECTED,
	_TMS_NOTIMPLEMENTED,
};

static const char* TriggerMessageStatus_texts[]={
		"Accepted","Rejected", "NotImplemented"
};

//7.46
enum UnlockStatus{
	_US_UNLOCKED,
	_US_UNLOCKFAILED,
	_US_NOTSUPPORTED,
};

static const char* UnlockStatus_texts[]={
		"Unlocked","UnlockFailed", "NotSupported"
};

//7.47
enum UpdateStatus{
	_UPS_ACCEPTED,
	_UPS_FAILED,
	_UPS_NOTSUPPORTED,
	_UPS_VERSIONMISMATCH
};

static const char* UpdateStatus_texts[]={
		"Accepted","Failed", "NotSupported", "VersionMismatch"
};

//7.48
enum UpdateType{
	_UT_DIFFERENTIAL,
	_UT_FULL
};

static const char* UpdateType_texts[]={
		"Differential","Full"
};

////////////////////////////////////////////////////////////////////////////


#define _MV_MEASURAND_AMBIENT_TEMPERATURE "AmbientTemperature"
#define _MV_MEASURAND_OIL_TEMPERATURE "OilTemperature"

////FORMAT //From Schemas document:
#define _MV_FORMAT_RAW "Raw"
#define _MV_FORMAT_SIGNED_DATA "SignedData"

////////////////////////////////////////////////////////////////////////////
//7.45
////UNIT //From Schemas document:
enum UnitOfMeasure{
	_MV_UNIT_WH ,
	_MV_UNIT_KWH,
	_MV_UNIT_VARH ,
	_MV_UNIT_KVARH,
	_MV_UNIT_W ,
	_MV_UNIT_KW ,
	_MV_UNIT_VA ,
	//_MV_UNIT_VAH ,
	_MV_UNIT_KVA ,
	_MV_UNIT_VAR ,
	_MV_UNIT_KVAR,
	_MV_UNIT_A,
	_MV_UNIT_V,
	_MV_UNIT_CELSIUS,
	_MV_UNIT_FAHRENHEIT,
	_MV_UNIT_KELVIN,
	_MV_UNIT_HZ,
	//_MV_UNIT_C,
	_MV_UNIT_PERCENTAGE,
};

static const char* UnitOfMeasureTexts[]={
	"Wh",
	"kWh",
	"varh",
	"kvarh",
	"W",
	"kW",
	"VA",
	//"VAh",
	"kVA",
	"var",
	"kvar",
	"A",
	"V",
	"Celsius",
	"Fahrenheit",
	"K",
	"Hz",
	//"C",
	"Percent"
};

///////////////////////////////////////////////////////////////////
//7.49 - OJO, DEBE IR ANTES DEL 7.43
enum ValueFormat{
	_VF_RAW,
	_VF_SIGNEDDATA,
};

static const char* ValueFormatTexts[]={
	"Raw",
	"SignedData"
};


////////////////////////////////////////////////////////////////////////////
//7.43
struct SampledValue{
	char *value;
	enum ReadingContext context;
	enum UnitOfMeasure unit;
	enum ValueFormat formato;
	enum Measurand measurand;
	enum Location location;
	enum Phase phase;
};
//Warning: useless class storage specifier in empty declaration
//You get the message because you're not actually declaring, you're only defining something.
//You can later use this definition to declare a variable of that type. That variable may be a static or instance variable, but the
//definition doesn't need (and shouldn't have) the storage specifier attached to it.


static int enums_as_integers=1;
static int send_tags=0;
static int reservationId=1001;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                             AUXILIARY FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

char *getCurrentTime();

//Esta funcion crea un mensaje de JSON de peticion vacío con el MessageType, el UniqueId y la action que se le pasan como parámetro
//El mensaje creado no tiene payload, que se le debera añadir mas adelante
//Tiene en cuenta si la variable send_tags esta activada o no
char *prepareWrapper(int MessageType, int UniqueId, char *action);

//Esta funcion crea un mensaje JSON de respuesta vacío con el MessageType, el UniqueId que se le pasan como parámetro
//Los mensajes de respuesta no tienen action.
//El mensaje creado no tiene payload, que se le debera añadir mas adelante
//Tiene en cuenta si la variable send_tags esta activada o no
char *prepareResponseWrapper(int MessageType, const char *UniqueId);

//Igual que el anterior, pero el UniqueId que se le pasa es un entero y no una cadena.
char *prepareResponseWrapper_i(int MessageType, int UniqueId);

char *generateIdTag();

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                             REQUESTS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//4.1
//
char * prepare_authorize_request(int n, const char * idtagtext);
//
//4.2
//
char * prepare_bootNotification_request(char * vendor_data, char *chargepointmodel_data, char *chargeBoxSerialNumber_data, char *chargePointSerialNumber_data, char *firmwareVersion_data, char *ICCID_data, char *IMSI_data, char *meterSerialNumber_data, char *meterType_data);
//
//4.3
//
char * prepare_dataTransfer_request(int n, const char *vendorId, const char *messageId, const char *data);
//
// 4.4
//
char * prepare_diagnosticsStatusNotification_request(int n, int status_val);
//
// 4.5
//
char * prepare_firmwareStatusNotification_request(int n, int status_val);
//
// 4.6
//
char * prepare_heartbeat_request(int n);
//
//4.7
//
char * prepare_metervalues_request(int transactionId, int numMeterValues, int uniqueId, int connectorId, int numSamples, struct SampledValue value_list[1][numSamples]);
//
//4.8
//
char * prepare_starttransaction_request(int n, int connectorId, int meterStartValue, const char *idTag, int reservationId);
//
//4.9
//
char * prepare_statusnotification_request(int n, const int connectorId, int newStatus, int errorCode, char *extraInfo, char *vendorId, char *vendorErrorCode);
//
//4.10
//
char *prepare_stoptransaction_request(int n, int meterStopValue, int numMeterValues, int numSamples, int transactionId, int reason, char *idTag, struct SampledValue value_list[numMeterValues][numSamples]);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                          RESPONSES
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//5.1
char *prepare_cancelreservation_response(const char *UniqueId, int status);
//5.2
char *prepareChangeAvailabilityResponse(const char *uniqueId, int responseStatus);
//5.3
char *prepareChangeConfigurationResponse(const char *UniqueId_str, int responseStatus);
//5.4
char * prepareClearCacheResponse(const char * UniqueId_str, int status);
//5.6
char * prepareDataTransferResponse(const char * UniqueId_str, int status, char *data);
//5.8
char *prepare_getconfiguration_response(const char *UniqueId_str, char *response_string, char *response_error_string);
//5.9
char *prepare_getDiagnostics_response(const char *UniqueId_str, char *filename);
//5.10
char *prepare_getLocalListVersion_response(const char *UniqueId, int listVersion);
//5.11
char *prepare_remotestarttransaction_response(const char *uniqueid_str, int status);
//5.12
char *prepare_remotestoptransaction_response(const char *uniqueid_str, int status);
//5.13
char * prepare_reservenow_response(const char *UniqueId, int status);
//5.14
char * prepare_reset_response(const char * UniqueId, int status);
//5.15
char *prepare_sendlocallist_response(const char *UniqueId_str, int retorno);
//5.16
char *prepare_setchargingprofile_response(char *UniqueId_str, int status);
//5.17
char * prepare_triggermessage_response(const char * UniqueId_str, int status);
//5.18
char * prepare_unlockconnector_response(const char *UniqueId_str, int status);

//ERROR
char *prepareErrorResponse(const char *UniqueId, int error_code);

#endif /* OCPP_CLIENT_H_ */
