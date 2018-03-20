#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


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


int main()
{
	char c;
	int shmid;
	key_t key;
	char *shm, *s;
	struct shared_data *datos;
	datos=(struct shared_data *)calloc(1, sizeof(struct shared_data));

//S-I-C-A --> 19-9-3-1
	key=19931;

	//Creamos el segmento
	if ((shmid=shmget(key,sizeof(datos), IPC_CREAT| 0666)) < 0)
	{
		perror("shmget error");
		exit (1);
	}

	if ((shm=shmat(shmid,NULL,0))==(char *) -1)
	{
		perror("shmat error");
		exit (1);
	}

	*shm='\0';
	
	
	s=shm;

	while (*shm != '*') sleep(1);
	datos=(struct shared_data *)s;	

	printf("%d, %d, %f",datos->transaction1allowed, datos->rebooting, datos->voltageConnector1);

	exit(0);
}	
	
