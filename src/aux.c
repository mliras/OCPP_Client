#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
//#include "client.h"
#include "aux.h"

int ipow(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

// reverses a string 'str' of length 'len'
void reverse(char *str, int len)
{
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}

 // Converts a given integer x to string str[].  d is the number
 // of digits required in output. If d is more than the number
 // of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

// Converts a floating point number to string.
void ftoa(float n, char *res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = intToStr(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * ipow(10, afterpoint);

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

int charCounter(char* pString, char c)
{
    int count = 0;
    char* pTemp = pString;

    while(pTemp != NULL)
    {
        pTemp = strchr(pTemp, c);
        if( pTemp ) {
            pTemp++;
            count++;
        }
    }

    return count;
}

char * replace(char const * const original,char const * const pattern,char const * const replacement)
{
  size_t const replen = strlen(replacement);
  size_t const patlen = strlen(pattern);
  size_t const orilen = strlen(original);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
  {
    patcnt++;
  }

  {
    // allocate memory for the new string
    size_t const retlen = orilen + patcnt * (replen - patlen);
    char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

    if (returned != NULL)
    {
      // copy the original string,
      // replacing all the instances of the pattern
      char * retptr = returned;
      for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
      {
        size_t const skplen = patloc - oriptr;
        // copy the section until the occurence of the pattern
        strncpy(retptr, oriptr, skplen);
        retptr += skplen;
        // copy the replacement
        strncpy(retptr, replacement, replen);
        retptr += replen;
      }
      // copy the rest of the string.
      strcpy(retptr, oriptr);
    }
    return returned;
  }
}

char * strlwr(char * s)
{
        char *t = s;

        if (!s)
        {
                return 0;
        }

        int i = 0;
        while ( *t != '\0' )
        {
                if (*t >= 'A' && *t <= 'Z' )
                {
                        *t = *t + ('a' - 'A');
                }
                t++;
        }

        return s;
}


//Returns a copy of the string in lower letters. The String remains unmodified
char * strlwr_ex(char * s)
{
        if (!s)
        {
                return 0;
        }


        char *t, *u;
        t=strdup(s);
        u=t;
        int i = 0;
        while ( *t != '\0' )
        {
                if (*t >= 'A' && *t <= 'Z' )
                {
                        *t = *t + ('a' - 'A');
                }
                t++;
        }

        return u;
}

void toLowerCase(char *p)
{
	for ( ; *p; ++p) *p = tolower(*p);
}


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = calloc(1, sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

char *convert(int i)
{
	char *b=(char *)calloc(1, sizeof(char) * 6);

	int digitos=1;
	int temp=10;

	while ((i/temp)>0){
		digitos++;
		temp*=10;
	}

	temp/=10;

	for (int k=0; k<digitos; k++){
		b[k]=48+(i/temp);
		i=i%temp;
		temp/=10;
	}

	b[digitos]='\0';

	return b;
}



void writeLogToFile(char *texto, enum log_levels warning_level)
{
	if (!texto || !logfile ||warning_level==NO_LOG) return;

	fwrite(texto , 1 , strlen(texto) , logfile);
	fwrite("\n", 1 , 2, logfile);
	fflush(logfile); //write modifcations to disk
}

////////////////////////////////////////////////////////////////////////////////
//  MESSAGE QUEUE
////////////////////////////////////////////////////////////////////////////////
//
//Desencola el primer elemento de la cola
void Dequeue()
{
		queue_node *p=Message_queue;

		while (mutex) usleep(10);
		mutex=1;
		// Cola vacia
        if(!Message_queue)
        {
                addLog("Queue is Empty. Don't call dequeue!!", NULL,NULL, LOG_INFO, FICHERO);
                //return;
        }
        else
        {
        	//Lo quita del principio
       		Message_queue=p->next;
       		if (p->payload)
       		{
       			addLog("Dequeing %s", p->payload, NULL, LOG_DEBUG, FICHERO);
       			free(p->payload);
       		}

       		p->payload=NULL;
    		if (p) free(p);
//NUEVAS
    		p=NULL;
        }
        mutex=0;
}

//Encola un nuevo mensaje al final de la cola
void Enqueue(queue_node *new_node)
{
		queue_node *p=Message_queue;
		queue_node *r=new_node;

		//printf("ENQUEING %s", new_node->payload);
		while (mutex) usleep(100);
		mutex=1;

		//Si la cola esta vacia, hago que Message_queue apunte al nuevo nodo y borro el que hacia que estuviera vacio.
		if (!Message_queue)
		{
			Message_queue=r;
			Message_queue->next=NULL;
		}
		else
		{
			//Lo añade al final
			while (p->next!=NULL) p=p->next;
			p->next=r;
			r->next=NULL;
		}
		mutex=0;
}

//Busca un mensaje con el UniqueId que le indicamos y nos devuelve el action Code asociado.
int getActionFromUniqueId(const char* UniqueId_str)
{
	queue_node *p=Message_queue;
	int n=atoi(UniqueId_str);

	while (mutex) usleep(100);
	mutex=1;

	while (p && (n != p->UniqueId))
	{
		p=p->next;
	}

	mutex=0;
	if (p)
		return p->MessageAction;
	else
		return p;
}

//Comprueba si existe un mensaje con dicho UniqueId y si lo hay devuelve la entrada del message queue y si no, devuelve NULL
queue_node * checkMessageinMessageQueue(int uniqueId)
{
	queue_node *p=Message_queue;
	while (mutex) usleep(100);
	mutex=1;

	while (p && (uniqueId != p->UniqueId))
	{
			p=p->next;
	}

	mutex=0;

	return p;
}

char *encrypt(char *text, int last)
{
	printf("%s", text);
	if (!text) return NULL;
//	printf("strlen de texto es %d", strlen(text));
//	printf("NO ES NULL");
	char *temp=(char *)calloc(1,sizeof(char)*strlen(text));

	char secret1[4] = { 53, 44, 71, 66};
	int i;
	char secret2[4] = { 177, 253, 122, last};

	for(i = 0; i < strlen(text); i++)
	{
	   	if ((i%8)<4)
		{
	    		temp[i] = (text[i] ^ secret1[i%4])+secret2[i%4];
	    		if (temp[i]>=0) temp[i]++;//printf("\n%d", temp[i]);
	   	}
	   	else
	   	{
	    		temp[i] = (text[i] ^ secret2[i%4])+secret1[i%4];
	    		if (temp[i]>=0) temp[i]++;//printf("\n%d", temp[i]);
	   	}
	 }
	 temp[i]='\0';

	 return temp;
}

char *decrypt(char *text, int last)
{
	if (!text) return NULL;

	char *temp=(char *)calloc(1,sizeof(char)*strlen(text));

	char secret1[4] = { 53, 44, 71, 66};
	int i;
	char secret2[4] = { 177, 253, 122, last};

	    for(i = 0; i < strlen(text); i++)
	    {
	    	if ((i%8)<4)
			{
	    		//if (text[i]>0) text[i]--;
	    		int tmp;
	    //		printf("%d", text[i]);
	    		tmp=text[i];
	    		if (text[i]>0) tmp--;

	    		temp[i] = (tmp - secret2[i%4]) ^ secret1[i%4];
	    	}
	    	else
	    	{
	    		int tmp;
	    	//	printf("%d", text[i]);
	    		tmp=text[i];
	    		if (text[i]>0) tmp--;
	    		//else tmp=text[i];
	    		temp[i] = (tmp - secret1[i%4]) ^ secret2[i%4];
	    	}
	    }
	 temp[i]='\0';

	// printf("Antes de salir nos sale %s", temp);
	 return temp;
}

char *convertF(float myFloat)
{
	char *b=(char *)calloc(64,sizeof(char));
	//char buffer[64];
	//for (int i=0; i<64; i++) buffer[i]='\0';

	int ret = snprintf(b, 64, "%.1f", myFloat);

	if (ret < 0) {
	    return NULL;
	}

	//char *b=(char *)calloc(strlen(buffer)+1,sizeof(char));
	//strncpy(b, buffer, strlen(buffer));

	return b;
}

int isThereAMessageOfType(int Action)
{
	queue_node *p=Message_queue;
	while (mutex) usleep(100);
	mutex=1;

	while (p && p->MessageAction!=Action) p=p->next;

	mutex=0;
	if (!p) return 0;
	else return 1;
}

char *getPayloadFromMessage(int uniqueId)
{
	queue_node *p=Message_queue;
	while (mutex) usleep(100);
	mutex=1;

	while (p && (uniqueId != p->UniqueId))
	{
			p=p->next;
	}


	mutex=0;

	if (p) return p->payload;

	return NULL;
}

//Desencola el elemento con el uniqueId indicado. Si no lo encuentra... no lo desencola
void Dequeue_id(int uniqueId)
{
	queue_node *p=Message_queue;
	queue_node *anterior=Message_queue;

	while (mutex) usleep(100);
	mutex=1;

	 if(!Message_queue)
     {
	      addLog("Queue is Empty\n", NULL,NULL,LOG_INFO, ANY);
	      //return;
	 }
	 else
	 {
		while (p && (uniqueId != p->UniqueId))
		{
				anterior=p;
				p=p->next;
		}

		if (p)
		{
			anterior->next=p->next;

			if (p->payload) free(p->payload);
//			p->payload=NULL;
			if (p) free(p);

			//NUEVAS
//    		p=NULL;
		}

	 }

	mutex=0;
}

//////////////////////////////////////////////////////////////////////////////

char * getExpiryDateFromCurrentTime()
{
		time_t now;
		struct tm* now_tm;
		now_tm = localtime(&now);

		//Da un tiempo de una hora de reserva
		now_tm->tm_hour+=1;

		char *oneHourPlus=(char *)calloc(1, sizeof(char)*80);
		strftime (oneHourPlus, 80, "%Y-%m-%dT%H:%M:%S.", now_tm);
		oneHourPlus[strlen(oneHourPlus)]='\0';
	    return oneHourPlus;
}

int checkIsNumber(char *tmp)
{
	int j=0;
	int isDigit=0;
	while(j<strlen(tmp)){
	  isDigit = isdigit(tmp[j]);
	  if (isDigit == 0) break;
	  j++;
	}

	return isDigit;
}

char *getRandomString(int length, int mins, int mays, int nums)
{
	int idTokenLength=length;
	char *idToken=(char *)calloc(1, sizeof(char) * idTokenLength);

	//Here we should prepare the idTag. It's not well explained neither in 4.8 nor in 6.45, nor in 7.28 sections

	int l=0;
	if (mins) l+=26;
	if (mays) l+=26;
	if (nums) l+=10;

	if (l>0)
	{
		char charset[l];
		if (mins) strcat(charset, "abcdefghijklmnopqrstuvwxyz");
		if (mays) strcat(charset, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		if (nums) strcat(charset, "0123456789");


		if (idToken) {
		 for (int n = 0;n < idTokenLength; n++) {
	    	int key = rand() % (int)(sizeof(charset) -1);
	        idToken[n] = charset[key];
		 }
	            idToken[idTokenLength] = '\0';
		}

	return idToken;
	}

	return NULL;
}


char *getSystemName()
{
struct addrinfo hints, *info, *p;
int gai_result;

char hostname[1024];
hostname[1023] = '\0';
gethostname(hostname, 1023);

memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_CANONNAME;

if ((gai_result = getaddrinfo(hostname, "http", &hints, &info)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_result));
    return NULL;
}

//freeaddrinfo(info);
if (info)
	return info->ai_canonname;
else return NULL;

}


