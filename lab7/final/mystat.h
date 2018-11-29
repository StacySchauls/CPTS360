#ifndef MYSTAT_H
#define MYSTAT_H

#include "type.h"
#include <time.h>
#include <stdlib.h>

#define MAX 256

struct dirent{
    unsigned int d_ino;
    unsigned int d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};


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

int test_mode(struct stat *input, enum stat_mode mode);
int test_perm(struct stat *input, enum perm_mode mode);
char* printFileName(char* filename, struct stat *file_info);
int printStat1(DIR *fd);
char* printInfo(struct stat *file_info);


#endif
