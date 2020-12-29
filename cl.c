#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

/* default size pentru orice tip de input/ output */
#define BUFF_SIZE 1024

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
  char* buf = (char*)malloc(BUFF_SIZE);
  char* r = (char*)malloc(BUFF_SIZE); 	/* pentru response voi folosi char r 
  										pentru a se vedea clar ca se citeste raspunsul dat de sv*/
  struct sockaddr_in server;	// structura folosita pentru conectare 
  int sd;			// descriptorul de socket

  /* exista toate argumentele in linia de comanda? */
  
  if (argc != 3)
  {
      printf ("[error] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
  }

  /* stabilim portul */
  
  port = atoi (argv[2]);

  /* cream socketul */
  
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
      perror ("[error] Eroare la socket().\n");
      return errno;
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */

  server.sin_family = AF_INET;
  
  /* adresa IP a serverului */
  
  server.sin_addr.s_addr = inet_addr(argv[1]);
  
  /* portul de conectare */
  
  server.sin_port = htons (port);
  
  /* ne conectam la server */

  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
  {
      perror ("[client] Eroare la connect().\n");
      return errno;
  }

  while (1)
  {
    /* citirea mesajului */

    printf ("[client] Introduceti mesajul: ");
    fflush (stdout);
    read (0, buf, BUFF_SIZE);
    
    /* tratez intai cazul de quit */

    if (strncmp(buf, "quit", 4) == 0)
    {
        printf ("[client] Am primit comanda quit, iesim!\n");
        write (sd, buf, BUFF_SIZE);
        break;
    }
    else
    {
	    printf("[client] Am citit %s\n", buf);

	    /* trimiterea mesajului la server */

	    if (write (sd, buf, BUFF_SIZE) <= 0)
	    {
	        perror ("[client] Eroare la write() spre server.\n");
	        return errno;
	    }

	    /* citirea raspunsului dat de server 
	       (apel blocant pina cind serverul raspunde) */

	    if (read (sd, r, BUFF_SIZE) < 0)
	    {
	        perror ("[client] Eroare la read() de la server.\n");
	        return errno;
	    }

	    /* afisam mesajul primit */

	    printf ("[client] Mesajul primit este: %s\n", r);
	  }

		buf = (char*)malloc(BUFF_SIZE);
  		r = (char*)malloc(BUFF_SIZE);
    }

  /* inchidem conexiunea, am terminat */
  close (sd);
}