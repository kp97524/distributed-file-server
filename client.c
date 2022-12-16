#include<stdio.h>
#include "udp.h"
#include "message.h"

int main(int argc, char* argv[]){
    struct sockaddr_in addrSnd, addrRcv;
    int sd = UDP_Open(2000);
    int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10000);
    message_t m;

    m.mtype = MFS_READ;
    //m.str = "hello";
    strcpy(m.str, "hello");

    printf("Client send message %d , msg %s\n", m.mtype, m.str);
    rc = UDP_Write(sd, &addrSnd, (char *)&m, sizeof(message_t));
    if(rc<0){
        printf("Client failed to send\n");
        exit(1);
    }
    printf("Client wait for reply...\n");
    rc = UDP_Read(sd,&addrRcv, (char *)&m, sizeof(message_t));
    printf("Client got reply [size: %d rc %d type: %d %s]\n", rc, m.rc, m.mtype, m.str);

    return 0;
}