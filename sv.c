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
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define PORT 3000      /* used port */
#define BUFF_SIZE 2048 /* generif buffer size used over all the application */

/* Declarations */

extern int errno;

typedef struct thData{
	int idThread;     
	int cl;           // desc returned by accept
	int is_logged_in; // idThread+1 in case login() was succesful (a must for calling other funcs)
}thData;

int quit_var = 0; // changes to 1 in case of quit() call

/* --------------*/

/* Func prototypes */

int register_user(char*, struct thData);
int add_account(char*, struct thData);
int login_user(char*, struct thData);
static void *treat(void *);
void raspunde(void *);
int find_user(char*);

/* --------------- */

int main ()
{
  struct sockaddr_in server;	/* server struct */ 
  struct sockaddr_in from;	
  pthread_t th[100];  
  int i=0;                      /* when a client connects to server, i increases */    
  int sd;

  /* creating a socket */

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
      perror ("[server] Eroare la socket().\n");
      return errno;
  }

  /* when a client disconects, the addres of it can be reused */

  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* initialing server struct */

  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* filling server struct */
  /* socket family */

  server.sin_family = AF_INET;	
  
  /* any addres is accepted */
  
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  
  /* converts the unsigned short integer hostshort from host byte order to network byte order. */
  
  server.sin_port = htons (PORT);
  
  /* attaching the socketul */

  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
  {
      perror ("[server] Eroare la bind().\n");
      return errno;
  }

  /* listening for new clients */

  if (listen (sd, 2) == -1)
  {
      perror ("[server] Eroare la listen().\n");
      return errno;
  }

  /* every client has a thread asigned */

  while (1)
  {
    int client;
    thData * td;
    int length = sizeof (from);

    printf ("[server] Asteptam la portul %d...\n",PORT);
    fflush (stdout);

    /* new client is accepted */

    if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
  	{
  	  perror ("[server] Eroare la accept().\n");
  	  continue;
  	}
  	
    /* waiting for client input */
    
  	td=(struct thData*)malloc(sizeof(struct thData));	
  	td->idThread=i++;
  	td->cl=client;

  	pthread_create(&th[i], NULL, &treat, td);	      
				
	}
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

        if (quit_var == 1)
        {
        	printf("[debug] Am dat de quit si am schimbat quit_var!\n");
            quit_var = 0;
            tdL.is_logged_in = -1;
            close ((intptr_t)arg);
            return(NULL); 
        }
  	}
};

void raspunde(void *arg)
{
    char* msg = (char*)malloc(BUFF_SIZE);
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
	else if (strncmp(msg, "register", 8) == 0)
    {
	  	printf ("[Thread %d] register() called\n",tdL.idThread);
	  		      
	  	char* register_data = (char*)malloc(BUFF_SIZE);

	  	register_data = strtok(msg, ":");
	  	register_data = strtok(NULL, ":");

	  	if (register_user(register_data, tdL) == -1)
	  	{
	  		printf("[debug] register_user() failed! Wrong syntax!\n");
	  	}
    }
    else if (strncmp(msg, "login", 5) == 0)
    {
	  	printf ("[Thread %d] login() called\n",tdL.idThread);
	  		      
	  	char* login_data = (char*)malloc(BUFF_SIZE);

	  	login_data = strtok(msg, ":");
	  	login_data = strtok(NULL, ":");

	  	if (login_user(login_data, tdL) == -1)
	  	{
	  		printf("[debug] login_user() failed! Wrong syntax!\n");
	  	}
    }
    else if (strncmp(msg, "add_account", 11) == 0 && tdL.is_logged_in != -1)
    {
    	printf ("[Thread %d] add_account() called\n",tdL.idThread);
    	char* acc_data = (char*)malloc(BUFF_SIZE);

	  	acc_data = strtok(msg, ":");
	  	acc_data = strtok(NULL, ":");

	  	if (add_account(acc_data, tdL) == -1)
	  	{
	  		printf("[debug] add_account) failed! Wrong syntax!\n");
	  	}
    }
    else
    {
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
}

int find_dir(char* string)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(string, dir->d_name) == 0)
            	return 1;
        }
        closedir(d);
    }
    return 0;
}

int register_user(char* string, struct thData tdL)
{
	/* input string: username;password */

	char* response = (char*)malloc(BUFF_SIZE);
	char* uname = (char*)malloc(BUFF_SIZE);
	char* passwd = (char*)malloc(BUFF_SIZE);
	char* p = (char*)malloc(BUFF_SIZE);

	/* parsing the input */

	if (tdL.idThread > 0)
	{
		chdir("..");
	}

	p = strtok(string, ";");

	if (p == NULL && strlen(p) < 3)
	{
		strcpy(response, "Wrong data provided! Try again: register:uname;passwd");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	strcpy(uname, p);

	if (find_dir(uname) == 1)
	{
		strcpy(response, "User already registered! Login or try another username!");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	p = strtok(NULL, ";");

	if (p == NULL && strlen(p) < 3)
	{
		strcpy(response, "Wrong data provided! Try again: register:uname;passwd");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	strcpy(passwd, p);

	strcat(response, "You are now registered ");
	strcat(response, uname);

	if (write (tdL.cl, response, BUFF_SIZE) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread] Eroare la write() catre client.\n");
	}
	else
	{
		printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
	}

	/* Creating the env for the user - a directory for each one which will contain all the data */

	struct stat st = {0};

	if (stat(uname, &st) == -1) 
	{
	    mkdir(uname, 0700);
	}

	chdir(uname);

	int fd = open("login_data.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);

	/* generating login-data file */

	char* buffer = (char*)malloc(BUFF_SIZE);
	strcpy(buffer, "username:");
	strcat(buffer, uname);
	strcat(buffer, "\n");
	strcat(buffer, "password:");
	strcat(buffer, passwd);
	strcat(buffer, "\0");

	/* --------------------------- */

	if (write(fd, buffer, strlen(buffer)) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread] Eroare la write() in fisierul login_data.txt.\n");
	}

	chdir("..");
	close(fd);

	return 0;

}

int login_user(char* string, struct thData tdL)
{
	/* input string: username;password */

	char* response = (char*)malloc(BUFF_SIZE);
	char* uname = (char*)malloc(BUFF_SIZE);
	char* passwd = (char*)malloc(BUFF_SIZE);
	char* p = (char*)malloc(BUFF_SIZE);

	/* parsing the input */

	p = strtok(string, ";");

	if (p == NULL && strlen(p) < 3)
	{
		strcpy(response, "Wrong data provided! Try again: login:uname;passwd");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	strcpy(uname, p);

	p = strtok(NULL, ";");

	if (p == NULL && strlen(p) < 3)
	{
		strcpy(response, "Wrong data provided! Try again: login:uname;passwd");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	strcpy(passwd, p);

	/* Here is the login part after all the data was correctly parsed */

	if (chdir(uname) == -1)
	{
		strcpy(response, "Wrong username provided! Try again with a valid username or register first!");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	int fd = open("login_data.txt", O_RDONLY);

	char* login_data_buff = (char*)malloc(BUFF_SIZE);

	if (read(fd, login_data_buff, BUFF_SIZE) <= 0)
	{
		printf("[Thread %d]\n",tdL.idThread);
  	    perror ("Eroare la read() din fisierul de login_data.\n");
	}

	/* Extracting the passwd from login_data.txt - used for checking if the input data is correct */

	p = strtok(login_data_buff, ":");
	p = strtok(NULL, ":");
	p = strtok(NULL, ":");

	/* ----------------------------------- */

	if (strncmp(p, passwd, strlen(passwd)) != 0)
	{
		strcpy(response, "Wrong password provided! Try again with a valid password!");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}
	else
	{
		tdL.is_logged_in = tdL.idThread + 1;
		strcat(response, "You are now logged in! ");
		strcat(response, uname);

		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}
	}

	close(fd);

	return 0;
}

int add_account(char* string, struct thData tdL)
{
	/* category, acount title, username, password, url, notes */

	char* p = (char*)malloc(BUFF_SIZE);
	char* buffer = (char*)malloc(BUFF_SIZE); /* this will be user for formating the account_data file */
	char* response = (char*)malloc(BUFF_SIZE);

	/* categ will be used to know where the new acc will be saved (directory name) */

	p = strtok(string, ",");

	if (p == NULL || strlen(p) < 2)
	{
		strcpy(response, "Category name is invalid! Please try again!");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	/* first we check if the category already exists */
	/* if it doesn't exists, we are going to create it */

	struct stat st = {0};

	if (stat(p, &st) == -1) 
	{
	    mkdir(p, 0700);
	}

	chdir(p);

	/* ---------------------------------------------------------- */

	/* title will be used to name the file which will contain the acc data */

	p = strtok(NULL, ",");

	if (p == NULL || strlen(p) < 2)
	{
		strcpy(response, "Title of the file is invalid! Please try again!");
		if (write (tdL.cl, response, BUFF_SIZE) <= 0)
		{
			printf("[Thread %d] ",tdL.idThread);
			perror ("[Thread] Eroare la write() catre client.\n");
		}
		else
		{
			printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
		}

		return -1;
	}

	/* we create the file that will hold the acc_data */

	char* filename = (char*)malloc(BUFF_SIZE);
	strcpy(filename, p);
	strcat(filename, ".txt");

	int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);

	/* ------------------------------------------------------------------- */

	p = strtok(NULL, ",");
	strcpy(buffer, "Username: ");
	strcat(buffer, p);
	strcat(buffer, "\n");
	p = strtok(NULL, ",");
	strcat(buffer, "Password: ");
	strcat(buffer, p);
	strcat(buffer, "\n");
	p = strtok(NULL, ",");
	strcat(buffer, "URL: ");
	strcat(buffer, p);
	strcat(buffer, "\n");
	p = strtok(NULL, ",");
	strcat(buffer, "Notes: ");
	strcat(buffer, p);
	strcat(buffer, "\n");

	if (write(fd, buffer, strlen(buffer)) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread] Eroare la write() in fisierul cu account_data.\n");
	}

	strcat(response, "Everything was parsed fine ");

	if (write (tdL.cl, response, BUFF_SIZE) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread] Eroare la write() catre client.\n");
	}
	else
	{
		printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
	}

	chdir("..");
	close(fd);

	return 0;
}