#include<stdio.h>
#include "udp.h"
#include "message.h"
#include<sys/stat.h>
#include <sys/mman.h>
#include<string.h>
#include "ufs.h"
#include <signal.h>
#include "mfs.h"

int fd, sd, rc, imageFD, currentINodeNum;
struct sockaddr_in *s;
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

   if(get_bit(inode_bitmap->bits,pinum) == 0){
    return -1;
  }
  int parent_dir = (itable->inodes[pinum].direct[0]);
   dir_block_t * parent_di = (void *) (image + (parent_dir)* UFS_BLOCK_SIZE);
   dir_block_t *parent = malloc(sizeof(dir_block_t));

   memcpy(parent, parent_di, sizeof(dir_block_t));
   for(int i=0;i<128;i++){
    if(!strcmp(parent->entries[i].name, name)){
        return parent->entries[i].inum;
    }
   }
  return -1;
}

int MFS_Stat(int inum, MFS_Stat_t *m){

  if(get_bit(inode_bitmap->bits,inum) == 0){
    return 0;
  }
   int type = itable->inodes[inum].type;
   int size = itable->inodes[inum].size;
   MFS_Stat_t *local_m = malloc(sizeof(MFS_Stat_t));
   local_m->size = size;
   local_m->type = type;
   memcpy(m, local_m, sizeof(local_m));
   
   return 1;
}

int MFS_Creat(int pinum, int type, char *name){
 
  int new_inode_num, new_data_num;

  for(int i=0; i < sup->num_inodes; i++){
    if(get_bit(inode_bitmap->bits,i) == 0){
      new_inode_num = i;
      break;
    }
  }

  if(type == UFS_REGULAR_FILE ){

  itable->inodes[new_inode_num].type = type;
  itable->inodes[new_inode_num].size = 0; 
  for(int i = 0; i< DIRECT_PTRS; i++){
    itable->inodes[new_inode_num].direct[i] = -1;
  }

   memcpy(inode_index, itable, sizeof(inode_block));

  int parent_dir = (itable->inodes[pinum].direct[0]);
  dir_block_t * parent_di = (void *) (image + (sup->data_region_addr + parent_dir)* UFS_BLOCK_SIZE);
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

//find free datablock
  for(int i=0; i < sup->num_data; i++){
    if(get_bit(data_bitmap->bits,i) == 0){
      new_data_num = i;
      break;
    }
  }
  itable->inodes[new_inode_num].type = type;
  itable->inodes[new_inode_num].size = 2 * sizeof(dir_ent_t); // in bytes
  itable->inodes[new_inode_num].direct[0] =  new_data_num; // TODO need to change here

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

}

int MFS_Unlink(int pinum, char *name){
  int parent_dir = (itable->inodes[pinum].direct[0]);
  printf(" parent_dir = %d = ",parent_dir);
  dir_block_t * parent_di = (void *) (image + (parent_dir)* UFS_BLOCK_SIZE);
  dir_block_t *parent = malloc(sizeof(dir_block_t));
  memcpy(parent, parent_di, sizeof(dir_block_t));
  int fileDir_exists=0;
  int inum, index_child_dir;
  for(int i=0;i<10;i++){ // change it to 128
      printf("this is for %s \n", name);
    printf("pinum = %d, inodenum = %d, name = %s\n\n", pinum, parent->entries[i].inum, parent->entries[i].name );
    if(!strcmp(parent->entries[i].name,name)){
      fileDir_exists=1;
      inum = parent->entries[i].inum;
      index_child_dir = i;
      break;
    }
  }
  if(fileDir_exists==0){
    printf(" %s file doesn't exist in inode\n", name);
    return -1;
  }
  // file
  if(itable->inodes[inum].type==UFS_REGULAR_FILE){
    if(itable->inodes[inum].size==0){
     // no need to do anything with inode entries
      parent->entries[index_child_dir].inum = -1;
      strcpy(parent->entries[index_child_dir].name,""); 
      memcpy(parent_di, parent, sizeof(dir_block_t));
      set_bit_zero(inode_bitmap->bits, inum);
      memcpy(i_bitmap, inode_bitmap, sizeof(bitmap_t));

    }
    else{
      int data_blk_st;
      for(int i=0;i<30;i++){
      if(itable->inodes[inum].direct[i]!=-1){
        itable->inodes[inum].direct[i]=-1;
        set_bit_zero(data_bitmap->bits,itable->inodes[inum].direct[i]);
        }

      }
      memcpy(d_bitmap, data_bitmap, sizeof(bitmap_t));
      memcpy(inode_index, itable, sizeof(inode_block));

      parent->entries[index_child_dir].inum = -1;
      strcpy(parent->entries[index_child_dir].name,""); 
      memcpy(parent_di, parent, sizeof(dir_block_t));
      set_bit_zero(inode_bitmap->bits, inum);
      memcpy(i_bitmap, inode_bitmap, sizeof(bitmap_t));
    
    }
  }
  else if(itable->inodes[inum].type==UFS_DIRECTORY){

    //check for empty directory

  int dir = (itable->inodes[inum].direct[0]);
  dir_block_t * dir_table = (void *) (image + (sup->data_region_addr + dir)* UFS_BLOCK_SIZE);
  dir_block_t *dir_tab = malloc(sizeof(dir_block_t));
  memcpy(dir_tab, dir_table, sizeof(dir_block_t));

  for(int i=2;i<30;i++){
    if(dir_tab->entries[i].inum!=-1){
      printf("index i = %d inum = %d name =%s\n\n", i, dir_tab->entries[i].inum, dir_tab->entries[i].name);
      // if it has a file or directory under it
       printf(" has a file or directory under it\n");
      return -1;
    }
  }

  for(int i=0;i<30;i++){
    if(itable->inodes[inum].direct[i]!=-1){
      itable->inodes[inum].direct[i]=-1;
      set_bit_zero(data_bitmap->bits,itable->inodes[inum].direct[i]);
      }

    }
  memcpy(d_bitmap, data_bitmap, sizeof(bitmap_t));
  memcpy(inode_index, itable, sizeof(inode_block));

  parent->entries[index_child_dir].inum = -1;
  strcpy(parent->entries[index_child_dir].name,""); 

  memcpy(parent_di, parent, sizeof(dir_block_t));
  set_bit_zero(inode_bitmap->bits, inum);
  memcpy(i_bitmap, inode_bitmap, sizeof(bitmap_t));
 
  }

   return 0;


}

int MFS_Read(int inum, char *buffer, int offset, int nbytes){

  int chk;
  if(get_bit(inode_bitmap->bits,inum) == 0){
      return -1;
    }
  
  chk = itable->inodes[inum].direct[0];
  
  int size = itable->inodes[inum].size;

  if(chk == 0 || chk == -1 || offset < 0 || offset > size || nbytes > UFS_BLOCK_SIZE || nbytes < 0){
    return -1;
  }
  char *read = (void *) (image + (sup->data_region_addr + chk )* UFS_BLOCK_SIZE + offset);

  memcpy(buffer, read, nbytes);

  return 0;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes){

  int chk;
    if(get_bit(inode_bitmap->bits,inum) == 0 || nbytes > UFS_BLOCK_SIZE || nbytes < 0 || offset < 0 ){
      return -1;
    }
  chk = itable->inodes[inum].direct[0];
  int type = itable->inodes[inum].type;
  if( type == UFS_DIRECTORY){
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

  char *write = (void *) (image + (sup->data_region_addr + new_data_num )* UFS_BLOCK_SIZE + offset);
  if(offset + nbytes > UFS_BLOCK_SIZE){
    //create new block and write remaining bytes to tat block
  }
  else{
    memcpy(write, buffer, nbytes);
  }

  }
  //write from offset
  else{
    char *write = (void *) (image + (sup->data_region_addr + chk )* UFS_BLOCK_SIZE + offset);
    if(offset + nbytes > UFS_BLOCK_SIZE){
      //create new block and write remaining bytes to tat block
    }
    else{
      memcpy(write, buffer, nbytes);
    }
  }
  fsync(fd);
}

int MFS_Shutdown(){
  fsync(fd);
  exit(0);
}

int main(int argc, char* argv[]){

    signal(SIGINT, intHandler);
    int sd = UDP_Open(10001);
    assert(sd>-1);

      remove("test1.img");
      system("./mkfs -f test1.img");

    fd = open("test1.img", O_RDWR);
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

  int x = MFS_Lookup(0,"file.txt");
  
   set_bit(data_bitmap->bits, 0);//change 2nd arg to data block number

    memcpy(i_bitmap, inode_bitmap, sizeof(bitmap_t));

   MFS_Creat(0,UFS_DIRECTORY, "a");
  MFS_Creat(1,UFS_REGULAR_FILE, "file1.txt");
   MFS_Creat(1,UFS_REGULAR_FILE, "file2.txt");
  MFS_Creat(0,UFS_DIRECTORY, "b");
  // int a_ret = MFS_Unlink(0,"a");
  // int b_ret = MFS_Unlink(0,"b");
  // int file_ret = MFS_Unlink(1,"file2.txt");

  // printf("\n a_ ret = %d\n", a_ret);
  // printf("\n b_ ret = %d\n", b_ret);
  // printf("\n file_ ret = %d\n", file_ret);

  MFS_Write(2,"hello this is file 1!", 10, 20);
  char *buff = malloc(20);
  MFS_Read(2, buff, 10, 20);
  printf("data after mfs read: %s\n", buff);
  //MFS_Write(3,"hello this is file 2!", 10, 20);

  super_t *parent = malloc(UFS_BLOCK_SIZE);                                                                                                                                                                                                                           

    rc = read(fd, parent, UFS_BLOCK_SIZE);

    printf("\n4\n");
    print_dir(4);

    printf("\n5\n");
    print_dir(5);

    printf("\n6\n");
    print_dir(6);
   
    char *dir = malloc(20);
    ssize_t s = pread(fd, dir, 20, 6*UFS_BLOCK_SIZE+10); // to do: offset issue
    printf("\ndata to file from disk: %s\n\n", dir);

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
    exit(0);
    return 0;
}