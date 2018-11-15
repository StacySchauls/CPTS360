#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>

#define BLKSIZE 1024
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;



// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;
char *cp;
char dbuf[BLKSIZE];
int fd;
int dev;
int numTokens;
char *disk;
int iblock;
char pathname[256]; 
char temp[128];
char dname[128], bname[128];
char buf[BLKSIZE];
char *names[128];
int bno;


int get_block(int fd, int blk, char buf[ ]);
void initialize(int argc, char *argv[]);
int tokenize(char *pathname);
int search(INODE *ip, char *name);
