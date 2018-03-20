/*
 * gtk.h
 *
 *  Created on: Nov 9, 2017
 *      Author: root
 */

#ifndef OCPP_GTK_H_
#define OCPP_GTK_H_

#include <gtk/gtk.h>
/*
#include <gtk/gtkwindow.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmain.h>
#include <gtk/deprecated/gtktable.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkcomboboxtext.h>
*/
#include "ChargePoint.h"
#include "SQLiteConnection.h"


static int onUpdate=0;
static int logMutex=0;
int identifierPresented;
int numSampleValues;
int guiready;
//8 valores como maximo
struct SampledValue value_list[NUM_CONNECTORS][8];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//GtkWidget *window;
GtkWindow *window;
GtkWidget *table;
GtkBuilder      *builder, *dialogbuilder;

//labels
GtkWidget *title, *chargeboxid, *centralsystemendpoint, *chargeboxendpoint, *bootnotificationlabel, *cpvendorlabel, *cpmodellabel, *cpsnlabel, *protocolVerlabel;
GtkWidget *cbsnlabel, *FWVersionlabel, *ICCIDlabel, *IMSIlabel, *metertypelabel, *metersnlabel, *applicationloglabel, *blank, *blank2 ;
GtkWidget *statuslabel, *hbintervallabel, *currtimelabel, *ocppverlabel, *messageIdlabel, *messageDatalabel;
GtkWidget *diagnosticsstatuslabel, *firmwarestatuslabel, *connector1label, *connector2label, *connector1statuslabel, *connector1errorlabel;
GtkWidget *transactionslabel;

//Textboxes
GtkWidget *logstext, *chargeboxidtext, *centralsystemendpointtext, *chargeboxendpointtext, *cpvendortext, *cpmodeltext, *cpsntext, *cbsntext, *fwvertext, *protocolVertext;
GtkWidget *iccidtext, *imsitext, *metertypetext, *metersntext, *statustext, *currtimetext, *hbinttext, *hbinttext2, *messageIdtext, *messageDatatext;
GtkWidget *connector1text, *connector2text, *transactionidtext, *transactionidtag, *idTagtext;
GtkWidget *stoptransactionconnectorcombo, *stoptransactionconnectorcombo2;

//Solapa 4
GtkWidget *connectornotiftext[4];
GtkWidget *connectorstatuscombo[4];
GtkWidget *connectorerrorcombo[4], *connectorlabel1[4], *connectorlabel2[4], *connectorlabel3[4];

//Solapa 7
GtkWidget *valoresguardados;
GtkWidget *connectorlabel[4];
GtkWidget *connectortext[4];
GtkWidget *measurandcombo[4];
GtkWidget *unitcombo[4];
GtkWidget *locationcombo[4];
GtkWidget *contextcombo[4];
GtkWidget *phasecombo[4];

GtkWidget *logsbuffer, *startIdTagText, *startReservationText, *startConnectorText, *connector1, *connector2, *connector3, *connector4, *connector5;
GtkWidget *stopIdTagText, *stopReservationText, *stopConnectorText, *stopConnectorIdText, *stopTransactionText;
GtkWidget *stopTransactionTab, *startTransactionTab;

//ComboBox
GtkWidget *diagnosticsstatuscombo, *firmwarestatuscombo, *connector1combo, *connector1statuscombo, *connector1errorcombo, *connector2statuscombo, *connector2errorcombo, *connector3statuscombo, *connector3errorcombo, *connectorId;
GtkWidget *connector1unit,*connector2unit,*connector3unit;
GtkWidget *connector1location,*connector2location,*connector3location,*connector1measurand,*connector2measurand,*connector3measurand,*connector3phase,*connector2phase,*connector1phase,*connector1context,*connector2context,*connector3context;
GtkWidget *stopReasonCombo;

GtkWidget *connector4statuscombo, *connector4errorcombo, *connector5statuscombo, *connector5errorcombo;
GtkWidget *connector4unit,*connector5unit;

GtkWidget *halign;
GtkWidget *halign2;
GtkWidget *valign;

//Buttons
GtkWidget *authorizeBtn, *dataTransferBtn, *firmwareStatusNotificationBtn, *heartbeatBtn, *diagnosticsStatusNotificationBtn, *bootNotificationBtn;
GtkWidget *meterValuesBtn, *startTransactionBtn, *stopTransactionBtn, *statusNotificationBtn, *rndIdTagBtn, *statusnotificationbtn;
GtkWidget *stoptransactionbtn1, *stoptransactionbtn2, *savevaluesbtn;
GtkWidget *getReservationIdBtn, *getRandomIdTagBtn, *notebook;
GtkTextView *LogTextView2;

//DIALOGOS
GtkWidget *OKbutton;
GtkDialog *dialog_window;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
//                        Auxiliary functions
///////////////////////////////////////////////////////////////////////////

//Escribe un IdTag aleatorio en un campo que se le pasa como parametro widget.
void getRandomIdTagGUI( GtkWidget *widget,gpointer data );

void printLogToGUI(char *message, int warning_level);
///////////////////////////////////////////////////////////////
//            TOP LEVEL FUNCTIONS
///////////////////////////////////////////////////////////////

//Función llamada por el HMI/Dispositivo cuando el usuario presenta su RFID durante una transacción. Lo que hace es llamar a StopTransaction
void presentIdentifierToDisconnectEV(GtkWidget *widget, gpointer data);

//Los conectores tienen lock de manera que no se puede desconectar un coche electrico si no 'desbloquea' el conector antes. Esta funcion es llamada
//por el dispositivo cuando se intenta desbloquear un conector. Si la clave de configuración UnlockConnectorOnEVSideDisconnect esta activada es
//necesario que el usuario presente el RFID card antes. Devuelve 1 si permite el desbloqueo o 0 si no lo permite.
int  allowUnlockCable(int connectorId);

//Esta funcion Cambia el estado del conector, comprueba una clave de configuración si es válida desbloquea el conector y envia un mensaje de
//stopTransaction al central system.
void disconnectEV(int connectorId);


//Igual que el anterior para inicio de transacciones.
void startTransactionInGUI(int connector, const char *idTag);

////////////////////////////////////////////////////////////////
//                 GUI
////////////////////////////////////////////////////////////////
//getters()
//This function returns the string given a visual widget. Used to read user values from textboxes
const gchar *getGUIData(GtkWidget* textbox);

//This function obtains meter value of a specific connector. Returns an integer (should be a double)
int getMeterValue(int conn);
//This function obtains the meter unit of a specific connector, read from the GUI.
int getConnectorMeterUnit(int connector);
//This function obtains the meter measurand of a specific connector, read from the GUI
int getConnectorMeterMeasurand(int connector);
//This function obtains the meter location of a specific connector, read from the GUI
int getConnectorMeterLocation(int connector);
//This function obtains the meter Phase of a specific connector, read from the GUI
int getConnectorMeterPhase(int connector);
//This function obtains the meter format of a specific connector, read from the GUI
int getConnectorMeterFormat(int connector);
//This function obtains the meter context of a specific connector, read from the GUI
int getConnectorMeterContext(int connector);
//This functions checks the error state of a specific connector
//conn debe ser la posicion del array ya (0..n-1)
//returns 1 on error and 0 on NO ERROR
int checkErrorStateOfConnector(int conn);
//This function turns down the GUI
void destroy( GtkWidget *widget,gpointer   data );
//This function draws the GUI. Is the main function.
int drawGUI();
//This function sets data to the GUI widgets (from configuration file, etc)
void setGUIData();

void updateStopTransactionConnectorCombo();
void updateStopTransactionInGUI();
////////////////////////////////////////////////////////////////
//         OCPP request functions
////////////////////////////////////////////////////////////////

//4.1
void sendauthorize( GtkWidget *widget, gpointer   data );
//4.2
void sendBootNotification( GtkWidget *widget, gpointer   data );
//4.3
void senddataTransfer( GtkWidget *widget, gpointer   data );
//4.4
void senddiagnosticsStatusNotification( GtkWidget *widget, gpointer   data );
//4.5
void sendfirmwareStatusNotification( GtkWidget *widget, gpointer   data );
//4.6
void sendheartbeat( GtkWidget *widget, gpointer   data );
//4.7
void sendmeterValues( GtkWidget *widget, gpointer   data );
//4.8
void sendstartTransaction( GtkWidget *widget, gpointer   data );
//4.9
void sendstatusNotification( GtkWidget *widget, gpointer   data );
//4.10
void sendstopTransaction( GtkWidget *widget, gpointer   data );



#endif /* OCPP_GTK_H_ */
