#include<stdio.h>
#include "udp.h"
#include "message.h"

int sd, rc, imageFD, currentINodeNum;
struct sockaddr_in *s;

int main(int argc, char* argv[]){
  /*  int port;
	char *image, *args[3];

	port = atoi(argv[1]);
	image = argv[2];

    sd = UDP_Open(port);
    assert(sd > -1);*/

    int sd = UDP_Open(10000);
    assert(sd>-1);

    while(1){
        struct sockaddr_in addr;
        message_t m;
        printf("server waiting....\n");
        int rc = UDP_Read(sd,&addr,(char *)&m, sizeof(message_t));
        printf("server read message [size: %d contents: {%d} string = %s ]\n", rc, m.mtype, m.str);
        if(rc>0){
            m.rc = 3;
            rc = UDP_Write(sd,&addr,(char *)&m, sizeof(message_t));
            printf("Server : reply\n");
        }
    }
    return 0;
}