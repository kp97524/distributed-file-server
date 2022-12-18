#include<stdio.h>
#include "udp.h"
#include "message.h"
#include<sys/stat.h>
#include <sys/mman.h>
#include<string.h>
#include "ufs.h"
#include <signal.h>
#include "mfs.h"
#include <time.h>
#include <stdlib.h>


int MIN_PORT = 20000;
int MAX_PORT = 40000;


int fd, sd, rc, imageFD, currentINodeNum;
struct sockaddr_in s;
//struct dir_ent_t root;
typedef struct {
	inode_t inodes[UFS_BLOCK_SIZE / sizeof(inode_t)];
} inode_block;
typedef struct {
	unsigned int bits[UFS_BLOCK_SIZE / sizeof(unsigned int)];
} bitmap_t;
typedef struct {
  dir_ent_t entries[128];
} dir_block_t;

inode_block *itable;
inode_block *inode_index;
super_t *sup;
bitmap_t * inode_bitmap;
bitmap_t * data_bitmap;
bitmap_t * i_bitmap;
bitmap_t * d_bitmap;
char *image;

 void bin(unsigned n)
{
    if (n > 1)
        bin(n >> 1);
 
    printf("%d", n & 1);
}

unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);

   unsigned int x =  (bitmap[index] >> offset) & 0x1;
  //   printf("\ngetbit....\n");
  //  bin(bitmap[index]);
  //   printf("\n");
   return x;
}

void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
  //  printf("\nsetbit....\n");
  //  bin(bitmap[index]);
  //  printf("\n");
}

void set_bit_zero(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
  bitmap[index] &=  ~(1 << (offset -1));
}

void print_inode_table(int b_ind){

  inode_block *dir = malloc(sizeof(inode_block));
    ssize_t s = pread(fd, dir, sizeof(inode_block), b_ind*UFS_BLOCK_SIZE);
    
    for(int i = 0; i<10; i++){
      printf("type = %d , size = %d direct[0] = %d\n", dir->inodes[i].type , dir->inodes[i].size, dir->inodes[i].direct[0]);
    }
    

}

void print_dir(int b_ind){

  dir_block_t *dir = malloc(sizeof(dir_block_t));
    ssize_t s = pread(fd, dir, sizeof(dir_block_t), b_ind*UFS_BLOCK_SIZE);
    printf("inode num = %d , name = %s\n", dir->entries[0].inum , dir->entries[0].name);
    printf("inode num = %d , name = %s\n", dir->entries[1].inum , dir->entries[1].name);
    printf("inode num = %d , name = %s\n", dir->entries[2].inum , dir->entries[2].name);
    printf("inode num = %d , name = %s\n", dir->entries[3].inum , dir->entries[3].name);
     printf("inode num = %d , name = %s\n", dir->entries[4].inum , dir->entries[4].name);
    printf("inode num = %d , name = %s\n", dir->entries[5].inum , dir->entries[5].name);
      printf("inode num = %d , name = %s\n", dir->entries[6].inum , dir->entries[6].name);

}
void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

int MFS_Lookup(int pinum, char *name){
  char reply[UFS_BLOCK_SIZE];

   if(get_bit(inode_bitmap->bits,pinum) == 0 || itable->inodes[pinum].type == UFS_REGULAR_FILE){
      printf("\n in look up pinode doesnt exists \n");
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
  }
  int parent_dir = (itable->inodes[pinum].direct[0]);
   dir_block_t * parent_di = (void *) (image + (parent_dir)* UFS_BLOCK_SIZE);
   dir_block_t *parent = malloc(sizeof(dir_block_t));

   memcpy(parent, parent_di, sizeof(dir_block_t));
   
   for(int i=0;i<128;i++){
    if(!strcmp(parent->entries[i].name, name)){

    printf("\n look up success : 0 for file =  %s 114\n", name);
    sprintf(reply, "%d", parent->entries[i].inum);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
        return parent->entries[i].inum;
    }
   }

     printf("\n couldnt find child dir: -1   158\n");
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
  return -1;
}

int MFS_Stat(int inum, MFS_Stat_t *m){

   char reply[UFS_BLOCK_SIZE];
  if(get_bit(inode_bitmap->bits,inum) == 0){
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
  }
   int type = itable->inodes[inum].type;
   int size = itable->inodes[inum].size;
   MFS_Stat_t *local_m = malloc(sizeof(MFS_Stat_t));
   local_m->size = size;
   local_m->type = type;
   //memcpy(m, local_m, sizeof(local_m));

    printf("MFS_STAT : type = %d size = %d\n", type, size);
   sprintf(reply, "%d~%d~%d", 0, local_m->type, local_m->size);
   rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
   return 0;
}

int MFS_Creat(int pinum, int type, char *name){
 printf("hello\n");
  int new_inode_num, new_data_num;
  char reply[UFS_BLOCK_SIZE];

  if( get_bit(inode_bitmap->bits,pinum) != 1){
    fsync(fd);
    printf("response from server which is not sent: -1   137\n");
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
  }
  if(strlen(name) > 28){
     printf("response from server which is not sent: -1   137\n");
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
  }
  if(itable->inodes[pinum].type == UFS_REGULAR_FILE){
    printf("file within file cant be done!\n");
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
  }

  int parent_dir = (itable->inodes[pinum].direct[0]);// will get parent's directory data blocks

  dir_block_t * parent_di = (void *) (image + ( parent_dir)* UFS_BLOCK_SIZE);
  dir_block_t *parent = malloc(sizeof(dir_block_t));
  memcpy(parent, parent_di, sizeof(dir_block_t));

  int fl = 0;
  for(int i=0;i<128;i++){
    
    if(!strcmp(parent->entries[i].name,name)){
      fl =1;
      break;
    }
  }
  if(fl){
    fsync(fd);
    printf("\nresponse from server which is not sent: 0   158\n");
    sprintf(reply, "%d", 0);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return 0;
  }

  for(int i=0; i < sup->num_inodes; i++){
    if(get_bit(inode_bitmap->bits,i) == 0){
      new_inode_num = i;
      break;
    }
  }

  if(type == UFS_REGULAR_FILE ){

  itable->inodes[new_inode_num].type = type;
  itable->inodes[new_inode_num].size = 0; 
   printf("inode num = %d, type = %d,  size = %d",new_inode_num, itable->inodes[new_inode_num].type,itable->inodes[new_inode_num].size);
  for(int i = 0; i< DIRECT_PTRS; i++){
    itable->inodes[new_inode_num].direct[i] = -1;
  }
 printf("parent inode size before: %d", itable->inodes[pinum].size);
  itable->inodes[pinum].size += sizeof(dir_ent_t);
  printf("parent inode size after: %d", itable->inodes[pinum].size);

   memcpy(inode_index, itable, sizeof(inode_block));

  fsync(fd);

  int parent_dir = (itable->inodes[pinum].direct[0]);
  dir_block_t * parent_di = (void *) (image + (parent_dir)* UFS_BLOCK_SIZE);
  dir_block_t *parent = malloc(sizeof(dir_block_t));

  memcpy(parent, parent_di, sizeof(dir_block_t));

  for(int i=0;i<128;i++){
    if(parent->entries[i].inum == -1){
      parent->entries[i].inum = new_inode_num;
      strcpy(parent->entries[i].name, name);
      break;
      }
  }
  memcpy(parent_di, parent, sizeof(dir_block_t));

  set_bit(inode_bitmap->bits,new_inode_num);
  memcpy(i_bitmap, inode_bitmap, sizeof(bitmap_t));
  
  }
  else if(type == UFS_DIRECTORY){
    printf("creating directory...\n");

//find free datablock
  for(int i=0; i < sup->num_data; i++){
    if(get_bit(data_bitmap->bits,i) == 0){
      new_data_num = i;
      break;
    }
  }
  itable->inodes[new_inode_num].type = type;
  itable->inodes[new_inode_num].size = 2 * sizeof(dir_ent_t); // in bytes
  itable->inodes[new_inode_num].direct[0] =  sup->data_region_addr + new_data_num; 

  // updating parent directory size after adding a new directory 
  printf("parent inode size before: %d", itable->inodes[pinum].size);
  itable->inodes[pinum].size += sizeof(dir_ent_t);
  printf("parent inode size after: %d", itable->inodes[pinum].size);

  memcpy(inode_index, itable, sizeof(inode_block));


  dir_block_t * dir_entry = (void *)(image + ( sup->data_region_addr + new_data_num)* UFS_BLOCK_SIZE);
    
  dir_block_t *new_dir = malloc(UFS_BLOCK_SIZE);

  memcpy(new_dir, dir_entry, sizeof(dir_block_t));

  strcpy(new_dir->entries[0].name, ".");
  strcpy(new_dir->entries[1].name, "..");
    
  new_dir->entries[0].inum = new_inode_num;
  new_dir->entries[1].inum = pinum;

   
  for (int i = 2; i < 128; i++)
    new_dir->entries[i].inum = -1;

  memcpy(dir_entry, new_dir, sizeof(dir_block_t));

  int parent_dir = (itable->inodes[pinum].direct[0]);
  printf("This is parent  num directory block = %d", parent_dir );
  dir_block_t * parent_di = (void *) (image + ( parent_dir)* UFS_BLOCK_SIZE);
  dir_block_t *parent = malloc(sizeof(dir_block_t));

  memcpy(parent, parent_di, sizeof(dir_block_t));

  for(int i=0;i<128;i++){
  if(parent->entries[i].inum == -1){
    parent->entries[i].inum = new_inode_num;
    strcpy(parent->entries[i].name, name);
    break;
  }
  }
  memcpy(parent_di, parent, sizeof(dir_block_t));

  set_bit(inode_bitmap->bits,new_inode_num);
  set_bit(data_bitmap->bits,new_data_num);

  memcpy(d_bitmap, data_bitmap, sizeof(bitmap_t));
  memcpy(i_bitmap, inode_bitmap, sizeof(bitmap_t));

  }
  fsync(fd);

  printf("\n end response from server which is not sent: 0\n");
  sprintf(reply, "%d", 0);
  rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
  return 0;
}
int MFS_Unlink(int pinum, char *name){
  char reply[UFS_BLOCK_SIZE];
  int curr_inum, data_block_num, ind_of_child_in_parent_dir;

  int parent_dir = (itable->inodes[pinum].direct[0]);// will get parent's directory data blocks

  dir_block_t * parent_di = (void *) (image + ( parent_dir)* UFS_BLOCK_SIZE);
  dir_block_t *parent = malloc(sizeof(dir_block_t));
  memcpy(parent, parent_di, sizeof(dir_block_t));

int fl = 0;
  for(int i=0;i<128;i++){
    
    if(!strcmp(parent->entries[i].name,name)){
      curr_inum = parent->entries[i].inum;
      fl =1;
      ind_of_child_in_parent_dir = i;
      break;
    }
  }
  if(!fl){ 
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
    }

  if(itable->inodes[curr_inum].type == UFS_DIRECTORY){
    int child_dir = (itable->inodes[curr_inum].direct[0]);// will get parent's directory data blocks
    dir_block_t * child_di = (void *) (image + (child_dir)* UFS_BLOCK_SIZE);
    dir_block_t *child = malloc(sizeof(dir_block_t));
    memcpy(child, child_di, sizeof(dir_block_t));

    for(int i =2; i <128; i++){
      if(child->entries[i].inum != -1){
        sprintf(reply, "%d", -1);
        rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
        return -1;
      }
    }
    //directory is not empty
    parent->entries[ind_of_child_in_parent_dir].inum = -1; // make inum for given dir name as -1 in parent dir table
    strcpy(parent->entries[ind_of_child_in_parent_dir].name, "");
    memcpy(parent_di, parent, sizeof(dir_block_t)); //writing inum = -1 to memory

    //curr_inum  child_dir set inode bitmap and databitmap bits to zero
    set_bit_zero(inode_bitmap->bits, curr_inum);
    set_bit_zero(data_bitmap-> bits, child_dir - sup->data_region_addr);

    memcpy(inode_bitmap, i_bitmap, sizeof(bitmap_t));
    memcpy(data_bitmap, d_bitmap, sizeof(bitmap_t));
  }
  else if(itable->inodes[curr_inum].type == UFS_REGULAR_FILE){
    
    parent->entries[ind_of_child_in_parent_dir].inum = -1;// make inum for given dir name as -1 in parent dir table
    strcpy(parent->entries[ind_of_child_in_parent_dir].name, "");
    memcpy(parent_di, parent, sizeof(dir_block_t)); //writing inum = -1 to memory

    set_bit_zero(inode_bitmap->bits, curr_inum);
    memcpy(inode_bitmap, i_bitmap, sizeof(bitmap_t));
    
    if(itable->inodes[curr_inum].size != 0){
       int child_dir = (itable->inodes[curr_inum].direct[0]);// will get parent's directory data blocks
       set_bit_zero(data_bitmap-> bits, child_dir - sup->data_region_addr);
       memcpy(data_bitmap, d_bitmap, sizeof(bitmap_t));
    }

  }
  fsync(fd);
  sprintf(reply, "%d", 0);
  rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
 return 0;

}

int MFS_Read(int inum, char *buffer, int offset, int nbytes){
  printf("read called\n\n");
  char reply[2*UFS_BLOCK_SIZE];
  int chk;
  if(get_bit(inode_bitmap->bits,inum) == 0){
       printf("inonde not found error \n\n");
      sprintf(reply, "%d", -1);
      rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
      return -1;
    }
  
  chk = itable->inodes[inum].direct[0];
  
  int size = itable->inodes[inum].size;
  printf("read called 368 \n\n");

  if(chk == 0 || chk == -1 || offset < 0 || offset > size || nbytes > UFS_BLOCK_SIZE || nbytes < 0){
     printf("multiple read conditions fialed  \n\n");
    sprintf(reply, "%d", -1);
    rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
  }
  char *read = (void *) (image + (sup->data_region_addr + chk )* UFS_BLOCK_SIZE + offset);

  memcpy(buffer, read, nbytes);

  printf("buffer from read: %s\n", buffer);
  sprintf(reply, "%d~%s", 0, buffer);
  rc = UDP_Write(sd, &s, reply, 2*UFS_BLOCK_SIZE);
  return 0;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes){
  char reply[2*UFS_BLOCK_SIZE];
  char *write_buf;
  int chk;
    if(get_bit(inode_bitmap->bits,inum) == 0 || nbytes > UFS_BLOCK_SIZE || nbytes < 0 || offset < 0 ){
      return -1;
    }
  chk = itable->inodes[inum].direct[0];
  int type = itable->inodes[inum].type;
  if( type == UFS_DIRECTORY){
    sprintf(reply, "%d", -1);
   rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
    return -1;
  }
  
  //new file write for first time
  if(chk == -1){
  
  int new_data_num;

  //get free datablock from data bitmap
  for(int i=0; i < sup->num_data; i++){
    if(get_bit(data_bitmap->bits,i) == 0){
      new_data_num = i;
      break;
    }
  }
  //update data bitmap as new data block created
  set_bit(data_bitmap->bits,new_data_num);
  memcpy(d_bitmap, data_bitmap, sizeof(bitmap_t));

  //updating inode table entry for new file's data
  itable->inodes[inum].direct[0] = new_data_num;
  itable->inodes[inum].size += nbytes;
   memcpy(inode_index, itable, sizeof(inode_block));

  write_buf = (void *) (image + (sup->data_region_addr + new_data_num )* UFS_BLOCK_SIZE + offset);
  if(offset + nbytes > UFS_BLOCK_SIZE){
    //create new block and write remaining bytes to tat block
  }
  else{
    memcpy(write_buf, buffer, nbytes);
  }

  }
  //write from offset
  else{
    write_buf = (void *) (image + (sup->data_region_addr + chk )* UFS_BLOCK_SIZE + offset);
    if(offset + nbytes > UFS_BLOCK_SIZE){
      //create new block and write remaining bytes to tat block
    }
    else{
      memcpy(write_buf, buffer, nbytes);
    }
  }
  fsync(fd);
  sprintf(reply, "%d~%s", 0, write_buf);
  rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
  return 0;
}

int MFS_Shutdown(){
  char reply[UFS_BLOCK_SIZE];

  printf("shutdown done!\n");
  fsync(fd);
  sprintf(reply, "%d", 0);
  rc = UDP_Write(sd, &s, reply, UFS_BLOCK_SIZE);
  exit(0);

  return 0;
}

int main(int argc, char* argv[]){

    signal(SIGINT, intHandler);
   // sd = UDP_Open(1000);
  
    int port = atoi(argv[1]);
    sd = UDP_Open(port);
  //sd = UDP_Open(10002);
    assert(sd>-1);

      //remove("test1.img");
      //system("./mkfs -f test1.img");

    fd = open("test.img", O_RDWR);
    assert(fd>-1);

    struct stat sbuf;
    int rc = fstat(fd, &sbuf);
    assert(rc>-1);

    int img_size = (int) sbuf.st_size;
    image = mmap(NULL, img_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd,0);
    assert(image!=MAP_FAILED);

  //mapping of super block to memory
    sup = malloc(sizeof(super_t));
    memcpy(sup, image, sizeof(super_t));
    

  //itable is declared globally
  //getting inode table in memory
   itable= malloc(sizeof(inode_block));
   inode_index = (void *) (image + (*sup).inode_region_addr * UFS_BLOCK_SIZE);
   memcpy(itable, inode_index, sizeof(inode_block));

  //mapping of bitmaps to memory

  inode_bitmap=malloc(sizeof( bitmap_t));
  data_bitmap=malloc(sizeof( bitmap_t));

  i_bitmap= (void*) (image + (*sup).inode_bitmap_addr * UFS_BLOCK_SIZE);  
  d_bitmap= (void *)(image + (*sup).data_bitmap_addr * UFS_BLOCK_SIZE);  

  memcpy(inode_bitmap, i_bitmap, sizeof(bitmap_t));
  memcpy(data_bitmap, d_bitmap, sizeof(bitmap_t));

  //int x = MFS_Lookup(0,"file.txt");
  
  //  set_bit(data_bitmap->bits, 0);//change 2nd arg to data block number

  // memcpy(i_bitmap, inode_bitmap, sizeof(bitmap_t));

   /* MFS_Creat(0,UFS_REGULAR_FILE, "testdir");
   int lkp = MFS_Lookup(0, "testdir");
   printf("lookup = %d\n", lkp);
   int ret = MFS_Creat(1,UFS_REGULAR_FILE, "testfile");
   printf("ret = %d\n", ret);*/

  // MFS_Creat(1,UFS_REGULAR_FILE, "testfile");
  // MFS_Creat(1,UFS_REGULAR_FILE, "file1.txt");
  // //MFS_Creat(1,UFS_REGULAR_FILE, "file2.txt");
 
  // // int a_ret = MFS_Unlink(0,"a");
  //  int b_ret = MFS_Unlink(1,"b");
  //  int file_ret = MFS_Unlink(1,"file1.txt");
  //  int file_ret1 = MFS_Unlink(1,"file2.txt");

  // // printf("\n a_ ret = %d\n", a_ret);
  //  printf("\n b_ ret = %d\n", b_ret);
  //  printf("\n file1_ ret = %d\n", file_ret);
  // printf("\n file2_ ret = %d\n", file_ret1);

  // MFS_Write(2,"hello this is file mkfs!!", 0, 20);
  
  /* char *buff = malloc(20);
   MFS_Read(2, buff, 0, 20);

   printf("data from mfs read : %s\n", buff);*/


 // printf("data after mfs read: %s\n", buff);
  //MFS_Write(3,"hello this is file 2!", 10, 20);

   
   

    //  printf("\n7\n");
    // char *dir1 = malloc(20);
    // ssize_t s1 = pread(fd, dir1, 20, 7*UFS_BLOCK_SIZE);
    // printf("\ndata to file from disk: %s\n\n", dir1);

  /*  while(1){
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
    }*/

    // char *readbuf = malloc(2*UFS_BLOCK_SIZE);
    //    MFS_Read(2, readbuf, 0, 20);
  

char  *args[4];
    while (1) {
		char buffer[2*UFS_BLOCK_SIZE];
		rc = UDP_Read(sd, &s, buffer, UFS_BLOCK_SIZE);
		if (rc > 0) {
			printf("SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
      int c = atoi(strtok(buffer, "~"));
    
			switch(c) {
        
				case 0:
        printf("its in case 0 mkdir");
					args[0] = strtok(NULL, "~");
					args[1] = strtok(NULL, "~");
					MFS_Lookup(atoi(args[0]), args[1]);
					break;
				case 1:
					args[0] = strtok(NULL, "~");//inum
          MFS_Stat_t *m;
					MFS_Stat(atoi(args[0]), m);
					break;
				case 2:
					args[0] = strtok(NULL, "~");
          args[1] = strtok(NULL, "~");
					args[2] = strtok(NULL, "~");
          args[3] = strtok(NULL, "~");
					// char *writeBuffer = malloc(UFS_BLOCK_SIZE);
					// rc = UDP_Read(sd, &s, writeBuffer, UFS_BLOCK_SIZE);
					// if (rc > 0) {
					// 	args[1] = writeBuffer;
					// 	MFS_Write(atoi(args[0]), args[1], atoi(args[2]), atoi(args[3]));
					// }
          printf("size of buf %ld",strlen(args[1]));
          MFS_Write(atoi(args[0]), args[1], atoi(args[2]), atoi(args[3]));
					break;
				case 3: 
					args[0] = strtok(NULL, "~");//pinum
          args[2] = strtok(NULL, "~");//offset
					args[3] = strtok(NULL, "~");//nbytes

          printf("inside read: inum =%d offset = %d nbytes = %d\n", atoi(args[0]), atoi(args[2]), atoi(args[3]));

          char *readBuffer = malloc(UFS_BLOCK_SIZE);
          //rc = UDP_Read(sd, &s, readBuffer, 2*UFS_BLOCK_SIZE);
					if (rc > 0) {
          args[1] = readBuffer;
				//	MFS_Read(atoi(args[0]), args[1], atoi(args[2]), atoi(args[3]));
          MFS_Read(atoi(args[0]), readBuffer, atoi(args[2]), atoi(args[3]));
          }
					break; 
				case 4:
					args[0] = strtok(NULL, "~");
					args[1] = strtok(NULL, "~");
					args[2] = strtok(NULL, "~");
					MFS_Creat(atoi(args[0]), atoi(args[1]), args[2]);
					break;
				case 5: 
					args[0] = strtok(NULL, "~");
					args[1] = strtok(NULL, "~");
					MFS_Unlink(atoi(args[0]), args[1]);
					break;
				case 6:
					MFS_Shutdown();
					break;
			}
   /* printf("/nfter the switch/n");
    printf("\n3\n");
    print_inode_table(3);
    
    printf("\n4\n");
    print_dir(4);

    printf("\n5\n");
    print_dir(5);

    printf("\n6\n");
    print_dir(6);*/
		
    }
    }
    exit(0);
    return 0;
}