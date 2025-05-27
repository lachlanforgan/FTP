/* 
 * server FTP program
 *
 * NOTE: Starting homework #2, add more comments here describing the overall function
 * performed by server ftp program
 * This includes, the list of ftp commands processed by server ftp.
 *
 */

#include <stdio.h>
#include <unistd.h>

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <errno.h>

#define SERVER_FTP_PORT 8041
#define DATA_FTP_PORT 8042


/* Error and OK codes */
#define OK 0
#define ER_INVALID_HOST_NAME -1
#define ER_CREATE_SOCKET_FAILED -2
#define ER_BIND_FAILED -3
#define ER_CONNECT_FAILED -4
#define ER_SEND_FAILED -5
#define ER_RECEIVE_FAILED -6
#define ER_ACCEPT_FAILED -7
#define LOGGED_OUT -1
#define LOGGED_IN 5
const char *FILE_DNE = "no good\0";
const char *FILE_EXISTS = "good\0";
const char *EMPTY_MSG = "";


/* Function prototypes */

int svcInitServer(int *s);
int sendMessage (int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);
void processCommand(char *cmd, char *argument, int socket, char* replyMsg, int* loginStatus, char* directory[3][2]);
void initDirectory(char *array[3][2]);
int clntConnect(char *serverName, int *s);

/* List of all global variables */

char userCmd[1024];	/* user typed ftp command line received from client */
char cmd[1024];		/* ftp command (without argument) extracted from userCmd */
char argument[1024];	/* argument (without ftp command) extracted from userCmd */
char replyMsg[1024];       /* buffer to send reply message to client */
char fileBuffer[1024];


/*
 * main
 *
 * Function to listen for connection request from client
 * Receive ftp command one at a time from client
 * Process received command
 * Send a reply message to the client after processing the command with staus of
 * performing (completing) the command
 * On receiving QUIT ftp command, send reply to client and then close all sockets
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	0			- Successful execution until QUIT command from client 
 *	ER_ACCEPT_FAILED	- Accepting client connection request failed
 *	N			- Failed stauts, value of N depends on the command processed
 */

int main(	
	int argc,
	char *argv[]
	)
{
	/* List of local varibale */

	int msgSize;        /* Size of msg received in octets (bytes) */
	int listenSocket;   /* listening server ftp socket for client connect request */
	int ccSocket;        /* Control connection socket - to be used in all client communication */
	int status;
	int loginStatus = -1;	// Variable for controlling user access
	char* directory[3][2];
	/*
	 * NOTE: without \n at the end of format string in printf,
         * UNIX will buffer (not flush)
	 * output to display and you will not see it on monitor.
	*/
	printf("Started execution of server ftp\n");

	 /*initialize server ftp*/
	printf("Initialize ftp server\n");	/* changed text */

	status=svcInitServer(&listenSocket);
	if(status != 0)
	{
		printf("Exiting server ftp due to svcInitServer returned error\n");

	}
	initDirectory(directory);
	printf("ftp server is waiting to accept connection\n");

	/* wait until connection request comes from client ftp */
	ccSocket = accept(listenSocket, NULL, NULL);

	if(ccSocket < 0)
	{
		perror("cannot accept connection:");
		printf("Server ftp is terminating after closing listen socket.\n");
		close(listenSocket);  /* close listen socket */
		return (ER_ACCEPT_FAILED);  // error exist
	}
	
	printf("Connected to client, calling receiveMsg to get ftp cmd from client \n");

	/* Receive and process ftp commands from client until quit command.
 	 * On receiving quit command, send reply to client and 
         * then close the control connection socket "ccSocket". 
	 */
	do
	{
	    /* Receive client ftp commands until */
 	    status=receiveMessage(ccSocket, userCmd, sizeof(userCmd), &msgSize);
	    if(status < 0)
	    {
		printf("Receive message failed. Closing control connection \n");
		printf("Server ftp is terminating.\n");
		break;
	    }

	    /* Separate command and argument from userCmd */
	    strcpy(cmd, userCmd);
	    strcpy(argument, "");

	    char *token = strtok(cmd, " ");
	    if (token) 
	    {
		    strcpy(cmd, token);
		    token = strtok(NULL, " ");
		    if (token)
			    strcpy(argument, token);
	    }
	    processCommand(cmd, argument, ccSocket, replyMsg, &loginStatus, directory);

	    /*
 	     * ftp server sends only one reply message to the client for 
	     * each command received in this implementation.
	     */
	    status=sendMessage(ccSocket,replyMsg,strlen(replyMsg) + 1);	/* Added 1 to include NULL character in */
				/* the reply string strlen does not count NULL character */
	    if(status < 0)
	    {
		break;  /* exit while loop */
	    }
	    printf("\n");
	}
	while(strcmp(cmd, "quit") != 0);

	printf("Closing control connection socket.\n");
	close (ccSocket);  /* Close client control connection socket */

	printf("Closing listen socket.\n");
	close(listenSocket);  /*close listen socket */

	printf("Existing from server ftp main \n");

	return (status);
}

void processCommand(char *cmd, char *arg, int socket, char* replyMsg, int *loginStatus, char* directory[3][2])
{
	int status;
	if (*loginStatus != LOGGED_IN)
	{

		if (strcmp(cmd, "user") == 0)
		{
			for (int i = 0; i < 3; i++) // hard coding number of loop recurrances 
						    // because I know size of directory
			{
				if (strcmp(arg, directory[i][0]) == 0)
				{
					*loginStatus = i;
					strcpy(replyMsg, "331 User name okay, need password.\n");
					break;
				}
				else if (i == 2)
				{
					strcpy(replyMsg, "530 Invalid user.\n");
					*loginStatus = LOGGED_OUT;
				}
			}
		}
		else if (strcmp(cmd, "pass") == 0)
		{
			if (*loginStatus != LOGGED_OUT)
			{
				if (strcmp(arg, directory[*loginStatus][1]) == 0)
				{
					*loginStatus = LOGGED_IN;
					strcpy(replyMsg, "230 User logged in, process.\n");
				}
				else 
				{
					strcpy(replyMsg, "530 Login incorrect.\n");
				}
			}
			else 
			{
				strcpy(replyMsg, "530 No username given.\n");
			}
		}


		else if (strcmp(cmd, "stat") == 0)
		{
			 if (strcmp(arg, "") == 0)
                        {
                                FILE *fp;
                                long uptime = 0;

                                fp = fopen("/proc/uptime", "r");
                                if (fp != NULL)
                                {
                                    if (fscanf(fp, "%ld", &uptime) != 1)
                                    {
                                        snprintf(replyMsg, 1024, "500 Error reading uptime\r\n");
                                        fclose(fp);
                                        return; 
                                    }
                                    fclose(fp);
                                }

                                struct statvfs stat;
                                if (statvfs("/", &stat) != 0)
                                {
                                    snprintf(replyMsg, 1024, "500 Error retrieving disk space\r\n");
                                    return; 
                                }

                                long free_space = stat.f_bavail * stat.f_frsize;

                                char info[1024];
                                struct utsname sys_info;
                                if (uname(&sys_info) == 0)
				{
					snprintf(info, 1024, "%s %s %s", sys_info.sysname, sys_info.nodename, sys_info.release);
                                }
                                else
                                {
                                        snprintf(info, 1024, "Unknown system information");
                                }
                                long hours = uptime / 3600;
                                long minutes = (uptime % 3600) / 60;

                                snprintf(replyMsg, 1024, "200 Command okay.\r\nServer is up for %ld hours and %ld minutes.\r\n"
                                "Free disk space: %ld bytes.\r\nSystem info: %s %s %s\r\n", hours, minutes, free_space, sys_info.sysname, sys_info.nodename, sys_info.release);
			}
			else
			{
				strcpy(replyMsg, "550 Cannot open file. Must login first, or use stat cmd without argument.\n");
			}

		}
		else if (strcmp(cmd, "help") == 0)
		{
			strcpy(replyMsg,
				"214-The following FTP commands are recognized:\n"
				"USER <username>	- Send user name\n"
				"PASS <password>	- Send password\n"
				"MKDIR <dir>		- Create directory\n"
				"RMDIR <dir>		- Remove directory\n"
				"LS [dir]		- List contents of the current or specified directory\n"
				"CD <dir>		- Change the working directory\n"
				"PWD			- Print the current working directory\n"
				"DELE <file>		- Delete a file on the server\n"
				"STAT [file]		- Display server status and current directory\n"
				"QUIT			- Disconnect from the server\n"
				"HELP			- Display this help message\n"
				"214 End of help.\n"
		      );
		}
		else if (strcmp(cmd, "quit") == 0)
		{
			strcpy(replyMsg, "221 Goodbye");
			sendMessage(socket, replyMsg, strlen(replyMsg) + 1);
			return;
		}
		else
			strcpy(replyMsg, "530 Not yet logged in to user.\n");
	}
	else 
	{

		if (strcmp(cmd, "user") == 0)
		{
			strcpy(replyMsg, "530 Already logged in.\n");
		}
		else if (strcmp(cmd, "pass") == 0)
		{
			strcpy(replyMsg, "530 Already logged in.\n");
		}
		else if (strcmp(cmd, "mkdir") == 0)
		{
			if (strcmp(arg, "") == 0)
			{
				strcpy(replyMsg, "550 Failed to create directory.\nYou must specify a path after a mkdir command.\n");
			}
			else if (mkdir(arg, 0777) == 0)
			{
				strcpy(replyMsg, "257 Directory created\n");
			}
			else
			{
				perror("mkdir failed");
				strcpy(replyMsg, "550 Failed to create directory. ");
				strcat(replyMsg, strerror(errno));
				strcat(replyMsg, ".\n");
			}
		}
		else if (strcmp(cmd, "rmdir") == 0)
		{
			if (strcmp(arg, "") == 0)
			{
				strcpy(replyMsg, "550 Failed to remove directory.\nYou must specify a path after a rmdir command.\n");
			}
			else if (rmdir(arg) == 0)
			{
				strcpy(replyMsg, "250 Directory removed\n");
			}
			else
			{
				perror("rmdir failed");
				strcpy(replyMsg, "550 Failed to remove directory. ");
				strcat(replyMsg, strerror(errno));
				strcat(replyMsg, ".\n");
			}
		}
		else if (strcmp(cmd, "ls") == 0)
		{
			DIR *dir;
			struct dirent *entry;
			char listMsg[1024] = "200 Here are the directory contents:\n";
			if (strcmp(arg, "") == 0)
			{
				dir = opendir(".");
			}
			else
				dir = opendir(arg);
			if (dir)
			{
				int check = 0;
				while ((entry = readdir(dir)) != NULL)
				{
					if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
					{
						check += 1;
						strcat(listMsg, entry->d_name);
						strcat(listMsg, "\n");
					}
				}
				if (check <= 0)
				{
					strcat(listMsg, "\n");	// add space so that it is more evident
								// when directory is empty
				}
				closedir(dir);
				strcpy(replyMsg, listMsg);
				return;
			}
			else
			{
				perror("opendir failed");
				strcpy(replyMsg, "550 Requested action not taken: Unable to list directory\n");
			}
		}
		else if (strcmp(cmd, "cd") == 0)
		{
			if (strcmp(arg, "") == 0)
			{
				arg = getenv("HOME");
				if (arg == NULL)
				{
					strcpy(replyMsg, "550 Home directory not set.\n");
					return;
				}
				// navigate the working directory back to server directory
				strcat(arg, "/fall24/server");
			}
			if (chdir(arg) == 0)
			{
				strcpy(replyMsg, "250 Directory change\n");
			}
			else
			{
				perror("cd failed");
				strcpy(replyMsg, "550 Failed to change directory\n");
			}
		}
		else if (strcmp(cmd, "dele") == 0)
		{
			if (strcmp(arg, "") == 0)
			{
				strcpy(replyMsg, "550 Must specify file for deletion.\n");
			}
			else if (remove(arg) == 0)
			{
				strcpy(replyMsg, "250 File deleted\n");
			}
			else
			{
				perror("dele failed");
				strcpy(replyMsg, "550 Failed to delete file. ");
				strcat(replyMsg, strerror(errno));
				strcat(replyMsg, ".\n");
			}
		}
		else if (strcmp(cmd, "pwd") == 0)
		{
			char cwd[1024];
			if (getcwd(cwd, sizeof(cwd)) != NULL)
			{
				snprintf(replyMsg, 1024, "257 \"%s\"\n", cwd);
			}
			else
			{
				perror("pwd failed");
				strcpy(replyMsg, "550 Failed to get current directory\n");
			}
		}
		else if (strcmp(cmd, "stat") == 0)
		{
			if (strcmp(arg, "") == 0)
			{
				FILE *fp;
				long uptime = 0;

				fp = fopen("/proc/uptime", "r");
				if (fp != NULL)
				{
				    if (fscanf(fp, "%ld", &uptime) != 1)
				    {
					snprintf(replyMsg, 1024, "500 Error reading uptime\r\n");
					fclose(fp);
					return; // Handle error
				    }
				    fclose(fp);
				}

				struct statvfs stat;
				if (statvfs("/", &stat) != 0)
				{
				    snprintf(replyMsg, 1024, "500 Error retrieving disk space\r\n");
				    return; // Handle error
				}

				long free_space = stat.f_bavail * stat.f_frsize;

				char info[1024];
				struct utsname sys_info;
				if (uname(&sys_info) == 0) {
			    	snprintf(info, 1024, "%s %s %s", sys_info.sysname, sys_info.nodename, sys_info.release);
				}
				else
				{
			    		snprintf(info, 1024, "Unknown system information");
				}
				long hours = uptime / 3600;
				long minutes = (uptime % 3600) / 60;

				snprintf(replyMsg, 1024, "200 Command okay.\r\nServer is up for %ld hours and %ld minutes.\r\n"
                                "Free disk space: %ld bytes.\r\nSystem info: %s %s %s\r\n", hours, minutes, free_space, sys_info.sysname, sys_info.nodename, sys_info.release);
			}
			else 
			{
				struct stat file_stat;
				if (stat(arg, &file_stat) == 0)
				{
					struct tm *last_modified_time = localtime(&file_stat.st_mtime);
					char time_str[100];
					strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", last_modified_time);
					snprintf(replyMsg, 1024,
					"213 File Status: Size = %ld bytes, Last modified = %s\r\n",
					file_stat.st_size, time_str);
				}
				else 
				{
					strcpy(replyMsg, "550 File not found.\n");
				}
			}
		}
		else if (strcmp(cmd, "help") == 0)
		{
			strcpy(replyMsg,
				"214-The following FTP commands are recognized:\n"
				"USER <username>	- Send user name\n"
				"PASS <password>	- Send password\n"
				"MKDIR <dir>		- Create directory\n"
				"RMDIR <dir>		- Remove directory\n"
				"LS [dir]		- List contents of the current or specified directory\n"
				"CD <dir>		- Change the working directory\n"
				"PWD			- Print the current working directory\n"
				"DELE <file>		- Delete a file on the server\n"
				"STAT			- Display server status and current directory\n"
				"QUIT			- Disconnect from the server\n"
				"HELP			- Display this help message\n"
				"214 End of help.\n"
		      );
		}
		else if (strcmp(cmd, "send") == 0)
		{
			int dcSocket;
			char fileBuffer[100];
			int fileSize;
			status = clntConnect("127.0.0.1", &dcSocket);
			if (status != 0 )
			{
				printf("Data connection to client failed, exiting main. \n");
				strcpy(replyMsg, "500 Data connection failed.\n");
			}

			else if (strcmp(arg, "") == 0)
			{

				strcpy(replyMsg, "550 No file specified for upload.\n");
			}
			else
			{
				FILE *file = fopen(arg, "wb"); //*****
				if (file == NULL)
				{
					perror("send (upload) failed");
					strcpy(replyMsg, "550 Failed to open file for writing on the server.\n");
				}
				else 
				{
					do 
					{
						int status = receiveMessage(dcSocket, fileBuffer, 100, &fileSize);
						if (status == OK)
						{
							status = fwrite(fileBuffer, sizeof(char), fileSize, file);
							if (status < 0)
							{
								strcpy(replyMsg, "Error writing to file.\n");
							}
							else
							{
								strcpy(replyMsg, "226 File uploaded successfully.\n");
							}
						}
						else
						{
							fclose(file);
								strcpy(replyMsg, "550 File transfer failed.\n");
						}
					} while (fileSize > 0);
				} 
				close(dcSocket);
			}
			printf("Reached end of send.\n");
		}
		else if (strcmp(cmd, "recv") == 0)
		{
			size_t bytesRead;
			int dcSocket;
			status = clntConnect("127.0.0.1", &dcSocket);
			if (status != 0)
			{
				printf("ERROR CONNECTING.\n");
			}
			if (strcmp(arg, "") == 0)
			{
					strcpy(replyMsg, "550 No file specified for download.\n");
			}
			else 
			{
				FILE *file = fopen(arg, "rb");
				if (file == NULL)
				{
					perror("recv (download) failed");
					strcpy(replyMsg, "550 File not found on server.\n");
					status = sendMessage(dcSocket, FILE_DNE, 8);
					status = sendMessage(dcSocket, EMPTY_MSG, 0);
				}
				else 
				{
					status = sendMessage(dcSocket, FILE_EXISTS, 5);
					printf("\n");
					do
					{
						bytesRead = fread(fileBuffer, sizeof(char), 1024, file);
						if (bytesRead < 0)
						{
							strcpy(replyMsg, "550 Error reading from file.\n");
						}
						status = sendMessage(dcSocket, fileBuffer, bytesRead);
						if (status == OK)
						{
							strcpy(replyMsg, "226 File sent successfully.\n");
						}
						else 
						{
							strcpy(replyMsg, "550 Failed to send file to client.\n");
						}
					} while (bytesRead > 0);
					fclose(file);
				}
			}
			close(dcSocket);
			
		}
		else if (strcmp(cmd, "quit") == 0)
		{
			strcpy(replyMsg, "221 Goodbye\n");
			return;
		}
		else
		{
			strcpy(replyMsg, "500 Syntax error, command unrecognized\n");
		}
	}
}


/*
 * svcInitServer
 *
 * Function to create a socket and to listen for connection request from client
 *    using the created listen socket.
 *
 * Parameters
 * s		- Socket to listen for connection request (output)
 *
 * Return status
 *	OK			- Successfully created listen socket and listening
 *	ER_CREATE_SOCKET_FAILED	- socket creation failed
 */

int svcInitServer (
	int *s 		/*Listen socket number returned from this function */
	)
{
	int sock;
	struct sockaddr_in svcAddr;
	int qlen;

	/*create a socket endpoint */
	if( (sock=socket(AF_INET, SOCK_STREAM,0)) <0)
	{
		perror("cannot create socket");
		return(ER_CREATE_SOCKET_FAILED);
	}

	/*initialize memory of svcAddr structure to zero. */
	memset((char *)&svcAddr,0, sizeof(svcAddr));

	/* initialize svcAddr to have server IP address and server listen port#. */
	svcAddr.sin_family = AF_INET;
	svcAddr.sin_addr.s_addr=htonl(INADDR_ANY);  /* server IP address */
	svcAddr.sin_port=htons(SERVER_FTP_PORT);    /* server listen port # */

	/* bind (associate) the listen socket number with server IP and port#.
	 * bind is a socket interface function 
	 */
	if(bind(sock,(struct sockaddr *)&svcAddr,sizeof(svcAddr))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}

	/* 
	 * Set listen queue length to 1 outstanding connection request.
	 * This allows 1 outstanding connect request from client to wait
	 * while processing current connection request, which takes time.
	 * It prevents connection request to fail and client to think server is down
	 * when in fact server is running and busy processing connection request.
	 */
	qlen=1; 


	/* 
	 * Listen for connection request to come from client ftp.
	 * This is a non-blocking socket interface function call, 
	 * meaning, server ftp execution does not block by the 'listen' funcgtion call.
	 * Call returns right away so that server can do whatever it wants.
	 * The TCP transport layer will continuously listen for request and
	 * accept it on behalf of server ftp when the connection requests comes.
	 */

	listen(sock,qlen);  /* socket interface function call */

	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /*successful return */
}


/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character arrary containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */

int sendMessage(
	int    s,	/* socket to be used to send msg to client */
	char   *msg, 	/* buffer having the message data */
	int    msgSize 	/* size of the message/data in bytes */
	)
{
	int i;
	/* Print the message to be sent byte by byte as character */
	for(i=0; i<msgSize; i++)
	{
		printf("%c",msg[i]);
	}
	if((send(s, msg, msgSize, 0)) < 0) /* socket interface call to transmit */
	{
		perror("unable to send ");
		return(ER_SEND_FAILED);
	}

	return(OK); /* successful send */
}


/*
 * receiveMessage
 *
 * Function to receive message from client ftp
 *
 * Parameters
 * s		- Socket to be used to receive msg from client (input)
 * buffer  	- Pointer to character arrary to store received msg (input/output)
 * bufferSize	- Maximum size of the array, "buffer" in octent/byte (input)
 *		    This is the maximum number of bytes that will be stored in buffer
 * msgSize	- Actual # of bytes received and stored in buffer in octet/byes (output)
 *
 * Return status
 *	OK			- Msg successfully received
 *	R_RECEIVE_FAILED	- Receiving msg failed
 */


int receiveMessage (
	int s, 		/* socket */
	char *buffer, 	/* buffer to store received msg */
	int bufferSize, /* how large the buffer is in octet */
	int *msgSize 	/* size of the received msg in octet */
	)
{
	int i;

	*msgSize=recv(s,buffer,bufferSize,0); /* socket interface call to receive msg */
	if(*msgSize<0)
	{
		perror("unable to receive");
		return(ER_RECEIVE_FAILED);
	}

	/* Print the received msg byte by byte */
	for(i=0;i<*msgSize;i++)
	{
		printf("%c", buffer[i]);
	}
	printf("\n");

	return (OK);
}

void initDirectory(char* directory[3][2])
{
	directory[0][0] = "bob";
	directory[0][1] = "123";
	directory[1][0] = "bruh";
	directory[1][1] = "234";
	directory[2][0] = "bib";
	directory[2][1] = "987";
}
int clntConnect (
        char *serverName, /* server IP address in dot notation (input) */
        int *s            /* control connection socket number (output) */
        )
{
        int sock;       /* local variable to keep socket number */

        struct sockaddr_in clientAddress;       /* local client IP address */
        struct sockaddr_in serverAddress;       /* server IP address */
        struct hostent     *serverIPstructure;  /* host entry having server IP address in binary */


        /* Get IP address os server in binary from server name (IP in dot natation) */
        if((serverIPstructure = gethostbyname(serverName)) == NULL)
        {
                printf("%s is unknown server. \n", serverName);
                return (ER_INVALID_HOST_NAME);  /* error return */
        }

        /* Create control connection socket */
        if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                perror("cannot create socket ");
                return (ER_CREATE_SOCKET_FAILED);       /* error return */
        }

        /* initialize client address structure memory to zero */
        memset((char *) &clientAddress, 0, sizeof(clientAddress));

        /* Set local client IP address, and port in the address structure */
        clientAddress.sin_family = AF_INET;     /* Internet protocol family */
        clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY is 0, which means */
                                                 /* let the system fill client IP address */
        clientAddress.sin_port = 0;  /* With port set to 0, system will allocate a free port */
                          /* from 1024 to (64K -1) */

        /* Associate the socket with local client IP address and port */
        if(bind(sock,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
        {
                perror("cannot bind");
                close(sock);
                return(ER_BIND_FAILED); /* bind failed */
        }
        /* Initialize serverAddress memory to 0 */
        memset((char *) &serverAddress, 0, sizeof(serverAddress));

        /* Set ftp server ftp address in serverAddress */
        serverAddress.sin_family = AF_INET;
        memcpy((char *) &serverAddress.sin_addr, serverIPstructure->h_addr,
                        serverIPstructure->h_length);
        serverAddress.sin_port = htons(DATA_FTP_PORT);

        /* Connect to the server */
        if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
        {
                perror("Cannot connect to server ");
                close (sock);   /* close the control connection socket */
                return(ER_CONNECT_FAILED);      /* error return */
        }


        /* Store listen socket number to be returned in output parameter 's' */
        *s=sock;

        return(OK); /* successful return */
}  // end of clntConnect() 




