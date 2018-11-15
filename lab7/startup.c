#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include "type.h"


MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[256];
char *name[64]; // assume at most 64 components in pathnames
int  n;

int  fd, dev;
int  nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256], buf[256];
char *rootDev = "mydisk";

int init(){
  proc[0].uid = 0; proc[1].uid = 1;
  int i;

  //set all cwd to 0
  for(i = 0; i<NPROC; i++){
    proc[i].cwd = 0;
  }
  //set all ref count to 0
  for(i = 0; i < NMINODE; i++){
    minode[i].refCount = 0;;
  }

  //init root to 0
  MINODE *root;
  root = 0;



  return 0;
}

int mount_root(){
  //get the disk
  printf("Enter name for disk to open (Hit enter for default \'mydisk\'\n");
  gets(line);
  //check the line is empty or not. if empty use default
  if(line[0] != 0){
    if(DEBUG)
      printf("line: %s\n",line);
    rootDev = line;
  }

  //open the device.
  dev  = open(rootDev, O_RDWR);
  if(dev < 0){
    printf("Error opening disk.\n");
    exit(-1);
  }
  printf("Open Success\n");


  //Read super block to verify EXT2 FS
  get_block(dev,1,buf);
  sp = (SUPER *) buf;

  if(sp->s_magic != 0xEF53){
    printf("Disk is not an EXT2 FS.\n");
    exit(-1);
  }

  printf("is an EXT2 FS\n");

  //get the number of blocks and number of inodes from the super block
  nblocks = sp->s_blocks_count;
  ninodes = sp->s_inodes_count;
  printf("There are %d blocks and %d inodes. \n", nblocks, ninodes);

  //read GD0, record bmap, imap inode start
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  bmap = gp->bg_block_bitmap;
  imap = gp->bg_block_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmap: %d\timap: %d\tinode_start: %d\n",bmap, imap, inode_start);
  
  //get root inode!
  root = iget(dev,2);
  if(DEBUG)
    printf("root ino is: %d\n", root->ino);
return 0;
}


int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}


MINODE *iget(int dev, int ino){
  int i, block, offset;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;
  block = offset = 0;
  mip = ip = 0;
  if(DEBUG)
    printf(RED"IN IGET\n"RESET);
  //search through the list of minodes we have. look for the given inode;
  for(i = 0; i<NMINODE; i++){
    MINODE *node = &minode[i];
    if(node->refCount > 0 && node->dev == dev && node->ino == ino){
      //we have found the ino we are looking for. update the reference count and return.
      node->refCount +=1;
      printf("found what we are looking for\n");
      return node;
    }
  }

  //if we got here, then we didnt find any match. Look for the first open minode we have (refcount == 0)
  for(i = 0; i < NMINODE; i++){
    MINODE *node = &minode[i];
    if(node->refCount == 0){
      mip = node;
      if(DEBUG)
        printf("found empty!\n");
      break;
    }
  }

  //if mip is null, then all minodes are in use.
  if(!mip){
    printf("No free minode.\n");
    exit(1);
  }

 //if here, mip points at a vacant node. we then need to use mailmans alg to get block and offset
 block = (ino -1) / 8 + inode_start;
  offset = (ino - 1) % 8;

  //now we get the block and inode within the block based off of the alg
  get_block(dev, block, buf);
  ip = (INODE *)buf + offset;

  //copy ip into mips inode
  memcpy(&mip->INODE, (((INODE *)buf) + offset),sizeof(INODE));

  //populate values of mip
  mip->dev = dev;
  mip->ino = ino;
  mip->refCount = 1;
  mip->dirty = 0;
  mip->mounted = 0;
  mip->mptr = 0;

  return mip;
}
