#ifndef LAB_6_H
#define LAB_6_H
#include "type.h"
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

//Global Variables
int dev;
PROC *running;
PROC *readQueue;
PROC proc[NPROC];
MINODE *root;
MINODE minode[NMINODES];
MOUNT MountTable[NMOUNT];
int inodeBegin;
char pathname[128];
int bmap;
int imap;
int ninodes;
//char buf[BLKSIZE];

//Functions
int init();
int menu(char *pathname);
int get_block(int dev, int blk, char *buf);
int put_block(int dev, int blk, char *buf);
char** tokenPath(char *pathname);
int search(int dev, char *str, INODE *ip);
int searchByIno(int dev, int ino, INODE *ip, char *temp);
unsigned int getino(int dev, char *path);
MINODE *iget(int dev, unsigned int ino);
int iput(int dev, MINODE *mip);
int mount_root(char *devName);
int get_super(int dev, char *buf);
void get_inode_table(int dev);
int is_ext2(char *buf);
int get_gd(int dev, char *buf);
int ls(char* path);
int cd(char* pathname);
int do_pwd(char* pathname);
int pwd(MINODE *wd);
int noPrintPwd(MINODE *wd, char buf[256]);
int ialloc(int dev);
int idalloc(int dev, int ino);
int balloc(int dev);
int bdalloc(int dev, int ino);
int quit(char *pathname);
int tst_bit(char* buf, int i);
int set_bit(char* buf, int i);
int clr_bit(char* buf, int i);
int decFreeInodes(int dev);
int incFreeInodes(int dev);
int decFreeBlocks(int dev);
int incFreeBlocks(int dev);
int stat1(char* pathname);
int findBlocks(INODE *ip, int printStat);
int printDirs(int block, int printStat);
int printStat1(DIR *dp);
int myStat(DIR *dp, struct stat *s);
int test_mode(struct stat *input, enum stat_mode mode);
int test_perm(struct stat *input, enum perm_mode mode);
int myDirname(char *pathname, char buf[256]);
int myBasename(char *pathname, char buf[256]);
int make_dir(char *pathname);
int my_mkdir(MINODE *pip, char child[256]);
int findLastBlock(MINODE *pip);
int addLastBlock(MINODE *pip, int bnumber);
int touch(char* name);
int my_chmod(char* pathname);
int chown(char* pathname);
int chgrp(char* pathname);
int creat_file(char* pathname);
int my_creat(MINODE *pip, char child[256]);
int my_rmdir(char* pathname);
int rm_child(MINODE *pip, char *child);
int is_empty(MINODE *mip);
int split_paths(char* original, char* path1, char* path2);
int link(char* pathname);
int unlink(char* pathname);
int rm(MINODE *mip);
int symlink(char *pathname);
int readlink(char *pathname, char *linkStr);
int open_file(char *pathname);
int truncate(MINODE *mip);
int close_file(char *pathname);
int mlseek(char *pathname);
long my_lseek(int fd, long position);
int pfd(char *pathname);
int read_file(char *pathname);
int myread(int fd, char *buf, int nbytes);
int cat(char *pathname);
int write_file(char *pathname);
int mywrite(int fd, char *buf, int nbytes);
int cp_file(char *pathname);
int mv_file(char *pathname);
int pmt(char *pathname);
int my_mount(char *pathname);
int my_umount(char *pathname);

static int (*fptr[])(char*) = {(int (*)())ls, stat1, cd, do_pwd, stat1, make_dir, my_chmod, chown, chgrp, creat_file, my_rmdir, link, unlink, symlink, touch, open_file, close_file, mlseek, pfd, read_file, cat, write_file, cp_file, mv_file, menu, my_mount, my_umount, quit};
static char *cmnds[] = {"ls", "ll", "cd", "pwd", "stat", "mkdir", "chmod", "chown", "chgrp", "creat", "rmdir", "link", "unlink", "symlink", "touch", "open", "close", "lseek", "pfd", "read", "cat", "write", "cp", "mv", "menu", "mount", "umount", "quit"};


#endif
