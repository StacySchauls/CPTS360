#include "functions.h"

extern GD    *gp;
extern SUPER *sp;
extern INODE *ip;
extern DIR   *dp; 
extern int fd;
extern int dev;
extern int bno;
extern char *disk;
extern char pathname[256]; 
extern char temp[128];
extern char dname[128];
extern char bname[128];
extern char buf[BLKSIZE];
extern int numTokens;


/*******GET BLOCK*******/
int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

/******INIT / MAIN FUNCTION **********/
void initialize(int argc, char *argv[]){
  int ino, index, blk, offset, i;
  char ibuf[BLKSIZE];
  //make sure the arguments are provided
  if(argc < 3){
    printf("Must provide a device name and path\n");
    exit(-1);
  }
  disk = argv[1];
  printf("Using disk \'%s\'\n", disk);

  memset(pathname, 0, sizeof(pathname));
 // memcpy(pathname, argv[2], sizeof(argv[2]));
  strcpy(pathname, argv[2]);
  printf("argv2 is %s\n", argv[2]);
  printf("PATHNAME IS: %s\n",pathname);
  fd = dev = 0;
  fd = open(disk, O_RDONLY);
  if(fd < 0){
    printf("Opening %s failed.\n",disk);
    exit(-1);
  }


  //read super block
  get_block(fd,1, buf);
  sp = (SUPER *)buf;
  //check that its an ext2 file system by checking the magic number

  if(sp->s_magic != 0xEF53){
    printf("%s is not an ext2 file system.\n",argv[1]);
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
    if (index != numTokens - 1 && !S_ISDIR(ip->i_mode)) // If we are expecting a directory and it isn't one, exit
    {
      printf("\t%s is not a directory. Aborting.\n", names[index]);
      exit(-1);
    }

    bno = (ino - 1) / 8 + iblock; //disk block contains this inode
    ino = (ino - 1) % 8;       //offset of inode in this block
    get_block(fd, bno, ibuf);
    ip = (INODE *)ibuf + ino;  //ip -> new inode
   // printf("here\n");
  }

  //ip should be pointing at the inode of pathname
  //extract information from inode as needed:
  i = 0;
  while(1){
    if(ip->i_block[i] == 0)
        break;
    if(ip->i_block[i] == 12)
      printf("Indirect Blocks\n");
    if(ip->i_block[i] == 13)
      printf("Double Indirect Blocks\n");
    printf("i_block[%d]: %d\n",i,ip->i_block[i]);
    i++;
  }
}


/*******TOKENIZE**********/
int tokenize(char *pathname){
  char temp[256];
  int i, n, ino;
  i = 0;
 // printf("pathname is %s \n",pathname);
  //step 2
  //Tokenize the pathname into components
  char *token;

  token = strtok(pathname, "/");
  names[0] = (char *)malloc(strlen(token));
  strcpy(names[0], token);
 // printf("names[%d] = %s\n",i,token);
  i++;
  while(token = strtok(NULL, "/")){
    names[i] = (char *)malloc(strlen(token));
    strcpy(names[i], token);
    printf("names[%d] = %s\n",i,token);
    i++;
  }
  numTokens = i;
  return 0;
}


/**********SEARCH****************/
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
