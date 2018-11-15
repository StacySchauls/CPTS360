#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>


//globals
char f1_type;   //f1 type
char f2_type;   //f2 type
char rand_file_type;
char cwd[256];                          //cwd
struct stat fileStat1, fileStat2, randFileStat;   //fileStats
char buf[128];
extern int errno;
//prototype
int myrcp(char *f1, char *f2);
char file_type_check(char *file);
int cpf2f(char *f1, char *f2);
int cpf2d(char *f1, char *d1);
int cpd2d(char *d1, char *d2);
int sameFileCheck(char *f1, char *f2);
int subDirCheck(char *f1, char *f2);



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
	//printf("file: %s\n", file);
	//  printf("file1\n");
	if(lstat(file, &fileStat) == -1){
		//printf("Error in here.");
	//	perror("Stat");
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
	//printf("cpf2f: f1 : %s\t f2 : %s\n",f1,f2);
	f1_type = file_type_check(f1);
	//printf("f1 type is: %c\n",f1_type);
	f2_type = file_type_check(f2);
	//	printf("f2 type is: %c\n",f2_type);
	if(sameFileCheck(f1,f2)){
		return -1;
	}

	if(f1_type == 'l' && f2_type != 'u'){
		printf("f1 is LNK and f2 exists: reject!\n");
		return -1;
	}



	if(f1_type == 'l' && f2_type == 'u'){
	//	printf("F1 is LNK, f2 doesnt exist. Creat LNK file f2 same as f1\n");
		//	getcwd(cwd, 128);
		//	strcat(cwd, "/");
		//	strcat(cwd, f2);
		//	printf("attempting create link from %s to %s \n",f1, cwd);
		if((ln = symlink(f1,f2)) == 0){
			//printf("link success\n");
			return 0;
		}else{
			//printf("value of errno: %d\n",errno);

			printf("Error: %s\n",strerror(errno));
			//	printf("error creating link\n");
			exit(1);
		}

	}

	in = open(f1, O_RDONLY);
	out = open(f2, O_WRONLY|O_CREAT|O_TRUNC, fileStat1.st_mode);

	while( (fileread = read(in, buf, sizeof(buf))) > 0  ){

		if(write(out,buf,fileread) != fileread){
			//printf("writing error\n");
			exit(-1);
		}
	}

	//	printf("write exited with: %d\n", wr);
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

	if((dir = opendir(d1)) == NULL){
		printf("Cannot open %s\n",d1);
		exit(1);
	}

	x = basename(f1);
	strcat(slash, d1);
	strcat(slash, "/");
	strcat(slash,x);

	acc = access(slash,F_OK);
	if(!acc){
		// printf("file exists\n");
		rand_file_type = file_type_check(slash);
		if(rand_file_type == 'r')
			cpf2f(f1,slash);
		else if(rand_file_type == 'd')
			cpf2d(f1, slash);
	}else{
		out = creat(slash, 0644);
		cpf2f(f1, slash);
	}


	return 0;
}

int cpd2d(char *d1, char *d2){
	int i = 0, flag =1;
	int j = 0;
	struct dirent *entry;
	DIR *dir;
	char pathTocopyTo[256], localCwd[256];
	char dest[256], bname[128],dname[128];
	char * dummy, dummyd1[128];
	char dummyd2[128];

	memset(bname,0,sizeof(bname));
	memset(dname,0,sizeof(dname));
	memset(dest,0,sizeof(dest));
	memset(cwd, 0, sizeof(cwd));
	memset(pathTocopyTo, 0, sizeof(cwd));
	//:w
	//memset(dummyd2,0,sizeof(dummyd2));
	//	memset(dummyd1,0,sizeof(dummyd1));
	if(!strcmp(d1, d2)){
		printf("Cannot copy directory, \'%s\' into itself, \'%s/%s\n",d1,d1,d1);
		exit(1);
	}
//printf("subdir check\n");
	if(!subDirCheck(d1,d2)){
		printf("cannot copy dir into its own subdir\n");
		exit(1);
	}


	//printf("from: %s\n",d1);
	//printf("to: %s\n",d2);
	dummy=d2;
	if((dir = opendir(d1)) == NULL){
		perror("Cannot open\n");
		exit(1);
	}
	//printf("making dir: %s\n",d2);
	if(mkdir(d2, 0755)){
		 flag = 0;
		//printf("error %s\n",strerror(errno));
		memcpy(dummyd2, d2,sizeof(d2));
		strcat(dummyd2,"/");
		strcat(dummyd2, d1);
	//	printf("%s \n", dummyd2);
	//	mkdir(dummyd2,0755);
		memset(dummyd1,0,sizeof(d2));
		memcpy(dummyd1,dummyd2,sizeof(dummyd2));
	}
	//printf("made dir: %s\n",d2);
	while((entry = readdir(dir)) != NULL){

		memset(dest,0,sizeof(dest));
		if((strcmp(entry->d_name, "..")) == 0 || (strcmp(entry->d_name, "." ) == 0)){
			//printf("in the if. should break");
			continue;
		}
		getcwd(cwd,256);
		//printf("cwd:: %s\n",cwd);
		memset(cwd, 0, sizeof(cwd));
		if(flag){
		memset(dummyd1, 0, sizeof(dummyd1));
		memset(dummyd2, 0, sizeof(dummyd2));
		}
		memset(pathTocopyTo, 0, sizeof(cwd));
		//	printf("entry name: %s\n", entry->d_name);
		switch(entry->d_type){
			case DT_REG: //printf("%s is reg\n", entry->d_name);
				memcpy(dest, d2, sizeof(d2));
				strcpy(dest, d2);
				memcpy(dummyd1,d1,sizeof(d1));
				// printf("d2 in here: %s\n",d2);
				strcat(dummyd1, "/");
				strcat(dummyd1, entry->d_name);
				// printf("d1 in here: %s\n",d1);

				//getcwd(cwd, 256);
				//strcat(cwd, "/");
				// printf("cwd: %s\n",cwd);
				memcpy(dummyd2, d2, sizeof(d2));
				//strcat(dummyd2, "/");
				// strcat(dummyd2, entry->d_name);
			//	printf("copying %s to %s\n",dummyd1, dummyd2 );
				// printf("d2 in here: %s\n",d2);
				cpf2d(dummyd1, dummyd2);
				break;
			case DT_DIR:

				memcpy(dest, d2,sizeof(d2));
				memcpy(dummyd1,d1,sizeof(d1));
				// printf("%s is dir \n", entry->d_name);
				strcat(dummyd1,"/");
				strcat(dummyd1, entry->d_name);

				strcat(dest, "/");
				strcat(dest, entry->d_name);
				//printf("cpd2d %s and %s\n",dummyd1, dest);
				i++;
				cpd2d(dummyd1, dest);

				break;

			case DT_LNK:// printf("%s is lnk\n",entry->d_name);
				memcpy(dummyd1,d1,sizeof(d1));
				strcat(dummyd1,"/");
				strcat(dummyd1, entry->d_name);

				memcpy(dummyd2,d2,sizeof(d2));
				strcat(dummyd2,"/");
				strcat(dummyd2, entry->d_name);
				getcwd(cwd, sizeof(cwd));
				strcat(cwd, "/");
				strcat(cwd, dummyd1);

				getcwd(pathTocopyTo,sizeof(pathTocopyTo));
				strcat(pathTocopyTo,"/");
				strcat(pathTocopyTo,dummyd2);
			//	printf("cpf2f %s and %s\n",cwd, pathTocopyTo);
				cpf2f (cwd, pathTocopyTo);
				break;
			default :// printf("Doesnt exists.Create it.\n");
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


int subDirCheck(char *d1, char *d2){
	DIR *dir;
	char dummyd2[256],dummy[256];
	struct dirent *entry;
	memset(dummyd2,0,sizeof(dummyd2));

	memset(dummy,0,sizeof(dummyd2));

	memcpy(dummyd2,d2,sizeof(d2));
	memcpy(dummy,d2,sizeof(d2));
	//printf("d1 is: %s\t d2 is %s\n",d1,d2);
	if(strcmp(d1,d2) == 0){
		//	printf("cant copy dir into subdir\n");
		return 0;
	}else if(strcmp(d2, ".")==0){
		//printf("d2 is .\n");
		return 1;
	}else{
		memcpy(dummyd2,dirname(dummy),sizeof(d2));

		//printf("returning sub dir of  %s\t %s\n",d1,dummyd2);

		return subDirCheck(d1, dummyd2);



	}
}
