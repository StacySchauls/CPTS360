#include "type.h"
#include "util.h"

int main(int argc, char *argv[]){
  int i, cmd;
  char line[128], cname[64], parameter[64];
  if(argc == 2)
  {
    if(strcmp(argv[2], "-d") == 0) {DEBUG = 1;}
  }
  init();
  mount_root(argv[1]);
  printf("Dev=%d\tinodeBegin=%d\tbmap=%d\timap=%d\tninodes=%d\n", dev, inodeBegin, bmap, imap, ninodes);
  menu();
  while(1){
    memset(pathname, 0, 128);
    memset(parameter, 0, 64);
    memset(cname,0,64);
    printf("P%d running:\n", running->pid);
    printf("Command_> ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;
    if(line[0] == 0) {continue;}

    sscanf(line, "%s %s %s", cname, pathname, parameter);

    if(DEBUG) { printf("cname: %s, pathname:%s, param:%s\n",cname, pathname, parameter); }

    if(parameter[0] != 0){
      strcat(pathname, " ");
      strcat(pathname, parameter);
    }

    //use function pointer to look for command in the list of commands we have.
    for(i = 0; i < 10; i++){
      if(strcmp(cname, cmnds[i]) == 0){
        fptr[i](pathname);
        continue;
      }
    }

  }
  return 0;
}
