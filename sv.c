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

int show_account(char*, struct thData, int);
void output_sender(char*, struct thData);
void list_accounts(char*, struct thData);
int register_user(char*, struct thData);
int edit_account(char*, struct thData);
int add_account(char*, struct thData);
int login_user(char*, struct thData);
void list_categories(struct thData);
static void *treat(void *);
void respond(void *);
int finder(char*);

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
  server.sin_addr.s_addr = htonl (INADDR_ANY);   /* any addres is accepted */  
  server.sin_port = htons (PORT);   			 /* host byte order to network byte order. */
  
  /* attaching the socket */

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
        respond((struct thData*)arg);

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

void respond(void *arg)
{
    char* msg = (char*)malloc(BUFF_SIZE);
  	struct thData tdL; 
  	tdL= *((struct thData*)arg);

  	if (read (tdL.cl, msg, BUFF_SIZE) <= 0)
  	{

  	   printf("[Thread %d]\n",tdL.idThread);
  	   perror ("Eroare la read() de la client.\n");
  			
  	}

  	/* here i treat every comand. If invalid command is given, nothing will be done (client receives warning) */

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

	  	if (register_data == NULL || strlen(register_data) < 2)
	  	{
	  		output_sender("Invalid register data provided!", tdL);
	  	}
	  	else if (register_user(register_data, tdL) == -1)
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

	  	if (login_data == NULL || strlen(login_data) < 2)
	  	{
	  		output_sender("Invalid login data provided!", tdL);
	  		login_data = (char*)malloc(BUFF_SIZE);
	  	}
	  	else if (login_user(login_data, tdL) == -1)
	  	{
	  		printf("[debug] login_user() failed! Wrong syntax!\n");
	  		login_data = (char*)malloc(BUFF_SIZE);
	  	}
    }
    else if (strncmp(msg, "add_account", 11) == 0 && tdL.is_logged_in != -1)
    {
    	printf ("[Thread %d] add_account() called\n",tdL.idThread);
    	char* acc_data = (char*)malloc(BUFF_SIZE);

	  	acc_data = strtok(msg, ":");
	  	acc_data = strtok(NULL, ":");

	  	if (acc_data == NULL || strlen(acc_data) < 2)
	  	{
	  		output_sender("Invalid account data provided!", tdL);
	  		char* acc_data = (char*)malloc(BUFF_SIZE);
	  	}
	  	else if (add_account(acc_data, tdL) == -1)
	  	{
	  		printf("[debug] add_account() failed! Wrong syntax!\n");
	  		char* acc_data = (char*)malloc(BUFF_SIZE);
	  	}
    }
    else if (strncmp(msg, "show_account", 12) == 0 && tdL.is_logged_in != -1)
    {
    	printf ("[Thread %d] show_account() called\n",tdL.idThread);
    	char* acc_data = (char*)malloc(BUFF_SIZE);

	  	acc_data = strtok(msg, ":");
	  	acc_data = strtok(NULL, ":");

	  	if (acc_data == NULL || strlen(acc_data) < 2)
	  	{
	  		output_sender("Invalid account data provided!", tdL);
	  		char* acc_data = (char*)malloc(BUFF_SIZE);
	  	}
	  	else if (show_account(acc_data, tdL, 0) == -1)
	  	{
	  		printf("[debug] show_account() failed! Wrong syntax!\n");
	  		char* acc_data = (char*)malloc(BUFF_SIZE);
	  	}
    }
    else if (strncmp(msg, "edit_account", 12) == 0 && tdL.is_logged_in != -1)
    {
    	printf ("[Thread %d] edit_account() called\n",tdL.idThread);
    	char* acc_data = (char*)malloc(BUFF_SIZE);

	  	acc_data = strtok(msg, ":");
	  	acc_data = strtok(NULL, ":");

	  	if (acc_data == NULL || strlen(acc_data) < 2)
	  	{
	  		output_sender("Invalid account data provided!", tdL);
	  		char* acc_data = (char*)malloc(BUFF_SIZE);
	  	}
	  	else if (edit_account(acc_data, tdL) == -1)
	  	{
	  		printf("[debug] show_account() failed! Wrong syntax!\n");
	  		char* acc_data = (char*)malloc(BUFF_SIZE);
	  	}
    }
    else if (strncmp(msg, "list_categories", 15) == 0 && tdL.is_logged_in != -1)
    {
    	printf ("[Thread %d] list_categories() called\n",tdL.idThread);
    	list_categories(tdL);
    }
    else if (strncmp(msg, "list_accounts", 13) == 0 && tdL.is_logged_in != -1)
    {
    	printf ("[Thread %d] list_accounts() called\n",tdL.idThread);
    	char* p = (char*)malloc(BUFF_SIZE);

	  	p = strtok(msg, ":");
	  	p = strtok(NULL, ":");
	  	char* categ = (char*)malloc(strlen(p)-1);
	  	strncpy(categ, p, strlen(p)-1);

    	list_accounts(categ, tdL);
    }
    else
    {	
    	output_sender("Invalid command! Review the help menu!", tdL);
    	msg = (char*)malloc(BUFF_SIZE);
    }
}

int finder(char* string)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strncmp(string, dir->d_name, strlen(dir->d_name)) == 0)
            {
            	return 1;
            }
        }
        closedir(d);
    }
    return 0;
}

void encrypt(char* string)
{
	for (int i=0; i<strlen(string); i++)
	{
		string[i] = string[i] + 100;
	}
}

void decrypt(char* string)
{
	for (int i=0; i<strlen(string); i++)
	{
		string[i] = string[i] - 100;
	}
}

void output_sender(char* string, struct thData tdL)
{
	if (write (tdL.cl, string, BUFF_SIZE) <= 0)
	{
	  	printf("[Thread %d] ",tdL.idThread);
	  	perror ("[Thread] Eroare la write() catre client.\n");
	}
	else
	{
	  	printf ("[Thread %d] Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
	}
}


int register_user(char* string, struct thData tdL)
{
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

	if (p == NULL || strlen(p) < 2)
	{
		output_sender("Wrong data provided! Try again: register:uname;passwd", tdL);
		return -1;
	}

	strcpy(uname, p);

	if (finder(uname) == 1)
	{
		output_sender("User already registered! Login or try another username!", tdL);
		return -1;
	}

	p = strtok(NULL, ";");

	if (p == NULL || strlen(p) < 2)
	{
		output_sender("Wrong data provided! Try again: register:uname;passwd", tdL);
		return -1;
	}

	strcpy(passwd, p);

	strcat(response, "You are now registered ");
	strcat(response, uname);

	output_sender(response, tdL);

	/* Creating the env for the user - a directory for each one which will contain all the data */

	struct stat st = {0};

	if (stat(uname, &st) == -1) 
	{
	    mkdir(uname, 0700);
	}

	chdir(uname);

	int fd = open(".login_data.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);

	/* generating login-data file */

	char* buffer = (char*)malloc(BUFF_SIZE);
	strcpy(buffer, "username:");
	strcat(buffer, uname);
	strcat(buffer, "\n");
	strcat(buffer, "password:");
	strcat(buffer, passwd);
	strcat(buffer, "\0");
	encrypt(buffer);

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
	char* response = (char*)malloc(BUFF_SIZE);
	char* uname = (char*)malloc(BUFF_SIZE);
	char* passwd = (char*)malloc(BUFF_SIZE);
	char* p = (char*)malloc(BUFF_SIZE);

	/* parsing the input */

	p = strtok(string, ";");

	if (p == NULL || strlen(p) < 2)
	{
		output_sender("Wrong data provided! Try again: login:uname;passwd", tdL);
		return -1;
	}

	strcpy(uname, p);

	p = strtok(NULL, ";");

	if (p == NULL || strlen(p) < 2)
	{
		output_sender("Wrong data provided! Try again: login:uname;passwd", tdL);
		return -1;
	}

	strcpy(passwd, p);

	/* Here is the login part after all the data was correctly parsed */

	if (chdir(uname) == -1)
	{
		output_sender("Wrong username provided! Try again with a valid username or register first!", tdL);
		return -1;
	}

	int fd = open(".login_data.txt", O_RDONLY);

	char* login_data_buff = (char*)malloc(BUFF_SIZE);

	if (read(fd, login_data_buff, BUFF_SIZE) <= 0)
	{
		printf("[Thread %d]\n",tdL.idThread);
  	    perror ("Eroare la read() din fisierul de login_data.\n");
	}

	/* Extracting the passwd from login_data.txt - used for checking if the input data is correct */

	decrypt(login_data_buff);
	p = strtok(login_data_buff, ":");
	p = strtok(NULL, ":");
	p = strtok(NULL, ":");

	/* ----------------------------------- */

	if (strncmp(p, passwd, strlen(passwd)) != 0)
	{
		output_sender("Wrong password provided! Try again with a valid password!", tdL);
		return -1;
	}
	else
	{
		tdL.is_logged_in = tdL.idThread + 1;
		strcat(response, "You are now logged in! ");
		strcat(response, uname);
		output_sender(response, tdL);
	}

	close(fd);

	return 0;
}

int add_account(char* string, struct thData tdL)
{
	char* p = (char*)malloc(BUFF_SIZE);
	char* buffer = (char*)malloc(BUFF_SIZE); 	/* used for formating the account_data file */
	char* response = (char*)malloc(BUFF_SIZE);

	/* categ will be used to know where the new acc will be saved (directory name) */

	p = strtok(string, ",");

	if (p == NULL || strlen(p) < 2)
	{
		output_sender("Category name is invalid! Please try again!", tdL);
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
		output_sender("Title of the file is invalid! Please try again!", tdL);
		return -1;
	}

	/* we create the file that will hold the acc_data */

	int fd = open(p, O_WRONLY | O_APPEND | O_CREAT, 0644);

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
	encrypt(buffer);

	if (write(fd, buffer, strlen(buffer)) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread] Eroare la write() in fisierul cu account_data.\n");
	}

	output_sender("Everything was parsed fine!", tdL);
	chdir("..");
	close(fd);

	return 0;
}

int show_account(char* string, struct thData tdL, int if_used)
{
	/* parsing the input */

	char* response = (char*)malloc(BUFF_SIZE);
	char* p = (char*)malloc(BUFF_SIZE);
	p = strtok(string, ";");

	if (chdir(p) == -1)
	{
		output_sender("This category doesn't exists! Please try again!", tdL);
		return -1;
	}

	p = strtok(NULL, ";");

	char* title = (char*)malloc(strlen(p)-1);

	strncpy(title, p, strlen(p)-1); /* somehow if i didn't explicitly copied this, it did't work */

	if (finder(title) == 0)
	{
		output_sender("This title file doesn't exists! Please try again!", tdL);
		return -1;
	}

	int fd = open(title, O_RDONLY);

	if (fd < 1)
	{
		printf("[Thread %d]\n",tdL.idThread);
  	    perror ("Eroare la open() a fisierului account_data, bad file descriptor!.\n");
	}

	char* buffer = (char*)malloc(BUFF_SIZE);

	if (read(fd, buffer, BUFF_SIZE) <= 0)
	{
		printf("[Thread %d]\n",tdL.idThread);
  	    perror ("Eroare la read() din fisierul de account_data.\n");
	}

	decrypt(buffer);

	if (if_used == 1)
	{
		strcat(buffer, "What do you want to modify?");
		strcat(buffer, "\n");
		strcat(buffer, "Username, Password, URL, Notes");
		strcat(buffer, "\n");
		strcat(buffer, "(Remember: Case sensitive!)");
	}
	else
	{
		chdir("..");
	}

	output_sender(buffer, tdL);
	close(fd);

	return 0;
}

int edit_account(char* string, struct thData tdL)
{
	char* modification = (char*)malloc(BUFF_SIZE);
	char* clone = (char*)malloc(BUFF_SIZE);
	strcpy(clone, string);

	/* obtaining file name */

	char* pp = (char*)malloc(BUFF_SIZE);
	pp = strtok(clone, ";");
	pp = strtok(NULL, ";");
	char* title = (char*)malloc(strlen(pp)-1);
	strncpy(title, pp, strlen(pp)-1); 

	/* ------------------ */

	/* showing the file in the terminal for client */

	if (show_account(string, tdL, 1) == -1)
	{
	  	printf("[debug] show_account() failed! Wrong syntax!\n");
	}

	/* ------------------------------------------ */

	char* p = (char*)malloc(BUFF_SIZE);

	/* asking user for what modification (s)he wants to do - field:modified content*/

  	if (read (tdL.cl, modification, BUFF_SIZE) <= 0)
  	{

  	   printf("[Thread %d]\n",tdL.idThread);
  	   perror ("Eroare la read() de la client.\n");
  			
  	}

  	p = strtok(modification, ":");

	if (p == NULL || strlen(p) < 2)
	{
		output_sender("Field is invalid! Please try again!", tdL);
		return -1;
	}

	char* field = (char*)malloc(strlen(p));
	strcpy(field, p);

	p = strtok(NULL, ":");

	if (p == NULL || strlen(p) < 2)
	{
		output_sender("Modification is invalid! Please try again!", tdL);
		return -1;
	}

	char* content = (char*)malloc(strlen(p));
	strcpy(content, p);

	/* ------------------------------------- */

	int ff1 = open(title, O_RDONLY);

	char* file_content = (char*)malloc(BUFF_SIZE);
	char* modified_file_content = (char*)malloc(BUFF_SIZE);
	strcpy(modified_file_content, "This file was modified!\n");

	if (read (ff1, file_content, BUFF_SIZE) <= 0)
  	{

  	   printf("[Thread %d]\n",tdL.idThread);
  	   perror ("Eroare la read() din acc_data file.\n");
  			
  	}

  	decrypt(file_content);

  	char *p_modifier = (char*)malloc(BUFF_SIZE);

  	p_modifier = strtok(file_content, "\n");

  	while (p_modifier != NULL)
  	{
  		if (strncmp(p_modifier, field, strlen(field)) == 0)
  		{
  			char* new_line = (char*)malloc(BUFF_SIZE);
  			strcpy(new_line, field);
  			strcat(new_line, ": ");
  			strcat(new_line, content);
  			strcat(modified_file_content, new_line);
  		}
  		else
  		{
	  		strcat(modified_file_content, p_modifier);
	  		strcat(modified_file_content, "\n");
	  	}
	  	p_modifier = strtok(NULL, "\n");
  	}

  	close(ff1);
  	int ff2 = open(title, O_CREAT | O_TRUNC | O_WRONLY, 0644);

  	encrypt(modified_file_content);

  	if (write(ff2, modified_file_content, strlen(modified_file_content)) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread] Eroare la write() in fisierul login_data.txt.\n");
	}

	output_sender("SUCCES on getting the data!", tdL);

	close(ff2);
	chdir("..");
	return 0;
}

void list_accounts(char* string, struct thData tdL)
{
	char* output = (char*)malloc(BUFF_SIZE);
	strcpy(output, "Accounts:\n");
	DIR *d;
    struct dirent *dir;
    d = opendir(string);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
        	if (strncmp(dir->d_name, ".", 1) != 0)
        	{
        		strcat(output, dir->d_name);
        		strcat(output, "\n");
			}
        }
        closedir(d);
    }
    output_sender(output, tdL);
}

void list_categories(struct thData tdL)
{
	char* output = (char*)malloc(BUFF_SIZE);
	strcpy(output, "Categories:\n");
	DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
        	if (strncmp(dir->d_name, ".", 1) != 0)
        	{
        		strcat(output, dir->d_name);
        		strcat(output, "\n");
			}
        }
        closedir(d);
    }
    output_sender(output, tdL);
}