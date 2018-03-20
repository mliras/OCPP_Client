
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

//#include "client.h"
//#include "ocpp_client.h"
#include "ocpp_gtk.h"
#include <glib-2.0/glib.h>

typedef char   gchar;

//#define FALSE 0
//#define TRUE 1

void updateValue(GtkWidget *widget, gpointer data)
{
	if (!onUpdate)
	{
		const char *activo=gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
		int connector=-1;
		if (activo)	connector=atoi(activo);

		if (connector>0)
		{
			gtk_entry_set_text(GTK_ENTRY(data), convertF(connectorValues[connector-1]));
		}
		else
		{
			gtk_entry_set_text(GTK_ENTRY(data), "UNKNOWN");
		}

		gtk_entry_set_text(GTK_ENTRY(stopTransactionText),convert(transactions[connector-1]->transactionId));
	}
}

//Esta funcion es la que ejecuta cada thread que simula la carga en un conector.
//connector es un valor de 0 a n-1
static void *cargaConector(void *connector)
{
	//Este valor simula el tiempo que tarda en ir creciendo la carga. Es COMPLETAMENTE inventado
	int tiempo=223000;

	int conn=(int)connector;
	//printf("\nY LLAMAMOS A CRGA CONECTOR CON %d", conn);

	if (conn<0) return 0;

	while (1)
	{
	//	printf("I:%d", conn);
		if (connectorStatus[conn]==_CP_STATUS_CHARGING)
		{
	//		printf("C:%d", conn);
			connectorValues[conn]+=0.01;
				//	gtk_entry_set_text(GTK_ENTRY(connectortext[conn]),convertF(connectorValues[conn]));
		}
		usleep(tiempo);
	}
}

void show_info(char *texto) {

  GtkWidget *dialog;
  dialog = gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,texto);
  gtk_window_set_title(GTK_WINDOW(dialog), "Information");
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void CloseDialog (GtkWidget *widget, gpointer data)
{
    /* --- Remove grab --- */
    gtk_grab_remove (GTK_WIDGET (data));

    /* --- Close it. --- */
    gtk_widget_destroy (GTK_WIDGET (data));
}

void updateStartTransactionTab( GtkWidget *widget,gpointer   data )
{
	int connector=gtk_combo_box_get_active(connectorId);
	const gchar *value;

	if (connector==0){
		value=gtk_entry_get_text(GTK_ENTRY (connector1));
	} else if (connector==1){
		value=gtk_entry_get_text(GTK_ENTRY (connector2));
	} else if (connector==2){
		value=gtk_entry_get_text(GTK_ENTRY (connector3));
	}

		gtk_entry_set_text (startConnectorText, value);
}

///////////////////////////////////////////////
// GUI
///////////////////////////////////////////////



//Esta funcion pone en el combo de parada los conectores que estan bajo una transaccion
void updateStopTransactionConnectorCombo()
{
	showTransactions();
	onUpdate=1;
	int numTransactions=0;
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX(stoptransactionconnectorcombo));
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		if (transactions[i])
		{
			gtk_combo_box_text_append_text(GTK_COMBO_BOX(stoptransactionconnectorcombo), convert(transactions[i]->connector));
			numTransactions++;
		}
	}

	if (numTransactions==0)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(stoptransactionconnectorcombo), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(stoptransactionconnectorcombo), TRUE);
	}

	//Esto es para evitar que se modifiquen los cambios e stopTransaction cuando no se debe
	onUpdate=0;
}


//Esta funcion es llamada cuando se modifica el valor del combobox del connector Id.
//Actualiza el valor actual del campo "connector value".
void updateConnectorValue( GtkWidget *widget,gpointer data )
{
	gtk_widget_set_sensitive(startTransactionBtn, TRUE);
	//Primero leemos el idtag. Si no lo hemos escrito, nos da un mensaje y sale
	char *idtag=gtk_entry_get_text(GTK_ENTRY(startIdTagText));
	if (!idtag || strlen(idtag)<3)
	{
		show_info("First introduce an idtag value");
		gtk_combo_box_set_active(GTK_COMBO_BOX_TEXT(connectorId),-1);
		gtk_widget_set_sensitive(startTransactionBtn, FALSE);
		return;
	}

	//Luego leemos el conector seleccionado
	int connector=gtk_combo_box_get_active(GTK_COMBO_BOX(connectorId));

	if (connector>=0)
	{
			gtk_entry_set_text (GTK_ENTRY(data),convertF(connectorValues[connector]));

		if (allowReservations[connector])
		{
			if (connectorReservations[connector])
			{
				if (strcmp(connectorReservations[connector]->idTag,idtag)==0)
				{
									gtk_entry_set_text (GTK_ENTRY(startReservationText),convert(connectorReservations[connector]->reservationId));
				}
				else
				{
					char *text=(char *)calloc(1, 180);
					sprintf(text, "--RESERVATION %d FOR IDTAG: %s--", connectorReservations[connector]->reservationId, connectorReservations[connector]->idTag);
					gtk_entry_set_text (GTK_ENTRY(startReservationText),text);
					free(text);
					gtk_widget_set_sensitive(startTransactionBtn, FALSE);
				}
			}
			else
			{
						gtk_entry_set_text (GTK_ENTRY(startReservationText),"--CONNECTOR NOT RESERVED--");
			}
		}
		else
		{
			gtk_entry_set_text (GTK_ENTRY(startReservationText),"--RESERVATIONS NOT ALLOWED--");
		}
	}
}

void getRandomIdTagGUI( GtkWidget *widget,gpointer   data )
{
	char *idTag=generateIdTag();

		if (idTag) gtk_entry_set_text (GTK_ENTRY(data), idTag);
}

void destroy( GtkWidget *widget,gpointer   data )
{
  gtk_main_quit  ();
}

///////////////////////////////////////////////////////////////
//            TOP LEVEL
///////////////////////////////////////////////////////////////

//Ver Pag 41
void insertPlug(int conn)
{
	if (currentChargePointState==_CP_STATUS_AVAILABLE)
	{
		currentChargePointState=_CP_STATUS_PREPARING;
	}

	if (connectorStatus[conn]==_CP_STATUS_AVAILABLE)
	{
		connectorStatus[conn]=_CP_STATUS_PREPARING;
	}
}

//Pag 46
void presentIdentifier(int connectorId)
{
	if (connectorStatus[connectorId]==_CP_STATUS_AVAILABLE)
	{
		connectorStatus[connectorId]=_CP_STATUS_PREPARING;
	}

	if (connectorStatus[connectorId]==_CP_STATUS_PREPARING)
	{
		connectorStatus[connectorId]=_CP_STATUS_AVAILABLE;
	}

	if (connectorStatus[connectorId]==_CP_STATUS_CHARGING)
	{
		connectorStatus[connectorId]=_CP_STATUS_AVAILABLE;
		identifierPresented=1;
		sendstopTransaction(NULL,NULL);
	}
}

void presentIdentifierToDisconnectEV(GtkWidget *widget, gpointer data)
{
	//show_info(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(stoptransactionconnectorcombo)));

	//connector es un valor de 1 a N
	int connector=atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(stoptransactionconnectorcombo)));

	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		if (transactions[i] && transactions[i]->connector==connector)
		{
				gtk_entry_set_text(GTK_ENTRY(stopIdTagText), transactions[i]->idTag);
		}
	}

}

//Devuelve 1 si permite el desbloqueo o 0 si no lo permite.
int  allowUnlockCable(int connectorId)
{
	//Pag 46: With	UnlockConnectorOnEVSideDisconnect set to false, the Connector SHALL remain locked at the Charge Point until the user presents the identifier.

	if (getConfigurationKeyIntValue("UnlockConnectorOnEVSideDisconnect"))
	{
		if (!identifierPresented)
		{
			return 0;
		}
		else
		{
				//Desbloqueamos fisicamente el cable

				identifierPresented=0;
				//if (!authorize(idtag);) return 0

				return 1;
		}
	}

	return 1;
}


//Procedimiento de desconexión ordenada de un coche electrico.
void disconnectEV(int connector2)
{
	//Pag 46: As part of the normal transaction termination, the Charge Point SHALL unlock the cable (if not permanently attached).
	//The Charge Point MAY unlock the cable (if not permanently attached) when the cable is disconnected
	//at  the  EV.  If  supported,  this  functionality  is  reported  and  controlled  by  the  configuration key "UnlockConnectorOnEVSideDisconnect".

	//Pag 46: The  Charge  Point  MAY  stop  a  running  transaction  when  the  cable  is  disconnected  at  the  EV.  If
	//supported,	this	functionality	is	reported	and	controlled	by	the	configuration	key StopTransactionOnEVSideDisconnect	.

	//Pag 47: If	StopTransactionOnEVSideDisconnect	  is  set  to	false,  this  SHALL  have  priority  over UnlockConnectorOnEVSideDisconnect	.
	//In other words: cables always remain locked when the cable is disconnected at EV side when StopTransactionOnEVSideDisconnect  is	false.

for (int connector=0; connector<NUM_CONNECTORS; connector++)
{
	if (connectorStatus[connector]==_CP_STATUS_PREPARING)
	{
		changeConnectorStatus(connector,_CP_STATUS_AVAILABLE);
	}
	else if (connectorStatus[connector]==_CP_STATUS_FINISHING)
	{
		if (!isConnectorPermanentlyAttached())
		{
			if (allowUnlockCable(connector))
			{
				if (!unlockCable(connector))
				{

				}
			}
		}

		changeConnectorStatus(connector,_CP_STATUS_AVAILABLE);
	}
	else if (connectorStatus[connector]==_CP_STATUS_CHARGING)
	{
		if (getConfigurationKeyIntValue("StopTransactionOnEVSideDisconnect"))
		{
			if (!isConnectorPermanentlyAttached())
			{
				if (allowUnlockCable(connector))
				{
					if (!unlockCable(connector))
					{

					}
				}
			}
			//NO QUEDA CLARO SI SE DEBE ENVIAR EL MENSAJE AL CENTRAL SYSTEM SI NO SE PUDO DESBLOQUEAR EL CABLE... LO DEJO COMO QUE SI.

			//luego desde esta funcion se llama a la peticion de red y a la parada fisica de la transaccion
			sendstopTransaction(NULL, NULL);
		}
		changeConnectorStatus(connector,_CP_STATUS_AVAILABLE);
	}
}
}



//updates GUI. Actualiza el combo con los conectores que estan bajo transaccion
void startTransactionInGUI(int connector, const char *idTag)
{
	updateStopTransactionConnectorCombo();
}


//Ver Pag 41
void bayOccupancyChangeDetected(int connector)
{
	if (connector>0 && connector<num_connectors)
	{
		 if (connectorStatus[connector]==_CP_STATUS_AVAILABLE)
		 {
			changeConnectorStatus(connector, _CP_STATUS_PREPARING);
		 }
		 else if (connectorStatus[connector]==_CP_STATUS_PREPARING)
		 {
			changeConnectorStatus(connector, _CP_STATUS_AVAILABLE);
		 }
	}
}

//Ver Pag 41
void pushStartButton(int connector)
{
	if (connector>0 && connector<num_connectors)
	{

		if (connectorStatus[connector]==_CP_STATUS_AVAILABLE)
		{
			changeConnectorStatus(connector, _CP_STATUS_PREPARING);
			//send_starttransaction_request(connector, int meterStartValue, char *idTag);  <-- AUN NO
		}
	}
}

//Ver Pag 41
void faultDetected(int connector)
{
	changeConnectorStatus(connector, _CP_STATUS_FAULTED);
}

///////////////////////////////////////////////////
//
// 4.1 AUTHORIZE
//
void sendauthorize( GtkWidget *widget, gpointer   data )
{
	const char *idTag=getGUIData(idTagtext);

	if (strlen(idTag)<3)
	{
		show_info("Please a correct value for the IdTag");
		return;
	}

	send_authorize_request(idTag);

		gtk_entry_set_text(GTK_ENTRY(idTagtext), "");
}

//
// 4.2 BOOT NOTIFICATION
//
void sendBootNotification( GtkWidget *widget, gpointer   data )
{
	send_bootNotification_request(getGUIData(cpvendortext), getGUIData(cpmodeltext), getGUIData(cbsntext), getGUIData(cpsntext), getGUIData(fwvertext), getGUIData(iccidtext), getGUIData(imsitext), getGUIData(metersntext), getGUIData(metertypetext));
}

//
// 4.3. DATA TRANSFER
//
void senddataTransfer( GtkWidget *widget, gpointer   data )
{
	const char *cp=getGUIData(cpvendortext);
	const char *mid=getGUIData(messageIdtext);
	const char *cb=getGUIData(messageDatatext);

	//Pag 36: The length of data in both the request and response PDU is undefined and should be agreed upon by all
	//parties involved. Lo voy a obtener del parametro de configuracion "MaxDataTransferBytes"

	int maxdata=getConfigurationKeyIntValue("MaxDataTransferBytes");

	if (maxdata>0)
	{
		if (cb)
		{
			if (strlen(cb)>maxdata) return ;
		}
	}

	send_dataTransfer_request(cp, mid, cb);
}

//
// 4.4. DIAGNOSTICS STATUS NOTIFICATION
//
void senddiagnosticsStatusNotification( GtkWidget *widget, gpointer   data )
{
		int value=gtk_combo_box_get_active(diagnosticsstatuscombo);

		if (value>=0)
		{
			send_diagnosticsstatus_request(value);
		}
		else
		{
			gtk_widget_show(window);
			//LA IDEA AQUI ES MOSTRAR UN CUADRO DE DIALOGO
			/*
			GtkWidget *dialog;
			GtkWindow *miwindow = data;
			GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
			*/
	//		dialog = gtk_dialog_new_with_buttons ("Error",miwindow,flags,GTK_STOCK_OK,NULL);
		//	gtk_widget_show_all (dialog);
			//sleep(3);
			//gtk_widget_destroy (GTK_WIDGET (dialog));
		}
}

//
// 4.5
//
void sendfirmwareStatusNotification( GtkWidget *widget, gpointer   data )
{
	int value=gtk_combo_box_get_active(firmwarestatuscombo);
	if (value>=0)
	{
			send_firmwareStatusNotification_request(value);
	}
	else
	{
			printf("\n[LOG] Intento de envio de estado de notificacion");
	}
}

//
// 4.6 HEARTBEAT
//
void sendheartbeat( GtkWidget *widget, gpointer   data )
{
			queue_node *nuevo_mensaje;
			int n=getNextUniqueID();
			nuevo_mensaje = (queue_node *)calloc(1, sizeof(queue_node));
			nuevo_mensaje->MessageAction=DIAGNOSTICS_STATUS_NOTIFICATION;
			nuevo_mensaje->UniqueId=n;

			nuevo_mensaje->payload=prepare_heartbeat_request(n);
			Enqueue(nuevo_mensaje);
}
/*
//
// 4.7. METER VALUES
//
void sendmeterValues(GtkWidget *widget, gpointer data )
{
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		value_list[i][numSampleValues].value=gtk_entry_get_text(GTK_ENTRY(connectortext[i]));
		value_list[i][numSampleValues].context=gtk_combo_box_get_active(GTK_COMBO_BOX(contextcombo[i]));
		value_list[i][numSampleValues].measurand=gtk_combo_box_get_active(GTK_COMBO_BOX(measurandcombo[i]));
		value_list[i][numSampleValues].location=gtk_combo_box_get_active(GTK_COMBO_BOX(locationcombo[i]));
		value_list[i][numSampleValues].unit=gtk_combo_box_get_active(GTK_COMBO_BOX(unitcombo[i]));
		value_list[i][numSampleValues].phase=gtk_combo_box_get_active(GTK_COMBO_BOX(phasecombo[i]));
		value_list[i][numSampleValues].formato=-1;
	}

	numSampleValues++;

	int transactionId=getTransactionId(connector);
	send_metervalues_request(transactionId, connector, numConnectors, numSampleValues, value_list);
	send_metervalues_request(NUM_CONNECTORS, numSampleValues, value_list);
	numSampleValues=0;
}
*/




//
// 4.8.- Esta funcion es llamada cuando se pulsa el boton "Start Transaction" de la GUI
//
void sendstartTransaction( GtkWidget *widget, gpointer   data )
{
	//Obtenemos los valores de los distintos campos del formulario
	const char *idTag=getGUIData(startIdTagText);

	//reservationId. Si no es un numero, es que o bien no se puede reservar dicho o bien ya esta reservado.
	const char *resId=getGUIData(startReservationText);
	int reservationId;

	if (!resId || !checkIsNumber(resId)) reservationId=-1;
	else reservationId=atoi(resId);

	//Comprobamos campos válidos
	if (strlen(idTag)==0)
	{
			show_info("Fill all fields before sending request");
			return;
	}
/*
	if (strlen(resId)==0){
		show_info("Fill all fields before sending request");
		  return;
	}
*/
	//conn sera la posicion del array (de 0 a N-1)
	int conn=-1;
	conn=gtk_combo_box_get_active (GTK_COMBO_BOX(connectorId));

	if (conn<0){
		show_info("Fill all fields before sending request");
		  return;
	}

	if (checkErrorStateOfConnector(conn))
	{
		show_info("The connector is on an error state. Cannot start transaction with this connector");
		return;
	}

	printf("\n\nCONNECTOR STATUS ES: %d", connectorStatus[conn]);
	if (connectorStatus[conn]==_CP_STATUS_AVAILABLE || connectorStatus[conn]==_CP_STATUS_RESERVED)
	{
		if (allowReservations[conn])
		{
			if (connectorReservations[conn])
			{
				if (strcmp(connectorReservations[conn]->idTag, idTag)!=0)
				{
					char *text=(char *)calloc(1,80);
					sprintf(text,"The connector %d is reserved for IdTag %s. Start Transaction cancelled.", conn, idTag);
					free(text);
					return;
				}
			}
		}

		changeConnectorStatus(conn, _CP_STATUS_PREPARING);

		//Iniciamos fisicamente la transaccion
		startTransaction(conn, idTag);  //<--NOT IMPLEMENTED. En la pag 39 dice que a continuacion se informara de una transaccion que ya ha comenzado.

		if (debug) showConnectorStatuses();

		//Enviamos el mensajes:
		send_starttransaction_request(conn+1,atoi(getGUIData(startConnectorText)), idTag, reservationId);

		//changeConnectorStatus(conn, _CP_STATUS_CHARGING);
		gtk_combo_box_set_active (GTK_COMBO_BOX(connectorstatuscombo[conn-1]),_CP_STATUS_CHARGING);
		//updates GUI. Actualiza el combo con los conectores que estan bajo transaccion
		updateStopTransactionConnectorCombo();
	}
	else
	{
		show_info("The connector is not currently available (Under transaction?)");
	}
	gtk_entry_set_text(GTK_ENTRY(startReservationText), "");
	gtk_entry_set_text(GTK_ENTRY(startConnectorText), "");
	gtk_combo_box_set_active(GTK_COMBO_BOX(connectorId), -1);
	gtk_entry_set_text(GTK_ENTRY(startIdTagText), "");

	//Ponemos los valores en los campos de parada:
	//gtk_entry_set_text(GTK_ENTRY(stopReservationText), convert(reservationId));
	//gtk_entry_set_text(GTK_ENTRY(stopConnectorText), getGUIData(startConnectorText));
	//gtk_entry_set_text(GTK_ENTRY(stopConnectorIdText), gtk_combo_box_text_get_active_text(connectorId));
	//gtk_entry_set_text(GTK_ENTRY(stopIdTagText), idTag);

	//Activamos uno y desactivamos el otro

	//gtk_widget_set_sensitive (startTransactionTab , FALSE);

}

//
// 4.9
// A este método se le puede llamar porque se haya producido un cambio de estado (en el combobox), en cuyo caso widget es el combobox
// o porque se haya producido un error, en cuyo caso widget es 0
void sendstatusNotification( GtkWidget *widget, gpointer   data )
{
	char *internalname=	gtk_widget_get_name (widget);

	if ((strcmp(internalname, "connector1statuscombo")==0)||(strcmp(internalname, "connector1errorcombo")==0)||(strcmp(internalname, "statusnotificationbtn")==0))
	{
		send_statusnotification_request(1, gtk_combo_box_get_active (GTK_COMBO_BOX(connector1statuscombo)), gtk_combo_box_get_active (GTK_COMBO_BOX(connector1errorcombo)),NULL, NULL,NULL);
	}

	if ((strcmp(internalname, "connector2statuscombo")==0)||(strcmp(internalname, "connector2errorcombo")==0)||(strcmp(internalname, "statusnotificationbtn")==0))
	{
			send_statusnotification_request(2, gtk_combo_box_get_active (GTK_COMBO_BOX(connector2statuscombo)), gtk_combo_box_get_active (GTK_COMBO_BOX(connector2errorcombo)),NULL, NULL,NULL);
	}

	if ((strcmp(internalname, "connector3statuscombo")==0)||(strcmp(internalname, "connector3errorcombo")==0)||(strcmp(internalname, "statusnotificationbtn")==0))
	{
			send_statusnotification_request(3, gtk_combo_box_get_active (GTK_COMBO_BOX(connector3statuscombo)), gtk_combo_box_get_active (GTK_COMBO_BOX(connector3errorcombo)),NULL, NULL,NULL);
	}

}

void updateStopTransactionInGUI()
{
	numSampleValues=0;
	//Limpiamos los campos
	gtk_entry_set_text (GTK_ENTRY(stopConnectorText), "");
	gtk_entry_set_text (GTK_ENTRY(stopTransactionText), "");
	gtk_entry_set_text (GTK_ENTRY(stopIdTagText), "");
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX(stoptransactionconnectorcombo));
	gtk_combo_box_set_active(GTK_COMBO_BOX(stopReasonCombo), -1);
}

/////////////////////////////////////
//
//4.10 STOP TRANSACTION
//
void sendstopTransaction( GtkWidget *widget, gpointer data)
{
	char *idtag=gtk_entry_get_text(GTK_ENTRY(stopIdTagText));

	if (!idtag)
	{
		show_info("Introduce the IdTag or rpesent the identifier");
		return;
	}

	if (strlen(idtag)<2)
	{
		show_info("Invalid IdTag");
		return;
	}


	//reason
	int reason=gtk_combo_box_get_active (GTK_COMBO_BOX(stopReasonCombo));
		//Pag46: If  a  transaction  is  ended  in  a  normal  way  (e.g.  EV-driver  presented  his  identification  to  stop  the
		//transaction),  the 	Reason	  element  MAY  be  omitted  and  the	Reason	  SHOULD  be  assumed  'Local'.  If  the	transaction is not ended normally, the	Reason
		// SHOULD be set to a correct value.
	if (reason<0) reason=_ST_LOCAL;


	//conn es un valor de 1 a N
	int conn=atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(stoptransactionconnectorcombo)));


	//transactionId;
	int currentTransaction;
	const char *tId=getGUIData(stopTransactionText);
	if (tId)
	{
		currentTransaction=atoi(tId);

		//Debe haber una transaction en curso, en caso contrario, no hacemos nada.
		if (!currentTransaction){
			show_info("There should be a transaction in course");
			return;
		}

		if (!transactions[conn-1])
		{
			show_info("No transaction found in the transactions data strcuture");
			return;
		}

		if (transactions[conn-1]->connector!=conn)
		{
			show_info("Invalid data found in the transactions data strcuture");
			return;
		}
	}

	if (strcmp(transactions[conn-1]->idTag, idtag)!=0)
	{
		show_info("Invalid idTag. Transaction was started by other idtag");
		return;
	}

	//Paramos el connector
	gtk_combo_box_set_active (GTK_COMBO_BOX(connectorstatuscombo[conn-1]),_CP_STATUS_AVAILABLE);
	changeConnectorStatus(conn-1, _CP_STATUS_AVAILABLE);

	stopTransaction(conn); //<-- Paramos fisicamente la transaccion. NOT IMPLEMENTED
	//currentIdTag=NULL;

	send_stoptransaction_request(connectorValues[conn-1],currentTransaction, reason, idtag);
//	gtk_widget_set_sensitive (stopTransactionTab , FALSE);

	updateStopTransactionInGUI();
	//Activamos la solapa de inicio de transaccion
	//gtk_widget_set_sensitive (startTransactionTab , TRUE);
}

void addButton(GtkWidget* button, GtkWidget *tabla, int row, int column, GdkColor *color )
{
	gtk_widget_set_size_request(button, 50, 30);
	gtk_table_attach(GTK_TABLE(tabla), GTK_BUTTON(button), row, row+1, column, column+1, GTK_FILL, GTK_SHRINK, 8,8);
	if (color) gtk_widget_modify_fg (button , GTK_STATE_NORMAL, color);
}

void printLogToGUI(char *message, int warning_level)
{
	//    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (LogTextView2));



	   // text = gtk_text_view_get_text (GTK_TEXT_VIEW (LogTextView2));
	  //  char *text=strdup(gtk_entry_get_text(GTK_TEXT_VIEW (LogTextView2)));
	//    char *text=gtk_text_buffer_get_text ()
	/*	char *newtext=calloc(1, strlen(text)+strlen(texto)+10);
		strcpy(newtext, text);
		strcat(newtext, "\n");
		strcat(newtext, mensaje);
*/
	/*     mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER(buffer));
	     gtk_text_buffer_get_iter_at_mark (buffer, &iter, GTK_TEXT_MARK(mark));
	     gtk_text_buffer_insert (buffer, &iter, texto, -1);

		    gtk_text_buffer_get_start_iter (buffer, &start);
		    gtk_text_buffer_get_end_iter (buffer, &end);
		    char *text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	     printf("TEXT ES: %s", text);
	     /* Insert newline (only if there's already text in the buffer).
	     if (gtk_text_buffer_get_char_count(buffer))
	     gtk_text_buffer_insert (buffer, &iter, "\n", 1);
	     gtk_text_view_set_buffer(GTK_TEXT_VIEW (LogTextView2), buffer);
*/
		//gtk_entry_set_text(GTK_TEXT_VIEW (LogTextView2),newtext);
//		free(text);

}

const gchar *getGUIData(GtkWidget* textbox)
{
	const gchar *text;
	text=gtk_entry_get_text(GTK_ENTRY (textbox));
	return text;
}


char *getGUIData2(GtkWidget* textbox)
{
	char *text=(char *)calloc(1, sizeof(char)*256);
/*	GtkTextBuffer *buffer;
	GtkTextIter start;
	GtkTextIter end;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textbox));
	gtk_text_buffer_get_start_iter (buffer, &start);

	gtk_text_buffer_get_end_iter (buffer, &end);
	text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
*/
	return text;
}
//
// Este método lee los valores del fichero .INI y los coloca en
void setGUIData()
{
	char *temp=getConfigurationKeyStringValue("ChargeBoxID");
  	if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(chargeboxidtext), temp);

  	temp=getConfigurationKeyStringValue("CentralSystemIP");
	if (temp != NULL)
 	{
		//buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (centralsystemendpointtext));
	 	char *Endpoint=(char *)calloc(1, sizeof(char) * (strlen(temp)+strlen(getConfigurationKeyStringValue("CentralSystemPort"))+strlen(getConfigurationKeyStringValue("CentralSystemURL"))+9-2));

	 	strcpy(Endpoint, "http://");
	 	strncat(Endpoint, temp, strlen(temp));

	 	//CENTRAL SYSTEM PORT
	 	temp=getConfigurationKeyStringValue("CentralSystemPort");
	 	if(temp)
	 	{
	 		strcat(Endpoint, ":");
	 	 	strncat(Endpoint, temp , strlen(temp));
	 	}

	 	//CENTRAL SYSTEM URL
	 	temp=getConfigurationKeyStringValue("CentralSystemURL");
	 	if(temp)
	 	{
	 		strcat(Endpoint, "/");
	 	 	strncat(Endpoint, temp, strlen(temp));
	 	}


	 	gtk_entry_set_text (GTK_ENTRY(centralsystemendpointtext), Endpoint);
	 	if (Endpoint) free(Endpoint);
 	}

	temp=getConfigurationKeyStringValue("ChargeBoxIP");
	if (temp != NULL)
	{
 		char *Endpoint=(char *)calloc(1, sizeof(char) * (strlen(temp)+strlen(getConfigurationKeyStringValue("ChargeBoxPort"))+strlen(getConfigurationKeyStringValue("ChargeBoxURL"))+9-2));

 		strcpy(Endpoint, "http://");
	 	strncat(Endpoint, temp, strlen(temp));

		temp=getConfigurationKeyStringValue("ChargeBoxPort");
	 	if(temp)
	 	{
	 		strcat(Endpoint, ":");
	 		strncat(Endpoint, temp, strlen(temp));
	 	}

	 	temp=getConfigurationKeyStringValue("ChargeBoxURL");
	 	if(temp)
	 	{
	 		strncat(Endpoint, "/" , 1);
	 		strncat(Endpoint, temp, strlen(temp));
	 	}

	 	gtk_entry_set_text (GTK_ENTRY(chargeboxendpointtext), Endpoint);
	 	if (Endpoint) free(Endpoint);
	  }

	//PROTOCOL VERSION
	  temp=getConfigurationKeyStringValue("ProtocolVersion");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(protocolVertext), temp);

	//VENDOR
	  temp=getConfigurationKeyStringValue("ChargePointVendor");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(cpvendortext), temp);

	  //MODEL
	  temp=getConfigurationKeyStringValue("ChargePointModel");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(cpmodeltext), temp);

	  //CP SN
	  temp=getConfigurationKeyStringValue("ChargePointSerialNumber");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(cpsntext), temp);

	  //FW Version
	  temp=getConfigurationKeyStringValue("FirmwareVersion");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(fwvertext), temp);

	  //CB SN
	  temp=getConfigurationKeyStringValue("ChargeBoxSerialNumber");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(cbsntext), temp);

	  //ICCID
	  temp=getConfigurationKeyStringValue("ChargePointICCID");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(iccidtext), temp);

	  //IMSI
	  temp=getConfigurationKeyStringValue("ChargePointIMSI");
	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(imsitext), temp);

	  //Meter Type
  	  temp=getConfigurationKeyStringValue("ChargePointMeterType");
  	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(metertypetext), temp);

	  //Meter SN
  	  temp=getConfigurationKeyStringValue("ChargePointMeterSerialNumber");
  	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(metersntext), temp);

	  //Heartbeat
  	  temp=getConfigurationKeyStringValue("HeartbeatInterval");
  	  if (temp != NULL) gtk_entry_set_text (GTK_ENTRY(hbinttext), temp);
	  //{
		  	  	  //char *b=(char *)calloc(1, sizeof(char)*8); //MAX HEARTBEAT=99.999.999ms
		  	  	  //sprintf(b,"%d",temp);
	 	  	  	  //buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (hbinttext));
	 	//  	  	  gtk_entry_set_text (hbinttext, temp);
	 	  	  	  //if (b) free(b);
	  //}

}

void initialize()
{
	identifierPresented=0;
	numSampleValues=0;
}

int drawGUI(){

	initialize();


	    gtk_init(0, NULL);

	    builder = gtk_builder_new();
	    gtk_builder_add_from_file (builder, "/etc/ocpp/client.glade", NULL);

	    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
	    gtk_builder_connect_signals(builder, NULL);

	    //SOLAPA 1
	    logstext = GTK_WIDGET(gtk_builder_get_object(builder, "logstext"));
	    chargeboxidtext = GTK_WIDGET(gtk_builder_get_object(builder, "chargeboxidtext"));
	    centralsystemendpointtext = GTK_WIDGET(gtk_builder_get_object(builder, "centralystemendpointtext"));
	    chargeboxendpointtext = GTK_WIDGET(gtk_builder_get_object(builder, "chargeboxendpointtext"));
	    cpvendortext = GTK_WIDGET(gtk_builder_get_object(builder, "cpvendortext"));
	    cpmodeltext = GTK_WIDGET(gtk_builder_get_object(builder, "cpmodeltext"));
	    cpsntext = GTK_WIDGET(gtk_builder_get_object(builder, "cpserialtext"));
	    cbsntext = GTK_WIDGET(gtk_builder_get_object(builder, "chargeboxserialtext"));
	    fwvertext = GTK_WIDGET(gtk_builder_get_object(builder, "fwversiontext"));
	    protocolVertext = GTK_WIDGET(gtk_builder_get_object(builder, "protocolversiontext"));
	    metertypetext = GTK_WIDGET(gtk_builder_get_object(builder, "metertypetext"));
	    metersntext = GTK_WIDGET(gtk_builder_get_object(builder, "metersntext"));
	    iccidtext = GTK_WIDGET(gtk_builder_get_object(builder, "iccidtext"));
	    imsitext = GTK_WIDGET(gtk_builder_get_object(builder, "imsitext"));
	    statustext = GTK_WIDGET(gtk_builder_get_object(builder, "statustext"));
	    hbinttext = GTK_WIDGET(gtk_builder_get_object(builder, "heartbeattext"));
	    bootNotificationBtn = GTK_WIDGET(gtk_builder_get_object(builder, "sendnotificationbtn"));

	    g_signal_connect(G_OBJECT(bootNotificationBtn),"clicked",G_CALLBACK(sendBootNotification),G_OBJECT(bootNotificationBtn));

	    //SOLAPA 2 - Data Transfer
	    messageIdtext = GTK_WIDGET(gtk_builder_get_object(builder, "messageidtext"));
	    messageDatatext = GTK_WIDGET(gtk_builder_get_object(builder, "messagedatatext"));
	    dataTransferBtn = GTK_WIDGET(gtk_builder_get_object(builder, "datatransferbtn"));

	    g_signal_connect(G_OBJECT(dataTransferBtn),"clicked",G_CALLBACK(senddataTransfer),G_OBJECT(dataTransferBtn));

	    //TAB 3 - Authorize
	    idTagtext = GTK_WIDGET(gtk_builder_get_object(builder, "idtagtext"));
	    rndIdTagBtn = GTK_WIDGET(gtk_builder_get_object(builder, "rndidtagbtn"));
	    authorizeBtn = GTK_WIDGET(gtk_builder_get_object(builder, "sendauthorizebtn"));

	    g_signal_connect(G_OBJECT(authorizeBtn),"clicked",G_CALLBACK(sendauthorize),G_OBJECT(authorizeBtn));

	    //TAB 4 - Status Notification
	    connectornotiftext[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1notiftext"));
	    connectornotiftext[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2notiftext"));
	    connectornotiftext[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3notiftext"));
	    connectornotiftext[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4notiftext"));

	    connectorlabel1[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1label"));
	    connectorlabel1[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2label"));
	    connectorlabel1[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3label"));
	    connectorlabel1[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4label"));
	    connectorlabel2[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1label2"));
	    connectorlabel2[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2label2"));
	    connectorlabel2[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3label2"));
	    connectorlabel2[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4label2"));
	    connectorlabel3[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1label3"));
	    connectorlabel3[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2label3"));
	    connectorlabel3[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3label3"));
	    connectorlabel3[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4label3"));

	    connectorstatuscombo[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1statuscombo"));
	    connectorstatuscombo[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2statuscombo"));
	    connectorstatuscombo[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3statuscombo"));
	    connectorstatuscombo[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4statuscombo"));

	    connectorerrorcombo[0]= GTK_WIDGET(gtk_builder_get_object(builder, "connector1errorcombo"));
	    connectorerrorcombo[1]= GTK_WIDGET(gtk_builder_get_object(builder, "connector2errorcombo"));
	    connectorerrorcombo[2]= GTK_WIDGET(gtk_builder_get_object(builder, "connector3errorcombo"));
	    connectorerrorcombo[3]= GTK_WIDGET(gtk_builder_get_object(builder, "connector4errorcombo"));

	    for (int i=0; i<num_connectors; i++)
	    {
	    	//gtk_combo_box_text_append_text( GTK_COMBO_BOX(connector1combo), convert(i+1) );
	    	gtk_entry_set_text( GTK_ENTRY(connectornotiftext[i]), convertF(connectorValues[i]) );

	    	for (int j=0; j<15; j++)
	    	{
	    		gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(connectorstatuscombo[i]), ChargePointErrorCodeTexts[j] );
	    	}

	    	for (int k=0; k<9; k++)
	    	{
	    		gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT(connectorerrorcombo[i]), ChargePointStatusTexts[k] );
	    	}

    	   gtk_combo_box_set_active (GTK_COMBO_BOX(connectorstatuscombo[i]),5);
    	   gtk_combo_box_set_active (GTK_COMBO_BOX(connectorerrorcombo[i]),0);

    	   g_signal_connect(G_OBJECT(connectorstatuscombo[i]),"changed",G_CALLBACK(sendstatusNotification),G_OBJECT(connectorstatuscombo[i]));
    	   g_signal_connect(G_OBJECT(connectorerrorcombo[i]),"changed",G_CALLBACK(sendstatusNotification),G_OBJECT(connectorerrorcombo[i]));

    	   gtk_widget_set_visible(GTK_WIDGET(connectornotiftext[i]), TRUE);
    	   gtk_widget_set_visible(GTK_WIDGET(connectorstatuscombo[i]), TRUE);
    	   gtk_widget_set_visible(GTK_WIDGET(connectorerrorcombo[i]), TRUE);
    	   gtk_widget_set_visible(GTK_WIDGET(connectorlabel1[i]), TRUE);
    	   gtk_widget_set_visible(GTK_WIDGET(connectorlabel2[i]), TRUE);
		   gtk_widget_set_visible(GTK_WIDGET(connectorlabel3[i]), TRUE);
	    }

	    statusnotificationbtn = GTK_WIDGET(gtk_builder_get_object(builder, "statusnotificationbtn"));
	    g_signal_connect(G_OBJECT(statusnotificationbtn),"clicked",G_CALLBACK(sendstatusNotification),G_OBJECT(statusnotificationbtn));

	    //SOLAPA 5 - Diagnostics
	    diagnosticsstatuscombo = GTK_WIDGET(gtk_builder_get_object(builder, "diagnosticsstatuscombo"));
	    gtk_combo_box_text_append_text( GTK_COMBO_BOX( diagnosticsstatuscombo), "Idle" );
	    gtk_combo_box_text_append_text( GTK_COMBO_BOX( diagnosticsstatuscombo), "Uploaded" );
	    gtk_combo_box_text_append_text( GTK_COMBO_BOX( diagnosticsstatuscombo), "UploadingFailed" );
	    gtk_combo_box_text_append_text( GTK_COMBO_BOX( diagnosticsstatuscombo), "Uploading" );

	    diagnosticsStatusNotificationBtn = GTK_WIDGET(gtk_builder_get_object(builder, "senddiagnosticsbtn"));

	    g_signal_connect(G_OBJECT(diagnosticsStatusNotificationBtn),"clicked",G_CALLBACK(senddiagnosticsStatusNotification),G_OBJECT (diagnosticsStatusNotificationBtn));

	   	//SOLAPA 6 - Firmware
	   	firmwarestatuscombo = GTK_WIDGET(gtk_builder_get_object(builder, "firmwarestatuscombo"));
	   	gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( firmwarestatuscombo), "Downloaded" );
	   	gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( firmwarestatuscombo), "DownloadFailed" );
	   	gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( firmwarestatuscombo), "Downloading" );
	   	gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( firmwarestatuscombo), "Idle" );
	   	gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( firmwarestatuscombo), "InstallationFailed" );
	   	gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( firmwarestatuscombo), "Installing" );
	   	gtk_combo_box_text_append_text( GTK_COMBO_BOX_TEXT( firmwarestatuscombo), "Installed" );

	   	firmwareStatusNotificationBtn = GTK_WIDGET(gtk_builder_get_object(builder, "sendfirmwarebtn"));
	   	g_signal_connect(G_OBJECT(firmwareStatusNotificationBtn),"clicked",G_CALLBACK(sendfirmwareStatusNotification),G_OBJECT(firmwareStatusNotificationBtn));
/*
	   	//SOLAPA 7 - Meter Values

	   	connectorlabel[0]= GTK_WIDGET(gtk_builder_get_object(builder, "connectorlabel1"));
	   	connectorlabel[1]= GTK_WIDGET(gtk_builder_get_object(builder, "connectorlabel2"));
	   	connectorlabel[2]= GTK_WIDGET(gtk_builder_get_object(builder, "connectorlabel3"));
	   	connectorlabel[3]= GTK_WIDGET(gtk_builder_get_object(builder, "connectorlabel4"));

	   	valoresguardados= GTK_WIDGET(gtk_builder_get_object(builder, "connectorlabel6"));

	   	connectortext[0]= GTK_WIDGET(gtk_builder_get_object(builder, "connector1value1text"));
	   	connectortext[1]= GTK_WIDGET(gtk_builder_get_object(builder, "connector2value1text"));
	   	connectortext[2]= GTK_WIDGET(gtk_builder_get_object(builder, "connector3value1text"));
	   	connectortext[3]= GTK_WIDGET(gtk_builder_get_object(builder, "connector4value1text"));

	   	gtk_entry_set_text(GTK_ENTRY(connectortext[0]), convert(connectorValues[0]));
	   	gtk_entry_set_text(GTK_ENTRY(connectortext[1]), convert(connectorValues[1]));
	   	gtk_entry_set_text(GTK_ENTRY(connectortext[2]), convert(connectorValues[2]));
	   	gtk_entry_set_text(GTK_ENTRY(connectortext[3]), convert(connectorValues[3]));

	   	measurandcombo[0]= GTK_WIDGET(gtk_builder_get_object(builder, "connector1measurandcombo"));
	   	measurandcombo[1]= GTK_WIDGET(gtk_builder_get_object(builder, "connector2measurandcombo"));
	   	measurandcombo[2]= GTK_WIDGET(gtk_builder_get_object(builder, "connector3measurandcombo"));
	   	measurandcombo[3]= GTK_WIDGET(gtk_builder_get_object(builder, "connector4measurandcombo"));

	    for (int i=0; i<22; i++)
	    {
	    	for (int j=0; j<4; j++)
	    	{
	    		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(measurandcombo[j]), Measurand_texts[i] );
	    	}
	   	}

	   	unitcombo[0]= GTK_WIDGET(gtk_builder_get_object(builder, "connector1unitcombo"));
	   	unitcombo[1]= GTK_WIDGET(gtk_builder_get_object(builder, "connector2unitcombo"));
	   	unitcombo[2]= GTK_WIDGET(gtk_builder_get_object(builder, "connector3unitcombo"));
	   	unitcombo[3]= GTK_WIDGET(gtk_builder_get_object(builder, "connector4unitcombo"));

	   	for (int i=0; i<16; i++)
	   	{
	   		for (int j=0; j<4; j++)
	   		{
	   			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(unitcombo[j]), UnitOfMeasureTexts[i] );
	   		}
	   	}

	   	locationcombo[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1locationcombo"));
	   	locationcombo[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2locationcombo"));
	   	locationcombo[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3locationcombo"));
	   	locationcombo[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4locationcombo"));

	    for (int i=0; i<5; i++)
	    {
	    	for (int j=0; j<4; j++)
	    	{
	    		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(locationcombo[j]), Location_texts[i] );
	    	}
	   	}

	    contextcombo[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1contextcombo"));
	    contextcombo[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2contextcombo"));
	    contextcombo[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3contextcombo"));
	    contextcombo[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4contextcombo"));
	    for (int i=0; i<8; i++)
	    {
	    	for (int j=0; j<4; j++)
	    	{
	    		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(contextcombo[j]), ReadingContextTexts[i] );
	    	}
	    }

	    phasecombo[0] = GTK_WIDGET(gtk_builder_get_object(builder, "connector1phasecombo"));
	    phasecombo[1] = GTK_WIDGET(gtk_builder_get_object(builder, "connector2phasecombo"));
	    phasecombo[2] = GTK_WIDGET(gtk_builder_get_object(builder, "connector3phasecombo"));
	    phasecombo[3] = GTK_WIDGET(gtk_builder_get_object(builder, "connector4phasecombo"));
	    for (int i=0; i<10; i++)
	    {
	    	for (int j=0; j<4; j++)
	    	{
	    		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phasecombo[j]), Phase_texts[i] );
	    	}
	    }

	    for (int i=1; i<NUM_CONNECTORS; i++)
	    {
	    	gtk_widget_set_visible(phasecombo[i], TRUE );
	    	gtk_widget_set_visible(contextcombo[i], TRUE );
	    	gtk_widget_set_visible(locationcombo[i], TRUE );
	    	gtk_widget_set_visible(unitcombo[i], TRUE );
	    	gtk_widget_set_visible(measurandcombo[i], TRUE );
	    	gtk_widget_set_visible(connectortext[i], TRUE );
	    	gtk_widget_set_visible(connectorlabel[i], TRUE );
	    }

	    meterValuesBtn = GTK_WIDGET(gtk_builder_get_object(builder, "sendmetervaluesbtn"));
	    savevaluesbtn = GTK_WIDGET(gtk_builder_get_object(builder, "savevaluesbtn"));

		g_signal_connect(G_OBJECT(savevaluesbtn),"clicked",G_CALLBACK(saveValues),NULL);
	 //   g_signal_connect(G_OBJECT(meterValuesBtn),"clicked",G_CALLBACK(sendmeterValues),G_OBJECT(meterValuesBtn));
*/
	    //SOLAPA 8 - START TRANSACTION
	    startTransactionTab = GTK_WIDGET(gtk_builder_get_object(builder, "starttransactiongrid"));
	    connectorId = GTK_WIDGET(gtk_builder_get_object(builder, "startconnectorcombo"));
		startIdTagText = GTK_WIDGET(gtk_builder_get_object(builder, "startidtagtext"));
		startConnectorText = GTK_WIDGET(gtk_builder_get_object(builder, "startconnectortext"));
	   	startReservationText = GTK_WIDGET(gtk_builder_get_object(builder, "startreservationtext"));

	    getRandomIdTagBtn = GTK_WIDGET(gtk_builder_get_object(builder, "getrandomidtagbtn"));
	    startTransactionBtn = GTK_WIDGET(gtk_builder_get_object(builder, "starttransactionbtn"));

	    for (int i=0; i<NUM_CONNECTORS; i++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(connectorId),convert(i+1));

		g_signal_connect(G_OBJECT(connectorId),"changed",G_CALLBACK(updateStartTransactionTab),G_OBJECT(connectorId));
	    g_signal_connect(G_OBJECT(connectorId),"changed",G_CALLBACK(updateConnectorValue),G_OBJECT(startConnectorText));
	    g_signal_connect(G_OBJECT(getRandomIdTagBtn),"clicked",G_CALLBACK(getRandomIdTagGUI),G_OBJECT(startIdTagText));
	    g_signal_connect(G_OBJECT(startTransactionBtn),"clicked",G_CALLBACK(sendstartTransaction),G_OBJECT(startTransactionBtn));

	    //SOLAPA 9
	//    heartbeatBtn = GTK_WIDGET(gtk_builder_get_object(builder, "sendheartbeatbtn"));
	  //  g_signal_connect(G_OBJECT(heartbeatBtn),"clicked",G_CALLBACK(sendheartbeat),G_OBJECT(heartbeatBtn));


	    //SOLAPA 10
	   // stopTransactionTab = GTK_WIDGET(gtk_builder_get_object(builder, "stoptransactiongrid"));
	    //
	    stoptransactionconnectorcombo=GTK_WIDGET(gtk_builder_get_object(builder, "stoptransactionconnectorcombo"));
	    //
	    stopIdTagText = GTK_WIDGET(gtk_builder_get_object(builder, "stopidtagtext"));
	    //
	    stopConnectorText = GTK_WIDGET(gtk_builder_get_object(builder, "stopconnectortext"));
	    //
	    stopTransactionText = GTK_WIDGET(gtk_builder_get_object(builder, "stoptransactiontext"));
	    //
	    stopReasonCombo = GTK_WIDGET(gtk_builder_get_object(builder, "stopreasoncombo"));

	    for (int i=0; i<11; i++)
	    {
	    	gtk_combo_box_text_append_text( GTK_COMBO_BOX(stopReasonCombo), Stopping_Transaction_Reason_texts[i] );
	    }
	    //
	    stopTransactionBtn = GTK_WIDGET(gtk_builder_get_object(builder, "stoptransactionbtn"));
	    stoptransactionbtn1= GTK_WIDGET(gtk_builder_get_object(builder, "stoptransactionbtn1"));
		stoptransactionbtn2= GTK_WIDGET(gtk_builder_get_object(builder, "stoptransactionbtn2"));

	    g_signal_connect(G_OBJECT(stoptransactionbtn2),"clicked",G_CALLBACK(disconnectEV),G_OBJECT(stopTransactionBtn));
	    g_signal_connect(G_OBJECT(stoptransactionbtn1),"clicked",G_CALLBACK(presentIdentifierToDisconnectEV),G_OBJECT(stopTransactionBtn));
	    g_signal_connect(G_OBJECT(stopTransactionBtn),"clicked",G_CALLBACK(sendstopTransaction),G_OBJECT(stopTransactionBtn));
	    g_signal_connect(G_OBJECT(stoptransactionconnectorcombo),"changed",G_CALLBACK(updateValue),stopConnectorText);

	    LogTextView2=GTK_WIDGET(gtk_builder_get_object(builder, "LogTextView2"));
	    notebook=GTK_WIDGET(gtk_builder_get_object(builder, "notebook"));

	  	    //DIALOGOS
	    dialogbuilder = gtk_builder_new();
	   	gtk_builder_add_from_file (dialogbuilder, "/etc/ocpp/TextDialog.glade", NULL);
	    OKbutton = GTK_WIDGET(gtk_builder_get_object(dialogbuilder, "OKBtn"));
	    g_signal_connect (G_OBJECT (OKbutton), "clicked",G_CALLBACK (CloseDialog),G_OBJECT(dialog_window));

	    g_object_unref(builder);
	    //

	    setGUIData();

	    ////////////////////
	   	//Creamos un thread para cada conector. De esta forma, simulamos la carga

	   	pthread_t pid[NUM_CONNECTORS];
	   	for (int conn=0; conn<NUM_CONNECTORS; conn++)
	   	{
	   	   	pthread_create(&pid[conn], NULL, cargaConector, conn);
	   	   	pthread_detach(pid[conn]);
//	   	   	printf("\nCREAMOS UN THREAD: %d", conn);
	   	}

	   	guiready=1;
	    gtk_widget_show(window);
	    gtk_main();
	   // gtk_window_set_type_hint (window,GDK_WINDOW_TYPE_HINT_NORMAL);
	    //gtk_combo_box_set_active (connector1statuscombo,0);




	    return 0;
}

void on_window_main_destroy()
{

    gtk_main_quit();
}

