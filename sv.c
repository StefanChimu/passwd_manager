#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

/* portul folosit */

#define PORT 3000
#define BUFF_SIZE 1024

/* codul de eroare returnat de anumite apeluri */

extern int errno;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;

int quit_var = 0;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  int i=0;  

  /* crearea unui socket */

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
      perror ("[server] Eroare la socket().\n");
      return errno;
  }

  /* utilizarea optiunii SO_REUSEADDR */

  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */

  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */

  server.sin_family = AF_INET;	
  
  /* acceptam orice adresa */
  
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  
  /* utilizam un port utilizator */
  
  server.sin_port = htons (PORT);
  
  /* atasam socketul */

  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
  {
      perror ("[server] Eroare la bind().\n");
      return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */

  if (listen (sd, 2) == -1)
  {
      perror ("[server] Eroare la listen().\n");
      return errno;
  }

  /* servim in mod concurent clientii...folosind thread-uri */

  while (1)
  {
    int client;
    thData * td; //parametru functia executata de thread     
    int length = sizeof (from);

    printf ("[server] Asteptam la portul %d...\n",PORT);
    fflush (stdout);

    // client= malloc(sizeof(int));
    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */

    if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
  	{
  	  perror ("[server] Eroare la accept().\n");
  	  continue;
  	}
  	
    /* s-a realizat conexiunea, se astepta mesajul */
      
  	// int idThread; //id-ul threadului
  	// int cl; //descriptorul intors de accept

  	td=(struct thData*)malloc(sizeof(struct thData));	
  	td->idThread=i++;
  	td->cl=client;

  	pthread_create(&th[i], NULL, &treat, td);	      
				
	}//while    
};		

static void *treat(void * arg)
{		
    while (quit_var == 0)
    {
        struct thData tdL; 
        tdL= *((struct thData*)arg);  
        printf ("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
        fflush (stdout);     
        pthread_detach(pthread_self());   
        raspunde((struct thData*)arg);

        /* am terminat cu acest client, inchidem conexiunea */

        if (quit_var == 1)
        {
        	printf("[debug] Am dat de quit si am schimbat quit_var!\n");
            quit_var = 0;
            close ((intptr_t)arg);
            return(NULL); 
        }
  	}
};


void raspunde(void *arg)
{
    char* msg = (char*)malloc(BUFF_SIZE);;
  	struct thData tdL; 
  	tdL= *((struct thData*)arg);

  	if (read (tdL.cl, msg, BUFF_SIZE) <= 0)
  	{

  	   printf("[Thread %d]\n",tdL.idThread);
  	   perror ("Eroare la read() de la client.\n");
  			
  	}

    if (strncmp(msg, "quit", 4) == 0)
    {
        printf ("[Thread %d] Am primit comanda quit, iesim!\n", tdL.idThread);
        quit_var++;
        return;
    }

  	printf ("[Thread %d] Mesajul a fost receptionat...%s\n",tdL.idThread, msg);
  		      
  	/*pregatim mesajul de raspuns */
  	printf("[Thread %d] Trimitem mesajul inapoi...%s\n",tdL.idThread, msg);
  		      
  		      
  	/* returnam mesajul clientului */
  	if (write (tdL.cl, msg, BUFF_SIZE) <= 0)
  	{
  		 printf("[Thread %d] ",tdL.idThread);
  		 perror ("[Thread] Eroare la write() catre client.\n");
  	}
  	else
    {
  		printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
    }

}