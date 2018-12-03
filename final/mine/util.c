#include "util.h"

int init(){
  proc[0].uid = 0;
  proc[0].cwd = 0;
  proc[1].uid = 1;
  proc[1].cwd = 0;

  //set running to the 0th proc
  running = &(proc[0]);
  readQueue = &(proc[1]); //next in queue
  int i;
  //set ref count for minodes to 0
  for(i = 0; i<100; i++){
    minode[i].refCount = 0;
    minode[i].ino = 0;
  }
  root = 0;
}

int mount_root(char *devName){

  char buf[BLKSIZE];
  dev = open(devName, O_RDWR); //openthe device for read and write
  if(dev < 0){
    printf("Error Opening %s.\n",devName);
    exit(1);
  }
  get_super(dev, buf); //get block 1
  sp = (SUPER *)buf;
  if(is_ext2(buf) <= 0){
    printf("Not an EXT2 file System.\n");
    exit(1);
  }

  get_inode_table(dev); //sets the global variable inode begin to tstart of the inode table
  ninodes = sp->s_inodes_count; //get the number of inodes
  root = iget(dev, ROOT_INODE); //set the root inode
  proc[0].cwd = proc[1].cwd = iget(dev, ROOT_INODE);//set the cwdd to the root
  MountTable[0].mounted_inode = root; //update the mount table
  MountTable[0].ninodes = ninodes;
  MountTable[0].nblocks = sp->s_blocks_count;
  MountTable[0].dev = dev;
  strncpy(MountTable[0].name, devName, 256);

  printf("Device Mounted.\n");
  return dev;
}

void menu(){
  printf("***** Level 1 *****\n");
  printf("ls\tpwd\tcd\tmkdir\trmdir\ncreat\tlink\tsymlink\tunlink\tchmod\n");

}

//returns the block in the buffer
int get_block(int dev1, int blk, char *buf){
  if(lseek(dev1, (long)(blk*BLKSIZE), 0) == -1){
    printf("%s\n", strerror(errno));
    assert(0);
  }
  read(dev1,buf, BLKSIZE);
}


//reads the super block
int get_super(int dev1, char *buf){
  get_block(dev1, SUPERBLOCK, buf);
}

//checks if a dev is ext2
int is_ext2(char *buf){
  sp = (SUPER *)buf;
  if(SUPER_MAGIC != sp->s_magic){
    printf("Error. Not an EXT2 File System\n");
  }
}

//gets the inode table and poplates global values
void get_inode_table(int dev1){
  char buf[BLKSIZE];
  get_gd(dev1, buf);
  gp = (GD*)buf;
  inodeBegin = gp->bg_inode_table;
  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
}
//reads the group descriptor
int get_gd(int dev1, char *buf){
  get_block(dev1, GDBLOCK, buf);
}



/**** LS *****/
int ls(char *path){
  int ino;
  MINODE *mip;
  if(!path || !pathname[0]) 
  {
    ino = running->cwd->ino;   //if the pathname is null then set ino to root ino
  }else { ino = getino(dev, path);} //get the ino from the path
  if(0>= ino){
    printf("Invalid path\n");
    return -1;
  }
  mip = iget(dev, ino); //use iget to get an minode based on the ino

  findBlocks(&(mip->INODE), 0); //use find blocks. this will help us print the valuesin the directory we are ls-ing
  iput(mip->dev, mip); //put it back!
}


/************ CD ************/

int cd(char *pathname){
  unsigned int ino;
  MINODE *mip;
  //check if we use absolue or realative path
  //
  if(!pathname || !pathname[0] || (pathname[0] == '/' && !pathname[1] ))
  {
    ino = root->ino;
  }else //else get the ino from the pathname
  {
    ino = getino(dev, pathname);
  }

  if(!ino) //if ino is invalid
  {
    printf("Invalid pathname\n");
    return -1;
  }
  mip = iget(dev, ino); //get the minode
  //verify we are tring to cd into a dir
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Error: path is not a directory\n");
    iput(dev, mip); //put the minode back and return
    return -1;
  }

  //if we here then it should be adir
  //put the current minode away
  iput(dev, running->cwd);

  //set new cwd to the new inode
  running->cwd = mip;
  return 0;
}

/******* PWD *********/

int do_pwd(char *pathname){
  printf("cwd = ");
  pwd(running->cwd);
  putchar(10);
}

int pwd(MINODE *wd){
  int ino = 0;
  MINODE *next = NULL;
  char temp[256];
  if(DEBUG){printf("WD: %d ROOT %d\n", wd->ino, root->ino);}
  //if we are at the root print only the root
  if(wd->ino == root->ino)
  {
    if(DEBUG){printf("in here\n");}
    printf("/");
    return 1;
  }


  //get the parents minode
  ino = search(dev, "..", &(wd->INODE));
  if(ino <=0)
  {
    printf("Error accesssing inode\n");
    return -1;
  }

  next = iget(dev, ino);
  if(DEBUG){printf("next: %d ROOT %d\n", next->ino, root->ino);}
  if(!next)
  {
    printf("Error. Could not find inode\n");
    return -1;
  }
  pwd(next); //recursively go through each parent and print out their names
  memset(temp, 0, 256);
  searchByIno(next->dev, wd->ino, &(next->INODE), temp);
  printf("%s/", temp);
  iput(next->dev, next);
  return 0;
}


int make_dir(char *pathname)
{
  int dev1, ino, r;
  char parent[256], child[256], origPath[512];
  MINODE *mip;
  memset(parent, 0, 256);
  memset(child, 0, 256);
  memset(origPath, 0, 512);
  strcpy(origPath, pathname);
  //check for absolute vs relative

  if(pathname[0] == '/') { dev1 = root->dev;}
  else { dev1 = running->cwd->dev;}

  //get basename and dirname

  strcpy(parent, dirname(pathname));
  strcpy(child, basename(origPath));

  if(DEBUG) { printf("parent: %s  child: %s \n", parent, child);}
  //get the ino for the parent
  ino = getino(dev1, parent);
  if(0>=ino)
  {
    printf("Error, invalid pathname\n");
    return -1;
  }
  mip = iget(dev1, ino);
  //check ifthe parent is a directory?
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Error, not a directory\n");
    iput(dev1, mip);
    return -1;
  }

  //look to see if the item already exists in the current dir
  ino = search(dev1, child, &(mip->INODE));
  if(ino <=0)
  {
    printf("Error. Directory already exists\n");
    iput(mip->dev, mip);
    return -1;
  }

  //acutually make the dir now. Above was simple checks and stuff
  r = my_mk(mip, child);
  iput(mip->dev, mip);

}


int my_mk(MINODE *pip, char child[256])
{
  int inum, bnum, idealLen, neededLen, newRec, i j;
  MINODE *mip;
  char *cp;
  DIR *prevDP;
  char buf[BLKSIZE];
  char buf2[BLKSIZE];
  int blk[256];

  //get inode and block num
  inum = ialloc(pip->dev);
  bnum = balloc(pip->dev);

  //get an minode for the inode
  mip = iget(pip->dev, inum);

  //update the contesnts of the minode
  mip->INODE.i_mode = 0x41ED;  //its a dir
  mip->INODE.i_uid = running->uid;
  mip->INODE.i_gid = running->gid;
  mip->INODE.i_size = BLKSIZE;
  mip->INODE.i_links_count = 2;
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.mtime = time(0L); //update time
  mip->INODE.mip->dirty = 1;
  for(i = 0; i < 15; i++)
  {
    mip->INODE->i_block[i] = 0;
  }

  mip->INODE->i_block[0] = bnum;
  iput(mip->dev, mip);

  //make . and .. for data block 0
  dp = (DIR *)buf;
  dp->inode = inum;
  strncpy(dp->name, ".", 1);
  dp->name_len = 1;
  dp->rec_len = 12;

  cp = buf + 12;
  dp = (DIR *)cp;
  dp0>inode = pip->ino;
  dp->name_len = 2;
  strncpy(dp->name, "..", 2);
  dp->rec_len = BLKSIZE - 12;

  //write to disk
  put_block(pip->dev, bnum, buf);

  //update the parent inode
  memset(buf, 0, BLKSIZE);
  neededLen = 4*((8+strlen(child)+3)/4);
  //check if there is room in the last block in the parents directory
  bnum = findLast(pip);

}


int findLast(MINODE *pip)
{
  int buf[256], buf2[256], bnum, i, j;

  //find last used block in the given pip
  if(pip->INODE.i_block[0] == 0) {return 0;}

  for(i = 0; i<12; i++)
  {
    if(pip->INODE.i_block[i] == 0) {return pip->INODE.i_block[i-1];}
  }

  if(pip->INODE.i_block[12] == 0) { return pip->INODE.i_block[i-1];}
  get_block(dev, pip>INODE.i_block[12], (char *)buf);
  for(i = 0; i<256; i++)
  {


  }
}

//find space for a new inode
int ialloc(int dev1)
{
  int i;
  char buf[BLKSIZE];
  //GET INODE BITMAP INTO BUF
  get_block(dev1, imap, buf);

  for(i = 0; i < ninodes; i++)
  {
    if(tst_bit(buf, i) == 0) //test thje bit, looking for a free one.
    {
      set_bit(buf, i); //toggle the bit since we found one
      put_block(dev1, imap, buf); //write the imap block back to the disk after the toggle

      //update free inode count in SUPER and DG on device
      decFreeInodes(dev1);
      return (i+1);
    }
  }
  return 0;
}

//basically the same as ialloc except were allocating spce for a block instead of an inode
int balloc(int dev1)
{

  int i;
  char buf[BLKSIZE];
  //GET INODE BITMAP INTO BUF
  get_block(dev1, bmap, buf);

  for(i = 0; i < BLKSIZE; i++)
  {
    if(tst_bit(buf, i) == 0) //test thje bit, looking for a free one.
    {
      set_bit(buf, i); //toggle the bit since we found one
      put_block(dev1, bmap, buf); //write the imap block back to the disk after the toggle

      //update free inode count in SUPER and DG on device
      decFreeInodes(dev1);
      return (i+1);
    }
  }
  return 0;
}

int test_bit(char *buf, int i)
{
  int byte, offset;
  byte = i/8;
  offset = i%8;
  return (((*(buf+byte))>>offset)&1);
}

int set_bit(char *buf, int i)
{
  int byte, offset;
  char temp;
  char *tempBuf;
  bye = i/8;
  offset = i%8;
  tempBuf = (buf + byte);
  temp |= (1<<offset);
  *tempBuf = temp;
  return 1;

}


int clr_bit(char *buf, int i)
{
  int byte, offset;
  char temp;
  char *tempBuf;
  byte = i/8;
  offset = i%8;
  tempBuf = (byte + buf);
  temp = *tempBuf;
  temp &= (~(1<<offset));
  *tempBuf = temp;
  return 1;

}



int decFreeInodes(int dev1)
{
  char buf[BLKSIZE];
  get_super(dev, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count -=1;
  put_block(dev, SUPERBLOCK, buf);
  get_gd(dev, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count -=1;
  put_block(dev, GDBLOCK, buf);
  return 1;
}

int incFreeInodes(int dev1)
{
  char buf[BLKSIZE];
  get_super(dev, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count +=1;
  put_block(dev, SUPERBLOCK, buf);
  get_gd(dev, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count +=1;
  put_block(dev, GDBLOCK, buf);
  return 1;
}

//finds all the data blocks from a pointer to that inode and prints the dir names in the data blocks
int findBlocks(INODE *ip, int printStat){
  int i, j, k;
  unsigned int buf[256], buf2[256];

  //print the direct blocks

  for(i = 0; i < 12; i++){

    if(ip->i_block[i] != 0){
      printDirs(ip->i_block[i], printStat);
    }

  }

  //print the indirect blocks
  //check that the 12th iblock actually exists so we can print the indirect blocks
  if(ip->i_block[12])
  {
    get_block(dev, ip->i_block[12], (char*)buf); //get the block
    for(i = 0; i < 256; i++)
    {
      if(buf[i]) {printDirs(buf[i], printStat);}
      {
      }

      //print dirs in double indirect blocks
      //check that the 13th actually exists
      if(ip->i_block[13])
      {
        //get the block
        get_block(dev, ip->i_block[13], (char*)buf);
        for(i = 0; i < 256; i++)
        {
          if(buf[i])
          {
            get_block(dev, buf[i], (char*)buf2); //double indirect so get the second block
            for(j = 0; j< 256; j++){
              if(buf2[j]) {printDirs(buf2[j], printStat);} 
            }
          }
        }
      }
    }
  }
}

int printDirs(int block, int printStat){

  int i;
  char *cp;
  DIR *dp;
  char buf[BLKSIZE], temp[256];
  get_block(dev, block, buf); //get the block that we are going print
  dp = (DIR *)buf;
  cp = buf;

  //go through each record and print its information
  while(cp < buf+BLKSIZE){
    memset(temp, 0, 256);
    strncpy(temp, dp->name, dp->name_len);
    printf("%4d %4d %4d %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

    cp+=dp->rec_len;
    dp = (DIR *)cp;
  }
  return 0;

}


int getino(int dev1, char *path){
  int ino = 0;
  int i = 0;
  char **tokens;
  MINODE *mip = NULL;
  if(path && path[0]){ //check if the path isnt null
    tokens = tokenPath(path); //get the tokens of the path
  }else{  //no pathname, so set ino to the cwd
    ino = running->cwd->ino;
    return ino;
  }

  if(path[0] =='/'){ //absolute path, start at the root dir
    ip = &(root->INODE);
    ino = root->ino;
  }else{  //else its relative, start at the current dir
    ip = &(running->cwd->INODE);
  }

  while(tokens[i]){
    ino = search(dev, tokens[i], ip); //search for the inode associated with the current token
    if(0 >= ino){
      if(mip){ iput(mip->dev, mip);} //write the mip to the disk and return if ino is null
      return -1;
    }
    if(mip) { iput(mip->dev, mip); }
    i++;
    if(tokens[i]){ //if the token is not null get the ip associated with it
      mip = iget(dev, ino);
      ip = &(mip->INODE);
    }
  }
  i = 0;
  while(tokens[i]){

    free(tokens[i]);
    i++;
  }
  if(mip) { iput(mip->dev, mip);}
  return ino;

}

//gets the minode associated with the ino
MINODE *iget(int dev1, unsigned int ino){
  int i, blk, offset;
  char buf[BLKSIZE];
  MINODE *mip = NULL;
  //search the minode list to see if the inode already exists
  for(i = 0; i < NMINODES; i++){
    //if inode is alread in the array, set mip to point to the MINODE in the array and increment the ref count
    if(minode[i].refCount == 0 && minode[i].ino == ino){
      mip = &minode[i];
      minode[i].refCount++;
      return mip;
    }
  }

  //if we are here then the inode does not exists. we need to put inode from disk into the minode array

  i = 0;
  while(minode[i].refCount > 0 && i < NMINODES){ i++;} //looks for a free spot in the array
  if(i == NMINODES){
    printf("MINODE array full.\n");
    return 0;
  }
  //use mailmans alg to get the location of a free inode
  blk = (ino -1)/8 + inodeBegin;
  offset = (ino -1)%8;
  get_block(dev1, blk, buf); //get the block
  ip = (INODE *)buf + offset;
  memcpy(&(minode[i].INODE), ip, sizeof(INODE)); //copy inode from disk into minode array
  //update values for the new minode
  minode[i].dev = dev1;
  minode[i].ino = ino;
  minode[i].refCount = 1;
  minode[i].dirty = 0;
  minode[i].mounted = 0;
  minode[i].mountptr = NULL;
  return &minode[i];

}

int iput(int dev1, MINODE *mip){
  char buf[BLKSIZE];
  int blk, offset;
  INODE *tip;
  mip->refCount--;  //putting away, so decrement the reference count

  if(mip->refCount > 0){return 0;} //if the ref count is > 0, someone else isstill using it
  if(mip->dirty == 0){return 1;} //if its not dirty then it wasnt modified, so return

  //must write the inode back to the disk.
  //use mailmans alg to get blck and offset for th enode
  blk = (mip->ino -1)/8 + inodeBegin;
  offset = (mip->ino)%8;
  get_block(dev1, blk, buf);
  tip = (INODE *)buf + offset; //get temp ip based off bufand offset
  memcpy(tip, &(mip->INODE), sizeof(INODE)); //use memcpy to write to the block fast and efficiently
  put_block(mip->dev, blk, buf);
  return 1;
}

//writes a block to the disk
int put_block(int dev1, int blk, char *buf){
  if(lseek(dev1,(long)(blk * BLKSIZE ), 0) == -1) {assert(0);}
  write(dev, buf, BLKSIZE);
  return 1;
}

int search(int dev1, char *str, INODE *ip){
  int i;
  char *cp;
  DIR *dp;
  char buf[BLKSIZE], temp[256];
  //look through direct blocks for the str
  for(i = 0; i < 12; i++){
    if(ip->i_block[i] == 0){break;} //if we reach null, break
    get_block(dev, ip->i_block[i], buf); //get the block associated
    dp = (DIR *)buf;
    cp = buf;

    //go through each record in the block. search for the string given
    while(cp < buf + BLKSIZE){
      memset(temp, 0, 256);
      strncpy(temp, dp->name, dp->name_len);
      if(strcmp(temp, str) == 0){
        return dp->inode; //found the name, return the inode
      }
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return 0;
}


//just like search except we are looking for matchin inodes now
int searchByIno(int dev, int ino, INODE *ip, char *temp)
{

  int i;
  char *cp;
  DIR *dp;
  char buf[BLKSIZE];

  for(i=0; i<12; i++)
  {

    if(ip->i_block[i] == 0) {break;}

    get_block(dev, ip->i_block[i], buf);
    cp = buf;
    dp = (DIR *)buf;
    while(cp < buf + BLKSIZE)
    {
      if(ino == dp->inode)
      {
        strncpy(temp, dp->name, dp->name_len);
        return 1;
      }
      cp+=dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return 0;

}







char ** tokenPath(char *path){
  int i;
  char **name;
  char *temp;
  name = (char **)malloc(sizeof(char *)*256);
  name[0] = strtok(path, "/");
  i = 1;
  while((name[i] = strtok(NULL,"/")) != NULL) { i++; }//get the other tokens in the pathname

  name[i] = 0;
  i = 0;
  while(name[i]){
    temp = (char *)malloc(sizeof(char)*strlen(name[i]));
    strcpy(temp, name[i]);
    name[i] = temp;
    i++;
  }
  return name;

}







int rm_dir(char *pathname){
}

int mylink(char *pathname){
}

int mysymlink(char *pathname){
}

int myunlink(char *pathname){
}

int mychmod(char *pathname){
}



