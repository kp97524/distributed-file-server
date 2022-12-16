#include<stdio.h>
#include "mfs.h"
#include "udp.h"
#include <ctype.h>

#define BUFFER_SIZE (4096)

int sd = -1;
struct sockaddr_in saddr;

int MFS_Init(char *hostname, int port){
    sd = UDP_Open(-1);
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
		char buffer[BUFFER_SIZE];
		int rc = UDP_Read(sd, &raddr, buffer, BUFFER_SIZE);
		printf("CLIENT:: read %d bytes (message: '%s')\n", rc, buffer);
		return atoi(buffer);
	}

	return 0;
}
int MFS_Stat(int inum, MFS_Stat_t *m){
    return 0;
}
int MFS_Write(int inum, char *buffer, int offset, int nbytes){
    return 0;
}
int MFS_Read(int inum, char *buffer, int offset, int nbytes){
    return 0;
}
int MFS_Creat(int pinum, int type, char *name){
    return 0;
}
int MFS_Unlink(int pinum, char *name){
    return 0;
}
int MFS_Shutdown(){
    return 0;
}