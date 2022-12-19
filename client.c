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

    int cr = MFS_Creat(0,UFS_REGULAR_FILE, "testdir");
    int lk = MFS_Lookup(0 , "testdir");
    printf("create ret = %d , testdir inum/lkup = %d\n", cr, lk);

    cr = MFS_Creat(lk, UFS_REGULAR_FILE, "testfile");
    printf("\n create ret = %d\n",cr);
    

    return 0;
}