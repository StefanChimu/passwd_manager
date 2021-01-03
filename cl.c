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

/* default size for input/ output */

#define BUFF_SIZE 2048

/* Declarations */

extern int errno;
int port;

/* ----------- */

/* Func prototypes */

void help_menu();

/* --------------- */

int main (int argc, char *argv[])
{

  /* Declarations */

  char* buf = (char*)malloc(BUFF_SIZE); /* input buffer - which will be send to the server */
  char* r = (char*)malloc(BUFF_SIZE); 	/* what the server returns */
  struct sockaddr_in server;			/* connect struct */
  int sd;								/* socket desc */

  /* ------------ */

  /* Veryfing calling syntax */
  
  if (argc != 3)
  {
      printf ("[error] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
  }
  
  port = atoi (argv[2]);

  /* creating the socket */
  
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
      perror ("[error] Eroare la socket().\n");
      return errno;
  }

  /* filling server struct */
  /* socket family */

  server.sin_family = AF_INET;
  
  /* server IP */
  
  server.sin_addr.s_addr = inet_addr(argv[1]);
  
  /* connection port */
  
  server.sin_port = htons (port);
  
  /* connecting to the server */

  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
  {
      perror ("[client] Eroare la connect().\n");
      return errno;
  }

  /* showing the avalible commands */

  help_menu();

  while (1)
  {
    /* taking the input from the client */

    printf ("[client] Introduceti mesajul: ");
    fflush (stdout);
    read (0, buf, BUFF_SIZE);

    if (strncmp(buf, "help", 4) == 0)
    {
    	help_menu();
    }
    else if (strncmp(buf, "quit", 4) == 0)
    {
        printf ("[client] Am primit comanda quit, iesim!\n");
        write (sd, buf, BUFF_SIZE);
        break;
    }
    else
    {
	    printf("[client] Am citit %s\n", buf);

	    /* sending input to the server */

	    if (write (sd, buf, BUFF_SIZE) <= 0)
	    {
	        perror ("[client] Eroare la write() spre server.\n");
	        return errno;
	    }

	    /* reading the output of the server */

	    if (read (sd, r, BUFF_SIZE) < 0)
	    {
	        perror ("[client] Eroare la read() de la server.\n");
	        return errno;
	    }

	    /* printing the output */

	    printf ("[client] Mesajul primit de la server este: %s\n", r);
	  }

	  	/* cleaning both input and output buffs */

		buf = (char*)malloc(BUFF_SIZE);
  		r = (char*)malloc(BUFF_SIZE);
    }

  close (sd);
}

void help_menu()
{
	printf("\n--- Welcome to the password_manager app! To start, enter one of the commands below: ---\n");
	printf("--- (Remember): To start using the app, you must register or log in first! ---\n");
	printf("--- Avalible commands: ---\n");
	printf("--- <register> <log in> <reset_password> <add_account> <show_accounts> <edit_account> ---\n");
	printf("--- Syntax of the commands: ---\n");
	printf("--- For <register> and <log in>: command:username;password ---\n");
	printf("--- For <reset_password>: command:username ---\n");
	printf("--- For <add_account>: command:category ---\n");
	printf("--- <add_acount> will create a file with the following fields (in this order): ---\n");
	printf("--- category, account title, username, password, url, notes. ---\n");
	printf("--- The input for <add_account> will be given in this order, every field being separated by a quote ---\n");
	printf("--- For <show_accounts>: command:category;title ---\n");
	printf("--- For <edit_account>: command:name_of_account ---\n\n");
}