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

char *disk = "mydisk";
char gpath[256];
char names[64][64]; // assume at most 64 components in pathnames
int  n;

int  fd, dev,iblock;
int  nblocks, ninodes, bmap, imap, inode_start,numTokens;
char line[256], cmd[32], pathname[256], buf[256];
char *rootDev = "mydisk";
char *cmdList[] = {"mkdir", "cd", "pwd", "ls", "mount", "umount", "creat", "rmdir",
  "rm", "open", "close", "read", "write", "cat", "cp", "mv", "pfd", "lseek",
  "rewind", "stat", "pm", "menu", "access", "chmod", "chown", "cs", "fork",
  "ps", "kill", "quit", "touch", "sync", "link", "unlink", "symlink"};
//func fpointer[] = {ls_file};
int MAXTOKENSIZE = 64;

/*********** Init ***********/
int init(){
  int i;

  //set all cwd to 0
  for(i = 0; i<NPROC; i++){
    proc[i].cwd = 0;
    proc[i].pid = i;
    proc[i].uid = i;
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
/******* mount root *********/
int mount_root(){
  if(DEBUG){
    printf("in mount dev is: %d\n",dev);
  }
  //get the disk
  //  printf("Enter name for disk to open (Hit enter for default \'mydisk\'\n");
  // mygets(line,sizeof(line));
  //check the line is empty or not. if empty use default

  if(DEBUG)
    printf("rootdev: %s\n",disk);

  //open the device.


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
  iblock = gp->bg_inode_table;
  bmap = gp->bg_block_bitmap;
  imap = gp->bg_block_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmap: %d\timap: %d\tinode_start: %d\n",bmap, imap, inode_start);

  //get root inode!
  if(DEBUG){
    printf("before iget dev is: %d\n",dev);
  }
  root = iget(dev,2);
  if(DEBUG){
    printf("after dev is: %d\n",dev);
  }
  if(DEBUG)
    printf("root ino is: %d\n", root->ino);

  //point p0 and p1 cwd to root DID THIS IN MAIN
  //proc[0].cwd = iget(dev,2);
  //proc[1].cwd = iget(dev,2);
  //if(DEBUG)
  //printf("proc[0] and proc[1] cwd ino (should be root.): %d : %d\troot:%d\n", proc[0].cwd->ino,proc[1].cwd->ino,root->ino);

  //set running to root
  // running = &proc[0];
  return 0;
}

/*********** get block ***************/
int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

/********* myget ***********/
char *mygets(char *buf, size_t size){
  if (buf != NULL && size > 0) {
    if (fgets(buf, size, stdin)) {
      buf[strcspn(buf, "\n")] = 0;
      return buf;
    }
    *buf = '\0';  /* clear buffer at end of file */
  }
  return NULL;
}


/*********** iget ************/
MINODE *iget(int num, int ino){
  int i, block, offset;
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;
  block = offset = 0;
  mip = 0;
  ip = 0;

  if(DEBUG){
    printf("in iget dev is: %d\n",dev);
  }
  if(DEBUG)
    printf(RED"IN IGET\n"RESET);
  //search through the list of minodes we have. look for the given inode;
  for(i = 0; i<NMINODE; i++){
    MINODE *node = &minode[i];
    if(node->refCount > 0 && node->dev == dev && node->ino == ino){
      //we have found the ino we are looking for. update the reference count and return.
      node->refCount++;
      if(DEBUG)
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

  if(DEBUG){
    printf("mip->dev=%d\tmip->ino=%d\n",mip->dev,mip->ino);
  }
  return mip;
}

int findCmd(char *command)
{
  int i = 0;

  while (cmdList[i])
  {
    if (strcmp(command, cmdList[i]) == 0)
      return i;
    i++;
  }

  return -1;
}

/********* ls ***********/




int getino(char *path){
  if(DEBUG){
    printf("WERE IN GETINO. fd =:%d\n",fd);
  }

  int ino, index, blk, offset, i,bno;
  char *cp;
  get_block(fd,1, buf);
  sp = (SUPER *)buf;
  //check that its an ext2 file system by checking the magic number

  if(sp->s_magic != 0xEF53){
    printf("not an ext2\n");
    exit(-1);
  }else{
    printf("disk is an ext2 file system. \n");
  }

  // get where inodes begin
  get_block(fd, 2, buf);
  gp = (GD *)buf;
  cp = buf;

  iblock = gp->bg_inode_table;
  printf("Inodes begin at: %d\n\n",iblock);

  //read inode root dir
  get_block(fd, iblock, buf);
  ip = (INODE *)buf + 1;

  tokenize(pathname);

 // printf("starting search\n");

  //start search

  for(index = 0; index < numTokens; index++){
    // printf("first Child : %s\n", dp->name);
    ino = 0;
    bno = 0;

    //strncpy(temp, dp->name, dp->name_len);
    //temp[dp->name_len] = 0;
    //printf("temp: %s\nname len: %d",temp, dp->name_len);
   // printf("searching for %s\n",names[index]);
    ino = search(ip, names[index]);
    //if(!ino) exit(-1);
    printf("inode is: %d\n",ino);

    return ino;
    if (index != numTokens - 1 && !S_ISDIR(ip->i_mode)) // If we are expecting a directory and it isn't one, exit
    {
      printf("\t%s is not a directory. Aborting.\n", names[index]);
      exit(-1);
    }

    bno = (ino - 1) / 8 + iblock; //disk block contains this inode
    ino = (ino - 1) % 8;       //offset of inode in this block
  }

  return 0;
}

/**** Tokenize ***/
int tokenize(char *pathname)
{
  char *token, path[256];
  int index = 1;
  memset(names,0,sizeof(names));
  numTokens = 0;

  strcpy(path, pathname);                                   // Make copy of pathname to preserve it

  if (DEBUG) printf(YELLOW"Enter tokenizePath().\n\tGiven Path: %s\n"RESET, path);

  if (strlen(pathname) == 0)
  {
    printf(RED"Can't tokenize empty string.\n"RESET);
    return 0;
  }

  if (path[0] == '/')                                       // If path starts with a '/', remove it to simplify tokens
    token++;

  token = strtok(path, "/");
  if(DEBUG){
    printf("madeit here\n");
  }
  if (strlen(token) > MAXTOKENSIZE)
  {
    printf(RED"Token size too large. Maximum characters allowed is 64.\n"RESET);
    return 0;
  }
  strcpy(names[0], token);
  // memcpy(names[0], token, sizeof(token));
  if(DEBUG){
    printf("madeit here\n");
  }
  if (DEBUG) printf(YELLOW"\tToken 1: %s\n"RESET, token);

  numTokens++;

  while (token = strtok(NULL, "/"))                         // Allocate and copy the rest of the tokens
  {
    if (DEBUG) printf(YELLOW"\tToken %d: %s\n"RESET, index + 1, token);

    if (strlen(token) > MAXTOKENSIZE)
    {
      printf(RED"Token size too large. Maximum characters allowed is 64.\n"RESET);
      return 0;
    }

    strcpy(names[index], token);
    numTokens++;
    index++;
  }

  // Blank out the rest of the names so old tokenizations won't interfere with new tasks
  while (index < MAXTOKENSIZE)
  {
    memset(names[index], 0, MAXTOKENSIZE);
    index++;
  }

  return 1;

  /****** Search *********/
}



int search(INODE *ip, char *name){
  int i, inode;
  char dbuf[BLKSIZE], temp[256];
  INODE *pip = ip;


  for (i=0; i < 12; i++){  // assume at most 12 direct blocks
    char *cp, dirname[256];
    DIR *dp;
    get_block(fd,pip->i_block[i],dbuf);

    cp = dbuf;
    dp = (DIR *)dbuf;


    while (cp < dbuf + BLKSIZE){
      strncpy(dirname, dp->name, dp->name_len);
      dirname[dp->name_len] = 0;
      printf("looking for %s in %s\n",name, dirname);
      if(!dp->inode) return 0;
      if(strcmp(dirname, name) == 0){
        printf("found %s. \n", name);
        return dp->inode;
      }
      printf("Inode\tRec_len\tName\tDirname\n");
      printf("%4d\t%4d\t%4d\t%s\n\n",
          dp->inode, dp->rec_len, dp->name_len, dirname);

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
    return 0;
  }
}

int pwd(){
  MINODE *mp, *mpp;
  int ino, pino;
  char temp[256],path[256],tempPath[256];
  if(DEBUG){
    printf("In PWD\n");
  }
  //set the mp, check if we are at the root or not;
  mp = running->cwd;
  if(mp->ino == root->ino){
    printf("/\n");
    return 1;
  }
  findino(mp,&ino,&pino);//get the parent ino
  if(DEBUG){
    printf("returned from find ino, %d %d\n",ino, pino);
  }

  //climb up to root and print the results after
  while(ino != pino){
    mpp = iget(mp->dev, pino);
    whatsMyName(mpp, ino, temp);
    strcpy(tempPath,path);
    strcpy(path,temp);
    strcat(path,"/");
    strcat(path, tempPath);

  }

}

int whatsMyName(MINODE *parent, int myino, char *myname){
  char buf[BLKSIZE], *cp, temp[256];
  DIR *dp;
  int i;
  //go through 12 blocks and look for me
  for(i = 0; i < 12; i++){
    if(parent->INODE.i_block[i] == 0) continue;
    get_block(parent->dev, (int)parent->INODE.i_block[i], buf);
    cp = (char *)buf;
    dp = (DIR *)buf;
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    while(cp < (buf + BLKSIZE)){
      if(dp->inode == myino){ //we found it
        strcpy(myname, temp);
        return 1;
      }
      cp += dp->rec_len;
      dp = (DIR *)cp;
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;

    }

  }
  return 0;
}

int cdMe(char *pathname){
  int ino;
  MINODE *mip;

  printf("fd: %d dev: %d\n",fd,dev);
  //check for pathname
  if(strlen(pathname) == 0){
    iput(running->cwd);
    running->cwd = root;
    return 1;
  }else{
    ino = getino(pathname);
    if(DEBUG){
      printf("ino returned is: %d\n",ino);
    }
    mip = iget(dev, ino);
    if(DEBUG){
    printf("mip i_mode: %d\n",mip->INODE.i_mode);
    }
    if(!S_ISDIR(mip->INODE.i_mode)){
      printf("%s is not a dir.\n",pathname);
      return -1;
    }
    iput(running->cwd);
    running->cwd = mip;
  }
  return 0;
}






/******** id dir ***********/

int iput(MINODE *mip) //This function releases a Minode[] pointed by mip.
{
  int block, offset, i;
  char buf[BLKSIZE];
  INODE *ip;

  if (DEBUG) printf(YELLOW "Enter iget(). dev/ino: %d/%d\n" RESET, mip->dev, mip->ino);

  // First, decremement mip's refCount;
  mip->refCount--;
  if(mip->refCount>0){
    return 0;
  }

  //change the dirty back to clean
  if(mip->dirty == 0)
    return 0;

  // Otherwise, we need to write the INODE back to disk
  block = (mip->ino - 1) / 8 + iblock;
  offset = (mip->ino - 1) % 8;

  get_block(mip->dev, block, buf);              // Get the INODES' block
  ip = (INODE *) buf + offset;                  // Get the inode within the block

  //memcpy(ip, &mip->INODE, 128);               // Copy the new inode data into block from mip
  ip->i_mode = mip->INODE.i_mode;               // I can't tell if the memcpy is working so I'm doing the copy manually
  ip->i_uid = mip->INODE.i_uid;
  ip->i_size = mip->INODE.i_size;
  ip->i_atime = mip->INODE.i_atime;
  ip->i_ctime = mip->INODE.i_ctime;
  ip->i_mtime = mip->INODE.i_mtime;
  ip->i_dtime = mip->INODE.i_dtime;
  ip->i_gid = mip->INODE.i_gid;
  ip->i_links_count = mip->INODE.i_links_count;

  for(i = 0; i < 12; i++)
  {
    ip->i_block[i] = mip->INODE.i_block[i];
  }

  put_block(mip->dev, block, buf);

  if (DEBUG) printf(YELLOW "iput successful.\n" RESET);

  return 1;
}

int put_block(int fd, int blk, char buf[])
{
  lseek(fd, (long)blk*BLKSIZE, SEEK_SET);
  write(fd, buf, BLKSIZE);

  return 1;
}



int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
int findino(MINODE *mip, int *myino, int *parentino)
{
  char buf[BLKSIZE], *cp;
  INODE *ip = &mip->INODE;
  DIR *dp;

  get_block(mip->dev, (int)mip->INODE.i_block[0], buf);
  cp = (char *)buf;
  dp = (DIR *)cp;

  *myino = dp->inode;

  cp+= dp->rec_len;
  dp = (DIR *)cp;

  *parentino = dp->inode;

  return 1;
}
