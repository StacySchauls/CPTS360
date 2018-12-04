#include "util.h"

int init(){
	proc[0].uid = 0;
	proc[0].cwd = 0;
	proc[1].uid = 1;
	proc[1].cwd = 0;

	//set running to the 0th proc
	running = &(proc[0]);
	readQueue = &(proc[1]); //next in queue
	int i;
	//set ref count for minodes to 0
	for(i = 0; i<100; i++){
		minode[i].refCount = 0;
		minode[i].ino = 0;
	}
	root = 0;
}

int mount_root(char *devName){

	char buf[BLKSIZE];
	dev = open(devName, O_RDWR); //openthe device for read and write
	if(dev < 0){
		printf("Error Opening %s.\n",devName);
		exit(1);
	}
	get_super(dev, buf); //get block 1
	sp = (SUPER *)buf;
	if(is_ext2(buf) <= 0){
		printf("Not an EXT2 file System.\n");
		exit(1);
	}

	get_inode_table(dev); //sets the global variable inode begin to tstart of the inode table
	ninodes = sp->s_inodes_count; //get the number of inodes
	root = iget(dev, ROOT_INODE); //set the root inode
	proc[0].cwd = proc[1].cwd = iget(dev, ROOT_INODE);//set the cwdd to the root
	MountTable[0].mounted_inode = root; //update the mount table
	MountTable[0].ninodes = ninodes;
	MountTable[0].nblocks = sp->s_blocks_count;
	MountTable[0].dev = dev;
	strncpy(MountTable[0].name, devName, 256);

	printf("Device Mounted.\n");
	return dev;
}

void menu(){
	printf("***** Level 1 *****\n");
	printf("ls\tpwd\tcd\tmkdir\trmdir\ncreat\tlink\tsymlink\tunlink\tchmod\n");

}

//returns the block in the buffer
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


//reads the super block
int get_super(int dev1, char *buf){
	get_block(dev1, SUPERBLOCK, buf);
}

//checks if a dev is ext2
int is_ext2(char *buf){
	sp = (SUPER *)buf;
	if(SUPER_MAGIC != sp->s_magic){
		printf("Error. Not an EXT2 File System\n");
	}
}

//gets the inode table and poplates global values
void get_inode_table(int dev1){
	char buf[BLKSIZE];
	get_gd(dev1, buf);
	gp = (GD*)buf;
	inodeBegin = gp->bg_inode_table;
	bmap = gp->bg_block_bitmap;
	imap = gp->bg_inode_bitmap;
}
//reads the group descriptor
int get_gd(int dev1, char *buf){
	get_block(dev1, GDBLOCK, buf);
}



/**** LS *****/
int ls(char *path){
	int ino;
	MINODE *mip;
	if(!path || !pathname[0])
	{
		ino = running->cwd->ino;   //if the pathname is null then set ino to root ino
	}else if(pathname[0] == '/' && !pathname[1]) { ino = root->ino;}	//get the ino from the path
	else { ino = getino(dev,path);}

	if(0>= ino){
		printf("Invalid path\n");
		return -1;
	}
	mip = iget(dev, ino); //use iget to get an minode based on the ino

	findBlocks(&(mip->INODE), 0); //use find blocks. this will help us print the valuesin the directory we are ls-ing
	iput(mip->dev, mip); //put it back!
}


/************ CD ************/

int cd(char *pathname){
	unsigned int ino;
	MINODE *mip;
	//check if we use absolue or realative path
	//
	if(!pathname || !pathname[0] || (pathname[0] == '/' && !pathname[1] ))
	{
		ino = root->ino;
	}else //else get the ino from the pathname
	{
		ino = getino(dev, pathname);
	}

	if(!ino) //if ino is invalid
	{
		printf("Invalid pathname\n");
		return -1;
	}
	mip = iget(dev, ino); //get the minode
	//verify we are tring to cd into a dir
	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("Error: path is not a directory\n");
		iput(dev, mip); //put the minode back and return
		return -1;
	}

	//if we here then it should be adir
	//put the current minode away
	iput(dev, running->cwd);

	//set new cwd to the new inode
	running->cwd = mip;
	return 0;
}

/******* PWD *********/

int do_pwd(char *pathname){
	printf("cwd = ");
	pwd(running->cwd);
	putchar(10);
}

int pwd(MINODE *wd){
	int ino = 0;
	MINODE *next = NULL;
	char temp[256];
	if(DEBUG){printf("WD: %d ROOT %d\n", wd->ino, root->ino);}
	//if we are at the root print only the root
	if(wd->ino == root->ino)
	{
		if(DEBUG){printf("in here\n");}
		printf("/");
		return 1;
	}


	//get the parents minode
	ino = search(dev, "..", &(wd->INODE));
	if(ino <=0)
	{
		printf("Error accesssing inode\n");
		return -1;
	}

	next = iget(dev, ino);
	if(DEBUG){printf("next: %d ROOT %d\n", next->ino, root->ino);}
	if(!next)
	{
		printf("Error. Could not find inode\n");
		return -1;
	}
	pwd(next); //recursively go through each parent and print out their names
	memset(temp, 0, 256);
	searchByIno(next->dev, wd->ino, &(next->INODE), temp);
	printf("%s/", temp);
	iput(next->dev, next);
	return 0;
}

/***MAKE DIR*******/
int make_dir(char *pathname)
{
	int dev1, ino, r;
	char parent[256], child[256], origPathname[512];
	MINODE *mip;
	memset(parent, 0, 256);
	memset(child, 0, 256);
	memset(origPathname, 0, 512);
	strcpy(origPathname, pathname);
	if(pathname[0] == '/') { dev1 = root->dev; }
	else { dev1 = running->cwd->dev; }

	//get basename and dirname

	strcpy(parent, dirname(pathname));
	strcpy(child, basename(origPathname));

	if(DEBUG) { printf("parent: %s  child: %s \n", parent, child);}
	//get the ino for the parent
	ino = getino(dev1, parent);
	if(0>=ino)
	{
		printf("Error, invalid pathname\n");
		return -1;
	}
	mip = iget(dev1, ino);
	//check ifthe parent is a directory?
	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("Error, not a directory\n");
		iput(dev1, mip);
		return -1;
	}

	//look to see if the item already exists in the current dir
	ino = search(dev1, child, &(mip->INODE));
	printf("ino is %d\n", ino);
	if(ino >0)
	{
		printf("Error. Directory already exists\n");
		iput(mip->dev, mip);
		return -1;
	}

	//acutually make the dir now. Above was simple checks and stuff
	r = my_mk(mip, child);
	iput(mip->dev, mip);
	return r;
}


int my_mk(MINODE *pip, char child[256])
{
	int inum, bnum, idealLen, neededLen, newRec, i, j;
	MINODE *mip;
	char *cp;
	DIR *prevDP;
	char buf[BLKSIZE];
	char buf2[BLKSIZE];
	int blk[256];

	//get inode and block num
	inum = ialloc(pip->dev);
	bnum = balloc(pip->dev);

	//get an minode for the inode
	mip = iget(pip->dev, inum);

	//update the contesnts of the minode
	mip->INODE.i_mode = 040755;  //its a dir
	mip->INODE.i_uid = running->uid;
	mip->INODE.i_gid = running->gid;
	mip->INODE.i_size = BLKSIZE;
	mip->INODE.i_links_count = 2;
	mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L); //update time
	mip->dirty = 1;

	for(i = 0; i < 15; i++)
	{
		mip->INODE.i_block[i] = 0;
	}
	if(DEBUG){printf("mip->INODE.i_block[0] = %d\n",bnum);}
	mip->INODE.i_block[0] = bnum;
	iput(mip->dev, mip);

	//make . and .. for data block 0
	dp = (DIR *)buf;
	dp->inode = inum;
	strncpy(dp->name, ".", 1);
	dp->name_len = 1;
	dp->rec_len = 12;

	cp = buf + 12;
	dp = (DIR *)cp;
	dp->inode = pip->ino;
	dp->name_len = 2;
	strncpy(dp->name, "..", 2);
	dp->rec_len = BLKSIZE - 12;

	//write to disk
	put_block(pip->dev, bnum, buf);

	//update the parent inode
	memset(buf, 0, BLKSIZE);
	neededLen = 4*((8+strlen(child)+3)/4);
	//check if there is room in the last block in the parents directory
	bnum = findLast(pip);
	get_block(pip->dev, bnum, buf); //get the last block into memory
	cp = buf;
	dp = (DIR *)cp;
	//go to the last entry
	while((dp->rec_len + cp) < buf + BLKSIZE)
	{
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

	idealLen = 4*((8+dp->name_len+3)/4); //gets the ideal length
	if(dp->rec_len - idealLen >= neededLen) //then we have room to put a new record in
	{
		newRec = dp->rec_len - idealLen;
		dp->rec_len = idealLen;
		cp += dp->rec_len;
		dp = (DIR *)cp;
		if(DEBUG){printf("dp->inode = %d\n",inum);}
		dp->inode = inum;
		dp->name_len = strlen(child);
		strncpy(dp->name, child, dp->name_len);
		dp->rec_len = newRec;
	}else //else there isnt room. so we need to allocate new data block
	{
		//use balloc to allocate new block
		bnum = balloc(pip->dev);
		dp = (DIR *)buf;
		dp->inode = inum;
		dp->name_len = strlen(child);
		strncpy(dp->name, child, dp->name_len);
		dp->rec_len = BLKSIZE;
		//this adds block to the end
		addLastBlock(pip, bnum);
	}

	//write block back to disk
	put_block(pip->dev, bnum, buf);
	pip->dirty = 1;
	pip->INODE.i_links_count++;
	memset(buf, 0 , BLKSIZE);
	//search for parents ino
	searchByIno(pip->dev, pip->ino, &running->cwd->INODE, buf);
	//touch it!
	touch(buf);

}

/**** RM DIR ****/

int rm_dir(char *pathname)
{
	int ino, i;
	char parent[256], child[256], orig[512];
	MINODE *pip = NULL;
	MINODE  *mip = NULL;
	strcpy(orig, pathname);

	//check that w were actually given a path
	if(!pathname || !pathname[0])
	{
		printf("Error. No pathname specified.\n");
		return -1;
	}
	else
	{
		//if we made it here then there is a pathname. get the ino to make sure its valid
		ino = getino(dev, pathname);
		if(DEBUG){printf("found ino: %d\n",ino);}
	}
	//check that it is valid

	if(0 >= ino)
	{
		printf("Error. Not a valid pathname.\n");
		return -1;
	}

	//if we are here then its valid path. make sure its a directory

	mip = iget(dev, ino);

	if(DEBUG)
	{
		printf("%x\n", mip->INODE.i_mode);

	}




	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("Error. Not a directory.\n");
		iput(mip->dev, mip);
		return -1;
	}

	//if we are here then it is a directory. Check if it is an empty directory

	if(mip->INODE.i_links_count > 2)
	{
		printf("Error. Directory not empty.\n");
		iput(mip->dev, mip);
		return -1;
	}

	if(is_empty(mip) != 0)
	{
		printf("Error. Directory not empty.\n");
		iput(mip->dev, mip);
		return -1;
	}

	//if we are here then it is emptpy. start to remove it.
	//use block and inode DEallocation
	//deallocate the blocks
	for(i = 0; i < 12; i++)
	{
		//if the block is in use (not null) delallocate
		if(mip->INODE.i_block[i] != 0)
		{
			if(DEBUG) {printf("In rmdir, entering bdalloc\n");}
			bdalloc(mip->dev, mip->INODE.i_block[i]);
		}
	}
	//dealloc the inode now
	idalloc(mip->dev, mip->ino);

	//get dir and basename for parent and child
	strncpy(child, basename(orig), 256);
	strncpy(parent, dirname(pathname), 256);
	if(DEBUG){printf("parent: %s Child: %s\n", parent, child);}
	//get parent ino and load it into memory
	ino = getino(mip->dev, parent);
	pip = iget(mip->dev, ino);
	//write it to the disk
	iput(mip->dev, mip);
	//work on removing the child
	rm_child(pip, child);
	pip->INODE.i_links_count--;
	//update the time of the parent
	touch(parent);
	pip->dirty = 1;
	//write the parent back
	iput(pip->dev, pip);
	return 1;
}

int is_empty(MINODE *mip)
{
	int i;
	char *cp, buf[BLKSIZE], temp[256];
	DIR *dp;

	for(i = 0; i < 12; i++)
	{
		if(ip->i_block[i] == 0) {return 0;}

		get_block(dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)buf;

		while(cp < buf + BLKSIZE)
		{
			memset(temp, 0, 256);
			strncpy(temp, dp->name, dp->name_len);
			printf("%s = temp\n",temp);
			if(strncmp(".", temp,1) != 0 && strncmp("..", temp,2) != 0) {return 1;} //NOT EMPTY
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
}


int rm_child(MINODE *pip, char *child)
{
	int i, size, found = 0;
	char *cp, *cp2;
	DIR *dp, *dp2, *dpPrev;
	char buf[BLKSIZE], buf2[BLKSIZE], temp[256];

	memset(buf2,0,BLKSIZE);
	for(i = 0; i < 12; i++)
	{
		if(pip->INODE.i_block[i] == 0) { return 0; } //blocks are empty!!
		//else theyre not empty. so go thorhg em, and remove thigns
		get_block(pip->dev, pip->INODE.i_block[i], buf);
		dp = (DIR *)buf;
		dp2 = (DIR *)buf;
		dpPrev = (DIR *)buf;
		cp = buf;
		cp2 = buf;


		while(cp < buf + BLKSIZE && !found)
		{
			memset(temp,0,256);
			strncpy(temp,dp->name, dp->name_len);
			if(strcmp(child, temp) == 0) //we found the child we are looking for. kill it!
			{
				//check if child is the only entry in the data block
				if(cp == buf && dp->rec_len == BLKSIZE)
				{
					bdalloc(pip->dev, pip->INODE.i_block[i]);
					pip->INODE.i_block[i] = 0;
					pip->INODE.i_blocks--;
					found = 1;
				}
				//else delete child and move entries over to the left
				else
				{
					while((dp2->rec_len + cp2) < buf + BLKSIZE)
					{
						dpPrev = dp2;
						cp2 += dp2->rec_len;
						dp2 = (DIR *)cp2;
					}
					if(dp2 == dp) //this means the child is the last entry
					{
						dpPrev->rec_len += dp->rec_len;
						found = 1;
					}
					else //child is not the last entry
					{
						size = ((buf + BLKSIZE) - (cp + dp->rec_len)); //size to the end of the block
						dp2->rec_len += dp->rec_len;
						memmove(cp, (cp + dp->rec_len), size); //moves data at cp+recLen to where cp is. shifting to the left, size # of bytes
						dpPrev = (DIR *)cp;
						memset(temp, 0, 256);
						strncpy(temp, dpPrev->name, dpPrev->name_len);
						found = 1;
					}
				}
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		if(found)
		{
			//if we successfully found the child to remove, write out the block
			put_block(pip->dev, pip->INODE.i_block[i], buf);
			return 1;
		}
	}
	printf("Error: child not found.\n");
	return -1;
}










/******* CREAT ******/
int creat_file(char *fileName)
{
	int ino,dev1,r;
	char parent[256],child[256], temp[256];
	MINODE *mip;
	memset(parent,0,256);
	memset(child,0,256);

	//check if this is relative or absolute path
	if(fileName[0] == '/' ) { dev1 = root->dev;} //absolute
	else { dev1 = running->cwd->dev; } //relative

	strncpy(temp,fileName, 256);

	//get dirname and basename
	strcpy(parent, dirname(temp));
	strcpy(child, basename(fileName));

	//get the ino of the parent
	ino = getino(dev1, parent);
	if(ino <= 0)
	{
		printf("Error. Parent directory does not exist\n");
		return -1;
	}

	//load the parents inode inot memort
	mip = iget(dev1, ino);
	//check that the inode is a directory
	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("Error. Path is not a directory.\n");
		return -1;
	}
	//search for the child in the parent directory
	ino = search(dev1, child, &(mip->INODE));
	if(0 < ino) //if this is true then the file already exists
	{
		printf("Error. File alread exists.\n");
		return -1;
	}
	//create the file now.
	r = my_creat(mip, child);
	//write the creation to the disk
	iput(mip->dev, mip);
	return 1;
}

int my_creat(MINODE *pip, char child[])
{
	int inum, bnum, idealLen, needLen, newRec, i, j, blk[256];
	MINODE *mip;
	char *cp, buf[BLKSIZE],buf2[BLKSIZE];
	DIR *dpPrev;

	//first allocate an inode for the file onthe parent device
	inum = ialloc(pip->dev);
	//load that inode
	mip = iget(pip->dev, inum);

	//update and write the contents of the file into the inode
	mip->INODE.i_mode = 0x81A4; //file mode
	mip->INODE.i_uid = running->uid;
	mip->INODE.i_gid = running->gid;
	mip->INODE.i_size = 0;
	mip->INODE.i_links_count = 1;
	mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
	mip->INODE.i_blocks = 0;
	mip->dirty = 1;
	for(i = 0; i<15; i++)
	{
		mip->INODE.i_block[i] = 0;
	}
	//write the changes to the disk
	iput(mip->dev, mip);

	//write to parents directory
	memset(buf,0,256);
	needLen = 4*((8+strlen(child)/3)+4);
	//get the block number by looking up the last used block
	bnum = findLast(pip);
	//check for room in thelast block now
	get_block(pip->dev, bnum, buf); //load the block
	cp = buf;
	dp = (DIR *)cp;

	while((dp->rec_len + cp) < buf + BLKSIZE)
	{
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

	idealLen = 4*((8+dp->name_len+3)/4);
	if(dp->rec_len - idealLen >= needLen) //there is room in the block
	{
		newRec = dp->rec_len - idealLen;//new rec length
		dp->rec_len = idealLen;
		cp += dp->rec_len;
		dp = (DIR *)cp;
		dp->inode = inum;
		dp->name_len = strlen(child);
		strncpy(dp->name, child, dp->name_len);
		dp->rec_len = newRec;
	}//if the above was false, then we need to allocate a new block for the data
	else
	{
		bnum = balloc(pip->dev);
		dp = (DIR *)buf;
		dp->inode = inum;
		dp->name_len = strlen(child);
		strncpy(dp->name, child, dp->name_len);
		dp->rec_len = BLKSIZE;
		//add the block to the last
		addLastBlock(pip, bnum);
	}
	//write the block back to the disk
	put_block(pip->dev, bnum, buf);
	pip->dirty = 1;
	memset(buf, 0, BLKSIZE);
	//get the parent ino and update the times
	searchByIno(pip->dev, pip->ino, &running->cwd->INODE, buf);
	touch(buf);
	return 1;

}







//updates access times, creates file if needed
int touch(char *name)
{
	char buf[BLKSIZE];
	int ino;
	MINODE *mip;
	//get ino for file via name
	ino = getino(dev, name);
	if(0 >= ino) //the file doent exist so create it
	{
		//CALL CREATE
		if(DEBUG){printf("FILE oesnt exist, ccreate it\n");}
		creat_file(name);
		return 1;
	}
	mip = iget(dev, ino); //load the inode into memory
	mip->INODE.i_atime = mip->INODE.i_mtime = mip->INODE.i_ctime = time(0L); //update times
	mip->dirty = 1;
	//write back to disk
	iput(mip->dev, mip);
	return 1;
}



//add a block to the end
int addLastBlock(MINODE *pip, int bnumber)
{
	int buf[256], buf2[256],i,j,newBlk, newBlk2;
	//look for the last black in the parents directory
	//search through direct blocks first
	for(i=0; i<12; i++)
	{
		if(pip->INODE.i_block[i] == 0) {pip->INODE.i_block[i] = bnumber; return 1;}
	}
	if(pip->INODE.i_block[12] == 0) //we have to make a direct block
	{
		newBlk = balloc(pip->dev); //block alloc
		pip->INODE.i_block[12] = newBlk;
		memset(buf,0,256);
		get_block(pip->dev, newBlk, (char *)buf); //load the new block into the buf
		buf[0] = bnumber;
		put_block(pip->dev, newBlk, (char *)buf); //write the block back to memory
		return 1;
	}
	memset(buf,0,256);
	get_block(pip->dev, pip->INODE.i_block[12], (char *)buf); //get the start of the double indirect. Check if we nee to make a new one
	for(i = 0; i<256; i++)
	{
		if(buf[i] == 0) {buf[i] = bnumber; return 1;}
	}
	if(pip->INODE.i_block[13] == 0) //need to make a doble indirect block
	{
		newBlk = balloc(pip->dev); //alloc a new block
		pip->INODE.i_block[13] = newBlk;
		memset(buf,0,256);
		get_block(pip->dev, newBlk, (char *)buf);
		newBlk2 = balloc(pip->dev);
		buf[0] = newBlk2;
		//write the block back
		put_block(pip->dev, newBlk2,(char *)buf);
		memset(buf2, 0, 256);
		get_block(pip->dev, newBlk2, (char*)buf2);
		buf2[0] = bnumber;
		put_block(pip->dev, newBlk2, (char*)buf2);
		return 1;
	}
	memset(buf,0,256);
	get_block(pip->dev, pip->INODE.i_block[13], (char *)buf);
	for(i = 0; i<256; i++)
	{
		if(buf[i] == 0)
		{
			newBlk2 = balloc(pip->dev);
			buf[i] = newBlk2;
			//write the block back
			put_block(pip->dev, pip->INODE.i_block[13],(char *)buf);
			memset(buf2, 0 , 256);
			get_block(pip->dev, newBlk2, (char *)buf2);
			buf2[0] = bnumber;
			put_block(pip->dev, newBlk2, (char *)buf2);
			return 1;
		}
		memset(buf2, 0, 256);
		get_block(pip->dev, buf[i], (char *)buf2);
		for(j = 0; j< 256; j++)
		{
			if(buf2[j]== 0){buf2[j] = bnumber; return 1;}
		}
	}

	printf("Error. Could not add block to inode.\n");
	return -1;

}

int findLast(MINODE *pip)
{
	int buf[256], buf2[256], bnum, i, j;

	//find last used block in the given pip
	if(pip->INODE.i_block[0] == 0) {return 0;}

	for(i = 0; i<12; i++)
	{
		if(pip->INODE.i_block[i] == 0) {printf("returning %d\n",pip->INODE.i_block[i-1]); return pip->INODE.i_block[i-1];}
	}

	if(pip->INODE.i_block[12] == 0) { printf("returning %d\n",pip->INODE.i_block[i-1]);return pip->INODE.i_block[i-1];}
	get_block(dev, pip->INODE.i_block[12], (char*)buf);
	for(i = 0; i < 256; i++)
	{
		if(buf[i] == 0) {return buf[i-1];}
	}
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

//find space for a new inode
int ialloc(int dev1)
{
    int i;
    char buf[BLKSIZE];            // BLKSIZE=block size in bytes

    // get inode Bitmap into buf[ ]

    get_block(dev1, imap, buf);       // assume FD, bmap block# = 4

    for (i=0; i < ninodes; i++){  // assume you know ninodes
        if (test_bit(buf, i)==0){    // assume you have tst_bit() function
        set_bit(buf, i);          // assume you have set_bit() function
        put_block(dev1, imap, buf);   // write imap block back to disk

        // update free inode count in SUPER and GD on dev
        decFreeInodes(dev1);
				printf("returning %d\n",i+1);       // assume you write this function
        return (i+1);
        }
    }
 return 0;                     // no more FREE inodes
}


int idalloc(int dev1, int ino)
{
	int i;
	char buf[BLKSIZE];
	get_block(dev1, imap, buf);
	clr_bit(buf, ino-1);
	put_block(dev1, imap, buf);
	incFreeInodes(dev1);
}



//basically the same as ialloc except were allocating spce for a block instead of an inode
int balloc(int dev1)
{
    int i;
    char buf[BLKSIZE];            // BLKSIZE=block size in bytes

    get_block(dev1, bmap, buf);

    for (i=0; i < BLKSIZE; i++){  // assume you know ninodes
        if (test_bit(buf, i)==0){    // assume you have tst_bit() function
        set_bit(buf, i);          // assume you have set_bit() function
        put_block(dev1, bmap, buf);   // write bmap block back to disk

        // update free inode count in SUPER and GD on dev
        decFreeBlocks(dev1);       // assume you write this function
        memset(buf, 0, BLKSIZE);
        put_block(dev1, i+1, buf);
        return (i+1);
        }
    }
 return 0;                     // no more FREE inodes
}



int bdalloc(int dev1, int ino)
{
	int i;
	char buf[BLKSIZE];
	//load the block to dealloc
	get_block(dev1, BBITMAP, buf);
	clr_bit(buf, ino-1); //set the bit to 0 to indicate its no longer in use
	//write the block back to the disk
	put_block(dev1, BBITMAP, buf);
	incFreeBlocks(dev);
}






int test_bit(char *buf, int i)
{
    int byt, offset;
    byt = i/8;
    offset = i%8;
    return (((*(buf+byt))>>offset)&1);
}

int set_bit(char *buf, int i)
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


int clr_bit(char *buf, int i)
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

int decFreeInodes(int dev1)
{
	char buf[BLKSIZE];
	get_super(dev, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count -=1;
	put_block(dev, SUPERBLOCK, buf);
	get_gd(dev, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count -=1;
	put_block(dev, GDBLOCK, buf);
	return 1;
}

int incFreeInodes(int dev1)
{
	char buf[BLKSIZE];
	get_super(dev, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count +=1;
	put_block(dev, SUPERBLOCK, buf);
	get_gd(dev, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count +=1;
	put_block(dev, GDBLOCK, buf);
	return 1;
}


int incFreeBlocks(int dev1)
{
	char buf[BLKSIZE];
	get_super(dev1, buf);
	sp->s_free_blocks_count++;
	//write back to disk
	put_block(dev1, SUPERBLOCK, buf);
	//get hte group descriptor block
	get_gd(dev1, buf);
	gp->bg_free_blocks_count++;
	//write back
	put_block(dev1, GDBLOCK, buf);
	return 1;
}


//finds all the data blocks from a pointer to that inode and prints the dir names in the data blocks
int findBlocks(INODE *ip, int printStat){
	int i, j, k;
	unsigned int buf[256], buf2[256];

	//print the direct blocks

	for(i = 0; i < 12; i++){

		if(ip->i_block[i] != 0){
			printDirs(ip->i_block[i], printStat);
		}

	}

	//print the indirect blocks
	//check that the 12th iblock actually exists so we can print the indirect blocks
	if(ip->i_block[12])
	{
		get_block(dev, ip->i_block[12], (char*)buf); //get the block
		for(i = 0; i < 256; i++)
		{
			if(buf[i]) {printDirs(buf[i], printStat);}
			{
			}

			//print dirs in double indirect blocks
			//check that the 13th actually exists
			if(ip->i_block[13])
			{
				//get the block
				get_block(dev, ip->i_block[13], (char*)buf);
				for(i = 0; i < 256; i++)
				{
					if(buf[i])
					{
						get_block(dev, buf[i], (char*)buf2); //double indirect so get the second block
						for(j = 0; j< 256; j++){
							if(buf2[j]) {printDirs(buf2[j], printStat);}
						}
					}
				}
			}
		}
	}
}

int printDirs(int block, int printStat){

	int i;
	char *cp;
	DIR *dp;
	char buf[BLKSIZE], temp[256];
	get_block(dev, block, buf); //get the block that we are going print
	dp = (DIR *)buf;
	cp = buf;

	//go through each record and print its information
	while(cp < buf+BLKSIZE){
		memset(temp, 0, 256);
		strncpy(temp, dp->name, dp->name_len);
		printf("%4d %4d %4d %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

		cp+=dp->rec_len;
		dp = (DIR *)cp;
	}
	return 0;

}


int getino(int dev1, char *path){
	int ino = 0;
	int i = 0;
	char **tokens;
	MINODE *mip = NULL;
	if(path && path[0]){ //check if the path isnt null
		tokens = tokenPath(path); //get the tokens of the path
	}else{  //no pathname, so set ino to the cwd
		ino = running->cwd->ino;
		return ino;
	}

	if(path[0] =='/'){ //absolute path, start at the root dir
		ip = &(root->INODE);
		ino = root->ino;
	}else{  //else its relative, start at the current dir
		ip = &(running->cwd->INODE);
	}

	while(tokens[i]){
		ino = search(dev, tokens[i], ip); //search for the inode associated with the current token
		if(0 >= ino){
			if(mip){ iput(mip->dev, mip);} //write the mip to the disk and return if ino is null
			return -1;
		}
		if(mip) { iput(mip->dev, mip); }
		i++;
		if(tokens[i]){ //if the token is not null get the ip associated with it
			mip = iget(dev, ino);
			ip = &(mip->INODE);
		}
	}
	i = 0;
	while(tokens[i]){

		free(tokens[i]);
		i++;
	}
	if(mip) { iput(mip->dev, mip);}
	return ino;

}

//gets the minode associated with the ino
MINODE *iget(int dev1, unsigned int ino){
	int i = 0, blk, offset;
	char buf[BLKSIZE];
	MINODE *mip = NULL;
	//search minode[100] to see if inode already exists in array
	for(i = 0; i < 100; i++)
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

	//if we are here then the inode does not exists. we need to put inode from disk into the minode array

	i = 0;
	while(minode[i].refCount > 0 && i < 100) { i++;}
	if(i == 100)
	{
			printf("Error: NO SPACE IN MINODE ARRAY\n");
			return 0;
	}
	blk = (ino-1)/8 + inodeBegin;
	offset = (ino-1)%8;
	get_block(dev1, blk, buf);
	ip = (INODE *)buf + offset;
	memcpy(&(minode[i].INODE), ip, sizeof(INODE)); //Copy inode from disk into minode array
	minode[i].dev = dev1;
	minode[i].ino = ino;
	minode[i].refCount = 1;
	minode[i].dirty = 0;
	minode[i].mounted = 0;
	minode[i].mountptr = NULL;
	return &minode[i];
	}

int iput(int dev1, MINODE *mip){
	char buf[BLKSIZE];
	int blk, offset;
	INODE *tip;
	mip->refCount--;  //putting away, so decrement the reference count

	if(mip->refCount > 0){return 0;} //if the ref count is > 0, someone else isstill using it
	if(mip->dirty == 0){return 1;} //if its not dirty then it wasnt modified, so return

	//must write the inode back to the disk.
	//use mailmans alg to get blck and offset for th enode
	blk = (mip->ino -1)/8 + inodeBegin;
	offset = (mip->ino)%8;
	get_block(dev1, blk, buf);
	tip = (INODE *)buf + offset; //get temp ip based off bufand offset
	memcpy(tip, &(mip->INODE), sizeof(INODE)); //use memcpy to write to the block fast and efficiently
	put_block(mip->dev, blk, buf);
	return 1;
}

//writes a block to the disk
int put_block(int dev, int blk, char *buf)
{
    if (-1 == lseek(dev, (long)(blk*BLKSIZE), 0)){ assert(0);}
        write(dev, buf, BLKSIZE);
        return 1;
}

int search(int dev1, char *str, INODE *ip){
	int i;
	char *cp;
	DIR *dp;
	char buf[BLKSIZE], temp[256];
	//look through direct blocks for the str
	for(i = 0; i < 12; i++)
	{
		if(ip->i_block[i] == 0){break;} //if we reach null, break
		get_block(dev, ip->i_block[i], buf); //get the block associated
		dp = (DIR *)buf;
		cp = buf;

		//go through each record in the block. search for the string given
		while(cp < buf + BLKSIZE){
			memset(temp, 0, 256);
			strncpy(temp, dp->name, dp->name_len);
			if(strcmp(temp, str) == 0){
				return dp->inode; //found the name, return the inode
			}
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	return 0;
}


//just like search except we are looking for matchin inodes now
int searchByIno(int dev, int ino, INODE *ip, char *temp)
{

	int i;
	char *cp;
	DIR *dp;
	char buf[BLKSIZE];

	for(i=0; i<12; i++)
	{

		if(ip->i_block[i] == 0) {break;}

		get_block(dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)buf;
		while(cp < buf + BLKSIZE)
		{
			if(ino == dp->inode)
			{
				strncpy(temp, dp->name, dp->name_len);
				return 1;
			}
			cp+=dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	return 0;

}







char ** tokenPath(char *path){
	int i;
	char **name;
	char *temp;
	name = (char **)malloc(sizeof(char *)*256);
	name[0] = strtok(path, "/");
	i = 1;
	while((name[i] = strtok(NULL,"/")) != NULL) { i++; }//get the other tokens in the pathname

	name[i] = 0;
	i = 0;
	while(name[i]){
		temp = (char *)malloc(sizeof(char)*strlen(name[i]));
		strcpy(temp, name[i]);
		name[i] = temp;
		i++;
	}
	return name;

}


int quit(char *pathname)
{
	int i;
	char str[256];
	//check to see if there are any files open
	for(i = 0; i < 10; i++)
	{
		if(running->fd[i] != NULL)
		{
			//snprintf(str, 10, "%d", i);
			//close(str);
		}
	}

	//check all the minodes, if they are in use, and have been modified, write back to the disk and quit
	for(i = 0; i < NMINODES; i++)
	{
		if(minode[i].refCount > 0)
		{
			if(minode[i].dirty != 0)
			{
				minode[i].refCount = 1;
				iput(dev, &minode[i]);
			}
		}
	}
	printf("Exiting program.\n");
	exit(0);
}





int mylink(char *pathname){
}

int mysymlink(char *pathname){
}

int myunlink(char *pathname){
}

int mychmod(char *pathname){
}
