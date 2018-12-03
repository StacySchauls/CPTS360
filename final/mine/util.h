#include "type.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

/*Global Variables*/
int dev, DEBUG;
PROC *running;
PROC *readQueue;
PROC proc[NPROC];
MINODE *root;
MINODE minode[NMINODES];
MOUNT MountTable[NMOUNT];
int inodeBegin, bmap, imap, ninodes;
char pathname[128];




/*Functions*/
int init();
int mount_root(char *devName);
int get_block(int dev1, int blk, char *buf);
int get_super(int dev1, char *buf);
void get_inode_table(int dev1);
int is_ext2(char *buf);
int get_gd(int dev1, char *buf);
void menu();
int ls(char *path);
int do_pwd(char *pathname);
int cd(char *pathname);
int make_dir(char *pathname);
int rm_dir(char *pathname);
int mylink(char *pathname);
int mysymlink(char *pathname);
int myunlink(char *pathname);
int mychmod(char *pathname);
int getino(int dev1, char *path);
int searchByIno(int dev, int ino, INODE *ip, char *temp);

int pwd(MINODE *wd);
MINODE *iget(int dev1, unsigned int ino);
int findBlocks(INODE *ip, int printStat);
int printDirs(int block, int printStat);
int getino(int dev1, char *path);
int iput(int dev1, MINODE *mip);
int put_block(int dev1, int blk, char *buf);
int search(int dev1, char *str, INODE *ip);
char **tokenPath(char *path);










static char *cmnds[] = { "ls","pwd","cd","mkdir","rmdir","creat","link","symlink","unlink","chmod"};
static int (*fptr[])(char *) = {(int (*)())ls, do_pwd, cd, make_dir, rm_dir, mylink, mysymlink, myunlink, mychmod};
