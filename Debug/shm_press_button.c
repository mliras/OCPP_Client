#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct shared_data{
	char flag;
	int boton1;
	int boton2;
        int transaction1allowed;
        int transaction2allowed;
        int rebooting;
        int cable1locked;
        int cable2locked;
        int statusConnector1;
        int statusConnector2;
        int stopTransaction;
        float voltageConnector1;
        float voltageConnector2;
        float currentConnector1;
        float currentConnector2;
        int error1;
        int error2;
        int error3;
        float currentOffered;
        float powerlineFrequency;
        float powerFactor;
        float fanSpeed;
        float stateOfCharge1;
        float stateOfCharge2;
};


int main(int argc, char *argv[])
{
	if (argc<2)
	{
		printf ("Syntax Error: %s <conector> <valor>", argv[0]);
		exit (-3);
	}

	int connector=atoi(argv[1]);
	int valor=atoi(argv[2]);
	
	char c;
	int shmid;
	key_t key;
	char *shm, *s;
	struct shared_data *datos;
	memset(&datos, 0, sizeof(datos));

//S-I-C-A --> 19-9-3-1
	key=19931;

	//Creamos el segmento
	if ((shmid=shmget(key,sizeof(datos), IPC_CREAT| 0666)) < 0)
	{
		perror("shmget error");
		exit (1);
	}

	datos=(struct shared_data *)shmat(shmid,NULL,0);

	if (connector==1) datos->boton1=valor;
	if (connector==2) datos->boton2=valor;

	//s=shm;

	//memcpy(s, &datos, sizeof(datos));

	datos->flag='*';

	exit(0);
}	
	
