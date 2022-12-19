
#include<stdio.h>
#include "mfs.h"
#include "udp.h"
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#define BUFFER_SIZE (4096)
/*
int MIN_PORT = 20000;
int MAX_PORT = 40000;

srand(time(0));
int port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);*/

int sd = -1;
struct sockaddr_in saddr, raddr;

typedef struct mess{
	int type;
	int inum;
	int offset;
	int nbytes;
	char buffer[4096];
}msg;
msg buf;

int MFS_Init(char *hostname, int port){
    sd = UDP_Open(0);
	//sd = UDP_Open(port_num);
	if (sd < 0) {
		return sd;
	} 
	int rc = UDP_FillSockAddr(&saddr, hostname, port);
	if (rc < 0) {
		return rc;
	} 
	return 0;
}
int MFS_Lookup(int pinum, char *name){
    if (sd < 0) {
		return sd;
	} 
	char message[BUFFER_SIZE];
    sprintf(message, "0~%d~%s", pinum, name);
    int rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) \"%s\"\n", rc, message);
	if (rc < 0) {
	    sd = UDP_Open(-1);
		//return rc;
	} 

   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec = 300;
	timeout.tv_usec = 0;
	/* Create a descriptor set containing our two sockets.  */
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	rc = select(sd+1, &fds, NULL, NULL, &timeout);
	
	/* select error */
	if(rc < 0) {
		return -1;
	}
	/* No data in five seconds */	
	else if (rc == 0) {
		return MFS_Lookup(pinum, name);
	}
	/* Data is available */
	else {
		struct sockaddr_in raddr;
		char buffer[BUFFER_SIZE];
		int rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
		printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
		return atoi(buffer);
	}

	return 0;
}
int MFS_Stat(int inum, MFS_Stat_t *m){
	if (sd < 0) {
		return sd;
	} 
	char message[BUFFER_SIZE];
    sprintf(message, "1~%d", inum);
    int rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) \"%s\"\n", rc, message);
	if (rc < 0) {
		sd = UDP_Open(-1);		
		//return rc;
	} 

   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec = 300;
	timeout.tv_usec = 0;
	/* Create a descriptor set containing our two sockets.  */
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	rc = select(sd+1, &fds, NULL, NULL, &timeout);
	
	/* select error */
	if(rc < 0) {
		return -1;
	}
	/* No data in five seconds */	
	else if (rc == 0) {
		return MFS_Stat(inum, m);
	}
	/* Data is available */
	else {
		char buffer[BUFFER_SIZE];
		struct sockaddr_in raddr;
		int rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
		printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
		printf("CLIENT:: STAT SUCCESSFUL\n");
		int retCode = atoi(strtok(buffer, "~"));
		m->type = atoi(strtok(NULL, "~"));
		m->size = atoi(strtok(NULL, "~"));

		printf("type= %d  size =%d\n", m->type, m-> size);
		return retCode;
	}

	return 0;


    
}
int MFS_Write(int inum, char *buffer, int offset, int nbytes){
    if (sd < 0) {
		return sd;
	} 
	char message[2*BUFFER_SIZE];
    sprintf(message, "2~%d~%s~%d~%d", inum, buffer, offset,nbytes);
	printf("size of buf %ld\n",strlen(buffer));
    int rc = UDP_Write(sd, &saddr, message, 2*BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) \"%s\"\n", rc, message);
	if (rc < 0) {
		sd = UDP_Open(-1);		
		//return rc;
	} 

	//to send buffer from client to server
  //  rc = UDP_Write(sd, &saddr, buffer, BUFFER_SIZE);
   // printf("CLIENT:: sent message (%d) buffer = %s\n", rc, buffer);
	/*int i;	
	for (i = 0; i < BUFFER_SIZE; i++ ) {
  		putc( isprint(buffer[i]) ? buffer[i] : '.' , stdout );
	}
	printf("\n");
	if (rc < 0) {
		return rc;
	} */

   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec = 300;
	timeout.tv_usec = 0;
	/* Create a descriptor set containing our two sockets.  */
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	rc = select(sd+1, &fds, NULL, NULL, &timeout);
	
	/* select error */
	if(rc < 0) {
		return -1;
	}
	/* No data in five seconds */	
	else if (rc == 0) {
		return MFS_Write(inum, buffer, offset, nbytes);
	}
	/* Data is available */
	else {
		struct sockaddr_in raddr;
		int rc = UDP_Read(sd, &raddr, message, BUFFER_SIZE);
		printf("CLIENT:: read %d bytes (message: '%s')\n", rc, message);
        int retCode = atoi(strtok(message, "~"));
		buffer = strtok(NULL, "~");
		return retCode;
	}

	return 0;
}
int MFS_Read(int inum, char *buffer, int offset, int nbytes){
   if (sd < 0) {
		return sd;
	} 
	char message[BUFFER_SIZE];
    sprintf(message, "3~%d~%d~%d", inum,offset,nbytes);
    int rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) \"%s\"\n", rc, message);
	if (rc < 0) {
		sd = UDP_Open(-1);		
		//return rc;
	} 

   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec = 300;
	timeout.tv_usec = 0;
	/* Create a descriptor set containing our two sockets.  */
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	rc = select(sd+1, &fds, NULL, NULL, &timeout);
	
	/* select error */
	if(rc < 0) {
		return -1;
	}
	/* No data in five seconds */	
	else if (rc == 0) {
		return MFS_Read(inum, buffer, offset,nbytes);
	}
	/* Data is available */
	else {
		struct sockaddr_in raddr;
		int rc = UDP_Read(sd, &raddr, message, BUFFER_SIZE);
		printf("CLIENT:: read %d bytes (message: '%s')\n", rc, message);
		//int retCode = atoi(message);

		int retCode = atoi(strtok(message, "~"));
		buffer = strtok(NULL, "~");
		//memcpy(buffer, message, BUFFER_SIZE);
		//rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
		//printf("CLIENT:: read %d bytes (message: 'BUFFER')\n", rc);
		printf("buffer in mfs read = %s\n", buffer);
		/*int i;		
		for (i = 0; i < BUFFER_SIZE; i++ ) {
	  		putc( isprint(buffer[i]) ? buffer[i] : '.' , stdout );
		}
		printf("\n");
		printf("CLIENT:: test 'BUFFER'\n\"");
		for (i = 0; i < BUFFER_SIZE; i++ ) {
			if(buffer[i] != test[i]) {
	  			putc( isprint(buffer[i]) ? buffer[i] : '.' , stdout );
			}
		}
		printf("\"\n");*/
		return retCode;
	}

	return 0;
}



int MFS_Creat(int pinum, int type, char *name) {
	if (sd < 0) {
		return sd;
	} 
	char message[BUFFER_SIZE];
    sprintf(message, "4~%d~%d~%s", pinum, type, name);
    int rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) \"%s\"\n", rc, message);
	if (rc < 0) {
		sd = UDP_Open(-1);		
		return rc;
	} 

   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec =  300;
	timeout.tv_usec = 0;
	/* Create a descriptor set containing our two sockets.  */
	FD_ZERO(&fds);
	FD_SET(sd, &fds);

	rc = select(sd+1, &fds, NULL, NULL, &timeout);

	// select error 
	if(rc == -1) {
		return -1;
	}
	// Data is available 
	else if (rc) {
		char buffer[BUFFER_SIZE];
		struct sockaddr_in raddr;
		int rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
		printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
		printf("response buffer = %d\n", atoi(buffer));
		return atoi(buffer);
	}
	//No data in five seconds 
	else {
		printf("No data received from server\n\n");
		return MFS_Creat(pinum, type, name);
	}

	//return 0;
}


int MFS_Unlink(int pinum, char *name){
  if (sd < 0) {
		return sd;
	} 
	char message[BUFFER_SIZE];
    sprintf(message, "5~%d~%s", pinum, name);
    int rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) \"%s\"\n", rc, message);
	if (rc < 0) {
		sd = UDP_Open(-1);		
		//return rc;
	} 

   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	/* Create a descriptor set containing our two sockets.  */
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	rc = select(sd+1, &fds, NULL, NULL, &timeout);
	
	/* select error */
	if(rc < 0) {
		return -1;
	}
	/* No data in five seconds */	
	else if (rc == 0) {
		return MFS_Unlink(pinum, name);
	}
	/* Data is available */
	else {
		char buffer[BUFFER_SIZE];
		struct sockaddr_in raddr;
		int rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
		printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
		return atoi(buffer);
	}

	return 0;
}
int MFS_Shutdown(){
  if (sd < 0) {
		return sd;
	} 
	char message[BUFFER_SIZE];
    sprintf(message, "6");
    int rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) \"%s\"\n", rc, message);
	if (rc < 0) {
		return rc;
	} 

	UDP_Close(sd);
	return 0;
}