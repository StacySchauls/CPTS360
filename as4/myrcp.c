#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

//globals
char f1_type;   //f1 type
char f2_type;   //f2 type
char rand_file_type;
char cwd[256];                          //cwd
struct stat fileStat1, fileStat2, randFileStat;   //fileStats
char buf[128];
//prototype
int myrcp(char *f1, char *f2);
char file_type_check(char *file);
int cpf2f(char *f1, char *f2);
int cpf2d(char *f1, char *d1);
int cpd2d(char *d1, char *d2);
int sameFileCheck(char *f1, char *f2);

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("Error: Not enough arguments.\n");
		exit(0);
	}
	return myrcp(argv[1], argv[2]);
}


int myrcp(char *f1, char *f2){
	//get the cwd


	//check the file types

	f1_type = file_type_check(f1);
	printf("f1 type is %c\n",f1_type);
	if(f1_type != 'r' && f1_type != 'l'){
		//printf("File1 Not REG or LNK\n");
		//return 0;
	}

	f2_type = file_type_check(f2);
	
	if(f2_type != 'r' && f2_type != 'l'){
		//printf("File2 Not REG or LNK\n");
		//return 0;

	}

	if(f1_type == 'r'){
		if(f2_type == 'u' || f2_type == 'r')
			return cpf2f(f1,f2);
		else
			if(f2_type == 'd')
				return cpf2d(f1,f2);
	}

	if(f1_type == 'd'){
		if(f2_type != 'u' && f2_type != 'd')
			return -1;
		if(f2_type == 'u'){
			getcwd(cwd, 256);
			strcat(cwd, "/");
			strcat(cwd, f2);
			mkdir(cwd, S_IRUSR | S_IWUSR | S_IXUSR);
		}

		return cpd2d(f1, f2);
	}

	if(f1_type == 'l' && f2_type != 'u'){
		printf("file1 is lnk and file 2 exists. reject!\n");
		return -1;
	}else if(f1_type == 'l' && f2_type == 'u'){
		return cpf2f(f1,f2);
	}

}



char file_type_check(char *file){
	struct stat fileStat;

	//if its file
	printf("file: %s\n", file);
	//  printf("file1\n");
	if(lstat(file, &fileStat) == -1){
		//printf("Error in here.");
		perror("Stat");
		//exit(EXIT_FAILURE);
		return 'u';
	}

	//use stat to check the types of files        change file type char
	switch (fileStat.st_mode & S_IFMT) {
		case S_IFBLK:  	return 'b';       break;
		case S_IFCHR:	return 'c';       break;
		case S_IFDIR:  	return 'd';       break;
		case S_IFIFO:  	return 'f';       break;
		case S_IFLNK:  	return 'l';       break;
		case S_IFREG:  	return 'r';       break;
		case S_IFSOCK:	return 's';       break;
		default:		return 'u';       break;
	}

}

int cpf2f(char *f1, char * f2){
	//check that the files are not the same
	int in,out,wr, ln;
	char cwd[128];
	long int fileread;
	printf("cpf2f: f1 : %s\t f2 : %s\n",f1,f2);
	f1_type = file_type_check(f1);
	printf("f1 type is: %c\n",f1_type);
	f2_type = file_type_check(f2);
	printf("f2 type is: %c\n",f2_type);
	if(sameFileCheck(f1,f2)){
		return -1;
	}

	if(f1_type == 'l' && f2_type != 'u'){
		printf("f1 is LNK and f2 exists: reject!");
		return -1;
	}



	if(f1_type == 'l' && f2_type == 'u'){
		printf("F1 is LNK, f2 doesnt exist. Creat LNK file f2 same as f1\n");
		getcwd(cwd, 128);
		if((ln = symlink(f1,f2)) == 0){
			printf("link success\n");
			return 0;
		}else{
			printf("error creating link\n");
		}

	}

	in = open(f1, O_RDONLY);
	out = open(f2, O_WRONLY|O_CREAT|O_TRUNC, fileStat1.st_mode);

	while( (fileread = read(in, buf, sizeof(buf))) > 0  ){

		if(write(out,buf,fileread) != fileread){
			printf("writing error\n");
			exit(-1);
		}
	}

	printf("write exited with: %d\n", wr);
	close(wr);
	close(in);

	return 0;
}

int cpf2d(char *f1, char *d1){
	char *path, bname[128], dname[128], *x, slash[128];
	int acc, out;
	struct dirent *entry;
	DIR *dir;
	memset(slash, 0, sizeof(slash));

	//printf("d1: %s\n",d1);
	//path = d1;
	//strcpy(bname, basename(path));
	//path = d1;
	//strcpy(dname,dirname(path));
	//printf("Basame: %s\nDirname: %s\n", bname,dname);
	printf("d1: %s\n",d1);
	if((dir = opendir(d1)) == NULL){
		printf("Cannot open %s\n",d1);
		exit(1);
	}

	x = basename(f1);
	//slash[0] = '/';
	//printf("d1: %s\n", d1);
	strcat(slash, d1);
	strcat(slash, "/");
	strcat(slash,x);
	//printf("pthnam: %s\n",slash);

	acc = access(slash,F_OK);
	if(!acc){
		// printf("file exists\n");
		rand_file_type = file_type_check(slash);
		if(rand_file_type == 'r')
			cpf2f(f1,slash);
		else if(rand_file_type == 'd')
			cpf2d(f1, slash);
		//file_type_check(slash, &rand_file_type, 2);
	}else{
		//printf("File doesnt exist\n");
		//printf("cpf2f : %s  :  %s\n",f1, slash);
		out = creat(slash, 0644);
		//printf("creating file: %s\n",slash);	
		cpf2f(f1, slash);
	}


	return 0;
}

int cpd2d(char *d1, char *d2){
	struct dirent *entry;
	DIR *dir;
	char pathTocopyTo[256], localCwd[256];
	memset(cwd, 0, sizeof(cwd));
	memset(pathTocopyTo, 0, sizeof(cwd));

	if(!strcmp(d1, d2)){
		printf("Cannot copy directory, \'%s\' into itself, \'%s/%s\n",d1,d1,d1);
		exit(1);
	}
	printf("d1: %s\n",d2);
	printf("d2: %s\n",d1);
	if((dir = opendir(d1)) == NULL){
		perror("Cannot open\n");
		exit(1);
	}

	getcwd(cwd, 256);
	printf("CWD %s\n",cwd);
	/*	strcat(cwd, "/");
		strcat(cwd, d2);
		strcat(cwd, "/");
		strcat(cwd, d1);
		*/
	//strcat(d1,"/");
	//	strcat(d1,d2);
	printf("making dir: %s\n",d2);
	mkdir(d2, S_IRUSR | S_IWUSR | S_IXUSR);

	while((entry = readdir(dir)) != NULL){
		memset(cwd, 0, sizeof(cwd));
		memset(pathTocopyTo, 0, sizeof(cwd));
		//printf("entry name: %s\n", entry->d_name);
		switch(entry->d_type){
			case DT_REG: printf("is reg\n");
						 getcwd(cwd,256);
						 strcat(cwd, "/");
						 strcat(cwd, d2);
						 // strcat(cwd, "/");

						 printf("copying %s to %s\n",entry->d_name, cwd);

						 //printf("CWD: %s\n",cwd);
						 cpf2d(entry->d_name, cwd);
						 break;
			case DT_DIR:
						 if(!strcmp(entry->d_name, "..") || !(strcmp(entry->d_name, "." ))){
							 break;
						 }
						 printf("%s is dir \n", entry->d_name);
						 getcwd(localCwd,256);
						 strcat(localCwd, "/");
						 strcat(localCwd, d2);
						 strcat(localCwd, "/");
						 strcat(localCwd, entry->d_name);
						 printf("cwd is : %s\n",localCwd);

						 getcwd(pathTocopyTo, 256);
						 strcat(pathTocopyTo,"/");
						 strcat(pathTocopyTo,d1);
						 strcat(pathTocopyTo,"/");
						 strcat(pathTocopyTo,entry->d_name);
						 printf("cpd2d %s and %s\n",pathTocopyTo, localCwd);
						 cpd2d(pathTocopyTo, localCwd);
						 break;

			case DT_LNK: printf("is lnk\n");
						 printf("Creating a link\n");
						 getcwd(cwd,256);
						 strcat(cwd, "/");
						 strcat(cwd, d2);
						 strcat(cwd, "/");
						 strcat(cwd,entry->d_name);
						 printf("Creat link at: %s\n",cwd);
						 getcwd(pathTocopyTo,256);
						 strcat(pathTocopyTo, "/");
						 strcat(pathTocopyTo, d1);
						 strcat(pathTocopyTo, "/");
						 strcat(pathTocopyTo, entry->d_name);
						 cpf2f(pathTocopyTo, cwd);
						 break;
			default : printf("Doesnt exists.Create it.\n");
					  strcat(cwd,entry->d_name);
					  creat(cwd,0755);
					  break;
		}

	}

	return 0;
}

int sameFileCheck(char *f1, char *f2){
	if(lstat(f1, &fileStat1) == -1){
		//printf("Error stating file %s\n",f1);
		return 0;
	}
	if(lstat(f2, &fileStat2) == -1){
		//printf("Error stating file %s\n",f2);
		return 0;
	}

	if((fileStat1.st_dev == fileStat2.st_dev) && (fileStat1.st_ino == fileStat2.st_ino)){
		printf("Same file. reject!\n");
		return 1;
	}
	return 0;

}
