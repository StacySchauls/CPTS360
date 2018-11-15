#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>   // NOTE: Ubuntu users MAY NEED "ext2_fs.h"
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;
int DEBUG;
#define BLKSIZE  1024

#define NMINODE    64
#define NOFT       64
#define NFD        10
#define NMOUNT      4
#define NPROC       2
#define FREE        0
#define READY       1

//colors for neatness and debugging

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"



typedef struct minode{
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
  int mounted;
  struct mntable *mptr;
}MINODE;

typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          uid, gid,status,ppid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

int init();
int mount_root();
int get_block(int fd, int blk, char buf[]);
char *mygets(char *buf, size_t size);
MINODE *iget(int dev, int ino);
int findCmd(char *command);
int ls(char *pathname);
int statMe(MINODE *mp, char *pathname);
int print(MINODE *mip);
int getino(char *pathname);
int tokenize(char *pathname);
int search(INODE *ip, char *name);
int isDir(int ino);
int iput(MINODE *mip);
int put_block(int fd, int blk, char buf[]);
int tst_bit(char *buf, int bit);
int set_bit (char *buf, int bit);
int set_bit (char *buf, int bit);
int statMe2(int ino);
int ls2(char *pathname);
int quit();
int ch_dir(char *pathname);
int pwd();
int findino(MINODE *mip, int *myino, int *parentino);
int whatsMyName(MINODE *parent, int myino, char *myname);
int cdMe(char *pathname);
