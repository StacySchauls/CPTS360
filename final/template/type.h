#ifndef TYPE_H
#define TYPE_H

/*	type.h for CS360 Project             */

#include <stdio.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE        1024
#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define BBITMAP           3
#define IBITMAP           4
#define INODEBLOCK        5
#define ROOT_INODE        2

// Default dir and regulsr file modes
#define DIR_MODE          0040777 
#define FILE_MODE         0100644
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define READY             1
#define RUNNING           2

// Table sizes
#define NMINODES         100
#define NMOUNT            10
#define NPROC             10
#define NFD               10
#define NOFT             100

// Open File Table
typedef struct oft{
  int   mode;
  int   refCount;
  struct minode *inodeptr;
  int   offset;
} OFT;

// PROC structure
typedef struct proc{
  int   uid;
  int   pid;
  int   gid;
  int   ppid;
  struct proc *parent;
  int   status;
  struct minode *cwd;
  OFT   *fd[NFD];
} PROC;
      
// In-memory inodes structure
typedef struct minode{		
  INODE    INODE;               // disk inode
  int      dev, ino;
  int      refCount;
  int      dirty;
  int      mounted;
  struct mount *mountptr;
} MINODE;

// Mount Table structure
typedef struct mount{
        int    ninodes;
        int    nblocks;
        int    dev;
        MINODE *mounted_inode;
        char   name[256]; 
        char   mount_name[64];
} MOUNT;

enum stat_mode
{
   MODE_REG = 0b1000,
   MODE_DIR = 0b0100,
   MODE_LNK = 0b1010
};

enum perm_mode
{
   USER_READ   = 0b100000000,
   USER_WRITE  = 0b010000000,
   USER_EXEC   = 0b001000000,
   GROUP_READ  = 0b000100000,
   GROUP_WRITE = 0b000010000,
   GROUP_EXEC  = 0b000001000,
   OTHER_READ  = 0b000000100,
   OTHER_WRITE = 0b000000010,
   OTHER_EXEC  = 0b000000001
};


#endif
