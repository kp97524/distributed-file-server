#include<stdio.h>
#include "udp.h"
#include "message.h"
#include "mfs.h"
#include "ufs.h"


int main(int argc, char* argv[]){
    struct sockaddr_in addrSnd, addrRcv;
   // int sd = UDP_Open(200087);
   // int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10002);
    message_t m;
    MFS_Stat_t *m1 = malloc(UFS_BLOCK_SIZE);

   /* printf("Client send message %d , msg %s\n", m.mtype, m.str);
    rc = UDP_Write(sd, &addrSnd, (char *)&m, sizeof(message_t));
    if(rc<0){
        printf("Client failed to send\n");
        exit(1);
    }
    printf("Client wait for reply...\n");*/
    MFS_Init("localhost", 10005);

    /*
    int rc = MFS_Creat(0, UFS_REGULAR_FILE, "test");
    printf("%d\n", rc);

    int lkup = MFS_Lookup(0, "test");
    printf("%d\n", lkup);

    rc = MFS_Stat(0, m1);
    printf("%d\n", rc);

    rc = MFS_Stat(1, m1);
    printf("%d\n", rc);

    */

     MFS_Creat(0,UFS_REGULAR_FILE, "file1.txt");
    char * buff = "BUFFER";
    int write_rc= MFS_Write(1,buff,0,4096);
    printf("\n write retur %d\n",write_rc);
    char * data = malloc(50000);
    int rd_rc = MFS_Read(1,data,0,4096);
    printf("\nread retur %d\n",rd_rc);
    printf("\ndata read %s \n",data);

    //rc = UDP_Read(sd,&addrRcv, (char *)&m, sizeof(message_t));
    //printf("Client got reply [size: %d rc %d type: %d %s]\n", rc, m.rc, m.mtype, m.str);

    return 0;
}