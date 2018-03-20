/*
 * ftpDiagnostics.c
 *
 *  Created on: Dec 18, 2017
 *      Author: root
 */

#include "ftpDiagnostics.h"

//HILO QUE SE QUEDA ESPERANDO HASTA LA HORA DE LA DESCARGA DEL FICHERO Y LUEGO LO DESCARGA
int getFirmwareFile(void *parameters)
{
	struct ftp_parameters *params = parameters;
	time_t now;
	struct tm* now_tm;
	struct tm* startTime;
	struct tm * _fecha;

	time(&now);
	now_tm = localtime(&now);

	strptime(params->startTime, "%Y-%m-%dT%H:%M:%S.", startTime);
	__time_t fecha = mktime(startTime);
	double diffSecs = difftime(fecha, now);

	//Esperamos hasta que sea la hora. Comprobamos cada 10 segundos
	while (diffSecs>0.0)
	{
		usleep(10000);
		time(&now);
		diffSecs = difftime(fecha, now);
	}

	lastFirmwareUploadStatus=_FS_DOWNLOADING;
	//Lo enviamos ya
	int res=ftpGetProcess(params->location, 21, params->username, params->password, params->originalfilename,params->destinationfilename);

		//Si falla el envio
		if (res!=1)
		{
			//Esperamos un tiempo indicado en retryInterval
			if (params->retryInterval>0)
			{
				usleep(1000*params->retryInterval);
			}
			else
			{
				usleep(5000);  //por defecto lo reintenta a los 5 segundos
			}

			//Comprueba si hay un numero de reenvios y si los hay, lo reintenta
			for (int i=0; i<params->retries;i++)
			{
					res=ftpGetProcess(params->location, 21, params->username, params->password, params->originalfilename,params->destinationfilename);

					if (res!=1)
					{
						if (params->retryInterval>0)
						{
							usleep(1000*params->retryInterval);
						}
						else
						{
							usleep(5000);  //por defecto lo reintenta a los 5 segundos
						}
					}
					//Si en un intento lo consigue, sale del bucle
					else
					{
						lastFirmwareUploadStatus=_FS_DOWNLOADED;
						break;
					}
			}

			lastFirmwareUploadStatus=_FS_DOWNLOADFAILED;
		}
		else
		{
			lastFirmwareUploadStatus=_FS_DOWNLOADED;
		}

		//send_diagnosticsstatus_request(lastDiagnosticsFileStatus);
		//ENVIO OK
		return 0;
}

//HILO QUE SE QUEDA ESPERANDO HASTA LA HORA DEL ENVIO DEL FICHERO Y LUEGO LO ENVIA
//Devuelve -1, si no puede mandarlo
//Devuelve 0 si se pudo mandar
int sendDiagnosticsFile(void *parameters)
{
	struct ftp_parameters *params = parameters;
	int res;
	time_t now;
	struct tm* now_tm;
	struct tm * _fecha;
	time(&now);
	now_tm = localtime(&now);

	busyUploadingDiagnosticsFile=1;

	if (params->startTime)
	{
		struct tm* startTime=(struct tm *)calloc(1, sizeof(struct tm));
		strptime(params->startTime, "%Y-%m-%dT%H:%M:%S.", startTime);
		__time_t fecha = mktime(startTime);

		double diffSecs = difftime(fecha, now);

		//Esperamos hasta que sea la hora. Comprobamos cada 10 segundos
		while (diffSecs>0.0)
		{
				usleep(10000);
				time(&now);
				diffSecs = difftime(fecha, now);
		}

		//AHORA YA HA PASADO LA HORA DE INICIO... PERO HA PASADO TAMBIEN LA HORA FINAL?

		if (params->stopTime)
		{
			struct tm* stopTime=(struct tm *)calloc(1, sizeof(struct tm));
			strptime(params->stopTime, "%Y-%m-%dT%H:%M:%S.", stopTime);
			__time_t stopfecha = mktime(stopTime);

			double diffSecs2 = difftime(stopfecha, now);
			if (diffSecs2>0.0)
			{
				//YA HA PASADO TAMBIEN LA HORA DE FINALIZACION
				lastDiagnosticsFileStatus=_DSN_UPLOADFAILED;
				send_diagnosticsstatus_request(lastDiagnosticsFileStatus);
				busyUploadingDiagnosticsFile=0;
				return -1;
			}
		}
	}

	lastDiagnosticsFileStatus=_DSN_UPLOADING;
	//Lo enviamos ya
	res=ftpSendProcess(params->location, 21, params->username, params->password, params->originalfilename,params->destinationfilename);

		//Si falla el envio
	if (res!=1)
	{
			//Esperamos un tiempo indicado en retryInterval
			if (params->retryInterval>0)
			{
				usleep(1000*params->retryInterval);
			}
			else
			{
				usleep(5000);  //por defecto lo reintenta a los 5 segundos
			}

			//Comprueba si hay un numero de reenvios y si los hay, lo reintenta
			for (int i=0; i<params->retries;i++)
			{
					res=ftpSendProcess(params->location, 21, params->username, params->password, params->originalfilename,params->destinationfilename);

					if (res!=1)
					{
						if (params->retryInterval>0)
						{
							usleep(1000*params->retryInterval);
						}
						else
						{
							usleep(5000);  //por defecto lo reintenta a los 5 segundos
						}
					}
					//Si en un intento lo consigue, sale del bucle
					else
					{
						lastDiagnosticsFileStatus=_DSN_UPLOADED;
						break;
					}
			}

			lastDiagnosticsFileStatus=_DSN_UPLOADFAILED;
	}
	else
	{
			lastDiagnosticsFileStatus=_DSN_UPLOADED;
	}

	send_diagnosticsstatus_request(lastDiagnosticsFileStatus);

	busyUploadingDiagnosticsFile=0;
	//ENVIO OK
	return 0;
}

//Funcion que abre un socket contra la IP y el puerto solcitado
//Devuelve el socket FD o 1 en caso de error
int startConnection(char *ip, int port)
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr;

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	        printf("\n Error : Could not create socket \n");
	        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
	{
	     printf("\n inet_pton error occurred\n");
	     return 1;
	}

	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
	    return 1;
	}

	return sockfd;
}

////wait for ftp server to start talking
int ftpRecvResponse(int sock, char * buf) {
   int i;

   if (recv(sock, buf, 5120, 0) == -1) {//receive the data
      perror("recv");
      return 0;;
   }

   for(i=(strlen(buf) - 1);i>0;i--) {
      if(buf[i]=='.' || buf[i]=='\r') {
         buf[i+1]='\0';
         break;
      }
   }
   return 1;
}

//Envia un mensaje FTP a un socket
int ftpNewCmd(int sock, char * buf, char * cmd, char * param) {
   strcpy(buf,cmd);
   if(strlen(param) > 0) {
      strcat(buf," ");
      strcat(buf,param);
   }
   strcat(buf,"\r\n");
//   printf("*%s",buf); //print the cmd to the screen
   if(send(sock, buf, strlen(buf), 0) == -1) {
      perror("send");
      return 0;
   }
   //clear the buffer
   return 1;
}

//Convierte lo recibido del servidor en una IP y un puerto, para saber donde nos indica
//que enviemos realmente el fichero. En hostname y port se escribiran estos datos
//...a partir de lo que tenemos en buf
int ftpConvertAddy(char * buf, char * hostname, int * port) {
   unsigned int i,t=0;
   int flag=0,decCtr=0,tport1,tport2;
   char tmpPort[6];

   for(i=0;i<strlen(buf);i++) {
      if(buf[i]=='(') {
         flag = 1;
         i++;
      }
      if(buf[i]==')') {
         hostname[t]='\0';
         break;
      }

      if(flag==1) {
         if(buf[i] != ',') {
            hostname[t]=buf[i];
            t++;
         } else {
            hostname[t]='.';
            t++;
         }
      }
   }
   t=0;
   for(i=0;i<strlen(hostname);i++) {
      if(hostname[i]=='.')
         decCtr++;
      if(decCtr==4 && hostname[i]!='.') {
         tmpPort[t]=hostname[i];
         t++;
         if(hostname[i+1]=='.') {
            tmpPort[t]='\0';
            tport1=atoi(tmpPort);
            t=0;
         }
      }

      if(decCtr==5 && hostname[i]!='.') {
         tmpPort[t]=hostname[i];
         t++;
         if(hostname[i+1]=='\0') {
            tmpPort[t]='\0';
            tport2=atoi(tmpPort);
            t=0;
         }
      }
   }

   *port = tport1 * 256 + tport2;
   decCtr=0;
   for(i=0;i<strlen(hostname);i++) {
      if(hostname[i]=='.') {
         decCtr++;
      }
      if(decCtr>3)
         hostname[i]='\0';
   }
   return 0;
}

//Envia el fichero que le indcamos al host y al puerto que le pasamos como parámetro
int ftpSendFile(char * filename, char * host, int port) {
   int sd;
   struct sockaddr_in pin;
   struct hostent *hp;
   char *buf;
   char buffer[256];

   if ((hp = gethostbyname(host)) == 0) {
      perror("gethostbyname");
      return -1;
   }

   memset(&pin, 0, sizeof(pin));
   pin.sin_family = AF_INET;
   pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   pin.sin_port = htons(port);

   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      return -1;
   }

   if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
      perror("Error while connecting.");
      return -1;
   }

   //LECTURA DEL FICHERO
   FILE *pf;
   unsigned long fsize;
   pf = fopen(filename, "rb");
   if (pf == NULL)
   {
       printf("File not found!\n");
       return 1;
   }
   else
   {
       fseek(pf, 0, SEEK_END);
       fsize = ftell(pf);
       rewind(pf);
   }

   while (1)
   {
       // Read data into buffer.  We may not have enough to fill up buffer, so we
       // store how many bytes were actually read in bytes_read.
	   for (int i=0; i<sizeof(buffer); i++) buffer[i]='\0';
       int bytes_read = fread(buffer, sizeof(char),sizeof(buffer), pf);
       if (bytes_read == 0) // We're done reading from the file
           break;

       if (bytes_read < 0)
       {
               perror("ERROR reading from file");
               return -1;
       }

       void *p = buffer;
       while (bytes_read > 0)
       {
               int bytes_written = send(sd, buffer, bytes_read, 0);
               if (bytes_written <= 0)
               {
                   perror("ERROR writing to socket\n");
                   return -1;
               }
               bytes_read -= bytes_written;
               p += bytes_written;
       }
   }

   fclose(pf);
   close(sd); //close the socket
   return 0;
}

//Funcion que inicia una conexion FTP contra el HOSTNAME y puerto indicado con el usuario y contraseña
//indicados y envia el fichero que se le pasa como parámetro.
//Para ello utiliza otras tres funciones anteriormente descritas: ftpNewCmd, ftpConvertAddy y ftpSendFile
int ftpSendProcess(char *hostname, int port, char *username, char *password, char *file, char *filename)
{
   int sd;
   struct sockaddr_in pin;
   struct hostent *hp;
   char tmpHost[100];
   int tmpPort;
   char buf[5120];

   if ((hp = gethostbyname(hostname)) == 0) {
      perror("Error looking for host by name");
      return -1;
   }

   memset(&pin, 0, sizeof(pin));
   pin.sin_family = AF_INET;
   pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   pin.sin_port = htons(port);

   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Error opening socket");
      return -1;
   }
   if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
      perror("Error during connection");
      return -1;
   }

   if(ftpRecvResponse(sd,buf)==1)
   {  //wait for ftp server to start talking
	   if (debug) printf("PASO A");
    if(strncmp(buf,"220",3)==0)
    {  //make sure it ends with a 220
    	if (debug) printf("PASO B");
     if(ftpNewCmd(sd,buf,"USER",username)==1)
     {  //issue the command to login
    	 if (debug) printf("PASO C");
      if(ftpRecvResponse(sd,buf)==1)
      {  //wait for response
    	  if (debug) printf("PASO D");
       if(strncmp(buf,"331",3)==0)
       {  //make sure response is a 331
    	   if (debug) printf("PASO E");
        if(ftpNewCmd(sd,buf,"PASS",password)==1)
        {
        	if (debug) printf("PASO F");
        	if(ftpRecvResponse(sd,buf)==1)
        	{ //wait for response
        		if (debug) printf("PASO G");
        	  if(strncmp(buf,"230",3)==0)
        	  { //make sure its a 230
        		  if (debug) printf("PASO H");
        	    if(ftpNewCmd(sd,buf,"PASV","")==1)
        	    {  //notify server we want to pass a file
        	    	if (debug) printf("\nPASO I");
        	       if(ftpRecvResponse(sd,buf)==1)
        	       {  //wait for response
        	    	   if (debug) printf("\nPASO J");
        	          if(strncmp(buf,"227",3)==0)
        	          {  //make sure it starts with a 227
        	        	  if (debug) printf("\nPASO K");
        	              ftpConvertAddy(buf,tmpHost,&tmpPort); //then convert the return text to usable data
        	            if(ftpNewCmd(sd,buf,"STOR",filename)==1)
        	            {  //set the file name AND
        	            	if (debug) printf("\nPASO L");
        	               ftpSendFile(file,tmpHost,tmpPort);
        	               if(ftpRecvResponse(sd,buf)==1)
        	               {
        	            	   if (debug) printf("\nPASO M");
        	            	   //wait for a response
        	                if(strncmp(buf,"150",3)==0)
        	                {  //make sure its a 150 so we know the server got it all
        	                	if (debug) printf("\nPASO N");
        	                 if(ftpRecvResponse(sd,buf)==1)
        	                 {
        	                	 if (debug) printf("\nPASO O");
        	                  if(strncmp(buf,"226",3)==0)
        	                  {
        	                	  if (debug) printf("\nPASO P");
        	                	  return 1;
        	                  }
        	                 }
        	                }
        	               }
        	              }
        	             }
        	            }
        	           }
        	          }
        	}
        }
       }
      }
     }
    }
   }
   return 0;
}

//Funcion que se descarga un fichero del hostname y puerto indicados, con el usuario y password indicados
int ftpGetProcess(char *hostname, int port, char *username, char *password, char *file, char *filename) {
   int sd;
   struct sockaddr_in pin;
   struct hostent *hp;
   char tmpHost[100];
   int tmpPort;
   char buf[5120];

   //DATA
   int dataSock;
   struct sockaddr_in dataServAddr;  /* data address*/
   unsigned short dataServPort;
   int numberOfConn;



   if ((hp = gethostbyname(hostname)) == 0) {
      perror("gethostbyname");
      return -1;
   }

   memset(&pin, 0, sizeof(pin));
   pin.sin_family = AF_INET;
   pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   pin.sin_port = htons(port);

   //Socket for port 21
   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket 21");
      return -1;
   }

   if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
         perror("connect");
         return -1;
   }
/*
   ////////////////////////
   //Socket for port 20
   if ((dataSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket 20");
      return -1;
   }

   dataServPort = 20;

   memset(&dataServAddr, 0, sizeof(dataServAddr));
   dataServAddr.sin_family         = AF_INET;
   dataServAddr.sin_addr.s_addr    = htonl(INADDR_ANY);
   dataServAddr.sin_port           = htons(dataServPort);


   bind(dataSock,(struct sockaddr *) &dataServAddr, sizeof(dataServAddr));


   listen(dataSock, numberOfConn);
*/

   if(ftpRecvResponse(sd,buf)==1)
   {  //wait for ftp server to start talking
    if(strncmp(buf,"220",3)==0)
    {  //make sure it ends with a 220
     if(ftpNewCmd(sd,buf,"USER",username)==1)
     {  //issue the command to login
      if(ftpRecvResponse(sd,buf)==1)
      {  //wait for response
       if(strncmp(buf,"331",3)==0)
       {  //make sure response is a 331
        if(ftpNewCmd(sd,buf,"PASS",password)==1)
        {
        	if(ftpRecvResponse(sd,buf)==1)
        	{ //wait for response
        	  if(strncmp(buf,"230",3)==0)
        	  { //make sure its a 230
        	    if(ftpNewCmd(sd,buf,"EPSV","2")==1)
        	    {  //notify server we want to pass a file
        	       if(ftpRecvResponse(sd,buf)==1)
        	       {  //wait for response
        	          if(strncmp(buf,"229",3)==0)
        	          {  //make sure it starts with a 229


        	              ftpConvertAddy(buf,tmpHost,&tmpPort); //then convert the return to usable data
        	           //   printf("\nHABRIA DEVUELTO %s", buf);
        	           //   printf("\nY ESTO SIGNIFICARIA HOST: %s, PORT:%d", buf, tmpHost, tmpPort);

        	              ////////////////////////////////////////////////
        	              //NUEVA CONEXION DE DATOS
        	              int sd2;
        	              struct sockaddr_in pin;
        	              struct hostent *hp;
        	              char *buf;
        	              char buffer[256];

        	              if ((hp = gethostbyname(tmpHost)) == 0) {
        	                    perror("gethostbyname");
        	                    return -1;
        	              }

        	              memset(&pin, 0, sizeof(pin));
        	              pin.sin_family = AF_INET;
        	              pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
        	              pin.sin_port = htons(tmpPort);

        	              if ((sd2 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        	              {
        	                    perror("socket port 20");
        	                    return -1;
        	              }

        	              if (connect(sd2,(struct sockaddr *)  &pin, sizeof(pin)) == -1)
        	              {
        	                    perror("Error while connecting.");
        	                    return -1;
        	                 }
        	                 ////////////////////////////////////////////////

        	              if(ftpNewCmd(sd,buf,"RETR",filename)==1) {  //set the file name AND
        	               if(ftpRecvResponse(sd,buf)==1) {        //wait for a response
        	                if(strncmp(buf,"150",3)==0) {  //make sure its a 150 so we know the server got it all
        	                	ftpGetFile(file,sd2);
        	                 if(ftpRecvResponse(sd,buf)==1) {
        	                  if(strncmp(buf,"226",3)==0) {
                	              if(ftpNewCmd(sd,buf,"QUIT","")==1) {  //set the file name AND
                	            	  return 1;
                	              }
        	                  }
        	                 }
        	                }
        	               }
        	              }
        	             }
        	            }
        	           }
        	          }
        	}
        }
       }
      }
     }
    }
   }
   return 0;
}

int ftpGetFile(char * filename, int sd) {
	//Cogido de https://www.linuxquestions.org/questions/programming-9/tcp-file-transfer-in-c-with-socket-server-client-on-linux-help-with-code-4175413995/

   char revbuf[512];
   FILE *pf;
   unsigned long fsize;
   pf = fopen(filename, "wb");
   if (pf == NULL)
   {
       printf("File not found!\n");
       return 1;
   }
   else
   {
	   bzero(revbuf, 512);
	   int block_sz = 0;
	   while((block_sz = recv(sd, revbuf, 512, 0)) >= 0)
	   {
			int write_sz = fwrite(revbuf, sizeof(char), block_sz, pf);

			if(write_sz < block_sz)
			{
				perror("File write failed.\n");
			}

			bzero(revbuf, 512);

			if (block_sz == 0 || block_sz != 512)
			{
							break;
			}
	   }

	   if(block_sz < 0)
	   {
		   if (errno == EAGAIN)
		   {
		   				printf("recv() timed out.\n");
		   }
		   else
		   {
		   				fprintf(stderr, "recv() failed due to errno = %d\n", errno);
		   }
	   }

	   printf("Ok received from server!\n");

	   fclose(pf);
   }

   close(sd); //close the socket
   return 0;
}
