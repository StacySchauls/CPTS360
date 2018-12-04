#include "type.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

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
int quit(char *pathname);
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
int creat_file(char *pathname);
int my_creat(MINODE *pip, char child[]);
int my_mk(MINODE *pip, char child[256]);
int touch(char *name);
int addLastBlock(MINODE *pip, int bnumber);
int findLast(MINODE *pip);
int ialloc(int dev1);
int balloc(int dev1);
int test_bit(char *buf, int i);
int set_bit(char *buf, int i);
int clr_bit(char *buf, int i);
int decFreeInodes(int dev1);
int incFreeInodes(int dev1);
int incFreeBlocks(int dev1);
int touch(char *name);


int rm_dir(char *pathname);
int rm_child(MINODE *pip, char *child);
int is_empty(MINODE *mip);
int idalloc(int dev1, int ino);
int bdalloc(int dev1, int ino);






static char *cmnds[] = { "ls","pwd","cd","mkdir","rmdir","creat","link","symlink","unlink","chmod","quit"};
static int (*fptr[])(char *) = {(int (*)())ls, do_pwd, cd, make_dir, rm_dir,creat_file,mylink, mysymlink, myunlink, mychmod, quit};
