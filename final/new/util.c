#include "util.h"

// Initializes global variables
int init()
{
  proc[0].uid = 0;
  proc[0].cwd = 0;
  proc[1].uid = 1;
  proc[1].cwd = 0;

  running = &(proc[0]);
  readQueue = &(proc[1]);
  int i = 0;
  for(i = 0; i < 64; i++)
  {
    minode[i].refCount = 0;
    minode[i].ino = 0;
  }
  for(i = 0; i < 10; i++) { MountTable[i].dev = 0;}
  root = 0;
}

int menu(char* pathanme)
{
  printf("********* Level 1 Commands************\n");

  printf(" ls cd pwd mkdir chmod creat rmdir link unlink symlink touch quit \n");

  printf("******************************************\n");

  printf("********* Level 2 Commands************\n");

  printf("open write close \n");

  printf("******************************************\n");
}


int mount_root(char *devName)
{
  char buf[BLKSIZE];

  dev = open(devName, O_RDWR); //Open for read/write
  if (dev < 0){ //Could not open device
    printf("open %s failed\n", devName);
    exit(1);
  }
  get_super(dev, buf);//get block 1
  sp = (SUPER*)buf;
  if(is_ext2(buf) <= 0) {exit(0);} //Check is ext2 filesystem, if not exit program

  get_inode_table(dev); //Sets global variable inodeBegin to start of inode table
  ninodes = sp->s_inodes_count;
  root = iget(dev, ROOT_INODE); //Set root inode
  proc[0].cwd = iget(dev, ROOT_INODE); // Set cwd for procedure 1 & 2 to root inode
  proc[1].cwd = iget(dev, ROOT_INODE);
  MountTable[0].mounted_inode = root;
  MountTable[0].ninodes = ninodes;
  MountTable[0].nblocks = sp->s_blocks_count;
  MountTable[0].dev = dev;
  strncpy(MountTable[0].name, devName, 256);
  return dev;
}

// read SUPER block (block 1) and set contents to buf
int get_super(int dev1, char *buf)
{
  get_block(dev1, SUPERBLOCK, buf);
}

// Sets global variable inodeBegin to start of inode table for given device
void get_inode_table(int dev1)
{
  char buf[BLKSIZE];
  get_gd(dev1, buf);
  gp = (GD*)buf;
  inodeBegin = gp->bg_inode_table;
  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
}

//Determines if device is an ext2 filesystem. If not exits program.
//Parameter is a buffer of BLKSIZE that contains the super block
int is_ext2(char *buf)
{
  sp = (SUPER *)buf;
  if (SUPER_MAGIC != sp->s_magic)
  {
    printf("Error: Not an EXT2 file sytem\n");
    return -1;
  }
  return 1;
}

// Reads group descriptor information in block 2
int get_gd(int dev1, char *buf)
{
  get_block(dev1, GDBLOCK, buf);
}

//Returns the requested block in buffer
int get_block(int dev1, int blk, char *buf)
{
  if (-1 == lseek(dev1, (long)(blk*BLKSIZE), 0))
  {
    printf("%s\n", strerror(errno));
    assert(0);
  }
  read(dev1, buf, BLKSIZE);
}

int put_block(int dev, int blk, char *buf)
{
  if (-1 == lseek(dev, (long)(blk*BLKSIZE), 0)){ assert(0);}
  write(dev, buf, BLKSIZE);
  return 1;
}

//Tokenizes given pathname into an array of strings terminated with a null string.
char** tokenPath(char* pathname)
{
  int i = 0;
  char** name;
  char* tmp;
  name = (char**)malloc(sizeof(char*)*256);
  name[0] = strtok(pathname, "/");
  i = 1;
  while ((name[i] = strtok(NULL, "/")) != NULL) { i++;}
  name[i] = 0;
  i = 0;
  while(name[i])
  {
    tmp = (char*)malloc(sizeof(char)*strlen(name[i]));
    strcpy(tmp, name[i]);
    name[i] = tmp;
    i++;
  }
  return name;
}

//searches through 12 blocks to look for str. If found returns ino number, else returns 0 (indicating filepath does not exist)
int search(int dev, char *str, INODE *ip)
{
  int i;
  char *cp;
  DIR *dp;
  char buf[BLKSIZE], temp[256];

  for(i = 0; i < 12; i++)
  {
    if(ip->i_block[i] == 0){break;} //if the block is null then return, not in use
    get_block(dev, ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while(cp < buf+BLKSIZE)
    {
      memset(temp, 0, 256);
      strncpy(temp, dp->name, dp->name_len);
      if(strcmp(str, temp) == 0){ return dp->inode;}
      cp += dp->rec_len;
      dp = (DIR*)cp;
    }
  }
  return 0;
}

//searched for a file/dir based on the ino. similar to the search function but we compare the ino
int searchByIno(int dev, int ino, INODE *ip, char* temp)
{
  int i;
  char *cp;
  DIR *dp;
  char buf[BLKSIZE];

  for(i = 0; i < 12; i++)
  {
    if(ip->i_block[i] == 0){ break;}
    get_block(dev, ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while(cp < buf+BLKSIZE)
    {
      if(ino == dp->inode) //Found the right inode
      {
        strncpy(temp, dp->name, dp->name_len);
        return 1;
      }
      cp += dp->rec_len;
      dp = (DIR*)cp;
    }
  }
  return 0;
}

//Gets the ino number from a given pathname
unsigned int getino(int dev, char *path)
{
  int ino = 0, i = 0;
  char **tokens;
  MINODE *mip = NULL;
  if(path && path[0])//checks if ther is a path
  {
    tokens = tokenPath(path);
  }
  else //No pathname given so set ino to cwds ino
  {
    ino = running->cwd->ino;
    return ino;
  }
  if(path[0]=='/') //start at root dir
  {
    ip = &(root->INODE);
    ino = root->ino;
  }
  else //start at cwd
  {
    ip = &(running->cwd->INODE);
  }
  while(tokens[i]) //go through all the tokens searching for the token in inode
  {
    ino = search(dev, tokens[i], ip);
    if(0 >= ino) //check that the inode is valid
    {
      if(mip){ iput(mip->dev, mip);} //invalid. put the mip back to the disk, returns
      return -1;
    }
    if(mip) { iput(mip->dev, mip);}
    i++;
    if(tokens[i])
    {
      mip = iget(dev, ino);
      ip = &(mip->INODE);
    }
  }
  i = 0;
  while(tokens[i]) //free the tokens for future use
  {
    free(tokens[i]);
    i++;
  }
  if(mip) { iput(mip->dev, mip);}
  return ino;
}


//return minode pointer to the loaded inode
MINODE *iget(int dev1, unsigned int ino)
{
  int i = 0, blk, offset;
  char buf[BLKSIZE];
  MINODE *mip = NULL;
  //search minode[] to see if inode already exists in array
  for(i = 0; i < 64; i++)
  {
    // If inode is already in array, set mip to point to MINODE in array, increment MINODE's refCount by 1.
    if(minode[i].refCount > 0 && minode[i].ino == ino)
    {
      //printf("MINODE for inode %d already exists, just copying\n", minode[i].ino); //FOR TESTING
      mip = &minode[i];
      minode[i].refCount++;
      return mip;
    }
  }

  //If you have reached here then the inode does not currently exist in minode[100]. Put inode from disk into free MINODE in array's INODE field.
  i = 0;
  while(minode[i].refCount > 0 && i < 64) { i++;}
  if(i == 64)
  {
    printf("Error: NO SPACE IN MINODE ARRAY\n");
    return 0;
  }
  //mailmans alg to get block number and offset
  blk = (ino-1)/8 + inodeBegin;
  offset = (ino-1)%8;
  get_block(dev1, blk, buf);
  ip = (INODE *)buf + offset;
  memcpy(&(minode[i].INODE), ip, sizeof(INODE)); //Copy inode from disk into minode array using memcpy
  minode[i].dev = dev1;
  minode[i].ino = ino;
  minode[i].refCount = 1;
  minode[i].dirty = 0;
  minode[i].mounted = 0;
  minode[i].mountptr = NULL;
  return &minode[i];
}

//write the minode back to the disk
int iput(int dev, MINODE *mip)
{
  char buf[BLKSIZE];
  int blk, offset;
  INODE *tip;

  mip->refCount--;  //we're writing back to the disk, so we need to decrement it since we are done using it
  if(mip->refCount > 0) {return 1;} //if its not 0, then someone else is still using it
  if(mip->dirty == 0) {return 1;} // if its not dirty then it wasnt modified, so no need to write it, so return

  //Must write INODE back to disk
  //use mailmans alg to get the block and offset for the inode
  blk = (mip->ino-1)/8 + inodeBegin;
  offset = (mip->ino-1)%8;

  get_block(dev, blk, buf); //load the selected block into memory

  tip = (INODE*)buf + offset;
  memcpy(tip, &(mip->INODE), sizeof(INODE)); //use memcpy to copy the value in minode to the temp ip
  put_block(mip->dev, blk, buf); //use put block to put the block into memory after we wrote the bata
  return 1;
}





///change the directory
int cd(char* pathname)
{
  MINODE *mip;
  unsigned int ino;
  if(!pathname || !pathname[0] || (pathname[0] == '/' && !pathname[1])){ ino = root->ino;} //if the pathname is empty or is / then we use the root ino
  else { ino = getino(dev, pathname);} //else we get the ino fro the pathname using getino
  if(ino <= 0) //check if the inode is valid
  {
    printf("Error: Invalid Pathname\n");
    return 0;
  }
  mip = iget(dev, ino); //load the inode into memory
  //Verify inode is a dir
  if(!S_ISDIR(mip->INODE.i_mode)) // Use this to check if it a directory or not
  {
    printf("Error: End of path is not a directory\n");
    iput(dev, mip); //write the minode back and exit
    return 0;
  }
  iput(dev, running->cwd); //put the cwd minode away
  running->cwd = mip;   //set new minode to the mip we got earlier via the pathname.
}

//prints the working directory
int do_pwd(char *pathname)
{
  printf("cwd = ");
  pwd(running->cwd); //recursive
  printf("\n");
}
//acutal pwd
int pwd(MINODE *wd)
{
  int ino = 0;
  MINODE *next = NULL;
  char temp[256];
  if(wd == root) //if our working directory is only the root then we just have '/'
  {
    printf("/");
    return 1;
  }
  //Get parent's MINODE
  ino = search(dev, "..", &(wd->INODE));
  if(ino <= 0) //if this is >0 we have a problem.
  {
    printf("ERROR: Bad inode\n");
    return -1;
  }
  next = iget(dev, ino); //load the parents inode into memory
  if(!next)
  {
    printf("ERROR: Could not find inode\n");
    return -1;
  }
  pwd(next); //recursively call pwd until we reach the root
  memset(temp, 0, 256);
  searchByIno(next->dev, wd->ino, &(next->INODE), temp); //search via the inno number to get the name of the directories
  printf("%s/", temp); //print the name of the directories
  iput(next->dev, next); //put the inodes back
  return 1;
}


//allocate space for an inode
int ialloc(int dev1)
{
  int i;
  char buf[BLKSIZE];            // BLKSIZE=block size in bytes

  // get inode Bitmap into buf[ ]

  get_block(dev1, imap, buf);     //loads the imap block into the buf sp we cam modify it

  for (i=0; i < ninodes; i++){  // assume you know ninodes
    if (tst_bit(buf, i)==0){    // tests to see if the bit is 1 or 0, in use or not
      set_bit(buf, i);          //sets the bit to 1, its in use now
      put_block(dev1, imap, buf);   // write imap block back to disk

      // update free inode count in SUPER and GD on dev
      decFreeInodes(dev1);       // decr the number of free inodes on the device
      return (i+1);             //returns the position
    }
  }
  return 0;                     // no more FREE inodes
}


//de-allocs an inode
int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];

  get_block(dev, imap, buf); //loads the imap into the buf
  clr_bit(buf, ino-1);          //clears the bit from 1 to 0 signifying that its no longer in use
  put_block(dev, imap, buf); //writes the biutmap back
  incFreeInodes(dev);           //incr the number of free nodes on the device
}

//allocates space for a block
int balloc(int dev1)
{
  int i;
  char buf[BLKSIZE];            // BLKSIZE=block size in bytes

  get_block(dev1, bmap, buf);

  for (i=0; i < BLKSIZE; i++){  // assume you know ninodes
    if (tst_bit(buf, i)==0){    // tests if the bit is 1 or 0, looking for a free block
      set_bit(buf, i);          // sets a bit rom 1 to 0 signifying its in use
      put_block(dev1, bmap, buf);   // write bmap block back to disk

      // update free inode count in SUPER and GD on dev
      decFreeBlocks(dev1);       // decr the number of free blocks on the device
      memset(buf, 0, BLKSIZE); //set the buf to 0
      put_block(dev1, i+1, buf);  //write contents of buf to the block at that location
      return (i+1);
    }
  }
  return 0;                     // no more FREE inodes
}

//decallocates a block
int bdalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];

  get_block(dev, bmap, buf);  //loads the block map into the buf
  clr_bit(buf, ino-1);        //sets the bit from 1 to 0
  put_block(dev, bmap, buf);  //writes the block map back
  incFreeBlocks(dev);         //increases the number of free blocks
}

//Quit program iputs all dirty MINODEs
int quit(char* pathname)
{
  int i = 0;
  char str[256];
  for(i = 0; i < 64; i++) //go througth all minodes
  {
    if(minode[i].refCount > 0)
    {
      if(minode[i].dirty != 0) //if theyre dirty write em back to the block
      {
        minode[i].refCount = 1;
        iput(dev, &minode[i]);
      }
    }
  }
  printf("Good-Bye.\n");
  exit(0);
}

//tests if a bite is 1 or 0
int tst_bit(char* buf, int i)
{
  int byt, offset;
  byt = i/8;
  offset = i%8;
  return (((*(buf+byt))>>offset)&1);
}
//sets a bit
int set_bit(char* buf, int i)
{
  int byt, offset;
  char temp;
  char *tempBuf;
  byt = i/8;
  offset = i%8;
  tempBuf = (buf+byt);
  temp = *tempBuf;
  temp |= (1<<offset);
  *tempBuf = temp;
  return 1;
}
//clears a bit
int clr_bit(char* buf, int i)
{
  int byt, offset;
  char temp;
  char *tempBuf;
  byt = i/8;
  offset = i%8;
  tempBuf = (buf+byt);
  temp = *tempBuf;
  temp &= (~(1<<offset));
  *tempBuf = temp;
  return 1;
}

//decrements the number of free inodes
int decFreeInodes(int dev)
{
  char buf[BLKSIZE];
  get_super(dev, buf); //load the super block
  sp = (SUPER*)buf;
  sp->s_free_inodes_count -= 1; //dec the nuimnber of free inodes
  put_block(dev, SUPERBLOCK, buf); //write the super block back to the disk
  get_gd(dev, buf); //get the group descriptor
  gp = (GD*)buf;
  gp->bg_free_inodes_count -=1;  //dec the number of free nodes in the descriptor
  put_block(dev, GDBLOCK, buf); //write it back
  return 1;
}

//increment the number of free nodes
int incFreeInodes(int dev)
{
  char buf[BLKSIZE];
  get_super(dev, buf);
  sp = (SUPER*)buf;
  sp->s_free_inodes_count += 1;
  put_block(dev, SUPERBLOCK, buf);
  get_gd(dev, buf);
  gp = (GD*)buf;
  gp->bg_free_inodes_count +=1;
  put_block(dev, GDBLOCK, buf);
  return 1;

}

int decFreeBlocks(int dev1)
{
  char buf[BLKSIZE];
  get_super(dev, buf);
  sp = (SUPER*)buf;
  sp->s_free_blocks_count -= 1;
  put_block(dev1, SUPERBLOCK, buf);
  get_gd(dev1, buf);
  gp = (GD*)buf;
  gp->bg_free_blocks_count -=1;
  put_block(dev1, GDBLOCK, buf);
  return 1;
}

int incFreeBlocks(int dev1)
{
  char buf[BLKSIZE];
  get_super(dev, buf);
  sp = (SUPER*)buf;
  sp->s_free_blocks_count += 1;
  put_block(dev1, SUPERBLOCK, buf);
  get_gd(dev1, buf);
  gp = (GD*)buf;
  gp->bg_free_blocks_count +=1;
  put_block(dev1, GDBLOCK, buf);
  return 1;

}


//my own dirname. Used these because dirname() and basename() gave me garbage values
int myDirname(char *pathname, char buf[256])
{
  int i = 0;
  memset(buf, 0, 256);
  strcpy(buf, pathname); //get the pathname into a buiffer
  while(buf[i]) { i++; } //while the buffer is not null incremenet a counter
  while(i >= 0)          //go through the buffer
  {
    if(buf[i] == '/')   //if we find a '/' we then split.
    {
      buf[i+1] = 0;   //set that to 0, kinda like a stopping point
      //printf("parent = %s\n", buf); //this is our parent
      return 1;
    }
    i--;
  }
  buf[0] = 0;
  return 1;
}

//again, used this because basename() wasnt alwasy working. I was probably using it wrong
int myBasename(char *pathname, char *buf)
{
  int i = 0, j = 0;
  if(!pathname[0]) {return -1;} //if pathname is empty then we have an problem
  i = strlen(pathname);         //get the length of the PATHNAME
  while(i >= 0 && pathname[i] != '/') { i--; }  //go through until we find a '/'
  if(pathname[i] == '/')            //if we find one
  {
    i++;
    while(pathname[i])
    {
      buf[j++] = pathname[i++]; //basically copy this part of pathname into buffer
    }
    buf[j] = 0;
    return 1;
  }
  else { strncpy(buf, pathname, 256);} //if pathname[i] wasnt '/' then we already have out basename
  return 1;
}

//makes a directory
int make_dir(char *pathname)
{
  int dev1, ino, r;
  char parent[256], child[256], origPathname[512];
  MINODE *mip;
  memset(parent, 0, 256);
  memset(child, 0, 256);
  memset(origPathname, 0, 512);

  strcpy(origPathname, pathname); //make a copy
  if(pathname[0] == '/') { dev1 = root->dev; }  //determine if we make it at the root or in the current directory
  else { dev1 = running->cwd->dev; }

  //get the parent and the child
  myDirname(pathname, parent);
  myBasename(origPathname, child);

  //get the ino for the parent
  ino = getino(dev1, parent);
  //check if the pathname is valid
  if(ino <= 0)
  {
    printf("Error: Invalid pathname.\n");
    return -1;
  }
  //valid pathname, so load the inode into memory
  mip = iget(dev1, ino);

  //check that the locatin we are creating a directory is in fact a directory
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Error. Not a directory.\n");
    iput(dev1, mip);
    return -1;
  }
  //now look to see if the child exists or not
  ino = search(dev1, child, &(mip->INODE));
  if(ino > 0)
  {
    printf("Error. Directory already exists.\n");
    iput(mip->dev, mip);
    return -1;
  }
  //printf("Going into the mkdir function\n");
  //call the bulk of the make dir function
  r = my_mkdir(mip, child);
  //write back to the block witht he idates we have
  iput(mip->dev, mip);
  return r;
}
//function that actually makes the directory
int my_mkdir(MINODE *pip, char child[256])
{
  int inumber, bnumber, idealLen, needLen, newRec, i, j;
  MINODE *mip;
  char *cp;
  DIR *dpPrev;
  char buf[BLKSIZE];
  char buf2[BLKSIZE];
  int blk[256];

  //get an inode number and a block number for the new directory
  inumber = ialloc(pip->dev);
  bnumber = balloc(pip->dev);

  //load the new inode into memory
  mip = iget(pip->dev, inumber);

  //Write contents into inode for Directory entry
  mip->INODE.i_mode = 0x41ED; //directory mode
  mip->INODE.i_uid = running->uid;
  mip->INODE.i_gid = running->gid;
  mip->INODE.i_size = BLKSIZE;
  mip->INODE.i_links_count = 2;
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
  mip->INODE.i_blocks = 2;
  mip->dirty = 1;
  for(i = 0; i <15; i++) { mip->INODE.i_block[i] = 0; } //set all its blocks t onull

  mip->INODE.i_block[0] = bnumber;
  //write it to the disk
  iput(mip->dev, mip);

  //Write . and .. entries
  dp = (DIR*)buf;
  dp->inode = inumber;
  strncpy(dp->name, ".", 1);
  dp->name_len = 1;
  dp->rec_len = 12;
  //printf("Wrote \".\" entry\n"); //FOR TESTING

  cp = buf + 12;
  dp = (DIR*)cp;
  dp->inode = pip->ino;
  dp->name_len = 2;
  strncpy(dp->name, "..", 2);
  dp->rec_len = BLKSIZE - 12;

  //write this block back to memory
  put_block(pip->dev, bnumber, buf);

  // Put name into parents directory
  memset(buf, 0, BLKSIZE);
  needLen = 4*((8+strlen(child)+3)/4);
  //find the last available block
  bnumber = findLastBlock(pip);
  //Check if rooom in last block in parents directory
  //load the last block into the buffer
  get_block(pip->dev, bnumber, buf);

  cp = buf;
  dp = (DIR*)cp;
  //traverse to the last item in the block
  while((dp->rec_len + cp) < buf+BLKSIZE)
  {
    cp += dp->rec_len;
    dp = (DIR*)cp;
  }
  idealLen = 4*((8+dp->name_len+3)/4);

  //check for room in the block
  if(dp->rec_len - idealLen >= needLen) //There is room in this block
  {
    //printf("There is room in this block\n");

    //update the contents of the directory
    newRec = dp->rec_len - idealLen;
    dp->rec_len = idealLen;
    cp += dp->rec_len;
    dp = (DIR*)cp;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = newRec;
  }
  else // Allocate new data block
  {
    //get a new block
    bnumber = balloc(pip->dev);
    dp = (DIR*)buf;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = BLKSIZE;
    //ad the info to the last block
    addLastBlock(pip, bnumber);
  }

  //write the block to the disk
  put_block(pip->dev, bnumber, buf);
  pip->dirty = 1;
  pip->INODE.i_links_count++;
  memset(buf, 0, BLKSIZE);
  //printf("Finding parent's ino\n");
  //check that its there
  searchByIno(pip->dev, pip->ino, &running->cwd->INODE, buf);
  //update write time of the parent
  touch(buf);
  return 1;
}


//finds the last block in an minode
int findLastBlock(MINODE *pip)
{
  int buf[256];
  int buf2[256];
  int bnumber, i, j;

  //Find last used block in parents directory
  if(pip->INODE.i_block[0] == 0) {return 0;}
  for(i = 0; i < 12; i++) //Check direct blocks
  {
    if(pip->INODE.i_block[i] == 0)
    {
      return (pip->INODE.i_block[i-1]);
    }
  }
  if(pip->INODE.i_block[12] == 0) {return pip->INODE.i_block[i-1];}
  //if we are here then we need to go through the indirect blocks
  //load the block into the buffer
  get_block(dev, pip->INODE.i_block[12], (char*)buf);
  for(i = 0; i < 256; i++)
  {
    //look for the free blocks
    if(buf[i] == 0) {return buf[i-1];}
  }
  //if we are here then we have to look into the double indirect blocks
  if(pip->INODE.i_block[13] == 0) {return buf[i-1];}
  //Print dirs in double indirect blocks
  memset(buf, 0, 256);
  get_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
  for(i = 0; i < 256; i++)
  {
    if(buf[i] == 0) {return buf2[j-1];}
    if(buf[i])
    {
      get_block(pip->dev, buf[i], (char*)buf2);
      for(j = 0; j < 256; j++)
      {
        if(buf2[j] == 0) {return buf2[j-1];}
      }
    }
  }
}

//adds to the last block of an minodes
int addLastBlock(MINODE *pip, int bnumber)
{
  int buf[256];
  int buf2[256];
  int i, j, newBlk, newBlk2;
  //Find last used block in parents directory
  for(i = 0; i < 12; i++) //Check direct blocks
  {
    if(pip->INODE.i_block[i] == 0) {pip->INODE.i_block[i] = bnumber; return 1;}
  }
  if(pip->INODE.i_block[12] == 0) //Have to make indirect block
  {
    //allocate spot for a block
    newBlk = balloc(pip->dev);
    pip->INODE.i_block[12] = newBlk;
    memset(buf, 0, 256);
    //load the block into the buf
    get_block(pip->dev, newBlk, (char*)buf);
    buf[0] = bnumber;
    //write the block back to the disk
    put_block(pip->dev, newBlk, (char*)buf);
    return 1;
  }
  //if we're here then we have to tdo the same as above but for the indirect block
  memset(buf, 0, 256);
  get_block(pip->dev, pip->INODE.i_block[12], (char*)buf);
  for(i = 0; i < 256; i++)
  {
    if(buf[i] == 0) {buf[i] = bnumber; return 1;}
  }
  if(pip->INODE.i_block[13] == 0) //Make double indirect block
  {
    newBlk = balloc(pip->dev);
    pip->INODE.i_block[13] = newBlk;
    memset(buf, 0, 256);
    get_block(pip->dev, newBlk, (char*)buf);
    newBlk2 = balloc(pip->dev);
    buf[0] = newBlk2;
    put_block(pip->dev, newBlk, (char*)buf);
    memset(buf2, 0, 256);
    get_block(pip->dev, newBlk2, (char*)buf2);
    buf2[0] = bnumber;
    put_block(pip->dev, newBlk2, (char*)buf2);
    return 1;
  }
  memset(buf, 0, 256);
  get_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
  for(i = 0; i < 256; i++)
  {
    if(buf[i] == 0)
    {
      newBlk2 = balloc(pip->dev);
      buf[i] = newBlk2;
      put_block(pip->dev, pip->INODE.i_block[13], (char*)buf);
      memset(buf2, 0, 256);
      get_block(pip->dev, newBlk2, (char*)buf2);
      buf2[0] = bnumber;
      put_block(pip->dev, newBlk2, (char*)buf2);
      return 1;
    }
    memset(buf2, 0, 256);
    get_block(pip->dev, buf[i], (char*)buf2);
    for(j = 0; j < 256; j++)
    {
      if(buf2[j] == 0) {buf2[j] = bnumber; return 1;}
    }
  }
  printf("Error. Could not add block to node\n");
  return -1;
}

//updates the access tiems of a file and or creats a file if needed
int touch (char* name)
{
  char buf[1024];
  int ino;
  MINODE *mip;

  //check for the file
  ino = getino(dev, name);
  //of the inode is invalid, weneed to creat a file
  if(ino <= 0)
  {
    creat_file(name);
    return 1;
  }
  //load the inode into memory
  mip = iget(dev, ino);
  //update the times of the inode
  mip->INODE.i_atime = mip->INODE.i_mtime = mip->INODE.i_ctime =(unsigned)time(NULL);
  mip->dirty = 1;
  //write back to the disk
  iput(mip->dev, mip);
  return 1;
}

//change the mode/permissions of a file
int my_chmod(char* pathname)
{
  char buf[1024];
  char nMode[256];
  char path[256];
  int ino, newMode, i;
  MINODE* mip;

  //splits the pathname based off of " "
  if(split_paths(pathname, nMode, path) <= 0) { return -1; }
  //converts nMode into a unsigned octal
  newMode = strtoul(nMode, NULL, 8);
  //check if the path is valid
  ino = getino(dev, path);
  if(ino <= 0)
  {
    printf("Error: Invalid path\n");
    return -1;
  }

  //load the inode into memory
  mip = iget(dev, ino);
  //bitwise not operation
  i = ~0x1FF;
  //change the imode using bitwise & and |
  mip->INODE.i_mode &= i;
  mip->INODE.i_mode |= newMode;
  mip->dirty = 1;
  iput(dev, mip);
  return 1;
}

//splits the paths based off " "
int split_paths(char *original, char *path1, char *path2)
{
  char *temp;
  //printf("Original = %s\n", original);
  temp = strtok(original, " ");
  strcpy(path1, temp);
  //printf("Path1 = %s\n", path1);
  temp = strtok(NULL, " ");
  if(temp == NULL)
  {
    printf("Error: No second path given\n");
    return -1;
  }
  strcpy(path2, temp);
  return 1;
}

//creates a file
int creat_file(char* pathname)
{
  int dev1, ino, r;
  char parent[256];
  char child[256];
  MINODE *mip;
  memset(parent, 0, 256);
  memset(child, 0, 256);

  //if the pathname is / then we need to start at the ROOT
  if(pathname[0] == '/') { dev1 = root->dev;}
  else { dev1 = running->cwd->dev; } //sle we start in the cwd

  //get the dirname and basename, we have a parent and a child
  myDirname(pathname, parent);
  myBasename(pathname, child);

  //get the ino of a parent chec if its valid
  ino = getino(dev1, parent);
  if(ino <= 0)
  {
    printf("Error: invalid path\n");
    return -1;
  }
  //load the inode into memory
  mip = iget(dev1, ino);
  //check if the parent is an inode or not
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Error: Not a directory\n");
    iput(dev1, mip);
    return -1;
  }
  //check if the directory/file already exists
  ino = search(dev1, child, &(mip->INODE));
  if(ino > 0)
  {
    printf("ERROR: File already exists\n");
    //put the inode back to the disk
    iput(mip->dev, mip);
    return -1;
  }
  //the bulk of the creat
  r = my_creat(mip, child);
  //write back to the disk
  iput(mip->dev, mip);
  return r;
}

//bulk of the create
int my_creat(MINODE *pip, char child[256])
{
  int inumber, bnumber, idealLen, needLen, newRec, i, j;
  MINODE *mip;
  char *cp;
  DIR *dpPrev;
  char buf[BLKSIZE];
  char buf2[BLKSIZE];
  int blk[256];

  //get an allocated inode for the file
  inumber = ialloc(pip->dev);
  //load the inode into memory
  mip = iget(pip->dev, inumber);

  //Write contents into inode for Directory entry
  mip->INODE.i_mode = 0x81A4;
  mip->INODE.i_uid = running->uid;
  mip->INODE.i_gid = running->gid;
  mip->INODE.i_size = 0;
  mip->INODE.i_links_count = 1;
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
  mip->INODE.i_blocks = 0;
  mip->dirty = 1;
  for(i = 0; i <15; i++)
  {
    mip->INODE.i_block[i] = 0;
  }
  iput(mip->dev, mip);
  // Put name into parents directory
  memset(buf, 0, BLKSIZE);
  needLen = 4*((8+strlen(child)+3)/4);
  bnumber = findLastBlock(pip);
  //Check if rooom in last block in parents directory
  get_block(pip->dev, bnumber, buf);
  dp = (DIR*)buf;
  cp = buf;

  while((dp->rec_len + cp) < buf+BLKSIZE)
  {
    cp += dp->rec_len;
    dp = (DIR*)cp;
  }
  idealLen = 4*((8+dp->name_len+3)/4);
  if(dp->rec_len - idealLen >= needLen) //There is room in this block
  {
    newRec = dp->rec_len - idealLen;
    dp->rec_len = idealLen;
    cp += dp->rec_len;
    dp = (DIR*)cp;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = newRec;
  }
  else // Allocate new data block
  {
    bnumber = balloc(pip->dev);
    dp = (DIR*)buf;
    dp->inode = inumber;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = BLKSIZE;
    addLastBlock(pip, bnumber);
  }
  //printf("Putting parent block back\n"); //FOR TESTING

  //write the bnlock back to memory
  put_block(pip->dev, bnumber, buf);
  pip->dirty = 1;
  memset(buf, 0, BLKSIZE);
  searchByIno(pip->dev, pip->ino, &running->cwd->INODE, buf);
  touch(buf);
  return 1;
}

//removes a directory
int my_rmdir(char *pathname)
{
  int ino, i;
  char parent[256], child[256], origPathname[512];
  MINODE *pip = NULL;
  MINODE *mip = NULL;
  //create a copy
  strcpy(origPathname, pathname);
  //printf("pathname = %s\n", pathname);
  //check for a directory
  if(!pathname || !pathname[0])
  {
    printf("Error: No directory given\n");
  }
  else
  {
    //load inode
    ino = getino(dev, pathname);
  }
  if(0 >= ino) //check the inode is valid
  {
    printf("Invalid pathname\n");
    return -1;
  }
  //load into memory and make sure its a dir
  mip = iget(dev, ino);
  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("Error: Not a directory\n");
    iput(mip->dev, mip);
    return -1;
  }
  //check if the directory is currently in use
  if(mip->refCount > 1)
  {
    printf("Error: Directory in use\n");
    return -1;
  }

  //Check if empty
  if(mip->INODE.i_links_count > 2)
  {
    printf("Error: Directory not empty\n");
    iput(mip->dev, mip);
    return -1;
  }
  //Check if files exist in directory by checking its data blocks
  if(is_empty(mip) != 0)
  {
    printf("Error: Directory not empty\n");
    iput(mip->dev, mip);
    return -1;
  }
  //deallocate the blocks in the inode
  for(i = 0; i < 12; i++)
  {
    if(mip->INODE.i_block[i] != 0)
    {
      bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
  }
  //finally deallocate the inode
  idalloc(mip->dev, mip->ino);

  //get the dirname and basename
  myDirname(origPathname, parent);
  myBasename(origPathname, child);
  //printf("dirname = %s, basename = %s\n", parent, child);
  ino = getino(mip->dev, parent);
  pip = iget(mip->dev, ino);
  iput(mip->dev, mip);
  //printf("Going to remove child now!\n");
  rm_child(pip, child);
  //decremenet the counts of the links
  pip->INODE.i_links_count--;
  //printf("pip->links = %d\n", pip->INODE.i_links_count);
  touch(parent);
  pip->dirty = 1;
  iput(pip->dev, pip);
  return 1;
}

//removes a child from a parent directory
int rm_child(MINODE *pip, char *child)
{
  int i, size, found = 0;
  char *cp, *cp2;
  DIR *dp, *dp2, *dpPrev;
  char buf[BLKSIZE], buf2[BLKSIZE], temp[256];

  memset(buf2, 0, BLKSIZE);
  //checks the direct Blocks
  for(i = 0; i < 12; i++)
  {
    if(pip->INODE.i_block[i] == 0) { return 0; } //if its null return

    //load the block into memory
    get_block(pip->dev, pip->INODE.i_block[i], buf);
    dp = (DIR *)buf;
    dp2 = (DIR *)buf;
    dpPrev = (DIR *)buf;
    cp = buf;
    cp2 = buf;

    //while we havent reched the end of the block and we havent found the last item in the block
    while(cp < buf+BLKSIZE && !found)
    {

      memset(temp, 0, 256);
      strncpy(temp, dp->name, dp->name_len);
      if(strcmp(child, temp) == 0)
      {
        //if child is only entry in block
        if(cp == buf && dp->rec_len == BLKSIZE)
        {
          //deallocate the block
          bdalloc(pip->dev, pip->INODE.i_block[i]);
          pip->INODE.i_block[i] = 0;
          pip->INODE.i_blocks--;
          found = 1;
        }
        //else delete child and move entries over left
        else
        {
          while((dp2->rec_len + cp2) < buf+BLKSIZE)
          {
            dpPrev = dp2;
            cp2 += dp2->rec_len;
            dp2 = (DIR*)cp2;
          }
          if(dp2 == dp) //Child is last entry
          {
            //printf("Child is last entry\n");
            dpPrev->rec_len += dp->rec_len;
            found = 1;
          }
          else
          {
            //printf("Child is not the last entry\n");
            //need to move things to the left
            size = ((buf + BLKSIZE) - (cp + dp->rec_len));
            //printf("Size to end = %d\n", size);
            dp2->rec_len += dp->rec_len;
            //printf("dp2 len = %d\n", dp2->rec_len);

            //copy the contents over
            memmove(cp, (cp + dp->rec_len), size);
            dpPrev = (DIR*)cp;
            memset(temp, 0, 256);
            //copy the name over
            strncpy(temp, dpPrev->name, dpPrev->name_len);
            //printf("new dp name = %s\n", temp);
            found = 1;
          }
        }
      }
      cp += dp->rec_len;
      dp = (DIR*)cp;
    }
    if(found)
    {
      //write back to the disk
      put_block(pip->dev, pip->INODE.i_block[i], buf);
      return 1;
    }
  }
  printf("Error: Child not found\n");
  return -1;
}

//check to see if a given directory is empty or not
int is_empty(MINODE *mip)
{
  int i;
  char *cp;
  DIR *dp;
  char buf[BLKSIZE], temp[256];

  //checks the blocks of the inode
  for(i = 0; i < 12; i++)
  {
    if(ip->i_block[i] == 0) { return 0; } //if its null then return

    get_block(dev, ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    //go through the items in the directory
    while(cp < buf+BLKSIZE)
    {
      memset(temp, 0, 256);
      strncpy(temp, dp->name, dp->name_len);
      //printf("%s = temp\n",temp);
      //ignore . and ..
      if(strncmp(".", temp, 1) != 0 && strncmp("..", temp, 2) != 0) { return 1;}
      cp += dp->rec_len;
      dp = (DIR*)cp;
    }
  }
}


//functino to link
int my_link(char* pathname)
{
  char oldFile[256], newFile[256], parent[256], child[256], buf[BLKSIZE];
  int ino, ino2, bnumber, needLen, idealLen, newRec;
  MINODE *mip, *mip2;
  char *cp;
  DIR *dp;
  //split into the two files
  if(0 >= split_paths(pathname, oldFile, newFile)) { return -1; }
  //printf("newFile: %s\n",newFile);
  //check that first file exists
  ino = getino(dev, oldFile);

  if(ino <= 0)
  {
    printf("Error. File doesnt exist.\n");
    return -1;
  }

  //load the iunode into memory
  mip = iget(dev, ino);
  //check that it is a regular file
  if(!S_ISREG(mip->INODE.i_mode))
  {
    printf("Error. Not a regular file\n");
    iput(mip->dev, mip);
    return -1;
  }
  //ge the parent and child
  myDirname(newFile, parent);
  myBasename(newFile, child);

  //printf("parent: %s\n",child);
  //printf("child: %s\n", parent);

  //check if the second file exists or not
  ino2 = getino(mip->dev, parent);
  if(0 >= ino2)
  {
    printf("Error: File doesnt exist");
    iput(mip->dev, mip);
    return -1;
  }
  mip2 = iget(mip->dev, ino2);
  //check that a parent is a dir or not
  if(!S_ISDIR(mip2->INODE.i_mode))
  {
    printf("Error: Not a directory\n");
    iput(mip->dev, mip);
    iput(mip2->dev, mip2);
    return -1;
  }
  //printf("Child = %s\n", child);
  //check if the file exists or not
  ino2 = search(mip2->dev, child, &(mip2->INODE));
  if(ino2 > 0)
  {
    printf("Error: File already exists\n");
    iput(mip->dev, mip);
    iput(mip2->dev, mip2);
    return -1;
  }

  //Put name in parents block
  memset(buf, 0, BLKSIZE);
  needLen = 4*((8+strlen(child)+3)/4);
  bnumber = findLastBlock(mip2);
  //Check if rooom in last block in parents directory
  get_block(mip2->dev, bnumber, buf);
  dp = (DIR*)buf;
  cp = buf;

  while((dp->rec_len + cp) < buf+BLKSIZE)
  {
    cp += dp->rec_len;
    dp = (DIR*)cp;
  }
  idealLen = 4*((8+dp->name_len+3)/4);
  if(dp->rec_len - idealLen >= needLen) //There is room in this block
  {
    newRec = dp->rec_len - idealLen;
    dp->rec_len = idealLen;
    cp += dp->rec_len;
    dp = (DIR*)cp;
    dp->inode = ino;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = newRec;
  }
  else // Allocate new data block
  {
    bnumber = balloc(mip2->dev);
    dp = (DIR*)buf;
    dp->inode = ino;
    dp->name_len = strlen(child);
    strncpy(dp->name, child, dp->name_len);
    dp->rec_len = BLKSIZE;
    addLastBlock(mip2, bnumber);
  }
  //printf("Putting parent block back\n"); //FOR TESTING

  put_block(mip2->dev, bnumber, buf);
  mip->dirty = 1;
  mip->INODE.i_links_count++;
  memset(buf, 0, BLKSIZE);
  searchByIno(mip2->dev, mip2->ino, &running->cwd->INODE, buf);
  iput(mip->dev, mip);
  iput(mip2->dev, mip2);
  return 1;
}

//unlinks a file (removes it!!)
int my_unlink(char *pathname)
{
  char oldFile[256], parent[256], child[256], buf[BLKSIZE];
  int ino, ino2, bnumber, needLen, idealLen, newRec;
  MINODE *mip, *mip2;
  char *cp;
  DIR *dp;

  //check that the file is valid
  ino = getino(dev, pathname);
  if(ino <= 0)
  {
    printf("Error: File doesnt exist\n");
    return -1;
  }
  //load inode into memory and make sure its a reg file or a link
  mip = iget(dev, ino);
  if(!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode))
  {
    printf("Error: not a file\n");
    iput(mip->dev, mip);
    return -1;
  }
  //dec the link counts
  mip->INODE.i_links_count--;
  //if there is no more things in the directory then just remove it
  if(mip->INODE.i_links_count <= 0)
  {
    rm(mip);
  }

  //get parent and child of the pathname
  myDirname(pathname, parent);
  myBasename(pathname, child);
  //load the parent inode into memory
  ino2 = getino(mip->dev, parent);

  //checkif there is a child, if is then you remove it
  if(ino2 <= 0) {return -1;}
  mip2 = iget(mip->dev, ino2);
  rm_child(mip2, child);
  iput(mip->dev, mip);
  iput(mip->dev, mip2);
}

//remove function
int rm(MINODE *mip)
{
  int i, j;
  int buf[256], buf2[256];

  if(!S_ISLNK(mip->INODE.i_mode))
  {
    for(i = 0; i < 12; i++)
    {
      if(mip->INODE.i_block[i] != 0)
      {
        //deallocate the blocks of the child
        bdalloc(mip->dev, mip->INODE.i_block[i]);
      }
    }
    //deallocate indriect blocks if needed
    if(mip->INODE.i_block[12] != 0)
    {
      memset(buf, 0, 256);
      get_block(mip->dev, mip->INODE.i_block[12], (char*)buf);
      for(i = 0; i < 256; i++)
      {
        if(buf[i] != 0) {bdalloc(mip->dev, buf[i]);}
      }
      bdalloc(mip->dev, mip->INODE.i_block[12]);
    }
    //deallocate douible indriect blocks if needed
    if(mip->INODE.i_block[13] != 0)
    {
      memset(buf, 0, 256);
      get_block(mip->dev, mip->INODE.i_block[13], (char*)buf);
      for(i = 0; i < 256; i++)
      {
        if(buf[i] != 0)
        {
          memset(buf2, 0, 256);
          get_block(mip->dev, buf[i], (char*)buf2);
          for(j = 0; j < 256; j++)
          {
            if(buf2[j] != 0) {bdalloc(mip->dev, buf2[j]);}
          }
          bdalloc(mip->dev, buf[i]);
        }
      }
      bdalloc(mip->dev, mip->INODE.i_block[13]);
    }
  }
  //finally deallocate the inode
  idalloc(mip->dev, mip->ino);
  return 1;
}
//creates a symbolic link between two files
int my_symlink(char *pathname)
{
  char oldname[256], newname[256];
  int ino;
  MINODE *mip;

  //get the name of  the two files`
  if(split_paths(pathname, oldname, newname) <= 0) {return -1;}
  if(0 >= (ino = getino(dev, oldname)))
  {
    printf("Error: File does not exist\n");
    return -1;
  }
  //create a new file
  creat_file(newname);
  if(0 >= (ino = getino(dev, newname)))
  {
    printf("Error: Could not create file\n");
    return -1;
  }
  mip = iget(dev, ino);
  mip->INODE.i_mode = 0120000; //symlink mode
  mip->dirty = 1;

  strcpy((char*)mip->INODE.i_block, oldname);
  iput(mip->dev, mip);
}


//basically removed everythhing from a file
int my_truncate(MINODE *mip)
{
  int buf[256];
  int buf2[256];
  int bnumber, i, j;

  if(mip == NULL)
  {
    printf("Error: No file.\n");
    return -1;
  }

  // Deallocate all used blocks
  for(i = 0; i < 12; i++) //Check direct blocks
  {
    if(mip->INODE.i_block[i] != 0)
    {
      bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
  }
  //Indirect blocks
  if(mip->INODE.i_block[12] == 0) {return 1;}
  get_block(dev, mip->INODE.i_block[12], (char*)buf);
  for(i = 0; i < 256; i++)
  {
    if(buf[i] != 0) {bdalloc(mip->dev, buf[i]);}
  }
  bdalloc(mip->dev, mip->INODE.i_block[12]);
  if(mip->INODE.i_block[13] == 0) {return 1;}

  //deallocate all double indirect blocks
  memset(buf, 0, 256);
  get_block(mip->dev, mip->INODE.i_block[13], (char*)buf);
  for(i = 0; i < 256; i++)
  {
    if(buf[i])
    {
      get_block(mip->dev, buf[i], (char*)buf2);
      for(j = 0; j < 256; j++)
      {
        if(buf2[j] != 0) {bdalloc(mip->dev, buf2[j]);}
      }
      bdalloc(mip->dev, buf[i]);
    }
  }
  bdalloc(mip->dev, mip->INODE.i_block[13]);
  mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
  mip->INODE.i_size = 0;
  mip->dirty = 1;
  return 1;
}






//funciton to ls
int do_ls(char *path)
{

  unsigned long ino;
  MINODE *mip, *pip;
  int device = running->cwd->dev;
  char *child;

  // if there was no additional argument, print the cwd
  if(path[0] == 0)
  {
    mip = iget(device, running->cwd->ino);

    printChild(device, mip);

  }
  // otherwise...
  else
  {
    // if we're to print the root, change device to it
    if(path[0] == '/')
    {
      device = root->dev;
    }
    // get the inode for the current path part

    ino = getino(device, path);

    // pathname doesn't exist
    if( 0 >= ino){	return 1;}

    mip = iget(device, ino);

    // if we're not looking at a directory...
    if(((mip->INODE.i_mode) & 0040000)!= 0040000)
    {
      // find the parent of the path part and get its's basename
      if(findparent(path))
        child = basename(pathname);
      // if it doesn't exist, get the name of the child path
      else
      {
        child = (char *)malloc((strlen(pathname) + 1) * sizeof(char));
        strcpy(child, path);
      }
      // print the child path
      printFile(mip, child);
      return 0;
    }

    // print child part
    printChild(device, mip);
  }

  iput(mip->dev, mip);
  return 0;
}

// print information on a file
void printFile(MINODE *mip, char *namebuf)
{
  char *Time;
  unsigned short mode;
  int type;

  mode = mip->INODE.i_mode;

  // print out info in the file in same format as ls -l in linux
  // print the file type
  if((mode & 0120000) == 0120000)
  {
    printf("l");
    type = LINK;
  }
  else if((mode & 0040000) == 0040000)
  {
    printf("d");
    type = DIRECTORY;
  }
  else if((mode & 0100000) == 0100000)
  {
    printf("-");
    type = FILE;
  }

  // print the permissions
  // uses bitwise and to check each byte in the mode.
  if((mode & (1 << 8)))
    printf("r");
  else
    printf("-");
  if((mode & (1 << 7)) )
    printf("w");
  else
    printf("-");
  if((mode & (1 << 6)) )
    printf("x");
  else
    printf("-");

  if((mode & (1 << 5)) )
    printf("r");
  else
    printf("-");
  if((mode & ( 1 << 4)) )
    printf("w");
  else
    printf("-");
  if((mode & (1 << 3)) )
    printf("x");
  else
    printf("-");
  if((mode & (1 << 2)) )
    printf("r");
  else
    printf("-");
  if((mode & (1 << 1)) )
    printf("w");
  else
    printf("-");
  if(mode & 1)
    printf("x");
  else
    printf("-");

  // print the file info
  printf("\t%d\t%d\t%d\t", mip->INODE.i_links_count, mip->INODE.i_size, mip->ino);
  Time = ctime( (const time_t *) &(mip->INODE.i_mtime));
  Time[strlen(Time) -1 ] = 0;
  printf("%s\t%s\t", Time, namebuf);
  

  // if this is a symlink file, show the file it points to
  if((mode & 0120000) == 0120000)
    printf(" => %s\n",(char *)(mip->INODE.i_block));
  else
    printf("\n");

  iput(mip->dev, mip); // write back to disk
}

// print contents of child directory
void printChild(int devicename, MINODE *mp)
{
  char buf[BLKSIZE], namebuf[256], *cp;
  DIR *dp;
  int i, ino;
  MINODE *temp;

  // cycle through direct blocks and print them
  for(i = 0; i < 12; i++)
  {

    if(mp->INODE.i_block[i])
    {
      get_block(devicename, mp->INODE.i_block[i], buf);
      cp = buf;
      dp = (DIR *)buf;

      while(cp < &buf[BLKSIZE])
      {

        strncpy(namebuf, dp->name, dp->name_len);
        namebuf[dp->name_len] = 0;

        ino = dp->inode;
        temp = iget(devicename, ino);

        printFile(temp, namebuf);

        cp+=dp->rec_len;
        dp = (DIR *)cp;
      }
    }
  }

}

// checks if there is a parent by looking for a '/'
int findparent(char *pathn)
{
  int i = 0;
  while(i < strlen(pathn))
  {
    if(pathn[i] == '/')
      return 1;
    i++;
  }
  return 0;
}



int fileOpen(char *pathname)
{
  char pathToFile[256], cFlags[256];
  int flags, ino, i;
  MINODE *mip;
  OFT *oftp; //ope file table

  //get the file anme and the open mode
  if(split_paths(pathname, pathToFile, cFlags) <= 0) { return -1; }
  //flags = atoi(cFlags);
  //printf("cFlags = %s, flags = %d\n",cFlags, (int)cFlags);
  if(strcmp("R", cFlags) == 0)
  {
    flags = 0;
  }
  if(strcmp("W", cFlags) == 0)
  {
    flags = 1;
  }

  if(strcmp("RW", cFlags) == 0)
  {
    flags = 2;
  }

  if(strcmp("APPEND", cFlags) == 0)
  {
    flags = 3;
  }
  //get the minode of the file
  ino = getino(dev, pathToFile);
  if( 0 >= ino)
  {
    //if file doesnt exist create it
    creat_file(pathToFile);
  }
  ino = getino(dev,pathToFile);
  if( 0 >= ino)
  {
    printf("Error opening file.\n");
    return -1;
  }
  //get the inode into memory
  mip = iget(dev, ino);
  //check that its a regular file
  if(!S_ISREG(mip->INODE.i_mode))
  {
    printf("Error. Not a regular file.\n");
    iput(mip->dev, mip);
    return -1;
  }

    //make sure file isnt in use
    for(i = 0; i < 10; i++)
    {
      if(running->fd[i] != NULL)
      {
        if(running->fd[i]->inodeptr == mip)
        {
          if(running->fd[i]->mode != 0 || flags != 0)
          {
            printf("Error. File in use.\n");
            iput(mip->dev, mip);
            return -1;
          }
        }
      }
    }

    //allocate a spot in the open file table
    oftp = (OFT *)malloc(sizeof(OFT));
    oftp->mode = flags;
    printf("Flag is: %d\n");
    oftp->refCount = 1;
    oftp->inodeptr = mip;
//set offset based off R W RW APPEND
    switch(flags)
    {
      case 0: oftp->offset = 0;
              printf("File opened for read-only\n");
              break;
      case 1: my_truncate(mip);
              printf("File open for write-only\n");
              oftp->offset = 0;
              break;
      case 2: oftp->offset = 0;
              printf("File open for read/write\n");
              break;
      case 3: oftp->offset = mip->INODE.i_size;
              printf("Open for Append\n");
              break;
      default: printf("Error. Flag is invalid. R W RW\n");
               iput(mip->dev, mip);
               free(oftp);
               return -1;
               break;
    }

  //check that ther actually is room in fd to open the file
  i = 0;
  while(running->fd[i] != NULL && i < 10){ i++; }
  if(i == 10) //fd isfull
  {
    printf("Error. No room to open the file. Close another first.\n");
    iput(mip->dev, mip);
    free(oftp);
    return -1;
  }
  //else its free. so put it in there
  printf("File fd is %d\n",i);
  running->fd[i] = oftp;
  if(flags != 0) {mip->dirty = 1;}
  return i;
}

int fileClose(char *path)
{
  MINODE *mip;
  OFT *oftp;
  int fd;
  if(!path[0])
  {
    printf("Error: No file desciptor given\n");
    return -1;
  }

  fd = atoi(path);
  printf("fd is: %d\n", fd);
  if(fd < 0 || fd > 9)
  {
    printf("Error. Not a valid fd\n");
    return -1;
  }

  //check the descriptor is in use
  if(running->fd[fd] == NULL)
  {
    printf("Error. file not currently open.\n");
    return -1;
  }
  //close everything.
  oftp = running->fd[fd];
  running->fd[fd] = 0;
  oftp->refCount--;
  if(oftp->refCount > 0) {return -1;} //there is someone else using it
  mip = oftp->inodeptr;
  //write the inode back to the disk
  iput(mip->dev, mip);
  free(oftp);
  printf("\nFile closed successfullly.\n");
  return 1;
}

int fileWrite(char *pathname)
{
  int fd;
  char writeMe[BLKSIZE];
  fd = atoi(pathname);
  if(fd < 0 || fd > 9)
  {
    printf("Error. Not a valid fd\n");
    return -1;
  }

  //check the descriptor is in use
  if(running->fd[fd] == NULL)
  {
    printf("Error. file not currently open.\n");
    return -1;
  }
  //check that we are able to write to it
  if(running->fd[fd]->mode == 0)
  {
    printf("Error. File is in read-only mode\n");
    return -1;
  }

  printf("What would you like to write?\n");
  fgets(writeMe, BLKSIZE, stdin);
  writeMe[strlen(writeMe) -1] = 0;
  if(writeMe[0] == 0)
  {
    return 0;
  }
  return myWrite(fd, writeMe, strlen(writeMe));
}

int myWrite(int fd, char *buf, int nbytes)
{
  int count, lblk, start, blk, dblk, remain;
  int ibuf[256], dbuf[256];
  char writeBuf[BLKSIZE], *cp, *cq;
  count = 0;
  while(nbytes)
  {
    //comoute logical block
    lblk = running->fd[fd]->offset / BLKSIZE;
    start = running->fd[fd]->offset % BLKSIZE;
    
    //convert logical block ot physical block
    if(lblk < 12 ) //direct blocks
    {
      blk = running->fd[fd]->inodeptr->INODE.i_block[lblk];
    }
    else if(12 <= lblk < 12 + 256) //indirect blocks
    {
      //load the block 12 inot ibuf
      memset(ibuf,0,256);
      get_block(running->fd[fd]->inodeptr->dev, running->fd[fd]->inodeptr->INODE.i_block[12], (char *)ibuf);
      blk = ibuf[lblk - 12];
    }
    else //double indirect blocks
    {
      memset(ibuf, 0, 256);
      get_block(running->fd[fd]->inodeptr->dev, running->fd[fd]->inodeptr->INODE.i_block[13], (char  *)dbuf);
      lblk -= (12 + 256);
      dblk = dbuf[lblk / 256];
      get_block(running->fd[fd]->inodeptr->dev, dblk, (char *)dbuf);
      blk = dbuf[lblk % 256];
    }



    memset(writeBuf,0,BLKSIZE);
    //read the blk we got into the write buffre
    get_block(running->fd[fd]->inodeptr->dev, blk, writeBuf);
    //init the cp and the remaining
    cp = writeBuf + start;
    remain = BLKSIZE - start;


    if(remain < nbytes) //copy remain
    {
      strncpy(cp, cq, remain); //copies all at once, as opposed to one at a time.
      count += remain;
      nbytes -= remain;
      running->fd[fd]->offset += remain;
      //checkc that the ofset is within bounds
      if(running->fd[fd]->offset > running->fd[fd]->inodeptr->INODE.i_size)
      {
        running->fd[fd]->inodeptr->INODE.i_size += remain;
      }
      remain -= remain;
    }
    else //copy nbytes
    {
      strncpy(cp, cq, nbytes);
      remain -= nbytes;
      count += nbytes;
      running->fd[fd]->offset += nbytes;
      //again  check that we are within bounds
      if(running->fd[fd]->offset > running->fd[fd]->inodeptr->INODE.i_size)
      {
        running->fd[fd]->inodeptr->INODE.i_size += nbytes;
      }
      nbytes -= nbytes;
    }
    //write it back
    put_block(running->fd[fd]->inodeptr->dev, blk, writeBuf);
    running->fd[fd]->inodeptr->dirty = 1;
    printf("Wrote %d chars into file.\n", count);
  }
}
