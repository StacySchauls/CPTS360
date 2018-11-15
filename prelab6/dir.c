/*
   struct ext2_dir_entry_2 {
   u32  inode;        // Inode number; count from 1, NOT from 0
   u16  rec_len;      // This entry length in bytes
   u8   name_len;     // Name length in bytes
   u8   file_type;    // for future use
   char name[EXT2_NAME_LEN];  // File name: 1-255 chars, no NULL byte
   }; */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include<ext2fs/ext2_fs.h>

#define BLKSIZE 1024

typedef struct ext2_group_desc	GD;
typedef struct ext2_super_block	SUPER;
typedef struct ext2_inode	INODE;
typedef struct ext2_dir_entry_2	DIR;


GD	*gp;
SUPER	*sp;
INODE	*ip;
DIR	*dp;
int fd;
int iblock;

int get_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}


dir()
{
	char buf[1024], *cp, temp[256];
	char dbuf[BLKSIZE];
	int i;

	get_block(fd, 2, buf);
	gp = (GD *)buf;
	cp = buf;
	dp = (DIR *)buf;

	iblock = gp->bg_inode_table;   // get inode start block#

	get_block(fd, iblock, buf);
	ip = (INODE *)buf + 1;
	printf("inode_block=%d\n", iblock);
	printf("ip->iblock[0] = %d\n",ip->i_block[0]);
	for(i=0; i<12; i++){
		if(ip->i_block[i] ==0)
			break;
		printf("i_block[%d] = %d\n",i,ip->i_block[i]);
		get_block(fd,ip->i_block[i],dbuf);
		
		printf("	ino rec-len name_len name\n");
		
		dp = (DIR *)dbuf;
		cp = dbuf;

		while(cp < dbuf + BLKSIZE){
		//	printf("%s, len: %d\n", dp->name,dp->name_len);
			memset(temp, 0, sizeof(dp->name));
		//	strncpy(temp, dp->name, dp->name_len);
		
		
		temp[dp->name_len] = 0;
			
			printf("%4d %6d %6d		%s\n",dp->inode, dp->rec_len, dp->name_len, dp->name);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}

	}
}

int search(INODE *ip, char *name){

	char buf[1024], *cp, temp[256];
	char dbuf[BLKSIZE];
	int i;

	get_block(fd, 2, buf);
	gp = (GD *)buf;
	cp = buf;
	dp = (DIR *)buf;

	iblock = gp->bg_inode_table;   // get inode start block#

	get_block(fd, iblock, buf);
	ip = (INODE *)buf + 1;
//	printf("inode_block=%d\n", iblock);
//	printf("ip->iblock[0] = %d\n",ip->i_block[0]);
	for(i=0; i<12; i++){
		if(ip->i_block[i] ==0)
			break;
//		printf("i_block[%d] = %d\n",i,ip->i_block[i]);
		get_block(fd,ip->i_block[i],dbuf);
		
//		printf("	ino rec-len name_len name\n");
		
		dp = (DIR *)dbuf;
		cp = dbuf;

		while(cp < dbuf + BLKSIZE){
		//	printf("%s, len: %d\n", dp->name,dp->name_len);
			memset(temp, 0, sizeof(dp->name));
		//	strncpy(temp, dp->name, dp->name_len);
		
		
		temp[dp->name_len] = 0;
			if(!strcmp(dp->name, name) == 0){
				printf("found %s\n",name);
				return dp->inode;
			}
		//	printf("%4d %6d %6d		%s\n",dp->inode, dp->rec_len, dp->name_len, dp->name);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		printf("Could not find %s\n",name);
		return 0;

	}
}

char *disk = "mydisk";				// Default disk

main (int argc, char *argv[])
{
	if (argc > 1) disk = argv[1];

	fd = open(disk, O_RDONLY);

	if (fd < 0)
	{
		printf("Open %s failure\n", disk);
		exit(1);
	}

	dir();
	
}
