// structure libmfs
#include<stdio.h>
#include "mfs.h"
#include "udp.h"
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include "MFS_Msg.h"

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

	MFS_Msg_t m;
	m.inum = pinum;
	m.type = MFS_LOOKUP;
	memcpy(m.buffer, name, 4096);

	int rc = UDP_Write(sd, &saddr, (char *)&m, sizeof(MFS_Msg_t));

	// char message[BUFFER_SIZE];
    // sprintf(message, "0~%d~%s", pinum, name);
    // int rc = UDP_Write(sd, &saddr, message, BUFFER_SIZE);
    printf("CLIENT:: sent message (%d) MFS_Lookup pinum = %d name = %s\n", rc, m.inum, m.buffer);
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
		return MFS_Lookup(pinum, name);
	}
	/* Data is available */
	else {
		struct sockaddr_in raddr;
		MFS_Msg_t reply;
		int rc = UDP_Read(sd, &raddr, (char *)&reply, sizeof(MFS_Msg_t));
		printf("CLIENT:: read %d bytes (message: '%d')\n", rc, reply.retCode);
		return reply.retCode;
	}

	return 0;
}
int MFS_Stat(int inum, MFS_Stat_t *mstat){
	if (sd < 0) {
		return sd;
	} 
	MFS_Msg_t m;
	m.inum = inum;
	m.type = MFS_STAT;

	int rc = UDP_Write(sd, &saddr, (char *)&m, sizeof(MFS_Msg_t));

    printf("CLIENT:: sent message (%d) MFS_Stat inum = %d\n", rc, m.inum);
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
		return MFS_Stat(inum, mstat);
	}
	/* Data is available */
	else {
		MFS_Msg_t reply;
		//mstat = malloc(4096);
		int rc = UDP_Read(sd, &raddr, (char *)&reply, sizeof(MFS_Msg_t));
		printf("CLIENT:: read %d bytes (message: '%d')\n", rc, reply.retCode);
		MFS_Stat_t *mstat1;
		mstat1 = (MFS_Stat_t *)reply.buffer;

		mstat->size = mstat1->size;
		mstat->type= mstat1->type;
		printf("type= %d  size =%d\n", mstat->type, mstat-> size);

		return reply.retCode;
	}

	return 0;


    
}
int MFS_Write(int inum, char *buffer, int offset, int nbytes){
    if (sd < 0) {
		return sd;
	} 
	MFS_Msg_t m;
	m.inum = inum;
	m.type = MFS_WRITE;
	m.offset = offset;
	m.nbytes = nbytes;
	memcpy(m.buffer, buffer, nbytes);
   // print to vheck data
 
	int rc = UDP_Write(sd, &saddr, (char *)&m, sizeof(MFS_Msg_t));

    printf("CLIENT:: sent message (%d) MFS_WRITE offset = %d, nbytes= %d\n", rc,  m.offset, m.nbytes);
	if (rc < 0) {
		sd = UDP_Open(-1);		
		//return rc;
	} 
   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec = 100;
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
		//return MFS_Write(inum, buffer, offset, nbytes);
	}
	/* Data is available */
	else {
		struct sockaddr_in raddr;
		MFS_Msg_t reply;
		int rc = UDP_Read(sd, &raddr, (char *)&reply, sizeof(MFS_Msg_t));
		printf("CLIENT:: read %d bytes (message: '%d')\n", rc, reply.retCode);

		return reply.retCode;
	}

	return 0;
}
int MFS_Read(int inum, char *buffer, int offset, int nbytes){
   if (sd < 0) {
		return sd;
	} 
	MFS_Msg_t m;
	m.inum = inum;
	m.type = MFS_READ;
	m.offset = offset;
	m.nbytes = nbytes;
	//memcpy(m.buffer, buffer, 4096);

	int rc = UDP_Write(sd, &saddr, (char *)&m, sizeof(MFS_Msg_t));

    printf("CLIENT:: sent message (%d) MFS_Read offset = %d, nbytes = %d\n", rc, m.offset, m.nbytes);

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
		return MFS_Read(inum, buffer, offset,nbytes);
	}
	/* Data is available */
	else {
	struct sockaddr_in raddr;
		MFS_Msg_t reply;
		int rc = UDP_Read(sd, &raddr, (char *)&reply, sizeof(MFS_Msg_t));
		printf("CLIENT:: read %d bytes (message: '%d')\n", rc, reply.retCode);
		
		memcpy(buffer, reply.buffer, 4096);
		
		return reply.retCode;
	}

	return 0;
}



int MFS_Creat(int pinum, int type, char *name) {
	if (sd < 0) {
		return sd;
	} 
	MFS_Msg_t m;
	m.inum = pinum;
	m.type = MFS_CREAT;
	m.fileType = type;
	memcpy(m.buffer, name, 4096);

	int rc = UDP_Write(sd, &saddr, (char *)&m, sizeof(MFS_Msg_t));

    printf("CLIENT:: sent message (%d) MFS_Creat, inum = %d, type = %d, name = %s\n", rc, m.inum, m.fileType, m.buffer);

	if (rc < 0) {
		sd = UDP_Open(-1);		
		return rc;
	} 

   	fd_set fds;
	struct timeval timeout;
	/* Set time limit. */
	timeout.tv_sec =  5;
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
		struct sockaddr_in raddr;
		MFS_Msg_t reply;
		int rc = UDP_Read(sd, &raddr, (char *)&reply, sizeof(MFS_Msg_t));
		printf("CLIENT:: read %d bytes (message: '%d')\n", rc, reply.retCode);

		return reply.retCode;
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
	MFS_Msg_t m;
	m.inum = pinum;
	m.type = MFS_UNLINK;
	memcpy(m.buffer, name, 4096);

	int rc = UDP_Write(sd, &saddr, (char *)&m, sizeof(MFS_Msg_t));

    printf("CLIENT:: sent message (%d) MFS_UNLINK pinum = %d  name =%s\n", rc, m.inum, m.buffer);
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
		struct sockaddr_in raddr;
		MFS_Msg_t reply;
		int rc = UDP_Read(sd, &raddr, (char *)&reply, sizeof(MFS_Msg_t));
		printf("CLIENT:: read %d bytes (message: '%d')\n", rc, reply.retCode);
		return reply.retCode;
	}

	return 0;
}
int MFS_Shutdown(){
  if (sd < 0) {
		return sd;
	} 
	MFS_Msg_t m;
	m.type = MFS_SHUTDOWN;

	int rc = UDP_Write(sd, &saddr, (char *)&m, sizeof(MFS_Msg_t));

    printf("CLIENT:: sent message SHUTDOWN (%d)\n", rc );
	if (rc < 0) {
		return rc;
	} 

	UDP_Close(sd);
	return 0;
}