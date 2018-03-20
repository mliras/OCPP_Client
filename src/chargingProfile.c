/*
 * chargingProfile.c
 *
 *  Created on: Dec 19, 2017
 *      Author: root
 */
#include "chargingProfile.h"

//Esta funcion inicializa toda a NULL la tabla currentChargingProfiles
void chargingProfileInitialize()
{
	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		currentChargingProfiles[i]=NULL;
	}
}

//Libera de memoria toda la compleja estructura de la ChargingProfileList pasada como parámetro
void freeChargingProfileList(struct ChargingProfileList *temp)
{
	if (!temp) return;
	struct ChargingProfile *cp;
	cp=temp->chargingProfile;
	freeChargingProfile(cp);

	free(temp);
}

//Libera de memoria toda la compleja estructura de la ChargingProfile pasada como parámetro
void freeChargingProfile(struct ChargingProfile *cp)
{
	if (!cp) return;
	struct ChargingSchedulePeriod *period1, *period2;
	struct ChargingSchedule *cs;

	cs=&cp->chargingSchedule;
	if (!cs) return;

	period1=&cs->chargingSchedulePeriods;
	if (!period1) return;

	period2=period1;

	while (period2)
	{
		free (period2);
		period1=period1->next;
		period2=period1;
	}

	if (cs)
	{
		if (cs->duration) free(cs->duration);
		if (cs->minChargingRate) free(cs->minChargingRate);
		if (cs->startSchedule) free(cs->startSchedule);
		free (cs);
	}

	if (cp)
	{
		if (cp->recurrencyKind) free(cp->recurrencyKind);
		if (cp->transactionId) free(cp->transactionId);
		if (cp->validFrom) free(cp->validFrom);
		if (cp->validTo) free(cp->validTo);
		free(cp);
	}
}

//Esta funcion reemplaza un charging profile para un connectorId dado.
//Si actualmente no tiene ningun chargingProfile, se lo asigna.
//Si no, libera el anterior (para lo que llama a la funcion freeChargingProfile) y alloca el nuevo
//connector debe ser un valro de 0 a N-1
void replaceChargingProfile(struct ChargingProfile *ChPr, int connector)
{
	if (!ChPr) return;

	struct ChargingProfileList *temp, *anterior, *aux, *aux2;

	if (currentChargingProfiles[connector]==NULL)
	{
		//Actualmente el connector no tiene charging Profiles.
		struct ChargingProfileList *CPL=(struct ChargingProfileList *)calloc(1, sizeof(struct ChargingProfileList));

		CPL->next=NULL;
		CPL->chargingProfile=ChPr;
		currentChargingProfiles[connector]=CPL;
	}
	else
	{
		struct ChargingProfileList *temp=currentChargingProfiles[connector];
		int found=0;
		while (temp)
		{
			//recorremos los charging profiles de ese conector y los sustituimos si se cumple la condicion
			if (temp->chargingProfile)
			{
				//Pag 58: If  a charging   profile   with   the   same	chargingProfileId, or the same combination of stackLevel/ChargingProfilePurpose,
				//exists  on  the  Charge  Point,  the  new  charging  profile  SHALL  replace  the	existing    charging  profile,  otherwise  it  SHALL  be  added.
				if ((temp->chargingProfile->chargingProfileId==ChPr->chargingProfileId) || ((temp->chargingProfile->stackLevel==ChPr->stackLevel) && (temp->chargingProfile->chargingProfilePurpose==ChPr->chargingProfilePurpose)))
				{

					//Reemplazamos

					//Pag 58: When   a ChargingProfile   is   refreshed   during   execution,   it   is   advised   to   put   the startSchedule  of  the  new
					//ChargingProfile  in  the  past,  so  there  is  no  period  of  default charging behaviour inbetween the ChargingProfiles. The Charge Point SHALL continue
					//to execute the existing ChargingProfile until the new ChargingProfile is installed.

					if (ChPr->transactionId>0)
					{
						///Estamos en una transactionId

						if (ChPr->chargingSchedule.startSchedule)
						{
							time_t now;
							struct tm* now_tm;

						//	struct tm* startTime;
							__time_t fechainicio;

							time(&now);
							now_tm = localtime(&now);

							fechainicio = mktime(ChPr->chargingSchedule.startSchedule);
							double diffSecs = difftime(fechainicio, now);

							if (diffSecs<0.0)
							{
								free(ChPr->chargingSchedule.startSchedule);
								ChPr->chargingSchedule.startSchedule=now_tm;
							}
						}
					}

					struct ChargingProfile *ttt=temp->chargingProfile;

					temp->chargingProfile=ChPr;

					freeChargingProfile(ttt);
				}
			}

			temp=temp->next;
		}
	}
}

//connectorId debe ser un valor de 0 a N-1
void reevaluate_chargingProfiles(connectorId)
{

}

//Devuelve 1 si se añadio exitosamente
//Devuelve 0 si NO pudo añadir el charging profile
//connector debe ser un valor de 0 a N-1
int addChargingProfile(struct ChargingProfile *ChPr, int connector)
{
	struct ChargingProfileList *CPL=(struct ChargingProfileList *)calloc(1, sizeof(struct ChargingProfileList));
	struct ChargingProfileList *temp, *anterior, *aux, *aux2;

	CPL->chargingProfile=ChPr;

	if (connector>=0)
	{
		//Pag 23: ChargePointMaxProfile can only be set at Charge Point ConnectorId 0.
		if (ChPr->chargingProfilePurpose==_CPPT_CHARGEPOINTMAXPROFILE) return 0;

		if (currentChargingProfiles[connector]==NULL)
		{
			//Actualmente el connector no tiene charging Profiles.
			CPL->next=NULL;
			CPL->chargingProfile=ChPr;

			currentChargingProfiles[connector]=CPL;
			return 1;
		}
		else
		{
			temp=currentChargingProfiles[connector];

			//Pag 24: To  avoid  conflicts,  the  existence  of  multiple  Charging  Profiles  with  the  same  stackLevel  and  Purposes in  a  Charge  Point  is  not  allowed.

			while ((ChPr->chargingProfilePurpose!=temp->chargingProfile->chargingProfilePurpose) || (ChPr->stackLevel!=temp->chargingProfile->stackLevel)) temp=temp->next;

			if (temp)
			{
				//Hemos encontrado un charging profile en el que se cumple que son iguales el purpose y el stacklevel. No esta permitido, devolvemos 0.
				return 0;
			}
			else
			{
				//Pag24: It  is  allowed  to  stack  charging  profiles  of  the  same  charging  profile  purpose  in  order  to  describe complex calendars.
				//Precedence of charging profiles is determined by the value of their StackLevel parameter. At any point
				//in  time  the  prevailing  charging  profile  SHALL  be  the  charging  profile  with  the  highest  stackLevel
				//among  the  profiles  that  are  valid  at  that  point  in  time,  as  determined  by  their  validFrom  and  validTo
				//parameters.
				temp=currentChargingProfiles[connector];
				anterior=temp;


				while ((temp) && (ChPr->chargingProfilePurpose==temp->chargingProfile->chargingProfilePurpose && ChPr->stackLevel < temp->chargingProfile->chargingProfilePurpose))
				{
					//Buscamos la posicion donde colocarlo.
					anterior=temp;
					temp=temp->next;
				}

				if (!temp)
				{
					//Hemos llegado al final de la lista. Y son del mismo purpose. Lo añadimos al final.
					if (anterior)
						anterior->next=CPL;
					else
						anterior=CPL;

					CPL->next=NULL;

					return 1;
				}
				else
				{
					//No es el final de la lista... o bien el purpose es diferente

					if (temp->chargingProfile->chargingProfilePurpose!=ChPr->chargingProfilePurpose)
					{
						//Se trata de un purpose diferente. Hay que susituirlo. A el y a sus "hijos"
						aux=temp->next;

						//temp sigue apuntando al actual. Eliminamos todos sus "hijos"
						while (aux)
						{
							aux2=aux->next;
							free(aux);
							aux=aux2;
						}

						//Tambien liberamos temp
						free(temp);

						//Y ahora añadimos el ChargingProfile nuevo.
						if (anterior)
							anterior->next=CPL;
						else
							anterior=CPL;

						return 1;
					}
					else
					{
						//Con el mismo purpose y no es el final de la lista

						if (anterior)
							anterior->next=CPL;
						else
							anterior=CPL;

						CPL->next=temp;

						return 1;
					}
				}
			}
		}
	}
	else
	{
		//Si el conector es -1, Añadimos el charging profile a todas las conexiones. Haciendo una llamada a la propia funcion
		for (int j=0;j<NUM_CONNECTORS;j++)
		{
			addChargingProfile(ChPr,j);
		}
	}

	return 1;
}

//
//5.5.  Ver pag 67
//
// This function removes charging profiles from the currentChargingProfiles array depending on the parameters passed
// Parameters are integers. If their values are negative, they are not taken into account.
//
// Returns 0 if it finds any charging profile to remove
// Returns 1 in other case
//
//connector debe ser un valor de 1 a N. Si se recibe un 0 es que no llego el connectorId
int removeChargingProfilesFromConnector(int connector, int purpose, int stacklevel)
{
	struct ChargingProfileList *temp, *aux;
	int notfound=1;

	//Hay 8 combinaciones sobre si se recibe el connectorId, el purpose o el stacklevel
	if (connector>0)
	{
		temp=currentChargingProfiles[connector-1];

		if (purpose>=0)
		{
			if (stacklevel>=0)
			{
				while (temp)
				{
					if (temp->chargingProfile->stackLevel==stacklevel && temp->chargingProfile->chargingProfilePurpose==purpose)
					{
						aux=temp;
						free(temp->chargingProfile);
						temp=aux->next;
						free(aux);

						if (aux==currentChargingProfiles[connector-1]) currentChargingProfiles[connector-1]=temp;

						notfound=0;
					}
				}

			}
			else
			{
				while (temp)
				{
					if (temp->chargingProfile->chargingProfilePurpose==purpose)
					{
						aux=temp;
						free(temp->chargingProfile);
						temp=aux->next;
						free(aux);

						if (aux==currentChargingProfiles[connector-1]) currentChargingProfiles[connector-1]=temp;

						notfound=0;
					}
				}
			}
		}
		else //purpose no se recibio
		{
			if (stacklevel>=0)
			{
				while (temp)
				{
					if (temp->chargingProfile->stackLevel==stacklevel)
					{
						aux=temp;
						free(temp->chargingProfile);
						temp=aux->next;
						free(aux);

						if (aux==currentChargingProfiles[connector-1]) currentChargingProfiles[connector-1]=temp;

						notfound=0;
					}
				}

			}
			else//No se recibio ni purpose ni stacklevel. Se borran todos los del connectorId
			{
				while (temp)
				{
						aux=temp;
						free(temp->chargingProfile);
						temp=aux->next;
						free(aux);
						notfound=0;
				}

				currentChargingProfiles[connector-1]=NULL;
			}

		}
	}
	else  //No se recibio connectorId (llega un cero o un -1). Los recorremos todos
	{
		//un connector de 0 es como si no se recibiera. Se refiere a todos.

	   		if (purpose>=0)
			{
				if (stacklevel>=0)
				{
					for (int i=0; i<NUM_CONNECTORS; i++)
					{
						temp=currentChargingProfiles[i];

						while (temp)
						{
							if (temp->chargingProfile->stackLevel==stacklevel && temp->chargingProfile->chargingProfilePurpose==purpose)
							{
								aux=temp;
								free(temp->chargingProfile);
								temp=aux->next;
								free(aux);

								if (aux==currentChargingProfiles[i]) currentChargingProfiles[i]=temp;

								notfound=0;
							}
						}
					}
				}
				else //stacklevel not received
				{
					for (int i=0; i<NUM_CONNECTORS; i++)
					{
						temp=currentChargingProfiles[i];

						while (temp)
						{
							if (temp->chargingProfile->chargingProfilePurpose==purpose)
							{
								aux=temp;
								free(temp->chargingProfile);
								temp=aux->next;
								free(aux);

								if (aux==currentChargingProfiles[i]) currentChargingProfiles[i]=temp;

								notfound=0;
							}
						}
					}
				}
			}
	   		else //no envian ni connector ni purpose
	   		{
	   			if (stacklevel>=0)//...pero si el stacklevel
	   			{
	   				for (int i=0; i<NUM_CONNECTORS; i++)
	   				{
	   					temp=currentChargingProfiles[i];

	   					while (temp)
	   					{
	   						if (temp->chargingProfile->stackLevel==stacklevel )
	   						{
	   							aux=temp;
	   							free(temp->chargingProfile);
	   							temp=aux->next;
	   							free(aux);

	   							if (aux==currentChargingProfiles[i]) currentChargingProfiles[i]=temp;

	   							notfound=0;
	   						}
	   					}
	   				}
	   			}
	   			else //stacklevel not received either. No se recibe nada. Lo vaciamos todo
	   			{
	   				for (int i=0; i<NUM_CONNECTORS; i++)
	   				{
	   					temp=currentChargingProfiles[i];

	   					while (temp)
	   					{
	   							aux=temp;
	   							free(temp->chargingProfile);
	   							temp=aux->next;
	   							free(aux);

	   							if (aux==currentChargingProfiles[i]) currentChargingProfiles[i]=temp;

	   							notfound=0;
	   					}
	   				}
	   			}
	   		}
		}

	return notfound;
}

//5.5. Ver pag 67
// This function removes charging profiles given the profileId passed
// It browse the whole currentChargingProfiles array looking for that specific charging profile.
//
// Returns 0 if it finds any charging profile to remove
// Returns 1 in other case
int removeChargingProfilesFromId(int ProfileId)
{
	struct ChargingProfileList *temp, *aux;

	for (int i=0; i<NUM_CONNECTORS; i++)
	{
		temp=currentChargingProfiles[i];

		while (temp)
		{
			if (temp->chargingProfile->chargingProfileId==ProfileId)
			{
				aux->next=temp->next;

				aux=temp;
				free(temp->chargingProfile);
				temp=aux->next;
				free(aux);

				return 0;
			}
			else
			{
				aux=temp;
				temp=aux->next;
			}
		}
	}

	return 1;
}

