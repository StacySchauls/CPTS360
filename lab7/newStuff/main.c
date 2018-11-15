#include "type.h"

char command[256], line[256], pathname[256];
int cmdNum;
extern char *disk;
int fd;
extern int dev;
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

int main(int argc, char *argv[]){
  //look for debug flag
  if(argc > 2){
    if(strcmp(argv[2], "-d") == 0){
      DEBUG = 1;
    }else{
      DEBUG = 0;
    }
  }
  if(argc > 1){
    disk = argv[1];
  }


  int ino;
  char buf[BLKSIZE];
  if (argc > 1)
     disk = argv[1];

  fd = open(disk, O_RDWR);
  if (fd < 0){
     printf("open %s failed\n", disk);
     exit(1);
  }
  dev = fd;
  if(DEBUG){
    printf("dev is: %d\n",dev);
  }

  init();
  if(DEBUG){
    printf("after init dev is: %d\n",dev);
  }
  mount_root();
  if(DEBUG){
    printf("after mount dev is: %d\n",dev);
  }

  if(DEBUG){
    printf("root ref count = %d\n",root->refCount);
    printf("creating p0 as running process\n");
}
    running = &proc[0];
   // running->status = READY;
    running->cwd = iget(dev,2);
    proc[1].cwd = iget(dev,2);

  while(1){
    memset(line, 0, sizeof(line)); //reset the line command
    memset(command, 0, sizeof(command));
    memset(pathname,0,sizeof(pathname));
    printf("Command_>");
    fgets(line, 128,stdin);
    line[strlen(line)-1] = 0;
    sscanf(line, "%s %s", command, pathname);
    cmdNum = findCmd(command);

    if(DEBUG)
      printf("command: %s\tpathname: %s\n",command, pathname);
    switch(cmdNum){
      case 3:
        if(DEBUG){
          printf(RED"in ls\n"RESET);
        }
       // ls2(pathname);
        break;

      case 1:
        if(DEBUG)
          printf(RED"in cd\n"RESET);
        printf("dev is: %d\n",dev);
        cdMe(pathname);
        break;

      case 2:
        if(DEBUG)
          printf(RED"in pwd\n"RESET);
        pwd();
        break;

      case 29:
        if(DEBUG)
          printf(RED"in quit\n"RESET);
        quit();
        break;
      default:
        printf("Error. Not a usable command.\n");
        break;

    }

  }



  return 0;
}
